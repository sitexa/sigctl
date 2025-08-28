/**@file sntp.h
 * @note Anhui Chaoyuan Info Technology Co., Ltd. All Right Reserved.
 * @brief    stor api
 * @author   ben
 * @date     20131210
 * @version  v1.0.0
 * @note ///Description here 
 * @note History:        
 * @note     <author>   <time>    <version >   <desc>
 * @note    ben       20131210  V1.0.0        Create
 * @warning  
 */
#ifndef _SNTP_H_
#define _SNTP_H_

#ifdef  __cplusplus
extern "C" {
#endif

int net_init_sntp_lib(void *func_ptr,void *func_zone);

int net_sntp_client(char *host, int port);

void net_fini_sntp_lib(void);

#ifdef __cplusplus
}
#endif

#endif

