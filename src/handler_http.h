#ifndef _HANDLER_HTTP_H_
#define _HANDLER_HTTP_H_
/*****************************************************************************************
 *Copyright (C) 2007-2016 DriveTheLife. All Rights Reserved.
 *文件名称：handler_cache.h
 *描		述：Redis更新处理
 *当前版本：1.0
 *作		者：zhangcp
 *创建日期：2016-11-24 14:00
 *修改日期：2016-11-24
*******************************************************************************************/
#include "event2/util.h"
#include <string>
using namespace std;
#include "../jsoncpp/json/json.h"
using namespace Json;
#include "mysql.h"
#include "cache_conn.h"
#include "pre_statement.h"

//http后台登陆验证
void http_login(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//获取公司列表
void http_get_company_list(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//获取公司内所有员工列表
void http_get_company_user_list(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//获取公司架构
void http_get_company_organization(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//获取组织架构(只包括部门节点)
void http_get_organization(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

// 部门内所有员工节点信息列表
void http_get_all_node_member(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

// 部门内所有员工信息列表
void http_get_all_member(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//获取组织部门内成员
void http_get_node_member(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//获取用户详细信息
void http_get_user_info(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//添加公司
void http_add_company(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//添加公司
void http_modify_company_info(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//添加部门
void http_add_department(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//修改部门信息
void http_modify_department_info(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//删除部门
void http_del_department(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//添加用户
void http_add_user(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//修改用户信息
void http_modify_user_info(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//为用户添加职位
void http_add_user_title(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//为用户修改职位名称
void http_modify_user_title(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//为用户删除职位
void http_del_user_title(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//删除用户
void http_del_user(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//移动用户
void http_mov_node(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

void http_msg_statistics_rsp(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

void http_login_statistics_rsp(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//新建用户角色
void http_add_role(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//修改角色属性
void http_modify_role_attribute(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//获取公司内所有用户角色列表
void http_query_role_list(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//获取公司内所有用户角色列表
void http_query_role_authority(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//添加用户角色权限
void http_add_role_authority(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//修改用户角色权限
void http_modify_role_authority(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//删除用户角色权限
void http_del_role_authority(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//删除用户角色
void http_del_role(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//查询拥有该角色的用户列表
void http_query_role_user_list(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//为用户添加角色
void http_add_user_role(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//查询用户拥有的角色
void http_query_user_role(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

//删除用户拥有的角色
void http_del_user_role(evutil_socket_t fd, string msgid, Value &body, MYSQL *mysql, cache_conn *redis);

void http_response(evutil_socket_t &fd, string &rsp);


#endif
