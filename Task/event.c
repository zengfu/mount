#include "stm32l0xx_hal.h"
#include "cmsis_os.h"
#include "event.h"
#include "board.h"
#include "lis3dx.h"
#include "lte.h"

extern osMessageQId EventQHandle;
extern osThreadId lteHandle;
extern COMP_HandleTypeDef hcomp1;
extern COMP_HandleTypeDef hcomp2;

extern osMutexId EventLockHandle;

static void PowerEventHandle(EventTypeDef *event);
static void AccelEventHandle(EventTypeDef *event);
void LteEventHandle(EventTypeDef *event);
void MicroWaveEventHandle(EventTypeDef *event);
void PirEventHandle(EventTypeDef *event);


uint16_t GlobalEvent=0x8000;
void EventTask()
{
  EventTypeDef event;
  osDelay(1000);
  Lis3dxInit();
  HAL_COMP_Start(&hcomp2);
  HAL_COMP_Start(&hcomp1);
  PirLevelSet(1);
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
      MicroWaveEventHandle(&event);
    }
    else if(event.evt==EvtLte)
    {
      LteEventHandle(&event);
    }
    else if(event.evt==EvtPir1)
    {
      PirEventHandle(&event);
    }
    else if(event.evt==EvtPir2)
    {
      PirEventHandle(&event);
    }
    //TODO:
  }
}
void MicroWaveEventHandle(EventTypeDef *event)
{
  static uint32_t PressTime,RealeaseTime;
  if(event->io==GPIO_PIN_SET)
  {
    PressTime=event->tick;
    LedSet(0,1);
    //PirLevelSet(cnt%4);
    //cnt++;
  }
  else
  {
    RealeaseTime=event->tick;
    LedSet(0,0);
    //printf("wm:%d\r\n",(RealeaseTime-PressTime));
  }
}
void PirEventHandle(EventTypeDef *event)
{
  LedSet(1,1);
  osDelay(300);
  LedSet(1,0);
}
void LteEventHandle(EventTypeDef *event)
{
  //check the LteTask status;
  
  if(osThreadIsSuspended(lteHandle)==osOK)
  {
    LedTog(2);
    uint8_t state=0;
    //HAL_NVIC_DisableIRQ(EXTI4_15_IRQn);
    state=CheckFrame();
    //osDelay(5000);
    //HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
    if((state&LTE_WAKEUP))
    {
      //wake up thing
      LedSet(2,1);
#ifdef S2L_DEBUG
      S2L_LOG("wakeup\r\n");
#else
      printf("wakeup\r\n");
#endif
      osDelay(100);
      LedSet(2,0);
      //todo:
    }
  }
}
static void AccelEventHandle(EventTypeDef *event)
{
  static uint32_t PressTime,RealeaseTime;
  if(event->io==GPIO_PIN_SET)
  {
    PressTime=event->tick;
    //LedTog(1);
    //PirLevelSet(cnt%4);
    //cnt++;
    osMutexWait(EventLockHandle,osWaitForever);
  //
    GlobalEvent|=0x0001;
    osMutexRelease(EventLockHandle);
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
    
    cnt++;
  }
  else
  {
    RealeaseTime=event->tick;
  }
  //TODO:
}

void HAL_COMP_TriggerCallback(COMP_HandleTypeDef *hcomp)
{
  if(hcomp->Instance==COMP1)
  {
    EventTypeDef tmpevt;
    tmpevt.evt=EvtPir1;
    tmpevt.tick=HAL_GetTick();
    tmpevt.io=(GPIO_PinState)HAL_COMP_GetOutputLevel(hcomp);
    //osMessagePut(dangerHandle,*(uint32_t *)&tmpevt,0);
    xQueueSendFromISR(EventQHandle,&tmpevt,0);
  }
  if(hcomp->Instance==COMP2)
  {
    EventTypeDef tmpevt;
    tmpevt.evt=EvtPir2;
    tmpevt.tick=HAL_GetTick();
    tmpevt.io=(GPIO_PinState)HAL_COMP_GetOutputLevel(hcomp);
    //osMessagePut(dangerHandle,*(uint32_t *)&tmpevt,0);
    xQueueSendFromISR(EventQHandle,&tmpevt,0);
  }
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
    LedTog(1);
    if(!HAL_GPIO_ReadPin(LTE_IT,GPIO_Pin))
    {
      tmpevt.io=GPIO_PIN_RESET;
    //osMessagePut(dangerHandle,*(uint32_t *)&tmpevt,0);
      xQueueSendFromISR(EventQHandle,&tmpevt,0);
    }
  }
  if(GPIO_Pin==GPIO_PIN_9)
  {
    EventTypeDef tmpevt;
    tmpevt.evt=EvtMicroWave;
    tmpevt.tick=HAL_GetTick();
    tmpevt.io=HAL_GPIO_ReadPin(MW_IT,GPIO_Pin);
    //osMessagePut(dangerHandle,*(uint32_t *)&tmpevt,0);
    xQueueSendFromISR(EventQHandle,&tmpevt,0);
  }
}
