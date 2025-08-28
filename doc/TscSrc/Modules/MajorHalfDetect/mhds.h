/* 
 * File:   mhds.h
 * Author: sk
 *
 * Created on 2013年11月25日, 下午2:41
 */

#ifndef MHDS_H
#define	MHDS_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../../Inc/mydef.h"

typedef struct mahpinfo_t
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
}mahpinfo_t;

typedef struct mahdata_t
{
	fcdata_t			*fd;
	tscdata_t			*td;
	mahpinfo_t			*pi;
	ChannelStatus_t		*cs;		
}mahdata_t;

typedef struct mahpcdata_t
{
	unsigned char		*pchan;
	unsigned char		time;
	unsigned short		*markbit;
	int					*bbserial;
	sockfd_t			*sockfd;
	ChannelStatus_t		*cs;
}mahpcdata_t;

typedef struct detectorpro_t
{
    unsigned char       deteid;
    unsigned char       validmark; //the detector is invalid at intial stage;
    unsigned char       err; //err is 1 mean that the detector has been status of error;
    unsigned short      intime; //invalid time;
    struct timeval      stime;
}detectorpro_t;

typedef struct phasedetect_t
{
    unsigned char       phaseid;
    unsigned char       phasetype;
    unsigned char       indetenum; //invalid detector arrive the number,begin to degrade;
    unsigned char       validmark; //the phase is valid at intial stage;
    unsigned char       factnum; //fact the number of invalid detector;
     
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
    unsigned char       indetect[10];//fact invalid detector
    detectorpro_t       detectpro[10];
}phasedetect_t;

typedef struct monpendphase_t
{
    int             *pendpipe;
    Detector_t      *detector;
}monpendphase_t;

#ifdef RED_FLASH
typedef struct redflash_mah
{
    unsigned char       tcline;
    unsigned char       slnum;
    unsigned char       snum;
	unsigned char		rft;
    unsigned char       *chan;
	mahdata_t			*mah;
}redflash_mah; 

void mah_red_flash(void *arg);
#endif

typedef struct cyellowflash_t
{
    unsigned char       cyft;
    unsigned char       mark;
    unsigned char       *chan;
    mahdata_t           *mah;
}cyellowflash_t;
void mah_channel_yellow_flash(void *arg);

#ifdef	__cplusplus
}
#endif

#endif	/* MHDS_H */

