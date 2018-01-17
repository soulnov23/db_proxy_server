#include "handler_group.h"
#include "pre_statement.h"
#include <sys/time.h>
#include "im_log.h"
#include "net_server.h"
#include "handler_cache.h"
#include "../protobuf/IM.BaseDefine.pb.h"
using namespace IM::BaseDefine;
#include "../protobuf/IM.Group.pb.h"
#include "../protobuf/IM.Buddy.pb.h"
#include "../protobuf/IM.D2G.pb.h"

namespace db_proxy
{
	void get_group_list(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Group::IMGroupListReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t company_id = msg_data.company_id();
			string table_name_a = string("t_im_user_group_") + to_string(company_id % 8);
			string table_name_b = string("t_im_group_list_") + to_string(company_id % 8);
			string sql = string("select a.friend_company_id, a.friend_group_id, a.is_delete, a.group_type, a.order_value, a.is_disturb, a.is_show, a.update_time, b.group_name, b.group_topic, b.owner_id, b.is_public, b.member_count, b.head_img from  ") +
							table_name_a + string(" as a, ") + table_name_b + string(" as b where a.company_id = ? and a.user_id = ? and b.company_id = a.friend_company_id and \
							b.group_id = a.friend_group_id and a.update_time > ? order by a.update_time asc limit 0, 100");
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
			uint32_t friend_group_id;
			length = sizeof(friend_group_id);
			statement.set_param_result(1, MYSQL_TYPE_LONG, (char*)(&friend_group_id), length, NULL);
			uint8_t is_delete;
			length = sizeof(is_delete);
			statement.set_param_result(2, MYSQL_TYPE_TINY, (char*)(&is_delete), length, NULL);
			uint8_t group_type;
			length = sizeof(group_type);
			statement.set_param_result(3, MYSQL_TYPE_TINY, (char*)(&group_type), length, NULL);
			uint32_t order_value;
			length = sizeof(order_value);
			statement.set_param_result(4, MYSQL_TYPE_LONG, (char*)(&order_value), length, NULL);
			uint32_t is_disturb;
			length = sizeof(is_disturb);
			statement.set_param_result(5, MYSQL_TYPE_LONG, (char*)(&is_disturb), length, NULL);
			uint32_t is_show;
			length = sizeof(is_show);
			statement.set_param_result(6, MYSQL_TYPE_LONG, (char*)(&is_show), length, NULL);
			uint64_t update_time_temp;
			length = sizeof(update_time_temp);
			statement.set_param_result(7, MYSQL_TYPE_LONGLONG, (char*)(&update_time_temp), length, NULL);
			char name[150] = { '\0' };
			length = sizeof(name);
			statement.set_param_result(8, MYSQL_TYPE_STRING, name, length, NULL);
			char topic[150] = { '\0' };
			length = sizeof(topic);
			statement.set_param_result(9, MYSQL_TYPE_STRING, topic, length, NULL);
			uint32_t owner_id;
			length = sizeof(owner_id);
			statement.set_param_result(10, MYSQL_TYPE_LONG, (char*)(&owner_id), length, NULL);
			uint8_t is_public;
			length = sizeof(is_public);
			statement.set_param_result(11, MYSQL_TYPE_TINY, (char*)(&is_public), length, NULL);
			uint32_t member_count;
			length = sizeof(member_count);
			statement.set_param_result(12, MYSQL_TYPE_LONG, (char*)(&member_count), length, NULL);
			char head_img[255] = { '\0' };
			length = sizeof(head_img);
			statement.set_param_result(13, MYSQL_TYPE_STRING, head_img, length, NULL);
			if (0 != statement.get_result())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::Group::IMGroupListRsp msg_rsp;
			uint32_t is_more_data = 0;
			unsigned long long num = statement.get_num_rows();
			if (num == 100)
			{
				is_more_data = 1;
			}
			log(LOG_DEBUG, "get_group_list user_id:%d num:%d update_time:%lld", user_id, num, update_time);
			while (0 == statement.fetch_result())
			{
				log(LOG_DEBUG, "get_group_list user_id:%d group_id:%d group_name:%s group_topic:%s is_delete:%d update_time:%lld", user_id, friend_group_id, name, topic, is_delete, update_time);
				IM::BaseDefine::GroupInfo *group_info_list = msg_rsp.add_group_info_list();
				group_info_list->set_company_id(friend_company_id);
				group_info_list->set_group_id(friend_group_id);
				group_info_list->set_is_delete(is_delete);
				group_info_list->set_fixtop_priority(order_value);
				group_info_list->set_show(is_show);
				group_info_list->set_not_disturb(is_disturb);
				group_info_list->set_group_name(name);
				memset(name, '\0', 150);
				group_info_list->set_topic(topic);
				memset(topic, '\0', 150);
				group_info_list->set_group_avatar(head_img);
				memset(head_img, '\0', 255);
				group_info_list->set_group_creator_id(owner_id);
				group_info_list->set_shield_status(is_public);
				group_info_list->set_group_member_count(member_count);
				group_info_list->set_group_type(GROUP_TYPE_MULTICHAT);
				group_info_list->set_update_time(update_time_temp);
				msg_rsp.set_update_time(update_time_temp);
			}
			msg_rsp.set_is_more_data(is_more_data);
			msg->set_cmd_id(CID_GROUP_LIST_RSP);
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
		} while (false);
		statement.free();
	}

	void set_group_list(task *ptask, MYSQL *mysql, cache_conn *redis)
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
			string table_name = string("t_im_user_group_") + to_string(company_id % 8);
			uint32_t user_id = msg_data.user_id();
			uint32_t dest_company_id = msg_data.dest_company_id();
			uint32_t dest_user_id = msg_data.dest_user_id();
			IM::BaseDefine::ListItemOptType type = msg_data.type();
			if (type = LIST_ITEM_FIXTOP)
			{
				string sql = string("update ") + table_name + string(" set order_value = ?, update_time = ? where company_id = ? and user_id = ? \
								  and friend_company_id = ? and friend_group_id = ?");
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				uint32_t order_value = time(NULL);
				statement.set_param_bind(0, order_value);
				struct timeval tv;
				gettimeofday(&tv, NULL);
				uint64_t update_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
				statement.set_param_bind(1, update_time);
				statement.set_param_bind(2, company_id);
				statement.set_param_bind(3, user_id);
				statement.set_param_bind(4, dest_company_id);
				statement.set_param_bind(5, dest_user_id);
				if (0 != statement.execute())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
			}
			else if (type == LIST_ITEM_UNFIXTOP)
			{
				string sql = string("update ") + table_name + string(" set order_value = 0, update_time = ? where company_id = ? and user_id = ? \
								  and friend_company_id = ? and friend_group_id = ?");
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				struct timeval tv;
				gettimeofday(&tv, NULL);
				uint64_t update_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
				statement.set_param_bind(0, update_time);
				statement.set_param_bind(1, company_id);
				statement.set_param_bind(2, user_id);
				statement.set_param_bind(3, dest_company_id);
				statement.set_param_bind(4, dest_user_id);
				if (0 != statement.execute())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
			}
			else if (type == LIST_ITEM_SHOW)
			{
				string sql = string("update ") + table_name + string(" set is_show = 1, update_time = ? where company_id = ? and user_id = ? \
								  and friend_company_id = ? and friend_group_id = ?");
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				struct timeval tv;
				gettimeofday(&tv, NULL);
				uint64_t update_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
				statement.set_param_bind(0, update_time);
				statement.set_param_bind(1, company_id);
				statement.set_param_bind(2, user_id);
				statement.set_param_bind(3, dest_company_id);
				statement.set_param_bind(4, dest_user_id);
				if (0 != statement.execute())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
			}
			else if (type == LIST_ITEM_HIDE)
			{
				string sql = string("update ") + table_name + string(" set is_show = 0, update_time = ? where company_id = ? and user_id = ? \
								  and friend_company_id = ? and friend_group_id = ?");
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				struct timeval tv;
				gettimeofday(&tv, NULL);
				uint64_t update_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
				statement.set_param_bind(0, update_time);
				statement.set_param_bind(1, company_id);
				statement.set_param_bind(2, user_id);
				statement.set_param_bind(3, dest_company_id);
				statement.set_param_bind(4, dest_user_id);
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

	void set_group_disturb(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do
		{
			IM::Group::IMGroupDisturbOptReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t company_id = msg_data.company_id();
			string table_name = string("t_im_user_group_") + to_string(company_id % 8);
			string sql = string("update ") + table_name + string(" set is_disturb = ?, update_time = ? where company_id = ? and user_id = ? \
							  and friend_company_id = ? and friend_group_id = ?");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint8_t is_disturb = msg_data.not_disturb();
			statement.set_param_bind(0, is_disturb);
			struct timeval tv;
			gettimeofday(&tv, NULL);
			uint64_t update_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
			statement.set_param_bind(1, update_time);
			statement.set_param_bind(2, company_id);
			uint32_t user_id = msg_data.user_id();
			statement.set_param_bind(3, user_id);
			uint32_t group_company_id = msg_data.group_company_id();
			statement.set_param_bind(4, group_company_id);
			uint32_t group_id = msg_data.group_id();
			statement.set_param_bind(5, group_id);
			if (0 != statement.execute())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::Group::IMGroupDisturbOptRsp msg_rsp;
			msg->set_cmd_id(CID_GROUP_DISTURB_OPT_RSP);
			msg_rsp.set_company_id(company_id);
			msg_rsp.set_user_id(user_id);
			msg_rsp.set_group_company_id(group_company_id);
			msg_rsp.set_group_id(group_id);
			msg_rsp.set_result_code(OPT_RESULT_SUCCESS);
			msg_rsp.set_group_type(msg_data.group_type());
			msg_rsp.set_not_disturb(msg_data.not_disturb());
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
		} while (false);
		statement.free();
	}

	void set_group_owner(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do
		{
			IM::Group::IMGroupSetOwnerReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			string sql = string("call set_group_owner(?, ?, ?, ?, ?, ?, ?)");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t group_company_id = msg_data.group_company_id();
			statement.set_param_bind(0, group_company_id);
			uint32_t group_id = msg_data.group_id();
			statement.set_param_bind(1, group_id);
			uint32_t from_company_id = msg_data.from_company_id();
			statement.set_param_bind(2, from_company_id);
			uint32_t from_user_id = msg_data.from_user_id();
			statement.set_param_bind(3, from_user_id);
			uint32_t to_company_id = msg_data.to_company_id();
			statement.set_param_bind(4, to_company_id);
			uint32_t to_user_id = msg_data.to_user_id();
			statement.set_param_bind(5, to_user_id);
			struct timeval tv;
			gettimeofday(&tv, NULL);
			uint64_t update_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
			statement.set_param_bind(6, update_time);
			log(LOG_DEBUG, "set_group_owner group_company_id:%d group_id:%d from_company_id:%d from_user_id:%d to_company_id:%d to_user_id:%d", group_company_id, group_id, from_company_id, from_user_id, to_company_id, to_user_id);
			if (0 != statement.execute())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::Group::IMGroupSetOwnerRsp msg_rsp;
			msg->set_cmd_id(CID_GROUP_OWNER_SET_RSP);
			msg_rsp.set_group_company_id(group_company_id);
			msg_rsp.set_group_id(group_id);
			msg_rsp.set_from_company_id(from_company_id);
			msg_rsp.set_from_user_id(from_user_id);
			msg_rsp.set_to_company_id(to_company_id);
			msg_rsp.set_to_user_id(to_user_id);
			msg_rsp.set_group_type(msg_data.group_type());
			msg_rsp.set_result_code(OPT_RESULT_SUCCESS);
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
		} while (false);
		statement.free();
	}

	void get_group_info(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Group::IMGroupInfoReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::Group::IMGroupInfoRsp msg_rsp;
			for (int i = 0; i < msg_data.group_item_list_size(); i++)
			{
				IM::BaseDefine::GroupListItem group_item = msg_data.group_item_list(i);
				IM::BaseDefine::GroupType group_type = group_item.group_type();
				string sql;
				uint32_t company_id = group_item.company_id();
				uint32_t group_id = group_item.group_id();
				if (GROUP_TYPE_MULTICHAT != group_type)
				{
					log(LOG_ERROR, "[ERROR] get_group_info group_id:%d group_type error!", group_id);
					continue;
				}
				string table_name = string("t_im_group_list_") + to_string(company_id % 8);
				string table_name_temp = string("group_member_count_") + to_string(company_id % 8);
				sql = string("select owner_id, group_name, group_topic, is_public, (select member_count from ") + table_name_temp +
					string(" where group_company_id = ? and group_id = ?), head_img, update_time from ") +
					table_name + string(" where company_id = ? and group_id = ?");
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				statement.set_param_bind(0, company_id);
				statement.set_param_bind(1, group_id);
				statement.set_param_bind(2, company_id);
				statement.set_param_bind(3, group_id);
				if (0 != statement.query())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				uint32_t owner_id;
				unsigned long length = sizeof(owner_id);
				statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&owner_id), length, NULL);
				char group_name[150] = { '\0' };
				length = sizeof(group_name);
				statement.set_param_result(1, MYSQL_TYPE_STRING, group_name, length, NULL);
				char group_topic[150] = { '\0' };
				length = sizeof(group_topic);
				statement.set_param_result(2, MYSQL_TYPE_STRING, group_topic, length, NULL);
				uint8_t is_public;
				length = sizeof(is_public);
				statement.set_param_result(3, MYSQL_TYPE_TINY, (char*)(&is_public), length, NULL);
				uint32_t member_count;
				length = sizeof(member_count);
				statement.set_param_result(4, MYSQL_TYPE_LONG, (char*)(&member_count), length, NULL);
				char head_img[255] = { '\0' };
				length = sizeof(head_img);
				statement.set_param_result(5, MYSQL_TYPE_STRING, head_img, length, NULL);
				uint64_t update_time;
				length = sizeof(update_time);
				statement.set_param_result(6, MYSQL_TYPE_LONGLONG, (char*)(&update_time), length, NULL);
				if (0 != statement.get_result())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				if (0 == statement.fetch_result())
				{
					log(LOG_DEBUG, "get_group_info group_id:%d group_name:%s group_topic:%s", group_id, group_name, group_topic);
					IM::BaseDefine::GroupInfo *group_info = msg_rsp.add_group_info_list();
					group_info->set_company_id(company_id);
					group_info->set_group_id(group_id);
					group_info->set_update_time(update_time);
					group_info->set_group_name(group_name);
					group_info->set_topic(group_topic);
					group_info->set_group_avatar(head_img);
					group_info->set_group_creator_id(owner_id);
					group_info->set_group_type(group_type);
					group_info->set_shield_status(is_public);
					group_info->set_group_member_count(member_count);
				}
			}
			msg->set_cmd_id(CID_GROUP_INFO_RSP);
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
		} while (false);
		statement.free();	
	}

	void set_group_info(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Group::IMGroupInfoModifyReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t group_id = msg_data.group_id();
			GroupType type = msg_data.group_type();
			if (GROUP_TYPE_MULTICHAT != type)
			{
				log(LOG_ERROR, "[ERROR] set_group_info group_id:%d group_type error!", group_id);
				break;
			}
			uint32_t group_company_id = msg_data.group_company_id();
			string table_name = string("t_im_group_list_") + to_string(group_company_id % 8);
			string sql = string("update ") + table_name + string(" set group_name = ?, group_topic = ?, head_img = ?, head_update_time = ?, update_time = ? where company_id = ? and group_id = ?");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			string group_name = msg_data.group_name();
			statement.set_param_bind(0, group_name);
			string group_topic = msg_data.topic();
			statement.set_param_bind(1, group_topic);
			string group_avatar = msg_data.group_avatar();
			statement.set_param_bind(2, group_avatar);
			struct timeval tv;
			gettimeofday(&tv, NULL);
			uint64_t update_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
			statement.set_param_bind(3, update_time);
			statement.set_param_bind(4, update_time);
			statement.set_param_bind(5, group_company_id);
			statement.set_param_bind(6, group_id);
			int ret = statement.execute();
			log(LOG_DEBUG, "set_group_info group_id:%d group_name:%s group_topic:%s", group_id, group_name.c_str(), group_topic.c_str());
			OptResultCode result = OPT_RESULT_FAIL;
			if (0 == ret)
			{
				result = OPT_RESULT_SUCCESS;
			}
			IM::Group::IMGroupInfoModifyRsp msg_rsp;
			msg_rsp.set_result_code(result);
			msg_rsp.set_group_type(msg_data.group_type());
			msg_rsp.set_company_id(msg_data.company_id());
			msg_rsp.set_user_id(msg_data.user_id());
			msg_rsp.set_group_company_id(group_company_id);
			msg_rsp.set_group_id(group_id);
			msg_rsp.set_update_time(update_time);
			statement.free();
			table_name.clear();
			table_name = string("t_im_group_member_") + to_string(group_company_id % 8);
			sql = string("select user_company_id from ") + table_name + string(" where group_company_id = ? and group_id = ? group by user_company_id");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			statement.set_param_bind(0, group_company_id);
			statement.set_param_bind(1, group_id);
			if (0 != statement.query())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t user_company_id;
			unsigned long length = sizeof(user_company_id);
			statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&user_company_id), length, NULL);
			if (0 != statement.get_result())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			while (0 == statement.fetch_result())
			{
				pre_statement state;
				string table_name_temp = string("t_im_user_group_") + to_string(user_company_id % 8);
				string sql = string("update ") + table_name_temp + string(" set update_time = ? where friend_company_id = ? and friend_group_id = ?");
				if (0 != state.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				state.set_param_bind(0, update_time);
				state.set_param_bind(1, group_company_id);
				state.set_param_bind(2, group_id);
				if (0 != state.execute())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				state.free();
			}
			table_name.clear();
			table_name = string("t_im_group_list_") + to_string(group_company_id % 8);
			string table_name_temp = string("group_member_count_") + to_string(group_company_id % 8);
			sql = string("select owner_id, group_name, group_topic, is_public, (select member_count from ") + table_name_temp +
					string(" where group_company_id = ? and group_id = ?), head_img, update_time from ") +
					table_name + string(" where company_id = ? and group_id = ?");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			statement.set_param_bind(0, group_company_id);
			statement.set_param_bind(1, group_id);
			statement.set_param_bind(2, group_company_id);
			statement.set_param_bind(3, group_id);
			if (0 != statement.query())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t owner_id;
			length = sizeof(owner_id);
			statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&owner_id), length, NULL);
			char group_name_temp[150] = { '\0' };
			length = sizeof(group_name_temp);
			statement.set_param_result(1, MYSQL_TYPE_STRING, group_name_temp, length, NULL);
			char group_topic_temp[150] = { '\0' };
			length = sizeof(group_topic_temp);
			statement.set_param_result(2, MYSQL_TYPE_STRING, group_topic_temp, length, NULL);
			uint8_t is_public;
			length = sizeof(is_public);
			statement.set_param_result(3, MYSQL_TYPE_TINY, (char*)(&is_public), length, NULL);
			uint32_t member_count;
			length = sizeof(member_count);
			statement.set_param_result(4, MYSQL_TYPE_LONG, (char*)(&member_count), length, NULL);
			char head_img[255] = { '\0' };
			length = sizeof(head_img);
			statement.set_param_result(5, MYSQL_TYPE_STRING, head_img, length, NULL);
			uint64_t update_time_temp;
			length = sizeof(update_time_temp);
			statement.set_param_result(6, MYSQL_TYPE_LONGLONG, (char*)(&update_time_temp), length, NULL);
			if (0 != statement.get_result())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			if (0 == statement.fetch_result())
			{
				IM::BaseDefine::GroupInfo *group_info = msg_rsp.mutable_group_info();
				group_info->set_company_id(group_company_id);
				group_info->set_group_id(group_id);
				group_info->set_update_time(update_time_temp);
				group_info->set_group_name(group_name_temp);
				group_info->set_topic(group_topic_temp);
				group_info->set_group_avatar(head_img);
				group_info->set_group_creator_id(owner_id);
				group_info->set_group_type(msg_data.group_type());
				group_info->set_shield_status(is_public);
				group_info->set_group_member_count(member_count);
			}
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->set_cmd_id(CID_GROUP_INFO_MODIFY_RSP);
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
		} while (false);
		statement.free();
	}

	void get_group_members(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Group::IMGroupMemberListReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t dest_company_id = msg_data.dest_company_id();
			uint32_t dest_group_id = msg_data.dest_group_id();
			GroupType type = msg_data.group_type();
			if (GROUP_TYPE_MULTICHAT != type)
			{
				log(LOG_ERROR, "[ERROR] get_group_members group_id:%d group_type error!", dest_group_id);
				break;
			}
			string table_name = string("t_im_group_member_") + to_string(dest_company_id % 8);
			string  table_name_temp = string("t_im_user_vcard_") + to_string(dest_company_id % 8);
			string sql = string("select a.user_company_id, a.user_id, a.is_delete, a.role, a.update_time, b.name from ") + table_name + string(" as a, ") + table_name_temp +
							  string(" as b where a.group_company_id = ? and a.group_id = ? and a.user_company_id = b.company_id and a.user_id = b.user_id \
					   					  and a.update_time > ? order by a.update_time asc limit 0, 100");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			statement.set_param_bind(0, dest_company_id);
			statement.set_param_bind(1, dest_group_id);
			uint64_t update_time = msg_data.update_time();
			statement.set_param_bind(2, update_time);
			if (0 != statement.query())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t company_id;
			unsigned long length = sizeof(company_id);
			statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&company_id), length, NULL);
			uint32_t user_id;
			length = sizeof(user_id);
			statement.set_param_result(1, MYSQL_TYPE_LONG, (char*)(&user_id), length, NULL);
			uint8_t is_delete;
			length = sizeof(is_delete);
			statement.set_param_result(2, MYSQL_TYPE_TINY, (char*)(&is_delete), length, NULL);
			uint8_t role;
			length = sizeof(role);
			statement.set_param_result(3, MYSQL_TYPE_TINY, (char*)(&role), length, NULL);
			uint64_t update_time_temp;
			length = sizeof(update_time_temp);
			statement.set_param_result(4, MYSQL_TYPE_LONGLONG, (char*)(&update_time_temp), length, NULL);
			char user_name[100] = { '\0' };
			length = sizeof(user_name);
			statement.set_param_result(5, MYSQL_TYPE_STRING, user_name, length, NULL);
			if (0 != statement.get_result())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::Group::IMGroupMemberListRsp msg_rsp;
			msg_rsp.set_company_id(dest_company_id);
			msg_rsp.set_group_id(dest_group_id);
			uint32_t is_more_data = 0;
			unsigned long long num = statement.get_num_rows();
			log(LOG_DEBUG, "get_group_members group_id:%d num:%d update_time:%lld", dest_group_id, num, update_time);
			if (num == 100)
			{
				is_more_data = 1;
			}
			while (0 == statement.fetch_result())
			{
				log(LOG_DEBUG, "get_group_members group_id:%d user_id:%d user_name:%s is_delete:%d update_time:%lld", dest_group_id, user_id, user_name, is_delete, update_time_temp);
				IM::BaseDefine::GroupMemberItem *group_member_list = msg_rsp.add_group_member_list();
				group_member_list->set_company_id(company_id);
				group_member_list->set_member_id(user_id);
				group_member_list->set_is_delete(is_delete);
				group_member_list->set_member_name(user_name);
				group_member_list->set_member_role(role);
				msg_rsp.set_update_time(update_time_temp);
			}
			msg_rsp.set_is_more_data(is_more_data);
			msg->set_cmd_id(CID_GROUP_MEMBER_RSP);
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
		} while (false);
		statement.free();
	}

// 	void get_group_member_list(task *ptask, MYSQL *mysql, cache_conn *redis)
// 	{
// 		log(LOG_DEBUG, "%s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
// 		
// 		
// 		pre_statement statement;
// 		IM::DTG::DTGroupMemberListQuery msg_data;
// 		if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
// 		{
// 			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
// 			return;
// 		}
// 		IM::BaseDefine::GroupType group_type = msg_data.group_type();
// 		string sql;
// 		uint32_t company_id = msg_data.company_id();
// 		if ((GROUP_TYPE_NORMAL == group_type) || (GROUP_TYPE_TMP == group_type))
// 		{
// 			string table_name = string("t_im_group_member_") + to_string(company_id % 8);
// 			sql = string("select user_company_id, user_id from ") +
// 								table_name + string(" where group_company_id = ? and group_id = ?");
// 		}
// 		else if (GROUP_TYPE_ORG == group_type)
// 		{
// 			string table_name = string("t_im_orggroup_member_") + to_string(company_id % 8);
// 			sql = string("select user_company_id, user_id from ") +
// 								table_name + string(" where org_company_id = ? and org_id = ?");
// 		}
// 		if (0 != statement.init(mysql, sql))
// 		{
// 			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
// 			return;
// 		}	
// 		statement.set_param_bind(0, company_id);
// 		uint32_t group_id = msg_data.group_id();
// 		statement.set_param_bind(1, group_id);
// 		if (0 != statement.query())
// 		{
// 			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
// 			return;
// 		}
// 		uint32_t user_company_id;
// 		unsigned long length = sizeof(user_company_id);
// 		statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&user_company_id), length, NULL);
// 		uint32_t user_id;
// 		length = sizeof(user_id);
// 		statement.set_param_result(1, MYSQL_TYPE_LONG, (char*)(&user_id), length, NULL);
// 		if (0 != statement.get_result())
// 		{
// 			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
// 			return;
// 		}
// 		IM::DTG::DTGroupMemberListResult msg_rsp;
// 		msg_rsp.set_company_id(company_id);
// 		msg_rsp.set_group_id(group_id);
// 		msg_rsp.set_result_code(0);
// 		while (0 == statement.fetch_result())
// 		{
// 			IM::DTG::DTGroupMemberItem *group_member_list = msg_rsp.add_group_member_list();
// 			group_member_list->set_company_id(user_company_id);
// 			group_member_list->set_member_id(user_id);
// 			group_member_list->set_member_role(3);
// 		}
// 		msg->set_cmd_id(IM::DTG::DTG_MEMBER_LIST_RSP);
// 		string str = msg_rsp.SerializePartialAsString();
// 		if (str.empty())
// 		{
// 			log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
// 			return;
// 		}
// 		msg->set_pb_length(str.length());
// 		msg->write_msg(str.c_str());
// 		string string_data;
// 		string_data.append(msg->get_data(), HEADER_LEN + str.length());
// 		net_server::get_instance()->add_response(string_data, fd);
// 		statement.free();
// 		
// 		log(LOG_DEBUG, "%s:%s():%d:Response success and length = %d", __FILE__, __FUNCTION__, __LINE__, HEADER_LEN + str.length());
// 	}

	void set_group_create(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::DTG::DTGroupCreateReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t owner_id = msg_data.user_id();
			GroupType type = msg_data.group_type();
			if (GROUP_TYPE_MULTICHAT != type)
			{
				log(LOG_ERROR, "[ERROR] set_group_create owner_id:%d group_type error!", owner_id);
				break;
			}
			string sql("call create_group(?, ?, ?, ?, ?, ?, ?, ?, ?)");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t company_id = msg_data.company_id();
			statement.set_param_bind(0, company_id);
			uint32_t group_id = msg_data.group_id();
			statement.set_param_bind(1, group_id);
			statement.set_param_bind(2, owner_id);
			string group_name = msg_data.group_name();
			statement.set_param_bind(3, group_name);
			string group_topic = msg_data.topic();
			statement.set_param_bind(4, group_topic);
			string group_avatar = msg_data.group_avatar();
			statement.set_param_bind(5, group_avatar);
			struct timeval tv;
			gettimeofday(&tv, NULL);
			uint64_t update_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
			statement.set_param_bind(6, update_time);
			//uint64_t create_time = msg_data.update_time();
			statement.set_param_bind(7, update_time);
			statement.set_param_bind(8, update_time);
			int ret_temp = statement.execute();
			log(LOG_DEBUG, "set_group_create owner_id:%d group_id:%d group_name:%s group_topic:%s update_time:%lld", owner_id, group_id, group_name.c_str(), group_topic.c_str(), update_time);
			redis_user_group_list(company_id, owner_id, mysql, redis);
			statement.free();
			sql = "call group_member_add(?, ?, ?, ?, ?, ?, ?)";
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			for (int i = 0; i < msg_data.member_id_list_size(); i++)
			{
				IM::BaseDefine::UserIdItem member_id = msg_data.member_id_list(i);
				statement.set_param_bind(0, company_id);
				statement.set_param_bind(1, group_id);
				uint32_t member_company_id = member_id.company_id();
				statement.set_param_bind(2, member_company_id);
				uint32_t member_user_id = member_id.user_id();
				statement.set_param_bind(3, member_user_id);
				uint8_t role = 2;
				statement.set_param_bind(4, role);
				struct timeval tv;
				gettimeofday(&tv, NULL);
				uint64_t update_time_temp = tv.tv_sec * 1000 + tv.tv_usec / 1000;
				statement.set_param_bind(5, update_time_temp);
				statement.set_param_bind(6, update_time_temp);
				if (0 != statement.execute())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					log(LOG_ERROR, "set_group_create company_id:%d user_id:%d", member_company_id, member_user_id);
				}
				log(LOG_DEBUG, "set_group_create member_id:%d group_id:%d update_time:%lld", member_user_id, group_id, update_time_temp);
				redis_user_group_list(member_company_id, member_user_id, mysql, redis);
			}
			statement.free();
			IM::DTG::DTGroupCreateRsp msg_rsp;
			if (0 == ret_temp)
			{
				msg_rsp.set_result_code(OPT_RESULT_SUCCESS);
			}
			else if (1 == ret_temp)
			{
				msg_rsp.set_result_code(OPT_RESULT_FAIL);
			}
			msg_rsp.set_company_id(company_id);
			msg_rsp.set_user_id(owner_id);
			msg_rsp.set_group_id(group_id);
			msg_rsp.set_group_type(msg_data.group_type());
			msg_rsp.set_memberlist_update_time(update_time);
			msg->set_cmd_id(CID_GROUP_CREATE_RSP);
			statement.free();
			string table_name = string("t_im_group_member_") + to_string(company_id % 8);
			sql = string("select user_company_id, user_id from ") + table_name + string(" where group_company_id = ? and group_id = ?");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			statement.set_param_bind(0, company_id);
			statement.set_param_bind(1, group_id);
			if (0 != statement.query())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t user_company_id;
			unsigned long length = sizeof(user_company_id);
			statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&user_company_id), length, NULL);
			uint32_t user_id;
			length = sizeof(user_id);
			statement.set_param_result(1, MYSQL_TYPE_LONG, (char*)(&user_id), length, NULL);
			if (0 != statement.get_result())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;;
			}
			while (0 == statement.fetch_result())
			{
				IM::BaseDefine::UserIdItem *group_member_list = msg_rsp.add_member_id_list();
				group_member_list->set_company_id(user_company_id);
				group_member_list->set_user_id(user_id);
			}
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
			redis_group_member_list(company_id, group_id, mysql, redis);
		} while (false);
		statement.free();	
	}

	void set_group_dissolve(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Group::IMGroupDissolveReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t group_id = msg_data.group_id();
			GroupType type = msg_data.group_type();
			if (GROUP_TYPE_MULTICHAT != type)
			{
				log(LOG_ERROR, "[ERROR] set_group_dissolve group_id:%d group_type error!", group_id);
				break;
			}
			string sql("call dissolve_group(?, ?, ?)");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t company_id = msg_data.company_id();
			statement.set_param_bind(0, company_id);
			statement.set_param_bind(1, group_id);
			struct timeval tv;
			gettimeofday(&tv, NULL);
			uint64_t update_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
			statement.set_param_bind(2, update_time);
			int ret_temp = statement.execute();
			log(LOG_DEBUG, "set_group_dissolve group_id:%d", group_id);
			IM::Group::IMGroupDissolveRsp msg_rsp;
			if (0 == ret_temp)
			{
				msg_rsp.set_result_code(OPT_RESULT_SUCCESS);
			}
			else if (1 == ret_temp)
			{
				msg_rsp.set_result_code(OPT_RESULT_FAIL);
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			}
			msg_rsp.set_company_id(company_id);
			msg_rsp.set_group_id(group_id);
			msg_rsp.set_group_type(msg_data.group_type());
			msg->set_cmd_id(CID_GROUP_DISSOLVE_RSP);
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
			redis_group_member_list(company_id, group_id, mysql, redis);
			statement.free();
			string table_name = string("t_im_group_member_") + to_string(company_id % 8);
			sql = string("select user_company_id, user_id from ") + table_name + string(" where group_company_id = ? and group_id = ?");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			statement.set_param_bind(0, company_id);
			statement.set_param_bind(1, group_id);
			if (0 != statement.query())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t user_company_id;
			unsigned long length = sizeof(user_company_id);
			statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&user_company_id), length, NULL);
			uint32_t user_id;
			length = sizeof(user_id);
			statement.set_param_result(1, MYSQL_TYPE_LONG, (char*)(&user_id), length, NULL);
			if (0 != statement.get_result())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;;
			}
			while (0 == statement.fetch_result())
			{
				redis_user_group_list(user_company_id, user_id, mysql, redis);
			}
		} while (false);
		statement.free();
	}

	void set_group_change(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Group::IMGroupChangeMemberReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::BaseDefine::GroupMemberOptType change_type = msg_data.change_type();
			IM::BaseDefine::GroupListItem group_item = msg_data.group_item_info();
			uint32_t group_company_id = group_item.company_id();
			uint32_t group_id = group_item.group_id();
			int ret_temp;
			if (GROUP_MEMBER_OPT_ADD == change_type)
			{
				string sql("call group_member_add(?, ?, ?, ?, ?, ?, ?)");
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				statement.set_param_bind(0, group_company_id);
				statement.set_param_bind(1, group_id);
				uint8_t role = 2;
				statement.set_param_bind(4, role);
				for (int i = 0; i < msg_data.member_id_list_size(); i++)
				{
					IM::BaseDefine::UserIdItem user_item_list = msg_data.member_id_list(i);
					uint32_t company_id = user_item_list.company_id();
					statement.set_param_bind(2, company_id);
					uint32_t user_id = user_item_list.user_id();
					statement.set_param_bind(3, user_id);
					struct timeval tv;
					gettimeofday(&tv, NULL);
					uint64_t update_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
					statement.set_param_bind(5, update_time);
					statement.set_param_bind(6, update_time);
					ret_temp = statement.execute();
					log(LOG_DEBUG, "set_group_change ADD group_id:%d user_id:%d update_time:%lld", group_id, user_id, update_time);
					if (0 != ret_temp)
					{
						log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					}
					redis_user_group_list(company_id, user_id, mysql, redis);
				}
			}
			else if (GROUP_MEMBER_OPT_DEL == change_type)
			{
				string sql("call exit_group(?, ?, ?, ?, ?)");
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				statement.set_param_bind(0, group_company_id);
				statement.set_param_bind(1, group_id);
				for (int i = 0; i < msg_data.member_id_list_size(); i++)
				{
					IM::BaseDefine::UserIdItem user_item_list = msg_data.member_id_list(i);
					uint32_t company_id = user_item_list.company_id();
					statement.set_param_bind(2, company_id);
					uint32_t user_id = user_item_list.user_id();
					statement.set_param_bind(3, user_id);
					struct timeval tv;
					gettimeofday(&tv, NULL);
					uint64_t update_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
					statement.set_param_bind(4, update_time);
					ret_temp = statement.execute();
					log(LOG_DEBUG, "set_group_change DEL group_id:%d user_id:%d update_time:%lld", group_id, user_id, update_time);
					if (0 != ret_temp)
					{
						log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					}
					redis_user_group_list(company_id, user_id, mysql, redis);
				}
			}
			else if (GROUP_MEMBER_OPT_MODIFY == change_type)
			{

			}
			IM::Group::IMGroupChangeMemberRsp msg_rsp;
			msg_rsp.set_user_id(msg_data.user_id());
			msg_rsp.set_change_type(msg_data.change_type());
			if (0 == ret_temp)
			{
				msg_rsp.set_result_code(OPT_RESULT_SUCCESS);
			}
			else if (1 == ret_temp)
			{
				msg_rsp.set_result_code(OPT_RESULT_FAIL);
			}
			IM::BaseDefine::GroupListItem *group_item_temp = msg_rsp.mutable_group_item_info();
			group_item_temp->set_company_id(group_company_id);
			group_item_temp->set_group_id(group_id);
			group_item_temp->set_group_type(group_item.group_type());
			group_item_temp->set_update_time(group_item.update_time());
			for (int i = 0; i < msg_data.member_id_list_size(); i++)
			{
				IM::BaseDefine::UserIdItem user_item_list = msg_data.member_id_list(i);
				IM::BaseDefine::UserIdItem *user_item_list_temp = msg_rsp.add_chg_user_id_list();
				user_item_list_temp->set_company_id(user_item_list.company_id());
				user_item_list_temp->set_user_id(user_item_list.user_id());
			}
			msg->set_cmd_id(CID_GROUP_CHANGE_MEMBER_RSP);
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
			redis_group_member_list(group_company_id, group_id, mysql, redis);
		} while (false);
		statement.free();
	}
}
