/* 
 * File:   rcc.h
 * Author: sk
 *
 * Created on 2016年6月28日, 下午2:19
 */

#ifndef RCC_H
#define	RCC_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../../Inc/mydef.h"

typedef struct rccpinfo_t
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
	unsigned char			maxgtime2;
	unsigned char			pdelay;
	unsigned char			fixgtime; //green time of fix phase
	unsigned char			pgtime;//person green time
    unsigned char           pctime;//person clear time
    unsigned char           cpcexist;//whether person channel exist in change channels;
	unsigned char			pcexist;
    unsigned char           chan[MAX_CHANNEL]; //all channels 
	unsigned char			npchan[MAX_CHANNEL];
	unsigned char			pchan[MAX_CHANNEL];
    unsigned char           cchan[MAX_CHANNEL];//all change channels;
    unsigned char           cnpchan[MAX_CHANNEL];//change channels not include person channels
    unsigned char           cpchan[MAX_CHANNEL]; //change person channels
}rccpinfo_t;

typedef struct rccdata_t
{
	fcdata_t			*fd;
	tscdata_t			*td;
	rccpinfo_t			*pi;
	ChannelStatus_t		*cs;		
}rccdata_t;


typedef struct rccpcdata_t
{
	unsigned char		*pchan;
	unsigned char		time;
	unsigned short		*markbit;
	int					*bbserial;
	sockfd_t			*sockfd;
	ChannelStatus_t		*cs;
}rccpcdata_t;

typedef struct detectorpro_t
{
	unsigned char		deteid;
	unsigned char		detetype;
	unsigned char		validmark; //the detector is invalid at intial stage;
	unsigned char		err; //err is 1 mean that the detector has been status of error;
	unsigned short		intime; //invalid time;
	struct timeval		stime;
}detectorpro_t;

typedef struct phasedetect_t
{
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
    int             *pendpipe;
    Detector_t      *detector;
}monpendphase_t;


#ifdef	__cplusplus
}
#endif

#endif	/* RCC_H */

