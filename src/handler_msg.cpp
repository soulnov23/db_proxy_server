#include "handler_msg.h"
#include "pre_statement.h"
#include <sys/time.h>
#include "im_log.h"
#include "net_server.h"
#include "../protobuf/IM.BaseDefine.pb.h"
using namespace IM::BaseDefine;
#include "../protobuf/IM.Message.pb.h"

namespace db_proxy
{
	void set_offline_msg_data(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Message::IMMsg msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d:ParsePartialFromArray", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::BaseDefine::MsgData msg_array = msg_data.msg_data();
			IM::BaseDefine::SessionType session_type = msg_array.session_type();
			if (SESSION_TYPE_SINGLE == session_type)//单点离线消息入离线消息表
			{
				string sql("insert into t_im_chat_msg(msg_id, from_company_id, from_user_id, to_company_id, \
					  			 to_user_id, create_time, msg_data) values(?, ?, ?, ?, ?, ?, ?)");
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				string msg_id = msg_array.msg_id();
				statement.set_param_bind(0, msg_id);
				uint32_t from_company_id = msg_array.from_company_id();
				statement.set_param_bind(1, from_company_id);
				uint32_t from_user_id = msg_array.from_user_id();
				statement.set_param_bind(2, from_user_id);
				uint32_t to_company_id = msg_array.to_company_id();
				statement.set_param_bind(3, to_company_id);
				uint32_t to_user_id = msg_array.to_user_id();
				statement.set_param_bind(4, to_user_id);
				uint64_t create_time = msg_array.create_time();
				statement.set_param_bind(5, create_time);
				string msgdata = msg_array.SerializePartialAsString();
				statement.set_param_bind(6, msgdata);
				if (0 != statement.execute())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				}
				log(LOG_DEBUG, "set_offline_msg_data msg_id:%s from_user_id:%d to_user_id:%d create_time:%lld", msg_id.c_str(), from_user_id, to_user_id, create_time);
				statement.free();
				sql = "insert into t_im_chat_offline_msg(msg_id, from_company_id, from_user_id, to_company_id, \
						  to_user_id, create_time) values(?, ?, ?, ?, ?, ?)";
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				statement.set_param_bind(0, msg_id);
				statement.set_param_bind(1, from_company_id);
				statement.set_param_bind(2, from_user_id);
				statement.set_param_bind(3, to_company_id);
				statement.set_param_bind(4, to_user_id);
				statement.set_param_bind(5, create_time);
				if (0 != statement.execute())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				}
			}
		} while (false);
		delete ptask;
		statement.free();	
	}

	void set_online_msg_data(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Message::IMMsg msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::BaseDefine::MsgData msg_array = msg_data.msg_data();
			IM::BaseDefine::SessionType session_type = msg_array.session_type();
			string sql;
			if (SESSION_TYPE_SINGLE == session_type)//单点消息入单点消息表
			{
				sql = "insert into t_im_chat_msg(msg_id, from_company_id, from_user_id, to_company_id, \
					  	  to_user_id, create_time, msg_data) values(?, ?, ?, ?, ?, ?, ?)";
			}
			else if (SESSION_TYPE_MULTICHAT == session_type)//群组消息入群组消息表
			{
				sql = "insert into t_im_group_msg(msg_id, from_company_id, from_user_id, to_company_id, \
					      to_group_id, create_time, msg_data) values(?, ?, ?, ?, ?, ?, ?)";
			}
			else if (SESSION_TYPE_ORGGROUP == session_type)//组织架构消息入群组消息表
			{
				sql = "insert into t_im_orggroup_msg(msg_id, from_company_id, from_user_id, to_company_id, \
					  	  to_org_id, create_time, msg_data) values(?, ?, ?, ?, ?, ?, ?)";
			}
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			string msg_id = msg_array.msg_id();
			statement.set_param_bind(0, msg_id);
			uint32_t from_company_id = msg_array.from_company_id();
			statement.set_param_bind(1, from_company_id);
			uint32_t from_user_id = msg_array.from_user_id();
			statement.set_param_bind(2, from_user_id);
			uint32_t to_company_id = msg_array.to_company_id();
			statement.set_param_bind(3, to_company_id);
			uint32_t to_user_id = msg_array.to_user_id();
			statement.set_param_bind(4, to_user_id);
			uint64_t create_time = msg_array.create_time();
			statement.set_param_bind(5, create_time);
			string msgdata = msg_array.SerializePartialAsString();
			statement.set_param_bind(6, msgdata);
			if (0 != statement.execute())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			}
			log(LOG_DEBUG, "set_online_msg_data msg_id:%s from_user_id:%d to_user_id:%d create_time:%lld", msg_id.c_str(), from_user_id, to_user_id, create_time);
		} while (false);
		delete ptask;
		statement.free();
	}

	void set_recall_msg_data(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do
		{
			IM::Message::IMMsg msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::BaseDefine::MsgData msg_array = msg_data.msg_data();
			IM::BaseDefine::SessionType session_type = msg_array.session_type();
			string sql;
			if (SESSION_TYPE_SINGLE == session_type)//单点消息入单点消息表
			{
				sql = "update t_im_chat_offline_msg set msg_id = ? where msg_id = ?";
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				string msg_id = msg_array.msg_id();
				statement.set_param_bind(0, msg_id);
				string msg_id_temp = msg_array.msg_content(0).msg_data();
				statement.set_param_bind(1, msg_id_temp);
				if (0 != statement.execute())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				}
				statement.free();
				sql = "update t_im_chat_msg set msg_data = ?, msg_id = ? where msg_id = ?";
			}
			else if (SESSION_TYPE_MULTICHAT == session_type)//群组消息入群组消息表
			{
				sql = "update t_im_group_msg set msg_data = ?, msg_id = ? where msg_id = ?";
			}
			else if (SESSION_TYPE_ORGGROUP == session_type)//组织架构消息入群组消息表
			{
				sql = "update t_im_orggroup_msg set msg_data = ?, msg_id = ? where msg_id = ?";
			}
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			string msgdata = msg_array.SerializePartialAsString();
			statement.set_param_bind(0, msgdata);
			string msg_id = msg_array.msg_id();
			statement.set_param_bind(1, msg_id);
			string msg_id_temp = msg_array.msg_content(0).msg_data();
			statement.set_param_bind(2, msg_id_temp);
			if (0 != statement.execute())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			}
			log(LOG_DEBUG, "set_recall_msg_data msg_id:%s new_msg_id:%s", msg_id_temp.c_str(), msg_id.c_str());
		} while (false);
		delete ptask;
		statement.free();
	}

	void set_receipt_status(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		log(LOG_DEBUG, "%s:%s()", __FILE__, __FUNCTION__);
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Message::IMMsgOptNotify msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::BaseDefine::SessionType session_type = msg_data.session_type();
			string sql;
			if (SESSION_TYPE_SINGLE == session_type)//单点消息入单点消息表
			{
				sql = "update t_im_chat_msg set status = 1 where msg_id = ? and from_company_id = ? and from_user_id = ? \
					  	  and to_company_id = ? and to_user_id = ?";
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
			}
			else if ((SESSION_TYPE_MULTICHAT == session_type) || (SESSION_TYPE_ORGGROUP == session_type))//群组消息入群组消息表
			{
				sql = "insert into t_im_receipt_msg(msg_id, from_company_id, from_user_id, to_company_id, to_user_id, update_time) \
					  	  values(?, ?, ?, ?, ?, ?) ";
				if (0 != statement.init(mysql, sql))
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				struct timeval tv;
				gettimeofday(&tv, NULL);
				uint64_t update_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
				statement.set_param_bind(5, update_time);
			}
			string msg_id = msg_data.msg_id();
			statement.set_param_bind(0, msg_id);
			uint32_t from_company_id = msg_data.from_company_id();
			statement.set_param_bind(1, from_company_id);
			uint32_t from_user_id = msg_data.from_user_id();
			statement.set_param_bind(2, from_user_id);
			uint32_t to_company_id = msg_data.to_company_id();
			statement.set_param_bind(3, to_company_id);
			uint32_t to_user_id = msg_data.to_user_id();
			statement.set_param_bind(4, to_user_id);
			if (0 != statement.execute())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			}
		} while (false);
		delete ptask;
		statement.free();	
	}

	void get_receipt_list(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		log(LOG_DEBUG, "%s:%s()", __FILE__, __FUNCTION__);
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Message::IMMsgReceiptListReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			string sql("select to_company_id, to_user_id, update_time from t_im_receipt_msg where msg_id = ? \
					        and from_company_id = ? and from_user_id = ? and update_time < ? order by update_time desc limit 0, 100");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			string msg_id = msg_data.msg_id();
			statement.set_param_bind(0, msg_id);
			uint32_t from_company_id = msg_data.company_id();
			statement.set_param_bind(1, from_company_id);
			uint32_t from_user_id = msg_data.user_id();
			statement.set_param_bind(2, from_user_id);
			uint64_t update_time = msg_data.update_time();
			if (update_time == 0)
			{
				struct timeval tv;
				gettimeofday(&tv, NULL);
				update_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
			}
			statement.set_param_bind(3, update_time);
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
			uint64_t update_time_temp;
			length = sizeof(update_time_temp);
			statement.set_param_result(2, MYSQL_TYPE_LONGLONG, (char*)(&update_time_temp), length, NULL);
			if (0 != statement.get_result())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::Message::IMMsgReceiptListRsp msg_rsp;
			msg_rsp.set_msg_id(msg_id);
			msg_rsp.set_group_company_id(msg_data.group_company_id());
			msg_rsp.set_group_id(msg_data.group_id());
			uint32_t is_more_data = 0;
			if (statement.get_num_rows() == 100)
			{
				is_more_data = 1;
			}
			while (0 == statement.fetch_result())
			{
				IM::BaseDefine::UserIdItem *receipt_user_list = msg_rsp.add_receipt_user_list();
				receipt_user_list->set_company_id(to_company_id);
				receipt_user_list->set_user_id(to_user_id);
				msg_rsp.set_update_time(update_time_temp);
			}
			msg_rsp.set_is_more_data(is_more_data);
			msg->set_cmd_id(CID_MSG_RECEIPT_LIST_RSP);
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
		} while (false);
		statement.free();
	}

	void get_offline_msg_cnt(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			string sql("select count(*) as cnt, msg_id from (select * from t_im_chat_offline_msg where to_company_id = ? and to_user_id = ? order by create_time desc) \
							as test group by from_company_id, from_user_id");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::Message::IMOfflineMsgCntReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t to_company_id = msg_data.from_company_id();
			statement.set_param_bind(0, to_company_id);
			uint32_t to_user_id = msg_data.from_user_id();
			log(LOG_DEBUG, "get_offline_msg_cnt user_id:%d", to_user_id);
			statement.set_param_bind(1, to_user_id);
			if (0 != statement.query())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			int cnt;
			unsigned long length = sizeof(cnt);
			statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&cnt), length, NULL);
			char msg_id[32];
			unsigned long msg_id_length = sizeof(msg_id);
			statement.set_param_result(1, MYSQL_TYPE_STRING, msg_id, msg_id_length, &msg_id_length);
			if (0 != statement.get_result())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::Message::IMOfflineMsgCntRsp msg_rsp;
			char *msg_blob = new char[8 * 1024];
			pre_statement state;
			sql = string("select msg_data from t_im_chat_msg where msg_id = ?");
			if (0 != state.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d:state.init( ) failed", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			while (0 == statement.fetch_result())
			{
				string str_msg_id;
				str_msg_id.append(msg_id, msg_id_length);
				state.set_param_bind(0, str_msg_id);
				if (0 != state.query())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d:state.query( ) failed", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				unsigned long msg_length = 8 * 1024;
				//调用完成之后，length保存结果的长度
				state.set_param_result(0, MYSQL_TYPE_STRING, msg_blob, msg_length, &msg_length);
				if (0 != state.get_result())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d:state.get_result( ) failed", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				if (0 == state.fetch_result())
				{
					IM::BaseDefine::OfflineMsgCntInfo *msg_cnt = msg_rsp.add_offline_msg_list();
					msg_cnt->set_msg_cnt(cnt);
					IM::BaseDefine::MsgData *msg_blob_temp = msg_cnt->mutable_lastest_msg_data();
					msg_blob_temp->ParsePartialFromArray(msg_blob, msg_length);
					log(LOG_DEBUG, "get_offline_msg_cnt user_id:%d num:%d", msg_blob_temp->from_user_id(), cnt);
				}
			}
			delete msg_blob;
			statement.free();
			sql = "delete from t_im_chat_offline_msg where to_company_id = ? and to_user_id = ?";
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			statement.set_param_bind(0, to_company_id);
			statement.set_param_bind(1, to_user_id);
			if (0 != statement.execute())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			}
			msg->set_cmd_id(CID_MSG_OFFLINE_COUNT_RSP);
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
		} while (false);
		statement.free();
	}

	void get_group_offline_msg_cnt(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do
		{
			IM::Message::IMOfflineMsgCntReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t to_company_id = msg_data.from_company_id();
			string table_name = string("t_im_user_group_") + to_string(to_company_id % 8);
			string table_name_temp = string("t_im_user_vcard_") + to_string(to_company_id % 8);
			string sql = string("select friend_company_id, friend_group_id, (select pc_offline_time from ") + table_name_temp + 
							string(" where company_id = ? and user_id = ?), (select mobile_offline_time from ") + table_name_temp + 
							string(" where company_id = ? and user_id = ?) from ") + table_name + string(" where company_id = ? and user_id = ? and is_delete = 0");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			statement.set_param_bind(0, to_company_id);
			uint32_t to_user_id = msg_data.from_user_id();
			log(LOG_DEBUG, "get_group_offline_msg_cnt user_id:%d", to_user_id);
			statement.set_param_bind(1, to_user_id);
			statement.set_param_bind(2, to_company_id);
			statement.set_param_bind(3, to_user_id);
			statement.set_param_bind(4, to_company_id);
			statement.set_param_bind(5, to_user_id);
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
			uint64_t pc_offline_time;
			length = sizeof(pc_offline_time);
			statement.set_param_result(2, MYSQL_TYPE_LONGLONG, (char*)(&pc_offline_time), length, NULL);
			uint64_t mobile_offline_time;
			length = sizeof(mobile_offline_time);
			statement.set_param_result(3, MYSQL_TYPE_LONGLONG, (char*)(&mobile_offline_time), length, NULL);
			if (0 != statement.get_result())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::Message::IMOfflineMsgCntRsp msg_rsp;
			char *msg_blob = new char[8 * 1024];
			pre_statement state;
			sql = string("select count(1) as cnt, msg_data from (select msg_data from t_im_group_msg where to_company_id = ? and to_group_id = ? \
						 		and create_time >= ? order by create_time desc) as test");
			if (0 != state.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			while (0 == statement.fetch_result())
			{
				state.set_param_bind(0, friend_company_id);
				state.set_param_bind(1, friend_group_id);
				uint64_t msg_time = ((pc_offline_time >= mobile_offline_time)?pc_offline_time:mobile_offline_time);
				state.set_param_bind(2, msg_time);
				if (0 != state.query())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				int cnt;
				unsigned long length = sizeof(cnt);
				state.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&cnt), length, NULL);
				length = 8 * 1024;
				state.set_param_result(1, MYSQL_TYPE_STRING, msg_blob, length, &length);
				if (0 != state.get_result())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				while (0 == state.fetch_result())
				{
					if (cnt == 0)
					{
						continue;
					}
					log(LOG_DEBUG, "get_group_offline_msg_cnt group_id:%d num:%d", friend_group_id, cnt);
					IM::BaseDefine::OfflineMsgCntInfo *msg_cnt = msg_rsp.add_offline_msg_list();
					msg_cnt->set_msg_cnt(cnt);
					IM::BaseDefine::MsgData *msg_blob_temp = msg_cnt->mutable_lastest_msg_data();
					msg_blob_temp->ParsePartialFromArray(msg_blob, length);
				}
			}
			statement.free();
			string table_org = string("t_im_org_") + to_string(to_company_id % 8);
			sql = string("select p.node_id from ") + table_org + string(" as p, ") + table_org + string(" as n where n.lft between p.lft and p.rgh \
									   	 and n.company_id = ? and n.user_id = ? and p.node_type = 0 and p.node_id != 0");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			statement.set_param_bind(0, to_company_id);
			statement.set_param_bind(1, to_user_id);
			if (0 != statement.query())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t org_id;
			length = sizeof(org_id);
			statement.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&org_id), length, NULL);
			if (0 != statement.get_result())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			state.free();
			sql = string("select count(1) as cnt, msg_data from (select msg_data from t_im_group_msg where to_company_id = ? and to_group_id = ? \
						 		and create_time >= ? order by create_time desc) as test");
			if (0 != state.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			while (0 == statement.fetch_result())
			{
				state.set_param_bind(0, friend_company_id);
				state.set_param_bind(1, org_id);
				uint64_t msg_time = ((pc_offline_time >= mobile_offline_time) ? pc_offline_time : mobile_offline_time);
				state.set_param_bind(2, msg_time);
				if (0 != state.query())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				int cnt;
				unsigned long length = sizeof(cnt);
				state.set_param_result(0, MYSQL_TYPE_LONG, (char*)(&cnt), length, NULL);
				length = 8 * 1024;
				state.set_param_result(1, MYSQL_TYPE_STRING, msg_blob, length, &length);
				if (0 != state.get_result())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				while (0 == state.fetch_result())
				{
					if (cnt == 0)
					{
						continue;
					}
					log(LOG_DEBUG, "get_group_offline_msg_cnt org_id:%d num:%d", org_id, cnt);
					IM::BaseDefine::OfflineMsgCntInfo *msg_cnt = msg_rsp.add_offline_msg_list();
					msg_cnt->set_msg_cnt(cnt);
					IM::BaseDefine::MsgData *msg_blob_temp = msg_cnt->mutable_lastest_msg_data();
					msg_blob_temp->ParsePartialFromArray(msg_blob, length);
				}
			}
			delete msg_blob;
			msg->set_cmd_id(CID_GMSG_OFFLINE_COUNT_RSP);
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
		} while (false);
		statement.free();
	}

	void get_offline_msg(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do
		{
			IM::Message::IMGetMsgDataListReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::BaseDefine::SessionType session_type = msg_data.session_type();
			string sql;
			if (SESSION_TYPE_SINGLE == session_type)
			{
				uint64_t create_time = msg_data.latest_msg_time();
				if (0 == create_time)
				{
					uint64_t end_time = msg_data.end_msg_time();
					if (0 == end_time)
					{
						sql = "select msg_data from t_im_chat_msg where (from_company_id = ? and from_user_id = ? \
								  and to_company_id = ? and to_user_id = ?) or (from_company_id = ? and from_user_id = ? \
								  and to_company_id = ? and to_user_id = ?) order by create_time desc";
						if (0 != statement.init(mysql, sql))
						{
							log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
							break;
						}
						uint32_t from_company_id = msg_data.from_company_id();
						statement.set_param_bind(0, from_company_id);
						uint32_t from_user_id = msg_data.from_user_id();
						statement.set_param_bind(1, from_user_id);
						uint32_t to_company_id = msg_data.to_company_id();
						statement.set_param_bind(2, to_company_id);
						uint32_t to_user_id = msg_data.to_user_id();
						statement.set_param_bind(3, to_user_id);
						statement.set_param_bind(4, to_company_id);
						statement.set_param_bind(5, to_user_id);
						statement.set_param_bind(6, from_company_id);
						statement.set_param_bind(7, from_user_id);
						log(LOG_DEBUG, "get_offline_msg latest_msg_time:%lld end_msg_time:%lld", create_time, end_time);
					}
					else
					{
						sql = "select msg_data from t_im_chat_msg where ((from_company_id = ? and from_user_id = ? \
							  	  and to_company_id = ? and to_user_id = ?) or (from_company_id = ? and from_user_id = ? \
								  and to_company_id = ? and to_user_id = ?)) and create_time >= ? order by create_time desc";
						if (0 != statement.init(mysql, sql))
						{
							log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
							break;
						}
						uint32_t from_company_id = msg_data.from_company_id();
						statement.set_param_bind(0, from_company_id);
						uint32_t from_user_id = msg_data.from_user_id();
						statement.set_param_bind(1, from_user_id);
						uint32_t to_company_id = msg_data.to_company_id();
						statement.set_param_bind(2, to_company_id);
						uint32_t to_user_id = msg_data.to_user_id();
						statement.set_param_bind(3, to_user_id);
						statement.set_param_bind(4, to_company_id);
						statement.set_param_bind(5, to_user_id);
						statement.set_param_bind(6, from_company_id);
						statement.set_param_bind(7, from_user_id);
						statement.set_param_bind(8, end_time);
						log(LOG_DEBUG, "get_offline_msg latest_msg_time:%lld end_msg_time:%lld", create_time, end_time);
					}
				}
				else
				{
					uint64_t end_time = msg_data.end_msg_time();
					if (0 == end_time)
					{
						sql = "select msg_data from t_im_chat_msg where ((from_company_id = ? and from_user_id = ? \
								  and to_company_id = ? and to_user_id = ?) or (from_company_id = ? and from_user_id = ? \
								  and to_company_id = ? and to_user_id = ?)) and create_time <= ? order by create_time desc";
						if (0 != statement.init(mysql, sql))
						{
							log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
							break;
						}
						uint32_t from_company_id = msg_data.from_company_id();
						statement.set_param_bind(0, from_company_id);
						uint32_t from_user_id = msg_data.from_user_id();
						statement.set_param_bind(1, from_user_id);
						uint32_t to_company_id = msg_data.to_company_id();
						statement.set_param_bind(2, to_company_id);
						uint32_t to_user_id = msg_data.to_user_id();
						statement.set_param_bind(3, to_user_id);
						statement.set_param_bind(4, to_company_id);
						statement.set_param_bind(5, to_user_id);
						statement.set_param_bind(6, from_company_id);
						statement.set_param_bind(7, from_user_id);
						statement.set_param_bind(8, create_time);
						log(LOG_DEBUG, "get_offline_msg latest_msg_time:%lld end_msg_time:%lld", create_time, end_time);
					}
					else
					{
						sql = "select msg_data from t_im_chat_msg where ((from_company_id = ? and from_user_id = ? \
							  	  and to_company_id = ? and to_user_id = ?) or (from_company_id = ? and from_user_id = ? \
								  and to_company_id = ? and to_user_id = ?)) and (create_time between ? and ?) order by create_time desc";
						if (0 != statement.init(mysql, sql))
						{
							log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
							break;
						}
						uint32_t from_company_id = msg_data.from_company_id();
						statement.set_param_bind(0, from_company_id);
						uint32_t from_user_id = msg_data.from_user_id();
						statement.set_param_bind(1, from_user_id);
						uint32_t to_company_id = msg_data.to_company_id();
						statement.set_param_bind(2, to_company_id);
						uint32_t to_user_id = msg_data.to_user_id();
						statement.set_param_bind(3, to_user_id);
						statement.set_param_bind(4, to_company_id);
						statement.set_param_bind(5, to_user_id);
						statement.set_param_bind(6, from_company_id);
						statement.set_param_bind(7, from_user_id);
						statement.set_param_bind(8, end_time);
						statement.set_param_bind(9, create_time);
						log(LOG_DEBUG, "get_offline_msg latest_msg_time:%lld end_msg_time:%lld", create_time, end_time);
					}
				}
			}
			else if (SESSION_TYPE_MULTICHAT == session_type)
			{
				uint64_t create_time = msg_data.latest_msg_time();
				if (0 == create_time)
				{
					uint64_t end_time = msg_data.end_msg_time();
					if (0 == end_time)
					{
						sql = "select msg_data from t_im_group_msg where to_company_id = ? and to_group_id = ? order by create_time desc";
						if (0 != statement.init(mysql, sql))
						{
							log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
							break;
						}
						uint32_t to_company_id = msg_data.to_company_id();
						statement.set_param_bind(0, to_company_id);
						uint32_t to_group_id = msg_data.to_user_id();
						statement.set_param_bind(1, to_group_id);
						log(LOG_DEBUG, "get_offline_msg group_id:%lld latest_msg_time:%lld end_msg_time:%lld", to_group_id, create_time, end_time);
					}
					else
					{
						sql = "select msg_data from t_im_group_msg where to_company_id = ? and to_group_id = ? and create_time >= ? order by create_time desc";
						if (0 != statement.init(mysql, sql))
						{
							log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
							break;
						}
						uint32_t to_company_id = msg_data.to_company_id();
						statement.set_param_bind(0, to_company_id);
						uint32_t to_group_id = msg_data.to_user_id();
						statement.set_param_bind(1, to_group_id);
						statement.set_param_bind(2, end_time);
						log(LOG_DEBUG, "get_offline_msg group_id:%lld latest_msg_time:%lld end_msg_time:%lld", to_group_id, create_time, end_time);
					}
				}
				else
				{
					uint64_t end_time = msg_data.end_msg_time();
					if (0 == end_time)
					{
						sql = "select msg_data from t_im_group_msg where to_company_id = ? and to_group_id = ? and create_time <= ? order by create_time desc";
						if (0 != statement.init(mysql, sql))
						{
							log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
							break;
						}
						uint32_t to_company_id = msg_data.to_company_id();
						statement.set_param_bind(0, to_company_id);
						uint32_t to_group_id = msg_data.to_user_id();
						statement.set_param_bind(1, to_group_id);
						statement.set_param_bind(2, create_time);
						log(LOG_DEBUG, "get_offline_msg group_id:%lld latest_msg_time:%lld end_msg_time:%lld", to_group_id, create_time, end_time);
					}
					else
					{
						sql = "select msg_data from t_im_group_msg where to_company_id = ? and to_group_id = ? and (create_time between ? and ?) order by create_time desc";
						if (0 != statement.init(mysql, sql))
						{
							log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
							break;
						}
						uint32_t to_company_id = msg_data.to_company_id();
						statement.set_param_bind(0, to_company_id);
						uint32_t to_group_id = msg_data.to_user_id();
						statement.set_param_bind(1, to_group_id);
						statement.set_param_bind(2, end_time);
						statement.set_param_bind(3, create_time);
						log(LOG_DEBUG, "get_offline_msg group_id:%lld latest_msg_time:%lld end_msg_time:%lld", to_group_id, create_time, end_time);
					}
				}
			}
			else if (SESSION_TYPE_ORGGROUP == session_type)
			{
				uint64_t create_time = msg_data.latest_msg_time();
				if (0 == create_time)
				{
					uint64_t end_time = msg_data.end_msg_time();
					if (0 == end_time)
					{
						sql = "select msg_data from t_im_orggroup_msg where to_company_id = ? and to_org_id = ? order by create_time desc";
						if (0 != statement.init(mysql, sql))
						{
							log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
							break;
						}
						uint32_t to_company_id = msg_data.to_company_id();
						statement.set_param_bind(0, to_company_id);
						uint32_t to_org_id = msg_data.to_user_id();
						statement.set_param_bind(1, to_org_id);
						log(LOG_DEBUG, "get_offline_msg group_id:%lld latest_msg_time:%lld end_msg_time:%lld", to_org_id, create_time, end_time);
					}
					else
					{
						sql = "select msg_data from t_im_orggroup_msg where to_company_id = ? and to_org_id = ? and create_time >= ? order by create_time desc";
						if (0 != statement.init(mysql, sql))
						{
							log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
							break;
						}
						uint32_t to_company_id = msg_data.to_company_id();
						statement.set_param_bind(0, to_company_id);
						uint32_t to_org_id = msg_data.to_user_id();
						statement.set_param_bind(1, to_org_id);
						statement.set_param_bind(2, end_time);
						log(LOG_DEBUG, "get_offline_msg group_id:%lld latest_msg_time:%lld end_msg_time:%lld", to_org_id, create_time, end_time);
					}
				}
				else
				{
					uint64_t end_time = msg_data.end_msg_time();
					if (0 == end_time)
					{
						sql = "select msg_data from t_im_orggroup_msg where to_company_id = ? and to_org_id = ? and create_time <= ? order by create_time desc";
						if (0 != statement.init(mysql, sql))
						{
							log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
							break;
						}
						uint32_t to_company_id = msg_data.to_company_id();
						statement.set_param_bind(0, to_company_id);
						uint32_t to_org_id = msg_data.to_user_id();
						statement.set_param_bind(1, to_org_id);
						statement.set_param_bind(2, create_time);
						log(LOG_DEBUG, "get_offline_msg group_id:%lld latest_msg_time:%lld end_msg_time:%lld", to_org_id, create_time, end_time);
					}
					else
					{
						sql = "select msg_data from t_im_orggroup_msg where to_company_id = ? and to_org_id = ? and (create_time between ? and ?) order by create_time desc";
						if (0 != statement.init(mysql, sql))
						{
							log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
							break;
						}
						uint32_t to_company_id = msg_data.to_company_id();
						statement.set_param_bind(0, to_company_id);
						uint32_t to_org_id = msg_data.to_user_id();
						statement.set_param_bind(1, to_org_id);
						statement.set_param_bind(2, end_time);
						statement.set_param_bind(3, create_time);
						log(LOG_DEBUG, "get_offline_msg group_id:%lld latest_msg_time:%lld end_msg_time:%lld", to_org_id, create_time, end_time);
					}
				}
			}
			if (0 != statement.query())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			char *msg_blob = new char[8 * 1024];
			unsigned long msg_length = 8 * 1024;
			//调用完成之后，length保存结果的长度
			statement.set_param_result(0, MYSQL_TYPE_STRING, msg_blob, msg_length, &msg_length);
			if (0 != statement.get_result())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::Message::IMGetMsgDataListRsp msg_rsp;
			uint32_t is_more_data = 0;
			unsigned long pack_length = HEADER_LEN;
			while (0 == statement.fetch_result())
			{
				pack_length += msg_length;
				if (pack_length > 8 * 1024)
				{
					is_more_data = 1;
					break;
				}
				IM::BaseDefine::MsgData *msg_data_list = msg_rsp.add_msg_data_list();
				msg_data_list->ParsePartialFromArray(msg_blob, msg_length);
			}
			msg->set_cmd_id(CID_MSG_OFFLINE_LIST_RSP);
			msg_rsp.set_company_id(msg_data.to_company_id());
			msg_rsp.set_user_id(msg_data.to_user_id());
			msg_rsp.set_session_type(msg_data.session_type());
			msg_rsp.set_is_more_data(is_more_data);
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
			delete msg_blob;
		} while (false);
		statement.free();
	}

	void get_history_msg(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do
		{
			IM::Message::IMGetMsgDataListReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::BaseDefine::SessionType session_type = msg_data.session_type();
			string sql;
			if (SESSION_TYPE_SINGLE == session_type)
			{
				uint64_t create_time = msg_data.latest_msg_time();
				if (0 == create_time)
				{
					uint64_t end_time = msg_data.end_msg_time();
					if (0 == end_time)
					{
						sql = "select msg_data from t_im_chat_msg where (from_company_id = ? and from_user_id = ? \
							  	  and to_company_id = ? and to_user_id = ?) or (from_company_id = ? and from_user_id = ? \
								  and to_company_id = ? and to_user_id = ?) order by create_time desc";
						if (0 != statement.init(mysql, sql))
						{
							log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
							break;
						}
						uint32_t from_company_id = msg_data.from_company_id();
						statement.set_param_bind(0, from_company_id);
						uint32_t from_user_id = msg_data.from_user_id();
						statement.set_param_bind(1, from_user_id);
						uint32_t to_company_id = msg_data.to_company_id();
						statement.set_param_bind(2, to_company_id);
						uint32_t to_user_id = msg_data.to_user_id();
						statement.set_param_bind(3, to_user_id);
						statement.set_param_bind(4, to_company_id);
						statement.set_param_bind(5, to_user_id);
						statement.set_param_bind(6, from_company_id);
						statement.set_param_bind(7, from_user_id);
						log(LOG_DEBUG, "get_history_msg user_id:%d user_id:%d latest_msg_time:%lld end_msg_time:%lld", from_user_id, to_user_id, create_time, end_time);
					}
					else
					{
						sql = "select msg_data from t_im_chat_msg where ((from_company_id = ? and from_user_id = ? \
							  	  and to_company_id = ? and to_user_id = ?) or (from_company_id = ? and from_user_id = ? \
								  and to_company_id = ? and to_user_id = ?)) and create_time >= ? order by create_time desc";
						if (0 != statement.init(mysql, sql))
						{
							log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
							break;
						}
						uint32_t from_company_id = msg_data.from_company_id();
						statement.set_param_bind(0, from_company_id);
						uint32_t from_user_id = msg_data.from_user_id();
						statement.set_param_bind(1, from_user_id);
						uint32_t to_company_id = msg_data.to_company_id();
						statement.set_param_bind(2, to_company_id);
						uint32_t to_user_id = msg_data.to_user_id();
						statement.set_param_bind(3, to_user_id);
						statement.set_param_bind(4, to_company_id);
						statement.set_param_bind(5, to_user_id);
						statement.set_param_bind(6, from_company_id);
						statement.set_param_bind(7, from_user_id);
						statement.set_param_bind(8, end_time);
						log(LOG_DEBUG, "get_history_msg user_id:%d user_id:%d latest_msg_time:%lld end_msg_time:%lld", from_user_id, to_user_id, create_time, end_time);
					}
				}
				else
				{
					uint64_t end_time = msg_data.end_msg_time();
					if (0 == end_time)
					{
						sql = "select msg_data from t_im_chat_msg where ((from_company_id = ? and from_user_id = ? \
							  	  and to_company_id = ? and to_user_id = ?) or (from_company_id = ? and from_user_id = ? \
								  and to_company_id = ? and to_user_id = ?)) and create_time <= ? order by create_time desc";
						if (0 != statement.init(mysql, sql))
						{
							log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
							break;
						}
						uint32_t from_company_id = msg_data.from_company_id();
						statement.set_param_bind(0, from_company_id);
						uint32_t from_user_id = msg_data.from_user_id();
						statement.set_param_bind(1, from_user_id);
						uint32_t to_company_id = msg_data.to_company_id();
						statement.set_param_bind(2, to_company_id);
						uint32_t to_user_id = msg_data.to_user_id();
						statement.set_param_bind(3, to_user_id);
						statement.set_param_bind(4, to_company_id);
						statement.set_param_bind(5, to_user_id);
						statement.set_param_bind(6, from_company_id);
						statement.set_param_bind(7, from_user_id);
						statement.set_param_bind(8, create_time);
						log(LOG_DEBUG, "get_history_msg user_id:%d user_id:%d latest_msg_time:%lld end_msg_time:%lld", from_user_id, to_user_id, create_time, end_time);
					}
					else
					{
						sql = "select msg_data from t_im_chat_msg where ((from_company_id = ? and from_user_id = ? \
							  	  and to_company_id = ? and to_user_id = ?) or (from_company_id = ? and from_user_id = ? \
								  and to_company_id = ? and to_user_id = ?)) and (create_time between ? and ?) order by create_time desc";
						if (0 != statement.init(mysql, sql))
						{
							log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
							break;
						}
						uint32_t from_company_id = msg_data.from_company_id();
						statement.set_param_bind(0, from_company_id);
						uint32_t from_user_id = msg_data.from_user_id();
						statement.set_param_bind(1, from_user_id);
						uint32_t to_company_id = msg_data.to_company_id();
						statement.set_param_bind(2, to_company_id);
						uint32_t to_user_id = msg_data.to_user_id();
						statement.set_param_bind(3, to_user_id);
						statement.set_param_bind(4, to_company_id);
						statement.set_param_bind(5, to_user_id);
						statement.set_param_bind(6, from_company_id);
						statement.set_param_bind(7, from_user_id);
						statement.set_param_bind(8, end_time);
						statement.set_param_bind(9, create_time);
						log(LOG_DEBUG, "get_history_msg user_id:%d user_id:%d latest_msg_time:%lld end_msg_time:%lld", from_user_id, to_user_id, create_time, end_time);
					}
				}
			}
			else if (SESSION_TYPE_MULTICHAT == session_type)
			{
				uint64_t create_time = msg_data.latest_msg_time();
				if (0 == create_time)
				{
					uint64_t end_time = msg_data.end_msg_time();
					if (0 == end_time)
					{
						sql = "select msg_data from t_im_group_msg where to_company_id = ? and to_group_id = ? order by create_time desc";
						if (0 != statement.init(mysql, sql))
						{
							log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
							break;
						}
						uint32_t to_company_id = msg_data.to_company_id();
						statement.set_param_bind(0, to_company_id);
						uint32_t to_group_id = msg_data.to_user_id();
						statement.set_param_bind(1, to_group_id);
						log(LOG_DEBUG, "get_history_msg group_id:%d latest_msg_time:%lld end_msg_time:%lld", to_group_id, create_time, end_time);
					}
					else
					{
						sql = "select msg_data from t_im_group_msg where to_company_id = ? and to_group_id = ? and create_time >= ? order by create_time desc";
						if (0 != statement.init(mysql, sql))
						{
							log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
							break;
						}
						uint32_t to_company_id = msg_data.to_company_id();
						statement.set_param_bind(0, to_company_id);
						uint32_t to_group_id = msg_data.to_user_id();
						statement.set_param_bind(1, to_group_id);
						statement.set_param_bind(2, end_time);
						log(LOG_DEBUG, "get_history_msg group_id:%d latest_msg_time:%lld end_msg_time:%lld", to_group_id, create_time, end_time);
					}
				}
				else
				{
					uint64_t end_time = msg_data.end_msg_time();
					if (0 == end_time)
					{
						sql = "select msg_data from t_im_group_msg where to_company_id = ? and to_group_id = ? and create_time <= ? order by create_time desc";
						if (0 != statement.init(mysql, sql))
						{
							log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
							break;
						}
						uint32_t to_company_id = msg_data.to_company_id();
						statement.set_param_bind(0, to_company_id);
						uint32_t to_group_id = msg_data.to_user_id();
						statement.set_param_bind(1, to_group_id);
						statement.set_param_bind(2, create_time);
						log(LOG_DEBUG, "get_history_msg group_id:%d latest_msg_time:%lld end_msg_time:%lld", to_group_id, create_time, end_time);
					}
					else
					{
						sql = "select msg_data from t_im_group_msg where to_company_id = ? and to_group_id = ? and (create_time between ? and ?) order by create_time desc";
						if (0 != statement.init(mysql, sql))
						{
							log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
							break;
						}
						uint32_t to_company_id = msg_data.to_company_id();
						statement.set_param_bind(0, to_company_id);
						uint32_t to_group_id = msg_data.to_user_id();
						statement.set_param_bind(1, to_group_id);
						statement.set_param_bind(2, end_time);
						statement.set_param_bind(3, create_time);
						log(LOG_DEBUG, "get_history_msg group_id:%d latest_msg_time:%lld end_msg_time:%lld", to_group_id, create_time, end_time);
					}
				}
			}
			else if (SESSION_TYPE_ORGGROUP == session_type)
			{
				uint64_t create_time = msg_data.latest_msg_time();
				if (0 == create_time)
				{
					uint64_t end_time = msg_data.end_msg_time();
					if (0 == end_time)
					{
						sql = "select msg_data from t_im_orggroup_msg where to_company_id = ? and to_org_id = ? order by create_time desc";
						if (0 != statement.init(mysql, sql))
						{
							log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
							break;
						}
						uint32_t to_company_id = msg_data.to_company_id();
						statement.set_param_bind(0, to_company_id);
						uint32_t to_org_id = msg_data.to_user_id();
						statement.set_param_bind(1, to_org_id);
						log(LOG_DEBUG, "get_history_msg org_id:%d latest_msg_time:%lld end_msg_time:%lld", to_org_id, create_time, end_time);
					}
					else
					{
						sql = "select msg_data from t_im_orggroup_msg where to_company_id = ? and to_org_id = ? and create_time >= ? order by create_time desc";
						if (0 != statement.init(mysql, sql))
						{
							log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
							break;
						}
						uint32_t to_company_id = msg_data.to_company_id();
						statement.set_param_bind(0, to_company_id);
						uint32_t to_org_id = msg_data.to_user_id();
						statement.set_param_bind(1, to_org_id);
						statement.set_param_bind(2, end_time);
						log(LOG_DEBUG, "get_history_msg org_id:%d latest_msg_time:%lld end_msg_time:%lld", to_org_id, create_time, end_time);
					}
				}
				else
				{
					uint64_t end_time = msg_data.end_msg_time();
					if (0 == end_time)
					{
						sql = "select msg_data from t_im_orggroup_msg where to_company_id = ? and to_org_id = ? and create_time <= ? order by create_time desc";
						if (0 != statement.init(mysql, sql))
						{
							log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
							break;
						}
						uint32_t to_company_id = msg_data.to_company_id();
						statement.set_param_bind(0, to_company_id);
						uint32_t to_org_id = msg_data.to_user_id();
						statement.set_param_bind(1, to_org_id);
						statement.set_param_bind(2, create_time);
						log(LOG_DEBUG, "get_history_msg org_id:%d latest_msg_time:%lld end_msg_time:%lld", to_org_id, create_time, end_time);
					}
					else
					{
						sql = "select msg_data from t_im_orggroup_msg where to_company_id = ? and to_org_id = ? and (create_time between ? and ?) order by create_time desc";
						if (0 != statement.init(mysql, sql))
						{
							log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
							break;
						}
						uint32_t to_company_id = msg_data.to_company_id();
						statement.set_param_bind(0, to_company_id);
						uint32_t to_org_id = msg_data.to_user_id();
						statement.set_param_bind(1, to_org_id);
						statement.set_param_bind(2, end_time);
						statement.set_param_bind(3, create_time);
						log(LOG_DEBUG, "get_history_msg org_id:%d latest_msg_time:%lld end_msg_time:%lld", to_org_id, create_time, end_time);
					}
				}
			}
			if (0 != statement.query())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			char *msg_blob = new char[8 * 1024];
			unsigned long msg_length = 8 * 1024;
			//调用完成之后，length保存结果的长度
			statement.set_param_result(0, MYSQL_TYPE_STRING, msg_blob, msg_length, &msg_length);
			if (0 != statement.get_result())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::Message::IMGetMsgDataListRsp msg_rsp;
			uint32_t is_more_data = 0;
			unsigned long pack_length = HEADER_LEN;
			uint64_t create_time_temp = 1111111111111;
			int num = 0;
			while (0 == statement.fetch_result())
			{
				pack_length += msg_length;
				if (pack_length > 8 * 1024)
				{
					is_more_data = 1;
					break;
				}
				IM::BaseDefine::MsgData *msg_data_list = msg_rsp.add_msg_data_list();
				msg_data_list->ParsePartialFromArray(msg_blob, msg_length);
				log(LOG_DEBUG, "get_history_msg msg_id:%s create_time:%lld", msg_data_list->msg_id().c_str(), msg_data_list->create_time());
				num++;
				create_time_temp = msg_data_list->create_time();
			}
			if (session_type == 1)
			{
				log(LOG_DEBUG, "get_history_msg user_id:%d user_id:%d num:%d new_msg_time:%lld is_more:%d", msg_data.from_user_id(), msg_data.to_user_id(), num, create_time_temp, is_more_data);
			}
			else if (session_type == 2)
			{
				log(LOG_DEBUG, "get_history_msg group_id:%d num:%d new_msg_time:%lld is_more:%d", msg_data.to_user_id(), num, create_time_temp, is_more_data);
			}
			else if (session_type == 3)
			{
				log(LOG_DEBUG, "get_history_msg org_id:%d num:%d new_msg_time:%lld is_more:%d", msg_data.to_user_id(), num, create_time_temp, is_more_data);
			}
			msg->set_cmd_id(CID_MSG_DATA_LIST_RSP);
			msg_rsp.set_company_id(msg_data.to_company_id());
			msg_rsp.set_user_id(msg_data.to_user_id());
			msg_rsp.set_session_type(msg_data.session_type());
			msg_rsp.set_is_more_data(is_more_data);
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
			delete msg_blob;
		} while (false);
		statement.free();
	}

	void get_latest_msg_id(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		log(LOG_DEBUG, "%s:%s()", __FILE__, __FUNCTION__);
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Message::IMGetLatestMsgIdReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::BaseDefine::SessionType session_type = msg_data.session_type();
			string sql;
			if (SESSION_TYPE_SINGLE == session_type)
			{
				sql = "select msg_id from t_im_chat_msg where from_company_id = ? and from_user_id = ? \
					  	  and to_company_id = ? and to_user_id = ? order by create_time desc limit 1";
			}
			else if (SESSION_TYPE_MULTICHAT == session_type)
			{
				sql = "select msg_id from t_im_group_msg where from_company_id = ? and from_user_id = ? \
					  	  and to_company_id = ? and to_group_id = ? order by create_time desc limit 1";
			}
			else if (SESSION_TYPE_ORGGROUP == session_type)
			{
				sql = "select msg_id from t_im_orggroup_msg where from_company_id = ? and from_user_id = ? \
					  	  and to_company_id = ? and to_org_id = ? order by create_time desc limit 1";
			}
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t from_company_id = msg_data.from_company_id();
			statement.set_param_bind(0, from_company_id);
			uint32_t from_user_id = msg_data.from_user_id();
			statement.set_param_bind(1, from_user_id);
			uint32_t to_company_id = msg_data.to_company_id();
			statement.set_param_bind(2, to_company_id);
			uint32_t to_user_id = msg_data.to_user_id();
			statement.set_param_bind(3, to_user_id);
			if (0 != statement.query())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
			}
			else
			{
				char msg_id_cnt[32];
				unsigned long length = sizeof(msg_id_cnt);
				statement.set_param_result(0, MYSQL_TYPE_STRING, msg_id_cnt, length, NULL);
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
				IM::Message::IMGetLatestMsgIdRsp msg_rsp;
				msg_rsp.set_from_company_id(from_company_id);
				msg_rsp.set_from_user_id(from_user_id);
				msg_rsp.set_session_type(session_type);
				msg_rsp.set_to_company_id(to_company_id);
				msg_rsp.set_to_user_id(to_user_id);
				msg_rsp.set_latest_msg_id(msg_id_cnt, length);
				msg->set_cmd_id(CID_MSG_GET_LATEST_MSG_ID_RSP);
				msg->set_pb_length(msg_rsp.ByteSize());
				msg->write_msg(&msg_rsp);
				net_server::get_instance()->add_response(ptask);
			}
		} while (false);
		statement.free();
	}

	void get_msg_by_id(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		log(LOG_DEBUG, "%s:%s()", __FILE__, __FUNCTION__);
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do 
		{
			IM::Message::IMGetMsgByIdReq msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::BaseDefine::SessionType session_type = msg_data.session_type();
			string sql;
			if (SESSION_TYPE_SINGLE == session_type)
			{
				sql = "select msg_data from t_im_chat_msg where msg_id = ?";
			}
			else if (SESSION_TYPE_MULTICHAT == session_type)
			{
				sql = "select msg_data from t_im_group_msg where msg_id = ?";
			}
			else if (SESSION_TYPE_ORGGROUP == session_type)
			{
				sql = "select msg_data from t_im_orggroup_msg where msg_id = ?";
			}
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			IM::Message::IMGetMsgByIdRsp msg_rsp;
			int msg_id_cnt = msg_data.msg_id_list_size();
			char *msg_blob = new char[8 * 1024];
			for (int i = 0; i < msg_id_cnt; i++)
			{
				string msg_id = msg_data.msg_id_list(i);
				statement.set_param_bind(0, msg_id);
				if (0 != statement.query())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					continue;
				}
				
				unsigned long length = 8 * 1024;
				//调用完成之后，length保存结果的长度
				statement.set_param_result(0, MYSQL_TYPE_STRING, msg_blob, length, &length);
				if (0 != statement.get_result())
				{
					log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
					break;
				}
				if (0 == statement.fetch_result())
				{
					IM::BaseDefine::MsgData *msg_blol_data = msg_rsp.add_msg_data();
					msg_blol_data->ParsePartialFromArray(msg_blob, length);
				}		
			}
			msg->set_cmd_id(CID_MSG_GET_BY_MSG_ID_RES);
			msg->set_pb_length(msg_rsp.ByteSize());
			msg->write_msg(&msg_rsp);
			net_server::get_instance()->add_response(ptask);
		} while (false);
		statement.free();
	}
}