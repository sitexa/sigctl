/* 
 * File:   yft.h
 * Author: sk
 *
 * Created on 20140218
 */

#ifndef YFT_H
#define	YFT_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../../Inc/mydef.h"

typedef struct yftpinfo_t
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
}yftpinfo_t;

typedef struct yftdata_t
{
	fcdata_t			*fd;
	tscdata_t			*td;
	yftpinfo_t			*pi;
	ChannelStatus_t		*cs;		
}yftdata_t;

typedef struct pyfdata_t
{
	unsigned char		*chan;
	unsigned short		*markbit;
	int					*bbserial;
	sockfd_t			*sockfd;
	ChannelStatus_t		*cs;
}pyfdata_t;

typedef struct yftpcdata_t
{
	unsigned char		*pchan;
	unsigned char		time;
	unsigned short		*markbit;
	int					*bbserial;
	sockfd_t			*sockfd;
	ChannelStatus_t		*cs;
}yftpcdata_t;

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

#endif	/* YFT_H */

