#include "stm32l0xx_hal.h"
#include "cJSON.h"
#include "socket.h"
#include "cmsis_os.h"
#include "lte.h"
#include "string.h"

ScmCommHead scm_head;
extern osMessageQId ltecmdHandle;
extern lte_status_s lte;
const char base[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
void test()
{
  cJSON *root;
  char*out;
  root=cJSON_CreateObject();	
  cJSON_AddItemToObject(root, "name", cJSON_CreateString("Jack (\"Bee\") Nimble"));
  cJSON_AddItemToObject(root, "test", cJSON_CreateString("haha"));
  out=cJSON_Print(root);
  cJSON_Delete(root);
  printf("%s\n",out);
}
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
  root=cJSON_CreateObject();
  cJSON_AddItemToObject(root, "device_id", cJSON_CreateString(device_id));
  cJSON_AddItemToObject(root, "user_id", cJSON_CreateString(user_id));
  cJSON_AddItemToObject(root, "token", cJSON_CreateString(token));
  char* out=cJSON_PrintUnformatted(root);
  cJSON_Delete(root);
  char* encode;
  //printf("%s\n",out);
  encode=b64_encode(out);
  //printf("%s\n",encode);
  vPortFree(out);
  scm_head.cmd=SCM360_LOGIN_REQ;
  scm_head.size=4+strlen(encode);
  //length=sizeof(ltecmdHandle)+strlen(out);
  uint8_t tmp[4];
  tmp[0]=((uint8_t*)&scm_head)[1];
  tmp[1]=((uint8_t*)&scm_head)[0];
  tmp[2]=((uint8_t*)&scm_head)[3];
  tmp[3]=((uint8_t*)&scm_head)[2];
  if((err=SocketWriteBin(tmp,sizeof(scm_head)))!=0)
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
  scm_head.cmd=SCM360_HEARBEAT_REQ;
  scm_head.size=4;
  uint8_t tmp[4];
  tmp[0]=((uint8_t*)&scm_head)[1];
  tmp[1]=((uint8_t*)&scm_head)[0];
  tmp[2]=((uint8_t*)&scm_head)[3];
  tmp[3]=((uint8_t*)&scm_head)[2];
  if(SocketWriteBin(tmp,sizeof(scm_head)))
     return 1;
  return 0;
}
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
