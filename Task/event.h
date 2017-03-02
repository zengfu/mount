#ifndef _EVENT_H
#define _EVENT_H


#include "bsp.h"
#include "stm32l0xx_hal.h"




typedef enum
{
  EvtPower=0U,
  EvtAccel1,
  EvtAceel2,
  EvtMicroWave,
  EvtLte,
  EvtPir1,
  EvtPir2
}EventEnum;

typedef struct
{
  EventEnum evt;
  uint32_t tick;
  GPIO_PinState io;
}EventTypeDef;

void EventTask();

#endif