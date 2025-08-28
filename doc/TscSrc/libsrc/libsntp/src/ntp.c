/*  $OpenBSD: ntp.c,v 1.29 2006/09/17 17:03:56 ckuethe Exp $    */

/*
 * Copyright (c) 1996, 1997 by N.M. Maclaren. All rights reserved.
 * Copyright (c) 1996, 1997 by University of Cambridge. All rights reserved.
 * Copyright (c) 2002 by Thorsten "mirabile" Glaser.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the university may be used to
 *    endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "ntpleaps.h"
#include "arc4random.h"

#include "sntp.h"
#include "sntperrno.h"
//无锡所送检前测试延迟误差用
//#define SNTPDEBUG 1

/*
 * NTP definitions.  Note that these assume 8-bit bytes - sigh.  There
 * is little point in parameterising everything, as it is neither
 * feasible nor useful.  It would be very useful if more fields could
 * be defined as unspecified.  The NTP packet-handling routines
 * contain a lot of extra assumptions.
 */
#define JAN_1970_INT   2208988800     /* 1970 - 1900 in seconds */

#define JAN_1970   2208988800.0     /* 1970 - 1900 in seconds */
#define NTP_SCALE  4294967296.0     /* 2^32, of course! */
#define NTPFRAC(x) (4294*(x)+((1981*(x))>>11))

#define NTP_MODE_CLIENT       3     /* NTP client mode */
#define NTP_MODE_SERVER       4     /* NTP server mode */
#define NTP_VERSION           4     /* The current version */
#define NTP_VERSION_MIN       1     /* The minum valid version */
#define NTP_VERSION_MAX       4     /* The maximum valid version */
#define NTP_STRATUM_MAX      14     /* The maximum valid stratum */
#define NTP_INSANITY     3600.0     /* Errors beyond this are hopeless */

#define NTP_PACKET_MIN       48     /* Without authentication */
#define NTP_PACKET_MAX       68     /* With authentication (ignored) */

#define NTP_DISP_FIELD        8     /* Offset of dispersion field */
#define NTP_REFERENCE        16     /* Offset of reference timestamp */
#define NTP_ORIGINATE        24     /* Offset of originate timestamp */
#define NTP_RECEIVE          32     /* Offset of receive timestamp */
#define NTP_TRANSMIT         40     /* Offset of transmit timestamp */

#define STATUS_NOWARNING      0     /* No Leap Indicator */
#define STATUS_LEAPHIGH       1     /* Last Minute Has 61 Seconds */
#define STATUS_LEAPLOW        2     /* Last Minute Has 59 Seconds */
#define STATUS_ALARM          3     /* Server Clock Not Synchronized */

#define MAX_QUERIES          25
#define MAX_DELAY            15

#define MILLION_L       1000000l    /* For conversion to/from timeval */
#define MILLION_D          1.0e6    /* Must be equal to MILLION_L */

#define SA_LEN(x)       sizeof(*x)


/* data structure */
struct ntp_data {
    u_char      status;
    u_char      version;
    u_char      mode;
    u_char      stratum;
    double      receive;
    double      transmit;
    double      current;
    u_int64_t   recvck;

    /* Local State */
    double      originate;
    u_int64_t   xmitck;
};


/* local functions */
static int      sync_ntp(int, double *, double *);
static int      write_packet(int, struct ntp_data *);
static int      read_packet(int, struct ntp_data *, double *, double *);
static void     unpack_ntp(struct ntp_data *, u_char *);
static double   current_time(double);
static void     create_timeval(double, struct timeval *, struct timeval *);
extern int udp_connect (const char *host, const char *serv);

#ifdef SNTPDEBUG
static void     print_packet(const struct ntp_data *);
#endif


/*************************************************
 Function:  udp_connect
 Description:   creat a udp connected socket with host & serv
 Input:         host: domain name or ip string
                        serv: port or service name
 Output:
 Return:        success: socket fd    error:-1
*************************************************/
int udp_connect (const char *host, const char *serv)
{
    int     sockfd=-1, n;
    struct addrinfo hints, *res, *ressave;

    bzero (&hints, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ( (n = getaddrinfo (host, serv, &hints, &res)) != 0)
    {
        return -1;
    }

    ressave = res;

    while ( res != NULL)
    {
        sockfd = socket (res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0)
        {
            usleep(1);
            res = res->ai_next;
            continue;         /* ignore this one */
        }

        if (connect (sockfd, res->ai_addr, res->ai_addrlen) == 0)
        {
            break;            /* success */
        }

        close (sockfd);       /* ignore this one */
        usleep(1);
        res = res->ai_next;
    }

    if (res == NULL)
    {
        freeaddrinfo(ressave);
        return -1;
    }

    freeaddrinfo (ressave);

    return (sockfd);
}

/*******************************************************************************
 *         Name: ntp_get_time 
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
int ntp_get_time(char *hostname, struct timeval *new,
    struct timeval *adjust, int port)
{
    double  offset;
    double  error;
    int     accept = 0;
    int     ret = -1;
    int     s = -1;
    char buf[8] = {0};

    sprintf(buf, "%d", port > 0?port:123);
    s = udp_connect(hostname, buf); 
    if(s < 0)
    {
        SNTPPRT(("SNTP create socket failed. errno=0x%x\n", errno));
        return ESNTP_NORES;
    }

    ret = sync_ntp(s, &offset, &error);
    if(ret < 0)
    {
        SNTPPRT(("SNTP sync server error\n"));
        close(s);
        s = -1;
        return ret;
    }
    close(s);
    s = -1;
    accept++;

    SNTPPRT(("Correction: %.6f +/- %.6f\n", offset, error));

    if (accept < 1)
    {
        SNTPPRT(("Unable to get a reasonable time estimate\n"));
    }

    create_timeval(offset, new, adjust);

    return SNTPOK;
}


/*******************************************************************************
 *         Name: sync_ntp 
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
int sync_ntp(int fd, double *offset, double *error)
{
    int     accepts = 0;
    int     rejects = 0;
    int     attempts = 0;
    int     delay = MAX_DELAY;
    int     ret;
    double deadline;
    double a, b, x, y;
    double minerr = 0.1;        /* Maximum ignorable variation */
    struct ntp_data data;

    deadline = current_time(JAN_1970) + delay;
    *offset = 0.0;
    *error = NTP_INSANITY;

    while ((accepts < MAX_QUERIES) && (attempts < 2 * MAX_QUERIES))
    {
        memset(&data, 0, sizeof(data));

        if (current_time(JAN_1970) > deadline)
        {
            SNTPPRT(("Not enough valid responses received in time\n"));
            return ESNTP_OTHER;
        }

        if (write_packet(fd, &data) < 0)
        {
            return ESNTP_WRPKT;
        }

        ret = read_packet(fd, &data, &x, &y);
        if (ret < 0)
        {
            return ESNTP_RDPKT;
        }
        else if (ret > 0)
        {
#ifdef SNTPDEBUG
            print_packet(&data);
#endif
            if (++rejects > MAX_QUERIES)
            {
                SNTPPRT(("Too many bad or lost packets\n"));
                return ESNTP_MAXQUERY;
            }
            else
            {
                continue;
            }
        }
        else
        {
            ++accepts;
        }

        printf("Offset: %.6f +/- %.6f\n", x + 28800, y);

        if ((a = x - *offset) < 0.0)
        {
            a = -a;
        }

        if (accepts <= 1)
        {
            a = 0.0;
        }

        b = *error + y;
        if (y < *error)
        {
            *offset = x;
            *error = y;
        }

        printf("Best: %.6f +/- %.6f\n", *offset, *error);
        
//        printf("Best: %.6f +/- %.6f\n", *offset, *error);

        if (a > b)
        {
            SNTPPRT(("Inconsistent times received from NTP server\n"));
            return ESNTP_INCONSIST;
        }

        if ((data.status & STATUS_ALARM) == STATUS_ALARM)
        {
            SNTPPRT(("Ignoring NTP server with alarm flag set\n"));
            return ESNTP_ALARM;
        }

        if (*error <= minerr)
        {
            break;
        }
    }

    return (accepts);
}


/*
 * Get the current UTC time in seconds since the Epoch plus an offset
 * (usually the time from the beginning of the century to the Epoch)
 */
double
current_time_v2(double offset,long *pInSec,long *pInUSec)
{
    struct timeval current;
    u_int64_t t;

    if (gettimeofday(&current, NULL))
    {
        SNTPPRT(("Could not get local time of day\n"));
        return(0.0);
    }

    /*
     * At this point, current has the current TAI time.
     * Now subtract leap seconds to set the posix tick.
     */
    t = SEC_TO_TAI64(current.tv_sec);
    *pInSec = current.tv_sec;
    *pInUSec = current.tv_usec;
    return (offset + TAI64_TO_SEC(t) + 1.0e-6 * current.tv_usec);
}

/*******************************************************************************
 *         Name: write_packet 
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
int
write_packet(int fd, struct ntp_data *data)
{
    u_char  packet[NTP_PACKET_MIN];
    ssize_t length = 0;

    memset(packet, 0, sizeof(packet));

    packet[0] = (NTP_VERSION << 3) | (NTP_MODE_CLIENT);
#if 0
    data->xmitck = (u_int64_t)arc4random() << 32 | arc4random();
#endif
    /*
     * Send out a random 64-bit number as our transmit time.  The NTP
     * server will copy said number into the originate field on the
     * response that it sends us.  This is totally legal per the SNTP spec.
     *
     * The impact of this is two fold: we no longer send out the current
     * system time for the world to see (which may aid an attacker), and
     * it gives us a (not very secure) way of knowing that we're not
     * getting spoofed by an attacker that can't capture our traffic
     * but can spoof packets from the NTP server we're communicating with.
     *
     * No endian concerns here.  Since we're running as a strict
     * unicast client, we don't have to worry about anyone else finding
     * the transmit field intelligible.
     */

    //*(u_int64_t *)(packet + NTP_TRANSMIT) = data->xmitck;
#if 1
    long inSec,inUsec;
    data->originate = current_time_v2(JAN_1970,&inSec,&inUsec);   
#else
    data->originate = current_time(JAN_1970);
    *(u_int32_t *)(packet + NTP_REFERENCE) = htonl(inSec+JAN_1970_INT);
    *(u_int32_t *)(packet + NTP_REFERENCE + 4) = htonl(NTPFRAC(inUsec));

    *(u_int32_t *)(packet + NTP_ORIGINATE) = htonl(inSec+JAN_1970_INT);
    *(u_int32_t *)(packet + NTP_ORIGINATE + 4) = htonl(NTPFRAC(inUsec));
#endif

    *(u_int32_t *)(packet + NTP_TRANSMIT) = htonl(inSec+JAN_1970_INT - 28800);
    *(u_int32_t *)(packet + NTP_TRANSMIT + 4) = htonl(NTPFRAC(inUsec));

    data->xmitck = ((u_int64_t)(*(u_int32_t *)(packet + NTP_TRANSMIT)) << 32) |\
        (*(u_int32_t *)(packet + NTP_TRANSMIT + 4));
#ifdef  SNTPDEBUG
    struct ntp_data struData;
    unpack_ntp(&struData, packet);
    print_packet(&struData);
#endif
    length = write(fd, packet, sizeof(packet));
    if (length != sizeof(packet))
    {
        SNTPPRT(("Unable to send NTP packet to server"));
        return ESNTP_WRPKT;
    }
    return SNTPOK;
}


/*
 * Check the packet and work out the offset and optionally the error.
 * Note that this contains more checking than xntp does. Return 0 for
 * success, 1 for failure. Note that it must not change its arguments
 * if it fails.
 */
int
read_packet(int fd, struct ntp_data *data, double *off, double *error)
{
    u_char  receive[NTP_PACKET_MAX];
    struct  timeval tv;
    double  x, y;
    int     length = 0, r = 0;
    fd_set  *rfds;

    rfds = calloc(howmany(fd + 1, NFDBITS), sizeof(fd_mask));
    if (rfds == NULL)
    {
        SNTPPRT(("calloc"));
        return ESNTP_NORES;
    }

    FD_SET(fd, rfds);

retry:
    tv.tv_sec = 0;
    tv.tv_usec = 1000000 * MAX_DELAY / MAX_QUERIES;

    r = select(fd + 1, rfds, NULL, NULL, &tv);

    if (r < 0)
    {
        if (errno == EINTR)
        {
            goto retry;
        }
        else
        {
            SNTPPRT(("select error. errno=0x%x\n", errno));
        }

        free(rfds);
        return (r);
    }

    if (r != 1 || !FD_ISSET(fd, rfds)) {
        free(rfds);
        return (1);
    }

    free(rfds);

    length = read(fd, receive, NTP_PACKET_MAX);
    if (length < 0)
    {
        SNTPPRT(("Unable to receive NTP packet from server\n"));
        return ESNTP_RDPKT;
    }

    if (length < NTP_PACKET_MIN || length > NTP_PACKET_MAX)
    {
        SNTPPRT(("Invalid NTP packet size, packet rejected\n"));
        return (1);
    }

    unpack_ntp(data, receive);
#if 0
    if (data->recvck != data->xmitck)
    {
        SNTPPRT(("Invalid cookie received, packet rejected\n"));
        return (1);
    }
#endif
    if (data->version < NTP_VERSION_MIN ||
        data->version > NTP_VERSION_MAX)
    {
        SNTPPRT(("Received NTP version %u, need %u or lower\n",
                    data->version, NTP_VERSION));
        return (1);
    }

    if (data->mode != NTP_MODE_SERVER)
    {
        SNTPPRT(("Invalid NTP server mode, packet rejected\n"));
        return (1);
    }

    if (data->stratum > NTP_STRATUM_MAX)
    {
        SNTPPRT(("Invalid stratum received, packet rejected\n"));
        return (1);
    }

    if (data->transmit == 0.0)
    {
        SNTPPRT(("Server clock invalid, packet rejected\n"));
        return (1);
    }

    x = data->receive - data->originate;
    y = data->transmit - data->current;

    //客户端与服务端的时间系统的偏移
    *off = (x + y) / 2;
    //网络的往返延迟定义为δ
    *error = x - y;

    x = (data->current - data->originate) / 2;

    if (x > *error)
    {
        *error = x;
    }

    return (0);
}


/*
 * Unpack the essential data from an NTP packet, bypassing struct
 * layout and endian problems.  Note that it ignores fields irrelevant
 * to SNTP.
 */
void
unpack_ntp(struct ntp_data *data, u_char *packet)
{
    int i;
    double d;

    data->current = current_time(JAN_1970);

    data->status = (packet[0] >> 6);
    data->version = (packet[0] >> 3) & 0x07;
    data->mode = packet[0] & 0x07;
    data->stratum = packet[1];

#ifdef SNTPDEBUG
    for (i = 0, d = 0.0; i < 8; ++i)
    {
        d = 256.0*d+packet[NTP_ORIGINATE+i];
    }

    data->originate = d / NTP_SCALE;
#endif
    for (i = 0, d = 0.0; i < 8; ++i)
    {
        d = 256.0*d+packet[NTP_RECEIVE+i];
    }

    data->receive = d / NTP_SCALE;

    for (i = 0, d = 0.0; i < 8; ++i)
    {
        d = 256.0*d+packet[NTP_TRANSMIT+i];
    }

    data->transmit = d / NTP_SCALE;
    
  
    /* See write_packet for why this isn't an endian problem. */
    data->recvck = *(u_int64_t *)(packet + NTP_ORIGINATE);
}


/*
 * Get the current UTC time in seconds since the Epoch plus an offset
 * (usually the time from the beginning of the century to the Epoch)
 */
double
current_time(double offset)
{
    struct timeval current;
    u_int64_t t;

    if (gettimeofday(&current, NULL))
    {
        SNTPPRT(("Could not get local time of day\n"));
        return(0.0);
    }

    /*
     * At this point, current has the current TAI time.
     * Now subtract leap seconds to set the posix tick.
     */
    t = SEC_TO_TAI64(current.tv_sec);

    return (offset + TAI64_TO_SEC(t) + 1.0e-6 * current.tv_usec);
}

/*
 * Change offset into current UTC time. This is portable, even if
 * struct timeval uses an unsigned long for tv_sec.
 */
void
create_timeval(double difference, struct timeval *new, struct timeval *adjust)
{
    struct timeval old;
    long n;

    /* Start by converting to timeval format. Note that we have to
     * cater for negative, unsigned values. */
    if ((n = (long) difference) > difference)
    {
        --n;
    }

    adjust->tv_sec = n;
    adjust->tv_usec = (long) (MILLION_D * (difference-n));
    errno = 0;

    if (gettimeofday(&old, NULL))
    {
        SNTPPRT(("Could not get local time of day\n"));
    }
    new->tv_sec = old.tv_sec + adjust->tv_sec;
    new->tv_usec = (n = (long) old.tv_usec + (long) adjust->tv_usec);

    if (n < 0)
    {
        new->tv_usec += MILLION_L;
        --new->tv_sec;
    }
    else if (n >= MILLION_L)
    {
        new->tv_usec -= MILLION_L;
        ++new->tv_sec;
    }

    return;
}

#ifdef SNTPDEBUG
void
print_packet(const struct ntp_data *data)
{
    printf("status:      %u\n", data->status);
    printf("version:     %u\n", data->version);
    printf("mode:        %u\n", data->mode);
    printf("stratum:     %u\n", data->stratum);
    printf("originate:   %f\n", data->originate);
    printf("receive:     %f\n", data->receive);
    printf("transmit:    %f\n", data->transmit);
    printf("current:     %f\n", data->current);
    printf("xmitck:      0x%0lX\n", data->xmitck);
    printf("recvck:      0x%0lX\n", data->recvck);
};
#endif

