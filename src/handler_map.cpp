#include "handler_map.h"
#include "handler_msg.h"
#include "handler_buddy.h"
#include "handler_server.h"
#include "handler_router.h"
#include "handler_group.h"
#include "handler_cache.h"
#include "../protobuf/IM.D2G.pb.h"
#include "../protobuf/IM.BaseDefine.pb.h"
using namespace IM::BaseDefine;

handler_map *handler_map::m_instance = new handler_map;

handler_map::handler_map()
{
	//消息处理
	m_handler_map.insert(make_pair(uint32_t(CID_MSG_DATA_OFFLINE), db_proxy::set_offline_msg_data));
	m_handler_map.insert(make_pair(uint32_t(CID_MSG_DATA_ONLINE), db_proxy::set_online_msg_data));
	m_handler_map.insert(make_pair(uint32_t(CID_MSG_DATA_DEL), db_proxy::set_recall_msg_data));
	m_handler_map.insert(make_pair(uint32_t(CID_MSG_DATA_OPT_NOTIFY), db_proxy::set_receipt_status));
	m_handler_map.insert(make_pair(uint32_t(CID_MSG_RECEIPT_LIST_REQ), db_proxy::get_receipt_list));
	m_handler_map.insert(make_pair(uint32_t(CID_MSG_OFFLINE_COUNT_REQ), db_proxy::get_offline_msg_cnt));
	m_handler_map.insert(make_pair(uint32_t(CID_GMSG_OFFLINE_COUNT_REQ), db_proxy::get_group_offline_msg_cnt));
	m_handler_map.insert(make_pair(uint32_t(CID_MSG_DATA_LIST_REQ), db_proxy::get_history_msg));
	m_handler_map.insert(make_pair(uint32_t(CID_MSG_OFFLINE_LIST_REQ), db_proxy::get_offline_msg));
	m_handler_map.insert(make_pair(uint32_t(CID_MSG_GET_LATEST_MSG_ID_REQ), db_proxy::get_latest_msg_id));
	m_handler_map.insert(make_pair(uint32_t(CID_MSG_GET_BY_MSG_ID_REQ), db_proxy::get_msg_by_id));
	
	//好友管理处理
	//m_handler_map.insert(make_pair(uint32_t(CID_BUDDY_LIST_RECENT_CONTACT_LIST_REQUEST), db_proxy::get_recent_chat));
	//m_handler_map.insert(make_pair(uint32_t(CID_BUDDY_LIST_RECENT_CONTACT_UPDATE_REQUEST), db_proxy::set_recent_chat));
	m_handler_map.insert(make_pair(uint32_t(CID_BUDDY_LISTITEM_REQ), db_proxy::set_recent_chat));
	m_handler_map.insert(make_pair(uint32_t(CID_BUDDY_LIST_FREQUENT_CONTACT_LIST_REQUEST), db_proxy::get_frequent_chat));
	m_handler_map.insert(make_pair(uint32_t(CID_BUDDY_LIST_SINGLE_USER_INFO_REQUEST), db_proxy::get_single_user_info));
	m_handler_map.insert(make_pair(uint32_t(CID_BUDDY_LIST_MODIFY_USER_INFO_REQUEST), db_proxy::set_user_info));
	//m_handler_map.insert(make_pair(uint32_t(CID_BUDDY_LIST_ALL_USER_REQUEST), db_proxy::get_all_user_info));
	m_handler_map.insert(make_pair(uint32_t(CID_BUDDY_LIST_CHANGE_AVATAR_REQUEST), db_proxy::set_user_avatar));
	m_handler_map.insert(make_pair(uint32_t(CID_BUDDY_LIST_MODIFY_NICKNAME_REQUEST), db_proxy::set_friend_remark));
	m_handler_map.insert(make_pair(uint32_t(CID_BUDDY_LIST_CHANGE_SIGN_INFO_REQUEST), db_proxy::set_user_sign_info));
	m_handler_map.insert(make_pair(uint32_t(CID_BUDDY_LIST_REQUEST_LIST_REQUEST), db_proxy::get_friend_req_list));

	//Server协议处理
	m_handler_map.insert(make_pair(uint32_t(CID_OTHER_REQ_TO_MYSQL_REQ), db_proxy::get_login_result));
	m_handler_map.insert(make_pair(uint32_t(CID_OTHER_ALTER_PSWD_REQ), db_proxy::set_modify_pwd));
	m_handler_map.insert(make_pair(uint32_t(CID_OTHER_GET_BUDDY_CONGFIG_REQUEST), db_proxy::get_buddy_config));
	m_handler_map.insert(make_pair(uint32_t(CID_OTHER_BUDDY_OPT_STORA_REQ), db_proxy::set_friend_req));
	m_handler_map.insert(make_pair(uint32_t(CID_OTHER_BUDDY_REQINFO_STORAGE), db_proxy::set_friend_req_result));
	m_handler_map.insert(make_pair(uint32_t(CID_OTHER_BUDDY_STORAGE_REQ), db_proxy::set_friend_manage));
	m_handler_map.insert(make_pair(uint32_t(CID_OTHER_RECENT_CONTACT_STORAGE_REQ), db_proxy::set_add_recent_chat));
	m_handler_map.insert(make_pair(uint32_t(CID_OTHER_DEL_RECENT_CONTCAT), db_proxy::set_del_recent_chat));
	m_handler_map.insert(make_pair(uint32_t(CID_OTHER_REPORT_ONLINE_INFO), db_proxy::set_online_report));
	m_handler_map.insert(make_pair(uint32_t(CID_OTHER_REPORT_MSG_INFO), db_proxy::set_msg_report));
	m_handler_map.insert(make_pair(uint32_t(CID_OTHER_GET_USER_ROLE_REQ), db_proxy::get_user_role));
	
	//路由处理
	m_handler_map.insert(make_pair(uint32_t(CID_ROUTER_CLIENT_REGISTER_RSP), db_proxy::get_register_rsp));
	m_handler_map.insert(make_pair(uint32_t(CID_OTHER_HEARTBEAT), db_proxy::get_heart_beat));
	m_handler_map.insert(make_pair(uint32_t(S_CID_USER_STAT_PUSH_REQ), db_proxy::get_user_state));
	m_handler_map.insert(make_pair(uint32_t(CID_ROUTER_PUSH_SERVER_STATUS), db_proxy::get_router_table));

	//群组管理处理
	m_handler_map.insert(make_pair(uint32_t(CID_GROUP_LIST_REQ), db_proxy::get_group_list));
	m_handler_map.insert(make_pair(uint32_t(CID_GROUP_LISTITEM_SET_REQ), db_proxy::set_group_list));
	m_handler_map.insert(make_pair(uint32_t(CID_GROUP_DISTURB_OPT_REQ), db_proxy::set_group_disturb));
	m_handler_map.insert(make_pair(uint32_t(CID_GROUP_OWNER_SET_REQ), db_proxy::set_group_owner));
	m_handler_map.insert(make_pair(uint32_t(CID_GROUP_INFO_REQ), db_proxy::get_group_info));
	m_handler_map.insert(make_pair(uint32_t(CID_GROUP_INFO_MODIFY_REQ), db_proxy::set_group_info));
	m_handler_map.insert(make_pair(uint32_t(CID_GROUP_MEMBER_REQ), db_proxy::get_group_members));
	m_handler_map.insert(make_pair(uint32_t(CID_GROUP_CREATE_REQ), db_proxy::set_group_create));
	m_handler_map.insert(make_pair(uint32_t(CID_GROUP_DISSOLVE_REQ), db_proxy::set_group_dissolve));
	m_handler_map.insert(make_pair(uint32_t(CID_GROUP_CHANGE_MEMBER_REQ), db_proxy::set_group_change));

	//Redis处理
	m_handler_map.insert(make_pair(uint32_t(S_CID_DB_COMPANY_USER_LIST_REQ), db_proxy::update_user_id));
	m_handler_map.insert(make_pair(uint32_t(S_CID_DB_USER_INFO_REQ), db_proxy::update_user_info));
	m_handler_map.insert(make_pair(uint32_t(S_CID_DB_USER_FRIEND_LIST_REQ), db_proxy::update_user_friend_list));
	m_handler_map.insert(make_pair(uint32_t(S_CID_DB_USER_GROUP_LIST_REQ), db_proxy::update_user_group_list));
	m_handler_map.insert(make_pair(uint32_t(S_CID_DB_GROUP_MEMBER_REQ), db_proxy::update_group_member_list));
	m_handler_map.insert(make_pair(uint32_t(S_CID_DB_ORGTREE_REQ), db_proxy::update_org_tree));
}

handler_map::~handler_map()
{
	delete m_instance;
	m_instance = NULL;
}

handler_map *handler_map::get_instance()
{
	return m_instance;
}

msg_handler_t handler_map::get_handler(uint16_t cmd_id)
{
	handler_map_t::iterator it = m_handler_map.find(cmd_id);
	if (it != m_handler_map.end())
	{
		return it->second;
	}
	else
	{
		return NULL;
	}
}