/* 
 * File:   mydef.h
 * Author: sk
 *
 * Created on 20130622
 */

#ifndef _MYDEF_H
#define _MYDEF_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netdb.h>
#include <termios.h>
#include <sys/socket.h>
#include <pthread.h>
#include <linux/watchdog.h>
#include <math.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <net/if.h>       /* for ifconf */
#include <linux/sockios.h>    /* for net status mask */
#include <netinet/in.h>       /* for sockaddr_in */
#include "cy_tsc.h"


//#define COMMON_DEBUG
#define MAIN_DEBUG
#define TIMING_DEBUG
#define FULL_DETECT_DEBUG
#define SAME_DEBUG
#define MAJOR_DEBUG
#define PERSON_DEBUG
#define MINOR_DEBUG
#define BUS_DEBUG
#define SELF_ADAPT_DEBUG
#define GT_DEBUG
#define YFT_DEBUG
#define SYSTEM_OPTIMIZE_ADAPT_DEBUG
#define SYSTEM_OPTIMIZE_COORD_DEBUG
#define SYSTEM_OPTIMIZE_TIMING_DEBUG
#define COORD_DETECT_DEBUG
#define RCC_DEBUG

#define ROADINFO_DEBUG
#define CRC_DEBUG
#define NTP_TIME
#define SERVER_CREATE
#define FLASH_DEBUG

#define OC_DEBUG
#define TIME_OUT 

#define YFCHANNEL 32
#define CHANNEL_YELLOW_FLASH
//#define CHANNEL_CLOSE
//#define MINGREEN_DELAY

//#define GPS
//#define SELF_DOWN_TIME
//#define TIMING_DOWN_TIME
//#define FULL_DOWN_TIME

//#define EMBED_CONFIGURE_TOOL

#define BASE_BOARD_CONTROL
#define WUXI_CHECK
#define RED_FLASH
//#define CLOSE_LAMP
//#define STANDARD_NEW
#define VEHICLE_ROAD_COORD
#define BRAIN_SELF_ADAPT_DEBUG

//#define V2X_DEBUG

#define SUCCESS 		0
#define FAIL			-1
#define MEMERR			-2
#define FAIL2			-3
#define CY_TSC       	"./data/cy_tsc.dat"
#define SINGALER_MACHINE_SERVER_PORT 12810
#define SINGALER_MACHINE_SERVER_PORT_ONE 12812

#define UDP_SERVER 12811
#define IPFILE			"/mnt/nandflash/userinfo.txt"
#define FLOWLOG			"./data/flow.dat"
#define EVENTLOG		"./data/event.txt"
#define EVENTDAT		"./data/event.dat"
#define INTERVALTIME  	15
#define DEFAULTBYTE 	32
#define IPPORT_FILE		"./data/ipport.txt"
#define V2XPORT_FILE	"./data/v2xport.txt"
#define GDV2XPORT_FILE	"./data/gdv2xport.txt"
#define CONFIG_FILE		"./data/config.txt"
#define BUS_CONFIG		"./data/busc.txt"
#define MACHINE_VER		"./data/ver.dat"
#define DEV_CODE		"./data/dev.dat"
#ifdef NTP_TIME
#define NTPPORT_FILE		"./data/ntpport.txt"
#endif
#define MAX(a,b) ((a)>(b))?(a):(b)
#define BUSN		1000
#define RFIDT		60
#define CHUFA		10
#define CY_BUS			"./data/cy_bus.dat"
#define ROADN  		88//48
#define DETN		10
#define BSAL1		30
#define BSAL2		80
#define	HEADWAY		2.3 
#define CARLEN		5
#define BLINDLEN 	30  //盲区长度
#define PIE                      3.1415926
#define EARTH_RADIUS            6378.137
#define PERSONTIME			2
//#define IPFILE "./userinfo.txt"
typedef struct sockfd_t
{
	int			*ssockfd;
	int 		*csockfd;
	int			*lhsockfd;
	int			*n_csockfd; 
	int         *edgersockd;
}sockfd_t;

typedef struct timedown_t
{
	unsigned char       mode;
	unsigned char       pattern;
	unsigned char       lampcolor;
	unsigned char       lamptime;
	unsigned char       phaseid;
	unsigned char       stageline;
}timedown_t;

typedef struct statusinfo_t
{
	unsigned char		conmode;
	unsigned char		pattern;
	unsigned char		color;
	unsigned char		time;
	unsigned char		stage;
	unsigned char		chans;
	unsigned short      cyclet;
	unsigned int		phase;
	unsigned char		csta[MAX_CHANNEL];
}statusinfo_t;

//V2X
typedef struct SignalLightGroup_t
{
	unsigned char			slgid;
	unsigned char			slgstatus;
	unsigned char			countdown;
	unsigned char			nslgstatus;
	//高德需求
	unsigned char			greent;
	unsigned char			yellowt;
	unsigned char			redt;
	unsigned char			allrt;
	//end
}SignalLightGroup_t;

typedef struct SignalLightData_t
{
	unsigned char				verid;
	unsigned char				type;
	unsigned char				objid;
	unsigned short				cityid;
	unsigned int				areaid;
	unsigned short				roadid;
	unsigned char				roadstatus;
	unsigned int				commtime;
	unsigned char				slgnum;
	SignalLightGroup_t			slg[MAX_CHANNEL];
}SignalLightData_t;


typedef struct fcdata_t
{
//	unsigned char		*shour;
//	unsigned char		*smin;
//	unsigned char		*coordtime;
//	unsigned char		*poffset;
	unsigned char		*lip;
	unsigned char		*coordphase;

//	unsigned char		*nshour;
//	unsigned char		*nsmin;
//	unsigned char		*ncoordtime;
//	unsigned char		*npoffset;
	unsigned char		*ncoordphase;	

	unsigned char		*contmode;
	unsigned char		*ncontmode;
	unsigned char		*auxfunc;
	unsigned char		*reflags; //added on 20150818
	unsigned char		*halfdownt;
	unsigned char		*specfunc;
	unsigned char       *patternid;
	unsigned char		*slnum;
	unsigned char		*stageid;
	unsigned short		*markbit;
	unsigned short		*markbit2;
	unsigned short		*cyclet;
//	unsigned short		*ncyclet;

	unsigned int		*phaseid; //child module return phase id
	unsigned char		*color;
	unsigned char		*wlmark;
	int					*flowpipe;
	int					*pendpipe;
	int					*conpipe;
	int					*synpipe;//child module tell main module its end time according to the pipe;
	int					*endpipe;
	int					*ffpipe;
	int					*bbserial;//base board serial;
	int					*fbserial;//face board serial;
	int					*sdserial;//side door serial;
	int					*vhserial;//vehicle board serial;

	int					*roadinfo;
	int                 *roadinforeceivetime;
	int                 *phasedegrphid;
	long				st;
	long				nst;

	sockfd_t            *sockfd;
	EventClass_t		*ec;
	EventLog_t			*el;
	statusinfo_t		*sinfo;
	unsigned char		**softevent;//added by shikang on 20171110
	unsigned int		*ccontrol;
	unsigned int		*fcontrol;
	unsigned char		*trans;

	unsigned char		*v2xmark; //V2x
	SignalLightGroup_t	*slg; //V2X
}fcdata_t;

typedef struct tscdata_t
{
	TSCHeader_t				*tscheader;
	Unit_t					*unit;	
	Schedule_t				*schedule;
	TimeSection_t			*timesection;

	Pattern_t				*pattern;
	TimeConfig_t			*timeconfig;
	Phase_t					*phase;
	PhaseError_t			*phaseerror;
	Channel_t				*channel;
	ChannelHint_t			*channelhint;
	Detector_t				*detector;

//	DetectorData_t			*detectdata;
//	EventClass_t			*eventclass;
//	EventLog_t				*eventlog;
}tscdata_t;

typedef struct yfdata_t
{
	unsigned char			second;
	//unsigned char           bcrcyellow;
	unsigned short			*markbit;
	unsigned short			*markbit2;
	int						serial;
	//int                     synpipe;
	sockfd_t				*sockfd;
	ChannelStatus_t 		*cs;
}yfdata_t;

typedef struct infotype_t
{
	unsigned char		highbit;
	unsigned char		objectn;
	unsigned char		opertype;	
}infotype_t;

#define MAX_VALUE_SIZE 1024 
typedef struct objectinfo_t
{
	unsigned char		objectid;
	unsigned char		objects;
	unsigned char		indexn;
	unsigned char		cobject;
	unsigned char		cobjects;
	unsigned char		index[3];
	unsigned short		objectvs;
	unsigned char		objectv[MAX_VALUE_SIZE];
}objectinfo_t;

typedef struct object_t
{
	unsigned char		obid;//object id
	unsigned char		obsize;//object size
	unsigned char		fieldn;// total field num of object
	unsigned char		field1s;//firstly field size
	unsigned char		field2s;//secondly field size
	unsigned char		field3s;//thirdly field size
	unsigned char		field4s;
	unsigned char		field5s;
	unsigned char		field6s;
	unsigned char		field7s;
	unsigned char		field8s;
	unsigned char		field9s;
	unsigned char		field10s;
	unsigned char		field11s;
	unsigned char		field12s;//12th field size	
}object_t;


typedef struct detectinfo_t
{
	unsigned char		*pattern;
	unsigned char		*stage;
	unsigned char		*color;
	unsigned char		detectid;
	unsigned int		time;
	unsigned int		*phase;	
}detectinfo_t;

typedef struct versioninfo_t
{
	unsigned char		majorid;
	unsigned char		minorid1;
	unsigned char		minorid2;
	unsigned char		year;
	unsigned char		month;
	unsigned char		day;
}versioninfo_t;

//the following struct is used to control of self adapt
typedef struct drivewayflow_t
{
	unsigned char		driveway;
	unsigned short		flow;
}drivewayflow_t; 

typedef struct phasedegree_t
{
	unsigned char		phaseid;
	unsigned char		phaset;
	unsigned short		pdegree;
	drivewayflow_t		dwf[10];
}phasedegree_t;

typedef struct commoninfo_t
{
	unsigned char		pattern;
	unsigned short		cyclet;
	unsigned short		msgs;
	unsigned int		stime;
	unsigned int		etime;
}commoninfo_t;

typedef struct terminalpp_t
{
	unsigned char		head;
	unsigned char		size;
	unsigned char		id;
	unsigned char		type;
	unsigned char		sta; //'1' means has been connected,on the contrary is '0'
	unsigned char		func[4];
	unsigned char		end;
}terminalpp_t; 

//added by sk on 20150721
typedef struct optidata_t
{
	unsigned char			patternid;
	unsigned char			phasenum;
	unsigned char			*reflags;
	unsigned short			cycle;
	unsigned short			opticycle;
	unsigned short			*markbit;
	unsigned short			*markbit2;
}optidata_t;

typedef struct markdata_t
{
    unsigned char       *redl;
    unsigned char       *closel;
    unsigned char       *rettl;
	unsigned char		*dircon;//wether direction control,1 means happen;
	unsigned char		*firdc;//first direct control;
	unsigned char		*kstep;
    unsigned int       	*yfl;
    pthread_t           *yfid;
    yfdata_t            *ardata;
    fcdata_t            *fcdata;
    tscdata_t           *tscdata;
    ChannelStatus_t     *chanstatus;
	statusinfo_t		*sinfo;
}markdata_t;
//end add

typedef struct markdata_c
{
    unsigned char       *redl;
    unsigned char       *closel;
    unsigned char       *rettl;
	unsigned char       *dircon;//wehther direction control,1 means happen;
    unsigned char       *firdc;//first direct control;
    unsigned char       *yfl;
	unsigned char		*kstep;
    pthread_t           *yfid;
    yfdata_t            *ardata;
    fcdata_t            *fcdata;
    tscdata_t           *tscdata;
    ChannelStatus_t     *chanstatus;
    statusinfo_t        *sinfo;
}markdata_c;

typedef struct bus_pri_t
{
    unsigned short      busid;
    unsigned short      rwid;
    unsigned short      dectid;
}bus_pri_t;


typedef struct standard_data
{
	unsigned char			Head;
	unsigned char			VerId;
	unsigned char			SendMark;
	unsigned char			RecMark;
	unsigned char			DataLink;
	unsigned char			AreaId;
	unsigned short          CrossId;
	unsigned char			OperType;
	unsigned char			ObjectId;
	unsigned char			Reserve[5];
	unsigned char			Data[1024];
//	unsigned char			CheckCode;
	unsigned char			End;
}standard_data_t;


typedef struct BusPhaseList_t
{
	unsigned char			chanid;
	unsigned char			validt;
	unsigned char			validl;
	unsigned char			udlink;
	unsigned char			rev;
	unsigned short			busn;
}BusPhaseList_t;
typedef struct BusList_t
{
	unsigned short			RoadId;
	unsigned short			FactBusPhaseNum;
	unsigned int			LngInteg;
	unsigned int			LngDeci;
	unsigned int			LatInteg;
	unsigned int			LatDeci;
	BusPhaseList_t			busphaselist[BUSN];
}BusList_t;

typedef struct RecSenData_t
{
	unsigned char			Head;
	unsigned char			Len;
	unsigned char			Func;
	unsigned short			Busn;
	unsigned char			Udlink;
	unsigned char			ChePaiN[13];
	unsigned char			Err;
	unsigned char			End;
}RecSenData_t;

typedef struct SenData1_t
{
	unsigned char			Head;
	unsigned char			Len;
	unsigned char			Func;
	unsigned char			ChePaiN[13];
	unsigned char			PhaseId;
	unsigned char			End;
}SenData1_t;

typedef struct PhaseInfo_t
{
	unsigned char			PhaseId;
	unsigned char			LampColor;
	unsigned short			PhaseT;
}PhaseInfo_t;

typedef struct SenData2_t
{
	unsigned char			Head;
	unsigned char			Len;
	unsigned char			Func;
	unsigned char			PhaseN;
	PhaseInfo_t				PI[20];
	unsigned char			End;				
}SenData2_t;

typedef struct SenServer_t
{
	unsigned char			Head;
	unsigned char			Operate;
	unsigned char			ObjectId;
	unsigned char			Rec;//0x00;
	unsigned char			ChePaiN[13];
	unsigned char			udlink;
	unsigned char			PhaseId;
	unsigned char			Mark;//1:green delay; 2: red wait;
	unsigned short			TimeT;
}SenServer_t;

//**************雷达故障数据***********
typedef struct RadarErr_t
{
	unsigned char			RoadId;
	unsigned char			RoadStatus;
	unsigned char			ErrType;
	unsigned char			valid;
}RadarErr_t;
typedef struct RadarErrList_t
{
	unsigned char			RoadNum;
	RadarErr_t				RadarErr[ROADN];
	struct timeval          tl;
}RadarErrList_t;
//********************************************

//**************统计数据***********************
typedef struct StatisData_t
{
	unsigned char			RoadId;
	unsigned char			TypeAFlow;
	unsigned char			TypeBFlow;
	unsigned char			TypeCFlow;
	unsigned char			Occupancy;//车辆平均占有率。单位: 0.5%
	unsigned char			AverSpeed;//车辆平均行驶速度,255 表示溢出。 单位:km/h
	unsigned char			AverCarLen;//车辆平均车长, 255 表示溢出。单位: 0.1m
	unsigned char			AverHeadWay;//车辆平均车头时距, 255 表示溢出。单位: s
	unsigned char			QueueLen;
	unsigned char			TrafficInfo;
	unsigned char			Reser1;
	unsigned char			Reser2;
	unsigned char			Reser3;
	unsigned char           valid;
}StatisData_t;
typedef struct StatisDataList_t
{
	unsigned char           TypeALen;//取值130
    unsigned char           TypeBLen;//取值60
    unsigned char           TypeCLen;//取值0
	unsigned char           RoadNum;
	unsigned char			Reser1;
	unsigned char           Reser2;
	unsigned char           Reser3;
	unsigned char           Reser4;
	unsigned short          StatisCycle;//统计周期；
	unsigned int			FormTime;//生成时间；
	StatisData_t			StatisData[ROADN];
	struct timeval          tl;
}StatisDataList_t; 
//********************************************

//******************排队数据********************
typedef struct QueueData_t
{
	unsigned char			RoadId;
	unsigned char			QueueLen;
	unsigned char			QueueHead;
	unsigned char			QueueEnd;
	unsigned char			QueueNum;
	unsigned char           valid;
}QueueData_t;
typedef struct QueueDataList_t
{
	unsigned char			RoadNum;
	QueueData_t				QueueData[ROADN];
	struct timeval          tl;
}QueueDataList_t;
//***********************************************

//************************路况数据*********************
typedef struct TrafficData_t
{
	unsigned char			RoadId;
	unsigned char			CarNum;
	unsigned char			SpaceOccu;
	unsigned char			AverSpeed;
	unsigned char			SpreadInfo;
	unsigned char			LastCarLoca;
	unsigned char           valid;
}TrafficData_t;
typedef struct TrafficDataList_t
{
	unsigned char			RoadNum;
	TrafficData_t			TrafficData[ROADN];
	struct timeval          tl;
}TrafficDataList_t;
//***************************************************

//***********************评价指数***********************
typedef struct	EvaData_t
{
	unsigned char			RoadId;
	unsigned char			AverStop;
	unsigned char			AverDelay;
	unsigned char			FuelConsume;
	unsigned char			GasEmiss;
	unsigned char           valid;
}EvaData_t;
typedef struct EvaDataList_t
{
	unsigned char			RoadNum;
	EvaData_t				EvaData[ROADN];
	struct timeval          tl;
}EvaDataList_t;

typedef struct JingWeiDu_t
{
	unsigned int			wdz;
	unsigned int			wdx;
	unsigned int			jdz;
	unsigned int			jdx;
}JingWeiDu_t;
//*************************************************

typedef struct PhInfo_t
{
	unsigned char		pid;
	unsigned char		pming;
	unsigned char		pmaxg;
	unsigned char		pfactt;
	unsigned char		paddt;
	unsigned int		phasestarttime;
	unsigned int		phaseendtime;
}PhInfo_t;
typedef struct AdaptData_t
{
	unsigned char		pattid;
	unsigned short		cycle;
	unsigned char		contype;
	unsigned char		stageid;
	unsigned int		time;
	unsigned char		phasenum;
	PhInfo_t			phi[32];
}AdaptData_t;
#endif
