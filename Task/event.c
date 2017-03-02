#include "stm32l0xx_hal.h"
#include "cmsis_os.h"
#include "event.h"
#include "board.h"
#include "lis3dx.h"


extern osMessageQId EventQHandle;

static void PowerEventHandle(EventTypeDef *event);
static void AccelEventHandle(EventTypeDef *event);



void EventTask()
{
  EventTypeDef event;
  osDelay(1000);
  Lis3dxInit();
  
  
  while(1)
  {
    xQueueReceive(EventQHandle,&event,portMAX_DELAY);
    if(event.evt==EvtPower)
    {
      PowerEventHandle(&event);
    }
    else if(event.evt==EvtAccel1)
    {
      AccelEventHandle(&event);
    }
    else if(event.evt==EvtMicroWave)
    {
    }
    else if(event.evt==EvtLte)
    {
      
    }
    else if(event.evt==EvtPir1)
    {
    }
    else if(event.evt==EvtPir2)
    {
    }
    //TODO:
  }
}
static void AccelEventHandle(EventTypeDef *event)
{
  static uint32_t PressTime,RealeaseTime;
  if(event->io==GPIO_PIN_SET)
  {
    PressTime=event->tick;
    LedTog(1);
    //PirLevelSet(cnt%4);
    //cnt++;
  }
  else
  {
    RealeaseTime=event->tick;
  }
}


static void PowerEventHandle(EventTypeDef *event)
{
  static uint8_t cnt=0;
  static uint32_t PressTime,RealeaseTime;
  if(event->io==GPIO_PIN_RESET)
  {
    PressTime=event->tick;
    LedTog(0);
    PirLevelSet(cnt%4);
    cnt++;
  }
  else
  {
    RealeaseTime=event->tick;
  }
  //TODO:
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if(GPIO_Pin==GPIO_PIN_13)
  {
    EventTypeDef tmpevt;
    tmpevt.evt=EvtPower;
    tmpevt.tick=HAL_GetTick();
    tmpevt.io=HAL_GPIO_ReadPin(POWER_PORT,GPIO_Pin);
    //osMessagePut(dangerHandle,*(uint32_t *)&tmpevt,0);
    xQueueSendFromISR(EventQHandle,&tmpevt,0);
  }
  if(GPIO_Pin==GPIO_PIN_1)
  {
    EventTypeDef tmpevt;
    tmpevt.evt=EvtAccel1;
    tmpevt.tick=HAL_GetTick();
    tmpevt.io=HAL_GPIO_ReadPin(AC_IT1,GPIO_Pin);
    //osMessagePut(dangerHandle,*(uint32_t *)&tmpevt,0);
    xQueueSendFromISR(EventQHandle,&tmpevt,0);
  }
  if(GPIO_Pin==GPIO_PIN_5)
  {
    EventTypeDef tmpevt;
    tmpevt.evt=EvtLte;
    tmpevt.tick=HAL_GetTick();
    tmpevt.io=HAL_GPIO_ReadPin(LTE_IT,GPIO_Pin);
    //osMessagePut(dangerHandle,*(uint32_t *)&tmpevt,0);
    xQueueSendFromISR(EventQHandle,&tmpevt,0);
  }
}
