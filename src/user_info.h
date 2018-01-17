#ifndef _USER_INFO_H_
#define _USER_INFO_H_

#include <stdint.h>

struct user_info
{
	uint32_t company_id;
	uint32_t user_id;
	char account[3 * 100];
	char pwd[3 * 64];
	char name[3 * 100];
	char english_name[3 * 100];
	char nick_name[3 * 100];
	char work_id[3 * 16];
	char birthday[11];
	uint8_t sex;
	char email[3 * 100];
	char mobile[3 * 25];
	char tel[3 * 32];
	char qq[3 * 20];
	char weixin[3 * 20];
	char address[3 * 100];
	char head_img[3 * 255];
	char head_update_time[20];
	uint32_t heigh;
	uint32_t weigth;
	char nationality[3 * 20];								//民族	
	char native_place[3 * 20];							//籍贯
	uint8_t marital_status;								//婚否0 1未婚 2已婚 3离婚 4丧偶
	uint8_t education;										//学历0 1初中 2中专/高中 3大专 4本科 5硕士 6博士
	char specialities[3 * 20];							//专业
	char graduation_time[11];
	uint8_t auth_admin;
	uint8_t auth_to;
	char entry_data[11];									//入职日期
	char leave_date[11];									//离职日期
	uint8_t status;											//0实习 1试用期 2正式 3离职
	//uint32_t admin_user_id;
	//char update_time[20];
};

#endif