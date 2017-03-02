#ifndef SOCKET_H
#define SOCKET_H

#include "stm32l0xx_hal.h"

typedef struct
{
  int16_t size;
  int16_t cmd;
}ScmCommHead;


#define SCM360_LOGIN_REQ 0X0010
#define SCM360_LOGIN_ACK 0x0011
#define SCM360_HEARBEAT_REQ 0x0012
#define SCM360_HEARBEAT_ACK 0x0013
#define SCM360_NOTIFY_AWAKEN 0x0014
#define SCM360_NOTIFY_AWAKEN_ACK 0x0015




void test();
uint8_t SCMLogin(uint8_t* device_id,uint8_t* user_id,uint8_t* token);
uint8_t* b64_encode (uint8_t *in);
uint8_t* b64_decode (uint8_t *in);
uint8_t LoginAck();
uint8_t SendHeart();
uint8_t HeartAck();

#endif