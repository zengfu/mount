#ifndef SOCKET_H
#define SOCKET_H

#include "stm32l0xx_hal.h"

typedef struct
{
  int16_t size;
  int16_t cmd;
}ScmCommHead;




uint8_t* SocketRead();
uint8_t SocketWrite(uint8_t *data);
uint8_t SocketWriteBin(uint8_t *data,uint8_t data_len);
uint8_t SocketOpen();
uint8_t SocketClose();
uint8_t SocketInit();

uint8_t* read();
uint8_t CheckAT();
uint8_t CheckCard();


#endif