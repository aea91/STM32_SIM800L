#ifndef __Sim800L_h
#define __Sim800L_h

#include "stm32f10x.h"

/* uC-SIM buffer */
#define MIN_BUFFER 255
#define MAX_SMS 35

/* define SIM command */
#define IDX_CMD_MAX             6
#define IDX_CMD_TEXT_MODE       1
#define IDX_CMD_READ_SMS        2
#define IDX_CMD_DELE_SMS        3
#define IDX_CMD_SEND_SMS        4
#define IDX_CMD_CNMI_MODE       5
#define IDX_CMD_RESPOND_OK      6

#define LEN_PHONE_NUM           12
#define LEN_CMD_TEXT_MODE       9                     //length of text mode command
#define LEN_CMD_READ_SMS        10                      //length of read sms command
#define LEN_CMD_DELE_SMS        10                      //length of delete sms command
#define LEN_CMD_SEND_SMS        10
#define LEN_CMD_CNMI_MODE       17                    //length of CNMI command
#define LEN_CMD_RESPOND_OK      6                    //length of respond from module SIM
#define LEN_PUBLISH_MES         60
/* define message code */
#define SMS_READ 0
#define SMS_UNREAD 1

/* define return message code */
#define SIM_RES_ERROR         0x1
#define SIM_RES_OK            0x2
#define NO_SMS                0x4
#define IDX_OOR               0x8
#define LEN_OOR               0x10

/* manage contact */
struct PHONEBOOK {
  char number[LEN_PHONE_NUM];                //contact number including send sms command
  bool subscribed;                       //this contact subscribed to receive message
  bool published;                       //published warning/activating code to this contact
};

uint8_t sim_read_sms(uint8_t, char*);
uint8_t sim_dele_sms(uint8_t, char*);
uint8_t sim_send_sms(char*, char*, char*);
uint8_t sim_set_text_mode(uint8_t,char*);
uint8_t sim_set_cnmi_mode(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, char*);
uint8_t sim_check_res(char*);
uint8_t sim_get_sms_contact(char*, char*);
uint8_t sim_get_sms_data(char*, char*);
uint8_t sim_get_sms_state(char*);

void push_cmd(char*, uint8_t);
int find_c(char*, uint8_t, uint8_t, char);    //return index of char in buffer

#endif 