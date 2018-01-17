#include "handler_router.h"
#include "pre_statement.h"
#include <sys/time.h>
#include "im_log.h"
#include "net_server.h"
#include "../protobuf/IM.BaseDefine.pb.h"
using namespace IM::BaseDefine;
#include "../protobuf/IM.Router.pb.h"
#include "../protobuf/IM.UserState.pb.h"

namespace db_proxy
{
	void get_register_rsp(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		do 
		{
			IM::Server::RegisterServerRsp msg_data;
			message *msg = static_cast<message *>(ptask->get_msg());
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			uint32_t node_id = msg_data.node_id();		
			log(LOG_DEBUG, "This is register router response package node_id %d", node_id);
		} while (false);
		delete ptask;
	}

	void get_heart_beat(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		//log(LOG_DEBUG, "This is heartbeat package");
		delete ptask;
	}

	void get_user_state(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do
		{
			IM::UserState::UserStatUpdatePush msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			if (2 == msg_data.status())
			{
				if (msg_data.client_type() & CLIENT_TYPE_PC)
				{
					uint32_t company_id = msg_data.company_id();
					string table_name_a = string("t_im_user_vcard_") + to_string(company_id % 8);
					string sql = string("update ") + table_name_a + string(" set pc_offline_time = ? where company_id = ? and user_id = ?");
					if (0 != statement.init(mysql, sql))
					{
						log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
						break;
					}
					uint64_t update_time = msg_data.offline_time();
					statement.set_param_bind(0, update_time);
					statement.set_param_bind(1, company_id);
					uint32_t user_id = msg_data.user_id();
					statement.set_param_bind(2, user_id);
					log(LOG_DEBUG, "get_user_state pc_offline user_id=%d update_time=%lld", user_id, update_time);
					if (0 != statement.execute())
					{
						log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
						break;
					}
				}
				else if (msg_data.client_type() & CLIENT_TYPE_MOBILE)
				{
					uint32_t company_id = msg_data.company_id();
					string table_name_a = string("t_im_user_vcard_") + to_string(company_id % 8);
					string sql = string("update ") + table_name_a + string(" set mobile_offline_time = ? where company_id = ? and user_id = ?");
					if (0 != statement.init(mysql, sql))
					{
						log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
						break;
					}
					uint64_t update_time = msg_data.offline_time();
					statement.set_param_bind(0, update_time);
					statement.set_param_bind(1, company_id);
					uint32_t user_id = msg_data.user_id();
					statement.set_param_bind(2, user_id);
					log(LOG_DEBUG, "get_user_state mobile_offline user_id=%d update_time=%lld", user_id, update_time);
					if (0 != statement.execute())
					{
						log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
						break;
					}
				}
			}
		} while (false);
		delete ptask;
		statement.free();
	}

	void get_router_table(task *ptask, MYSQL *mysql, cache_conn *redis)
	{
		message *msg = static_cast<message *>(ptask->get_msg());
		pre_statement statement;
		do
		{
			IM::Server::ServerStatusPush msg_data;
			if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			if (!(net_server::get_instance()->m_status_enable))
			{
				int size = msg_data.server_list_size();
				for (int i = 0; i < size; i++)
				{
					IM::Server::ServerStatus server = msg_data.server_list(i);
					int len = server.service_type_size();
					for (int j = 0; j < len; j++)
					{
						if (SID_S2S_BROADCAST == server.service_type(j))
						{
							net_server::get_instance()->set_status_ip(server.server_ip());
							net_server::get_instance()->set_status_port(server.server_port());
							if (0 != net_server::get_instance()->do_status_connect())
							{
								net_server::get_instance()->do_status_re_connect();
							}
						}
					}
				}
			}
		} while (false);
		delete ptask;
		statement.free();	
	}
}
