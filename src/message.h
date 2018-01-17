#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <stdint.h>
#include "../protobuf/IM.BaseDefine.pb.h"

#define HEADER_LEN 27							//协议头长度   
const uint8_t S_TAG_7E = 0x7E;
const uint8_t S_TAG_AA = 0xAA;
const uint16_t S_TAG_VALUE = 0x7EAA;	//服务端协议头标识
const uint16_t C_TAG_VALUE = 0x6623;	//客户端协议头标识
const uint8_t  VERSION = 0x01;					//当前的协议版本

struct msg_head
{
	uint16_t s_tag;				//服务端头标识
	uint32_t s_node;			//服务器节点ID，专门指定connect服务器的
	uint32_t s_socket;			//socket_id
	uint8_t s_router;			//是否为路由消息
	uint16_t c_tag;				//客户端头标识
	uint8_t version;				//版本号
	uint16_t seq_num;		//序列号，用来标识用户的某个请求包
	uint16_t service_id;		//服务类型，大类型
	uint16_t cmd_id;			//子命令类型，小类型
	uint32_t company_id;	//企业ID
	uint16_t pb_length;		//pb长度
	uint8_t check_sum;		//校验和
};

class message
{
public:
	message();
	~message();

public:
	bool read_head(struct evbuffer *input);
	void read_msg(struct evbuffer *input);
	uint32_t get_s_node();
	uint32_t get_s_socket();
	uint8_t get_s_router();
	uint16_t get_seq_num();
	uint16_t get_service_id();
	uint16_t get_cmd_id();
	uint32_t get_company_id();
	uint16_t get_pb_length();
	uint8_t get_check_sum();
	char *get_pb_data();

public:
	void set_s_node(uint32_t s_node);
	void set_s_socket(uint32_t s_socket);
	void set_s_router(uint8_t s_router);
	void set_seq_num(uint8_t seq_num);
	void set_service_id(uint16_t service_id);
	void set_cmd_id(uint16_t cmd_id);
	void set_company_id(uint32_t company_id);
	void set_pb_length(uint16_t pb_length);
	void write_msg(const google::protobuf::MessageLite *pb_data);
	char *get_data();

private:
	bool handle_head(struct evbuffer *input);

private:
	char *m_msg;
	struct msg_head m_msg_head;
};

#endif