#ifndef _LTE_H
#define _LTE_H


#include "bsp.h"


void LteTask();
uint8_t SocketWriteBin(uint8_t *data,uint8_t data_len);
uint8_t SocketWrite(uint8_t *data);
uint8_t* SocketRead();

#define BUF_SIZE 200

#define SCM360_LOGIN_REQ 0X0010
#define SCM360_LOGIN_ACK 0x0011
#define SCM360_HEARBEAT_REQ 0x0012
#define SCM360_HEARBEAT_ACK 0x0013
#define SCM360_NOTIFY_AWAKEN 0x0014
#define SCM360_NOTIFY_AWAKEN_ACK 0x0015

typedef struct
{
  uint8_t init;
  uint8_t card;
  uint8_t socket;
  uint8_t conn;
  uint8_t login;
  uint8_t rx_buf[BUF_SIZE];
}lte_status_s;


typedef struct
{
  int16_t size;
  int16_t cmd;
}ScmCommHead;




#endif