/*
 * File:   mc.c
 * Author: sk
 *
 * Created on 20130701
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

#include "mcs.h"
#ifdef NTP_TIME
#include "sntp.h"
#define DEFAULT_TIME_ZONE       (28800)
#endif
#define SBYTE  /*36030*/37310
#define GC1 720/*6*/
#define GC2 719/*5*/
#define TNN 4/*3*/
#define DGTSID 16
#define BBYTE       8020
//0bit set '1' means that one wireless terminal is connecting signaler machine;
//1bit set '1' means that key is controlling signaler machine;
//2bit set '1' means that wireless treminal is controlling signaler machine;
//3bit set '1' means that system software is controlling signaler machine;
//4bit set '1' means that configure tool is controlling signaler machine;
//5bit set '1' means that wireless terminal is lawful;
//6bit set '1' means not need yellowflash and allred after software reboot
//7bit set '1' means there are flow data or detector has been linked;
//8bit set '1' means RFID is controlling;
//9bit set '1' means EMBED configure tool is connecting and monitor;
//10bit set '1' means system optimize need to degrade since network err;

//11bit set '1' means control or cancel control comes from side door serial;
//12bit set '1' means step by step comes from side door serial;
//13bit set '1' means yellow flash comes from side door serial
//14bit set '1' means all red comes from side door serial;
//15bit set '1' means connect successfully according to new standard;
static unsigned short			markbit2 = 0;
static unsigned char            lip = 0;//the last status of ip address
static pthread_t           		servid;
static unsigned int				seryes = 0;
static pthread_t				clivid;
static unsigned char			cliyes = 0; 
static unsigned char			houtaiip[17] = {0};
static unsigned short			houtaiport = 0;
#ifdef NTP_TIME
static unsigned char			ntpip[17] = {0};
static unsigned short			ntpport = 0;
#endif
static int						netfd = 0;
static int						ssockfd = 0;
static int						edgersockd = 0;
static int						csockfd = 0;
static int						n_csockfd = 0;
static int						lhsockfd = 0;
static TSCHeader_t				tscheader;
static Unit_t					unit;
static Schedule_t				schedule;
static TimeSection_t			timesection;
static Pattern_t				pattern;
static Pattern1_t				pattern1;//added by sk on 20150609
static TimeConfig_t				timeconfig;
static Phase_t					phase;
static PhaseError_t				phaseerror;
static Channel_t				channel;
static ChannelHint_t			channelhint;
static Detector_t				detector;

static TSCHeader_t              stscheader;
static Unit_t                   sunit;
static Schedule_t               sschedule;
static TimeSection_t            stimesection;
static Pattern_t                spattern;
static TimeConfig_t             stimeconfig;
static Phase_t                  sphase;
static PhaseError_t             sphaseerror;
static Channel_t                schannel;
static ChannelHint_t            schannelhint;
static Detector_t               sdetector;

static DetectorData_t			detectdata;
static EventClass_t				eventclass;
static EventLog_t				eventlog;

static ChannelStatus_t			chanstatus;
static tscdata_t				tscdata;
static tscdata_t                stscdata;
static lamperrtype_t			lamperrtype;

static BusList_t				buslist;

static JingWeiDu_t				jwd;
static unsigned char			jwdi[11] = {0};

static int						serial[5] = {0};//serial[0]:base board serial port; 
											//serial[1]:vehicle check board serial port; 
											//serial[2]:face board serial port; 
											//serial[3]: GPS;
											//serial[4]: /dev/ttyS2.reserverd
static unsigned char			wlmark[4] = {0}; 
static statusinfo_t				sinfo;
static unsigned int				update = 0; //update pattern or clock is changed;
//0bit set '1' means end time of current pattern has arrived;
//1bit set '1' means conf tool connect successfully; 
//2bit set '1' means server can set whole pattern;
//3bit set '1' means have new flow;

//4bit set '1' means start monitor;on the contrary,stop monitor;
//5bit set '1' means face board request send latest five lamp err or restore info;
//6bit set '1' means configure data or clock do have update
//7bit set '1' means lamp err yellow flash;

//8bit set '1' means next phase is coordinate control,end current pattern before two cycles;
//9bit set '1' means there is existing event;
//10bit set '1' means now phase is transition stage 
//11bit set '1' means there need to reagain open and set serial port;

//12bit set '1' means there need to connect server again since connect err;
//13bit set '1' means there is existing  lamp status
//14bit set '1' means center server is controlling signaler machine;
//15bit set '1' means active report is closed; 
static int						roadinfo = 0;
static int                		roadinforeceivetime = 0;
static int                      phasedegrphid = 0;
static unsigned short			markbit = 0;
static int						flowpipe[2] = {0};//the pipe of sending vehicle flow
static int						pendpipe[2] = {0};//info source for monitor pending phase or degrade
static int						conpipe[2] = {0};//the pipe of sending traffic control info
static int						synpipe[2] = {0}; //child module tell main module its end time 
static int						endpipe[2] = {0};
static int						ffpipe[2] = {0}; //pipe of fastforward
static unsigned char			retpattern = 0;
static unsigned char           	contmode = 0;
static unsigned char           	ncontmode = 0;
static unsigned char			auxfunc = 0;
static unsigned char			halfdownt = 0;
static unsigned int				retphaseid = 0;//child module need modify the variable;
static unsigned char			retslnum = 0;
static unsigned char			retstageid = 0;//child module need modify the variable,its value starts from 1;
static unsigned char			retcolor = 4;//'0' red,'1'yellow,'2'green,'3'close
static unsigned char			wifiEnable = 0;//added by sk on 20150703
static pthread_t           		cswid;
static driboardstatus_t			dbstatus;
static fiveerrlampinfo_t		felinfo;
#define DETECTORNUM 60
static DeteId_t					detepro[DETECTORNUM];
static unsigned int				ccontrol = 0;
static unsigned int				fcontrol = 0; 
static unsigned char			trans = 0;//transition stage	
//RFID mark
//0bit set '1' means err of North Reader and Writer;
//1bit set '1' means err of East Reader and Wrtiter;
//2bit set '1' means err of South Reader and Writer;
//3bit set '1' means err of West Reader and Writer;
static unsigned char            rfidm = 0;

static SignalLightGroup_t		slg[MAX_CHANNEL] = {0}; 
static unsigned short			v2xcyct = 0;
//0 位设置为1，意思为感应控制或自适应有明确倒计时了；
static unsigned char			v2xmark = 0;
static unsigned char   			fanganID = 0;

//20220916
static struct sockaddr_in	    my_addr; 
static int                      iNetServerSocketId = 0;
static pthread_t           		NetServerPid;
static unsigned int				NetServerYes = 0;
static ClientSocketInfo_t 	    pstClientSocketInfo[10];


//软件事件
static unsigned char *softevent[] = {"软件开始运行","串口打开成功","串口打开错误","串口配置成功", \
       "串口配置错误","看门狗打开成功","看门狗打开错误","配置文件打开成功", "配置文件打开错误", \
	   "无时段可以执行","无方案可以执行","无效的控制模式","同阶段内相位冲突","无有效相位","写底板串口失败", \
	   "写面板串口失败","本地服务器建立成功","本地服务器建立失败","本地服务器接收数据失败",\
       "本地服务器发送数据失败","远程服务器连接失败","本地客户端发送数据失败","本地客户端接收数据失败", \
	   "读取GPS数据成功","读取GPS数据失败","远程服务器连接成功","面板按键管制信号机成功", \
	   "面板按键管制信号机失败","面板按键解除管制成功","面板按键解除管制失败","面板按键步进操作成功", \
	   "面板按键步进操作失败","面板按键黄闪操作成功","面板按键黄闪操作失败","面板按键全红操作成功", \
	   "面板按键全红操作失败","面板按键修改了信号机和服务器IP及端口号","面板按键修改了信号机时钟", \
	   "面板按键修改了相位时间","手持遥控终端管制信号机成功","手持遥控终端管制信号机失败", \
	   "手持遥控终端取消管制成功","手持遥控终端取消管制失败","手持遥控终端步进操作成功", \
	   "手持遥控终端步进操作失败","手持遥控终端黄闪操作成功","手持遥控终端黄闪操作失败", \
	   "手持遥控终端全红操作成功","手持遥控终端全红操作失败","手持遥控终端全灭操作成功", \
       "手持遥控终端全灭操作失败","手持遥控终端南北直右控制成功","手持遥控终端南北直右控制失败",\
	   "手持遥控终端南北左右控制成功","手持遥控终端南北左右控制失败","手持遥控终端东西直右控制成功",\
	   "手持遥控终端东西直右控制失败","手持遥控终端东西左右控制成功","手持遥控终端东西左右控制失败", \
	   "手持遥控终端北三向控制成功","手持遥控终端北三向控制失败","手持遥控终端东三向控制成功", \
	   "手持遥控终端东三向控制失败","手持遥控终端南三向控制成功","手持遥控终端南三向控制失败", \
	   "手持遥控终端西三向控制成功","手持遥控终端西三向控制失败","手持遥控终端与信号机失去连接", \
	   "手持遥控终端对信号机进行了快进操作","手持终端对信号机进行了连接请求","配置工具更新了信号机方案", \
	   "配置工具设置信号机时间","配置工具设置信号机IP地址","配置工具设置服务端IP地址","配置工具重启信号机", \
	   "配置工具清除日志","配置工具清除流量","中心管理软件进行全红操作","中心管理软件进行黄闪操作", \
	   "中心管理软件进行步进操作","中心管理软件进行全灭操作","中心管理软件进行快进操作", \
	   "中心管理软件修改了信号机方案","中心管理软件管制信号机","中心管理软件解除管制信号机", \
	   "中心管理软件修改了信号机时钟","中心管理软件重启了信号机","中心管理软件进行了相位控制操作",\
	   "中心管理软件进行了联动控制操作","配置工具对信号机进行了管制操作","配置工具对信号机解除了管制", \
	   "配置工具对信号机进行了步进操作","配置工具对信号机进行了黄闪操作","配置工具对信号机进行了全红操作", \
	   "网络故障，开始降级运行","网络恢复，开始系统协调优化控制","北支路接收读写器主机故障", \
	   "东支路接收读写器主机故障","南支路接收读写器主机故障","西支路接收读写器主机故障", \
	   "北支路接收读写器主机正常","东支路接收读写器主机正常","南支路接收读写器主机正常", \
	   "西支路接收读写器主机正常","配置工具对信号机进行了快进操作","配置工具对信号机进行了全灭操作", \
	   "配置工具对信号机进行了南北直右管控操作","配置工具对信号机进行了南北左右管控操作", \
	   "配置工具对信号机进行了东西直右管控操作","配置工具对信号机进行了东西左右管控操作", \
	   "配置工具对信号机进行了北三向管控操作","配置工具对信号机进行了东三向管控操作", \
	   "配置工具对信号机进行了南三向管控操作","配置工具对信号机进行了西三向管控操作", \
	   "配置工具对信号机进行了阶段相位1管控操作","配置工具对信号机进行了阶段相位2管控操作", \
	   "配置工具对信号机进行了阶段相位3管控操作","配置工具对信号机进行了阶段相位4管控操作", \
	   "配置工具对信号机进行了阶段相位5管控操作","配置工具对信号机进行了阶段相位6管控操作", \
	   "配置工具对信号机进行了阶段相位7管控操作","配置工具对信号机进行了阶段相位8管控操作", \
	   "Lora通信接收主机故障","Lora通信接收从机故障","Lora通信接收从机故障","Lora通信接收从机故障", \
	   "中心管理软件进行了通道管制操作","检测器故障导致相位降级","检测器故障恢复，相位正常运行"};



static unsigned char    VersionId[] = "CYT0V1.0.820241122END";
static unsigned char    MajorVerId = 1;
static unsigned char    MinorVerId1 = 0;
static unsigned char    MinorVerId2 = 8;
static unsigned char    Year = 24; 
static unsigned char    Month = 11;
static unsigned char    Day = 22;

//超过1次就死机，目前还没找到根源
const int c_iMaxLoopCnt = 1;



void setup_net_server(void *arg);
void setup_client(void *arg);
void Create_New_Net_Server(void *arg);

//yellow flash since err
void err_yellow_flash(void *arg)
{
//	int *sp = arg;
//	all_yellow_flash(*sp,0);
	yfdata_t		*yfdata = arg;
	new_yellow_flash(yfdata);

	pthread_exit(NULL);
}

void localhost_client(void *arg)
{
	#ifdef MAIN_DEBUG
	printf("Localhost client thread,lhsockfd: %d,File: %s,Line: %d\n",lhsockfd,__FILE__,__LINE__);
	#endif

	fd_set					lhRead;
	struct timeval			lhmt,lhptt;
	unsigned char			lhrevbuf[256] = {0};
	unsigned char			lherr[10] = {0};
	
	while(1)
	{//1
		FD_ZERO(&lhRead);
		FD_SET(lhsockfd,&lhRead);
		lhmt.tv_sec = 15;
		lhmt.tv_usec = 0;
		int lhret = select(lhsockfd+1,&lhRead,NULL,NULL,&lhmt);
		if (lhret < 0)
		{
		#ifdef MAIN_DEBUG
			printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			break;
		}
		if (0 == lhret)
		{
		#ifdef MAIN_DEBUG
			printf("Time out of monitor,FIle: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			break;
		}//lhret == 0
		if (lhret > 0)
		{//lhret > 0
			memset(lhrevbuf,0,sizeof(lhrevbuf));
			if (read(lhsockfd,lhrevbuf,sizeof(lhrevbuf)) <= 0)
			{
			#ifdef MAIN_DEBUG
				printf("Since client exit,continue to listen,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				break;
			}
			#ifdef MAIN_DEBUG
			printf("Reading network port,lhrevbuf: %s,File: %s,Line: %d\n",lhrevbuf,__FILE__,__LINE__);
			#endif
			if (!strcmp(lhrevbuf,"IAMALIVE"))
			{
				continue;	
			}
	
			if (!strcmp(lhrevbuf,"FastForward"))
			{
				unsigned char           tpd[32] = {0};
				//clean pipe
				while (1)
				{
					memset(tpd,0,sizeof(tpd));
					if (read(ffpipe[0],tpd,sizeof(tpd)) <= 0)
						break;
				}
				//end clean pipe
				if (!wait_write_serial(ffpipe[1]))
				{
					if (write(ffpipe[1],"fastforward",11) < 0)
					{
					#ifdef MAIN_DEBUG
						printf("write pipe error,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
				}
				else
				{
				#ifdef MAIN_DEBUG
					printf("can not write,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
				gettimeofday(&lhptt,NULL);
				update_event_list(&eventclass,&eventlog,1,105,lhptt.tv_sec,&markbit);
				if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
				{
				#ifdef MAIN_DEBUG
					printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
			}
	
			if (!strcmp("SetConfigure",lhrevbuf))
			{//2
			#ifdef MAIN_DEBUG
				printf("%s,File: %s,Line: %d\n",lhrevbuf,__FILE__,__LINE__);
			#endif
				unsigned char			configys[9] = "CONFIGYS";
				unsigned char			revdata = 0;
				int						num = 0;
				int						i = 0;
				unsigned char			head[5] = {0};
				int						slen = 0;
				int						tslen = 0;
				unsigned char			end[4] = {0};
				unsigned char			sdata[SBYTE] = {0};

				if (write(lhsockfd,configys,sizeof(configys)) < 0)
				{
				#ifdef MAIN_DEBUG
					printf("write file error,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif	
				}
				num = 0;
				while (1)
				{//3
					revdata = 0;
					num += 1;
					if (num > (SBYTE+11))
						break;
					read(lhsockfd,&revdata,1);
					if (num <= 4)
					{
						head[num-1] = revdata;
						continue;
					}
					if ((num >= 5) && (num <= 8))
					{
						tslen = 0;
						tslen |= revdata;
						tslen <<= ((num-5)*8);
						slen |= tslen;
						continue;
					}
					if ((num >= 9) && (num <= (SBYTE+8)))
					{
						sdata[num-9] = revdata;
						continue;
					}
					if ((num >= (SBYTE+9)) && (num <= (SBYTE+11)))
					{
						end[num - SBYTE -9] = revdata;
						continue;
					}
				}//3
				if ((strcmp("CYT4",head)) || ((SBYTE+11) != slen) || (strcmp("END",end)))
				{
				#ifdef MAIN_DEBUG
					printf("Not our data,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					write(lhsockfd,"CONFIGER",9);
					continue;
				}
				#ifdef MAIN_DEBUG
				printf("This is our data,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				write(lhsockfd,"CONFIGOK",9);

				if (-1 == system("rm -f ./data/cy_tsc.dat"))
				{
				#ifdef MAIN_DEBUG
					printf("delete file error,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
				#ifdef MAIN_DEBUG
					printf("delete file successfully,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}	
				FILE					*nfp = NULL;
				nfp = fopen(CY_TSC,"wb+");
				if (NULL == nfp)
				{
				#ifdef MAIN_DEBUG
					printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					continue;
				}


				fwrite(sdata,sizeof(sdata),1,nfp);
				
				fseek(nfp,0,SEEK_SET);
				memset(&tscheader,0,sizeof(tscheader));
				memset(&unit,0,sizeof(unit));
//					fread(&tscheader,sizeof(tscheader),1,nfp);
//    				fread(&unit,sizeof(unit),1,nfp);
				memset(&stscheader,0,sizeof(stscheader));
				memset(&sunit,0,sizeof(sunit));
				memset(&sschedule,0,sizeof(sschedule));
				memset(&stimesection,0,sizeof(stimesection));
				memset(&spattern,0,sizeof(spattern));
				memset(&stimeconfig,0,sizeof(stimeconfig));
				memset(&sphase,0,sizeof(sphase));
				memset(&sphaseerror,0,sizeof(sphaseerror));
				memset(&schannel,0,sizeof(schannel));
				memset(&schannelhint,0,sizeof(schannelhint));
				memset(&sdetector,0,sizeof(sdetector));
		
				fread(&stscheader,sizeof(stscheader),1,nfp);
				memcpy(&tscheader,&stscheader,sizeof(stscheader));
				fread(&sunit,sizeof(sunit),1,nfp);
				memcpy(&unit,&sunit,sizeof(sunit));

				fread(&sschedule,sizeof(sschedule),1,nfp);
				fread(&stimesection,sizeof(stimesection),1,nfp);

				//added by sk on 20150609   
				if (unit.RemoteFlag & 0x01)
				{//phase offset is 2 bytes
					fread(&spattern,sizeof(spattern),1,nfp);
				}//phase offset is 2 bytes
				else
				{//phase offset is 1 byte
					memset(&pattern1,0,sizeof(pattern1));
					fread(&pattern1,sizeof(pattern1),1,nfp);
					pattern_switch(&spattern,&pattern1);
				}//phase offset is 1 byte

//    				fread(&spattern,sizeof(spattern),1,nfp);
				fread(&stimeconfig,sizeof(stimeconfig),1,nfp);
				fread(&sphase,sizeof(sphase),1,nfp);
				fread(&sphaseerror,sizeof(sphaseerror),1,nfp);
				fread(&schannel,sizeof(schannel),1,nfp);
				fread(&schannelhint,sizeof(schannelhint),1,nfp);
				fread(&sdetector,sizeof(sdetector),1,nfp);

				fclose(nfp);
				unsigned char		backup[4] = {0};
				backup[0] = 0xA7;
				backup[1] = unit.sntime;
				backup[2] = unit.ewtime;
				backup[3] = 0xED;  
				//inform stm32 that green time of backup pattern
				if (!wait_write_serial(serial[0]))
				{
					if (write(serial[0],backup,sizeof(backup)) < 0)
					{
					#ifdef MAIN_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
				}
				else
				{
				#ifdef MAIN_DEBUG
					printf("serial cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				//end inform stm32;	

				markbit |= 0x0040;//configure data do have update
				if (!wait_write_serial(endpipe[1]))
				{
					if (write(endpipe[1],"UpdateFile",11) < 0)
					{
					#ifdef MAIN_DEBUG
						printf("Write endpipe err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
				}
				else
				{
				#ifdef MAIN_DEBUG
					printf("Can not write endpipe,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}

				continue;
			}//2,setconfigure
			if (!strcmp("GetTSCTime",lhrevbuf))
			{//gethosttime
				struct timeval				systime;
				int							settime = 0;
				int							ti= 0;
				unsigned char				timebuf[11] = {0};

				memset(&systime,0,sizeof(systime));
				if (-1 == gettimeofday(&systime,NULL))
				{
				#ifdef MAIN_DEBUG
					printf("Get system time error,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					continue;
				}
				settime = systime.tv_sec;
				timebuf[0] = 'C';
				timebuf[1] = 'Y';
				timebuf[2] = 'T';
				timebuf[3] = '7';
				ti = settime;
				ti &= 0x000000ff;
				timebuf[4] |= ti; 
				ti = settime;
				ti &= 0x0000ff00;
				ti >>= 8;
				timebuf[5] |= ti;
				ti = settime;
				ti &= 0x00ff0000;
				ti >>= 16;
				timebuf[6] |= ti;
				ti = settime;
				ti &= 0xff000000;
				ti >>= 24;
				timebuf[7] |= ti;
				timebuf[8] = 'E';
				timebuf[9] = 'N';
				timebuf[10] = 'D';
				if (write(lhsockfd,timebuf,sizeof(timebuf)) < 0)
				{
				#ifdef MAIN_DEBUG
					printf("send data err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif	
				}

				continue;
			}//gethosttime

			if (!strcmp("GetConfigure",lhrevbuf))
			{//3
			#ifdef MAIN_DEBUG
				printf("%s,File: %s,Line: %d\n",lhrevbuf,__FILE__,__LINE__);
			#endif
				FILE					*fp = NULL;
				unsigned char			*buf = NULL;
				int						len = 0;
				int						total = 0;
				unsigned char			*sdata = NULL;

				fp = fopen(CY_TSC,"rb");
				if (NULL == fp)
				{
				#ifdef MAIN_DEBUG
					printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					continue;
				}
				fseek(fp,0,SEEK_END);
				len = ftell(fp);
				fseek(fp,0,SEEK_SET);
				#ifdef MAIN_DEBUG
				printf("****************len: %d,File: %s,Line: %d\n",len,__FILE__,__LINE__);
				#endif
				buf = (unsigned char *)malloc(len);
				if (NULL == buf)
				{
				#ifdef MAIN_DEBUG
					printf("malloc error,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					continue;
				}
				memset(buf,0,len);
				if (fread(buf,len,1,fp) != 1)
				{
				#ifdef MAIN_DEBUG
					printf("read file error,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					continue;
				}
				sdata = (unsigned char *)malloc(len+11);
				if (NULL == sdata)
				{
				#ifdef MAIN_DEBUG
					printf("malloc error,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					if (NULL != buf)
					{
						free(buf);
						buf = NULL;
					}
					continue;
				}
				total = len + 11;
				memset(sdata,0,len+11);
				mc_combine_str(sdata,"CYT4",total,buf,len,"END");
				if (write(lhsockfd,sdata,total) < 0)
				{
				#ifdef MAIN_DEBUG
					printf("send data err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
		
				fclose(fp);
				if (NULL!= buf)
				{
					free(buf);
					buf = NULL;
				}
				if (NULL != sdata)
				{
					free(sdata);
					sdata = NULL;
				}

				continue;
			}//3,getconfig
			if (!strcmp("GetLampStatus",lhrevbuf))
			{//4
			#ifdef MAIN_DEBUG
				printf("%s,retstageid:%d,File: %s,Line: %d\n",lhrevbuf,retstageid,__FILE__,__LINE__);
			#endif
				unsigned char				ls[30] = {0};
				if (SUCCESS != mc_lights_status(ls,contmode,retstageid,retphaseid,&chanstatus))
				{
				#ifdef MAIN_DEBUG
					printf("mc_lights_status err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
					continue;
				}
				if (write(lhsockfd,ls,sizeof(ls)) < 0)
				{
				#ifdef MAIN_DEBUG
					printf("send data err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif	
				}
				continue;				
			}//4,GetLampStatus
			
			if (!strcmp("BeginMonitor",lhrevbuf))
			{
				markbit2 |= 0x0200;
				continue;
			}
			if (!strcmp("EndMonitor",lhrevbuf))
			{
				markbit2 &= 0xfdff;
				continue;
			}
		}//netret > 0
		
	}//1,2th while loop	

	return;
}

//check the status of wireless card
void check_wireless_card(void *arg)
{
	int 			cwsockfd;
	struct ifreq 	ifr;

	bzero(&ifr,sizeof(struct ifreq));
	cwsockfd = socket(AF_INET,SOCK_DGRAM,0);
    strcpy(ifr.ifr_name,"ra0");
    ioctl(cwsockfd,SIOCGIFFLAGS,&ifr);

	while (10)
	{
		sleep(10);
       	if(ifr.ifr_flags & IFF_UP) 
		{
		#ifdef MAIN_DEBUG
			printf("wireless card is on!\n");
		#endif
       	}
       	else 
		{
		#ifdef MAIN_DEBUG
			printf("wireless card is off!\n");	
		#endif
			system("ifconfig ra0 up");
			system("ifconfig ra0 192.168.99.2");
			system("udhcpd /mnt/nandflash/udhcpd.conf");
       	}
		strcpy(ifr.ifr_name,"ra0");
		ioctl(cwsockfd,SIOCGIFFLAGS,&ifr);
	}//while loop
}

//call fit control pattern according to pattern and control
void run_control_pattern(void *arg)
{
	fcdata_t	*fcdata = arg;

	switch (*(fcdata->contmode))
	{
		case 0: //free control or timing control
			#ifdef MAIN_DEBUG
			printf("Begin to timing control,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			timing_control(fcdata,&tscdata,&chanstatus);
			break;
		case 1: //all lamps will be closed;
			#ifdef MAIN_DEBUG
            printf("Begin to close all lamps,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
			same_control(fcdata,&tscdata,&chanstatus);
			break;
		case 2: //all lamps will be yellow flash
			#ifdef MAIN_DEBUG
            printf("Begin to yellow flash,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            same_control(fcdata,&tscdata,&chanstatus);
			break;
		case 3: //all lamps will be red
			#ifdef MAIN_DEBUG
            printf("Begin to all red,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            same_control(fcdata,&tscdata,&chanstatus);
			break;
		case 4: //coordinate control
			#ifdef MAIN_DEBUG
            printf("Begin to coordinate control,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
			timing_control(fcdata,&tscdata,&chanstatus);
			break;
		case 5: //full detect control
			#ifdef MAIN_DEBUG
			printf("Begin to detect control,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			full_detect_control(fcdata,&tscdata,&chanstatus);
			break;
		case 6: //major stem half detect control
			#ifdef MAIN_DEBUG
			printf("Begin to major detect control,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			major_half_detect(fcdata,&tscdata,&chanstatus);
			break;
		case 7: //minor stem half detect control
			#ifdef MAIN_DEBUG
			printf("Begin to minor detect control,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			minor_half_detect(fcdata,&tscdata,&chanstatus);
			break;
		case 8: //single self adapt
			#ifdef MAIN_DEBUG
			printf("Begin to self adapt control,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			self_adapt_control(fcdata,&tscdata,&chanstatus);
			break;
		case 9: //person pass street
			#ifdef MAIN_DEBUG
			printf("Begin to person pass street,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			person_pass_street(fcdata,&tscdata,&chanstatus);
			break;
		case 10: //coordinate detect control
			#ifdef MAIN_DEBUG
			printf("Begin to coord detect,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			coord_detect_control(fcdata,&tscdata,&chanstatus);
			break;
		case 11: //master-slave line control
			#ifdef MAIN_DEBUG
			printf("Begin to major slave line detect control,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			major_salve_detect_control(fcdata,&tscdata,&chanstatus);
			break;
		case 12: //system optimize
			#ifdef MAIN_DEBUG
            printf("Begin to system optimize control,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
			if ((!(auxfunc & 0x10)) && (!(auxfunc & 0x20)))
			{//area coordinate of system optimize control
				optimize_coordinate_control(fcdata,&tscdata,&chanstatus);
			}//area coordinate of system optimize control
			else if ((!(auxfunc & 0x20)) && (auxfunc & 0x10))
			{//area self adapt of system optimize control
				optimize_self_adapt_control(fcdata,&tscdata,&chanstatus);
			}//area self adapt of system optimize control
			else if ((auxfunc & 0x20) && (!(auxfunc & 0x10)))
			{//area timing control of system optimize control
				optimize_timing_control(fcdata,&tscdata,&chanstatus);
			}//area timing control of system optimize control
			break;
		case 13: //intervene line control
		case 14: //green+trigger
			if (auxfunc & 0x40)
			{
			#ifdef MAIN_DEBUG
            	printf("Begin to rail crossing control,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
				rail_crossing_control(fcdata,&tscdata,&chanstatus);
			}
			else if (auxfunc & 0x80)
			{
			#ifdef MAIN_DEBUG
            	printf("Begin to overload control,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
				overload_control(fcdata,&tscdata,&chanstatus);
			}
			else
			{
			#ifdef MAIN_DEBUG
            	printf("Begin to green trigger,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
				green_trigger(fcdata,&tscdata,&chanstatus);
			}
			break;
		case 15: //yellow+trigger
			#ifdef MAIN_DEBUG
            printf("Begin to yellow flash trigger,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            yellow_flash_trigger(fcdata,&tscdata,&chanstatus);
            break;
		case 27: //bus priority
			#ifdef MAIN_DEBUG
			printf("Begin to bus priority control,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			bus_priority_control(fcdata,&tscdata,&chanstatus,&buslist);	
			break;
		case 32://brain self adapt
			#ifdef MAIN_DEBUG
            printf("Begin to brain self adapt control,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
			brain_self_adapt_control(fcdata,&tscdata,&chanstatus);
			break;
		case 33://环岛控制
			#ifdef MAIN_DEBUG
			printf("Begin to cycle road control,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			cycle_road_control(fcdata,&tscdata,&chanstatus);
			break;
		case 34://一体化信号控制
			#ifdef MAIN_DEBUG
			printf("Begin to inter grate control,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			Inter_Grate_Control(fcdata,&tscdata,&chanstatus);
			break;
		default:
			break;
	}

	pthread_exit(NULL);
}

//monitor serial port
void monitor_serial_port(void *arg)
{
	sockfd_t                *sockfd = arg;
	fd_set					nRead;
	int						i = 0,j = 0,z = 0;
	int						max = 0;	
	timedown_t				timedown;
	unsigned char			spbuf[512] = {0};//modidied by shikang on 20171123
	unsigned char			*pspbuf = spbuf;
	unsigned char			num = 0;
	unsigned char			mark = 0;
	unsigned char			dbnum = 0;	
	struct timeval			stt;
	unsigned char			elv = 0;	
	unsigned char			fenum = 0;
	unsigned char			err[10] = {0};
	unsigned char			gcinfo[12] = {0};//added by sk on 20151215
	unsigned char			wlinfo[13] = {0};
	unsigned char			ipstr[16] = {0};
	unsigned char           nmstr[16] = {0};
	unsigned char           gwstr[16] = {0};
	unsigned char           sec1[4] = {0};
	unsigned char           sec2[4] = {0};
	unsigned char           sec3[4] = {0};
	unsigned char           sec4[4] = {0};

	unsigned int			phaseid = 0;
	unsigned short			factfn = 0;
	unsigned int			mlip = 0;
	unsigned int			gcbit = 0;//added by sk on 20151216

	unsigned char			staid = 0;
	unsigned char			stacon[3] = {0};

	unsigned char			dibuf[16] = {0};
	detectinfo_t			dinfo;
	dinfo.pattern = &retpattern;
	dinfo.stage = &retstageid;
	dinfo.color = &retcolor;
	dinfo.phase = &retphaseid;
	dinfo.detectid = 0;
	dinfo.time = 0;

	terminalpp_t			tpp;
	unsigned char			terbuf[11] = {0};

	unsigned char			cdata[256] = {0}; //buf for clean pipe 
	unsigned char			fbdata[4] = {0};//send data to face board;
	fbdata[0] = 0xC9;
	fbdata[3] = 0xED;
	unsigned char			dianya[5] = {0};
	dianya[0] = 0xCB;
	dianya[4] = 0xED;
	unsigned char			errfb[6] = {0};
	errfb[0] = 0xC1;
	errfb[2] = 0;
	errfb[3] = 0;
	errfb[4] = 0;
	errfb[5] = 0xED;
	unsigned char			dedata[8] = {0};//detector err data
	unsigned char			dfdata[8] = {0};//detector flow data
	unsigned char			dbsdata[9] = {0};//driver board status data
	unsigned char			lampdata[10] = {0};//lamp status
	unsigned char			wtbuf[10] = {0};
	dbsdata[0] = 'C';
	dbsdata[1] = 'Y';
	dbsdata[2] = 'T';
	dbsdata[3] = 'D';
	dbsdata[6] = 'E';
	dbsdata[7] = 'N';
	dbsdata[8] = 'D';
 
	lampdata[0] = 'C';
	lampdata[1] = 'Y';
	lampdata[2] = 'T';
	lampdata[3] = 'E';
	lampdata[7] = 'E';
	lampdata[8] = 'N';
	lampdata[9] = 'D';

	dedata[0] = 'C';
	dedata[1] = 'Y';
	dedata[2] = 'T';
	dedata[3] = 'A';
	dedata[5] = 'E';
	dedata[6] = 'N';
	dedata[7] = 'D';
	
	dfdata[0] = 'C';
	dfdata[1] = 'Y';
	dfdata[2] = 'T';
	dfdata[3] = 'B';
	dfdata[5] = 'E';
	dfdata[6] = 'N';
	dfdata[7] = 'D';

	while(1)
	{//0
		FD_ZERO(&nRead);
		//add serial port of base/vehicle/face board into fd_set;
		max = 0;
		for (i = 0; i < /*3*/5; i++)
		{
			if (3 == i)
				continue;
			if (max < serial[i])
				max = serial[i];
			FD_SET(serial[i],&nRead);
		}
		int ret = select(max+1,&nRead,NULL,NULL,NULL);
		if (ret < 0)
		{
		#ifdef MAIN_DEBUG
			printf("select call error,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			continue;
		}
		if (ret > 0)
		{
			if (FD_ISSET(serial[4],&nRead))
			{//if (FD_ISSET(serial[4],&nRead))
				memset(spbuf,0,sizeof(spbuf));
				pspbuf = spbuf;
				mark = 0;
				num = read(serial[4],spbuf,sizeof(spbuf));
				if (num > sizeof(spbuf))
                    continue;
				if (num > 0)
				{//if (num > 0)
					mark = 0;
					while (1)
					{//1
						if (mark >= num)
							break;
						if ((0xDA == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+2)))
						{//if ((0xDA == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+2)))
							//check stage whether exist in current pattern;
							if (((*(pspbuf+mark+1)<0x11))||((0x20<=*(pspbuf+mark+1))&&(*(pspbuf+mark+1)<=0x27)))
							{//not jump stage control
								stacon[0] = *(pspbuf+mark);
								stacon[1] = *(pspbuf+mark+1);
								stacon[2] = *(pspbuf+mark+2);

								while (1)
								{
									memset(cdata,0,sizeof(cdata));
									if (read(conpipe[0],cdata,sizeof(cdata)) <= 0)
										break;
								}
								//end clean pipe
								if (!wait_write_serial(conpipe[1]))
								{
									if (write(conpipe[1],stacon,sizeof(stacon)) < 0)
									{
									#ifdef MAIN_DEBUG
										printf("write pipe error,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								else
								{
								#ifdef MAIN_DEBUG
									printf("can not write,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
								if (0x01 == *(pspbuf+mark+1))
								{
									markbit2 |= 0x0800;
								}//control or cancel control
								else if (0x02 == *(pspbuf+mark+1))
								{
									markbit2 |= 0x1000;
								}//step by step
								else if (0x03 == *(pspbuf+mark+1))
								{
									markbit2 |= 0x2000;
								}//yellow flash
								else if (0x04 == *(pspbuf+mark+1))
                                {
									markbit2 |= 0x4000;
                                }//all red	
							}//not jump stage control
							if ((0x11 <= *(pspbuf+mark+1)) && (*(pspbuf+mark+1) <= 0x18))
							{//jump stage control
								staid = (0x0f & (*(pspbuf+mark+1)));
								#ifdef MAIN_DEBUG
								printf("staid: %d,File:%s,Line: %d\n",staid,__FILE__,__LINE__);
								#endif 
								if (SUCCESS == check_stageid_valid(staid,retpattern,&tscdata))
								{//2
									stacon[0] = *(pspbuf+mark);
									stacon[1] = *(pspbuf+mark+1);
									stacon[2] = *(pspbuf+mark+2);
								#ifdef MAIN_DEBUG
                                    printf("stacon[1]:%d,File: %s,Line: %d\n",stacon[1],__FILE__,__LINE__);
                                #endif
									while (1)
									{
										memset(cdata,0,sizeof(cdata));
										if (read(conpipe[0],cdata,sizeof(cdata)) <= 0)
											break;
									}
									//end clean pipe
									if (!wait_write_serial(conpipe[1]))
									{
										if (write(conpipe[1],stacon,sizeof(stacon)) < 0)
										{
										#ifdef MAIN_DEBUG
											printf("write pipe error,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
									#ifdef MAIN_DEBUG
										printf("can not write,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}//2
							}//jump stage control
							mark += 3;
							continue;
						}//if ((0xDA == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+2)))
						else
						{
							mark += 1;
							continue;
						}	
					}//1
				}//if (num > 0)
				continue;
			}//if (FD_ISSET(serial[4],&nRead))
			if (FD_ISSET(serial[0],&nRead))
			{//base board serial port
				memset(spbuf,0,sizeof(spbuf));
				pspbuf = spbuf;
				mark = 0;
				num = read(serial[0],spbuf,sizeof(spbuf));
				if (num > sizeof(spbuf))
                    continue;
	//			tcflush(serial[0],TCIOFLUSH);		
				if (num > 0)
				{
					mark = 0;
					while(1)
					{//1
						if (mark >= num)
							break;
						if ((0xB9 == *(pspbuf+mark)) && ((0xED == *(pspbuf+mark+5)) \
								|| (0xED==*(pspbuf+mark+9)) || (0xED==*(pspbuf+mark+4))))
						{//data of wireless terminal
							if (0 == lip)
							{
								get_last_ip_field(&mlip);
    							lip = mlip;
							#ifdef MAIN_DEBUG
								printf("Reagain get lip: %d,File: %s,Line: %d\n",lip,__FILE__,__LINE__);
							#endif
							}
							if((1 == *(pspbuf+mark+3)) && (8 == *(pspbuf+mark+1)) \
								&& (0 == *(pspbuf+mark+2))/* && (!(markbit2 & 0x0001))*/)
							{//if((1 == *(pspbuf+mark+3))&&(8 == *(pspbuf+mark+1))&&(0 == *(pspbuf+mark+2)))

								if (markbit2 & 0x0001)
								{
									if ((wlmark[0] != *(pspbuf+mark+5)) || (wlmark[1] != *(pspbuf+mark+6)) \
									|| (wlmark[2] != *(pspbuf+mark+7)) || (wlmark[3] != *(pspbuf+mark+8)))
									{
										mark += 10;
										continue;
									}
								}
								
								wlmark[0] = *(pspbuf+mark+5);
                                wlmark[1] = *(pspbuf+mark+6);
                                wlmark[2] = *(pspbuf+mark+7);
                                wlmark[3] = *(pspbuf+mark+8);
							
								if (!(markbit & 0x1000))
								{//if (!(markbit & 0x1000))
									memset(wtbuf,0,sizeof(wtbuf));
									wireless_terminal_mark(wtbuf,wlmark);
									write(csockfd,wtbuf,sizeof(wtbuf));
									sleep(1);
								#ifdef MAIN_DEBUG
									printf("markbit2: 0x%02x,File: %s,Line: %d\n",markbit2,__FILE__,__LINE__);
								#endif
									if (!(markbit2 & 0x0020))
									{//not lawful
										mark += 10;
										continue;
									}//not lawful
								}//if (!(markbit & 0x1000))
						
								tpp.head = 0xA9;
								tpp.size = 8;
								tpp.id = lip;
								tpp.type = 0x01;
								if (markbit2 & 0x0004)
									tpp.sta = 0x01;
								else
									tpp.sta = 0x00;
								tpp.func[0] = *(pspbuf+mark+5);
								tpp.func[1] = *(pspbuf+mark+6);
								tpp.func[2] = *(pspbuf+mark+7);		
								tpp.func[3] = *(pspbuf+mark+8);
							/*
								wlmark[0] = *(pspbuf+mark+5);
								wlmark[1] = *(pspbuf+mark+6);
								wlmark[2] = *(pspbuf+mark+7);
								wlmark[3] = *(pspbuf+mark+8);
							*/
								tpp.end = 0xED;
								memset(terbuf,0,sizeof(terbuf));
								if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
								{
									if (!wait_write_serial(serial[0]))
    								{
        								if (write(serial[0],terbuf,sizeof(terbuf)) < 0)
        								{
        								#ifdef MAIN_DEBUG
            								printf("write base board serial err,File: %s,Line: %d\n", \
														__FILE__,__LINE__);
        								#endif
        								}
										else
										{
											markbit2 |= 0x0001;			
										#ifdef MAIN_DEBUG	
										printf("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \
													0x%02x 0x%02x 0x%02x,File: %s,Line: %d\n",\
													terbuf[0],terbuf[1],terbuf[2],terbuf[3], \
													terbuf[4],terbuf[5],terbuf[6],terbuf[7], \
													terbuf[8],__FILE__,__LINE__);
										#endif
										}
    								}
    								else
    								{
    								#ifdef MAIN_DEBUG
        								printf("base board serial cannot write,File: %s,Line: %d\n", \
												__FILE__,__LINE__);
    								#endif
    								}
								}//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
				
								if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
            					{//actively report is not probitted and connect successfully
                					memset(wlinfo,0,sizeof(wlinfo));
									gettimeofday(&stt,NULL);
                					if (SUCCESS != wireless_info_report(wlinfo,stt.tv_sec,wlmark,0x01))
                					{
                					#ifdef MAIN_DEBUG
                    					printf("wireless_info_report,File: %s,Line: %d\n",__FILE__,__LINE__);
                					#endif
                					}
                					else
                					{
                    					write(csockfd,wlinfo,sizeof(wlinfo));
                					}
									update_event_list(&eventclass,&eventlog,1,70,stt.tv_sec,&markbit);
            					}
								else
								{
									gettimeofday(&stt,NULL);
									update_event_list(&eventclass,&eventlog,1,70,stt.tv_sec,&markbit);
								}	
								mark += 10;
                                continue;	
							}//if((1 == *(pspbuf+mark+3))&&(8 == *(pspbuf+mark+1))&&(0 == *(pspbuf+mark+2)))
							else if ((0x02 == *(pspbuf+mark+3)) && (3 == *(pspbuf+mark+1)))
							{//pant
						//		printf("receive pant of wireless,File: %s,LIne: %d\n",__FILE__,__LINE__);
								if (lip == *(pspbuf+mark+2))
                                {//if (lip == *(pspbuf+mark+2))
						//			printf("receive pant of wireless,File: %s,LIne: %d\n",__FILE__,__LINE__);
									tpp.head = 0xB9;
									tpp.size = 3;
									tpp.id = *(pspbuf+mark+2);
									tpp.type = 0x02;
									tpp.end = 0xED;
									memset(terbuf,0,sizeof(terbuf));
									if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
									{
										//clean pipe
										while (1)
										{	
											memset(cdata,0,sizeof(cdata));
											if (read(conpipe[0],cdata,sizeof(cdata)) <= 0)
												break;
										}
										//end clean pipe

										if (!wait_write_serial(conpipe[1]))
										{
											if (write(conpipe[1],terbuf,sizeof(terbuf)) < 0)
											{
											#ifdef MAIN_DEBUG
												printf("write pipe err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										else
										{
										#ifdef MAIN_DEBUG
											printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))

						//			printf("send pant to conpipe,File: %s,LIne: %d\n",__FILE__,__LINE__);
									if (markbit2 & 0x0001)
									{//disconnect status not send pant
										tpp.head = 0xA9;
										tpp.size = 3;
										tpp.id = lip;
										tpp.type = 0x02;
										tpp.end = 0xED;
										memset(terbuf,0,sizeof(terbuf));
										if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
										{
											if (!wait_write_serial(serial[0]))
											{
												if (write(serial[0],terbuf,sizeof(terbuf)) < 0)
												{
												#ifdef MAIN_DEBUG
													printf("write base board serial err,File: %s,Line: %d\n", \
															__FILE__,__LINE__);
												#endif
												}
											}
											else
											{
											#ifdef MAIN_DEBUG
												printf("base board serial cannot write,File: %s,Line: %d\n", \
													__FILE__,__LINE__);
											#endif
											}
										}//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
									}//disconnect status not send pant;
								}//if (lip == *(pspbuf+mark+2))

								mark += 5;
								continue;
							}//pant
							else if ((0x03 == *(pspbuf+mark+3)) && (3 == *(pspbuf+mark+1)))
							{//fastforward
								if((markbit2&0x0002)||(markbit2&0x0008)||(markbit2&0x0010)||(markbit2&0x0004))
                                {//have key or software or configure tool or wireless terminal \
									//is controlling signaler machine;
                                    mark += 5;
                                    continue;
                                }//have key or software or configure tool or wireless terminal \
									// controlling signaler machine;

								if ((lip == *(pspbuf+mark+2)) && (!(markbit & 0x0400)))
								{
									//clean pipe
                                	while (1)
                                	{
                                   		memset(cdata,0,sizeof(cdata));
                                   		if (read(ffpipe[0],cdata,sizeof(cdata)) <= 0)
                                       		break;
                                	}
                                	//end clean pipe

                                	if (!wait_write_serial(ffpipe[1]))
                                	{
                                   		if (write(ffpipe[1],"fastforward",11) < 0)
                                   		{
                                   		#ifdef MAIN_DEBUG
                                       		printf("write err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   		#endif
                                   		}
                                	}
                                	else
                                	{
                                	#ifdef MAIN_DEBUG
                                   		printf("can't write,File:%s,Line:%d\n",__FILE__,__LINE__);
                                	#endif
                                	}

									tpp.head = 0xA9;
									tpp.size = 3;
									tpp.id = lip;
									tpp.type = 0x03;
									tpp.end = 0xED;
									memset(terbuf,0,sizeof(terbuf));
									if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
									{
										if (!wait_write_serial(serial[0]))
    									{
        									if (write(serial[0],terbuf,sizeof(terbuf)) < 0)
        									{
        									#ifdef MAIN_DEBUG
            									printf("write base board serial err,File: %s,Line: %d\n", \
														__FILE__,__LINE__);
        									#endif
        									}
											else
											{
											#ifdef MAIN_DEBUG   
                                        		printf("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \
                                                    0x%02x 0x%02x 0x%02x,File: %s,Line: %d\n",\
                                                    terbuf[0],terbuf[1],terbuf[2],terbuf[3], \
                                                    terbuf[4],terbuf[5],terbuf[6],terbuf[7], \
                                                    terbuf[8],__FILE__,__LINE__);
                                        	#endif
											}
    									}
    									else
    									{
    									#ifdef MAIN_DEBUG
        									printf("base board serial cannot write,File: %s,Line: %d\n", \
												__FILE__,__LINE__);
    									#endif
    									}
									}//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))

									if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                	{//actively report is not probitted and connect successfully
                                    	memset(wlinfo,0,sizeof(wlinfo));
                                    	gettimeofday(&stt,NULL);
                                    	if (SUCCESS != wireless_info_report(wlinfo,stt.tv_sec,wlmark,0x02))
                                    	{
                                    	#ifdef MAIN_DEBUG
                                        	printf("wireless_info_report,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                    	else
                                    	{
                                        	write(csockfd,wlinfo,sizeof(wlinfo));
                                    	}
										update_event_list(&eventclass,&eventlog,1,69,stt.tv_sec,&markbit);
                                	}
									else
									{
										gettimeofday(&stt,NULL);
                                    	update_event_list(&eventclass,&eventlog,1,69,stt.tv_sec,&markbit);		
									}
								}

								mark += 5;
								continue;
							}//fastforward
							else if ((0x04 == *(pspbuf+mark+3)) && (4 == *(pspbuf+mark+1)))
							{//control info
								if ((markbit2 & 0x0002)||(markbit2 & 0x0008)||(markbit2 & 0x0010))
                                {//have key or software or configure tool is controlling signaler machine;
									mark += 6;
                                    continue;
                                }//have key or software or configure tool is controlling signaler machine;
								//if (lip == *(pspbuf+mark+2))
								if ((lip == *(pspbuf+mark+2)) && (!(markbit & 0x0400)))
								{
									tpp.head = 0xB9;
									tpp.size = 4;
                                	tpp.id = *(pspbuf+mark+2);
                                	tpp.type = 0x04;
									tpp.func[0] = *(pspbuf+mark+4);
									tpp.func[1] = 0;
									tpp.func[2] = 0;
									tpp.func[3] = 0;
									tpp.end = 0xED;
                                	memset(terbuf,0,sizeof(terbuf));
                                	if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
                                	{
       									//clean pipe
										while (1)
										{	
											memset(cdata,0,sizeof(cdata));
											if (read(conpipe[0],cdata,sizeof(cdata)) <= 0)
												break;
										}
										//end clean pipe

										if (!wait_write_serial(conpipe[1]))
										{
											if (write(conpipe[1],terbuf,sizeof(terbuf)) < 0)
											{
											#ifdef MAIN_DEBUG
												printf("write pipe err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										else
										{
										#ifdef MAIN_DEBUG
											printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										} 
                                	}//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
								}
								mark += 6;
								continue;
							}//control info
							else
							{//invalid info
								if (0xED == *(pspbuf+mark+5))
									mark += 6;
								if (0xED == *(pspbuf+mark+9))
									mark += 10;
								if (0xED == *(pspbuf+mark+4))
									mark += 5;
								
								continue;
							}//invalid info
						}//data of wireless terminal
						else if ((0xB6 == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+4)))
						{//else if ((0xB6 == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+3)))
							//send dianya/wendu/shidu to face board;
							dianya[1] = *(pspbuf+mark+1);
							dianya[2] = *(pspbuf+mark+2);
							dianya[3] = *(pspbuf+mark+3);
							if (!wait_write_serial(serial[2]))
							{
								if (write(serial[2],dianya,sizeof(dianya)) < 0)
								{
								#ifdef MAIN_DEBUG
									printf("write face board err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
							}
							else
							{
							#ifdef MAIN_DEBUG
								printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}
							if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                            {//actively report is not probitted and connect successfully
								unsigned char		object = 0;
								unsigned char		tma = *(pspbuf+mark+1);
								switch (tma)
								{
									case 0x01:
										object = 0xE4;
										break;
									case 0x02:
										object = 0xE5;
                                        break;
									case 0x03:
										object = 0xE6;
										break;
									case 0x04:
										object = 0xF1;
										break;
									case 0x05:
										object = 0xF2;
										break;
									case 0x06:
										object = 0xF3;
										break;
									default:
										break;
								}
								memset(err,0,sizeof(err));
								if (SUCCESS != environment_pack(err,object,*(pspbuf+mark+2),*(pspbuf+mark+3)))
								{
								#ifdef MAIN_DEBUG
                                    printf("environment_pack call err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
								}
								else
								{
									write(csockfd,err,sizeof(err));
								}		
							}//actively report is not probitted and connect successfully

							mark += 5;
							continue;
						}//else if ((0xB6 == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+3)))
						else if ((0xB1 == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+3)))
						{//status of drive board
							gettimeofday(&stt,NULL);
							switch (*(pspbuf+mark+2))
							{
								case 3://yellow flash since err
									elv = *(pspbuf+mark+1)+48;
									update_event_list(&eventclass,&eventlog,19,elv,stt.tv_sec,&markbit);
									if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                    {//actively report is not probitted and connect successfully
                                        memset(err,0,sizeof(err));
                                        if (SUCCESS != err_report(err,stt.tv_sec,19,elv))
                                        {
                                        #ifdef MAIN_DEBUG
                                            printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                        }
										else
                                        {
                                            write(csockfd,err,sizeof(err));
                                        }
                                    }
									break;
								case 4://yellow flash since close
									elv = *(pspbuf+mark+1)+64;
									update_event_list(&eventclass,&eventlog,19,elv,stt.tv_sec,&markbit);
									if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                    {//actively report is not probitted and connect successfully
                                        memset(err,0,sizeof(err));
                                        if (SUCCESS != err_report(err,stt.tv_sec,19,elv))
                                        {
                                        #ifdef MAIN_DEBUG
                                            printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                        }
										else
                                        {
                                            write(csockfd,err,sizeof(err));
                                        }
                                    }
									break;
								case 5://yellow flash since control
									elv = *(pspbuf+mark+1)+80;
									update_event_list(&eventclass,&eventlog,19,elv,stt.tv_sec,&markbit);
									if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                    {//actively report is not probitted and connect successfully
                                        memset(err,0,sizeof(err));
                                        if (SUCCESS != err_report(err,stt.tv_sec,19,elv))
                                        {
                                        #ifdef MAIN_DEBUG
                                            printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                        }
										else
                                        {
                                            write(csockfd,err,sizeof(err));
                                        }
                                    }
									break;
								default:
									break;
							}
							for (dbnum = 0; dbnum < DRIBOARDNUM; dbnum++)
							{
								if ((0 == dbstatus.driboardstatusList[dbnum].dribid) || \
									(*(pspbuf+mark+1) == dbstatus.driboardstatusList[dbnum].dribid))
								{
									dbstatus.driboardstatusList[dbnum].dribid = *(pspbuf+mark+1);
									dbstatus.driboardstatusList[dbnum].status = *(pspbuf+mark+2);
									break;
								}
							}						

							if ((markbit & 0x0010) && (markbit & 0x0002))
                            {
								dbsdata[4] = *(pspbuf+mark+1);
								dbsdata[5] = *(pspbuf+mark+2);
                                if (write(ssockfd,dbsdata,sizeof(dbsdata)) < 0)
                                {
                                    gettimeofday(&stt,NULL);
                                    update_event_list(&eventclass,&eventlog,1,20,stt.tv_sec,&markbit);
                                    if (SUCCESS!=generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                                    {
                                    #ifdef MAIN_DEBUG
                                        printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
									if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                    {//actively report is not probitted and connect successfully
                                        memset(err,0,sizeof(err));
                                        if (SUCCESS != err_report(err,stt.tv_sec,1,20))
                                        {
                                        #ifdef MAIN_DEBUG
                                            printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                        }
										else
                                        {
                                            write(csockfd,err,sizeof(err));
                                        }
                                    }
                                }
                            }

							mark += 4;
							continue;
						}//if ((0xB1 == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+3)))
						else if ((0xBD == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+2)))
						{//err yellow flash
							markbit |= 0x0080;
							mark += 3;
							if (3 == contmode)
							{
								errfb[1] = 4;//all red status of face board
							}
							else
							{
								contmode = 30;
								errfb[1] = contmode;
							}
							if (!wait_write_serial(serial[2]))
							{
								if (write(serial[2],errfb,sizeof(errfb)) < 0)
								{
								#ifdef MAIN_DEBUG
									printf("write face board err,File: %s,Line: %d\n",__FILE__,__LINE__);
									output_log("Main control,write face board err");
								#endif
								}
							}
							else
							{
							#ifdef MAIN_DEBUG
								printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}
							//added by sk on 20150707
							if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
							{
								unsigned char       sibuf[64] = {0};
								sinfo.conmode = errfb[1];
								if (SUCCESS != status_info_report(sibuf,&sinfo))
								{
								#ifdef MAIN_DEBUG
									printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
								else
								{
									write(csockfd,sibuf,sizeof(sibuf));
								}
							}
							if ((markbit & 0x0002) && (markbit & 0x0010))
							{
								memset(&timedown,0,sizeof(timedown));
								timedown.mode = errfb[1];
								timedown.pattern = 0;
								timedown.lampcolor = 0;
								timedown.lamptime = 0;
								timedown.phaseid = 0;
								timedown.stageline = 0;
								if (SUCCESS != timedown_data_to_conftool(sockfd,&timedown))
								{
								#ifdef MAIN_DEBUG
									printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							//end add

							continue;
						}//err yellow flash
						else if ((0xB3 == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+4)))
						{//lamp err and restore
							//save latest five lamp err or restore info; 
							if (fenum >= FIVEERRLAMPINFO)
							{
								fenum = 0;
							}
							felinfo.fiveerrlampinfoList[fenum].mark = *(pspbuf+mark+1);
							felinfo.fiveerrlampinfoList[fenum].chanid = *(pspbuf+mark+2);
							felinfo.fiveerrlampinfoList[fenum].errtype = *(pspbuf+mark+3);
							fenum += 1;							
	
							if (1 != *(pspbuf+mark+3))
							{//err or restore of green conflict will not send configure tool;
								if ((markbit & 0x0010) && (markbit & 0x0002))
								{
									lampdata[4] = *(pspbuf+mark+2);
									lampdata[5] = *(pspbuf+mark+1);
									lampdata[6] = *(pspbuf+mark+3);
									if (write(ssockfd,lampdata,sizeof(lampdata)) < 0)
									{
										gettimeofday(&stt,NULL);
                                    	update_event_list(&eventclass,&eventlog,1,20,stt.tv_sec,&markbit);
                                      if(SUCCESS!=generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                                    	{
                                    	#ifdef MAIN_DEBUG
                                        	printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                    	{//actively report is not probitted and connect successfully
                                        	memset(err,0,sizeof(err));
                                        	if (SUCCESS != err_report(err,stt.tv_sec,1,20))
                                        	{
                                        	#ifdef MAIN_DEBUG
                                            	printf("err_report call err,File: %s,Line: %d\n", \
														__FILE__,__LINE__);
                                        	#endif
                                        	}
											else
                                            {
                                                write(csockfd,err,sizeof(err));
                                            }
                                    	}	
									}
								}
								//update lamp err type
								if (SUCCESS != update_lamp_status(&lamperrtype,*(pspbuf+mark+2), \
											*(pspbuf+mark+3),*(pspbuf+mark+1),&markbit))
								{
								#ifdef MAIN_DEBUG
									printf("update lamp status err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}					
							}//err or restore of green conflict will not send configure tool;

							if (markbit & 0x0020)
							{//send  lamp err or restore info to face board when info updated
								unsigned char			errdata[5] = {0};
								errdata[0] = 0xC4;
								errdata[1] = *(pspbuf+mark+1);
								errdata[2] = *(pspbuf+mark+2);
								errdata[3] = *(pspbuf+mark+3);
								errdata[4] = 0xED;
								if (!wait_write_serial(serial[2]))
								{
									if (write(serial[2],errdata,sizeof(errdata)) < 0)
									{
									#ifdef MAIN_DEBUG
										printf("write err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
										markbit |= 0x0800;
										gettimeofday(&stt,NULL);
										update_event_list(&eventclass,&eventlog,1,16,stt.tv_sec,&markbit);
                						if (SUCCESS != \
											generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                						{
                						#ifdef MAIN_DEBUG
                    						printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                						#endif
                						}
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                    	{//actively report is not probitted and connect successfully
                                        	memset(err,0,sizeof(err));
                                        	if (SUCCESS != err_report(err,stt.tv_sec,1,16))
                                        	{
                                        	#ifdef MAIN_DEBUG
                                            	printf("err_report call err,File: %s,Line: %d\n", \
														__FILE__,__LINE__);
                                        	#endif
                                        	}
											else
                                            {
                                                write(csockfd,err,sizeof(err));
                                            }
                                    	}
									}
								}
								else
								{
								#ifdef MAIN_DEBUG
									printf("can't be written,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}//send lamp err or restore info to face board when info updated

							gettimeofday(&stt,NULL);
							elv = *(pspbuf+mark+2);
							if (0 == *(pspbuf+mark+1))
							{//lamp err restore
								switch (*(pspbuf+mark+3))
								{
									case 1://green conflict
										update_event_list(&eventclass,&eventlog,2,1,stt.tv_sec,&markbit);
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                    	{//actively report is not probitted and connect successfully
                                        	memset(err,0,sizeof(err));
                                        	if (SUCCESS != err_report(err,stt.tv_sec,2,1))
                                        	{
                                        	#ifdef MAIN_DEBUG
                                            	printf("err_report call err,File: %s,Line: %d\n", \
														__FILE__,__LINE__);
                                        	#endif
                                        	}
											else
                                            {
                                                write(csockfd,err,sizeof(err));
                                            }
                                    	}
										break;
									case 2://red and green are lighted
										update_event_list(&eventclass,&eventlog,4,elv,stt.tv_sec,&markbit);
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        {//actively report is not probitted and connect successfully
                                            memset(err,0,sizeof(err));
                                            if (SUCCESS != err_report(err,stt.tv_sec,4,elv))
                                            {
                                            #ifdef MAIN_DEBUG
                                                printf("err_report call err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                            #endif
                                            }
											else
                                            {
                                                write(csockfd,err,sizeof(err));
                                            }
                                        }
										break;
									case 3://red lamp is not lighted;
										update_event_list(&eventclass,&eventlog,5,elv,stt.tv_sec,&markbit);
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        {//actively report is not probitted and connect successfully
                                            memset(err,0,sizeof(err));
                                            if (SUCCESS != err_report(err,stt.tv_sec,5,elv))
                                            {
                                            #ifdef MAIN_DEBUG
                                                printf("err_report call err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                            #endif
                                            }
											else
                                            {
                                                write(csockfd,err,sizeof(err));
                                            }
                                        }
										break;
									case 4://red lamp is lighted in error;
										update_event_list(&eventclass,&eventlog,6,elv,stt.tv_sec,&markbit);
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        {//actively report is not probitted and connect successfully
                                            memset(err,0,sizeof(err));
                                            if (SUCCESS != err_report(err,stt.tv_sec,6,elv))
                                            {
                                            #ifdef MAIN_DEBUG
                                                printf("err_report call err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                            #endif
                                            }
											else
                                            {
                                                write(csockfd,err,sizeof(err));
                                            }
                                        }
										break;
									case 5://yellow lamp is not lighted;
										update_event_list(&eventclass,&eventlog,7,elv,stt.tv_sec,&markbit);
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        {//actively report is not probitted and connect successfully
                                            memset(err,0,sizeof(err));
                                            if (SUCCESS != err_report(err,stt.tv_sec,7,elv))
                                            {
                                            #ifdef MAIN_DEBUG
                                                printf("err_report call err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                            #endif
                                            }
											else
                                            {
                                                write(csockfd,err,sizeof(err));
                                            }
                                        }
										break;
									case 6://yellow lamp is lighted in error
										update_event_list(&eventclass,&eventlog,8,elv,stt.tv_sec,&markbit);
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        {//actively report is not probitted and connect successfully
                                            memset(err,0,sizeof(err));
                                            if (SUCCESS != err_report(err,stt.tv_sec,8,elv))
                                            {
                                            #ifdef MAIN_DEBUG
                                                printf("err_report call err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                            #endif
                                            }
											else
                                            {
                                                write(csockfd,err,sizeof(err));
                                            }
                                        }
										break;
									case 7://green lamp is not lighted
										update_event_list(&eventclass,&eventlog,9,elv,stt.tv_sec,&markbit);
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        {//actively report is not probitted and connect successfully
                                            memset(err,0,sizeof(err));
                                            if (SUCCESS != err_report(err,stt.tv_sec,9,elv))
                                            {
                                            #ifdef MAIN_DEBUG
                                                printf("err_report call err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                            #endif
                                            }
											else
                                            {
                                                write(csockfd,err,sizeof(err));
                                            }
                                        }
										break;
									case 8: //green lamp is lighted in error
										update_event_list(&eventclass,&eventlog,10,elv,stt.tv_sec,&markbit);
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        {//actively report is not probitted and connect successfully
                                            memset(err,0,sizeof(err));
                                            if (SUCCESS != err_report(err,stt.tv_sec,10,elv))
                                            {
                                            #ifdef MAIN_DEBUG
                                                printf("err_report call err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                            #endif
                                            }
											else
        									{
            									write(csockfd,err,sizeof(err));
        									}
                                        }
										break;
									default:
										break;
								}
							}//lamp err restore
							if (1 == *(pspbuf+mark+1))
							{//lamp err
								switch (*(pspbuf+mark+3))
								{
									case 1://green conflict
										update_event_list(&eventclass,&eventlog,3, \
															/*1*/*(pspbuf+mark+2),stt.tv_sec,&markbit);
										if (99 != *(pspbuf+mark+2))
										{
											gcbit |= 0x00000001 << (*(pspbuf+mark+2) - 1);
										}
										else
										{//if (99 == *(pspbuf+mark+2))
											if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        	{//actively report is not probitted and connect successfully
												memset(gcinfo,0,sizeof(gcinfo));
												if (SUCCESS != green_conflict_pack(gcinfo,stt.tv_sec,gcbit))
												{
												#ifdef MAIN_DEBUG
													printf("green_conflict _report call err, \
															File: %s,Line: %d\n",__FILE__,__LINE__);
												#endif
												}
												else
												{
													write(csockfd,gcinfo,sizeof(gcinfo));
												}	
											}//actively report is not probitted and connect successfully
											gcbit = 0;
										}//if (99 == *(pspbuf+mark+2))	
										
										break;
									case 2://red and green are lighted
										update_event_list(&eventclass,&eventlog,11,elv,stt.tv_sec,&markbit);
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        {//actively report is not probitted and connect successfully
                                            memset(err,0,sizeof(err));
                                            if (SUCCESS != err_report(err,stt.tv_sec,11,elv))
                                            {
                                            #ifdef MAIN_DEBUG
                                                printf("err_report call err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                            #endif
                                            }
											else
        									{
            									write(csockfd,err,sizeof(err));
        									}
                                        }
										break;
									case 3://red lamp is not lighted;
										update_event_list(&eventclass,&eventlog,12,elv,stt.tv_sec,&markbit);
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        {//actively report is not probitted and connect successfully
                                            memset(err,0,sizeof(err));
                                            if (SUCCESS != err_report(err,stt.tv_sec,12,elv))
                                            {
                                            #ifdef MAIN_DEBUG
                                                printf("err_report call err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                            #endif
                                            }
											else
        									{
            									write(csockfd,err,sizeof(err));
        									}
                                        }
										break;
									case 4://red lamp is lighted in error;
										update_event_list(&eventclass,&eventlog,13,elv,stt.tv_sec,&markbit);
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        {//actively report is not probitted and connect successfully
                                            memset(err,0,sizeof(err));
                                            if (SUCCESS != err_report(err,stt.tv_sec,13,elv))
                                            {
                                         //   #ifdef MAIN_DEBUG
                                          //      printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                         //   #endif
                                            }
											else
        									{
            									write(csockfd,err,sizeof(err));
        									}
                                        }
										break;
									case 5://yellow lamp is not lighted;
										update_event_list(&eventclass,&eventlog,14,elv,stt.tv_sec,&markbit);
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        {//actively report is not probitted and connect successfully
                                            memset(err,0,sizeof(err));
                                            if (SUCCESS != err_report(err,stt.tv_sec,14,elv))
                                            {
                                            #ifdef MAIN_DEBUG
                                                printf("err_report call err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                            #endif
                                            }
											else
        									{
            									write(csockfd,err,sizeof(err));
        									}
                                        }
										
										break;
									case 6://yellow lamp is lighted in error
										update_event_list(&eventclass,&eventlog,15,elv,stt.tv_sec,&markbit);
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        {//actively report is not probitted and connect successfully
                                            memset(err,0,sizeof(err));
                                            if (SUCCESS != err_report(err,stt.tv_sec,15,elv))
                                            {
                                            #ifdef MAIN_DEBUG
                                                printf("err_report call err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                            #endif
                                            }
											else
        									{
            									write(csockfd,err,sizeof(err));
        									}
                                        }
										
										break;
									case 7://green lamp is not lighted
										update_event_list(&eventclass,&eventlog,16,elv,stt.tv_sec,&markbit);
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        {//actively report is not probitted and connect successfully
                                            memset(err,0,sizeof(err));
                                            if (SUCCESS != err_report(err,stt.tv_sec,16,elv))
                                            {
                                            #ifdef MAIN_DEBUG
                                                printf("err_report call err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                            #endif
                                            }
											else
        									{
            									write(csockfd,err,sizeof(err));
        									}
                                        }
										
										break;
									case 8://green lamp is lighted in error
										update_event_list(&eventclass,&eventlog,17,elv,stt.tv_sec,&markbit);
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        {//actively report is not probitted and connect successfully
                                            memset(err,0,sizeof(err));
                                            if (SUCCESS != err_report(err,stt.tv_sec,17,elv))
                                            {
                                            #ifdef MAIN_DEBUG
                                                printf("err_report call err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                            #endif
                                            }
											else
        									{
            									write(csockfd,err,sizeof(err));
        									}
                                        }
										break;
									default:
										break;
								}
							}//lamp err
							mark += 5;
							continue;
						}//if ((0xB3 == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+4)))
						else if ((0xB4 == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+3)))
						{//CAN ERR
							contmode = 31;
							errfb[1] = contmode;
                            if (!wait_write_serial(serial[2]))
                            {
                                if (write(serial[2],errfb,sizeof(errfb)) < 0)
                                {
                                #ifdef MAIN_DEBUG
                                    printf("write face board err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
                            }
                            else
                            {
                            #ifdef MAIN_DEBUG
                                printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }

							gettimeofday(&stt,NULL);
							elv = *(pspbuf+mark+2);
							if (0 == elv)
							{//main control board
								switch (*(pspbuf+mark+1))
								{
									case 1://err
										update_event_list(&eventclass,&eventlog,18,33,stt.tv_sec,&markbit);
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        {//actively report is not probitted and connect successfully
                                            memset(err,0,sizeof(err));
                                            if (SUCCESS != err_report(err,stt.tv_sec,18,33))
                                            {
                                            #ifdef MAIN_DEBUG
                                                printf("err_report call err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                            #endif
                                            }
											else
        									{
            									write(csockfd,err,sizeof(err));
        									}
                                        }
										break;
									case 0://normal
										update_event_list(&eventclass,&eventlog,18,34,stt.tv_sec,&markbit);
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        {//actively report is not probitted and connect successfully
                                            memset(err,0,sizeof(err));
                                            if (SUCCESS != err_report(err,stt.tv_sec,18,34))
                                            {
                                            #ifdef MAIN_DEBUG
                                                printf("err_report call err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                            #endif
                                            }
											else
        									{
            									write(csockfd,err,sizeof(err));
        									}
                                        }
										break;
									default:
										break;
								}
							}//main control board
							else
							{//drive board
								switch (*(pspbuf+mark+1))
								{
									case 1://err
										update_event_list(&eventclass,&eventlog,18,elv,stt.tv_sec,&markbit);
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        {//actively report is not probitted and connect successfully
                                            memset(err,0,sizeof(err));
                                            if (SUCCESS != err_report(err,stt.tv_sec,18,elv))
                                            {
                                            #ifdef MAIN_DEBUG
                                                printf("err_report call err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                            #endif
                                            }
											else
        									{
            									write(csockfd,err,sizeof(err));
        									}
                                        }
										break;
									case 0://normal
										update_event_list(&eventclass,&eventlog,18,elv+16,stt.tv_sec,&markbit);
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        {//actively report is not probitted and connect successfully
                                            memset(err,0,sizeof(err));
                                            if (SUCCESS != err_report(err,stt.tv_sec,18,elv+16))
                                            {
                                            #ifdef MAIN_DEBUG
                                                printf("err_report call err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                            #endif
                                            }
											else
        									{
            									write(csockfd,err,sizeof(err));
        									}
                                        }
										break;
									default:
										break;
								}
							}//drive board
							mark += 4;
							continue;
						}//if ((0xB4 == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+4)))
						#ifdef BASE_BOARD_CONTROL
						else if ((0xDA == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+2)))
						{//manual control
							if ((markbit2 & 0x0004)||(markbit2 & 0x0008)||(markbit2 & 0x0010))
							{//wireless or software or configure tool is control signaler machine;
								mark += 3;
								continue;
							}//wireless or software or configure tool is control signaler machine;
							unsigned char			cond[3] = {0};
							cond[0] = *(pspbuf+mark);
							cond[1] = *(pspbuf+mark+1);
							cond[2] = *(pspbuf+mark+2);

							//clean pipe
							while (1)
							{
								memset(cdata,0,sizeof(cdata));
								if (read(conpipe[0],cdata,sizeof(cdata)) <= 0)
									break;
							}
							//end clean pipe
							if (!wait_write_serial(conpipe[1]))
							{
								if (write(conpipe[1],cond,sizeof(cond)) < 0)
								{
								#ifdef MAIN_DEBUG
									printf("write pipe error,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							else
							{
							#ifdef MAIN_DEBUG
								printf("can not write,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
							mark += 3;
							continue;
						}//manual control
						#endif
						else if((0xF1 == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+5)))//一体化信号机接受
						{
							//printf("interate control receive data \n");
							//printf("******************num:%d,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,%s,%d**********************\n", \
							//num,*(pspbuf+mark),*(pspbuf+mark+1),*(pspbuf+mark+2),*(pspbuf+mark+3),*(pspbuf+mark+4),*(pspbuf+mark+5),__FILE__,__LINE__);	
							if (0x01 == *(pspbuf+mark+1))//车辆进入
							{
								//clean data of pipe
								unsigned char senddatabuf[6]={0};
								senddatabuf[0]=*(pspbuf+mark);
								senddatabuf[1]=*(pspbuf+mark+1);
								senddatabuf[2]=*(pspbuf+mark+2);
								senddatabuf[3]=*(pspbuf+mark+3);
								senddatabuf[4]=*(pspbuf+mark+4);
								senddatabuf[5]=*(pspbuf+mark+5);
							
								while (1)
								{	
									memset(cdata,0,sizeof(cdata));
									if (read(flowpipe[0],cdata,sizeof(cdata)) <= 0)
										break;
								}
								//end clean pipe

								if (!wait_write_serial(flowpipe[1]))
								{
									if (write(flowpipe[1],senddatabuf,sizeof(senddatabuf)) < 0)
									{
										#ifdef MAIN_DEBUG
											printf("write pipe err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
									}
								}
								else
								{
										#ifdef MAIN_DEBUG
											printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
								}
								while (1)
								{
									memset(cdata,0,sizeof(cdata));
									if (read(pendpipe[0],cdata,sizeof(cdata)) <= 0)
										break;
								}
								if (!wait_write_serial(pendpipe[1]))
								{
									if (write(pendpipe[1],senddatabuf,sizeof(senddatabuf)) < 0)
									{
									#ifdef MAIN_DEBUG
										printf("write pipe err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								else
								{
									#ifdef MAIN_DEBUG
									printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
								}
							}
							else if(0x06 == *(pspbuf+mark+1))//雷达故障
							{
								unsigned char fangxiang = *(pspbuf+mark+2);
								if(1 == fangxiang)
								{
									unsigned char cradioerroinfo[] = "北方向雷达出现故障";
									output_log(cradioerroinfo);	
								}
								else if(2 == fangxiang)
								{
									unsigned char cradioerroinfo[] = "东方向雷达出现故障";
									output_log(cradioerroinfo);	
								}
								else if(3 == fangxiang)
								{
									unsigned char cradioerroinfo[] = "南方向雷达出现故障";
									output_log(cradioerroinfo);	
								}
								else if(4 == fangxiang)
								{
									unsigned char cradioerroinfo[] = "西方向雷达出现故障";
									output_log(cradioerroinfo);	
								}
								else
								{
									unsigned char cradioerroinfo[100] = {0};
									sprintf(cradioerroinfo,"%c方向雷达出现故障",fangxiang);
									output_log(cradioerroinfo);	
								}
								
							}	
							mark += 6;
							continue;
						}
						else
						{
							mark += 1;
							continue;
						}
					}//1,while loop
				}//num >= 0
				continue;
			}//base board serial port
			if (FD_ISSET(serial[1],&nRead))
			{//vehicle check board serial port
				memset(spbuf,0,sizeof(spbuf));
				mark = 0;
				pspbuf = spbuf;
				num = read(serial[1],spbuf,sizeof(spbuf));
				if (num > sizeof(spbuf))
					continue;
				if (num > 0)
				{
					printf("******************num:%d,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,%s,%d**********************\n", \
							num,spbuf[0],spbuf[1],spbuf[2],spbuf[3],spbuf[4],spbuf[5],__FILE__,__LINE__);			

					mark = 0;
					//clean data of pipe
					while (1)
					{	
						memset(cdata,0,sizeof(cdata));
						if (read(flowpipe[0],cdata,sizeof(cdata)) <= 0)
							break;
					}
					//end clean pipe

					if (!wait_write_serial(flowpipe[1]))
					{
						if (write(flowpipe[1],spbuf,sizeof(spbuf)) < 0)
						{
						#ifdef MAIN_DEBUG
							printf("write pipe err,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
					}
					else
					{
					#ifdef MAIN_DEBUG
						printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					while (1)
					{
						memset(cdata,0,sizeof(cdata));
						if (read(pendpipe[0],cdata,sizeof(cdata)) <= 0)
							break;
					}
					if (!wait_write_serial(pendpipe[1]))
					{
						if (write(pendpipe[1],spbuf,sizeof(spbuf)) < 0)
						{
						#ifdef MAIN_DEBUG
							printf("write pipe err,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
					}
					else
					{
					#ifdef MAIN_DEBUG
						printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					
					if (0x7E == *pspbuf)
						continue;
					//以上判断目的是为了减少不必要的处理，因为0x7E是属于雷达检测器的数据，
					//在后面的智能自适应模块中处理；因此主控模块只要转发就行了；

					mark = 0;
					if ((0xDC == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+2)))
					{//接受边缘计算设备发送过来的黄闪指令 0x01 黄闪      
					// 0x02 解除
						if ((markbit2 & 0x0004)||(markbit2 & 0x0008)||(markbit2 & 0x0010)||(markbit2 & 0x0002))
						{//wireless or software or configure tool or key is control signaler machine;
							continue;
						}//wireless or software or configure tool or key is control signaler machine;
						unsigned char			cond[3] = {0};
						if (0x01 == *(pspbuf+mark+1))
						{//control come from edge device;
							cond[0] = 0xBA;
							cond[1] = 0x01;
							cond[2] = 0xBD;
							
							//clean pipe
							while (1)
							{
								memset(cdata,0,sizeof(cdata));
								if (read(conpipe[0],cdata,sizeof(cdata)) <= 0)
									break;
							}
							//end clean pipe
							if (!wait_write_serial(conpipe[1]))
							{
								if (write(conpipe[1],cond,sizeof(cond)) < 0)
								{
									#ifdef MAIN_DEBUG
									printf("write pipe error,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
								}
							}
							else
							{
								#ifdef MAIN_DEBUG
								printf("can not write,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
							}
							continue;
						}//control come from edge device;
						if (0x02 == *(pspbuf+mark+1))
						{
					
							cond[0] = 0xBA;
							cond[1] = 0x02;
							cond[2] = 0xBD;
							
							//clean pipe
							while (1)
							{
								memset(cdata,0,sizeof(cdata));
								if (read(conpipe[0],cdata,sizeof(cdata)) <= 0)
									break;
							}
							//end clean pipe
							if (!wait_write_serial(conpipe[1]))
							{
								if (write(conpipe[1],cond,sizeof(cond)) < 0)
								{
									#ifdef MAIN_DEBUG
									printf("write pipe error,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
								}
							}
							else
							{
								#ifdef MAIN_DEBUG
								printf("can not write,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
							}
							continue;
						
						}
						continue;
					}



					/*mark = 0;
					if ((0xDC == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+2)))
					{//接受边缘计算设备发送过来的管控指令,因此主控模块只要转发就行了;
						if ((markbit2 & 0x0004)||(markbit2 & 0x0008)||(markbit2 & 0x0010))
						{//wireless or software or configure tool is control signaler machine;
							continue;
						}//wireless or software or configure tool is control signaler machine;
						unsigned char			cond[3] = {0};
						struct timeval          spatime;
						unsigned char			jn = 0;
						if (0x01 == *(pspbuf+mark+1))
						{//control come from edge device;
							if (markbit2 & 0x0002)
							{//has been status of key control
								continue;
							}//has been status of key control
							cond[0] = 0xDA;
							cond[1] = 0x01;
							cond[2] = 0xED;
							
							//clean pipe
							while (1)
							{
								memset(cdata,0,sizeof(cdata));
								if (read(conpipe[0],cdata,sizeof(cdata)) <= 0)
									break;
							}
							//end clean pipe
							if (!wait_write_serial(conpipe[1]))
							{
								if (write(conpipe[1],cond,sizeof(cond)) < 0)
								{
								#ifdef MAIN_DEBUG
									printf("write pipe error,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							else
							{
							#ifdef MAIN_DEBUG
								printf("can not write,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
							continue;
						}//control come from edge device;
						if (0x02 == *(pspbuf+mark+1))
						{//restore normal run
							if (markbit2 & 0x0002)
							{//has been status of key control
								cond[0] = 0xDA;
								cond[1] = 0x01;
								cond[2] = 0xED;
								
								//clean pipe
								while (1)
								{
									memset(cdata,0,sizeof(cdata));
									if (read(conpipe[0],cdata,sizeof(cdata)) <= 0)
										break;
								}
								//end clean pipe
								if (!wait_write_serial(conpipe[1]))
								{
									if (write(conpipe[1],cond,sizeof(cond)) < 0)
									{
									#ifdef MAIN_DEBUG
										printf("write pipe error,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								else
								{
								#ifdef MAIN_DEBUG
									printf("can not write,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
								continue;
							}//has been status of key control
							else
							{
								continue;
							}
						}//restore normal run
						if (0x03 == *(pspbuf+mark+1))
						{//control of yellow flash
							if (markbit2 & 0x0002)
							{//has been status of key control
								cond[0] = 0xDA;
								cond[1] = 0x03;
								cond[2] = 0xED;
								
								//clean pipe
								while (1)
								{
									memset(cdata,0,sizeof(cdata));
									if (read(conpipe[0],cdata,sizeof(cdata)) <= 0)
										break;
								}
								//end clean pipe
								if (!wait_write_serial(conpipe[1]))
								{
									if (write(conpipe[1],cond,sizeof(cond)) < 0)
									{
									#ifdef MAIN_DEBUG
										printf("write pipe error,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								else
								{
								#ifdef MAIN_DEBUG
									printf("can not write,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
								continue;
							}//has been status of key control
							else
							{//not status of key control,need to wait;
								jn = 0;
								while (1)
								{//while (1)
									spatime.tv_sec = 0;
                                	spatime.tv_usec = 500000;
									select(0,NULL,NULL,NULL,&spatime);

									if (markbit2 & 0x0002)
									{//has been status of key control
										cond[0] = 0xDA;
										cond[1] = 0x03;
										cond[2] = 0xED;
										//clean pipe
										while (1)
										{
											memset(cdata,0,sizeof(cdata));
											if (read(conpipe[0],cdata,sizeof(cdata)) <= 0)
												break;
										}
										//end clean pipe
										if (!wait_write_serial(conpipe[1]))
										{
											if (write(conpipe[1],cond,sizeof(cond)) < 0)
											{
											#ifdef MAIN_DEBUG
												printf("write pipe error,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										else
										{
										#ifdef MAIN_DEBUG
											printf("can not write,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}										

										break;
									}//has been status of key control

									jn += 1;
									if (jn >= 20)
									{//10 seconds to wait;
										break;
									}//10 seconds to wait;	
								}//while (1)
								continue;
							}//not status of key control,need to wait;
						}//control of yellow flash
						continue;
					}//接受边缘计算设备发送过来的管控指令,因此主控模块只要转发就行了;
					*/
					mark = 0;
					while(1)
					{
						if (mark >= num)
							break;
						if (0xF2 == *(pspbuf+mark))
						{//RFID info of err of normal
							unsigned char			bn = 0;//byte number;
							bn = *(pspbuf+mark+1);
							if (0xED == *(pspbuf+mark+bn+1))
							{//RFID info
								if (0x01 == *(pspbuf+mark+2))
								{//RFID err
									if (0x01 == *(pspbuf+mark+3))
									{//North Reader and Writer err
										rfidm |= 0x01;
										gettimeofday(&stt,NULL);
										update_event_list(&eventclass,&eventlog,1,97,stt.tv_sec,&markbit);
										if(SUCCESS!=generate_event_file(&eventclass,&eventlog, \
														softevent,&markbit))
										{
										#ifdef MAIN_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
										{//actively report is not probitted and connect successfully
											memset(err,0,sizeof(err));
											if (SUCCESS != err_report(err,stt.tv_sec,0x18,0x01))
											{
											#ifdef MAIN_DEBUG
												printf("err_report call err,File: %s,Line: %d\n", \
															__FILE__,__LINE__);
											#endif
											}
											else
											{
												write(csockfd,err,sizeof(err));
											}
										}
									}//North Reader and Writer err
									else if (0x02 == *(pspbuf+mark+3))
									{//East Reader and Writer err
										rfidm |= 0x02;
										gettimeofday(&stt,NULL);
										update_event_list(&eventclass,&eventlog,1,98,stt.tv_sec,&markbit);
										if(SUCCESS!=generate_event_file(&eventclass,&eventlog, \
														softevent,&markbit))
										{
										#ifdef MAIN_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
										{//actively report is not probitted and connect successfully
											memset(err,0,sizeof(err));
											if (SUCCESS != err_report(err,stt.tv_sec,0x18,0x02))
											{
											#ifdef MAIN_DEBUG
												printf("err_report call err,File: %s,Line: %d\n", \
															__FILE__,__LINE__);
											#endif
											}
											else
											{
												write(csockfd,err,sizeof(err));
											}
										}
									}//East Reader and Writer err
									else if (0x03 == *(pspbuf+mark+3))
                                    {//South Reader and Writer err
										rfidm |= 0x04;
										gettimeofday(&stt,NULL);
										update_event_list(&eventclass,&eventlog,1,99,stt.tv_sec,&markbit);
										if(SUCCESS!=generate_event_file(&eventclass,&eventlog, \
														softevent,&markbit))
										{
										#ifdef MAIN_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
										{//actively report is not probitted and connect successfully
											memset(err,0,sizeof(err));
											if (SUCCESS != err_report(err,stt.tv_sec,0x18,0x03))
											{
											#ifdef MAIN_DEBUG
												printf("err_report call err,File: %s,Line: %d\n", \
															__FILE__,__LINE__);
											#endif
											}
											else
											{
												write(csockfd,err,sizeof(err));
											}
										}
                                    }//South Reader and Writer err
									else if (0x04 == *(pspbuf+mark+3))
                                    {//West Reader and Writer err
										rfidm |= 0x08;
										gettimeofday(&stt,NULL);
										update_event_list(&eventclass,&eventlog,1,100,stt.tv_sec,&markbit);
										if(SUCCESS!=generate_event_file(&eventclass,&eventlog, \
														softevent,&markbit))
										{
										#ifdef MAIN_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
										{//actively report is not probitted and connect successfully
											memset(err,0,sizeof(err));
											if (SUCCESS != err_report(err,stt.tv_sec,0x18,0x04))
											{
											#ifdef MAIN_DEBUG
												printf("err_report call err,File: %s,Line: %d\n", \
															__FILE__,__LINE__);
											#endif
											}
											else
											{
												write(csockfd,err,sizeof(err));
											}
										}
                                    }//West Reader and Writer err
								}//RFID err
								if (0x02 == *(pspbuf+mark+2))
								{//RFID normal
									if (0x01 == *(pspbuf+mark+3))
                                    {//North Reader and Writer has been normal
										rfidm &= 0xfe;	
										gettimeofday(&stt,NULL);
										update_event_list(&eventclass,&eventlog,1,101,stt.tv_sec,&markbit);
										if(SUCCESS!=generate_event_file(&eventclass,&eventlog, \
														softevent,&markbit))
										{
										#ifdef MAIN_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
										{//actively report is not probitted and connect successfully
											memset(err,0,sizeof(err));
											if (SUCCESS != err_report(err,stt.tv_sec,0x18,0x05))
											{
											#ifdef MAIN_DEBUG
												printf("err_report call err,File: %s,Line: %d\n", \
															__FILE__,__LINE__);
											#endif
											}
											else
											{
												write(csockfd,err,sizeof(err));
											}
										}
                                    }//North Reader and Writer has been normal
                                    else if (0x02 == *(pspbuf+mark+3))
                                    {//East Reader and Writer has been normal
										rfidm &= 0xfd;
										gettimeofday(&stt,NULL);
										update_event_list(&eventclass,&eventlog,1,102,stt.tv_sec,&markbit);
										if(SUCCESS!=generate_event_file(&eventclass,&eventlog, \
														softevent,&markbit))
										{
										#ifdef MAIN_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
										{//actively report is not probitted and connect successfully
											memset(err,0,sizeof(err));
											if (SUCCESS != err_report(err,stt.tv_sec,0x18,0x06))
											{
											#ifdef MAIN_DEBUG
												printf("err_report call err,File: %s,Line: %d\n", \
															__FILE__,__LINE__);
											#endif
											}
											else
											{
												write(csockfd,err,sizeof(err));
											}
										}
                                    }//East Reader and Writer has been normal
                                    else if (0x03 == *(pspbuf+mark+3))
                                    {//South Reader and Writer has been normal
										rfidm &= 0xfb;
										gettimeofday(&stt,NULL);
										update_event_list(&eventclass,&eventlog,1,103,stt.tv_sec,&markbit);
										if(SUCCESS!=generate_event_file(&eventclass,&eventlog, \
														softevent,&markbit))
										{
										#ifdef MAIN_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
										{//actively report is not probitted and connect successfully
											memset(err,0,sizeof(err));
											if (SUCCESS != err_report(err,stt.tv_sec,0x18,0x07))
											{
											#ifdef MAIN_DEBUG
												printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
											}
											else
											{
												write(csockfd,err,sizeof(err));
											}
										}
                                    }//South Reader and Writer has been normal
                                    else if (0x04 == *(pspbuf+mark+3))
                                    {//West Reader and Writer has been normal
										rfidm &= 0xf7;
										gettimeofday(&stt,NULL);
										update_event_list(&eventclass,&eventlog,1,104,stt.tv_sec,&markbit);
										if(SUCCESS!=generate_event_file(&eventclass,&eventlog, \
														softevent,&markbit))
										{
										#ifdef MAIN_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
										{//actively report is not probitted and connect successfully
											memset(err,0,sizeof(err));
											if (SUCCESS != err_report(err,stt.tv_sec,0x18,0x08))
											{
											#ifdef MAIN_DEBUG
												printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
											}
											else
											{
												write(csockfd,err,sizeof(err));
											}
										}
                                    }//West Reader and Writer has been normal
								}//RFID normal								
							}//RFID info
							mark += bn;
							mark += 2;
							continue;
						}//RFID info of err or normal
						else if ((0xB9 == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+5))) 
						{//data of RFID,added by sk on 20150616
							if ((0x04 == *(pspbuf+mark+3)) && (4 == *(pspbuf+mark+1)))
							{//control info
								if ((markbit2 & 0x0002)||(markbit2 & 0x0008)||(markbit2 & 0x0010))
                                {//have key or software or configure or wireless is controlling machine;
									mark += 6;
                                    continue;
                                }//have key or software or configure tool is controlling signaler machine;
								//if (lip == *(pspbuf+mark+2))
								if (!(markbit & 0x0400))
								{
									tpp.head = 0xB9;
									tpp.size = 4;
                                	tpp.id = 0;
                                	tpp.type = 0x04;
									tpp.func[0] = *(pspbuf+mark+4);
									tpp.func[1] = 0;
									tpp.func[2] = 0;
									tpp.func[3] = 0;
									tpp.end = 0xED;
                                	memset(terbuf,0,sizeof(terbuf));
                                	if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
                                	{
       									//clean pipe
										while (1)
										{	
											memset(cdata,0,sizeof(cdata));
											if (read(conpipe[0],cdata,sizeof(cdata)) <= 0)
												break;
										}
										//end clean pipe

										if (!wait_write_serial(conpipe[1]))
										{
											if (write(conpipe[1],terbuf,sizeof(terbuf)) < 0)
											{
											#ifdef MAIN_DEBUG
												printf("write pipe err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											else
											{
												if (0x01 == *(pspbuf+mark+4))
												{//rfid control
													markbit2 |= 0x0100;	
												}//rfid control
												if (0x00 == *(pspbuf+mark+4))
												{//restore
													markbit2 &= 0xfeff;
												}//restore
												tpp.head = 0xA9;
												tpp.size = 4;
												tpp.id = 0;
												tpp.type = 0x04;
												tpp.func[0] = *(pspbuf+mark+4);
												tpp.func[1] = 0;
												tpp.func[2] = 0;
												tpp.func[3] = 0;
												tpp.end = 0xED;
												memset(terbuf,0,sizeof(terbuf));
												if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
												{
													if (!wait_write_serial(serial[1]))
    												{
        												if (write(serial[1],terbuf,sizeof(terbuf)) < 0)
        												{
        												#ifdef MAIN_DEBUG
            												printf("write base board serial err,File: %s,Line: %d\n", __FILE__,__LINE__);
        												#endif
        												}
														else
														{
														#ifdef MAIN_DEBUG
                                        				printf("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \
                                                    	File: %s,Line: %d\n",terbuf[0],terbuf[1],terbuf[2], \
														terbuf[3],terbuf[4],terbuf[5],__FILE__,__LINE__);
														#endif
														}
    												}
    												else
    												{
    												#ifdef MAIN_DEBUG
        												printf("base board serial cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
    												#endif
    												}
												}//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
											}
										}
										else
										{
										#ifdef MAIN_DEBUG
											printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										} 
                                	}//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
								}
								mark += 6;
								continue;
							}//control info
							else
							{//invalid info
								if (0xED == *(pspbuf+mark+5))
									mark += 6;
								if (0xED == *(pspbuf+mark+9))
									mark += 10;
								if (0xED == *(pspbuf+mark+4))
									mark += 5;
								
								continue;
							}//invalid info
						}//data of RFID,added by sk on 20150616
						else if ((0xF1 == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+5)))
						{
							//send data to face board
							fbdata[1] = *(pspbuf+mark+1);
							fbdata[2] = *(pspbuf+mark+2);
							if (!wait_write_serial(serial[2]))
							{
								if (write(serial[2],fbdata,sizeof(fbdata)) < 0)
								{
								#ifdef MAIN_DEBUG
									printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									markbit |= 0x0800;
									gettimeofday(&stt,NULL);
									update_event_list(&eventclass,&eventlog,1,16,stt.tv_sec,&markbit);
                                    if(SUCCESS!=generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                                    {
                                    #ifdef MAIN_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
									if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                    {//actively report is not probitted and connect successfully
                                        memset(err,0,sizeof(err));
                                        if (SUCCESS != err_report(err,stt.tv_sec,1,16))
                                        {
                                        #ifdef MAIN_DEBUG
                                            printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                        }
										else
        								{
            								write(csockfd,err,sizeof(err));
        								}
                                    }
								}
							}
							else
							{
							#ifdef MAIN_DEBUG
								printf("cannot write serial,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}
							//end send data;
							
							//added on 20150408
							markbit2 |= 0x0080;//there are flow data or detector has been linked
							if ((0x02 != *(pspbuf+mark+1)) && (0x03 != *(pspbuf+mark+1)))
							{
								for (i = 0; i < DETECTORNUM; i++)
								{
									if (0 == detepro[i].deteid)
										break;
									if (detepro[i].deteid == *(pspbuf+mark+2))
									{
										detepro[i].err = 0;
										if (1 == detepro[i].ereport)
										{
											detepro[i].ereport = 0;
											if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
											{
												memset(err,0,sizeof(err));
												if (SUCCESS != detect_err_pack(err,0x17,detepro[i].deteid))
												{
												#ifdef MAIN_DEBUG
													printf("detect report err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
												else
												{
													write(csockfd,err,sizeof(err));					
												}
											}//if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))	
										}
										gettimeofday(&detepro[i].stime,NULL);
									}//if (detepro[i].deteid == *(pspbuf+mark+2))
								}
							}
							//end add
	
							//secondly,flow msg will be written to ./data/flow.dat
							if (0x01 == *(pspbuf+mark+1))
							{//vehicle pass detector;
								gettimeofday(&stt,NULL);
								if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
            					{//actively report is not probitted and connect successfully
									dinfo.time = stt.tv_sec;
									dinfo.detectid = *(pspbuf+mark+2);
									memset(dibuf,0,sizeof(dibuf));
									if (SUCCESS != detect_info_report(dibuf,&dinfo))
									{
									#ifdef MAIN_DEBUG
										printf("detect_info_report err,File: %s,LIne: %d\n",__FILE__,__LINE__);
									#endif
									}
									else
									{
										write(csockfd,dibuf,sizeof(dibuf));
									}	
								}//actively report is not probitted and connect successfully	

								phaseid = 0;
								factfn = 0;
								for (j = 0; j < detector.FactDetectorNum; j++)
								{
									if (0 == detector.DetectorList[j].DetectorId)
										break;
									if (*(pspbuf+mark+2) == detector.DetectorList[j].DetectorId)
									{
										phaseid |= (0x01 << (detector.DetectorList[j].DetectorPhase - 1));
									}
								}//a detector map serval phases;
								if (detectdata.DataId >= FLOWNUM)
								{
									detectdata.DataId = 1;
								}
								else
								{
									detectdata.DataId += 1;
								}
								if (detectdata.FactFlowNum < FLOWNUM)
								{
									detectdata.FactFlowNum += 1;
								}
								factfn = detectdata.DataId;
								detectdata.DetectorDataList[factfn-1].DetectorId = *(pspbuf+mark+2);
								detectdata.DetectorDataList[factfn-1].DetectorData = 1;
								detectdata.DetectorDataList[factfn-1].PhaseId = phaseid;
								gettimeofday(&stt,NULL);
								detectdata.DetectorDataList[factfn-1].RecvTime = stt.tv_sec;
//								fwrite(&detectdata,sizeof(detectdata),1,ffp);
								markbit |= 0x0008; //flow have update

								if ((markbit & 0x0010) && (markbit & 0x0002))
								{
								#ifdef MAIN_DEBUG
									printf("send flow to conftool,deteid: %d,File:%s,Line:%d\n",*(pspbuf+mark+2),__FILE__,__LINE__);
								#endif
									dfdata[4] = *(pspbuf+mark+2);
									if (write(ssockfd,dfdata,sizeof(dfdata)) < 0)
                					{
                    					gettimeofday(&stt,NULL);
                    					update_event_list(&eventclass,&eventlog,1,20,stt.tv_sec,&markbit);
                    					if (SUCCESS != \
											generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                    					{
                    					#ifdef MAIN_DEBUG
                        					printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                    					#endif
                    					}
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                       	{//actively report is not probitted and connect successfully
                                           	memset(err,0,sizeof(err));
                                           	if (SUCCESS != err_report(err,stt.tv_sec,1,20))
                                           	{
                                           	#ifdef MAIN_DEBUG
                                               	printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                           	#endif
                                           	}
											else
        									{
            									write(csockfd,err,sizeof(err));
        									}
                                       	}
                					}
								}//if ((markbit & 0x0010) && (markbit & 0x0002))
							}//if (0x01 == *(pspbuf+mark+1))
							if (0x03 == *(pspbuf+mark+1))
							{//detector err
								unsigned char				detectID = 0;
								gettimeofday(&stt,NULL);
								detectID = *(pspbuf+mark+2);
								update_event_list(&eventclass,&eventlog,20,detectID,stt.tv_sec,&markbit);
                                if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                                {
                                #ifdef MAIN_DEBUG
                                    printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
							
								//added on 20150408
								for (i = 0; i < DETECTORNUM; i++)
								{
									if (0 == detepro[i].deteid)
										break;
									if (detepro[i].deteid == *(pspbuf+mark+2))
									{
										detepro[i].err = 1;
										if (0 == detepro[i].ereport)
										{
											detepro[i].ereport = 1;
                                            if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                            {
                                                memset(err,0,sizeof(err));
                                                if (SUCCESS!=err_report(err,stt.tv_sec,0x14,detepro[i].deteid))
                                                {
                                                #ifdef MAIN_DEBUG
                                                    printf("detect report err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                                #endif
                                                }
                                                else
                                                {
                                                    write(csockfd,err,sizeof(err));
                                                }
                                            }//if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
										}
										gettimeofday(&detepro[i].stime,NULL);
									}//if (detepro[i].deteid == *(pspbuf+mark+2))
								}
								//end add

								//send detector err to conf tool;									
								if ((markbit & 0x0010) && (markbit & 0x0002))
								{
									dedata[4] = *(pspbuf+mark+2);
									if (write(ssockfd,dedata,sizeof(dedata)) < 0)
                                    {
                                        gettimeofday(&stt,NULL);
                                       	update_event_list(&eventclass,&eventlog,1,20,stt.tv_sec,&markbit);
                                        if (SUCCESS != \
											generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                                        {
                                        #ifdef MAIN_DEBUG
                                            printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                        #endif
                                        }
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        {//actively report is not probitted and connect successfully
                                            memset(err,0,sizeof(err));
                                            if (SUCCESS != err_report(err,stt.tv_sec,1,20))
                                            {
                                            #ifdef MAIN_DEBUG
                                                printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                            #endif
                                            }
											else
        									{
            									write(csockfd,err,sizeof(err));
        									}
                                        }
                                    }
								}
							}//detector err

							mark += 6;
							continue;
						}//if ((0xF1 == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+5)))
						else
						{
							mark += 1;
							continue;
						}
					}//1, while(1)
				}//num >= 0
				continue;
			}//vehicle check board serial port
			if (FD_ISSET(serial[2],&nRead))
			{//face board serial port
				memset(spbuf,0,sizeof(spbuf));
				mark = 0;
				pspbuf = spbuf;
				num = read(serial[2],spbuf,sizeof(spbuf));
				if (num > sizeof(spbuf))
                    continue;
				if (num >= 0)
				{//num >= 0
					mark = 0;
					unsigned char			condata[3] = {0};
					unsigned char			numi = 0;
					while (1)
					{//while loop
						if (mark >= num)
							break;
						if ((0xD0 == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+3)))
						{
							if (0x01 == *(pspbuf+mark+1))
							{
								markbit &= 0xffdf;
							}//cancel send latest five lamp err or restore info
							if (0x02 == *(pspbuf+mark+1))
							{//send control pattern info to face board
								unsigned char		padata[5] = {0};
								unsigned char		tcid = 0;
								int					tphid = 0;
								padata[0] = 0xC2;
								padata[4] = 0xED;
								//get all stage and phase info according to pattern id;
								if (0 == retpattern)
								{
									padata[1] = 0x00;
									padata[2] = 0x00;
									padata[3] = 0x00;
									if (!wait_write_serial(serial[2]))
									{
										if (write(serial[2],padata,sizeof(padata)) < 0)
										{
										#ifdef MAIN_DEBUG
											printf("write err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
											markbit |= 0x0800;
											gettimeofday(&stt,NULL);
											update_event_list(&eventclass,&eventlog,1,16,stt.tv_sec,&markbit);
                                        	if (SUCCESS != \
												generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                                        	{
                                        	#ifdef MAIN_DEBUG
                                            	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        	#endif
                                        	}
											if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        	{//actively report is not probitted and connect successfully
                                            	memset(err,0,sizeof(err));
                                            	if (SUCCESS != err_report(err,stt.tv_sec,1,16))
                                            	{
                                            	#ifdef MAIN_DEBUG
                                                	printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                            	#endif
                                            	}
												else
        										{
            										write(csockfd,err,sizeof(err));
        										}
                                        	}
										}
									}
									else
									{
									#ifdef MAIN_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
								}
								else
								{
									//get all stage and phase info according to retpattern;
									for (j = 0; j < pattern.FactPatternNum; j++)
									{
										if (0 == pattern.PatternList[j].PatternId)
											break;
										if (retpattern == pattern.PatternList[j].PatternId)
										{
											tcid = pattern.PatternList[j].TimeConfigId;
											break;
										}
									}
									if (0 == tcid)
									{
									#ifdef MAIN_DEBUG
										printf("Not have fit timeconfigid,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
									else
									{//tcid != 0
										for (j = 0; j < timeconfig.FactTimeConfigNum; j++)
										{
											if (0 == timeconfig.TimeConfigList[j][0].TimeConfigId)
												break;
											if (tcid == timeconfig.TimeConfigList[j][0].TimeConfigId)
											{
												for (z = 0; z < timeconfig.FactStageNum; z++)
												{
													if (0 == timeconfig.TimeConfigList[j][z].StageId)
														break;
													padata[1] = 0x01;
													padata[2] = timeconfig.TimeConfigList[j][z].StageId;
													tphid = timeconfig.TimeConfigList[j][z].PhaseId;
													get_phase_id(tphid,&padata[3]);
													if (!wait_write_serial(serial[2]))
													{
														if (write(serial[2],padata,sizeof(padata)) < 0)
														{
														#ifdef MAIN_DEBUG
															printf("write err,Line:%d\n",__LINE__);
														#endif
															markbit |= 0x0800;
															gettimeofday(&stt,NULL);
															update_event_list(&eventclass,&eventlog,1,16,\
																				stt.tv_sec,&markbit);
                                        					if(SUCCESS != generate_event_file(&eventclass, \
																				&eventlog,softevent,&markbit))
                                        					{
                                        					#ifdef MAIN_DEBUG
                                            					printf("err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        					#endif
                                        					}
															if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        					{
                                            					memset(err,0,sizeof(err));
                                            					if (SUCCESS != err_report(err,stt.tv_sec,1,16))
                                            					{
                                            					#ifdef MAIN_DEBUG
                                                					printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                                            					#endif
                                            					}
																else
        														{
            														write(csockfd,err,sizeof(err));
        														}
                                        					}
														}
													}
													else
													{
													#ifdef MAIN_DEBUG
														printf("serial cannot be written,Line:%d\n",__LINE__);
													#endif
													}
												}//for (z = 0; z < timeconfig.FactStageNum; z++)
											//the following code means that pattern has been sent completely;
												padata[1] = 0x02;
												padata[2] = 0x00;
												padata[3] = 0x00;
												if (!wait_write_serial(serial[2]))
												{
													if (write(serial[2],padata,sizeof(padata)) < 0)
													{
													#ifdef MAIN_DEBUG
														printf("write err,Line: %d\n",__LINE__);
													#endif
														markbit |= 0x0800;
														gettimeofday(&stt,NULL);
														update_event_list(&eventclass,&eventlog,1,16, \
																			stt.tv_sec,&markbit);
                                        				if (SUCCESS != generate_event_file(&eventclass, \
																			&eventlog,softevent,&markbit))
                                        				{
                                        				#ifdef MAIN_DEBUG
                                            				printf("err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        				#endif
                                        				}
														if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        				{//actively report is not probitted 
                                            				memset(err,0,sizeof(err));
                                            				if (SUCCESS != err_report(err,stt.tv_sec,1,16))
                                            				{
                                            				#ifdef MAIN_DEBUG
                                                				printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                            				#endif
                                            				}
															else
        													{
            													write(csockfd,err,sizeof(err));
        													}
                                        				}
													}
												}
												else
												{
												#ifdef MAIN_DEBUG
													printf("serial cannot be written,Line: %d\n",__LINE__);
												#endif
												}
												break;
											}//if (tcid == timeconfig.TimeConfigList[j][0].TimeConfigId)
										}//for (j = 0; j < timeconfig.FactTimeConfigNum; j++)
									}//tcid != 0
								}//get all stage and phase info according to retpattern
							}//send control pattern info to face board
							if (0x03 == *(pspbuf+mark+1))
							{//send network info to face board
								FILE 				*ipfp = NULL;

								ipfp = fopen(IPFILE,"rb");
								if (NULL == ipfp)
								{
								#ifdef MAIN_DEBUG
									printf("open file error,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
								if (NULL != ipfp)
								{
									unsigned char			gw[20] = {0};
									unsigned char			ip[20] = {0};
									unsigned char			sm[20] = {0};
									unsigned char			msg[64] = {0};
									unsigned char			ipmark = 0;
									int						pos = 0;
									int						tint = 0;
									unsigned char			*tem1 = NULL;
									unsigned char			*tem2 = NULL;
									unsigned char			netdata[7] = {0};

									while (1)
									{
										memset(msg,0,sizeof(msg));
										if (NULL == fgets(msg,sizeof(msg),ipfp))
											break;
										if (SUCCESS == find_out_child_str(msg,"DefaultGateway",&pos))
										{
											tem1 = msg;
											tem2 = gw;
											while (1)
											{
												if ('\0' == *tem1)
													break;
												if ((('0' <= *tem1) && (*tem1 <= '9')) || ('.' == *tem1))
												{
													*tem2 = *tem1;
													tem2++;
												}
												tem1++;
											}
											continue;	
										}
										if (0 == ipmark)
										{//have two "IPAddress" in userinfo.txt,we need first "IPAddress"; 
											if (SUCCESS == find_out_child_str(msg,"IPAddress",&pos))
											{
												ipmark = 1;
												tem1 = msg;
												tem2 = ip;
												while (1)
												{
													if ('\0' == *tem1)
														break;
													if ((('0' <= *tem1) && (*tem1 <= '9')) || ('.' == *tem1))
													{
														*tem2 = *tem1;
														tem2++;
													}
													tem1++;
												}
												continue;
											}
										}
										if (SUCCESS == find_out_child_str(msg,"SubnetMask",&pos))
										{
											tem1 = msg;
											tem2 = sm;
											while (1)
											{
												if ('\0' == *tem1)
													break;
												if ((('0' <= *tem1) && (*tem1 <= '9')) || ('.' == *tem1))
												{
													*tem2 = *tem1;
													tem2++;
												}
												tem1++;
											}
											continue;
										}
									}//while (NULL != fgets(msg,sizeof(msg),ipfp))
									fclose(ipfp);

									netdata[0] = 0xC3;
									netdata[6] = 0xED;
									unsigned char		section1[4] = {0};
									unsigned char		section2[4] = {0};
									unsigned char		section3[4] = {0};
									unsigned char		section4[4] = {0};
									unsigned short		tshort = 0;
									if (SUCCESS != get_network_section(ip,section1,section2,section3,section4))
									{
									#ifdef MAIN_DEBUG
										printf("get section err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
									//send ip info to face board
									netdata[1] = 0x01;
									string_to_digit(section1,&tint);
									netdata[2] = 0;
									netdata[2] |= tint;
									string_to_digit(section2,&tint);
									netdata[3] = 0;
									netdata[3] |= tint;
									string_to_digit(section3,&tint);
									netdata[4] = 0;
									netdata[4] |= tint;
									string_to_digit(section4,&tint);
									netdata[5] = 0;
									netdata[5] |= tint;
									if (!wait_write_serial(serial[2]))
									{
										if (write(serial[2],netdata,sizeof(netdata)) < 0)
										{
										#ifdef MAIN_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											markbit |= 0x0800;
											gettimeofday(&stt,NULL);
											update_event_list(&eventclass,&eventlog,1,16,stt.tv_sec,&markbit);
                                        	if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                                        	{
                                        	#ifdef MAIN_DEBUG
                                            	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        	#endif
                                        	}
											if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        	{//actively report is not probitted and connect successfully
                                            	memset(err,0,sizeof(err));
                                            	if (SUCCESS != err_report(err,stt.tv_sec,1,16))
                                            	{
                                            	#ifdef MAIN_DEBUG
                                                	printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                            	#endif
                                            	}
												else
        										{
            										write(csockfd,err,sizeof(err));
        										}
                                        	}
										}
									}
									else
									{
									#ifdef MAIN_DEBUG
										printf("serial cannot be written,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
									//end send ip info to face board
									memset(section1,0,sizeof(section1));
									memset(section2,0,sizeof(section2));
									memset(section3,0,sizeof(section3));
									memset(section4,0,sizeof(section4));
									if (SUCCESS != get_network_section(sm,section1,section2,section3,section4))
                                    {
                                    #ifdef MAIN_DEBUG
                                        printf("get section err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }	
									//send net mask to face board
									netdata[1] = 0x02;
                                    string_to_digit(section1,&tint);
                                    netdata[2] = 0;
                                    netdata[2] |= tint;
                                    string_to_digit(section2,&tint);
                                    netdata[3] = 0;
                                    netdata[3] |= tint;
                                    string_to_digit(section3,&tint);
                                    netdata[4] = 0;
                                    netdata[4] |= tint;
                                    string_to_digit(section4,&tint);
                                    netdata[5] = 0;
                                    netdata[5] |= tint;
                                    if (!wait_write_serial(serial[2]))
                                    {
                                        if (write(serial[2],netdata,sizeof(netdata)) < 0)
                                        {
                                        #ifdef MAIN_DEBUG
                                            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
											markbit |= 0x0800;
											gettimeofday(&stt,NULL);
											update_event_list(&eventclass,&eventlog,1,16,stt.tv_sec,&markbit);
                                        	if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                                        	{
                                        	#ifdef MAIN_DEBUG
                                            	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        	#endif
                                        	}
											if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        	{//actively report is not probitted and connect successfully
                                            	memset(err,0,sizeof(err));
                                            	if (SUCCESS != err_report(err,stt.tv_sec,1,16))
                                            	{
                                            	#ifdef MAIN_DEBUG
                                                	printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                            	#endif
                                            	}
												else
        										{
            										write(csockfd,err,sizeof(err));
        										}
                                        	}
                                        }
                                    }
                                    else
                                    {
                                    #ifdef MAIN_DEBUG
                                        printf("serial cannot be written,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
									//end send net mask to face board
									memset(section1,0,sizeof(section1));
									memset(section2,0,sizeof(section2));
									memset(section3,0,sizeof(section3));
									memset(section4,0,sizeof(section4));
									if (SUCCESS != get_network_section(gw,section1,section2,section3,section4))
                                    {
                                    #ifdef MAIN_DEBUG
                                        printf("get section err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
									//send gateway info to face borad
									netdata[1] = 0x03;
                                    string_to_digit(section1,&tint);
                                    netdata[2] = 0;
                                    netdata[2] |= tint;
                                    string_to_digit(section2,&tint);
                                    netdata[3] = 0;
                                    netdata[3] |= tint;
                                    string_to_digit(section3,&tint);
                                    netdata[4] = 0;
                                    netdata[4] |= tint;
                                    string_to_digit(section4,&tint);
                                    netdata[5] = 0;
                                    netdata[5] |= tint;
                                    if (!wait_write_serial(serial[2]))
                                    {
                                        if (write(serial[2],netdata,sizeof(netdata)) < 0)
                                        {
                                        #ifdef MAIN_DEBUG
                                            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
											markbit |= 0x0800;
											gettimeofday(&stt,NULL);
											update_event_list(&eventclass,&eventlog,1,16,stt.tv_sec,&markbit);
                                        	if (SUCCESS != generate_event_file(&eventclass, \
																&eventlog,softevent,&markbit))
                                        	{
                                        	#ifdef MAIN_DEBUG
                                            	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        	#endif
                                        	}
											if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        	{//actively report is not probitted and connect successfully
                                            	memset(err,0,sizeof(err));
                                            	if (SUCCESS != err_report(err,stt.tv_sec,1,16))
                                            	{
                                            	#ifdef MAIN_DEBUG
                                                	printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                            	#endif
                                            	}
												else
        										{
            										write(csockfd,err,sizeof(err));
        										}
                                        	}
                                        }
                                    }
                                    else
                                    {
                                    #ifdef MAIN_DEBUG
                                        printf("serial cannot be written,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
									//end send gateway info to face borad	

									//send server ip info to face board
									memset(section1,0,sizeof(section1));
                                    memset(section2,0,sizeof(section2));
                                    memset(section3,0,sizeof(section3));
                                    memset(section4,0,sizeof(section4));
								if(SUCCESS!=get_network_section(houtaiip,section1,section2,section3,section4))
									{
									#ifdef MAIN_DEBUG
										printf("get section err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
									netdata[1] = 0x04;
									string_to_digit(section1,&tint);
									netdata[2] = 0;
									netdata[2] |= tint;
									string_to_digit(section2,&tint);
									netdata[3] = 0;
									netdata[3] |= tint;
									string_to_digit(section3,&tint);
									netdata[4] = 0;
									netdata[4] |= tint;
									string_to_digit(section4,&tint);
									netdata[5] = 0;
									netdata[5] |= tint;
									if (!wait_write_serial(serial[2]))
									{
										if (write(serial[2],netdata,sizeof(netdata)) < 0)
										{
										#ifdef MAIN_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											markbit |= 0x0800;
											gettimeofday(&stt,NULL);
											update_event_list(&eventclass,&eventlog,1,16,stt.tv_sec,&markbit);
                                        	if (SUCCESS != generate_event_file(&eventclass,&eventlog, \
																				softevent,&markbit))
                                        	{
                                        	#ifdef MAIN_DEBUG
                                            	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        	#endif
                                        	}
											if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        	{//actively report is not probitted and connect successfully
                                            	memset(err,0,sizeof(err));
                                            	if (SUCCESS != err_report(err,stt.tv_sec,1,16))
                                            	{
                                            	#ifdef MAIN_DEBUG
                                                	printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                            	#endif
                                            	}
												else
        										{
            										write(csockfd,err,sizeof(err));
        										}
                                        	}
										}
									}
									else
									{
									#ifdef MAIN_DEBUG
										printf("serial cannot be written,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
									//send server ip info to face board

									//send port info to face board
									netdata[1] = 0x05;
									tshort = houtaiport;
									tshort &= 0x00ff;
									netdata[2] = 0;
									netdata[2] |= tshort;
									tshort = houtaiport;
									tshort &= 0xff00;
									tshort >>= 8;
									netdata[3] = 0;
									netdata[3] |= tshort;

									tshort = SINGALER_MACHINE_SERVER_PORT;
									tshort &= 0x00ff;
									netdata[4] = 0;
									netdata[4] |= tshort;
									tshort = SINGALER_MACHINE_SERVER_PORT;
									tshort &= 0xff00;
									tshort >>= 8;
									netdata[5] = 0;
									netdata[5] |= tshort;
									if (!wait_write_serial(serial[2]))
                                    {
                                        if (write(serial[2],netdata,sizeof(netdata)) < 0)
                                        {
                                        #ifdef MAIN_DEBUG
                                            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
											markbit |= 0x0800;
											gettimeofday(&stt,NULL);
											update_event_list(&eventclass,&eventlog,1,16,stt.tv_sec,&markbit);
                                        	if (SUCCESS != generate_event_file(&eventclass, \
																&eventlog,softevent,&markbit))
                                        	{
                                        	#ifdef MAIN_DEBUG
                                            	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        	#endif
                                        	}
											if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        	{//actively report is not probitted and connect successfully
                                            	memset(err,0,sizeof(err));
                                            	if (SUCCESS != err_report(err,stt.tv_sec,1,16))
                                            	{
                                            	#ifdef MAIN_DEBUG
                                                	printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                            	#endif
                                            	}
												else
        										{
            										write(csockfd,err,sizeof(err));
        										}
                                        	}
                                        }
                                    }
                                    else
                                    {
                                    #ifdef MAIN_DEBUG
                                        printf("serial cannot be written,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    } 
									//end send port info to face board	
								}//NULL != ipfp
							}//send network info to face board
							if (0x04 == *(pspbuf+mark+1))
							{
								markbit |= 0x0020;//send latest five lamp err or restore info when have update
								unsigned char           errdata[5] = {0};
                                errdata[0] = 0xC4;
                                errdata[4] = 0xED;
                                for (j = 0; j < FIVEERRLAMPINFO; j++)
                                {
									if (0 == (felinfo.fiveerrlampinfoList[j].chanid))
										continue;
                                    errdata[1] = felinfo.fiveerrlampinfoList[j].mark;
                                    errdata[2] = felinfo.fiveerrlampinfoList[j].chanid;
                                    errdata[3] = felinfo.fiveerrlampinfoList[j].errtype;
                                    if (!wait_write_serial(serial[2]))
                                    {
                                        if (write(serial[2],errdata,sizeof(errdata)) < 0)
                                        {
                                        #ifdef MAIN_DEBUG
                                            printf("write err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                        #endif
											markbit |= 0x0800;
											gettimeofday(&stt,NULL);
											update_event_list(&eventclass,&eventlog,1,16,stt.tv_sec,&markbit);
                                        	if (SUCCESS != generate_event_file(&eventclass, \
															&eventlog,softevent,&markbit))
                                        	{
                                        	#ifdef MAIN_DEBUG
                                            	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        	#endif
                                        	}
											if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                            {//actively report is not probitted and connect successfully
                                                memset(err,0,sizeof(err));
                                                if (SUCCESS != err_report(err,stt.tv_sec,1,16))
                                                {
                                                #ifdef MAIN_DEBUG
                                                    printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                                #endif
                                                }
												else
        										{
            										write(csockfd,err,sizeof(err));
        										}
                                            }
                                        }
                                    }
                                    else
                                    {
                                    #ifdef MAIN_DEBUG
                                        printf("can't be written,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }//for loop
							}//if (0x04 == *(pspbuf+mark+1))
							if (0x05 == *(pspbuf+mark+1))
							{//send status of drive board to face board
								unsigned char           dbdata[4] = {0};
                                dbdata[0] = 0xC5;
                                dbdata[3] = 0xED;
                                for (j = 0; j < DRIBOARDNUM; j++)
                                {
									if (0 == (dbstatus.driboardstatusList[j].dribid))
										continue;	
                                	dbdata[1] = dbstatus.driboardstatusList[j].dribid;
                                    dbdata[2] = dbstatus.driboardstatusList[j].status;
                                    if (!wait_write_serial(serial[2]))
                                    {
                                    	if (write(serial[2],dbdata,sizeof(dbdata)) < 0)
                                        {
                                        #ifdef MAIN_DEBUG
                                            printf("write err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                        #endif
											markbit |= 0x0800;
											gettimeofday(&stt,NULL);
											update_event_list(&eventclass,&eventlog,1,16,stt.tv_sec,&markbit);
                                        	if (SUCCESS != generate_event_file(&eventclass, \
																&eventlog,softevent,&markbit))
                                        	{
                                        	#ifdef MAIN_DEBUG
                                            	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        	#endif
                                        	}
											if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                            {//actively report is not probitted and connect successfully
                                                memset(err,0,sizeof(err));
                                                if (SUCCESS != err_report(err,stt.tv_sec,1,16))
                                                {
                                                #ifdef MAIN_DEBUG
                                                    printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                                #endif
                                                }
												else
        										{
            										write(csockfd,err,sizeof(err));
        										}
                                            }
                                        }
                                    }
                                    else
                                    {
                                    #ifdef MAIN_DEBUG
                                        printf("serial cannot write,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }//for (j = 0; j < DRIBOARDNUM; j++)
							}//send status of drive board to face board
							if (0x06 == *(pspbuf+mark+1))
							{//send system time to face board
								struct tm				*ntime;
								unsigned char			ntdata[9] = {0};

								ntime = (struct tm *)get_system_time();
								ntdata[0] = 0xC6;
								ntdata[1] = ntime->tm_year - 70;
								ntdata[2] = ntime->tm_mon + 1;
								ntdata[3] = ntime->tm_mday;
								ntdata[4] = ntime->tm_hour;
								ntdata[5] = ntime->tm_min;
								ntdata[6] = ntime->tm_sec;
								ntdata[7] = ntime->tm_wday;
								ntdata[8] = 0xED;
								if (!wait_write_serial(serial[2]))
								{
									if (write(serial[2],ntdata,sizeof(ntdata)) < 0)
									{
									#ifdef MAIN_DEBUG
										printf("write err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
										markbit |= 0x0800;
										gettimeofday(&stt,NULL);
										update_event_list(&eventclass,&eventlog,1,16,stt.tv_sec,&markbit);
                                        if (SUCCESS != generate_event_file(&eventclass,&eventlog, \
																			softevent,&markbit))
                                        {
                                        #ifdef MAIN_DEBUG
                                            printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                        }
										if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                        {//actively report is not probitted and connect successfully
                                            memset(err,0,sizeof(err));
                                            if (SUCCESS != err_report(err,stt.tv_sec,1,16))
                                            {
                                            #ifdef MAIN_DEBUG
                                                printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                            #endif
                                            }
											else
        									{
            									write(csockfd,err,sizeof(err));
        									}
                                        }
									}
								}
								else
								{
								#ifdef MAIN_DEBUG
									printf("serial cannot write,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}//send system time to face board
							if (0x07 == *(pspbuf+mark+1))
							{//send phase info to face board
								unsigned char			phid = 0;
								unsigned int			phnum = 0;
								unsigned char			pdata[6] = {0};
								unsigned char			tcid = 0;

								pdata[0] = 0xC7;
								pdata[1] = *(pspbuf+mark+2);
								for (j = 0; j < pattern.FactPatternNum; j++)
								{
									if (0 == pattern.PatternList[j].PatternId)
										break;
									if (retpattern == pattern.PatternList[j].PatternId)
									{
										tcid = pattern.PatternList[j].TimeConfigId;
										break;
									}
								}
								if (0 != tcid)
								{
									for (j = 0; j < timeconfig.FactTimeConfigNum; j++)
									{
										if (0 == timeconfig.TimeConfigList[j][0].TimeConfigId)
											break;
										if (tcid == timeconfig.TimeConfigList[j][0].TimeConfigId)
										{
											for (z = 0; z < timeconfig.FactStageNum; z++)
											{
												if (0 == timeconfig.TimeConfigList[j][z].StageId)
													break;
												phnum = timeconfig.TimeConfigList[j][z].PhaseId;
												get_phase_id(phnum,&phid);	
												if (phid == *(pspbuf+mark+2))
												{
													pdata[2] = timeconfig.TimeConfigList[j][z].GreenTime;
													break;
												}
											}
											break;	
										}
									}//for (j = 0; j < timeconfig.FactTimeConfigNum; j++)
									for (j = 0; j < phase.FactPhaseNum; j++)
									{
										if (0 == phase.PhaseList[j].PhaseId)
											break;
										if (*(pspbuf+mark+2) == phase.PhaseList[j].PhaseId)
										{
											pdata[3] = phase.PhaseList[j].PhaseMinGreen;
											pdata[4] = phase.PhaseList[j].PhaseMaxGreen1;
											break;
										}
									}
									pdata[5] = 0xED;
									if (!wait_write_serial(serial[2]))
									{
										if (write(serial[2],pdata,sizeof(pdata)) < 0)
										{
										#ifdef MAIN_DEBUG
											printf("write err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
											markbit |= 0x0800;
											gettimeofday(&stt,NULL);
											update_event_list(&eventclass,&eventlog,1,16,stt.tv_sec,&markbit);
                                        	if (SUCCESS != generate_event_file(&eventclass,&eventlog, \
																					softevent,&markbit))
                                        	{
                                        	#ifdef MAIN_DEBUG
                                            	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        	#endif
                                        	}
											if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                                            {//actively report is not probitted and connect successfully
                                                memset(err,0,sizeof(err));
                                                if (SUCCESS != err_report(err,stt.tv_sec,1,16))
                                                {
                                                #ifdef MAIN_DEBUG
                                                    printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                                #endif
                                                }
												else
        										{
            										write(csockfd,err,sizeof(err));
        										}
                                            }
										}
									}
									else
									{
									#ifdef MAIN_DEBUG
										printf("serial cannot write,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}//0 != tcid
								else
								{
								#ifdef MAIN_DEBUG
									printf("Not have fit tcid,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}//send phase info to face board
							mark += 4;
							continue;
						}
						else if ((0xD3 == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+6)))
						{//modify IP info of signal machine
							if (1 == *(pspbuf+mark+1))
							{
								memset(sec1,0,sizeof(sec1));
                                digit_to_string(*(pspbuf+mark+2),sec1);
                                memset(sec2,0,sizeof(sec2));
                                digit_to_string(*(pspbuf+mark+3),sec2);
                                memset(sec3,0,sizeof(sec3));
                                digit_to_string(*(pspbuf+mark+4),sec3);
                                memset(sec4,0,sizeof(sec4));
                                digit_to_string(*(pspbuf+mark+5),sec4);
                                memset(ipstr,0,sizeof(ipstr));
                                strcpy(ipstr,sec1);
                                strcat(ipstr,".");
                                strcat(ipstr,sec2);
                                strcat(ipstr,".");
                                strcat(ipstr,sec3);
                                strcat(ipstr,".");
                                strcat(ipstr,sec4);
                                numi += 1;
                                #ifdef MAIN_DEBUG
                                printf("ipstr:%s,File:%s,Line:%d\n",ipstr,__FILE__,__LINE__);
                                #endif
							}
							else if (2 == *(pspbuf+mark+1))
							{
								memset(sec1,0,sizeof(sec1));
                                digit_to_string(*(pspbuf+mark+2),sec1);
                                memset(sec2,0,sizeof(sec2));
                                digit_to_string(*(pspbuf+mark+3),sec2);
                                memset(sec3,0,sizeof(sec3));
                                digit_to_string(*(pspbuf+mark+4),sec3);
                                memset(sec4,0,sizeof(sec4));
                                digit_to_string(*(pspbuf+mark+5),sec4);
                                memset(nmstr,0,sizeof(nmstr));
                                strcpy(nmstr,sec1);
                                strcat(nmstr,".");  
                                strcat(nmstr,sec2);
                                strcat(nmstr,".");  
                                strcat(nmstr,sec3);
                                strcat(nmstr,".");
                                strcat(nmstr,sec4);
								numi += 1;
								#ifdef MAIN_DEBUG
                                printf("nmstr:%s,File:%s,Line:%d\n",nmstr,__FILE__,__LINE__);
                                #endif
							}
							else if (3 == *(pspbuf+mark+1))
                            {
								memset(sec1,0,sizeof(sec1));
                                digit_to_string(*(pspbuf+mark+2),sec1);
                                memset(sec2,0,sizeof(sec2));
                                digit_to_string(*(pspbuf+mark+3),sec2);
                                memset(sec3,0,sizeof(sec3));
                                digit_to_string(*(pspbuf+mark+4),sec3);
                                memset(sec4,0,sizeof(sec4));
                                digit_to_string(*(pspbuf+mark+5),sec4);
                                memset(gwstr,0,sizeof(gwstr));
                                strcpy(gwstr,sec1);
                                strcat(gwstr,".");
                                strcat(gwstr,sec2);
                                strcat(gwstr,".");
                                strcat(gwstr,sec3);
                                strcat(gwstr,".");
                                strcat(gwstr,sec4);
								numi += 1;
								#ifdef MAIN_DEBUG
                                printf("gwstr:%s,File:%s,Line:%d\n",gwstr,__FILE__,__LINE__);
                                #endif
                            }
							else if (4 == *(pspbuf+mark+1))
                            {//server ip
								memset(sec1,0,sizeof(sec1));
                                digit_to_string(*(pspbuf+mark+2),sec1);
                                memset(sec2,0,sizeof(sec2));
                                digit_to_string(*(pspbuf+mark+3),sec2);
                                memset(sec3,0,sizeof(sec3));
                                digit_to_string(*(pspbuf+mark+4),sec3);
                                memset(sec4,0,sizeof(sec4));
                                digit_to_string(*(pspbuf+mark+5),sec4);
                                memset(houtaiip,0,sizeof(houtaiip));
                                strcpy(houtaiip,sec1);
                                strcat(houtaiip,".");
                                strcat(houtaiip,sec2);
                                strcat(houtaiip,".");
                                strcat(houtaiip,sec3);
                                strcat(houtaiip,".");
                                strcat(houtaiip,sec4);
                                numi += 1;
                                #ifdef MAIN_DEBUG
                                printf("server ip :%s,File:%s,Line:%d\n",houtaiip,__FILE__,__LINE__);
                                #endif
                            }//server ip
							else if (5 == *(pspbuf+mark+1))
                            {
								houtaiport = 0;
								houtaiport |= *(pspbuf+mark+3);
								houtaiport <<= 8;
								houtaiport &= 0xff00;
								houtaiport |= *(pspbuf+mark+2);
								numi += 1;
								#ifdef MAIN_DEBUG
                                printf("houtaiport:%d,File:%s,Line:%d\n",houtaiport,__FILE__,__LINE__);
                                #endif
                            }
							if (5 == numi)
							{//begin to modify ip info
								FILE 					*ipfp = NULL;
								unsigned char			msg[64] = {0};
								int						pos = 0;
								int						offset = 0;
								unsigned char			ipmark = 0;
								unsigned char			lm[20] = {0};//local_machine
								unsigned char			dhcp[16] = {0};//dhcp
								unsigned char			ns[16] = {0};//nfs server
								unsigned char			ip2[32] = {0};//ip address of nfs server
								unsigned char			mp[64] = {0};//mount path
								unsigned char			ue[16] = {0};//user exe
								unsigned char			name[64] = {0};//run name of process
								unsigned char			para[32] = {0};//parameter
								unsigned char			ipaddr[64] = {0};
								unsigned char			ipmask[64] = {0};
								unsigned char			ipgw[64] = {0};

								//Firstly,save info of userinfo.txt
								ipfp = fopen(IPFILE,"r");
								if (NULL == ipfp)
								{
								#ifdef MAIN_DEBUG
									printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									mark += 7;
									continue;
								}
								ipmark = 0;
								while(1)
								{
									memset(msg,0,sizeof(msg));
									if (NULL == fgets(msg,sizeof(msg),ipfp))
										break;
									if (SUCCESS == find_out_child_str(msg,"IPAddress",&pos))
									{
										if (1 == ipmark)
										{
											ipmark = 0;
											strcpy(ip2,msg);
											continue;
										}
										ipmark = 1;
										continue;
									}
									if (SUCCESS == find_out_child_str(msg,"LOCAL_MACHINE",&pos))
									{
										strcpy(lm,msg);
										continue;
									}
									if (SUCCESS == find_out_child_str(msg,"DHCP",&pos))
                        			{
                            			strcpy(dhcp,msg);
										continue;
                        			}
									if (SUCCESS == find_out_child_str(msg,"NFS_SERVER",&pos))
                        			{
                            			strcpy(ns,msg);
										continue;
                        			}
									if (SUCCESS == find_out_child_str(msg,"Mountpath",&pos))
                        			{
                            			strcpy(mp,msg);
										continue;
                        			}
									if (SUCCESS == find_out_child_str(msg,"USER_EXE",&pos))
                        			{
                            			strcpy(ue,msg);
										continue;
                        			}
									if (SUCCESS == find_out_child_str(msg,"Name",&pos))
                        			{
                            			strcpy(name,msg);
										continue;
                        			}
									if (SUCCESS == find_out_child_str(msg,"Parameters",&pos))
                        			{
                            			strcpy(para,msg);
										continue;
                        			}
								}//while(NULL != fgets(msg,sizeof(msg),ipfp))
								fclose(ipfp);
								//Secondly,delete "userinfo.txt"
								if (-1 == system("rm -f /mnt/nandflash/userinfo.txt"))
								{
								#ifdef MAIN_DEBUG
									printf("Delete userinfo.txt error,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									mark += 7;
									continue;
								}
								//Threely,create a new "userinfo.txt" again;
								ipfp = fopen(IPFILE,"wb+");
								if (NULL == ipfp)
								{
								#ifdef MAIN_DEBUG
									printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									mark += 7;
									continue;
								}
								memset(ipaddr,0,sizeof(ipaddr));
								sprintf(ipaddr,"IPAddress=\"%s\"",ipstr);
								memset(ipmask,0,sizeof(ipmask));
								sprintf(ipmask,"SubnetMask=\"%s\"",nmstr);
								memset(ipgw,0,sizeof(ipgw));
								sprintf(ipgw,"DefaultGateway=\"%s\"",gwstr);
								
								fputs(lm,ipfp);
								fputs(dhcp,ipfp);
								fputs(ipgw,ipfp);
								fputs("\n",ipfp);
								fputs(ipaddr,ipfp);
								fputs("\n",ipfp);
								fputs(ipmask,ipfp);
								fputs("\n",ipfp);
								fputs(ns,ipfp);
								fputs(ip2,ipfp);
								fputs(mp,ipfp);
								fputs(ue,ipfp);
								fputs(name,ipfp);
								fputs(para,ipfp);
								fclose(ipfp);	
								//Fourly,temply change ip address;
								
								if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                            	{//actively report is not probitted and connect successfully
                                	memset(err,0,sizeof(err));
                                	gettimeofday(&stt,NULL);
                                	if (SUCCESS != err_report(err,stt.tv_sec,22,11))
                                	{
                                	#ifdef MAIN_DEBUG
                                    	printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
                                	}
                                	else
                                	{
                                    	write(csockfd,err,sizeof(err));
                                	}
                            	}
								gettimeofday(&stt,NULL);
                                update_event_list(&eventclass,&eventlog,1,37,stt.tv_sec,&markbit);

								unsigned char			nia[64] = {0};//new ip address
								unsigned char			ngw[64] = {0};//new Gateway
//								int						enable = 1;
								unsigned int			nslip = 0;
								sprintf(nia,"%s %s %s %s","ifconfig eth0",ipstr,"netmask",nmstr);
								sprintf(ngw,"%s %s","route add default gw",gwstr);
								if (-1 == system(nia))
								{
								#ifdef MAIN_DEBUG
									printf("set ip address error,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									mark += 7;
									continue;
								}
								if (-1 == system(ngw))
								{
								#ifdef MAIN_DEBUG
									printf("set gateway error,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									mark += 7;
									continue;
								}
								if (1 == seryes)
								{
									close(netfd);
									pthread_cancel(servid);
									pthread_join(servid,NULL);
									seryes = 0;
								}
								if (0 == seryes)
    							{
        							int ser = pthread_create(&servid,NULL,(void *)setup_net_server,NULL);
        							if (0 != ser)
        							{
        							#ifdef MAIN_DEBUG
            							printf("build server error,File: %s,Line: %d\n",__FILE__,__LINE__);
        							#endif
            							return;
        							}
        							seryes = 1;
    							}
								ipfp = fopen(IPPORT_FILE,"wb+");
								if (NULL != ipfp)
								{//if (NULL != ipfp)
									fputs(houtaiip,ipfp);
									fputs("\n",ipfp);
									unsigned char			tsport[8] = {0};
									short_to_string(houtaiport,tsport);
									fputs(tsport,ipfp);
									fclose(ipfp);
									if (1 == cliyes)
                               		{
                                   		close(csockfd);
                                   		pthread_cancel(clivid);
                                   		pthread_join(clivid,NULL);
                                   		cliyes = 0;
                               		}
                               		if (0 == cliyes)
                               		{
                                   		int cli = pthread_create(&clivid,NULL,(void *)setup_client,NULL);
                                   		if (0 != cli)
                                   		{
                                   		#ifdef MAIN_DEBUG
                                       		printf("build client error,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   		#endif
                                   		}
                                   		cliyes = 1;
                               		}
								}//if (NULL != ipfp)	
								get_last_ip_field(&nslip);
                                lip = nslip;	
							}//begin to modify ip info
							mark += 7;
					
							continue;
						}//modify ip info of signal machine
						else if ((0xD6 == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+8)))
						{//modify system clock
							struct tm				*mtime;
							long					ltime = 0;
							struct timeval			tvtime,nt6;

							nt6.tv_sec = 0;
                            nt6.tv_usec = 0;
                            gettimeofday(&nt6,NULL);							

							mtime = (struct tm *)get_system_time();
							mtime->tm_year = *(pspbuf+mark+1) + 70;
							mtime->tm_mon = *(pspbuf+mark+2) - 1;
							mtime->tm_mday = *(pspbuf+mark+3);
							mtime->tm_hour = *(pspbuf+mark+4);
							mtime->tm_min = *(pspbuf+mark+5);
							mtime->tm_sec = *(pspbuf+mark+6);
							mtime->tm_wday = *(pspbuf+mark+7);
							ltime = (long)mktime(mtime);
							tvtime.tv_sec = ltime;
							tvtime.tv_usec = 0;
							if (-1 == settimeofday(&tvtime,NULL))
							{
							#ifdef MAIN_DEBUG
								printf("set system time err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif	
							}							
							if (-1 == system("hwclock -w"))
							{
							#ifdef MAIN_DEBUG
								printf("set time error,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}
							if (-1 == system("sync"))
							{
							#ifdef MAIN_DEBUG
								printf("set time error,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}

							if (nt6.tv_sec > tvtime.tv_sec)
							{
								if ((nt6.tv_sec - tvtime.tv_sec) > 20)
								{
								#ifdef MAIN_DEBUG
                            		printf("Time difference exceed 20 sec,File:%s,Line:%d\n",__FILE__,__LINE__);
                        		#endif
                            		markbit |= 0x0040;//clock data do have update
                            		if (!wait_write_serial(endpipe[1]))
                            		{
                                		if (write(endpipe[1],"UpdateFile",11) < 0)
                                		{
                                		#ifdef MAIN_DEBUG
                                    		printf("Write endpipe err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
                                		}
                            		}
                            		else
                            		{
                            		#ifdef MAIN_DEBUG
                                		printf("Can not write endpipe,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
                            		}
                        		}//must reagain get time interval and restart pattern
							}
							else
							{
								if ((tvtime.tv_sec - nt6.tv_sec) > 20)
								{
								#ifdef MAIN_DEBUG
                                    printf("Time difference exceed 20 sec,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                    markbit |= 0x0040;//clock data do have update
                                    if (!wait_write_serial(endpipe[1]))
                                    {
                                        if (write(endpipe[1],"UpdateFile",11) < 0)
                                        {
                                        #ifdef MAIN_DEBUG
                                            printf("Write endpipe err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef MAIN_DEBUG
                                        printf("Can not write endpipe,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
								}//must reagain get time interval and restart pattern
							}

							mark += 9;
							if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                            {//actively report is not probitted and connect successfully
                                memset(err,0,sizeof(err));
                                gettimeofday(&stt,NULL);
                                if (SUCCESS != err_report(err,stt.tv_sec,22,12))
                                {
                                #ifdef MAIN_DEBUG
                                    printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
                                else
                                {
                                    write(csockfd,err,sizeof(err));
                                }
                            }
							gettimeofday(&stt,NULL);
                            update_event_list(&eventclass,&eventlog,1,38,stt.tv_sec,&markbit);					

							continue;
						}//modify system clock
						else if ((0xD7 == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+5)))
						{//modify phase info
							unsigned char				mphid = 0;
							unsigned char				gt = 0;
							unsigned char				mingt = 0;
							unsigned char				maxgt = 0;
							unsigned char				mtcid = 0;
							unsigned char				tmphid = 0;
							unsigned short				cyct = 0;
							FILE						*cfp = NULL;
							unsigned char				pex = 0;
							mphid = *(pspbuf+mark+1);
							gt = *(pspbuf+mark+2);
							mingt = *(pspbuf+mark+3);
							maxgt = *(pspbuf+mark+4);
							for (j = 0; j < phase.FactPhaseNum; j++)
							{
								if (0 == (phase.PhaseList[j].PhaseId))
									break;
								if (mphid == (phase.PhaseList[j].PhaseId))
								{
									pex = 1;
									phase.PhaseList[j].PhaseMinGreen = mingt;
									phase.PhaseList[j].PhaseMaxGreen1 = maxgt;
									break;
								}
							}
							if (0 == pex)
							{
							#ifdef MAIN_DEBUG
								printf("Not have the phase,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								mark += 6;
								continue;
							}
							for (j = 0; j < pattern.FactPatternNum; j++)
							{
								if (0 == (pattern.PatternList[j].PatternId))
									break;
								if (retpattern == (pattern.PatternList[j].PatternId))
								{
									mtcid = pattern.PatternList[j].TimeConfigId;
									break;
								}	
							}
							if (0 != mtcid)
							{
								for (j = 0; j < timeconfig.FactTimeConfigNum; j++)
								{
									if (0 == (timeconfig.TimeConfigList[j][0].TimeConfigId))
										break;
									if (mtcid == (timeconfig.TimeConfigList[j][0].TimeConfigId))
									{
										for (z = 0; z < timeconfig.FactStageNum; z++)
										{
											if (0 == (timeconfig.TimeConfigList[j][z].StageId))
												break;
											get_phase_id(timeconfig.TimeConfigList[j][z].PhaseId,&tmphid);
											if (mphid == tmphid)
											{
												timeconfig.TimeConfigList[j][z].GreenTime = gt;
											}
											//Reagain compute time of cycle;
											cyct += timeconfig.TimeConfigList[j][z].GreenTime + \
													timeconfig.TimeConfigList[j][z].YellowTime + \
													timeconfig.TimeConfigList[j][z].RedTime;	 
										}
										break;
									}
								}
								for (j = 0; j < pattern.FactPatternNum; j++)
                            	{
                                	if (0 == (pattern.PatternList[j].PatternId))
                                    	break;
                                	if (retpattern == (pattern.PatternList[j].PatternId))
                                	{
                                    	pattern.PatternList[j].CycleTime = cyct;//update cycle time;
                                    	break;
                                	}
                            	}
							}//if (0 != mtcid)
							else
							{
							#ifdef MAIN_DEBUG
								printf("Not have fit timeconfig,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								mark += 6;
								continue;
							}
							if (0 == access(CY_TSC,F_OK))
							{
							#ifdef MAIN_DEBUG
								printf("delete 'cy_tsc.dat',File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								system("rm -f ./data/cy_tsc.dat");
							}
							cfp = fopen(CY_TSC,"wb+");
							if (NULL == cfp)
							{
							#ifdef MAIN_DEBUG
								printf("Update configure file err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
							if (NULL != cfp)
							{
								fwrite(&tscheader,sizeof(tscheader),1,cfp);
								fwrite(&unit,sizeof(unit),1,cfp);
								fwrite(&schedule,sizeof(schedule),1,cfp);
								fwrite(&timesection,sizeof(timesection),1,cfp);
								fwrite(&pattern,sizeof(pattern),1,cfp);
								fwrite(&timeconfig,sizeof(timeconfig),1,cfp);
								fwrite(&phase,sizeof(phase),1,cfp);
								fwrite(&phaseerror,sizeof(phaseerror),1,cfp);
								fwrite(&channel,sizeof(channel),1,cfp);
								fwrite(&channelhint,sizeof(channelhint),1,cfp);
								fwrite(&detector,sizeof(detector),1,cfp);
								fclose(cfp);
								markbit |= 0x0040;//configure data do have update
                				if (!wait_write_serial(endpipe[1]))
                				{
                    				if (write(endpipe[1],"UpdateFile",11) < 0)
                    				{
                    				#ifdef MAIN_DEBUG
                        				printf("Write endpipe err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    				#endif
                    				}
                				}
                				else
                				{
                				#ifdef MAIN_DEBUG
                    				printf("Can not write endpipe,File: %s,Line: %d\n",__FILE__,__LINE__);
                				#endif
                				}
							}
							mark += 6;

							if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                            {//actively report is not probitted and connect successfully
                                memset(err,0,sizeof(err));
								gettimeofday(&stt,NULL);
                                if (SUCCESS != err_report(err,stt.tv_sec,22,13))
                                {
                                #ifdef MAIN_DEBUG
                            	    printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
                                else
                                {
                                	write(csockfd,err,sizeof(err));
                                }
                            }
							gettimeofday(&stt,NULL);
                            update_event_list(&eventclass,&eventlog,1,39,stt.tv_sec,&markbit);

							continue;
						}//modify phase info
						else if ((0xDA == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+2)))
						{//manual control
							if ((markbit2 & 0x0004)||(markbit2 & 0x0008)||(markbit2 & 0x0010))
							{//wireless or software or configure tool is control signaler machine;
								mark += 3;
								continue;
							}//wireless or software or configure tool is control signaler machine;
							condata[0] = *(pspbuf+mark);
							condata[1] = *(pspbuf+mark+1);
							condata[2] = *(pspbuf+mark+2);
							//火车道口增加日志,added by shikang on 20220213
							if (0x01 == *(pspbuf+mark+1))
							{
								output_log("*************************face board sends control,line 4586**************************");
							}
							//end add

							//clean pipe
							while (1)
							{
								memset(cdata,0,sizeof(cdata));
								if (read(conpipe[0],cdata,sizeof(cdata)) <= 0)
									break;
							}
							//end clean pipe
							if (!wait_write_serial(conpipe[1]))
							{
								if (write(conpipe[1],condata,sizeof(condata)) < 0)
								{
								#ifdef MAIN_DEBUG
									printf("write pipe error,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							else
							{
							#ifdef MAIN_DEBUG
								printf("can not write,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
							mark += 3;
							continue;
						}//manual control
						else if ((0xDB == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+4)))
						{//wendu/shidu/zhengdong value
							if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                            {//actively report is not probitted and connect successfully
								if (0x01 == *(pspbuf+mark+1))
								{//wendu value
									memset(err,0,sizeof(err));
									if (SUCCESS != wendu_pack(err,*(pspbuf+mark+2),*(pspbuf+mark+3)))
									{
									#ifdef MAIN_DEBUG
										printf("wendu_pack call err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
									else
									{
										write(csockfd,err,sizeof(err));
									}
								}//wendu value
								if (0x02 == *(pspbuf+mark+1))
								{//shidu value
									memset(err,0,sizeof(err));
									if (SUCCESS != shidu_pack(err,*(pspbuf+mark+2),*(pspbuf+mark+3)))
									{
									#ifdef MAIN_DEBUG
										printf("shidu_pack call err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
									else
									{
										write(csockfd,err,sizeof(err));
									}
								}//shidu value
								if (0x03 == *(pspbuf+mark+1))
								{//zhengdong value
									memset(err,0,sizeof(err));
									if (SUCCESS != zdong_pack(err,*(pspbuf+mark+2),*(pspbuf+mark+3)))
									{
									#ifdef MAIN_DEBUG
										printf("zdong_pack call err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
									else
									{
										write(csockfd,err,sizeof(err));
									}
								}//zhengdong value							
							}//actively report is not probitted and connect successfully
							mark += 5;
							continue;
						}//wendu/shidu/zhengdong value
				#if 0
						else if ((0xDA == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+2)))
						{//if ((0xDA == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+2)))
							//check stage whether exist in current pattern;
							if ((*(pspbuf+mark+1) < 0x11))
							{//not jump stage control
								stacon[0] = *(pspbuf+mark);
								stacon[1] = *(pspbuf+mark+1);
								stacon[2] = *(pspbuf+mark+2);
								#ifdef MAIN_DEBUG
								printf("stacon[1]: %d,File:%s,Line: %d\n",stacon[1],__FILE__,__LINE__);
								#endif
								while (1)
								{
									memset(cdata,0,sizeof(cdata));
									if (read(conpipe[0],cdata,sizeof(cdata)) <= 0)
										break;
								}
								//end clean pipe
								if (!wait_write_serial(conpipe[1]))
								{
									if (write(conpipe[1],stacon,sizeof(stacon)) < 0)
									{
									#ifdef MAIN_DEBUG
										printf("write pipe error,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								else
								{
								#ifdef MAIN_DEBUG
									printf("can not write,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
								if (0x01 == *(pspbuf+mark+1))
								{
									markbit2 |= 0x0800;
								}//control or cancel control
								else if (0x02 == *(pspbuf+mark+1))
								{
									markbit2 |= 0x1000;
								}//step by step
								else if (0x03 == *(pspbuf+mark+1))
								{
									markbit2 |= 0x2000;
								}//yellow flash
								else if (0x04 == *(pspbuf+mark+1))
                                {
									markbit2 |= 0x4000;
                                }//all red	
							}//not jump stage control
							else 
							{//jump stage control
								staid = (0x0f & (*(pspbuf+mark+1)));
								#ifdef MAIN_DEBUG
								printf("staid: %d,File:%s,Line: %d\n",staid,__FILE__,__LINE__);
								#endif 
								if (SUCCESS == check_stageid_valid(staid,retpattern,&tscdata))
								{//2
									stacon[0] = *(pspbuf+mark);
									stacon[1] = *(pspbuf+mark+1);
									stacon[2] = *(pspbuf+mark+2);
								#ifdef MAIN_DEBUG
                                    printf("stacon[1]:%d,File: %s,Line: %d\n",stacon[1],__FILE__,__LINE__);
                                #endif
									while (1)
									{
										memset(cdata,0,sizeof(cdata));
										if (read(conpipe[0],cdata,sizeof(cdata)) <= 0)
											break;
									}
									//end clean pipe
									if (!wait_write_serial(conpipe[1]))
									{
										if (write(conpipe[1],stacon,sizeof(stacon)) < 0)
										{
										#ifdef MAIN_DEBUG
											printf("write pipe error,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
									#ifdef MAIN_DEBUG
										printf("can not write,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}//2
							}//jump stage control
							mark += 3;
							continue;
						}//if ((0xDA == *(pspbuf+mark)) && (0xED == *(pspbuf+mark+2)))
				#endif
						else
						{
							mark += 1;
							continue;
						}
					}//while loop
				}//num >= 0
				continue;
			}//face board serial port
		}//ret > 0	
	}//0,while(1)
	pthread_exit(NULL);
}

#ifdef V2X_DEBUG
void setup_gd_v2x_client(void *arg)
{
	FILE						*v2xfp = NULL;
	unsigned char				v2xip[17] = {0};
	unsigned short           	v2xport = 0;	
	unsigned char				i = 0,j = 0;
	unsigned char				buf[256] = {0};
	unsigned short				totaln = 0;
	int							gd_sockfd = 0;
	unsigned char				gdcontm = 0;

	if (0 == access(GDV2XPORT_FILE,F_OK))
	{
		unsigned char				ttii = 0;
		unsigned char				htp[12] = {0};

		v2xfp = fopen(GDV2XPORT_FILE,"rb");
		if (NULL != v2xfp)
		{
			fgets(v2xip,17,v2xfp);
			fgets(htp,10,v2xfp);
			fclose(v2xfp);

			for (ttii = 0; ;ttii++)	
			{
				if (('\n' == v2xip[ttii]) || ('\0' == v2xip[ttii]))
				{
					v2xip[ttii] = '\0';
					break;
				}
			}
			for (ttii = 0; ;ttii++)
            {
                if (('\n' == htp[ttii]) || ('\0' == htp[ttii]))
                {
					htp[ttii] = '\0';
                    break;
                }
            }
			int				thtp = 0;
			string_to_digit(htp,&thtp);
			v2xport = thtp;
		}//if (NULL != fp)
	}//if (0 == access(IPPORT_FILE,F_OK))
	else
	{
		strcpy(v2xip,"36.7.79.167");
		v2xport = 5000;
	}
	
	sleep(20);
	struct sockaddr_in			n_server_addr;
	memset(&n_server_addr,0,sizeof(n_server_addr));
    n_server_addr.sin_family = AF_INET;
    n_server_addr.sin_addr.s_addr = inet_addr(v2xip);
    n_server_addr.sin_port = htons(v2xport);
    bzero(&(n_server_addr.sin_zero),8);

	if ((gd_sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
    #ifdef MAIN_DEBUG
        printf("create new client socket err,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
        return;
    }
	else
	{//create new socket successfully;
		while (1)
		{//1 while
			if (connect(gd_sockfd,(struct sockaddr *)&n_server_addr,sizeof(struct sockaddr)) < 0)
			{//connect err
			#ifdef MAIN_DEBUG
				printf("**********************************connect new GAODE v2x server err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				sleep(10);
	
				close(gd_sockfd);
				gd_sockfd = socket(AF_INET,SOCK_STREAM,0);

				continue;
			}//connect err;
			else
			{//connect successfully
				#ifdef MAIN_DEBUG
                printf("***************************connect GAODE v2x server successfully,File: %s,Line: %d\n",__FILE__,__LINE__);
            	#endif
				gdcontm = 0;
				if (0x00 == contmode)
					gdcontm = 21;
				else if ((0x05 == contmode) || (32 == contmode))
				{
					if (v2xmark & 0x01)
                        gdcontm = 23;
                    else
                        gdcontm = 22;
				}

				memset(buf,0,sizeof(buf));
				gaode_v2x_data_encode(buf,slg,&tscdata,&totaln,&v2xcyct,&gdcontm);
				write(gd_sockfd,buf,totaln);
				while (1)
				{//2 while
					sleep(3);
					//由于另外一个线程已经做了倒计时自减，因此这里不需要再做重复的动作；
					gdcontm = 0;
                	if (0x00 == contmode)
                    	gdcontm = 21;
                	else if ((5 == contmode) || (32 == contmode))
					{
						if (v2xmark & 0x01)
                    		gdcontm = 23;
						else
							gdcontm = 22;
					}
					memset(buf,0,sizeof(buf));
					gaode_v2x_data_encode(buf,slg,&tscdata,&totaln,&v2xcyct,&gdcontm);
					if (write(gd_sockfd,buf,totaln) < 0)
					{
					#ifdef MAIN_DEBUG
						printf("network has break off or write err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						break;
					}					
				}//2 while
			}//connect successfully
		}//1 while	
	}//create new socket successfully;
	return;
}

void setup_baidu_v2x_client(void *arg)
{
	FILE						*v2xfp = NULL;
	unsigned char				v2xip[17] = {0};
	unsigned short           	v2xport = 0;	
	unsigned char				i = 0,j = 0;
	unsigned char				buf[256] = {0};
	struct timeval				nowt;
	unsigned short				totaln = 0;

	if (0 == access(V2XPORT_FILE,F_OK))
	{
		unsigned char				ttii = 0;
		unsigned char				htp[12] = {0};

		v2xfp = fopen(V2XPORT_FILE,"rb");
		if (NULL != v2xfp)
		{
			fgets(v2xip,17,v2xfp);
			fgets(htp,10,v2xfp);
			fclose(v2xfp);

			for (ttii = 0; ;ttii++)	
			{
				if (('\n' == v2xip[ttii]) || ('\0' == v2xip[ttii]))
				{
					v2xip[ttii] = '\0';
					break;
				}
			}
			for (ttii = 0; ;ttii++)
            {
                if (('\n' == htp[ttii]) || ('\0' == htp[ttii]))
                {
					htp[ttii] = '\0';
                    break;
                }
            }
			int				thtp = 0;
			string_to_digit(htp,&thtp);
			v2xport = thtp;
		}//if (NULL != fp)
	}//if (0 == access(IPPORT_FILE,F_OK))
	else
	{
		strcpy(v2xip,"172.16.1.119");
		v2xport = 7650;
	}
	#ifdef MAIN_DEBUG
	printf("***************************************v2xip: %s,v2xport: %d,line:%d\n",v2xip,v2xport,__LINE__);
	#endif

	sleep(20);
	struct sockaddr_in			n_server_addr;
	memset(&n_server_addr,0,sizeof(n_server_addr));
    n_server_addr.sin_family = AF_INET;
    n_server_addr.sin_addr.s_addr = inet_addr(v2xip);
    n_server_addr.sin_port = htons(v2xport);
    bzero(&(n_server_addr.sin_zero),8);

	if ((n_csockfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
    #ifdef MAIN_DEBUG
        printf("create new client socket err,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
        return;
    }
	else
	{//create new socket successfully;
		while (1)
		{//1 while
		#ifdef MAIN_DEBUG
			printf("************************************Begin to connect v2x server,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			if (connect(n_csockfd,(struct sockaddr *)&n_server_addr,sizeof(struct sockaddr)) < 0)
			{//connect err
			#ifdef MAIN_DEBUG
				printf("**********************************connect new v2x server err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				sleep(10);
	
				close(n_csockfd);
				n_csockfd = socket(AF_INET,SOCK_STREAM,0);

				continue;
			}//connect err;
			else
			{//connect successfully
				#ifdef MAIN_DEBUG
                printf("***************************connect new v2x server successfully,File: %s,Line: %d\n",__FILE__,__LINE__);
            	#endif
				gettimeofday(&nowt,NULL);
				memset(buf,0,sizeof(buf));
				v2x_data_encode(buf,slg,&tscdata,&nowt,&totaln);
				write(n_csockfd,buf,totaln);
				while (1)
				{//2 while
					sleep(1);
					for (j = 0; j < MAX_CHANNEL; j++)
					{
						if (((slg[j].slgid) > 0) && (slg[j].countdown > 0) && (0xff != slg[j].countdown))
						{
							slg[j].countdown = slg[j].countdown - 1;
				//			if (0 == slg[j].countdown)
				//				slg[j].countdown = 1;
						}
					}
					gettimeofday(&nowt,NULL);
					memset(buf,0,sizeof(buf));
					v2x_data_encode(buf,slg,&tscdata,&nowt,&totaln);
					if (write(n_csockfd,buf,totaln) < 0)
					{
					#ifdef MAIN_DEBUG
						printf("network has break off or write err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						break;
					}					
				}//2 while
			}//connect successfully
		}//1 while	
	}//create new socket successfully;

	return;
}
#endif

#ifdef NTP_TIME

void set_up_ntp_updae_time(void *arg)
{
	struct timeval			stWaitTime;
	int iNtpPort = 123;
	int retVal = -1;
	//开机后立即校时10次
	int iLoopCnt = 10;
	while(1)
	{
		if( iLoopCnt-- > 0 )
		{
			stWaitTime.tv_sec = 3;
		}
		else
		{
			stWaitTime.tv_sec = 1800;
		}
		stWaitTime.tv_usec = 0;
		select(0,NULL,NULL,NULL,&stWaitTime);

		if(strlen(ntpip)<=0)
		{
			continue;
		}
		if( ntpport!=0 )
		{
			iNtpPort = ntpport;
		}
		
#ifdef MAIN_DEBUG
		//校时成功后更新时间，不在进行多次校时
		printf("net_sntp_client ip %s port %d\n",ntpip,iNtpPort);
#endif
		retVal = net_sntp_client((char*)ntpip, iNtpPort);
		if(retVal == 0)
		{
		#ifdef MAIN_DEBUG
			//校时成功后更新时间，不在进行多次校时
			printf("ntp time auto check success\n");
		#endif
			iLoopCnt = 0;
		}
	}
	pthread_exit(NULL);
}

#endif

void setup_client(void *arg)
{
	struct sockaddr_in      server_addr;
	struct timeval			tt,tt21;
	fd_set					cliRead;
	unsigned char			cbuf[/*1024*/SBYTE+9] = {0};
	unsigned char			errbuf[4] = {0};
	unsigned short			num = 0;
	unsigned char			sendyes = 0;
	unsigned char			pantinfo[4] = {0x21,0x80,0xD7,0x00}; //info of heart beat
	unsigned char			pantn = 0;	//number of heart beat
	infotype_t				itype;
	objectinfo_t			objecti[8] = {0};	
	unsigned char			extend = 0;
	unsigned char			i = 0;
	unsigned short			j = 0;
	unsigned char			condata[3] = {0};
	unsigned char			cdata[256] = {0};
	int						sret = 0;
	unsigned short			factovs = 0;
	unsigned char			netcon[3] = {0xCC,0,0xED};
	unsigned char			cleanb[16] = {0};
	versioninfo_t			vi;
	unsigned char			vibuf[10] = {0};
	unsigned int			ttct = 0;
	unsigned char			err[10] = {0};
	unsigned char			flags;
	sleep(30);	

	memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(/*"192.168.10.17"*/houtaiip);
//  remote_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(/*8080*/houtaiport);
    bzero(&(server_addr.sin_zero),8);

	markbit |= 0x1000;//default not connect server 
	markbit |= 0x8000; //default report actively is closed
	if ((csockfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
    #ifdef MAIN_DEBUG
       	printf("create client socket err,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
		return;
    }
	else
	{//create sockfd successfully
		while (1)
		{//1 while
            flags = fcntl(csockfd,F_GETFL,0);//get pro of sockfd
			#ifdef MAIN_DEBUG
            printf("Begin to connect server,houtaip:%s,ipport:%d,File: %s,Line: %d\n",houtaiip,houtaiport,__FILE__,__LINE__);
            #endif
			fcntl(csockfd,F_SETFL,flags|O_NONBLOCK);//set sockfd non-block
			if (connect(csockfd,(struct sockaddr *)&server_addr,sizeof(struct sockaddr)) < 0)
			{//connect err
				if(errno == EINTR)
				{
					//output_log("continue connect");
					#ifdef MAIN_DEBUG
					printf("**********************************continue connect,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					continue;
				}
				if(EINPROGRESS != errno)
	            {
	            	#ifdef MAIN_DEBUG
					printf("**********************************connect server err ,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					close(csockfd);
					sleep(10);
					csockfd = socket(AF_INET,SOCK_STREAM,0);
					//output_log("connect error 1");
	           
					if (0 == sendyes)
					{//ensure that only record one error of connect server; 
						gettimeofday(&tt,NULL);
	            		update_event_list(&eventclass,&eventlog,1,21,tt.tv_sec,&markbit);
	            		if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
	            		{
	            			#ifdef MAIN_DEBUG
	            			printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
	            			#endif
	            		}
						sendyes = 1;
					}//ensure that only record one error of connect server;
					 
					markbit |= 0x1000;	
	//				markbit |= 0x8000; //default report actively is closed

					#ifdef WUXI_CHECK
					if (12 == contmode)
					{//system optimize mode
						if (!(markbit2 & 0x0400))
						{//if (markbit2 & 0x0400)
							#ifdef MAIN_DEBUG
							printf("***********Need to send updatefile,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							markbit2 |= 0x0400;//need to degrade since network err
							markbit |= 0x0040;//configure data do have update
							if (!wait_write_serial(endpipe[1]))
							{
								if (write(endpipe[1],"UpdateFile",11) < 0)
								{
								#ifdef MAIN_DEBUG
									printf("Write endpipe err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
							}
							else
							{
							#ifdef MAIN_DEBUG
								printf("Can not write endpipe,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}

							gettimeofday(&tt,NULL);
							update_event_list(&eventclass,&eventlog,1,95,tt.tv_sec,&markbit);
							if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
							{
							#ifdef MAIN_DEBUG
								printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}
						}//if (markbit2 & 0x0400)
					}//system optimize mode
					#endif
					continue;
				}
				else
				{
						fd_set rset;
						FD_ZERO(&rset);
						FD_SET(csockfd, &rset);
						struct timeval tm;
						tm. tv_sec = 1;
						tm.tv_usec = 0;
						if ( select(csockfd + 1, NULL, &rset, NULL, &tm) <= 0)
						{
							close(csockfd);
							sleep(10);
							csockfd = socket(AF_INET,SOCK_STREAM,0);
							//output_log("connect error 2");
							#ifdef MAIN_DEBUG
							printf("**********************************connect server err ,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							if (0 == sendyes)
							{//ensure that only record one error of connect server; 
								gettimeofday(&tt,NULL);
			            		update_event_list(&eventclass,&eventlog,1,21,tt.tv_sec,&markbit);
			            		if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
			            		{
			            		#ifdef MAIN_DEBUG
			            			printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
			            		#endif
			            		}
								sendyes = 1;
							}//ensure that only record one error of connect server;
							 
							markbit |= 0x1000;	
			//				markbit |= 0x8000; //default report actively is closed

							#ifdef WUXI_CHECK
							if (12 == contmode)
							{//system optimize mode
								if (!(markbit2 & 0x0400))
								{//if (markbit2 & 0x0400)
									#ifdef MAIN_DEBUG
									printf("***********Need to send updatefile,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									markbit2 |= 0x0400;//need to degrade since network err
									markbit |= 0x0040;//configure data do have update
									if (!wait_write_serial(endpipe[1]))
									{
										if (write(endpipe[1],"UpdateFile",11) < 0)
										{
										#ifdef MAIN_DEBUG
											printf("Write endpipe err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
									#ifdef MAIN_DEBUG
										printf("Can not write endpipe,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}

									gettimeofday(&tt,NULL);
									update_event_list(&eventclass,&eventlog,1,95,tt.tv_sec,&markbit);
									if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
									{
									#ifdef MAIN_DEBUG
										printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
								}//if (markbit2 & 0x0400)
							}//system optimize mode
							#endif
							continue;
						}

						if (FD_ISSET(csockfd, &rset))
						{
							int err = -1;
							socklen_t len = sizeof(int);
							if ( getsockopt(csockfd,  SOL_SOCKET, SO_ERROR ,&err, &len) < 0 )
							{
								close(csockfd);
								sleep(10);
								csockfd = socket(AF_INET,SOCK_STREAM,0);
								//output_log("connect error 3");
								#ifdef MAIN_DEBUG
								printf("**********************************connect server err ,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								if (0 == sendyes)
								{//ensure that only record one error of connect server; 
									gettimeofday(&tt,NULL);
				            		update_event_list(&eventclass,&eventlog,1,21,tt.tv_sec,&markbit);
				            		if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
				            		{
				            			#ifdef MAIN_DEBUG
				            			printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
				            			#endif
				            		}
									sendyes = 1;
								}//ensure that only record one error of connect server;
								 
								markbit |= 0x1000;	
				//				markbit |= 0x8000; //default report actively is closed

								#ifdef WUXI_CHECK
								if (12 == contmode)
								{//system optimize mode
									if (!(markbit2 & 0x0400))
									{//if (markbit2 & 0x0400)
										#ifdef MAIN_DEBUG
										printf("***********Need to send updatefile,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										markbit2 |= 0x0400;//need to degrade since network err
										markbit |= 0x0040;//configure data do have update
										if (!wait_write_serial(endpipe[1]))
										{
											if (write(endpipe[1],"UpdateFile",11) < 0)
											{
											#ifdef MAIN_DEBUG
												printf("Write endpipe err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
											}
										}
										else
										{
										#ifdef MAIN_DEBUG
											printf("Can not write endpipe,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}

										gettimeofday(&tt,NULL);
										update_event_list(&eventclass,&eventlog,1,95,tt.tv_sec,&markbit);
										if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
										{
										#ifdef MAIN_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}//if (markbit2 & 0x0400)
								}//system optimize mode
								#endif								
								continue;
							}

							if (err)
							{
								close(csockfd);
								sleep(10);
								csockfd = socket(AF_INET,SOCK_STREAM,0);
								//output_log("connect error 4");
								#ifdef MAIN_DEBUG
								printf("**********************************connect server err ,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								if (0 == sendyes)
								{//ensure that only record one error of connect server; 
									gettimeofday(&tt,NULL);
									update_event_list(&eventclass,&eventlog,1,21,tt.tv_sec,&markbit);
									if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
									{
										#ifdef MAIN_DEBUG
										printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
									}
									sendyes = 1;
								}//ensure that only record one error of connect server;
								 
								markbit |= 0x1000;	
				//				markbit |= 0x8000; //default report actively is closed
				
								#ifdef WUXI_CHECK
								if (12 == contmode)
								{//system optimize mode
									if (!(markbit2 & 0x0400))
									{//if (markbit2 & 0x0400)
										#ifdef MAIN_DEBUG
										printf("***********Need to send updatefile,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										markbit2 |= 0x0400;//need to degrade since network err
										markbit |= 0x0040;//configure data do have update
										if (!wait_write_serial(endpipe[1]))
										{
											if (write(endpipe[1],"UpdateFile",11) < 0)
											{
												#ifdef MAIN_DEBUG
												printf("Write endpipe err,File: %s,Line: %d\n",__FILE__,__LINE__);
												#endif
											}
										}
										else
										{
											#ifdef MAIN_DEBUG
											printf("Can not write endpipe,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
										}
				
										gettimeofday(&tt,NULL);
										update_event_list(&eventclass,&eventlog,1,95,tt.tv_sec,&markbit);
										if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
										{
											#ifdef MAIN_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
										}
									}//if (markbit2 & 0x0400)
								}//system optimize mode
								#endif	
								continue;
							}
						}
						//output_log("connect success");
						goto NEXT;
				}
			
			}//connect err;
			//else
			//{//connect successfully;
			NEXT:
			#ifdef MAIN_DEBUG
            	printf("connect server successfully,File: %s,Line: %d\n",__FILE__,__LINE__);
        	#endif
				markbit &= 0xefff;
				markbit &= 0x7fff;
				gettimeofday(&tt,NULL);
                update_event_list(&eventclass,&eventlog,1,26,tt.tv_sec,&markbit);
				if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
        		{//actively report is not probitted and connect successfully
            		memset(err,0,sizeof(err));
            		if (SUCCESS != err_report(err,tt.tv_sec,1,26))
            		{
            		#ifdef MAIN_DEBUG
                		printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
            		#endif
            		}
            		else
            		{
                		write(csockfd,err,sizeof(err));
            		}
        		}

				sendyes = 0;
				fcntl(csockfd,F_SETFL,flags); //restore the pro of sockfd
//				markbit |= 0x8000; //default report actively is closed
				tt.tv_sec = 0;
				tt.tv_usec = 500000;
				if (setsockopt(csockfd,SOL_SOCKET,SO_SNDTIMEO,&tt,sizeof(struct timeval)) < 0)
        		{
        		#ifdef MAIN_DEBUG
            		printf("set timeout err,File: %s,Line: %d\n",__FILE__,__LINE__);
					output_log("MainCont,set sockopt send err");
        		#endif
        		}
				tt.tv_sec = 0;
                tt.tv_usec = 500000;
                if (setsockopt(csockfd,SOL_SOCKET,SO_RCVTIMEO,&tt,sizeof(struct timeval)) < 0)
                {
                #ifdef MAIN_DEBUG
                    printf("set timeout err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }

				if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
				{//report info to server actively
					unsigned char		sibuf[64] = {0};
          			if (SUCCESS != status_info_report(sibuf,&sinfo))
            		{
            		#ifdef MAIN_DEBUG
                		printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            		#endif
            		}
            		else
            		{
                		write(csockfd,sibuf,sizeof(sibuf));
            		}
					
					if (SUCCESS != send_ip_address(&csockfd))
					{
					#ifdef MAIN_DEBUG
						printf("Send ip address err,File: %s,LIne: %d\n",__FILE__,__LINE__);
					#endif
					}	
				}//report info to server actively

				//report info of drive board
				if (!(markbit & 0x1000))
				{
					if (SUCCESS != signaler_status_report(&dbstatus,&csockfd))
					{
					#ifdef MAIN_DEBUG
						printf("report info of drive board err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
				}
				//end report info of drive board	

				#ifdef WUXI_CHECK
				if (markbit2 & 0x0400)
				{//now is degrade work mode of network err
					markbit2 &= 0xfbff;
					markbit |= 0x0040;//configure data do have update
					if (!wait_write_serial(endpipe[1]))
					{
						if (write(endpipe[1],"UpdateFile",11) < 0)
						{
						#ifdef MAIN_DEBUG
							printf("Write endpipe err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
					}
					else
					{
					#ifdef MAIN_DEBUG
						printf("Can not write endpipe,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					gettimeofday(&tt,NULL);
                    update_event_list(&eventclass,&eventlog,1,96,tt.tv_sec,&markbit);
                    if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                    {
                    #ifdef MAIN_DEBUG
                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                    }
				}//now is degrade work mode of network err 
				#endif

				pantn = 0;
				while (1)
				{//while,read server data;
					FD_ZERO(&cliRead);
					FD_SET(csockfd,&cliRead);
					tt21.tv_sec = 20;
					tt21.tv_usec = 0;
				#ifdef MAIN_DEBUG
					printf("Begin to monitor server,File: %s,LIne: %d\n",__FILE__,__LINE__);
				#endif
					int cli = select(csockfd+1,&cliRead,NULL,NULL,&tt21);
					if (cli < 0)
					{
					#ifdef MAIN_DEBUG
						printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						sleep(10);
						continue;
					}
					if (0 == cli)
					{//timeout
						
						pantn += 1;
						if (pantn > 2)
						{
						#ifdef MAIN_DEBUG
							printf("Network break off,please reconnect,pantn: %d,File: %s,Line: %d\n",pantn,__FILE__,__LINE__);
						#endif
						
							if (markbit & 0x4000)//center software is controlling signaler machine
							{
								//send data to conpipe to degrade
								markbit &= 0xbfff;//center software will not controlling signaler machine;
								netcon[1] = 0xf1;
								while (1)
                            	{
                                	memset(cleanb,0,sizeof(cleanb));
                                	if (read(conpipe[0],cleanb,sizeof(cleanb)) <= 0)
                                    	break;
                            	}
                                if (!wait_write_serial(conpipe[1]))
                                {
                                	if (write(conpipe[1],netcon,sizeof(netcon)) < 0)
                                    {
                                    #ifdef MAIN_DEBUG
                                    	printf("write error,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                                else
                                {
                                #ifdef MAIN_DEBUG
                                	printf("can not write,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
							}//if (markbit & 0x4000)//center software is controlling signaler machine
							#ifdef WUXI_CHECK
							if (12 == contmode)
                            {//system optimize mode
                                markbit2 |= 0x0400;//need to degrade since network err
                                markbit |= 0x0040;//configure data do have update
                                if (!wait_write_serial(endpipe[1]))
                                {
                                    if (write(endpipe[1],"UpdateFile",11) < 0)
                                    {
                                    #ifdef MAIN_DEBUG
                                        printf("Write endpipe err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                                else
                                {
                                #ifdef MAIN_DEBUG
                                    printf("Can not write endpipe,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
								gettimeofday(&tt,NULL);
								update_event_list(&eventclass,&eventlog,1,95,tt.tv_sec,&markbit);
								if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
								{
								#ifdef MAIN_DEBUG
									printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
                            }//system optimize mode
							#endif

//							markbit |= 0x8000; //active report is probited;
							markbit |= 0x1000;//connect err
							break;
						}
						#ifdef MAIN_DEBUG
						printf("***************************************Send pantn to server,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						write(csockfd,pantinfo,sizeof(pantinfo));
						continue;
					}//timeout
					if (cli > 0)
					{//have data from server
						pantn = 0;
						if (FD_ISSET(csockfd,&cliRead))
						{
							memset(cbuf,0,sizeof(cbuf));
							num = read(csockfd,cbuf,sizeof(cbuf)); 
							if (num > sizeof(cbuf))
								continue;
							if (num > 0)
							{
								memset(&itype,0,sizeof(infotype_t));
								memset(objecti,0,sizeof(objectinfo_t));
								extend = 0;
								ttct = 0;
								sret = server_data_parse(&itype,objecti,&extend,&ttct,cbuf);
								if (MEMERR == sret)
								{
								#ifdef MAIN_DEBUG
									printf("Pointer address err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									continue;
								}
								if (FAIL == sret)
								{
								#ifdef MAIN_DEBUG
									printf("Not fit protol,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									if (0 == itype.highbit)
										itype.highbit = 0x01;
									itype.opertype = 0x06;
									memset(errbuf,0,sizeof(errbuf));
									err_data_pack(errbuf,&itype,0x02,0x00);
									if (write(csockfd,errbuf,sizeof(errbuf)) < 0)
									{
									#ifdef MAIN_DEBUG
										printf("send data err,*************************LIne:%d\n",__LINE__);
									#endif
									}
									continue;
								}
								if (0xD7 == objecti[0].objectid)
								{//pant info of server;
								#ifdef MAIN_DEBUG
									printf("***************Server send pant info,sockfd:%d,File: %s,Line: %d\n", csockfd,__FILE__,__LINE__);
								#endif
									continue;
								}//pant info of server;
								
								if (0 == itype.opertype)
								{//inquiry request
									if (0xD8 == objecti[0].objectid)
									{//inquiry configure data
										FILE					*nfp = NULL;
										unsigned char			*ndata = NULL;
										unsigned char			*nbuf = NULL;
										int						nlen = 0;
										int						ntotal = 0;

										nfp = fopen(CY_TSC,"rb");
										if (NULL == nfp)
										{
										#ifdef MAIN_DEBUG
											printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											continue;
										}
										fseek(nfp,0,SEEK_END);
										nlen = ftell(nfp);
										fseek(nfp,0,SEEK_SET);
										nbuf = (unsigned char *)malloc(nlen);
										if (NULL == nbuf)
										{
										#ifdef MAIN_DEBUG
											printf("malloc error,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											continue;
										}
										memset(nbuf,0,nlen);
										if (fread(nbuf,nlen,1,nfp) != 1)
										{
										#ifdef MAIN_DEBUG
											printf("read file error,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											continue;
										}
										ndata = (unsigned char *)malloc(nlen+8);
										if (NULL == ndata)
										{
										#ifdef MAIN_DEBUG
											printf("malloc error,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											if (NULL != nbuf)
											{
												free(nbuf);
												nbuf = NULL;
											}
											continue;
										}
										ntotal = nlen + 8;
										memset(ndata,0,nlen+8);
										#ifdef MAIN_DEBUG
										printf("nlen: %d,File: %s,Line: %d\n",nlen,__FILE__,__LINE__);
										#endif
										netdata_combine(ndata,nlen,nbuf);
//										mc_combine_str(sdata,"CYT4",total,buf,len,"END");
										if (write(csockfd,ndata,ntotal) < 0)
										{
										#ifdef MAIN_DEBUG
											printf("send data err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif	
										}
			
										fclose(nfp);
										if (NULL!= nbuf)
										{
											free(nbuf);
											nbuf = NULL;
										}
										if (NULL != ndata)
										{
											free(ndata);
											ndata = NULL;
										}

										continue;
									}//inquiry configure data
									if (0xD9 == objecti[0].objectid)
                                	{//software version requery;
                                	#ifdef MAIN_DEBUG
                                    	printf("Inquiry software version,File:%s,Line:%d\n",__FILE__,__LINE__);
                                	#endif
										memset(&vi,0,sizeof(versioninfo_t));
										vi.majorid = MajorVerId;
										vi.minorid1 = MinorVerId1;
										vi.minorid2 = MinorVerId2;
										vi.year = Year;
										vi.month = Month;
										vi.day = Day;
										memset(vibuf,0,sizeof(vibuf));
										if (SUCCESS != version_id_report(vibuf,&vi))
										{
										#ifdef MAIN_DEBUG
											printf("version id pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
											continue;
										}
										write(csockfd,vibuf,sizeof(vibuf));	
                                    	continue;
                                	}//software version requery
									if (0xDA == objecti[0].objectid)
									{//time requery
										gettimeofday(&tt,NULL);
									#ifdef MAIN_DEBUG
										printf("now time: %d,File:%s,LIne:%d\n",tt.tv_sec,__FILE__,__LINE__);
									#endif
										memset(vibuf,0,sizeof(vibuf));
										if (SUCCESS != time_info_report(vibuf,tt.tv_sec))
                                        {
                                        #ifdef MAIN_DEBUG
                                            printf("version id pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
                                            continue;
                                        }
										write(csockfd,vibuf,8);//time info only have 8 bytes buf;
                                        continue;
									}//time requery									
									if (0xEB == objecti[0].objectid)
									{//ID code and version requery;
										#ifdef MAIN_DEBUG
										printf("Requiry ID and version,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										unsigned char			bufp[40] = {0};
										unsigned char			*pbuf = bufp;
										FILE					*pzfp = NULL;
										unsigned char			pzi = 0,stiii=0;
										unsigned char			buf[] = "CYTJXHJ-CW-GA-CYTSC1CYTSC1-1705-0008END";						
										DevCode_st              stcitycode;
										unsigned char           isread = 0;

										memset(&stcitycode,0,sizeof(stcitycode));
										if (0 == access(MACHINE_VER,F_OK))
										{
											#ifdef MAIN_DEBUG
											printf("read ver.data,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
											pzfp = fopen(MACHINE_VER,"rb");
											if (NULL == pzfp)
											{
												#ifdef MAIN_DEBUG
												printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
												#endif
											}
											if(fread(&stcitycode,sizeof(stcitycode),1,pzfp) == 1)
											{
												for (stiii=0;stiii<stcitycode.strlen;stiii++)
    											 buf[stiii]=stcitycode.cydevcode[stiii]-stiii-5;
											}
											fclose(pzfp);	
											isread = 1;
										}
										*pbuf = 0x21;
										pbuf++;
										*pbuf = 0x84;
										pbuf++;
										*pbuf = 0xEB;
										pbuf++;
										*pbuf = 0x00;
										pbuf++;
										if(isread == 0)
										{
											stcitycode.strlen = strlen(buf);
										}
										for (pzi = 4; pzi < stcitycode.strlen; pzi++)
										{
											if(buf[pzi] == 0)
											{
												break;
											}
											if(buf[pzi] == 'E' && buf[pzi+1] == 'N' && buf[pzi+2] == 'D')
											{
												break;
											}
											*pbuf = buf[pzi];
											pbuf++;
										}
										write(csockfd,bufp,36);
										continue;	
									}//ID code and version requery;

									for (i = 0; i < (itype.objectn + 1); i++)
									{//for (i = 0; i < (itype.objectn + 1); i++)
										sret = get_object_value(&stscdata,objecti[i].objectid, \
												objecti[i].indexn,objecti[i].index,objecti[i].cobject, \
												&(objecti[i].objectvs),objecti[i].objectv);
										if (MEMERR == sret)
										{
										#ifdef MAIN_DEBUG
											printf("Pointer address err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											break;
										}
										if (FAIL == sret)
										{
										#ifdef MAIN_DEBUG
											printf("get object err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											if (0 == itype.highbit)
                                        		itype.highbit = 0x01;
                                    		itype.opertype = 0x06;
                                    		memset(errbuf,0,sizeof(errbuf));
                                    		err_data_pack(errbuf,&itype,0x05,0x00);
                                    		write(csockfd,errbuf,sizeof(errbuf));
											break;
										}
									}//for (i = 0; i < (itype.objectn + 1); i++)
									if ((MEMERR == sret) || (FAIL == sret))
										continue;
									memset(cbuf,0,sizeof(cbuf));
									if (0 == itype.highbit)
										itype.highbit = 0x01;
									itype.opertype = 0x04;
									factovs = 0;
									if (SUCCESS != server_data_pack(cbuf,&itype,objecti,&factovs))
									{
									#ifdef MAIN_DEBUG
										printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										continue;
									}
									write(csockfd,cbuf,factovs);			
									continue;
								}//inquiry request
								else if ((1 == itype.opertype) || (2 == itype.opertype))
								{//set request or no response;
									if (0 == itype.objectn)
									{
										if (0xD3 == objecti[0].objectid)
										{//switch of report info
											if (0 == extend)
											{
												markbit |= 0x8000;//must not report actively
											}
											else if (1 == extend)
											{
												markbit &= 0x7fff;//report actively
											}
											continue;
										}//switch of report info
										else if ((0xDE == objecti[0].objectid) && (1 == extend))
										{//fast forward
											if (markbit & 0x0400)
											{//transition stage
												memset(cbuf,0,sizeof(cbuf));
												if (0 == itype.highbit)
													itype.highbit = 0x01;
												itype.opertype = 0x05;//set response
												factovs = 0;
												for (i = 0; i < (itype.objectn + 1); i++)
												{
													objecti[i].objectvs = 0;
												}
												if (SUCCESS != fastforward_data_pack(cbuf,&itype,objecti, \
																&factovs,0x00))
												{
												#ifdef MAIN_DEBUG
                                                    printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                                #endif
													continue;
												}
												write(csockfd,cbuf,factovs);
                                            	continue;
											}//transition stage
											else
											{
												//clean pipe
                                            	while (1)
                                            	{
                                                	memset(cdata,0,sizeof(cdata));
                                                	if (read(ffpipe[0],cdata,sizeof(cdata)) <= 0)
                                                    	break;
                                            	}
                                            	//end clean pipe

                                            	if (!wait_write_serial(ffpipe[1]))
                                            	{
                                                	if (write(ffpipe[1],"fastforward",11) < 0)
                                                	{
                                                	#ifdef MAIN_DEBUG
                                                    	printf("write err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                                	#endif
                                                	}
                                            	}
                                            	else
                                            	{
                                            	#ifdef MAIN_DEBUG
                                                	printf("can't write,File:%s,Line:%d\n",__FILE__,__LINE__);
                                            	#endif
                                            	}

												memset(cbuf,0,sizeof(cbuf));
                                                if (0 == itype.highbit)
                                                    itype.highbit = 0x01;
                                                itype.opertype = 0x05;//set response
                                                factovs = 0;
                                                for (i = 0; i < (itype.objectn + 1); i++)
                                                {
                                                    objecti[i].objectvs = 0;
                                                }
                                                if (SUCCESS != fastforward_data_pack(cbuf,&itype,objecti, \
                                                                &factovs,0x01))
                                                {
                                                #ifdef MAIN_DEBUG
                                                    printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                                #endif
                                                    continue;
                                                }
												write(csockfd,cbuf,factovs);
												gettimeofday(&tt,NULL);
                        					  update_event_list(&eventclass,&eventlog,1,82,tt.tv_sec,&markbit);
                                            	continue;
											}
										}//fast forward
										else if (0xDC == objecti[0].objectid)
										{//start order of set whole pattern or not
											if (1 == extend)
											{
												markbit |= 0x0004;
											}
											memset(cbuf,0,sizeof(cbuf));
                                            if (0 == itype.highbit)
                                                itype.highbit = 0x01;
                                            itype.opertype = 0x05;//set response
                                            factovs = 0;
                                            for (i = 0; i < (itype.objectn + 1); i++)
                                            {
                                                objecti[i].objectvs = 0;
                                            }
                                            if (SUCCESS != server_data_pack(cbuf,&itype,objecti,&factovs))
                                            {
                                            #ifdef MAIN_DEBUG
                                                printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                            #endif
                                                continue;
                                            }
                                            write(csockfd,cbuf,factovs);

                                            continue;
										}//set whole pattern or not
										else if (0xDD == objecti[0].objectid)
                                        {//end order of set whole pattern or not
                                        	sret = check_single_fangan(&stscdata);
	                                    	if (MEMERR == sret)
	                                    	{
	                                    	#ifdef MAIN_DEBUG
	                                        	printf("Pointer address err,File: %s,Line: %d\n",__FILE__,__LINE__);
	                                    	#endif
	                                        	continue;
	                                    	}
											if(SUCCESS != sret)
											{
												unsigned char errorinfo[4] = {0};
												errorinfo[0] = 0X21;
												errorinfo[1] = 0X85;
												errorinfo[2] = 0XDD;
												//errorinfo[3] = 0X00;
												errorinfo[3] = sret;
												write(csockfd,errorinfo,sizeof(errorinfo));
												continue;
											}
                                            if (2 == extend)
                                            {
                                                markbit &= 0xfffb;
                                            }
                                        }//end order of set whole pattern or not
										else if (0xD4 == objecti[0].objectid)
										{//traffic control
											if ((markbit2 & 0x0002)||(markbit2 & 0x0004)||(markbit2 & 0x0010))
											{//have key or wireless or configure tool is controlling ;
												if (0 == itype.highbit)
                                                	itype.highbit = 0x01;
                                            	itype.opertype = 0x06;
                                            	memset(errbuf,0,sizeof(errbuf));
                                            	err_data_pack(errbuf,&itype,0x05,0x00);
                                            	write(csockfd,errbuf,sizeof(errbuf));
												continue;
											}//have key or wireless or configure tool  is controlling ;
											netcon[1] = extend;
											if (0 == netcon[1])
												continue;
											while (1)
                                			{
                                    			memset(cleanb,0,sizeof(cleanb));
                                    			if (read(conpipe[0],cleanb,sizeof(cleanb)) <= 0)
                                        			break;
                                			}
											if (!wait_write_serial(conpipe[1]))
                            				{
                                				if (write(conpipe[1],netcon,sizeof(netcon)) < 0)
                                				{
                                				#ifdef MAIN_DEBUG
                                    				printf("write err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                				#endif
                                				}
                            				}
                            				else
                            				{
                            				#ifdef MAIN_DEBUG
                                				printf("can not write,File:%s,Line:%d\n",__FILE__,__LINE__);
                            				#endif
                            				}
											continue;
										}//traffic control
										else if (0xF4 == objecti[0].objectid)
										{//use one key to green wave
											fanganID = extend;
										#ifdef MAIN_DEBUG
											printf("objectid: 0x%02x,fanganID: %d,Line: %d\n",objecti[0].objectid,fanganID,__LINE__);
										#endif
											unsigned char	pexist = 0;
											//find out fangan whether it is exist;
											for (i = 0; i < pattern.FactPatternNum; i++)
											{
												if (0 == pattern.PatternList[i].PatternId)
													break;
												if (fanganID == pattern.PatternList[i].PatternId)
												{
													if (0 != pattern.PatternList[i].CoordPhase)
													{
														pexist = 1;
													}
												}	
											}
											if (1 == pexist)
											{//pattern is exist
												//send updatefile to ingore re-read file;
												markbit |= 0x0040;//configure data do have update
												if (!wait_write_serial(endpipe[1]))
												{
													if (write(endpipe[1],"UpdateFile",11) < 0)
													{
													#ifdef MAIN_DEBUG
														printf("Write endpipe err,File: %s,Line: %d\n",__FILE__,__LINE__);
													#endif
													}
												}
												else
												{
												#ifdef MAIN_DEBUG
													printf("Can not write endpipe,File: %s,Line: %d\n",__FILE__,__LINE__);
												#endif
												}

												memset(cbuf,0,sizeof(cbuf));
												if (0 == itype.highbit)
													itype.highbit = 0x01;
												itype.opertype = 0x05;//set response
												factovs = 0;
												for (i = 0; i < (itype.objectn + 1); i++)
												{
													objecti[i].objectvs = 0;
												}
												if (SUCCESS != fastforward_data_pack(cbuf,&itype,objecti,&factovs,0x01))
												{
												#ifdef MAIN_DEBUG
													printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
												#endif
												}
												write(csockfd,cbuf,factovs);
											}//pattern is exist
											else
											{//pattern is not exist
												memset(cbuf,0,sizeof(cbuf));
												if (0 == itype.highbit)
													itype.highbit = 0x01;
												itype.opertype = 0x05;//set response
												factovs = 0;
												for (i = 0; i < (itype.objectn + 1); i++)
												{
													objecti[i].objectvs = 0;
												}
												if (SUCCESS != fastforward_data_pack(cbuf,&itype,objecti,&factovs,0x02))
												{
												#ifdef MAIN_DEBUG
													printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
												#endif
												}
												write(csockfd,cbuf,factovs);
											}//pattern is not exist

											continue;
										}//use one key to green wave
										else if (0xF5 == objecti[0].objectid)
										{//use one key to restore
											fanganID = 0;
										#ifdef MAIN_DEBUG
             								printf("objectid: 0x%02x,fanganID: %d,extend: %d,Line: %d\n", \
														objecti[0].objectid,fanganID,extend,__LINE__);
                                        #endif
											if (0x01 == extend)
											{
												markbit |= 0x0040;//configure data do have update
                                                if (!wait_write_serial(endpipe[1]))
                                                {
                                                    if (write(endpipe[1],"UpdateFile",11) < 0)
                                                    {
                                                    #ifdef MAIN_DEBUG
                                                        printf("Write endpipe err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                                    #endif
                                                    }
                                                }
                                                else
                                                {
                                                #ifdef MAIN_DEBUG
                                                    printf("Can not write endpipe,File: %s,Line: %d\n",__FILE__,__LINE__);
                                                #endif
                                                }
											}	
											continue;
										}//use one key to restore
										else if (0xEF == objecti[0].objectid)
										{//channel traffic control
											if ((markbit2 & 0x0002)||(markbit2 & 0x0004)||(markbit2 & 0x0010))
                                            {//have key or wireless or configure tool is controlling ;
                                                if (0 == itype.highbit)
                                                    itype.highbit = 0x01;
                                                itype.opertype = 0x06;
                                                memset(errbuf,0,sizeof(errbuf));
                                                err_data_pack(errbuf,&itype,0x05,0x00);
                                                write(csockfd,errbuf,sizeof(errbuf));
												netcon[1] = 0;
												fcontrol = 0;
												ccontrol = 0;
                                                continue;
                                            }//have key or wireless or configure tool  is controlling ;
											if (trans & 0x01)
												continue;//transition stage,ignore;	
											netcon[1] = 0xC8;//200
											fcontrol = ccontrol;
											ccontrol = ttct;
											while (1)
                                			{
                                    			memset(cleanb,0,sizeof(cleanb));
                                    			if (read(conpipe[0],cleanb,sizeof(cleanb)) <= 0)
                                        			break;
                                			}
											if (!wait_write_serial(conpipe[1]))
                            				{
                                				if (write(conpipe[1],netcon,sizeof(netcon)) < 0)
                                				{
                                				#ifdef MAIN_DEBUG
                                    				printf("write err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                				#endif
                                				}
                            				}
                            				else
                            				{
                            				#ifdef MAIN_DEBUG
                                				printf("can not write,File:%s,Line:%d\n",__FILE__,__LINE__);
                            				#endif
                            				}
											continue;
										}//channel traffic control
										else if (0xD7 == objecti[0].objectid)
                                        {//heart pant
											//ignore info, must not process;
											continue;
                                        }//heart pant
										else if (0xD8 == objecti[0].objectid)
										{//set configure data
											continue;
										}//set configure data
										else if (0xDA == objecti[0].objectid)
										{//set time
											struct timeval			ngtime;
											memset(&ngtime,0,sizeof(ngtime));
											gettimeofday(&ngtime,NULL);
											tt.tv_sec = ttct;
											tt.tv_usec = 0;		
											if (-1 == settimeofday(&tt,NULL))
                            				{
                            				#ifdef MAIN_DEBUG
                                				printf("set time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            				#endif
                            				}
                            				if (-1 == system("hwclock -w"))
                            				{
                            				#ifdef MAIN_DEBUG
                                				printf("set time error,File: %s,Line: %d\n",__FILE__,__LINE__);
                            				#endif
                            				}
                            				if (-1 == system("sync"))
                            				{
                            				#ifdef MAIN_DEBUG
                                				printf("set time error,File: %s,Line: %d\n",__FILE__,__LINE__);
                            				#endif
                            				}

											if (ngtime.tv_sec > tt.tv_sec)
											{
												if ((ngtime.tv_sec - tt.tv_sec) > 20)
												{
                            						markbit |= 0x0040;//clock data do have update
                            						if (!wait_write_serial(endpipe[1]))
                            						{
                                						if (write(endpipe[1],"UpdateFile",11) < 0)
                                						{
                                						#ifdef MAIN_DEBUG
                                    						printf("Write endpipe err,File: %s,Line: %d\n", \
																	__FILE__,__LINE__);
                                						#endif
                                						}
                            						}
                            						else
                            						{
                            						#ifdef MAIN_DEBUG
                                						printf("Can not write endpipe,File: %s,Line: %d\n", \
																	__FILE__,__LINE__);
                            						#endif
                            						}
                        						}//must reagain get time interval and restart pattern
											}
											else
											{
												if ((tt.tv_sec - ngtime.tv_sec) > 20)
												{
                                    				markbit |= 0x0040;//clock data do have update
                                    				if (!wait_write_serial(endpipe[1]))
                                    				{
                                        				if (write(endpipe[1],"UpdateFile",11) < 0)
                                        				{
                                        				#ifdef MAIN_DEBUG
                                            				printf("Write endpipe err,File: %s,Line: %d\n", \
																	__FILE__,__LINE__);
                                        				#endif
                                        				}
                                    				}
                                    				else
                                    				{
                                    				#ifdef MAIN_DEBUG
                                        				printf("Can not write endpipe,File: %s,Line: %d\n", \
																__FILE__,__LINE__);
                                    				#endif
                                    				}
												}//must reagain get time interval and restart pattern
											}

											memset(cbuf,0,sizeof(cbuf));
                                            if (0 == itype.highbit)
                                                itype.highbit = 0x01;
                                            itype.opertype = 0x05;//set response
                                            factovs = 0;
                                            for (i = 0; i < (itype.objectn + 1); i++)
                                            {
                                                objecti[i].objectvs = 0;
                                            }
                                            if (SUCCESS != server_data_pack(cbuf,&itype,objecti,&factovs))
                                            {
                                            #ifdef MAIN_DEBUG
                                                printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                            #endif
                                                continue;
                                            }
                                            write(csockfd,cbuf,factovs);
												
											gettimeofday(&tt,NULL);
                                            update_event_list(&eventclass,&eventlog,1,86,tt.tv_sec,&markbit);
											continue;
										}//set time
										else if (0xE1 == objecti[0].objectid)
										{//get whether or not wireless terminal is lawful
											if (0x01 == extend)
											{//lawful
												markbit2 |= 0x0020;
											}//lawful
											else if (0x00 == extend)
											{//not lawful
												markbit2 &= 0xffdf;
											}//not lawful
	
                    						continue;	
										}//get whether or not wireless terminal is lawful
										else if (0xFA == objecti[0].objectid)
										{//system send road info;
											roadinfo = ttct;
											gettimeofday(&tt,NULL);
											roadinforeceivetime = tt.tv_sec;
										#ifdef MAIN_DEBUG
											printf("*******************roadinfo: %d,File: %s,Line: %d\n",roadinfo,__FILE__,__LINE__);
										#endif
											unsigned char	roaddata[7] = {0};
											roaddata[0] = 0xF4;
											roaddata[1] = 0x01;// base on huawei device;
											roaddata[2] = (((roadinfo & 0xff000000) >> 24) & 0x000000ff);
											roaddata[3] = (((roadinfo & 0x00ff0000) >> 16) & 0x000000ff);
											roaddata[4] = (((roadinfo & 0x0000ff00) >> 8) & 0x000000ff);
											roaddata[5] = roadinfo & 0x000000ff;
											roaddata[6] = 0xED;

											while (1)
											{	
												memset(cdata,0,sizeof(cdata));
												if (read(flowpipe[0],cdata,sizeof(cdata)) <= 0)
													break;
											}
											if (!wait_write_serial(flowpipe[1]))
											{
												if (write(flowpipe[1],roaddata,sizeof(roaddata)) < 0)
												{
												#ifdef MAIN_DEBUG
													printf("write pipe err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
											}
											else
											{
											#ifdef MAIN_DEBUG
												printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
											}

											continue;
										}//system send road info;
										else if (0xF8 == objecti[0].objectid)
										{//system send delay time of green lamp
										#ifdef MAIN_DEBUG
											printf("*****************extend: %d,ttct: %d,File: %s,Line:%d\n",extend,ttct,__FILE__,__LINE__);
										#endif
											unsigned char		sysdelay[6] = {0};
											sysdelay[0] = 0xF1;
											sysdelay[1] = 0x01;
											sysdelay[2] = extend;//num of detector
											sysdelay[3] = 0;
											sysdelay[4] = ttct;
											sysdelay[5] = 0xED;

											while (1)
											{	
												memset(cdata,0,sizeof(cdata));
												if (read(flowpipe[0],cdata,sizeof(cdata)) <= 0)
													break;
											}
											if (!wait_write_serial(flowpipe[1]))
											{
												if (write(flowpipe[1],sysdelay,sizeof(sysdelay)) < 0)
												{
												#ifdef MAIN_DEBUG
													printf("write pipe err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
											}
											else
											{
											#ifdef MAIN_DEBUG
												printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
											}
											while (1)
											{
												memset(cdata,0,sizeof(cdata));
												if (read(pendpipe[0],cdata,sizeof(cdata)) <= 0)
													break;
											}
											if (!wait_write_serial(pendpipe[1]))
											{
												if (write(pendpipe[1],sysdelay,sizeof(sysdelay)) < 0)
												{
												#ifdef MAIN_DEBUG
													printf("write pipe err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
											}
											else
											{
											#ifdef MAIN_DEBUG
												printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
											}

											continue;	
										}//system send delay time of green lamp
										else if ((0xDB == objecti[0].objectid) && (0x02 == extend))
										{//use software to reboot machine
											gettimeofday(&tt,NULL);
                        					update_event_list(&eventclass,&eventlog,1,87,tt.tv_sec,&markbit);
											close(csockfd);

											if (0 == access("./data/slnum.dat",F_OK))
											{
												system("rm -f ./data/slnum.dat");
											}
											if (markbit & 0x0040)
											{//file is updated or clock is changed
												update = 1;
											}//file is updated or clock is changed
											else
											{
												update = 0;
											}
											FILE	*sfp = NULL;
											sfp = fopen("./data/slnum.dat","wb+");
											if (NULL != sfp)
											{
												fwrite(&retslnum,sizeof(retslnum),1,sfp);
												fwrite(&update,sizeof(update),1,sfp);
											}
											fclose(sfp);

											system("reboot");
	
                    						continue;	
										}//use software to reboot machine
										else if ((0xDB == objecti[0].objectid) && (0x01 == extend))
										{//use hardware to reboot machine
											gettimeofday(&tt,NULL);
                        					update_event_list(&eventclass,&eventlog,1,87,tt.tv_sec,&markbit);

											unsigned char       rm[3] = {0xAC,0xAC,0xED};
                    						if (!wait_write_serial(serial[0]))
                    						{
                        						if (write(serial[0],rm,sizeof(rm)) < 0)
                        						{
                        						#ifdef MAIN_DEBUG
                            						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                        						#endif
                            						continue;
                        						}
                    						}
                    						else
                    						{
                    						#ifdef MAIN_DEBUG
                        						printf("cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
                    						#endif
                        						continue;
                    						}
											close(csockfd);	
										/*
											if (1 == cliyes)
                                            {
                                                close(csockfd);
                                                pthread_cancel(clivid);
                                                pthread_join(clivid,NULL);
                                                cliyes = 0;
                                            }												
										*/
                    						continue;	
										}//use hardware to reboot machine
									}//if (0 == itype.objectn)
									if (0xDD != objecti[0].objectid)
									{//if (0xDD != objecti[0].objectid)
										sret = set_object_value(&stscdata,&itype,objecti);
                                    	if (MEMERR == sret)
                                    	{
                                    	#ifdef MAIN_DEBUG
                                        	printf("Pointer address err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                        	continue;
                                    	}
                                    	if (FAIL == sret)
                                    	{
                                    	#ifdef MAIN_DEBUG
                                        	printf("set object err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                        	if (0 == itype.highbit)
                                            	itype.highbit = 0x01;
                                        	itype.opertype = 0x06;
                                        	memset(errbuf,0,sizeof(errbuf));
                                        	err_data_pack(errbuf,&itype,0x05,0x00);
                                        	write(csockfd,errbuf,sizeof(errbuf));

                                        	for (i = 0; i < (itype.objectn + 1); i++)
                                        	{
                                            	if (0x8D == objecti[i].objectid)
                                            	{
                                                	memset(&sschedule,0,sizeof(sschedule));
                                                	memcpy(&sschedule,&schedule,sizeof(schedule));
                                            	}
                                            	else if (0x8E == objecti[i].objectid)
                                            	{
                                                	memset(&stimesection,0,sizeof(stimesection));
                                                	memcpy(&stimesection,&timesection,sizeof(timesection));
												}
                                            	else if (0x95 == objecti[i].objectid)
                                            	{
                                                	memset(&sphase,0,sizeof(sphase));
                                                	memcpy(&sphase,&phase,sizeof(phase));
                                            	}
                                            	else if (0x97 == objecti[i].objectid)
                                            	{
                                                	memset(&sphaseerror,0,sizeof(sphaseerror));
                                               		memcpy(&sphaseerror,&phaseerror,sizeof(phaseerror));
                                            	}
                                            	else if (0x9F == objecti[i].objectid)
                                            	{
                                                	memset(&sdetector,0,sizeof(sdetector));
                                                	memcpy(&sdetector,&detector,sizeof(detector));
                                            	}
                                            	else if (0xB0 == objecti[i].objectid)
                                            	{
                                                	memset(&schannel,0,sizeof(channel));
                                                	memcpy(&schannel,&channel,sizeof(channel));
                                            	}
                                            	else if (0xC0 == objecti[i].objectid)
                                            	{
                                                	memset(&spattern,0,sizeof(spattern));
                                                	memcpy(&spattern,&pattern,sizeof(pattern));
                                            	}
                                            	else if (0xC1 == objecti[i].objectid)
                                            	{
                                                	memset(&stimeconfig,0,sizeof(stimeconfig));
                                                	memcpy(&stimeconfig,&timeconfig,sizeof(timeconfig));
                                            	}
											}//for (i = 0; i < (itype.objectn + 1); i++)
                                        	continue;
                                    	}//if (FAIL == sret)
									}//if (0xDD != objecti[0].objectid)

                                    //set object value sucessfully
									if (!(markbit & 0x0004))
									{//if (!(markbit & 0x0004))
                                    	if (-1 == system("rm -f ./data/cy_tsc.dat"))
                                    	{
                                    	#ifdef MAIN_DEBUG
                                        	printf("delete file error,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                    	else
                                    	{
                                    	#ifdef MAIN_DEBUG
                                        	printf("delete file succ,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                    	FILE                    *sfp = NULL;
                                    	sfp = fopen(CY_TSC,"wb+");
                                    	if (NULL == sfp)
                                    	{
                                    	#ifdef MAIN_DEBUG
                                        	printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                        	continue;
                                    	}
										fwrite(&stscheader,sizeof(stscheader),1,sfp);
                                    	fwrite(&sunit,sizeof(sunit),1,sfp);
                                    	fwrite(&sschedule,sizeof(sschedule),1,sfp);
                                    	fwrite(&stimesection,sizeof(stimesection),1,sfp);
                                    	fwrite(&spattern,sizeof(spattern),1,sfp);
                                    	fwrite(&stimeconfig,sizeof(stimeconfig),1,sfp);
                                    	fwrite(&sphase,sizeof(sphase),1,sfp);
                                    	fwrite(&sphaseerror,sizeof(sphaseerror),1,sfp);
                                    	fwrite(&schannel,sizeof(schannel),1,sfp);
                                    	fwrite(&schannelhint,sizeof(schannelhint),1,sfp);
                                    	fwrite(&sdetector,sizeof(sdetector),1,sfp);
                                    	fclose(sfp);
										markbit |= 0x0040;//configure data do have update
                                    	if (!wait_write_serial(endpipe[1]))
                                    	{
                                        	if (write(endpipe[1],"UpdateFile",11) < 0)
                                        	{
                                        	#ifdef MAIN_DEBUG
                                            	printf("Write endpipe err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                        	#endif
                                        	}
                                    	}
                                    	else
                                    	{
                                    	#ifdef MAIN_DEBUG
                                        	printf("Can not write endpipe,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
										gettimeofday(&tt,NULL);
                        				update_event_list(&eventclass,&eventlog,1,83,tt.tv_sec,&markbit);
									}//if (!(markbit & 0x0004))

									if (1 == itype.opertype)
									{//set request
										memset(cbuf,0,sizeof(cbuf));
                                    	if (0 == itype.highbit)
                                        	itype.highbit = 0x01;
                                    	itype.opertype = 0x05;//set response
										factovs = 0;
										for (i = 0; i < (itype.objectn + 1); i++)
										{
											objecti[i].objectvs = 0;
										}
                                    	if (SUCCESS != server_data_pack(cbuf,&itype,objecti,&factovs))
                                    	{
                                    	#ifdef MAIN_DEBUG
                                        	printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
											continue;
                                    	}
                                    	write(csockfd,cbuf,factovs);									
									}//set request

									continue;
								}//set request or no response;
							}//if (num > 0)
							else
							{
							#ifdef MAIN_DEBUG
								printf("server is closed or break off,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								if (markbit & 0x4000)//center software is controlling signaler machine
                            	{
                                	//send data to conpipe to degrade
                                	markbit &= 0xbfff;//center software will not controlling signaler machine;
                                	netcon[1] = 0xf1;
                                	while (1)
                                	{
                                    	memset(cleanb,0,sizeof(cleanb));
                                    	if (read(conpipe[0],cleanb,sizeof(cleanb)) <= 0)
                                        	break;
                                	}
                                	if (!wait_write_serial(conpipe[1]))
                                	{
                                    	if (write(conpipe[1],netcon,sizeof(netcon)) < 0)
                                    	{
                                    	#ifdef MAIN_DEBUG
                                        	printf("write error,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                	}
                                	else
                                	{
                                	#ifdef MAIN_DEBUG
                                    	printf("can not write,File:%s,Line:%d\n",__FILE__,__LINE__);
                                	#endif
                                	}
                            	}//if (markbit & 0x4000)//center software is controlling signaler machine
								break;
							}//num <= 0
						}//if (FD_ISSET(csockfd,&cliRead))
						else
						{
							continue;
						}
					}//have data from server
				}//while,read server data;
			//}//connect successfully
		}//1 while
	}//create sockfd successfully

	pthread_exit(NULL);
}

//build watch dog/GPS/client thread
void setup_wd_gps(void *arg)
{
	int						i = 0,j = 0;
	struct timeval			ptt,tgtime,ngtime;
	unsigned char			basewd[3] = {0xA2,0xA2,0xED};
	unsigned char			gpsbuf[512] = {0};
	int						gdn = 0;
	int						year = 0;
	int						mon = 0;
	int						day = 0;
	int						hour = 0;
	int						min = 0;
	int						sec = 0;
	int						week = 0;
	struct tm               *gpstime;
	long					lgtime = 0;
	unsigned char			tn = 0;
	unsigned char			err[10] = {0};
	FILE					*tffp = NULL;
	unsigned char			rn = 0; //report num of detector,added on 20150408
	unsigned char			firstly = 1;
	unsigned char			rfidn = 0;
//	struct sigaction        sa;

	//shield SIGPIPE
 //   memset(&sa,0,sizeof(sa));
//    sa.sa_handler = SIG_IGN;
//    sigaction(SIGPIPE,&sa,0);
	//end shield SIGPIPE	

	//added by sk on 20170208
	pid_t		tid = syscall(SYS_gettid);
	int			skret = setpriority(PRIO_PROCESS,tid,-20);

	//feed watch dog of base board
	if (!wait_write_serial(serial[0]))
	{
		if (write(serial[0],basewd,sizeof(basewd)) < 0)
		{
			markbit |= 0x0800;
			gettimeofday(&ptt,NULL);
			update_event_list(&eventclass,&eventlog,1,15,ptt.tv_sec,&markbit);
            if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
            {
            #ifdef MAIN_DEBUG
               	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
			if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
            {//actively report is not probitted and connect successfully
            	memset(err,0,sizeof(err));
                if (SUCCESS != err_report(err,ptt.tv_sec,1,15))
                {
                #ifdef MAIN_DEBUG
                	printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                #endif
                }
				else
        		{
            		write(csockfd,err,sizeof(err));
        		}
            }
		}
	}
	else
	{
	#ifdef MAIN_DEBUG
		printf("serial cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
	}
	//end feed dog;
	gettimeofday(&ptt,NULL);
	int wid = open("/dev/watchdog",O_RDONLY);
	if (-1 != wid)
	{
	#ifdef MAIN_DEBUG
		printf("open watch dog successfully,wid: %d,File: %s,Line: %d\n",wid,__FILE__,__LINE__);
	#endif
		update_event_list(&eventclass,&eventlog,1,6,ptt.tv_sec,&markbit);
		if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
        {//actively report is not probitted and connect successfully
            memset(err,0,sizeof(err));
            if (SUCCESS != err_report(err,ptt.tv_sec,1,6))
            {
            #ifdef MAIN_DEBUG
                printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
            #endif
            }
			else
        	{
            	write(csockfd,err,sizeof(err));
        	}
        }
		i = 0;
		tn = 0;
		while(1)
		{//0
			sleep(10);

			if((!(markbit & 0x4000))&&(!(markbit2 & 0x0008))&&(!(markbit2 & 0x0004)) \
				&&(!(markbit2 & 0x0002))&&(!(markbit2 & 0x0010)))
			{//not control mode
				if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
				{//report info to server actively
					if ((2 == contmode) || (1 == contmode) || (3 == contmode))
					{//if ((2 == contmode) || (1 == contmode) || (3 == contmode))
						unsigned char       sibuf[64] = {0};
						unsigned char		*csta = sinfo.csta;
						unsigned char		tcsta = 0;
						sinfo.conmode = contmode;
						sinfo.pattern = retpattern;
						sinfo.cyclet = 0;
						sinfo.time = 0;
						if (1 == contmode)
							sinfo.color = 0x04;
						if (2 == contmode)
							sinfo.color = 0x05;
						if (3 == contmode)
							sinfo.color = 0x00;
						sinfo.chans = 0;
						memset(sinfo.csta,0,sizeof(sinfo.csta));
						csta = sinfo.csta;
						for (i = 0; i < MAX_CHANNEL; i++)
						{
							sinfo.chans += 1;
							tcsta = i + 1;
							tcsta <<= 2;
							tcsta &= 0xfc;
							*csta = tcsta;
							csta++;
						}
						if (SUCCESS != status_info_report(sibuf,&sinfo))
						{
						#ifdef MAIN_DEBUG 
							printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
						else
						{
							write(csockfd,sibuf,sizeof(sibuf));
						}
					}//if ((2 == contmode) || (1 == contmode) || (3 == contmode))
				}//report info to server actively
			}//not control mode

			rfidn += 1;
			if (60 == rfidn)
			{//10 minutes arrived;
				rfidn = 0;
				if (rfidm & 0x01)
				{//North Reader and Writer has been err;
					gettimeofday(&ptt,NULL);
                    update_event_list(&eventclass,&eventlog,1,97,ptt.tv_sec,&markbit);
                    if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                    {
                    #ifdef MAIN_DEBUG
                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                    }
					
                    if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                    {//actively report is not probitted and connect successfully
                        memset(err,0,sizeof(err));
                        if (SUCCESS != err_report(err,ptt.tv_sec,0x18,0x01))
                        {
                        #ifdef MAIN_DEBUG
                            printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                        #endif
                        }
                        else
                        {
                            write(csockfd,err,sizeof(err));
                        }
                    }
				}//North Reader and Writer has been err;
				else if (rfidm & 0x02)
				{//East Reader and Writer has been err;
					gettimeofday(&ptt,NULL);
                    update_event_list(&eventclass,&eventlog,1,98,ptt.tv_sec,&markbit);
                    if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                    {
                    #ifdef MAIN_DEBUG
                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                    }
                    
                    if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                    {//actively report is not probitted and connect successfully
                        memset(err,0,sizeof(err));
                        if (SUCCESS != err_report(err,ptt.tv_sec,0x18,0x02))
                        {
                        #ifdef MAIN_DEBUG
                            printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                        #endif
                        }
                        else
                        {
                            write(csockfd,err,sizeof(err));
                        }
                    }
				}//East Reader and Writer has been err;
				else if (rfidm & 0x04)
				{//South Reader and Writer has been err;
					gettimeofday(&ptt,NULL);
                    update_event_list(&eventclass,&eventlog,1,99,ptt.tv_sec,&markbit);
                    if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                    {
                    #ifdef MAIN_DEBUG
                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                    }
                    
                    if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                    {//actively report is not probitted and connect successfully
                        memset(err,0,sizeof(err));
                        if (SUCCESS != err_report(err,ptt.tv_sec,0x18,0x03))
                        {
                        #ifdef MAIN_DEBUG
                            printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                        #endif
                        }
                        else
                        {
                            write(csockfd,err,sizeof(err));
                        }
                    }
				}//South Reader and Writer has been err;
				else if (rfidm & 0x08)
				{//West Reader and Writer has been err;
					gettimeofday(&ptt,NULL);
                    update_event_list(&eventclass,&eventlog,1,100,ptt.tv_sec,&markbit);
                    if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                    {
                    #ifdef MAIN_DEBUG
                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                    }
                    
                    if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                    {//actively report is not probitted and connect successfully
                        memset(err,0,sizeof(err));
                        if (SUCCESS != err_report(err,ptt.tv_sec,0x18,0x04))
                        {
                        #ifdef MAIN_DEBUG
                            printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                        #endif
                        }
                        else
                        {
                            write(csockfd,err,sizeof(err));
                        }
                    }
				}//West Reader and Writer has been err;
			}//10 minutes arrived;
	
		//	#ifdef WUXI_CHECK
		//	printf("Copy file and pattern,File:%s,Line:%d\n",__FILE__,__LINE__);
		//	system("cp -rf /mnt/nandflash/TscBin  /mnt/usb1/update");	
		//	#endif

			i++;
			rn++; //added on 20150408
			ioctl(wid, WDIOC_KEEPALIVE, 0); //feed watch dog of arm
			#ifdef MAIN_DEBUG
			printf("rn:%d,update:%d,retslnum:%d,lip:%d,seryes: %d,cliyes: %d,markbit2: 0x%04x,markbit: 0x%04x, \
					Line: %d\n",rn,update,retslnum,lip,seryes,cliyes,markbit2,markbit,__LINE__);
			#endif
			//added on 20150408
			if ((markbit2 & 0x0080) && (2 == rn))
			{
			#ifdef MAIN_DEBUG
				printf("Begin to check err of detector,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
				rn = 0;
				for (j = 0; j < DETECTORNUM; j++)
				{
					if (0 == detepro[j].deteid)
						break;
					if (0 == detepro[j].err)
					{
						gettimeofday(&ptt,NULL);
						if ((ptt.tv_sec - detepro[j].stime.tv_sec) >= detepro[j].intime)
						{
							detepro[j].err = 1;
							if (0 == detepro[j].ereport)
							{
								detepro[j].ereport = 1;
								if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
        						{//actively report is not probitted and connect successfully
            						memset(err,0,sizeof(err));

            						if (SUCCESS != detect_err_pack(err,0x14,detepro[j].deteid))
            						{
            						#ifdef MAIN_DEBUG
                						printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(csockfd,err,sizeof(err));
            						}
        						}
							}
						}
					}//if (0 == detepro[j].err)
				}//for (j = 0; j < DETECTORNUM; j++)
			}//if ((markbit2 & 0x0080) && (2 == rn))
			else if (2 == rn)
			{
				rn = 0;
			}
			//end add

			if (markbit & 0x0800)
			{
				for (j = 0; j < 5; j++)
				{
					close(serial[j]);
				}
				memset(serial,0,sizeof(serial));
				if (SUCCESS != mc_open_serial_port(serial))
				{
				#ifdef MAIN_DEBUG
					printf("open serial port error,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					gettimeofday(&ptt,NULL);
					update_event_list(&eventclass,&eventlog,1,3,ptt.tv_sec,&markbit);
        			if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
        			{
        			#ifdef MAIN_DEBUG
        				printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
        			#endif
        			}
					if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
        			{//actively report is not probitted and connect successfully
            			memset(err,0,sizeof(err));
            			if (SUCCESS != err_report(err,ptt.tv_sec,1,3))
            			{
            			#ifdef MAIN_DEBUG
                			printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
            			#endif
            			}
						else
        				{
            				write(csockfd,err,sizeof(err));
        				}
        			}
					markbit |= 0x0800;
				}
				else
				{
					markbit &= 0xf7ff;
				}

				if (SUCCESS != mc_set_serial_port(serial))
				{
				#ifdef MAIN_DEBUG
					printf("set serial port error,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					gettimeofday(&ptt,NULL);
					update_event_list(&eventclass,&eventlog,1,5,ptt.tv_sec,&markbit);
        			if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
        			{
        			#ifdef MAIN_DEBUG
        				printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
        			#endif
        			}
					if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
        			{//actively report is not probitted and connect successfully
            			memset(err,0,sizeof(err));
            			if (SUCCESS != err_report(err,ptt.tv_sec,1,5))
            			{
            			#ifdef MAIN_DEBUG
                			printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
            			#endif
            			}
						else
        				{
            				write(csockfd,err,sizeof(err));
        				}
        			}
					markbit |= 0x0800;
				}
				else
				{
					markbit &= 0xf7ff;
				}
			}//if (markbit & 0x0800)

			//feed watch dog of base board
			if (!wait_write_serial(serial[0]))
			{
				if (write(serial[0],basewd,sizeof(basewd)) < 0)
				{
					markbit |= 0x0800;
					gettimeofday(&ptt,NULL);
					update_event_list(&eventclass,&eventlog,1,15,ptt.tv_sec,&markbit);
                    if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                    {
                    #ifdef MAIN_DEBUG
                    	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                    }
					if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
        			{//actively report is not probitted and connect successfully
            			memset(err,0,sizeof(err));
            			if (SUCCESS != err_report(err,ptt.tv_sec,1,15))
            			{
            			#ifdef MAIN_DEBUG
                			printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
            			#endif
            			}
						else
        				{
            				write(csockfd,err,sizeof(err));
        				}
        			}
				}
			}
			else
			{
			#ifdef MAIN_DEBUG
				printf("serial cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
			//end feed dog;
			if (markbit & 0x0008)
			{//have flow update
				tffp = fopen(FLOWLOG,"wb+");
				if (NULL == tffp)
				{
				#ifdef MAIN_DEBUG
					printf("open file err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					fwrite(&detectdata,sizeof(DetectorData_t),1,tffp);	
					markbit &= 0xfff7;
				}
				fclose(tffp);	
			}//have flow update

			if (markbit & 0x0200)
			{//there is event is happening;
			#ifdef MAIN_DEBUG
				printf("Begin to generate new text file,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
				if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
				{
				#ifdef MAIN_DEBUG
					printf("generate text file err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
			}//there is event is happening;
			if (markbit & 0x2000)
			{
			#ifdef MAIN_DEBUG
                printf("Begin to generate new text file,File:%s,Line:%d\n",__FILE__,__LINE__);
            #endif
				if (SUCCESS != generate_lamp_err_file(&lamperrtype,&markbit))
				{
				#ifdef MAIN_DEBUG
                    printf("generate text file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
				}
			#ifdef MAIN_DEBUG
				printf("\n");
			#endif
			}

			while (1)
            {
                memset(gpsbuf,0,sizeof(gpsbuf));
                if (read(serial[3],gpsbuf,sizeof(gpsbuf)) <= 0)
                    break;
            }

	//	#ifdef GPS
			#ifdef MAIN_DEBUG
			printf("********firstly:%d,auxfunc: 0x%02x,i: %d,Line: %d\n",firstly,auxfunc,i,__LINE__);
			#endif
			if (auxfunc & 0x02)
			{//if (auxfunc & 0x02) 
				if (1 == firstly)
				{
				#ifdef MAIN_DEBUG
					printf("**********************firstly:%d,File:%s,Line:%d\n",firstly,__FILE__,__LINE__);
				#endif
					i = GC1;
					firstly = 0;
				}
				if (GC1 == i)//one day cycle
				{
				#ifdef MAIN_DEBUG
					printf("******************tn: %d,File: %s,Line: %d\n",tn,__FILE__,__LINE__);
				#endif
					if (TNN == tn)
					{
						tn = 0;
						i = 0;
						continue;
					}

					fd_set				nRead;
					struct timeval		gtt;
					gtt.tv_sec = 3;
					gtt.tv_usec= 0;
					FD_ZERO(&nRead);
					FD_SET(serial[3],&nRead);
					int ret = select(serial[3]+1,&nRead,NULL,NULL,&gtt);
					if (ret < 0)
					{
					#ifdef MAIN_DEBUG
						printf("select call error,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						continue;
					}
					if (0 == ret)
					{
					#ifdef MAIN_DEBUG
						printf("Read GPS data,time out,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
	//					ioctl(wid, WDIOC_KEEPALIVE, 0); //feed dog of arm
						i = GC2;
						tn++;
						continue;
					}//time out 
					if (ret > 0)
					{
	//					ioctl(wid, WDIOC_KEEPALIVE, 0); //feed dog of arm
						if (FD_ISSET(serial[3],&nRead))
						{
						//some code to read GPS data from serial
							memset(gpsbuf,0,sizeof(gpsbuf));
							memset(&jwd,0,sizeof(jwd));
							gdn = read(serial[3],gpsbuf,sizeof(gpsbuf));
							gps_parse(gdn,gpsbuf,&year,&mon,&day,&hour,&min,&sec,&week,&jwd);
						#ifdef MAIN_DEBUG
							printf("read %d bytes GPS data, %s,File:%s,Line:%d\n",gdn,gpsbuf,__FILE__,__LINE__);
							printf("year: %d,mon: %d,day: %d,hour: %d,min: %d,sec: %d,week: %d\n",year,mon, \
									day,hour,min,sec,week);
							printf("%d.%d,%d.%d\n",jwd.wdz,jwd.wdx,jwd.jdz,jwd.jdx);
							printf("**********Begin to send jingwei info*************,File: %s,Line: %d\n",__FILE__,__LINE__);
							if ( (0 == jwd.wdz) && (0 == jwd.jdz))
							{
								jwd.wdz = 32;
								jwd.wdx = 297752;
								jwd.jdz = 118;
								jwd.jdx = 327792;
							}
							memset(jwdi,0,sizeof(jwdi));
							jwdi[0] = 0xFA;
							jwdi[1] = 0x01;
							jwdi[2] |= (jwd.jdz & 0x0000ff00) >> 8;
							jwdi[3] |= jwd.jdz & 0x000000ff;
							jwdi[4] |= (jwd.jdx & 0x0000ff00) >> 8;
							jwdi[5] |= jwd.jdx & 0x000000ff;
							jwdi[6] |= (jwd.wdz & 0x0000ff00) >> 8;
							jwdi[7] |= jwd.wdz & 0x000000ff;
							jwdi[8] |= (jwd.wdx & 0x0000ff00) >> 8;
							jwdi[9] |= jwd.wdx & 0x000000ff;
							jwdi[10] = 0xED;

							if (!wait_write_serial(serial[0]))
							{
								if (write(serial[0],jwdi,sizeof(jwdi)) < 0)
								{
								#ifdef MAIN_DEBUG
									printf("write base board serial err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
							}
							else
							{
							#ifdef MAIN_DEBUG
								printf("serial cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}
						#endif

							if ((0 < mon) && (0 < day) && (2000 < year))
							{//if ((0 < mon) && (0 < day) && (0 < year))
							#ifdef MAIN_DEBUG
								printf("*****************Begin to set time**********************\n");
							#endif
								memset(&ngtime,0,sizeof(ngtime));
								gettimeofday(&ngtime,NULL);
								gpstime = (struct tm *)get_system_time();
								gpstime->tm_year = year - 1900;
								gpstime->tm_mon = mon - 1;
								gpstime->tm_mday = day;
								gpstime->tm_hour = hour;
								gpstime->tm_min = min;
								gpstime->tm_sec = sec;
								gpstime->tm_wday = week ;
								lgtime = (long)mktime(gpstime);
								tgtime.tv_sec = lgtime;
								tgtime.tv_usec = 0;
								if (-1 == settimeofday(&tgtime,NULL))
								{
								#ifdef MAIN_DEBUG
									printf("set system time err,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
									i = GC2;
									tn++;
									continue;
								}
								if (-1 == system("hwclock -w"))
								{
								#ifdef MAIN_DEBUG
									printf("set time error,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									i = GC2;
									tn++;
									continue;
								}
								if (-1 == system("sync"))
								{
								#ifdef MAIN_DEBUG
									printf("set time error,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									i = GC2;
									tn++;
									continue;
								}

								if (ngtime.tv_sec > tgtime.tv_sec)
								{
									if ((ngtime.tv_sec - tgtime.tv_sec) > 20)
									{
										markbit |= 0x0040;//clock data do have update
										if (!wait_write_serial(endpipe[1]))
										{
											if (write(endpipe[1],"UpdateFile",11) < 0)
											{
											#ifdef MAIN_DEBUG
												printf("Write err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
											}
										}
										else
										{
										#ifdef MAIN_DEBUG
											printf("Can not write ,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}//must reagain get time interval and restart pattern
								}
								else
								{
									if ((tgtime.tv_sec - ngtime.tv_sec) > 20)
									{
										markbit |= 0x0040;//clock data do have update
										if (!wait_write_serial(endpipe[1]))
										{
											if (write(endpipe[1],"UpdateFile",11) < 0)
											{
											#ifdef MAIN_DEBUG
												printf("Write err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
											}
										}
										else
										{
										#ifdef MAIN_DEBUG
											printf("Can not write ,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}//must reagain get time interval and restart pattern
								}
								i = 0;
								tn = 0;
								continue;						
							}//if ((0 < mon) && (0 < day) && (2000 < year))
							else
							{
								i = GC2;
								tn++;
								continue;
							}
						}//if (FD_ISSET(serial[3],&nRead))
						else
						{
							i = GC2;
							tn++;
							continue;
						}
					}//ret >0	
				}//one day period
			}//if (auxfunc & 0x02)
			else
			{
	//	#else
				i = 0;
				continue;
	//	#endif
			}
		}//0,while loop
	}
	else
	{
	#ifdef MAIN_DEBUG
		printf("Open watch dog error,File: %s,Line:%d\n",__FILE__,__LINE__);
	#endif
		update_event_list(&eventclass,&eventlog,1,7,ptt.tv_sec,&markbit);
        if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
        {
        #ifdef MAIN_DEBUG
        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
        }
		if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
        {//actively report is not probitted and connect successfully
            memset(err,0,sizeof(err));
            if (SUCCESS != err_report(err,ptt.tv_sec,1,7))
            {
            #ifdef MAIN_DEBUG
                printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
            #endif
            }
			else
        	{
            	write(csockfd,err,sizeof(err));
        	}
        }
	}
	pthread_exit(NULL);
}

//build a udp server that configure tool can get ip address of server
void setup_udp_server(void *arg)
{
	int 						socket_udp;
	struct	sockaddr_in			udp_addr;
	int							udp_len;
	unsigned char				buf[32] = {0};

	bzero(&udp_addr,sizeof(udp_addr));
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_addr.s_addr = INADDR_ANY;
	udp_addr.sin_port = htons(UDP_SERVER);
	udp_len = sizeof(udp_addr);

	socket_udp = socket(AF_INET,SOCK_DGRAM,0);
	bind(socket_udp,(struct sockaddr *)&udp_addr,sizeof(udp_addr));
	while(1)
	{
		recvfrom(socket_udp,buf,sizeof(buf),0,(struct sockaddr *)&udp_addr,&udp_len);
		if (!strncmp(buf,"GetIPAddr",9))
		{

			unsigned char			ip[64] = {0};
			unsigned char			netmask[64] = {0};
			unsigned char			gw[64] = {0};
			FILE					*ipfp = NULL;
			unsigned char			msg[64] = {0};
			int						pos = 0;
			unsigned char			ipmark = 0;
			unsigned char			sendmsg[128] = {0};

			ipfp = fopen(IPFILE,"rb");
			if (NULL == ipfp)
			{
			#ifdef MAIN_DEBUG
				printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				continue;
			}
			while (1)
			{
				memset(msg,0,sizeof(msg));
				if (NULL == fgets(msg,sizeof(msg),ipfp))
					break;
				if (SUCCESS == find_out_child_str(msg,"DefaultGateway",&pos))
				{
					strcpy(gw,msg);
					continue;
				}
				if (0 == ipmark)
				{//have two "IPAddress" in userinfo.txt,we need first "IPAddress"; 
					if (SUCCESS == find_out_child_str(msg,"IPAddress",&pos))
					{
						strcpy(ip,msg);
						ipmark = 1;
						continue;
					}
				}
				if (SUCCESS == find_out_child_str(msg,"SubnetMask",&pos))
				{
					strcpy(netmask,msg);
					continue;
				}
			}//while (NULL != fgets(msg,sizeof(msg),ipfp))
			strcpy(sendmsg,"CYT8,DHCP=\"0\",");
			//the end of gw is space;
			strncat(sendmsg,gw,(strlen(gw)-1));
			strcat(sendmsg,",");
			//the end of ip is space
			strncat(sendmsg,ip,(strlen(ip)-1));
			strcat(sendmsg,",");
			//the end of netmask is space
			strncat(sendmsg,netmask,(strlen(netmask)-1));
			strcat(sendmsg,",END");

			sendto(socket_udp,sendmsg,strlen(sendmsg),0,(struct sockaddr *)&udp_addr,udp_len);
		}//GetIPAddr
	}	

	return;
}


int SendDataToPie(unsigned char *data,unsigned short len)
{

	if (NULL == data)
	{
	#ifdef MAIN_DEBUG
		printf("Pointer address err,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR; 
	}

	unsigned char			cdata[256] = {0};
	unsigned char			spbuf[512] = {0};
	memcpy(spbuf,data,sizeof(spbuf));

	if(len > 512)
	{
		len = 512;
	}

	
	while (1)
	{	
		memset(cdata,0,sizeof(cdata));
		if (read(flowpipe[0],cdata,sizeof(cdata)) <= 0)
			break;
	}
	//end clean pipe

	if (!wait_write_serial(flowpipe[1]))
	{
		if (write(flowpipe[1],spbuf,len) < 0)
		{
		#ifdef MAIN_DEBUG
			printf("write pipe err,File:%s,Line:%d\n",__FILE__,__LINE__);
		#endif
		}
	}
	else
	{
		#ifdef MAIN_DEBUG
		printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
	}
	while (1)
	{
		memset(cdata,0,sizeof(cdata));
		if (read(pendpipe[0],cdata,sizeof(cdata)) <= 0)
			break;
	}
	if (!wait_write_serial(pendpipe[1]))
	{
		if (write(pendpipe[1],spbuf,len) < 0)
		{
		#ifdef MAIN_DEBUG
			printf("write pipe err,File:%s,Line:%d\n",__FILE__,__LINE__);
		#endif
		}
	}
	else
	{
		#ifdef MAIN_DEBUG
		printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
	}
	return SUCCESS;
}


int ParserHeadWayData(unsigned char *data,unsigned short len)
{
	if (NULL == data)
	{
		#ifdef MAIN_DEBUG
		printf("Pointer address err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		return MEMERR; 
	}	
	SendDataToPie(data,len);
	return SUCCESS;
}

int ParserQuenueData(unsigned char *data,unsigned short len)
{
	if (NULL == data)
	{
		#ifdef MAIN_DEBUG
		printf("Pointer address err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		return MEMERR; 
	}
	SendDataToPie(data,len);
	return SUCCESS;
}

int ParserTrafficIncident(unsigned char *data)
{
	if (NULL == data)
	{
		#ifdef MAIN_DEBUG
		printf("Pointer address err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		return MEMERR; 
	}

	unsigned char		*psbuf = data;
	unsigned int		uiTrafficIncidentData = 0;
	unsigned int		temp = 0;
	unsigned char		cdata[256] = {0};
	unsigned char		roaddata[7] = {0};
	struct timeval		stCurrentTime;

	
	psbuf=psbuf+5;
	temp = 0;
	temp |= *psbuf;
	temp <<= 24;
	temp &= 0xff000000;
	uiTrafficIncidentData |= temp;

	psbuf++;
	temp = 0;
	temp |= *psbuf;
	temp <<= 16;
	temp &= 0x00ff0000;
	uiTrafficIncidentData |= temp;

	psbuf++;
	temp = 0;
	temp |= *psbuf;
	temp <<= 8;
	temp &= 0x0000ff00;
	uiTrafficIncidentData |= temp;			

	psbuf++;
	temp = 0;
	temp |= *psbuf;
	temp &= 0x000000ff;
	uiTrafficIncidentData |= temp;

	

	roadinfo = uiTrafficIncidentData;
	gettimeofday(&stCurrentTime,NULL);
	roadinforeceivetime = stCurrentTime.tv_sec;
	#ifdef MAIN_DEBUG
	printf("*******************roadinfo: %d,File: %s,Line: %d\n",roadinfo,__FILE__,__LINE__);
	#endif
	
	roaddata[0] = 0xF4;
	roaddata[1] = 0x01;// base on huawei device;
	roaddata[2] = (((roadinfo & 0xff000000) >> 24) & 0x000000ff);
	roaddata[3] = (((roadinfo & 0x00ff0000) >> 16) & 0x000000ff);
	roaddata[4] = (((roadinfo & 0x0000ff00) >> 8) & 0x000000ff);
	roaddata[5] = roadinfo & 0x000000ff;
	roaddata[6] = 0xED;

	while (1)
	{	
		memset(cdata,0,sizeof(cdata));
		if (read(flowpipe[0],cdata,sizeof(cdata)) <= 0)
			break;
	}
	if (!wait_write_serial(flowpipe[1]))
	{
		if (write(flowpipe[1],roaddata,sizeof(roaddata)) < 0)
		{
			#ifdef MAIN_DEBUG
			printf("write pipe err,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
		}
	}
	else
	{
		#ifdef MAIN_DEBUG
		printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
	}
	return SUCCESS;
}

int ParsePersonData(unsigned char *data)
{

	if (NULL == data)
	{
		#ifdef MAIN_DEBUG
		printf("Pointer address err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		return MEMERR; 
	}

	unsigned char 			senddata[6] = {0xF1,0x04,0x00,0x00,0x00,0xED};
	unsigned char           radar_address = 0;
	unsigned char           persoin_wait_zone = 0,persoin_walk_zone = 0;

	unsigned char           i = 0,j=0;
	unsigned char 			fbdata[4] = {0};
	unsigned char			err[10] = {0};
	unsigned char			dibuf[16] = {0};
	detectinfo_t			dinfo;
	unsigned int			phaseid = 0;
	unsigned short			factfn = 0;
	unsigned char			dfdata[8] = {0};//detector flow data
	unsigned char			dedata[8] = {0};//detector err data
	struct timeval			stt;

	dinfo.pattern = &retpattern;
	dinfo.stage = &retstageid;
	dinfo.color = &retcolor;
	dinfo.phase = &retphaseid;
	dinfo.detectid = 0;
	dinfo.time = 0;
	fbdata[0] = 0xC9;
	fbdata[3] = 0xED;

	dedata[0] = 'C';
	dedata[1] = 'Y';
	dedata[2] = 'T';
	dedata[3] = 'A';
	dedata[5] = 'E';
	dedata[6] = 'N';
	dedata[7] = 'D';
	
	dfdata[0] = 'C';
	dfdata[1] = 'Y';
	dfdata[2] = 'T';
	dfdata[3] = 'B';
	dfdata[5] = 'E';
	dfdata[6] = 'N';
	dfdata[7] = 'D';



	
	
	radar_address = data[1];
	persoin_wait_zone = data[19];
	persoin_walk_zone = data[31];
	if((persoin_wait_zone == 1)&&(persoin_walk_zone == 0))
	{

		if((radar_address == 0x07)||(radar_address == 0x0f))
		{
			senddata[2] = 49;
		}
		if((radar_address == 0x17)||(radar_address == 0x1f))
		{
			senddata[2] = 51;
		}
		SendDataToPie(senddata,6);
	}
	if((persoin_wait_zone == 0)&&(persoin_walk_zone == 1))
	{

		if((radar_address == 0x07)||(radar_address == 0x0f))
		{
			senddata[2] = 50;
		}
		if((radar_address == 0x17)||(radar_address == 0x1f))
		{
			senddata[2] = 52;
		}
		SendDataToPie(senddata,6);
	}
	if((persoin_wait_zone == 1)&&(persoin_walk_zone == 1))
	{

		if((radar_address == 0x07)||(radar_address == 0x0f))
		{
			senddata[2] = 49;
		}
		if((radar_address == 0x17)||(radar_address == 0x1f))
		{
			senddata[2] = 51;
		}
		SendDataToPie(senddata,6);
		
		if((radar_address == 0x07)||(radar_address == 0x0f))  //南北向斑马线
		{
			senddata[2] = 50;
		}
		if((radar_address == 0x17)||(radar_address == 0x1f))  //东西向斑马线
		{
			senddata[2] = 52;
		}
		SendDataToPie(senddata,6);
	}


	fbdata[1] = senddata[1];
	fbdata[2] = senddata[2];
	if (!wait_write_serial(serial[2]))//下发面板
	{
		if (write(serial[2],fbdata,sizeof(fbdata)) < 0)
		{
	#ifdef MAIN_DEBUG
			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
			markbit |= 0x0800;
			gettimeofday(&stt,NULL);
			update_event_list(&eventclass,&eventlog,1,16,stt.tv_sec,&markbit);
			if(SUCCESS!=generate_event_file(&eventclass,&eventlog,softevent,&markbit))
			{
        #ifdef MAIN_DEBUG
				printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
			}
			if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
			{//actively report is not probitted and connect successfully
				memset(err,0,sizeof(err));
				if (SUCCESS != err_report(err,stt.tv_sec,1,16))
				{
            	#ifdef MAIN_DEBUG
					printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
            	#endif
				}
				else
				{
					write(csockfd,err,sizeof(err));
				}
			}
		}
	}
	else
	{
	#ifdef MAIN_DEBUG
		printf("cannot write serial,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
	}




	markbit2 |= 0x0080;//there are flow data or detector has been linked
	if ((0x02 != senddata[1]) && (0x03 != senddata[1]))
	{
		for (i = 0; i < DETECTORNUM; i++)
		{
			if (0 == detepro[i].deteid)
				break;
			if (detepro[i].deteid == senddata[2])
			{
				detepro[i].err = 0;
				if (1 == detepro[i].ereport)
				{
					detepro[i].ereport = 0;
					if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
					{
						memset(err,0,sizeof(err));
						if (SUCCESS != detect_err_pack(err,0x17,detepro[i].deteid))
						{
					#ifdef MAIN_DEBUG
							printf("detect report err,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
						}
						else
						{
							write(csockfd,err,sizeof(err)); 				
						}
					}//if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))	
				}
				gettimeofday(&detepro[i].stime,NULL);
			}//if (detepro[i].deteid == *(pspbuf+mark+2))
		}
	}




	//secondly,flow msg will be written to ./data/flow.dat
	if (0x01 == senddata[1])
	{//vehicle pass detector;
		gettimeofday(&stt,NULL);
		if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
		{//actively report is not probitted and connect successfully
			dinfo.time = stt.tv_sec;
			dinfo.detectid = senddata[2];
			memset(dibuf,0,sizeof(dibuf));
			if (SUCCESS != detect_info_report(dibuf,&dinfo))
			{
		#ifdef MAIN_DEBUG
				printf("detect_info_report err,File: %s,LIne: %d\n",__FILE__,__LINE__);
		#endif
			}
			else
			{
				write(csockfd,dibuf,sizeof(dibuf));
			}	
		}//actively report is not probitted and connect successfully	

		phaseid = 0;
		factfn = 0;
		for (j = 0; j < detector.FactDetectorNum; j++)
		{
			if (0 == detector.DetectorList[j].DetectorId)
				break;
			if (senddata[2] == detector.DetectorList[j].DetectorId)
			{
				phaseid |= (0x01 << (detector.DetectorList[j].DetectorPhase - 1));
			}
		}//a detector map serval phases;
		if (detectdata.DataId >= FLOWNUM)
		{
			detectdata.DataId = 1;
		}
		else
		{
			detectdata.DataId += 1;
		}
		if (detectdata.FactFlowNum < FLOWNUM)
		{
			detectdata.FactFlowNum += 1;
		}
		factfn = detectdata.DataId;
		detectdata.DetectorDataList[factfn-1].DetectorId = senddata[2];
		detectdata.DetectorDataList[factfn-1].DetectorData = 1;
		detectdata.DetectorDataList[factfn-1].PhaseId = phaseid;
		gettimeofday(&stt,NULL);
		detectdata.DetectorDataList[factfn-1].RecvTime = stt.tv_sec;
//								fwrite(&detectdata,sizeof(detectdata),1,ffp);
		markbit |= 0x0008; //flow have update

		if ((markbit & 0x0010) && (markbit & 0x0002))
		{
		#ifdef MAIN_DEBUG
			printf("send flow to conftool,deteid: %d,File:%s,Line:%d\n",senddata[2],__FILE__,__LINE__);
		#endif
			dfdata[4] = senddata[2];
			if (write(ssockfd,dfdata,sizeof(dfdata)) < 0)
			{
				gettimeofday(&stt,NULL);
				update_event_list(&eventclass,&eventlog,1,20,stt.tv_sec,&markbit);
				if (SUCCESS != \
					generate_event_file(&eventclass,&eventlog,softevent,&markbit))
				{
			#ifdef MAIN_DEBUG
					printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
				}
				if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
				{//actively report is not probitted and connect successfully
					memset(err,0,sizeof(err));
					if (SUCCESS != err_report(err,stt.tv_sec,1,20))
					{
               	#ifdef MAIN_DEBUG
						printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
               	#endif
					}
					else
					{
						write(csockfd,err,sizeof(err));
					}
				}
			}
		}//if ((markbit & 0x0010) && (markbit & 0x0002))
	}//if (0x01 == *(pspbuf+mark+1))



	if (0x03 == senddata[1])
	{//detector err
		unsigned char				detectID = 0;
		gettimeofday(&stt,NULL);
		detectID = senddata[2];
		update_event_list(&eventclass,&eventlog,20,detectID,stt.tv_sec,&markbit);
		if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
		{
    	#ifdef MAIN_DEBUG
			printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
   		 #endif
		}
	
		//added on 20150408
		for (i = 0; i < DETECTORNUM; i++)
		{
			if (0 == detepro[i].deteid)
				break;
			if (detepro[i].deteid ==senddata[2])
			{
				detepro[i].err = 1;
				if (0 == detepro[i].ereport)
				{
					detepro[i].ereport = 1;
					if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
					{
						memset(err,0,sizeof(err));
						if (SUCCESS!=err_report(err,stt.tv_sec,0x14,detepro[i].deteid))
						{
                   		 #ifdef MAIN_DEBUG
							printf("detect report err,File:%s,Line:%d\n",__FILE__,__LINE__);
                   		 #endif
						}
						else
						{
							write(csockfd,err,sizeof(err));
						}
					}//if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
				}
				gettimeofday(&detepro[i].stime,NULL);
			}//if (detepro[i].deteid == *(pspbuf+mark+2))
		}
		//end add

		//send detector err to conf tool;									
		if ((markbit & 0x0010) && (markbit & 0x0002))
		{
			dedata[4] = senddata[2];
			if (write(ssockfd,dedata,sizeof(dedata)) < 0)
			{
				gettimeofday(&stt,NULL);
				update_event_list(&eventclass,&eventlog,1,20,stt.tv_sec,&markbit);
				if (SUCCESS != \
					generate_event_file(&eventclass,&eventlog,softevent,&markbit))
				{
            	#ifdef MAIN_DEBUG
					printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
            	#endif
				}
				if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
				{//actively report is not probitted and connect successfully
					memset(err,0,sizeof(err));
					if (SUCCESS != err_report(err,stt.tv_sec,1,20))
					{
                #ifdef MAIN_DEBUG
						printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
					}
					else
					{
						write(csockfd,err,sizeof(err));
					}
				}
			}
		}
	}//detector err

	return SUCCESS;
}

int ParserCarPassData(unsigned char *data)
{
	if (NULL == data)
	{
		#ifdef MAIN_DEBUG
		printf("Pointer address err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		return MEMERR; 
	}
	unsigned int    		huangshu = 0,lanenum = 0;
	unsigned char 			i = 0,j=0;
	unsigned char 			senddata[6] = {0xF1,0x00,0x00,0x00,0xEE,0xED};
	unsigned char 			fbdata[4] = {0};
	unsigned char			err[10] = {0};
	unsigned char			dibuf[16] = {0};
	detectinfo_t			dinfo;
	unsigned int			phaseid = 0;
	unsigned short			factfn = 0;
	unsigned char			dfdata[8] = {0};//detector flow data
	unsigned char			dedata[8] = {0};//detector err data
	struct timeval			stt;

	dinfo.pattern = &retpattern;
	dinfo.stage = &retstageid;
	dinfo.color = &retcolor;
	dinfo.phase = &retphaseid;
	dinfo.detectid = 0;
	dinfo.time = 0;
	fbdata[0] = 0xC9;
	fbdata[3] = 0xED;
	huangshu = data[5];

	dedata[0] = 'C';
	dedata[1] = 'Y';
	dedata[2] = 'T';
	dedata[3] = 'A';
	dedata[5] = 'E';
	dedata[6] = 'N';
	dedata[7] = 'D';
	
	dfdata[0] = 'C';
	dfdata[1] = 'Y';
	dfdata[2] = 'T';
	dfdata[3] = 'B';
	dfdata[5] = 'E';
	dfdata[6] = 'N';
	dfdata[7] = 'D';

	
	for(lanenum=6;lanenum<6+huangshu*2;lanenum=lanenum+2)
	{
		//printf("i ： %d , cycle : %d \n",lanenum,(6+huangshu*2));
		senddata[2] = data[lanenum]; //检测器编号
		senddata[1] = data[lanenum+1];//车辆状态

		//SendDataToPie(senddata,6);

		fbdata[1] = senddata[1];
		fbdata[2] = senddata[2];
		if (!wait_write_serial(serial[2]))//下发面板
		{
			if (write(serial[2],fbdata,sizeof(fbdata)) < 0)
			{
			#ifdef MAIN_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				markbit |= 0x0800;
				gettimeofday(&stt,NULL);
				update_event_list(&eventclass,&eventlog,1,16,stt.tv_sec,&markbit);
                if(SUCCESS!=generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                {
                #ifdef MAIN_DEBUG
                    printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
				if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                {//actively report is not probitted and connect successfully
                    memset(err,0,sizeof(err));
                    if (SUCCESS != err_report(err,stt.tv_sec,1,16))
                    {
                    	#ifdef MAIN_DEBUG
                        printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    	#endif
                    }
					else
					{
						write(csockfd,err,sizeof(err));
					}
                }
			}
		}
		else
		{
			#ifdef MAIN_DEBUG
			printf("cannot write serial,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
		}




		markbit2 |= 0x0080;//there are flow data or detector has been linked
		if ((0x02 != senddata[1]) && (0x03 != senddata[1]))
		{
			for (i = 0; i < DETECTORNUM; i++)
			{
				if (0 == detepro[i].deteid)
					break;
				if (detepro[i].deteid == senddata[2])
				{
					detepro[i].err = 0;
					if (1 == detepro[i].ereport)
					{
						detepro[i].ereport = 0;
						if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
						{
							memset(err,0,sizeof(err));
							if (SUCCESS != detect_err_pack(err,0x17,detepro[i].deteid))
							{
							#ifdef MAIN_DEBUG
								printf("detect report err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
							else
							{
								write(csockfd,err,sizeof(err));					
							}
						}//if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))	
					}
					gettimeofday(&detepro[i].stime,NULL);
				}//if (detepro[i].deteid == *(pspbuf+mark+2))
			}
		}




		//secondly,flow msg will be written to ./data/flow.dat
		if (0x01 == senddata[1])
		{//vehicle pass detector;
			SendDataToPie(senddata,6);
			gettimeofday(&stt,NULL);
			if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
			{//actively report is not probitted and connect successfully
				dinfo.time = stt.tv_sec;
				dinfo.detectid = senddata[2];
				memset(dibuf,0,sizeof(dibuf));
				if (SUCCESS != detect_info_report(dibuf,&dinfo))
				{
				#ifdef MAIN_DEBUG
					printf("detect_info_report err,File: %s,LIne: %d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					write(csockfd,dibuf,sizeof(dibuf));
				}	
			}//actively report is not probitted and connect successfully	

			phaseid = 0;
			factfn = 0;
			for (j = 0; j < detector.FactDetectorNum; j++)
			{
				if (0 == detector.DetectorList[j].DetectorId)
					break;
				if (senddata[2] == detector.DetectorList[j].DetectorId)
				{
					phaseid |= (0x01 << (detector.DetectorList[j].DetectorPhase - 1));
				}
			}//a detector map serval phases;
			if (detectdata.DataId >= FLOWNUM)
			{
				detectdata.DataId = 1;
			}
			else
			{
				detectdata.DataId += 1;
			}
			if (detectdata.FactFlowNum < FLOWNUM)
			{
				detectdata.FactFlowNum += 1;
			}
			factfn = detectdata.DataId;
			detectdata.DetectorDataList[factfn-1].DetectorId = senddata[2];
			detectdata.DetectorDataList[factfn-1].DetectorData = 1;
			detectdata.DetectorDataList[factfn-1].PhaseId = phaseid;
			gettimeofday(&stt,NULL);
			detectdata.DetectorDataList[factfn-1].RecvTime = stt.tv_sec;
//								fwrite(&detectdata,sizeof(detectdata),1,ffp);
			markbit |= 0x0008; //flow have update

			if ((markbit & 0x0010) && (markbit & 0x0002))
			{
				#ifdef MAIN_DEBUG
				printf("send flow to conftool,deteid: %d,File:%s,Line:%d\n",senddata[2],__FILE__,__LINE__);
				#endif
				dfdata[4] = senddata[2];
				if (write(ssockfd,dfdata,sizeof(dfdata)) < 0)
				{
					gettimeofday(&stt,NULL);
					update_event_list(&eventclass,&eventlog,1,20,stt.tv_sec,&markbit);
					if (SUCCESS != \
						generate_event_file(&eventclass,&eventlog,softevent,&markbit))
					{
					#ifdef MAIN_DEBUG
    					printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
					if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                   	{//actively report is not probitted and connect successfully
                       	memset(err,0,sizeof(err));
                       	if (SUCCESS != err_report(err,stt.tv_sec,1,20))
                       	{
                       	#ifdef MAIN_DEBUG
                           	printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                       	#endif
                       	}
						else
						{
							write(csockfd,err,sizeof(err));
						}
                   	}
				}
			}//if ((markbit & 0x0010) && (markbit & 0x0002))
		}//if (0x01 == *(pspbuf+mark+1))



		if (0x03 == senddata[1])
		{//detector err
			unsigned char				detectID = 0;
			gettimeofday(&stt,NULL);
			detectID = senddata[2];
			update_event_list(&eventclass,&eventlog,20,detectID,stt.tv_sec,&markbit);
	        if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
	        {
	        	#ifdef MAIN_DEBUG
	            printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
	       		 #endif
	        }
		
			//added on 20150408
			for (i = 0; i < DETECTORNUM; i++)
			{
				if (0 == detepro[i].deteid)
					break;
				if (detepro[i].deteid ==senddata[2])
				{
					detepro[i].err = 1;
					if (0 == detepro[i].ereport)
					{
						detepro[i].ereport = 1;
	                    if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
	                    {
	                        memset(err,0,sizeof(err));
	                        if (SUCCESS!=err_report(err,stt.tv_sec,0x14,detepro[i].deteid))
	                        {
	                       		 #ifdef MAIN_DEBUG
	                            printf("detect report err,File:%s,Line:%d\n",__FILE__,__LINE__);
	                       		 #endif
	                        }
	                        else
	                        {
	                            write(csockfd,err,sizeof(err));
	                        }
	                    }//if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
					}
					gettimeofday(&detepro[i].stime,NULL);
				}//if (detepro[i].deteid == *(pspbuf+mark+2))
			}
			//end add

			//send detector err to conf tool;									
			if ((markbit & 0x0010) && (markbit & 0x0002))
			{
				dedata[4] = senddata[2];
				if (write(ssockfd,dedata,sizeof(dedata)) < 0)
	            {
	                gettimeofday(&stt,NULL);
	               	update_event_list(&eventclass,&eventlog,1,20,stt.tv_sec,&markbit);
	                if (SUCCESS != \
						generate_event_file(&eventclass,&eventlog,softevent,&markbit))
	                {
	                	#ifdef MAIN_DEBUG
	                    printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
	                	#endif
	                }
					if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
	                {//actively report is not probitted and connect successfully
	                    memset(err,0,sizeof(err));
	                    if (SUCCESS != err_report(err,stt.tv_sec,1,20))
	                    {
	                    #ifdef MAIN_DEBUG
	                        printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
	                    #endif
	                    }
						else
						{
							write(csockfd,err,sizeof(err));
						}
	                }
	            }
			}
		}//detector err
		
	}

	return SUCCESS;
}


void ParsePipeData(void *arg)
{
	unsigned short iCurrentPostion = 0;
	unsigned short iMovePostion = 0;
    unsigned char cAllRecviveBuf[51200]={0};
	unsigned char cCurrentRecviveBuf[5120]={0};
	unsigned char *pPosition = NULL;
	unsigned char cbEnd = 0;
	short 		  RecvLen = 0;
	unsigned char bNotHeader = 0;
	int           *readpipe = arg;

	fd_set fdRead;
	int ret = 0;
	struct timeval	aTime;
	while(1)
	{
		FD_ZERO(&fdRead);
		FD_SET(*readpipe, &fdRead);
		aTime.tv_sec = 0;
		aTime.tv_usec = 500000;
		ret = select(*readpipe+1, &fdRead, NULL, NULL, &aTime);
		if(ret < 0)
		{
			printf("ParsePipeData select error,File: %s,Line: %d\n",__FILE__,__LINE__);
			return;
		}
		if(FD_ISSET(*readpipe,&fdRead))
		{
			RecvLen = 0;
			memset(cCurrentRecviveBuf,0,sizeof(cCurrentRecviveBuf));
			RecvLen = read(*readpipe, cCurrentRecviveBuf, sizeof(cCurrentRecviveBuf));
			if(RecvLen > 5120)
			{
				//#ifdef MAIN_DEBUG
				printf("receive client data error max is out 5120,File: %s,Line: %d\n",__FILE__,__LINE__);
				//#endif
				continue;
			}
			if(RecvLen <=0)
			{
				//#ifdef MAIN_DEBUG
				printf("receive client data error,File: %s,Line: %d\n",__FILE__,__LINE__);
				//#endif
				continue;
			}
			//printf("iCurrentPostion : %d,File: %s,Line: %d\n",iCurrentPostion,__FILE__,__LINE__);
			if((iCurrentPostion+RecvLen) >= 51200)
			{
				iCurrentPostion = 0;
				memset(cAllRecviveBuf, 0,sizeof(cAllRecviveBuf));
				printf("receive data and add last datalen max out 512000,File: %s,Line: %d\n",__FILE__,__LINE__);
				
			}
			memcpy(cAllRecviveBuf + iCurrentPostion, cCurrentRecviveBuf, RecvLen);
			iCurrentPostion += RecvLen;
			pPosition = cAllRecviveBuf;
			cbEnd = 0;
			bNotHeader = 0;
			iMovePostion = 0;
			while (1)
			{	//7e 01 02 03 21 05 06 7e 7e 01 02 
				//printf("cbEnd : %d , iMovePostion : %d ,iCurrentPostion : %d ,bNotHeader : %d \n",cbEnd,iMovePostion,iCurrentPostion,bNotHeader);
				if(iMovePostion+1 >= 51200)
				{
					memset(cAllRecviveBuf,0,sizeof(cAllRecviveBuf));
					iMovePostion = 0;
					iCurrentPostion = 0;
					bNotHeader = 0;
					break;
				}
				if(cbEnd == 3)
				{
					break;
				}
				if((*(pPosition+iMovePostion) == 0) && (iMovePostion >=iCurrentPostion))
				{	
					printf("find data and not find,File: %s,Line: %d\n",__FILE__,__LINE__);
					break;
				}
				if(0x7E == *(pPosition+iMovePostion))
				{	
					
					iMovePostion++;
					if(0x7E == *(pPosition+iMovePostion))
					{
						bNotHeader++;
						continue;	
					}
					cbEnd = 1;
					while(1)
					{			
						if(0x7E != *(pPosition+iMovePostion))
						{
							
							if((iCurrentPostion - iMovePostion-1) == 0)//找到最后，没有找到第二个7E,结束查找
							{
								//printf("current page not complete\n");
								cbEnd = 3;//结束查找
								break;
							}
						}
						else if(0x7E == *(pPosition+iMovePostion))//找到第二个7E
						{
							cbEnd = 2;//初始化，重新找
							
							if((iCurrentPostion - iMovePostion-1) == 0)
							{
								//printf("last data is complete\n");
								cbEnd = 3;//结束正好完整的包，结束查找
								unsigned char cOneReceiceData[5120] = {0};
								memcpy(cOneReceiceData, cAllRecviveBuf+bNotHeader, iMovePostion+1-bNotHeader);
								switch (cOneReceiceData[4])
								{
									case 0x05://车头时距
										ParserHeadWayData(cOneReceiceData,iMovePostion+1-bNotHeader);
										break;
									case 0x51://排队数据
										ParserQuenueData(cOneReceiceData,iMovePostion+1-bNotHeader);
										break;
									case 0x08://过车数据
										ParserCarPassData(cOneReceiceData);
										break;
									case 0x06://交通事件数据
										ParserTrafficIncident(cOneReceiceData);
										break;
									case 0x7C://行人请求
										ParsePersonData(cOneReceiceData);
										break;
									default:
										//#ifdef MAIN_DEBUG
										printf("0x7E data but not  parse data and send,File: %s,Line: %d\n",__FILE__,__LINE__);
										//#endif
										break;
								}
								iCurrentPostion = 0;
								memset(cAllRecviveBuf, 0,sizeof(cAllRecviveBuf));
								iMovePostion = 0;
								bNotHeader = 0; 				
								break;					
							}
							else
							{
								//printf("cbEnd : %d , iMovePostion : %d ,iCurrentPostion : %d ,bNotHeader : %d \n",cbEnd,iMovePostion,iCurrentPostion,bNotHeader);
								unsigned char cOneReceiceData[5120] = {0};
								memcpy(cOneReceiceData, cAllRecviveBuf+bNotHeader, iMovePostion+1-bNotHeader);
								iCurrentPostion = iCurrentPostion - iMovePostion-1;
								memcpy(cAllRecviveBuf, cAllRecviveBuf+iMovePostion+1,iCurrentPostion);
								memset(cAllRecviveBuf+iCurrentPostion, 0, iMovePostion+1);
								switch (cOneReceiceData[4])
								{
									case 0x05://车头时距
										ParserHeadWayData(cOneReceiceData,iMovePostion+1-bNotHeader);
										break;
									case 0x51://排队数据
										ParserQuenueData(cOneReceiceData,iMovePostion+1-bNotHeader);
										break;
									case 0x08://过车数据
										ParserCarPassData(cOneReceiceData);
										break;
									case 0x06://交通事件数据
										ParserTrafficIncident(cOneReceiceData);
										break;
									case 0x7C://行人请求
										ParsePersonData(cOneReceiceData);
										break;
									default:
										#ifdef MAIN_DEBUG
										printf("not parse data,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										break;
								}
								iMovePostion = 0;
								bNotHeader = 0;
								break;
							}
						}
						
						if(iMovePostion+1>iCurrentPostion)
						{
							cbEnd = 3;//结束查找
							break;
						}
						else
						{
							iMovePostion++;
						}
						continue;
					}//end while2
				}
				else
				{
					if(iMovePostion+1 == iCurrentPostion)
					{
						bNotHeader=0;
						iMovePostion=0;
						break;
					}
					bNotHeader++;
					iMovePostion++;
					continue;
				}
			}//end while1	

		}
		
	}
	
	pthread_exit(NULL);
}


void ParseRadioInfoOrDectorInfo(void *arg)
{

	ClientSocketInfo_t *pstClientSocketData = arg;
	fd_set fdRead;
	int ret = 0;
	struct timeval	aTime;
    int RecvLen = 0;
	unsigned char cCurrentRecviveBuf[5120]={0};
	unsigned char createparsepthread = 0;
	pthread_t     parsedatapthread;
	int           clientpipe[2];
	memset(clientpipe,0,sizeof(clientpipe));
	
	if (0 != pipe2(clientpipe,O_NONBLOCK))
	{
		#ifdef MAIN_DEBUG
		printf("Create pipe error,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		return;
	}

	if (0 == createparsepthread)
	{
		int ser = pthread_create(&parsedatapthread,NULL,(void *)ParsePipeData,&clientpipe[0]);
		if (0 != ser)
		{
			#ifdef MAIN_DEBUG
			printf("creatte server error,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			return ;
		}
		createparsepthread = 1;
	}
	

    /* keep_idle 后开始按 keep_interval 间隔探测，超出 keep_count 次后判定失联 */
    int keep_alive = 1;     // open keepalive
    int keep_idle = 5;      // after this time, begin detect alive
    int keep_interval = 1;  // alive detect frequency
    int keep_count = 5;     // alive detect numbers
    ret = setsockopt(pstClientSocketData->ClientSocketId, SOL_SOCKET, SO_KEEPALIVE, (void*) &keep_alive, sizeof(keep_alive));
    if (-1 == ret) {
        printf("setsockopt KEEPALIVE %s\n", strerror(errno));
    }
    ret = setsockopt(pstClientSocketData->ClientSocketId, SOL_TCP, TCP_KEEPIDLE, (void*) &keep_idle, sizeof(keep_idle));
    if (-1 == ret) {
        printf("setsockopt KEEPIDLE %s\n", strerror(errno));
    }
    ret = setsockopt(pstClientSocketData->ClientSocketId, SOL_TCP, TCP_KEEPINTVL, (void*) &keep_interval, sizeof(keep_interval));
    if (-1 == ret) {
        printf("setsockopt KEEPINTVL %s\n", strerror(errno));
    }
    ret = setsockopt(pstClientSocketData->ClientSocketId, SOL_TCP, TCP_KEEPCNT, (void*) &keep_count, sizeof(keep_count));
    if (-1 == ret) {
        printf("setsockopt KEEPCNT %s\n", strerror(errno));
    }
	while(1)
	{
		FD_ZERO(&fdRead);
		FD_SET(pstClientSocketData->ClientSocketId, &fdRead);
		aTime.tv_sec = 0;
		aTime.tv_usec = 5000000;
		ret = select(pstClientSocketData->ClientSocketId+1, &fdRead, NULL, NULL, &aTime);
		if( ret < 0 )
		{
			#ifdef MAIN_DEBUG
				printf("ParseRadioInfoOrDectorInfo select error,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			pstClientSocketData->Close_Flag = 1;
			if(1== createparsepthread)
			{
				pthread_cancel(parsedatapthread);
        		pthread_join(parsedatapthread,NULL);
			}
			sleep(10);
		}
		else
		{
			if(FD_ISSET(pstClientSocketData->ClientSocketId,&fdRead))
			{

				RecvLen = 0;
				memset(cCurrentRecviveBuf,0,sizeof(cCurrentRecviveBuf));
				RecvLen = recv(pstClientSocketData->ClientSocketId, cCurrentRecviveBuf, sizeof(cCurrentRecviveBuf),0);
				if(RecvLen > 5120)
				{
					#ifdef MAIN_DEBUG
					printf("receive client data error,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					continue;
				}
				if(RecvLen <=0)
				{
					#ifdef MAIN_DEBUG
					printf("receive client data error,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					pstClientSocketData->Close_Flag = 1;
					if(1== createparsepthread)
					{
						pthread_cancel(parsedatapthread);
		        		pthread_join(parsedatapthread,NULL);
					}
					sleep(10);
				}
				if(RecvLen == 0)
				{
					#ifdef MAIN_DEBUG
					printf("receive client data is 0,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					continue;
				}
				if( RecvLen>=0)
				{
					cCurrentRecviveBuf[RecvLen] = 0;
				}
				if (!wait_write_serial(clientpipe[1]))
				{
					if (write(clientpipe[1],cCurrentRecviveBuf,RecvLen) < 0)
					{
						#ifdef MAIN_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						pstClientSocketData->Close_Flag = 1;
						if(1== createparsepthread)
						{
							pthread_cancel(parsedatapthread);
			        		pthread_join(parsedatapthread,NULL);
						}
						sleep(10);
					}
				}
				else
				{
					#ifdef MAIN_DEBUG
					printf("pipe cannot be written,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					pstClientSocketData->Close_Flag = 1;
					if(1== createparsepthread)
					{
						pthread_cancel(parsedatapthread);
		        		pthread_join(parsedatapthread,NULL);
					}
					sleep(10);
				}

				
				continue;
			}
		}
	}
	
	pthread_exit(NULL);
}


void Create_New_Net_Server(void *arg)
{
	int                                 sin_size;
	int 								Client_Accept_Id;
	struct timeval						ptt;
	int 								enable = 1;
	unsigned char						neterr = 0;
	fd_set								netRead;
	unsigned char						err[10] = {0};
	unsigned char                       i = 0;


	
	gettimeofday(&ptt,NULL);
	memset(&my_addr,0,sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = INADDR_ANY;
	my_addr.sin_port = htons(SINGALER_MACHINE_SERVER_PORT_ONE);//listen port :12812
	bzero(&(my_addr.sin_zero),8);

	if ((iNetServerSocketId = socket(AF_INET,SOCK_STREAM,0)) < 0)
	{
		#ifdef MAIN_DEBUG
			printf("create new server socket error,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		update_event_list(&eventclass,&eventlog,1,18,ptt.tv_sec,&markbit);
        if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
        {
        #ifdef MAIN_DEBUG
        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
        }
		if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
        {//actively report is not probitted and connect successfully
            memset(err,0,sizeof(err));
            if (SUCCESS != err_report(err,ptt.tv_sec,1,18))
            {
            #ifdef MAIN_DEBUG
                printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
            #endif
            }
			else
        	{
            	write(csockfd,err,sizeof(err));
        	}
        }
		return;
	}
	
	setsockopt(iNetServerSocketId,SOL_SOCKET,SO_REUSEADDR,&enable,sizeof(int));
	if (bind(iNetServerSocketId,(struct sockaddr *)&my_addr,sizeof(struct sockaddr)) < 0)
	{
		#ifdef MAIN_DEBUG
			printf("bind addr error,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		update_event_list(&eventclass,&eventlog,1,18,ptt.tv_sec,&markbit);
        if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
        {
        #ifdef MAIN_DEBUG
        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
        }
		if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
        {//actively report is not probitted and connect successfully
            memset(err,0,sizeof(err));
            if (SUCCESS != err_report(err,ptt.tv_sec,1,18))
            {
            #ifdef MAIN_DEBUG
                printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
            #endif
            }
			else
        	{
            	write(csockfd,err,sizeof(err));
        	}
        }
		return;
	}
	listen(iNetServerSocketId,5);
	
	update_event_list(&eventclass,&eventlog,1,17,ptt.tv_sec,&markbit);
	if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
    {//actively report is not probitted and connect successfully
        memset(err,0,sizeof(err));
        if (SUCCESS != err_report(err,ptt.tv_sec,1,17))
        {
			#ifdef MAIN_DEBUG
		            printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
			#endif
        }
		else
        {
            write(csockfd,err,sizeof(err));
        }
    }

	while(1)
	{
		#ifdef MAIN_DEBUG
		printf("Accepting request of radio or other dector connect request,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		ptt.tv_sec = 8;
		ptt.tv_usec = 0;
		FD_ZERO(&netRead);
		FD_SET(iNetServerSocketId,&netRead);
		neterr = select(iNetServerSocketId+1,&netRead,NULL,NULL,&ptt);
		if (neterr < 0 )
		{
			close(iNetServerSocketId);
			break;
		}
		else
		{
			if (FD_ISSET(iNetServerSocketId,&netRead))
			{
				struct sockaddr_in remote_addr;
				sin_size = 0;
				Client_Accept_Id = 0;
				sin_size = sizeof(remote_addr);
				Client_Accept_Id = accept(iNetServerSocketId, (struct sockaddr *)&remote_addr, (socklen_t*)&sin_size);
				
				if(Client_Accept_Id > 0)
				{	
					for(i = 0;i < 10;i++)
					{
						if((i+1) == 10)
						{
							#ifdef MAIN_DEBUG
		           			printf("current connect client has arrive max,File: %s,Line: %d\n", __FILE__,__LINE__);
							#endif
							close(Client_Accept_Id);
							break;
						}
						int size = 1024*1024;
						setsockopt(Client_Accept_Id, SOL_SOCKET, SO_RCVBUF, (char*)&size, sizeof(size));
						if(pstClientSocketInfo[i].CreatePthreadYes == 0)
						{
							#ifdef MAIN_DEBUG
							printf("accept one client: %s,port: %d i:%d\n",inet_ntoa(remote_addr.sin_addr),ntohs(remote_addr.sin_port),i);
							#endif
							pstClientSocketInfo[i].ClientSocketId = Client_Accept_Id;
							pstClientSocketInfo[i].Close_Flag = 0;
							edgersockd = Client_Accept_Id;
							int createstaus = pthread_create(&(pstClientSocketInfo[i].ClientPthreadId), NULL,(void*)ParseRadioInfoOrDectorInfo,(void*)&pstClientSocketInfo[i]);
							if(0 != createstaus)
							{
//								#ifdef MAIN_DEBUG
			           			printf("create one RadioInfoOrDectorInfo pthread error,File: %s,Line: %d\n", __FILE__,__LINE__);
//								#endif
								memset(&pstClientSocketInfo[i],0,sizeof(pstClientSocketInfo[i]));
								continue;
							}
							else
							{
//								#ifdef MAIN_DEBUG
								printf("create one RadioInfoOrDectorInfo pthread success,File: %s,Line: %d\n", __FILE__,__LINE__);
//								#endif
								pstClientSocketInfo[i].CreatePthreadYes = 1;
								break;
							}
							
						}

					}
				}
				else
				{
					#ifdef MAIN_DEBUG
		            printf("create one client error ,File: %s,Line: %d\n", __FILE__,__LINE__);
					#endif
					continue;
				}
			}
		}
		for(i = 0;i <10;i++)
		{
			
			if(pstClientSocketInfo[i].Close_Flag == 1)
			{
				
				if(pstClientSocketInfo[i].CreatePthreadYes == 1)
				{	
//					#ifdef MAIN_DEBUG
					printf("cancle one client pthread The location is %d,File: %s,Line: %d\n",i, __FILE__,__LINE__);
//					#endif
					close(pstClientSocketInfo[i].ClientSocketId);
					pthread_cancel(pstClientSocketInfo[i].ClientPthreadId);
					pthread_join(pstClientSocketInfo[i].ClientPthreadId,NULL);
					memset(&pstClientSocketInfo[i],0,sizeof(pstClientSocketInfo[i]));
				}

			}
		}	
	}

	pthread_exit(NULL);
}


//build a server that configure tool can connect;
void setup_net_server(void *arg)
{

	//struct sockaddr_in					my_addr;
	struct sockaddr_in					remote_addr;
	int									sin_size = 0;
//	int									fd = 0;
	int									len = 0;
	unsigned char						revbuf[256] = {0};
	unsigned char           			*prevbuf = revbuf;
	unsigned char						newip = 0;
	struct timeval						ptt,tout;
	int 								enable = 1;
	unsigned char						neterr = 0;
	fd_set								netRead;
	struct timeval						netmt;
	unsigned char						err[10] = {0};
	FILE								*tffp = NULL;

	gettimeofday(&ptt,NULL);
	memset(&my_addr,0,sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = INADDR_ANY;
	my_addr.sin_port = htons(SINGALER_MACHINE_SERVER_PORT);//port:12810 
	bzero(&(my_addr.sin_zero),8);

	if ((netfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
	{
	#ifdef MAIN_DEBUG
		printf("create socket error,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		update_event_list(&eventclass,&eventlog,1,18,ptt.tv_sec,&markbit);
        if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
        {
        #ifdef MAIN_DEBUG
        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
        }
		if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
        {//actively report is not probitted and connect successfully
            memset(err,0,sizeof(err));
            if (SUCCESS != err_report(err,ptt.tv_sec,1,18))
            {
            #ifdef MAIN_DEBUG
                printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
            #endif
            }
			else
        	{
            	write(csockfd,err,sizeof(err));
        	}
        }
		return;
	}
	setsockopt(netfd,SOL_SOCKET,SO_REUSEADDR,&enable,sizeof(int));
	if (bind(netfd,(struct sockaddr *)&my_addr,sizeof(struct sockaddr)) < 0)
	{
	#ifdef MAIN_DEBUG
		printf("bind addr error,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		update_event_list(&eventclass,&eventlog,1,18,ptt.tv_sec,&markbit);
        if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
        {
        #ifdef MAIN_DEBUG
        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
        }
		if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
        {//actively report is not probitted and connect successfully
            memset(err,0,sizeof(err));
            if (SUCCESS != err_report(err,ptt.tv_sec,1,18))
            {
            #ifdef MAIN_DEBUG
                printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
            #endif
            }
			else
        	{
            	write(csockfd,err,sizeof(err));
        	}
        }
		return;
	}
	listen(netfd,5);

	update_event_list(&eventclass,&eventlog,1,17,ptt.tv_sec,&markbit);
	if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
    {//actively report is not probitted and connect successfully
        memset(err,0,sizeof(err));
        if (SUCCESS != err_report(err,ptt.tv_sec,1,17))
        {
        #ifdef MAIN_DEBUG
            printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
        #endif
        }
		else
        {
            write(csockfd,err,sizeof(err));
        }
    }
	newip = 0;

	while (1)
	{
		printf("Accepting request of client,File: %s,Line: %d\n",__FILE__,__LINE__);\
		//12810 资源不释放,benqiu 20241017
		if( ssockfd>0 )
		{
			close(ssockfd);
			ssockfd = -1;
		}
	/*
		int ttt = 0;
		for (ttt = 0; ttt < 60; ttt++)
		{
			printf("lip:%d,seryes: %d,cliyes: %d,Line: %d\n",lip,seryes,cliyes,__LINE__);
			sleep(2);
		}
	*/
		sin_size = sizeof(struct sockaddr_in);
		memset(&remote_addr,0,sizeof(remote_addr));
		markbit &= 0xfffd;//conf tool do not connect signal machine successfully;
		if ((ssockfd = accept(netfd,(struct sockaddr *)&remote_addr,&sin_size)) < 0)
		{
		#ifdef MAIN_DEBUG
			perror("************************accept client ");
		#endif
			close(ssockfd);
			close(netfd);
			if ((netfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
			{
			#ifdef MAIN_DEBUG
				printf("create socket error,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				gettimeofday(&ptt,NULL);
				update_event_list(&eventclass,&eventlog,1,18,ptt.tv_sec,&markbit);
        		if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
        		{
        		#ifdef MAIN_DEBUG
        			printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
        		#endif
        		}
				if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
        		{//actively report is not probitted and connect successfully
            		memset(err,0,sizeof(err));
            		if (SUCCESS != err_report(err,ptt.tv_sec,1,18))
            		{
            		#ifdef MAIN_DEBUG
                		printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
            		#endif
            		}
					else
        			{
            			write(csockfd,err,sizeof(err));
        			}
        		}
				return;
			}
			setsockopt(netfd,SOL_SOCKET,SO_REUSEADDR,&enable,sizeof(int));
			if (bind(netfd,(struct sockaddr *)&my_addr,sizeof(struct sockaddr)) < 0)
			{
			#ifdef MAIN_DEBUG
				printf("bind addr error,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				gettimeofday(&ptt,NULL);
				update_event_list(&eventclass,&eventlog,1,18,ptt.tv_sec,&markbit);
        		if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
        		{
        		#ifdef MAIN_DEBUG
        			printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
        		#endif
        		}
				if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
        		{//actively report is not probitted and connect successfully
            		memset(err,0,sizeof(err));
            		if (SUCCESS != err_report(err,ptt.tv_sec,1,18))
            		{
            		#ifdef MAIN_DEBUG
                		printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
            		#endif
            		}
					else
        			{
            			write(csockfd,err,sizeof(err));
        			}
        		}
				return;
			}
			listen(netfd,5);
			continue;
		}

		printf("accept client: %s,port: %d\n",inet_ntoa(remote_addr.sin_addr),ntohs(remote_addr.sin_port));
		tout.tv_sec = 0;
    	tout.tv_usec = 100000;
    	if (setsockopt(ssockfd,SOL_SOCKET,SO_SNDTIMEO,&tout,sizeof(struct timeval)) < 0)
    	{
    	#ifdef MAIN_DEBUG
        	printf("set timeout err,File: %s,Line: %d\n",__FILE__,__LINE__);
    	#endif
    	}
		
		memset(revbuf,0,sizeof(revbuf));
		if (read(ssockfd,revbuf,sizeof(revbuf)) < 0)
		{
		#ifdef MAIN_DEBUG
			printf("read data error,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			gettimeofday(&ptt,NULL);
			update_event_list(&eventclass,&eventlog,1,19,ptt.tv_sec,&markbit);
            if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
            {
            #ifdef MAIN_DEBUG
            	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
			if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
        	{//actively report is not probitted and connect successfully
            	memset(err,0,sizeof(err));
            	if (SUCCESS != err_report(err,ptt.tv_sec,1,19))
            	{
            	#ifdef MAIN_DEBUG
                	printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
            	#endif
            	}
				else
        		{
            		write(csockfd,err,sizeof(err));
        		}
        	}
			continue;
		}

		#ifdef MAIN_DEBUG
		printf("revbuf: %s,File:%s,Line:%d\n",revbuf,__FILE__,__LINE__);
		#endif
		if ((!strcmp("GetVerId",revbuf)) || (!strcmp("WLTCGetVerId",revbuf)))
		{
//			unsigned char			temp[] = "CYT0V1.0.320140621END";
			if (write(ssockfd,VersionId,sizeof(VersionId)) < 0)
			{
				gettimeofday(&ptt,NULL);
				update_event_list(&eventclass,&eventlog,1,20,ptt.tv_sec,&markbit);
                if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                {
                #ifdef MAIN_DEBUG
                	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
				if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
        		{//actively report is not probitted and connect successfully
            		memset(err,0,sizeof(err));
            		if (SUCCESS != err_report(err,ptt.tv_sec,1,20))
            		{
            		#ifdef MAIN_DEBUG
                		printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
            		#endif
            		}
					else
        			{
            			write(csockfd,err,sizeof(err));
        			}
        		}	
				continue;
			}
		}
		else
		{
			continue;
		}
		markbit |= 0x0002; //conf tool or wireless terminal connect successfully

		#ifdef EMBED_CONFIGURE_TOOL
		if (!strcmp("127.0.0.1",(char *)(inet_ntoa(remote_addr.sin_addr))))
		{
			pthread_t			lhpid;
			lhsockfd = ssockfd;
			int lhret = pthread_create(&lhpid,NULL,(void *)localhost_client,NULL);
			if (0 != lhret)
			{
				printf("create thread error,File: %s,Line: %d\n",__FILE__,__LINE__);
			}			

			continue;
		}
		#else
		if (!strcmp("127.0.0.1",(char *)(inet_ntoa(remote_addr.sin_addr))))
        {
            continue;
        }
		#endif

		while(1)
		{//1
			if ((1 == newip) || (1 == neterr))
            {
                newip = 0;
				neterr = 0;
                break;
            }
			FD_ZERO(&netRead);
			FD_SET(ssockfd,&netRead);
			netmt.tv_sec = 15;
			netmt.tv_usec = 0;
			int netret = select(ssockfd+1,&netRead,NULL,NULL,&netmt);
			if (netret < 0)
			{
			#ifdef MAIN_DEBUG
				printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				break;
			}
			if (0 == netret)
			{
				neterr = 1;

				if (markbit2 & 0x0010)
				{//configure tool is control machine;
					markbit2 &= 0xffef;
					unsigned char           wltbuf[13] = {0};
                    unsigned char           wtpd[32] = {0};
					//clean pipe
                    while (1)
                    {
                        memset(wtpd,0,sizeof(wtpd));
                        if (read(conpipe[0],wtpd,sizeof(wtpd)) <= 0)
                            break;
                    }
					strcpy(wltbuf,"WLTC1SOCKEND");
                    //end clean pipe            
                    if (!wait_write_serial(conpipe[1]))
                    {
                        if (write(conpipe[1],wltbuf,sizeof(wltbuf)) < 0)
                        {
                        #ifdef MAIN_DEBUG
                            printf("write pipe error,File:%s,Line:%d\n",__FILE__,__LINE__);
                        #endif
                        }
                    }
                    else
                    {
                    #ifdef MAIN_DEBUG
                        printf("can not write,File:%s,Line:%d\n",__FILE__,__LINE__);
                    #endif
                    }
				}//configure tool is control machine;

				continue;
			}//netret == 0
			if (netret > 0)
			{//netret > 0
				memset(revbuf,0,sizeof(revbuf));
				if (read(ssockfd,revbuf,sizeof(revbuf)) <= 0)
				{
					if (markbit2 & 0x0010)
					{//configure tool is control machine;
						markbit2 &= 0xffef;
						unsigned char           wltbuf[13] = {0};
						unsigned char           wtpd[32] = {0};
						//clean pipe
						while (1)
						{
							memset(wtpd,0,sizeof(wtpd));
							if (read(conpipe[0],wtpd,sizeof(wtpd)) <= 0)
								break;
						}
						strcpy(wltbuf,"WLTC1SOCKEND");
						//end clean pipe            
						if (!wait_write_serial(conpipe[1]))
						{
							if (write(conpipe[1],wltbuf,sizeof(wltbuf)) < 0)
							{
							#ifdef MAIN_DEBUG
								printf("write pipe error,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
						}
						else
						{
						#ifdef MAIN_DEBUG
							printf("can not write,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
					}//configure tool is control machine;
					break;
				}
				if (!strcmp(revbuf,"IAMALIVE"))
				{
				#ifdef MAIN_DEBUG
					printf("Client is alive,FIle: %s,LIne: %d\n",__FILE__,__LINE__);
				#endif
					continue;	
				}

				if (!strcmp(revbuf,"FastForward"))
				{
					unsigned char           tpd[32] = {0};
					//clean pipe
					while (1)
					{
						memset(tpd,0,sizeof(tpd));
						if (read(ffpipe[0],tpd,sizeof(tpd)) <= 0)
							break;
					}
					//end clean pipe
					if (!wait_write_serial(ffpipe[1]))
					{
						if (write(ffpipe[1],"fastforward",11) < 0)
						{
						#ifdef MAIN_DEBUG
							printf("write pipe error,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
					}
					else
					{
					#ifdef MAIN_DEBUG
						printf("can not write,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
					gettimeofday(&ptt,NULL);
					update_event_list(&eventclass,&eventlog,1,105,ptt.tv_sec,&markbit);
					if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
					{
					#ifdef MAIN_DEBUG
						printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
				}
			
				if (!strcmp(revbuf,"GetServerNet"))
				{//if (!strcmp(revbuf,"GetServerNet"))
					unsigned char			snbuf[64] = {0};
					unsigned char			spbuf[10] = {0};
					strcpy(snbuf,"CYTI,IP=");
					strcat(snbuf,houtaiip);
					strcat(snbuf,",PORT=");
					short_to_string(houtaiport,spbuf);
					strcat(snbuf,spbuf);
					strcat(snbuf,",END");
					write(ssockfd,snbuf,strlen(snbuf));
				
					if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
            		{//actively report is not probitted and connect successfully
						gettimeofday(&ptt,NULL);
                		memset(err,0,sizeof(err));
                		if (SUCCESS != err_report(err,ptt.tv_sec,21,13))
                		{
                		#ifdef MAIN_DEBUG
                    		printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                		#endif
                		}
                		else
                		{
                    		write(csockfd,err,sizeof(err));
                		}
            		}					
	
					continue;
				}//if (!strcmp(revbuf,"GetServerNet"))
	
				if (!strncmp("CYTJ",revbuf,4))
				{//if (!strncmp("CYTJ",revbuf,4))
					unsigned char	 	*prevbuf = NULL;
					unsigned char	 	head[5] = {0};
					unsigned char	 	end[4] = {0};
					unsigned char		i = 0;	
					unsigned char		*phe = NULL;
					unsigned char		serip[16] = {0};
					unsigned char		sport[10] = {0};
					unsigned char		dnum = 0;
			
					strncpy(head,revbuf,4);
					dnum = 0;
					prevbuf = revbuf;
					phe = end;
					for (i = 0; ;i++)
					{//for (i = 0; ;i++)
						if ('\0' == *prevbuf)
							break;
						if (',' == *prevbuf)
						{
							dnum += 1;
							prevbuf++;
							continue;
						}
						if (3 == dnum)
						{
							*phe = *prevbuf;
							phe++;	
						}
						prevbuf++;
					}//for (i = 0; ;i++)
					if (!strcmp("CYTJ",head) && !strcmp("END",end))
					{//if (!strcmp("CYTJ",head) && !strcmp("END",end))
						prevbuf = revbuf;
						phe = serip; 
						dnum = 0;
						for (i = 0; ;i++)
						{
							if ('\0' == *prevbuf)
								break;
							if ((1 == dnum) && (',' == *prevbuf))
								break;
							if ('=' == *prevbuf)
							{
								dnum += 1;
								prevbuf++;
								continue;
							}
							if (1 == dnum)
							{
								*phe = *prevbuf;
								prevbuf++;
								phe++;
								continue;
							}
							prevbuf++;
						}//for (i = 0; ;i++)
						memset(houtaiip,0,sizeof(houtaiip));
						strcpy(houtaiip,serip);

						prevbuf = revbuf;
						phe = sport;
						dnum = 0;
						for (i = 0; ;i++)
                        {
                            if ('\0' == *prevbuf)
                                break;
                            if ((2 == dnum) && (',' == *prevbuf))
                                break;
                            if ('=' == *prevbuf)
                            {   
                                dnum += 1;
                                prevbuf++;
                                continue;
                            }
                            if (2 == dnum)
                            {
                                *phe = *prevbuf;
                                prevbuf++;
                                phe++;
                                continue;
                            }
							prevbuf++;
                        }//for (i = 0; ;i++)
						houtaiport = 0;
						unsigned int		ihoutaip = 0;
						string_to_digit(sport,&ihoutaip);
						houtaiport = ihoutaip;
				//		string_to_digit(sport,&houtaiport);

						if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                        {//actively report is not probitted and connect successfully
                            gettimeofday(&ptt,NULL);
                            memset(err,0,sizeof(err));
                            if (SUCCESS != err_report(err,ptt.tv_sec,21,15))
                            {
                            #ifdef MAIN_DEBUG
                                printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                            #endif
                            }
                            else
                            {
                                write(csockfd,err,sizeof(err));
                            }
							update_event_list(&eventclass,&eventlog,1,74,ptt.tv_sec,&markbit);
                        }
						else
						{
							gettimeofday(&ptt,NULL);
                        	update_event_list(&eventclass,&eventlog,1,74,ptt.tv_sec,&markbit);
						}						

						FILE	*ipfp = fopen(IPPORT_FILE,"wb+");
						if (NULL != ipfp)
						{//if (NULL != ipfp)
							fputs(houtaiip,ipfp);
							fputs("\n",ipfp);
							fputs(sport,ipfp);
							fclose(ipfp);
						#ifdef MAIN_DEBUG
							printf("cliyes: %d,lip: %d,File: %s,LIne: %d\n",cliyes,lip,__FILE__,__LINE__);
						#endif
							if (1 == cliyes)
                           	{
                           		close(csockfd);
                           		pthread_cancel(clivid);
                           		pthread_join(clivid,NULL);
                           		cliyes = 0;
                           	}
                           	if (0 == cliyes)
                           	{
                            	int cli = pthread_create(&clivid,NULL,(void *)setup_client,NULL);
                            	if (0 != cli)
                            	{
                            	#ifdef MAIN_DEBUG
                               		printf("build client error,File: %s,Line: %d\n",__FILE__,__LINE__);
                            	#endif
                            	}
                            	cliyes = 1;
                            }
						#ifdef MAIN_DEBUG
                            printf("cliyes: %d,lip: %d,File: %s,LIne: %d\n",cliyes,lip,__FILE__,__LINE__);
                        #endif
						}//if (NULL != ipfp)
						write(ssockfd,"SETSERVEROK",11);
					}//if (!strcmp("CYTJ",head) && !strcmp("END",end))
					else
					{
						write(ssockfd,"SETSERVERER",11);
					}
					continue;
				}//if (!strncmp("CYTJ",revbuf,4))

				if (!strcmp(revbuf,"RebootMachine2"))
				{
					if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                    {//actively report is not probitted and connect successfully
                        gettimeofday(&ptt,NULL);
                        memset(err,0,sizeof(err));
                        if (SUCCESS != err_report(err,ptt.tv_sec,21,25))
                        {
                        #ifdef MAIN_DEBUG
                            printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                        #endif
                        }
                        else
                        {
                            write(csockfd,err,sizeof(err));
                        }
						update_event_list(&eventclass,&eventlog,1,75,ptt.tv_sec,&markbit);
                    }
					else
					{
						gettimeofday(&ptt,NULL);
                        update_event_list(&eventclass,&eventlog,1,75,ptt.tv_sec,&markbit);
					}

					if (1 == cliyes)
                    {
                        close(csockfd);
                        pthread_cancel(clivid);
                        pthread_join(clivid,NULL);
                        cliyes = 0;
                    }
					write(ssockfd,"RebootOK2",9);

					if (0 == access("./data/slnum.dat",F_OK))
					{
						system("rm -f ./data/slnum.dat");
					}
					if (markbit & 0x0040)
					{//file is updated or clock is changed
						update = 1;
					}//file is updated or clock is changed
					else
					{
						update = 0;
					}
					FILE	*sfp = NULL;
					sfp = fopen("./data/slnum.dat","wb+");
					if (NULL != sfp)
					{
						fwrite(&retslnum,sizeof(retslnum),1,sfp);
						fwrite(&update,sizeof(update),1,sfp);
					}
					fclose(sfp);

					system("reboot");

					continue;		
				}//reboot machine

				if (!strcmp(revbuf,"RebootMachine"))
				{
					if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                    {//actively report is not probitted and connect successfully
                        gettimeofday(&ptt,NULL);
                        memset(err,0,sizeof(err));
                        if (SUCCESS != err_report(err,ptt.tv_sec,21,25))
                        {
                        #ifdef MAIN_DEBUG
                            printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                        #endif
                        }
                        else
                        {
                            write(csockfd,err,sizeof(err));
                        }
						update_event_list(&eventclass,&eventlog,1,75,ptt.tv_sec,&markbit);
                    }
					else
					{
						gettimeofday(&ptt,NULL);
                        update_event_list(&eventclass,&eventlog,1,75,ptt.tv_sec,&markbit);
					}

					if (1 == cliyes)
                    {
                        close(csockfd);
                        pthread_cancel(clivid);
                        pthread_join(clivid,NULL);
                        cliyes = 0;
                    }
					unsigned char		rm[3] = {0xAC,0xAC,0xED};
					if (!wait_write_serial(serial[0]))
					{
						if (write(serial[0],rm,sizeof(rm)) < 0)
						{
						#ifdef MAIN_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							write(ssockfd,"RebootER",9);
							continue;	
						}
					}
					else
					{
					#ifdef MAIN_DEBUG
						printf("serial cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						write(ssockfd,"RebootER",9);
						continue;
					}
					write(ssockfd,"RebootOK",9);

					continue;		
				}//reboot machine
				if (!strncmp("WLTC1",revbuf,5))
				{//wireless terminal control
					if ((markbit2 & 0x0002) || (markbit2 & 0x0004) || (markbit2 & 0x0008))
					{//have key or wireless or software is controlling signaler machine;
						continue;
					}//have key or wireless or software is controlling signaler machine;
					else
					{//not have key or wireless or software is controlling signaler machine;
						unsigned char			wlbuf[13] = {0};
						unsigned char			tpd[32] = {0};

						strncpy(wlbuf,revbuf,12);
					#ifdef MAIN_DEBUG
						printf("***************wlbuf: %s,File: %s,Line: %d\n",wlbuf,__FILE__,__LINE__);
					#endif
						//clean pipe
						while (1)
						{
							memset(tpd,0,sizeof(tpd));
							if (read(conpipe[0],tpd,sizeof(tpd)) <= 0)
								break;
						}
						//end clean pipe			
						if (!wait_write_serial(conpipe[1]))
						{
							if (write(conpipe[1],wlbuf,sizeof(wlbuf)) < 0)
							{
							#ifdef MAIN_DEBUG
								printf("write pipe error,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
						}
						else
						{
						#ifdef MAIN_DEBUG
							printf("can not write,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
					}//not have key or wireless or software is controlling signaler machine;
					continue;
				}//wireless terminal control	
				if (!strncmp("CYT7",revbuf,4))
				{//set clock and date
				#ifdef MAIN_DEBUG
					printf("%s,File: %s,Line: %d\n",revbuf,__FILE__,__LINE__);
				#endif
					unsigned char			head[5] = {0};
					unsigned char			end[4] = {0};
					unsigned char			tdata[4] = {0};
					int						i = 0;
					struct timeval			systime;

					prevbuf = revbuf;
					while(1)
					{
						i += 1;
						if (i > 11)
						{
							break;
						}
						if (i <= 4)
						{
							head[i-1] = *prevbuf;
				//			printf("head[%d-1]: %c\n",i,head[i-1]);
							prevbuf++;
							continue;
						}
						if ((i >= 5) && (i <= 8))
						{
							tdata[i - 5] = *prevbuf;
				//			printf("tdata[%d-1]: 0x%02x\n",i,tdata[i-1]);
							prevbuf++;
							continue;
						}
						if ((i >= 9) && (i <= 11))
						{
							end[i - 9] = *prevbuf;
				//			printf("end[%d-1]: %c\n",i,end[i-1]);
							prevbuf++;
							continue;
						}	
					}
					if ((!strcmp("CYT7",head)) && (!strcmp("END",end)))
					{
						int 				ti7 = 0;
						int					tti7 = 0;
						struct timeval		nt7;
						
						nt7.tv_sec = 0;
                    	nt7.tv_usec = 0;
                    	gettimeofday(&nt7,NULL);

						tti7 = 0;
						tti7 |= tdata[0];
						ti7 |= tti7;
						tti7 = 0;
						tti7 |= tdata[1];
						tti7 <<= 8;
						ti7 |= tti7;
						tti7 = 0;
						tti7 |= tdata[2];
						tti7 <<= 16;
						ti7 |= tti7;
						tti7 = 0;
						tti7 |= tdata[3];
						tti7 <<= 24;
						ti7 |= tti7;
						memset(&systime,0,sizeof(systime));
						systime.tv_sec = ti7;
						systime.tv_usec= 0;
						if (-1 == settimeofday(&systime,NULL))
						{
						#ifdef MAIN_DEBUG
							printf("set time error,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							write(ssockfd,"TIMECFGER",9);
							continue;
						}
						if (-1 == system("hwclock -w"))
						{
						#ifdef MAIN_DEBUG
							printf("set time error,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							write(ssockfd,"TIMECFGER",9);
							continue;
						}
						if (-1 == system("sync"))
						{
						#ifdef MAIN_DEBUG
                        	printf("set time error,File: %s,Line: %d\n",__FILE__,__LINE__);
                    	#endif
                        	write(ssockfd,"TIMECFGER",9);
                        	continue;
						}
						write(ssockfd,"TIMECFGOK",9);
						#ifdef MAIN_DEBUG
						printf("*******************ti7: %d,nt7.tv_sec: %d,Line: %d\n",ti7,nt7.tv_sec,__LINE__);
						#endif
						if (nt7.tv_sec > ti7)
						{
							if ((nt7.tv_sec - ti7) > 20)
							{
							#ifdef MAIN_DEBUG
								printf("Time difference exceed 20 sec,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								markbit |= 0x0040;//clock data do have update
                				if (!wait_write_serial(endpipe[1]))
                				{
                    				if (write(endpipe[1],"UpdateFile",11) < 0)
                    				{
                    				#ifdef MAIN_DEBUG
                        				printf("Write endpipe err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    				#endif
                    				}
                				}
                				else
                				{
                				#ifdef MAIN_DEBUG
                    				printf("Can not write endpipe,File: %s,Line: %d\n",__FILE__,__LINE__);
                				#endif
                				}
							}//must reagain get time interval and restart pattern
						}
						else
						{
							if ((ti7 - nt7.tv_sec) > 20)
							{
							#ifdef MAIN_DEBUG
                            	printf("Time difference exceed 20 sec,File: %s,Line: %d\n",__FILE__,__LINE__);
                        	#endif
                            	markbit |= 0x0040;//clock data do have update
                            	if (!wait_write_serial(endpipe[1]))
                            	{
                                	if (write(endpipe[1],"UpdateFile",11) < 0)
                                	{
                                	#ifdef MAIN_DEBUG
                                    	printf("Write endpipe err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
                                	}
                            	}
                            	else
                            	{
                            	#ifdef MAIN_DEBUG
                                	printf("Can not write endpipe,File: %s,Line: %d\n",__FILE__,__LINE__);
                            	#endif
                            	}
							}//must reagain get time interval and restart pattern
						}
						if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                        {//actively report is not probitted and connect successfully
                            memset(err,0,sizeof(err));
                            if (SUCCESS != err_report(err,nt7.tv_sec,21,7))
                            {
                            #ifdef MAIN_DEBUG
                                printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                            #endif
                            }
                            else
                            {
                                write(csockfd,err,sizeof(err));
                            }
                        }
                        update_event_list(&eventclass,&eventlog,1,72,nt7.tv_sec,&markbit);
					}//if ((!strcmp("CYT7",head)) && (!strcmp("END",end)))
					else
					{
					#ifdef MAIN_DEBUG
						printf("time set error,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						write(ssockfd,"TIMECFGER",9);
						if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                        {//actively report is not probitted and connect successfully
                            gettimeofday(&ptt,NULL);
                            memset(err,0,sizeof(err));
                            if (SUCCESS != err_report(err,ptt.tv_sec,21,8))
                            {
                            #ifdef MAIN_DEBUG
                                printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                            #endif
                            }
                            else
                            {
                                write(csockfd,err,sizeof(err));
                            }
                        }
					}
					continue;
				}//CYT7
				if (!strncmp("CYT8",revbuf,4))
				{//CYT8, set IP address
				#ifdef MAIN_DEBUG
					printf("%s,File: %s,Line: %d\n",revbuf,__FILE__,__LINE__);
				#endif
					unsigned char				iph[5] = {0}; //head
					unsigned char				ipd[10] = {0};//dhcp
					unsigned char				ipgw[64] = {0};//gw
					unsigned char				ipgwn[20] = {0};
					unsigned char				ipaddr[64] = {0};//ip addr
					unsigned char				ipaddrn[20] = {0};
					unsigned char				ipmask[64] = {0};//mask
					unsigned char				ipmaskn[20] = {0};
					unsigned char               ipend[4] = {0};//end
					unsigned char				comma = 0;
					unsigned char				ipi = 0;
					unsigned char				wend = 0;
					unsigned char				setip[256] = {0};
					unsigned char				setgw[64] = {0};
					prevbuf = revbuf;
					while(1)
					{//0
						if (1 == wend)
						{
							break;
						}
						for (ipi = 0; ;ipi++)
						{//1
							if (*prevbuf == ',')
							{
								prevbuf++;
								comma += 1;
								break;
							}
							if (*prevbuf == '\0')
							{
								wend = 1;
								break;
							}
							if (0 == comma)
							{
								iph[ipi] = *prevbuf;
								prevbuf++;
								continue;
							}
							if (1 == comma)
							{
								ipd[ipi] = *prevbuf;
								prevbuf++;
								continue;
							}
							if (2 == comma)
							{
								ipgw[ipi] = *prevbuf;
								prevbuf++;
								continue;
							}
							if (3 == comma)
							{
								ipaddr[ipi] = *prevbuf;
								prevbuf++;
								continue;
							}
							if (4 == comma)
							{
								ipmask[ipi] = *prevbuf;
								prevbuf++;
								continue;
							}
							if (5 == comma)
							{
								ipend[ipi] = *prevbuf;
								prevbuf++;
								continue;
							}
						}//1
					}//0
					if ((!strcmp("CYT8",iph)) && (!strcmp("END",ipend)))
					{
						unsigned char				*ptemp1 = NULL;
						unsigned char				*ptemp2 = NULL;
						//extract ip addr from ipaddr;
						ptemp1 = ipaddr;
                    	ptemp2 = ipaddrn;
						while (1)
						{
							if ('\0' == *ptemp1)
								break;
							if ((('0' <= *ptemp1) && (*ptemp1 <= '9')) || ('.' == *ptemp1))
							{
								*ptemp2 = *ptemp1;
								ptemp2++;
							}
							ptemp1++;
						}
						//extract gw addr from ipgw
						ptemp1 = ipgw;
						ptemp2 = ipgwn;
						while (1)
						{
							if ('\0' == *ptemp1)
                            	break;
                        	if ((('0' <= *ptemp1) && (*ptemp1 <= '9')) || ('.' == *ptemp1))
                        	{
                            	*ptemp2 = *ptemp1;
                            	ptemp2++;
                        	}
                        	ptemp1++;
						}
						//extract mask from ipmask
						ptemp1 = ipmask;
						ptemp2 = ipmaskn;
						while (1)
						{
							if ('\0' == *ptemp1)
                            	break;
                        	if ((('0' <= *ptemp1) && (*ptemp1 <= '9')) || ('.' == *ptemp1))
                        	{ 
                            	*ptemp2 = *ptemp1;
                            	ptemp2++;
                        	}
                        	ptemp1++;
						}


						FILE 					*ipfp = NULL;
						unsigned char			msg[64] = {0};
						int						pos = 0;
						int						offset = 0;
						unsigned char			ipmark = 0;
						unsigned char			lm[20] = {0};//local_machine
						unsigned char			dhcp[16] = {0};//dhcp
						unsigned char			ns[16] = {0};//nfs server
						unsigned char			ip2[32] = {0};//ip address of nfs server
						unsigned char			mp[64] = {0};//mount path
						unsigned char			ue[16] = {0};//user exe
						unsigned char			name[64] = {0};//run name of process
						unsigned char			para[32] = {0};//parameter

 
						//Firstly,save info of userinfo.txt
						ipfp = fopen(IPFILE,"r");
						if (NULL == ipfp)
						{
						#ifdef MAIN_DEBUG
							printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							continue;
						}
						ipmark = 0;
						while(1)
						{
							memset(msg,0,sizeof(msg));
							if (NULL == fgets(msg,sizeof(msg),ipfp))
								break;
							if (SUCCESS == find_out_child_str(msg,"IPAddress",&pos))
							{
								if (1 == ipmark)
								{
									ipmark = 0;
									strcpy(ip2,msg);
									continue;
								}
								ipmark = 1;
								continue;
							}
			#if 0
							if (1 == ipmark)
							{
								if (SUCCESS == find_out_child_str(msg,"IPAddress",&pos))
								{
									ipmark = 0;
									strcpy(ip2,msg);
									continue;
								}
							}
			#endif
							if (SUCCESS == find_out_child_str(msg,"LOCAL_MACHINE",&pos))
							{
								strcpy(lm,msg);
								continue;
							}
							if (SUCCESS == find_out_child_str(msg,"DHCP",&pos))
                        	{
                            	strcpy(dhcp,msg);
								continue;
                        	}
							if (SUCCESS == find_out_child_str(msg,"NFS_SERVER",&pos))
                        	{
                            	strcpy(ns,msg);
								continue;
                        	}
							if (SUCCESS == find_out_child_str(msg,"Mountpath",&pos))
                        	{
                            	strcpy(mp,msg);
								continue;
                        	}
							if (SUCCESS == find_out_child_str(msg,"USER_EXE",&pos))
                        	{
                            	strcpy(ue,msg);
								continue;
                        	}
							if (SUCCESS == find_out_child_str(msg,"Name",&pos))
                        	{
                            	strcpy(name,msg);
								continue;
                        	}
							if (SUCCESS == find_out_child_str(msg,"Parameters",&pos))
                        	{
                            	strcpy(para,msg);
								continue;
                        	}
						}//while(NULL != fgets(msg,sizeof(msg),ipfp))
						fclose(ipfp);
						//Secondly,delete "userinfo.txt"
						if (-1 == system("rm -f /mnt/nandflash/userinfo.txt"))
						{
						#ifdef MAIN_DEBUG
							printf("Delete userinfo.txt error,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							continue;
						}
						//Threely,create a new "userinfo.txt" again;
						ipfp = fopen(IPFILE,"wb+");
						if (NULL == ipfp)
						{
						#ifdef MAIN_DEBUG
							printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							continue;
						}
						printf("ipaddr: %s,ip2: %s,File: %s,Line: %d\n",ipaddr,ip2,__FILE__,__LINE__);
						fputs(lm,ipfp);
						fputs(dhcp,ipfp);
						fputs(ipgw,ipfp);
						fputs("\n",ipfp);
						fputs(ipaddr,ipfp);
						fputs("\n",ipfp);
						fputs(ipmask,ipfp);
						fputs("\n",ipfp);
						fputs(ns,ipfp);
						fputs(ip2,ipfp);
						fputs(mp,ipfp);
						fputs(ue,ipfp);
						fputs(name,ipfp);
						fputs(para,ipfp);
						fclose(ipfp);	
						//Fourly,temply change ip address;

						if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                        {//actively report is not probitted and connect successfully
                            gettimeofday(&ptt,NULL);
                            memset(err,0,sizeof(err));
                            if (SUCCESS != err_report(err,ptt.tv_sec,21,11))
                            {
                            #ifdef MAIN_DEBUG
                                printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                            #endif
                            }
                            else
                            {
                                write(csockfd,err,sizeof(err));
                            }
							update_event_list(&eventclass,&eventlog,1,73,ptt.tv_sec,&markbit);
                        }
						else
						{
							gettimeofday(&ptt,NULL);
                        	update_event_list(&eventclass,&eventlog,1,73,ptt.tv_sec,&markbit);
						}

						unsigned char			nia[64] = {0};//new ip address
						unsigned char			ngw[64] = {0};//new Gateway
						sprintf(nia,"%s %s %s %s","ifconfig eth0",ipaddrn,"netmask",ipmaskn);
						sprintf(ngw,"%s %s","route add default gw",ipgwn);
						if (-1 == system(nia))
						{
						#ifdef MAIN_DEBUG
							printf("set ip address error,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							continue;
						}
						if (-1 == system(ngw))
						{
						#ifdef MAIN_DEBUG
							printf("set gateway error,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							continue;
						}
						memset(&my_addr,0,sizeof(my_addr));
						my_addr.sin_family = AF_INET;
						my_addr.sin_addr.s_addr = INADDR_ANY;
						my_addr.sin_port = htons(SINGALER_MACHINE_SERVER_PORT);//port:12810 
						bzero(&(my_addr.sin_zero),8);
						close(ssockfd);
						close(netfd);//close old fd;
						if ((netfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
						{
						#ifdef MAIN_DEBUG
							printf("create socket error,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							return;
						}
    					setsockopt(netfd,SOL_SOCKET,SO_REUSEADDR,&enable,sizeof(int));
						if (bind(netfd,(struct sockaddr *)&my_addr,sizeof(struct sockaddr)) < 0)
						{
							perror("Bind address ");
						#ifdef MAIN_DEBUG
							printf("bind addr error,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							return;
						}
						listen(netfd,5);					
						newip = 1;

						unsigned int			nslip = 0;
						get_last_ip_field(&nslip);
                        lip = nslip;

						
                        if (1 == cliyes)
                        {
	                        close(csockfd);
                            pthread_cancel(clivid);
                            pthread_join(clivid,NULL);
                            cliyes = 0;
                        }
                        if (0 == cliyes)
                        {
     	                   int cli = pthread_create(&clivid,NULL,(void *)setup_client,NULL);
                           if (0 != cli)
                           {
                           #ifdef MAIN_DEBUG
        	                   printf("build client error,File: %s,Line: %d\n",__FILE__,__LINE__);
                           #endif
                           }
                           cliyes = 1;
                       	}
						continue;
					}//if ((!strcmp("CYT8",iph)) && (!strcmp("END",ipend)))
				}//CYT8,set IP address
				if (!strcmp("GetNetAddress",revbuf))
				{//conf tool want to get net configure
				#ifdef MAIN_DEBUG
					printf("%s,File: %s,Line: %d\n",revbuf,__FILE__,__LINE__);
				#endif
					//get ip,netmask and gw from userinfo.txt
					unsigned char			ip[64] = {0};
					unsigned char			netmask[64] = {0};
					unsigned char			gw[64] = {0};
					FILE					*ipfp = NULL;
					unsigned char			msg[64] = {0};
					int						pos = 0;
					unsigned char			ipmark = 0;
					unsigned char			sendmsg[128] = {0};

					ipfp = fopen(IPFILE,"rb");
					if (NULL == ipfp)
					{
					#ifdef MAIN_DEBUG
						printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						continue;
					}
					while (1)
					{
						memset(msg,0,sizeof(msg));
						if (NULL == fgets(msg,sizeof(msg),ipfp))
							break;
						if (SUCCESS == find_out_child_str(msg,"DefaultGateway",&pos))
						{
							strcpy(gw,msg);
							continue;
						}
						if (0 == ipmark)
						{//have two "IPAddress" in userinfo.txt,we need first "IPAddress"; 
							if (SUCCESS == find_out_child_str(msg,"IPAddress",&pos))
                    		{
								strcpy(ip,msg);
								ipmark = 1;
								continue;
                    		}
						}
						if (SUCCESS == find_out_child_str(msg,"SubnetMask",&pos))
                    	{
							strcpy(netmask,msg);
							continue;
                    	}
					}//while (NULL != fgets(msg,sizeof(msg),ipfp))
					strcpy(sendmsg,"CYT8,DHCP=\"0\",");
					//the end of gw is space;
					strncat(sendmsg,gw,(strlen(gw)-1));
					strcat(sendmsg,",");
					//the end of ip is space
					strncat(sendmsg,ip,(strlen(ip)-1));
					strcat(sendmsg,",");
					//the end of netmask is space
					strncat(sendmsg,netmask,(strlen(netmask)-1));
					strcat(sendmsg,",END");

					if (write(ssockfd,sendmsg,strlen(sendmsg)) < 0)
					{
						gettimeofday(&ptt,NULL);
                		update_event_list(&eventclass,&eventlog,1,20,ptt.tv_sec,&markbit);
                    	if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                    	{
                    	#ifdef MAIN_DEBUG
                    		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    	#endif
                    	}
						if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
        				{//actively report is not probitted and connect successfully
            				memset(err,0,sizeof(err));
            				if (SUCCESS != err_report(err,ptt.tv_sec,21,10))
            				{
            				#ifdef MAIN_DEBUG
                				printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
            				#endif
            				}
							else
        					{
            					write(csockfd,err,sizeof(err));
        					}
        				}
					}
					else
					{
						if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                        {//actively report is not probitted and connect successfully
                            gettimeofday(&ptt,NULL);
                            memset(err,0,sizeof(err));
                            if (SUCCESS != err_report(err,ptt.tv_sec,21,9))
                            {
                            #ifdef MAIN_DEBUG
                                printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                            #endif
                            }
                            else
                            {
                                write(csockfd,err,sizeof(err));
                            }
                        }
					}

					fclose(ipfp);
					continue;	
				}//conf tool want get net configure
				if (!strcmp("GetTSCTime",revbuf))
				{//gethosttime
				#ifdef MAIN_DEBUG
					printf("%s,File: %s,Line: %d\n",revbuf,__FILE__,__LINE__);
				#endif
					struct timeval				systime;
					int							settime = 0;
					int							ti= 0;
					unsigned char				timebuf[11] = {0};

					memset(&systime,0,sizeof(systime));
					if (-1 == gettimeofday(&systime,NULL))
					{
					#ifdef MAIN_DEBUG
						printf("Get system time error,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						continue;
					}
					settime = systime.tv_sec;
					timebuf[0] = 'C';
					timebuf[1] = 'Y';
					timebuf[2] = 'T';
					timebuf[3] = '7';
					ti = settime;
					ti &= 0x000000ff;
					timebuf[4] |= ti; 
					ti = settime;
					ti &= 0x0000ff00;
					ti >>= 8;
					timebuf[5] |= ti;
					ti = settime;
					ti &= 0x00ff0000;
					ti >>= 16;
					timebuf[6] |= ti;
					ti = settime;
					ti &= 0xff000000;
					ti >>= 24;
					timebuf[7] |= ti;
					timebuf[8] = 'E';
					timebuf[9] = 'N';
					timebuf[10] = 'D';

					if (write(ssockfd,timebuf,sizeof(timebuf)) < 0)
					{
						gettimeofday(&ptt,NULL);
                		update_event_list(&eventclass,&eventlog,1,20,ptt.tv_sec,&markbit);
                    	if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                    	{
                    	#ifdef MAIN_DEBUG
                    		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    	#endif
                    	}
						if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
        				{//actively report is not probitted and connect successfully
            				memset(err,0,sizeof(err));
            				if (SUCCESS != err_report(err,ptt.tv_sec,21,6))
            				{
            				#ifdef MAIN_DEBUG
                				printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
            				#endif
            				}
							else
        					{
            					write(csockfd,err,sizeof(err));
        					}
        				}
					}
					else
					{
						if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                    	{//actively report is not probitted and connect successfully
                        	gettimeofday(&ptt,NULL);
                        	memset(err,0,sizeof(err));
                        	if (SUCCESS != err_report(err,ptt.tv_sec,21,5))
                        	{
                        	#ifdef MAIN_DEBUG
                            	printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                        	#endif
                        	}
                        	else
                        	{
                            	write(csockfd,err,sizeof(err));
                        	}
                    	}
					}

					continue;
				}//gethosttime
				if (!strcmp("SetConfigure",revbuf))
				{//2
				#ifdef MAIN_DEBUG
					printf("%s,File: %s,Line: %d\n",revbuf,__FILE__,__LINE__);
				#endif
					unsigned char			configys[9] = "CONFIGYS";
					unsigned char			revdata = 0;
					int						num = 0;
					int						i = 0;
					unsigned char			head[5] = {0};
					int						slen = 0;
					int						tslen = 0;
					unsigned char			end[4] = {0};
					unsigned char			sdata[SBYTE] = {0};

					if (write(ssockfd,configys,sizeof(configys)) < 0)
					{
						gettimeofday(&ptt,NULL);
                		update_event_list(&eventclass,&eventlog,1,20,ptt.tv_sec,&markbit);
                    	if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                    	{
                    	#ifdef MAIN_DEBUG
                    		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    	#endif
                    	}
						if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                        {//actively report is not probitted and connect successfully
                            memset(err,0,sizeof(err));
                            if (SUCCESS != err_report(err,ptt.tv_sec,1,20))
                            {
                            #ifdef MAIN_DEBUG
                                printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                            #endif
                            }
							else
        					{
            					write(csockfd,err,sizeof(err));
        					}
                        }
					}
#if 0
					if (-1 == system("rm -f ./data/cy_tsc.dat"))
					{
					#ifdef MAIN_DEBUG
						printf("delete file error,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					else
					{
					#ifdef MAIN_DEBUG
						printf("delete file successfully,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}	
					FILE					*nfp = NULL;
					nfp = fopen(CY_TSC,"wb+");
					if (NULL == nfp)
					{
					#ifdef MAIN_DEBUG
						printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						continue;
					}
#endif
					num = 0;
					while (1)
					{//3
						revdata = 0;
						num += 1;
						if (num > (SBYTE+11))
							break;
						read(ssockfd,&revdata,1);
						if (num <= 4)
						{
							head[num-1] = revdata;
							continue;
						}
						if ((num >= 5) && (num <= 8))
						{
							tslen = 0;
							tslen |= revdata;
							tslen <<= ((num-5)*8);
							slen |= tslen;
							continue;
						}
						if ((num >= 9) && (num <= (SBYTE+8)))
						{
							sdata[num-9] = revdata;
							continue;
						}
						if ((num >= (SBYTE+9)) && (num <= (SBYTE+11)))
						{
							end[num - SBYTE -9] = revdata;
							continue;
						}
					}//3
					if ((strcmp("CYT4",head)) || ((SBYTE+11) != slen) || (strcmp("END",end)))
					{
					#ifdef MAIN_DEBUG
						printf("Not our data,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						write(ssockfd,"CONFIGER",9);
						if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                    	{//actively report is not probitted and connect successfully
                        	gettimeofday(&ptt,NULL);
                        	memset(err,0,sizeof(err));
                        	if (SUCCESS != err_report(err,ptt.tv_sec,21,4))
                        	{
                        	#ifdef MAIN_DEBUG
                            	printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                        	#endif
                        	}
                        	else
                        	{
                            	write(csockfd,err,sizeof(err));
                        	}
                    	}
						continue;
					}
					#ifdef MAIN_DEBUG
					printf("This is our data,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					write(ssockfd,"CONFIGOK",9);

					if (-1 == system("rm -f ./data/cy_tsc.dat"))
					{
					#ifdef MAIN_DEBUG
						printf("delete file error,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					else
					{
					#ifdef MAIN_DEBUG
						printf("delete file successfully,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}	
					FILE					*nfp = NULL;
					nfp = fopen(CY_TSC,"wb+");
					if (NULL == nfp)
					{
					#ifdef MAIN_DEBUG
						printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						continue;
					}


					fwrite(sdata,sizeof(sdata),1,nfp);
					
					fseek(nfp,0,SEEK_SET);
					memset(&tscheader,0,sizeof(tscheader));
    				memset(&unit,0,sizeof(unit));
//					fread(&tscheader,sizeof(tscheader),1,nfp);
//    				fread(&unit,sizeof(unit),1,nfp);
					memset(&stscheader,0,sizeof(stscheader));
            		memset(&sunit,0,sizeof(sunit));
            		memset(&sschedule,0,sizeof(sschedule));
            		memset(&stimesection,0,sizeof(stimesection));
            		memset(&spattern,0,sizeof(spattern));
            		memset(&stimeconfig,0,sizeof(stimeconfig));
            		memset(&sphase,0,sizeof(sphase));
            		memset(&sphaseerror,0,sizeof(sphaseerror));
            		memset(&schannel,0,sizeof(schannel));
            		memset(&schannelhint,0,sizeof(schannelhint));
            		memset(&sdetector,0,sizeof(sdetector));
			
					fread(&stscheader,sizeof(stscheader),1,nfp);
					memcpy(&tscheader,&stscheader,sizeof(stscheader));
    				fread(&sunit,sizeof(sunit),1,nfp);
					memcpy(&unit,&sunit,sizeof(sunit));

    				fread(&sschedule,sizeof(sschedule),1,nfp);
    				fread(&stimesection,sizeof(stimesection),1,nfp);

					//added by sk on 20150609   
    				if (unit.RemoteFlag & 0x01)
    				{//phase offset is 2 bytes
        				fread(&spattern,sizeof(spattern),1,nfp);
    				}//phase offset is 2 bytes
    				else
    				{//phase offset is 1 byte
						memset(&pattern1,0,sizeof(pattern1));
        				fread(&pattern1,sizeof(pattern1),1,nfp);
        				pattern_switch(&spattern,&pattern1);
    				}//phase offset is 1 byte

//    				fread(&spattern,sizeof(spattern),1,nfp);
    				fread(&stimeconfig,sizeof(stimeconfig),1,nfp);
    				fread(&sphase,sizeof(sphase),1,nfp);
    				fread(&sphaseerror,sizeof(sphaseerror),1,nfp);
    				fread(&schannel,sizeof(schannel),1,nfp);
    				fread(&schannelhint,sizeof(schannelhint),1,nfp);
    				fread(&sdetector,sizeof(sdetector),1,nfp);

					fclose(nfp);
					unsigned char		backup[4] = {0};
					backup[0] = 0xA7;
					backup[1] = unit.sntime;
					backup[2] = unit.ewtime;
					backup[3] = 0xED; 
					#ifdef MAIN_DEBUG
					printf("unit.reflags: 0x%02x,File: %s,Line: %d\n",unit.RemoteFlag,__FILE__,__LINE__);
					#endif 
					//inform stm32 that green time of backup pattern
            		if (!wait_write_serial(serial[0]))
            		{
                		if (write(serial[0],backup,sizeof(backup)) < 0)
                		{
                		#ifdef MAIN_DEBUG
                    		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                		#endif
                		}
            		}
            		else
            		{
            		#ifdef MAIN_DEBUG
                		printf("serial cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
            		#endif
            		}
            		//end inform stm32;	

					markbit |= 0x0040;//configure data do have update
					if (!wait_write_serial(endpipe[1]))
					{
						if (write(endpipe[1],"UpdateFile",11) < 0)
						{
						#ifdef MAIN_DEBUG
							printf("Write endpipe err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
					}
					else
					{
					#ifdef MAIN_DEBUG
						printf("Can not write endpipe,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                    {//actively report is not probitted and connect successfully
                        gettimeofday(&ptt,NULL);
                        memset(err,0,sizeof(err));
                        if (SUCCESS != err_report(err,ptt.tv_sec,21,3))
                        {
                        #ifdef MAIN_DEBUG
                            printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                        #endif
                        }
                        else
                        {
                            write(csockfd,err,sizeof(err));
                        }
						update_event_list(&eventclass,&eventlog,1,71,ptt.tv_sec,&markbit);
                    }
					else
					{
						gettimeofday(&ptt,NULL);
                        update_event_list(&eventclass,&eventlog,1,71,ptt.tv_sec,&markbit);
					}

					continue;
				}//2,setconfigure
				if (!strncmp("ClearEventInfo",revbuf,14))
				{//clear event log
				#ifdef MAIN_DEBUG
					printf("****************%s,File: %s,Line: %d\n",revbuf,__FILE__,__LINE__);
				#endif
					unsigned char 	et = 0;
					unsigned char	fen = 0;
					
					memset(&eventlog,0,sizeof(EventLog_t));
					for (et = 1; et <= 20; et++)
					{
					#if 0
						for (fen = 0; fen < 255; fen++)
						{
							if (0 == (eventlog.EventLogList[(et-1)*255+fen].EventClassId))
								break;
							eventlog.EventLogList[(et-1)*255+fen].EventClassId = 0;
							eventlog.EventLogList[(et-1)*255+fen].EventLogId = 0;
							eventlog.EventLogList[(et-1)*255+fen].EventLogTime = 0;
							eventlog.EventLogList[(et-1)*255+fen].EventLogValue = 0;
						}
					#endif
						eventclass.EventClassList[et-1].EventClassRowNum = 0;
                        gettimeofday(&ptt,NULL);
                        eventclass.EventClassList[et-1].EventClassClearTime = ptt.tv_sec;
					}
				
					if (0 == access(EVENTDAT,F_OK))
					{
					#ifdef MAIN_DEBUG
						printf("delete old file:event.dat,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						system("rm -f ./data/event.dat");
					}
#if 0
					if (0 == access(EVENTLOG,F_OK))
                    {
                    #ifdef MAIN_DEBUG
                        printf("delete old file:event.txt,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                        system("rm -f ./data/event.dat");
                    }
#endif
					for (et = 0; et < MAX_CHANNEL_EX; et++)
    				{
        				lamperrtype.lamperrList[et].etype = 0;
    				}
					if (0 == access("./data/lamperr.dat",F_OK))
                    {
                    #ifdef MAIN_DEBUG
                        printf("delete old file:lamperr.dat,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                        system("rm -f ./data/lamperr.dat");
                    }

					for (et = 0; et < FIVEERRLAMPINFO; et++)
					{
						felinfo.fiveerrlampinfoList[et].mark = 0;
						felinfo.fiveerrlampinfoList[et].chanid = 0;
						felinfo.fiveerrlampinfoList[et].errtype = 0;
					}

					write(ssockfd,"ClearEventOK",13);

					if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                    {//actively report is not probitted and connect successfully
                        gettimeofday(&ptt,NULL);
                        memset(err,0,sizeof(err));
                        if (SUCCESS != err_report(err,ptt.tv_sec,21,19))
                        {
                        #ifdef MAIN_DEBUG
                            printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                        #endif
                        }
                        else
                        {
                            write(csockfd,err,sizeof(err));
                        }
					//	update_event_list(&eventclass,&eventlog,1,76,ptt.tv_sec,&markbit);
                    }
				#if 0
					else
					{
						gettimeofday(&ptt,NULL);
                        update_event_list(&eventclass,&eventlog,1,76,ptt.tv_sec,&markbit);
					}
				#endif

					continue;
				}//clear event log
				if (!strcmp("ClearDetectInfo",revbuf))
				{//clear detect data
				#ifdef MAIN_DEBUG
					printf("%s,File: %s,Line: %d\n",revbuf,__FILE__,__LINE__);
				#endif
					memset(&detectdata,0,sizeof(DetectorData_t));
					if (0 == access(FLOWLOG,F_OK))
    				{
    				#ifdef MAIN_DEBUG
        				printf("delete old file:flow.dat,File: %s,Line: %d\n",__FILE__,__LINE__);
    				#endif
						//detect FLOWLOG
        				system("rm -f ./data/flow.dat");
    				}
					write(ssockfd,"ClearOK",9);

					if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                    {//actively report is not probitted and connect successfully
                        gettimeofday(&ptt,NULL);
                        memset(err,0,sizeof(err));
                        if (SUCCESS != err_report(err,ptt.tv_sec,21,23))
                        {
                        #ifdef MAIN_DEBUG
                            printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                        #endif
                        }
                        else
                        {
                            write(csockfd,err,sizeof(err));
                        }
						update_event_list(&eventclass,&eventlog,1,77,ptt.tv_sec,&markbit);
                    }
					else
					{
						gettimeofday(&ptt,NULL);
                        update_event_list(&eventclass,&eventlog,1,77,ptt.tv_sec,&markbit);
					}					

					continue;
				}//clear detect data
				if (!strcmp("GetDetectInfo",revbuf))
				{
				#ifdef MAIN_DEBUG
                	printf("*****************%s,File: %s,Line: %d\n",revbuf,__FILE__,__LINE__);
            	#endif
					int 					len = 0;
					int 					total = 0;
					unsigned char			*buf = NULL;
					unsigned char			*sdata = NULL;
				
					tffp = fopen(FLOWLOG,"rb");	
					if (NULL == tffp)
					{
					#ifdef MAIN_DEBUG
						printf("flow file is not exist,File:%s,Line: %d\n",__FILE__,__LINE__);
					#endif
						write(ssockfd,"DETECTDATAER",13);
						continue;
					}
					fseek(tffp,0,SEEK_SET);
					fseek(tffp,0,SEEK_END);
					len = ftell(tffp);
					if (0 == len)
					{
					#ifdef MAIN_DEBUG
						printf("stream is null,File:%s,Line: %d\n",__FILE__,__LINE__);
					#endif
						write(ssockfd,"DETECTDATAER",13);
						continue;
					}
					#ifdef MAIN_DEBUG
					printf("len: %d,File: %s,Line: %d,*************************\n",len,__FILE__,__LINE__);
					#endif
					buf = (unsigned char *)malloc(len);
					if (NULL == buf)
					{
					#ifdef MAIN_DEBUG
						printf("Mem alloc err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						continue;
					}
					total = len + 11;
					sdata = (unsigned char *)malloc(total);
					if (NULL == sdata)
					{
					#ifdef MAIN_DEBUG
						printf("Mem alloc err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						if (NULL != buf)
						{
							free(buf);
							buf = NULL;
						}
						continue;
					}

					fseek(tffp,0,SEEK_SET);
					//Begin to read ./data/flow.dat
					memset(buf,0,len);
					fread(buf,len,1,tffp);
					memset(sdata,0,total);
                	mc_combine_str(sdata,"CYT9",total,buf,len,"END");
                	if (write(ssockfd,sdata,total) < 0)
					{
						gettimeofday(&ptt,NULL);
                		update_event_list(&eventclass,&eventlog,1,20,ptt.tv_sec,&markbit);
                    	if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                    	{
                    	#ifdef MAIN_DEBUG
                    		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    	#endif
                    	}
						if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                        {//actively report is not probitted and connect successfully
                            memset(err,0,sizeof(err));
                            if (SUCCESS != err_report(err,ptt.tv_sec,21,22))
                            {
                            #ifdef MAIN_DEBUG
                                printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                            #endif
                            }
							else
        					{
            					write(csockfd,err,sizeof(err));
        					}
                        }
					}
					else
					{
						if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                        {//actively report is not probitted and connect successfully
                            gettimeofday(&ptt,NULL);
                            memset(err,0,sizeof(err));
                            if (SUCCESS != err_report(err,ptt.tv_sec,21,21))
                            {
                            #ifdef MAIN_DEBUG
                                printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                            #endif
                            }
                            else
                            {
                                write(csockfd,err,sizeof(err));
                            }
                        }
					}

					if (NULL != buf)
					{
						free(buf);
						buf = NULL;
					}
					if (NULL != sdata)
					{
						free(sdata);
						sdata = NULL;
					}
					continue;
				}
				if (!strcmp("GetLampErrType",revbuf))
                {
#if 0
				#ifdef MAIN_DEBUG
					printf("%s **********File: %s,Line: %d\n",revbuf,__FILE__,__LINE__);
				#endif
					int 						len = 0;
					int 						total = 0;
					unsigned char				*buf = NULL;
					unsigned char				*sdata = NULL;

					len = sizeof(lamperrtype);
					total = len + 11;
					buf = (unsigned char *)malloc(len);
					if (NULL == buf)
					{
					#ifdef MAIN_DEBUG
						printf("memory alloc err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						write(ssockfd,"LAMPERR",8);
						continue;
					}
					sdata = (unsigned char *)malloc(total);
					if (NULL == sdata)
					{
					#ifdef MAIN_DEBUG
						printf("memory alloc err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						if (NULL != buf)
						{
							free(buf);
							buf = NULL;
						}
						write(ssockfd,"LAMPERR",8);
						continue;
					}
					memset(buf,0,len);
					memcpy(buf,&lamperrtype,sizeof(lamperrtype));
					
					memset(sdata,0,total);
                	mc_combine_str(sdata,"CYTH",total,buf,len,"END");
                	if (write(ssockfd,sdata,total) < 0)
					{
						gettimeofday(&ptt,NULL);
                		update_event_list(&eventclass,&eventlog,1,20,ptt.tv_sec,&markbit);
                    	if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                    	{
                    	#ifdef MAIN_DEBUG
                    		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    	#endif
                    	}
						if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                        {//actively report is not probitted and connect successfully
                            memset(err,0,sizeof(err));
                            if (SUCCESS != err_report(err,ptt.tv_sec,1,20))
                            {
                            #ifdef MAIN_DEBUG
                                printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                            #endif
                            }
							else
        					{
            					write(csockfd,err,sizeof(err));
        					}
                        }
					}
					if (NULL != buf)
					{
						free(buf);
						buf = NULL;
					}
					if (NULL != sdata)
					{
						free(sdata);
						sdata = NULL;
					}
		#endif					
					continue;
                }
				if (!strcmp("GetEventInfo",revbuf))
				{//4
					unsigned char					eventhead[] = "ChaoYuan-TSC-1.0.0";
					unsigned char					*buf = NULL;
					unsigned char					*sdata = NULL;
					int								len = 0;
					int								total = 0;

					len = strlen(eventhead)+sizeof(eventclass)+sizeof(eventlog);
					total = len + 11;
					buf = (unsigned char *)malloc(len);
					if (NULL == buf)
					{
					#ifdef MAIN_DEBUG
						printf("memory alloc err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						continue;
					}
					sdata = (unsigned char *)malloc(total);
					if (NULL == sdata)
					{
					#ifdef MAIN_DEBUG
						printf("memory alloc err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						if (NULL != buf)
						{
							free(buf);
							buf = NULL;
						}
						continue;
					}
					memset(buf,0,len);
					memcpy(buf,eventhead,strlen(eventhead));
					memcpy(buf+strlen(eventhead),&eventclass,sizeof(eventclass));
					memcpy(buf+strlen(eventhead)+sizeof(eventclass),&eventlog,sizeof(eventlog));

					memset(sdata,0,total);
                	mc_combine_str(sdata,"CYT6",total,buf,len,"END");
                	if (write(ssockfd,sdata,total) < 0)
					{
						gettimeofday(&ptt,NULL);
                		update_event_list(&eventclass,&eventlog,1,20,ptt.tv_sec,&markbit);
                    	if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                    	{
                    	#ifdef MAIN_DEBUG
                    		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    	#endif
                    	}
						if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                        {//actively report is not probitted and connect successfully
                            memset(err,0,sizeof(err));
                            if (SUCCESS != err_report(err,ptt.tv_sec,21,18))
                            {
                            #ifdef MAIN_DEBUG
                                printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                            #endif
                            }
							else
        					{
            					write(csockfd,err,sizeof(err));
        					}
                        }
					}
					else
					{
						if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                        {//actively report is not probitted and connect successfully
                            gettimeofday(&ptt,NULL);
                            memset(err,0,sizeof(err));
                            if (SUCCESS != err_report(err,ptt.tv_sec,21,17))
                            {
                            #ifdef MAIN_DEBUG
                                printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                            #endif
                            }
                            else
                            {
                                write(csockfd,err,sizeof(err));
                            }
                        }
					}
	
					if (NULL != buf)
					{
						free(buf);
						buf = NULL;
					}
					if (NULL != sdata)
					{
						free(sdata);
						sdata = NULL;
					}
					continue;
				}//4
				if (!strcmp("GetMapData",revbuf))
				{//GetMapData
				#ifdef MAIN_DEBUG
                	printf("%s,File: %s,Line: %d\n",revbuf,__FILE__,__LINE__);
            	#endif
					FILE					*fp = NULL;
					unsigned char			*buf = NULL;
					int						len = 0;
					int						total = 0;
					unsigned char			*sdata = NULL;

					fp = fopen(CY_BUS,"rb");
					if (NULL == fp)
					{
					#ifdef MAIN_DEBUG
						printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						continue;
					}
					fseek(fp,0,SEEK_END);
					len = ftell(fp);
					fseek(fp,0,SEEK_SET);
					#ifdef MAIN_DEBUG
					printf("****************len: %d,File: %s,Line: %d\n",len,__FILE__,__LINE__);
					#endif
					buf = (unsigned char *)malloc(len);
					if (NULL == buf)
					{
					#ifdef MAIN_DEBUG
						printf("malloc error,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						continue;
					}
					memset(buf,0,len);
					if (fread(buf,len,1,fp) != 1)
					{
					#ifdef MAIN_DEBUG
						printf("read file error,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						continue;
					}
					sdata = (unsigned char *)malloc(len+11);
					if (NULL == sdata)
					{
					#ifdef MAIN_DEBUG
						printf("malloc error,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						if (NULL != buf)
						{
							free(buf);
							buf = NULL;
						}
						continue;
					}
					total = len + 11;
					memset(sdata,0,len+11);
					mc_combine_str(sdata,"CYTK",total,buf,len,"END");
					write(ssockfd,sdata,total);
					fclose(fp);
					if (NULL!= buf)
					{
						free(buf);
						buf = NULL;
					}
					if (NULL != sdata)
					{
						free(sdata);
						sdata = NULL;
					}

					continue;
				}//GetMapData
				if (!strcmp("SetMapData",revbuf))
				{//SetMapData
				#ifdef MAIN_DEBUG
					printf("******************************%s,File: %s,Line: %d\n",revbuf,__FILE__,__LINE__);
				#endif
					unsigned char			configys[6] = "MAPYS";
					unsigned char			revdata = 0;
					int						num = 0;
					int						i = 0;
					unsigned char			head[5] = {0};
					int						slen = 0;
					int						tslen = 0;
					unsigned char			end[4] = {0};
					unsigned char			sdata[BBYTE] = {0};

					write(ssockfd,configys,sizeof(configys));
					num = 0;
					while (1)
					{//3
						revdata = 0;
						num += 1;
						if (num > (BBYTE+11))
							break;
						read(ssockfd,&revdata,1);
						if (num <= 4)
						{
							head[num-1] = revdata;
							continue;
						}
						if ((num >= 5) && (num <= 8))
						{
							tslen = 0;
							tslen |= revdata;
							tslen <<= ((num-5)*8);
							slen |= tslen;
							continue;
						}
						if ((num >= 9) && (num <= (BBYTE+8)))
						{
							sdata[num-9] = revdata;
							continue;
						}
						if ((num >= (BBYTE+9)) && (num <= (BBYTE+11)))
						{
							end[num - BBYTE -9] = revdata;
							continue;
						}
					}//3
					if ((strcmp("CYTK",head)) || ((BBYTE+11) != slen) || (strcmp("END",end)))
					{
					#ifdef MAIN_DEBUG
						printf("Not our data,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						write(ssockfd,"MAPER",6);
						continue;
					}
					#ifdef MAIN_DEBUG
					printf("This is our data,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					write(ssockfd,"MAPOK",6);

					if (-1 == system("rm -f ./data/cy_bus.dat"))
					{
					#ifdef MAIN_DEBUG
						printf("delete file error,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					else
					{
					#ifdef MAIN_DEBUG
						printf("delete file successfully,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}	
					FILE					*nfp = NULL;
					nfp = fopen(CY_BUS,"wb+");
					if (NULL == nfp)
					{
					#ifdef MAIN_DEBUG
						printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						continue;
					}


					fwrite(sdata,sizeof(sdata),1,nfp);
					
					fseek(nfp,0,SEEK_SET);
					memset(&buslist,0,sizeof(buslist));
					fread(&buslist,sizeof(buslist),1,nfp);

					fclose(nfp);

					continue;
				}//SetMapData
				if (!strcmp("GetConfigure",revbuf))
				{//3
				#ifdef MAIN_DEBUG
                	printf("%s,File: %s,Line: %d\n",revbuf,__FILE__,__LINE__);
            	#endif
					FILE					*fp = NULL;
					unsigned char			*buf = NULL;
					int						len = 0;
					int						total = 0;
					unsigned char			*sdata = NULL;

					fp = fopen(CY_TSC,"rb");
					if (NULL == fp)
					{
					#ifdef MAIN_DEBUG
						printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						continue;
					}
					fseek(fp,0,SEEK_END);
					len = ftell(fp);
					fseek(fp,0,SEEK_SET);
					#ifdef MAIN_DEBUG
					printf("****************len: %d,File: %s,Line: %d\n",len,__FILE__,__LINE__);
					#endif
					buf = (unsigned char *)malloc(len);
					if (NULL == buf)
					{
					#ifdef MAIN_DEBUG
						printf("malloc error,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						continue;
					}
					memset(buf,0,len);
					if (fread(buf,len,1,fp) != 1)
					{
					#ifdef MAIN_DEBUG
						printf("read file error,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						continue;
					}
					sdata = (unsigned char *)malloc(len+11);
					if (NULL == sdata)
					{
					#ifdef MAIN_DEBUG
						printf("malloc error,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						if (NULL != buf)
						{
							free(buf);
							buf = NULL;
						}
						continue;
					}
					total = len + 11;
					memset(sdata,0,len+11);
					mc_combine_str(sdata,"CYT4",total,buf,len,"END");
					if (write(ssockfd,sdata,total) < 0)
					{
						gettimeofday(&ptt,NULL);
                		update_event_list(&eventclass,&eventlog,1,20,ptt.tv_sec,&markbit);
                    	if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                    	{
                    	#ifdef MAIN_DEBUG
                    		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    	#endif
                    	}
						if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                        {//actively report is not probitted and connect successfully
                            memset(err,0,sizeof(err));
                            if (SUCCESS != err_report(err,ptt.tv_sec,21,2))
                            {
                            #ifdef MAIN_DEBUG
                                printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                            #endif
                            }
							else
        					{
            					write(csockfd,err,sizeof(err));
        					}
                        }
					}
					else
					{
						if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                        {//actively report is not probitted and connect successfully
							gettimeofday(&ptt,NULL);
                            memset(err,0,sizeof(err));
                            if (SUCCESS != err_report(err,ptt.tv_sec,21,1))
                            {
                            #ifdef MAIN_DEBUG
                                printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                            #endif
                            }
                            else
                            {
                                write(csockfd,err,sizeof(err));
                            }
                        }	
					}
			
					fclose(fp);
					if (NULL!= buf)
					{
						free(buf);
						buf = NULL;
					}
					if (NULL != sdata)
					{
						free(sdata);
						sdata = NULL;
					}

					continue;
				}//3,getconfig
				if (!strcmp("GetLampStatus",revbuf))
				{//4
				#ifdef MAIN_DEBUG
                	printf("%s,File: %s,Line: %d\n",revbuf,__FILE__,__LINE__);
            	#endif
					unsigned char				ls[30] = {0};
					if (SUCCESS != mc_lights_status(ls,contmode,retstageid,retphaseid,&chanstatus))
					{
					#ifdef MAIN_DEBUG
						printf("mc_lights_status err,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
						continue;
					}
					if (write(ssockfd,ls,sizeof(ls)) < 0)
					{
						gettimeofday(&ptt,NULL);
                		update_event_list(&eventclass,&eventlog,1,20,ptt.tv_sec,&markbit);
                    	if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                    	{
                    	#ifdef MAIN_DEBUG
                    		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    	#endif
                    	}
						if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                        {//actively report is not probitted and connect successfully
                            memset(err,0,sizeof(err));
                            if (SUCCESS != err_report(err,ptt.tv_sec,1,20))
                            {
                            #ifdef MAIN_DEBUG
                                printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                            #endif
                            }
							else
        					{
            					write(csockfd,err,sizeof(err));
        					}
                        }
					}
					continue;				
				}//4,GetLampStatus
				if (!strcmp("GetDriveBoardInfo",revbuf))
				{//get info of drive board
				#ifdef MAIN_DEBUG
                	printf("%s,File: %s,Line: %d\n",revbuf,__FILE__,__LINE__);
            	#endif
					unsigned char           *buf = NULL;
					unsigned char			*sdata = NULL;
					int						total = 0;
					int						len = 0;
	
					len = sizeof(dbstatus);
					total = len + 11;
					buf = (unsigned char *)malloc(len);
					if (NULL == buf)
					{
					#ifdef MAIN_DEBUG
						printf("Mem alloc err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						if (write(ssockfd,"DRIVEINFOER",12) < 0)
						{
						#ifdef MAIN_DEBUG
							printf("write err,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
							gettimeofday(&ptt,NULL);
							update_event_list(&eventclass,&eventlog,1,20,ptt.tv_sec,&markbit);
                        	if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                        	{
                        	#ifdef MAIN_DEBUG
                        		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                        	#endif
                        	}
							if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                        	{//actively report is not probitted and connect successfully
                            	memset(err,0,sizeof(err));
                            	if (SUCCESS != err_report(err,ptt.tv_sec,1,20))
                            	{
                            	#ifdef MAIN_DEBUG
                                	printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                            	#endif
                            	}
								else
        						{
            						write(csockfd,err,sizeof(err));
        						}
                        	}
						}
						continue;
					}
					sdata = (unsigned char *)malloc(total);
					if (NULL == sdata)
					{
					#ifdef MAIN_DEBUG
						printf("Mem alloc err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						if (NULL != buf)
						{
							free(buf);
							buf = NULL;
						}
						if (write(ssockfd,"DRIVEINFOER",12) < 0)
                    	{
						#ifdef MAIN_DEBUG
							printf("write err,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
                        	gettimeofday(&ptt,NULL);
                        	update_event_list(&eventclass,&eventlog,1,20,ptt.tv_sec,&markbit);
                        	if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                        	{
                        	#ifdef MAIN_DEBUG
                        		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                        	#endif
                        	}
							if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                        	{//actively report is not probitted and connect successfully
                            	memset(err,0,sizeof(err));
                            	if (SUCCESS != err_report(err,ptt.tv_sec,1,20))
                            	{
                            	#ifdef MAIN_DEBUG
                                	printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                            	#endif
                            	}
								else
        						{
            						write(csockfd,err,sizeof(err));
        						}
                        	}
                    	}
						continue;
					}
					memset(buf,0,len);
					memcpy(buf,&dbstatus,len);
					memset(sdata,0,total);
					mc_combine_str(sdata,"CYTC",total,buf,len,"END");																
					if (write(ssockfd,sdata,total) < 0)
					{
						gettimeofday(&ptt,NULL);
						update_event_list(&eventclass,&eventlog,1,20,ptt.tv_sec,&markbit);
                    	if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
                    	{
                    	#ifdef MAIN_DEBUG
                    		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    	#endif
                    	}
						if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
                        {//actively report is not probitted and connect successfully
                            memset(err,0,sizeof(err));
                            if (SUCCESS != err_report(err,ptt.tv_sec,1,20))
                            {
                            #ifdef MAIN_DEBUG
                                printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
                            #endif
                            }
							else
        					{
            					write(csockfd,err,sizeof(err));
        					}
                        }
					}									

					if (NULL != buf)
					{
						free(buf);
						buf = NULL;
					}
					if (sdata != buf)
					{
						free(sdata);
						sdata = NULL;
					}
					continue;
				}//get info of drive board
				if (!strcmp("BeginMonitor",revbuf))
				{
				#ifdef MAIN_DEBUG
                	printf("%s,File: %s,Line: %d\n",revbuf,__FILE__,__LINE__);
            	#endif
					markbit |= 0x0010;
					continue;
				}
				if (!strcmp("EndMonitor",revbuf))
				{
				#ifdef MAIN_DEBUG
                	printf("%s,File: %s,Line: %d\n",revbuf,__FILE__,__LINE__);
            	#endif
					markbit &= 0xffef;
					continue;
				}
				if (!strcmp("GetModel",revbuf))
				{
					#ifdef MAIN_DEBUG
					printf("Get Version and ID Code,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					if (0 == access(MACHINE_VER,F_OK))
					{
						#ifdef MAIN_DEBUG
						printf("read ver.data,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						FILE 					*pzfp = NULL;
						DevCode_st              stcitycode;
						unsigned char           i=0;
						unsigned char           buf[256]={0};
						memset(&stcitycode,0,sizeof(stcitycode));
						pzfp = fopen(MACHINE_VER,"rb");
						if (NULL == pzfp)
						{
							#ifdef MAIN_DEBUG
							printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							unsigned char			bufp[] = "CYTJXHJ-CW-GA-CYTSC1CYTSC1-1705-0008END";
							fclose(pzfp);
							write(ssockfd,bufp,strlen(bufp));
							continue;
						}
						if(fread(&stcitycode,sizeof(stcitycode),1,pzfp) != 1)
						{
							fclose(pzfp);
							write(ssockfd,"GetModelERR",strlen("GetModelERR"));
							continue;
						}
						fclose(pzfp);
						for (i=0;i<stcitycode.strlen;i++)
    					buf[i]=stcitycode.cydevcode[i]-i-5;
						printf("GetModel11******************************************cydevcode:%s  strlen:%d\n",stcitycode.cydevcode,stcitycode.strlen);
						write(ssockfd,buf,stcitycode.strlen);
					}
					else
					{
						unsigned char			bufp[] = "CYTJXHJ-CW-GA-CYTSC1CYTSC1-1705-0008END";
						write(ssockfd,bufp,strlen(bufp));
					}
					continue;
				}
				if (!strcmp("GetDevCode",revbuf))
				{
					#ifdef MAIN_DEBUG
					printf("Get dev Code,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					if (0 == access(DEV_CODE,F_OK))
					{
						#ifdef MAIN_DEBUG
						printf("read ver.dat,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						FILE 					*pzfp = NULL;
						DevCode_st              stdevcode;
						unsigned char           i=0;
						unsigned char           buf[256]={0};
						memset(&stdevcode,0,sizeof(stdevcode));
						pzfp = fopen(DEV_CODE,"rb");
						if (NULL == pzfp)
						{
							#ifdef MAIN_DEBUG
							printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							unsigned char			bufp[] = "GetDevCode20230110END";
							fclose(pzfp);
							write(ssockfd,bufp,strlen(bufp));
							continue;
						}
						if(fread(&stdevcode,sizeof(stdevcode),1,pzfp) != 1)
						{
							fclose(pzfp);
							write(ssockfd,"GetDevCodeERR",strlen("GetDevCodeERR"));
							continue;
						}
						fclose(pzfp);
						for (i=0;i<stdevcode.strlen;i++)
    					buf[i]=stdevcode.cydevcode[i]-i-5;
						printf("GetDevCode22******************************************cydevcode:%s  strlen:%d\n",stdevcode.cydevcode,stdevcode.strlen);
						write(ssockfd,buf,stdevcode.strlen);
					}
					else
					{
						unsigned char			bufp[] = "GetDevCode20230110END";
						write(ssockfd,bufp,strlen(bufp));
					}
					continue;
				}
				if (!strncmp("SetDevCode",revbuf,10))
				{
					#ifdef MAIN_DEBUG
					printf("set dev Code,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					FILE 					*pzfp = NULL;
					DevCode_st              stdevcode;
					unsigned  char          *devcode = NULL;
					unsigned char           buf[256] = "GetDevCode";
					unsigned char           i = 0;
					memset(&stdevcode,0,sizeof(stdevcode));

					devcode = revbuf;
					devcode = devcode+10;
					if(devcode == NULL)
					{
						write(ssockfd,"SetDevCodeERR",strlen("SetDevCodeERR"));
						continue;
					}
					if (-1 == system("rm -f ./data/dev.dat"))
					{
						#ifdef MAIN_DEBUG
						printf("delete file error,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
					}
					else
					{
						#ifdef MAIN_DEBUG
						printf("delete file successfully,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
					}
					pzfp = fopen(DEV_CODE,"wb+");
					if (NULL == pzfp)
					{
						#ifdef MAIN_DEBUG
						printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						write(ssockfd,"SetDevCodeERR",strlen("SetDevCodeERR"));
						fclose(pzfp);
						continue;
					}
					strcat(buf,devcode);
					for (i=0;i<strlen(buf);i++)
    				stdevcode.cydevcode[i]=buf[i]+i+5;
					//strcpy(stdevcode.cydevcode,buf);
					stdevcode.strlen = strlen(buf);
					fwrite(&stdevcode,sizeof(DevCode_st),1,pzfp);
					fclose(pzfp);
					write(ssockfd,"SetDevCodeOK",strlen("SetDevCodeOK"));
					continue;
				}
				if (!strncmp("SetCityCode",revbuf,11))
				{
					#ifdef MAIN_DEBUG
					printf("set dev Code,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					FILE 					*pzfp = NULL;
					DevCode_st              stdevcode;
					unsigned  char          *citycode = NULL;
					unsigned  char           buf[50] = "CYTJXHJ-CW-GA-CYTSC1CYTSC1-1705-";
					unsigned char            i =0;
					memset(&stdevcode,0,sizeof(stdevcode));
					
					citycode = revbuf;
					citycode = citycode+11;
					if(citycode == NULL)
					{
						write(ssockfd,"SetCityCodeERR",strlen("SetCityCodeERR"));
						continue;
					}
					if (-1 == system("rm -f ./data/ver.dat"))
					{
						#ifdef MAIN_DEBUG
						printf("delete file error,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
					}
					else
					{
						#ifdef MAIN_DEBUG
						printf("delete file successfully,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
					}
					pzfp = fopen(MACHINE_VER,"wb+");
					if (NULL == pzfp)
					{
						#ifdef MAIN_DEBUG
						printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						write(ssockfd,"SetCityCodeERR",strlen("SetCityCodeERR"));
						fclose(pzfp);
						continue;
					}
					
					strcat(buf,citycode);
					for (i=0;i<strlen(buf);i++)
    				stdevcode.cydevcode[i]=buf[i]+i+5;
					//strcpy(stdevcode.cydevcode,buf);
					stdevcode.strlen = strlen(buf);
					fwrite(&stdevcode,sizeof(DevCode_st),1,pzfp);
					fclose(pzfp);
					write(ssockfd,"SetCityCodeOK",strlen("SetCityCodeOK"));
					continue;
				}
				if (!strcmp("OpenFrontDoor",revbuf))
				{
					unsigned char cOpenDoor[5] = {0};
					cOpenDoor[0] = 0xA6;
					cOpenDoor[1] = 0x10;
					cOpenDoor[2] = 0x01;
					cOpenDoor[3] = 0x01;
					cOpenDoor[4] = 0xED;
					if (!wait_write_serial(serial[0]))
				    {
				        if (write(serial[0],cOpenDoor,sizeof(cOpenDoor)) < 0)
				        {
							#ifdef MAIN_DEBUG
				            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							write(ssockfd,"OPENFRONTERROR",strlen("OPENFRONTERROR"));
							continue;
				        }
				    }
				    else
				    {
						#ifdef MAIN_DEBUG
				        printf("serial cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						write(ssockfd,"OPENFRONTERROR ",strlen("OPENFRONTERROR"));
						continue;
				    }
					write(ssockfd,"OPENFRONTOK",strlen("OPENFRONTOK"));
					continue;
				}
				if (!strcmp("OpenBackDoor",revbuf))
				{
					unsigned char cOpenDoor[5] = {0};
					cOpenDoor[0] = 0xA6;
					cOpenDoor[1] = 0x10;
					cOpenDoor[2] = 0x02;
					cOpenDoor[3] = 0x01;
					cOpenDoor[4] = 0xED;
					if (!wait_write_serial(serial[0]))
				    {
				        if (write(serial[0],cOpenDoor,sizeof(cOpenDoor)) < 0)
				        {
							#ifdef MAIN_DEBUG
				            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							write(ssockfd,"OPENBACKERROR",strlen("OPENBACKERROR"));
							continue;
				        }
				    }
				    else
				    {
						#ifdef MAIN_DEBUG
				        printf("serial cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						write(ssockfd,"OPENBACKERROR",strlen("OPENBACKERROR"));
						continue;
				    }
					write(ssockfd,"OPENBACKOK",strlen("OPENBACKOK"));
					continue;
				}
			}//netret > 0
		}//1,2th while loop
	}//0,1th while loop
	pthread_exit(NULL);
}
#ifdef NTP_TIME
static int net_sntp_time_offset(void)
{
    return 0;
}

static int net_sntp_set_dev_systime(struct timeval *tv, void *tz)
{
    struct tm  sysTime;
	
	struct timeval			stCurrentTime,stBeforeTime;
    int timeDiff = DEFAULT_TIME_ZONE;

	stBeforeTime.tv_sec = 0;
	stBeforeTime.tv_usec = 0;
	gettimeofday(&stBeforeTime,NULL);
	
	//时区偏移
    tv->tv_sec +=  timeDiff;
	settimeofday(tv,NULL);
	
#ifdef MAIN_DEBUG
    printf("NTP settime:%d:%02d:%02d:%02d:%02d:%02d\n", (1900+sysTime.tm_year), (1+sysTime.tm_mon), 
        sysTime.tm_mday, sysTime.tm_hour, sysTime.tm_min, sysTime.tm_sec);
#endif

	if (-1 == system("hwclock -w"))
	{
#ifdef MAIN_DEBUG
		printf("set time error,File: %s,Line: %d\n",__FILE__,__LINE__);
#endif
		return -1;
	}

	stCurrentTime.tv_sec = 0;
	stCurrentTime.tv_usec = 0;
	gettimeofday(&stCurrentTime,NULL);

	if (stBeforeTime.tv_sec > stCurrentTime.tv_sec)
	{
		if ((stBeforeTime.tv_sec - stCurrentTime.tv_sec) > 20)
		{
			#ifdef MAIN_DEBUG
				printf("Time difference exceed 20 sec,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			markbit |= 0x0040;//clock data do have update
			if (!wait_write_serial(endpipe[1]))
			{
				if (write(endpipe[1],"UpdateFile",11) < 0)
				{
				#ifdef MAIN_DEBUG
					printf("Write endpipe err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
			}
			else
			{
			#ifdef MAIN_DEBUG
				printf("Can not write endpipe,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
		}//must reagain get time interval and restart pattern
	}
	else
	{
		if ((stCurrentTime.tv_sec - stBeforeTime.tv_sec) > 20)
		{
			#ifdef MAIN_DEBUG
		    	printf("Time difference exceed 20 sec,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			markbit |= 0x0040;//clock data do have update
			if (!wait_write_serial(endpipe[1]))
			{
		    	if (write(endpipe[1],"UpdateFile",11) < 0)
		    	{
		    	#ifdef MAIN_DEBUG
		        	printf("Write endpipe err,File: %s,Line: %d\n",__FILE__,__LINE__);
		    	#endif
		    	}
			}
			else
			{
			#ifdef MAIN_DEBUG
		    	printf("Can not write endpipe,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
		}//must reagain get time interval and restart pattern
	}
	update_event_list(&eventclass,&eventlog,1,72,stCurrentTime.tv_sec,&markbit);
	return 0;
}


static void mc_init_ntp()
{
	pthread_t			ntpid; //ntp update time
	
	FILE						*fp = NULL;
	int retVal = -1; 
	retVal = net_init_sntp_lib((void *)net_sntp_set_dev_systime,(void *)net_sntp_time_offset);
	if(retVal < 0)
	{
	
#ifdef MAIN_DEBUG
		printf("init SNTP lib failed File\n");
#endif
		return ;
	}
		

	if (0 == access(NTPPORT_FILE,F_OK))
	{
		unsigned char				ttii = 0;
		unsigned char				htp[12] = {0};

		fp = fopen(NTPPORT_FILE,"rb");
		if (NULL != fp)
		{
			fgets(ntpip,17,fp);
			fgets(htp,10,fp);
			fclose(fp);

			for (ttii = 0; ;ttii++) 
			{
				if (('\n' == ntpip[ttii]) || ('\0' == ntpip[ttii]))
				{
					ntpip[ttii] = '\0';
					break;
				}
			}
			for (ttii = 0; ;ttii++)
			{
				if (('\n' == htp[ttii]) || ('\0' == htp[ttii]))
				{
					htp[ttii] = '\0';
					break;
				}
			}
			int 			thtp = 0;
			string_to_digit(htp,&thtp);
			ntpport = thtp;
		}//if (NULL != fp)
	}//if (0 == access(IPPORT_FILE,F_OK))
	
	int ntpiderr = pthread_create(&ntpid,NULL,(void *)set_up_ntp_updae_time,NULL);
	if (0 != ntpiderr)
	{
	#ifdef MAIN_DEBUG
			printf("build set_up_ntp_updae_time error,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return;
	}
	return;	

}
#endif

int main(int argc, char *argv[])
{
	FILE						*fp = NULL;
	struct tm					*systime = NULL;
	unsigned char				eci = 0;
	unsigned char				eli = 0;
	unsigned char				erri = 0;
	//added on 20150407
	unsigned char				di = 0;
	unsigned char				dj = 0;
	unsigned char				tnum = 0;
	unsigned char				tdect = 0;
	//end add
	struct timeval				tt;
	sockfd_t					sockfd;
//	timedown_t					timedown;	
	fd_set						synRead;
	yfdata_t					yfdata;//data of yellow flash
	yfdata_t					ardata;//data of all red
	unsigned char				bbr[3] = {0xA3,0xA3,0xED};
	struct sigaction        	sa;
	unsigned char				sntime = 0;
	unsigned char				ewtime = 0;

	unsigned char				err[10] = {0};
	unsigned int				blip = 0;
	unsigned char				specfunc = 0;
	
	//initial static variable
	memset(&sinfo,0,sizeof(statusinfo_t));
	memset(houtaiip,0,sizeof(houtaiip));
	houtaiport = 0;
	lip = 0;
	seryes = 0;
	cliyes = 0;
	ssockfd = 0;
	edgersockd = 0;
	csockfd = 0;
	n_csockfd = 0;	
	lhsockfd = 0;
	markbit = 0;
	markbit2 = 0;
	retpattern = 0;
	retphaseid = 0;
	retslnum = 0;
	retstageid = 0;
	retcolor = 4;
	contmode = 0;
	auxfunc = 0;
	halfdownt = 0;
	ncontmode = 0;
	update = 0;
	wifiEnable = 0;
	rfidm = 0;
	ccontrol = 0;
	fcontrol = 0;
	trans = 0;
	roadinfo = 0;
	roadinforeceivetime = 0;
	
	v2xcyct = 0; //V2X
	v2xmark = 0; //V2X
	fanganID = 0; //one key to green wave

	//20220916
	iNetServerSocketId = 0;
	NetServerYes = 0;
	memset(pstClientSocketInfo,0,sizeof(pstClientSocketInfo));
	memset(&my_addr,0,sizeof(my_addr));

	memset(wlmark,0,sizeof(wlmark));
	memset(serial,0,sizeof(serial));
	memset(&detectdata,0,sizeof(detectdata));
	memset(&eventclass,0,sizeof(eventclass));
	memset(&eventlog,0,sizeof(eventlog));
	memset(&chanstatus,0,sizeof(chanstatus));
	memset(&tscdata,0,sizeof(tscdata));
	memset(&lamperrtype,0,sizeof(lamperrtype));

	memset(&tscheader,0,sizeof(tscheader));
	memset(&unit,0,sizeof(unit));
	memset(&schedule,0,sizeof(schedule));
	memset(&timesection,0,sizeof(timesection));
	memset(&pattern,0,sizeof(pattern));
	memset(&pattern1,0,sizeof(pattern1));//added by sk on 20150609
	memset(&timeconfig,0,sizeof(timeconfig));
	memset(&phase,0,sizeof(phase));
	memset(&phaseerror,0,sizeof(phaseerror));
	memset(&channel,0,sizeof(channel));
	memset(&channelhint,0,sizeof(channelhint));
	memset(&detector,0,sizeof(detector));

	memset(&stscheader,0,sizeof(stscheader));
    memset(&sunit,0,sizeof(sunit));
    memset(&sschedule,0,sizeof(sschedule));
    memset(&stimesection,0,sizeof(stimesection));
    memset(&spattern,0,sizeof(spattern));
    memset(&stimeconfig,0,sizeof(stimeconfig));
    memset(&sphase,0,sizeof(sphase));
    memset(&sphaseerror,0,sizeof(sphaseerror));
    memset(&schannel,0,sizeof(channel));
    memset(&schannelhint,0,sizeof(schannelhint));
    memset(&sdetector,0,sizeof(sdetector));

	memset(&dbstatus,0,sizeof(dbstatus));
	memset(&felinfo,0,sizeof(felinfo));

	memset(slg,0,sizeof(slg));//V2X

	sockfd.ssockfd = &ssockfd;
	sockfd.csockfd = &csockfd;
	sockfd.lhsockfd = &lhsockfd;
	sockfd.n_csockfd = &n_csockfd;
	sockfd.edgersockd = &edgersockd;
//	memset(&timedown,0,sizeof(timedown));
	memset(flowpipe,0,sizeof(flowpipe));
	memset(conpipe,0,sizeof(conpipe));
	memset(pendpipe,0,sizeof(pendpipe));
	memset(synpipe,0,sizeof(synpipe));
	memset(endpipe,0,sizeof(endpipe));
	//end initial static variable

    //shield SIGPIPE
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE,&sa,0);
    //end shield SIGPIPE


	//Initial event class  list
	for (eci = 0; eci < MAX_EVENTCLASS_LINE; eci++)
	{
		eventclass.EventClassList[eci].EventClassId = eci + 1;
	}
	for (erri = 0; erri < MAX_CHANNEL_EX; erri++)
	{
		lamperrtype.lamperrList[erri].chanid = erri + 1;
/*
		if (2 == lamperrtype.lamperrList[erri].chanid)
			lamperrtype.lamperrList[erri].etype = 2;
*/
	}
	
	if (0 != pipe2(flowpipe,O_NONBLOCK))
	{
	#ifdef MAIN_DEBUG
		printf("Create pipe error,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return FAIL;
	}
	if (0 != pipe2(conpipe,O_NONBLOCK))
	{
	#ifdef MAIN_DEBUG
		printf("Create pipe error,File:%s,Line:%d\n",__FILE__,__LINE__);
	#endif
		return FAIL;
	}
	if (0 != pipe2(synpipe,O_NONBLOCK))
	{
	#ifdef MAIN_DEBUG
		printf("Create pipe error,File:%s,Line:%d\n",__FILE__,__LINE__);
	#endif
		return FAIL;
	}
	if (0 != pipe2(pendpipe,O_NONBLOCK))
	{
	#ifdef MAIN_DEBUG
		printf("Create pipe error,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return FAIL;
	}

	if (0 != pipe2(endpipe,O_NONBLOCK))
    {
    #ifdef MAIN_DEBUG
        printf("Create pipe error,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
        return FAIL;
    }
	if (0 != pipe2(ffpipe,O_NONBLOCK))
    {
    #ifdef MAIN_DEBUG
        printf("Create pipe error,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
        return FAIL;
    }

	if (0 == access(EVENTDAT,F_OK))
	{
	#ifdef MAIN_DEBUG
		printf("read old dat event,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		fp = fopen(EVENTDAT,"rb");
		if (NULL == fp)
		{
		#ifdef MAIN_DEBUG
			printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
		memset(&eventclass,0,sizeof(eventclass));
    	memset(&eventlog,0,sizeof(eventlog));
		fread(&eventclass,sizeof(EventClass_t),1,fp);
		fread(&eventlog,sizeof(EventLog_t),1,fp);
		fclose(fp);	
	}

	if (0 == access("./data/lamperr.dat",F_OK))
	{
	#ifdef MAIN_DEBUG
        printf("read old dat event,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
		fp = fopen("./data/lamperr.dat","rb");
		if (NULL == fp)
        {
        #ifdef MAIN_DEBUG
            printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
            return FAIL;
        }
		memset(&lamperrtype,0,sizeof(lamperrtype));
		fread(&lamperrtype,sizeof(lamperrtype),1,fp);
		fclose(fp);
	}

	gettimeofday(&tt,NULL);
	update_event_list(&eventclass,&eventlog,1,1,tt.tv_sec,&markbit);

	if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
	{//actively report is not probitted and connect successfully
		memset(err,0,sizeof(err));
		if (SUCCESS != err_report(err,tt.tv_sec,1,1))
		{
		#ifdef MAIN_DEBUG
			printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}
		else
        {
            write(csockfd,err,sizeof(err));
        }
	}

	//CY_TSC------------"./data/cy_tsc.dat"
	fp = fopen(CY_TSC,"rb");
	if (NULL == fp)
	{
	#ifdef MAIN_DEBUG
		printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		update_event_list(&eventclass,&eventlog,1,9,tt.tv_sec,&markbit);
        if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
        {
        #ifdef MAIN_DEBUG
        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
        }
		if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
    	{//actively report is not probitted and connect successfully
        	memset(err,0,sizeof(err));
        	if (SUCCESS != err_report(err,tt.tv_sec,1,9))
        	{
        	#ifdef MAIN_DEBUG
            	printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
        	#endif
        	}
			else
        	{
            	write(csockfd,err,sizeof(err));
        	}
    	}
		return FAIL;
	}
	update_event_list(&eventclass,&eventlog,1,8,tt.tv_sec,&markbit);
	if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
    {//actively report is not probitted and connect successfully
        memset(err,0,sizeof(err));
        if (SUCCESS != err_report(err,tt.tv_sec,1,8))
        {
        #ifdef MAIN_DEBUG
            printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
        }
		else
        {
            write(csockfd,err,sizeof(err));
        }
    }

	fread(&tscheader,sizeof(tscheader),1,fp);
    memcpy(&stscheader,&tscheader,sizeof(tscheader));
    fread(&unit,sizeof(unit),1,fp);
    memcpy(&sunit,&unit,sizeof(unit));
    fread(&schedule,sizeof(schedule),1,fp);
    memcpy(&sschedule,&schedule,sizeof(schedule));
    fread(&timesection,sizeof(timesection),1,fp);
    memcpy(&stimesection,&timesection,sizeof(timesection));

	//added by sk on 20150609	
	if (unit.RemoteFlag & 0x01)
	{//phase offset is 2 bytes
    	fread(&pattern,sizeof(pattern),1,fp);
    	memcpy(&spattern,&pattern,sizeof(pattern));
	}//phase offset is 2 bytes
	else
	{//phase offset is 1 byte
		fread(&pattern1,sizeof(pattern1),1,fp);
		pattern_switch(&pattern,&pattern1);
		memcpy(&spattern,&pattern,sizeof(pattern));
	}//phase offset is 1 byte

    fread(&timeconfig,sizeof(timeconfig),1,fp);
    memcpy(&stimeconfig,&timeconfig,sizeof(timeconfig));
    fread(&phase,sizeof(phase),1,fp);
    memcpy(&sphase,&phase,sizeof(phase));
    fread(&phaseerror,sizeof(phaseerror),1,fp);
    memcpy(&sphaseerror,&phaseerror,sizeof(phaseerror));
    fread(&channel,sizeof(channel),1,fp);
    memcpy(&schannel,&channel,sizeof(channel));
    fread(&channelhint,sizeof(channelhint),1,fp);
    memcpy(&schannelhint,&channelhint,sizeof(channelhint));
    fread(&detector,sizeof(detector),1,fp);
    memcpy(&sdetector,&detector,sizeof(detector));
    fclose(fp);

	//added on 20150407
	memset(detepro,0,sizeof(detepro));
	tnum = 0;
	tdect = 0;
	dj = 0;
	for (di = 0; di < detector.FactDetectorNum; di++)
	{
		if (0 == detector.DetectorList[di].DetectorId)
			break;
		//lest same detector id is save into array;
		if (tdect == detector.DetectorList[di].DetectorId)
			continue;
		//lest same detector id is save into array;
		tdect = detector.DetectorList[di].DetectorId;
		detepro[dj].deteid = detector.DetectorList[di].DetectorId;
		tnum = detector.DetectorList[di].DetectorSpecFunc;
		tnum &= 0xfc;//get 2~7bit
        tnum >>= 2;
        tnum &= 0x3f;
        detepro[dj].intime = tnum * 60; //seconds;
		gettimeofday(&detepro[dj].stime,NULL);	//added by sk on 20150625	
		dj++;
	}
	//end add

/*
	#ifdef MAIN_DEBUG
	for (di = 0; di < 60; di++)
	{
		if (0 == detepro[di].deteid)
			break;
		printf("detepro[%d].deteid:%d,detepro[%d].intime:%d,detepro[%d].stime.tv_sec:%d\n", \
				di,detepro[di].deteid,di,detepro[di].intime,di,detepro[di].stime.tv_sec);
	}
	#endif
*/
	sntime = unit.sntime;
	if (sntime < 12)
		sntime = 12;
	ewtime = unit.ewtime;
	if (ewtime < 12)
		ewtime = 12;
	tscdata.tscheader = &tscheader;
	tscdata.unit = &unit;
	tscdata.schedule = &schedule;
	tscdata.timesection = &timesection;
	tscdata.pattern = &pattern;
	tscdata.timeconfig = &timeconfig;
	tscdata.phase = &phase;
	tscdata.phaseerror = &phaseerror;
	tscdata.channel = &channel;
	tscdata.channelhint = &channelhint;
	tscdata.detector = &detector;

	stscdata.tscheader = &stscheader;
    stscdata.unit = &sunit;
    stscdata.schedule = &sschedule;
    stscdata.timesection = &stimesection;
    stscdata.pattern = &spattern;
    stscdata.timeconfig = &stimeconfig;
    stscdata.phase = &sphase;
    stscdata.phaseerror = &sphaseerror;
    stscdata.channel = &schannel;
    stscdata.channelhint = &schannelhint;
    stscdata.detector = &sdetector;	

	if (SUCCESS != mc_open_serial_port(serial))
	{
	#ifdef MAIN_DEBUG
		printf("open serial port error,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		update_event_list(&eventclass,&eventlog,1,3,tt.tv_sec,&markbit);
        if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
        {
        #ifdef MAIN_DEBUG
        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
        }
		if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
    	{//actively report is not probitted and connect successfully
        	memset(err,0,sizeof(err));
        	if (SUCCESS != err_report(err,tt.tv_sec,1,3))
        	{
        	#ifdef MAIN_DEBUG
            	printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
        	#endif
        	}
			else
            {
                write(csockfd,err,sizeof(err));
            }
    	}
		return FAIL;
	}

	update_event_list(&eventclass,&eventlog,1,2,tt.tv_sec,&markbit);
	if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
    {//actively report is not probitted and connect successfully
        memset(err,0,sizeof(err));
        if (SUCCESS != err_report(err,tt.tv_sec,1,2))
        {
        #ifdef MAIN_DEBUG
            printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
        }
		else
        {
            write(csockfd,err,sizeof(err));
        }
    }

	if (SUCCESS != mc_set_serial_port_new(serial))
	{
	#ifdef MAIN_DEBUG
		printf("set serial port error,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		update_event_list(&eventclass,&eventlog,1,5,tt.tv_sec,&markbit);
        if (SUCCESS != generate_event_file(&eventclass,&eventlog,softevent,&markbit))
        {
        #ifdef MAIN_DEBUG
        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
        }
		if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
    	{//actively report is not probitted and connect successfully
        	memset(err,0,sizeof(err));
        	if (SUCCESS != err_report(err,tt.tv_sec,1,5))
        	{
        	#ifdef MAIN_DEBUG
            	printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
        	#endif
        	}
			else
            {
                write(csockfd,err,sizeof(err));
            }
    	}
		return FAIL;
	}
	update_event_list(&eventclass,&eventlog,1,4,tt.tv_sec,&markbit);
	if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
    {//actively report is not probitted and connect successfully
        memset(err,0,sizeof(err));
        if (SUCCESS != err_report(err,tt.tv_sec,1,4))
        {
        #ifdef MAIN_DEBUG
            printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
        }
		else
        {
            write(csockfd,err,sizeof(err));
        }
    }

	//watch dog,client to connect server and GPS;

	pthread_t			wdid; //watch dog thread id
	int wdp = pthread_create(&wdid,NULL,(void *)setup_wd_gps,NULL);
	if (0 != wdp)
	{
	#ifdef MAIN_DEBUG
		printf("build thread error,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return FAIL;
	}

	//thread that monitor three serial port
	pthread_t			serid;
	int se = pthread_create(&serid,NULL,(void *)monitor_serial_port,&sockfd);
	if (0 != se)
	{
	#ifdef MAIN_DEBUG
		printf("create thread error,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return FAIL;
	}

	//build a server that configure tool can connect;
	
	#ifdef SERVER_CREATE
	if (0 == NetServerYes)
	{
		int ser = pthread_create(&NetServerPid,NULL,(void *)Create_New_Net_Server,NULL);
		if (0 != ser)
		{
			#ifdef MAIN_DEBUG
				printf("build parse server error,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			return FAIL;
		}
		NetServerYes = 1;
	}
	#endif
	
	if (0 == seryes)
	{
		int ser = pthread_create(&servid,NULL,(void *)setup_net_server,NULL);
		if (0 != ser)
		{
		#ifdef MAIN_DEBUG
			printf("build server error,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
		seryes = 1;
	}	

	pthread_t		udpid;
	int udp = pthread_create(&udpid,NULL,(void *)setup_udp_server,NULL);
	if (0 != udp)
	{
	#ifdef MAIN_DEBUG
		printf("build udp server error,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
	}

	if (0 == access(IPPORT_FILE,F_OK))
	{
		unsigned char				ttii = 0;
		unsigned char				htp[12] = {0};

		fp = fopen(IPPORT_FILE,"rb");
		if (NULL != fp)
		{
			fgets(houtaiip,17,fp);
			fgets(htp,10,fp);
			fclose(fp);

			for (ttii = 0; ;ttii++)	
			{
				if (('\n' == houtaiip[ttii]) || ('\0' == houtaiip[ttii]))
				{
					houtaiip[ttii] = '\0';
					break;
				}
			}
			for (ttii = 0; ;ttii++)
            {
                if (('\n' == htp[ttii]) || ('\0' == htp[ttii]))
                {
					htp[ttii] = '\0';
                    break;
                }
            }
			int				thtp = 0;
			string_to_digit(htp,&thtp);
			houtaiport = thtp;
		}//if (NULL != fp)
	}//if (0 == access(IPPORT_FILE,F_OK))
	else
	{
		strcpy(houtaiip,"192.168.10.152");
		houtaiport = 7654;
	}
#ifdef NTPPORT_FILE
	mc_init_ntp();
#endif
	//added by sk on 20150609
	wifiEnable = 0;
	if (unit.RemoteFlag & 0x02)
	{//Enable wifi
		int cswret = pthread_create(&cswid,NULL,(void *)check_wireless_card,NULL);
		if (0 != cswret)
		{
		#ifdef MAIN_DEBUG
			printf("create thread error,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
		wifiEnable = 1;
	}//Enable wifi
	//end add

	#ifdef V2X_DEBUG
	pthread_t                n_clivid;
	int n_cli = pthread_create(&n_clivid,NULL,(void *)setup_baidu_v2x_client,NULL);
	if (0 != n_cli)
	{
	#ifdef MAIN_DEBUG
		printf("build new standard pthread error,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
	}
	pthread_t				gd_clivid;
	int gd_cli = pthread_create(&gd_clivid,NULL,(void *)setup_gd_v2x_client,NULL);
	if (0 != gd_cli)
	{
	#ifdef MAIN_DEBUG
		printf("build new standard pthread error,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
	}	
	#endif

	if (0 == cliyes)
	{
		int cli = pthread_create(&clivid,NULL,(void *)setup_client,NULL);
		if (0 != cli)
		{
		#ifdef MAIN_DEBUG
			printf("build client error,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
		cliyes = 1;
	}


	//create file ./data/flow.dat to store vehicle flow info;
	if (0 == access(FLOWLOG,F_OK))
	{
		fp = fopen(FLOWLOG,"rb");
        if (NULL == fp)
        {
        #ifdef MAIN_DEBUG
            printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
            return FAIL;
        }
		memset(&detectdata,0,sizeof(DetectorData_t));
        fread(&detectdata,sizeof(DetectorData_t),1,fp);
        fclose(fp);
	}
	//all channels of configure tool update all red
	unsigned char	sardata[8] = {'C','Y','T','F',0x00,'E','N','D'};
	unsigned char	ari = 0;
	if ((markbit & 0x0010) && (markbit & 0x0002))
	{
		if (write(ssockfd,sardata,sizeof(sardata)) < 0)
        {
        #ifdef MAIN_DEBUG
            printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
        #endif
        }	
	}	
	for (ari = 0; ari < MAX_CHANNEL_STATUS; ari++)
	{
		chanstatus.ChannelStatusList[ari].ChannelStatusReds = 0xff;
		chanstatus.ChannelStatusList[ari].ChannelStatusYellows = 0;
		chanstatus.ChannelStatusList[ari].ChannelStatusGreens = 0;	
	}

	//inform stm32 that arm is started successfully;
	unsigned char		sucrun[3] = {0xA1,0xA1,0xED};
	if (!wait_write_serial(serial[0]))
	{
		if (write(serial[0],sucrun,sizeof(sucrun)) < 0)
		{
		#ifdef MAIN_DEBUG
			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
	}
	else
	{
	#ifdef MAIN_DEBUG
		printf("serial cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return FAIL;
	}
	//end inform

	//inform stm32 that green time of backup pattern
    unsigned char       backup[4] = {0xA7,sntime,ewtime,0xED};
    if (!wait_write_serial(serial[0]))
    {
        if (write(serial[0],backup,sizeof(backup)) < 0)
        {
        #ifdef MAIN_DEBUG
            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
            return FAIL;
        }
    }
    else
    {
    #ifdef MAIN_DEBUG
        printf("serial cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
        return FAIL;
    }
    //end inform stm32;
#ifdef VEHICLE_ROAD_COORD
	//get bus map list
	fp = fopen(CY_BUS,"rb");
	if (NULL == fp)
	{
	#ifdef MAIN_DEBUG
		printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
	}
	else
	{
		memset(&buslist,0,sizeof(BusList_t));
		fread(&buslist,sizeof(BusList_t),1,fp);
		fclose(fp);
	}
	printf("RoadId: %d,FactBusPhaseNum: %d,LngInteg: %d,LngDeci: %d,LatInteg: %d,LatDeci: %d\n", \
			buslist.RoadId,buslist.FactBusPhaseNum,buslist.LngInteg,buslist.LngDeci, \
			buslist.LatInteg,buslist.LatDeci);
/*
	unsigned short			bnn = 0;
	for (bnn = 0; bnn < buslist.FactBusPhaseNum; bnn++)
	{
		printf("chanid: %d,validt: %d,validl: %d,udlink: %d,rev: %d,busn: %d\n", \
			buslist.busphaselist[bnn].chanid,buslist.busphaselist[bnn].validt, \
			buslist.busphaselist[bnn].validl,buslist.busphaselist[bnn].udlink, \
			buslist.busphaselist[bnn].rev,buslist.busphaselist[bnn].busn);
	}
*/	
#endif
	//get slnum of run
	if (0 == access("./data/slnum.dat",F_OK))
	{
		fp = fopen("./data/slnum.dat","rb");
		if (NULL == fp)
		{
		#ifdef MAIN_DEBUG
            printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
		}
		else
		{
			retslnum = 0;
			update = 0;
			fread(&retslnum,sizeof(retslnum),1,fp);
			fread(&update,sizeof(update),1,fp);
			fclose(fp);
			system("rm -f ./data/slnum.dat");
			markbit2 |= 0x0040;
		}
	}
	//end get slnum of run

	if (!(markbit2 & 0x0040))
	{
		retslnum = 0;
		//initial yellow flash and all red
		memset(&yfdata,0,sizeof(yfdata));
		yfdata.second = unit.StartUpFlash;
		yfdata.markbit = &markbit;
		yfdata.markbit2 = &markbit2;
		yfdata.serial = serial[0];
		yfdata.sockfd = &sockfd;
		yfdata.cs = &chanstatus;
#ifdef FLASH_DEBUG
		char szInfo[32] = {0};
		char szInfoT[64] = {0};
		snprintf(szInfo,sizeof(szInfo)-1,"%s",__FUNCTION__);
		snprintf(szInfoT,sizeof(szInfoT)-1,"%d #####MainStart#####",__LINE__);
		tsc_save_eventlog(szInfo,szInfoT);
#endif
		new_yellow_flash(&yfdata);
		yfdata.second = 0; //because time of yellow falsh is all 0 second;
	//	all_yellow_flash(serial[0],unit.StartUpFlash);
		memset(&ardata,0,sizeof(ardata));
		ardata.second = unit.StartUpRed;
		ardata.markbit = &markbit;
		ardata.markbit2 = &markbit2;
		ardata.serial = serial[0];
		ardata.sockfd = &sockfd;
		ardata.cs = &chanstatus;
		new_all_red(&ardata);
		ardata.second = 0; //because time of red lamp is all 0 second;
	//	all_red(serial[0],unit.StartUpRed);
	}
	else
	{
		//software reboot happen
		retslnum = 0;
		memset(&yfdata,0,sizeof(yfdata));
		yfdata.second = unit.StartUpFlash;
		yfdata.markbit = &markbit;
		yfdata.markbit2 = &markbit2;
		yfdata.serial = serial[0];
		yfdata.sockfd = &sockfd;
		yfdata.cs = &chanstatus;
#ifdef FLASH_DEBUG
		char szInfo[32] = {0};
		char szInfoT[64] = {0};
		snprintf(szInfo,sizeof(szInfo)-1,"%s",__FUNCTION__);
		snprintf(szInfoT,sizeof(szInfoT)-1,"%d ######MainStart###L",__LINE__);
		tsc_save_eventlog(szInfo,szInfoT);
#endif
		new_yellow_flash(&yfdata);
		yfdata.second = 0; //because time of yellow falsh is all 0 second;
		memset(&ardata,0,sizeof(ardata));
		ardata.second = unit.StartUpRed;
		ardata.markbit = &markbit;
		ardata.markbit2 = &markbit2;
		ardata.serial = serial[0];
		ardata.sockfd = &sockfd;
		ardata.cs = &chanstatus;
		new_all_red(&ardata);
		ardata.second = 0; //because time of red lamp is all 0 second;
		markbit2 &= 0xffbf;
#if 0
		if (1 == update)
		{//pattern is update or clock is changed
			//initial yellow flash and all red
			memset(&yfdata,0,sizeof(yfdata));
			yfdata.second = unit.StartUpFlash;
			yfdata.markbit = &markbit;
			yfdata.serial = serial[0];
			yfdata.sockfd = &sockfd;
			yfdata.cs = &chanstatus;
			new_yellow_flash(&yfdata);
			yfdata.second = 0; //because time of yellow falsh is all 0 second;
			memset(&ardata,0,sizeof(ardata));
			ardata.second = unit.StartUpRed;
			ardata.markbit = &markbit;
			ardata.serial = serial[0];
			ardata.sockfd = &sockfd;
			ardata.cs = &chanstatus;
			new_all_red(&ardata);
			ardata.second = 0; //because time of red lamp is all 0 second;
			markbit2 &= 0xffbf;
		}//pattern is updated
		else
		{
			memset(&yfdata,0,sizeof(yfdata));
			yfdata.markbit = &markbit;
			yfdata.serial = serial[0];
			yfdata.sockfd = &sockfd;
			yfdata.cs = &chanstatus;
			yfdata.second = 0;
			memset(&ardata,0,sizeof(ardata));
			ardata.markbit = &markbit;
			ardata.serial = serial[0];
			ardata.sockfd = &sockfd;
			ardata.cs = &chanstatus;
			ardata.second = 0;
			markbit2 &= 0xffbf;
		}
	#endif
		update = 0; //restore default value of 'update'
	}//software reboot happen

	//send info to get status of drive status
	if (!wait_write_serial(serial[0]))
	{
		if (write(serial[0],bbr,sizeof(bbr)) < 0)
		{
		#ifdef MAIN_DEBUG
			printf("write base board serial err,File: %s,Line: %d\n",__FILE__,__LINE__);
			output_log("Main control,write base board err");
		#endif
		}
	}
	else
	{
	#ifdef MAIN_DEBUG
		printf("base board serial cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
	}		

	//info side door serial software is starting;
	unsigned char               sdserial[3] = {0xCC,0xCC,0xED};
	if (!wait_write_serial(serial[4]))
    {
        if (write(serial[4],sdserial,sizeof(sdserial)) < 0)
        {
        #ifdef MAIN_DEBUG
            printf("write base board serial err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
        }
    }
    else
    {
    #ifdef MAIN_DEBUG
        printf("base board serial cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
    }
	//info side door serial software is starting;

	int						i = 0,j = 0,z = 0,s = 0,x = 0;
	unsigned char			tsid = 0;
	unsigned char			yfyes = 0;//yellow flash
	pthread_t				yfid;
	unsigned char			cpyes = 0; //current pattern
	pthread_t				cpid;
	struct timeval			nowt,lastt,endmt;
	fd_set					endnRead;
	int						mtime = 0;
	unsigned char			enddata[15] = {0};
	int						leat = 0;
	unsigned char			endnum = 0;
	unsigned char			updateyes = 0;
	unsigned char           patternId = 0;
    unsigned char           npatternId = 0;
    int iFlashCnt = 0;
	get_last_ip_field(&blip);
	lip = blip;

	#ifdef MAIN_DEBUG
	printf("*************************lip: %d,File: %s,Line: %d\n",lip,__FILE__,__LINE__);
	#endif
	unsigned char		vcset[3] = {0xEC,0xEC,0xED};
	if (!wait_write_serial(serial[1]))
    {
        if (write(serial[1],vcset,sizeof(vcset)) < 0)
        {
        #ifdef MAIN_DEBUG
            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
            return FAIL;
        }
    }
    else
    {
    #ifdef MAIN_DEBUG
        printf("serial cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
        return FAIL;
    }

	while (1)
	{// 1th while loop
	
#ifdef MAIN_DEBUG
	//printf("***********************************while1:%s,Line: %d\n",__FILE__,__LINE__);
#endif
		if (markbit & 0x0040)
		{//configure data have update or time difference exceed 20 secs
			if (1 == cpyes)
			{
				while(1)
				{//while loop
					FD_ZERO(&synRead);
					FD_SET(synpipe[0],&synRead);
					#ifdef MAIN_DEBUG
					printf("***********************************wait child module end info,File:%s,Line: %d\n",__FILE__,__LINE__);
					#endif
					int synret = select(synpipe[0]+1,&synRead,NULL,NULL,NULL);
					if (synret < 0)
					{
					#ifdef MAIN_DEBUG
						printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						return FAIL;
					}
					if (synret > 0)
					{
						unsigned char           syndata[4] = {0};
						read(synpipe[0],syndata,sizeof(syndata));
						if ((0xCC == syndata[0]) && (0xDD == syndata[1]) && (0xED == syndata[2]))
						{
							break;
						}
						else
						{
							continue;
						}
					}
				}//while loop
				pthread_cancel(cpid);
				pthread_join(cpid,NULL);
				cpyes = 0;
				tc_end_child_thread();
				dc_end_child_thread();
				ms_end_child_thread();
				madc_end_child_thread();
				pps_end_child_thread();
				sc_end_child_thread();
				mih_end_child_thread();
				bus_end_child_thread();
				sa_end_child_thread();
				yft_end_child_thread();
                gt_end_child_thread();
				soa_end_child_thread();
				soc_end_child_thread();
				sot_end_child_thread();
				cd_end_child_thread();
				rcc_end_child_thread();
				oc_end_child_thread();
				bsa_end_child_thread();
				crc_end_child_thread();
				Inter_Control_End_Child_Thread();
				markbit &= 0xfffe;
				markbit &= 0xfeff;
			}// if (1 == cpyes)
			#ifdef MAIN_DEBUG
			printf("Begin to reagin read conf data,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			markbit &= 0xffbf;
            fp = fopen(CY_TSC,"rb");
            if (NULL == fp)
            {
            #ifdef MAIN_DEBUG
                printf("open file err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
                return FAIL;
            }
			memset(&tscheader,0,sizeof(tscheader));
    		memset(&unit,0,sizeof(unit));
    		memset(&schedule,0,sizeof(schedule));
    		memset(&timesection,0,sizeof(timesection));
    		memset(&pattern,0,sizeof(pattern));
			memset(&pattern1,0,sizeof(pattern1));
    		memset(&timeconfig,0,sizeof(timeconfig));
    		memset(&phase,0,sizeof(phase));
    		memset(&phaseerror,0,sizeof(phaseerror));
    		memset(&channel,0,sizeof(channel));
    		memset(&channelhint,0,sizeof(channelhint));
    		memset(&detector,0,sizeof(detector));

			fread(&tscheader,sizeof(tscheader),1,fp);
    		fread(&unit,sizeof(unit),1,fp);
    		fread(&schedule,sizeof(schedule),1,fp);
    		fread(&timesection,sizeof(timesection),1,fp);

			//added by sk on 20150609   
    		if (unit.RemoteFlag & 0x01)
    		{//phase offset is 2 bytes
        		fread(&pattern,sizeof(pattern),1,fp);
    		}//phase offset is 2 bytes
    		else
    		{//phase offset is 1 byte
        		fread(&pattern1,sizeof(pattern1),1,fp);
        		pattern_switch(&pattern,&pattern1);
    		}//phase offset is 1 byte
//    		fread(&pattern,sizeof(pattern),1,fp);

    		fread(&timeconfig,sizeof(timeconfig),1,fp);
    		fread(&phase,sizeof(phase),1,fp);
    		fread(&phaseerror,sizeof(phaseerror),1,fp);
    		fread(&channel,sizeof(channel),1,fp);
    		fread(&channelhint,sizeof(channelhint),1,fp);
    		fread(&detector,sizeof(detector),1,fp);
    		fclose(fp);
			//added on 20150407
			memset(detepro,0,sizeof(detepro));
			tnum = 0;
			tdect = 0;
			dj = 0;
			for (di = 0; di < detector.FactDetectorNum; di++)
			{
				if (0 == detector.DetectorList[di].DetectorId)
					break;
				//lest same detector id is save into array;
				if (tdect == detector.DetectorList[di].DetectorId)
					continue;
				//lest same detector id is save into array;
				tdect = detector.DetectorList[di].DetectorId;
				detepro[dj].deteid = detector.DetectorList[di].DetectorId;
				
				tnum = detector.DetectorList[di].DetectorSpecFunc;
				tnum &= 0xfc;//get 2~7bit
				tnum >>= 2;
				tnum &= 0x3f;
				detepro[dj].intime = tnum * 60; //seconds;
				gettimeofday(&detepro[dj].stime,NULL);//added by sk on 20150625   
				dj++;
			}
			//end add
			//added by sk on 20150703
			if (unit.RemoteFlag & 0x02)
			{
				if (0 == wifiEnable)
				{
					int cswret = pthread_create(&cswid,NULL,(void *)check_wireless_card,NULL);
        			if (0 != cswret)
        			{
        			#ifdef MAIN_DEBUG
            			printf("create thread error,File: %s,Line: %d\n",__FILE__,__LINE__);
        			#endif
            			return FAIL;
        			}	
					wifiEnable = 1;
				}
			}
			else
			{
				if (1 == wifiEnable)
				{
					pthread_cancel(cswid);
					pthread_join(cswid,NULL);
					wifiEnable = 0;
				}
			}
			//end add
		}//configure data have update
	
		tsid = 0;
		systime = (struct tm *)get_system_time();
		#ifdef MAIN_DEBUG
		printf("month:%d,mday:%d,wday:%d,hour:%d,min:%d,sec:%d,File:%s,Line:%d\n",systime->tm_mon+1, \
				systime->tm_mday,systime->tm_wday+1,systime->tm_hour,systime->tm_min,systime->tm_sec,\
				__FILE__,__LINE__);
		#endif
		for (i = 0; i < schedule.FactScheduleNum; i++)
		{//0
			if (0 == schedule.ScheduleList[i].ScheduleId)
				break;
//			printf("Month:0x%04x,week:0x%02x,day:0x%08x\n",schedule.ScheduleList[i].ScheduleMonth, 
//					schedule.ScheduleList[i].ScheduleWeek,schedule.ScheduleList[i].ScheduleDay);
			if (schedule.ScheduleList[i].ScheduleMonth & (0x01 << (systime->tm_mon+1)))
			{//0~11 means January~December,but 1bit~12bit of ScheduleMonth means January~December;
				if (schedule.ScheduleList[i].ScheduleWeek & (0x01 << (systime->tm_wday+1)))
				{//0~6 means sunday~saturday,but 1bit~7bit of ScheduleWeek means sunday~saturday;
					if (schedule.ScheduleList[i].ScheduleDay & (0x01 << (systime->tm_mday)))
					{//1~31 means 1 day ~ 31 day,but 1bit~31bit of ScheduleDay means 1 day ~ 31 day;
						tsid = schedule.ScheduleList[i].TimeSectionId;
						break;
					}
					else
					{
					#ifdef MAIN_DEBUG
						printf("Day %d is not scheduleID,Line:%d\n",systime->tm_mday,__LINE__);
					#endif
						continue;
					}
				}
				else
				{
				#ifdef MAIN_DEBUG
					printf("week %d is not scheduleID,File:%s,Line:%d\n",systime->tm_wday,__FILE__,__LINE__);	
				#endif
					continue;
				}
			}
			else
			{
			#ifdef MAIN_DEBUG
				printf("Month %d is not scheduleID,File:%s,Line:%d\n",systime->tm_mon+1,__FILE__,__LINE__);
			#endif
				continue;
			}
		}//0
		if (0 == tsid)
		{
		#ifdef MAIN_DEBUG
			printf("Not have fit scheduleID,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			gettimeofday(&tt,NULL);
			update_event_list(&eventclass,&eventlog,1,10,tt.tv_sec,&markbit);
			if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
    		{//actively report is not probitted and connect successfully
        		memset(err,0,sizeof(err));
        		if (SUCCESS != err_report(err,tt.tv_sec,1,10))
        		{
        		#ifdef MAIN_DEBUG
            		printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
        		#endif
        		}
				else
                {
                    write(csockfd,err,sizeof(err));
                }
    		}
			//end current running pattern and its child thread, Note: smooth transition;
			if (1 == cpyes)
			{
				while(1)
				{//while loop
					FD_ZERO(&synRead);
					FD_SET(synpipe[0],&synRead);
					int synret = select(synpipe[0]+1,&synRead,NULL,NULL,NULL);
					if (synret < 0)
					{
					#ifdef MAIN_DEBUG
						printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						return FAIL;
					}
					if (synret > 0)
					{
						unsigned char           syndata[4] = {0};
						read(synpipe[0],syndata,sizeof(syndata));
						if ((0xCC == syndata[0]) && (0xDD == syndata[1]) && (0xED == syndata[2]))
						{
							break;
						}
						else
						{
							continue;
						}
					}
				}//while loop
				pthread_cancel(cpid);
				pthread_join(cpid,NULL);
				cpyes = 0;
				tc_end_child_thread();
				dc_end_child_thread();
				ms_end_child_thread();
				madc_end_child_thread();
				pps_end_child_thread();
				sc_end_child_thread();
				mih_end_child_thread();
				bus_end_child_thread();
				sa_end_child_thread();
				yft_end_child_thread();
                gt_end_child_thread();
				soa_end_child_thread();
                soc_end_child_thread();
				sot_end_child_thread();
				cd_end_child_thread();
				rcc_end_child_thread();
				oc_end_child_thread();
				bsa_end_child_thread();
				crc_end_child_thread();
				Inter_Control_End_Child_Thread();
				markbit &= 0xfffe;
				markbit &= 0xfeff;
			}// if (1 == cpyes)

			//all red
//    		new_all_red(&ardata);
//			all_red(serial[0],0);

			//yellow flash 
			if (0 == yfyes)
			{
#ifdef FLASH_DEBUG
				char szInfo[32] = {0};
				char szInfoT[64] = {0};
				snprintf(szInfo,sizeof(szInfo)-1,"%s",__FUNCTION__);
				snprintf(szInfoT,sizeof(szInfoT)-1,"%d",__LINE__);
				tsc_save_eventlog(szInfo,szInfoT);
#endif
				int yfret = pthread_create(&yfid,NULL,(void *)err_yellow_flash,&yfdata);
				if (0 != yfret)
				{
				#ifdef MAIN_DEBUG
					printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					return FAIL;		
				}	
				yfyes = 1;
				output_log("tsic is 0");
			}

			sleep(10);
			continue;
		}// if (0 == tsid)		

		unsigned char			eventnum = 0;
//		unsigned char           patternId = 0;
//		unsigned char           npatternId = 0;
		unsigned short          cyclet = 0;
		unsigned short          ncyclet = 0;
		unsigned short           poffset = 0;
		unsigned short           npoffset = 0;
		unsigned char           coordphase = 0;
		unsigned char			ncoordphase = 0;
		unsigned char           tcid = 0;
		unsigned char			ntcid = 0;
		unsigned char			coordtime = 0;
		unsigned char			ncoordtime = 0;

		unsigned char           tempid = 0;
		unsigned char			npidf = 0;
		unsigned char			pidf = 0;

		unsigned char			shour = 0;
		unsigned char			nshour = 0;
		unsigned char			smin = 0;
		unsigned char			nsmin = 0;
		unsigned char			chour = 0;
		unsigned char			cmin = 0;
		int						waittime = 0;
		fcdata_t				fcdata;
		long					cst = 0;//current time
		long					nst = 0;//start time of next pattern
		long					st = 0;//start time of current pattern
		struct timeval			tv;

		#ifdef WUXI_CHECK
		if (markbit2 & 0x0400)
		{//network degrade mode
			tsid = DGTSID;
		}//network degrade mode
		#endif

		for (i = 0; i < timesection.FactTimeSectionNum; i++)
		{
		
#ifdef MAIN_DEBUG
//		printf("***********************************i:%d fact %d tsid %d:%s,Line: %d\n",i,timesection.FactTimeSectionNum,tsid,__FILE__,__LINE__);
#endif
			if (tsid == timesection.TimeSectionList[i][0].TimeSectionId)
			{
				for (j = 0; j < timesection.FactEventNum; j++)
				{

					if ((0 != timesection.TimeSectionList[i][j].EventId) && \
                        (0 == timesection.TimeSectionList[i][j].PatternId))
					{
					#ifdef MAIN_DEBUG
						printf("Not fit pattern,FIle:%s,Line: %d\n",__FILE__,__LINE__);
					#endif
						//added by sk on 20150707
						if ((1 != timesection.TimeSectionList[i][j].ControlMode) && \
							(2 != timesection.TimeSectionList[i][j].ControlMode) && \
							(3 != timesection.TimeSectionList[i][j].ControlMode))
						//end add
						{
							eventnum = 0;
							break;
						}
					}
					if ((0 == timesection.TimeSectionList[i][j].EventId) && \
                        (0 != timesection.TimeSectionList[i][j].PatternId))
                    {
                    #ifdef MAIN_DEBUG
                        printf("Not fit EventId,FIle:%s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                        eventnum = 0;
                        break;
                    }
					if ((0 == timesection.TimeSectionList[i][j].EventId) && \
						(0 == timesection.TimeSectionList[i][j].PatternId))
					{
						break;
					}
					eventnum++;
				}
				if (0 == eventnum)
				{
					break;
				}
				//continue to check valid of invalid of pattern;
				for (j = 0; j < pattern.FactPatternNum; j++)
				{
#ifdef MAIN_DEBUG
//					printf("***********************************j:%d fact %d:%s,Line: %d\n",j,timesection.FactTimeSectionNum,__FILE__,__LINE__);
#endif
					if ((0 != pattern.PatternList[j].PatternId) && (0 == pattern.PatternList[j].TimeConfigId))
					{
					#ifdef MAIN_DEBUG
                        printf("Not fit TimeconfigId,FIle:%s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                       	eventnum = 0;
                       	break;
					}
					if ((0 == pattern.PatternList[j].PatternId) && (0 != pattern.PatternList[j].TimeConfigId))
                    {
                    #ifdef MAIN_DEBUG
                        printf("Not fit patternId,FIle:%s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                        eventnum = 0;
                        break;
                    }
					if ((0 == pattern.PatternList[j].PatternId) && (0 == pattern.PatternList[j].TimeConfigId))
					{
						#ifdef MAIN_DEBUG
                        printf("Not fit patternId and TimeConfigId,FIle:%s,Line: %d\n",__FILE__,__LINE__);
                    	#endif
						eventnum = 0;
						break;
					}
				}
				//end check valid of invalid of pattern;	
				if (0 == eventnum)
                {
                    break;
                }

				if (0 != fanganID)
				{
					eventnum = 1;
					#ifdef MAIN_DEBUG
					printf("***************************fanganID: %d,eventnum: %d,Line: %d\n",fanganID,eventnum,__LINE__);
					#endif
				}

				if (eventnum > 1)
				{
//				#ifdef MAIN_DEBUG
					printf("**************************************The pattern has multiple events,File: %s,Line: %d\n",__FILE__,__LINE__);
//				#endif
FLASH_LOOP:
					for (j = 0; j < timesection.FactEventNum; j++)
					{
						//end it and its children if cur running pattern exists,NOTE: smooth transition;
						if (1 == cpyes)
						{
							while(1)
							{//while loop
								FD_ZERO(&synRead);
								FD_SET(synpipe[0],&synRead);
				//				#ifdef MAIN_DEBUG
								printf("********************************wait child module end info,File:%s,Line: %d\n",__FILE__,__LINE__);
				//				#endif
								int synret = select(synpipe[0]+1,&synRead,NULL,NULL,NULL);
								if (synret < 0)
								{
								#ifdef MAIN_DEBUG
									printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									return FAIL;
								}
								if (synret > 0)
								{
									unsigned char           syndata[4] = {0};
									read(synpipe[0],syndata,sizeof(syndata));
									if ((0xCC == syndata[0]) && (0xDD == syndata[1]) && (0xED == syndata[2]))
									{
										break;
									}
									else
									{
										continue;
									}
								}
							}//while loop
							pthread_cancel(cpid);
							pthread_join(cpid,NULL);
							cpyes = 0;
							tc_end_child_thread();
							dc_end_child_thread();
							ms_end_child_thread();
							madc_end_child_thread();
							pps_end_child_thread();	
							sc_end_child_thread();
							mih_end_child_thread();
							bus_end_child_thread();
							sa_end_child_thread();
							yft_end_child_thread();
                        	gt_end_child_thread();
							soa_end_child_thread();
                			soc_end_child_thread();
							sot_end_child_thread();
                			cd_end_child_thread();
							rcc_end_child_thread();
							oc_end_child_thread();
							bsa_end_child_thread();
							crc_end_child_thread();
							Inter_Control_End_Child_Thread();
							markbit &= 0xfffe;
							markbit &= 0xfeff;
						}// if (1 == cpyes)

						if (0 == timesection.TimeSectionList[i][j].PatternId)
						{
                        	break;
						}

						cst = 0;
						st = 0;
						nst = 0;
						tv.tv_sec = 0;
						tv.tv_usec = 0;

						gettimeofday(&tv,NULL);
						cst = tv.tv_sec;

						//防止校时间隙出现问题
						//1 )  校时间隙       2）配置文件乱套
						systime = (struct tm *)get_system_time();
						shour = timesection.TimeSectionList[i][j].StartHour;
						smin = timesection.TimeSectionList[i][j].StartMinute;
						systime->tm_hour = (long)shour;
						systime->tm_min = (long)smin;
						systime->tm_sec = 0;
						st = (long)mktime(systime);
						//若因为临界区导致配置文件乱套，重新读一次

						if (cst < st)
						{
#ifdef FLASH_DEBUG
							char szInfo[32] = {0};
							char szInfoT[64] = {0};
							int iLastH = 0, iLastM = 0;
							snprintf(szInfo,sizeof(szInfo)-1,"Cst:%ld",cst);
							snprintf(szInfoT,sizeof(szInfoT)-1,"St:%ld",st);
							tsc_save_eventlog(szInfo,szInfoT);

							memset(szInfo,0,sizeof(szInfo));
							memset(szInfoT,0,sizeof(szInfoT));
							//记录上一次的值，看是否丢失导致
							if( j>0 )
							{
								 
								 iLastH = timesection.TimeSectionList[i][j-1].StartHour;
								 iLastM = timesection.TimeSectionList[i][j-1].StartMinute;
							}
							snprintf(szInfo,sizeof(szInfo)-1,"StartHour:%d iLastH %d",shour,iLastH);
							snprintf(szInfoT,sizeof(szInfoT)-1,"StartMinute:%d iLastM %d",smin,iLastM);
							tsc_save_eventlog(szInfo,szInfoT);
							
							memset(szInfo,0,sizeof(szInfo));
							memset(szInfoT,0,sizeof(szInfoT));
							snprintf(szInfo,sizeof(szInfo)-1,"i:%d j:%d",i,j);
							snprintf(szInfoT,sizeof(szInfoT)-1,"FactEventNum:%d",timesection.FactEventNum);
							tsc_save_eventlog(szInfo,szInfoT);
#endif
							sleep(2);
							cst = 0;
							st = 0;
							nst = 0;
							tv.tv_sec = 0;
							tv.tv_usec = 0;

							gettimeofday(&tv,NULL);
							cst = tv.tv_sec;

							systime = (struct tm *)get_system_time();
							shour = timesection.TimeSectionList[i][j].StartHour;
							smin = timesection.TimeSectionList[i][j].StartMinute;
							systime->tm_hour = (long)shour;
							systime->tm_min = (long)smin;
							systime->tm_sec = 0;
							st = (long)mktime(systime);	

						}


						contmode = timesection.TimeSectionList[i][j].ControlMode;
						auxfunc = timesection.TimeSectionList[i][j].AuxFunc;
						halfdownt = timesection.TimeSectionList[i][j].SpecFunc & 0x0f;
						specfunc = timesection.TimeSectionList[i][j].SpecFunc;
						patternId = timesection.TimeSectionList[i][j].PatternId;
						retpattern = patternId;

						coordtime = 0;
						ncoordtime = 0;
						poffset = 0;
						npoffset = 0; 
						cyclet = 0;
						ncyclet = 0;
						coordphase = 0;
						ncoordphase = 0;
						tcid = 0;
						ntcid = 0;
						tempid = 0;
						npidf = 0;
						pidf = 0;
						waittime = 0;
						if ((j + 1) >= eventnum)
						{
							nshour = 0;
							nsmin = 0;
							nst = 0;
							ncontmode = 0;
							npatternId = 0;
						}
						else
						{
							nshour = timesection.TimeSectionList[i][j+1].StartHour;
							nsmin = timesection.TimeSectionList[i][j+1].StartMinute;
							systime->tm_hour = (long)nshour;
							systime->tm_min = (long)nsmin;
							systime->tm_sec = 0;
							nst = (long)mktime(systime);
							ncontmode = timesection.TimeSectionList[i][j+1].ControlMode;
							npatternId = timesection.TimeSectionList[i][j+1].PatternId;
						}

						pidf = 0;
						npidf = 0;
						for (z = 0; z < pattern.FactPatternNum; z++)
						{//for (z = 0; z < pattern.FactPatternNum; z++)
							if (0 == pattern.PatternList[z].PatternId)
								break;
							if (patternId == pattern.PatternList[z].PatternId)
                            {
                                poffset = pattern.PatternList[z].PhaseOffset;
                                coordphase = pattern.PatternList[z].CoordPhase;
                                cyclet = pattern.PatternList[z].CycleTime;
								v2xcyct = cyclet; //V2X 
                                tcid = pattern.PatternList[z].TimeConfigId;
								pidf = 1;
                            }
							if (npatternId == pattern.PatternList[z].PatternId)
							{
								npoffset = pattern.PatternList[z].PhaseOffset;
								ncoordphase = pattern.PatternList[z].CoordPhase;
								ncyclet = pattern.PatternList[z].CycleTime;
								ntcid = pattern.PatternList[z].TimeConfigId;
								npidf = 1;
							}
							if (0 == npatternId)
							{
								npidf = 1;
							}
							if ((1 == pidf) && (1 == npidf))
								break;
						}//for (z = 0; z < pattern.FactPatternNum; z++)
						if ((0 != coordphase) && (4 == contmode))
						{
							for (z = 0; z < timeconfig.FactTimeConfigNum; z++)
                            {
                                if (0 == timeconfig.TimeConfigList[z][0].TimeConfigId)
                                    break;
                                if (tcid == timeconfig.TimeConfigList[z][0].TimeConfigId)
                                {
                                    for (s = 0; s < timeconfig.FactStageNum; s++)
                                    {
                                        if (0 == timeconfig.TimeConfigList[z][s].StageId)
                                            break;
                                        get_phase_id(timeconfig.TimeConfigList[z][s].PhaseId,&tempid);
                                        if (coordphase == tempid)
                                        {
                                            for (x = 0; x < s; x++)
                                            {
                                                coordtime += timeconfig.TimeConfigList[z][x].GreenTime \
                                                        + timeconfig.TimeConfigList[z][x].YellowTime \
                                                        + timeconfig.TimeConfigList[z][x].RedTime;
                                            }//for (x = 0; x < s; x++)
                                            break;
                                        }//if (coordphase == tempid)    
                                    }//for (s = 0; s < timeconfig.FactStageNum; s++)
                                    break;
                                }//if (tcid == timeconfig.TimeConfigList[z][0].TimeConfigId)
                            }//for (z = 0; z < timeconfig.FactTimeConfigNum; z++)
						}//if ((0 != coordphase) && (4 == contmode))
						if ((12 == contmode)&&(0 != coordphase)&&(!(auxfunc & 0x10))&&(!(auxfunc & 0x20)))
						{
							for (z = 0; z < timeconfig.FactTimeConfigNum; z++)
                            {
                                if (0 == timeconfig.TimeConfigList[z][0].TimeConfigId)
                                    break;
                                if (tcid == timeconfig.TimeConfigList[z][0].TimeConfigId)
                                {
                                    for (s = 0; s < timeconfig.FactStageNum; s++)
                                    {
                                        if (0 == timeconfig.TimeConfigList[z][s].StageId)
                                            break;
                                        get_phase_id(timeconfig.TimeConfigList[z][s].PhaseId,&tempid);
                                        if (coordphase == tempid)
                                        {
                                            for (x = 0; x < s; x++)
                                            {
                                                coordtime += timeconfig.TimeConfigList[z][x].GreenTime \
                                                        + timeconfig.TimeConfigList[z][x].YellowTime \
                                                        + timeconfig.TimeConfigList[z][x].RedTime;
                                            }//for (x = 0; x < s; x++)
                                            break;
                                        }//if (coordphase == tempid)    
                                    }//for (s = 0; s < timeconfig.FactStageNum; s++)
                                    break;
                                }//if (tcid == timeconfig.TimeConfigList[z][0].TimeConfigId)
                            }//for (z = 0; z < timeconfig.FactTimeConfigNum; z++)
						}//if ((0 != coordphase) && (4 == contmode))

						if ((0 != ncoordphase) && (4 == ncontmode))
						{
							for (z = 0; z < timeconfig.FactTimeConfigNum; z++)
							{
								if (0 == timeconfig.TimeConfigList[z][0].TimeConfigId)
									break;
								if (ntcid == timeconfig.TimeConfigList[z][0].TimeConfigId)
								{
									for (s = 0; s < timeconfig.FactStageNum; s++)
									{
										if (0 == timeconfig.TimeConfigList[z][s].StageId)
											break;
										get_phase_id(timeconfig.TimeConfigList[z][s].PhaseId,&tempid);
										if (ncoordphase == tempid)
										{
											for (x = 0; x < s; x++)
											{
												ncoordtime += timeconfig.TimeConfigList[z][x].GreenTime \
															+ timeconfig.TimeConfigList[z][x].YellowTime \
															+ timeconfig.TimeConfigList[z][x].RedTime;
											}
											break;
										}//if (ncoordphase == tempid)
									}//for (s = 0; s < timeconfig.FactStageNum; s++)
									break;
								}//if (ntcid == timeconfig.TimeConfigList[z][0].TimeConfigId)
							}//for (z = 0; z < timeconfig.FactTimeConfigNum; z++)
						}//if ((0 != ncoordphase) && (4 == ncontmode))
						if ((12==ncontmode)&&(0!=ncoordphase)&&(!(auxfunc & 0x10))&&(!(auxfunc & 0x20)))
						{
                            for (z = 0; z < timeconfig.FactTimeConfigNum; z++)
                            {
                                if (0 == timeconfig.TimeConfigList[z][0].TimeConfigId)
                                    break;
                                if (ntcid == timeconfig.TimeConfigList[z][0].TimeConfigId)
                                {
                                    for (s = 0; s < timeconfig.FactStageNum; s++)
                                    {
                                        if (0 == timeconfig.TimeConfigList[z][s].StageId)
                                            break;
                                        get_phase_id(timeconfig.TimeConfigList[z][s].PhaseId,&tempid);
                                        if (ncoordphase == tempid)
                                        {
                                            for (x = 0; x < s; x++)
                                            {
                                                ncoordtime += timeconfig.TimeConfigList[z][x].GreenTime \
                                                            + timeconfig.TimeConfigList[z][x].YellowTime \
                                                            + timeconfig.TimeConfigList[z][x].RedTime;
                                            }
                                            break;
                                        }//if (ncoordphase == tempid)
                                    }//for (s = 0; s < timeconfig.FactStageNum; s++)
                                    break;
                                }//if (ntcid == timeconfig.TimeConfigList[z][0].TimeConfigId)
                            }//for (z = 0; z < timeconfig.FactTimeConfigNum; z++)
                        }//if ((0 != ncoordphase) && (4 == ncontmode))
				
					
						if ((0 != coordphase) && (4 == contmode))
						{
							st += poffset;
							st -= coordtime; 
						}
						else if ((12 == contmode)&&(0 != coordphase)&&(!(auxfunc & 0x10))&&(!(auxfunc & 0x20)))
                    	{//area coordinate
                        	st += poffset;
                        	st -= coordtime;
                    	}//area coordinate
						//added by sk on 20160322
						else if (10 == contmode)
						{//coord detect
							st += poffset;
						//	st -= coordtime;
						}//coord detect
						//end add
						if ((0 != ncoordphase) && (4 == ncontmode))
						{
							nst += npoffset;
							nst -= ncoordtime;
						}
						else if ((12==ncontmode)&&(0!=ncoordphase)&&(!(auxfunc & 0x10))&&(!(auxfunc & 0x20)))
                    	{//area coordinate
                        	nst += npoffset;
                        	nst -= ncoordtime;
                    	}//area coordinate
						//added by sk on 20160322
						else if (10 == ncontmode)
						{//coord detect
							nst += npoffset;
						//	nst -= ncoordtime;
						}//coord detect
						//end add

						#ifdef MAIN_DEBUG
						printf("current time: %d,start time: %d, next start time: %d****\n",cst,st,nst);
						#endif

						if (cst < st)
						{
						#ifdef MAIN_DEBUG
							printf("Begin to yellow flash,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						//	if (0 == timesection.TimeSectionList[i][0].StartHour)
						//		continue;
							//重新循环
							if(iFlashCnt ++  < c_iMaxLoopCnt)
							{
#ifdef FLASH_DEBUG
								char szInfo[32] = {0};
								char szInfoT[64] = {0};
								snprintf(szInfo,sizeof(szInfo)-1,"%s",__FUNCTION__);
								snprintf(szInfoT,sizeof(szInfoT)-1,"%d Flashcnt %d",__LINE__,iFlashCnt);
								tsc_save_eventlog(szInfo,szInfoT);
#endif

								sleep(2);
								markbit |= 0x0040;
								break;
							}
							//added by sk on 20150315 to solve bug that yellow flash 
							//after control whole night

							//all red
            	//			new_all_red(&ardata);
					//		all_red(serial[0],0);
							//yellow flash
							if (0 == yfyes)
							{
							
#ifdef FLASH_DEBUG
								char szInfo[32] = {0};
								char szInfoT[64] = {0};
								snprintf(szInfo,sizeof(szInfo)-1,"%s",__FUNCTION__);
								snprintf(szInfoT,sizeof(szInfoT)-1,"%d",__LINE__);
								tsc_save_eventlog(szInfo,szInfoT);
#endif
								int yfret = pthread_create(&yfid,NULL,(void *)err_yellow_flash,&yfdata);
								if (0 != yfret)
								{
								#ifdef MAIN_DEBUG
									printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									return FAIL;
								}
								yfyes = 1;
								update_event_list(&eventclass,&eventlog,1,10,tt.tv_sec,&markbit);
								output_log("time is before");
							}
							waittime = 0;
							waittime = (int)(st - cst);

							mtime = waittime;
							updateyes = 0;
							while (1)
							{
								FD_ZERO(&endnRead);
								FD_SET(endpipe[0],&endnRead);
								endmt.tv_sec = mtime;
								endmt.tv_usec = 0;
								lastt.tv_sec = 0;
								lastt.tv_usec = 0;
								gettimeofday(&lastt,NULL);
								int endret = select(endpipe[0]+1,&endnRead,NULL,NULL,&endmt);
								if (endret < 0)
								{
								#ifdef MAIN_DEBUG
									printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									return FAIL;
								} 
								if (0 == endret)
								{
								#ifdef MAIN_DEBUG
									printf("Timeout,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									if (1 == yfyes)
									{
										pthread_cancel(yfid);
										pthread_join(yfid,NULL);
										yfyes = 0;
									}
									break;
								}
								if (endret > 0)
								{
									if (FD_ISSET(endpipe[0],&endnRead))
									{//if (FD_ISSET(endpipe[0],&endnRead))
										memset(enddata,0,sizeof(enddata));
										endnum = read(endpipe[0],enddata,sizeof(enddata));
										if (endnum > 0)
										{
											if (!strcmp(enddata,"UpdateFile"))
											{
												if (1 == yfyes)
                                    			{
                                        			pthread_cancel(yfid);
                                        			pthread_join(yfid,NULL);
                                        			yfyes = 0;
                                    			}
												markbit |= 0x0040;
												updateyes = 1;
												break;
											}
											else
											{
												nowt.tv_sec = 0;
												nowt.tv_usec = 0;
												gettimeofday(&nowt,NULL);
												leat = nowt.tv_sec - lastt.tv_sec;
												mtime -= leat;
												continue;
											}
										}//endnum > 0
										else
										{//endnum <=0
											nowt.tv_sec = 0;
											nowt.tv_usec = 0;
											gettimeofday(&nowt,NULL);
											leat = nowt.tv_sec - lastt.tv_sec;
											mtime -= leat;
											continue;
										}//endnum <=0	
									}//if (FD_ISSET(endpipe[0],&endnRead))
									else
									{// Not 'if (FD_ISSET(endpipe[0],&endnRead))'
										nowt.tv_sec = 0;
										nowt.tv_usec = 0;
										gettimeofday(&nowt,NULL);
										leat = nowt.tv_sec - lastt.tv_sec;
										mtime -= leat;
										continue;
									}//Not 'if (FD_ISSET(endpipe[0],&endnRead))'
								}//endret > 0
							}//inline 2 while loop

							if (1 == updateyes)
							{
								cpyes = 0;
								break;
							}
							//all red
            				new_all_red(&ardata);
						//	all_red(serial[0],0);						
	
							//call fit control pattern according to patternID and contmode;
							if (0 == cpyes)
							{
								//add 20220803 清除相位降级信息
								//printf("start clean phase degrphid info ,File: %s,Line: %d\n",__FILE__,__LINE__);
								unsigned char				err1[10] = {0};
								int iIndex = 0;
								for(iIndex = 0; iIndex<MAX_PHASE_LINE;iIndex++)
								{
									if((phasedegrphid >> iIndex) & 0x00000001)
									{
										memset(err1,0,sizeof(err1));
										gettimeofday(&nowt,NULL);
										if (SUCCESS != err_report(err1,nowt.tv_sec,21,(82+iIndex)))
										{
												#ifdef MAIN_DEBUG
												printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
												#endif
										}
										else
										{
											write(csockfd,err1,sizeof(err1));
										}
									}
								}
								phasedegrphid = 0;
								//printf("clean phase degrphid info end,File: %s,Line: %d\n",__FILE__,__LINE__);
								//add 20220803

								memset(&fcdata,0,sizeof(fcdata));
								fcdata.lip = &lip;
								fcdata.patternid = &patternId;
								fcdata.slnum = &retslnum;
								fcdata.stageid = &retstageid;
								fcdata.phaseid = &retphaseid;
								fcdata.color = &retcolor;
                            	fcdata.markbit = &markbit;
								fcdata.markbit2 = &markbit2;
								fcdata.wlmark = wlmark;
								fcdata.sockfd = &sockfd;
                            	fcdata.flowpipe = &flowpipe[0];
								fcdata.pendpipe = &pendpipe[0];
                            	fcdata.conpipe = &conpipe[0];
                            	fcdata.synpipe = &synpipe[1];
								fcdata.endpipe = &endpipe[1];
								fcdata.ffpipe = &ffpipe[0];
                            	fcdata.bbserial = &serial[0];
								fcdata.vhserial = &serial[1];
								fcdata.fbserial = &serial[2];
								fcdata.sdserial = &serial[4];
								fcdata.ec = &eventclass;
                            	fcdata.el = &eventlog;

								fcdata.contmode = &contmode;
								fcdata.ncontmode = &ncontmode;
								fcdata.auxfunc = &auxfunc;
								fcdata.reflags = &(unit.RemoteFlag);
								fcdata.halfdownt = &halfdownt;
								fcdata.specfunc = &specfunc;
								fcdata.roadinfo = &roadinfo;
								fcdata.roadinforeceivetime = &roadinforeceivetime;
								fcdata.phasedegrphid = & phasedegrphid;
//								fcdata.shour = &shour;
//								fcdata.smin = &smin;
//								fcdata.coordtime = &coordtime;
//								fcdata.poffset = &poffset;
								fcdata.cyclet = &cyclet;
								fcdata.coordphase = &coordphase;

//								fcdata.nshour = &nshour;
//								fcdata.nsmin = &nsmin;
//								fcdata.ncoordtime = &ncoordtime;
//								fcdata.npoffset = &npoffset;
//								fcdata.ncyclet = &ncyclet;
								fcdata.ncoordphase = &ncoordphase;
								fcdata.st = st;	
								fcdata.nst = nst;
								fcdata.sinfo = &sinfo;
								fcdata.softevent = softevent;//added by shikang on 20171110
								fcdata.ccontrol = &ccontrol;//added by shikang on 20180118
								fcdata.fcontrol = &fcontrol;
								fcdata.trans = &trans;

								fcdata.v2xmark = &v2xmark; //V2X
								fcdata.slg = slg;
								int cpret = pthread_create(&cpid,NULL,(void *)run_control_pattern,&fcdata);
								if (0 != cpret)
								{
								#ifdef MAIN_DEBUG
									printf("Create thread err,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
									return FAIL;
								} 
								cpyes = 1;
							}	
							waittime = 0;
							waittime = (int)(nst - st);
							if ((0 != ncoordphase) && (4 == ncontmode))
							{
								if (waittime > (2 * cyclet))
								{
									waittime -= 2*cyclet;
									mtime = waittime;
									updateyes = 0;
									while (1)
									{//inline 2 while loop
										FD_ZERO(&endnRead);
										FD_SET(endpipe[0],&endnRead);
										endmt.tv_sec = mtime;
										endmt.tv_usec = 0;
										lastt.tv_sec = 0;
										lastt.tv_usec = 0;
										gettimeofday(&lastt,NULL);
										int endret = select(endpipe[0]+1,&endnRead,NULL,NULL,&endmt);
										if (endret < 0)
										{
										#ifdef MAIN_DEBUG
											printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											return FAIL;
										} 
										if (0 == endret)
										{
										#ifdef MAIN_DEBUG
											printf("Timeout,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											if (1 == yfyes)
											{
												pthread_cancel(yfid);
												pthread_join(yfid,NULL);
												yfyes = 0;
											}
											break;
										}
										if (endret > 0)
										{
											if (FD_ISSET(endpipe[0],&endnRead))
											{//if (FD_ISSET(endpipe[0],&endnRead))
												memset(enddata,0,sizeof(enddata));
												endnum = read(endpipe[0],enddata,sizeof(enddata));
												if (endnum > 0)
												{
													if (!strcmp(enddata,"UpdateFile"))
													{
														if (1 == yfyes)
                                    					{
                                        					pthread_cancel(yfid);
                                        					pthread_join(yfid,NULL);
                                        					yfyes = 0;
                                    					}
														markbit |= 0x0040;
														updateyes = 1;
														break;
													}
													else
													{
														nowt.tv_sec = 0;
														nowt.tv_usec = 0;
														gettimeofday(&nowt,NULL);
														leat = nowt.tv_sec - lastt.tv_sec;
														mtime -= leat;
														continue;
													}
												}//endnum > 0
												else
												{//endnum <=0
													nowt.tv_sec = 0;
													nowt.tv_usec = 0;
													gettimeofday(&nowt,NULL);
													leat = nowt.tv_sec - lastt.tv_sec;
													mtime -= leat;
													continue;
												}//endnum <=0	
											}//if (FD_ISSET(endpipe[0],&endnRead))
											else
											{// Not 'if (FD_ISSET(endpipe[0],&endnRead))'
												nowt.tv_sec = 0;
												nowt.tv_usec = 0;
												gettimeofday(&nowt,NULL);
												leat = nowt.tv_sec - lastt.tv_sec;
												mtime -= leat;
												continue;
											}//Not 'if (FD_ISSET(endpipe[0],&endnRead))'
										}//endret > 0
									}//inline 2 while loop
									if (1 == updateyes)
									{
										markbit |= 0x0001;
										break;
									}
									markbit |= 0x0100;
								#ifdef MAIN_DEBUG
									printf("Left two cycles,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
									mtime = 2*cyclet;
									updateyes = 0;
									while (1)
									{//inline 2 while loop
										FD_ZERO(&endnRead);
										FD_SET(endpipe[0],&endnRead);
										endmt.tv_sec = mtime;
										endmt.tv_usec = 0;
										lastt.tv_sec = 0;
										lastt.tv_usec = 0;
										gettimeofday(&lastt,NULL);
										int endret = select(endpipe[0]+1,&endnRead,NULL,NULL,&endmt);
										if (endret < 0)
										{
										#ifdef MAIN_DEBUG
											printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											return FAIL;
										} 
										if (0 == endret)
										{
										#ifdef MAIN_DEBUG
											printf("Timeout,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											if (1 == yfyes)
											{
												pthread_cancel(yfid);
												pthread_join(yfid,NULL);
												yfyes = 0;
											}
											break;
										}
										if (endret > 0)
										{
											if (FD_ISSET(endpipe[0],&endnRead))
											{//if (FD_ISSET(endpipe[0],&endnRead))
												memset(enddata,0,sizeof(enddata));
												endnum = read(endpipe[0],enddata,sizeof(enddata));
												if (endnum > 0)
												{
													if (!strcmp(enddata,"UpdateFile"))
													{
														if (1 == yfyes)
                                    					{
                                        					pthread_cancel(yfid);
                                        					pthread_join(yfid,NULL);
                                        					yfyes = 0;
                                    					}
														markbit |= 0x0040;
														updateyes = 1;
														break;
													}
													else
													{
														nowt.tv_sec = 0;
														nowt.tv_usec = 0;
														gettimeofday(&nowt,NULL);
														leat = nowt.tv_sec - lastt.tv_sec;
														mtime -= leat;
														continue;
													}
												}//endnum > 0
												else
												{//endnum <=0
													nowt.tv_sec = 0;
													nowt.tv_usec = 0;
													gettimeofday(&nowt,NULL);
													leat = nowt.tv_sec - lastt.tv_sec;
													mtime -= leat;
													continue;
												}//endnum <=0	
											}//if (FD_ISSET(endpipe[0],&endnRead))
											else
											{// Not 'if (FD_ISSET(endpipe[0],&endnRead))'
												nowt.tv_sec = 0;
												nowt.tv_usec = 0;
												gettimeofday(&nowt,NULL);
												leat = nowt.tv_sec - lastt.tv_sec;
												mtime -= leat;
												continue;
											}//Not 'if (FD_ISSET(endpipe[0],&endnRead))'
										}//endret > 0
									}//inline 2 while loop
									markbit |= 0x0001;
									if (1 == updateyes)
									{
										break;
									}	
								}
								else
								{
									mtime = waittime;
									updateyes = 0;
									while (1)
									{//inline 2 while loop
										FD_ZERO(&endnRead);
										FD_SET(endpipe[0],&endnRead);
										endmt.tv_sec = mtime;
										endmt.tv_usec = 0;
										lastt.tv_sec = 0;
										lastt.tv_usec = 0;
										gettimeofday(&lastt,NULL);
										int endret = select(endpipe[0]+1,&endnRead,NULL,NULL,&endmt);
										if (endret < 0)
										{
										#ifdef MAIN_DEBUG
											printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											return FAIL;
										} 
										if (0 == endret)
										{
										#ifdef MAIN_DEBUG
											printf("Timeout,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											if (1 == yfyes)
											{
												pthread_cancel(yfid);
												pthread_join(yfid,NULL);
												yfyes = 0;
											}
											break;
										}
										if (endret > 0)
										{
											if (FD_ISSET(endpipe[0],&endnRead))
											{//if (FD_ISSET(endpipe[0],&endnRead))
												memset(enddata,0,sizeof(enddata));
												endnum = read(endpipe[0],enddata,sizeof(enddata));
												if (endnum > 0)
												{
													if (!strcmp(enddata,"UpdateFile"))
													{
														if (1 == yfyes)
                                    					{
                                        					pthread_cancel(yfid);
                                        					pthread_join(yfid,NULL);
                                        					yfyes = 0;
                                    					}
														markbit |= 0x0040;
														updateyes = 1;
														break;
													}
													else
													{
														nowt.tv_sec = 0;
														nowt.tv_usec = 0;
														gettimeofday(&nowt,NULL);
														leat = nowt.tv_sec - lastt.tv_sec;
														mtime -= leat;
														continue;
													}
												}//endnum > 0
												else
												{//endnum <=0
													nowt.tv_sec = 0;
													nowt.tv_usec = 0;
													gettimeofday(&nowt,NULL);
													leat = nowt.tv_sec - lastt.tv_sec;
													mtime -= leat;
													continue;
												}//endnum <=0	
											}//if (FD_ISSET(endpipe[0],&endnRead))
											else
											{// Not 'if (FD_ISSET(endpipe[0],&endnRead))'
												nowt.tv_sec = 0;
												nowt.tv_usec = 0;
												gettimeofday(&nowt,NULL);
												leat = nowt.tv_sec - lastt.tv_sec;
												mtime -= leat;
												continue;
											}//Not 'if (FD_ISSET(endpipe[0],&endnRead))'
										}//endret > 0
									}//inline 2 while loop
									markbit |= 0x0001;
									if (1 == updateyes)
									{
										break;
									}
								}
							}
							else
							{
								mtime = waittime;
								updateyes = 0;
								while (1)
								{//inline 2 while loop
									FD_ZERO(&endnRead);
									FD_SET(endpipe[0],&endnRead);
									endmt.tv_sec = mtime;
									endmt.tv_usec = 0;
									lastt.tv_sec = 0;
									lastt.tv_usec = 0;
									gettimeofday(&lastt,NULL);
									int endret = select(endpipe[0]+1,&endnRead,NULL,NULL,&endmt);
									if (endret < 0)
									{
									#ifdef MAIN_DEBUG
										printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										return FAIL;
									} 
									if (0 == endret)
									{
									#ifdef MAIN_DEBUG
										printf("Timeout,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										if (1 == yfyes)
										{
											pthread_cancel(yfid);
											pthread_join(yfid,NULL);
											yfyes = 0;
										}
										break;
									}
									if (endret > 0)
									{
										if (FD_ISSET(endpipe[0],&endnRead))
										{//if (FD_ISSET(endpipe[0],&endnRead))
											memset(enddata,0,sizeof(enddata));
											endnum = read(endpipe[0],enddata,sizeof(enddata));
											if (endnum > 0)
											{
												if (!strcmp(enddata,"UpdateFile"))
												{
													if (1 == yfyes)
                                   					{
                                       					pthread_cancel(yfid);
                                       					pthread_join(yfid,NULL);
                                       					yfyes = 0;
                                   					}
													markbit |= 0x0040;
													updateyes = 1;
													break;
												}
												else
												{
													nowt.tv_sec = 0;
													nowt.tv_usec = 0;
													gettimeofday(&nowt,NULL);
													leat = nowt.tv_sec - lastt.tv_sec;
													mtime -= leat;
													continue;
												}
											}//endnum > 0
											else
											{//endnum <=0
												nowt.tv_sec = 0;
												nowt.tv_usec = 0;
												gettimeofday(&nowt,NULL);
												leat = nowt.tv_sec - lastt.tv_sec;
												mtime -= leat;
												continue;
											}//endnum <=0	
										}//if (FD_ISSET(endpipe[0],&endnRead))
										else
										{// Not 'if (FD_ISSET(endpipe[0],&endnRead))'
											nowt.tv_sec = 0;
											nowt.tv_usec = 0;
											gettimeofday(&nowt,NULL);
											leat = nowt.tv_sec - lastt.tv_sec;
											mtime -= leat;
											continue;
										}//Not 'if (FD_ISSET(endpipe[0],&endnRead))'
									}//endret > 0
								}//inline 2 while loop
								markbit |= 0x0001;
								if (1 == updateyes)
								{
									break;
								}	
							}	
						}//if ((chour < shour) || ((chour == shour) && (cmin < smin)))

						if (cst >= st)
						{
							if (cst < nst)
							{
								iFlashCnt = 0;
							#ifdef MAIN_DEBUG
								printf("Hello,I am on here,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
								//end it if yellow flash exists;
								if (1 == yfyes)	
								{
									pthread_cancel(yfid);
									pthread_join(yfid,NULL);
									yfyes = 0;
								}
								//all red
                            	new_all_red(&ardata);
							//	all_red(serial[0],0);
								//call fit pattern according to patternid and contmode;
								if (0 == cpyes)
								{
									//add 20220803 清除相位降级信息
									//printf("start clean phase degrphid info ,File: %s,Line: %d\n",__FILE__,__LINE__);
									unsigned char				err1[10] = {0};
									int iIndex = 0;
									for(iIndex = 0; iIndex<MAX_PHASE_LINE;iIndex++)
									{
										if((phasedegrphid >> iIndex) & 0x00000001)
										{
											memset(err1,0,sizeof(err1));
											gettimeofday(&nowt,NULL);
											if (SUCCESS != err_report(err1,nowt.tv_sec,21,(82+iIndex)))
											{
												#ifdef MAIN_DEBUG
													printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
												#endif
											}
											else
											{
												write(csockfd,err1,sizeof(err1));
											}
										}
									}
									phasedegrphid = 0;
									//printf("clean phase degrphid info end,File: %s,Line: %d\n",__FILE__,__LINE__);
									//add 20220803
									memset(&fcdata,0,sizeof(fcdata));
									fcdata.lip = &lip;
                                    fcdata.patternid = &patternId;
									fcdata.slnum = &retslnum;
									fcdata.stageid = &retstageid;
									fcdata.phaseid = &retphaseid;
									fcdata.color = &retcolor;
                            		fcdata.markbit = &markbit;
									fcdata.markbit2 = &markbit2;
									fcdata.wlmark = wlmark;
									fcdata.sockfd = &sockfd;
                            		fcdata.flowpipe = &flowpipe[0];
									fcdata.pendpipe = &pendpipe[0];
                            		fcdata.conpipe = &conpipe[0];
                            		fcdata.synpipe = &synpipe[1];
									fcdata.endpipe = &endpipe[1];
									fcdata.ffpipe = &ffpipe[0];
                            		fcdata.bbserial = &serial[0];
									fcdata.vhserial = &serial[1];
									fcdata.fbserial = &serial[2];
									fcdata.sdserial = &serial[4];
									fcdata.ec = &eventclass;
                            		fcdata.el = &eventlog;

									fcdata.contmode = &contmode;
                                	fcdata.ncontmode = &ncontmode;
									fcdata.auxfunc = &auxfunc;
									fcdata.reflags = &(unit.RemoteFlag);
									fcdata.halfdownt = &halfdownt;
									fcdata.specfunc = &specfunc;
									fcdata.roadinfo = &roadinfo;
									fcdata.roadinforeceivetime = &roadinforeceivetime;
									fcdata.phasedegrphid = & phasedegrphid;
//                                	fcdata.shour = &shour;
//                                	fcdata.smin = &smin;
//                                	fcdata.coordtime = &coordtime;
//                                	fcdata.poffset = &poffset;
									fcdata.cyclet = &cyclet;
									fcdata.coordphase = &coordphase;

//                                	fcdata.nshour = &nshour;
//                                	fcdata.nsmin = &nsmin;
//                                	fcdata.ncoordtime = &ncoordtime;
//                                	fcdata.npoffset = &npoffset;
//									fcdata.ncyclet = &ncyclet;
									fcdata.ncoordphase = &ncoordphase;
									fcdata.st = st;
									fcdata.nst = nst;
									fcdata.sinfo = &sinfo;
									fcdata.softevent = softevent;//added by shikang on 20171110
									fcdata.ccontrol = &ccontrol;//added by shikang on 20180118
									fcdata.fcontrol = &fcontrol;
									fcdata.trans = &trans;

									fcdata.v2xmark = &v2xmark; //V2X
									fcdata.slg = slg;
									int cpret = pthread_create(&cpid,NULL,(void *)run_control_pattern,&fcdata);
									if (0 != cpret)
									{
									#ifdef MAIN_DEBUG
										printf("Create thread err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
										return FAIL;
									}
									cpyes = 1;
								}
								waittime = 0;
								waittime = (int)(nst - cst);
								if ((0 != ncoordphase) && (4 == ncontmode))
								{
									if (waittime > (2 * cyclet))
									{
										waittime -= 2*cyclet;
										mtime = waittime;
										updateyes = 0;
										while (1)
										{//inline 2 while loop
											FD_ZERO(&endnRead);
											FD_SET(endpipe[0],&endnRead);
											endmt.tv_sec = mtime;
											endmt.tv_usec = 0;
											lastt.tv_sec = 0;
											lastt.tv_usec = 0;
											gettimeofday(&lastt,NULL);
											int endret = select(endpipe[0]+1,&endnRead,NULL,NULL,&endmt);
											if (endret < 0)
											{
											#ifdef MAIN_DEBUG
												printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												return FAIL;
											} 
											if (0 == endret)
											{
											#ifdef MAIN_DEBUG
												printf("Timeout,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												if (1 == yfyes)
												{
													pthread_cancel(yfid);
													pthread_join(yfid,NULL);
													yfyes = 0;
												}
												break;
											}
											if (endret > 0)
											{
												if (FD_ISSET(endpipe[0],&endnRead))
												{//if (FD_ISSET(endpipe[0],&endnRead))
													memset(enddata,0,sizeof(enddata));
													endnum = read(endpipe[0],enddata,sizeof(enddata));
													if (endnum > 0)
													{
														if (!strcmp(enddata,"UpdateFile"))
														{
															if (1 == yfyes)
                                    						{
                                        						pthread_cancel(yfid);
                                        						pthread_join(yfid,NULL);
                                        						yfyes = 0;
                                    						}
															markbit |= 0x0040;
															updateyes = 1;
															break;
														}
														else
														{
															nowt.tv_sec = 0;
															nowt.tv_usec = 0;
															gettimeofday(&nowt,NULL);
															leat = nowt.tv_sec - lastt.tv_sec;
															mtime -= leat;
															continue;
														}
													}//endnum > 0
													else
													{//endnum <=0
														nowt.tv_sec = 0;
														nowt.tv_usec = 0;
														gettimeofday(&nowt,NULL);
														leat = nowt.tv_sec - lastt.tv_sec;
														mtime -= leat;
														continue;
													}//endnum <=0	
												}//if (FD_ISSET(endpipe[0],&endnRead))
												else
												{// Not 'if (FD_ISSET(endpipe[0],&endnRead))'
													nowt.tv_sec = 0;
													nowt.tv_usec = 0;
													gettimeofday(&nowt,NULL);
													leat = nowt.tv_sec - lastt.tv_sec;
													mtime -= leat;
													continue;
												}//Not 'if (FD_ISSET(endpipe[0],&endnRead))'
											}//endret > 0
										}//inline 2 while loop
										if (1 == updateyes)
										{
											markbit |= 0x0001;
											break;
										}	
										markbit |= 0x0100;
									#ifdef MAIN_DEBUG
                                    	printf("Left two cycles,File:%s,Line:%d\n",__FILE__,__LINE__);
                                	#endif
										mtime = 2*cyclet;
										updateyes = 0;
										while (1)
										{//inline 2 while loop
											FD_ZERO(&endnRead);
											FD_SET(endpipe[0],&endnRead);
											endmt.tv_sec = mtime;
											endmt.tv_usec = 0;
											lastt.tv_sec = 0;
											lastt.tv_usec = 0;
											gettimeofday(&lastt,NULL);
											int endret = select(endpipe[0]+1,&endnRead,NULL,NULL,&endmt);
											if (endret < 0)
											{
											#ifdef MAIN_DEBUG
												printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												return FAIL;
											} 
											if (0 == endret)
											{
											#ifdef MAIN_DEBUG
												printf("Timeout,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												if (1 == yfyes)
												{
													pthread_cancel(yfid);
													pthread_join(yfid,NULL);
													yfyes = 0;
												}
												break;
											}
											if (endret > 0)
											{
												if (FD_ISSET(endpipe[0],&endnRead))
												{//if (FD_ISSET(endpipe[0],&endnRead))
													memset(enddata,0,sizeof(enddata));
													endnum = read(endpipe[0],enddata,sizeof(enddata));
													if (endnum > 0)
													{
														if (!strcmp(enddata,"UpdateFile"))
														{
															if (1 == yfyes)
                                    						{
                                        						pthread_cancel(yfid);
                                        						pthread_join(yfid,NULL);
                                        						yfyes = 0;
                                    						}
															markbit |= 0x0040;
															updateyes = 1;
															break;
														}
														else
														{
															nowt.tv_sec = 0;
															nowt.tv_usec = 0;
															gettimeofday(&nowt,NULL);
															leat = nowt.tv_sec - lastt.tv_sec;
															mtime -= leat;
															continue;
														}
													}//endnum > 0
													else
													{//endnum <=0
														nowt.tv_sec = 0;
														nowt.tv_usec = 0;
														gettimeofday(&nowt,NULL);
														leat = nowt.tv_sec - lastt.tv_sec;
														mtime -= leat;
														continue;
													}//endnum <=0	
												}//if (FD_ISSET(endpipe[0],&endnRead))
												else
												{// Not 'if (FD_ISSET(endpipe[0],&endnRead))'
													nowt.tv_sec = 0;
													nowt.tv_usec = 0;
													gettimeofday(&nowt,NULL);
													leat = nowt.tv_sec - lastt.tv_sec;
													mtime -= leat;
													continue;
												}//Not 'if (FD_ISSET(endpipe[0],&endnRead))'
											}//endret > 0
										}//inline 2 while loop
										markbit |= 0x0001;
										if (1 == updateyes)
										{
											break;
										}
									}
									else
									{
										mtime = waittime;
										updateyes = 0;
										while (1)
										{//inline 2 while loop
											FD_ZERO(&endnRead);
											FD_SET(endpipe[0],&endnRead);
											endmt.tv_sec = mtime;
											endmt.tv_usec = 0;
											lastt.tv_sec = 0;
											lastt.tv_usec = 0;
											gettimeofday(&lastt,NULL);
											int endret = select(endpipe[0]+1,&endnRead,NULL,NULL,&endmt);
											if (endret < 0)
											{
											#ifdef MAIN_DEBUG
												printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												return FAIL;
											} 
											if (0 == endret)
											{
											#ifdef MAIN_DEBUG
												printf("Timeout,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												if (1 == yfyes)
												{
													pthread_cancel(yfid);
													pthread_join(yfid,NULL);
													yfyes = 0;
												}
												break;
											}
											if (endret > 0)
											{
												if (FD_ISSET(endpipe[0],&endnRead))
												{//if (FD_ISSET(endpipe[0],&endnRead))
													memset(enddata,0,sizeof(enddata));
													endnum = read(endpipe[0],enddata,sizeof(enddata));
													if (endnum > 0)
													{
														if (!strcmp(enddata,"UpdateFile"))
														{
															if (1 == yfyes)
                                    						{
                                        						pthread_cancel(yfid);
                                        						pthread_join(yfid,NULL);
                                        						yfyes = 0;
                                    						}
															markbit |= 0x0040;
															updateyes = 1;
															break;
														}
														else
														{
															nowt.tv_sec = 0;
															nowt.tv_usec = 0;
															gettimeofday(&nowt,NULL);
															leat = nowt.tv_sec - lastt.tv_sec;
															mtime -= leat;
															continue;
														}
													}//endnum > 0
													else
													{//endnum <=0
														nowt.tv_sec = 0;
														nowt.tv_usec = 0;
														gettimeofday(&nowt,NULL);
														leat = nowt.tv_sec - lastt.tv_sec;
														mtime -= leat;
														continue;
													}//endnum <=0	
												}//if (FD_ISSET(endpipe[0],&endnRead))
												else
												{// Not 'if (FD_ISSET(endpipe[0],&endnRead))'
													nowt.tv_sec = 0;
													nowt.tv_usec = 0;
													gettimeofday(&nowt,NULL);
													leat = nowt.tv_sec - lastt.tv_sec;
													mtime -= leat;
													continue;
												}//Not 'if (FD_ISSET(endpipe[0],&endnRead))'
											}//endret > 0
										}//inline 2 while loop
										markbit |= 0x0001;
										if (1 == updateyes)
										{
											break;
										}
									}
								}
								else
								{
									mtime = waittime;
									updateyes = 0;
									while (1)
									{//inline 2 while loop
										FD_ZERO(&endnRead);
										FD_SET(endpipe[0],&endnRead);
										endmt.tv_sec = mtime;
										endmt.tv_usec = 0;
										lastt.tv_sec = 0;
										lastt.tv_usec = 0;
										gettimeofday(&lastt,NULL);
										int endret = select(endpipe[0]+1,&endnRead,NULL,NULL,&endmt);
										if (endret < 0)
										{
										#ifdef MAIN_DEBUG
											printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											return FAIL;
										} 
										if (0 == endret)
										{
										#ifdef MAIN_DEBUG
											printf("Timeout,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											if (1 == yfyes)
											{
												pthread_cancel(yfid);
												pthread_join(yfid,NULL);
												yfyes = 0;
											}
											break;
										}
										if (endret > 0)
										{
											if (FD_ISSET(endpipe[0],&endnRead))
											{//if (FD_ISSET(endpipe[0],&endnRead))
												memset(enddata,0,sizeof(enddata));
												endnum = read(endpipe[0],enddata,sizeof(enddata));
												if (endnum > 0)
												{
													if (!strcmp(enddata,"UpdateFile"))
													{
														if (1 == yfyes)
                                   						{
                                       						pthread_cancel(yfid);
                                       						pthread_join(yfid,NULL);
                                       						yfyes = 0;
                                   						}
														markbit |= 0x0040;
														updateyes = 1;
														break;
													}
													else
													{
														nowt.tv_sec = 0;
														nowt.tv_usec = 0;
														gettimeofday(&nowt,NULL);
														leat = nowt.tv_sec - lastt.tv_sec;
														mtime -= leat;
														continue;
													}
												}//endnum > 0
												else
												{//endnum <=0
													nowt.tv_sec = 0;
													nowt.tv_usec = 0;
													gettimeofday(&nowt,NULL);
													leat = nowt.tv_sec - lastt.tv_sec;
													mtime -= leat;
													continue;
												}//endnum <=0	
											}//if (FD_ISSET(endpipe[0],&endnRead))
											else
											{// Not 'if (FD_ISSET(endpipe[0],&endnRead))'
												nowt.tv_sec = 0;
												nowt.tv_usec = 0;
												gettimeofday(&nowt,NULL);
												leat = nowt.tv_sec - lastt.tv_sec;
												mtime -= leat;
												continue;
											}//Not 'if (FD_ISSET(endpipe[0],&endnRead))'
										}//endret > 0
									}//inline 2 while loop
									markbit |= 0x0001;
									if (1 == updateyes)
									{
										break;
									}
								}	
							}//if ((chour < nshour) || ((chour == nshour) && (cmin < nsmin)))
							if (0 == nst)
							{
							#ifdef MAIN_DEBUG
                                printf("Hello,I am the last stage,File:%s,Line:%d\n",__FILE__,__LINE__);
                            #endif
								//end it if yellow flash exists
								if (1 == yfyes)
								{
									pthread_cancel(yfid);
									pthread_join(yfid,NULL);
									yfyes = 0;
								}
								//all red
                            	new_all_red(&ardata);
							//	all_red(serial[0],0);							
	
								//call fit pattern according to patternid and contmode;
								if (0 == cpyes)
								{
																		//add 20220803 清除相位降级信息
									//printf("start clean phase degrphid info ,File: %s,Line: %d\n",__FILE__,__LINE__);
									unsigned char				err1[10] = {0};
									int iIndex = 0;
									for(iIndex = 0; iIndex<MAX_PHASE_LINE;iIndex++)
									{
										if((phasedegrphid >> iIndex) & 0x00000001)
										{
											memset(err1,0,sizeof(err1));
											gettimeofday(&nowt,NULL);
											if (SUCCESS != err_report(err1,nowt.tv_sec,21,(82+iIndex)))
											{
												#ifdef MAIN_DEBUG
													printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
												#endif
											}
											else
											{
												write(csockfd,err1,sizeof(err1));
											}
										}
									}
									phasedegrphid = 0;
									//printf("clean phase degrphid info end,File: %s,Line: %d\n",__FILE__,__LINE__);
									//add 20220803
									memset(&fcdata,0,sizeof(fcdata));
									fcdata.lip = &lip;
                                    fcdata.patternid = &patternId;
									fcdata.slnum = &retslnum;
									fcdata.stageid = &retstageid;
									fcdata.phaseid = &retphaseid;
									fcdata.color = &retcolor;
                            		fcdata.markbit = &markbit;
									fcdata.markbit2 = &markbit2;
									fcdata.wlmark = wlmark;
									fcdata.sockfd = &sockfd;
                            		fcdata.flowpipe = &flowpipe[0];
									fcdata.pendpipe = &pendpipe[0];
                            		fcdata.conpipe = &conpipe[0];
                            		fcdata.synpipe = &synpipe[1];
									fcdata.endpipe = &endpipe[1];
									fcdata.ffpipe = &ffpipe[0];
                            		fcdata.bbserial = &serial[0];
									fcdata.vhserial = &serial[1];
									fcdata.fbserial = &serial[2];
									fcdata.sdserial = &serial[4];
									fcdata.ec = &eventclass;
                            		fcdata.el = &eventlog;

									fcdata.contmode = &contmode;
                                	fcdata.ncontmode = &ncontmode;
									fcdata.auxfunc = &auxfunc;
									fcdata.reflags = &(unit.RemoteFlag);
									fcdata.halfdownt = &halfdownt;
									fcdata.specfunc = &specfunc;
									fcdata.roadinfo = &roadinfo;
									fcdata.roadinforeceivetime = &roadinforeceivetime;
									fcdata.phasedegrphid = & phasedegrphid;
//                                	fcdata.shour = &shour;
//                                	fcdata.smin = &smin;
//                                	fcdata.coordtime = &coordtime;
//                                	fcdata.poffset = &poffset;
									fcdata.cyclet = &cyclet;
									fcdata.coordphase = &coordphase;

//                                	fcdata.nshour = &nshour;
//                                	fcdata.nsmin = &nsmin;
//                                	fcdata.ncoordtime = &ncoordtime;
//                                	fcdata.npoffset = &npoffset;
//									fcdata.ncyclet = &ncyclet;
									fcdata.ncoordphase = &ncoordphase;
									fcdata.st = st;
									fcdata.nst = nst;
									fcdata.sinfo = &sinfo;
									fcdata.softevent = softevent;//added by shikang on 20171110
									fcdata.ccontrol = &ccontrol;//added by shikang on 20180118
									fcdata.fcontrol = &fcontrol;
									fcdata.trans = &trans;

									fcdata.v2xmark = &v2xmark; //V2X
									fcdata.slg = slg;
									int cpret = pthread_create(&cpid,NULL,(void *)run_control_pattern,&fcdata);
									if (0 != cpret)
									{
									#ifdef MAIN_DEBUG
										printf("Create thread err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
										return FAIL;
									}
									cpyes = 1;
								}

								systime->tm_hour = 23;
								systime->tm_min = 60;
								systime->tm_sec = 0;
								nst = (long)mktime(systime); 
								waittime = (int)(nst - cst);
								mtime = waittime;
								updateyes = 0;
								while (1)
								{//inline 2 while loop
									FD_ZERO(&endnRead);
									FD_SET(endpipe[0],&endnRead);
									endmt.tv_sec = mtime;
									endmt.tv_usec = 0;
									lastt.tv_sec = 0;
									lastt.tv_usec = 0;
									gettimeofday(&lastt,NULL);
									int endret = select(endpipe[0]+1,&endnRead,NULL,NULL,&endmt);
									if (endret < 0)
									{
									#ifdef MAIN_DEBUG
										printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										return FAIL;
									} 
									if (0 == endret)
									{
									#ifdef MAIN_DEBUG
										printf("Timeout,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										if (1 == yfyes)
										{
											pthread_cancel(yfid);
											pthread_join(yfid,NULL);
											yfyes = 0;
										}
										break;
									}
									if (endret > 0)
									{
										if (FD_ISSET(endpipe[0],&endnRead))
										{//if (FD_ISSET(endpipe[0],&endnRead))
											memset(enddata,0,sizeof(enddata));
											endnum = read(endpipe[0],enddata,sizeof(enddata));
											if (endnum > 0)
											{
												if (!strcmp(enddata,"UpdateFile"))
												{
													if (1 == yfyes)
                                    				{
                                        				pthread_cancel(yfid);
                                        				pthread_join(yfid,NULL);
                                        				yfyes = 0;
                                    				}
													markbit |= 0x0040;
													updateyes = 1;
													break;
												}
												else
												{
													nowt.tv_sec = 0;
													nowt.tv_usec = 0;
													gettimeofday(&nowt,NULL);
													leat = nowt.tv_sec - lastt.tv_sec;
													mtime -= leat;
													continue;
												}
											}//endnum > 0
											else
											{//endnum <=0
												nowt.tv_sec = 0;
												nowt.tv_usec = 0;
												gettimeofday(&nowt,NULL);
												leat = nowt.tv_sec - lastt.tv_sec;
												mtime -= leat;
												continue;
											}//endnum <=0	
										}//if (FD_ISSET(endpipe[0],&endnRead))
										else
										{// Not 'if (FD_ISSET(endpipe[0],&endnRead))'
											nowt.tv_sec = 0;
											nowt.tv_usec = 0;
											gettimeofday(&nowt,NULL);
											leat = nowt.tv_sec - lastt.tv_sec;
											mtime -= leat;
											continue;
										}//Not 'if (FD_ISSET(endpipe[0],&endnRead))'
									}//endret > 0
								}//inline 2 while loop
								markbit |= 0x0001;
								if (1 == updateyes)
								{
									break;
								}
							}//if ((0 == nshour) && (0 == nsmin))
						}//if (cst >= st)	
					}//for (j = 0; j < timesection.FactEventNum; j++)
					break; //come into 1th while loop again
				}//eventnum > 1
				if (1 == eventnum)
				{
				#ifdef MAIN_DEBUG
					printf("Only one stage,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					//end it and its children if cur running pattern exists,NOTE: smooth transition;
					if (1 == cpyes)
					{
						while(1)
						{//while loop
							FD_ZERO(&synRead);
							FD_SET(synpipe[0],&synRead);
							int synret = select(synpipe[0]+1,&synRead,NULL,NULL,NULL);
							if (synret < 0)
							{
							#ifdef MAIN_DEBUG
								printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								return FAIL;
							}
							if (synret > 0)
							{
								unsigned char           syndata[4] = {0};
								read(synpipe[0],syndata,sizeof(syndata));
								if ((0xCC == syndata[0]) && (0xDD == syndata[1]) && (0xED == syndata[2]))
								{
									break;
								}
								else
								{
									continue;
								}
							}
						}//while loop
						pthread_cancel(cpid);
						pthread_join(cpid,NULL);
						cpyes = 0;
						tc_end_child_thread();
						dc_end_child_thread();
						ms_end_child_thread();
						sc_end_child_thread();
						madc_end_child_thread();
						pps_end_child_thread();
						mih_end_child_thread();
						bus_end_child_thread();
						sa_end_child_thread();
						yft_end_child_thread();
						gt_end_child_thread();
						soa_end_child_thread();
                		soc_end_child_thread();
						sot_end_child_thread();
                		cd_end_child_thread();
						rcc_end_child_thread();
						oc_end_child_thread();
						bsa_end_child_thread();
						crc_end_child_thread();
						Inter_Control_End_Child_Thread();
						markbit &= 0xfffe;
						markbit &= 0xfeff;
					}// if (1 == cpyes)

					cst = 0;
                    st = 0;
                    nst = 0;
                    tv.tv_sec = 0;
                    tv.tv_usec = 0;
                    gettimeofday(&tv,NULL);
                    cst = tv.tv_sec;
					
					systime = (struct tm *)get_system_time();
					shour = timesection.TimeSectionList[i][0].StartHour;
					smin = timesection.TimeSectionList[i][0].StartMinute;
					if (0 != fanganID)//one key to green wave
					{
						systime->tm_hour = 0;
						systime->tm_min = 0;
					}
					else
					{
						systime->tm_hour = (long)shour;
                    	systime->tm_min = (long)smin;
					}
                    systime->tm_sec = 0;
                    st = (long)mktime(systime);

//					contmode = timesection.TimeSectionList[i][0].ControlMode;
					auxfunc = timesection.TimeSectionList[i][0].AuxFunc;
					halfdownt = timesection.TimeSectionList[i][0].SpecFunc & 0x0f;
					specfunc = timesection.TimeSectionList[i][0].SpecFunc;
					if (0 != fanganID)//one key to green wave
					{
						patternId = fanganID;
						retpattern = patternId;
						contmode = 0x04;
					}
					else
					{
						contmode = timesection.TimeSectionList[i][0].ControlMode;
						patternId = timesection.TimeSectionList[i][0].PatternId; 
						retpattern = patternId;
					}

                    coordtime = 0;
                    ncoordtime = 0;
                    poffset = 0;
                    npoffset = 0;
                    cyclet = 0;
                    ncyclet = 0;
                    coordphase = 0;
                    ncoordphase = 0;
                    tcid = 0;
                    ntcid = 0;
                    tempid = 0;
                    npidf = 0;
                    pidf = 0;
                    waittime = 0;
					nshour = 0;
                    nsmin = 0;
                    ncontmode = 0;
                    npatternId = 0;

					for (z = 0; z < pattern.FactPatternNum; z++)
					{
                       	if (0 == pattern.PatternList[z].PatternId)
                           	break;
                        if (patternId == pattern.PatternList[z].PatternId)
                        {
                            poffset = pattern.PatternList[z].PhaseOffset;
                            coordphase = pattern.PatternList[z].CoordPhase;
                            cyclet = pattern.PatternList[z].CycleTime;
							v2xcyct = cyclet; //V2X
                            tcid = pattern.PatternList[z].TimeConfigId;
                            break;
                        }
                    }
					if (0 != coordphase)
					{
						for (z = 0; z < timeconfig.FactTimeConfigNum; z++)
                        {
                            if (0 == timeconfig.TimeConfigList[z][0].TimeConfigId)
                                break;
                            if (tcid == timeconfig.TimeConfigList[z][0].TimeConfigId)
                            {
                                for (s = 0; s < timeconfig.FactStageNum; s++)
                                {
                                    if (0 == timeconfig.TimeConfigList[z][s].StageId)
                                        break;
                                    get_phase_id(timeconfig.TimeConfigList[z][s].PhaseId,&tempid);
                                    if (coordphase == tempid)
                                    {
                                        for (x = 0; x < s; x++)
                                        {
                                            coordtime += timeconfig.TimeConfigList[z][x].GreenTime \
                                                        + timeconfig.TimeConfigList[z][x].YellowTime \
                                                        + timeconfig.TimeConfigList[z][x].RedTime;
                                        }//for (x = 0; x < s; x++)
                                        break;
                                    }//if (coordphase == tempid)    
                                }//for (s = 0; s < timeconfig.FactStageNum; s++)
                                break;
                            }//if (tcid == timeconfig.TimeConfigList[z][0].TimeConfigId)
                        }//for (z = 0; z < timeconfig.FactTimeConfigNum; z++)
					}//if (0 != coordphase)
					if ((4 == contmode) && (0 != coordphase))
					{
						st += poffset;
						st -= coordtime;	
					}
					else if ((12 == contmode)&&(0 != coordphase)&&(!(auxfunc & 0x10))&&(!(auxfunc & 0x20)))
					{//area coordinate
						st += poffset;
                        st -= coordtime;
					}//area coordinate
					//added by sk on 20160322
					if (10 == contmode)
                    {
                        st += poffset;
                    //    st -= coordtime;
                    }
					//end add

					if (cst >= st)
					{
						//end it if yellow flash exists;
						if (1 == yfyes)
						{
							pthread_cancel(yfid);
							pthread_join(yfid,NULL);
							yfyes = 0;
						}
						//all red
                        new_all_red(&ardata);
					//	all_red(serial[0],0);

						//call fit control pattern according to contmode and patternid;
						if (0 == cpyes)
						{
							//add 20220803 清除相位降级信息
							//printf("start clean phase degrphid info ,File: %s,Line: %d\n",__FILE__,__LINE__);
							unsigned char				err1[10] = {0};
							
							int iIndex = 0;
							for(iIndex = 0; iIndex<MAX_PHASE_LINE;iIndex++)
							{
								if((phasedegrphid >> iIndex) & 0x00000001)
								{
									memset(err1,0,sizeof(err1));
									gettimeofday(&nowt,NULL);
									if (SUCCESS != err_report(err1,nowt.tv_sec,21,(82+iIndex)))
									{
										#ifdef MAIN_DEBUG
											printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
									}
									else
									{
										write(csockfd,err1,sizeof(err1));
									}
								}
							}
							phasedegrphid = 0;
							//printf("clean phase degrphid info end,File: %s,Line: %d\n",__FILE__,__LINE__);
							//add 20220803
							memset(&fcdata,0,sizeof(fcdata));
							fcdata.lip = &lip;
                            fcdata.patternid = &patternId;
							fcdata.cyclet = &cyclet;
							fcdata.coordphase = &coordphase;
							fcdata.slnum = &retslnum;
							fcdata.stageid = &retstageid;
							fcdata.phaseid = &retphaseid;
							fcdata.color = &retcolor;
                            fcdata.markbit = &markbit;
							fcdata.markbit2 = &markbit2;
							fcdata.wlmark = wlmark;
							fcdata.sockfd = &sockfd;
                            fcdata.flowpipe = &flowpipe[0];
							fcdata.pendpipe = &pendpipe[0];
                            fcdata.conpipe = &conpipe[0];
                            fcdata.synpipe = &synpipe[1];
							fcdata.endpipe = &endpipe[1];
							fcdata.ffpipe = &ffpipe[0];
                            fcdata.bbserial = &serial[0];
							fcdata.vhserial = &serial[1];
							fcdata.fbserial = &serial[2];
							fcdata.sdserial = &serial[4];
							fcdata.ec = &eventclass;
                            fcdata.el = &eventlog;

							fcdata.contmode = &contmode;
                            fcdata.ncontmode = &ncontmode;
							fcdata.auxfunc = &auxfunc;
							fcdata.reflags = &(unit.RemoteFlag);
							fcdata.halfdownt = &halfdownt;
							fcdata.specfunc = &specfunc;
							fcdata.roadinfo = &roadinfo;
							fcdata.roadinforeceivetime = &roadinforeceivetime;
							fcdata.phasedegrphid = & phasedegrphid;
//                            fcdata.shour = &shour;
//                            fcdata.smin = &smin;
//                            fcdata.coordtime = &coordtime;
//                            fcdata.poffset = &poffset;
//							fcdata.cyclet = &cyclet;
//							fcdata.coordphase = &coordphase;

//                            fcdata.nshour = &nshour;
//                            fcdata.nsmin = &nsmin;
//                            fcdata.ncoordtime = &ncoordtime;
//                          fcdata.npoffset = &npoffset;
//							fcdata.ncyclet = &ncyclet;
							fcdata.ncoordphase = &ncoordphase;
							fcdata.st = st;
							fcdata.nst = nst;
							fcdata.sinfo = &sinfo;
							fcdata.softevent = softevent;//added by shikang on 20171110
							fcdata.ccontrol = &ccontrol;//added by shikang on 20180118
							fcdata.fcontrol = &fcontrol;
							fcdata.trans = &trans;

							fcdata.v2xmark = &v2xmark; //V2X
							fcdata.slg = slg;
							int cpret = pthread_create(&cpid,NULL,(void *)run_control_pattern,&fcdata);
							if (0 != cpret)
							{
							#ifdef MAIN_DEBUG
								printf("Create thread err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
								return FAIL;
							}
							cpyes = 1;
						}
					
						systime->tm_hour = 23;
						systime->tm_min = 60;
						systime->tm_sec = 0;
						nst = (long)mktime(systime);
						waittime = (int)(nst - cst);

						mtime = waittime;
						updateyes = 0;
						while (1)
						{//inline 2 while loop
							FD_ZERO(&endnRead);
							FD_SET(endpipe[0],&endnRead);
							endmt.tv_sec = mtime;
							endmt.tv_usec = 0;
							lastt.tv_sec = 0;
							lastt.tv_usec = 0;
							gettimeofday(&lastt,NULL);
							int endret = select(endpipe[0]+1,&endnRead,NULL,NULL,&endmt);
							if (endret < 0)
							{
							#ifdef MAIN_DEBUG
								printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								return FAIL;
							} 
							if (0 == endret)
							{
							#ifdef MAIN_DEBUG
								printf("Timeout,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								if (1 == yfyes)
								{
									pthread_cancel(yfid);
									pthread_join(yfid,NULL);
									yfyes = 0;
								}
								break;
							}
							if (endret > 0)
							{
								if (FD_ISSET(endpipe[0],&endnRead))
								{
								    //if (FD_ISSET(endpipe[0],&endnRead))
									memset(enddata,0,sizeof(enddata));
									endnum = read(endpipe[0],enddata,sizeof(enddata));
									if (endnum > 0)
									{
										if (!strcmp(enddata,"UpdateFile"))
										{
											if (1 == yfyes)
                                   			{
                                       			pthread_cancel(yfid);
                                       			pthread_join(yfid,NULL);
                                       			yfyes = 0;
                                   			}
											markbit |= 0x0040;
											updateyes = 1;
											break;
										}
										else
										{
											nowt.tv_sec = 0;
											nowt.tv_usec = 0;
											gettimeofday(&nowt,NULL);
											leat = nowt.tv_sec - lastt.tv_sec;
											mtime -= leat;
											continue;
										}
									}//endnum > 0
									else
									{
									     //endnum <=0
										nowt.tv_sec = 0;
										nowt.tv_usec = 0;
										gettimeofday(&nowt,NULL);
										leat = nowt.tv_sec - lastt.tv_sec;
										mtime -= leat;
										continue;
									}//endnum <=0	
								}//if (FD_ISSET(endpipe[0],&endnRead))
								else
								{
								    // Not 'if (FD_ISSET(endpipe[0],&endnRead))'
									nowt.tv_sec = 0;
									nowt.tv_usec = 0;
									gettimeofday(&nowt,NULL);
									leat = nowt.tv_sec - lastt.tv_sec;
									mtime -= leat;
									continue;
								}//Not 'if (FD_ISSET(endpipe[0],&endnRead))'
							}//endret > 0
						}//inline 2 while loop
						markbit |= 0x0001;
						if (1 == updateyes)
						{
							break;
						}					
					}// if ((chour > shour) || ((chour == shour) && (cmin >= smin)))
					else
					{//current time do not have fit pattern;
						//all red
                        new_all_red(&ardata);
					//	all_red(serial[0],0);					
	
						//yellow flash
						if (0 == yfyes)
						{
#ifdef FLASH_DEBUG
							char szInfo[32] = {0};
							char szInfoT[64] = {0};
							snprintf(szInfo,sizeof(szInfo)-1,"%s",__FUNCTION__);
							snprintf(szInfoT,sizeof(szInfoT)-1,"%d",__LINE__);
							tsc_save_eventlog(szInfo,szInfoT);
#endif
							int yfret = pthread_create(&yfid,NULL,(void *)err_yellow_flash,&yfdata);
							if (0 != yfret)
							{
							#ifdef MAIN_DEBUG
								printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								return FAIL;
							}	
							yfyes = 1;
							update_event_list(&eventclass,&eventlog,1,10,tt.tv_sec,&markbit);
							output_log("time is before");
						}
						waittime = st - cst;
						mtime = waittime;
						updateyes = 0;
						while (1)
						{
						    //inline 2 while loop
							FD_ZERO(&endnRead);
							FD_SET(endpipe[0],&endnRead);
							endmt.tv_sec = mtime;
							endmt.tv_usec = 0;
							lastt.tv_sec = 0;
							lastt.tv_usec = 0;
							gettimeofday(&lastt,NULL);
							int endret = select(endpipe[0]+1,&endnRead,NULL,NULL,&endmt);
							if (endret < 0)
							{
							#ifdef MAIN_DEBUG
								printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								return FAIL;
							} 
							if (0 == endret)
							{
							#ifdef MAIN_DEBUG
								printf("Timeout,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								if (1 == yfyes)
								{
									pthread_cancel(yfid);
									pthread_join(yfid,NULL);
									yfyes = 0;
								}
								break;
							}
							if (endret > 0)
							{
								if (FD_ISSET(endpipe[0],&endnRead))
								{//if (FD_ISSET(endpipe[0],&endnRead))
									memset(enddata,0,sizeof(enddata));
									endnum = read(endpipe[0],enddata,sizeof(enddata));
									if (endnum > 0)
									{
										if (!strcmp(enddata,"UpdateFile"))
										{
											if (1 == yfyes)
                                   			{
                                       			pthread_cancel(yfid);
                                       			pthread_join(yfid,NULL);
                                       			yfyes = 0;
                                   			}
											markbit |= 0x0040;
											updateyes = 1;
											break;
										}
										else
										{
											nowt.tv_sec = 0;
											nowt.tv_usec = 0;
											gettimeofday(&nowt,NULL);
											leat = nowt.tv_sec - lastt.tv_sec;
											mtime -= leat;
											continue;
										}
									}//endnum > 0
									else
									{//endnum <=0
										nowt.tv_sec = 0;
										nowt.tv_usec = 0;
										gettimeofday(&nowt,NULL);
										leat = nowt.tv_sec - lastt.tv_sec;
										mtime -= leat;
										continue;
									}//endnum <=0	
								}//if (FD_ISSET(endpipe[0],&endnRead))
								else
								{// Not 'if (FD_ISSET(endpipe[0],&endnRead))'
									nowt.tv_sec = 0;
									nowt.tv_usec = 0;
									gettimeofday(&nowt,NULL);
									leat = nowt.tv_sec - lastt.tv_sec;
									mtime -= leat;
									continue;
								}//Not 'if (FD_ISSET(endpipe[0],&endnRead))'
							}//endret > 0
						}//inline 2 while loop
						if (1 == updateyes)
						{
							cpyes = 0;
							break;
						}						
						//all red
                        new_all_red(&ardata);
					//	all_red(serial[0],0);
						//call fit control pattern according to contmode and patternid;
						if (0 == cpyes)
                        {
							//add 20220803 清除相位降级信息
							//printf("start clean phase degrphid info ,File: %s,Line: %d\n",__FILE__,__LINE__);
							unsigned char				err1[10] = {0};
							int iIndex = 0;
							for(iIndex = 0; iIndex<MAX_PHASE_LINE;iIndex++)
							{
								if((phasedegrphid >> iIndex) & 0x00000001)
								{
									memset(err1,0,sizeof(err1));
									gettimeofday(&nowt,NULL);
									if (SUCCESS != err_report(err1,nowt.tv_sec,21,(82+iIndex)))
									{
											#ifdef MAIN_DEBUG
											printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
									}
									else
									{
										write(csockfd,err1,sizeof(err1));
									}
								}
							}
							phasedegrphid = 0;
							//printf("clean phase degrphid info end,File: %s,Line: %d\n",__FILE__,__LINE__);
							//add 20220803
                            memset(&fcdata,0,sizeof(fcdata));
							fcdata.lip = &lip;
                            fcdata.patternid = &patternId;
						    fcdata.cyclet = &cyclet;
							fcdata.coordphase = &coordphase;
							fcdata.slnum = &retslnum;
							fcdata.stageid = &retstageid;
							fcdata.phaseid = &retphaseid;
							fcdata.color = &retcolor; 
							fcdata.markbit = &markbit;
							fcdata.markbit2 = &markbit2;
							fcdata.wlmark = wlmark;	
							fcdata.sockfd = &sockfd;
							fcdata.flowpipe = &flowpipe[0];
							fcdata.pendpipe = &pendpipe[0];
							fcdata.conpipe = &conpipe[0];
							fcdata.synpipe = &synpipe[1];
							fcdata.endpipe = &endpipe[1];
							fcdata.ffpipe = &ffpipe[0];
							fcdata.bbserial = &serial[0];
							fcdata.vhserial = &serial[1];
							fcdata.fbserial = &serial[2];
							fcdata.sdserial = &serial[4];
							fcdata.ec = &eventclass;
							fcdata.el = &eventlog;

							fcdata.contmode = &contmode;
                            fcdata.ncontmode = &ncontmode;
							fcdata.auxfunc = &auxfunc;
							fcdata.reflags = &(unit.RemoteFlag);
							fcdata.halfdownt = &halfdownt;
							fcdata.specfunc = &specfunc;	
							fcdata.roadinfo = &roadinfo;
							fcdata.roadinforeceivetime = &roadinforeceivetime;
							fcdata.phasedegrphid = & phasedegrphid;
//                            fcdata.shour = &shour;
//                            fcdata.smin = &smin;
//                            fcdata.coordtime = &coordtime;
//                            fcdata.poffset = &poffset;
//							fcdata.cyclet = &cyclet;
//							fcdata.coordphase = &coordphase;

//                            fcdata.nshour = &nshour;
//                            fcdata.nsmin = &nsmin;
//                            fcdata.ncoordtime = &ncoordtime;
//                            fcdata.npoffset = &npoffset;
//							fcdata.ncyclet = &ncyclet;
							fcdata.ncoordphase = &ncoordphase;
							fcdata.st = st;
							fcdata.nst = nst;
							fcdata.sinfo = &sinfo;
							fcdata.softevent = softevent;//added by shikang on 20171110
							fcdata.ccontrol = &ccontrol;//added by shikang on 20180118
							fcdata.fcontrol = &fcontrol;
							fcdata.trans = &trans;

							fcdata.v2xmark = &v2xmark; //V2X
							fcdata.slg = slg;
                            int cpret = pthread_create(&cpid,NULL,(void *)run_control_pattern,&fcdata);
                            if (0 != cpret)
                            {
                            #ifdef MAIN_DEBUG
                                printf("Create thread err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            #endif
                                return FAIL;
                            }
                            cpyes = 1;
                        }
						systime->tm_hour = 23;
                        systime->tm_min = 60;
                        systime->tm_sec = 0;
                        nst = (long)mktime(systime);
						waittime = (int)(nst - st);
						mtime = waittime;
						updateyes = 0;
						while (1)
						{//inline 2 while loop
							FD_ZERO(&endnRead);
							FD_SET(endpipe[0],&endnRead);
							endmt.tv_sec = mtime;
							endmt.tv_usec = 0;
							lastt.tv_sec = 0;
							lastt.tv_usec = 0;
							gettimeofday(&lastt,NULL);
							int endret = select(endpipe[0]+1,&endnRead,NULL,NULL,&endmt);
							if (endret < 0)
							{
							#ifdef MAIN_DEBUG
								printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								return FAIL;
							} 
							if (0 == endret)
							{
							#ifdef MAIN_DEBUG
								printf("Timeout,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								if (1 == yfyes)
								{
									pthread_cancel(yfid);
									pthread_join(yfid,NULL);
									yfyes = 0;
								}
								break;
							}
							if (endret > 0)
							{
								if (FD_ISSET(endpipe[0],&endnRead))
								{//if (FD_ISSET(endpipe[0],&endnRead))
									memset(enddata,0,sizeof(enddata));
									endnum = read(endpipe[0],enddata,sizeof(enddata));
									if (endnum > 0)
									{
										if (!strcmp(enddata,"UpdateFile"))
										{
											if (1 == yfyes)
                                   			{
                                       			pthread_cancel(yfid);
                                       			pthread_join(yfid,NULL);
                                       			yfyes = 0;
                                   			}
											markbit |= 0x0040;
											updateyes = 1;
											break;
										}
										else
										{
											nowt.tv_sec = 0;
											nowt.tv_usec = 0;
											gettimeofday(&nowt,NULL);
											leat = nowt.tv_sec - lastt.tv_sec;
											mtime -= leat;
											continue;
										}
									}//endnum > 0
									else
									{//endnum <=0
										nowt.tv_sec = 0;
										nowt.tv_usec = 0;
										gettimeofday(&nowt,NULL);
										leat = nowt.tv_sec - lastt.tv_sec;
										mtime -= leat;
										continue;
									}//endnum <=0	
								}//if (FD_ISSET(endpipe[0],&endnRead))
								else
								{// Not 'if (FD_ISSET(endpipe[0],&endnRead))'
									nowt.tv_sec = 0;
									nowt.tv_usec = 0;
									gettimeofday(&nowt,NULL);
									leat = nowt.tv_sec - lastt.tv_sec;
									mtime -= leat;
									continue;
								}//Not 'if (FD_ISSET(endpipe[0],&endnRead))'
							}//endret > 0
						}//inline 2 while loop
						markbit |= 0x0001;
						if (1 == updateyes)
						{
							break;
						}
					}//current time do not have fit pattern;
					break;//come into 1th while loop again
				}// if (1 == eventnum)
			}//if (tsid == timesection.TimeSectionList[i][0].TimeSectionId)
		}//for (i = 0; i < timesection.FactTimeSectionNum; i++)

		if ((0 == eventnum) || (0 == patternId))
		{
		#ifdef MAIN_DEBUG
			printf("Not have fit pattern,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			update_event_list(&eventclass,&eventlog,1,11,tt.tv_sec,&markbit);
			if ((!(markbit & 0x1000)) && (!(markbit & 0x8000)))
    		{//actively report is not probitted and connect successfully
        		memset(err,0,sizeof(err));
        		if (SUCCESS != err_report(err,tt.tv_sec,1,11))
        		{
        		#ifdef MAIN_DEBUG
            		printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
        		#endif
        		}
				else
				{
        			write(csockfd,err,sizeof(err));
				}
    		}
			//Firstly,all red
        //    new_all_red(&ardata);
		//	all_red(serial[0],0);

			//yellow flash
			if (0 == yfyes)
			{
#ifdef FLASH_DEBUG
				char szInfo[32] = {0};
				char szInfoT[64] = {0};
				snprintf(szInfo,sizeof(szInfo)-1,"%s",__FUNCTION__);
				snprintf(szInfoT,sizeof(szInfoT)-1,"%d",__LINE__);
				tsc_save_eventlog(szInfo,szInfoT);
#endif
				int yfret = pthread_create(&yfid,NULL,(void *)err_yellow_flash,&yfdata);
				if (0 != yfret)
				{
				#ifdef MAIN_DEBUG
					printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					return FAIL;
				}
				yfyes = 1;
			}
			sleep(10);
			continue;
		}//if ((0 == eventnum) || (0 == patternId))	
	}//1th while loop

	return 0;
}
