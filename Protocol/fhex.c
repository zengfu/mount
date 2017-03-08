#include "fhex.h"
#include "cmsis_os.h"
#include "s2l.h"
#include "string.h"

#define flash_begin 0x080000c0 

extern DMA_HandleTypeDef hdma_usart1_tx;
extern UART_HandleTypeDef huart1;
extern osSemaphoreId Uart1LockHandle;
extern osMutexId EventLockHandle;


static uint8_t* TxP;

static void CmdSend(uint8_t* data,uint8_t length);
static void DumpFrame(uint8_t* data,uint16_t id,uint8_t length);

extern uint16_t GlobalEvent;

void GetVersion(uint8_t* data);
void SetAccel(uint8_t* data);
void GetAccel(uint8_t* data);
void ReadAccel(uint8_t* data);
void SetReset(uint8_t* data);
void SetLogin(uint8_t* data);
void ReadEvent(uint8_t* data);


#define GetVersionId 0X0000
#define SetAccelId   0x0001
#define GetAccelId   0x0002
#define ReadAccelId  0x0003
#define SetResetId   0x0004
#define SetLoginId   0x0010
#define ReadEventId  0x0020

const FuncTypeDef S2lFunc[]={
  {GetVersionId,GetVersion},
  {SetAccelId,SetAccel},
  {GetAccelId,GetAccel},
  {ReadAccelId,ReadAccel},
  {SetResetId,SetReset},
  {SetLoginId,SetLogin},
  {ReadEventId,ReadEvent},
};
__root const uint8_t version@flash_begin=1;


void ExecCmd(uint16_t id,uint8_t* pdata)
{
  for(int i=0;i<(sizeof(S2lFunc)/sizeof(S2lFunc[0]));i++)
  {
    if(S2lFunc[i].cmdid==id)
    {
      S2lFunc[i].exec(pdata);
      break;
    }
  }
}
void GetVersion(uint8_t* data)
{
  
  DumpFrame((uint8_t*)&version,GetVersionId,1);
  //CmdSend(TxP,12);
}
void SetAccel(uint8_t* data)
{
  
}
void GetAccel(uint8_t* data)
{
  
}
void ReadAccel(uint8_t* data)
{
  
}
void SetReset(uint8_t* data)
{
  
}
void SetLogin(uint8_t* data)
{
  uint16_t length;
  length=data[2]<<8|data[3];
  DumpFrame(data+8,SetLoginId,length-4);
}
void ReadEvent(uint8_t* data)
{
  uint8_t tmp[2];
  tmp[0]=(GlobalEvent>>8)&0xff;
  tmp[1]=GlobalEvent&0xff;
  DumpFrame(tmp,ReadEventId,2);
  
  osMutexWait(EventLockHandle,osWaitForever);
  //
  GlobalEvent=0x8000;
  osMutexRelease(EventLockHandle);
}


void SendAck()
{
  
//  Txp[0]=0xaa;
//  Txp[1]=0x55;//sof
//  Txp[2]=0x00;
//  Txp[3]=0x04;//len
//  Txp[4]=0xff;
//  Txp[5]=0xfb;//clen
//  Txp[6]=0xee;
//  Txp[7]=0xff;//ack id
//  Txp[8]=0x50;
//  Txp[9]=0x0c;//crc
//  Txp[10]=0x00;//eof
  DumpFrame(NULL,0xeeff,0);
 
}

static void DumpFrame(uint8_t* data,uint16_t id,uint8_t length)
{
  //no data
  //get the lock
  length+=11;
  osSemaphoreWait(Uart1LockHandle,osWaitForever);
  TxP=pvPortMalloc(length);
  uint16_t rl=length-7;
  TxP[0]=0xaa;
  TxP[1]=0x55;
  TxP[2]=(rl>>8)&0xff;
  TxP[3]=rl&0xff;
  TxP[4]=((~rl)>>8)&0xff;
  TxP[5]=(~rl)&0xff;
  TxP[6]=(id>>8)&0xff;
  TxP[7]=id&0xff;
  for(int i=0;i<(rl-4);i++)
  {
    TxP[8+i]=data[i];
  }
  uint16_t crcseed=0xffff;
  for(int i=0;i<rl-2;i++)
  {
    crcseed=crc16(TxP[i+6],crcseed);
  }
  TxP[length-3]=(crcseed>>8)&0xff;
  TxP[length-2]=crcseed&0xff;
  TxP[length-1]=0x00;
  CmdSend(TxP,length);
}
static void CmdSend(uint8_t* data,uint8_t length)
{
  
  HAL_UART_Transmit_DMA(&huart1,data,length);
}


void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if(huart->Instance ==USART1)
  {
    osSemaphoreRelease(Uart1LockHandle);
    vPortFree(TxP);
  }
}