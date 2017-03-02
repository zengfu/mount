#include "cmsis_os.h"
#include "socket.h"


static uint8_t* read()
{
  uint8_t length;
  length=hdma_lpuart1_rx.Instance->CNDTR;
  if(length==0)//overlimit 200
  {
    return NULL;
  }
  else
  {
    lte.rx_buf[BUF_SIZE-length]='\0';
    HAL_UART_DMAStop(&LTE_UART);
    HAL_UART_Receive_DMA(&LTE_UART,lte.rx_buf,BUF_SIZE);
    return lte.rx_buf;
  }
}