#include "handler_cache.h"
#include "pre_statement.h"
#include "cache_conn.h"
#include "net_server.h"
#include "im_log.h"
#include "../jsoncpp/json/json.h"
using namespace std;
#include "../protobuf/IM.BaseDefine.pb.h"
#include "../protobuf/IM.DBProxy.pb.h"
#include "../protobuf/IM.RedisStruct.pb.h"

namespace db_proxy
{
	void update_user_id(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		log(LOG_DEBUG, "%s:%s()", __FILE__, __FUNCTION__);
		message *msg = static_cast<message *>(ptask->get_msg());
		IM::DBProxy::IMDBCompanyUserListReq msg_data;
		if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
		{
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			return;
		}
		uint32_t company_id = msg_data.company_id();
		IM::DBProxy::DBUpdateInfoResultDef result = IM::DBProxy::DB_UPDATE_DATA_FAIL;
		if (0 == redis_user_id(company_id, mysql, redis))
		{
			result = IM::DBProxy::DB_UPDATE_DATA_SUCCESS;
		}
		IM::DBProxy::IMDBCompanyInfoRsp msg_rsp;
		msg_rsp.set_update_ret(result);
		msg_rsp.set_company_id(company_id);
		msg_rsp.set_opt_id(msg_data.opt_id());
		for (int i = 0; i < msg_data.attach_data_size(); i++)
		{
			msg_rsp.add_attach_data(msg_data.attach_data(i));
		}
		msg->set_cmd_id(IM::BaseDefine::S_CID_DB_COMPANY_USER_LIST_RSP);
		msg->set_pb_length(msg_rsp.ByteSize());
		msg->write_msg(&msg_rsp);
		net_server::get_instance()->add_response(ptask);
	}

	int redis_user_id(uint32_t company_id, MYSQL *mysql, cache_conn *redis)
	{
		int ret = 0;
		pre_statement statement;
		do
		{
			string table_name = string("t_im_user_vcard_") + to_string(company_id % 8);
			string sql = string("select user_id from ") + table_name + string(" where company_id = ?");
			if (0 != statement.init(mysql, sql))
			{
				ret = 1;
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			statement.set_param_bind(0, company_id);
			if (0 != statement.query())
			{
				ret = 1;
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t user_id;
			unsigned long length = sizeof(user_id);
			statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&user_id), length, NULL);
			if (0 != statement.get_result())
			{
				ret = 1;
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			CompanyUserList user_list;
			while (0 == statement.fetch_result())
			{
				user_list.add_user_id(user_id);
			}
			string key = string("cul_") + to_string(company_id);
			log(LOG_DEBUG, "Redis company all user_id %s", key.c_str());
			string value = user_list.SerializePartialAsString();
			if (value.empty())
			{
				ret = 1;
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			}
			if (NULL == redis)
			{
				ret = 1;
			}
			else
			{
				redis->set(key, value);
			}
		} while (false);
		statement.free();
		return ret;
	}

	void update_user_info(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		IM::DBProxy::IMDBUserInfoReq msg_data;
		if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
		{
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			return;
		}
		uint32_t company_id = msg_data.company_id();
		uint32_t user_id = msg_data.user_id();
		log(LOG_DEBUG, "update_user_info user_id:%d", user_id);
		IM::DBProxy::DBUpdateInfoResultDef result = IM::DBProxy::DB_UPDATE_DATA_FAIL;
		if (0 == redis_user_info(company_id, user_id, mysql, redis))
		{
			result = IM::DBProxy::DB_UPDATE_DATA_SUCCESS;
		}
		IM::DBProxy::IMDBUserInfoRsp msg_rsp;
		msg_rsp.set_update_ret(result);
		msg_rsp.set_company_id(company_id);
		msg_rsp.set_user_id(user_id);
		msg_rsp.set_opt_id(msg_data.opt_id());
		msg->set_cmd_id(IM::BaseDefine::S_CID_DB_USER_INFO_RSP);
		msg->set_pb_length(msg_rsp.ByteSize());
		msg->write_msg(&msg_rsp);
		net_server::get_instance()->add_response(ptask);
	}

	int redis_user_info(uint32_t company_id, uint32_t user_id, MYSQL *mysql, cache_conn *redis)
	{
		int ret = 0;
		pre_statement statement;
		do 
		{
			string tbOrg = string("t_im_org_") + to_string(company_id % 8);
			string tbUserVcard = string("t_im_user_vcard_") + to_string(company_id % 8);
			string sql =
				string(" select `node`.`node_id`,`hvnd`.`node_id` as `parent_id`,`hvnd`.`node_name`,`node`.`title`, ") +
				string("        `tbuser`.`work_id`,`tbuser`.`name`,`tbuser`.`english_name`,`tbuser`.`nick_name`, ") +
				string("        `tbuser`.`birthday`,`tbuser`.`sex`,`tbuser`.`email`,`tbuser`.`mobile`, ") +
				string("        `tbuser`.`tel`,`tbuser`.`sign_info`,`tbuser`.`head_img`, ") +
				string("        `tbuser`.`head_update_time`,`tbuser`.`status`,`tbuser`.`update_time` ") +
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
				ret = 1;
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
				ret = 1;
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t node_id;
			unsigned long length = sizeof(node_id);
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
			char name[100] = { '\0' };
			length = sizeof(name);
			statement.set_param_result(5, MYSQL_TYPE_STRING, name, length, NULL);
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
			uint64_t update_time;
			length = sizeof(update_time);
			statement.set_param_result(17, MYSQL_TYPE_LONGLONG, (char*)&update_time, length, NULL);
			if (0 != statement.get_result())
			{
				ret = 1;
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			if (0 != statement.fetch_result())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			UserInfo user_info;
			user_info.set_company_id(company_id);
			user_info.set_user_id(user_id);
			user_info.set_work_id(work_id);
			user_info.set_user_real_name(name);
			user_info.set_user_english_name(english_name);
			user_info.set_user_nick_name(nick_name);
			Json::Value pos_json;
			Json::Value posbody;
			posbody["parent_id"] = parent_id;
			posbody["node_id"] = node_id;
			posbody["department"] = node_name;
			posbody["title"] = title;
			pos_json.append(posbody);
			Json::FastWriter writer;
			string pos_json_temp = writer.write(pos_json);
			user_info.set_pos_json(pos_json_temp);
			user_info.set_user_borndate(birthday);
			user_info.set_user_gender(sex);
			user_info.set_email(email);
			user_info.set_user_mobile(mobile);
			user_info.set_user_tel(tel);
			user_info.set_sign_info(sign_info);
			user_info.set_avatar_url(head_img);
			user_info.set_avatar_update_time(head_update_time);
			user_info.set_status(status);
			user_info.set_update_time(update_time);
			string key = string("ui_") + to_string(company_id) + string("_") + to_string(user_id);
			log(LOG_DEBUG, "Redis user_info %s", key.c_str());
			string value = user_info.SerializePartialAsString();
			if (value.empty())
			{
				ret = 1;
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			}
			if (NULL == redis)
			{
				ret = 1;
			}
			else
			{
				redis->set(key, value);	
			}
		} while (false);
		statement.free();
		return ret;
	}

	void update_user_friend_list(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		log(LOG_DEBUG, "%s:%s()", __FILE__, __FUNCTION__);
		message *msg = static_cast<message *>(ptask->get_msg());
		IM::DBProxy::IMDBUserFriendListReq msg_data;
		if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
		{
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			return;
		}
		uint32_t company_id = msg_data.company_id();
		uint32_t user_id = msg_data.user_id();
		IM::DBProxy::DBUpdateInfoResultDef result = IM::DBProxy::DB_UPDATE_DATA_FAIL;
		if (0 == redis_user_friend_list(company_id, user_id, mysql, redis))
		{
			result = IM::DBProxy::DB_UPDATE_DATA_SUCCESS;
		}
		IM::DBProxy::IMDBUserFriendListRsp msg_rsp;
		msg_rsp.set_company_id(company_id);
		msg_rsp.set_user_id(user_id);
		msg_rsp.set_update_ret(result);
		msg_rsp.set_opt_id(msg_data.opt_id());
		for (int i = 0; i < msg_data.attach_data_size(); i++)
		{
			msg_rsp.add_attach_data(msg_data.attach_data(i));
		}
		msg->set_cmd_id(IM::BaseDefine::S_CID_DB_USER_FRIEND_LIST_RSP);
		msg->set_pb_length(msg_rsp.ByteSize());
		msg->write_msg(&msg_rsp);
		net_server::get_instance()->add_response(ptask);
	}

	int redis_user_friend_list(uint32_t company_id, uint32_t user_id, MYSQL *mysql, cache_conn *redis)
	{
		int ret = 0;
		pre_statement statement;
		do 
		{
			string  table_name = string("t_im_user_friend_") + to_string(company_id % 8);
			string sql = string("select friend_company_id, friend_user_id, is_delete, is_fixtop, fix_order, remark, update_time from ") + table_name + string(" where company_id = ? and user_id = ?");
			if (0 != statement.init(mysql, sql))
			{
				ret = 1;
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			statement.set_param_bind(0, company_id);
			statement.set_param_bind(1, user_id);
			if (0 != statement.query())
			{
				ret = 1;
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t friend_company_id;
			unsigned long length = sizeof(friend_company_id);
			statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&friend_company_id), length, NULL);
			uint32_t friend_user_id;
			length = sizeof(friend_user_id);
			statement.set_param_result(1, MYSQL_TYPE_LONG, (char*)(&friend_user_id), length, NULL);
			uint8_t is_delete;
			length = sizeof(is_delete);
			statement.set_param_result(2, MYSQL_TYPE_TINY, (char*)(&is_delete), length, NULL);
			uint8_t is_fixtop;
			length = sizeof(is_fixtop);
			statement.set_param_result(3, MYSQL_TYPE_TINY, (char*)(&is_fixtop), length, NULL);
			uint8_t fix_order;
			length = sizeof(fix_order);
			statement.set_param_result(4, MYSQL_TYPE_TINY, (char*)(&fix_order), length, NULL);
			char remark[100] = { '\0' };
			length = sizeof(remark);
			statement.set_param_result(5, MYSQL_TYPE_STRING, remark, length, NULL);
			uint64_t update_time;
			length = sizeof(update_time);
			statement.set_param_result(6, MYSQL_TYPE_LONGLONG, (char*)(&update_time), length, NULL);
			if (0 != statement.get_result())
			{
				ret = 1;
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			UserFriendList user_friend_list;
			while (0 == statement.fetch_result())
			{
				UserFriendItem *friend_item = user_friend_list.add_friend_list();
				friend_item->set_company_id(friend_company_id);
				friend_item->set_user_id(friend_user_id);
				friend_item->set_is_delete(is_delete);
				friend_item->set_is_fixtop(is_fixtop);
				friend_item->set_fix_order(fix_order);
				friend_item->set_remark(remark);
				friend_item->set_update_time(update_time);
			}
			string key = string("uf_") + to_string(company_id) + string("_") + to_string(user_id);
			log(LOG_DEBUG, "Redis user_friend_list %s", key.c_str());
			string value = user_friend_list.SerializePartialAsString();
			if (value.empty())
			{
				ret = 1;
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			}
			if (NULL == redis)
			{
				ret = 1;
			}
			else
			{
				redis->set(key, value);	
			}
		} while (false);
		statement.free();
		return ret;
	}

	void update_user_group_list(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		IM::DBProxy::IMDBUserGroupListReq msg_data;
		if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
		{
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			return;
		}
		uint32_t company_id = msg_data.company_id();
		uint32_t user_id = msg_data.user_id();
		log(LOG_DEBUG, "update_user_group_list user_id:%d", user_id);
		IM::DBProxy::DBUpdateInfoResultDef result = IM::DBProxy::DB_UPDATE_DATA_FAIL;
		if (0 == redis_user_group_list(company_id, user_id, mysql, redis))
		{
			result = IM::DBProxy::DB_UPDATE_DATA_SUCCESS;
		}
		IM::DBProxy::IMDBUserGroupListRsp msg_rsp;
		msg_rsp.set_company_id(company_id);
		msg_rsp.set_user_id(user_id);
		msg_rsp.set_update_ret(result);
		msg_rsp.set_opt_id(msg_data.opt_id());
		for (int i = 0; i < msg_data.attach_data_size(); i++)
		{
			msg_rsp.add_attach_data(msg_data.attach_data(i));
		}
		msg->set_cmd_id(IM::BaseDefine::S_CID_DB_USER_GROUP_LIST_RSP);
		msg->set_pb_length(msg_rsp.ByteSize());
		msg->write_msg(&msg_rsp);
		net_server::get_instance()->add_response(ptask);
	}

	int redis_user_group_list(uint32_t company_id, uint32_t user_id, MYSQL *mysql, cache_conn *redis)
	{
		int ret = 0;	
		pre_statement statement;
		do 
		{
			string table_name = string("t_im_user_group_") + to_string(company_id % 8);
			string sql = string("select friend_company_id, friend_group_id, is_delete, group_type, is_fixtop, fix_order, remark, update_time from ") +
				table_name + string(" where company_id = ? and user_id = ?");
			if (0 != statement.init(mysql, sql))
			{
				ret = 1;
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			statement.set_param_bind(0, company_id);
			statement.set_param_bind(1, user_id);
			if (0 != statement.query())
			{
				ret = 1;
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t friend_company_id;
			unsigned long length = sizeof(friend_company_id);
			statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&friend_company_id), length, NULL);
			uint32_t group_id;
			length = sizeof(group_id);
			statement.set_param_result(1, MYSQL_TYPE_LONG, (char*)(&group_id), length, NULL);
			uint8_t is_delete;
			length = sizeof(is_delete);
			statement.set_param_result(2, MYSQL_TYPE_TINY, (char*)(&is_delete), length, NULL);
			uint8_t group_type;
			length = sizeof(group_type);
			statement.set_param_result(3, MYSQL_TYPE_TINY, (char*)(&group_type), length, NULL);
			uint8_t is_fixtop;
			length = sizeof(is_fixtop);
			statement.set_param_result(4, MYSQL_TYPE_TINY, (char*)(&is_fixtop), length, NULL);
			uint8_t fix_order;
			length = sizeof(fix_order);
			statement.set_param_result(5, MYSQL_TYPE_TINY, (char*)(&fix_order), length, NULL);
			char remark[100] = { '\0' };
			length = sizeof(remark);
			statement.set_param_result(6, MYSQL_TYPE_STRING, remark, length, NULL);
			uint64_t update_time;
			length = sizeof(update_time);
			statement.set_param_result(7, MYSQL_TYPE_LONGLONG, (char*)(&update_time), length, NULL);
			if (0 != statement.get_result())
			{
				ret = 1;
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			UserGroupList user_group_list;
			while (0 == statement.fetch_result())
			{
				GroupListItem *group_item = user_group_list.add_group_list();
				group_item->set_company_id(friend_company_id);
				group_item->set_group_id(group_id);
				if (group_type == GROUP_TYPE_NORMAL)
				{
					group_item->set_group_type(GROUP_TYPE_NORMAL);
				}
				else if (group_type == GROUP_TYPE_TMP)
				{
					group_item->set_group_type(GROUP_TYPE_TMP);
				}
				else if (group_type == GROUP_TYPE_ORG)
				{
					group_item->set_group_type(GROUP_TYPE_ORG);
				}
				group_item->set_is_fixtop(is_fixtop);
				group_item->set_fix_order(fix_order);
				group_item->set_remark(remark);
				memset(remark, '\0', 100);
				group_item->set_update_time(update_time);
				group_item->set_is_delete(is_delete);
			}
			string key = string("ugl_") + to_string(company_id) + string("_") + to_string(user_id);
			log(LOG_DEBUG, "Redis user_group_list %s", key.c_str());
			string value = user_group_list.SerializePartialAsString();
			if (value.empty())
			{
				ret = 1;
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			}		
			if (NULL == redis)
			{
				ret = 1;
			}
			else
			{
				redis->set(key, value);			
			}
		} while (false);
		statement.free();
		return ret;
	}

	void update_group_member_list(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		IM::DBProxy::IMDBGroupMemberReq msg_data;
		if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
		{
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			return;
		}
		uint32_t opt_id = msg_data.opt_id();
		IM::DBProxy::DBUpdateInfoResultDef result = IM::DBProxy::DB_UPDATE_DATA_FAIL;
		if (1 == opt_id)
		{
			uint32_t company_id = msg_data.company_id();
			uint32_t group_id = msg_data.group_id();
			log(LOG_DEBUG, "update_group_member_list group_id:%d", group_id);
			if (0 == redis_group_member_list(company_id, group_id, mysql, redis))
			{
				result = IM::DBProxy::DB_UPDATE_DATA_SUCCESS;
			}
		}
		/*
		else if (2 == opt_id)
		{
			uint32_t company_id = msg_data.company_id();
			uint32_t org_id = msg_data.group_id();
			if (0 == redis_org_member_list(company_id, org_id))
			{
				result = IM::DBProxy::DB_UPDATE_DATA_SUCCESS;
			}	
		}
		*/
		IM::DBProxy::IMDBGroupMemberRsp msg_rsp;
		msg_rsp.set_company_id(msg_data.company_id());
		msg_rsp.set_group_id(msg_data.group_id());
		msg_rsp.set_update_ret(result);
		msg_rsp.set_opt_id(msg_data.opt_id());
		for (int i = 0; i < msg_data.attach_data_size(); i++)
		{
			msg_rsp.add_attach_data(msg_data.attach_data(i));
		}
		msg->set_cmd_id(IM::BaseDefine::S_CID_DB_GROUP_MEMBER_RSP);
		msg->set_pb_length(msg_rsp.ByteSize());
		msg->write_msg(&msg_rsp);
		net_server::get_instance()->add_response(ptask);
	}

	int redis_group_member_list(uint32_t company_id, uint32_t group_id, MYSQL *mysql, cache_conn *redis)
	{
		int ret = 0;	
		pre_statement statement;
		do 
		{
			string table_name = string("t_im_group_member_") + to_string(company_id % 8);
			string sql = string("select user_company_id, user_id, is_delete, user_remark, role, update_time from ") +
							  table_name + string(" where group_company_id = ? and group_id = ?");
			if (0 != statement.init(mysql, sql))
			{
				ret = 1;
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			statement.set_param_bind(0, company_id);
			statement.set_param_bind(1, group_id);
			if (0 != statement.query())
			{
				ret = 1;
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t user_company_id;
			unsigned long length = sizeof(user_company_id);
			statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&user_company_id), length, NULL);
			uint32_t user_id;
			length = sizeof(user_id);
			statement.set_param_result(1, MYSQL_TYPE_LONG, (char*)(&user_id), length, NULL);
			uint8_t is_delete;
			length = sizeof(is_delete);
			statement.set_param_result(2, MYSQL_TYPE_TINY, (char*)(&is_delete), length, NULL);
			char remark[100] = { '\0' };
			length = sizeof(remark);
			statement.set_param_result(3, MYSQL_TYPE_STRING, remark, length, NULL);
			uint8_t role;
			length = sizeof(role);
			statement.set_param_result(4, MYSQL_TYPE_TINY, (char*)(&role), length, NULL);
			uint64_t update_time;
			length = sizeof(update_time);
			statement.set_param_result(5, MYSQL_TYPE_LONGLONG, (char*)(&update_time), length, NULL);
			if (0 != statement.get_result())
			{
				ret = 1;
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			GroupMemberList member_list;
			while (0 == statement.fetch_result())
			{
				GroupMemberItem *member_item = member_list.add_member_list();
				member_item->set_company_id(user_company_id);
				member_item->set_member_id(user_id);
				member_item->set_is_delete(is_delete);
				member_item->set_member_remark(remark);
				memset(remark, '\0', 100);
				member_item->set_member_role(role);
				member_item->set_update_time(update_time);
			}
			string key = string("gml_") + to_string(company_id) + string("_") + to_string(group_id);
			log(LOG_DEBUG, "Redis group_member_list %s", key.c_str());
			string value = member_list.SerializePartialAsString();
			if (value.empty())
			{
				ret = 1;
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			}
			if (NULL == redis)
			{
				ret = 1;
			}
			else
			{
				redis->set(key, value);	
			}
		} while (false);
		statement.free();
		return ret;
	}

	void update_org_tree(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		IM::DBProxy::IMDBOrgTreeReq msg_data;
		if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
		{
			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			return;
		}
		uint32_t company_id = msg_data.company_id();
		log(LOG_DEBUG, "update_org_tree company_id:%d", company_id);
		IM::DBProxy::DBUpdateInfoResultDef result = IM::DBProxy::DB_UPDATE_DATA_FAIL;
		if (0 == redis_org_tree(company_id, mysql, redis))
		{
			result = IM::DBProxy::DB_UPDATE_DATA_SUCCESS;
		}
		IM::DBProxy::IMDBOrgTreeRsp msg_rsp;
		msg_rsp.set_update_ret(result);
		msg_rsp.set_company_id(company_id);
		for (int i = 0; i < msg_data.attach_data_size(); i++)
		{
			msg_rsp.add_attach_data(msg_data.attach_data(i));
		}
		msg->set_cmd_id(IM::BaseDefine::S_CID_DB_ORGTREE_RSP);
		msg->set_pb_length(msg_rsp.ByteSize());
		msg->write_msg(&msg_rsp);
		net_server::get_instance()->add_response(ptask);
	}

	int redis_org_tree(uint32_t company_id, MYSQL *mysql, cache_conn *redis)
	{
		int ret = 0;	
		pre_statement statement;
		do
		{
			string table_name = string("t_im_org_") + to_string(company_id % 8);
			string sql = string("select node_id, user_id, lft, rgh, node_type from ") + table_name + string(" where company_id = ? order by lft asc");
			if (0 != statement.init(mysql, sql))
			{
				ret = 1;
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			statement.set_param_bind(0, company_id);
			if (0 != statement.query())
			{
				ret = 1;
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t node_id;
			unsigned long length = sizeof(node_id);
			statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&node_id), length, NULL);
			uint32_t user_id;
			length = sizeof(user_id);
			statement.set_param_result(1, MYSQL_TYPE_LONG, (char*)(&user_id), length, NULL);
			uint32_t lft;
			length = sizeof(lft);
			statement.set_param_result(2, MYSQL_TYPE_LONG, (char*)(&lft), length, NULL);
			uint32_t rgh;
			length = sizeof(rgh);
			statement.set_param_result(3, MYSQL_TYPE_LONG, (char*)(&rgh), length, NULL);
			uint32_t node_type;
			length = sizeof(node_type);
			statement.set_param_result(4, MYSQL_TYPE_LONG, (char*)(&node_type), length, NULL);
			if (0 != statement.get_result())
			{
				ret = 1;
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			OrgTree org_tree;
			while (0 == statement.fetch_result())
			{
				OrgNode *org_node = org_tree.add_org_node_list();
				if (node_type == 0)
				{
					org_node->set_id(node_id);
				}
				else if (node_type == 1)
				{
					org_node->set_id(user_id);
				}
				org_node->set_lvalue(lft);
				org_node->set_rvalue(rgh);
				org_node->set_type(node_type);
			}
			string key = string("org_") + to_string(company_id);
			log(LOG_DEBUG, "Redis org_tree %s", key.c_str());
			string value = org_tree.SerializePartialAsString();
			if (value.empty())
			{
				ret = 1;
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			}	
			if (NULL == redis)
			{
				ret = 1;
			}
			else
			{
				redis->set(key, value);		
			}
		} while (false);
		statement.free();	
		return ret;
	}
}
