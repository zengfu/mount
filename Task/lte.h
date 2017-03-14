#ifndef _LTE_H
#define _LTE_H


#include "bsp.h"


void LteTask();


#define BUF_SIZE 300

#define SCM360_LOGIN_REQ 0X0010
#define SCM360_LOGIN_ACK 0x0011
#define SCM360_HEARBEAT_REQ 0x0012
#define SCM360_HEARBEAT_ACK 0x0013
#define SCM360_NOTIFY_AWAKEN 0x0014
#define SCM360_NOTIFY_AWAKEN_ACK 0x0015


#define AWAKE_STATE_DOING 1
#define AWAKE_STATE_SUCCESS 2
#define AWAKE_STATE_FAIL 3

#define RES_STATE_OK 0


#define LTE_ERROR 0x00
#define LTE_LOGIN 0X01
#define LTE_HEART 0X02
#define LTE_WAKEUP 0X04



typedef struct
{
  uint8_t init;
  uint8_t card;
  uint8_t socket;
  uint8_t conn;
  uint8_t login;
  uint8_t rx_buf[BUF_SIZE];
  uint16_t length;
}lte_status_s;


typedef struct
{
  int16_t size;
  int16_t cmd;
  uint8_t* data;
}FrameTypeDef;

uint8_t CheckFrame();



#endif