/*******************************************************************************
        Filename:  sntperrno.h
     Description:  SNTP error number defines
         Version:  1.0
         Created:  01/23/2008 03 CST
******************************************************************************/

#ifndef _SNTPERRNO_H
#define _SNTPERRNO_H

#ifdef  __cplusplus
extern "C" {
#endif

#define SNTPPRT(x)          //printf x

#define SNTPOK              0           /* No error */

#define ESNTP_INIT          (-1)        /* Init SNTP lib failed */

#define ESNTP_PARAM         (-2)        /* Input parameters error */
#define ESNTP_NORES         (-3)        /* Memory allocate failed or no resources */

#define ESNTP_WRPKT         (-4)        /* Write SNTP packet failed */
#define ESNTP_RDPKT         (-5)        /* Read SNTP packet failed */

#define ESNTP_MAXQUERY      (-6)        /* Reject exceed max query number */
#define ESNTP_INCONSIST     (-7)        /* Inconsistent received time */
#define ESNTP_ALARM         (-8)        /* Packet with alarm flag set */

#define ESNTP_MUTEX         (-9)        /* Lock/unlock set time mutex failed */
#define ESNTP_SETTIME       (-10)       /* Set system time failed */

#define ESNTP_OTHER         (-11)       /* Other misc errors */

#define ESNTP_CONTENT       1           /* Packet content incorrect */
//qiuhc

#ifdef  __cplusplus
}
#endif
#endif  /* _SNTPERRNO_H */

