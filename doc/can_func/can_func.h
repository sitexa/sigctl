/*-----------------------------------------------------------------------------
 * Copyright (C)，2023，CHAO YUAN INFORMATION. All rights reserved
 * 文 件 名： can_func.h
 * 作    者： CFG
 * 版    本： V1.0.0
 * 日    期： 2023-11-28
 * 概    述： 交通信号机CAN通讯功能定义
 ------------------------------------------------------------------------------
 * 修改记录：
 * <作者>     <日期>        <版本>       <改动说明>
 * 1.CFG     2023-11-28    V1.0.0      Initial
-----------------------------------------------------------------------------*/

#ifndef _CAN_FUNC_H_
#define _CAN_FUNC_H_

#ifdef _cplusplus
extern "C" {
#endif

#include <arpa/inet.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/can/error.h>

#include "common_type.h"

/*返回代码定义*/
typedef enum
{
  CAN_RET_SUCCESS = 0,    /*成功*/
  CAN_RET_FAIL = 1        /*失败*/
} CAN_RET_CODE;

/*CAN数据接收回调函数指针定义*/
typedef void (*CAN_RECV_CALLBACK)(U32, U8*, U8);

/*CAN通讯功能参数声明*/
typedef struct CAN_Param_t CANParam;

/*CAN通讯功能定义*/
typedef struct CAN_Func_t
{
  /*CAN通讯功能参数指针*/
  CANParam *pcan_param;

  /*CAN通讯初始化*/
  U8 (*can_init)(struct CAN_Func_t *pCAN, const S8 *pCANName, S32 nBitRate);
  /*CAN通讯逆初始化*/
  U8 (*can_uninit)(struct CAN_Func_t *pCAN);

  /*CAN通讯设置数据接收回调函数*/
  U8 (*can_set_recv_callback)(struct CAN_Func_t *pCAN, CAN_RECV_CALLBACK pCallback);
  /*CAN通讯获取设备文件描述符*/
  S32 (*can_get_canfd)(struct CAN_Func_t *pCAN);

  /*CAN通讯发送数据*/
  U8 (*can_send)(struct CAN_Func_t *pCAN, U32 nCANId, const U8 *pBuf, U8 nLen);
  /*CAN通讯接收数据*/
  U8 (*can_recv)(struct CAN_Func_t *pCAN, U32 *pCANId, U8 *pBuf, U8 *pLen);
} CANFunc;

/*CAN通讯功能创建*/
CANFunc *can_create();

/*CAN通讯功能释放*/
void can_free(CANFunc *pCAN);

#ifdef _cplusplus
}
#endif

#endif
