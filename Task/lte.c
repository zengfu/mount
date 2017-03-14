#include "lte.h"
#include "string.h"
#include "stm32l0xx_hal.h"
#include <stdlib.h>
#include "cJSON.h"
#include "base64.h"
#include "socket.h"
#include "event.h"

extern uint8_t* LoginHead;

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
#ifdef S2L_DEBUG
    S2L_LOG(frame.data);
#else
    printf("%s\n",frame.data);
#endif
    vPortFree(frame.data);
    return 1;
  }
  vPortFree(frame.data);
  return 0;
}
uint8_t const token[]={0x0A,0x74,0xFE,0x3E,0x63,0x26,0xE5,\
  0x56,0xA8,0x92,0x4D,0xA6,0xA1,0xBE,0x62,0x41,\
    0x9C,0xE4,0x7B,0x91,0x68,0xB6,0xDC,0x3C,0x80,0x4F,\
      0x39,0xBC,0x0F,0xBC,0x73,0x16,0x13,0x74,0xAF,0x8B,0x63,\
        0xD0,0xC1,0x0B,0x72,0xD2,0x92,0x4B,0x0F,\
          0x41,0xF8,0x76,0xE7,0xF8,0x7C,0xD9,0x2E,0x1E,0x2C,\
            0x9D,0xA7,0x3C,0x20,0xE7,0x50,0x11,0xD8,0xAB,0x4F,\
              0x14,0xA0,0x3A,0x38,0xF8,0xFF,0x5E,0xC2,0xFC,0x55,0xFF,0x59,\
                0x22,0x6C,0xD3,0x4F,0x14,0x24,0xF3,0xEF,0x2F,0xFC,0x9B,\
                  0xE9,0x6E,0x60,0x89,0x64,0xCE,0xCE,0x8D,0x2B};
uint8_t SCMLoginDirTest()
{ 
  uint16_t length;
  uint8_t err;
  FrameTypeDef frame;
  
  length=0x65;
  frame.cmd=SCM360_LOGIN_REQ;
  frame.size=length;//length-4+4
  frame.data=LoginHead+2;
  
  uint8_t tmp[4];
  tmp[0]=((uint8_t*)&frame)[1];
  tmp[1]=((uint8_t*)&frame)[0];
  tmp[2]=((uint8_t*)&frame)[3];
  tmp[3]=((uint8_t*)&frame)[2];
  if((err=SocketWriteBin(tmp,4)!=0))
  {
     return 1;
  }
  if((err=SocketWriteBin((uint8_t*)token,length-4)!=0))
  {
     return 1;
  }
  return 0;
  
}
uint8_t SCMLoginDir()
{
  if(LoginHead==NULL)
    return 1;
  
  uint16_t length;
  uint8_t err;
  FrameTypeDef frame;
  
  length=LoginHead[0]<<8|LoginHead[1];
  frame.cmd=SCM360_LOGIN_REQ;
  frame.size=length;//length-4+4
  frame.data=LoginHead+2;
  
  uint8_t tmp[4];
  tmp[0]=((uint8_t*)&frame)[1];
  tmp[1]=((uint8_t*)&frame)[0];
  tmp[2]=((uint8_t*)&frame)[3];
  tmp[3]=((uint8_t*)&frame)[2];
  if((err=SocketWriteBin(tmp,4)!=0))
  {
     return 1;
  }
  if((err=SocketWriteBin(frame.data,length-4)!=0))
  {
     return 1;
  }
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
#ifdef S2L_DEBUG
  S2L_LOG(out);
#else
  printf("%s\n",out);
#endif 
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
#ifdef S2L_DEBUG
    S2L_LOG(frame.data);
#else
    printf("%s\n",frame.data);
#endif
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


uint8_t ReadFrame(FrameTypeDef* frame,uint8_t *num)
{
  uint8_t *rdata;
  uint16_t read_len;
  rdata=SocketRead();
  if(rdata==NULL)
  {
    //printf("error1");
    return 1;
  }
  else
    read_len=strlen(rdata);
  if(read_len==0)
  {
    *num=0;
    return 1;
  }
  uint16_t cnt_len=0;
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
      return 1;
//       printf("error");
//       while(1);
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
#ifdef S2L_DEBUG
  S2L_LOG("HeartAck\r\n");
#else
  printf("HeartAck\r\n");
#endif
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
#ifdef S2L_DEBUG
  S2L_LOG(json);
#else
  printf("%s\n",json);
#endif
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
#ifdef S2L_DEBUG
    S2L_LOG(err);
#else
    printf("error:%s\n",err);
#endif
    cJSON_Delete(root);
    return 1;
  }
  
}

uint8_t CheckFrame()
{
  uint8_t num,status;
  status=0;
  memset(GlobalFrame,0,sizeof(GlobalFrame));
  if(ReadFrame(GlobalFrame,&num))
    return 0;
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
      return 0;
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
  //HAL_UART_Receive_DMA(&LTE_UART,lte.rx_buf,BUF_SIZE);//open receive hardware
  while(1)
  {
    //wait the module init
  at:   
    LedSet(0,0); 
    while(lte.init==0)
    {
      if(CheckAT())
      {
        osDelay(1000);
        LedTog(0);
      }
      lte.init=1;
      
    }
    
    //check the card
    if(CheckCard()==0)
    {
      lte.card=1;
    }
    else
    {
      lte.card=0;
    }
    //close the socket
    SocketClose();
    osDelay(1000);
    //init the socekt    
    if(SocketInit()==0)
    {
      lte.socket=1;  
    }
    else
    {
      lte.socket=0;
      goto at;
    }
    osDelay(1000);
    //connect to server
    if(SocketOpen()==0)
    {            
      lte.conn=1;        
    }
    else
    {
      lte.conn=0;
      goto at;
    }
    
    //send token
    uint8_t LteStatus=0;
    while(lte.login==0)
    {
      //if(SCMLogin("dcamera_1","fe19518498fe40aba34c63658b8e9eac","2213ffdaer1123"))
      if(SCMLoginDirTest())
      {
        osDelay(1000);
        goto at;
      }
      login_cnt=0;
      //LteStatus=CheckFrame();
      while((!(CheckFrame() & LTE_LOGIN))&&(login_cnt<10))//read 5 times
      {
#ifdef S2L_DEBUG
        S2L_LOG("login_re\r\n");
#else
        printf("login_re:%d\r\n",login_cnt);
#endif
        //SCMLogin("test1","test2","2213ffdaer1123");
        login_cnt++;
        osDelay(1000);
      }
      /*
        failed
        fresh socket,connect
      */
      if(login_cnt==10)
      {
        lte.login=0;
        goto at;
      }
      else
      {
        lte.login=1;
      }
    }
    while(lte.login)
    {
      LedSet(0,1);
      if( SendHeart())
      {
        osDelay(1000);
        lte.login=0;
        goto at;
      }
      heart_cnt=0;
      do
      {
        LteStatus=CheckFrame();
        if(LteStatus & LTE_WAKEUP)
        {
#ifdef S2L_DEBUG
          S2L_LOG("wakeup_in_lte\r\n");
#else
          printf("wakeup_in_lte\r\n");
#endif
        }
        if(LteStatus & LTE_HEART)
        {
          break;
        }
#ifdef S2L_DEBUG
        S2L_LOG("heart_re\r\n");
#else 
        printf("heart_re:%d\r\n",heart_cnt);
#endif
        heart_cnt++;
      }
      while(heart_cnt<5);
      /*
        failed
        fresh socket,connect,login
      */
      if(heart_cnt==5)
      {
        lte.login=0;
        goto at;
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