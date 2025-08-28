/* 
 * File:   ppss.h
 * Author: root
 *
 * Created on 2013年12月5日, 下午6:05
 */

#ifndef PPSS_H
#define	PPSS_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../../Inc/mydef.h"

typedef struct ppspinfo_t
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
}ppspinfo_t;

typedef struct ppsdata_t
{
	fcdata_t			*fd;
	tscdata_t			*td;
	ppspinfo_t			*pi;
	ChannelStatus_t		*cs;		
}ppsdata_t;


typedef struct ppspcdata_t
{
	unsigned char		*pchan;
	unsigned char		time;
	unsigned short		*markbit;
	int					*bbserial;
	sockfd_t			*sockfd;
	ChannelStatus_t		*cs;
}ppspcdata_t;

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
    
    unsigned char		downt;
	unsigned int		chans; 
	unsigned char		indetect[10];//fact invalid detector
	detectorpro_t		detectpro[10];
}phasedetect_t;

typedef struct monpendphase_t
{
    int             *pendpipe;
    Detector_t      *detector;
}monpendphase_t; //need the struct data when prepare to degrade

#ifdef RED_FLASH
typedef struct redflash_pps
{
    unsigned char       tcline;
    unsigned char       slnum;
    unsigned char       snum;
	unsigned char		rft;
    unsigned char       *chan;
	ppsdata_t			*pps;
}redflash_pps; 

void pps_red_flash(void *arg);
#endif


typedef struct cyellowflash_t
{
    unsigned char       cyft;
    unsigned char       mark;
    unsigned char       *chan;
    ppsdata_t            *pps;
}cyellowflash_t;
void pps_channel_yellow_flash(void *arg);
#ifdef	__cplusplus
}
#endif

#endif	/* PPSS_H */

