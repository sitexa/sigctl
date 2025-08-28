/* 
 * File:   igc.h
 * Author: root
 *
 * Created on 2022年7月9日, 上午1:25
 */

#ifndef IGC_H
#define	IGC_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../../Inc/mydef.h"
typedef struct InterGrateControlPhaseInfo_t
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
}InterGrateControlPhaseInfo_t;

typedef struct InterGrateControlData_t
{
	fcdata_t								*fd;
	tscdata_t								*td;
	InterGrateControlPhaseInfo_t			*pi;
	ChannelStatus_t		   					*cs;		
}InterGrateControlData_t;



typedef struct DetectOrPro_t
{
	unsigned char		deteid;
	unsigned char		validmark; //the detector is invalid at intial stage;
	unsigned char		err; //err is 1 mean that the detector has been status of error;
	unsigned short		intime; //invalid time;
	struct timeval		stime;
}DetectOrPro_t;

typedef struct PhaseDetect_t
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
	DetectOrPro_t		detectpro[10];
}PhaseDetect_t;


int igc_get_phase_info(fcdata_t *fd,tscdata_t *td,unsigned char tcline,unsigned char slnum,InterGrateControlPhaseInfo_t *pinfo);
int igc_get_timeconfig(tscdata_t *td,unsigned char *pattern,unsigned char *tcline);
int igc_set_lamp_color(int serial,unsigned char *chan,unsigned char color);
int igc_backup_pattern_data(int serial,unsigned char snum,PhaseDetect_t *phd);
int igc_dirch_control(markdata_c *mdt,unsigned char cktem,unsigned char ktem,unsigned char (*dirch)[8],InterGrateControlPhaseInfo_t *pinfo);
int igc_jump_stage_control(markdata_c *mdt,unsigned char staid,InterGrateControlPhaseInfo_t *pinfo);
int igc_mobile_jp_control(markdata_c *mdt,unsigned char staid,InterGrateControlPhaseInfo_t *pinfo,unsigned char *dirch);
int igc_mobile_direct_control(markdata_c *mdt,InterGrateControlPhaseInfo_t *pinfo,unsigned char *dirch,unsigned char *fdirch);
void igc_channel_yellow_flash(InterGrateControlData_t *stInterGrateControlData);



#ifdef	__cplusplus
}
#endif

#endif	/* IGC_H */

