/* 
 * File:   soas.h
 * Author: sk
 *
 * Created on 2015-06-05
 */

#ifndef SOA_H
#define	SOA_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../../Inc/mydef.h"

typedef struct soapinfo_t
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
}soapinfo_t;

typedef struct soadata_t
{
    fcdata_t                *fd;
    tscdata_t               *td;
    ChannelStatus_t         *cs;
	soapinfo_t				*pi;
}soadata_t;

typedef struct soapcdata_t
{
	unsigned char			*pchan;
	unsigned char           time;
	unsigned short			*markbit;
	int                     *bbserial;
	sockfd_t				*sockfd;
	ChannelStatus_t			*cs;
}soapcdata_t;

typedef struct soadetectorpro_t
{
    unsigned char       	deteid;

	unsigned char			notfirstdata;
	unsigned short			icarnum;//the car number of coming into detector;
	unsigned short			ocarnum;//the car number of leaving detector;
	unsigned short			fullflow;//full flow of key roadway;
	unsigned long			maxocctime;
	unsigned int			aveocctime;
	double					fulldegree;

    unsigned char       	validmark; //the detector is invalid at intial stage;
    unsigned char       	err; //err is 1 mean that the detector has been status of error;
    unsigned short      	intime; //invalid time;
	//added by sk on 20150721
	unsigned char			redflow;		
	//end

    struct timeval      	stime;
}soadetectorpro_t;

typedef struct soaphasedetect_t
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
	unsigned char       curpt;
	//added by sk on 20150721
	unsigned char		gtbak; //bak green time;
	unsigned char		pt;//phase time;
	unsigned char		optipt;//optimize phase time
	unsigned short		kf;//key road flow;
	unsigned short		pf;//phase flow;
	unsigned short		kff;//key road full flow;
	unsigned int		maot;//max ave occoate time;
	double				pfd;//phase full degree;
	unsigned char		kd;//key detector;
	double				kfr;//key flow ratio
	unsigned char		acs;//ave car speed
	double				acd;//ave car delay
	double				sct;//stop car time
	double				occrate;//occupy rate
	double				chw;//car head way
	unsigned short		pc;//pass capacity;
	//end add

    soadetectorpro_t    soadetectpro[10];
}soaphasedetect_t;

typedef struct soamonpendphase_t
{
    int             *pendpipe;
    Detector_t      *detector;
}soamonpendphase_t;


typedef struct soaPhaseMaxData_t
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
}soaPhaseMaxData_t;


#ifdef RED_FLASH
typedef struct redflash_soa
{
    unsigned char       tcline;
    unsigned char       slnum;
    unsigned char       snum;
	unsigned char		rft;
    unsigned char       *chan;
	soadata_t			*soa;
}redflash_soa; 

void soa_red_flash(void *arg);
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* SOA_H */

