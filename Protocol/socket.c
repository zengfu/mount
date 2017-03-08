#include "cmsis_os.h"
#include "socket.h"
#include "bsp.h"
#include "lte.h"
#include "string.h"

#define LTE_UART hlpuart1


extern lte_status_s lte;
extern UART_HandleTypeDef hlpuart1;
extern DMA_HandleTypeDef hdma_lpuart1_rx;

static uint8_t* read();

const char* ltecmd[]=
{
  "AT\r\n",
  "AT%STATUS=\"USIM\"\r\n",
  "AT%MEAS=\"0\"\r\n",
  "AT^SISC=0\r\n",//socket close
  "AT^SISS=0,srvType,STREAM\r\n",//tcp
  "AT^SISS=0,address,tscastle.cam2cloud.com\r\n",//ip address
  "AT^SISS=0,port,36003\r\n",//ip port
  //"AT^SISS=0,address,www.bananalife.top\r\n",//ip address
  //"AT^SISS=0,port,7014\r\n",//ip port
  "AT^SICI=0\r\n",//connect init
  "AT^SISO=0\r\n",//socket open  //NOTE:the cmd need refresh
  "AT^SISHR=0\r\n",
    
};

static uint8_t check(uint8_t id,uint8_t*data)
{
  if(data==NULL)
  {
    return 2;
  }
  else
  {
    uint8_t len=strlen(data);
    if(len==0)
      return 3;
    switch(id)
    {
    case 0:
      {
        for(int i=0;i<len;i++)
        {
          if(data[i]=='O'&&data[i+1]=='K')
          {
            return 0;
          }
        }
        return 1;
      }
    case 1:
      {
        for(int i=0;i<len;i++)
        {
          if(data[i]=='R'&&data[i+1]=='E'&&data[i+2]=='A'&&data[i+3]=='L')
          {
            return 0;
          }
        }
        return 1;
      }
    case 2:
      {
        for(int i=0;i<len;i++)
        {
          if(data[i]=='S'&&data[i+1]=='I'&&data[i+2]=='S'&&data[i+3]=='H'&&data[i+3]=='R'&&data[i+4]==':')
          {
            return 0;
          }
        }
        return 1;
      }
    }
   
  }
  return 1;
}
uint8_t CheckCard()
{
  uint8_t *rx;
  uint8_t *tmp="AT%STATUS=\"USIM\"\r\n";
      HAL_UART_Transmit(&LTE_UART,tmp,strlen(tmp),200);
      osDelay(300);
      rx=read();
      if(check(1,rx)==0)
      {
        lte.card=1;
        printf("%s\n",rx);
        return 0;
      }
      lte.card=0;
      return 1;
}
uint8_t CheckAT()
{
  uint8_t *rx;
      uint8_t *tmp="AT\r\n";
      
      HAL_UART_Transmit(&LTE_UART,tmp,strlen(tmp),200);
      //
      osDelay(300);
      rx=read();
      if(check(0,rx)==0)
      {
        lte.init=1;
        printf("%s\n",rx);
        return 0;
      }
      lte.init=0;
      return 1;
}
static uint8_t* read()
{
  uint8_t length;
  length=hdma_lpuart1_rx.Instance->CNDTR;
  if(length==0)//overlimit 200
  {
    //printf("***********\r\n");
    return NULL;
  }
  else
  {
    lte.rx_buf[BUF_SIZE-length]='\0';
    lte.length=BUF_SIZE-length;
    HAL_UART_DMAStop(&LTE_UART);
    HAL_UART_Receive_DMA(&LTE_UART,lte.rx_buf,BUF_SIZE);
    return lte.rx_buf;
  }
}

uint8_t* SocketRead()
{
  uint8_t* rx;
  uint8_t* data;
  
  uint8_t len;
  uint8_t index;
  
  data=NULL;
  memset(lte.rx_buf,0,BUF_SIZE);
  HAL_UART_Transmit(&LTE_UART,(uint8_t*)ltecmd[9],strlen(ltecmd[9]),200);
  osDelay(300);
  rx=read();
  len=strlen(rx);
  for(index=0;index<len;index++)
  {
    if(rx[index]==':') 
      data=rx+index+1;
    if(rx[index]=='\r'&&rx[index+1]=='\n'&&rx[index+2]=='\r'&&rx[index+3]=='\n')
    {
      rx[index]='\0';
    }
  }
  
  return data;
}


uint8_t SocketWrite(uint8_t *data)
{
  uint8_t* all;
  uint32_t len;
  uint8_t* rx;
  uint8_t* cmd="AT^SISW=0,";
  len=strlen(data)+strlen(cmd)+3;//+('\0')+'\r'+'\n'
  all=pvPortMalloc(len);
  if(all==NULL)
  {
    while(1);
  }
  strcpy(all,cmd);
  strcat(all,data);
  all[len-1]='\0';
  all[len-2]='\n';
  all[len-3]='\r';
  //printf("%s\n",all);
  HAL_UART_Transmit(&LTE_UART,all,strlen(all),200);
  osDelay(500);
  vPortFree(all);
  rx=read();
  if(check(0,rx)==0)
  {
    printf("write_sucess:%s\n",rx);
  }
  else
    return 1;
  return 0;
}
static void Num2Str(uint8_t num,uint8_t* str)
{
  uint8_t tmp[3]={0};
  sprintf(tmp,"%x",num);
  if(tmp[1]=='\0')
  {
    tmp[1]=tmp[0];
    tmp[0]=0x30;
    tmp[2]='\0';
  }
  memcpy(str,tmp,2);
}

uint8_t SocketWriteBin(uint8_t *data,uint8_t data_len)
{
  uint8_t* all;
  uint32_t len;
  uint8_t* rx;
  uint8_t* cmd="AT^SISH=%d(0),";
  uint8_t cmd_len;
  cmd_len=strlen(cmd);
  len=data_len*2+cmd_len+3;//+('\0')+'\r'+'\n'
  all=pvPortMalloc(len);
  strcpy(all,cmd);
  for(int i=0;i<data_len;i++)
  {
    uint8_t tmp[2];
    Num2Str(data[i],tmp);
    all[cmd_len+i*2]=tmp[0];
    all[cmd_len+i*2+1]=tmp[1];
  }
  all[len-1]='\0';
  all[len-2]='\n';
  all[len-3]='\r';
  len=strlen(all);
  HAL_UART_Transmit(&LTE_UART,all,strlen(all),200);
  vPortFree(all);
  osDelay(500);
  rx=read();
  if(check(0,rx)==0)
  {
    printf("write_bin sucess:%s\n",rx);
  }
  else
    return 1;
  return 0;
}
uint8_t SocketOpen()
{
  uint8_t* rx;
  uint8_t err;
  HAL_UART_Transmit(&LTE_UART,(uint8_t*)ltecmd[8],strlen(ltecmd[8]),200);
  osDelay(500);
  rx=read();
  if((err=check(0,rx))==0)
  {
    printf("%s\n",rx);
  }
  else
    return err;
  return 0;
}
uint8_t SocketClose()
{
  uint8_t* rx;
  uint8_t err;
  HAL_UART_Transmit(&LTE_UART,(uint8_t*)ltecmd[3],strlen(ltecmd[3]),200);
  osDelay(1000);
  rx=read();
  if((err=check(0,rx))==0)
  {
    printf("%s\n",rx);
  }
  else
    return err;
  return 0;
}
uint8_t SocketInit()
{
  uint8_t* rx;
  //tcp
  uint8_t len;
  len=strlen(ltecmd[4]);
  HAL_UART_Transmit(&LTE_UART,(uint8_t*)ltecmd[4],len,200);
  osDelay(300);
  rx=read();
  if(check(0,rx)==0)
  {
    printf("%s\n",rx);
  }
  else 
    return 1;
  //ip address
  len=strlen(ltecmd[5]);
  HAL_UART_Transmit(&LTE_UART,(uint8_t*)ltecmd[5],len,200);
  osDelay(300);
  rx=read();
  
  if(check(0,rx)==0)
  {
    printf("%s\n",rx);
  }
  else
    return 1;
  //port 
  len=strlen(ltecmd[6]);
  HAL_UART_Transmit(&LTE_UART,(uint8_t*)ltecmd[6],len,200);
  osDelay(300);
  rx=read();
  if(check(0,rx)==0)
  {
    printf("%s\n",rx);
  }
  else
    return 1;
  //connect init
  HAL_UART_Transmit(&LTE_UART,(uint8_t*)ltecmd[7],strlen(ltecmd[7]),200);
  osDelay(300);
  rx=read();
  if(check(0,rx)==0)
  {
    printf("%s\n",rx);
  }
  else
    return 1;
  return 0;
}
