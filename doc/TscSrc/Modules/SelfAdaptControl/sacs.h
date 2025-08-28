/* 
 * File:   sacs.h
 * Author: sk
 *
 * Created on 20140109
 */

#ifndef SACS_H
#define	SACS_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../../Inc/mydef.h"

typedef struct sapinfo_t
{
	unsigned char			stageline;
    unsigned char           phaseid;
	unsigned char			fcgtime;//fix cycle green time
    unsigned char           gtime;
	unsigned char			gftime;
	unsigned char			ytime;
	unsigned char			rtime;
	unsigned char           phasetype;
    unsigned char           mingtime;
    unsigned char           maxgtime1;
	unsigned char           fixgtime; //green time of fix phase
    unsigned char           pgtime;//person green time
    unsigned char           pctime;//person clear time
	unsigned char			cpcexist;//whether person channel exist in change channels;
	unsigned char			chan[MAX_CHANNEL]; //all channels 
	unsigned char			cchan[MAX_CHANNEL];//all change channels;
	unsigned char			cnpchan[MAX_CHANNEL];//change channels not include person channels
	unsigned char			cpchan[MAX_CHANNEL]; //change person channels
}sapinfo_t;

typedef struct sadata_t
{
    fcdata_t                *fd;
    tscdata_t               *td;
    ChannelStatus_t         *cs;
	sapinfo_t				*pi;
}sadata_t;

typedef struct sapcdata_t
{
	unsigned char			*pchan;
	unsigned char           time;
	unsigned short			*markbit;
	int                     *bbserial;
	sockfd_t				*sockfd;
	ChannelStatus_t			*cs;
}sapcdata_t;

typedef struct detectorpro_t
{
    unsigned char       	deteid;

	unsigned char			notfirstdata;
	unsigned short			icarnum;//the car number of coming into detector;
	unsigned short			ocarnum;//the car number of leaving detector;
	unsigned short			fullflow;//full flow of key roadway;
	unsigned long			maxocctime;
	unsigned int			aveocctime;
	float					fulldegree;

    unsigned char       	validmark; //the detector is invalid at intial stage;
    unsigned char       	err; //err is 1 mean that the detector has been status of error;
    unsigned short      	intime; //invalid time;
    struct timeval      	stime;
}detectorpro_t;

typedef struct phasedetect_t
{
	unsigned char		stageid;
    unsigned char       phaseid;
    unsigned char       phasetype;

	unsigned char		fixgtime;
	unsigned char		fcgtime;//fix cycle green time
	unsigned char		gtime; //not include green flash time;
	unsigned char		gftime;
	unsigned char		ytime;
	unsigned char		rtime;
	unsigned char       mingtime;
    unsigned char       maxgtime;
	unsigned char		vgtime;
	unsigned char		vmingtime;
	unsigned char		vmaxgtime;
	unsigned char		pgtime;
	unsigned char		pctime;
	unsigned char		startdelay;

    unsigned char       indetenum; //invalid detector arrive the number,begin to degrade;
    unsigned char       validmark; //the phase is valid at intial stage;
    unsigned char       factnum; //fact the number of invalid detector;
    unsigned char       indetect[10];//fact invalid detector
	
	unsigned char		downt;
	unsigned int		chans;

    detectorpro_t       detectpro[10];
}phasedetect_t;

typedef struct monpendphase_t
{
    int             *pendpipe;
    Detector_t      *detector;
}monpendphase_t;


typedef struct PhaseMaxData_t
{
    unsigned char       phaseid;
	unsigned char		gtime;
    unsigned char       vgtime; // phase valid green time;
    unsigned char       vmingtime;
    unsigned char       vmaxgtime;
    float           	maxfulldegree; 
    unsigned short      maxcarflow;
    unsigned short      maxfullflow;
    unsigned short      maxaveocctime;
}PhaseMaxData_t;

#ifdef RED_FLASH
typedef struct redflash_sa
{
    unsigned char       tcline;
    unsigned char       slnum;
    unsigned char       snum;
	unsigned char		rft;
    unsigned char       *chan;
	sadata_t			*sa;
}redflash_sa; 

void sa_red_flash(void *arg);
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* SACS_H */

