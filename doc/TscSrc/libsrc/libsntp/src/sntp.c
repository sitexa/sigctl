/*******************************************************************************
    Filename:  sntp.c
    Description:  SNTP lib main
    Version:  1.0
    Created:  03/20/2008 11 CST
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>

#include "sntp.h"
#include "sntperrno.h"

/* global variables */
typedef struct 
{
    int (*set_time_callback) (struct timeval *tv, struct timezone *tz);
    int (*gettimeZone)(void);
    pthread_mutex_t mutex;    
} sntp_callback_t;

sntp_callback_t sntp_handler;

/* external functions */
extern int ntp_get_time(char *hostname, struct timeval *new,
                        struct timeval *adjust, int port);

/*******************************************************************************
 *  Name: net_init_sntp_lib 
 *  Description:  
 * 
 *       Mode   Type         Name         Description
 * -----------------------------------------------------------------------------
 *        in:  
 *    in-out:  
 *       out:  
 *    return:  
 * -----------------------------------------------------------------------------
 *   Created: 03/20/2008 11 CST
 *  Revision: none
 ******************************************************************************/
int net_init_sntp_lib(void *func_ptr,void *func_zone)
{
    int ret = -1;
    static int done_init = 0;

    if(done_init != 0)
    {
        return SNTPOK;
    }
    
    memset(&sntp_handler, 0, sizeof(sntp_handler));

    sntp_handler.set_time_callback = func_ptr;
    sntp_handler.gettimeZone = func_zone;
    
    ret = pthread_mutex_init(&(sntp_handler.mutex), NULL);
    if(ret != 0)
    {
        SNTPPRT(("Create mutex failed. errno=0x%x\n", errno));
        return ESNTP_INIT;
    }
    
    done_init = 1;

    return SNTPOK;
}


/*******************************************************************************
 *  Name: net_fini_sntp_lib 
 *  Description:  
 * 
 *  Mode   Type         Name         Description
 * -----------------------------------------------------------------------------
 *        in:  
 *    in-out:  
 *       out:  
 *    return:  
 * -----------------------------------------------------------------------------
 *   Created: 03/20/2008 11 CST
 *  Revision: none
 ******************************************************************************/
void net_fini_sntp_lib(void)
{
    int ret = -1;

    sntp_handler.set_time_callback = NULL;
    
    ret = pthread_mutex_destroy(&(sntp_handler.mutex));
    if(ret != 0)
    {
        SNTPPRT(("Destroy mutex failed. errno=0x%x", errno));
    }
    
    return;
}

/*******************************************************************************
 *  Name: net_sntp_client 
 *  Description:  
 *  param[in] host the ntp server domain name  
 *  param[in] port the ntp server listening port  
 *  return:  SNTPOK:sucess other:error
 * -----------------------------------------------------------------------------
 *  Created: 03/20/2008 11 CST
 *  Revision: none
 ******************************************************************************/
int net_sntp_client(char *host, int port)
{
    int		family = PF_INET;       /* IPv4 support only now */
    int		ret = -1;
    double  adjsec;
    int timeZoneSec;
    struct timeval new, adjust;

    if((NULL == host) || (host[0] == '\0'))
    {
        SNTPPRT(("Please specify the NTP server.\n"));
        return ESNTP_PARAM;
    }

    memset(&new, 0, sizeof(new));
    memset(&adjust, 0, sizeof(adjust));

    ret = ntp_get_time(host, &new, &adjust, port);
    if(ret != SNTPOK)
    {
        SNTPPRT(("Get time form %s:%d failed\n", host, (port == 0)? 123:port));
        return ret;
    }

    ret = pthread_mutex_trylock(&(sntp_handler.mutex));   
    if(ret != 0)
    {
        SNTPPRT(("Lock semphore failed. errno=0x%x\n", errno));
        return ESNTP_MUTEX;
    }

    if(sntp_handler.set_time_callback == NULL)
    {
        /* correct system time */
        ret = settimeofday(&new, NULL);
        if(ret != 0)
        {
            SNTPPRT(("Call settimeofday failed. errno=0x%x\n", errno));
            return ESNTP_SETTIME;
        }
    }
    else
    {
        /* time need to be adjust form now */
        ret = sntp_handler.set_time_callback(&new, NULL);
        if(ret != SNTPOK)
        {
            SNTPPRT(("Call set time failed\n"));
            return ESNTP_SETTIME;
        }
    }

    ret = pthread_mutex_unlock(&(sntp_handler.mutex));
    if(ret != 0)
    {
        SNTPPRT(("unlock semphore failed. errno=0x%x\n", errno));
    }

    adjsec  = adjust.tv_sec + adjust.tv_usec / 1.0e6;
    timeZoneSec= sntp_handler.gettimeZone();
    adjsec = adjsec + timeZoneSec;
    SNTPPRT(("Adjust local clock by %.6f seconds\n", adjsec + 28800));

    return SNTPOK;
}

#define MINUTE 60
#define HOUR (60*MINUTE)
#define DAY (24*HOUR)
#define YEAR (365*DAY)
 
static int month[12] = {
 0,
 DAY*(31),
 DAY*(31+29),
 DAY*(31+29+31),
 DAY*(31+29+31+30),
 DAY*(31+29+31+30+31),
 DAY*(31+29+31+30+31+30),
 DAY*(31+29+31+30+31+30+31),
 DAY*(31+29+31+30+31+30+31+31),
 DAY*(31+29+31+30+31+30+31+31+30),
 DAY*(31+29+31+30+31+30+31+31+30+31),
 DAY*(31+29+31+30+31+30+31+31+30+31+30)
 };
 
int  kernel_mktime(struct tm * tm)
{
    int res;
    int year;

    if (NULL == tm)
    {
        printf ("kernel_mktime input error.\n");
        return 0;
    }
    year = tm->tm_year - 70;
    res = YEAR*year + DAY*((year+1)/4);
    res += month[tm->tm_mon];
    if (tm->tm_mon>1 && ((year+2)%4))
        res -= DAY;
    
    res += DAY*(tm->tm_mday-1);
    res += HOUR*tm->tm_hour;
    res += MINUTE*tm->tm_min;
    res += tm->tm_sec;
    
    return res;
 }


