/* 
 * File:   tc.h
 * Author: sk
 *
 * Created on 2013-8-2
 */

#ifndef TC_H
#define	TC_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../../Inc/mydef.h"

typedef struct phaseinfo_t
{
	unsigned char			stageline;
    unsigned char           phaseid;
    unsigned char           gtime;
	unsigned char			gftime;
	unsigned char			ytime;
	unsigned char			rtime;
	unsigned char           phasetype;
    unsigned char           mingtime;
    unsigned char           maxgtime1;
    unsigned char           pctime;//person clear time
	unsigned char			cpcexist;//whether person channel exist in change channels;
	unsigned char			chan[MAX_CHANNEL]; //all channels 
	unsigned char			cchan[MAX_CHANNEL];//all change channels;
	unsigned char			cnpchan[MAX_CHANNEL];//change channels not include person channels
	unsigned char			cpchan[MAX_CHANNEL]; //change person channels
}phaseinfo_t;

typedef struct tcdata_t
{
    fcdata_t                *fd;
    tscdata_t               *td;
    ChannelStatus_t         *cs;
	phaseinfo_t				*pi;
}tcdata_t;

typedef struct pcdata_t
{
	unsigned char			*pchan;
	unsigned char           time;
	unsigned short			*markbit;
	int                     *bbserial;
	sockfd_t				*sockfd;
	ChannelStatus_t			*cs;
}pcdata_t;

typedef struct phasedetect_t
{
	unsigned char		stageid;
    unsigned char       phaseid;
    unsigned char       phasetype;

	unsigned char		fixgtime;
	unsigned char		gtime; //not include green flash time;
	unsigned char		gftime;
	unsigned char		ytime;
	unsigned char		rtime;
	unsigned char       mingtime;
    unsigned char       maxgtime;
	unsigned char		pgtime;
	unsigned char		pctime;

	unsigned char		downt;
	unsigned int		chans;
}phasedetect_t;

#ifdef RED_FLASH
typedef struct redflash_t
{
    unsigned char       tcline;
    unsigned char       slnum;
    unsigned char       snum;
	unsigned char		rft;
    unsigned char       *chan;
	tcdata_t			*tc;
}redflash_t; 

void tc_red_flash(void *arg);
#endif

typedef struct cyellowflash_t
{
	unsigned char		cyft;
	unsigned char		mark;
	unsigned char		*chan;
	tcdata_t            *tc;
}cyellowflash_t;
void tc_channel_yellow_flash(void *arg);

#ifdef	__cplusplus
}
#endif

#endif	/* TC_H */

