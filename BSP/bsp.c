#include "bsp.h"
#include "lis3dx.h"

static void LteInit();

void BspInit()
{
  LteInit();
  //on
  HAL_GPIO_WritePin(ON_OFF_PORT,ON_OFF,GPIO_PIN_SET);
  HAL_GPIO_WritePin(IR_EN2_PORT,IR_EN2,GPIO_PIN_SET);
}
void LedTog(uint8_t num)
{
  switch(num)
  {
  case 0:
    {
      HAL_GPIO_TogglePin(LED1_PORT,LED1);
      break;
    }
  case 1:
    {
      HAL_GPIO_TogglePin(LED2_PORT,LED2);
      break;
    }
  case 2:
    {
      HAL_GPIO_TogglePin(LED3_PORT,LED3);
      break;
    }
  default:
    break;
    
  }
}
void LedSet(uint8_t num,uint8_t state)
{
  switch(num)
  {
  case 0:
    {
      HAL_GPIO_WritePin(LED1_PORT,LED1,(GPIO_PinState)state);
      break;
    }
  case 1:
    {
      HAL_GPIO_WritePin(LED2_PORT,LED2,(GPIO_PinState)state);
      break;
    }
  case 2:
    {
      HAL_GPIO_WritePin(LED3_PORT,LED3,(GPIO_PinState)state);
      break;
    }
  default:
    break;
    
  }
}
/***************
x:0,1,2,3
0:2.13
1:1.96
2:1.96
3:1.88

***************/
void PirLevelSet(uint8_t x)
{
  HAL_GPIO_WritePin(PIR_S1_PORT,PIR_S1,(GPIO_PinState)(x&0x01));
  HAL_GPIO_WritePin(PIR_S2_PORT,PIR_S2,(GPIO_PinState)(x&0x02));
}
//void LteStop()
//{
//  HAL_GPIO_WritePin(LTE_CLOSE_PORT,LTE_CLOSE,GPIO_PIN_SET);//OPEN
//}
static void LteInit()
{
  HAL_GPIO_WritePin(LTE_CLOSE_PORT,LTE_CLOSE,GPIO_PIN_RESET);//close
  HAL_GPIO_WritePin(LTE_WAKEUP_PORT,LTE_WAKEUP_PIN,GPIO_PIN_RESET);//AUTO SLEEP
  HAL_GPIO_WritePin(LTE_RESET_PORT,LTE_RESET,GPIO_PIN_SET);//NO RESET
}
void LteOpen()
{
  HAL_GPIO_WritePin(LTE_CLOSE_PORT,LTE_CLOSE,GPIO_PIN_RESET);
}
extern UART_HandleTypeDef huart1;
int fputc(int ch, FILE *f)
{

 

  uint8_t a;

  a=(uint8_t)ch;

  HAL_UART_Transmit(&huart1, &a, 1, 100);

  return ch;

}