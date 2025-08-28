/* 
 * File:   cd.h
 * Author: sk
 *
 * Created on 2016年2月18日, 下午4:00
 */

#ifndef CD_H
#define	CD_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../../Inc/mydef.h"
typedef struct cdpinfo_t
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
	unsigned char			fixgtime; //green time of fix phase
	unsigned char			pgtime;//person green time
    unsigned char           pctime;//person clear time
    unsigned char           cpcexist;//whether person channel exist in change channels;
    unsigned char           chan[MAX_CHANNEL]; //all channels 
    unsigned char           cchan[MAX_CHANNEL];//all change channels;
    unsigned char           cnpchan[MAX_CHANNEL];//change channels not include person channels
    unsigned char           cpchan[MAX_CHANNEL]; //change person channels
}cdpinfo_t;

typedef struct cddata_t
{
	fcdata_t			*fd;
	tscdata_t			*td;
	cdpinfo_t			*pi;
	ChannelStatus_t		*cs;		
}cddata_t;

typedef struct cdpephase_t
{
	unsigned char		phaseid;//pending phase id;
	unsigned char		mark; //'1' means have vehicle request, '0' means not have vehicle request;
}cdpephase_t;//pending phase

typedef struct cdpcdata_t
{
	unsigned char		*pchan;
	unsigned char		time;
	unsigned short		*markbit;
	int					*bbserial;
	sockfd_t			*sockfd;
	ChannelStatus_t		*cs;
}cdpcdata_t;

typedef struct cddetectorpro_t
{
	unsigned char		deteid;
	unsigned char		validmark; //the detector is invalid at intial stage;
	unsigned char		err; //err is 1 mean that the detector has been status of error;
	unsigned short		intime; //invalid time;
	struct timeval		stime;
}cddetectorpro_t;

typedef struct cdphasedetect_t
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
	cddetectorpro_t		detectpro[10];
}cdphasedetect_t;

typedef struct cdmonpendphase_t
{
	int				*pendpipe;
	Detector_t		*detector;	
}cdmonpendphase_t;

#ifdef RED_FLASH
typedef struct redflash_cd
{
    unsigned char       tcline;
    unsigned char       slnum;
    unsigned char       snum;
	unsigned char		rft;
    unsigned char       *chan;
	cddata_t			*cd;
}redflash_cd; 

void cd_red_flash(void *arg);
#endif
#ifdef	__cplusplus
}
#endif

#endif	/* CD_H */

