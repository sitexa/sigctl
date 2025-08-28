/* 
 * File:   buss.h
 * Author: root
 *
 * Created on 2013年12月17日, 下午6:49
 */

#ifndef BPS_H
#define	BPS_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../../Inc/mydef.h"

typedef struct buspinfo_t
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
    unsigned char           chan[MAX_CHANNEL]; //all channels 
    unsigned char           cchan[MAX_CHANNEL];//all change channels;
    unsigned char           cnpchan[MAX_CHANNEL];//change channels not include person channels
    unsigned char           cpchan[MAX_CHANNEL]; //change person channels
}buspinfo_t;

typedef struct busdata_t
{
	fcdata_t			*fd;
	tscdata_t			*td;
	buspinfo_t			*pi;
	ChannelStatus_t		*cs;		
}busdata_t;


typedef struct buspcdata_t
{
	unsigned char		*pchan;
	unsigned char		time;
	unsigned short		*markbit;
	int					*bbserial;
	sockfd_t			*sockfd;
	ChannelStatus_t		*cs;
}buspcdata_t;

typedef struct detectorpro_t
{
	unsigned char		deteid;
	unsigned char		detetype;
//	unsigned char		validmark; //the detector is invalid at intial stage;
//	unsigned char		err; //err is 1 mean that the detector has been status of error;
//	unsigned short		intime; //invalid time;
	unsigned char		validtime;
	unsigned char		request; //'1' means the detector has request,on the contrary, it is '0'
	struct timeval		bstime;
//	struct timeval		stime;
}detectorpro_t;

typedef struct phasedetect_t
{
	unsigned char		phaseid;
	unsigned char		phasetype;
     
     unsigned char		stageid;
    unsigned char		fixgtime;
	unsigned char		gtime; //not include green flash time;
	unsigned char		gftime;
	unsigned char		ytime;
	unsigned char		rtime;
	unsigned char       mingtime;
    unsigned char       maxgtime;
	unsigned char		pgtime;
	unsigned char		pctime; 
    unsigned int		chans; 
//	unsigned char		indetenum; //invalid detector arrive the number,begin to degrade;
//	unsigned char		validmark; //the phase is valid at intial stage;
//	unsigned char		factnum; //fact the number of invalid detector;
//	unsigned char		indetect[10];//fact invalid detector
	detectorpro_t		detectpro[10];
}phasedetect_t;

typedef struct monpendphase_t
{
    int             *pendpipe;
    Detector_t      *detector;
}monpendphase_t; //need the struct data when prepare to degrade

typedef struct busdetector_t
{
	unsigned char			detectid;
	unsigned int			phaseid; //bring value according to bit;
	unsigned char			request;
	unsigned char			validtime;
	struct timeval			*stime;
}busdetector_t;

#ifdef RED_FLASH
typedef struct redflash_bus
{
    unsigned char       tcline;
    unsigned char       slnum;
    unsigned char       snum;
	unsigned char		rft;
    unsigned char       *chan;
	busdata_t			*bd;
}redflash_bus; 

void bus_red_flash(void *arg);
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* BPS_H */

