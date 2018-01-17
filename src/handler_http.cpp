#include "handler_http.h"
#include "cache_conn.h"
#include "im_log.h"
#include "net_server.h"
#include "config_file_oper.h"
#include "handler_cache.h"
#include "message.h"
#include "../protobuf/IM.BaseDefine.pb.h"
#include "../protobuf/IM.DBProxy.pb.h"
#include "handler_cache.h"
using namespace db_proxy;

using namespace std;

#define ISVALIDFILED_STR(FILED) if(body[FILED].isNull()){break;}if(!body[FILED].isString()){break;}
#define ISVALIDFILED_INT(FILED) if(body[FILED].isNull()){break;}if(!body[FILED].isInt()){break;}
#define ISVALIDFILED_UINT(FILED) if(body[FILED].isNull()){break;}if(!body[FILED].isUInt()){break;}

//返回http
static void http_req_refresh_company_org(uint32_t company_id, MYSQL *mysql, cache_conn *redis);
//返回http
static void http_rsp(evutil_socket_t &fd, const string &prototype, const string &msgid, const string &result);
static void http_rsp(evutil_socket_t &fd, const string &prototype, const string &msgid, const string &result, Value &body);

//获取分表名
static string get_sub_table_name(string tablename, uint32_t value, uint32_t count = 8);

//生成部门节点架构的json结构
static void org_frame_json(pre_statement &statement,  int32_t &node_id, int32_t &lft, int32_t &rgh,  char* nodename, int32_t &sort, bool &is_valid, int32_t parent_lft, int32_t parent_rgh, int32_t parent_node_id, Value &body);

//生成公司架构(包括用户信息在内)
static bool make_org_json(uint32_t company_id, Value &root, MYSQL *mysql, int ntype = 1);

//生成部门节点+用户的json结构
static void org_frame_user_json(pre_statement &statement, int32_t &lft, int32_t &rgh, int32_t &node_id,int32_t &nodetype, char *nodename, char *title,
        int32_t &userid, char *email,int32_t &sex, char *name, char *mobile, int32_t &sort,  bool &is_valid, 
        int32_t parent_lft, int32_t parent_rgh, int32_t parent_node_id, Value &body, int ntype = 1);

//生成节点所有用户成员
static bool make_all_node_member_json(uint32_t company_id, uint32_t node_id, Value &root, MYSQL *mysql, int ntype = 1);

//生成部门内所有成员并带部门信息的json结构
static void node_list_user_json(pre_statement &statement, int32_t &lft, int32_t &rgh, int32_t &node_id, int32_t &nodetype, char *nodename, char *title,
	int32_t &userid, char *email, int32_t &sex, char *name, char *mobile, int32_t &sort, bool &is_valid,
	int32_t parent_lft, int32_t parent_rgh, int32_t parent_node_id, Value &body, int ntype = 1);


//为用户添加职业
static int user_add_title(uint32_t company_id, uint32_t user_id, uint32_t parent_id, string title, MYSQL *mysql);

//删除节点
static int del_node(uint32_t company_id, uint32_t node_id, MYSQL *mysql);

//移动节点
static int mov_node(uint32_t company_id, uint32_t node_id, uint32_t to_parent_node_id, MYSQL *mysql);

static void org_update_notify_req(uint32_t company_id, uint32_t user_id = 0, uint32_t user_opt_type = 0);

static void org_refresh_redis(cache_conn *redis, uint32_t company_id);


void http_login(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        
        ISVALIDFILED_STR("company_name");
        ISVALIDFILED_STR("admin_account");
        ISVALIDFILED_STR("password");

        string companyname = body["company_name"].asString();
        string adminaccount = body["admin_account"].asString();
        string password = body["password"].asString();


        if(NULL == mysql)
        {
            break;
        }
        
        string sqlstr = "select `company_id` from `t_ids_company` where `company_name`=? and \
                              `admin_name` = ? and `admin_pwd` = ? limit 1";
        pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement.set_param_bind(0, companyname);
        statement.set_param_bind(1, adminaccount);
        statement.set_param_bind(2, password);
        
        if (0 != statement.query())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }

        int64_t company_id = 0;
        statement.set_param_result(0, MYSQL_TYPE_LONGLONG, (char*)(&company_id), sizeof(company_id), NULL);
        if (0 != statement.get_result())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        if(0 != statement.fetch_result())
        {
            break;
        }
        if(company_id <= 0)
        {
            break;
        }
        result = "success";
        rspbody["company_id"] = company_id;
    }while(false);
    
    
    http_rsp(fd, "admin_login_rsp", msgid, result, rspbody);
}

void http_get_company_list(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        
        ISVALIDFILED_UINT("index_begin");
        ISVALIDFILED_UINT("index_count");

        uint32_t index_begin = body["index_begin"].asUInt();
        uint32_t index_count = body["index_count"].asUInt();
        if(index_count > 200)
        {
            index_count = 200;
        }
        
        rspbody["index_begin"] = index_begin;

        
        if(NULL == mysql)
        {
            break;
        }

        string sqlstr = "select `company_id`,`company_name`,`address` from `t_ids_company`  order by `company_id` desc limit ?,?";

        pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        index_count++;
        statement.set_param_bind(0, index_begin);
        statement.set_param_bind(1, index_count);
        
        if (0 != statement.query())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }

        uint32_t company_id = 0;
        char     company_name[200] = {'\0'};
        char     address[200] = {'\0'};
        statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&company_id), sizeof(company_id), NULL);
        statement.set_param_result(1, MYSQL_TYPE_STRING, company_name, sizeof(company_name) - 1, NULL);
        statement.set_param_result(2, MYSQL_TYPE_STRING, address, sizeof(address) - 1, NULL);

        if (0 != statement.get_result())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        bool ismore = false;
        index_count--;
        if(statement.get_num_rows() > index_count)
        {
            ismore = true;
        }
                        
        Value body;
        uint32_t index = 0;
        while (0 == statement.fetch_result() && index < index_count)
        {
            Value nodebody;
            nodebody["company_id"] = company_id;
            nodebody["name"] = company_name;
            nodebody["address"] = address;
            body.append(nodebody);
            index++;
        }

        result = "success";
        rspbody["index_count"] = index;
        rspbody["is_more"] = ismore;
        rspbody["company_list"] = body;
    }while(false);
    
    
    http_rsp(fd, "query_company_list_rsp", msgid, result, rspbody);
}

void http_get_company_user_list(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_UINT("index_begin");
        ISVALIDFILED_UINT("index_count");

        uint32_t company_id = body["company_id"].asUInt();
        uint32_t index_begin = body["index_begin"].asUInt();
        uint32_t index_count = body["index_count"].asUInt();
        if(index_count > 200)
        {
            index_count = 200;
        }
        
        rspbody["index_begin"] = index_begin;

        
        if(NULL == mysql)
        {
            break;
        }

        string sqlstr = string("select `user_id`,`name`,`sex`,`mobile`,`status` from `")+
            get_sub_table_name("t_im_user_vcard",company_id)+string("` where `company_id`=? order by `user_id` asc limit ?,?");

        pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        index_count++;
        statement.set_param_bind(0, company_id);
        statement.set_param_bind(1, index_begin);
        statement.set_param_bind(2, index_count);
        
        if (0 != statement.query())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }

        uint32_t user_id = 0;
        uint32_t sex = 0;
        uint32_t status = 0;
        char     name[200] = {'\0'};
        char     mobile[32] = {'\0'};
        statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&user_id), sizeof(user_id), NULL);
        statement.set_param_result(1, MYSQL_TYPE_STRING, name, sizeof(name) - 1, NULL);
        statement.set_param_result(2, MYSQL_TYPE_LONG, (char*)(&sex), sizeof(sex), NULL);
        statement.set_param_result(3, MYSQL_TYPE_STRING, mobile, sizeof(mobile) - 1, NULL);
        statement.set_param_result(4, MYSQL_TYPE_LONG, (char*)(&status), sizeof(status), NULL);

        if (0 != statement.get_result())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        bool ismore = false;
        index_count--;
        if(statement.get_num_rows() > index_count)
        {
            ismore = true;
        }
                        
        Value body;
        uint32_t index = 0;
        while (0 == statement.fetch_result() && index < index_count)
        {
            Value nodebody;
            nodebody["user_id"] = user_id;
            nodebody["name"] = name;
            nodebody["sex"] = sex;
            nodebody["mobile"] = mobile;
            nodebody["status"] = status;
            body.append(nodebody);
            index++;
        }

        result = "success";
        rspbody["index_count"] = index;
        rspbody["is_more"] = ismore;
        rspbody["user_list"] = body;
    }while(false);
    
    
    http_rsp(fd, "query_company_user_list_rsp", msgid, result, rspbody);
}


void http_get_company_organization(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    log(LOG_DEBUG, "http_get_company_organization  fd=%u", fd);
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        
        ISVALIDFILED_UINT("company_id");
        uint32_t company_id = body["company_id"].asUInt();
        rspbody["company_id"] = company_id;

        
        if(NULL == mysql)
        {
            break;
        }
        Value root;
        bool rtorg = make_org_json(company_id, root, mysql, 2);
        if(!rtorg)
        {
            break;
        }

        result = "success";
        rspbody["data"] = root;
    }while(false);
    
    
    http_rsp(fd, "get_company_organization_rsp", msgid, result, rspbody);
}

void http_get_organization(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_UINT("node_id");

        uint32_t company_id = body["company_id"].asUInt();
        uint32_t node_id = body["node_id"].asUInt();
        
        rspbody["company_id"] = company_id;

        
        if(NULL == mysql)
        {
            break;
        }

        string tbOrg = get_sub_table_name("t_im_org", company_id);
        string sqlstr = string(" select `org`.`node_id`, `org`.`lft`, `org`.`rgh`, `org`.`node_name`,`org`.`sort` ") + 
                             string(" from `") + tbOrg + string("` as `org`, ") +
                             string(" (select `lft`,`rgh` from `") + tbOrg + string("` where `company_id`= ? and `node_id` = ?) as `ndx` ") + 
                             string(" where `org`.`company_id`=? and  \
                                                 `ndx`.`lft` <= `org`.`lft` and \
                                                 `org`.`rgh` <= `ndx`.`rgh` and \
                                                 `org`.`node_type` = 0 \
                                           order by `org`.`lft` asc");
        pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement.set_param_bind(0, company_id);
        statement.set_param_bind(1, node_id);
        statement.set_param_bind(2, company_id);
	
        
        if (0 != statement.query())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }

        int32_t retnodeid = 0;
        int32_t lft = 0;
        int32_t rgh = 0;
        char    nodename[100] = {'\0'};
	int32_t sort = 0;
        statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&retnodeid), sizeof(retnodeid), NULL);
        statement.set_param_result(1, MYSQL_TYPE_LONG, (char*)(&lft), sizeof(lft), NULL);
        statement.set_param_result(2, MYSQL_TYPE_LONG, (char*)(&rgh), sizeof(rgh), NULL);
        statement.set_param_result(3, MYSQL_TYPE_STRING, nodename, sizeof(nodename) - 1, NULL);
	statement.set_param_result(4, MYSQL_TYPE_LONG, (char*)(&sort), sizeof(sort), NULL);
        if (0 != statement.get_result())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
                        
        Value body;
        if (0 == statement.fetch_result())
        {
            bool is_valid = true;
            org_frame_json(statement,  retnodeid, lft, rgh, nodename, sort, is_valid, lft, rgh, node_id, body);
        }
        
        result = "success";
        rspbody["data"] = body;
    }while(false);
    
    
    http_rsp(fd, "get_organization_rsp", msgid, result, rspbody);
}

void org_frame_json(pre_statement &statement,  int32_t &node_id, int32_t &lft, int32_t &rgh,  char* nodename, int32_t &sort, bool &is_valid, int32_t parent_lft, int32_t parent_rgh, int32_t parent_node_id, Value &body)
{
    if(lft == parent_lft && rgh == parent_rgh)
    {
        //根节点
        Value nodebody;
        if(node_id != parent_node_id)
        {
            nodebody["parent_id"] = parent_node_id;
        }
        nodebody["node_id"] = node_id;
        nodebody["name"] = nodename;
        nodebody["type"] = 0;
	nodebody["sort"] = sort;
        
        parent_node_id = node_id;
        if(0 == statement.fetch_result());
        {
            Value tpbody;
            is_valid = true;
            org_frame_json(statement,  node_id, lft, rgh, nodename, sort, is_valid, parent_lft, parent_rgh, parent_node_id, tpbody);
            nodebody["child"] = tpbody;
        }
        body.append(nodebody);
        return;
    }
    //子节点
    int32_t prelft = parent_lft;
    int32_t prergh = parent_rgh;
    int32_t prenodeid = parent_node_id;
    Value *newnodebody = NULL;
    do
    {
        if(!is_valid)
        {
            if(0 == statement.fetch_result())
            {
                is_valid = true;
            }
            else
            {
                is_valid = false;
                break;
            }
        }
        if(parent_rgh < lft)
        {
            return;
        }
        if((prelft == parent_lft && prergh == parent_rgh) || prergh < lft)
        {
            Value tpbody;
            tpbody["parent_id"] = parent_node_id;
            tpbody["node_id"] = node_id;
            tpbody["name"] = nodename;
            tpbody["type"] = 0;
	    tpbody["sort"] = sort;
            Value &tpnewnodebody = body.append(tpbody);
            newnodebody = &tpnewnodebody;

            prelft = lft;
            prergh = rgh;
            prenodeid = node_id;
            is_valid = false;
        }
        else
        {
            Value nodebody;
            org_frame_json(statement, node_id, lft, rgh, nodename, sort, is_valid, prelft, prergh, prenodeid, nodebody);
            (*newnodebody)["child"] = nodebody;
        }
    }while(true);
}


void http_get_all_node_member(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
	log(LOG_DEBUG, "http_get_all_node_member  fd=%u", fd);
	string result;
	Value rspbody;

	result = "fail";

	do
	{
		if (body.isNull())
		{
			break;
		}

		ISVALIDFILED_UINT("company_id");
		ISVALIDFILED_UINT("node_id");
		uint32_t company_id = body["company_id"].asUInt();
		uint32_t node_id = body["node_id"].asUInt();
		rspbody["company_id"] = company_id;
		rspbody["node_id"] = node_id;


		if (NULL == mysql)
		{
			break;
		}
		Value root;
		bool rtorg = make_all_node_member_json(company_id, node_id, root, mysql, 2);
		if (!rtorg)
		{
			break;
		}

		result = "success";
		rspbody["data"] = root;
	} while (false);


	http_rsp(fd, "get_all_node_member_rsp", msgid, result, rspbody);
}

void http_get_all_member(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
	string result;
	Value rspbody;

	result = "fail";

	do
	{
		if (body.isNull())
		{
			break;
		}

		ISVALIDFILED_UINT("company_id");
		ISVALIDFILED_UINT("node_id");
		uint32_t company_id = body["company_id"].asUInt();
		uint32_t node_id = body["node_id"].asUInt();
		rspbody["company_id"] = company_id;
		rspbody["node_id"] = node_id;

		if (NULL == mysql)
		{
			break;
		}

		string sqlstr = string("select `user_id`,`user_name`,`sex`,`status` from `") +
			get_sub_table_name("t_im_user_vcard", company_id) + string("` where `company_id`=?");

		pre_statement statement;
		if (0 != statement.init(mysql, sqlstr))
		{
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			break;
		}
		statement.set_param_bind(0, company_id);

		if (0 != statement.query())
		{
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			break;
		}

		uint32_t user_id = 0;
		char     user_name[200] = { '\0' };
		uint32_t gender = 0;
		uint32_t status = 0;
		statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&user_id), sizeof(user_id), NULL);
		statement.set_param_result(1, MYSQL_TYPE_STRING, user_name, sizeof(user_name) - 1, NULL);
		statement.set_param_result(2, MYSQL_TYPE_LONG, (char*)(&gender), sizeof(gender), NULL);
		statement.set_param_result(3, MYSQL_TYPE_LONG, (char*)(&status), sizeof(status), NULL);

		if (0 != statement.get_result())
		{
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			break;
		}

		Value body;
		while (0 == statement.fetch_result())
		{
			Value nodebody;
			nodebody["user_id"] = user_id;
			nodebody["user_name"] = user_name;
			nodebody["gender"] = gender;
			nodebody["status"] = status;
			body.append(nodebody);
		}

		result = "success";
		rspbody["user_list"] = body;
	} while (false);


	http_rsp(fd, "get_all_member_rsp", msgid, result, rspbody);
}

void http_get_node_member(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_UINT("node_id");
        ISVALIDFILED_UINT("index_begin");
        ISVALIDFILED_UINT("index_count");

        uint32_t company_id = body["company_id"].asUInt();
        uint32_t node_id = body["node_id"].asUInt();
        uint32_t index_begin = body["index_begin"].asUInt();
        uint32_t index_count = body["index_count"].asUInt();
        if(index_count > 200)
        {
            index_count = 200;
        }
        
        rspbody["company_id"] = company_id;
        rspbody["node_id"] = node_id;
        rspbody["index_begin"] = index_begin;

        
        if(NULL == mysql)
        {
            break;
        }
/*
        SELECT node.title, (COUNT(parent.node_id) - (sub_tree.depth + 1)) AS depth
        FROM t_im_org_2 AS node,
             t_im_org_2 AS parent,
             (
                  SELECT node.node_id,node.lft,node.rgh,(COUNT(parent.node_id) - 1) AS depth
                  FROM t_im_org_2 AS node,
                  t_im_org_2 AS parent
                  WHERE node.lft BETWEEN parent.lft AND parent.rgh AND node.node_id = 3
                  GROUP BY node.node_id
             )AS sub_tree 
        WHERE node.`company_id` = 100010 AND node.node_type=1 
                     AND node.lft BETWEEN parent.lft AND parent.rgh
                     AND node.lft BETWEEN sub_tree.lft AND sub_tree.rgh
                     GROUP BY node.node_id
        HAVING depth = 1
        ORDER BY node.lft;
*/
        string tbOrg = get_sub_table_name("t_im_org", company_id);
        string tbUserVcard = get_sub_table_name("t_im_user_vcard", company_id);
        string sqlstr = 
            string(" select `node`.`node_id`,`node`.`user_id`,`node`.`title`,`tbuser`.`name`,`tbuser`.`sex`,`tbuser`.`mobile`,`tbuser`.`email` ") +
            string(" from ( SELECT `node`.`node_id`,`node`.`user_id`,`node`.`title`,`node`.`node_name`, `node`.`lft`,`node`.`rgh`, count(1) -1  as `depth` ") +
            string("        FROM `")+tbOrg+string("` AS node join `")+tbOrg+string("` AS `parent` on `node`.`lft` BETWEEN `parent`.`lft` and `parent`.`rgh` ") +
            string("        WHERE `node`.`company_id` = ? AND `node`.`node_type` = 1 ") +
            string("        GROUP BY `node`.node_id,`node`.`user_id`,`node`.`title`,`node`.`node_name`, `node`.`lft`,`node`.`rgh` ) AS `node`, ") +
            string("      ( SELECT `node`.`node_id`,`node`.`lft`,`node`.`rgh`, count(1) -1  as `depth` ") +
            string("        FROM `")+tbOrg+string("` AS `node` join `")+tbOrg+string("` AS `parent` on `node`.`lft` BETWEEN `parent`.`lft` and `parent`.`rgh` ")+
            string("        WHERE `node`.`company_id` = ? AND `node`.`node_id` = ? ") +
            string("        GROUP BY `node`.`node_id`,`node`.`lft`,`node`.`rgh`) AS `hvnd`, ") +
            string("      `") + tbUserVcard + string("` AS `tbuser` ") +
            string(" where  `node`.`lft` BETWEEN `hvnd`.`lft` AND `hvnd`.`rgh` AND `node`.`depth` = `hvnd`.`depth` + 1 AND  ") +
            string("        `tbuser`.`company_id` = ? AND `tbuser`.`user_id` = `node`.`user_id` ") +
            string(" order by `node`.`lft` ASC ") +
            string(" limit ?,?"); 

        //log(LOG_DEBUG, "get_node_member  sql:\n%s", sqlstr.c_str()); 
        pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        index_count++;
        statement.set_param_bind(0, company_id);
        statement.set_param_bind(1, company_id);
        statement.set_param_bind(2, node_id);
        statement.set_param_bind(3, company_id);
        statement.set_param_bind(4, index_begin);
        statement.set_param_bind(5, index_count);
        
        if (0 != statement.query())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }

        int32_t retnodeid = 0;
        int32_t retuserid = 0;
        char    rettitle[100] = {'\0'};
        char    retusername[100] = {'\0'};
        int32_t retsex = 0;
        char    retmobile[32] = {'\0'};
        char    retemail[100] = {'\0'};
        statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&retnodeid), sizeof(retnodeid), NULL);
        statement.set_param_result(1, MYSQL_TYPE_LONG, (char*)(&retuserid), sizeof(retuserid), NULL);
        statement.set_param_result(2, MYSQL_TYPE_STRING, rettitle, sizeof(rettitle) - 1, NULL);
        statement.set_param_result(3, MYSQL_TYPE_STRING, retusername, sizeof(retusername) - 1, NULL);
        statement.set_param_result(4, MYSQL_TYPE_TINY, (char*)(&retsex), sizeof(retsex), NULL);
        statement.set_param_result(5, MYSQL_TYPE_STRING, retmobile, sizeof(retmobile) - 1, NULL);
        statement.set_param_result(6, MYSQL_TYPE_STRING, retemail, sizeof(retemail) - 1, NULL);

        if (0 != statement.get_result())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        bool ismore = false;
        index_count--;
        if(statement.get_num_rows() > index_count)
        {
            ismore = true;
        }
                        
        Value body;
        uint32_t index = 0;
        while (0 == statement.fetch_result() && index < index_count)
        {
            Value nodebody;
            nodebody["node_id"] = retnodeid;
            nodebody["parent_id"] = node_id;
            nodebody["user_id"] = retuserid;
            nodebody["name"] = retusername;
            nodebody["gender"] = retsex;
            nodebody["email"] = retemail;
            nodebody["phone"] = retmobile;
            nodebody["title"] = rettitle;
            nodebody["type"] = 1;
            body.append(nodebody);
            index++;
        }
        
        result = "success";
        rspbody["index_count"] = index;
        rspbody["is_more"] = ismore;
        rspbody["data"] = body;
    }while(false);
    
    
    http_rsp(fd, "get_node_member_rsp", msgid, result, rspbody);
}

void http_get_user_info(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_UINT("user_id");

        uint32_t company_id = body["company_id"].asUInt();
        uint32_t user_id = body["user_id"].asUInt();
        
        rspbody["company_id"] = company_id;
        rspbody["user_id"] = user_id;

        
        if(NULL == mysql)
        {
            break;
        }

/*
    select `node`.`node_id`,`hvnd`.`node_id` as `parent_id`,`hvnd`.`node_name`,`node`.`title`,
           `tbuser`.`work_id`,`tbuser`.`name`,`tbuser`.`english_name`,`tbuser`.`nick_name`,`tbuser`.`birthday`,
           `tbuser`.`sex`,`tbuser`.`email`,`tbuser`.`mobile`,`tbuser`.`tel`,`tbuser`.`sign_info`,`tbuser`.`head_img`,
           `tbuser`.`head_update_time`,`tbuser`.`status`,`tbuser`.`update_time`
               from   ( SELECT node.`node_id`,`node`.`user_id`,`node`.`title`, node.`lft`,node.`rgh`, count(1)  as `depth`
                       FROM t_im_org_2 AS node left join t_im_org_2 AS parent on parent.`lft` <= node.`lft` and node.`rgh` <= parent.`rgh`
                       WHERE node.`company_id` = 100010 AND `node`.`user_id` = 100000 
                       GROUP BY node.node_id,node.`node_id`,`node`.`user_id`,`node`.`title`, node.`lft`,node.`rgh` ) AS `node`,
           ( SELECT node.`node_id`, node.`node_name`, node.`lft`,node.`rgh`, count(1)   as `depth`
             FROM t_im_org_2 AS  node left join t_im_org_2 AS parent on parent.`lft` <= node.`lft` and node.`rgh` <= parent.`rgh`
             WHERE node.`company_id` = 100010 
             GROUP BY node.node_id, node.`node_name`, node.`lft`,node.`rgh` ) AS `hvnd`,
           `t_im_user_vcard_2` AS `tbuser`
               where   `hvnd`.`lft` <`node`.`lft` AND `node`.`rgh` < `hvnd`.`rgh` and `node`.`depth` = `hvnd`.`depth` + 1 AND 
               `tbuser`.`company_id` = 100010 AND `tbuser`.`user_id` = 100000 
               order by `node`.`lft` ASC
               */
        string tbOrg = get_sub_table_name("t_im_org", company_id);
        string tbUserVcard = get_sub_table_name("t_im_user_vcard", company_id);
        string sqlstr =
            string(" select `node`.`node_id`,`hvnd`.`node_id` as `parent_id`,`hvnd`.`node_name`,`node`.`title`, ") + 
            string("        `tbuser`.`work_id`,`tbuser`.`name`,`tbuser`.`english_name`,`tbuser`.`nick_name`, ") + 
            string("        `tbuser`.`birthday`,`tbuser`.`sex`,`tbuser`.`email`,`tbuser`.`mobile`, ") + 
            string("        `tbuser`.`tel`,`tbuser`.`sign_info`,`tbuser`.`head_img`, ") + 
            string("        `tbuser`.`head_update_time`,`tbuser`.`status`,`tbuser`.`update_time` ") + 
            string(" from   ( SELECT `node`.`node_id`,`node`.`user_id`,`node`.`title`, `node`.`lft`,`node`.`rgh`, count(1)  as `depth` ")+
            string("          FROM `")+tbOrg+string("` AS `node` left join `")+tbOrg+string("` AS `parent` ") + 
            string("                ON `parent`.`lft` <= `node`.`lft` and `node`.`rgh` <= `parent`.`rgh` ")+
            string("          WHERE `node`.`company_id` = ? AND `node`.`user_id` = ? ")+ 
            string("          GROUP BY `node`.`node_id`,`node`.`node_id`,`node`.`user_id`,`node`.`title`, `node`.`lft`,`node`.`rgh` ) AS `node`, ")+
            string("        ( SELECT `node`.`node_id`, `node`.`node_name`, `node`.`lft`,`node`.`rgh`, count(1)   as `depth` ")+
            string("          FROM `")+tbOrg+string("` AS  `node` left join `")+tbOrg+string("` AS `parent` ") + 
            string("                ON `parent`.`lft` <= `node`.`lft` and `node`.`rgh` <= `parent`.`rgh` ")+
            string("          WHERE `node`.`company_id` = ?  ")+
            string("          GROUP BY `node`.`node_id`, `node`.`node_name`, `node`.`lft`,`node`.`rgh` ) AS `hvnd`, ")+
            string("         `")+tbUserVcard+string("` AS `tbuser` ")+
            string(" where   `hvnd`.`lft` <`node`.`lft` AND `node`.`rgh` < `hvnd`.`rgh` and `node`.`depth` = `hvnd`.`depth` + 1 AND ")+ 
            string("        `tbuser`.`company_id` = ? AND `tbuser`.`user_id` = ? ")+
            string(" order by `node`.`lft` ASC ");
                             
        pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement.set_param_bind(0, company_id);
        statement.set_param_bind(1, user_id);
        statement.set_param_bind(2, company_id);
        statement.set_param_bind(3, company_id);
        statement.set_param_bind(4, user_id);
        
        if (0 != statement.query())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }

        int32_t node_id = 0;
        int32_t parent_id = 0;
        char node_name[50 * 3] = {'\0'};
        char title[100 * 3] = {'\0'};
        int32_t work_id = 0;
        char name[100 * 3] = {'\0'};
        char english_name[100 * 3] = {'\0'};
        char nick_name[100 * 3] = {'\0'};
        int32_t birthday = 0;
        int32_t sex = 0;
        char email[100 * 3] = {'\0'};
        char mobile[32 * 3] = {'\0'};
        char tel[32 * 3] = {'\0'};
        char sign_info[256 * 3] = {'\0'};
        char head_img[256 * 3] = {'\0'};
		uint64_t head_update_time = 0;
        int32_t status = 0;
		uint64_t update_time = 0;

        statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&node_id), sizeof(node_id), NULL);
        statement.set_param_result(1, MYSQL_TYPE_LONG, (char*)(&parent_id), sizeof(parent_id), NULL);
        statement.set_param_result(2, MYSQL_TYPE_STRING, node_name, sizeof(node_name) - 1, NULL);
        statement.set_param_result(3, MYSQL_TYPE_STRING, title, sizeof(title) - 1, NULL);
        statement.set_param_result(4, MYSQL_TYPE_LONG, (char*)(&work_id), sizeof(work_id), NULL);
        statement.set_param_result(5, MYSQL_TYPE_STRING, name, sizeof(name) - 1, NULL);
        statement.set_param_result(6, MYSQL_TYPE_STRING, english_name, sizeof(english_name) - 1, NULL);
        statement.set_param_result(7, MYSQL_TYPE_STRING, nick_name, sizeof(nick_name) - 1, NULL);
        statement.set_param_result(8, MYSQL_TYPE_LONG, (char*)(&birthday), sizeof(birthday), NULL);
        statement.set_param_result(9, MYSQL_TYPE_LONG, (char*)(&sex), sizeof(sex), NULL);
        statement.set_param_result(10, MYSQL_TYPE_STRING, email, sizeof(email) - 1, NULL);
        statement.set_param_result(11, MYSQL_TYPE_STRING, mobile, sizeof(mobile) - 1, NULL);
        statement.set_param_result(12, MYSQL_TYPE_STRING, tel, sizeof(tel) - 1, NULL);
        statement.set_param_result(13, MYSQL_TYPE_STRING, sign_info, sizeof(sign_info) - 1, NULL);
        statement.set_param_result(14, MYSQL_TYPE_STRING, head_img, sizeof(head_img) - 1, NULL);
        statement.set_param_result(15, MYSQL_TYPE_LONGLONG, (char*)(&head_update_time), sizeof(head_update_time), NULL);
        statement.set_param_result(16, MYSQL_TYPE_LONG, (char*)(&status), sizeof(status), NULL);
		statement.set_param_result(17, MYSQL_TYPE_LONGLONG, (char*)(&update_time), sizeof(update_time), NULL);
        if (0 != statement.get_result())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        if(statement.get_num_rows() <= 0)
        {
            break;
        }
                        
        Value body;
        Value pos;
        bool isfirst = true;
        while (0 == statement.fetch_result())
        {
            if(isfirst)
            {
                rspbody["work_id"] = work_id;
                rspbody["name"] = name;
                rspbody["english_name"] = english_name;
                rspbody["nick_name"] = nick_name;
                rspbody["birthday"] = birthday;
                rspbody["sex"] = sex;
                rspbody["email"] = email;
                rspbody["mobile"] = mobile;
                rspbody["tel"] = tel;
                rspbody["sign_info"] = sign_info;
                rspbody["head_img"] = head_img;
                rspbody["head_update_time"] = head_update_time;
                rspbody["status"] = status;
                rspbody["update_time"] = update_time;
            }
            Value posbody;
            posbody["parent_id"] = parent_id;
            posbody["node_id"] = node_id;
            posbody["department"] = node_name;
            posbody["title"] = title;
            pos.append(posbody);
        }
        rspbody["pos_json"] = pos;
        
        result = "success";
    }while(false);
    
    
    http_rsp(fd, "get_user_info_rsp", msgid, result, rspbody);
}

void http_add_company(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        ISVALIDFILED_STR("name");
        ISVALIDFILED_STR("country");
        ISVALIDFILED_STR("address");
        ISVALIDFILED_STR("admin_account");
        ISVALIDFILED_STR("password");

        string companyname = body["name"].asString();
        string country = body["country"].asString();
        string address = body["address"].asString();
        string adminaccount = body["admin_account"].asString();
        string password = body["password"].asString();

        
        if(NULL == mysql)
        {
            break;
        }
        //创建新公司
        string sqlstr = "call `create_company`(?,?,?,NULL,NULL,?,?,@ret_company_id);";
        pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement.set_param_bind(0, companyname);
        statement.set_param_bind(1, address);
        statement.set_param_bind(2, country);
        statement.set_param_bind(3, adminaccount);
        statement.set_param_bind(4, password);

        if (0 != statement.execute())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }

        //获取新增ID
        int company_id = 0;
        string sqlstr2 = "select @ret_company_id;";
        pre_statement statement2;
        if (0 != statement2.init(mysql, sqlstr2))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        if (0 != statement2.query())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement2.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&company_id), sizeof(company_id), NULL);
        if (0 != statement2.get_result())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        if (0 != statement2.fetch_result())
        {
            break;
        }
        if(company_id <= 0)
        {
           log(LOG_ERROR, "add_company fail!! companyname[%s]  country[%s]  address[%s]  admin[%s] pawd[%s] ret=%d",
                    companyname.c_str(),
                    country.c_str(),
                    address.c_str(),
                    adminaccount.c_str(),
                    password.c_str(),
                    company_id);
            break;
        }
        result = "success";
        rspbody["company_id"] = company_id;
    }while(false);
    
    http_rsp(fd, "add_company_rsp", msgid, result, rspbody);

    if("success" == result)
    {
        org_refresh_redis(redis, rspbody["company_id"].asUInt());
        http_req_refresh_company_org(rspbody["company_id"].asUInt(), mysql, redis);
        org_update_notify_req(rspbody["company_id"].asUInt());
    }
    
}

void http_modify_company_info(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_STR("name");
        ISVALIDFILED_STR("country");
        ISVALIDFILED_STR("address");
        ISVALIDFILED_STR("admin_account");
        ISVALIDFILED_STR("password");

        uint32_t company_id = body["company_id"].asUInt();
        string companyname = body["name"].asString();
        string country = body["country"].asString();
        string address = body["address"].asString();
        string adminaccount = body["admin_account"].asString();
        string password = body["password"].asString();

        rspbody["company_id"] = company_id;

        
        if(NULL == mysql)
        {
            break;
        }
        //创建新公司
        string sqlstr = "call `modify_company_info`(?,?,?,?,NULL,NULL,?,?,@ret_modify_company);";
        pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement.set_param_bind(0, company_id);
        statement.set_param_bind(1, companyname);
        statement.set_param_bind(2, address);
        statement.set_param_bind(3, country);
        statement.set_param_bind(4, adminaccount);
        statement.set_param_bind(5, password);

        if (0 != statement.execute())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }

        //获取新增ID
        int ret_modify = 0;
        string sqlstr2 = "select @ret_modify_company;";
        pre_statement statement2;
        if (0 != statement2.init(mysql, sqlstr2))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        if (0 != statement2.query())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement2.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&ret_modify), sizeof(ret_modify), NULL);
        if (0 != statement2.get_result())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        if (0 != statement2.fetch_result())
        {
            break;
        }
        if(ret_modify != 0)
        {
            log(LOG_ERROR, "modify_company fail!!  company_id[%u] companyname[%s]  country[%s]  address[%s]  admin[%s] pawd[%s] ret=%d",
                    company_id,
                    companyname.c_str(),
                    country.c_str(),
                    address.c_str(),
                    adminaccount.c_str(),
                    password.c_str(),
                    ret_modify);
            break;
        }
        result = "success";
    }while(false);
    
    http_rsp(fd, "modify_company_info_rsp", msgid, result, rspbody);

    if("success" == result)
    {
        org_refresh_redis(redis, rspbody["company_id"].asUInt());
        http_req_refresh_company_org(rspbody["company_id"].asUInt(), mysql, redis);
        org_update_notify_req(rspbody["company_id"].asUInt());
    }
    
}




void http_add_department(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_UINT("parent_id");
        ISVALIDFILED_STR("name");

        uint32_t company_id = body["company_id"].asUInt();
        uint32_t parent_id = body["parent_id"].asUInt();
        string department_name = body["name"].asString();
        rspbody["company_id"] = company_id;

        
        if(NULL == mysql)
        {
            break;
        }
        //创建新部门
        string sqlstr = "call `create_department`(?,?,?,@ret_node_id);";
        pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement.set_param_bind(0, company_id);
        statement.set_param_bind(1, parent_id);
        statement.set_param_bind(2, department_name);

        if (0 != statement.execute())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }

        //获取新增ID
        int node_id = 0;
        string sqlstr2 = "select @ret_node_id;";
        pre_statement statement2;
        if (0 != statement2.init(mysql, sqlstr2))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        if (0 != statement2.query())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement2.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&node_id), sizeof(node_id), NULL);
        if (0 != statement2.get_result())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        if (0 != statement2.fetch_result())
        {
            break;
        }
        if(node_id <= 0)
        {
            log(LOG_ERROR, "add_department fail!  company_id[%u] parent_id[%u] name[%s] ret=%d",
                    company_id,
                    parent_id,
                    department_name.c_str(),
                    node_id);
            break;
        }
        result = "success";
        rspbody["node_id"] = node_id;
    }while(false);
    
    http_rsp(fd, "add_department_rsp", msgid, result, rspbody);

    if("success" == result)
    {
        org_refresh_redis(redis, rspbody["company_id"].asUInt());
        http_req_refresh_company_org(rspbody["company_id"].asUInt(), mysql, redis);
        org_update_notify_req(rspbody["company_id"].asUInt());
    }
    
}

void http_modify_department_info(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_UINT("node_id");
        ISVALIDFILED_STR("name");

        uint32_t company_id = body["company_id"].asUInt();
        uint32_t node_id = body["node_id"].asUInt();
        string name = body["name"].asString();
        rspbody["company_id"] = company_id;

        
        if(NULL == mysql)
        {
            break;
        }
        //修改信息
        string sqlstr = string("update `")+get_sub_table_name("t_im_org", company_id)+
                             string("` set `node_name` = ? where `company_id`=? and `node_id`=?");
        pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement.set_param_bind(0, name);
        statement.set_param_bind(1, company_id);
        statement.set_param_bind(2, node_id);

        if (0 != statement.execute())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }

        result = "success";
    }while(false);
    http_rsp(fd, "modify_department_info_rsp", msgid, result, rspbody);
    
    if("success" == result)
    {
        org_refresh_redis(redis, rspbody["company_id"].asUInt());
        http_req_refresh_company_org(rspbody["company_id"].asUInt(), mysql, redis);
        org_update_notify_req(rspbody["company_id"].asUInt());
    }
    
}

void http_del_department(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_UINT("node_id");
        
        uint32_t company_id = body["company_id"].asUInt();
        uint32_t node_id = body["node_id"].asUInt();
  
        rspbody["company_id"] = company_id;
        rspbody["node_id"] =node_id;
        
        
        if(NULL == mysql)
        {
            break;
        }

        int tt = del_node(company_id, node_id, mysql);
        if(0 != tt)
        {
            log(LOG_ERROR, "del_department fail!  company_id[%u] node_id[%u]  ret=%d",
                    company_id,
                    node_id,
                    tt);
            break;
        }

        result = "success";
    }while(false);
    http_rsp(fd, "del_department_rsp", msgid, result, rspbody);
    
    if("success" == result)
    {
        org_refresh_redis(redis, rspbody["company_id"].asUInt());
        http_req_refresh_company_org(rspbody["company_id"].asUInt(), mysql, redis);
        org_update_notify_req(rspbody["company_id"].asUInt());
    }
    
}

void http_add_user(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_UINT("work_id");
        ISVALIDFILED_STR("name");
        ISVALIDFILED_STR("english_name");
        ISVALIDFILED_STR("nick_name");
        ISVALIDFILED_UINT("birthday");
        ISVALIDFILED_UINT("sex");
        ISVALIDFILED_STR("email");
        ISVALIDFILED_STR("mobile");
        ISVALIDFILED_STR("tel");
        ISVALIDFILED_STR("sign_info");
        ISVALIDFILED_STR("head_img");
        ISVALIDFILED_UINT("status");


        if(body["pos_json"].isNull())
        {
            break;
        }
        Value &pos_json = body["pos_json"];
        if(!pos_json.isArray())
        {
            break;
        }
        if(pos_json.size() < 1)
        {
            break;
        }
        if(pos_json[0]["parent_id"].isNull())break;if(!pos_json[0]["parent_id"].isUInt())break;
        if(pos_json[0]["title"].isNull())break;if(!pos_json[0]["title"].isString())break;

        
        uint32_t company_id = body["company_id"].asUInt();
        uint32_t work_id = body["work_id"].asUInt();
        string name = body["name"].asString();
        string english_name = body["english_name"].asString();
        string nick_name = body["nick_name"].asString();
        uint32_t birthday = body["birthday"].asUInt();
        uint32_t sex = body["sex"].asUInt();
        string email = body["email"].asString();
        string mobile = body["mobile"].asString();
        string tel = body["tel"].asString();
        string sign_info = body["sign_info"].asString();
        string head_img = body["head_img"].asString();
        uint32_t status = body["status"].asUInt();
        uint32_t parent_id = pos_json[0]["parent_id"].asUInt();
        string title = pos_json[0]["title"].asString();

        rspbody["company_id"] = company_id;
        
        if(NULL == mysql)
        {
            break;
        }
        //创建新用户
        string sqlstr = "call `create_user`(?,?,?,?,?,?,?,?,?,?,?,?,?,?,unix_timestamp(now()),?,@ret_user_node_id,@ret_user_id);";
        pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement.set_param_bind(0, company_id);
        statement.set_param_bind(1, parent_id);
        statement.set_param_bind(2, title);
        statement.set_param_bind(3, work_id);
        statement.set_param_bind(4, name);
        statement.set_param_bind(5, english_name);
        statement.set_param_bind(6, nick_name);
        statement.set_param_bind(7, birthday);
        statement.set_param_bind(8, sex);
        statement.set_param_bind(9, email);
        statement.set_param_bind(10, mobile);
        statement.set_param_bind(11, tel);
        statement.set_param_bind(12, sign_info);
        statement.set_param_bind(13, head_img);
        statement.set_param_bind(14, status);

        if (0 != statement.execute())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }

        //获取新增ID
        int node_id = 0;
        int user_id = 0;
        string sqlstr2 = "select @ret_user_node_id,@ret_user_id;";
        pre_statement statement2;
        if (0 != statement2.init(mysql, sqlstr2))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        if (0 != statement2.query())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement2.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&node_id), sizeof(node_id), NULL);
        statement2.set_param_result(1, MYSQL_TYPE_LONG, (char*)(&user_id), sizeof(user_id), NULL);
        if (0 != statement2.get_result())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        if (0 != statement2.fetch_result())
        {
            break;
        }
        if(node_id <= 0 || user_id <= 0)
        {
            log(LOG_ERROR, "add_user fail!   company_id[%u] parent_id[%u] title[%s] work_id[%u] name[%s] english_name[%s] nick_name[%s] birthday[%u] sex[%u] email[%s] mobile[%s] tel[%s] sign_info[%s] head_img[%s] status[%u] ret=%d" 
                    , company_id
                    , parent_id
                    , title.c_str()
                    , work_id
                    , name.c_str()
                    , english_name.c_str()
                    , nick_name.c_str()
                    , birthday
                    , sex
                    , email.c_str()
                    , mobile.c_str()
                    , tel.c_str()
                    , sign_info.c_str()
                    , head_img.c_str()
                    , status
                    , node_id);
            break;
        }

        //新增的职位信息
        Value resPosBody;
        Value pos1;
        pos1["parent_id"] = parent_id;
        pos1["node_id"] = node_id;
        pos1["title"] = title;
        resPosBody.append(pos1);

        //添加更多的职位
        for(uint32_t i = 1; i < pos_json.size(); i++)
        {
            if(pos_json[i]["parent_id"].isNull())continue;if(!pos_json[i]["parent_id"].isUInt())continue;
            if(pos_json[i]["title"].isNull())continue;if(!pos_json[i]["title"].isString())continue;
            uint32_t parent_id = pos_json[i]["parent_id"].asUInt();
            string title = pos_json[i]["title"].asString();

            int newnodeid = user_add_title(company_id, user_id, parent_id, title, mysql);
            if(newnodeid > 0)
            {
                Value pos;
                pos["parent_id"] = parent_id;
                pos["node_id"] = newnodeid;
                pos["title"] = title;
                resPosBody.append(pos);
            }
        }

        result = "success";
        rspbody["user_id"] = user_id;
        rspbody["pos_json"] = resPosBody;
    }while(false);
    http_rsp(fd, "add_user_rsp", msgid, result, rspbody);
    
    if("success" == result)
    {
        org_refresh_redis(redis, rspbody["company_id"].asUInt());
        http_req_refresh_company_org(rspbody["company_id"].asUInt(), mysql, redis);
        org_update_notify_req(rspbody["company_id"].asUInt(), rspbody["user_id"].asUInt(), 1);
    }
    
}

void http_modify_user_info(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_UINT("user_id");
        ISVALIDFILED_UINT("work_id");
        ISVALIDFILED_STR("name");
        ISVALIDFILED_STR("english_name");
        ISVALIDFILED_STR("nick_name");
        ISVALIDFILED_UINT("birthday");
        ISVALIDFILED_UINT("sex");
        ISVALIDFILED_STR("email");
        ISVALIDFILED_STR("mobile");
        ISVALIDFILED_STR("tel");
        ISVALIDFILED_STR("sign_info");
        ISVALIDFILED_STR("head_img");
        ISVALIDFILED_UINT("status");
        
        uint32_t company_id = body["company_id"].asUInt();
        uint32_t user_id = body["user_id"].asUInt();
        uint32_t work_id = body["work_id"].asUInt();
        string name = body["name"].asString();
        string english_name = body["english_name"].asString();
        string nick_name = body["nick_name"].asString();
        uint32_t birthday = body["birthday"].asUInt();
        uint32_t sex = body["sex"].asUInt();
        string email = body["email"].asString();
        string mobile = body["mobile"].asString();
        string tel = body["tel"].asString();
        string sign_info = body["sign_info"].asString();
        string head_img = body["head_img"].asString();
        uint32_t status = body["status"].asUInt();

        rspbody["company_id"] = company_id;
        rspbody["user_id"] = user_id;
        
        if(NULL == mysql)
        {
            break;
        }
        //修改信息
        string sqlstr = string("update `") + get_sub_table_name("t_im_user_vcard", company_id) +
                             string("` set `work_id`=?, ")+
                             string("      `name`=?, ")+
                             string("      `english_name`=?, ")+
                             string("      `nick_name`=?, ")+
                             string("      `birthday`=?, ")+
                             string("      `sex`=?, ")+
                             string("      `email`=?, ")+
                             string("      `mobile`=?, ")+
                             string("      `tel`=?, ")+
                             string("      `sign_info`=?, ")+
                             string("      `head_img`=?, ")+
                             string("      `status`=?, ")+
                             string("      `update_time`=unix_timestamp(now())  ")+
                             string("  where `company_id`=? and `user_id`=? ");
        pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement.set_param_bind(0, work_id);
        statement.set_param_bind(1, name);
        statement.set_param_bind(2, english_name);
        statement.set_param_bind(3, nick_name);
        statement.set_param_bind(4, birthday);
        statement.set_param_bind(5, sex);
        statement.set_param_bind(6, email);
        statement.set_param_bind(7, mobile);
        statement.set_param_bind(8, tel);
        statement.set_param_bind(9, sign_info);
        statement.set_param_bind(10, head_img);
        statement.set_param_bind(11, status);
        statement.set_param_bind(12, company_id);
        statement.set_param_bind(13, user_id);

        if (0 != statement.execute())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }

        result = "success";
    }while(false);
    http_rsp(fd, "modify_user_info_rsp", msgid, result, rspbody);
    
    if("success" == result)
    {
        org_refresh_redis(redis, rspbody["company_id"].asUInt());
        http_req_refresh_company_org(rspbody["company_id"].asUInt(), mysql, redis);
        org_update_notify_req(rspbody["company_id"].asUInt(), rspbody["user_id"].asUInt(), 3);
    }
    
}

void http_add_user_title(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_UINT("user_id");
        ISVALIDFILED_UINT("parent_id");
        ISVALIDFILED_STR("title");
        
        uint32_t company_id = body["company_id"].asUInt();
        uint32_t user_id = body["user_id"].asUInt();
        uint32_t parent_id = body["parent_id"].asUInt();
        string title = body["title"].asString();
  

        
        if(NULL == mysql)
        {
            break;
        }
        int newnodeid = user_add_title(company_id, user_id, parent_id, title, mysql);
        if(newnodeid < 0)
        {
            log(LOG_ERROR, "user_add_title fail! company_id[%u] user_id[%u] parent_id[%u] title[%s] ret=%d",
                    company_id,
                    user_id,
                    parent_id,
                    title.c_str(),
                    newnodeid);
            break;
        }

        //新增的职位信息
        result = "success";
        rspbody["company_id"] = company_id;
        rspbody["user_id"] = user_id;
        rspbody["parent_id"] = parent_id;
        rspbody["title"] = title;
        rspbody["node_id"] = newnodeid;
    }while(false);
    http_rsp(fd, "user_title_add_rsp", msgid, result, rspbody);
    
    if("success" == result)
    {
        org_refresh_redis(redis, rspbody["company_id"].asUInt());
        http_req_refresh_company_org(rspbody["company_id"].asUInt(), mysql, redis);
        org_update_notify_req(rspbody["company_id"].asUInt());
    }
    
}

void http_modify_user_title(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_UINT("node_id");
        ISVALIDFILED_STR("title");
        
        uint32_t company_id = body["company_id"].asUInt();
        uint32_t node_id = body["node_id"].asUInt();
        string title = body["title"].asString();
  

        
        if(NULL == mysql)
        {
            break;
        }


        //修改信息
        string sqlstr = string("update `")+get_sub_table_name("t_im_org", company_id)+
                             string("` set `title` = ? where `company_id`=? and `node_id`=?");
        pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement.set_param_bind(0, title);
        statement.set_param_bind(1, company_id);
        statement.set_param_bind(2, node_id);

        if (0 != statement.execute())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }

        //新增的职位信息
        result = "success";
        rspbody["company_id"] = company_id;
        rspbody["node_id"] =node_id;
        rspbody["title"] = title;
    }while(false);
    http_rsp(fd, "user_title_modify_rsp", msgid, result, rspbody);
    
    if("success" == result)
    {
        org_refresh_redis(redis, rspbody["company_id"].asUInt());
        http_req_refresh_company_org(rspbody["company_id"].asUInt(), mysql, redis);
        org_update_notify_req(rspbody["company_id"].asUInt());
    }
    
}

void http_del_user_title(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_UINT("node_id");
        
        uint32_t company_id = body["company_id"].asUInt();
        uint32_t node_id = body["node_id"].asUInt();
  
        rspbody["company_id"] = company_id;
        rspbody["node_id"] =node_id;
        
        
        if(NULL == mysql)
        {
            break;
        }

        int tt= del_node(company_id, node_id, mysql);
        if(0 != tt)
        {
            log(LOG_ERROR, "user_del_title fail! company_id[%u] node_id[%u]  ret=%d",
                    company_id,
                    node_id,
                    tt);
            break;
        }

        result = "success";
    }while(false);
    http_rsp(fd, "user_title_del_rsp", msgid, result, rspbody);
    
    if("success" == result)
    {
        org_refresh_redis(redis, rspbody["company_id"].asUInt());
        http_req_refresh_company_org(rspbody["company_id"].asUInt(), mysql, redis);
        org_update_notify_req(rspbody["company_id"].asUInt());
    }
    
}

void http_del_user(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_UINT("user_id");

        uint32_t company_id = body["company_id"].asUInt();
        uint32_t user_id = body["user_id"].asUInt();

        rspbody["company_id"] = company_id;
        rspbody["user_id"] = user_id;
        
        if(NULL == mysql)
        {
            break;
        }
        //创建新部门
        string sqlstr = "call `delete_user`(?,?,@ret_del_user);";
        pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement.set_param_bind(0, company_id);
        statement.set_param_bind(1, user_id);

        if (0 != statement.execute())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }

        //获取返回值 
        int ret_del_user = 0;
        string sqlstr2 = "select @ret_del_user;";
        pre_statement statement2;
        if (0 != statement2.init(mysql, sqlstr2))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        if (0 != statement2.query())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement2.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&ret_del_user), sizeof(ret_del_user), NULL);
        if (0 != statement2.get_result())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        if (0 != statement2.fetch_result())
        {
            break;
        }
        if(ret_del_user != 0)
        {
            log(LOG_ERROR, "del_user fail! company_id[%u] user_id[%u] ret=%d",
                    company_id,
                    user_id,
                    ret_del_user);
            break;
        }
		statement.free();
		string table_name_a = string("t_im_user_group_") + to_string(company_id % 8);
		string sql = string("select friend_company_id, friend_group_id from ") + table_name_a + string(" where company_id = ? and user_id = ?");
		if (0 != statement.init(mysql, sql))
		{
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			break;
		}
		statement.set_param_bind(0, company_id);
		statement.set_param_bind(1, user_id);
		if (0 != statement.query())
		{
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			break;
		}
		uint32_t friend_company_id;
		unsigned long length = sizeof(friend_company_id);
		statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&friend_company_id), length, NULL);
		uint32_t friend_group_id;
		length = sizeof(friend_group_id);
		statement.set_param_result(1, MYSQL_TYPE_LONG, (char*)(&friend_group_id), length, NULL);
		if (0 != statement.get_result())
		{
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			break;
		}
		struct timeval tv;
		gettimeofday(&tv, NULL);
		uint64_t update_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
		while (0 == statement.fetch_result())
		{
			pre_statement state;
			string sql("call exit_group(?, ?, ?, ?, ?)");
			if (0 != state.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			state.set_param_bind(0, friend_company_id);
			state.set_param_bind(1, friend_group_id);
			state.set_param_bind(2, company_id);
			state.set_param_bind(3, user_id);
			state.set_param_bind(4, update_time);
			state.execute();
			redis_user_group_list(company_id, user_id, mysql, redis);
			redis_group_member_list(friend_company_id, friend_group_id, mysql, redis);
		}
        result = "success";
    }while(false);
    http_rsp(fd, "del_user_rsp", msgid, result, rspbody);
    
    if("success" == result)
    {
        org_refresh_redis(redis, rspbody["company_id"].asUInt());
        http_req_refresh_company_org(rspbody["company_id"].asUInt(), mysql, redis);
        org_update_notify_req(rspbody["company_id"].asUInt(), body["user_id"].asUInt(), 2);
    }
    
}

void http_mov_node(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_UINT("node_id");
        ISVALIDFILED_UINT("to_parent_node_id");
        
        uint32_t company_id = body["company_id"].asUInt();
        uint32_t node_id = body["node_id"].asUInt();
        uint32_t to_parent_node_id = body["to_parent_node_id"].asUInt();
  
        rspbody["company_id"] = company_id;
        rspbody["node_id"] = node_id;
        rspbody["to_parent_node_id"] = to_parent_node_id;
        
        
        if(NULL == mysql)
        {
            break;
        }

        int tt = mov_node(company_id, node_id, to_parent_node_id, mysql);
        if( -3 == tt)
        {
            result = "nonodeid";
        }
        else if(-4 == tt)
        {
            result = "nodetoparentnodeid";
        }
        else if(0 != tt)
        {
            result = "fail";
        }
        else
        {
            result = "success";
        }
    }while(false);
    http_rsp(fd, "mov_node_rsp", msgid, result, rspbody);
    
    if("success" == result)
    {
        org_refresh_redis(redis, rspbody["company_id"].asUInt());
        http_req_refresh_company_org(rspbody["company_id"].asUInt(), mysql, redis);
        org_update_notify_req(rspbody["company_id"].asUInt());
    }
    
}

void http_msg_statistics_rsp(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    
    pre_statement statement;
    string result = "fail";
    Value rsp_body;
    do
    {
        if (body.isNull())
        {
            break;
        }
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_STR("time_begin");
        ISVALIDFILED_UINT("interval");

        
        string sql;
        uint32_t company_id = body["company_id"].asUInt();
        if (0 == company_id)
        {
            sql = string("select session_type, node_id, online_count, offline_count, update_time from t_im_msg_report where date_sub(?, interval ? day) <= date(update_time)");
            if (0 != statement.init(mysql, sql))
            {
                log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
                break;
            }
            string time_begin = body["time_begin"].asString();
            statement.set_param_bind(0, time_begin);
            uint32_t interval = body["interval"].asUInt();
            statement.set_param_bind(1, interval);
        }
        else
        {
            sql = string("select session_type, node_id, online_count, offline_count, update_time from t_im_msg_report where company_id = ? and (date_sub(?, interval ? day) <= date(update_time))");
            if (0 != statement.init(mysql, sql))
            {
                log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
                break;
            }
            statement.set_param_bind(0, company_id);
            string time_begin = body["time_begin"].asString();
            statement.set_param_bind(1, time_begin);
            uint32_t interval = body["interval"].asUInt();
            statement.set_param_bind(2, interval);
        }
        if (0 != statement.query())
        {
            log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        uint8_t session_type;
        unsigned long length = sizeof(session_type);
        statement.set_param_result(0, MYSQL_TYPE_TINY, (char*)(&session_type), length, NULL);
        uint32_t node_id;
        length = sizeof(node_id);
        statement.set_param_result(1, MYSQL_TYPE_LONG, (char*)(&node_id), length, NULL);
        uint32_t online_count;
        length = sizeof(online_count);
        statement.set_param_result(2, MYSQL_TYPE_LONG, (char*)(&online_count), length, NULL);
        uint32_t offline_count;
        length = sizeof(offline_count);
        statement.set_param_result(3, MYSQL_TYPE_LONG, (char*)(&offline_count), length, NULL);
        char update_time[32];
        length = sizeof(update_time);
        statement.set_param_result(4, MYSQL_TYPE_STRING, update_time, length, NULL);
        if (0 != statement.get_result())
        {
            log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        if (0 == statement.get_num_rows())
        {
            result = "nocompany";
            break;
        }
        while (0 == statement.fetch_result())
        {
            Value value;
            value["session_type"] = session_type;
            value["node_id"] = node_id;
            value["online_count"] = online_count;
            value["offline_count"] = offline_count;
            value["time"] = update_time;
            rsp_body.append(value);
        }
        result = "success";
    } while (false);
    http_rsp(fd, "get_msg_statistics_rsp", msgid, result, rsp_body);
    statement.free();
    
}

void http_login_statistics_rsp(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    
    pre_statement statement;
    string result = "fail";
    Value rsp_body;
    do
    {
        if (body.isNull())
        {
            break;
        }
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_STR("time_begin");
        ISVALIDFILED_UINT("interval");

        
        string sql;
        uint32_t company_id = body["company_id"].asUInt();
        if (0 == company_id)
        {
            sql = string("select node_id, online_count, online_peek, update_time from t_im_online_report where date_sub(?, interval ? day) <= date(update_time)");
            if (0 != statement.init(mysql, sql))
            {
                log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
                break;
            }
            string time_begin = body["time_begin"].asString();
            statement.set_param_bind(0, time_begin);
            uint32_t interval = body["time_end"].asUInt();
            statement.set_param_bind(1, interval);
        }
        else
        {
            sql = string("select node_id, online_count, online_peek, update_time from t_im_online_report where company_id = ? and (date_sub(?, interval ? day) <= date(update_time))");
            if (0 != statement.init(mysql, sql))
            {
                log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
                break;
            }
            statement.set_param_bind(0, company_id);
            string time_begin = body["time_begin"].asString();
            statement.set_param_bind(1, time_begin);
            uint32_t interval = body["time_end"].asUInt();
            statement.set_param_bind(2, interval);
        }
        if (0 != statement.query())
        {
            log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        uint32_t node_id;
        unsigned long length = sizeof(node_id);
        statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&node_id), length, NULL);
        uint32_t online_count;
        length = sizeof(online_count);
        statement.set_param_result(1, MYSQL_TYPE_LONG, (char*)(&online_count), length, NULL);
        uint32_t online_peek;
        length = sizeof(online_peek);
        statement.set_param_result(2, MYSQL_TYPE_LONG, (char*)(&online_peek), length, NULL);
        char update_time[32];
        length = sizeof(update_time);
        statement.set_param_result(3, MYSQL_TYPE_STRING, update_time, length, NULL);
        if (0 != statement.get_result())
        {
            log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        if (0 == statement.get_num_rows())
        {
            result = "nocompany";
            break;
        }
        while (0 == statement.fetch_result())
        {
            Value value;
            value["node_id"] = node_id;
            value["online_count"] = online_count;
            value["online_peek"] = online_peek;
            value["time"] = update_time;
            rsp_body.append(value);
        }
        result = "success";
    } while (false);
    http_rsp(fd, "get_login_statistics_rsp", msgid, result, rsp_body);
    statement.free();
    
}


void http_add_role(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_STR("role_name");
        
        uint32_t company_id = body["company_id"].asUInt();
    std::string role_name = body["role_name"].asString(); 
  
        rspbody["company_id"] = company_id;
        rspbody["role_name"] = role_name;
        
        if(NULL == mysql)
        {
            break;
        }

        string sqlstr = string("insert into  `")+get_sub_table_name("t_ids_role_dict", company_id)+
                             string("`(company_id,role_name) value (?,?)");
        pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement.set_param_bind(0, company_id);
        statement.set_param_bind(1, role_name);

        if (0 != statement.execute())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }

        result = "success";
    }while(false);

    http_rsp(fd, "add_role_rsp", msgid, result, rspbody);
}

void http_modify_role_attribute(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_UINT("role_id");
        ISVALIDFILED_STR("role_name");
        
        uint32_t company_id = body["company_id"].asUInt();
        uint32_t role_id = body["role_id"].asUInt();
    std::string role_name = body["role_name"].asString(); 
  
        rspbody["company_id"] = company_id;
        rspbody["role_id"] = role_id;
        rspbody["role_name"] = role_name;
        
        if(NULL == mysql)
        {
            break;
        }

        string sqlstr = string("update  `")+get_sub_table_name("t_ids_role_dict", company_id)+
                             string("` set role_name = ?,update_time = CURRENT_TIMESTAMP  where company_id = ? and role_id = ?");
        pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement.set_param_bind(0, role_name);
        statement.set_param_bind(1, company_id);
        statement.set_param_bind(2, role_id);

        if (0 != statement.execute())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }

        result = "success";
    }while(false);

    http_rsp(fd, "modify_role_attribute_rsp", msgid, result, rspbody);
}

void http_query_role_list(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        
        ISVALIDFILED_UINT("company_id");

        uint32_t company_id = body["company_id"].asUInt();
       
        if(NULL == mysql)
        {
            break;
        }

        string sqlstr = string("select `role_id`,`role_name` from `")+
            get_sub_table_name("t_ids_role_dict",company_id)+string("` where `company_id`=?");

        pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement.set_param_bind(0, company_id);
        
        if (0 != statement.query())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }

        uint32_t role_id = 0;
        char     role_name[200] = {'\0'};
        statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&role_id), sizeof(role_id), NULL);
        statement.set_param_result(1, MYSQL_TYPE_STRING, role_name, sizeof(role_name) - 1, NULL);

        if (0 != statement.get_result())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
                        
        Value body;
        while (0 == statement.fetch_result())
        {
            Value nodebody;
            nodebody["role_id"] = role_id;
            nodebody["role_name"] = role_name;
            body.append(nodebody);
        }

        result = "success";
        rspbody["role_list"] = body;
    }while(false);
    
    
    http_rsp(fd, "query_role_list_rsp", msgid, result, rspbody);
}

void http_query_role_authority(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_UINT("role_id");

        uint32_t company_id = body["company_id"].asUInt();
        uint32_t role_id = body["role_id"].asUInt();
       
        if(NULL == mysql)
        {
            break;
        }

        string sqlstr = string("select `auth_id`,`name`,`auth_value`,`ext_value1`,`ext_value2`,`ext_value3`,`ext_value4` from `")+
            get_sub_table_name("t_ids_authority_value",company_id)+string("` where `company_id`=? and `role_id`=?");

        pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement.set_param_bind(0, company_id);
        statement.set_param_bind(1, role_id);
        
        if (0 != statement.query())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }

        uint32_t auth_id = 0;
        char     auth_name[200] = {'\0'};
        char     auth_value[100] = {'\0'};
        char     ext_value1[100] = {'\0'};
        char     ext_value2[100] = {'\0'};
        char     ext_value3[100] = {'\0'};
        char     ext_value4[100] = {'\0'};
        statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&auth_id), sizeof(auth_id), NULL);
        statement.set_param_result(1, MYSQL_TYPE_STRING, auth_name, sizeof(auth_name) - 1, NULL);
        statement.set_param_result(2, MYSQL_TYPE_STRING, auth_value, sizeof(auth_value) - 1, NULL);
        statement.set_param_result(3, MYSQL_TYPE_STRING, ext_value1, sizeof(ext_value1) - 1, NULL);
        statement.set_param_result(4, MYSQL_TYPE_STRING, ext_value2, sizeof(ext_value2) - 1, NULL);
        statement.set_param_result(5, MYSQL_TYPE_STRING, ext_value3, sizeof(ext_value3) - 1, NULL);
        statement.set_param_result(6, MYSQL_TYPE_STRING, ext_value4, sizeof(ext_value4) - 1, NULL);

        if (0 != statement.get_result())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
                        
        if(0 == statement.fetch_result())
        {
            rspbody["auth_id"] = auth_id;
            rspbody["auth_name"] = auth_name;
            rspbody["auth_value"] = auth_value;
            rspbody["ext_value1"] = ext_value1;
            rspbody["ext_value2"] = ext_value2;
            rspbody["ext_value3"] = ext_value3;
            rspbody["ext_value4"] = ext_value4;
        }
        rspbody["role_id"] = role_id;
        result = "success";
    }while(false);
    
    
    http_rsp(fd, "query_role_authority_rsp", msgid, result, rspbody);
}

void http_add_role_authority(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_UINT("role_id");
        ISVALIDFILED_UINT("auth_id");
        ISVALIDFILED_STR("auth_name");
        ISVALIDFILED_STR("auth_value");
        ISVALIDFILED_STR("ext_value1");
        ISVALIDFILED_STR("ext_value2");
        ISVALIDFILED_STR("ext_value3");
        ISVALIDFILED_STR("ext_value4");

        uint32_t company_id = body["company_id"].asUInt();
        uint32_t role_id = body["role_id"].asUInt();
        uint32_t auth_id = body["auth_id"].asUInt();
        std::string auth_name = body["auth_name"].asString();
        std::string auth_value = body["auth_value"].asString();
        std::string ext_value1 = body["ext_value1"].asString();
        std::string ext_value2 = body["ext_value2"].asString();
        std::string ext_value3 = body["ext_value3"].asString();
        std::string ext_value4 = body["ext_value4"].asString();
       
        if(NULL == mysql)
        {
            break;
        }

        string sqlstr = string("insert  into `")+get_sub_table_name("t_ids_authority_value",company_id)+
	                string("`(`company_id`,`role_id`,`auth_id`,`name`,`auth_value`,`ext_value1`,`ext_value2`,`ext_value3`,`ext_value4`) value(?,?,?,?,?,?,?,?,?)");

        pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement.set_param_bind(0, company_id);
        statement.set_param_bind(1, role_id);
        statement.set_param_bind(2, auth_id);
        statement.set_param_bind(3, auth_name);
        statement.set_param_bind(4, auth_value);
        statement.set_param_bind(5, ext_value1);
        statement.set_param_bind(6, ext_value2);
        statement.set_param_bind(7, ext_value3);
        statement.set_param_bind(8, ext_value4);

        if (0 != statement.execute())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }

        result = "success";
    }while(false);

    http_rsp(fd, "add_role_authority_rsp", msgid, result, rspbody);
}

void http_modify_role_authority(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_UINT("role_id");
        ISVALIDFILED_UINT("auth_id");
        ISVALIDFILED_STR("auth_name");
        ISVALIDFILED_STR("auth_value");
        ISVALIDFILED_STR("ext_value1");
        ISVALIDFILED_STR("ext_value2");
        ISVALIDFILED_STR("ext_value3");
        ISVALIDFILED_STR("ext_value4");

        uint32_t company_id = body["company_id"].asUInt();
        uint32_t role_id = body["role_id"].asUInt();
        uint32_t auth_id = body["auth_id"].asUInt();
        std::string auth_name = body["auth_name"].asString();
        std::string auth_value = body["auth_value"].asString();
        std::string ext_value1 = body["ext_value1"].asString();
        std::string ext_value2 = body["ext_value2"].asString();
        std::string ext_value3 = body["ext_value3"].asString();
        std::string ext_value4 = body["ext_value4"].asString();
       
        if(NULL == mysql)
        {
            break;
        }

        string sqlstr = string("update `")+get_sub_table_name("t_ids_authority_value",company_id)+
	                string("` set  `name`=?,`auth_value`=?,`ext_value1`=?,`ext_value2`=?,`ext_value3`=?,`ext_value4`=?  where `company_id`=?  and `role_id` = ? and  `auth_id` = ?");

        pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement.set_param_bind(0, auth_name);
        statement.set_param_bind(1, auth_value);
        statement.set_param_bind(2, ext_value1);
        statement.set_param_bind(3, ext_value2);
        statement.set_param_bind(4, ext_value3);
        statement.set_param_bind(5, ext_value4);
        statement.set_param_bind(6, company_id);
        statement.set_param_bind(7, role_id);
        statement.set_param_bind(8, auth_id);

        if (0 != statement.execute())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }

        result = "success";
    }while(false);

    http_rsp(fd, "modify_role_authority_rsp", msgid, result, rspbody);
}

void http_del_role_authority(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_UINT("role_id");
        ISVALIDFILED_UINT("auth_id");
      
        uint32_t company_id = body["company_id"].asUInt();
        uint32_t role_id = body["role_id"].asUInt();
        uint32_t auth_id = body["auth_id"].asUInt();
      
        if(NULL == mysql)
        {
            break;
        }

        string sqlstr = string("delete from `")+get_sub_table_name("t_ids_authority_value",company_id)+
	                string("`  where `company_id`=?  and `role_id` = ? and  `auth_id` = ?");

        pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement.set_param_bind(0, company_id);
        statement.set_param_bind(1, role_id);
        statement.set_param_bind(2, auth_id);

        if (0 != statement.execute())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }

        result = "success";
    }while(false);

    http_rsp(fd, "del_role_authority_rsp", msgid, result, rspbody);
}


void http_del_role(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_UINT("role_id");

        uint32_t company_id = body["company_id"].asUInt();
        uint32_t role_id = body["role_id"].asUInt();

        rspbody["company_id"] = company_id;
        rspbody["role_id"] = role_id;
        
        if(NULL == mysql)
        {
            break;
        }
        //创建新部门
        string sqlstr = "call `delete_role`(?,?,@ret_del_role);";
        pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement.set_param_bind(0, company_id);
        statement.set_param_bind(1, role_id);

        if (0 != statement.execute())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }

        //获取返回值 
        int ret_del_role = 0;
        string sqlstr2 = "select @ret_del_role;";
        pre_statement statement2;
        if (0 != statement2.init(mysql, sqlstr2))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        if (0 != statement2.query())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement2.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&ret_del_role), sizeof(ret_del_role), NULL);
        if (0 != statement2.get_result())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        if (0 != statement2.fetch_result())
        {
            break;
        }
        if(ret_del_role != 0)
        {
            log(LOG_ERROR, "del_role fail! company_id[%u] role_id[%u] ret=%d",
                    company_id,
                    role_id,
                    ret_del_role);
            break;
        }
        result = "success";
    }while(false);
    http_rsp(fd, "del_role_rsp", msgid, result, rspbody);
}


void http_query_role_user_list(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_UINT("role_id");

        uint32_t company_id = body["company_id"].asUInt();
        uint32_t role_id = body["role_id"].asUInt();
        uint32_t index_begin = body["index_begin"].asUInt();
        uint32_t index_count = body["index_count"].asUInt();

        rspbody["company_id"] = company_id;
        rspbody["role_id"] = role_id;
        rspbody["index_begin"] = index_begin;
        rspbody["index_count"] = 0;
        rspbody["is_more"] = false;
        
        if(NULL == mysql)
        {
            break;
        }
        
	string sqlstr = std::string("select `role`.user_id,vcard.name from `")+get_sub_table_name("t_ids_user_role",company_id)+
	                string("`  as `role` left join  `")+get_sub_table_name("t_im_user_vcard",company_id)+
	                string("`  as `vcard` on `role`.`company_id` = `vcard`.`company_id` and `role`.user_id = `vcard`.`user_id`  where `role`.`company_id`=?  and `role`.`role_id` = ? order by `role`.`id` asc limit ?,?+1");


	pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement.set_param_bind(0, company_id);
        statement.set_param_bind(1, role_id);
        statement.set_param_bind(2, index_begin);
        statement.set_param_bind(3, index_count);
        
        if (0 != statement.query())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }

        uint32_t user_id = 0;
        char     user_name[100] = {'\0'};
        statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&user_id), sizeof(user_id), NULL);
        statement.set_param_result(1, MYSQL_TYPE_STRING, user_name, sizeof(user_name) - 1, NULL);

        if (0 != statement.get_result())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        bool ismore = false;
        if(statement.get_num_rows() > index_count)
        {
            ismore = true;
        }
                        
        Value body;
        uint32_t index = 0;
        while (0 == statement.fetch_result() && index < index_count)
        {
            Value nodebody;
            nodebody["user_id"] = user_id;
            nodebody["user_name"] = user_name;
            body.append(nodebody);
            index++;
        }

        result = "success";
        rspbody["index_count"] = index;
        rspbody["is_more"] = ismore;
        rspbody["user_list"] = body;
    }while(false);
    
    
    http_rsp(fd, "query_role_user_list_rsp", msgid, result, rspbody);
}

void http_add_user_role(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_UINT("user_id");
        ISVALIDFILED_UINT("role_id");

        uint32_t company_id = body["company_id"].asUInt();
        uint32_t user_id = body["user_id"].asUInt();
        uint32_t role_id = body["role_id"].asUInt();
       
        if(NULL == mysql)
        {
            break;
        }

        string sqlstr = string("insert  into `")+get_sub_table_name("t_ids_user_role",company_id)+
	                string("`(`company_id`,`user_id`,`role_id`) value(?,?,?)");

        pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement.set_param_bind(0, company_id);
        statement.set_param_bind(1, user_id);
        statement.set_param_bind(2, role_id);

        if (0 != statement.execute())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }

        result = "success";
    }while(false);

    http_rsp(fd, "add_user_role_rsp", msgid, result, rspbody);

}


void http_query_user_role(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_UINT("user_id");

        uint32_t company_id = body["company_id"].asUInt();
        uint32_t user_id = body["user_id"].asUInt();
       
        if(NULL == mysql)
        {
            break;
        }

        string sqlstr = std::string("select `role`.`role_id`,`role`.`role_name` from  `")+get_sub_table_name("t_ids_user_role", company_id)+
                        std::string("` as `user_role` left join  `")+get_sub_table_name("t_ids_role_dict",company_id)+
                        std::string("`  as `role` on  `user_role`.`company_id` = `role`.`company_id` and `user_role`.`role_id` = `role`.`role_id`  where `user_role`.`company_id`=? and `user_role`.`user_id`=?");

        pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement.set_param_bind(0, company_id);
        statement.set_param_bind(1, user_id);
        
        if (0 != statement.query())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }

        uint32_t role_id = 0;
        char     role_name[200] = {'\0'};
        statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&role_id), sizeof(role_id), NULL);
        statement.set_param_result(1, MYSQL_TYPE_STRING, role_name, sizeof(role_name) - 1, NULL);

        if (0 != statement.get_result())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
	Json::Value body;
        while(0 == statement.fetch_result())
        {
            Json::Value nodebody;
            nodebody["role_id"] = role_id;
            nodebody["role_name"] = role_name;
            body.append(nodebody);
        }
        rspbody["role_list"] = body;
        result = "success";
    }while(false);
    
    
    http_rsp(fd, "query_user_role_rsp", msgid, result, rspbody);
}


void http_del_user_role(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis)
{
    string result;
    Value rspbody;
    
    result = "fail";
    
    do
    {
        if(body.isNull())
        {
            break;
        }
        
        ISVALIDFILED_UINT("company_id");
        ISVALIDFILED_UINT("user_id");
        ISVALIDFILED_UINT("role_id");
      
        uint32_t company_id = body["company_id"].asUInt();
        uint32_t user_id = body["user_id"].asUInt();
        uint32_t role_id = body["role_id"].asUInt();
      
        if(NULL == mysql)
        {
            break;
        }

        string sqlstr = string("delete from `")+get_sub_table_name("t_ids_user_role",company_id)+
	                string("`  where `company_id`=? and  `user_id` = ? and `role_id` = ? ");

        pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }
        statement.set_param_bind(0, company_id);
        statement.set_param_bind(1, user_id);
        statement.set_param_bind(2, role_id);

        if (0 != statement.execute())
        {
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
            break;
        }

        result = "success";
    }while(false);

    http_rsp(fd, "del_user_role_rsp", msgid, result, rspbody);
}























int user_add_title(uint32_t company_id, uint32_t user_id, uint32_t parent_id, string title, MYSQL *mysql)
{
    if(NULL == mysql)
    {
        return -1;
    }

    string sqlstr = "call `user_add_title`(?,?,?,?,@ret_new_node_id);";
    pre_statement statement;
    if (0 != statement.init(mysql, sqlstr))
    {
        return -1;
    }
    statement.set_param_bind(0, company_id);
    statement.set_param_bind(1, user_id);
    statement.set_param_bind(2, parent_id);
    statement.set_param_bind(3, title);

    if (0 != statement.execute())
    {
        return -1;
    }

    //获取新增ID
    int node_id = 0;
    string sqlstr2 = "select @ret_new_node_id;";
    pre_statement statement2;
    if (0 != statement2.init(mysql, sqlstr2))
    {
        return -1;
    }
    if (0 != statement2.query())
    {
        return -1;
    }
    statement2.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&node_id), sizeof(node_id), NULL);
    if (0 != statement2.get_result())
    {
        return -1;
    }
    if (0 != statement2.fetch_result())
    {
        return -1;
    }
    return node_id; 
}

bool make_org_json(uint32_t company_id, Value &root, MYSQL *mysql, int ntype )
{
    if(NULL == mysql || company_id <= 0)
    {
        return false;
    }
    do
    {
        string tbOrg = get_sub_table_name("t_im_org", company_id);
        string tbVcard = get_sub_table_name("t_im_user_vcard", company_id);
        string sqlstr = string(" select `o`.`lft`,`o`.`rgh`, `o`.`node_id`,`o`.`node_type`,`o`.`node_name`,`o`.`title`, ") +
                             string("        `v`.`user_id`,`v`.`email`,`v`.`sex`,`v`.`name`,`v`.`mobile`,`o`.`sort` ") + 
                             string(" from `") + tbOrg + string("` as `o` left join `") + tbVcard + string("`  as `v` on `o`.`user_id` = `v`.`user_id` ") +
                             string(" where `o`.`company_id` = ? order by `o`.`lft` asc"); 

        pre_statement statement;
        if (0 != statement.init(mysql, sqlstr))
        {
            return false;
        }
        statement.set_param_bind(0, company_id);
        if (0 != statement.query())
        {
            return false;
        }

        int32_t lft = 0;
        int32_t rgh = 0;
        int32_t nodeid = 0;
        int32_t nodetype = 0;
        char    nodename[100] = {'\0'};
        char    title[100] = {'\0'};
        int32_t userid = 0;
        char    email[100] = {'\0'};
        int32_t sex = 0;
        char    name[100] = {'\0'};
        char    mobile[32] = {'\0'};
        int32_t sort = 0;
        statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&lft), sizeof(lft), NULL);
        statement.set_param_result(1, MYSQL_TYPE_LONG, (char*)(&rgh), sizeof(rgh), NULL);
        statement.set_param_result(2, MYSQL_TYPE_LONG, (char*)(&nodeid), sizeof(nodeid), NULL);
        statement.set_param_result(3, MYSQL_TYPE_LONG, (char*)(&nodetype), sizeof(nodetype), NULL);
        statement.set_param_result(4, MYSQL_TYPE_STRING, nodename, sizeof(nodename) - 1, NULL);
        statement.set_param_result(5, MYSQL_TYPE_STRING, title, sizeof(title) - 1, NULL);
        statement.set_param_result(6, MYSQL_TYPE_LONG, (char*)(&userid), sizeof(userid), NULL);
        statement.set_param_result(7, MYSQL_TYPE_STRING, email, sizeof(email) - 1, NULL);
        statement.set_param_result(8, MYSQL_TYPE_LONG, (char*)(&sex), sizeof(sex), NULL);
        statement.set_param_result(9, MYSQL_TYPE_STRING, name, sizeof(name) - 1, NULL);
        statement.set_param_result(10, MYSQL_TYPE_STRING, mobile, sizeof(mobile) - 1, NULL);
        statement.set_param_result(11, MYSQL_TYPE_LONG, (char*)(&sort), sizeof(sort) - 1, NULL);
        if (0 != statement.get_result())
        {
            return false;
        }
                        
        if (0 == statement.fetch_result())
        {
            //手动构建公司根节点信息
            if(1 == ntype)
            {
                root["companyId"] = company_id;
                root["nodeID"] = nodeid;
            }
            else
            {
                root["company_id"] = company_id;
                root["node_id"] = nodeid;
            }
            root["name"] = nodename;
            root["update_time"] = (uint32_t)time(NULL);

            int32_t prelft = lft;
            int32_t prergh = rgh;
            int32_t prenodeid = nodeid;
            Value nodebody;
            if (0 == statement.fetch_result())
            {
                bool is_valid = true;
                org_frame_user_json(statement, lft, rgh, nodeid, nodetype, nodename, title,
                        userid, email, sex, name, mobile, sort, is_valid, prelft, prergh, prenodeid, nodebody,ntype);
            }
            root["data"] = nodebody;
        }
        
    }while(false);
    return true;
}

void org_frame_user_json(pre_statement &statement, int32_t &lft, int32_t &rgh, int32_t &node_id,int32_t &nodetype, char *nodename, char *title,
        int32_t &userid, char *email,int32_t &sex, char *name, char *mobile, int32_t &sort,  bool &is_valid, int32_t parent_lft, int32_t parent_rgh, 
        int32_t parent_node_id, Value &body, int ntype)
{
    if(lft == parent_lft && rgh == parent_rgh)
    {
        //根节点
        Value nodebody;
        if(1 == ntype)
        {
            if(node_id != parent_node_id)
            {
                nodebody["parentId"] = parent_node_id;
            }
            nodebody["nodeID"] = node_id;
        }
        else
        {
            if(node_id != parent_node_id)
            {
                nodebody["parent_id"] = parent_node_id;
            }
            nodebody["node_id"] = node_id;
        }
        nodebody["type"] = nodetype;
        nodebody["sort"] = sort;
        if(0 == nodetype)
        {
            nodebody["name"] = nodename;
        }
        else
        {
            nodebody["email"] = email;
            nodebody["gender"] = sex;
            nodebody["name"] = name;
            nodebody["phone"] = mobile;
            nodebody["title"] = title;
            if(1 == ntype)
            {
                nodebody["userID"] = userid;
            }
            else
            {
                nodebody["user_id"] = userid;
            }
        }
        
        parent_node_id = node_id;
        if(0 == statement.fetch_result());
        {
            Value tpbody;
            is_valid = true;
            org_frame_user_json(statement, lft, rgh, node_id, nodetype, nodename, title, 
                    userid, email, sex, name, mobile, sort, is_valid, parent_lft, parent_rgh, parent_node_id, tpbody, ntype);
            nodebody["child"] = tpbody;
        }
        body.append(nodebody);
        return;
    }
    //子节点
    int32_t prelft = parent_lft;
    int32_t prergh = parent_rgh;
    int32_t prenodeid = parent_node_id;
    Value *newnodebody = NULL;

    do
    {
        if(!is_valid)
        {
            if(0 == statement.fetch_result())
            {
                is_valid = true;
            }
            else
            {
                is_valid = false;
                break;
            }
        }

        if(parent_rgh < lft)
        {
            return;
        }
        if((prelft == parent_lft && prergh == parent_rgh) || prergh < lft)
        {
            Value tpbody;
            if(1 == ntype)
            {
                tpbody["parentId"] = parent_node_id;
                tpbody["nodeID"] = node_id;
            }
            else
            {
                tpbody["parent_id"] = parent_node_id;
                tpbody["node_id"] = node_id;
            }
            tpbody["type"] = nodetype;
            tpbody["sort"] = sort;
            if(0 == nodetype)
            {
                tpbody["name"] = nodename;
            }
            else
            {
                tpbody["email"] = email;
                tpbody["gender"] = sex;
                tpbody["name"] = name;
                tpbody["phone"] = mobile;
                tpbody["title"] = title;
                if(1 == ntype)
                {
                    tpbody["userID"] = userid;
                }
                else
                {
                    tpbody["user_id"] = userid;
                }
            }

            Value &tpnewnodebody = body.append(tpbody);
            newnodebody = &tpnewnodebody;

            prelft = lft;
            prergh = rgh;
            prenodeid = node_id;
            is_valid = false;
        }
        else
        {
            
            Value nodebody;
            org_frame_user_json(statement, lft, rgh, node_id, nodetype, nodename, title, 
                    userid, email, sex, name, mobile, sort, is_valid,  prelft, prergh, prenodeid, nodebody, ntype);
            (*newnodebody)["child"] = nodebody;
        }
    }while(true);
}


bool make_all_node_member_json(uint32_t company_id, uint32_t node_id, Value &root, MYSQL *mysql, int ntype)
{
	if (NULL == mysql || company_id <= 0)
	{
		return false;
	}
	do
	{
		string tbOrg = get_sub_table_name("t_im_org", company_id);
		string tbVcard = get_sub_table_name("t_im_user_vcard", company_id);
		string sqlstr = string(" select `o`.`lft`,`o`.`rgh`, `o`.`node_id`,`o`.`node_type`,`o`.`node_name`,`o`.`title`, ") +
			string("        `v`.`user_id`,`v`.`email`,`v`.`sex`,`v`.`name`,`v`.`mobile`,`o`.`sort` ") +
			string(" from (select `node`.`lft`,`node`.`rgh`, `node`.`node_id`, `node`.`user_id`, `node`.`node_type`,`node`.`node_name`,`node`.`title`,`node`.`sort` from `") + tbOrg +
			string("` as `node` join  `") + tbOrg + string("` as `parent` on  `node`.`company_id` = `parent`.`company_id` and ") +
			string(" `node`.`lft` between `parent`.`lft` and `parent`.`rgh` where `parent`.`company_id` = ? and `parent`.`node_id` = ?) as `o`") +
			string(" left join `") + tbVcard + string("`  as `v` on `o`.`user_id` = `v`.`user_id` ") +
			string(" order by `o`.`lft` asc");

		pre_statement statement;
		if (0 != statement.init(mysql, sqlstr))
		{
			return false;
		}
		statement.set_param_bind(0, company_id);
		statement.set_param_bind(1, node_id);
		if (0 != statement.query())
		{
			return false;
		}

		int32_t lft = 0;
		int32_t rgh = 0;
		int32_t nodeid = 0;
		int32_t nodetype = 0;
		char    nodename[100] = { '\0' };
		char    title[100] = { '\0' };
		int32_t userid = 0;
		char    email[100] = { '\0' };
		int32_t sex = 0;
		char    name[100] = { '\0' };
		char    mobile[32] = { '\0' };
		int32_t sort = 0;
		statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&lft), sizeof(lft), NULL);
		statement.set_param_result(1, MYSQL_TYPE_LONG, (char*)(&rgh), sizeof(rgh), NULL);
		statement.set_param_result(2, MYSQL_TYPE_LONG, (char*)(&nodeid), sizeof(nodeid), NULL);
		statement.set_param_result(3, MYSQL_TYPE_LONG, (char*)(&nodetype), sizeof(nodetype), NULL);
		statement.set_param_result(4, MYSQL_TYPE_STRING, nodename, sizeof(nodename) - 1, NULL);
		statement.set_param_result(5, MYSQL_TYPE_STRING, title, sizeof(title) - 1, NULL);
		statement.set_param_result(6, MYSQL_TYPE_LONG, (char*)(&userid), sizeof(userid), NULL);
		statement.set_param_result(7, MYSQL_TYPE_STRING, email, sizeof(email) - 1, NULL);
		statement.set_param_result(8, MYSQL_TYPE_LONG, (char*)(&sex), sizeof(sex), NULL);
		statement.set_param_result(9, MYSQL_TYPE_STRING, name, sizeof(name) - 1, NULL);
		statement.set_param_result(10, MYSQL_TYPE_STRING, mobile, sizeof(mobile) - 1, NULL);
		statement.set_param_result(11, MYSQL_TYPE_LONG, (char*)(&sort), sizeof(sort) - 1, NULL);
		if (0 != statement.get_result())
		{
			return false;
		}

		if (0 == statement.fetch_result())
		{
			int32_t prelft = lft;
			int32_t prergh = rgh;
			int32_t prenodeid = nodeid;
			//Value nodebody;
			if (0 == statement.fetch_result())
			{
				bool is_valid = true;
				node_list_user_json(statement, lft, rgh, nodeid, nodetype, nodename, title,
					userid, email, sex, name, mobile, sort, is_valid, prelft, prergh, prenodeid, root, ntype);
			}
			//root["data"] = nodebody;
		}

	} while (false);
	return true;
}

//生成部门内所有成员并带部门信息的json结构
void node_list_user_json(pre_statement &statement, int32_t &lft, int32_t &rgh, int32_t &node_id, int32_t &nodetype, char *nodename, char *title,
	int32_t &userid, char *email, int32_t &sex, char *name, char *mobile, int32_t &sort, bool &is_valid,
	int32_t parent_lft, int32_t parent_rgh, int32_t parent_node_id, Value &body, int ntype)
{
	if (lft == parent_lft && rgh == parent_rgh)
	{
		//根节点
		Value nodebody;
		if (1 == nodetype)
		{
			nodebody["parent_id"] = parent_node_id;
			nodebody["parent_name"] = nodename;
			nodebody["node_id"] = node_id;
			nodebody["gender"] = sex;
			nodebody["name"] = name;
			nodebody["title"] = title;
			nodebody["user_id"] = userid;
			body.append(nodebody);
		}

		parent_node_id = node_id;
		if (0 == statement.fetch_result());
		{
			is_valid = true;
			node_list_user_json(statement, lft, rgh, node_id, nodetype, nodename, title,
				userid, email, sex, name, mobile, sort, is_valid, parent_lft, parent_rgh, parent_node_id, body, ntype);
		}

		return;
	}
	//子节点
	int32_t prelft = parent_lft;
	int32_t prergh = parent_rgh;
	int32_t prenodeid = parent_node_id;
	do
	{
		if (!is_valid)
		{
			if (0 == statement.fetch_result())
			{
				is_valid = true;
			}
			else
			{
				is_valid = false;
				break;
			}
		}

		if (parent_rgh < lft)
		{
			return;
		}
		if ((prelft == parent_lft && prergh == parent_rgh) || prergh < lft)
		{
			Value nodebody;
			if (1 == nodetype)
			{
				nodebody["parent_id"] = parent_node_id;
				nodebody["parent_name"] = nodename;
				nodebody["node_id"] = node_id;
				nodebody["gender"] = sex;
				nodebody["name"] = name;
				nodebody["title"] = title;
				nodebody["user_id"] = userid;
				body.append(nodebody);
			}
			prelft = lft;
			prergh = rgh;
			prenodeid = node_id;
			is_valid = false;
		}
		else
		{
			node_list_user_json(statement, lft, rgh, node_id, nodetype, nodename, title,
				userid, email, sex, name, mobile, sort, is_valid, prelft, prergh, prenodeid, body, ntype);
		}
	} while (true);
}


int del_node(uint32_t company_id, uint32_t node_id, MYSQL *mysql)
{
    if(NULL == mysql)
    {
        return -1;
    }

    string sqlstr = "call `del_node`(?,?,@ret_del_node);";
    pre_statement statement;
    if (0 != statement.init(mysql, sqlstr))
    {
        return -1;
    }
    statement.set_param_bind(0, company_id);
    statement.set_param_bind(1, node_id);

    if (0 != statement.execute())
    {
        return -1;
    }

    //获取返回值 
    int ret_del_node = 0;
    string sqlstr2 = "select @ret_del_node;";
    pre_statement statement2;
    if (0 != statement2.init(mysql, sqlstr2))
    {
        return -1;
    }
    if (0 != statement2.query())
    {
        return -1;
    }
    statement2.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&ret_del_node), sizeof(ret_del_node), NULL);
    if (0 != statement2.get_result())
    {
        return -1;
    }
    if (0 != statement2.fetch_result())
    {
        return -1;
    }
    if(ret_del_node != 0)
    {
        log(LOG_ERROR, "del_node fail! company_id:%u, node_id:%u ret=%d", company_id, node_id, ret_del_node); 
        return ret_del_node;
    }

    return 0; 
}

int mov_node(uint32_t company_id, uint32_t node_id, uint32_t to_parent_node_id, MYSQL *mysql)
{
    if(NULL == mysql)
    {
        return -1;
    }

    string sqlstr = "call `move_node`(?,?,?,@ret_mov_node);";
    pre_statement statement;
    if (0 != statement.init(mysql, sqlstr))
    {
        return -1;
    }
    statement.set_param_bind(0, company_id);
    statement.set_param_bind(1, node_id);
    statement.set_param_bind(2, to_parent_node_id);

    if (0 != statement.execute())
    {
        return -1;
    }

    //获取返回值 
    int ret_mov_node = 0;
    string sqlstr2 = "select @ret_mov_node;";
    pre_statement statement2;
    if (0 != statement2.init(mysql, sqlstr2))
    {
        return -1;
    }
    if (0 != statement2.query())
    {
        return -1;
    }
    statement2.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&ret_mov_node), sizeof(ret_mov_node), NULL);
    if (0 != statement2.get_result())
    {
        return -1;
    }
    if (0 != statement2.fetch_result())
    {
        return -1;
    }
    if(ret_mov_node != 0)
    {
        log(LOG_ERROR, "mov_node fail! company_id:%u, node_id:%u to_parent_node_id:%u ret=%d", 
                company_id, node_id, to_parent_node_id, ret_mov_node); 
        return ret_mov_node;
    }

    return 0; 
}



void http_req_refresh_company_org(uint32_t company_id, MYSQL *mysql, cache_conn *redis)
{
    Value root;
    bool rt = make_org_json(company_id, root, mysql);
    if(rt)
    {
        Json::FastWriter writer;
        string bodystr = writer.write(root);

        static config_file_oper config_file("server_config.conf");
        static char buf[512];
        int llen = sprintf(buf, "POST /%u HTTP/1.1\r\nContent-Length: %lu\r\nConnection: Keep-Alive\r\nAccept-Encoding: gzip\r\nAccept-Language: zh-CN,en,*\r\nUser-Agent: Mozilla/5.0\r\nHost: %s:%s    \r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n",
                company_id,
                bodystr.length(),
                config_file.get_config_value("http_ip"), 
                config_file.get_config_value("http_port"));
        buf[llen] = '\0';
        string reqstr = string(buf)+bodystr;
        net_server::get_instance()->http_send(reqstr.c_str(), reqstr.length());

        log(LOG_DEBUG, "http_req_refresh_company_org\n%s", reqstr.c_str());

        db_proxy::redis_org_tree(company_id, mysql, redis);

    }
}

void http_rsp(evutil_socket_t &fd, const string &prototype, const string &msgid, const string &result)
{
    Value body;
    http_rsp(fd, prototype, msgid, result, body);
}

void http_rsp(evutil_socket_t &fd, const string &prototype, const string &msgid, const string &result, Value &body)
{
    Value rsp;
    rsp["msgid"] = msgid;
    rsp["protocol_type"] = prototype;
    rsp["result"] = result;
    if(!body.isNull())
    {
        rsp["body"] = body;
    }

    Json::FastWriter writer;
    string rspstr = writer.write(rsp);
    http_response(fd, rspstr);
}

void http_response(evutil_socket_t &fd, string &rsp)
{
    string resp_str("HTTP/1.1 200 OK\r\n");
    resp_str.append("Content-Length:");
    resp_str.append(std::to_string(rsp.length()));
    resp_str.append("\r\n\r\n");

    log(LOG_DEBUG, "[http_response  fd=%u  len=%u]%s", fd, rsp.length() ,rsp.c_str());

    int rt = send(fd, resp_str.c_str(), resp_str.length(), 0);
    if(rt <= 0)
    {
        log(LOG_DEBUG, "response http fail!!  fd=%u rt=%d ", fd, rt);
        return;
    }

    size_t slen = 0;
    do
    {
        rt = send(fd, rsp.c_str()+slen, rsp.length()-slen, 0);
        if(0 == rt)
        {
            break;
        }
        if(-1 == rt)
        {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR))
			{
				continue;
			}
			else
			{
                log(LOG_DEBUG, "response http fail!! fd=%u  rt=%d  slen=%u, error:%s", fd, rt, slen, strerror(errno));
				break;
			}
        }
        slen += rt;
    }while(slen < rsp.length());
}


string get_sub_table_name(string tablename, uint32_t value, uint32_t count)
{
    char buf[32];
    int len = sprintf(buf, "%s_%u", tablename.c_str(),value % count);
    buf[len] = '\0';
    return string(buf);
}

void org_update_notify_req(uint32_t company_id, uint32_t user_id, uint32_t user_opt_type)
{
	IM::DBProxy::IMDBCompanyOrgUpdateReq notifyreq;
	notifyreq.set_company_id(company_id);
	notifyreq.set_user_opt_type(user_opt_type);
	if(0 != user_id)
	{
		notifyreq.add_user_list(user_id);
	}
	
	message notifymsg;
	notifymsg.set_service_id(IM::BaseDefine::SID_DB_PROXY);
	notifymsg.set_cmd_id(IM::BaseDefine::S_CID_DB_ORG_UPDATE_REQ);
	notifymsg.set_pb_length(notifyreq.ByteSize());
	notifymsg.write_msg(&notifyreq);
        net_server::get_instance()->status_send(notifymsg.get_data(), notifyreq.ByteSize() + HEADER_LEN);
}

void org_refresh_redis(cache_conn *redis, uint32_t company_id)
{
	if(NULL == redis)
	{
		return;
	}
	char key[32];
	int len = sprintf(key, "org_%u", company_id);
	key[len] = '\0';
	redis->del(key);

	len = sprintf(key, "cul_%u", company_id);
	key[len] = '\0';
	redis->del(key);
}

