#include "lte.h"
#include "string.h"
#include "stm32l0xx_hal.h"
#include <stdlib.h>
#include "cJSON.h"
#include "base64.h"
#include "socket.h"
#include "event.h"


lte_status_s lte;
FrameTypeDef GlobalFrame[3];//max=3

extern osMessageQId EventQHandle;

static uint8_t SCMWakeup(uint8_t status)
{
  uint8_t err;
  cJSON *root;
  FrameTypeDef frame;
  
  root=cJSON_CreateObject();
  cJSON_AddNumberToObject(root, "res",RES_STATE_OK);
  cJSON_AddNumberToObject(root, "status",(double)status);
  char* out=cJSON_PrintUnformatted(root);
  cJSON_Delete(root);
  frame.data=b64_encode(out);
  vPortFree(out);
  frame.cmd=SCM360_NOTIFY_AWAKEN_ACK;
  frame.size=4+strlen(frame.data);
  
  uint8_t tmp[4];
  tmp[0]=((uint8_t*)&frame)[1];
  tmp[1]=((uint8_t*)&frame)[0];
  tmp[2]=((uint8_t*)&frame)[3];
  tmp[3]=((uint8_t*)&frame)[2];
  if((err=SocketWriteBin(tmp,4)!=0))
  {
     vPortFree(frame.data);
     return 1;
  }
  if((err=SocketWrite(frame.data))!=0)
  {
    printf("%s\n",frame.data);
    vPortFree(frame.data);
    return 1;
  }
  vPortFree(frame.data);
  return 0;
}
static uint8_t SCMLogin(uint8_t* device_id,uint8_t* user_id,uint8_t* token)
{
  //uint32_t length;
  uint8_t err;
  cJSON *root;
  FrameTypeDef frame;
  
  root=cJSON_CreateObject();
  cJSON_AddItemToObject(root, "device_id", cJSON_CreateString(device_id));
  cJSON_AddItemToObject(root, "user_id", cJSON_CreateString(user_id));
  cJSON_AddItemToObject(root, "token", cJSON_CreateString(token));
  char* out=cJSON_PrintUnformatted(root);
  cJSON_Delete(root);
  //char* encode;
  printf("%s\n",out);
  
  //base 64 encode
  frame.data=b64_encode(out);
  vPortFree(out);
  //
  frame.cmd=SCM360_LOGIN_REQ;
  frame.size=4+strlen(frame.data);
  //big-endian
  uint8_t tmp[4];
  tmp[0]=((uint8_t*)&frame)[1];
  tmp[1]=((uint8_t*)&frame)[0];
  tmp[2]=((uint8_t*)&frame)[3];
  tmp[3]=((uint8_t*)&frame)[2];
  if((err=SocketWriteBin(tmp,4)!=0))
  {
    vPortFree(frame.data);
     return 1;
  }
  if((err=SocketWrite(frame.data))!=0)
  {
    printf("%s\n",frame.data);
    vPortFree(frame.data);
    return 1;
  }
  vPortFree(frame.data);
  return 0;
}
static uint8_t S2N(uint8_t* str)
{
  uint8_t tmp[2]={0};
  for(int i=0;i<2;i++)
  {
    //num
    if(str[i]>=0x30&&str[i]<=0x39)
      tmp[i]=str[i]-0x30;
    else
      tmp[i]=str[i]-0x61+10;
  }
  return (uint8_t)(tmp[0]<<4|tmp[1]);
}

uint8_t SendHeart()
{
  FrameTypeDef frame;
  frame.cmd=SCM360_HEARBEAT_REQ;
  frame.size=4;
  frame.data=NULL;
  uint8_t tmp[4];
  tmp[0]=((uint8_t*)&frame)[1];
  tmp[1]=((uint8_t*)&frame)[0];
  tmp[2]=((uint8_t*)&frame)[3];
  tmp[3]=((uint8_t*)&frame)[2];
  if(SocketWriteBin(tmp,4))
     return 1;
  return 0;
}
uint8_t read_len;
uint8_t *rdata;
uint8_t ReadFrame(FrameTypeDef* frame,uint8_t *num)
{
  
  rdata=SocketRead();
  if(rdata==NULL)
  {
    printf("error1");
    return 1;
  }
  else
    read_len=strlen(rdata);
  if(read_len==0)
  {
    *num=0;
    return 1;
  }
  uint8_t cnt_len=0;
  uint8_t buf[4];
  
  for((*num)=0;(*num)<3;(*num)++)
  {
    buf[0]=S2N(rdata+cnt_len);
    buf[1]=S2N(rdata+2+cnt_len);
    //cmdhead[0].size=(int16_t)(len_1<<8|len_2);
    buf[2]=S2N(rdata+4+cnt_len);
    buf[3]=S2N(rdata+6+cnt_len);
    frame[*num].size=(int16_t)(buf[0]<<8|buf[1]);
    frame[*num].cmd=(int16_t)(buf[2]<<8|buf[3]);
    frame[*num].data=rdata+8+cnt_len;
    if(!(frame[*num].cmd>=SCM360_LOGIN_REQ&&frame[*num].cmd<=SCM360_NOTIFY_AWAKEN_ACK))
    {
       printf("error");
       while(1);
    }
    cnt_len+=((frame[*num].size-4)*2+8);
    if(read_len==cnt_len)      
      break;
    
    if(read_len<cnt_len)
    {
      return 1; 
    }
  }
  (*num)++;
  if((*num)==4)
  {
    return 1;
  }
  return 0;
}
  


uint8_t HeartAck(FrameTypeDef* frame)
{
  //uint8_t data*=pvPortMalloc(frame->size-4+1);
  if(frame->size!=4)
    return 1;
  printf("HeartAck\r\n");
  return 0;
}
uint8_t LoginAck(FrameTypeDef* frame)
{
  cJSON* root;
  uint8_t* num;
  int res;
  
  num=pvPortMalloc((frame->size)-4+1);//reduce the packet head add '\0' 
  if(num==NULL)
  {
    return 1;
  }
  for(int i=0;i<((frame->size)-4);i++)
  {
    num[i]=S2N(frame->data+i*2);
  }
  num[frame->size-4]='\0';
  uint8_t* json=b64_decode(num);
  vPortFree(num);
  if(json==NULL)
    return 1;
  printf("%s\n",json);
  root=cJSON_Parse(json);
  vPortFree(json);
  res=cJSON_GetObjectItem(root,"res")->valueint;
  if(res==0)
  {
    cJSON_Delete(root);
    return 0;
  }
  else
  {
    uint8_t* err=cJSON_GetObjectItem(root,"err_msg")->string;
    printf("error:%s\n",err);
    cJSON_Delete(root);
    return 1;
  }
  
}

uint8_t CheckFrame()
{
  uint8_t num,status;
  status=0;
  memset(GlobalFrame,0,sizeof(GlobalFrame[1])*3);
  ReadFrame(GlobalFrame,&num);
  for(int i=0;i<num;i++)
  {
    switch(GlobalFrame[i].cmd)
    {
    case SCM360_LOGIN_ACK:
      {
          if(LoginAck(&GlobalFrame[i]))
            return LTE_ERROR;
          status|=LTE_LOGIN;
          break;
      }
    case SCM360_HEARBEAT_ACK:
      {
          if(HeartAck(&GlobalFrame[i]))
            return 1;
          status|=LTE_HEART;
          break;
      }
    case SCM360_NOTIFY_AWAKEN:
      {
        //todo:
        status|=LTE_WAKEUP;
        break;
      }
    default:
      return 1;
    }
  }
  return status; 
}
#define LTE_UART hlpuart1

extern UART_HandleTypeDef hlpuart1;

void LteTask()
{
  uint8_t login_cnt,heart_cnt;
  memset(&lte,0,sizeof(lte));//reset the space
  HAL_UART_Receive_DMA(&LTE_UART,lte.rx_buf,BUF_SIZE);//open receive hardware
  while(1)
  {
    //wait the module init
    while(lte.init==0)
    {
      CheckAT();
      osDelay(500);
    }
    //check the card
    while(lte.card==0)
    {
      CheckCard();
      osDelay(500);
    }
    //close the socket
    SocketClose();
    //init the socekt
    while(lte.socket==0)
    {
      if(SocketInit()==0)
      {
            lte.socket=1;
          
      }
    }
    //connect to server
    while(lte.conn==0)
    {
 
      if(SocketOpen()==0)
      {            
        lte.conn=1;        
      }
      else
      {
        static uint8_t conn_cnt=0;
        conn_cnt++;
        if(conn_cnt==10)
        {
          conn_cnt=0;
          lte.card=0;
          lte.init=0;
          lte.socket=0;
          break;
        }
      }
    }
    
    //send token
    uint8_t LteStatus=0;
    while(lte.login==0)
    {
      if(SCMLogin("dcamera_1","fe19518498fe40aba34c63658b8e9eac","2213ffdaer1123"))
        break;
      login_cnt=0;
      //LteStatus=CheckFrame();
      while((!(CheckFrame() & LTE_LOGIN))&&(login_cnt<5))//read 5 times
      {
        printf("login_re:%d\r\n",login_cnt);
        //SCMLogin("test1","test2","2213ffdaer1123");
        login_cnt++;
        osDelay(1000);
      }
      /*
        failed
        fresh socket,connect
      */
      if(login_cnt==5)
      {
        lte.socket=0;
        lte.conn=0;
        break;
      }
      else
      {
        lte.login=1;
      }
    }
    while(lte.login)
    {
      
      SendHeart();
      heart_cnt=0;
      do
      {
        LteStatus=CheckFrame();
        if(LteStatus & LTE_WAKEUP)
        {
          printf("wakeup_in_lte\r\n");
        }
        if(LteStatus & LTE_HEART)
        {
          break;
        }
        printf("heart_re:%d\r\n",heart_cnt);
        heart_cnt++;
      }
      while(heart_cnt<5);
      /*
        failed
        fresh socket,connect,login
      */
      if(heart_cnt==5)
      {
        lte.socket=0;
        lte.conn=0;
        lte.login=0;
        break;
      }
      /*ack failed 
      /reinit socket and conn*/
      //srand(HAL_GetTick());
      //uint32_t delay_time=(10000+rand()%50000);//10s-60s
      //printf("%d\n",delay_time);
      //osDelay(2000);//<5min);
      osThreadSuspend(NULL);
    }
    
  }
}
extern osThreadId lteHandle;
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
  osThreadResume(lteHandle);
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if(huart->Instance==LPUART1)
  {
    
  }
}