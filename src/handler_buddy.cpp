#include "handler_buddy.h"
#include "handler_cache.h"
#include "pre_statement.h"
#include <sys/time.h>
#include "im_log.h"
#include "net_server.h"
#include "../jsoncpp/json/json.h"
#include "../protobuf/IM.BaseDefine.pb.h"
using namespace IM::BaseDefine;
#include "../protobuf/IM.Buddy.pb.h"
#include "pb_task.h"

namespace db_proxy
{
	/*
	void get_recent_chat(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		log(LOG_DEBUG, "%s:%s()", __FILE__, __FUNCTION__);
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Buddy::IMRecentContactReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t company_id = msg_data.company_id();
			string  table_name = string("t_im_user_recent_chat_") + to_string(company_id % 8);
			string table_name_temp = string("t_im_user_vcard_") + to_string(company_id % 8);
			string sql = string("select a.chat_company_id, a.chat_user_id, a.session_type, a.is_fixtop, a.fix_order, a.remark, a.update_time, b.name, b.sex from ") + table_name +
							  string(" as a, ") + table_name_temp + string(" as b where a.company_id = ? and a.user_id = ? and a.chat_company_id = b.company_id and a.chat_user_id = b.user_id \
							  and a.update_time > ? order by a.update_time asc limit 0, 100");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			statement.set_param_bind(0, company_id);
			uint32_t user_id = msg_data.user_id();
			statement.set_param_bind(1, user_id);
			uint64_t update_time = msg_data.update_time();
			statement.set_param_bind(2, update_time);
			if (0 != statement.query())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t to_company_id;
			unsigned long length = sizeof(to_company_id);
			statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&to_company_id), length, NULL);
			uint32_t to_user_id;
			length = sizeof(to_user_id);
			statement.set_param_result(1, MYSQL_TYPE_LONG, (char*)(&to_user_id), length, NULL);
			int session_type;
			length = sizeof(session_type);
			statement.set_param_result(2, MYSQL_TYPE_LONG, (char*)(&session_type), length, NULL);
			int is_fixtop;
			length = sizeof(is_fixtop);
			statement.set_param_result(3, MYSQL_TYPE_LONG, (char*)(&is_fixtop), length, NULL);
			int fix_order;
			length = sizeof(fix_order);
			statement.set_param_result(4, MYSQL_TYPE_LONG, (char*)(&fix_order), length, NULL);
			char remark[100] = { '\0' };
			length = sizeof(remark);
			statement.set_param_result(5, MYSQL_TYPE_STRING, remark, length, NULL);
			uint64_t update_time_temp;
			length = sizeof(update_time_temp);
			statement.set_param_result(6, MYSQL_TYPE_LONGLONG, (char*)(&update_time_temp), length, NULL);
			char name[100] = { '\0' };
			length = sizeof(name);
			statement.set_param_result(7, MYSQL_TYPE_STRING, name, length, NULL);
			uint8_t sex;
			length = sizeof(sex);
			statement.set_param_result(8, MYSQL_TYPE_TINY, (char*)(&sex), length, NULL);
			if (0 != statement.get_result())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::Buddy::IMRecentContactRsp msg_rsp;
			uint32_t is_more_data = 0;
			if (statement.get_num_rows() == 100)
			{
				is_more_data = 1;
			}
			while (0 == statement.fetch_result())
			{
				IM::BaseDefine::RecentContactInfo *contact_info_list = msg_rsp.add_contact_info_list();
				contact_info_list->set_company_id(to_company_id);
				contact_info_list->set_contact_id(to_user_id);
				if (1 == session_type)
				{
					contact_info_list->set_session_type(SESSION_TYPE_SINGLE);
				}
				else if (2 == session_type)
				{
					contact_info_list->set_session_type(SESSION_TYPE_MULTICHAT);
				}
				else if (3 == session_type)
				{
					contact_info_list->set_session_type(SESSION_TYPE_ORGGROUP);
				}
				contact_info_list->set_contact_info_update_time(update_time_temp);
				contact_info_list->set_contact_name(name);
				contact_info_list->set_user_gender(sex);
				contact_info_list->set_contact_remark(remark);
				contact_info_list->set_is_fixtop(is_fixtop);
				contact_info_list->set_top_order(fix_order);
				msg_rsp.set_update_time(update_time_temp);
			}
			msg_rsp.set_is_more_data(is_more_data);
			msg->set_cmd_id(CID_BUDDY_LIST_RECENT_CONTACT_LIST_RESPONSE);
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
		} while (false);
		statement.free();
	}

	void set_recent_chat(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		log(LOG_DEBUG, "%s:%s()", __FILE__, __FUNCTION__);
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Buddy::IMUpdateRecentContactReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t company_id = msg_data.company_id();
			string  table_name = string("t_im_user_recent_chat_") + to_string(company_id % 8);
			bool is_delete = msg_data.isdelete();
			if (0 == is_delete)
			{
				string sql = string("insert into ") + table_name + string("(company_id, user_id, chat_company_id, chat_user_id, session_type, update_time) values (?, ?, ?, ?, ?, ?) on duplicate key update update_time = ?");
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				statement.set_param_bind(0, company_id);
				uint32_t user_id = msg_data.user_id();
				statement.set_param_bind(1, user_id);
				uint32_t chat_company_id = msg_data.contact_company_id();
				statement.set_param_bind(2, chat_company_id);
				uint32_t chat_user_id = msg_data.contact_id();
				statement.set_param_bind(3, chat_user_id);
				IM::BaseDefine::SessionType session_type = msg_data.session_type();
				uint32_t session = 0;
				if (SESSION_TYPE_SINGLE == session_type)
				{
					session = 1;
				}
				else if (SESSION_TYPE_MULTICHAT == session_type)
				{
					session = 2;
				}
				else if (SESSION_TYPE_ORGGROUP == session_type)
				{
					session = 3;
				}
				statement.set_param_bind(4, session);
				struct timeval tv;
				gettimeofday(&tv, NULL);
				uint64_t update_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
				statement.set_param_bind(5, update_time);
				statement.set_param_bind(6, update_time);
				if (0 != statement.execute())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				}
			}
			else
			{
				string sql = string("delete from ") + table_name + string(" where company_id = ? and user_id = ? and chat_company_id = ? and chat_user_id =? and session_type = ?");
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				statement.set_param_bind(0, company_id);
				uint32_t user_id = msg_data.user_id();
				statement.set_param_bind(1, user_id);
				uint32_t chat_company_id = msg_data.contact_company_id();
				statement.set_param_bind(2, chat_company_id);
				uint32_t chat_user_id = msg_data.contact_id();
				statement.set_param_bind(3, chat_user_id);
				IM::BaseDefine::SessionType session_type = msg_data.session_type();
				uint32_t session = 0;
				if (SESSION_TYPE_SINGLE == session_type)
				{
					session = 1;
				}
				else if (SESSION_TYPE_MULTICHAT == session_type)
				{
					session = 2;
				}
				else if (SESSION_TYPE_ORGGROUP == session_type)
				{
					session = 3;
				}
				statement.set_param_bind(4, session);
				if (0 != statement.execute())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				}
			}
		} while (false);
		delete ptask;
		statement.free();
	}
	*/

	void set_recent_chat(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do
		{
			IM::Buddy::IMListItemSetReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t company_id = msg_data.company_id();
			string table_name = string("t_im_user_recent_chat_") + to_string(company_id % 8);
			uint32_t user_id = msg_data.user_id();
			uint32_t dest_company_id = msg_data.dest_company_id();
			uint32_t dest_user_id = msg_data.dest_user_id();
			IM::BaseDefine::ListItemOptType type = msg_data.type();
			if (type = LIST_ITEM_FIXTOP)
			{
				string sql = string("insert into ") + table_name + string("(company_id, user_id, chat_company_id, chat_user_id, session_type, order_value, update_time) \
								  values (?, ?, ?, ?, ?, ?) on duplicate key update order_value = ?, update_time = ?");
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				statement.set_param_bind(0, company_id);
				statement.set_param_bind(1, user_id);
				statement.set_param_bind(2, dest_company_id);
				statement.set_param_bind(3, dest_user_id);
				uint8_t session_type = 1;
				statement.set_param_bind(4, session_type);
				uint32_t order_value = time(NULL);
				statement.set_param_bind(5, order_value);
				struct timeval tv;
				gettimeofday(&tv, NULL);
				uint64_t update_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
				statement.set_param_bind(6, update_time);
				statement.set_param_bind(7, order_value);
				statement.set_param_bind(8, update_time);
				if (0 != statement.execute())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
			}
			else if (type == LIST_ITEM_UNFIXTOP)
			{
				string sql = string("delete from ") + table_name + string(" where company_id = ? and user_id = ? and chat_company_id = ? and chat_user_id =?");
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				statement.set_param_bind(0, company_id);
				statement.set_param_bind(1, user_id);
				statement.set_param_bind(2, dest_company_id);
				statement.set_param_bind(3, dest_user_id);
				if (0 != statement.execute())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
			}
			IM::Buddy::IMListItemSetRsp msg_rsp;
			msg->set_cmd_id(CID_GROUP_LISTITEM_SET_RSP);
			msg_rsp.set_company_id(dest_company_id);
			msg_rsp.set_user_id(dest_user_id);
			msg_rsp.set_result_code(OPT_RESULT_SUCCESS);
			msg_rsp.set_type(type);
			msg_rsp.set_session_type(msg_data.session_type());
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
		} while (false);
		statement.free();
	}

	void get_frequent_chat(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Buddy::IMFrequentContactReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t company_id = msg_data.company_id();
			string  table_name = string("t_im_user_friend_") + to_string(company_id % 8);
			string  table_name_temp = string("t_im_user_vcard_") + to_string(company_id % 8);
			string sql = string("select a.friend_company_id, a.friend_user_id, a.is_delete, a.remark, a.update_time, b.name, b.sex from ") + table_name +
							  string(" as a, ") + table_name_temp + string(" as b where a.company_id = ? and a.user_id = ? and a.friend_company_id = b.company_id and a.friend_user_id = b.user_id \
							  and a.update_time > ? order by a.update_time asc limit 0, 100");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			statement.set_param_bind(0, company_id);
			uint32_t user_id = msg_data.user_id();
			statement.set_param_bind(1, user_id);
			uint64_t update_time = msg_data.update_time();
			statement.set_param_bind(2, update_time);
			if (0 != statement.query())
			{
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
			char remark[100] = { '\0' };
			length = sizeof(remark);
			statement.set_param_result(3, MYSQL_TYPE_STRING, remark, length, NULL);
			uint64_t update_time_temp;
			length = sizeof(update_time_temp);
			statement.set_param_result(4, MYSQL_TYPE_LONGLONG, (char*)(&update_time_temp), length, NULL);
			char name[100] = { '\0' };
			length = sizeof(name);
			statement.set_param_result(5, MYSQL_TYPE_STRING, name, length, NULL);
			uint8_t sex;
			length = sizeof(sex);
			statement.set_param_result(6, MYSQL_TYPE_TINY, (char*)(&sex), length, NULL);
			if (0 != statement.get_result())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::Buddy::IMFrequentContactRsp msg_rsp;
			uint32_t is_more_data = 0;
			unsigned long long num = statement.get_num_rows();
			log(LOG_DEBUG, "get_friend_list user_id:%d num:%d update_time:%lld", user_id, num, update_time);
			if (num == 100)
			{
				is_more_data = 1;
			}
			while (0 == statement.fetch_result())
			{
				IM::BaseDefine::FrequentContactInfo *frequent_info_list = msg_rsp.add_frequent_baseinfo_list();
				frequent_info_list->set_company_id(friend_company_id);
				frequent_info_list->set_contact_id(friend_user_id);
				frequent_info_list->set_is_delete(is_delete);
				frequent_info_list->set_contact_name(name);
				frequent_info_list->set_user_gender(sex);
				frequent_info_list->set_contact_nick_name(remark);
				frequent_info_list->set_last_update_time(update_time_temp);
				msg_rsp.set_update_time(update_time_temp);
				log(LOG_DEBUG, "get_friend_list user_id:%d friend_user_id:%d name:%s is_delete:%d update_time:%lld", user_id, friend_user_id, name, is_delete, update_time_temp);
			}
			msg_rsp.set_is_more_data(is_more_data);
			msg->set_cmd_id(CID_BUDDY_LIST_FREQUENT_CONTACT_LIST_RESPONSE);
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
		} while (false);
		statement.free();
	}

	void get_single_user_info(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Buddy::IMSingleUserInfoReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			int num = msg_data.user_id_list_size();
			IM::Buddy::IMSingleUserInfoRsp msg_rsp;
			for (int i = 0; i < num; i++)
			{
				IM::BaseDefine::UserIdItem user_item = msg_data.user_id_list(i);
				uint32_t company_id = user_item.company_id();
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

				pre_statement statement;
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				statement.set_param_bind(0, company_id);
				uint32_t user_id = user_item.user_id();
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
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				if (0 != statement.fetch_result())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d user_id:%d", __FILE__, __FUNCTION__, __LINE__, user_id);
					break;
				}
				log(LOG_DEBUG, "get_single_user_info user_id:%d name:%s", user_id, name);
				IM::BaseDefine::UserInfo *user_info = msg_rsp.add_user_info_list();
				user_info->set_company_id(company_id);
				user_info->set_user_id(user_id);
				user_info->set_work_id(work_id);
				user_info->set_user_real_name(name);
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
			}
			msg->set_cmd_id(CID_BUDDY_LIST_USER_INFO_RESPONSE);
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
		} while (false);
		statement.free();
	}

	void set_user_info(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Buddy::IMModifyUserInfoReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t company_id = msg_data.from_company_id();
			string table_name = string("t_im_user_vcard_") + to_string(company_id % 8);
			string sql = string("update ") + table_name + string(" set name = ?, english_name = ?, nick_name = ?, birthday = ?, sex = ?, email = ?, mobile = ?, \
							  tel = ?, sign_info = ?, head_img = ?, head_update_time = ?, update_time = ? where company_id = ? and user_id = ? ");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			UserInfo user_info = msg_data.user_info();
			//uint32_t work_id = user_info.work_id();
			//statement.set_param_bind(0, work_id);
			string name = user_info.user_real_name();
			statement.set_param_bind(0, name);
			string english_name = user_info.user_english_name();
			statement.set_param_bind(1, english_name);
			string nick_name = user_info.user_nick_name();
			statement.set_param_bind(2, nick_name);
			uint32_t birthday = user_info.user_borndate();
			statement.set_param_bind(3, birthday);
			uint32_t sex = user_info.user_gender();
			statement.set_param_bind(4, sex);
			string email = user_info.email();
			statement.set_param_bind(5, email);
			string mobile = user_info.user_mobile();
			statement.set_param_bind(6, mobile);
			string tel = user_info.user_tel();
			statement.set_param_bind(7, tel);
			string sign_info = user_info.sign_info();
			statement.set_param_bind(8, sign_info);
			string head_img = user_info.avatar_url();
			statement.set_param_bind(9, head_img);
			uint64_t head_img_time = user_info.avatar_update_time();
			statement.set_param_bind(10, head_img_time);
			struct timeval tv;
			gettimeofday(&tv, NULL);
			uint64_t update_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
			statement.set_param_bind(11, update_time);
			statement.set_param_bind(12, company_id);
			uint32_t user_id = msg_data.from_user_id();
			statement.set_param_bind(13, user_id);
			int ret = statement.execute();
			log(LOG_DEBUG, "set_user_info user_id:%d nick_name:%s email:%s mobile:%s head_img:%s", user_id, nick_name, email, mobile, head_img.c_str());
			OptResultCode result = OPT_RESULT_FAIL;
			if (0 == ret)
			{
				result = OPT_RESULT_SUCCESS;
			}
			else
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			}
			IM::Buddy::IMModifyUserInfoRsp msg_rsp;
			msg_rsp.set_result(result);
			msg_rsp.set_update_time(update_time);
			msg->set_cmd_id(CID_BUDDY_LIST_MODIFY_USER_INFO_RESPONSE);
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
			redis_user_info(company_id, user_id, mysql, redis);
		} while (false);
		statement.free();
	}

	void get_all_user_info(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Buddy::IMAllUserReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t company_id = msg_data.company_id();
			string table_name = string("t_im_user_vcard_") + to_string(company_id % 8);
			string table_name_temp = string("t_im_user_friend_") + to_string(company_id % 8);
			string sql = string("select a.company_id, a.user_id, a.work_id, a.name, a.english_name, a.nick_name, a.birthday, a.sex, a.email, \
								a.mobile, a.tel, a.sign_info, a.head_img, a.head_update_time, a.status, a.update_time from ") + table_name + string(" as a, ") + table_name_temp +
								string(" as b where a.company_id = b.friend_company_id and a.user_id = b.friend_user_id and b.company_id = ? and b.user_id = ? and b.update_time > ? \
								and b.is_delete = 0 order by b.update_time asc limit 0, 100");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			statement.set_param_bind(0, company_id);
			uint32_t user_id = msg_data.user_id();
			statement.set_param_bind(1, user_id);
			uint64_t update_time = msg_data.update_time();
			statement.set_param_bind(2, update_time);
			if (0 != statement.query())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t friend_company_id;
			unsigned long length = sizeof(friend_company_id);
			statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&friend_company_id), length, NULL);
			uint32_t friend_user_id;
			length = sizeof(friend_user_id);
			statement.set_param_result(1, MYSQL_TYPE_LONG, (char*)(&friend_user_id), length, NULL);
			uint32_t work_id;
			length = sizeof(work_id);
			statement.set_param_result(2, MYSQL_TYPE_LONG, (char*)(&work_id), length, NULL);
			char name[100] = { '\0' };
			length = sizeof(name);
			statement.set_param_result(3, MYSQL_TYPE_STRING, name, length, NULL);
			char english_name[100] = { '\0' };
			length = sizeof(english_name);
			statement.set_param_result(4, MYSQL_TYPE_STRING, english_name, length, NULL);
			char nick_name[100] = { '\0' };
			length = sizeof(nick_name);
			statement.set_param_result(5, MYSQL_TYPE_STRING, nick_name, length, NULL);
			uint32_t birthday;
			length = sizeof(birthday);
			statement.set_param_result(6, MYSQL_TYPE_LONG, (char*)&birthday, length, NULL);
			int sex;
			length = sizeof(sex);
			statement.set_param_result(7, MYSQL_TYPE_LONG, (char*)&sex, length, NULL);
			char email[100] = { '\0' };
			length = sizeof(email);
			statement.set_param_result(8, MYSQL_TYPE_STRING, email, length, NULL);
			char mobile[25] = { '\0' };
			length = sizeof(mobile);
			statement.set_param_result(9, MYSQL_TYPE_STRING, mobile, length, NULL);
			char tel[30] = { '\0' };
			length = sizeof(tel);
			statement.set_param_result(10, MYSQL_TYPE_STRING, tel, length, NULL);
			char sign_info[255] = { '\0' };
			length = sizeof(sign_info);
			statement.set_param_result(11, MYSQL_TYPE_STRING, sign_info, length, NULL);
			char head_img[255] = { '\0' };
			length = sizeof(head_img);
			statement.set_param_result(12, MYSQL_TYPE_STRING, head_img, length, NULL);
			uint64_t head_update_time;
			length = sizeof(head_update_time);
			statement.set_param_result(13, MYSQL_TYPE_LONGLONG, (char*)&head_update_time, length, NULL);
			int status;
			length = sizeof(status);
			statement.set_param_result(14, MYSQL_TYPE_LONG, (char*)&status, length, NULL);
			uint64_t update_time_temp;
			length = sizeof(update_time_temp);
			statement.set_param_result(15, MYSQL_TYPE_LONGLONG, (char*)&update_time_temp, length, NULL);
			if (0 != statement.get_result())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::Buddy::IMAllUserRsp msg_rsp;
			uint32_t is_more_data = 0;
			unsigned long long num = statement.get_num_rows();
			log(LOG_DEBUG, "get_all_user_info user_id:%d num:%d update_time:%lld", user_id, num, update_time);
			if (num == 100)
			{
				is_more_data = 1;
			}
			while (0 == statement.fetch_result())
			{
				log(LOG_DEBUG, "get_all_user_info user_id:%d friend_user_id:%d name:%s update_time:%lld", user_id, friend_user_id, name, update_time_temp);
				IM::BaseDefine::UserInfo *user_info_list = msg_rsp.add_user_list();
				user_info_list->set_company_id(friend_company_id);
				user_info_list->set_user_id(friend_user_id);
				user_info_list->set_user_gender(sex);
				user_info_list->set_user_nick_name(nick_name);
				user_info_list->set_avatar_url(head_img);
				user_info_list->set_avatar_update_time(head_update_time);
				user_info_list->set_email(email);
				user_info_list->set_user_real_name(name);
				user_info_list->set_user_mobile(mobile);
				user_info_list->set_user_tel(tel);
				user_info_list->set_user_english_name(english_name);
				user_info_list->set_status(status);
				user_info_list->set_user_borndate(birthday);
				user_info_list->set_update_time(update_time_temp);
				user_info_list->set_work_id(work_id);
				user_info_list->set_sign_info(sign_info);
				msg_rsp.set_update_time(update_time_temp);
			}
			msg_rsp.set_is_more_data(is_more_data);
			msg->set_cmd_id(CID_BUDDY_LIST_ALL_USER_RESPONSE);
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
		} while (false);
		statement.free();
	}

	void set_user_avatar(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		log(LOG_DEBUG, "%s:%s()", __FILE__, __FUNCTION__);
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Buddy::IMChangeAvatarReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t company_id = msg_data.company_id();
			string table_name = string("t_im_user_vcard_") + to_string(company_id % 8);
			string sql = string("update ") + table_name + string(" set head_img = ?, update_time = ?, head_update_time = ? where company_id = ? and user_id = ?");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			string avatar_url = msg_data.avatar_url();
			statement.set_param_bind(0, avatar_url);
			struct timeval tv;
			gettimeofday(&tv, NULL);
			uint64_t update_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
			statement.set_param_bind(1, update_time);
			statement.set_param_bind(2, update_time);
			statement.set_param_bind(3, company_id);
			uint32_t user_id = msg_data.user_id();
			statement.set_param_bind(4, user_id);
			int ret_temp = statement.execute();
			IM::Buddy::IMChangeAvatarRsp msg_rsp;
			if (0 == ret_temp)
			{
				msg_rsp.set_result_code(1);
			}
			else if (1 == ret_temp)
			{
				msg_rsp.set_result_code(0);
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			}
			msg_rsp.set_update_time(update_time);
			msg->set_cmd_id(CID_BUDDY_LIST_CHANGE_AVATAR_RESPONSE);
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
			redis_user_info(company_id, user_id, mysql, redis);
		} while (false);
		statement.free();
	}

	void set_user_sign_info(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		log(LOG_DEBUG, "%s:%s()", __FILE__, __FUNCTION__);
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Buddy::IMChangeSignInfoReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t company_id = msg_data.company_id();
			string table_name = string("t_im_user_vcard_") + to_string(company_id % 8);
			string sql = string("update ") + table_name + string(" set sign_info = ?, update_time = ? where company_id = ? and user_id = ?");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			string sign_info = msg_data.sign_info();
			statement.set_param_bind(0, sign_info);
			struct timeval tv;
			gettimeofday(&tv, NULL);
			uint64_t update_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
			statement.set_param_bind(1, update_time);
			statement.set_param_bind(2, company_id);
			uint32_t user_id = msg_data.user_id();
			statement.set_param_bind(3, user_id);
			int ret_temp = statement.execute();
			IM::Buddy::IMChangeSignInfoRsp msg_rsp;
			if (0 == ret_temp)
			{
				msg_rsp.set_result_code(1);
			}
			else if (1 == ret_temp)
			{
				msg_rsp.set_result_code(0);
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			}
			msg_rsp.set_update_time(update_time);
			msg->set_cmd_id(CID_BUDDY_LIST_CHANGE_SIGN_INFO_RESPONSE);
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
			redis_user_info(company_id, user_id, mysql, redis);
		} while (false);
		statement.free();
	}

	void set_friend_remark(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		log(LOG_DEBUG, "%s:%s()", __FILE__, __FUNCTION__);
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			string sql("update t_im_user_friend set remark = ?, update_time = ? where company_id = ? and user_id = ? and friend_company_id = ? friend_id = ?");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::Buddy::IMModiNickNameReq msg_data;
			msg_data.ParseFromArray(msg->get_pb_data(), msg->get_pb_length());
			string remark = msg_data.nick_name();
			statement.set_param_bind(0, remark);
			struct timeval tv;
			gettimeofday(&tv, NULL);
			uint64_t update_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
			statement.set_param_bind(1, update_time);
			uint32_t company_id = msg_data.company_id();
			statement.set_param_bind(2, company_id);
			uint32_t user_id = msg_data.user_id();
			statement.set_param_bind(3, user_id);
			uint32_t friend_company_id = msg_data.dest_company_id();
			statement.set_param_bind(4, friend_company_id);
			uint32_t friend_id = msg_data.dest_user_id();
			statement.set_param_bind(5, friend_id);
			if (0 != statement.execute())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			}
			IM::Buddy::IMModiNickNameRsp msg_rsp;
			msg_rsp.set_company_id(company_id);
			msg_rsp.set_user_id(user_id);
			msg_rsp.set_dest_company_id(friend_company_id);
			msg_rsp.set_dest_user_id(friend_id);
			msg_rsp.set_nick_name(remark);
			msg_rsp.set_update_time(update_time);
			msg->set_cmd_id(CID_BUDDY_LIST_REMARK_RESPONSE);
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
			statement.free();
			redis_user_friend_list(company_id, user_id, mysql, redis);
		} while (false);
		statement.free();
	}

	void get_friend_req_list(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		log(LOG_DEBUG, "%s:%s()", __FILE__, __FUNCTION__);
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Buddy::IMGetBuddyReqListReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t pack_index = msg_data.pack_index();
			if (0 == pack_index)
			{
				string sql("select count(*) from t_im_user_friend_req where dest_company_id = ? and dest_id = ?");
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				uint32_t company_id = msg_data.company_id();
				statement.set_param_bind(0, company_id);
				uint32_t user_id = msg_data.user_id();
				statement.set_param_bind(1, user_id);
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
				uint32_t pack_num = num / 100 + 1;
				statement.free();
				sql = "select company_id, user_id, dest_company_id, dest_id, req_result, remark, create_time from t_im_user_friend_req \
					  					  				      where dest_company_id = ? and dest_id = ? order by create_time desc limit 0, 100";
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
				uint32_t company_id_temp;
				length = sizeof(company_id_temp);
				statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&company_id_temp), length, NULL);
				uint32_t user_id_temp;
				length = sizeof(user_id_temp);
				statement.set_param_result(1, MYSQL_TYPE_LONG, (char*)(&user_id_temp), length, NULL);
				uint32_t dest_company_id;
				length = sizeof(dest_company_id);
				statement.set_param_result(2, MYSQL_TYPE_LONG, (char*)(&dest_company_id), length, NULL);
				uint32_t dest_user_id;
				length = sizeof(dest_user_id);
				statement.set_param_result(3, MYSQL_TYPE_LONG, (char*)(&dest_user_id), length, NULL);
				uint8_t req_result;
				length = sizeof(req_result);
				statement.set_param_result(4, MYSQL_TYPE_TINY, (char*)(&req_result), length, NULL);
				char remark[100] = { '\0' };
				length = sizeof(remark);
				statement.set_param_result(5, MYSQL_TYPE_STRING, remark, length, NULL);
				uint64_t update_time;
				length = sizeof(update_time);
				statement.set_param_result(6, MYSQL_TYPE_LONGLONG, (char*)(&update_time), length, NULL);
				if (0 != statement.get_result())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				IM::Buddy::IMGetBuddyReqListRsp msg_rsp;
				msg_rsp.set_pack_count(pack_num);
				msg_rsp.set_pack_index(1);
				uint64_t update_time_max = 0;
				while (0 == statement.fetch_result())
				{
					IM::BaseDefine::IMBuddyReqInfo *req_info_list = msg_rsp.add_buddysreqs();
					req_info_list->set_company_id(company_id_temp);
					req_info_list->set_user_id(user_id_temp);
					req_info_list->set_dest_company_id(dest_company_id);
					req_info_list->set_dest_user_id(dest_user_id);
					req_info_list->set_req_stat(req_result);
					req_info_list->set_req_time(update_time);
					if (update_time_max < update_time)
					{
						update_time_max = update_time;
					}
				}
				msg_rsp.set_update_time(update_time_max);
				msg->set_cmd_id(CID_BUDDY_LIST_REQUEST_LIST_RESPONSE);
				msg->set_pb_length(msg_rsp.ByteSize());
				msg->write_msg(&msg_rsp);
				net_server::get_instance()->add_response(ptask);
			}
			else if (0 < pack_index)
			{
				string sql("select company_id, user_id, dest_company_id, dest_id, req_result, remark, create_time from t_im_user_friend_req \
						   		where dest_company_id = ? and dest_id = ? and create_time < ? order by create_time desc limit ?, 100");
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				uint32_t company_id = msg_data.company_id();
				statement.set_param_bind(0, company_id);
				uint32_t user_id = msg_data.user_id();
				statement.set_param_bind(1, user_id);
				uint64_t update_time = msg_data.update_time();
				statement.set_param_bind(2, update_time);
				uint32_t offset = (pack_index - 1) * 100;
				statement.set_param_bind(3, offset);
				if (0 != statement.query())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				uint32_t company_id_temp;
				unsigned long length = sizeof(company_id_temp);
				statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&company_id_temp), length, NULL);
				uint32_t user_id_temp;
				length = sizeof(user_id_temp);
				statement.set_param_result(1, MYSQL_TYPE_LONG, (char*)(&user_id_temp), length, NULL);
				uint32_t dest_company_id;
				length = sizeof(dest_company_id);
				statement.set_param_result(2, MYSQL_TYPE_LONG, (char*)(&dest_company_id), length, NULL);
				uint32_t dest_user_id;
				length = sizeof(dest_user_id);
				statement.set_param_result(3, MYSQL_TYPE_LONG, (char*)(&dest_user_id), length, NULL);
				uint8_t req_result;
				length = sizeof(req_result);
				statement.set_param_result(4, MYSQL_TYPE_TINY, (char*)(&req_result), length, NULL);
				char remark[100] = { '\0' };
				length = sizeof(remark);
				statement.set_param_result(5, MYSQL_TYPE_STRING, remark, length, NULL);
				uint64_t update_time_temp;
				length = sizeof(update_time_temp);
				statement.set_param_result(6, MYSQL_TYPE_LONGLONG, (char*)(&update_time_temp), length, NULL);
				if (0 != statement.get_result())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				IM::Buddy::IMGetBuddyReqListRsp msg_rsp;
				msg_rsp.set_pack_index(pack_index);
				while (0 == statement.fetch_result())
				{
					IM::BaseDefine::IMBuddyReqInfo *req_info_list = msg_rsp.add_buddysreqs();
					req_info_list->set_company_id(company_id_temp);
					req_info_list->set_user_id(user_id_temp);
					req_info_list->set_dest_company_id(dest_company_id);
					req_info_list->set_dest_user_id(dest_user_id);
					req_info_list->set_req_stat(req_result);
					req_info_list->set_req_time(update_time_temp);
				}
				msg_rsp.set_update_time(update_time);
				msg->set_cmd_id(CID_BUDDY_LIST_REQUEST_LIST_RESPONSE);
				msg->set_pb_length(msg_rsp.ByteSize());
				msg->write_msg(&msg_rsp);
				net_server::get_instance()->add_response(ptask);
			}
		} while (false);
		statement.free();
	}
}