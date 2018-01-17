#include "handler_server.h"
#include "handler_cache.h"
#include "pre_statement.h"
#include <sys/time.h>
#include "im_log.h"
#include "net_server.h"
#include "../jsoncpp/json/json.h"
#include "../protobuf/IM.BaseDefine.pb.h"
using namespace IM::BaseDefine;
#include "../protobuf/IM.Server.pb.h"
using namespace IM::Server;
#include "../jsoncpp/json/json.h"
#include "md5.h"
#include "get_atomic.h"

namespace db_proxy
{
	/*
	void get_login_result(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		do
		{
			IM::Server::IMLoginToSqlReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			log(LOG_DEBUG, "get_login_result company_id:%d user_name:%s pwd:%s", msg_data.company_id(), msg_data.user_name().c_str(), msg_data.password().c_str());
			Json::Value json_data;
			json_data["companyID"] = msg_data.company_id();
			uint64_t msgid = get_atomic();
			log(LOG_DEBUG, "get_login_result msg_id:%lld", msgid);
			net_server::get_instance()->insert_map(msgid, ptask);
			json_data["msgid"] = to_string(msgid);
			json_data["protoType"] = "company:userinfo:login:req";
			Json::Value body;
			body["account"] = msg_data.user_name();
			string password1 = msg_data.password() + "59x!T%x3";
			MD5 md5_temp(password1);
			string password2 = "LfAdmPas$2017" + md5_temp.md5();
			MD5 md5(password2);
			body["password"] = md5.md5();
			body["companyCode"] = msg_data.company_id();
			json_data["body"] = body;
			Json::FastWriter writer;
			string json = writer.write(json_data);
			log(LOG_DEBUG, "get_login_result json_data:%s", json.c_str());
			json.append(1, '\0');
			net_server::get_instance()->account_send(json.c_str(), json.length());
		} while (false);
	}
	*/
	
	void get_login_result(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Server::IMLoginToSqlReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t company_id = msg_data.company_id();
			string table_name = string("t_im_user_vcard_") + to_string(company_id % 8);
			string sql = string("select count(*) from ") + table_name + string(" where company_id = ? and user_name = ? and status != 3");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			statement.set_param_bind(0, company_id);
			string name = msg_data.user_name();
			statement.set_param_bind(1, name);
			if (0 != statement.query())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			int num;
			unsigned long length = sizeof(num);
			statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&num), length, NULL);
			if (0 != statement.get_result())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			if (0 != statement.fetch_result())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::Server::IMLoginToSqlRsp msg_rsp;
			msg_rsp.set_company_id(msg_data.company_id());
			msg_rsp.set_user_name(msg_data.user_name());
			msg_rsp.set_password(msg_data.password());
			msg_rsp.set_client_type(msg_data.client_type());
			msg_rsp.set_online_status(msg_data.online_status());
			if (0 == num)
			{
				log(LOG_DEBUG, "Company_%d user_name_%s user_pwd_%s User not exist", msg_data.company_id(), msg_data.user_name().c_str(), msg_data.password().c_str());
				msg_rsp.set_result_code(RESULT_LOGIN_USER_OR_PWS_ERROR);
			}
			else if (1 == num)
			{
				statement.free();
				string sql = string("select user_pwd from ") + table_name + string(" where company_id = ? and user_name = ?");
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				statement.set_param_bind(0, company_id);
				statement.set_param_bind(1, name);
				if (0 != statement.query())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				char user_pwd[64] = { '\0' };
				unsigned long length = sizeof(user_pwd);
				statement.set_param_result(0, MYSQL_TYPE_STRING, user_pwd, length, NULL);
				if (0 != statement.get_result())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				if (0 != statement.fetch_result())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				if (0 == strcmp(user_pwd, msg_data.password().c_str()))
				{
					log(LOG_DEBUG, "Company_%d user_name_%s Login success", company_id, name.c_str());
					msg_rsp.set_result_code(RESULT_LOGIN_SUCCESSED);
					pre_statement statement;
					string table_name = string("t_im_user_vcard_") + to_string(company_id % 8);
					string sql = string("select user_id from ") + table_name + string(" where company_id = ? and user_name = ? ");
					if (0 != statement.init(mysql, sql))
					{
						log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
						break;
					}
					statement.set_param_bind(0, company_id);
					statement.set_param_bind(1, name);
					if (0 != statement.query())
					{
						log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
						break;
					}
					uint32_t user_id;
					unsigned long length = sizeof(user_id);
					statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&user_id), length, NULL);
					if (0 != statement.get_result())
					{
						log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
						break;
					}
					if (0 != statement.fetch_result())
					{
						log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
						break;
					}
					statement.free();
					string tbOrg = string("t_im_org_") + to_string(company_id % 8);
					string tbUserVcard = string("t_im_user_vcard_") + to_string(company_id % 8);
					sql =
						string(" select `node`.`node_id`,`hvnd`.`node_id` as `parent_id`,`hvnd`.`node_name`,`node`.`title`, ") +
						string("        `tbuser`.`work_id`,`tbuser`.`name`,`tbuser`.`english_name`,`tbuser`.`nick_name`, ") +
						string("        `tbuser`.`birthday`,`tbuser`.`sex`,`tbuser`.`email`,`tbuser`.`mobile`, ") +
						string("        `tbuser`.`tel`,`tbuser`.`sign_info`,`tbuser`.`head_img`, ") +
						string("        `tbuser`.`head_update_time`,`tbuser`.`status`,`tbuser`.`pc_offline_time`,`tbuser`.`mobile_offline_time`,`tbuser`.`update_time` ") +
						string(" from   ( SELECT `node`.`node_id`,`node`.`user_id`,`node`.`title`, `node`.`lft`,`node`.`rgh`, count(1)  as `depth` ") +
						string("          FROM `") + tbOrg + string("` AS `node` left join `") + tbOrg + string("` AS `parent` ") +
						string("                ON `parent`.`lft` <= `node`.`lft` and `node`.`rgh` <= `parent`.`rgh` ") +
						string("          WHERE `node`.`company_id` = ? AND `node`.`user_id` = ? ") +
						string("          GROUP BY `node`.`node_id`,`node`.`node_id`,`node`.`user_id`,`node`.`title`, `node`.`lft`,`node`.`rgh` ) AS `node`, ") +
						string("        ( SELECT `node`.`node_id`, `node`.`node_name`, `node`.`lft`,`node`.`rgh`, count(1)   as `depth` ") +
						string("          FROM `") + tbOrg + string("` AS  `node` left join `") + tbOrg + string("` AS `parent` ") +
						string("                ON `parent`.`lft` <= `node`.`lft` and `node`.`rgh` <= `parent`.`rgh` ") +
						string("          WHERE `node`.`company_id` = ?  ") +
						string("          GROUP BY `node`.`node_id`, `node`.`node_name`, `node`.`lft`,`node`.`rgh` ) AS `hvnd`, ") +
						string("         `") + tbUserVcard + string("` AS `tbuser` ") +
						string(" where   `hvnd`.`lft` <`node`.`lft` AND `node`.`rgh` < `hvnd`.`rgh` and `node`.`depth` = `hvnd`.`depth` + 1 AND ") +
						string("        `tbuser`.`company_id` = ? AND `tbuser`.`user_id` = ? ") +
						string(" order by `node`.`lft` ASC ");
					if (0 != statement.init(mysql, sql))
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
					uint32_t node_id;
					length = sizeof(node_id);
					statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&node_id), length, NULL);
					uint32_t parent_id;
					length = sizeof(parent_id);
					statement.set_param_result(1, MYSQL_TYPE_LONG, (char*)(&parent_id), length, NULL);
					char node_name[100] = { '\0' };
					length = sizeof(node_name);
					statement.set_param_result(2, MYSQL_TYPE_STRING, node_name, length, NULL);
					char title[100] = { '\0' };
					length = sizeof(title);
					statement.set_param_result(3, MYSQL_TYPE_STRING, title, length, NULL);
					uint32_t work_id;
					length = sizeof(work_id);
					statement.set_param_result(4, MYSQL_TYPE_LONG, (char*)(&work_id), length, NULL);
					char name_temp[100] = { '\0' };
					length = sizeof(name_temp);
					statement.set_param_result(5, MYSQL_TYPE_STRING, name_temp, length, NULL);
					char english_name[100] = { '\0' };
					length = sizeof(english_name);
					statement.set_param_result(6, MYSQL_TYPE_STRING, english_name, length, NULL);
					char nick_name[100] = { '\0' };
					length = sizeof(nick_name);
					statement.set_param_result(7, MYSQL_TYPE_STRING, nick_name, length, NULL);
					uint32_t birthday;
					length = sizeof(birthday);
					statement.set_param_result(8, MYSQL_TYPE_LONG, (char*)&birthday, length, NULL);
					int sex;
					length = sizeof(sex);
					statement.set_param_result(9, MYSQL_TYPE_LONG, (char*)&sex, length, NULL);
					char email[100] = { '\0' };
					length = sizeof(email);
					statement.set_param_result(10, MYSQL_TYPE_STRING, email, length, NULL);
					char mobile[25] = { '\0' };
					length = sizeof(mobile);
					statement.set_param_result(11, MYSQL_TYPE_STRING, mobile, length, NULL);
					char tel[30] = { '\0' };
					length = sizeof(tel);
					statement.set_param_result(12, MYSQL_TYPE_STRING, tel, length, NULL);
					char sign_info[255] = { '\0' };
					length = sizeof(sign_info);
					statement.set_param_result(13, MYSQL_TYPE_STRING, sign_info, length, NULL);
					char head_img[255] = { '\0' };
					length = sizeof(head_img);
					statement.set_param_result(14, MYSQL_TYPE_STRING, head_img, length, NULL);
					uint64_t head_update_time;
					length = sizeof(head_update_time);
					statement.set_param_result(15, MYSQL_TYPE_LONGLONG, (char*)&head_update_time, length, NULL);
					int status;
					length = sizeof(status);
					statement.set_param_result(16, MYSQL_TYPE_LONG, (char*)&status, length, NULL);
					uint64_t pc_offline_time;
					length = sizeof(pc_offline_time);
					statement.set_param_result(17, MYSQL_TYPE_LONGLONG, (char*)&pc_offline_time, length, NULL);
					uint64_t mobile_offline_time;
					length = sizeof(mobile_offline_time);
					statement.set_param_result(18, MYSQL_TYPE_LONGLONG, (char*)&mobile_offline_time, length, NULL);
					uint64_t update_time;
					length = sizeof(update_time);
					statement.set_param_result(19, MYSQL_TYPE_LONGLONG, (char*)&update_time, length, NULL);
					if (0 != statement.get_result())
					{
						log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
						break;
					}
					if (0 != statement.fetch_result())
					{
						log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
						break;
					}
					msg_rsp.set_pc_last_offline_time(pc_offline_time);
					msg_rsp.set_phone_last_offline_time(mobile_offline_time);
					IM::BaseDefine::UserInfo *user_info = msg_rsp.mutable_user_info();
					user_info->set_company_id(company_id);
					user_info->set_user_id(user_id);
					user_info->set_work_id(work_id);
					user_info->set_user_real_name(name_temp);
					user_info->set_user_english_name(english_name);
					user_info->set_user_nick_name(nick_name);
					Json::Value pos_json;
					Json::Value posbody;
					posbody["parent_id"] = parent_id;
					posbody["node_id"] = node_id;
					posbody["department"] = node_name;
					posbody["title"] = title;
					pos_json.append(posbody);
					Json::FastWriter writer;
					string pos_json_temp = writer.write(pos_json);
					user_info->set_pos_json(pos_json_temp);
					user_info->set_user_borndate(birthday);
					user_info->set_user_gender(sex);
					user_info->set_email(email);
					user_info->set_user_mobile(mobile);
					user_info->set_user_tel(tel);
					user_info->set_sign_info(sign_info);
					user_info->set_avatar_url(head_img);
					user_info->set_avatar_update_time(head_update_time);
					user_info->set_status(status);
					user_info->set_update_time(update_time);
					log(LOG_DEBUG, "get_login_result company_id:%d user_id:%d user_name:%s head:%s", company_id, user_id, name_temp, head_img);
				}
				else
				{
					msg_rsp.set_result_code(RESULT_LOGIN_USER_OR_PWS_ERROR);
					log(LOG_DEBUG, "Company_%d user_name_%s user_pwd_%s Password error", company_id, msg_data.user_name().c_str(), msg_data.password().c_str());
				}
			}
			msg->set_cmd_id(CID_OTHER_REQ_TO_MYSQL_RSP);
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
		} while (false);
		statement.free();	
	}

	void set_modify_pwd(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Server::IMAlterPswdReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t company_id = msg_data.company_id();
			string table_name = string("t_im_user_vcard_") + to_string(company_id % 8);
			string sql = string("select user_pwd from ") + table_name + string(" where company_id = ? and user_id = ?");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			statement.set_param_bind(0, company_id);
			uint32_t user_id = msg_data.user_id();
			statement.set_param_bind(1, user_id);
			if (0 != statement.query())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			char user_pwd[64] = { '\0' };
			unsigned long length = sizeof(user_pwd);
			statement.set_param_result(0, MYSQL_TYPE_STRING, user_pwd, length, &length);
			if (0 != statement.get_result())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			if (0 != statement.fetch_result())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IMAlterPswdRsp msg_rsp;
			OptResultCode result_code = OPT_RESULT_FAIL;
			if (0 == strncmp(user_pwd, msg_data.old_pswd().c_str(), length))
			{
				statement.free();
				string sql = string("update ") + table_name + string(" set user_pwd = ? where company_id = ? and user_id = ?");
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				string pwd = msg_data.new_pswd();
				log(LOG_DEBUG, "set_modify_pwd old_pwd = %s new_pswd = %s", msg_data.old_pswd().c_str(), pwd.c_str());
				statement.set_param_bind(0, pwd);
				statement.set_param_bind(1, company_id);
				statement.set_param_bind(2, user_id);
				if (0 != statement.execute())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				}
				result_code = OPT_RESULT_SUCCESS;
			}
			else
			{
				log(LOG_DEBUG, "set_modify_pwd old_pwd = %s error", msg_data.old_pswd().c_str());
			}
			msg_rsp.set_company_id(company_id);
			msg_rsp.set_user_id(user_id);
			msg_rsp.set_result_code(result_code);
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->set_cmd_id(CID_OTHER_ALTER_PSWD_RSP);
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
		} while (false);
		statement.free();	
	}

	void get_buddy_config(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		log(LOG_DEBUG, "%s:%s()", __FILE__, __FUNCTION__);
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			
			IM::Server::IMGetBuddyConfigReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			string sql("select data from t_im_company_config where company_id = ?");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t company_id = msg_data.company_id();
			statement.set_param_bind(0, company_id);
			uint32_t user_id = msg_data.user_id();
			if (0 != statement.query())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			char *company_data_blob = new char[8 * 1024];
			unsigned long msg_length = 8 * 1024;
			//调用完成之后，length保存结果的长度
			statement.set_param_result(0, MYSQL_TYPE_STRING, company_data_blob, msg_length, &msg_length);
			if (0 != statement.get_result())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			if (0 != statement.fetch_result())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::Server::IMGetBuddyConfigRsp msg_rsp;
			msg_rsp.set_company_id(company_id);
			msg_rsp.set_user_id(user_id);
			msg_rsp.set_company_config(company_data_blob);
			delete company_data_blob;
			statement.free();
			sql = "select data from t_im_user_config where company_id = ? and user_id = ?";
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
			char *user_data_blob = new char[8 * 1024];
			msg_length = 8 * 1024;
			//调用完成之后，length保存结果的长度
			statement.set_param_result(0, MYSQL_TYPE_STRING, user_data_blob, msg_length, &msg_length);
			if (0 != statement.get_result())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			if (0 != statement.fetch_result())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			msg_rsp.set_user_config(user_data_blob);
			delete user_data_blob;
			msg->set_cmd_id(CID_OTHER_GET_BUDDY_CONGFIG_RESPONSE);
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
		} while (false);
		statement.free();
	}

	void set_friend_req(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		log(LOG_DEBUG, "%s:%s()", __FILE__, __FUNCTION__);
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Server::IMOptBuddyStorageReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			string sql("delete from t_im_user_friend_req where company_id = ? and user_id = ? and dest_company_id = ? and dest_id = ?");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t company_id = msg_data.req_company_id();
			statement.set_param_bind(0, company_id);
			uint32_t user_id = msg_data.req_user_id();
			statement.set_param_bind(1, user_id);
			uint32_t dest_company_id = msg_data.dest_company_id();
			statement.set_param_bind(2, dest_company_id);
			uint32_t dest_user_id = msg_data.dest_user_id();
			statement.set_param_bind(3, dest_user_id);
			if (0 != statement.execute())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			}
			statement.free();
			sql = "insert into t_im_user_friend_req (company_id, user_id, dest_company_id, dest_id, req_stat, \
				  	  req_result, remark, create_time) values (?, ?, ?, ?, ?, ?, ?, ?)";
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			statement.set_param_bind(0, company_id);
			statement.set_param_bind(1, user_id);
			statement.set_param_bind(2, dest_company_id);
			statement.set_param_bind(3, dest_user_id);
			uint8_t req_stat = msg_data.req_stat();
			statement.set_param_bind(4, req_stat);
			uint8_t req_result = 1;
			statement.set_param_bind(5, req_result);
			string remark = msg_data.opt_remark();
			statement.set_param_bind(6, remark);
			uint64_t create_time = msg_data.create_time();
			statement.set_param_bind(7, create_time);
			if (0 != statement.execute())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			}
			msg->set_cmd_id(CID_OTHER_BUDDY_OPT_STORA_RSP);
			net_server::get_instance()->add_response(ptask);
		} while (false);
		statement.free();
	}

	void set_friend_req_result(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		log(LOG_DEBUG, "%s:%s()", __FILE__, __FUNCTION__);
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Server::IMBuddyReqInfoStorage msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			string sql("update t_im_user_friend_req set req_result = ? where company_id = ? and user_id = ? \
					   		and dest_company_id = ? and dest_id = ?");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t req_result = msg_data.req_stat();
			statement.set_param_bind(0, req_result);
			uint32_t company_id = msg_data.company_id();
			statement.set_param_bind(1, company_id);
			uint32_t user_id = msg_data.user_id();
			statement.set_param_bind(2, user_id);
			uint32_t dest_company_id = msg_data.dest_company_id();
			statement.set_param_bind(3, dest_company_id);
			uint32_t dest_user_id = msg_data.dest_user_id();
			statement.set_param_bind(4, dest_user_id);
			if (0 != statement.execute())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			}
		} while (false);
		delete ptask;
		statement.free();
	}

	void set_friend_manage(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		do 
		{
			IM::Server::IMBuddyStorageReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::BaseDefine::BuddyOptType opt_type = msg_data.opt_type();
			OptResultCode result = OPT_RESULT_FAIL;
			if (OPT_ADD_FRQUENT_CONTACT == opt_type)
			{
				pre_statement statement;
				uint32_t company_id = msg_data.req_company_id();
				string table_name = string("t_im_user_friend_") + to_string(company_id % 8);
				string sql = string("insert into ") + table_name + string(" (company_id, user_id, friend_company_id, friend_user_id, update_time) values (?, ?, ?, ?, ?) on duplicate key update is_delete = 0, update_time = ?");
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				statement.set_param_bind(0, company_id);
				uint32_t user_id = msg_data.req_user_id();
				statement.set_param_bind(1, user_id);
				uint32_t friend_company_id = msg_data.dest_company_id();
				statement.set_param_bind(2, friend_company_id);
				uint32_t friend_user_id = msg_data.dest_user_id();
				statement.set_param_bind(3, friend_user_id);
				uint64_t update_time = msg_data.update_time();
				statement.set_param_bind(4, update_time);
				statement.set_param_bind(5, update_time);
				log(LOG_DEBUG, "set_friend_manage both add friend user_id:%d add user_id:%d", company_id, user_id, friend_company_id, friend_user_id);
				if (0 != statement.execute())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				}
				redis_user_friend_list(company_id, user_id, mysql, redis);
				statement.free();
				table_name = string("t_im_user_friend_") + to_string(friend_company_id % 8);
				sql = string("insert into ") + table_name + string(" (company_id, user_id, friend_company_id, friend_user_id, update_time) values (?, ?, ?, ?, ?) on duplicate key update is_delete = 0, update_time = ?");
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				statement.set_param_bind(0, friend_company_id);
				statement.set_param_bind(1, friend_user_id);
				statement.set_param_bind(2, company_id);
				statement.set_param_bind(3, user_id);
				statement.set_param_bind(4, update_time);
				statement.set_param_bind(5, update_time);
				log(LOG_DEBUG, "set_friend_manage both add friend user_id:%d add user_id:%d", friend_user_id, user_id);
				if (0 != statement.execute())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				}
				redis_user_friend_list(friend_company_id, friend_user_id, mysql, redis);
				result = OPT_RESULT_SUCCESS;
				statement.free();
			}
			else if (OPT_DEL_FRQUENT_CONTACT == opt_type)
			{
				pre_statement statement;
				uint32_t company_id = msg_data.req_company_id();
				string table_name = string("t_im_user_friend_") + to_string(company_id % 8);
				string sql = string("update ") + table_name + string(" set is_delete = 1, update_time = ? where company_id = ? and user_id = ? and friend_company_id = ? and friend_user_id = ?");
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				uint64_t update_time = msg_data.update_time();
				statement.set_param_bind(0, update_time);
				statement.set_param_bind(1, company_id);
				uint32_t user_id = msg_data.req_user_id();
				statement.set_param_bind(2, user_id);
				uint32_t friend_company_id = msg_data.dest_company_id();
				statement.set_param_bind(3, friend_company_id);
				uint32_t friend_user_id = msg_data.dest_user_id();
				statement.set_param_bind(4, friend_user_id);
				log(LOG_DEBUG, "set_friend_manage both del friend user_id:%d add user_id:%d", user_id, friend_user_id);
				if (0 != statement.execute())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				}
				redis_user_friend_list(company_id, user_id, mysql, redis);
				statement.free();
				table_name = string("t_im_user_friend_") + to_string(friend_company_id % 8);
				sql = string("update ") + table_name + string(" set is_delete = 1, update_time = ? where company_id = ? and user_id = ? and friend_company_id = ? and friend_user_id = ?");
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				statement.set_param_bind(0, update_time);
				statement.set_param_bind(1, friend_company_id);
				statement.set_param_bind(2, friend_user_id);
				statement.set_param_bind(3, company_id);
				statement.set_param_bind(4, user_id);		
				log(LOG_DEBUG, "set_friend_manage both del friend user_id:%d add user_id:%d", friend_user_id, user_id);
				if (0 != statement.execute())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				}
				redis_user_friend_list(friend_company_id, friend_user_id, mysql, redis);
				result = OPT_RESULT_SUCCESS;
				statement.free();
			}
			else if (OPT_SINGLE_ADD_FRQUENT_CONTACT == opt_type)
			{
				pre_statement statement;
				uint32_t company_id = msg_data.req_company_id();
				string table_name = string("t_im_user_friend_") + to_string(company_id % 8);
				string sql = string("insert into ") + table_name + string(" (company_id, user_id, friend_company_id, friend_user_id, update_time) values (?, ?, ?, ?, ?) on duplicate key update is_delete = 0, update_time = ?");
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				statement.set_param_bind(0, company_id);
				uint32_t user_id = msg_data.req_user_id();
				statement.set_param_bind(1, user_id);
				uint32_t friend_company_id = msg_data.dest_company_id();
				statement.set_param_bind(2, friend_company_id);
				uint32_t friend_user_id = msg_data.dest_user_id();
				statement.set_param_bind(3, friend_user_id);
				uint64_t update_time = msg_data.update_time();
				statement.set_param_bind(4, update_time);
				statement.set_param_bind(5, update_time);
				log(LOG_DEBUG, "set_friend_manage single add friend user_id:%d add user_id:%d", user_id, friend_user_id);
				if (0 != statement.execute())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				}
				redis_user_friend_list(company_id, user_id, mysql, redis);
				result = OPT_RESULT_SUCCESS;
				statement.free();
			}
			msg->set_cmd_id(CID_OTHER_BUDDY_STORAGE_RSP);
			msg_data.set_result(result);
			msg->set_pb_length(msg_data.ByteSize());
			msg->write_msg(&msg_data);
			net_server::get_instance()->add_response(ptask);
		} while (false);
		
	}

	void set_add_recent_chat(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Server::IMRecentContactStorageReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			for (int i = 0; i < msg_data.recent_contact_size(); i++)
			{
				IM::Server::RecentContactItem recent_info = msg_data.recent_contact(i);
				uint32_t company_id = msg_data.req_company_id();
				string table_name = string("t_im_user_recent_chat_") + to_string(company_id % 8);
				string sql = string("insert into ") + table_name + string(" (company_id, user_id, chat_company_id, chat_user_id, session_type, update_time) values (?, ?, ?, ?, ?, ?) on duplicate key update update_time = ?");
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				statement.set_param_bind(0, company_id);
				uint32_t user_id = msg_data.req_user_id();
				statement.set_param_bind(1, user_id);
				uint32_t chat_company_id = recent_info.company_id();
				statement.set_param_bind(2, chat_company_id);
				uint32_t chat_user_id = recent_info.user_id();
				statement.set_param_bind(3, chat_user_id);
				uint8_t session_type = 1;
				statement.set_param_bind(4, session_type);
				uint64_t update_time = recent_info.update_time();
				statement.set_param_bind(5, update_time);
				statement.set_param_bind(6, update_time);
				log(LOG_DEBUG, "set_add_recent_chat user_id:%d add user_id:%d", user_id, chat_user_id);
				if (0 != statement.execute())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				}
			}
			msg->set_cmd_id(CID_OTHER_RECENT_CONTACT_STORAGE_RSP);
			net_server::get_instance()->add_response(ptask);
		} while (false);
		statement.free();
		
	}

	void set_del_recent_chat(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		log(LOG_DEBUG, "%s:%s()", __FILE__, __FUNCTION__);
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Server::IMDelRecentContactReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::BaseDefine::BuddyOptType opt_type = msg_data.opt_type();
			uint32_t company_id = msg_data.req_company_id();
			string table_name = string("t_im_user_recent_chat_") + to_string(company_id % 8);
			string sql = string("delete from ") + table_name + string(" where company_id = ? and user_id = ? and chat_company_id = ? and chat_user_id = ? and session_type = ?");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			statement.set_param_bind(0, company_id);
			uint32_t user_id = msg_data.req_user_id();
			statement.set_param_bind(1, user_id);
			uint32_t dest_company_id = msg_data.dest_company_id();
			statement.set_param_bind(2, dest_company_id);
			uint32_t dest_user_id = msg_data.dest_user_id();
			statement.set_param_bind(3, dest_user_id);
			if (OPT_DEL_RECENT_CONTACT == opt_type)
			{
				uint8_t session_type = 1;
				statement.set_param_bind(4, session_type);
			}
			else if (OPT_DEL_RECENT_CONTACT_GROUP == opt_type)
			{
				uint8_t session_type = 2;
				statement.set_param_bind(4, session_type);
			}
			if (0 != statement.execute())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			}
		} while (false);
		delete ptask;
		statement.free();
	}

	void set_online_report(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do
		{
			IM::Server::IMReportOnlineInfo msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			string sql = string("insert into t_im_online_report (company_id, node_id, online_count, online_peek) values (?, ?, ?, ?)");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t company_id = msg_data.company_id();
			statement.set_param_bind(0, company_id);
			uint32_t node_id = msg_data.node_id();
			statement.set_param_bind(1, node_id);
			uint32_t online_count = msg_data.online_count();
			statement.set_param_bind(2, online_count);
			uint32_t online_peek = msg_data.online_peek();
			statement.set_param_bind(3, online_peek);
			if (0 != statement.execute())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			}
			log(LOG_DEBUG, "set_online_report company_id:%d node_id:%d online_count:%d online_peek:%d", company_id, node_id, online_count, online_peek);
		} while (false);
		delete ptask;
		statement.free();
	}

	void set_msg_report(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do
		{
			IM::Server::IMReportChatInfo msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			string sql = string("insert into t_im_msg_report (company_id, session_type, node_id, online_count, offline_count) values (?, ?, ?, ?, ?)");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t company_id = msg_data.company_id();
			statement.set_param_bind(0, company_id);
			uint8_t session_type;
			if (msg_data.session_type() == SESSION_TYPE_SINGLE)
			{
				session_type = 1;
			}
			else if (msg_data.session_type() == SESSION_TYPE_MULTICHAT)
			{
				session_type = 2;
			}
			else if (msg_data.session_type() == SESSION_TYPE_ORGGROUP)
			{
				session_type = 3;
			}
			statement.set_param_bind(1, session_type);
			uint32_t node_id = msg_data.node_id();
			statement.set_param_bind(2, node_id);
			uint32_t online_count = msg_data.online_count();
			statement.set_param_bind(3, online_count);
			uint32_t offline_count = msg_data.offline_count();
			statement.set_param_bind(4, offline_count);
			if (0 != statement.execute())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			}
			log(LOG_DEBUG, "set_msg_report company_id:%d session_type:%d node_id:%d online_count:%d offline_count:%d", company_id, session_type, node_id, online_count, offline_count);
		} while (false);
		delete ptask;
		statement.free();
	}

	void get_user_role(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do
		{
			IM::Server::IMGetUserRoleReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t company_id = msg_data.company_id();
			string table_a = string("t_ids_user_role_") + to_string(company_id % 8);
			string sql = string("select role_id from ") + table_a + string(" where company_id = ? and user_id = ?");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			statement.set_param_bind(0, company_id);
			uint32_t user_id = msg_data.user_id();
			statement.set_param_bind(1, user_id);
			if (0 != statement.query())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t role_id;
			unsigned long length = sizeof(role_id);
			statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&role_id), length, NULL);
			if (0 != statement.get_result())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::Server::IMUserRoleAuthorityRsp msg_rsp;
			while (0 == statement.fetch_result())
			{
				IMUserRole *role = msg_rsp.add_role();
				role->set_role_id(role_id);
				pre_statement state;
				string table_b = string("t_ids_role_dict_") + to_string(company_id % 8);
				string table_c = string("t_ids_authority_value_") + to_string(company_id % 8);
				string sql = string("select b.role_name, c.auth_id, c.auth_value, c.ext_value1, c.ext_value2, c.ext_value3, c.ext_value4 from ") + table_b + 
					string(" as b, ") + table_c + string(" as c where b.company_id = ? and b.role_id = ? and c.company_id = b.company_id and c.role_id = b.role_id");
				if (0 != state.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				state.set_param_bind(0, company_id);
				state.set_param_bind(1, role_id);
				if (0 != state.query())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				char role_name[200] = { '\0' };
				length = sizeof(role_name);
				state.set_param_result(0, MYSQL_TYPE_STRING, role_name, length, NULL);
				uint32_t auth_id;
				length = sizeof(auth_id);
				state.set_param_result(1, MYSQL_TYPE_LONG, (char*)(&auth_id), length, NULL);
				char auth_value[100] = { '\0' };
				length = sizeof(auth_value);
				state.set_param_result(2, MYSQL_TYPE_STRING, auth_value, length, NULL);
				char ext_value1[100] = { '\0' };
				length = sizeof(ext_value1);
				state.set_param_result(3, MYSQL_TYPE_STRING, ext_value1, length, NULL);
				char ext_value2[100] = { '\0' };
				length = sizeof(ext_value2);
				state.set_param_result(4, MYSQL_TYPE_STRING, ext_value2, length, NULL);
				char ext_value3[100] = { '\0' };
				length = sizeof(ext_value3);
				state.set_param_result(5, MYSQL_TYPE_STRING, ext_value3, length, NULL);
				char ext_value4[100] = { '\0' };
				length = sizeof(ext_value4);
				state.set_param_result(6, MYSQL_TYPE_STRING, ext_value4, length, NULL);
				if (0 != state.get_result())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				while (0 == state.fetch_result())
				{
					role->set_role_name(role_name);
					IMUserAuthority *authority = role->add_authority();
					authority->set_auth_id(auth_id);
					authority->set_auth_value(auth_value);
					authority->set_ext_value1(ext_value1);
					authority->set_ext_value2(ext_value2);
					authority->set_ext_value3(ext_value3);
					authority->set_ext_value4(ext_value4);
					log(LOG_DEBUG, "get_user_role company_id:%d user_id:%d role_id:%d role_name:%s auth_id:%d auth_value:%s", 
						  company_id, user_id, role_id, role_name, auth_id, auth_value);
				}
			}
			msg->set_cmd_id(CID_OTHER_GET_USER_ROLE_RSP);
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
		} while (false);
		statement.free();
	}
}