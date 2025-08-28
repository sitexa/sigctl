/* 
 * File:   dcs.h
 * Author: sk
 *
 * Created on 20130922
 */

#ifndef DCS_H
#define	DCS_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../../Inc/mydef.h"
typedef struct dcpinfo_t
{
    unsigned char           stageline;
    unsigned char           phaseid;
    unsigned char           gtime;
    unsigned char           gftime;
    unsigned char           ytime;
    unsigned char           rtime;
    unsigned char           phasetype;
    unsigned char           mingtime;
    unsigned char           maxgtime1;
	unsigned char			pdelay;
	unsigned char			ldeti;
	unsigned char			fixgtime; //green time of fix phase
	unsigned char			pgtime;//person green time
    unsigned char           pctime;//person clear time
    unsigned char           cpcexist;//whether person channel exist in change channels;
    unsigned char           chan[MAX_CHANNEL]; //all channels 
    unsigned char           cchan[MAX_CHANNEL];//all change channels;
    unsigned char           cnpchan[MAX_CHANNEL];//change channels not include person channels
    unsigned char           cpchan[MAX_CHANNEL]; //change person channels
    unsigned char           roaddelaychan[MAX_CHANNEL]; //need delay channels
    unsigned char           roadbit[MAX_CHANNEL]; //quenue location recond
    unsigned char           persionroadbit[MAX_CHANNEL]; //persion location recond
    unsigned char           npchan[MAX_CHANNEL]; //next phase channel
    unsigned char          	xiredchannel[4]; //xibian red channel
    unsigned char          	nanredchannel[4]; //xibian red channel
    unsigned char          	dongredchannel[4]; //xibian red channel
    unsigned char          	beiredchannel[4]; //xibian red channel
    unsigned char          	redchannel[MAX_CHANNEL]; //xibian red channel
    
}dcpinfo_t;

typedef struct dcdata_t
{
	fcdata_t			*fd;
	tscdata_t			*td;
	dcpinfo_t			*pi;
	ChannelStatus_t		*cs;		
}dcdata_t;

typedef struct pephase_t
{
	unsigned char		phaseid;//pending phase id;
	unsigned char		mark; //'1' means have vehicle request, '0' means not have vehicle request;
}pephase_t;//pending phase

typedef struct dcpcdata_t
{
	unsigned char		*pchan;
	unsigned char		time;
	unsigned short		*markbit;
	int					*bbserial;
	sockfd_t			*sockfd;
	ChannelStatus_t		*cs;
}dcpcdata_t;

typedef struct detectorpro_t
{
	unsigned char		deteid;
	unsigned char		validmark; //the detector is invalid at intial stage;
	unsigned char		err; //err is 1 mean that the detector has been status of error;
	unsigned short		intime; //invalid time;
	struct timeval		stime;
}detectorpro_t;

typedef struct phasedetect_t
{
	unsigned char       stageid;
	unsigned char       fixgtime;
	unsigned char       gtime; //not include green flash time;
	unsigned char       gftime;
	unsigned char       ytime;
	unsigned char       rtime;
    unsigned char       mingtime;
	unsigned char       maxgtime;
    unsigned char       pgtime;
    unsigned char       pctime;
    unsigned char       downt;
    unsigned int        chans;

	unsigned char		phaseid;
	unsigned char		phasetype;
	unsigned char		indetenum; //invalid detector arrive the number,begin to degrade;
	unsigned char		validmark; //the phase is valid at intial stage;
	unsigned char		factnum; //fact the number of invalid detector;
	unsigned char		indetect[10];//fact invalid detector
	detectorpro_t		detectpro[10];
}phasedetect_t;

typedef struct monpendphase_t
{
	int				*pendpipe;
	Detector_t		*detector;	
}monpendphase_t;

#ifdef RED_FLASH
typedef struct redflash_dc
{
    unsigned char       tcline;
    unsigned char       slnum;
    unsigned char       snum;
	unsigned char		rft;
    unsigned char       *chan;
	dcdata_t			*dc;
}redflash_dc; 

void ms_red_flash(void *arg);
#endif

typedef struct cyellowflash_t
{
    unsigned char       cyft;
    unsigned char       mark;
    unsigned char       *chan;
   	dcdata_t			*dc; 
}cyellowflash_t;
void ms_channel_yellow_flash(void *arg);
#ifdef	__cplusplus
}
#endif

#endif	/* DCS_H */

