/*-----------------------------------------------------------------------------
 * Copyright (C)，2023，CHAO YUAN INFORMATION. All rights reserved
 * 文 件 名： can_func.c
 * 作    者： CFG
 * 版    本： V1.0.0
 * 日    期： 2023-11-28
 * 概    述： 交通信号机CAN通讯功能实现
 ------------------------------------------------------------------------------
 * 修改记录：
 * <作者>     <日期>        <版本>       <改动说明>
 * 1.CFG     2023-11-28    V1.0.0      Initial
-----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "can_func.h"

#ifndef PF_CAN
#define PF_CAN 29
#endif

#ifndef AF_CAN
#define AF_CAN PF_CAN
#endif

/*CAN通讯功能参数定义*/
struct CAN_Param_t
{
  S32 canfd;             //CAN通讯设备文件描述符
  pthread_t threadid;    //CAN处理线程ID
  CAN_RECV_CALLBACK can_recv_callback;    //CAN数据接收回调函数
};

/**********************************CANThread**********************************/
/*
* 函数原型：can_process_thread_proc
* 功能描述：CAN处理线程函数
* 输入参数：arg：CAN通讯功能指针
* 输出参数：无
* 返 回 值：无
*/
static void *can_process_thread_proc(void *arg)
{
  CANFunc *pcan;
  if (arg)
    pcan = (CANFunc *)arg;
  else
    return NULL;

  U32 canid = 0;
  U8 recvbuf[8] = {0}, len = 0;
  while (1)
  {
    if (pcan->can_recv(pcan,&canid,recvbuf,&len) == CAN_RET_SUCCESS)
    {
      if (pcan->pcan_param->can_recv_callback)
        pcan->pcan_param->can_recv_callback(canid,recvbuf,len);
    }
  }

  return NULL;
}

/********************************CANInterface*********************************/
/*
* 函数原型：can_init
* 功能描述：CAN通讯初始化
* 输入参数：pCAN：CAN通讯功能指针
          pCANName：设备文件名
          nBitRate：设备波特率
* 输出参数：无
* 返 回 值：CAN_RET_SUCCESS-成功，CAN_RET_FAIL-失败
*/
U8 can_init(CANFunc *pCAN, const S8 *pCANName, S32 nBitRate)
{
  //check param
  if ((pCAN == NULL) || (pCAN->pcan_param == NULL) || (pCANName == NULL) || (nBitRate <= 0))
    return CAN_RET_FAIL;

  S8 strcmd[255];

  memset(strcmd,0,sizeof(strcmd));
  sprintf(strcmd,"ifconfig %s down",pCANName);
  system(strcmd);

  memset(strcmd,0,sizeof(strcmd));
  sprintf(strcmd,"ip link set %s type can bitrate %d restart-ms 200",pCANName,nBitRate);
  system(strcmd);

  memset(strcmd,0,sizeof(strcmd));
  sprintf(strcmd,"ifconfig %s up",pCANName);
  system(strcmd);

  S32 canfd;
  canfd = socket(PF_CAN,SOCK_RAW,CAN_RAW);
  if (canfd < 0)
    return CAN_RET_FAIL;

  struct ifreq ifr;
  strcpy(ifr.ifr_name,pCANName);
  if (ioctl(canfd,SIOCGIFINDEX,&ifr) < 0)
  {
    close(canfd);
    return CAN_RET_FAIL;
  }

  struct sockaddr_can addr;
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;
  if (bind(canfd,(struct sockaddr *)&addr,sizeof(addr)) < 0)
  {
    close(canfd);
    return CAN_RET_FAIL;
  }

  //create can process thread
  pthread_t threadid = 0;
  if (pthread_create(&threadid,NULL,can_process_thread_proc,(void *)pCAN))
  {
    perror("pthread_create[CANProcess]");
    return CAN_RET_FAIL;
  }

  pCAN->pcan_param->canfd = canfd;
  pCAN->pcan_param->threadid = threadid;

  return CAN_RET_SUCCESS;
}

/*
* 函数原型：can_uninit
* 功能描述：CAN通讯逆初始化
* 输入参数：pCAN：CAN通讯功能指针
* 输出参数：无
* 返 回 值：CAN_RET_SUCCESS-成功，CAN_RET_FAIL-失败
*/
U8 can_uninit(CANFunc *pCAN)
{
  //check param
  if ((pCAN == NULL) || (pCAN->pcan_param == NULL))
    return CAN_RET_FAIL;

  //free can process thread
  if (pCAN->pcan_param->threadid != 0)
    pthread_cancel(pCAN->pcan_param->threadid);

  close(pCAN->pcan_param->canfd);

  pCAN->pcan_param->canfd = 0;
  pCAN->pcan_param->threadid = 0;
  pCAN->pcan_param->can_recv_callback = NULL;

  return CAN_RET_SUCCESS;
}

/*
* 函数原型：can_set_recv_callback
* 功能描述：CAN通讯设置数据接收回调函数
* 输入参数：pCAN：CAN通讯功能指针
          pCallback：CAN数据接收回调函数指针
* 输出参数：无
* 返 回 值：CAN_RET_SUCCESS-成功，CAN_RET_FAIL-失败
*/
U8 can_set_recv_callback(CANFunc *pCAN, CAN_RECV_CALLBACK pCallback)
{
  //check param
  if ((pCAN == NULL) || (pCAN->pcan_param == NULL))
    return CAN_RET_FAIL;

  pCAN->pcan_param->can_recv_callback = pCallback;

  return CAN_RET_SUCCESS;
}

/*
* 函数原型：can_get_canfd
* 功能描述：CAN通讯获取设备文件描述符
* 输入参数：pCAN：CAN通讯功能指针
* 输出参数：无
* 返 回 值：设备文件描述符
*/
S32 can_get_canfd(CANFunc *pCAN)
{
  //check param
  if ((pCAN == NULL) || (pCAN->pcan_param == NULL))
    return 0;

  return pCAN->pcan_param->canfd;
}

/*
* 函数原型：can_send
* 功能描述：CAN通讯发送数据
* 输入参数：pCAN：CAN通讯功能指针
          nCANId：数据标识
          pBuf：数据缓冲区指针
          nLen：数据缓冲区长度
* 输出参数：无
* 返 回 值：CAN_RET_SUCCESS-成功，CAN_RET_FAIL-失败
*/
U8 can_send(CANFunc *pCAN, U32 nCANId, const U8 *pBuf, U8 nLen)
{
  //check param
  if ((pCAN == NULL) || (pCAN->pcan_param == NULL) || (pCAN->pcan_param->canfd <= 0) || (nLen > 8))
    return CAN_RET_FAIL;

  U8 i;
  struct can_frame frame;
  frame.can_id = nCANId;
  frame.can_dlc = nLen;
  memset(&frame.data,0,sizeof(frame.data));
  for (i = 0; i < frame.can_dlc; ++i)
    frame.data[i] = pBuf[i];

  //write data
  S32 ret;
  ret = write(pCAN->pcan_param->canfd,&frame,sizeof(frame));
  if (ret == sizeof(frame))
    ret = CAN_RET_SUCCESS;
  else
    ret = CAN_RET_FAIL;

  return (ret & 0xFF);
}

/*
* 函数原型：can_recv
* 功能描述：CAN通讯接收数据
* 输入参数：pCAN：CAN通讯功能指针
          pCANId：数据标识指针
          pBuf：数据缓冲区指针
          pLen：数据缓冲区长度指针
* 输出参数：无
* 返 回 值：CAN_RET_SUCCESS-成功，CAN_RET_FAIL-失败
*/
U8 can_recv(CANFunc *pCAN, U32 *pCANId, U8 *pBuf, U8 *pLen)
{
  //check param
  if ((pCAN == NULL) || (pCAN->pcan_param == NULL) || (pCAN->pcan_param->canfd <= 0) || (pCANId == NULL) || (pBuf == NULL) || (pLen == NULL))
    return CAN_RET_FAIL;

  //read data
  S32 ret;
  struct can_frame frame;
  ret = read(pCAN->pcan_param->canfd,&frame,sizeof(frame));
  if (ret < 0)
  {
    perror("read[CANFrame]");
    return CAN_RET_FAIL;
  }
  else if (ret < sizeof(frame))
  {
    printf("CAN Recv Fail\n");
    return CAN_RET_FAIL;
  }
  else if (frame.can_id & CAN_ERR_FLAG)
  {
    printf("CAN Recv Error: %08X\n",frame.can_id);
    return CAN_RET_FAIL;
  }
  else
  {
    U8 i;
    *pCANId = frame.can_id;
    *pLen = frame.can_dlc;
    for (i = 0; i < frame.can_dlc; ++i)
      pBuf[i] = frame.data[i];
    return CAN_RET_SUCCESS;
  }
}

/**********************************CANFunc************************************/
/*
* 函数原型：can_create
* 功能描述：CAN通讯功能创建
* 输入参数：无
* 输出参数：无
* 返 回 值：!=NULL-CAN通讯功能指针，NULL-失败
*/
CANFunc *can_create()
{
  CANFunc *pcan;

  //create CANFunc
  pcan = (CANFunc *)malloc(sizeof(CANFunc));
  if (pcan == NULL)
    return pcan;

  //create CANParam
  pcan->pcan_param = (CANParam *)malloc(sizeof(CANParam));
  if (pcan->pcan_param == NULL)
  {
    free(pcan);
    pcan = NULL;
    return pcan;
  }

  pcan->pcan_param->canfd = 0;
  pcan->pcan_param->threadid = 0;
  pcan->pcan_param->can_recv_callback = NULL;

  pcan->can_init = can_init;
  pcan->can_uninit = can_uninit;
  pcan->can_set_recv_callback = can_set_recv_callback;
  pcan->can_get_canfd = can_get_canfd;
  pcan->can_send = can_send;
  pcan->can_recv = can_recv;

  return pcan;
}

/*
* 函数原型：can_free
* 功能描述：CAN通讯功能释放
* 输入参数：pCAN：CAN通讯功能指针
* 输出参数：无
* 返 回 值：无
*/
void can_free(CANFunc *pCAN)
{
  //free CANFunc
  if (pCAN)
  {
    //free CANParam
    if (pCAN->pcan_param)
    {
      free(pCAN->pcan_param);
      pCAN->pcan_param = NULL;
    }
    free(pCAN);
    pCAN = NULL;
  }
}
