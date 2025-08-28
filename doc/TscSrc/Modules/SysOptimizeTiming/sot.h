/* 
 * File:   sot.h
 * Author: root
 *
 * Created on 2015年9月6日, 下午6:56
 */

#ifndef SOT_H
#define	SOT_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../../Inc/mydef.h"

typedef struct sotpinfo_t
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
}sotpinfo_t;

typedef struct sotdata_t
{
    fcdata_t                *fd;
    tscdata_t               *td;
    ChannelStatus_t         *cs;
	sotpinfo_t				*pi;
}sotdata_t;

typedef struct sotpcdata_t
{
	unsigned char			*pchan;
	unsigned char           time;
	unsigned short			*markbit;
	int                     *bbserial;
	sockfd_t				*sockfd;
	ChannelStatus_t			*cs;
}sotpcdata_t;

typedef struct sotdetectorpro_t
{
    unsigned char       	deteid;

	unsigned char			notfirstdata;
	unsigned short			icarnum;//the car number of coming into detector;
	unsigned short			ocarnum;//the car number of leaving detector;
	unsigned short			fullflow;//full flow of key roadway;
	unsigned long			maxocctime;
	unsigned int			aveocctime;
	double					fulldegree;

	//added by sk on 20150721
	unsigned char			redflow;		
	//end
}sotdetectorpro_t;

typedef struct sotphasedetect_t
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
	unsigned char       vgtime;
    unsigned char       vmingtime;
    unsigned char       vmaxgtime;
	unsigned char		pgtime;
	unsigned char		pctime;
	unsigned char       startdelay;

	unsigned char		downt;
	unsigned int		chans;
	unsigned char       syscoofixgreen;
	unsigned char		curpt;//current phase time
	//added by sk on 20150721
	unsigned char		gtbak; //bak green time;
	unsigned char		pt;//phase time;
	unsigned char		optipt;
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
    sotdetectorpro_t    sotdetectpro[10];
	//end add
}sotphasedetect_t;

typedef struct sotmonpendphase_t
{
    int             *pendpipe;
    Detector_t      *detector;
}sotmonpendphase_t;

#ifdef RED_FLASH
typedef struct redflash_sot
{
    unsigned char       tcline;
    unsigned char       slnum;
    unsigned char       snum;
	unsigned char		rft;
    unsigned char       *chan;
	sotdata_t			*sot;
}redflash_sot; 

void sot_red_flash(void *arg);
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* SOT_H */

