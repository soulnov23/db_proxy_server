#include "handler_account.h"
#include "im_log.h"
#include "net_server.h"
#include "../protobuf/IM.BaseDefine.pb.h"
using namespace IM::BaseDefine;
#include "../protobuf/IM.Server.pb.h"
using namespace IM::Server;
#include "pre_statement.h"

void account_app(Json::Value &value)
{
	log(LOG_DEBUG, "%s:%s():%d result:%s", __FILE__, __FUNCTION__, __LINE__, value["result"].asString().c_str());
}

void account_init(Json::Value &value)
{
	log(LOG_DEBUG, "%s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
}

void account_login(evutil_socket_t fd, Json::Value &value, MYSQL *mysql, cache_conn *redis)
{
	log(LOG_DEBUG, "account_login msg_id:%s", value["msgid"].asString().c_str());
	do
	{
		task *ptask = net_server::get_instance()->find_map(atoi(value["msgid"].asString().c_str()));
		if (NULL == ptask)
		{
			break;
		}
		message *msg = static_cast<message *>(ptask->get_msg());
		IM::Server::IMLoginToSqlReq msg_data;
		if (!msg_data.ParsePartialFromArray(msg->get_pb_data(), msg->get_pb_length()))
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
		Json::Value body = value["body"];
		if (string("success") == value["result"].asString())
		{
			msg_rsp.set_result_code(RESULT_LOGIN_SUCCESSED);	
			IM::BaseDefine::UserInfo *user_info = msg_rsp.mutable_user_info();
			user_info->set_company_id(body["companyID"].asUInt());
			user_info->set_user_id(body["userID"].asUInt());
			user_info->set_user_real_name(body["name"].asString());
			user_info->set_user_english_name(body["englishName"].asString());
			user_info->set_user_nick_name(body["nickName"].asString());
			user_info->set_work_id(body["workID"].asUInt());
			Json::FastWriter writer;
			user_info->set_pos_json(writer.write(body["pos"]));
			user_info->set_user_borndate(body["birthday"].asUInt());
			user_info->set_user_gender(body["sex"].asUInt());
			user_info->set_email(body["email"].asString());
			user_info->set_user_mobile(body["mobile"].asString());
			user_info->set_user_tel(body["tel"].asString());
			user_info->set_avatar_url(body["headImg"].asString());
			user_info->set_avatar_update_time(body["headUpdateTime"].asUInt64());
			user_info->set_status(body["status"].asUInt());

			pre_statement statement;
			uint32_t company_id = msg_data.company_id();
			string table_name = string("t_im_user_vcard_") + to_string(company_id % 8);
			string sql = string("select sign_info, pc_offline_time, mobile_offline_time from ") + table_name + string(" where company_id = ? and user_id = ?");
			if (0 != statement.init(mysql, sql))
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			statement.set_param_bind(0, company_id);
			uint32_t user_id = body["userID"].asUInt();
			statement.set_param_bind(1, user_id);
			if (0 != statement.query())
			{
				log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
				break;
			}
			char sign_info[255] = { '\0' };
			unsigned long length = sizeof(sign_info);
			statement.set_param_result(0, MYSQL_TYPE_STRING, sign_info, length, NULL);
			uint64_t pc_offline_time;
			length = sizeof(pc_offline_time);
			statement.set_param_result(1, MYSQL_TYPE_LONGLONG, (char*)&pc_offline_time, length, NULL);
			uint64_t mobile_offline_time;
			length = sizeof(mobile_offline_time);
			statement.set_param_result(2, MYSQL_TYPE_LONGLONG, (char*)&mobile_offline_time, length, NULL);
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
			user_info->set_sign_info(sign_info);
			msg_rsp.set_pc_last_offline_time(pc_offline_time);
			msg_rsp.set_phone_last_offline_time(mobile_offline_time);
		}
		else if (string("fail") == value["result"].asString())
		{
			msg_rsp.set_result_code(RESULT_LOGIN_USER_OR_PWS_ERROR);
		}
		else if (string("nouser") == value["result"].asString())
		{
			msg_rsp.set_result_code(RESULT_LOGIN_VALIDATE_ERROR);
		}		
		msg->set_cmd_id(CID_OTHER_REQ_TO_MYSQL_RSP);
		msg->set_pb_length(msg_rsp.ByteSize());
		msg->write_msg(&msg_rsp);
		net_server::get_instance()->add_response(ptask);
	} while (false);
}