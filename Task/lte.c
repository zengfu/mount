#include "lte.h"
#include "string.h"
#include "stm32l0xx_hal.h"
#include <stdlib.h>
#include "cJSON.h"

#define LTE_UART hlpuart1




lte_status_s lte;



ScmCommHead scm_head;

uint8_t cmdid=0;


extern UART_HandleTypeDef hlpuart1;
extern DMA_HandleTypeDef hdma_lpuart1_rx;
extern UART_HandleTypeDef huart1;

const char base[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";


static void low_transmit(const char* data);
static void lte_handle(uint8_t num);

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
static void *(*b64_malloc)(size_t sz) = pvPortMalloc;
static void (*b64_free)(void *ptr) = vPortFree;
static uint8_t GetIndex(uint8_t num)
{
  uint8_t len;
  len=strlen(base);
  for(int i=0;i<len;i++)
  {
    if(base[i]==num)
      return i;
  }
  return 255;
}
uint8_t* b64_decode (uint8_t *in)
{
  uint32_t len=strlen(in);
  uint32_t mlen=0;
  uint32_t size=0;
  if(len%4!=0)
    return NULL;
  uint8_t *out;
  mlen=len/4*3;//maybe have '='
  if(in[len-1]=='=')
    mlen--;
  if(in[len-2]=='=')
    mlen--;
  out = b64_malloc(mlen+1);//add '\0'
  if(out==NULL)
    return NULL;
  
  uint32_t b[4];
  for (int i = 0; i < len; i += 4) 
  {
    b[0] =GetIndex(in[i]);
    b[1] =GetIndex(in[i+1]);
    b[2] =GetIndex(in[i+2]);
    b[3] =GetIndex(in[i+3]);
    out[size++]=(uint8_t)((b[0]<<2)|(b[1]>>4));
    if (b[2] < 64)
    {
      out[size++] = (uint8_t) ((b[1] << 4) | (b[2] >> 2));
                if (b[3] < 64)  
                {
                    out[size++] = (uint8_t) ((b[2] << 6) | b[3]);
                }
     }
  }
  out[size++]='\0';
  return out;
}

uint8_t* b64_encode (uint8_t *in)
{
  uint8_t *out;
  uint32_t len=strlen(in);
  uint32_t mlen;
  // alloc
  if(len%3)
  {
    mlen=(len/3)*4+4;
  }
  else
  {
    mlen=(len/3)*4;
  }
  out = b64_malloc(mlen+1);//add '\0'
  if(out==NULL)
    return NULL;
  int b;
  int size=0;
  for(int i=0;i<len;i+=3)
  {
    b=(in[i]&0xfc)>>2;
    out[size++]=base[b];
    b=(in[i]&0x03)<<4;
    if(i+1<len)//
    {
       b |= (in[i + 1] & 0xF0) >> 4;
       out[size++]=base[b];
       b = (in[i + 1] & 0x0F) << 2;
       if(i+2<len)
       {
          b |= (in[i + 2] & 0xC0) >> 6;
          out[size++]=base[b];
          b = in[i + 2] & 0x3F;
          out[size++]=base[b];
       }
       else
       {
         out[size++]=base[b];
         out[size++]= '=';
       }
    }
    else
    {
      out[size++]=base[b];
      out[size++]= '=';
      out[size++]= '=';
    }
  }
  out[size++]='\0';
  return out;
}
uint8_t SCMLogin(uint8_t* device_id,uint8_t* user_id,uint8_t* token)
{
  //uint32_t length;
  uint8_t err;
  cJSON *root;
  ScmCommHead cmd;
  root=cJSON_CreateObject();
  cJSON_AddItemToObject(root, "device_id", cJSON_CreateString(device_id));
  cJSON_AddItemToObject(root, "user_id", cJSON_CreateString(user_id));
  cJSON_AddItemToObject(root, "token", cJSON_CreateString(token));
  char* out=cJSON_PrintUnformatted(root);
  cJSON_Delete(root);
  char* encode;
  printf("%s\n",out);
  encode=b64_encode(out);
  //printf("%s\n",encode);
  vPortFree(out);
  cmd.cmd=SCM360_LOGIN_REQ;
  cmd.size=4+strlen(encode);
  //length=sizeof(ltecmdHandle)+strlen(out);
  uint8_t tmp[4];
  tmp[0]=((uint8_t*)&cmd)[1];
  tmp[1]=((uint8_t*)&cmd)[0];
  tmp[2]=((uint8_t*)&cmd)[3];
  tmp[3]=((uint8_t*)&cmd)[2];
  if((err=SocketWriteBin(tmp,sizeof(cmd)))!=0)
     return 1;
  if((err=SocketWrite(encode))!=0)
  {
    printf("%s\n",encode);
    
    return 1;
  }
  vPortFree(encode);
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
  ScmCommHead cmd;
  cmd.cmd=SCM360_HEARBEAT_REQ;
  cmd.size=4;
  uint8_t tmp[4];
  tmp[0]=((uint8_t*)&cmd)[1];
  tmp[1]=((uint8_t*)&cmd)[0];
  tmp[2]=((uint8_t*)&cmd)[3];
  tmp[3]=((uint8_t*)&cmd)[2];
  if(SocketWriteBin(tmp,sizeof(cmd)))
     return 1;
  return 0;
}
uint8_t buf[4];
uint8_t read_len;
uint8_t cnt_len=0;
uint8_t* rdata;
//uint8_t LteRead(uint8_t *num,ScmCommHead* cmdhead,uint8_t* addr)
//{
//  
//  cnt_len=0;
//  memset(cmdhead,0,sizeof(cmdhead[0])*4);//max buf 4
//  rdata=SocketRead();
//  addr=NULL;
//  if(rdata==NULL)
//    return 1;
//  addr=rdata;
//  read_len=strlen(rdata);
//  
//  for(*num=0;*num<4;*num++)
//  {
//    buf[0]=S2N(rdata+cnt_len);
//    buf[1]=S2N(rdata+2+cnt_len);
//    //cmdhead[0].size=(int16_t)(len_1<<8|len_2);
//    buf[2]=S2N(rdata+4+cnt_len);
//    buf[3]=S2N(rdata+6+cnt_len);
//    cmdhead[0].size=(int16_t)(buf[0]<<8|buf[1]);
//    cmdhead[0].cmd=(int16_t)(buf[2]<<8|buf[3]);
//    cnt_len+=cmdhead[0].size;
//    if(read_len==cnt_len)
//      break;
//    if(read_len<cnt_len)
//      return 1; 
//  }
//  *num++;
//  if(*num==5)
//    return 1;
//  return 0;
////  len_1=S2N(rdata);
////  len_2=S2N(rdata+2);
////  cmdhead[0].size=(int16_t)(len_1<<8|len_2);
////  len_1=S2N(rdata+4);
////  len_2=S2N(rdata+6);
////  cmdhead[0].cmd=(int16_t)(len_1<<8|len_2);
//  //
//  
//}
//uint8_t LoginAck(ScmCommHead cmd,uint8_t* data)
//{
//  cJSON* root;
//  uint8_t* num;
//  uint8_t res;
////  len_1=S2N(rdata);
////  len_2=S2N(rdata+2);
////  scm_head.size=(int16_t)(len_1<<8|len_2);
////  len_1=S2N(rdata+4);
////  len_2=S2N(rdata+6);
////  scm_head.cmd=(int16_t)(len_1<<8|len_2);
////  if(scm_head.cmd!=SCM360_LOGIN_ACK)
////    return 1;
//// printf("%s\n",rdata);
//  num=pvPortMalloc(cmd.size-4+1);//reduce the packet head add '\0' 
//  if(num==NULL)
//  {
//    return 1;
//  }
//  for(int i=0;i<(cmd.size-4);i++)
//  {
//    num[i]=S2N(data+i*2+8);
//  }
//  num[cmd.size-4]='\0';
//  uint8_t* json=b64_decode(num);
//  vPortFree(num);
//  if(json==NULL)
//    return 1;
//  printf("login_ack:%s\r\n",json);
//  root=cJSON_Parse(json);
//  vPortFree(json);
//  res=cJSON_GetObjectItem(root,"res")->valueint;
//  if(res==0)
//  {
//    cJSON_Delete(root);
//    return 0;
//  }
//  else
//  {
//    uint8_t* err=cJSON_GetObjectItem(root,"err_msg")->string;
//    printf("error:%s\n",err);
//    cJSON_Delete(root);
//    return 1;
//  }
//  
//}
//uint8_t HeartAck(ScmCommHead cmd,uint8_t* data)
//{
//  uint8_t* num;
// 
////  len_1=S2N(rdata);
////  len_2=S2N(rdata+2);
////  scm_head.size=(int16_t)(len_1<<8|len_2);
////  len_1=S2N(rdata+4);
////  len_2=S2N(rdata+6);
////  scm_head.cmd=(int16_t)(len_1<<8|len_2);
////  if(scm_head.cmd!=SCM360_LOGIN_ACK)
////    return 1;
//// printf("%s\n",rdata);
//  num=pvPortMalloc(cmd.size+1);//reduce the packet head add '\0' 
//  if(num==NULL)
//  {
//    return 1;
//  }
//  num[cmd.size]='\0';
//  printf("heart_ack:%s\r\n",num);
//  vPortFree(num);
//}
//max 4

uint8_t HeartAck()
{
  uint8_t* rdata;
  uint8_t  len_1,len_2;
  rdata=SocketRead();//the buffer is static,no need to free
  if(rdata==NULL)
    return 1;
  len_1=S2N(rdata);
  len_2=S2N(rdata+2);
  scm_head.size=(int16_t)(len_1<<8|len_2);
  len_1=S2N(rdata+4);
  len_2=S2N(rdata+6);
  scm_head.cmd=(int16_t)(len_1<<8|len_2);
  if(scm_head.cmd!=SCM360_HEARBEAT_ACK)
    return 1;
  printf("%s\n",rdata);
  return 0;
}
uint8_t LoginAck()
{
  uint8_t* rdata;
  cJSON* root;
  uint8_t* num;
  uint8_t  len_1,len_2;
  int res;
  rdata=SocketRead();
  if(rdata==NULL)
    return 1;
  len_1=S2N(rdata);
  len_2=S2N(rdata+2);
  scm_head.size=(int16_t)(len_1<<8|len_2);
  len_1=S2N(rdata+4);
  len_2=S2N(rdata+6);
  scm_head.cmd=(int16_t)(len_1<<8|len_2);
  if(scm_head.cmd!=SCM360_LOGIN_ACK)
    return 1;
  printf("%s\n",rdata);
  num=pvPortMalloc(scm_head.size-4+1);//reduce the packet head add '\0' 
  if(num==NULL)
  {
    return 1;
  }
  for(int i=0;i<(scm_head.size-4);i++)
  {
    num[i]=S2N(rdata+i*2+8);
  }
  num[scm_head.size-4]='\0';
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
//typedef enum
//{
//  LTE_ERROR=0,
//  LTE_LOGIN_ACK,
//  LTE_HEART_ACK,
//  LTE_WAKEUP_CMD
//}LTEStatus;
//uint8_t LteHandle()
//{
//  
////  uint8_t* rdata;
////  uint8_t  len_1,len_2;
////  rdata=SocketRead();//the buffer is static,no need to free
////  if(rdata==NULL)
////    return 1;
////  len_1=S2N(rdata);
////  len_2=S2N(rdata+2);
////  scm_head.size=(int16_t)(len_1<<8|len_2);
////  len_1=S2N(rdata+4);
////  len_2=S2N(rdata+6);
////  scm_head.cmd=(int16_t)(len_1<<8|len_2);
//  uint8_t num;
//  uint8_t* addr;
//  if(LteRead(&num,scm_head,addr))
//    return 1;
//  uint8_t length=0;
//  for(int i=0;i<num;i++)
//  {
//    switch(scm_head[i].cmd)
//    {
//    case SCM360_LOGIN_ACK:
//      {
//        if(LoginAck(scm_head[i],addr+length))
//          return 1;
//        break;
//      }
//    case SCM360_HEARBEAT_ACK:
//      {
//        if(HeartAck(scm_head[i],addr+length))
//          return 1;
//        break;
//      }
//    default:
//      return 1;
//      
//    }
//    length+=scm_head[i].size;
//  }
//  
//  //printf("%s\n",rdata);
//  return 0;
//}


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
}
static uint8_t SocketInit()
{
  uint8_t* rx;
//  //socket open
  //uint8_t* tmp="AT^SISC=0\r\n";
//  HAL_UART_Transmit_DMA(&LTE_UART,tmp,strlen(tmp));
//  osDelay(100);
//  rx=read();
//  if(check(0,rx)==0)
//  {
//    printf("%s\n",rx);
//  }
//  else 
//    return 1;
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
static uint8_t SocletClose()
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
static uint8_t SocketOpen()
{
  uint8_t* rx;
  uint8_t err;
  HAL_UART_Transmit(&LTE_UART,(uint8_t*)ltecmd[8],strlen(ltecmd[8]),200);
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
uint8_t* SocketRead()
{
  uint8_t* rx;
  uint8_t err;
  uint8_t len;
  uint8_t index;
  memset(lte.rx_buf,BUF_SIZE,0);
  HAL_UART_Transmit(&LTE_UART,(uint8_t*)ltecmd[9],strlen(ltecmd[9]),200);
  osDelay(500);
  rx=read();
  if(rx==NULL)
    return NULL;
  //the buffer is full
  //err=check(2,rx);
  len=strlen(rx);
  for(index=0;index<len;index++)
  {
    if(rx[index]==':') 
      return rx+index+1;
  }
  return NULL;
  
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
    printf("sucess:%s\n",rx);
  }
  else
    return 1;
  return 0;
}
static void Num2Str(uint8_t num,uint8_t* str)
{
  uint8_t tmp[3]=0;
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
  osDelay(500);
  vPortFree(all);
  rx=read();
  if(check(0,rx)==0)
  {
    printf("sucess:%s\n",rx);
  }
  else
    return 1;
  return 0;
}

void LteTask()
{
  uint8_t login_cnt,heart_cnt;
  memset(&lte,sizeof(lte),0);//reset the space
  HAL_UART_Receive_DMA(&LTE_UART,lte.rx_buf,BUF_SIZE);//open receive hardware
  while(1)
  {
    uint8_t *rx;
    //wait the module init
    while(lte.init==0)
    {
      uint8_t *tmp="AT\r\n";
      HAL_UART_Transmit(&LTE_UART,tmp,strlen(tmp),200);
      osDelay(500);
      rx=read();
      if(check(0,rx)==0)
      {
        lte.init=1;
        printf("%s\n",rx);
      }
    }
    //check the card
    while(lte.card==0)
    {
      uint8_t *tmp="AT%STATUS=\"USIM\"\r\n";
      HAL_UART_Transmit(&LTE_UART,tmp,strlen(tmp),200);
      osDelay(300);
      rx=read();
      if(check(1,rx)==0)
      {
        lte.card=1;
        printf("%s\n",rx);
      }
    }
    //close the socket
    SocletClose();
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
    while(lte.login==0)
    {
      if(SCMLogin("test1","test2","2213ffdaer1123"))
        break;
      login_cnt=0;
      while(LoginAck()&&(login_cnt<5))//read 5 times
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
      //SocketWrite("test");
      //osDelay(1000);
//      uint8_t num[3]={0,6,249};
//      SocketWriteBin(num,3);
//      osDelay(1000);
//      SCMLogin("test1","test2","2213ffdaer1123");
//      uint8_t* read;
//      read=SocketRead();
//      if(read!=NULL)
//        printf("%s\n",read);
      
      SendHeart();
      heart_cnt=0;
      while(HeartAck()&&(heart_cnt<5))
      {
        heart_cnt++;
        printf("heart_re:%d\r\n",heart_cnt);
        osDelay(1000);
      }
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
      srand(HAL_GetTick());
      uint32_t delay_time=(10000+rand()%50000);//10s-60s
      printf("%d\n",delay_time);
      osDelay(delay_time);//<5min);
      
    }
    
  }
}
//void lte_task()
//{
//  osEvent cmdevt;
//  //osMessagePut(ltecmdHandle,0,osWaitForever);//test cmd
//  osMessagePut(ltecmdHandle,1,osWaitForever);//usim cmd
//  osMessagePut(ltecmdHandle,2,osWaitForever);//usim cmd
//  HAL_UART_Receive_DMA(&LTE_UART,lte_rx.buf,BUF_SIZE);
//  while(1)
//  {
//     while(hdma_lpuart1_rx.Instance->CNDTR==BUF_SIZE)
//    {
//      low_transmit(ltecmd[lte.id]);
//      osDelay(500);
//    }
//    /*the buffer is changing*/
//    while(lte_rx.len!=(BUF_SIZE-hdma_lpuart1_rx.Instance->CNDTR))
//    {
//      lte_rx.len=BUF_SIZE-hdma_lpuart1_rx.Instance->CNDTR;
//      osDelay(200);
//    }
//    /*the buffer is freeze*/
//    if(lte_rx.len)
//    {
//      
//      lte.state=1;
//      //debug
//      HAL_UART_Transmit(&huart1,lte_rx.buf,lte_rx.len,200);
//      //handle data
//      lte_handle(lte.id);
//      
//      
//      // next 
//      memset(lte_rx.buf,0,lte_rx.len);
//      lte_rx.len=0;
//      HAL_UART_DMAStop(&LTE_UART);
//      HAL_UART_Receive_DMA(&LTE_UART,lte_rx.buf,BUF_SIZE);
//      //get id
//      cmdevt=osMessageGet(ltecmdHandle,osWaitForever);
//      lte.id=(uint8_t)cmdevt.value.v;
//    }
//  }
//}

//static void lte_handle(uint8_t num)
//{
//  switch(num)
//  {
//  case 0:
//    {
//      break;
//    }
//  case 1:
//    {
//      for(int i=0;i<lte_rx.len;i++)
//      {
//        if(lte_rx.buf[i]=='R'&&lte_rx.buf[i+1]=='E'&&lte_rx.buf[i+2]=='A'&&lte_rx.buf[i+3]=='L')
//        {
//          lte.card=1;
//          return;
//        }
//        else
//          lte.card=0;
//      }
//      break;
//    }
//     
//  }
//}

//
//static void low_transmit(const char * data)
//{
//  uint8_t len;
//  len=strlen(data);
//  while(HAL_UART_Transmit(&LTE_UART,(uint8_t*)data,len,1000));
//  //HAL_UART_Transmit(&huart1,(uint8_t*)data,len,200);
//  //memset(data,0,len);
//}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if(huart->Instance==LPUART1)
  {
    
  }
}