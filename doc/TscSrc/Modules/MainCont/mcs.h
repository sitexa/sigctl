/* 
 * File:   mcs.h
 * Author: sk
 *
 * Created on 2013年6月1日, 上午10:30
 */

#ifndef MCS_H
#define	MCS_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../../Inc/mydef.h"

typedef struct DeteId_t
{
	unsigned char		deteid;
	unsigned char		err;
	unsigned char		ereport; //err report
	unsigned short		intime;
	struct timeval		stime;	
}DeteId_t;


typedef struct PatternList1_t
{
	unsigned char  		PatternId1;
	unsigned short 		CycleTime1;
	unsigned char  		PhaseOffset1;
	unsigned char  		CoordPhase1;
	unsigned char  		TimeConfigId1;
} PatternList1_t;

typedef struct Pattern1_t
{
	unsigned char 		FactPatternNum1;
	PatternList1_t 		PatternList1[MAX_PATTERN_LINE];
} Pattern1_t;

typedef struct lhdata_t
{
	int					*lhsocked;
	unsigned char		*newip;
	unsigned char		*neterr;
}lhdata_t;


typedef struct ClientSocketInfo_t
{
	unsigned char     CreatePthreadYes;
	unsigned char     Close_Flag;
	pthread_t 	      ClientPthreadId;
	int	  		      ClientSocketId;
}ClientSocketInfo_t;

typedef struct DevCode_st
{
	unsigned char	  strlen;
	unsigned char     cydevcode[256];
}DevCode_st;

int mc_open_serial_port(int *serial);
int mc_set_serial_port(int *serial);
int mc_close_serial_port(int *serial);
int mc_combine_str(unsigned char *dstr, unsigned char *head, int datalen, unsigned char *stream, int slen, unsigned char *end);
int netdata_combine(unsigned char *dstr,int nlen,unsigned char *stream);
int start_localhost_client(lhdata_t *lhdata);
int mc_close_lamp(fcdata_t *fd,ChannelStatus_t *cs);



#ifdef	__cplusplus
}
#endif

#endif	/* MCS_H */

