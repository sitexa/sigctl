/* 
 * File:   bsa.h
 * Author: root
 *
 * Created on 2017年11月6日, 下午6:17
 */

#ifndef BSA_H
#define	BSA_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../../Inc/mydef.h"

typedef struct bsapinfo_t
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
	unsigned char			ldeti;//last detect time
	unsigned char			fixgtime; //green time of fix phase
	unsigned char			pgtime;//person green time
    unsigned char           pctime;//person clear time
    unsigned char           cpcexist;//whether person channel exist in change channels;
    unsigned char           chan[MAX_CHANNEL]; //all channels 
    unsigned char           cchan[MAX_CHANNEL];//all change channels;
    unsigned char           cnpchan[MAX_CHANNEL];//change channels not include person channels
    unsigned char           cpchan[MAX_CHANNEL]; //change person channels
}bsapinfo_t;
/*
typedef struct bsapinfo_t
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
}bsapinfo_t;
*/
typedef struct bsadata_t
{
    fcdata_t                *fd;
    tscdata_t               *td;
    ChannelStatus_t         *cs;
	bsapinfo_t				*pi;
}bsadata_t;

typedef struct bsapcdata_t
{
	unsigned char			*pchan;
	unsigned char           time;
	unsigned short			*markbit;
	int                     *bbserial;
	sockfd_t				*sockfd;
	ChannelStatus_t			*cs;
}bsapcdata_t;

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
	unsigned char		pgtime;
	unsigned char		pctime;

	unsigned char		mquel;//相位中最大的车道排队长度
	unsigned char		mquen;//相位中最大的车道排队数量
	unsigned char		dn;

	unsigned char		detectn[10];
	unsigned int		chans;
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

typedef struct HERadarErrList_t
{
    unsigned char           RoadId;
    unsigned char           RoadStatus;//0:normal,1:err
    unsigned char           ErrType;//0:unknown,1:front end err;2:undervoltage;3:high temperature;
}HERadarErrList_t;
typedef struct HERadarErr_t
{
    unsigned char           RoadNum;
    HERadarErrList_t        HERadarErrList[ROADN];
}HERadarErr_t;


#ifdef RED_FLASH
typedef struct redflash_bsa
{
    unsigned char       tcline;
    unsigned char       slnum;
    unsigned char       snum;
	unsigned char		rft;
    unsigned char       *chan;
	bsadata_t			*bsa;
}redflash_bsa; 

void bsa_red_flash(void *arg);
#endif

typedef struct BsaPhaseInfo_t
{
    unsigned char           pid;
	unsigned char			quel;
	unsigned char			quen;
}BsaPhaseInfo_t;

typedef struct cyellowflash_t
{
    unsigned char       cyft;
    unsigned char       mark;
    unsigned char       *chan;
    bsadata_t			*bsa;
}cyellowflash_t;

void bsa_channel_yellow_flash(void *arg);

#ifdef	__cplusplus
}
#endif

#endif	/* BSA_H */

