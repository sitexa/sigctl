/*
	File: 		igc.c
	Author: 	bpf
	Created on 20220804
*/
#include "igcs.h"

static struct timeval		gtime,gftime,ytime,rtime;
static unsigned char		InterGratedControlYes = 0;
static pthread_t			InterGratedControlPid;
static unsigned char		YellowFlashYes = 0;
static pthread_t			YellowFlashPid;
static unsigned char		rettl = 0;
static PhaseDetect_t		stPhaseDetect[MAX_PHASE_LINE] = {0};
static PhaseDetect_t		*pstPhaseDetect = NULL;
static DetectOrPro_t		*stDetectOrPro = NULL;
static statusinfo_t          stStatuSinfo;
static unsigned char		 phcon = 0;
static unsigned char         bAllLampClose  =0;
static unsigned char         friststartcontrol = 0;


//处理车道冲突
static unsigned char		CheckLaneIdYes = 0;
static pthread_t			CheckLaneIdPid;
static unsigned char        bLaneDirectionExitDanger = 0;
static int					noticepipe[2] = {0};


int Inter_Control_Yellow_Time_Wait(unsigned char *cLeftTime,unsigned char *noticenumer,unsigned char *bselecterror)
{
	struct timeval      stWaitTime,stCurrentTime,stLastTime;
	fd_set				nRead;
	unsigned char       readnum = 0;
	unsigned char		noticebuf[10] = {0};

	if((cLeftTime == NULL) || (noticenumer == NULL) || (bselecterror == NULL))
	{
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
		return MEMERR;
	}
	while (1)
	{
		FD_ZERO(&nRead);
		FD_SET(noticepipe[0],&nRead);
		stWaitTime.tv_sec = *cLeftTime;
		stWaitTime.tv_usec = 0;
		
		stLastTime.tv_sec = 0;
		stLastTime.tv_usec = 0;
		gettimeofday(&stLastTime,NULL);
		int fret = select(noticepipe[0]+1,&nRead,NULL,NULL,&stWaitTime);
		if (fret < 0)
		{
			#ifdef INTER_CONTROL_DEBUG
				printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			*bselecterror = 1;
			stCurrentTime.tv_sec = 0;
			stCurrentTime.tv_usec = 0;
			gettimeofday(&stCurrentTime,NULL);
			*cLeftTime = *cLeftTime - (stCurrentTime.tv_sec - stLastTime.tv_sec);
			break;
		}
		if(fret == 0)
		{
			*noticenumer = 0;
			break;
		}
		if(fret > 0)
		{
			if(FD_ISSET(noticepipe[0],&nRead))
			{
				memset(noticebuf,0,sizeof(noticebuf));
				readnum = 0;
				readnum = read(noticepipe[0],noticebuf,sizeof(noticebuf));
				if(readnum > 0)
				{
					if (!strncmp("danger",noticebuf,6))
					{
						*noticenumer++;
						stCurrentTime.tv_sec = 0;
						stCurrentTime.tv_usec = 0;
						gettimeofday(&stCurrentTime,NULL);
						*cLeftTime = *cLeftTime - (stCurrentTime.tv_sec - stLastTime.tv_sec);
						printf("current is last phase and receive danger num : %d info,File: %s,Line: %d\n",*noticenumer,__FILE__,__LINE__);
						continue;
					}
				}
			}
		}
	}
	return SUCCESS;
}

void Inter_Control_Close_All_Lamp(InterGrateControlData_t *stInterGrateControlData)
{
	InterGrateControlPhaseInfo_t  *pstInterPhaseData = stInterGrateControlData->pi;
	statusinfo_t 				stSinfo;
	unsigned char				num = 0;
	struct timeval          to;
	
	unsigned char	 sibuf[64] = {0};
	unsigned char    tcsta = 0;
	unsigned char    *csta = NULL;
	unsigned char    i =0;

	yfdata_t				acdata;
	memset(&acdata,0,sizeof(acdata));
	acdata.second = 0;
	acdata.markbit = stInterGrateControlData->fd->markbit;
	acdata.markbit2 = stInterGrateControlData->fd->markbit2;
	acdata.serial = *(stInterGrateControlData->fd->bbserial);
	acdata.sockfd = stInterGrateControlData->fd->sockfd;
	acdata.cs = stInterGrateControlData->cs;

	memset(&stSinfo,0,sizeof(statusinfo_t));

	new_all_close(&acdata);
	if ((!(*(stInterGrateControlData->fd->markbit) & 0x1000)) && (!(*(stInterGrateControlData->fd->markbit) & 0x8000)))
	{//report info to server actively
		stSinfo.conmode = *(stInterGrateControlData->fd->contmode);
		stSinfo.color = 0x04;
		stSinfo.time = 0;
		stSinfo.stage = 0;
		stSinfo.cyclet = 0;
		stSinfo.phase = 0;

		stSinfo.chans = 0;
		memset(stSinfo.csta,0,sizeof(stSinfo.csta));
		csta = stSinfo.csta;
		for (i = 0; i < MAX_CHANNEL; i++)
		{
			stSinfo.chans += 1;
			tcsta = i+1;
			tcsta <<= 2;
			tcsta &= 0xfc;
			*csta = tcsta;
			csta++;
		}
		memcpy(stInterGrateControlData->fd->sinfo,&stSinfo,sizeof(statusinfo_t));						
		memset(sibuf,0,sizeof(sibuf));
		if (SUCCESS != status_info_report(sibuf,&stSinfo))
		{
		#ifdef INTER_CONTROL_DEBUG
			printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
		#endif
		}
		else
		{
			write(*(stInterGrateControlData->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
		}
	}


}


void Inter_Control_Check_Lane_Id(void *arg)
{

	InterGrateControlData_t 			*stInterGrateControlData = arg;
	fd_set				nRead;
	unsigned char		buf[512] = {0};
	unsigned char		*pbuf = buf;
	unsigned short		mark = 0;	
	unsigned short		num = 0;
	unsigned char		i = 0, j = 0;
	unsigned char		cCurrentLaneID = 0;//车道编号;
	unsigned char		cLastLaneID = 0;//车道编号;
	unsigned char		cbCurrentEvenNumbers = 0;//奇偶数;
	unsigned char		cbLastEvenNumbers = 0;//奇偶数;
	unsigned int		cLeftTime = 1000000;
	struct timeval      stWaitTime,stCurrentTime,stLastTime;
			

	while (1)////main loop
	{
		if(1 == bLaneDirectionExitDanger)//存在一次冲突，等待开灯
		{
			sleep(5);
		}
		FD_ZERO(&nRead);
		FD_SET(*(stInterGrateControlData->fd->pendpipe),&nRead);
		cLastLaneID = 0;
		cbLastEvenNumbers = 0;
		cbCurrentEvenNumbers = 0;
		cCurrentLaneID = 0;
		bLaneDirectionExitDanger = 0;
		int fret = select(*(stInterGrateControlData->fd->pendpipe)+1,&nRead,NULL,NULL,NULL);
		if (fret < 0)
		{
			#ifdef INTER_CONTROL_DEBUG
				printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			return;
		}
		if (fret > 0)	
		{
			if (FD_ISSET(*(stInterGrateControlData->fd->pendpipe),&nRead))
			{
				memset(buf,0,sizeof(buf));
				num = read(*(stInterGrateControlData->fd->pendpipe),buf,sizeof(buf));	
				if (num > sizeof(buf))
                    continue;
				if (num > 0)
				{				
					pbuf = buf;
					mark = 0;
					while (1)//loop 1
					{
						if (mark >= num)
						{
							break;// jump loop 1
						}
							
						if(bLaneDirectionExitDanger == 1)
						{
							break;// jump loop 1
						}
						if ((0xF1 == *(pbuf+mark)) && (0xED == *(pbuf+mark+5)))
						{
							if (1 == *(pbuf+mark+1))//车辆进入
							{
								cLastLaneID = *(pbuf+mark+2);
								cbLastEvenNumbers = cLastLaneID % 2;
								//printf("igc.c frist second : %d , 22222 : %d \n",stLastTime.tv_sec,stLastTime.tv_usec);
								while(1)//loop 2
								{
									FD_ZERO(&nRead);
									FD_SET(*(stInterGrateControlData->fd->pendpipe),&nRead);
									cLeftTime = 1000000;
									stWaitTime.tv_sec = 0;
									stWaitTime.tv_usec = cLeftTime;
									stLastTime.tv_sec = 0;
									stLastTime.tv_usec = 0;
									gettimeofday(&stLastTime,NULL);
									int ret = select(*(stInterGrateControlData->fd->pendpipe)+1,&nRead,NULL,NULL,&stWaitTime);
									if(ret < 0)//select error
									{
										#ifdef INTER_CONTROL_DEBUG
										printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										return ;
									}
									if(ret == 0)//单位时间内其他方向无车辆经过
									{
										bLaneDirectionExitDanger = 0;
										break;
									}
									if(ret > 0)
									{
										if(FD_ISSET(*(stInterGrateControlData->fd->pendpipe),&nRead))
										{
											memset(buf,0,sizeof(buf));
											num = 0;
											num = read(*(stInterGrateControlData->fd->pendpipe),buf,sizeof(buf));
											if(num > 0)
											{
												pbuf = buf;
												mark = 0;
												while(1)//loop 3
												{
													if (mark >= num)
													{
														break;
													}
													if ((0xF1 == *(pbuf+mark)) && (0xED == *(pbuf+mark+5)))
													{
														
														if(1 == *(pbuf+mark+1))//车辆进入
														{
															cCurrentLaneID = *(pbuf+mark+2);
															cbCurrentEvenNumbers = cCurrentLaneID % 2;
															stCurrentTime.tv_sec = 0;
															stCurrentTime.tv_usec = 0;
															gettimeofday(&stCurrentTime,NULL);
															if(cbCurrentEvenNumbers == cbLastEvenNumbers)
															{
																cLeftTime = cLeftTime - (stCurrentTime.tv_usec - stLastTime.tv_usec);
																bLaneDirectionExitDanger = 0;
																break;// jump loop 3
															}
															else
															{
																bLaneDirectionExitDanger = 1;
																//printf("start to open lamp \n");
																unsigned char			cdata[10] = {0}; //buf for clean pipe 
																while (1)
																{	
																	memset(cdata,0,sizeof(cdata));
																	if (read(noticepipe[0],cdata,sizeof(cdata)) <= 0)
																		break;
																}
																//end clean pipe

																if (!wait_write_serial(noticepipe[1]))
																{
																	if (write(noticepipe[1],"danger",strlen("danger")) < 0)
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
																break;// jump loop 3
															}
														}
														mark+=6;
														continue;
													}
													else
													{
														mark +=1;
														continue;
													}
													
												}// loop 3
												if(bLaneDirectionExitDanger == 1)
												{
													break;// jump loop 2
												}
//												stCurrentTime.tv_sec = 0;
//												stCurrentTime.tv_usec = 0;
//												gettimeofday(&stCurrentTime,NULL);
//												cLeftTime = cLeftTime - (stCurrentTime.tv_usec - stLastTime.tv_usec);
												if(cLeftTime < 1000000)
												{
													continue;
												}
												else
												{
													break;
												}
												
											}
											else
											{
												break;
											}
										}
									}
								}//loop 2
							}
							if(bLaneDirectionExitDanger == 1)
							{
								break;
							}
							mark += 6;
							continue;
						}
						else
						{
							mark += 1;
							continue;
						}
					}//loop 1
					continue;
				}
				else
				{
					continue;
				}
			}
			else
			{
				continue;
			}
		}
	}//main loop

	pthread_exit(NULL);
}


void Inter_Control_End_Child_Thread()
{

    if (1 == CheckLaneIdYes)
    {
        pthread_cancel(CheckLaneIdPid);
        pthread_join(CheckLaneIdPid,NULL);
        CheckLaneIdYes = 0;
    }


    if (1 == YellowFlashYes)
    {
        pthread_cancel(YellowFlashPid);
        pthread_join(YellowFlashPid,NULL);
        YellowFlashYes = 0;
    }

	//主线程
    if (1 == InterGratedControlYes)
    {
        pthread_cancel(InterGratedControlPid);
        pthread_join(InterGratedControlPid,NULL);
        InterGratedControlYes = 0;
    }
}

void Inter_Control_End_Part_Child_Thread()
{
	
    if (1 == CheckLaneIdYes)
    {
        pthread_cancel(CheckLaneIdPid);
        pthread_join(CheckLaneIdPid,NULL);
        CheckLaneIdYes = 0;
    }

	if (1 == YellowFlashYes)
    {
        pthread_cancel(YellowFlashPid);
        pthread_join(YellowFlashPid,NULL);
        YellowFlashYes = 0;
    }
}

void Inter_Control_Yellow_Flash(void *arg)
{
	yfdata_t			*yfdata = arg;
	new_yellow_flash(yfdata);
	pthread_exit(NULL);
}


int Inter_Control_Clear_Down_Time(InterGrateControlData_t *stInterGrateControlData)
{
	unsigned char           ClearDownTime[8] = {0xA6,0xff,0xff,0xff,0xff,0x03,0xff,0xED};
	unsigned char           ClearEndDownTime[3] = {0xA5,0xA5,0xED};
	if(NULL == stInterGrateControlData)
	{
		#ifdef INTER_CONTROL_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		return MEMERR;
	}
	//清除倒计时
	if (!wait_write_serial(*(stInterGrateControlData->fd->bbserial)))
    {
    	if (write(*(stInterGrateControlData->fd->bbserial),ClearDownTime,sizeof(ClearDownTime)) < 0)
        {
		#ifdef INTER_CONTROL_DEBUG
			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif       
        }
    }
    else
    {
	    #ifdef INTER_CONTROL_DEBUG
	    	printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
	    #endif
		return FAIL;
    }
    if (!wait_write_serial(*(stInterGrateControlData->fd->bbserial)))
    {
    	if (write(*(stInterGrateControlData->fd->bbserial),ClearEndDownTime,sizeof(ClearEndDownTime)) < 0)
        {
	        #ifdef INTER_CONTROL_DEBUG
	        	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
	        #endif
        }
    }
    else
    {
	    #ifdef INTER_CONTROL_DEBUG
	    	printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
	    #endif
		return FAIL;
    }
	return SUCCESS;
	//清除倒计时
}



void Start_Inter_Grate_Control(void *arg)
{
	InterGrateControlData_t				*stInterGrateControlData = arg;
	unsigned char			tcline = 0;
	unsigned char			snum = 0; //max number of stage 
	unsigned char			slnum = 0;
	int						i = 0,j = 0,z = 0,k = 0,s = 0;
	unsigned char			tphid = 0;
	timedown_t				timedown;

	unsigned char			tnum = 0;
	unsigned int			tchans = 0;
	unsigned char			tclc1 = 0;
	unsigned char           downti[8] = {0};
	unsigned char           edownti[3] = {0};
	unsigned char           noticenumer = 0;
	unsigned char           bselecterror = 0;

	Inter_Control_Clear_Down_Time(stInterGrateControlData);
	//获取阶段配置表中阶段配时号
	if (SUCCESS != igc_get_timeconfig(stInterGrateControlData->td,stInterGrateControlData->fd->patternid,&tcline))
	{
		#ifdef INTER_CONTROL_DEBUG
			printf("igc_get_timeconfig call err,File: %s,Line: %d\n",__FILE__,__LINE__);
			output_log("InterGrate control,get timeconfig err");
		#endif
		Inter_Control_End_Part_Child_Thread();
		//return;
		
		struct timeval				time;
		unsigned char				yferr[10] = {0};
		gettimeofday(&time,NULL);
		update_event_list(stInterGrateControlData->fd->ec,stInterGrateControlData->fd->el,1,11,time.tv_sec,stInterGrateControlData->fd->markbit);
		if ((!(*(stInterGrateControlData->fd->markbit) & 0x1000)) && (!(*(stInterGrateControlData->fd->markbit) & 0x8000)))
		{
			memset(yferr,0,sizeof(yferr));
			if (SUCCESS != err_report(yferr,time.tv_sec,1,11))
			{
				#ifdef TIMING_DEBUG
				printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
			}
			else
			{
				write(*(stInterGrateControlData->fd->sockfd->csockfd),yferr,sizeof(yferr));
			}
		}

		yfdata_t					yfdata;
		if (0 == YellowFlashYes)
		{
			memset(&yfdata,0,sizeof(yfdata));
			yfdata.second = 0;
			yfdata.markbit = stInterGrateControlData->fd->markbit;
			yfdata.markbit2 = stInterGrateControlData->fd->markbit2;
			yfdata.serial = *(stInterGrateControlData->fd->bbserial);
			yfdata.sockfd = stInterGrateControlData->fd->sockfd;
			yfdata.cs = stInterGrateControlData->cs;		
#ifdef FLASH_DEBUG
			char szInfo[32] = {0};
			char szInfoT[64] = {0};
			snprintf(szInfo,sizeof(szInfo)-1,"%s",__FUNCTION__);
			snprintf(szInfoT,sizeof(szInfoT)-1,"%d",__LINE__);
			tsc_save_eventlog(szInfo,szInfoT);
#endif
			int yfret = pthread_create(&YellowFlashPid,NULL,(void *)Inter_Control_Yellow_Flash,&yfdata);
			if (0 != yfret)
			{
				#ifdef TIMING_DEBUG
				printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
				output_log("Inter_Control_Yellow_Flash control,create yellow flash err");
				#endif
				Inter_Control_End_Part_Child_Thread();
				return;
			}
			YellowFlashYes = 1;
		}		
		while(1)
		{
			if (*(stInterGrateControlData->fd->markbit) & 0x0001)
			{//end time of current pattern has arrived
				unsigned char           enddata[3] = {0xCC,0xDD,0xED};
				if (!wait_write_serial(*(stInterGrateControlData->fd->synpipe)))
				{
					if (write(*(stInterGrateControlData->fd->synpipe),enddata,sizeof(enddata)) < 0)
					{
						#ifdef FULL_DETECT_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						Inter_Control_End_Part_Child_Thread();
						return;
					}
				}
				else
				{
					#ifdef FULL_DETECT_DEBUG
					printf("pipe cannot be written,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					Inter_Control_End_Part_Child_Thread();
					return;
				}
				sleep(5);//wait main module end own;	
			}//end time of current pattern has arrived
			sleep(10);
			continue;
		}
	}
	//获取阶段配置表中阶段配时号

	
	rettl = tcline;
	memset(stPhaseDetect,0,sizeof(stPhaseDetect));
	pstPhaseDetect = stPhaseDetect;
	j = 0;

	//获取相位信息
	for (snum = 0; ;snum++)
	{//for (snum = 0; ;snum++)
		if (0 == stInterGrateControlData->td->timeconfig->TimeConfigList[tcline][snum].StageId)
			break;
		pstPhaseDetect->stageid = stInterGrateControlData->td->timeconfig->TimeConfigList[tcline][snum].StageId;
		get_phase_id(stInterGrateControlData->td->timeconfig->TimeConfigList[tcline][snum].PhaseId,&tphid);
		pstPhaseDetect->phaseid = tphid;
		pstPhaseDetect->gtime = stInterGrateControlData->td->timeconfig->TimeConfigList[tcline][snum].GreenTime;
		pstPhaseDetect->ytime = stInterGrateControlData->td->timeconfig->TimeConfigList[tcline][snum].YellowTime;
        pstPhaseDetect->rtime = stInterGrateControlData->td->timeconfig->TimeConfigList[tcline][snum].RedTime;

		//get channls of phase 
        for (i = 0; i < (stInterGrateControlData->td->channel->FactChannelNum); i++)
        {
            if (0 == (stInterGrateControlData->td->channel->ChannelList[i].ChannelId))
                break;
            if ((pstPhaseDetect->phaseid) == (stInterGrateControlData->td->channel->ChannelList[i].ChannelCtrlSrc))
            {
				#ifdef CLOSE_LAMP
                tclc1 = stInterGrateControlData->td->channel->ChannelList[i].ChannelId;
                if ((tclc1 >= 0x05) && (tclc1 <= 0x0c))
                {
                    if (*(stInterGrateControlData->fd->specfunc) & (0x01 << (tclc1 - 5)))
                        continue;
                }
				#else
				if ((*(stInterGrateControlData->fd->specfunc)&0x10)&&(*(stInterGrateControlData->fd->specfunc)&0x20))
				{
					tclc1 = stInterGrateControlData->td->channel->ChannelList[i].ChannelId;
					if(((5<=tclc1)&&(tclc1<=8)) || ((9<=tclc1)&&(tclc1<=12)))
						continue;
				}
				if ((*(stInterGrateControlData->fd->specfunc)&0x10)&&(!(*(stInterGrateControlData->fd->specfunc)&0x20)))
				{
					tclc1 = stInterGrateControlData->td->channel->ChannelList[i].ChannelId;
					if ((5 <= tclc1) && (tclc1 <= 8))
						continue;
				}
				if ((*(stInterGrateControlData->fd->specfunc)&0x20)&&(!(*(stInterGrateControlData->fd->specfunc)&0x10)))
				{
					tclc1 = stInterGrateControlData->td->channel->ChannelList[i].ChannelId;
					if ((9 <= tclc1) && (tclc1 <= 12))
						continue;
				}
                #endif
                pstPhaseDetect->chans |= (0x00000001 << ((stInterGrateControlData->td->channel->ChannelList[i].ChannelId) - 1));
                continue;
            }
        }
		for (i = 0; i < (stInterGrateControlData->td->phase->FactPhaseNum); i++)
		{//for (i = 0; i < (stInterGrateControlData->td->phase->FactPhaseNum); i++)
			if (0 == (stInterGrateControlData->td->phase->PhaseList[i].PhaseId))
				break;
			if (tphid == (stInterGrateControlData->td->phase->PhaseList[i].PhaseId))
			{
			
				pstPhaseDetect->phasetype = stInterGrateControlData->td->phase->PhaseList[i].PhaseType;				
                pstPhaseDetect->fixgtime = stInterGrateControlData->td->phase->PhaseList[i].PhaseFixGreen;
                pstPhaseDetect->mingtime = stInterGrateControlData->td->phase->PhaseList[i].PhaseMinGreen;
                pstPhaseDetect->maxgtime = stInterGrateControlData->td->phase->PhaseList[i].PhaseMaxGreen1;
                pstPhaseDetect->gftime = stInterGrateControlData->td->phase->PhaseList[i].PhaseGreenFlash;
                pstPhaseDetect->gtime -= pstPhaseDetect->gftime;//green time is not include green flash
                pstPhaseDetect->pgtime = stInterGrateControlData->td->phase->PhaseList[i].PhaseWalkGreen;
                pstPhaseDetect->pctime = stInterGrateControlData->td->phase->PhaseList[i].PhaseWalkClear;

				tnum = stInterGrateControlData->td->phase->PhaseList[i].PhaseSpecFunc;
				tnum &= 0xe0;//get 5~7bit
				tnum >>= 5;
				tnum &= 0x07; 
				pstPhaseDetect->indetenum = tnum;
				pstPhaseDetect->validmark = 1;
				pstPhaseDetect->factnum = 0;
				memset(pstPhaseDetect->indetect,0,sizeof(pstPhaseDetect->indetect));
				memset(pstPhaseDetect->detectpro,0,sizeof(pstPhaseDetect->detectpro));
				stDetectOrPro = pstPhaseDetect->detectpro;
				for (z = 0; z < (stInterGrateControlData->td->detector->FactDetectorNum); z++)
				{//1
					if (0 == (stInterGrateControlData->td->detector->DetectorList[z].DetectorId))
						break;
					if ((pstPhaseDetect->phaseid) == (stInterGrateControlData->td->detector->DetectorList[z].DetectorPhase))
					{
						stDetectOrPro->deteid = stInterGrateControlData->td->detector->DetectorList[z].DetectorId;
						stDetectOrPro->validmark = 0;
						stDetectOrPro->err = 0;
						tnum = stInterGrateControlData->td->detector->DetectorList[z].DetectorSpecFunc;
						tnum &= 0xfc;//get 2~7bit
						tnum >>= 2;
						tnum &= 0x3f;
						stDetectOrPro->intime = tnum * 60;
						memset(&(stDetectOrPro->stime),0,sizeof(struct timeval));
						gettimeofday(&(stDetectOrPro->stime),NULL);
						stDetectOrPro++;
					}
				}//1
				pstPhaseDetect++;
				break;
			}
		}//for (i = 0; i < (stInterGrateControlData->td->phase->FactPhaseNum); i++)
	}//for (snum = 0; ;snum++)
	//结束获取相位信息

	//开启车道信息检测线程
	if(0 == CheckLaneIdYes)
	{
		int ret = pthread_create(&CheckLaneIdPid,NULL,(void *)Inter_Control_Check_Lane_Id,stInterGrateControlData);
		if (0 != ret)
		{
			#ifdef INTER_CONTROL_DEBUG
				printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
				output_log("inter control,create Inter_Control_Check_Lane_Id thread err");
			#endif
			return;
		}
		CheckLaneIdYes = 1;
	}
	
    //data send of backup pattern control
	if (SUCCESS != igc_backup_pattern_data(*(stInterGrateControlData->fd->bbserial),snum,stPhaseDetect))
	{
		#ifdef INTER_CONTROL_DEBUG
			printf("backup_pattern_data call err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
	}
	//end data send
	slnum = *(stInterGrateControlData->fd->slnum);
	*(stInterGrateControlData->fd->stageid) = stInterGrateControlData->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
	if (0 == (stInterGrateControlData->td->timeconfig->TimeConfigList[tcline][slnum].StageId))
	{
		slnum = 0;
		*(stInterGrateControlData->fd->slnum) = 0;
		*(stInterGrateControlData->fd->stageid) = stInterGrateControlData->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
	}

	unsigned char									OneCycleEnd = 0;
	InterGrateControlPhaseInfo_t					*stInterGrateControlPhaseInfo = stInterGrateControlData->pi;
	

	unsigned char				CurrentGreenFlashTime = 0; //green flash time;
	struct timeval				timeout,mtime,nowtime,lasttime,ct;
	unsigned char				leatime = 0,mt = 0,bakv = 0;
	unsigned char				sltime = 0;
	unsigned char				ffw = 0;
	fd_set						nRead;
	unsigned char				buf[256] = {0};
	unsigned char				*pbuf = buf;
	unsigned short				num = 0,mark = 0;
	unsigned char				dtype = 0;//detector type;
	unsigned char				concyc = 0;
	unsigned char				fbdata[6] = {0};
	fbdata[0] = 0xC1;
	fbdata[5] = 0xED;
	unsigned char				endahead = 0;
	unsigned char				endcyclen = 0;
	unsigned int				lefttime = 0;
	unsigned char				c1gt = 0;
	unsigned char				c1gmt = 0;
	unsigned char				c2gt = 0;
	unsigned char				c2gmt = 0;
	unsigned char				twocycend = 0;
	unsigned char				onecycend = 0;
	unsigned char				sibuf[64] = {0};
	unsigned char               *csta = NULL;
    unsigned char               tcsta = 0;
	unsigned char               syndata[3] = {0xCC,0xDD,0xED};	

	yfdata_t				stAllRedData;//data of all red
	memset(&stAllRedData,0,sizeof(stAllRedData));
	stAllRedData.second = 0;
	stAllRedData.markbit = stInterGrateControlData->fd->markbit;
	stAllRedData.markbit2 = stInterGrateControlData->fd->markbit2;
	stAllRedData.serial = *(stInterGrateControlData->fd->bbserial);
	stAllRedData.sockfd = stInterGrateControlData->fd->sockfd;
	stAllRedData.cs = stInterGrateControlData->cs;	


	stStatuSinfo.conmode = *(stInterGrateControlData->fd->contmode);
    stStatuSinfo.pattern = *(stInterGrateControlData->fd->patternid);
    stStatuSinfo.cyclet = *(stInterGrateControlData->fd->cyclet);

	fbdata[1] = 2;
	fbdata[2] = 0;
	fbdata[3] = 0x01;
	fbdata[4] = 0;
	sendfaceInfoToBoard(stInterGrateControlData->fd,fbdata);


	while (1)
	{//while loop
		if (1 == OneCycleEnd)
		{
			OneCycleEnd = 0;

			if (*(stInterGrateControlData->fd->markbit) & 0x0001)
			{//end time of current pattern has arrived
				if (!wait_write_serial(*(stInterGrateControlData->fd->synpipe)))
				{
					if (write(*(stInterGrateControlData->fd->synpipe),syndata,sizeof(syndata)) < 0)
					{
					#ifdef INTER_CONTROL_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						output_log("InterGrate control,write synpipe err");
					#endif
						Inter_Control_End_Part_Child_Thread();
						return;
					}
				}
				else
				{
					#ifdef INTER_CONTROL_DEBUG
						printf("pipe cannot be written,File: %s,Line: %d\n",__FILE__,__LINE__);
						output_log("InterGrate control,synpipe cannot write");
					#endif
					Inter_Control_End_Part_Child_Thread();
					return;
				}
			//Note, 0th bit is clean 0 by main module
				sleep(5);//wait main module end own;	
			}//end time of current pattern has arrived	

			//处理车道冲突
			if(1 == bLaneDirectionExitDanger)
			{
				friststartcontrol = 0;
			}
			//处理车道冲突
			if(noticenumer != 0)
			{
				friststartcontrol = 0;
			}

			if ((*(stInterGrateControlData->fd->markbit) & 0x0100) && (0 == endahead))
			{//current pattern transit ahead two cycles
				//Note, 8th bit is clean 0 by main module
				if (0 != *(stInterGrateControlData->fd->ncoordphase))
				{//next coordphase is not 0
					endahead = 1;
					endcyclen = 0;
					lefttime = 0;
					gettimeofday(&ct,NULL);
					lefttime = (unsigned int)((stInterGrateControlData->fd->nst) - ct.tv_sec);
					#ifdef INTER_CONTROL_DEBUG
					printf("************lefttime: %d,File: %s,Line: %d\n",lefttime,__FILE__,__LINE__);
					#endif
					if (lefttime >= (*(stInterGrateControlData->fd->cyclet)*3)/2)
					{//use two cyc to end pattern
						c1gt = (lefttime/2 + lefttime%2)/snum - 3 -3;
						c1gmt = (lefttime/2 + lefttime%2)%snum;
						c2gt = (lefttime/2)/snum - 3 - 3;
						c2gmt = (lefttime/2)%snum;
						twocycend = 1;
						onecycend = 0;
					}//use two cyc to end pattern
					else
					{//use one cyc to end pattern
						c2gmt = 0;
						c2gt = 0;
						c1gt = lefttime/snum - 3 -3;
						c1gmt = lefttime%snum;
						twocycend = 0;
						onecycend = 1;				
					}//use one cyc to end pattern
					#ifdef INTER_CONTROL_DEBUG
                	printf("c1gt:%d,c1gmt:%d,c2gt:%d,c2gmt:%d,twocycend:%d,onecycend:%d,Line:%d\n", \
                    	c1gt,c1gmt,c2gt,c2gmt,twocycend,onecycend,__LINE__);
                	#endif
				}//next coordphase is not 0
			}//current pattern transit ahead two cycles

			if (1 == endahead)
            {
                endcyclen += 1;
				if (1 == endcyclen)
                {
                    stStatuSinfo.cyclet = (c1gt + 3 + 3)*snum + c1gmt;
                }
                if (3 == endcyclen)
                {
                    endcyclen = 2;
                    ct.tv_sec = 0;
                    ct.tv_usec = 200000;
                    select(0,NULL,NULL,NULL,&ct);
                    OneCycleEnd = 1;
                    continue;
                }
                if (2 == endcyclen)
                {
                    if ((1 == onecycend) && (0 == twocycend))
                    {
                        endcyclen = 2;
                        ct.tv_sec = 0;
                        ct.tv_usec = 200000;
                        select(0,NULL,NULL,NULL,&ct);
                        OneCycleEnd = 1;
                        continue;
                    }
					stStatuSinfo.cyclet = (c2gt + 3 + 3)*snum + c2gmt;
                }
			//	#ifdef FULL_DOWN_TIME
				if (*(stInterGrateControlData->fd->auxfunc) & 0x01)
				{//if (*(stInterGrateControlData->fd->auxfunc) & 0x01)
					if ((30 != *(stInterGrateControlData->fd->contmode)) && (31 != *(stInterGrateControlData->fd->contmode)))
					{//if ((30 != *(sadata->fd->contmode)) && (31 != *(sadata->fd->contmode)))
						*(stInterGrateControlData->fd->v2xmark) |= 0x01;//V2X
						for (i = 0;i < MAX_PHASE_LINE;i++)
						{
							if (0 == stPhaseDetect[i].stageid)
								break;
							if (*(stInterGrateControlData->fd->stageid) == stPhaseDetect[i].stageid)
							{
								break;
							}
						}
						for (j = 1; j < (MAX_CHANNEL+1); j++)
						{
							if (stPhaseDetect[i].chans & (0x00000001 << (j -1)))
								continue;
							for (z = 0; z < MAX_PHASE_LINE; z++)
							{
								if (0 == stPhaseDetect[z].stageid)
									break;
								if (z == i)
									continue;
								downti[6] = 0;
								if (stPhaseDetect[z].chans & (0x00000001 << (j - 1)))
								{
									k = i;
									if (z > i)
									{
										while (k != z)
										{
											if (1 == endcyclen)
											{
												downti[6] += c1gt + 3 + 3;	
											}
											else if (2 == endcyclen)
											{
												downti[6] += c2gt + 3 + 3;
											}
											k++;
										}
									}
									if (z < i)
									{
										while (k != z)
										{
											if (0 == stPhaseDetect[k].stageid)
											{
												k = 0;
												if (1 == endcyclen)
													downti[6] += c1gmt;
												if (2 == endcyclen)
													downti[6] += c2gmt;
												continue;
											}
											if (1 == endcyclen)
											{
												downti[6] += c1gt + 3 + 3;
											}
											else if (2 == endcyclen)
											{
												downti[6] += c2gt + 3 + 3;
											}
											k++;
										}
									}

									tchans = 0;
									tchans |= (0x00000001 << (j-1));
									downti[1] = 0;
									downti[1] |= (tchans & 0x000000ff);
									downti[2] = 0;
									downti[2] |= (((tchans & 0x0000ff00) >> 8) & 0x000000ff);
									downti[3] = 0;
									downti[3] |= (((tchans & 0x00ff0000) >> 16) & 0x000000ff);
									downti[4] = 0;
									downti[4] |= (((tchans & 0xff000000) >> 24) & 0x000000ff);
									downti[5] = 0;//red color

									
									if (!wait_write_serial(*(stInterGrateControlData->fd->bbserial)))
									{
										if (write(*(stInterGrateControlData->fd->bbserial),downti,sizeof(downti)) < 0)
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
									break;
								}//if (stPhaseDetect[z].chans & (0x00000001 << (j - 1)))
							}//for (z = 0; z < MAX_PHASE_LINE; z++)
						}//for (j = 1; j < (MAX_CHANNEL+1); j++)	
					}//if ((30 != *(stInterGrateControlData->fd->contmode)) && (31 != *(stInterGrateControlData->fd->contmode)))
				}//if (*(stInterGrateControlData->fd->auxfunc) & 0x01)
		//		#endif
            }//if (1 == endahead)
		}//1 == OneCycleEnd

		memset(stInterGrateControlPhaseInfo,0,sizeof(InterGrateControlPhaseInfo_t));
		//获取单相位信息
		if (SUCCESS != igc_get_phase_info(stInterGrateControlData->fd,stInterGrateControlData->td,tcline,slnum,stInterGrateControlPhaseInfo))
		{
			#ifdef INTER_CONTROL_DEBUG
				printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
				output_log("InterGrate control,get phase info err");
			#endif
			Inter_Control_End_Part_Child_Thread();
			return;
		}
		

		*(stInterGrateControlData->fd->phaseid) = 0;
		*(stInterGrateControlData->fd->phaseid) |= (0x01 << (stInterGrateControlPhaseInfo->phaseid - 1));
		stStatuSinfo.stage = *(stInterGrateControlData->fd->stageid);
        stStatuSinfo.phase = *(stInterGrateControlData->fd->phaseid);

		//printf("friststartcontrol : %d , slnum : %d , bLaneDirectionExitDanger : %d , bAllLampClose : %d \n",friststartcontrol,slnum,bLaneDirectionExitDanger,bAllLampClose);
		if ((friststartcontrol != 0) && (slnum == 0))
		{
			//信号机更新了方案，通知主模块，子模块运行结束
			if (*(stInterGrateControlData->fd->markbit) & 0x0001)
			{
				#ifdef INTER_CONTROL_DEBUG
	                printf("singer update fangan info,File: %s,Line: %d\n",__FILE__,__LINE__);
	            #endif
				if (!wait_write_serial(*(stInterGrateControlData->fd->synpipe)))
				{
					if (write(*(stInterGrateControlData->fd->synpipe),syndata,sizeof(syndata)) < 0)
					{
						#ifdef INTER_CONTROL_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
							output_log("InterGrate control,write synpipe err");
						#endif
						Inter_Control_End_Part_Child_Thread();
						return;
					}
				}
				else
				{
					#ifdef INTER_CONTROL_DEBUG
						printf("pipe cannot be written,File: %s,Line: %d\n",__FILE__,__LINE__);
						output_log("InterGrate control,synpipe cannot write");
					#endif
					Inter_Control_End_Part_Child_Thread();
					return;
				}
				sleep(5);	
			}
			if(1 == bLaneDirectionExitDanger)
			{
				friststartcontrol = 0;
				continue;
			}
			if(friststartcontrol == 2)//关灯后,静止0.5s循环
			{
				ct.tv_sec = 0;
				ct.tv_usec = 500000;
				select(0,NULL,NULL,NULL,&ct);
				continue;
			}
			//igc_channel_yellow_flash(stInterGrateControlData);
			Inter_Control_Close_All_Lamp(stInterGrateControlData);
			friststartcontrol = 2;
			bAllLampClose = 1;
		}
		else
		{
			//开灯的时候，进行3s黄闪处理
			if(friststartcontrol == 0)
			{
				friststartcontrol = 1;
			}
			if(bAllLampClose == 1)
			{
				bAllLampClose = 0;
				igc_channel_yellow_flash(stInterGrateControlData);
				new_all_red(&stAllRedData);
			}
				
			*(stInterGrateControlData->fd->color) = 0x02;
			*(stInterGrateControlData->fd->markbit) &= 0xfbff;
			if (SUCCESS != igc_set_lamp_color(*(stInterGrateControlData->fd->bbserial),stInterGrateControlPhaseInfo->chan,0x02))
            {
	            #ifdef INTER_CONTROL_DEBUG
	                printf("set green err,File: %s,Line: %d\n",__FILE__,__LINE__);
	            #endif
                gettimeofday(&ct,NULL);
                update_event_list(stInterGrateControlData->fd->ec,stInterGrateControlData->fd->el,1,15,ct.tv_sec,stInterGrateControlData->fd->markbit);
                if (SUCCESS != generate_event_file(stInterGrateControlData->fd->ec,stInterGrateControlData->fd->el, \
													stInterGrateControlData->fd->softevent,stInterGrateControlData->fd->markbit))
                {
                #ifdef INTER_CONTROL_DEBUG
                    printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
				*(stInterGrateControlData->fd->markbit) |= 0x0800;
            }


			memset(&gtime,0,sizeof(gtime));
            gettimeofday(&gtime,NULL);
            memset(&gftime,0,sizeof(gftime));
            memset(&ytime,0,sizeof(ytime));
            memset(&rtime,0,sizeof(rtime));

            if (SUCCESS != update_channel_status(stInterGrateControlData->fd->sockfd,stInterGrateControlData->cs,stInterGrateControlPhaseInfo->chan,0x02,stInterGrateControlData->fd->markbit))
            {
            #ifdef INTER_CONTROL_DEBUG
                printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
            #endif
            }
			if (0 == stInterGrateControlPhaseInfo->cchan[0])
			{
				if ((!(*(stInterGrateControlData->fd->markbit) & 0x1000)) && (!(*(stInterGrateControlData->fd->markbit) & 0x8000)))
				{//if ((!(*(stInterGrateControlData->fd->markbit) & 0x1000)) && (!(*(stInterGrateControlData->fd->markbit) & 0x8000)))
					stStatuSinfo.time = stInterGrateControlPhaseInfo->gtime + stInterGrateControlPhaseInfo->gftime + stInterGrateControlPhaseInfo->ytime + stInterGrateControlPhaseInfo->rtime;	
					stStatuSinfo.color = 0x02;
					stStatuSinfo.conmode = *(stInterGrateControlData->fd->contmode);//added on 20150529
					stStatuSinfo.chans = 0;
            		memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            		csta = stStatuSinfo.csta;
            		for (i = 0; i < MAX_CHANNEL; i++)
            		{
               			if (0 == stInterGrateControlPhaseInfo->chan[i])
                   			break;
               			stStatuSinfo.chans += 1;
               			tcsta = stInterGrateControlPhaseInfo->chan[i];
               			tcsta <<= 2;
               			tcsta &= 0xfc;
               			tcsta |= 0x02;
               			*csta = tcsta;
               			csta++;
            		}
					memcpy(stInterGrateControlData->fd->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            		memset(sibuf,0,sizeof(sibuf));
            		if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            		{
            		#ifdef INTER_CONTROL_DEBUG
                		printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            		#endif
            		}
            		else
            		{
                		write(*(stInterGrateControlData->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            		}	
				}//if ((!(*(stInterGrateControlData->fd->markbit) & 0x1000)) && (!(*(stInterGrateControlData->fd->markbit) & 0x8000)))
			}
			else
			{
				if ((!(*(stInterGrateControlData->fd->markbit) & 0x1000)) && (!(*(stInterGrateControlData->fd->markbit) & 0x8000)))
				{//if ((!(*(stInterGrateControlData->fd->markbit) & 0x1000)) && (!(*(stInterGrateControlData->fd->markbit) & 0x8000)))
					stStatuSinfo.time = stInterGrateControlPhaseInfo->gtime + stInterGrateControlPhaseInfo->gftime;	
					stStatuSinfo.color = 0x02;
					stStatuSinfo.conmode = *(stInterGrateControlData->fd->contmode);//added on 20150529
					stStatuSinfo.chans = 0;
            		memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            		csta = stStatuSinfo.csta;
            		for (i = 0; i < MAX_CHANNEL; i++)
            		{
               			if (0 == stInterGrateControlPhaseInfo->chan[i])
                   			break;
               			stStatuSinfo.chans += 1;
               			tcsta = stInterGrateControlPhaseInfo->chan[i];
               			tcsta <<= 2;
               			tcsta &= 0xfc;
               			tcsta |= 0x02;
               			*csta = tcsta;
               			csta++;
            		}
					memcpy(stInterGrateControlData->fd->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            		memset(sibuf,0,sizeof(sibuf));
            		if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            		{
            		#ifdef INTER_CONTROL_DEBUG
                		printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            		#endif
            		}
            		else
            		{
                		write(*(stInterGrateControlData->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            		}	
				}//if ((!(*(stInterGrateControlData->fd->markbit) & 0x1000)) && (!(*(stInterGrateControlData->fd->markbit) & 0x8000)))
			}

			if ((*(stInterGrateControlData->fd->markbit) & 0x0002) && (*(stInterGrateControlData->fd->markbit) & 0x0010))
            {
                memset(&timedown,0,sizeof(timedown));
                timedown.mode = *(stInterGrateControlData->fd->contmode);
                timedown.pattern = *(stInterGrateControlData->fd->patternid);
                timedown.lampcolor = 0x02;
                timedown.lamptime = stInterGrateControlPhaseInfo->gtime + stInterGrateControlPhaseInfo->gftime;
                timedown.phaseid = stInterGrateControlPhaseInfo->phaseid;
                timedown.stageline = stInterGrateControlPhaseInfo->stageline;
                if (SUCCESS != timedown_data_to_conftool(stInterGrateControlData->fd->sockfd,&timedown))
                {
                #ifdef INTER_CONTROL_DEBUG
                    printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                #endif
                }
            }
			#ifdef I_EMBED_CONFIGURE_TOOL
            if (*(stInterGrateControlData->fd->markbit2) & 0x0200)
            {
               	memset(&timedown,0,sizeof(timedown));
                timedown.mode = *(stInterGrateControlData->fd->contmode);
                timedown.pattern = *(stInterGrateControlData->fd->patternid);
                timedown.lampcolor = 0x02;
                timedown.lamptime = stInterGrateControlPhaseInfo->gtime + stInterGrateControlPhaseInfo->gftime;
                timedown.phaseid = stInterGrateControlPhaseInfo->phaseid;
                timedown.stageline = stInterGrateControlPhaseInfo->stageline; 
                if (SUCCESS != timedown_data_to_embed(stInterGrateControlData->fd->sockfd,&timedown))
                {
                #ifdef INTER_CONTROL_DEBUG
                    printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                #endif
                }
            }
            #endif
			#ifdef SEND_FACE_BOARD
			//send info to face board
			if (*(stInterGrateControlData->fd->contmode) < 27)
                fbdata[1] = *(stInterGrateControlData->fd->contmode) + 1;
            else
                fbdata[1] = *(stInterGrateControlData->fd->contmode);
			if ((30 == fbdata[1]) || (31 == fbdata[1]))
            {
                fbdata[2] = 0;
                fbdata[3] = 0;
                fbdata[4] = 0;
            }
            else
			{
				fbdata[2] = stInterGrateControlPhaseInfo->stageline;
            	fbdata[3] = 0x02;
            	fbdata[4] = stInterGrateControlPhaseInfo->gtime + stInterGrateControlPhaseInfo->gftime;
			}
            if (!wait_write_serial(*(stInterGrateControlData->fd->fbserial)))
            {
                if (write(*(stInterGrateControlData->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
                {
                #ifdef INTER_CONTROL_DEBUG
                    printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
					*(stInterGrateControlData->fd->markbit) |= 0x0800;
                    gettimeofday(&ct,NULL);
                    update_event_list(stInterGrateControlData->fd->ec,stInterGrateControlData->fd->el,1,16,ct.tv_sec,stInterGrateControlData->fd->markbit);
                    if (SUCCESS != generate_event_file(stInterGrateControlData->fd->ec,stInterGrateControlData->fd->el,\
														stInterGrateControlData->fd->softevent,stInterGrateControlData->fd->markbit))
                    {
                    #ifdef INTER_CONTROL_DEBUG
                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                    }
                }
            }
            else
            {
	            #ifdef INTER_CONTROL_DEBUG
	                printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
	            #endif
            }
			#endif
		//	#ifdef FULL_DOWN_TIME
			if (*(stInterGrateControlData->fd->auxfunc) & 0x01)
			{//if (*(stInterGrateControlData->fd->auxfunc) & 0x01)
				if ((30 != *(stInterGrateControlData->fd->contmode)) && (31 != *(stInterGrateControlData->fd->contmode)))
				{//11
					*(stInterGrateControlData->fd->v2xmark) |= 0x01;//V2X
					for (i = 0; i < MAX_PHASE_LINE; i++)
					{
						if (0 == stPhaseDetect[i].stageid)
							break;
						if (stInterGrateControlPhaseInfo->stageline == stPhaseDetect[i].stageid)
							break;
					}
					tchans = 0;
					downti[6] = 0;
					if (i != MAX_PHASE_LINE)
					{
						if (0 == stPhaseDetect[i+1].stageid)
						{
							tchans = stPhaseDetect[0].chans;
						}
						else
						{
							tchans = stPhaseDetect[i+1].chans;
						}
						downti[6] += stPhaseDetect[i].gtime + stPhaseDetect[i].gftime + \
									stPhaseDetect[i].ytime + stPhaseDetect[i].rtime;
						for (j = 0; j < MAX_CHANNEL; j++)
						{//for (j = 0; j < MAX_CHANNEL; j++)
							if (0 == (stInterGrateControlPhaseInfo->chan[j]))
								break;
							if (tchans & (0x00000001 << (stInterGrateControlPhaseInfo->chan[j] - 1)))
								tchans &= (~(0x00000001 << (stInterGrateControlPhaseInfo->chan[j] - 1)));
						}//for (j = 0; j < MAX_CHANNEL; j++)
					}//if (i != MAX_PHASE_LINE)
					downti[1] = 0;
					downti[1] |= (tchans & 0x000000ff);
					downti[2] = 0;
					downti[2] |= (((tchans & 0x0000ff00) >> 8) & 0x000000ff);
					downti[3] = 0;
					downti[3] |= (((tchans & 0x00ff0000) >> 16) & 0x000000ff);
					downti[4] = 0;
					downti[4] |= (((tchans & 0xff000000) >> 24) & 0x000000ff);	
					downti[5] = 0x00;
		
					if (!wait_write_serial(*(stInterGrateControlData->fd->bbserial)))
					{
						if (write(*(stInterGrateControlData->fd->bbserial),downti,sizeof(downti)) < 0)
						{
						#ifdef INTER_CONTROL_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
					}
					else
					{
					#ifdef INTER_CONTROL_DEBUG
						printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}	

					tchans = 0;
					for (j = 0; j < MAX_CHANNEL; j++)
					{
						if (0 == (stInterGrateControlPhaseInfo->cchan[j]))
							break;
						tchans |= (0x00000001 << ((stInterGrateControlPhaseInfo->cchan[j]) - 1));
					}
					downti[1] = 0;
					downti[1] |= (tchans & 0x000000ff);
					downti[2] = 0;
					downti[2] |= (((tchans & 0x0000ff00) >> 8) & 0x000000ff);
					downti[3] = 0;
					downti[3] |= (((tchans & 0x00ff0000) >> 16) & 0x000000ff);
					downti[4] = 0;
					downti[4] |= (((tchans & 0xff000000) >> 24) & 0x000000ff);	
					downti[5] = 0x02;
					downti[6] = stInterGrateControlPhaseInfo->gtime + stInterGrateControlPhaseInfo->gftime;

					if (!wait_write_serial(*(stInterGrateControlData->fd->bbserial)))
					{
						if (write(*(stInterGrateControlData->fd->bbserial),downti,sizeof(downti)) < 0)
						{
						#ifdef INTER_CONTROL_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
					}
					else
					{
					#ifdef INTER_CONTROL_DEBUG
						printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					if (!wait_write_serial(*(stInterGrateControlData->fd->bbserial)))
					{
						if (write(*(stInterGrateControlData->fd->bbserial),edownti,sizeof(edownti)) < 0)
						{
						#ifdef INTER_CONTROL_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
					}
					else
					{
					#ifdef INTER_CONTROL_DEBUG
						printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
				}//11	
			}//if (*(stInterGrateControlData->fd->auxfunc) & 0x01)
	//		#endif

			while (1)
			{	
				memset(buf,0,sizeof(buf));
				if (read(*(stInterGrateControlData->fd->ffpipe),buf,sizeof(buf)) <= 0)
					break;
			}
			if(stInterGrateControlPhaseInfo->gftime == 0)
			{
				sltime = stInterGrateControlPhaseInfo->gtime - 3;
				ffw = 0;
				while (1)	
				{
					FD_ZERO(&nRead);
					FD_SET(*(stInterGrateControlData->fd->ffpipe),&nRead);
					lasttime.tv_sec = 0;
					lasttime.tv_usec = 0;
					gettimeofday(&lasttime,NULL);
					bakv = sltime;
					mtime.tv_sec = sltime;
					mtime.tv_usec = 0;
					int mret = select(*(stInterGrateControlData->fd->ffpipe)+1,&nRead,NULL,NULL,&mtime);
					if (mret < 0)
					{
						#ifdef INTER_CONTROL_DEBUG
						printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						break;
					}
					if (0 == mret)
					{
						break;
					}
					if (mret > 0)
					{
						if (FD_ISSET(*(stInterGrateControlData->fd->ffpipe),&nRead))
						{
							memset(buf,0,sizeof(buf));
							read(*(stInterGrateControlData->fd->ffpipe),buf,sizeof(buf));
							if (!strncmp(buf,"fastforward",11))
							{
								ffw = 1;
								break;
							}
							else
							{
								nowtime.tv_sec = 0;
                        		nowtime.tv_usec = 0;
                        		gettimeofday(&nowtime,NULL);
                        		leatime = nowtime.tv_sec - lasttime.tv_sec;
                        		sltime -= leatime;
								if (sltime > bakv)
                                	sltime = bakv;
                        		continue;
							}
						}
						else
						{
							nowtime.tv_sec = 0;
							nowtime.tv_usec = 0;
							gettimeofday(&nowtime,NULL);
							leatime = nowtime.tv_sec - lasttime.tv_sec;
							sltime -= leatime;
							if (sltime > bakv)
                                sltime = bakv;
							continue;
						}
					}//mret > 0
				}//while (1)
				if(0 == ffw)
				{
					sltime = 3;
					int maxv = 0;
					noticenumer = 0;
					while (1)	
					{
						FD_ZERO(&nRead);
						FD_SET(*(stInterGrateControlData->fd->ffpipe),&nRead);
						FD_SET(noticepipe[0],&nRead);
						
						lasttime.tv_sec = 0;
						lasttime.tv_usec = 0;
						gettimeofday(&lasttime,NULL);
						bakv = sltime;
						mtime.tv_sec = sltime;
						mtime.tv_usec = 0;
						maxv = MAX(*(stInterGrateControlData->fd->ffpipe),noticepipe[0]);
						int mret = select(*(stInterGrateControlData->fd->ffpipe)+1,&nRead,NULL,NULL,&mtime);
						if (mret < 0)
						{
							#ifdef INTER_CONTROL_DEBUG
							printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							break;
						}
						if (0 == mret)
						{
							break;
						}
						if (mret > 0)
						{
							if (FD_ISSET(*(stInterGrateControlData->fd->ffpipe),&nRead))
							{
								memset(buf,0,sizeof(buf));
								read(*(stInterGrateControlData->fd->ffpipe),buf,sizeof(buf));
								if (!strncmp(buf,"fastforward",11))
								{
									break;
								}
								else
								{
									nowtime.tv_sec = 0;
	                        		nowtime.tv_usec = 0;
	                        		gettimeofday(&nowtime,NULL);
	                        		leatime = nowtime.tv_sec - lasttime.tv_sec;
	                        		sltime -= leatime;
									if (sltime > bakv)
	                                	sltime = bakv;
	                        		continue;
								}
							}
							else if(FD_ISSET(noticepipe[0],&nRead))
							{
								if (!strncmp(buf,"danger",6))
								{
									noticenumer++;
									nowtime.tv_sec = 0;
	                        		nowtime.tv_usec = 0;
	                        		gettimeofday(&nowtime,NULL);
	                        		leatime = nowtime.tv_sec - lasttime.tv_sec;
	                        		sltime -= leatime;
									if (sltime > bakv)
	                                	sltime = bakv;
									printf("current is last phase and receive danger num : %d info,File: %s,Line: %d\n",noticenumer,__FILE__,__LINE__);
									continue;
								}
								else
								{
									nowtime.tv_sec = 0;
	                        		nowtime.tv_usec = 0;
	                        		gettimeofday(&nowtime,NULL);
	                        		leatime = nowtime.tv_sec - lasttime.tv_sec;
	                        		sltime -= leatime;
									if (sltime > bakv)
	                                	sltime = bakv;
	                        		continue;
								}
							}	
							else
							{
								nowtime.tv_sec = 0;
								nowtime.tv_usec = 0;
								gettimeofday(&nowtime,NULL);
								leatime = nowtime.tv_sec - lasttime.tv_sec;
								sltime -= leatime;
								if (sltime > bakv)
	                                sltime = bakv;
								continue;
							}
						}//mret > 0
					}
				}
			}
			else
			{

				sltime = stInterGrateControlPhaseInfo->gtime;
				while (1)	
				{
					FD_ZERO(&nRead);
					FD_SET(*(stInterGrateControlData->fd->ffpipe),&nRead);
					lasttime.tv_sec = 0;
					lasttime.tv_usec = 0;
					gettimeofday(&lasttime,NULL);
					bakv = sltime;
					mtime.tv_sec = sltime;
					mtime.tv_usec = 0;
					int mret = select(*(stInterGrateControlData->fd->ffpipe)+1,&nRead,NULL,NULL,&mtime);
					if (mret < 0)
					{
					#ifdef INTER_CONTROL_DEBUG
						printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						break;
					}
					if (0 == mret)
					{
						break;
					}
					if (mret > 0)
					{
						if (FD_ISSET(*(stInterGrateControlData->fd->ffpipe),&nRead))
						{
							memset(buf,0,sizeof(buf));
							read(*(stInterGrateControlData->fd->ffpipe),buf,sizeof(buf));
							if (!strncmp(buf,"fastforward",11))
							{
								break;
							}
							else
							{
								nowtime.tv_sec = 0;
                        		nowtime.tv_usec = 0;
                        		gettimeofday(&nowtime,NULL);
                        		leatime = nowtime.tv_sec - lasttime.tv_sec;
                        		sltime -= leatime;
								if (sltime > bakv)
                                	sltime = bakv;
                        		continue;
							}
						}
						else
						{
							nowtime.tv_sec = 0;
							nowtime.tv_usec = 0;
							gettimeofday(&nowtime,NULL);
							leatime = nowtime.tv_sec - lasttime.tv_sec;
							sltime -= leatime;
							if (sltime > bakv)
                                sltime = bakv;
							continue;
						}
					}//mret > 0
				}//while (1)
			}//person channels do not exist

			//Begin to green flash
			if ((0 != stInterGrateControlPhaseInfo->cchan[0]) && (stInterGrateControlPhaseInfo->gftime > 0))
			{
				if ((!(*(stInterGrateControlData->fd->markbit) & 0x1000)) && (!(*(stInterGrateControlData->fd->markbit) & 0x8000)))
				{//if ((!(*(stInterGrateControlData->fd->markbit) & 0x1000)) && (!(*(stInterGrateControlData->fd->markbit) & 0x8000)))
					stStatuSinfo.time = stInterGrateControlPhaseInfo->gftime;	
					stStatuSinfo.color = 0x03;
					stStatuSinfo.conmode = *(stInterGrateControlData->fd->contmode);//added on 20150529
					for (i = 0; i < MAX_CHANNEL; i++)
           			{
            			if (0 == stInterGrateControlPhaseInfo->cchan[i])
                   			break;
               			for (j = 0; j < stStatuSinfo.chans; j++)
               			{
                   			if (0 == stStatuSinfo.csta[j])
                   				break;
                   			tcsta = stStatuSinfo.csta[j];
                   			tcsta &= 0xfc;
                   			tcsta >>= 2;
                   			tcsta &= 0x3f;
                   			if (tcsta == stInterGrateControlPhaseInfo->cchan[i])
                   			{
                       			stStatuSinfo.csta[j] &= 0xfc;
								stStatuSinfo.csta[j] |= 0x03;
								break;
                   			}
               			}
           			}
					memcpy(stInterGrateControlData->fd->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
					memset(sibuf,0,sizeof(sibuf));
           			if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
           			{
           			#ifdef INTER_CONTROL_DEBUG
                		printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            		#endif
            		}
            		else
            		{
                		write(*(stInterGrateControlData->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            		}	
				}//if ((!(*(stInterGrateControlData->fd->markbit) & 0x1000)) && (!(*(stInterGrateControlData->fd->markbit) & 0x8000)))
			}
	
			memset(&gtime,0,sizeof(gtime));
			memset(&gftime,0,sizeof(gftime));
			gettimeofday(&gftime,NULL);
			memset(&ytime,0,sizeof(ytime));
			memset(&rtime,0,sizeof(rtime));
			*(stInterGrateControlData->fd->markbit) |= 0x0400;
			if (stInterGrateControlPhaseInfo->gftime > 0)
			{
				struct timeval      stWaitTime,stCurrentTime,stLastTime;
				fd_set				nRead;
				unsigned char       readnum = 0;
				unsigned char		noticebuf[10] = {0};
				unsigned int		cLeftTime=500000;
				unsigned char		maxtime = (stInterGrateControlPhaseInfo->gftime) * 2;
				CurrentGreenFlashTime = 1;
				noticenumer = 0;
				while (1)
				{
					cLeftTime=500000;
					if(CurrentGreenFlashTime%2 !=0)
					{
						if (SUCCESS != igc_set_lamp_color(*(stInterGrateControlData->fd->bbserial),stInterGrateControlPhaseInfo->cchan,0x02))
						{
							#ifdef INTER_CONTROL_DEBUG
							printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							gettimeofday(&ct,NULL);
							update_event_list(stInterGrateControlData->fd->ec,stInterGrateControlData->fd->el,1,15,ct.tv_sec,stInterGrateControlData->fd->markbit);
							if (SUCCESS != generate_event_file(stInterGrateControlData->fd->ec,stInterGrateControlData->fd->el,\
																stInterGrateControlData->fd->softevent,stInterGrateControlData->fd->markbit))
							{
							#ifdef INTER_CONTROL_DEBUG
								printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}
							*(stInterGrateControlData->fd->markbit) |= 0x0800;
						}
						if (SUCCESS != update_channel_status(stInterGrateControlData->fd->sockfd,stInterGrateControlData->cs,stInterGrateControlPhaseInfo->cchan,0x02, \
															stInterGrateControlData->fd->markbit))
						{
							#ifdef INTER_CONTROL_DEBUG
							printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
						}
						while (1)//绿灯
						{
							FD_ZERO(&nRead);
							FD_SET(noticepipe[0],&nRead);
							stWaitTime.tv_sec = 0;
							stWaitTime.tv_usec = cLeftTime;
							
							stLastTime.tv_sec = 0;
							stLastTime.tv_usec = 0;
							gettimeofday(&stLastTime,NULL);
							int fret = select(noticepipe[0]+1,&nRead,NULL,NULL,&stWaitTime);
							if (fret < 0)
							{
								#ifdef INTER_CONTROL_DEBUG
									printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								break;
							}
							if(fret == 0)
							{
								//noticenumer = 0;
								break;
							}
							if(fret > 0)
							{
								if(FD_ISSET(noticepipe[0],&nRead))
								{
									memset(noticebuf,0,sizeof(noticebuf));
									readnum = 0;
									readnum = read(noticepipe[0],noticebuf,sizeof(noticebuf));
									if(readnum > 0)
									{
										if (!strncmp("danger",noticebuf,6))
										{
											noticenumer++;
											stCurrentTime.tv_sec = 0;
											stCurrentTime.tv_usec = 0;
											gettimeofday(&stCurrentTime,NULL);
											cLeftTime = cLeftTime - (stCurrentTime.tv_usec - stLastTime.tv_usec);
											printf("current is last phase and receive danger num : %d info,File: %s,Line: %d\n",noticenumer,__FILE__,__LINE__);
											continue;
										}
									}
								}
							}
						}
					}
					else
					{
						if (SUCCESS != igc_set_lamp_color(*(stInterGrateControlData->fd->bbserial),stInterGrateControlPhaseInfo->cchan,0x03))
						{
							#ifdef INTER_CONTROL_DEBUG
								printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							gettimeofday(&ct,NULL);
							update_event_list(stInterGrateControlData->fd->ec,stInterGrateControlData->fd->el,1,15,ct.tv_sec,stInterGrateControlData->fd->markbit);
							if (SUCCESS != generate_event_file(stInterGrateControlData->fd->ec,stInterGrateControlData->fd->el,\
																stInterGrateControlData->fd->softevent,stInterGrateControlData->fd->markbit))
							{
							#ifdef INTER_CONTROL_DEBUG
								printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}
							*(stInterGrateControlData->fd->markbit) |= 0x0800;
						}
						if (SUCCESS != update_channel_status(stInterGrateControlData->fd->sockfd,stInterGrateControlData->cs,stInterGrateControlPhaseInfo->cchan,0x03, \
															stInterGrateControlData->fd->markbit))
						{
						#ifdef INTER_CONTROL_DEBUG
							printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
						while (1)//灭灯
						{
							FD_ZERO(&nRead);
							FD_SET(noticepipe[0],&nRead);
							stWaitTime.tv_sec = 0;
							stWaitTime.tv_usec = cLeftTime;
							
							stLastTime.tv_sec = 0;
							stLastTime.tv_usec = 0;
							gettimeofday(&stLastTime,NULL);
							int fret = select(noticepipe[0]+1,&nRead,NULL,NULL,&stWaitTime);
							if (fret < 0)
							{
								#ifdef INTER_CONTROL_DEBUG
									printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								break;
							}
							if(fret == 0)
							{
								//noticenumer = 0;
								break;
							}
							if(fret > 0)
							{
								if(FD_ISSET(noticepipe[0],&nRead))
								{
									memset(noticebuf,0,sizeof(noticebuf));
									readnum = 0;
									readnum = read(noticepipe[0],noticebuf,sizeof(noticebuf));
									if(readnum > 0)
									{
										if (!strncmp("danger",noticebuf,6))
										{
											noticenumer++;
											stCurrentTime.tv_sec = 0;
											stCurrentTime.tv_usec = 0;
											gettimeofday(&stCurrentTime,NULL);
											cLeftTime = cLeftTime - (stCurrentTime.tv_usec - stLastTime.tv_usec);
											printf("current is last phase and receive danger num : %d info,File: %s,Line: %d\n",noticenumer,__FILE__,__LINE__);
											continue;
										}
									}
								}
							}
						}
					}

					CurrentGreenFlashTime += 1;
					if (CurrentGreenFlashTime > maxtime)
						break;
				}
			}//if (stInterGrateControlPhaseInfo->gftime > 0)	
			//end green flash
			if (1 == phcon)
            {
                *(stInterGrateControlData->fd->markbit) &= 0xfbff;
                memset(&gtime,0,sizeof(gtime));
                gettimeofday(&gtime,NULL);
                memset(&gftime,0,sizeof(gftime));
                memset(&ytime,0,sizeof(ytime));
                memset(&rtime,0,sizeof(rtime));
                phcon = 0;
                sleep(10);
            }

	
			//Begin to set yellow lamp 
			if (1 == (stInterGrateControlPhaseInfo->cpcexist))
			{//person channels exist in current phase
				//all person channels will be set red lamp;
				if (SUCCESS != igc_set_lamp_color(*(stInterGrateControlData->fd->bbserial),stInterGrateControlPhaseInfo->cpchan,0x00))
				{
				#ifdef INTER_CONTROL_DEBUG
					printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					gettimeofday(&ct,NULL);
                	update_event_list(stInterGrateControlData->fd->ec,stInterGrateControlData->fd->el,1,15,ct.tv_sec,stInterGrateControlData->fd->markbit);
                	if (SUCCESS != generate_event_file(stInterGrateControlData->fd->ec,stInterGrateControlData->fd->el,\
														stInterGrateControlData->fd->softevent,stInterGrateControlData->fd->markbit))
                	{
                	#ifdef INTER_CONTROL_DEBUG
                    	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                	}
					*(stInterGrateControlData->fd->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(stInterGrateControlData->fd->sockfd,stInterGrateControlData->cs,stInterGrateControlPhaseInfo->cpchan,0x00, \
                                                    stInterGrateControlData->fd->markbit))
                {
                #ifdef INTER_CONTROL_DEBUG
                    printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }	
				//other change channels will be set yellow lamp;
				if (SUCCESS != igc_set_lamp_color(*(stInterGrateControlData->fd->bbserial),stInterGrateControlPhaseInfo->cnpchan,0x01))
				{
				#ifdef INTER_CONTROL_DEBUG
					printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					gettimeofday(&ct,NULL);
                    update_event_list(stInterGrateControlData->fd->ec,stInterGrateControlData->fd->el,1,15,ct.tv_sec,stInterGrateControlData->fd->markbit);
                    if (SUCCESS != generate_event_file(stInterGrateControlData->fd->ec,stInterGrateControlData->fd->el,\
														stInterGrateControlData->fd->softevent,stInterGrateControlData->fd->markbit))
                    {
                    #ifdef INTER_CONTROL_DEBUG
                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                    }
					*(stInterGrateControlData->fd->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(stInterGrateControlData->fd->sockfd,stInterGrateControlData->cs,stInterGrateControlPhaseInfo->cnpchan,0x01, \
                                                    stInterGrateControlData->fd->markbit))
                {
                #ifdef INTER_CONTROL_DEBUG
                    printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
			}//person channels exist in current phase
			else
			{//Not person channels in current phase
				if (SUCCESS != igc_set_lamp_color(*(stInterGrateControlData->fd->bbserial),stInterGrateControlPhaseInfo->cchan,0x01))
				{
				#ifdef INTER_CONTROL_DEBUG
					printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					gettimeofday(&ct,NULL);
                    update_event_list(stInterGrateControlData->fd->ec,stInterGrateControlData->fd->el,1,15,ct.tv_sec,stInterGrateControlData->fd->markbit);
                    if (SUCCESS != generate_event_file(stInterGrateControlData->fd->ec,stInterGrateControlData->fd->el,\
														stInterGrateControlData->fd->softevent,stInterGrateControlData->fd->markbit))
                    {
                    #ifdef INTER_CONTROL_DEBUG
                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                    }
					*(stInterGrateControlData->fd->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(stInterGrateControlData->fd->sockfd,stInterGrateControlData->cs,stInterGrateControlPhaseInfo->cchan,0x01, \
                                                    stInterGrateControlData->fd->markbit))
                {
                #ifdef INTER_CONTROL_DEBUG
                    printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
			}//Not person channels in current phase

		//	#ifdef FULL_DOWN_TIME
			if (*(stInterGrateControlData->fd->auxfunc) & 0x01)
			{//if (*(stInterGrateControlData->fd->auxfunc) & 0x01)
				if ((30 != *(stInterGrateControlData->fd->contmode)) && (31 != *(stInterGrateControlData->fd->contmode)))
				{//11
					*(stInterGrateControlData->fd->v2xmark) |= 0x01;//V2X

					downti[5] = 0x01;
					downti[6] = stInterGrateControlPhaseInfo->ytime;

					if (!wait_write_serial(*(stInterGrateControlData->fd->bbserial)))
					{
						if (write(*(stInterGrateControlData->fd->bbserial),downti,sizeof(downti)) < 0)
						{
						#ifdef INTER_CONTROL_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
					}
					else
					{
					#ifdef INTER_CONTROL_DEBUG
						printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					if (!wait_write_serial(*(stInterGrateControlData->fd->bbserial)))
					{
						if (write(*(stInterGrateControlData->fd->bbserial),edownti,sizeof(edownti)) < 0)
						{
						#ifdef INTER_CONTROL_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
					}
					else
					{
					#ifdef INTER_CONTROL_DEBUG
						printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
				}//11	
			}//if (*(stInterGrateControlData->fd->auxfunc) & 0x01)
	//		#endif

			if ((0 != stInterGrateControlPhaseInfo->cchan[0]) && (stInterGrateControlPhaseInfo->ytime > 0))
			{
				if ((!(*(stInterGrateControlData->fd->markbit) & 0x1000)) && (!(*(stInterGrateControlData->fd->markbit) & 0x8000)))
				{//if ((!(*(stInterGrateControlData->fd->markbit) & 0x1000)) && (!(*(stInterGrateControlData->fd->markbit) & 0x8000)))
					stStatuSinfo.time = stInterGrateControlPhaseInfo->ytime;	
					stStatuSinfo.color = 0x01;
					stStatuSinfo.conmode = *(stInterGrateControlData->fd->contmode);//added on 20150529
					for (i = 0; i < MAX_CHANNEL; i++)
           			{
            			if (0 == stInterGrateControlPhaseInfo->cnpchan[i])
                   			break;
               			for (j = 0; j < stStatuSinfo.chans; j++)
               			{
                   			if (0 == stStatuSinfo.csta[j])
                   				break;
                   			tcsta = stStatuSinfo.csta[j];
                   			tcsta &= 0xfc;
                   			tcsta >>= 2;
                   			tcsta &= 0x3f;
                   			if (tcsta == stInterGrateControlPhaseInfo->cnpchan[i])
                   			{
                       			stStatuSinfo.csta[j] &= 0xfc;
								stStatuSinfo.csta[j] |= 0x01;
								break;
                   			}
               			}
            		}
					for (i = 0; i < MAX_CHANNEL; i++)
               		{
                   		if (0 == stInterGrateControlPhaseInfo->cpchan[i])
                       		break;
                   		for (j = 0; j < stStatuSinfo.chans; j++)
                   		{
                       		if (0 == stStatuSinfo.csta[j])
                           		break;
                       		tcsta = stStatuSinfo.csta[j];
                       		tcsta &= 0xfc;
                       		tcsta >>= 2;
                       		tcsta &= 0x3f;
                       		if (tcsta == stInterGrateControlPhaseInfo->cpchan[i])
                       		{
                           		stStatuSinfo.csta[j] &= 0xfc;
								break;
                       		}
                   		}
               		}
					memcpy(stInterGrateControlData->fd->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
					memset(sibuf,0,sizeof(sibuf));
            		if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            		{
            		#ifdef INTER_CONTROL_DEBUG
                		printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            		#endif
            		}
            		else
            		{
                		write(*(stInterGrateControlData->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            		}	
				}//if ((!(*(stInterGrateControlData->fd->markbit) & 0x1000)) && (!(*(stInterGrateControlData->fd->markbit) & 0x8000)))
			}

			if ((*(stInterGrateControlData->fd->markbit) & 0x0002) && (*(stInterGrateControlData->fd->markbit) & 0x0010))
            {
                memset(&timedown,0,sizeof(timedown));
                timedown.mode = *(stInterGrateControlData->fd->contmode);
                timedown.pattern = *(stInterGrateControlData->fd->patternid);
                timedown.lampcolor = 0x01;
                timedown.lamptime = stInterGrateControlPhaseInfo->ytime;
                timedown.phaseid = stInterGrateControlPhaseInfo->phaseid;
                timedown.stageline = stInterGrateControlPhaseInfo->stageline;
                if (SUCCESS != timedown_data_to_conftool(stInterGrateControlData->fd->sockfd,&timedown))
                {
                #ifdef INTER_CONTROL_DEBUG
                    printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                #endif
                }
            }
			#ifdef I_EMBED_CONFIGURE_TOOL
            if (*(stInterGrateControlData->fd->markbit2) & 0x0200)
            {
               	memset(&timedown,0,sizeof(timedown));
                timedown.mode = *(stInterGrateControlData->fd->contmode);
                timedown.pattern = *(stInterGrateControlData->fd->patternid);
                timedown.lampcolor = 0x01;
                timedown.lamptime = stInterGrateControlPhaseInfo->ytime;
                timedown.phaseid = stInterGrateControlPhaseInfo->phaseid;
                timedown.stageline = stInterGrateControlPhaseInfo->stageline; 
                if (SUCCESS != timedown_data_to_embed(stInterGrateControlData->fd->sockfd,&timedown))
                {
                #ifdef INTER_CONTROL_DEBUG
                    printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                #endif
                }
            }
            #endif
			#ifdef SEND_FACE_BOARD
			//send info to face board
			if (*(stInterGrateControlData->fd->contmode) < 27)
                fbdata[1] = *(stInterGrateControlData->fd->contmode) + 1;
            else
                fbdata[1] = *(stInterGrateControlData->fd->contmode);
			if ((30 == fbdata[1]) || (31 == fbdata[1]))
            {
                fbdata[2] = 0;
                fbdata[3] = 0;
                fbdata[4] = 0;
            }
            else
			{
				fbdata[2] = stInterGrateControlPhaseInfo->stageline;
            	fbdata[3] = 0x01;
            	fbdata[4] = stInterGrateControlPhaseInfo->ytime;
			}
            if (!wait_write_serial(*(stInterGrateControlData->fd->fbserial)))
            {
                if (write(*(stInterGrateControlData->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
                {
                #ifdef INTER_CONTROL_DEBUG
                    printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
					*(stInterGrateControlData->fd->markbit) |= 0x0800;
                    gettimeofday(&ct,NULL);
                    update_event_list(stInterGrateControlData->fd->ec,stInterGrateControlData->fd->el,1,16,ct.tv_sec,stInterGrateControlData->fd->markbit);
                    if (SUCCESS != generate_event_file(stInterGrateControlData->fd->ec,stInterGrateControlData->fd->el,\
														stInterGrateControlData->fd->softevent,stInterGrateControlData->fd->markbit))
                    {
                    #ifdef INTER_CONTROL_DEBUG
                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                    }
                }
            }
            else
            {
            #ifdef INTER_CONTROL_DEBUG
                printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
			#endif
			*(stInterGrateControlData->fd->color) = 0x01;
			memset(&gtime,0,sizeof(gtime));
            memset(&gftime,0,sizeof(gftime));
            memset(&ytime,0,sizeof(ytime));
			gettimeofday(&ytime,NULL);
            memset(&rtime,0,sizeof(rtime));
			if(0 == (stInterGrateControlData->td->timeconfig->TimeConfigList[tcline][slnum+1].StageId))
			{//最后一相位三秒黄灯处理
				unsigned char		cYellowTime = stInterGrateControlPhaseInfo->ytime;
				noticenumer=0;
				bselecterror = 0;	
				int ret = Inter_Control_Yellow_Time_Wait(&cYellowTime,&noticenumer,&bselecterror);
				if(MEMERR == ret)
				{
					bselecterror = 2;
					sleep(cYellowTime);
				}
				if(1 == bselecterror)
				{
					bselecterror = 0;
					noticenumer = 0;
					Inter_Control_Yellow_Time_Wait(&cYellowTime,&noticenumer,&bselecterror);
				}
			}
			else
			{
				sleep(stInterGrateControlPhaseInfo->ytime);
			}
			//end set yellow lamp
			
			//Begin to set red lamp
			if (SUCCESS != igc_set_lamp_color(*(stInterGrateControlData->fd->bbserial),stInterGrateControlPhaseInfo->cchan,0x00))
			{
			#ifdef INTER_CONTROL_DEBUG
				printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				gettimeofday(&ct,NULL);
                update_event_list(stInterGrateControlData->fd->ec,stInterGrateControlData->fd->el,1,15,ct.tv_sec,stInterGrateControlData->fd->markbit);
                if (SUCCESS != generate_event_file(stInterGrateControlData->fd->ec,stInterGrateControlData->fd->el, \
													stInterGrateControlData->fd->softevent,stInterGrateControlData->fd->markbit))
                {
                #ifdef INTER_CONTROL_DEBUG
                    printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
				*(stInterGrateControlData->fd->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(stInterGrateControlData->fd->sockfd,stInterGrateControlData->cs,stInterGrateControlPhaseInfo->cchan,0x00, \
												stInterGrateControlData->fd->markbit))
            {
            #ifdef INTER_CONTROL_DEBUG
                printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }

			*(stInterGrateControlData->fd->color) = 0x00;
			if (0 != stInterGrateControlPhaseInfo->cchan[0])
			{
				if ((!(*(stInterGrateControlData->fd->markbit) & 0x1000)) && (!(*(stInterGrateControlData->fd->markbit) & 0x8000)))
				{//if ((!(*(stInterGrateControlData->fd->markbit) & 0x1000)) && (!(*(stInterGrateControlData->fd->markbit) & 0x8000)))
					stStatuSinfo.time = stInterGrateControlPhaseInfo->rtime;	
					stStatuSinfo.color = 0x00;
					stStatuSinfo.conmode = *(stInterGrateControlData->fd->contmode);//added on 20150529
					for (i = 0; i < MAX_CHANNEL; i++)
           			{
           				if (0 == stInterGrateControlPhaseInfo->cchan[i])
                  			break;
               			for (j = 0; j < stStatuSinfo.chans; j++)
               			{
                   			if (0 == stStatuSinfo.csta[j])
                   				break;
                   			tcsta = stStatuSinfo.csta[j];
                   			tcsta &= 0xfc;
                   			tcsta >>= 2;
                   			tcsta &= 0x3f;
                   			if (tcsta == stInterGrateControlPhaseInfo->cchan[i])
                   			{
                       			stStatuSinfo.csta[j] &= 0xfc;
								break;
                   			}
               			}
           			}
					memcpy(stInterGrateControlData->fd->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
					memset(sibuf,0,sizeof(sibuf));
           			if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
           			{
           			#ifdef INTER_CONTROL_DEBUG
                		printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            		#endif
            		}
            		else
            		{
                		write(*(stInterGrateControlData->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            		}	
				}//if ((!(*(stInterGrateControlData->fd->markbit) & 0x1000)) && (!(*(stInterGrateControlData->fd->markbit) & 0x8000)))			
			}

			if (*(stInterGrateControlData->fd->auxfunc) & 0x01)
			{//if (*(stInterGrateControlData->fd->auxfunc) & 0x01)
				if ((30 != *(stInterGrateControlData->fd->contmode)) && (31 != *(stInterGrateControlData->fd->contmode)))
				{//11
					*(stInterGrateControlData->fd->v2xmark) |= 0x01;//V2X
					downti[5] = 0x00;
					downti[6] = stInterGrateControlPhaseInfo->rtime;
		
					if (!wait_write_serial(*(stInterGrateControlData->fd->bbserial)))
					{
						if (write(*(stInterGrateControlData->fd->bbserial),downti,sizeof(downti)) < 0)
						{
						#ifdef INTER_CONTROL_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
					}
					else
					{
					#ifdef INTER_CONTROL_DEBUG
						printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					if (!wait_write_serial(*(stInterGrateControlData->fd->bbserial)))
					{
						if (write(*(stInterGrateControlData->fd->bbserial),edownti,sizeof(edownti)) < 0)
						{
						#ifdef INTER_CONTROL_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
					}
					else
					{
					#ifdef INTER_CONTROL_DEBUG
						printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
				}//11	
			}//if (*(stInterGrateControlData->fd->auxfunc) & 0x01)
			if (stInterGrateControlPhaseInfo->rtime > 0)
            {
				memset(&gtime,0,sizeof(gtime));
            	memset(&gftime,0,sizeof(gftime));
            	memset(&ytime,0,sizeof(ytime));
            	memset(&rtime,0,sizeof(rtime));
            	gettimeofday(&rtime,NULL);

				if ((*(stInterGrateControlData->fd->markbit) & 0x0002) && (*(stInterGrateControlData->fd->markbit) & 0x0010))
            	{
               		memset(&timedown,0,sizeof(timedown));
               		timedown.mode = *(stInterGrateControlData->fd->contmode);
               		timedown.pattern = *(stInterGrateControlData->fd->patternid);
               		timedown.lampcolor = 0x00;
               		timedown.lamptime = stInterGrateControlPhaseInfo->rtime;
               		timedown.phaseid = stInterGrateControlPhaseInfo->phaseid;
               		timedown.stageline = stInterGrateControlPhaseInfo->stageline;
               		if (SUCCESS != timedown_data_to_conftool(stInterGrateControlData->fd->sockfd,&timedown))
               		{
               		#ifdef INTER_CONTROL_DEBUG
                   		printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
               		#endif
               		}
            	}
				#ifdef I_EMBED_CONFIGURE_TOOL
				if (*(stInterGrateControlData->fd->markbit2) & 0x0200)
				{
					memset(&timedown,0,sizeof(timedown));
                    timedown.mode = *(stInterGrateControlData->fd->contmode);
                    timedown.pattern = *(stInterGrateControlData->fd->patternid);
                    timedown.lampcolor = 0x00;
                    timedown.lamptime = stInterGrateControlPhaseInfo->rtime;
                    timedown.phaseid = stInterGrateControlPhaseInfo->phaseid;
                    timedown.stageline = stInterGrateControlPhaseInfo->stageline;	
					if (SUCCESS != timedown_data_to_embed(stInterGrateControlData->fd->sockfd,&timedown))
					{
					#ifdef INTER_CONTROL_DEBUG
						printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
				}
				#endif
				#ifdef SEND_FACE_BOARD
				//send info to face board
				if (*(stInterGrateControlData->fd->contmode) < 27)
                	fbdata[1] = *(stInterGrateControlData->fd->contmode) + 1;
            	else
                	fbdata[1] = *(stInterGrateControlData->fd->contmode);
				if ((30 == fbdata[1]) || (31 == fbdata[1]))
                {
                    fbdata[2] = 0;
                    fbdata[3] = 0;
                    fbdata[4] = 0;
                }
                else
				{
					fbdata[2] = stInterGrateControlPhaseInfo->stageline;
            		fbdata[3] = 0x00;
            		fbdata[4] = stInterGrateControlPhaseInfo->rtime;
				}
            	if (!wait_write_serial(*(stInterGrateControlData->fd->fbserial)))
            	{
               		if (write(*(stInterGrateControlData->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
               		{
               		#ifdef INTER_CONTROL_DEBUG
                   		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
               		#endif
						*(stInterGrateControlData->fd->markbit) |= 0x0800;
                   		gettimeofday(&ct,NULL);
                   		update_event_list(stInterGrateControlData->fd->ec,stInterGrateControlData->fd->el,1,16,ct.tv_sec,stInterGrateControlData->fd->markbit);
                   		if (SUCCESS != generate_event_file(stInterGrateControlData->fd->ec,stInterGrateControlData->fd->el,\
															stInterGrateControlData->fd->softevent,stInterGrateControlData->fd->markbit))
                   		{
                   		#ifdef INTER_CONTROL_DEBUG
                       		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                   		#endif
                   		}
               		}
            	}
            	else
            	{
            	#ifdef INTER_CONTROL_DEBUG
               		printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
            	#endif
            	}
				#endif
				sleep(stInterGrateControlPhaseInfo->rtime);
			}
			//end set red lamp


			slnum += 1;
			*(stInterGrateControlData->fd->slnum) = slnum;
			*(stInterGrateControlData->fd->stageid) = stInterGrateControlData->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
			if (0 == (stInterGrateControlData->td->timeconfig->TimeConfigList[tcline][slnum].StageId))
			{
				OneCycleEnd = 1;
				slnum = 0;
				*(stInterGrateControlData->fd->slnum) = 0;
				*(stInterGrateControlData->fd->stageid) = stInterGrateControlData->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
			}

			if (1 == phcon)
            {
				*(stInterGrateControlData->fd->markbit) &= 0xfbff;
                memset(&gtime,0,sizeof(gtime));
                gettimeofday(&gtime,NULL);
                memset(&gftime,0,sizeof(gftime));
                memset(&ytime,0,sizeof(ytime));
                memset(&rtime,0,sizeof(rtime));
                phcon = 0;
                sleep(10);
            }
		}
	}//while loop

	pthread_exit(NULL);
}

/*color: 0x00 means red,0x01 means yellow,0x02 means green,0x12 means green flash*/
int Get_Inter_Greate_Control_Status(unsigned char *color,unsigned char *leatime)
{
	if ((NULL == color) || (NULL == leatime))
	{
	#ifdef INTER_CONTROL_DEBUG
		printf("pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
		output_log("InterGrate control,pointer address is null");
	#endif
		return MEMERR;
	}

	if(1 == bAllLampClose)
	{
		*color = 0x02;
		*leatime = 0;
		return SUCCESS;
	}
	struct timeval			ntime;
	memset(&ntime,0,sizeof(ntime));
	gettimeofday(&ntime,NULL);
	if (0 != rtime.tv_sec)
	{
		*color = 0x00;
		*leatime = ntime.tv_sec - rtime.tv_sec;
	}
	if (0 != ytime.tv_sec)
	{
		*color = 0x01;
		*leatime = ntime.tv_sec - ytime.tv_sec;
	}
	if (0 != gftime.tv_sec)
	{
		*color = 0x12;
		*leatime = ntime.tv_sec - gftime.tv_sec;
	}
	if (0 != gtime.tv_sec)
	{
		*color = 0x02;
		*leatime = ntime.tv_sec - gtime.tv_sec;
	}
	return SUCCESS;
}

//一体化控制模式控制函数入口
int Inter_Grate_Control(fcdata_t *fcdata, tscdata_t *tscdata,ChannelStatus_t *chanstatus)
{
	if ((NULL == fcdata) || (NULL == tscdata) || (NULL == chanstatus))
	{
		#ifdef INTER_CONTROL_DEBUG
			printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
			output_log("inter control,pointer address is null");
		#endif
		return MEMERR;
	}

	
	InterGrateControlData_t			stInterGrateControlData;
	InterGrateControlPhaseInfo_t	stInterGrateControlPhaseInfo;
	unsigned char					contmode = *(fcdata->contmode);


	//初始化需要的变量
	InterGratedControlYes = 0;
	YellowFlashYes = 0;
	CheckLaneIdYes = 0;
	memset(&gtime,0,sizeof(gtime));
	memset(&gftime,0,sizeof(gftime));
	memset(&ytime,0,sizeof(ytime));
	memset(&rtime,0,sizeof(rtime));
	phcon = 0;
	rettl = 0;
	bAllLampClose = 0;
	friststartcontrol = 0;
	memset(fcdata->sinfo,0,sizeof(statusinfo_t));
	memset(&stStatuSinfo,0,sizeof(stStatuSinfo));
	memset(noticepipe,0,sizeof(noticepipe));
	//初始化

	if (0 != pipe2(noticepipe,O_NONBLOCK))
	{
		#ifdef MAIN_DEBUG
		printf("Create pipe error,File:%s,Line:%d\n",__FILE__,__LINE__);
		#endif
		return FAIL;
	}

	
	if (0 == InterGratedControlYes)
	{
		memset(&stInterGrateControlData,0,sizeof(stInterGrateControlData));
		memset(&stInterGrateControlPhaseInfo,0,sizeof(stInterGrateControlPhaseInfo));
		stInterGrateControlData.fd = fcdata;
		stInterGrateControlData.td = tscdata;
		stInterGrateControlData.pi = &stInterGrateControlPhaseInfo;
		stInterGrateControlData.cs = chanstatus;
		int ret = pthread_create(&InterGratedControlPid,NULL,(void *)Start_Inter_Grate_Control,&stInterGrateControlData);
		if (0 != ret)
		{
			#ifdef INTER_CONTROL_DEBUG
				printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
				output_log("inter control,create Start_Inter_Grate_Control thread err");
			#endif
			return FAIL;
		}
		InterGratedControlYes = 1;
	}

	sleep(1);

	unsigned char			rpc = 0; //running phase
	unsigned int			rpi = 0;//running phase
	unsigned char           netlock = 0;//network lock,'1'means lock,'0' means unlock
	unsigned char           close = 0;//'1' means lamps have been status of all close;
	unsigned char			cc[32] = {0};//channels of current phase;
	unsigned char			nc[32] = {0};//channels of next phase;
	unsigned char			wcc[32] = {0};//channels of will change;
	unsigned char			wnpcc[32] = {0};//not person channels of will change
	unsigned char			wpcc[32] = {0};//person channels of will change
	unsigned char			fpc = 0;//first phase control
	unsigned char			cp = 0;//current phase
	unsigned char			*pcc = NULL;
	unsigned char			ce = 0;//channel exist
	unsigned char			pce = 0;//person channel exist
	unsigned char			pex = 0;//phase exist in phase list
	unsigned char			i = 0, j = 0,z = 0,k = 0,s = 0;
	unsigned short			factovs = 0;
	unsigned char			cbuf[1024] = {0};
	infotype_t				itype;
	memset(&itype,0,sizeof(itype));
	itype.highbit = 0x01;
	itype.objectn = 0;
	itype.opertype = 0x05;
	objectinfo_t			objecti[8] = {0};
	objecti[0].objectid = 0xD4;
	objecti[0].objects = 1;
	objecti[0].indexn = 0;
	objecti[0].cobject = 0;
	objecti[0].cobjects = 0;
	objecti[0].index[0] = 0;
	objecti[0].index[1] = 0;
	objecti[0].index[2] = 0;
	objecti[0].objectvs = 1;
	yfdata_t				acdata;
	memset(&acdata,0,sizeof(acdata));
	acdata.second = 0;
	acdata.markbit = fcdata->markbit;
	acdata.markbit2 = fcdata->markbit2;
	acdata.serial = *(fcdata->bbserial);
	acdata.sockfd = fcdata->sockfd;
	acdata.cs = chanstatus;


	unsigned char           dcdownti[8] = {0xA6,0xff,0xff,0xff,0xff,0x03,0,0xED};
    unsigned char           dcedownti[3] = {0xA5,0xA5,0xED};
	fd_set					nread;
	unsigned char			keylock = 0;//key lock or unlock
	unsigned char			wllock = 0;//wireless terminal lock or unlock
	unsigned char			tcbuf[32] = {0};
	unsigned char			color = 3; //lamp is default closed;
	unsigned char			leatime = 0;
	unsigned char			dcred = 0; //'1' means lamp has been status of all red
	timedown_t				dctd;
	unsigned char			ngf = 0;
	struct timeval			to,ct;
//	struct timeval			wut;
	struct timeval			mont,ltime;
	yfdata_t				yfdata;//data of yellow flash
	yfdata_t				ardata;//data of all red
	unsigned char           fbdata[6] = {0};
	fbdata[0] = 0xC1;
	fbdata[5] = 0xED;
	unsigned char			dcecho[3] = {0};//send traffic control info to face board;
	dcecho[0] = 0xCA;
	dcecho[2] = 0xED;
	memset(&ardata,0,sizeof(ardata));
	ardata.second = 0;
	ardata.markbit = fcdata->markbit;
	ardata.markbit2 = fcdata->markbit2;
	ardata.serial = *(fcdata->bbserial);
	ardata.sockfd = fcdata->sockfd;
	ardata.cs = chanstatus;	

	unsigned char               sibuf[64] = {0};
//	statusinfo_t                stStatuSinfo;
//    memset(&stStatuSinfo,0,sizeof(statusinfo_t));
	unsigned char               *csta = NULL;
    unsigned char               tcsta = 0;
    stStatuSinfo.pattern = *(fcdata->patternid);
	unsigned char               ncmode = *(fcdata->contmode);

	unsigned char				wtlock = 0;
	struct timeval				wtltime;
	unsigned char				pantn = 0;
	unsigned char           	dirc = 0; //direct control
	unsigned char				kstep = 0;
	unsigned char				cktem = 0;
	//channes of eight direction mapping;
  	//left and right lamp all close
	unsigned char 		dirch1[8][8] = {{1,3,13,15,0,0,0,0},{0,0,0,0,0,0,0,0},
                                    {2,4,14,16,0,0,0,0},{0,0,0,0,0,0,0,0},
                                    {1,13,0,0,0,0,0,0},{2,14,0,0,0,0,0,0},
                                    {3,15,0,0,0,0,0,0},{4,16,0,0,0,0,0,0}};

	unsigned char       lkch1[4][8] = {{1,3,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
                                        {2,4,0,0,0,0,0,0},{0,0,0,0,0,0,0,0}};
	unsigned char		clch1[12] = {5,6,7,8,9,10,11,12,17,18,19,20};
	//only close left lamp
	unsigned char 		dirch2[8][8] = {{1,3,9,11,13,15,0,0},{9,11,0,0,0,0,0,0},
                                      {2,4,14,16,0,0,0,0},{10,12,0,0,0,0,0,0},
                                      {1,9,13,0,0,0,0,0},{2,10,14,0,0,0,0,0},
                                      {3,11,15,0,0,0,0,0},{4,12,16,0,0,0,0,0}};

	unsigned char       lkch2[4][8] = {{1,3,9,11,0,0,0,0},{9,11,0,0,0,0,0,0},
                                        {2,4,10,12,0,0,0,0},{10,12,0,0,0,0,0,0}};
	unsigned char		clch2[12] = {5,6,7,8,17,18,19,20,0,0,0,0};
	//only close right lamp
	unsigned char 		dirch3[8][8] = {{1,3,13,15,0,0,0,0},{5,7,17,19,0,0,0,0},
                                      {2,4,14,16,0,0,0,0},{6,8,18,20,0,0,0,0},
                                      {1,5,13,17,0,0,0,0},{2,6,14,18,0,0,0,0},
                                      {3,7,15,19,0,0,0,0},{4,8,16,20,0,0,0,0}};

	unsigned char       lkch3[4][8] = {{1,3,0,0,0,0,0,0},{5,7,17,19,0,0,0,0},
                                        {2,4,0,0,0,0,0,0},{6,8,18,20,0,0,0,0}};
	unsigned char		clch3[12] = {9,10,11,12,0,0,0,0,0,0,0,0};
	//Not close left and right lamp
	unsigned char		dirch0[8][8] = {{1,3,9,11,13,15,0,0},{5,7,9,11,17,19,0,0},
                                      {2,4,10,12,14,16,0,0},{6,8,10,12,18,20,0,0},
                                      {1,5,9,13,17,0,0,0},{2,6,10,14,18,0,0,0},
                                      {3,7,11,15,19,0,0,0},{4,8,12,16,20,0,0,0}};
	unsigned char       lkch0[4][8] = {{1,3,9,11,0,0,0,0},{5,7,9,11,17,19,0,0},
                                        {2,4,10,12,0,0,0,0},{6,8,10,12,18,20,0,0}};
	unsigned char	dirch[8][8] = {0};
	unsigned char	lkch[4][8] = {0};
	unsigned char	clch[12] = {0};
	#ifdef CLOSE_LAMP
	unsigned char       clch0[12] = {5,6,7,8,9,10,11,12,17,18,19,20};
	z = 0;
	k = 0;
	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 8; j++)
		{
			//left and right lamp
			if ((dirch0[i][j] >= 5) && (dirch0[i][j] <= 12))
			{
				if (*(fcdata->specfunc) & (0x01 << (dirch0[i][j] - 5)))
					continue;
			}
			//turn round lamp
			if ((dirch0[i][j] >= 17) && (dirch0[i][j] <= 20))
			{
				if (*(fcdata->specfunc) & (0x01 << (dirch0[i][j] - 17)))
					continue;
			}
			dirch[z][k] = dirch0[i][j];
			k++;	
		}
		z++;
		k = 0;
	}
	j = 0;
	for (i = 0; i < 12; i++)
	{
		if ((clch0[i] >= 5) && (clch0[i] <= 12))
		{
			if (*(fcdata->specfunc) & (0x01 << (clch0[i] - 5)))
			{
				clch[j] = clch0[i];
				j++;
			}
		}
		if ((clch0[i] >= 17) && (clch0[i] <= 20))
		{
			if (*(fcdata->specfunc) & (0x01 << (clch0[i] - 17)))
			{
				clch[j] = clch0[i];
				j++;
			}
		}
	}

	z = 0;
    k = 0;
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 8; j++)
        {
            //left and right lamp
            if ((lkch0[i][j] >= 5) && (lkch0[i][j] <= 12))
            {
                if (*(fcdata->specfunc) & (0x01 << (lkch0[i][j] - 5)))
                    continue;
            }
            //turn round lamp
            if ((lkch0[i][j] >= 17) && (lkch0[i][j] <= 20))
            {
                if (*(fcdata->specfunc) & (0x01 << (lkch0[i][j] - 17)))
                    continue;
            }
            lkch[z][k] = lkch0[i][j];
            k++;
        }
        z++;
        k = 0;
    }	
	#else
	if ((*(fcdata->specfunc) & 0x10) && (*(fcdata->specfunc) & 0x20))
	{
		for (i = 0; i < 8; i++)
		{
			for (j = 0; j < 8; j++)
			{
				dirch[i][j] = dirch1[i][j];
			}
		}
		for (i = 0; i < 4; i++)
        {
            for (j = 0; j < 8; j++)
            {
                lkch[i][j] = lkch1[i][j];
            }
        }
		for (i = 0; i < 12; i++)
		{
			clch[i] = clch1[i];
		}
	}
	if ((*(fcdata->specfunc) & 0x10) && (!(*(fcdata->specfunc) & 0x20)))
	{
		for (i = 0; i < 8; i++)
        {
            for (j = 0; j < 8; j++)
            {
                dirch[i][j] = dirch2[i][j];
            }
        }
		for (i = 0; i < 4; i++)
        {
            for (j = 0; j < 8; j++)
            {
                lkch[i][j] = lkch2[i][j];
            }
        }
		for (i = 0; i < 12; i++)
		{
			clch[i] = clch2[i];
		}	
	}
	if ((*(fcdata->specfunc) & 0x20) && (!(*(fcdata->specfunc) & 0x10)))
	{
		for (i = 0; i < 8; i++)
        {
            for (j = 0; j < 8; j++)
            {
                dirch[i][j] = dirch3[i][j];
            }
        }
		for (i = 0; i < 4; i++)
        {
            for (j = 0; j < 8; j++)
            {
                lkch[i][j] = lkch3[i][j];
            }
        }
		for (i = 0; i < 12; i++)
        {
            clch[i] = clch3[i];
        }
	}	
	if ((!(*(fcdata->specfunc) & 0x20)) && (!(*(fcdata->specfunc) & 0x10)))
	{
		for (i = 0; i < 8; i++)
		{
			for (j = 0; j < 8; j++)
			{
				dirch[i][j] = dirch0[i][j];
			}
		}
		for (i = 0; i < 4; i++)
		{
			for (j = 0; j < 8; j++)
			{
				lkch[i][j] = lkch0[i][j];
			}
		}
		memset(clch,0,sizeof(clch));
	}
	#endif  

	unsigned char				wltc[10] = {0};
	unsigned char				err[10] = {0};
	unsigned char               wlinfo[13] = {0};
	terminalpp_t                tpp;
    unsigned char               terbuf[11] = {0};
    tpp.head = 0xA9;
    tpp.size = 4;
    tpp.id = *(fcdata->lip);
    tpp.type = 0x04;
    tpp.func[0] = 0;
    tpp.func[1] = 0;
    tpp.func[2] = 0;
    tpp.func[3] = 0;
    tpp.end = 0xED;

	markdata_c					mdt;
	unsigned char               dircon = 0;//mobile direction control
    unsigned char               firdc = 1;//mobile first direction control
    unsigned char               fdirch[MAX_CHANNEL] = {0};//front direction control channel
    unsigned char               fdirn = 0;
    unsigned char               cdirch[MAX_CHANNEL] = {0};//current direction control channel
    unsigned char               cdirn = 0;
	unsigned char				tclc = 0;
	unsigned char				ccon[MAX_CHANNEL] = {0};
	unsigned char               in_phase = 0;
	while (1)
	{//while loop
		FD_ZERO(&nread);
		FD_SET(*(fcdata->conpipe),&nread);
		int	max = *(fcdata->conpipe);

		wtltime.tv_sec = RFIDT;
		wtltime.tv_usec = 0;
		int cpret = select(max+1,&nread,NULL,NULL,&wtltime);
		if (cpret < 0)
		{
			#ifdef INTER_CONTROL_DEBUG
				printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
				output_log("InterGrate control,select call err");
			#endif
			Inter_Control_End_Part_Child_Thread();
			return FAIL;
		}
		if (0 == cpret)
		{//time out
			if (*(fcdata->markbit2) & 0x0100)
                continue; //rfid is controlling
			*(fcdata->markbit2) &= 0xfffe;
//			printf("***********************Time out,File: %s,Line: %d\n",__FILE__,__LINE__);
			if (1 == wtlock)
            {//if (1 == wtlock)
            //    pantn += 1;
            //    if (3 == pantn)
            //    {//wireless terminal has disconnected with signaler machine;
//				printf("***********************Time out,File: %s,Line: %d\n",__FILE__,__LINE__);
				if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
				{
					memset(wlinfo,0,sizeof(wlinfo));
					gettimeofday(&ct,NULL);
					if (SUCCESS != wireless_info_report(wlinfo,ct.tv_sec,fcdata->wlmark,0x20))
					{
					#ifdef INTER_CONTROL_DEBUG
						printf("wireless_info_report,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
					else
					{
						write(*(fcdata->sockfd->csockfd),wlinfo,sizeof(wlinfo));
					}
					update_event_list(fcdata->ec,fcdata->el,1,68,ct.tv_sec,fcdata->markbit);   
				}
				else
				{
					gettimeofday(&ct,NULL);
					update_event_list(fcdata->ec,fcdata->el,1,68,ct.tv_sec,fcdata->markbit);
				}

				wtlock = 0;
				*(fcdata->markbit2) &= 0xfffe;
				dcred = 0;
				close = 0;
				fpc = 0;
				pantn = 0;
				cp = 0;

				if (1 == YellowFlashYes)
				{
					pthread_cancel(YellowFlashPid);
					pthread_join(YellowFlashPid,NULL);
					YellowFlashYes = 0;
				}
						
				if ((dirc >= 0x05) && (dirc <= 0x0c))
				{//direct control happen
					rpc = 0;
					rpi = tscdata->timeconfig->TimeConfigList[rettl][0].PhaseId;
					get_phase_id(rpi,&rpc);

					memset(nc,0,sizeof(nc));
					pcc = nc;
					for (i = 0; i < (tscdata->channel->FactChannelNum); i++)
					{
						if (0 == (tscdata->channel->ChannelList[i].ChannelId))
							break;
						if (rpc == (tscdata->channel->ChannelList[i].ChannelCtrlSrc))
						{
							#ifdef CLOSE_LAMP
							tclc = tscdata->channel->ChannelList[i].ChannelId;
							if ((tclc >= 0x05) && (tclc <= 0x0c))
							{
								if (*(fcdata->specfunc) & (0x01 << (tclc - 5)))
									continue;
							}
							#else
							if ((*(fcdata->specfunc)&0x10)&&(*(fcdata->specfunc)&0x20))
							{
								tclc = tscdata->channel->ChannelList[i].ChannelId;
								if(((5<=tclc)&&(tclc<=8)) || ((9<=tclc)&&(tclc<=12)))
									continue;
							}
							if ((*(fcdata->specfunc)&0x10)&&(!(*(fcdata->specfunc)&0x20)))
							{
								tclc = tscdata->channel->ChannelList[i].ChannelId;
								if ((5 <= tclc) && (tclc <= 8))
									continue;
							}
							if ((*(fcdata->specfunc)&0x20)&&(!(*(fcdata->specfunc)&0x10)))
							{
								tclc = tscdata->channel->ChannelList[i].ChannelId;
								if ((9 <= tclc) && (tclc <= 12))
									continue;
							}
							#endif
							*pcc = tscdata->channel->ChannelList[i].ChannelId;
							pcc++;
						}
					}
					memset(wcc,0,sizeof(wcc));
					memset(wnpcc,0,sizeof(wnpcc));
					memset(wpcc,0,sizeof(wpcc));
					z = 0;
					k = 0;
					s = 0;
					pce = 0;
					for (i = 0; i < MAX_CHANNEL; i++)
					{//for (i = 0; i < MAX_CHANNEL; i++)
						if (0 == dirch[dirc-5][i])
							break;
						ce = 0;
						for (j = 0; j < MAX_CHANNEL; j++)
						{//for (j = 0; j < MAX_CHANNEL; j++)
							if (0 == nc[j])
								break;
							if (dirch[dirc-5][i] == nc[j])
							{
								ce = 1;
								break;
							}
						}//for (j = 0; j < MAX_CHANNEL; j++)
						if (0 == ce)
						{
							wcc[z] = dirch[dirc-5][i];
							z++;
							if ((0x0d <= dirch[dirc-5][i]) && (dirch[dirc-5][i] <= 0x10))
							{
								wpcc[k] = dirch[dirc-5][i];
								k++;
								pce = 1;
								continue;
							}
							else
							{
								wnpcc[s] = dirch[dirc-5][i];
								s++;
								continue;
							}		
						}
					}//for (i = 0; i < MAX_CHANNEL; i++)

					if ((0==wcc[0])&&((stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0)))
					{
						if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
						{
							stStatuSinfo.conmode = 28;
							stStatuSinfo.color = 0x02;
							stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime + stInterGrateControlPhaseInfo.ytime + stInterGrateControlPhaseInfo.rtime;
							stStatuSinfo.stage = 0;
							stStatuSinfo.cyclet = 0;
							stStatuSinfo.phase = 0;
							stStatuSinfo.chans = 0;
							memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
							csta = stStatuSinfo.csta;
							for (i = 0; i < MAX_CHANNEL; i++)
							{
								if (0 == dirch[dirc-5][i])
									break;
								stStatuSinfo.chans += 1;
								tcsta = dirch[dirc-5][i];
								tcsta <<= 2;
								tcsta &= 0xfc;
								tcsta |= 0x02; //00000010-green 
								*csta = tcsta;
								csta++;
							}
							memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
							memset(sibuf,0,sizeof(sibuf));
							if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
							{
							#ifdef INTER_CONTROL_DEBUG
								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
							else
							{
								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
							}
						}
					}//if ((0==wcc[0])&&((stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0)))
					if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.gftime > 0))	
					{			
						if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
						{//report info to server actively
							stStatuSinfo.conmode = 28;
							stStatuSinfo.color = 0x03;
							stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime;
							stStatuSinfo.stage = 0;
							stStatuSinfo.cyclet = 0;
							stStatuSinfo.phase = 0;
														
							for (i = 0; i < MAX_CHANNEL; i++)
							{
								if (0 == wcc[i])
									break;
								for (j = 0; j < stStatuSinfo.chans; j++)
								{
									if (0 == stStatuSinfo.csta[j])
										break;
									tcsta = stStatuSinfo.csta[j];
									tcsta &= 0xfc;
									tcsta >>= 2;
									tcsta &= 0x3f;
									if (wcc[i] == tcsta)
									{
										stStatuSinfo.csta[j] &= 0xfc;
										stStatuSinfo.csta[j] |= 0x03; //00000011-green flash
										break;
									}
								}
							}
							memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
							memset(sibuf,0,sizeof(sibuf));
							if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
							{
							#ifdef INTER_CONTROL_DEBUG
								printf("status pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
							else
							{
								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
							}
						}//report info to server actively
					}//if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.gftime > 0))

					//green flash
					if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
					{
						memset(&dctd,0,sizeof(dctd));
						dctd.mode = 28;//traffic control
						dctd.pattern = *(fcdata->patternid);
						dctd.lampcolor = 0x02;
						dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
						dctd.phaseid = 0;
						dctd.stageline = stInterGrateControlPhaseInfo.stageline;
						if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
						{
						#ifdef INTER_CONTROL_DEBUG
							printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
					}
					#ifdef I_EMBED_CONFIGURE_TOOL
					if (*(fcdata->markbit2) & 0x0200)
					{
						memset(&dctd,0,sizeof(dctd));
                        dctd.mode = 28;//traffic control
                        dctd.pattern = *(fcdata->patternid);
                        dctd.lampcolor = 0x02;
                        dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
                        dctd.phaseid = 0;
                        dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
						if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
						{
						#ifdef INTER_CONTROL_DEBUG
							printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
					}
					#endif
					fbdata[1] = 28;
					fbdata[2] = stInterGrateControlPhaseInfo.stageline;
					fbdata[3] = 0x02;
					fbdata[4] = stInterGrateControlPhaseInfo.gftime;
					if (!wait_write_serial(*(fcdata->fbserial)))
					{
						if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
						{
						#ifdef INTER_CONTROL_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(fcdata->markbit) |= 0x0800;
							gettimeofday(&ct,NULL);
							update_event_list(fcdata->ec,fcdata->el,1,16, \
													ct.tv_sec,fcdata->markbit);
							if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
														fcdata->softevent,fcdata->markbit))
							{
							#ifdef INTER_CONTROL_DEBUG
								printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
						}
					}
					else
					{
					#ifdef INTER_CONTROL_DEBUG
						printf("face board serial port cannot write,Line:%d\n",__LINE__);
					#endif
					}
					if (stInterGrateControlPhaseInfo.gftime > 0)
					{
						ngf = 0;
						while (1)
						{
							if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
							{
							#ifdef INTER_CONTROL_DEBUG
								printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								*(fcdata->markbit) |= 0x0800;
							}
							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																	wcc,0x03,fcdata->markbit))
							{
							#ifdef INTER_CONTROL_DEBUG
								printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
							to.tv_sec = 0;
							to.tv_usec = 500000;
							select(0,NULL,NULL,NULL,&to);
							if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
							{
							#ifdef INTER_CONTROL_DEBUG
								printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
								*(fcdata->markbit) |= 0x0800;
							}
							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																		wcc,0x02,fcdata->markbit))
							{
							#ifdef INTER_CONTROL_DEBUG
								printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
							to.tv_sec = 0;
							to.tv_usec = 500000;
							select(0,NULL,NULL,NULL,&to);
							
							ngf += 1;
							if (ngf >= stInterGrateControlPhaseInfo.gftime)
								break;
						}
					}//if (stInterGrateControlPhaseInfo.gftime > 0)
					if (1 == pce)
					{
					//current phase begin to yellow lamp
						if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
						{
						#ifdef INTER_CONTROL_DEBUG
							printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(fcdata->markbit) |= 0x0800;
						}
						if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																wnpcc,0x01, fcdata->markbit))
						{
						#ifdef INTER_CONTROL_DEBUG
							printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
						if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
						{
						#ifdef INTER_CONTROL_DEBUG
							printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(fcdata->markbit) |= 0x0800;
						}
						if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																wpcc,0x00,fcdata->markbit))
						{
						#ifdef INTER_CONTROL_DEBUG
							printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
					}
					else
					{
						if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
						{
						#ifdef INTER_CONTROL_DEBUG
							printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(fcdata->markbit) |= 0x0800;
						}
						if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																wcc,0x01,fcdata->markbit))
						{
						#ifdef INTER_CONTROL_DEBUG
							printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
					}

					if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.ytime > 0))
					{
						if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
						{//report info to server actively
							stStatuSinfo.conmode = 28;
							stStatuSinfo.color = 0x01;
							stStatuSinfo.time = stInterGrateControlPhaseInfo.ytime;
							stStatuSinfo.stage = 0;
							stStatuSinfo.cyclet = 0;
							stStatuSinfo.phase = 0;
											
							for (i = 0; i < MAX_CHANNEL; i++)
							{
								if (0 == wnpcc[i])
									break;
								for (j = 0; j < stStatuSinfo.chans; j++)
								{
									if (0 == stStatuSinfo.csta[j])
										break;
									tcsta = stStatuSinfo.csta[j];
									tcsta &= 0xfc;
									tcsta >>= 2;
									tcsta &= 0x3f;
									if (wnpcc[i] == tcsta)
									{
										stStatuSinfo.csta[j] &= 0xfc;
										stStatuSinfo.csta[j] |= 0x01; //00000001-yellow
										break;
									}
								}
							}
							for (i = 0; i < MAX_CHANNEL; i++)
							{
								if (0 == wpcc[i])
									break;
								for (j = 0; j < stStatuSinfo.chans; j++)
								{
									if (0 == stStatuSinfo.csta[j])
										break;
									tcsta = stStatuSinfo.csta[j];
									tcsta &= 0xfc;
									tcsta >>= 2;
									tcsta &= 0x3f;
									if (wpcc[i] == tcsta)
									{
										stStatuSinfo.csta[j] &= 0xfc;
										break;
									}
								}
							}
							memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
							memset(sibuf,0,sizeof(sibuf));
							if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
							{
							#ifdef INTER_CONTROL_DEBUG
								printf("status pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
							else
							{
								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
							}
						}//report info to server actively
					}//if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.ytime > 0))

					if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
					{
						memset(&dctd,0,sizeof(dctd));
						dctd.mode = 28;//traffic control
						dctd.pattern = *(fcdata->patternid);
						dctd.lampcolor = 0x01;
						dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
						dctd.phaseid = 0;
						dctd.stageline = stInterGrateControlPhaseInfo.stageline;
						if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
						{
						#ifdef INTER_CONTROL_DEBUG
							printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
					}
					#ifdef I_EMBED_CONFIGURE_TOOL
					if (*(fcdata->markbit2) & 0x0200)
					{
						memset(&dctd,0,sizeof(dctd));
                        dctd.mode = 28;//traffic control
                        dctd.pattern = *(fcdata->patternid);
                        dctd.lampcolor = 0x01;
                        dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                        dctd.phaseid = 0;
                        dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
						if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
						{
						#ifdef INTER_CONTROL_DEBUG
							printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
					}
					#endif
					fbdata[1] = 28;
					fbdata[2] = stInterGrateControlPhaseInfo.stageline;
					fbdata[3] = 0x01;
					fbdata[4] = stInterGrateControlPhaseInfo.ytime;
					if (!wait_write_serial(*(fcdata->fbserial)))
					{
						if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
						{
						#ifdef INTER_CONTROL_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(fcdata->markbit) |= 0x0800;
							gettimeofday(&ct,NULL);
							update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
							if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																	fcdata->softevent,fcdata->markbit))
							{	
							#ifdef INTER_CONTROL_DEBUG
								printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
						}
					}
					else
					{
					#ifdef INTER_CONTROL_DEBUG
						printf("face board serial port cannot write,Line:%d\n",__LINE__);
					#endif
					}
					sleep(stInterGrateControlPhaseInfo.ytime);

					//current phase begin to red lamp
					if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
					{
					#ifdef INTER_CONTROL_DEBUG
						printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
						*(fcdata->markbit) |= 0x0800;
					}
					if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																		wcc,0x00,fcdata->markbit))
					{
					#ifdef INTER_CONTROL_DEBUG
						printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}

					if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.rtime > 0))
					{
						if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
						{//report info to server actively
							stStatuSinfo.conmode = 28;
							stStatuSinfo.color = 0x00;
							stStatuSinfo.time = stInterGrateControlPhaseInfo.rtime;
							stStatuSinfo.stage = 0;
							stStatuSinfo.cyclet = 0;
							stStatuSinfo.phase = 0;
														
							for (i = 0; i < MAX_CHANNEL; i++)
							{
								if (0 == wcc[i])
									break;
								for (j = 0; j < stStatuSinfo.chans; j++)
								{
									if (0 == stStatuSinfo.csta[j])
										break;
									tcsta = stStatuSinfo.csta[j];
									tcsta &= 0xfc;
									tcsta >>= 2;
									tcsta &= 0x3f;
									if (wcc[i] == tcsta)
									{
										stStatuSinfo.csta[j] &= 0xfc;
										break;
									}
								}
							}
							memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
							memset(sibuf,0,sizeof(sibuf));
							if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
							{
							#ifdef INTER_CONTROL_DEBUG
								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
							else
							{
								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
							}
						}//report info to server actively
					}//if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.rtime > 0))
					*(fcdata->slnum) = 0;
					*(fcdata->stageid) = tscdata->timeconfig->TimeConfigList[rettl][0].StageId;
				}//if (0 != directc)
				new_all_red(&ardata);

				*(fcdata->contmode) = contmode; //restore control mode;
				cp = 0;
				dirc = 0;
				*(fcdata->markbit2) &= 0xfffb;
				if (0 == InterGratedControlYes)
				{
					memset(&stInterGrateControlData,0,sizeof(stInterGrateControlData));
					memset(&stInterGrateControlPhaseInfo,0,sizeof(stInterGrateControlPhaseInfo));
					stInterGrateControlData.fd = fcdata;
					stInterGrateControlData.td = tscdata;
					stInterGrateControlData.pi = &stInterGrateControlPhaseInfo;
					stInterGrateControlData.cs = chanstatus;
					int dcret = pthread_create(&InterGratedControlPid,NULL,(void *)Start_Inter_Grate_Control,&stInterGrateControlData);
					if (0 != dcret)
					{
					#ifdef INTER_CONTROL_DEBUG
						printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
						output_log("InterGrate control,create full InterGrate thread err");
					#endif
						return FAIL;
					}
					InterGratedControlYes = 1;
				}
				continue;
            //    }//wireless terminal has disconnected with signaler machine;
            }//if (1 == wtlock)
            continue;
		}//time out
		if (cpret > 0)
		{//cpret > 0
			if (FD_ISSET(*(fcdata->conpipe),&nread))
			{
				memset(tcbuf,0,sizeof(tcbuf));
				read(*(fcdata->conpipe),tcbuf,sizeof(tcbuf));
				if ((0xB9==tcbuf[0]) && ((0xED==tcbuf[8]) || (0xED==tcbuf[4]) || (0xED==tcbuf[5])))
                {//wireless terminal control
					pantn = 0;
                    if (0x02 == tcbuf[3])
                    {//pant
				//		printf("Receive pant of main module,File: %s,LIne: %d\n",__FILE__,__LINE__);
                        continue;
                    }//pant
                    if ((0 == wllock) && (0 == keylock) && (0 == netlock))
                    {//terminal control is valid only when wireless and key and net control is invalid;
						if (0x04 == tcbuf[3])
                        {//control function
                            if ((0x01 == tcbuf[4]) && (0 == wtlock))
                            {//control will happen
								Get_Inter_Greate_Control_Status(&color,&leatime);
								if (0x02 != color)
								{
									struct timeval          spacetime;
									unsigned char           endlock = 0;
									
									while (1)
									{//while loop,000000
										spacetime.tv_sec = 0;
										spacetime.tv_usec = 200000;
										select(0,NULL,NULL,NULL,&spacetime);

										memset(tcbuf,0,sizeof(tcbuf));
										read(*(fcdata->conpipe),tcbuf,sizeof(tcbuf));
										if ((0xB9 == tcbuf[0]) && (0xED == tcbuf[5]))
										{
											if (0x00 == tcbuf[4])
											{
												endlock = 1;
												break;
											}
										}
										
										Get_Inter_Greate_Control_Status(&color,&leatime);
										if (0x02 == color)
										{
											break;
										}
										else
										{
											continue;
										}
									}//while loop,000000
									if (1 == endlock)
									{
										continue;
									}
								}//if (0x02 != color)
								tpp.func[0] = tcbuf[4];
                                memset(terbuf,0,sizeof(terbuf));
                                if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
                                {
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),terbuf,sizeof(terbuf)) < 0)
                                        {
                                        #ifdef INTER_CONTROL_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
										else
										{
											#ifdef INTER_CONTROL_DEBUG   
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
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))	

								wtlock = 1;
								dcred = 0;
								close = 0;
								fpc = 0;
								cp = 0;
								dirc = 0;
								Inter_Control_End_Child_Thread();//end main thread and its child thread

								*(fcdata->markbit2) |= 0x0004;
								*(fcdata->contmode) = 28;//wireless terminal control mode

								if (*(fcdata->auxfunc) & 0x01)
                                {//if (*(fcdata->auxfunc) & 0x01)
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),dcdownti,sizeof(dcdownti)) < 0)
                                        {
                                        #ifdef INTER_CONTROL_DEBUG
                                            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),dcedownti,sizeof(dcedownti)) < 0)
                                        {
                                        #ifdef INTER_CONTROL_DEBUG
                                            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (*(fcdata->auxfunc) & 0x01)

								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									stStatuSinfo.conmode = 28;
									if(0 == bAllLampClose)
									{
										stStatuSinfo.color = 0x02;
									}
									else
									{
										stStatuSinfo.color = 0x04;
									}
									stStatuSinfo.time = 0;
									stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
									stStatuSinfo.cyclet = 0;
									stStatuSinfo.phase = 0;
									stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
															
									stStatuSinfo.chans = 0;
            						memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            						csta = stStatuSinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						if (0 == stInterGrateControlPhaseInfo.chan[i])
                    						break;
                						stStatuSinfo.chans += 1;
                						tcsta = stInterGrateControlPhaseInfo.chan[i];
                						tcsta <<= 2;
                						tcsta &= 0xfc;
										if(0 == bAllLampClose)
										{
										tcsta |= 0x02; //00000010-green
										}	
                						*csta = tcsta;
                						csta++;
            						}
									memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            						{
            						#ifdef INTER_CONTROL_DEBUG
                						printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
									memset(wlinfo,0,sizeof(wlinfo));
                                    gettimeofday(&ct,NULL);
                                    if (SUCCESS != wireless_info_report(wlinfo,ct.tv_sec,fcdata->wlmark,0x04))
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("wireless_info_report,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                    else
                                    {
                                        write(*(fcdata->sockfd->csockfd),wlinfo,sizeof(wlinfo));
                                    }
									update_event_list(fcdata->ec,fcdata->el,1,40,ct.tv_sec,fcdata->markbit);
								}//report info to server actively
								else
								{
									gettimeofday(&ct,NULL);
                        			update_event_list(fcdata->ec,fcdata->el,1,40,ct.tv_sec,fcdata->markbit);
								}
						
								fbdata[1] = 28;
								fbdata[2] = stInterGrateControlPhaseInfo.stageline;
								fbdata[3] = 0x02;
								fbdata[4] = 0;
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
									{
										#ifdef INTER_CONTROL_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
                						update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                						if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																fcdata->softevent,fcdata->markbit))
                						{
                						#ifdef INTER_CONTROL_DEBUG
                   							printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                						#endif
                						}
									}
								}
								else
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("face board serial port cannot write,Line:%d\n",__LINE__);
								#endif
								}

								//send down time to configure tool
                            	if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            	{
								#ifdef INTER_CONTROL_DEBUG
									printf("send control info to configure tool,File: %s,Line: %d\n", \
											__FILE__,__LINE__);
								#endif
                                	memset(&dctd,0,sizeof(dctd));
                                	dctd.mode = 28;
                                	dctd.pattern = *(fcdata->patternid);
									if(0 == bAllLampClose)
									{
										dctd.lampcolor = 0x02;
									}
									else
									{
										dctd.lampcolor = 0x03;
									}
                                	dctd.lamptime = 0;
                                	dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                	dctd.stageline = stInterGrateControlPhaseInfo.stageline;
                                	if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
                                	{
                                	#ifdef INTER_CONTROL_DEBUG
                                    	printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                	#endif
                                	}
                            	}
								#ifdef I_EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
								{
									memset(&dctd,0,sizeof(dctd));
                                    dctd.mode = 28;
                                    dctd.pattern = *(fcdata->patternid);
                                    if(0 == bAllLampClose)
									{
										dctd.lampcolor = 0x02;
									}
									else
									{
										dctd.lampcolor = 0x03;
									}
                                    dctd.lamptime = 0;
                                    dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                    dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
									if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#endif
								if(0 == bAllLampClose)
								{
	                            	if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
	                                    				stInterGrateControlPhaseInfo.chan,0x02,fcdata->markbit))
	                            	{
	                            	#ifdef INTER_CONTROL_DEBUG
	                                	printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
	                            	#endif
	                            	}
								}
								else
								{
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
	                                    				stInterGrateControlPhaseInfo.chan,0x03,fcdata->markbit))
	                            	{
	                            	#ifdef INTER_CONTROL_DEBUG
	                                	printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
	                            	#endif
	                            	}
								}
								continue;
                            }//control will happen
                            else if ((0x00 == tcbuf[4]) && (1 == wtlock))
                            {//cancel control
								wtlock = 0;
								dcred = 0;
								close = 0;
								fpc = 0;
								cp = 0;

								tpp.func[0] = tcbuf[4];
                                memset(terbuf,0,sizeof(terbuf));
                                if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
                                {
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),terbuf,sizeof(terbuf)) < 0)
                                        {
                                        #ifdef INTER_CONTROL_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
										else
										{
										#ifdef INTER_CONTROL_DEBUG   
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
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
								if (1 == YellowFlashYes)
								{
									pthread_cancel(YellowFlashPid);
									pthread_join(YellowFlashPid,NULL);
									YellowFlashYes = 0;
								}
								if(1 == bAllLampClose)
								{
									friststartcontrol = 1;
								}
								if ((dirc >= 5) && (dirc <= 0x0c))
								{//direction control happen
									rpc = 0;
                                	rpi = tscdata->timeconfig->TimeConfigList[rettl][0].PhaseId;
                                	get_phase_id(rpi,&rpc);

									memset(nc,0,sizeof(nc));
                                    pcc = nc;
                                    for (i = 0; i < (tscdata->channel->FactChannelNum); i++)
                                    {
                                        if (0 == (tscdata->channel->ChannelList[i].ChannelId))
                                            break;
                                        if (rpc == (tscdata->channel->ChannelList[i].ChannelCtrlSrc))
                                        {
											#ifdef CLOSE_LAMP
											tclc = tscdata->channel->ChannelList[i].ChannelId;
											if ((tclc >= 0x05) && (tclc <= 0x0c))
											{
												if (*(fcdata->specfunc) & (0x01 << (tclc - 5)))
													continue;
											}
											#else
											if ((*(fcdata->specfunc)&0x10)&&(*(fcdata->specfunc)&0x20))
											{
												tclc = tscdata->channel->ChannelList[i].ChannelId;
												if(((5<=tclc)&&(tclc<=8)) || ((9<=tclc)&&(tclc<=12)))
													continue;
											}
											if ((*(fcdata->specfunc)&0x10)&&(!(*(fcdata->specfunc)&0x20)))
											{
												tclc = tscdata->channel->ChannelList[i].ChannelId;
												if ((5 <= tclc) && (tclc <= 8))
													continue;
											}
											if ((*(fcdata->specfunc)&0x20)&&(!(*(fcdata->specfunc)&0x10)))
											{
												tclc = tscdata->channel->ChannelList[i].ChannelId;
												if ((9 <= tclc) && (tclc <= 12))
													continue;
											}
											#endif
                                            *pcc = tscdata->channel->ChannelList[i].ChannelId;
                                            pcc++;
                                        }
                                    }
									memset(wcc,0,sizeof(wcc));
									memset(wnpcc,0,sizeof(wnpcc));
									memset(wpcc,0,sizeof(wpcc));
									z = 0;
									k = 0;
									s = 0;
									pce = 0;
									for (i = 0; i < MAX_CHANNEL; i++)
									{//for (i = 0; i < MAX_CHANNEL; i++)
										if (0 == dirch[dirc-5][i])
											break;
										ce = 0;
										for (j = 0; j < MAX_CHANNEL; j++)
										{//for (j = 0; j < MAX_CHANNEL; j++)
											if (0 == nc[j])
												break;
											if (dirch[dirc-5][i] == nc[j])
											{
												ce = 1;
												break;
											}
										}//for (j = 0; j < MAX_CHANNEL; j++)
										if (0 == ce)
										{
											wcc[z] = dirch[dirc-5][i];
											z++;
											if ((0x0d <= dirch[dirc-5][i]) && (dirch[dirc-5][i] <= 0x10))
											{
												wpcc[k] = dirch[dirc-5][i];
												k++;
												pce = 1;
												continue;
											}
											else
											{
												wnpcc[s] = dirch[dirc-5][i];
												s++;
												continue;
											}		
										}
									}//for (i = 0; i < MAX_CHANNEL; i++)

									if ((0==wcc[0])&&((stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0)))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{
											stStatuSinfo.conmode = 28;
											stStatuSinfo.color = 0x02;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime + stInterGrateControlPhaseInfo.ytime + stInterGrateControlPhaseInfo.rtime;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
											stStatuSinfo.chans = 0;
            								memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            								csta = stStatuSinfo.csta;
            								for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == dirch[dirc-5][i])
                    								break;
                								stStatuSinfo.chans += 1;
                								tcsta = dirch[dirc-5][i];
                								tcsta <<= 2;
                								tcsta &= 0xfc;
                								tcsta |= 0x02; //00000010-green 
                								*csta = tcsta;
                								csta++;
            								}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
											memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}
									}//if ((0==wcc[0])&&((stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0)))
									if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.gftime > 0))	
									{			
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = 28;
											stStatuSinfo.color = 0x03;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
															
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == wcc[i])
                    								break;
                								for (j = 0; j < stStatuSinfo.chans; j++)
                								{
                    								if (0 == stStatuSinfo.csta[j])
                        								break;
                    								tcsta = stStatuSinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (wcc[i] == tcsta)
                    								{
                        								stStatuSinfo.csta[j] &= 0xfc;
														stStatuSinfo.csta[j] |= 0x03; //00000011-green flash
														break;
                    								}
                								}
            								}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("status pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.gftime > 0))
	
									//green flash
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&dctd,0,sizeof(dctd));
										dctd.mode = 28;//traffic control
										dctd.pattern = *(fcdata->patternid);
										dctd.lampcolor = 0x02;
										dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
										dctd.phaseid = 0;
										dctd.stageline = stInterGrateControlPhaseInfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 28;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x02;
                                        dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
                                        dctd.phaseid = 0;
                                        dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = stInterGrateControlPhaseInfo.gftime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef INTER_CONTROL_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef INTER_CONTROL_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									if (stInterGrateControlPhaseInfo.gftime > 0)
									{
										ngf = 0;
										while (1)
										{
											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x03,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);
									
											ngf += 1;
											if (ngf >= stInterGrateControlPhaseInfo.gftime)
												break;
										}
									}//if (stInterGrateControlPhaseInfo.gftime > 0)
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef INTER_CONTROL_DEBUG
            								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}

									if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.ytime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = 28;
											stStatuSinfo.color = 0x01;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.ytime;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
															
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == wnpcc[i])
                    								break;
                								for (j = 0; j < stStatuSinfo.chans; j++)
                								{
                    								if (0 == stStatuSinfo.csta[j])
                        								break;
                    								tcsta = stStatuSinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (wnpcc[i] == tcsta)
                    								{
                        								stStatuSinfo.csta[j] &= 0xfc;
														stStatuSinfo.csta[j] |= 0x01; //00000001-yellow
														break;
                    								}
                								}
            								}
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == wpcc[i])
                                                	break;
                                            	for (j = 0; j < stStatuSinfo.chans; j++)
                                            	{
                                                	if (0 == stStatuSinfo.csta[j])
                                                    	break;
                                                	tcsta = stStatuSinfo.csta[j];
                                                	tcsta &= 0xfc;
                                                	tcsta >>= 2;
                                                	tcsta &= 0x3f;
                                                	if (wpcc[i] == tcsta)
                                                	{
                                                    	stStatuSinfo.csta[j] &= 0xfc;
														break;
                                                	}
                                            	}
                                        	}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("status pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.ytime > 0))

									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            		{
                            			memset(&dctd,0,sizeof(dctd));
                            			dctd.mode = 28;//traffic control
                            			dctd.pattern = *(fcdata->patternid);
                            			dctd.lampcolor = 0x01;
                            			dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                            			dctd.phaseid = 0;
                            			dctd.stageline = stInterGrateControlPhaseInfo.stageline;
                            			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
                            			{
                            			#ifdef INTER_CONTROL_DEBUG
                               				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            			#endif
                            			}
                            		}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 28;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x01;
                                        dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                                        dctd.phaseid = 0;
                                        dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            		fbdata[3] = 0x01;
                            		fbdata[4] = stInterGrateControlPhaseInfo.ytime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                            			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                            			{
                            			#ifdef INTER_CONTROL_DEBUG
                               				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                			{
                                			#ifdef INTER_CONTROL_DEBUG
                                   				printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(stInterGrateControlPhaseInfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                                		printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}

									if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.rtime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = 28;
											stStatuSinfo.color = 0x00;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.rtime;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
															
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == wcc[i])
                                                	break;
                                            	for (j = 0; j < stStatuSinfo.chans; j++)
                                            	{
                                                	if (0 == stStatuSinfo.csta[j])
                                                    	break;
                                                	tcsta = stStatuSinfo.csta[j];
                                                	tcsta &= 0xfc;
                                                	tcsta >>= 2;
                                                	tcsta &= 0x3f;
                                                	if (wcc[i] == tcsta)
                                                	{
                                                    	stStatuSinfo.csta[j] &= 0xfc;
														break;
                                                	}
                                            	}
                                        	}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.rtime > 0))
									*(fcdata->slnum) = 0;
                                    *(fcdata->stageid) = tscdata->timeconfig->TimeConfigList[rettl][0].StageId;
								}//direction control happen
								
								new_all_red(&ardata);

								*(fcdata->contmode) = contmode; //restore control mode;
								cp = 0;
								dirc = 0;
								*(fcdata->markbit2) &= 0xfffb;
								if (0 == InterGratedControlYes)
								{
									memset(&stInterGrateControlData,0,sizeof(stInterGrateControlData));
									memset(&stInterGrateControlPhaseInfo,0,sizeof(stInterGrateControlPhaseInfo));
									stInterGrateControlData.fd = fcdata;
									stInterGrateControlData.td = tscdata;
									stInterGrateControlData.pi = &stInterGrateControlPhaseInfo;
									stInterGrateControlData.cs = chanstatus;
									int dcret = pthread_create(&InterGratedControlPid,NULL, \
													(void *)Start_Inter_Grate_Control,&stInterGrateControlData);
									if (0 != dcret)
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
										output_log("InterGrate control,create full InterGrate thread err");
									#endif
										return FAIL;
									}
									InterGratedControlYes = 1;
								}
								if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
								{
									memset(wlinfo,0,sizeof(wlinfo));
                                    gettimeofday(&ct,NULL);
                                    if (SUCCESS != wireless_info_report(wlinfo,ct.tv_sec,fcdata->wlmark,0x06))
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("wireless_info_report,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                    else
                                    {
                                        write(*(fcdata->sockfd->csockfd),wlinfo,sizeof(wlinfo));
                                    }
									update_event_list(fcdata->ec,fcdata->el,1,42,ct.tv_sec,fcdata->markbit);
								}
								else
								{
									gettimeofday(&ct,NULL);
                        			update_event_list(fcdata->ec,fcdata->el,1,42,ct.tv_sec,fcdata->markbit);
								}

								continue;
                            }//cancel control
                            else if (((0x05 <= tcbuf[4]) && (tcbuf[4] <= 0x0c)) && (1 == wtlock))
                            {//direction control
								tpp.func[0] = tcbuf[4];
                                memset(terbuf,0,sizeof(terbuf));
                                if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
                                {
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),terbuf,sizeof(terbuf)) < 0)
                                        {
                                        #ifdef INTER_CONTROL_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
								if ((1 == dcred) || (1 == YellowFlashYes) || (1 == close))
                            	{//if ((1 == tcred) || (1 == tcyfyes) || (1 == close))
									dirc = tcbuf[4];
									if ((dirc < 5) && (dirc > 0x0c))
                                    {
                                        continue;
                                    }
									fpc = 1;
									cp = 0;
                                	if (1 == YellowFlashYes)
                                	{
                                    	pthread_cancel(YellowFlashPid);
                                    	pthread_join(YellowFlashPid,NULL);
                                    	YellowFlashYes = 0;
                                	}
									if (1 != dcred)
									{
										new_all_red(&ardata);
									}
									dcred = 0;
                                    close = 0;
			//						#ifdef CLOSE_LAMP
									igc_set_lamp_color(*(fcdata->bbserial),clch,0x03);
									update_channel_status(fcdata->sockfd,chanstatus,clch,0x03,fcdata->markbit);
			//						#endif
                                	if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),dirch[dirc-5],0x02))
                                	{
                                	#ifdef INTER_CONTROL_DEBUG
                                    	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
                                    	*(fcdata->markbit) |= 0x0800;
                                	}
                                	if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                                        				dirch[dirc-5],0x02,fcdata->markbit))
                                	{
                                	#ifdef INTER_CONTROL_DEBUG
                                    	printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                	#endif
                                	}
						
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										stStatuSinfo.conmode = 28;
										stStatuSinfo.color = 0x02;
										stStatuSinfo.time = 0;
										stStatuSinfo.stage = 0;
										stStatuSinfo.cyclet = 0;
										stStatuSinfo.phase = 0;
								/*	
										stStatuSinfo.chans = 0;
            							memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            							csta = stStatuSinfo.csta;
            							for (i = 0; i < MAX_CHANNEL; i++)
            							{
                							stStatuSinfo.chans += 1;
                							tcsta = i + 1;
                							tcsta <<= 2;
                							tcsta &= 0xfc;
											for (j = 0; j < MAX_CHANNEL; j++)
											{
												if (0 == dirch[dirc-5][j])
													break;
												if ((i+1) == dirch[dirc-5][j])
												{
													tcsta |= 0x02;
													break;
												}
											}
                							*csta = tcsta;
                							csta++;
            							}
								*/
										stStatuSinfo.chans = 0;
										memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
                                        csta = stStatuSinfo.csta;
                                        for (i = 0; i < MAX_CHANNEL; i++)
                                        {
                                            if (0 == dirch[dirc-5][i])
                                                break;
                                            stStatuSinfo.chans += 1;
                                            tcsta = dirch[dirc-5][i];
                                            tcsta <<= 2;
                                            tcsta &= 0xfc;
                                            tcsta |= 0x02; //00000010-green 
                                            *csta = tcsta;
                                            csta++;
                                        }
										memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            							memset(sibuf,0,sizeof(sibuf));
            							if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            							{
            							#ifdef INTER_CONTROL_DEBUG
                							printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            							#endif
            							}
            							else
            							{
                							write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            							}
									}//report info to server actively
									unsigned char				dce = 0;
									unsigned char				val = 0;
									if (0x05 == tcbuf[4])
									{
										dce = 0x10;
										val = 52;
									}
									else if (0x06 == tcbuf[4])
									{
										dce = 0x12;
										val = 54;
									}
									else if (0x07 == tcbuf[4])
									{
										dce = 0x14;
										val = 56;
									}
									else if (0x08 == tcbuf[4])
									{
                                    	dce = 0x16;
										val = 58;
									}
									else if (0x09 == tcbuf[4])
									{
                                    	dce = 0x18;
										val = 60;
									}
									else if (0x0a == tcbuf[4])
									{
                                    	dce = 0x1a;
										val = 62;
									}
									else if (0x0b == tcbuf[4])
									{
                                    	dce = 0x1c;
										val = 64;
									}
									else if (0x0c == tcbuf[4])
									{
                                    	dce = 0x1e;
										val = 66;
									}
									if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
                                	{
                                    	memset(wlinfo,0,sizeof(wlinfo));
                                    	gettimeofday(&ct,NULL);
                                    	if(SUCCESS!=wireless_info_report(wlinfo,ct.tv_sec,fcdata->wlmark,dce))
                                    	{
                                    	#ifdef INTER_CONTROL_DEBUG
                                        	printf("wireless_info_report,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                    	else
                                    	{
                                        	write(*(fcdata->sockfd->csockfd),wlinfo,sizeof(wlinfo));
                                    	}
									  update_event_list(fcdata->ec,fcdata->el,1,val,ct.tv_sec,fcdata->markbit);
                                	}
									else
									{
										gettimeofday(&ct,NULL);
                        			  update_event_list(fcdata->ec,fcdata->el,1,val,ct.tv_sec,fcdata->markbit);
									}

                                	continue;
                            	}//if ((1 == dcred) || (1 == YellowFlashYes) || (1 == close))
								if (0 == fpc)
								{//if (0 == fpc)
									fpc = 1;
									cp = stInterGrateControlPhaseInfo.phaseid;
									memset(cc,0,sizeof(cc));
                        			pcc = cc;
                        			for (i = 0; i < (tscdata->channel->FactChannelNum); i++)
                        			{
                    	    			if (0 == (tscdata->channel->ChannelList[i].ChannelId))
                        	    			break;
                            			if (cp == (tscdata->channel->ChannelList[i].ChannelCtrlSrc))
                            			{
											#ifdef CLOSE_LAMP
											tclc = tscdata->channel->ChannelList[i].ChannelId;
											if ((tclc >= 0x05) && (tclc <= 0x0c))
											{
												if (*(fcdata->specfunc) & (0x01 << (tclc - 5)))
													continue;
											}
											#else
											if ((*(fcdata->specfunc)&0x10)&&(*(fcdata->specfunc)&0x20))
											{
												tclc = tscdata->channel->ChannelList[i].ChannelId;
												if(((5<=tclc)&&(tclc<=8)) || ((9<=tclc)&&(tclc<=12)))
													continue;
											}
											if ((*(fcdata->specfunc)&0x10)&&(!(*(fcdata->specfunc)&0x20)))
											{
												tclc = tscdata->channel->ChannelList[i].ChannelId;
												if ((5 <= tclc) && (tclc <= 8))
													continue;
											}
											if ((*(fcdata->specfunc)&0x20)&&(!(*(fcdata->specfunc)&0x10)))
											{
												tclc = tscdata->channel->ChannelList[i].ChannelId;
												if ((9 <= tclc) && (tclc <= 12))
													continue;
											}
											#endif
                            				*pcc = tscdata->channel->ChannelList[i].ChannelId;
                                			pcc++;
                            			}
                        			}
									dirc = tcbuf[4];
									memset(wcc,0,sizeof(wcc));
									memset(wnpcc,0,sizeof(wnpcc));
									memset(wpcc,0,sizeof(wpcc));
									z = 0;
									k = 0;
									s = 0;
									pce = 0;
									for (i = 0; i < MAX_CHANNEL; i++)
									{//for (i = 0; i < MAX_CHANNEL; i++)
										if (0 == cc[i])
											break;
										ce = 0;
										for (j = 0; j < MAX_CHANNEL; j++)
										{//for (j = 0; j < MAX_CHANNEL; j++)
											if (0 == dirch[dirc-5][j])
												break;
											if (cc[i] == dirch[dirc-5][j])
											{
												ce = 1;
												break;
											}
										}//for (j = 0; j < MAX_CHANNEL; j++)
										if (0 == ce)
										{
											wcc[z] = cc[i];
											z++;
											if ((0x0d <= cc[i]) && (cc[i] <= 0x10))
											{
												wpcc[k] = cc[i];
												k++;
												pce = 1;
												continue;
											}
											else
											{
												wnpcc[s] = cc[i];
												s++;
												continue;
											}		
										}
									}//for (i = 0; i < MAX_CHANNEL; i++)
									if ((0==wcc[0])&&((stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0)))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{
											stStatuSinfo.conmode = 28;
											stStatuSinfo.color = 0x02;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime + stInterGrateControlPhaseInfo.ytime + stInterGrateControlPhaseInfo.rtime;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
											stStatuSinfo.phase |= (0x01 << (cp - 1));
											stStatuSinfo.chans = 0;
            								memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            								csta = stStatuSinfo.csta;
            								for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == cc[i])
                    								break;
                								stStatuSinfo.chans += 1;
                								tcsta = cc[i];
                								tcsta <<= 2;
                								tcsta &= 0xfc;
                								tcsta |= 0x02; //00000010-green 
                								*csta = tcsta;
                								csta++;
            								}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
											memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}
									}//if ((0==wcc[0])&&((stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0)))
									if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.gftime > 0))	
									{			
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = 28;
											stStatuSinfo.color = 0x03;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
											stStatuSinfo.phase |= (0x01 << (cp - 1));
	
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == wcc[i])
                    								break;
                								for (j = 0; j < stStatuSinfo.chans; j++)
                								{
                    								if (0 == stStatuSinfo.csta[j])
                        								break;
                    								tcsta = stStatuSinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (wcc[i] == tcsta)
                    								{
                        								stStatuSinfo.csta[j] &= 0xfc;
														stStatuSinfo.csta[j] |= 0x03; //00000011-green flash
														break;
                    								}
                								}
            								}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("status pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.gftime > 0))
	
									//green flash
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&dctd,0,sizeof(dctd));
										dctd.mode = 28;//traffic control
										dctd.pattern = *(fcdata->patternid);
										dctd.lampcolor = 0x02;
										dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
										dctd.phaseid = cp;
										dctd.stageline = stInterGrateControlPhaseInfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 28;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x02;
                                        dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
                                        dctd.phaseid = cp;
                                        dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = stInterGrateControlPhaseInfo.gftime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef INTER_CONTROL_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef INTER_CONTROL_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									if (stInterGrateControlPhaseInfo.gftime > 0)
									{
										ngf = 0;
										while (1)
										{
											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x03,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);
									
											ngf += 1;
											if (ngf >= stInterGrateControlPhaseInfo.gftime)
												break;
										}
									}//if (stInterGrateControlPhaseInfo.gftime > 0)
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef INTER_CONTROL_DEBUG
            								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}

									if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.ytime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = 28;
											stStatuSinfo.color = 0x01;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.ytime;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
											stStatuSinfo.phase |= (0x01 << (cp - 1));
				
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == wnpcc[i])
                    								break;
                								for (j = 0; j < stStatuSinfo.chans; j++)
                								{
                    								if (0 == stStatuSinfo.csta[j])
                        								break;
                    								tcsta = stStatuSinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (wnpcc[i] == tcsta)
                    								{
                        								stStatuSinfo.csta[j] &= 0xfc;
														stStatuSinfo.csta[j] |= 0x01; //00000001-yellow
														break;
                    								}
                								}
            								}
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == wpcc[i])
                                                	break;
                                            	for (j = 0; j < stStatuSinfo.chans; j++)
                                            	{
                                                	if (0 == stStatuSinfo.csta[j])
                                                    	break;
                                                	tcsta = stStatuSinfo.csta[j];
                                                	tcsta &= 0xfc;
                                                	tcsta >>= 2;
                                                	tcsta &= 0x3f;
                                                	if (wpcc[i] == tcsta)
                                                	{
                                                    	stStatuSinfo.csta[j] &= 0xfc;
														break;
                                                	}
                                            	}
                                        	}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("status pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.ytime > 0))

									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            		{
                            			memset(&dctd,0,sizeof(dctd));
                            			dctd.mode = 28;//traffic control
                            			dctd.pattern = *(fcdata->patternid);
                            			dctd.lampcolor = 0x01;
                            			dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                            			dctd.phaseid = cp;
                            			dctd.stageline = stInterGrateControlPhaseInfo.stageline;
                            			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
                            			{
                            			#ifdef INTER_CONTROL_DEBUG
                               				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            			#endif
                            			}
                            		}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 28;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x01;
                                        dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                                        dctd.phaseid = cp;
                                        dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            		fbdata[3] = 0x01;
                            		fbdata[4] = stInterGrateControlPhaseInfo.ytime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                            			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                            			{
                            			#ifdef INTER_CONTROL_DEBUG
                               				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                			{
                                			#ifdef INTER_CONTROL_DEBUG
                                   				printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(stInterGrateControlPhaseInfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                                		printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}

									if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.rtime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = 28;
											stStatuSinfo.color = 0x00;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.rtime;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
											stStatuSinfo.phase |= (0x01 << (cp - 1));
	
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == wcc[i])
                                                	break;
                                            	for (j = 0; j < stStatuSinfo.chans; j++)
                                            	{
                                                	if (0 == stStatuSinfo.csta[j])
                                                    	break;
                                                	tcsta = stStatuSinfo.csta[j];
                                                	tcsta &= 0xfc;
                                                	tcsta >>= 2;
                                                	tcsta &= 0x3f;
                                                	if (wcc[i] == tcsta)
                                                	{
                                                    	stStatuSinfo.csta[j] &= 0xfc;
														break;
                                                	}
                                            	}
                                        	}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.rtime > 0))									
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),dirch[dirc-5],0x02))
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																		dirch[dirc-5],0x02,fcdata->markbit))
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
                            		}
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										stStatuSinfo.conmode = 28;
										stStatuSinfo.color = 0x02;
										stStatuSinfo.time = 0;
										stStatuSinfo.stage = 0;
										stStatuSinfo.cyclet = 0;
										stStatuSinfo.phase = 0;

										stStatuSinfo.chans = 0;
            							memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            							csta = stStatuSinfo.csta;
            							for (i = 0; i < MAX_CHANNEL; i++)
            							{
                							if (0 == dirch[dirc-5][i])
                    							break;
                							stStatuSinfo.chans += 1;
                							tcsta = dirch[dirc-5][i];
                							tcsta <<= 2;
                							tcsta &= 0xfc;
                							tcsta |= 0x02;
                							*csta = tcsta;
                							csta++;
            							}
										memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            							memset(sibuf,0,sizeof(sibuf));
            							if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            							{
            							#ifdef INTER_CONTROL_DEBUG
                							printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            							#endif
            							}
            							else
            							{
                							write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            							}
									}//report info to server actively									
	
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&dctd,0,sizeof(dctd));
										dctd.mode = 28;//traffic control
										dctd.pattern = *(fcdata->patternid);
										dctd.lampcolor = 0x02;
										dctd.lamptime = 0;
										dctd.phaseid = 0;
										dctd.stageline = 0;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 28;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x02;
                                        dctd.lamptime = 0;
                                        dctd.phaseid = 0;
                                        dctd.stageline = 0;		
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = 0;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef INTER_CONTROL_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef INTER_CONTROL_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									cp = 0;
								}//if (0 == fpc)
								else if (1 == fpc)
								{//else if (1 == fpc)
									if (dirc == tcbuf[4])
									{//control is current phase
										continue;
									}//control is current phase
									unsigned char		*pcc = dirch[dirc-5];
									dirc = tcbuf[4];
									unsigned char		*pnc = dirch[dirc-5];

									memset(wcc,0,sizeof(wcc));
									memset(wnpcc,0,sizeof(wnpcc));
									memset(wpcc,0,sizeof(wpcc));
									z = 0;
									k = 0;
									s = 0;
									pce = 0;
									for (i = 0; i < MAX_CHANNEL; i++)
									{//for (i = 0; i < MAX_CHANNEL; i++)
										if (0 == pcc[i])
											break;
										ce = 0;
										for (j = 0; j < MAX_CHANNEL; j++)
										{//for (j = 0; j < MAX_CHANNEL; j++)
											if (0 == pnc[j])
												break;
											if (pcc[i] == pnc[j])
											{
												ce = 1;
												break;
											}
										}//for (j = 0; j < MAX_CHANNEL; j++)
										if (0 == ce)
										{
											wcc[z] = pcc[i];
											z++;
											if ((0x0d <= pcc[i]) && (pcc[i] <= 0x10))
											{
												wpcc[k] = pcc[i];
												k++;
												pce = 1;
												continue;
											}
											else
											{
												wnpcc[s] = pcc[i];
												s++;
												continue;
											}		
										}
									}//for (i = 0; i < MAX_CHANNEL; i++)
									
									if ((0==wcc[0])&&((stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0)))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{
											stStatuSinfo.conmode = 28;
											stStatuSinfo.color = 0x02;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime + stInterGrateControlPhaseInfo.ytime + stInterGrateControlPhaseInfo.rtime;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
											stStatuSinfo.chans = 0;
            								memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            								csta = stStatuSinfo.csta;
            								for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == cc[i])
                    								break;
                								stStatuSinfo.chans += 1;
                								tcsta = cc[i];
                								tcsta <<= 2;
                								tcsta &= 0xfc;
                								tcsta |= 0x02; //00000010-green 
                								*csta = tcsta;
                								csta++;
            								}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
											memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}
									}//if ((0==wcc[0])&&((stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0)))
									if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.gftime > 0))	
									{			
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = 28;
											stStatuSinfo.color = 0x03;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
	
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == wcc[i])
                    								break;
                								for (j = 0; j < stStatuSinfo.chans; j++)
                								{
                    								if (0 == stStatuSinfo.csta[j])
                        								break;
                    								tcsta = stStatuSinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (wcc[i] == tcsta)
                    								{
                        								stStatuSinfo.csta[j] &= 0xfc;
														stStatuSinfo.csta[j] |= 0x03; //00000011-green flash
														break;
                    								}
                								}
            								}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("status pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.gftime > 0))
	
									//green flash
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&dctd,0,sizeof(dctd));
										dctd.mode = 28;//traffic control
										dctd.pattern = *(fcdata->patternid);
										dctd.lampcolor = 0x02;
										dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
										dctd.phaseid = 0;
										dctd.stageline = 0;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 28;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x02;
                                        dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
                                        dctd.phaseid = 0;
                                        dctd.stageline = 0;		
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = 0;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = stInterGrateControlPhaseInfo.gftime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef INTER_CONTROL_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef INTER_CONTROL_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									if (stInterGrateControlPhaseInfo.gftime > 0)
									{
										ngf = 0;
										while (1)
										{
											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x03,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);
									
											ngf += 1;
											if (ngf >= stInterGrateControlPhaseInfo.gftime)
												break;
										}
									}//if (stInterGrateControlPhaseInfo.gftime > 0)
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef INTER_CONTROL_DEBUG
            								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}

									if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.ytime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = 28;
											stStatuSinfo.color = 0x01;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.ytime;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
				
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == wnpcc[i])
                    								break;
                								for (j = 0; j < stStatuSinfo.chans; j++)
                								{
                    								if (0 == stStatuSinfo.csta[j])
                        								break;
                    								tcsta = stStatuSinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (wnpcc[i] == tcsta)
                    								{
                        								stStatuSinfo.csta[j] &= 0xfc;
														stStatuSinfo.csta[j] |= 0x01; //00000001-yellow
														break;
                    								}
                								}
            								}
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == wpcc[i])
                                                	break;
                                            	for (j = 0; j < stStatuSinfo.chans; j++)
                                            	{
                                                	if (0 == stStatuSinfo.csta[j])
                                                    	break;
                                                	tcsta = stStatuSinfo.csta[j];
                                                	tcsta &= 0xfc;
                                                	tcsta >>= 2;
                                                	tcsta &= 0x3f;
                                                	if (wpcc[i] == tcsta)
                                                	{
                                                    	stStatuSinfo.csta[j] &= 0xfc;
														break;
                                                	}
                                            	}
                                        	}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("status pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.ytime > 0))

									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            		{
                            			memset(&dctd,0,sizeof(dctd));
                            			dctd.mode = 28;//traffic control
                            			dctd.pattern = *(fcdata->patternid);
                            			dctd.lampcolor = 0x01;
                            			dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                            			dctd.phaseid = 0;
                            			dctd.stageline = 0;
                            			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
                            			{
                            			#ifdef INTER_CONTROL_DEBUG
                               				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            			#endif
                            			}
                            		}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 28;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x01;
                                        dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                                        dctd.phaseid = 0;
                                        dctd.stageline = 0;		
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = 0;
                            		fbdata[3] = 0x01;
                            		fbdata[4] = stInterGrateControlPhaseInfo.ytime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                            			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                            			{
                            			#ifdef INTER_CONTROL_DEBUG
                               				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                			{
                                			#ifdef INTER_CONTROL_DEBUG
                                   				printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(stInterGrateControlPhaseInfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                                		printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}

									if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.rtime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = 28;
											stStatuSinfo.color = 0x00;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.rtime;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
	
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == wcc[i])
                                                	break;
                                            	for (j = 0; j < stStatuSinfo.chans; j++)
                                            	{
                                                	if (0 == stStatuSinfo.csta[j])
                                                    	break;
                                                	tcsta = stStatuSinfo.csta[j];
                                                	tcsta &= 0xfc;
                                                	tcsta >>= 2;
                                                	tcsta &= 0x3f;
                                                	if (wcc[i] == tcsta)
                                                	{
                                                    	stStatuSinfo.csta[j] &= 0xfc;
														break;
                                                	}
                                            	}
                                        	}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.rtime > 0))									
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),pnc,0x02))
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			pnc,0x02,fcdata->markbit))
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
                            		}
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										stStatuSinfo.conmode = 28;
										stStatuSinfo.color = 0x02;
										stStatuSinfo.time = 0;
										stStatuSinfo.stage = 0;
										stStatuSinfo.cyclet = 0;
										stStatuSinfo.phase = 0;

										stStatuSinfo.chans = 0;
            							memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            							csta = stStatuSinfo.csta;
            							for (i = 0; i < MAX_CHANNEL; i++)
            							{
                							if (0 == pnc[i])
                    							break;
                							stStatuSinfo.chans += 1;
                							tcsta = pnc[i];
                							tcsta <<= 2;
                							tcsta &= 0xfc;
                							tcsta |= 0x02;
                							*csta = tcsta;
                							csta++;
            							}
										memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));					
            							memset(sibuf,0,sizeof(sibuf));
            							if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            							{
            							#ifdef INTER_CONTROL_DEBUG
                							printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            							#endif
            							}
            							else
            							{
                							write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            							}
									}//report info to server actively									
	
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&dctd,0,sizeof(dctd));
										dctd.mode = 28;//traffic control
										dctd.pattern = *(fcdata->patternid);
										dctd.lampcolor = 0x02;
										dctd.lamptime = 0;
										dctd.phaseid = 0;
										dctd.stageline = 0;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 28;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x02;
                                        dctd.lamptime = 0;
                                        dctd.phaseid = 0;
                                        dctd.stageline = 0;		
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = 0;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef INTER_CONTROL_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef INTER_CONTROL_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
									}
								}//else if (1 == fpc)
								unsigned char				dce = 0;
								unsigned char				val = 0;
								if (0x05 == tcbuf[4])
								{
									dce = 0x10;
									val = 52;
								}
								else if (0x06 == tcbuf[4])
								{
									dce = 0x12;
									val = 54;
								}
								else if (0x07 == tcbuf[4])
								{
									dce = 0x14;
									val = 56;
								}
								else if (0x08 == tcbuf[4])
								{
                                   	dce = 0x16;
									val = 58;
								}
								else if (0x09 == tcbuf[4])
								{
                                   	dce = 0x18;
									val = 60;
								}
								else if (0x0a == tcbuf[4])
								{
                                   	dce = 0x1a;
									val = 62;
								}
								else if (0x0b == tcbuf[4])
								{
                                   	dce = 0x1c;
									val = 64;
								}
								else if (0x0c == tcbuf[4])
								{
                                   	dce = 0x1e;
									val = 66;
								}
								if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
                                {
                                   	memset(wlinfo,0,sizeof(wlinfo));
                                   	gettimeofday(&ct,NULL);
                                   	if(SUCCESS!=wireless_info_report(wlinfo,ct.tv_sec,fcdata->wlmark,dce))
                                   	{
                                   	#ifdef INTER_CONTROL_DEBUG
                                       	printf("wireless_info_report,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   	#endif
                                   	}
                                   	else
                                   	{
                                       	write(*(fcdata->sockfd->csockfd),wlinfo,sizeof(wlinfo));
                                   	}
								  	update_event_list(fcdata->ec,fcdata->el,1,val,ct.tv_sec,fcdata->markbit);
                                }
								else
								{
									gettimeofday(&ct,NULL);
                        		    update_event_list(fcdata->ec,fcdata->el,1,val,ct.tv_sec,fcdata->markbit);
								}
								
								continue;
                            }//direction control
                            else if ((0x02 == tcbuf[4]) && (1 == wtlock))
                            {//step by step
								cp = 0;
								fpc = 0;

								tpp.func[0] = tcbuf[4];
                                memset(terbuf,0,sizeof(terbuf));
                                if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
                                {
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),terbuf,sizeof(terbuf)) < 0)
                                        {
                                        #ifdef INTER_CONTROL_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
                              	if(1 == bAllLampClose)
								{
									new_all_red(&ardata);
								}
								if ((1 == dcred) || (1 == YellowFlashYes) || (1 == close))
								{//if ((1 == tcred) || (1 == tcyfyes) || (1 == close))
									if (1 == YellowFlashYes)
                                    {
                                        pthread_cancel(YellowFlashPid);
                                        pthread_join(YellowFlashPid,NULL);
                                        YellowFlashYes = 0;
                                    }
                                    if (1 != dcred)
                                    {
                                        new_all_red(&ardata);
                                    }
                                    dcred = 0;
                                    close = 0;
				//					#ifdef CLOSE_LAMP
                                    igc_set_lamp_color(*(fcdata->bbserial),clch,0x03);
									update_channel_status(fcdata->sockfd,chanstatus,clch,0x03,fcdata->markbit);
                //                    #endif	
									if ((dirc < 0x05) || (dirc > 0x0c))
									{//not have phase control
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.chan,0x02))
                                		{
                                		#ifdef INTER_CONTROL_DEBUG
                                    		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
                                    		*(fcdata->markbit) |= 0x0800;
                                		}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    									stInterGrateControlPhaseInfo.chan,0x02,fcdata->markbit))
                                		{
                                		#ifdef INTER_CONTROL_DEBUG
                                    		printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               			#endif
                                		}

										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = 28;
											stStatuSinfo.color = 0x02;
											stStatuSinfo.time = 0;
											stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
											stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
									/*				
											stStatuSinfo.chans = 0;
            								memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            								csta = stStatuSinfo.csta;
            								for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								stStatuSinfo.chans += 1;
                								tcsta = i + 1;
                								tcsta <<= 2;
                								tcsta &= 0xfc;
												for (j = 0; j < MAX_CHANNEL; j++)
												{
													if (0 == stInterGrateControlPhaseInfo.chan[j])
														break;
													if ((i+1) == stInterGrateControlPhaseInfo.chan[j])
													{
														tcsta |= 0x02;
														break;
													}
												}
                								*csta = tcsta;
                								csta++;
            								}
										*/
											stStatuSinfo.chans = 0;
											memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
                                            csta = stStatuSinfo.csta;
											for (i = 0; i < MAX_CHANNEL; i++)
											{
												if (0 == stInterGrateControlPhaseInfo.chan[i])
													break;
												stStatuSinfo.chans += 1;
												tcsta = stInterGrateControlPhaseInfo.chan[i];
												tcsta <<= 2;
												tcsta &= 0xfc;
												tcsta |= 0x02; //00000010-green 
												*csta = tcsta;
												csta++;
											}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//not have phase control
									else
									{//have direction control
										if (SUCCESS!=igc_set_lamp_color(*(fcdata->bbserial),dirch[dirc-5],0x02))
                                		{
                                		#ifdef INTER_CONTROL_DEBUG
                                    		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
                                    		*(fcdata->markbit) |= 0x0800;
                                		}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    									dirch[dirc-5],0x02,fcdata->markbit))
                                		{
                                		#ifdef INTER_CONTROL_DEBUG
                                    		printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               			#endif
                                		}

										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = 28;
											stStatuSinfo.color = 0x02;
											stStatuSinfo.time = 0;
											stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
									/*				
											stStatuSinfo.chans = 0;
            								memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            								csta = stStatuSinfo.csta;
            								for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								stStatuSinfo.chans += 1;
                								tcsta = i + 1;
                								tcsta <<= 2;
                								tcsta &= 0xfc;
												for (j = 0; j < MAX_CHANNEL; j++)
												{
													if (0 == dirch[dirc-5][j])
														break;
													if ((i+1) == dirch[dirc-5][j])
													{
														tcsta |= 0x02;
														break;
													}
												}
                								*csta = tcsta;
                								csta++;
            								}
										*/
											stStatuSinfo.chans = 0;
											memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
                                            csta = stStatuSinfo.csta;
											for (i = 0; i < MAX_CHANNEL; i++)
											{
												if (0 == dirch[dirc-5][i])
													break;
												stStatuSinfo.chans += 1;
												tcsta = dirch[dirc-5][i];
												tcsta <<= 2;
												tcsta &= 0xfc;
												tcsta |= 0x02; //00000010-green 
												*csta = tcsta;
												csta++;
											}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//have direction control
									if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
                                	{
                                    	memset(wlinfo,0,sizeof(wlinfo));
                                    	gettimeofday(&ct,NULL);
                                    	if(SUCCESS!=wireless_info_report(wlinfo,ct.tv_sec,fcdata->wlmark,0x08))
                                    	{
                                    	#ifdef INTER_CONTROL_DEBUG
                                        	printf("wireless_info_report,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                    	else
                                    	{
                                        	write(*(fcdata->sockfd->csockfd),wlinfo,sizeof(wlinfo));
                                    	}
                                     	update_event_list(fcdata->ec,fcdata->el,1,44,ct.tv_sec,fcdata->markbit);
                                	}
                                	else
                                	{
                                    	gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,44,ct.tv_sec,fcdata->markbit);
                                	}	

									continue;
								}//if ((1 == dcred) || (1 == YellowFlashYes) || (1 == close))
								if ((dirc < 0x05) || (dirc > 0x0c))
								{//not have direction control
									if((0==stInterGrateControlPhaseInfo.cchan[0])&&(stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = 28;
											stStatuSinfo.color = 0x02;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime + stInterGrateControlPhaseInfo.ytime + stInterGrateControlPhaseInfo.rtime;
											stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
											stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
											stStatuSinfo.chans = 0;
            								memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            								csta = stStatuSinfo.csta;
            								for (i = 0; i < MAX_CHANNEL; i++)
            								{
												if (0 == stInterGrateControlPhaseInfo.chan[i])
													break;
                								stStatuSinfo.chans += 1;
                								tcsta = stInterGrateControlPhaseInfo.chan[i];
                								tcsta <<= 2;
                								tcsta &= 0xfc;
												tcsta |= 0x02;
                								*csta = tcsta;
                								csta++;
            								}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//info.cchan[0])&&(stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0))

									if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.gftime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = 28;
											stStatuSinfo.color = 0x03;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime;
											stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
											stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
									
											for (i = 0; i < MAX_CHANNEL; i++)
                                			{
                                				if (0 == stInterGrateControlPhaseInfo.cchan[i])
                                        			break;
                                    			for (j = 0; j < stStatuSinfo.chans; j++)
                                    			{
                                        			if (0 == stStatuSinfo.csta[j])
                                            			break;
                                        			tcsta = stStatuSinfo.csta[j];
                                        			tcsta &= 0xfc;
                                        			tcsta >>= 2;
                                        			tcsta &= 0x3f;
                                        			if (stInterGrateControlPhaseInfo.cchan[i] == tcsta)
                                        			{
                                            			stStatuSinfo.csta[j] &= 0xfc;
                                           				stStatuSinfo.csta[j] |= 0x03; //00000001-green flash
														break;
                                        			}
                                    			}
                                			}								
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));	
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.gftime > 0))

									//current phase begin to green flash
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&dctd,0,sizeof(dctd));
										dctd.mode = 28;//traffic control
										dctd.pattern = *(fcdata->patternid);
										dctd.lampcolor = 0x02;
										dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
										dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
										dctd.stageline = stInterGrateControlPhaseInfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 28;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x02;
                                        dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
                                        dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                        dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = stInterGrateControlPhaseInfo.gftime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                                		if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                		{
                                		#ifdef INTER_CONTROL_DEBUG
                                    		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                    		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                    		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                    		{
                                    		#ifdef INTER_CONTROL_DEBUG
                                        		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
                                    		}
                                		}
                            		}
                            		else
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                                		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									if (stInterGrateControlPhaseInfo.gftime > 0)
									{	
									ngf = 0;
									while (1)
									{
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cchan,0x03))
        								{
        								#ifdef INTER_CONTROL_DEBUG
            								printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															stInterGrateControlPhaseInfo.cchan,0x03,fcdata->markbit))
        								{
        								#ifdef INTER_CONTROL_DEBUG
            								printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
        								#endif
        								}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);

										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cchan,0x02))
        								{
        								#ifdef INTER_CONTROL_DEBUG
            								printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    									stInterGrateControlPhaseInfo.cchan,0x02,fcdata->markbit))
                                		{
                                		#ifdef INTER_CONTROL_DEBUG
                                    		printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                		#endif
                                		}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);
								
										ngf += 1;
										if (ngf >= stInterGrateControlPhaseInfo.gftime)
											break;
									}
									}//if (stInterGrateControlPhaseInfo.gftime > 0)
									if (1 == (stInterGrateControlPhaseInfo.cpcexist))
									{
										//current phase begin to yellow lamp
										if (SUCCESS!=igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cnpchan,0x01))
        								{
        								#ifdef INTER_CONTROL_DEBUG
            								printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
													stInterGrateControlPhaseInfo.cnpchan,0x01, fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS!=igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cpchan,0x00))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																	stInterGrateControlPhaseInfo.cpchan,0x00,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cchan,0x01))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														stInterGrateControlPhaseInfo.cchan,0x01,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}

									if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.ytime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = 28;
											stStatuSinfo.color = 0x01;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.ytime;
											stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
											stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
															
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == stInterGrateControlPhaseInfo.cnpchan[i])
                    								break;
                								for (j = 0; j < stStatuSinfo.chans; j++)
                								{
                    								if (0 == stStatuSinfo.csta[j])
                        								break;
                    								tcsta = stStatuSinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (stInterGrateControlPhaseInfo.cnpchan[i] == tcsta)
                    								{
                        								stStatuSinfo.csta[j] &= 0xfc;
														stStatuSinfo.csta[j] |= 0x01; //00000001-yellow
														break;
                    								}
                								}
            								}
											for (i = 0; i < MAX_CHANNEL; i++)
                                			{
                                				if (0 == stInterGrateControlPhaseInfo.cpchan[i])
                                        			break;
                                    			for (j = 0; j < stStatuSinfo.chans; j++)
                                    			{
                                        			if (0 == stStatuSinfo.csta[j])
                                            			break;
                                        			tcsta = stStatuSinfo.csta[j];
                                        			tcsta &= 0xfc;
                                        			tcsta >>= 2;
                                        			tcsta &= 0x3f;
                                        			if (stInterGrateControlPhaseInfo.cpchan[i] == tcsta)
                                        			{
                                            			stStatuSinfo.csta[j] &= 0xfc;
														break;
                                        			}
                                    			}
                                			}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.ytime > 0))

									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            		{
                               			memset(&dctd,0,sizeof(dctd));
                               			dctd.mode = 28;//traffic control
                               			dctd.pattern = *(fcdata->patternid);
                               			dctd.lampcolor = 0x01;
                               			dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                               			dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                               			dctd.stageline = stInterGrateControlPhaseInfo.stageline;
                               			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
                               			{
                               			#ifdef INTER_CONTROL_DEBUG
                                   			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               			#endif
                               			}
                            		}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 28;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x01;
                                        dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                                        dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                        dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            		fbdata[3] = 0x01;
                            		fbdata[4] = stInterGrateControlPhaseInfo.ytime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef INTER_CONTROL_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef INTER_CONTROL_DEBUG
                                       			printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(stInterGrateControlPhaseInfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cchan,0x00))
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                                		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															stInterGrateControlPhaseInfo.cchan,0x00,fcdata->markbit))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}

									if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.rtime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = 28;
											stStatuSinfo.color = 0x00;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.rtime;
											stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
											stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
															
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == stInterGrateControlPhaseInfo.cchan[i])
                    								break;
                								for (j = 0; j < stStatuSinfo.chans; j++)
                								{
                    								if (0 == stStatuSinfo.csta[j])
                        								break;
                    								tcsta = stStatuSinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (stInterGrateControlPhaseInfo.cchan[i] == tcsta)
                    								{
                        								stStatuSinfo.csta[j] &= 0xfc;
														break;
                    								}
                								}
            								}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.rtime > 0))

									if (stInterGrateControlPhaseInfo.rtime > 0)
									{
										if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            			{
                                			memset(&dctd,0,sizeof(dctd));
                                			dctd.mode = 28;//traffic control
                                			dctd.pattern = *(fcdata->patternid);
                                			dctd.lampcolor = 0x00;
                                			dctd.lamptime = stInterGrateControlPhaseInfo.rtime;
                                			dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                			dctd.stageline = stInterGrateControlPhaseInfo.stageline;
                                			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
                                			{
                                			#ifdef INTER_CONTROL_DEBUG
                                    			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                            			}
										#ifdef I_EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&dctd,0,sizeof(dctd));
                                            dctd.mode = 28;//traffic control
                                            dctd.pattern = *(fcdata->patternid);
                                            dctd.lampcolor = 0x00;
                                            dctd.lamptime = stInterGrateControlPhaseInfo.rtime;
                                            dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                            dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#endif
										fbdata[1] = 28;
                                		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                                		fbdata[3] = 0x00;
                                		fbdata[4] = stInterGrateControlPhaseInfo.rtime;
                                		if (!wait_write_serial(*(fcdata->fbserial)))
                                		{
                                    		if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                    		{
                                    		#ifdef INTER_CONTROL_DEBUG
                                        		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
                                        		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                        		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                        		{
                                        		#ifdef INTER_CONTROL_DEBUG
                                            		printf("genfile err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                        		#endif
                                        		}
                                    		}
                                		}
                                		else
                                		{
                                		#ifdef INTER_CONTROL_DEBUG
                                    		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                		#endif
                                		}
										sleep(stInterGrateControlPhaseInfo.rtime);
									}

									*(fcdata->slnum) += 1;
									*(fcdata->stageid) = \
										tscdata->timeconfig->TimeConfigList[rettl][*(fcdata->slnum)].StageId;
								if (0==(tscdata->timeconfig->TimeConfigList[rettl][*(fcdata->slnum)].StageId))
									{
										*(fcdata->slnum) = 0;
										*(fcdata->stageid) = \
										tscdata->timeconfig->TimeConfigList[rettl][*(fcdata->slnum)].StageId;
									}
									//get phase info of next phase
									memset(&stInterGrateControlPhaseInfo,0,sizeof(InterGrateControlPhaseInfo_t));
        							if(SUCCESS!=igc_get_phase_info(fcdata,tscdata,rettl,*(fcdata->slnum),&stInterGrateControlPhaseInfo))
        							{
        							#ifdef INTER_CONTROL_DEBUG
            							printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
										output_log("Timing control,get phase info err");
        							#endif
										Inter_Control_End_Part_Child_Thread();
            							return FAIL;
        							}
        							*(fcdata->phaseid) = 0;
        							*(fcdata->phaseid) |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));

									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.chan,0x02))
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                                		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																stInterGrateControlPhaseInfo.chan,0x02,fcdata->markbit))
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                                		printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
                            		}
									ncmode = *(fcdata->contmode);
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										stStatuSinfo.conmode = 28;
										stStatuSinfo.color = 0x02;
										stStatuSinfo.time = 0;
										stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
										stStatuSinfo.cyclet = 0;
										stStatuSinfo.phase = 0;
										stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
															
										stStatuSinfo.chans = 0;
            							memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            							csta = stStatuSinfo.csta;
            							for (i = 0; i < MAX_CHANNEL; i++)
            							{
											if (0 == stInterGrateControlPhaseInfo.chan[i])
												break;
                							stStatuSinfo.chans += 1;
                							tcsta = stInterGrateControlPhaseInfo.chan[i];
                							tcsta <<= 2;
                							tcsta &= 0xfc;
											tcsta |= 0x02;
                							*csta = tcsta;
                							csta++;
            							}
										memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            							memset(sibuf,0,sizeof(sibuf));
            							if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            							{
            							#ifdef INTER_CONTROL_DEBUG
                							printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            							#endif
            							}
            							else
            							{
                							write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            							}
									}//report info to server actively
	
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&dctd,0,sizeof(dctd));
										dctd.mode = 28;//traffic control
										dctd.pattern = *(fcdata->patternid);
										dctd.lampcolor = 0x02;
										dctd.lamptime = 0;
										dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
										dctd.stageline = stInterGrateControlPhaseInfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 28;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x02;
                                        dctd.lamptime = 0;
                                        dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                        dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = 0;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                                		if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                		{
	                                		#ifdef INTER_CONTROL_DEBUG
	                                    		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
	                                		#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                    		update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                    		{
                                    		#ifdef INTER_CONTROL_DEBUG
                                        		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
                                    		}
                                		}
                            		}
                            		else
                            		{
	                            		#ifdef INTER_CONTROL_DEBUG
	                                		printf("face board serial port cannot write,Line:%d\n",__LINE__);
	                            		#endif
                            		}
								}// not have direction control
								else
								{//have direction control
									//default return to first stage;
									*(fcdata->slnum) = 0;
									*(fcdata->stageid) = \
                                               tscdata->timeconfig->TimeConfigList[rettl][0].StageId;
                                    memset(&stInterGrateControlPhaseInfo,0,sizeof(InterGrateControlPhaseInfo_t));
                                    if(SUCCESS != igc_get_phase_info(fcdata,tscdata,rettl,0,&stInterGrateControlPhaseInfo))
                                    {
	                                    #ifdef INTER_CONTROL_DEBUG
	                                        printf("phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
	                                        output_log("Timing control,get phase info err");
	                                    #endif
                                        Inter_Control_End_Part_Child_Thread();
                                        return FAIL;
                                    }
                                    *(fcdata->phaseid) = 0;
                                    *(fcdata->phaseid) |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));

									memset(wcc,0,sizeof(wcc));
									memset(wnpcc,0,sizeof(wnpcc));
									memset(wpcc,0,sizeof(wpcc));
									z = 0;
									k = 0;
									s = 0;
									pce = 0;
									for (i = 0; i < MAX_CHANNEL; i++)
									{//for (i = 0; i < MAX_CHANNEL; i++)
										if (0 == dirch[dirc-5][i])
											break;
										ce = 0;
										for (j = 0; j < MAX_CHANNEL; j++)
										{//for (j = 0; j < MAX_CHANNEL; j++)
											if (0 == stInterGrateControlPhaseInfo.chan[j])
												break;
											if (dirch[dirc-5][i] == stInterGrateControlPhaseInfo.chan[j])
											{
												ce = 1;
												break;
											}
										}//for (j = 0; j < MAX_CHANNEL; j++)
										if (0 == ce)
										{
											wcc[z] = dirch[dirc-5][i];
											z++;
											if ((dirch[dirc-5][i]>=13) && (dirch[dirc-5][i]<=16))
											{
												wpcc[k] = dirch[dirc-5][i];
												k++;
												pce = 1;
												continue;
											}
											else
											{
												wnpcc[s] = dirch[dirc-5][i];
												s++;
												continue;
											}
										}
									}//for (i = 0; i < MAX_CHANNEL; i++)

									if((0==wcc[0])&&(stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = 28;
											stStatuSinfo.color = 0x02;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime + stInterGrateControlPhaseInfo.ytime + stInterGrateControlPhaseInfo.rtime;
											stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
											stStatuSinfo.chans = 0;
            								memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            								csta = stStatuSinfo.csta;
            								for (i = 0; i < MAX_CHANNEL; i++)
            								{
												if (0 == dirch[dirc-5][i])
													break;
                								stStatuSinfo.chans += 1;
                								tcsta = dirch[dirc-5][i];
                								tcsta <<= 2;
                								tcsta &= 0xfc;
												tcsta |= 0x02;
                								*csta = tcsta;
                								csta++;
            								}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//info.cchan[0])&&(stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0))

									if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.gftime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = 28;
											stStatuSinfo.color = 0x03;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime;
											stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
									
											for (i = 0; i < MAX_CHANNEL; i++)
                                			{
                                				if (0 == wcc[i])
                                        			break;
                                    			for (j = 0; j < stStatuSinfo.chans; j++)
                                    			{
                                        			if (0 == stStatuSinfo.csta[j])
                                            			break;
                                        			tcsta = stStatuSinfo.csta[j];
                                        			tcsta &= 0xfc;
                                        			tcsta >>= 2;
                                        			tcsta &= 0x3f;
                                        			if (wcc[i] == tcsta)
                                        			{
                                            			stStatuSinfo.csta[j] &= 0xfc;
                                           				stStatuSinfo.csta[j] |= 0x03; //00000001-green flash
														break;
                                        			}
                                    			}
                                			}								
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
	            								#ifdef INTER_CONTROL_DEBUG
	                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
	            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.gftime > 0))

									//current phase begin to green flash
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&dctd,0,sizeof(dctd));
										dctd.mode = 28;//traffic control
										dctd.pattern = *(fcdata->patternid);
										dctd.lampcolor = 0x02;
										dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
										dctd.phaseid = 0;
										dctd.stageline = stInterGrateControlPhaseInfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
										{
											#ifdef INTER_CONTROL_DEBUG
												printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
										}
									}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 28;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x02;
                                        dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
                                        dctd.phaseid = 0;
                                        dctd.stageline = stInterGrateControlPhaseInfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = stInterGrateControlPhaseInfo.gftime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                                		if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                		{
                                		#ifdef INTER_CONTROL_DEBUG
                                    		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                    		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                    		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                    		{
                                    		#ifdef INTER_CONTROL_DEBUG
                                        		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
                                    		}
                                		}
                            		}
                            		else
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                                		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									if (stInterGrateControlPhaseInfo.gftime > 0)
									{	
										ngf = 0;
										while (1)
										{
											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x03,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("updatechannelerr,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("setgreenlamperr,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x02,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("updatechannelerr,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);
									
											ngf += 1;
											if (ngf >= stInterGrateControlPhaseInfo.gftime)
												break;
										}
									}//if (stInterGrateControlPhaseInfo.gftime > 0)
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS!=igc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef INTER_CONTROL_DEBUG
            								printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														wcc,0x01,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}

									if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.ytime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = 28;
											stStatuSinfo.color = 0x01;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.ytime;
											stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
															
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == wnpcc[i])
                    								break;
                								for (j = 0; j < stStatuSinfo.chans; j++)
                								{
                    								if (0 == stStatuSinfo.csta[j])
                        								break;
                    								tcsta = stStatuSinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (wnpcc[i] == tcsta)
                    								{
                        								stStatuSinfo.csta[j] &= 0xfc;
														stStatuSinfo.csta[j] |= 0x01; //00000001-yellow
														break;
                    								}
                								}
            								}
											for (i = 0; i < MAX_CHANNEL; i++)
                                			{
                                				if (0 == wpcc[i])
                                        			break;
                                    			for (j = 0; j < stStatuSinfo.chans; j++)
                                    			{
                                        			if (0 == stStatuSinfo.csta[j])
                                            			break;
                                        			tcsta = stStatuSinfo.csta[j];
                                        			tcsta &= 0xfc;
                                        			tcsta >>= 2;
                                        			tcsta &= 0x3f;
                                        			if (wpcc[i] == tcsta)
                                        			{
                                            			stStatuSinfo.csta[j] &= 0xfc;
														break;
                                        			}
                                    			}
                                			}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.ytime > 0))

									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            		{
                               			memset(&dctd,0,sizeof(dctd));
                               			dctd.mode = 28;//traffic control
                               			dctd.pattern = *(fcdata->patternid);
                               			dctd.lampcolor = 0x01;
                               			dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                               			dctd.phaseid = 0;
                               			dctd.stageline = stInterGrateControlPhaseInfo.stageline;
                               			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
                               			{
                               			#ifdef INTER_CONTROL_DEBUG
                                   			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               			#endif
                               			}
                            		}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 28;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x01;
                                        dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                                        dctd.phaseid = 0;
                                        dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            		fbdata[3] = 0x01;
                            		fbdata[4] = stInterGrateControlPhaseInfo.ytime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef INTER_CONTROL_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef INTER_CONTROL_DEBUG
                                       			printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(stInterGrateControlPhaseInfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                                		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															wcc,0x00,fcdata->markbit))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}

									if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.rtime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = 28;
											stStatuSinfo.color = 0x00;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.rtime;
											stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
															
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == wcc[i])
                    								break;
                								for (j = 0; j < stStatuSinfo.chans; j++)
                								{
                    								if (0 == stStatuSinfo.csta[j])
                        								break;
                    								tcsta = stStatuSinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (wcc[i] == tcsta)
                    								{
                        								stStatuSinfo.csta[j] &= 0xfc;
														break;
                    								}
                								}
            								}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.rtime > 0))

									if (stInterGrateControlPhaseInfo.rtime > 0)
									{
										if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            			{
                                			memset(&dctd,0,sizeof(dctd));
                                			dctd.mode = 28;//traffic control
                                			dctd.pattern = *(fcdata->patternid);
                                			dctd.lampcolor = 0x00;
                                			dctd.lamptime = stInterGrateControlPhaseInfo.rtime;
                                			dctd.phaseid = 0;
                                			dctd.stageline = stInterGrateControlPhaseInfo.stageline;
                                			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
                                			{
                                			#ifdef INTER_CONTROL_DEBUG
                                    			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                            			}
										#ifdef I_EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&dctd,0,sizeof(dctd));
                                            dctd.mode = 28;//traffic control
                                            dctd.pattern = *(fcdata->patternid);
                                            dctd.lampcolor = 0x00;
                                            dctd.lamptime = stInterGrateControlPhaseInfo.rtime;
                                            dctd.phaseid = 0;
                                            dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#endif
										fbdata[1] = 28;
                                		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                                		fbdata[3] = 0x00;
                                		fbdata[4] = stInterGrateControlPhaseInfo.rtime;
                                		if (!wait_write_serial(*(fcdata->fbserial)))
                                		{
                                    		if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                    		{
                                    		#ifdef INTER_CONTROL_DEBUG
                                        		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
                                        		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                        		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                        		{
                                        		#ifdef INTER_CONTROL_DEBUG
                                            		printf("file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        		#endif
                                        		}
                                    		}
                                		}
                                		else
                                		{
                                		#ifdef INTER_CONTROL_DEBUG
                                    		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                		#endif
                                		}
										sleep(stInterGrateControlPhaseInfo.rtime);
									}
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.chan,0x02))
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			stInterGrateControlPhaseInfo.chan,0x02,fcdata->markbit))
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
                            		}
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										stStatuSinfo.conmode = 28;
										stStatuSinfo.color = 0x02;
										stStatuSinfo.time = 0;
										stStatuSinfo.stage = 0;
										stStatuSinfo.cyclet = 0;
										stStatuSinfo.phase = 0;
										stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));

										stStatuSinfo.chans = 0;
            							memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            							csta = stStatuSinfo.csta;
            							for (i = 0; i < MAX_CHANNEL; i++)
            							{
                							if (0 == stInterGrateControlPhaseInfo.chan[i])
                    							break;
                							stStatuSinfo.chans += 1;
                							tcsta = stInterGrateControlPhaseInfo.chan[i];
                							tcsta <<= 2;
                							tcsta &= 0xfc;
                							tcsta |= 0x02;
                							*csta = tcsta;
                							csta++;
            							}
										memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));				
            							memset(sibuf,0,sizeof(sibuf));
            							if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            							{
            							#ifdef INTER_CONTROL_DEBUG
                							printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            							#endif
            							}
            							else
            							{
                							write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            							}
									}//report info to server actively									
	
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&dctd,0,sizeof(dctd));
										dctd.mode = 28;//traffic control
										dctd.pattern = *(fcdata->patternid);
										dctd.lampcolor = 0x02;
										dctd.lamptime = 0;
										dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
										dctd.stageline = stInterGrateControlPhaseInfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 28;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x02;
                                        dctd.lamptime = 0;
                                        dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                        dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = 0;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef INTER_CONTROL_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef INTER_CONTROL_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}	
								}//have direction control		
								if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
                                {
                                    memset(wlinfo,0,sizeof(wlinfo));
                                    gettimeofday(&ct,NULL);
                                    if (SUCCESS != wireless_info_report(wlinfo,ct.tv_sec,fcdata->wlmark,0x08))
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("wireless_info_report,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                    else
                                    {
                                        write(*(fcdata->sockfd->csockfd),wlinfo,sizeof(wlinfo));
                                    }
									update_event_list(fcdata->ec,fcdata->el,1,44,ct.tv_sec,fcdata->markbit);
                                }
								else
								{
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,44,ct.tv_sec,fcdata->markbit);
								}
					
								dirc = 0;
                                continue;
                            }//step by step
                            else if ((0x03 == tcbuf[4]) && (1 == wtlock))
                            {//yellow flash
								dirc = 0;
								dcred = 0;
								close = 0;
								cp = 0;
				
								tpp.func[0] = tcbuf[4];
                                memset(terbuf,0,sizeof(terbuf));
                                if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
                                {
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),terbuf,sizeof(terbuf)) < 0)
                                        {
                                        #ifdef INTER_CONTROL_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
								if (0 == YellowFlashYes)
								{
									memset(&yfdata,0,sizeof(yfdata));
									yfdata.second = 0;
									yfdata.markbit = fcdata->markbit;
									yfdata.markbit2 = fcdata->markbit2;
									yfdata.serial = *(fcdata->bbserial);
									yfdata.sockfd = fcdata->sockfd;
									yfdata.cs = chanstatus;
#ifdef FLASH_DEBUG
									char szInfo[32] = {0};
									char szInfoT[64] = {0};
									snprintf(szInfo,sizeof(szInfo)-1,"%s",__FUNCTION__);
									snprintf(szInfoT,sizeof(szInfoT)-1,"%d",__LINE__);
									tsc_save_eventlog(szInfo,szInfoT);
#endif
									int yfret = pthread_create(&YellowFlashPid,NULL,(void *)Inter_Control_Yellow_Flash,&yfdata);
									if (0 != yfret)
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
										output_log("Timing control,create yellow flash err");
									#endif
										Inter_Control_End_Part_Child_Thread();

										return FAIL;
									}
									YellowFlashYes = 1;
								}

								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									stStatuSinfo.conmode = 28;
									stStatuSinfo.color = 0x05;
									stStatuSinfo.time = 0;
									stStatuSinfo.stage = 0;
									stStatuSinfo.cyclet = 0;
									stStatuSinfo.phase = 0;

									stStatuSinfo.chans = 0;
            						memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            						csta = stStatuSinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						stStatuSinfo.chans += 1;
                						tcsta = i+1;
                						tcsta <<= 2;
                						tcsta &= 0xfc;
                						*csta = tcsta;
                						csta++;
            						}
									memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));						
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            						{
            						#ifdef INTER_CONTROL_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
									memset(wlinfo,0,sizeof(wlinfo));
                                    gettimeofday(&ct,NULL);
                                    if (SUCCESS != wireless_info_report(wlinfo,ct.tv_sec,fcdata->wlmark,0x0a))
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("wireless_info_report,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                    else
                                    {
                                        write(*(fcdata->sockfd->csockfd),wlinfo,sizeof(wlinfo));
                                    }
                                    update_event_list(fcdata->ec,fcdata->el,1,46,ct.tv_sec,fcdata->markbit);
								}//report info to server actively
								else
								{
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,46,ct.tv_sec,fcdata->markbit);
								}

								continue;
                            }//yellow flash
                            else if ((0x04 == tcbuf[4]) && (1 == wtlock))
                            {//all red
								dirc = 0;
								close = 0;
								cp = 0;
								if (1 == YellowFlashYes)
								{
									pthread_cancel(YellowFlashPid);
									pthread_join(YellowFlashPid,NULL);
									YellowFlashYes = 0;
								}
					
								if (0 == dcred)
								{	
									new_all_red(&ardata);
									dcred = 1;
								}
							
								tpp.func[0] = tcbuf[4];
                                memset(terbuf,0,sizeof(terbuf));
                                if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
                                {
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),terbuf,sizeof(terbuf)) < 0)
                                        {
                                        #ifdef INTER_CONTROL_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))	
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									stStatuSinfo.conmode = 28;
									stStatuSinfo.color = 0x00;
									stStatuSinfo.time = 0;
									stStatuSinfo.stage = 0;
									stStatuSinfo.cyclet = 0;
									stStatuSinfo.phase = 0;

									stStatuSinfo.chans = 0;
            						memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            						csta = stStatuSinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						stStatuSinfo.chans += 1;
                						tcsta = i+1;
                						tcsta <<= 2;
                						tcsta &= 0xfc;
                						*csta = tcsta;
                						csta++;
            						}
									memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));					
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            						{
            						#ifdef INTER_CONTROL_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
									memset(wlinfo,0,sizeof(wlinfo));
                                    gettimeofday(&ct,NULL);
                                    if (SUCCESS != wireless_info_report(wlinfo,ct.tv_sec,fcdata->wlmark,0x0c))
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("wireless_info_report,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                    else
                                    {
                                        write(*(fcdata->sockfd->csockfd),wlinfo,sizeof(wlinfo));
                                    }
									update_event_list(fcdata->ec,fcdata->el,1,48,ct.tv_sec,fcdata->markbit);
								}//report info to server actively
								else
								{
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,48,ct.tv_sec,fcdata->markbit);
								}

								continue;
                            }//all red
                            else if ((0x0d == tcbuf[4]) && (1 == wtlock))
                            {//close
								dirc = 0;
								dcred = 0;
								cp = 0;
								if (1 == YellowFlashYes)
                                {
                                    pthread_cancel(YellowFlashPid);
                                    pthread_join(YellowFlashPid,NULL);
                                    YellowFlashYes = 0;
                                }
								if (0 == close)
                                {
                                    new_all_close(&acdata);
                                    close = 1;
                                }
								tpp.func[0] = tcbuf[4];
                                memset(terbuf,0,sizeof(terbuf));
                                if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
                                {
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),terbuf,sizeof(terbuf)) < 0)
                                        {
                                        #ifdef INTER_CONTROL_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))

								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									stStatuSinfo.conmode = 28;
									stStatuSinfo.color = 0x04;
									stStatuSinfo.time = 0;
									stStatuSinfo.stage = 0;
									stStatuSinfo.cyclet = 0;
									stStatuSinfo.phase = 0;

									stStatuSinfo.chans = 0;
            						memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            						csta = stStatuSinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						stStatuSinfo.chans += 1;
                						tcsta = i+1;
                						tcsta <<= 2;
                						tcsta &= 0xfc;
                						*csta = tcsta;
                						csta++;
            						}
									memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            						{
            						#ifdef INTER_CONTROL_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
									memset(wlinfo,0,sizeof(wlinfo));
                                    gettimeofday(&ct,NULL);
                                    if (SUCCESS != wireless_info_report(wlinfo,ct.tv_sec,fcdata->wlmark,0x0e))
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("wireless_info_report,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                    else
                                    {
                                        write(*(fcdata->sockfd->csockfd),wlinfo,sizeof(wlinfo));
                                    }
									update_event_list(fcdata->ec,fcdata->el,1,50,ct.tv_sec,fcdata->markbit);
								}//report info to server actively
								else
								{
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,50,ct.tv_sec,fcdata->markbit);
								}
								continue;
                            }//close 
                        }//control function
                    }//terminal control is valid only when wireless and key and net control is invalid;
					continue;
                }//wireless terminal control	
				else if ((0xCC == tcbuf[0]) && (0xED == tcbuf[2]))
				{//network control
					if ((0 == wllock) && (0 == keylock) && (0 == wtlock))
					{//net control is valid only when wireless and key control is invalid;
						if ((0xf0 == tcbuf[1]) && (0 == netlock))
						{//net control will happen
							Get_Inter_Greate_Control_Status(&color,&leatime);
							in_phase = 0;
							if (0x02 != color)
							{
								struct timeval          spacetime;
								unsigned char           endlock = 0;
								phcon = 1;
								while (1)
								{//while loop,000000
									spacetime.tv_sec = 0;
									spacetime.tv_usec = 200000;
									select(0,NULL,NULL,NULL,&spacetime);

									memset(tcbuf,0,sizeof(tcbuf));
									read(*(fcdata->conpipe),tcbuf,sizeof(tcbuf));
									if ((0xCC == tcbuf[0]) && (0xED == tcbuf[2]))
									{
										if (0xf1 == tcbuf[1])
										{
											endlock = 1;
											phcon = 0;
											break;
										}
										else
										{
											in_phase = tcbuf[1];
										}
									}
								
									Get_Inter_Greate_Control_Status(&color,&leatime);
									if (0x02 == color)
									{
										break;
									}
									else
									{
										continue;
									}
								}//while loop,000000
								if (1 == endlock)
								{
									*(fcdata->markbit) &= 0xbfff;
									continue;
								}
							}//if (0x02 != color)
							netlock = 1;
							dcred = 0;
							close = 0;
							fpc = 0;
							cp = 0;
							Inter_Control_End_Child_Thread();//end main thread and its child thread

							*(fcdata->markbit2) |= 0x0008;
						#if 0
							new_all_red(&ardata);
						#endif
							if (0 == phcon)
							{
								if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.chan,0x02))
        						{
        						#ifdef INTER_CONTROL_DEBUG
            						printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        						#endif
									*(fcdata->markbit) |= 0x0800;
        						}
							}
						
							*(fcdata->markbit) |= 0x4000;								
							*(fcdata->contmode) = 29;//network control mode
				//			phcon = 0;

							fbdata[1] = 29;
							fbdata[2] = stInterGrateControlPhaseInfo.stageline;
							fbdata[3] = 0x02;
							fbdata[4] = 0;
							if (!wait_write_serial(*(fcdata->fbserial)))
							{
								if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									*(fcdata->markbit) |= 0x0800;
									gettimeofday(&ct,NULL);
                					update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                					if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																fcdata->softevent,fcdata->markbit))
                					{
                					#ifdef INTER_CONTROL_DEBUG
                   						printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                					#endif
                					}
								}
							}
							else
							{
							#ifdef INTER_CONTROL_DEBUG
								printf("face board serial port cannot write,Line:%d\n",__LINE__);
							#endif
							}
							
							if (*(fcdata->auxfunc) & 0x01)
							{//if (*(fcdata->auxfunc) & 0x01)
								if (!wait_write_serial(*(fcdata->bbserial)))
								{
									if (write(*(fcdata->bbserial),dcdownti,sizeof(dcdownti)) < 0)
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
								}
								else
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
								if (!wait_write_serial(*(fcdata->bbserial)))
								{
									if (write(*(fcdata->bbserial),dcedownti,sizeof(dcedownti)) < 0)
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
								}
								else
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
							}//if (*(fcdata->auxfunc) & 0x01)
	
							//send down time to configure tool
                            if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            {
							#ifdef INTER_CONTROL_DEBUG
								printf("send control info to configure tool,File: %s,Line: %d\n", \
											__FILE__,__LINE__);
							#endif
                                memset(&dctd,0,sizeof(dctd));
                                dctd.mode = 29;
                                dctd.pattern = *(fcdata->patternid);
								if(0 == bAllLampClose)
								{
									dctd.lampcolor = 0x02;
								}
								else
								{
									dctd.lampcolor = 0x03;
								} 
                                dctd.lamptime = 0;
                                dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                dctd.stageline = stInterGrateControlPhaseInfo.stageline;
                                if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
                            }
							#ifdef I_EMBED_CONFIGURE_TOOL
                            if (*(fcdata->markbit2) & 0x0200)
                            {
                                memset(&dctd,0,sizeof(dctd));
                                dctd.mode = 29;
                                dctd.pattern = *(fcdata->patternid);
                                if(0 == bAllLampClose)
								{
									dctd.lampcolor = 0x02;
								}
								else
								{
									dctd.lampcolor = 0x03;
								}
                                dctd.lamptime = 0;
                                dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                dctd.stageline = stInterGrateControlPhaseInfo.stageline;  
                                if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
                            }
                            #endif
							if(0 == bAllLampClose)
							{
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
	                                    stInterGrateControlPhaseInfo.chan,0x02,fcdata->markbit))
	                            {
	                            #ifdef INTER_CONTROL_DEBUG
	                                printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
	                            #endif
	                            }
							}
							else
							{
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
	                                    stInterGrateControlPhaseInfo.chan,0x03,fcdata->markbit))
	                            {
	                            #ifdef INTER_CONTROL_DEBUG
	                                printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
	                            #endif
	                            }
							}
							objecti[0].objectv[0] = 0xf2;
							factovs = 0;
							memset(cbuf,0,sizeof(cbuf));
							if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                            {
                            #ifdef INTER_CONTROL_DEBUG
                            	printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }
							if (!(*(fcdata->markbit) & 0x1000))
							{
                            	write(*(fcdata->sockfd->csockfd),cbuf,factovs);
							}
							gettimeofday(&ct,NULL);
                            update_event_list(fcdata->ec,fcdata->el,1,84,ct.tv_sec,fcdata->markbit);
                           	if(0 == in_phase)
							{
								continue;
							}
							else
							{
								tcbuf[1] = in_phase;
								igc_get_last_phaseinfo(fcdata,tscdata,rettl,&stInterGrateControlPhaseInfo);
							}
						}//net control will happen
						if ((0xf0 == tcbuf[1]) && (1 == netlock))
                        {//has been status of net control
                            objecti[0].objectv[0] = 0xf2;
                            factovs = 0;
                            memset(cbuf,0,sizeof(cbuf));
                            if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                            {
                            #ifdef INTER_CONTROL_DEBUG
                                printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }
                            if (!(*(fcdata->markbit) & 0x1000))
                            {
                                write(*(fcdata->sockfd->csockfd),cbuf,factovs);
                            }
                        }//has been status of net control
						if ((0xf1 == tcbuf[1]) && (0 == netlock))
                        {//not fit control or restrore
                            objecti[0].objectv[0] = 0xf3;
                            factovs = 0;
                            memset(cbuf,0,sizeof(cbuf));
                            if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x00))
                            {
                            #ifdef INTER_CONTROL_DEBUG
                                printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }
                            if (!(*(fcdata->markbit) & 0x1000))
                            {
                                write(*(fcdata->sockfd->csockfd),cbuf,factovs);
                            }
                        }//not fit control or restrore
						if ((0xf1 == tcbuf[1]) && (1 == netlock))
						{//net control will end
							netlock = 0;
							phcon = 0;
					//		dcred = 0;
					//		close = 0;
							fpc = 0;
					/*
							if (1 == YellowFlashYes)
							{
								pthread_cancel(YellowFlashPid);
								pthread_join(YellowFlashPid,NULL);
								YellowFlashYes = 0;
							}
					*/
							*(fcdata->markbit) &= 0xbfff;
							if(1 == bAllLampClose)
							{
								friststartcontrol = 1;
							}
							if (0 == cp)
                            {//not have phase control
                                if ((0 == dcred) && (0 == YellowFlashYes) && (0 == close))
                                {//not have all red/yellow flash/close
									if((0==stInterGrateControlPhaseInfo.cchan[0])&&(stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = 29;
											stStatuSinfo.color = 0x02;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime + stInterGrateControlPhaseInfo.ytime + stInterGrateControlPhaseInfo.rtime;
											stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
											stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
															
											stStatuSinfo.chans = 0;
            								memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            								csta = stStatuSinfo.csta;
            								for (i = 0; i < MAX_CHANNEL; i++)
            								{
												if (0 == stInterGrateControlPhaseInfo.chan[i])
													break;
                								stStatuSinfo.chans += 1;
                								tcsta = stInterGrateControlPhaseInfo.chan[i];
                								tcsta <<= 2;
                								tcsta &= 0xfc;
												tcsta |= 0x02;
                								*csta = tcsta;
                								csta++;
            								}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("Info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0==stInterGrateControlPhaseInfo.cchan[0])&&(stInterGrateControlPhaseInfo.gftime>0) 
									//||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0))

									if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.gftime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = 29;
											stStatuSinfo.color = 0x03;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime;
											stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
											stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
									
											for (i = 0; i < MAX_CHANNEL; i++)
                                			{
                                				if (0 == stInterGrateControlPhaseInfo.cchan[i])
                                        			break;
                                    			for (j = 0; j < stStatuSinfo.chans; j++)
                                    			{
                                        			if (0 == stStatuSinfo.csta[j])
                                            			break;
                                        			tcsta = stStatuSinfo.csta[j];
                                        			tcsta &= 0xfc;
                                        			tcsta >>= 2;
                                        			tcsta &= 0x3f;
                                        			if (stInterGrateControlPhaseInfo.cchan[i] == tcsta)
                                        			{
                                            			stStatuSinfo.csta[j] &= 0xfc;
                                           				stStatuSinfo.csta[j] |= 0x03; //00000001-green flash
														break;
                                        			}
                                    			}
                                			}								
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("Info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.gftime > 0))

									//current phase begin to green flash
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&dctd,0,sizeof(dctd));
										dctd.mode = 29;//traffic control
										dctd.pattern = *(fcdata->patternid);
										dctd.lampcolor = 0x02;
										dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
										dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
										dctd.stageline = stInterGrateControlPhaseInfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 29;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x02;
                                        dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
                                        dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                        dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = stInterGrateControlPhaseInfo.gftime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                                		if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                		{
                                		#ifdef INTER_CONTROL_DEBUG
                                    		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                    		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                    		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                    		{
                                    		#ifdef INTER_CONTROL_DEBUG
                                        		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
                                    		}
                                		}
                            		}
                            		else
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                                		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									if (stInterGrateControlPhaseInfo.gftime > 0)
									{	
									ngf = 0;
									while (1)
									{
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cchan,0x03))
        								{
        								#ifdef INTER_CONTROL_DEBUG
            								printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															stInterGrateControlPhaseInfo.cchan,0x03,fcdata->markbit))
        								{
        								#ifdef INTER_CONTROL_DEBUG
            								printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
        								#endif
        								}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);

										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cchan,0x02))
        								{
        								#ifdef INTER_CONTROL_DEBUG
            								printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    									stInterGrateControlPhaseInfo.cchan,0x02,fcdata->markbit))
                                		{
                                		#ifdef INTER_CONTROL_DEBUG
                                    		printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                		#endif
                                		}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);
								
										ngf += 1;
										if (ngf >= stInterGrateControlPhaseInfo.gftime)
											break;
									}
									}//if (stInterGrateControlPhaseInfo.gftime > 0)
									if (1 == (stInterGrateControlPhaseInfo.cpcexist))
									{
										//current phase begin to yellow lamp
										if(SUCCESS!=igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cnpchan,0x01))
        								{
        								#ifdef INTER_CONTROL_DEBUG
            								printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
													stInterGrateControlPhaseInfo.cnpchan,0x01, fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cpchan,0x00))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																	stInterGrateControlPhaseInfo.cpchan,0x00,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cchan,0x01))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														stInterGrateControlPhaseInfo.cchan,0x01,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}

									if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.ytime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = 29;
											stStatuSinfo.color = 0x01;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.ytime;
											stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
											stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
															
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == stInterGrateControlPhaseInfo.cnpchan[i])
                    								break;
                								for (j = 0; j < stStatuSinfo.chans; j++)
                								{
                    								if (0 == stStatuSinfo.csta[j])
                        								break;
                    								tcsta = stStatuSinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (stInterGrateControlPhaseInfo.cnpchan[i] == tcsta)
                    								{
                        								stStatuSinfo.csta[j] &= 0xfc;
														stStatuSinfo.csta[j] |= 0x01; //00000001-yellow
														break;
                    								}
                								}
            								}
											for (i = 0; i < MAX_CHANNEL; i++)
                                			{
                                				if (0 == stInterGrateControlPhaseInfo.cpchan[i])
                                        			break;
                                    			for (j = 0; j < stStatuSinfo.chans; j++)
                                    			{
                                        			if (0 == stStatuSinfo.csta[j])
                                            			break;
                                        			tcsta = stStatuSinfo.csta[j];
                                        			tcsta &= 0xfc;
                                        			tcsta >>= 2;
                                        			tcsta &= 0x3f;
                                        			if (stInterGrateControlPhaseInfo.cpchan[i] == tcsta)
                                        			{
                                            			stStatuSinfo.csta[j] &= 0xfc;
														break;
                                        			}
                                    			}
                                			}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("Info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.ytime > 0))

									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            		{
                               			memset(&dctd,0,sizeof(dctd));
                               			dctd.mode = 29;//traffic control
                               			dctd.pattern = *(fcdata->patternid);
                               			dctd.lampcolor = 0x01;
                               			dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                               			dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                               			dctd.stageline = stInterGrateControlPhaseInfo.stageline;
                               			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
                               			{
                               			#ifdef INTER_CONTROL_DEBUG
                                   			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               			#endif
                               			}
                            		}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 29;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x01;
                                        dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                                        dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                        dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            		fbdata[3] = 0x01;
                            		fbdata[4] = stInterGrateControlPhaseInfo.ytime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef INTER_CONTROL_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef INTER_CONTROL_DEBUG
                                       			printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(stInterGrateControlPhaseInfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cchan,0x00))
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                                		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															stInterGrateControlPhaseInfo.cchan,0x00,fcdata->markbit))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}

									if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.rtime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = 29;
											stStatuSinfo.color = 0x00;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.rtime;
											stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
											stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
															
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == stInterGrateControlPhaseInfo.cchan[i])
                    								break;
                								for (j = 0; j < stStatuSinfo.chans; j++)
                								{
                    								if (0 == stStatuSinfo.csta[j])
                        								break;
                    								tcsta = stStatuSinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (stInterGrateControlPhaseInfo.cchan[i] == tcsta)
                    								{
                        								stStatuSinfo.csta[j] &= 0xfc;
														break;
                    								}
                								}
            								}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("Info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.rtime > 0))

									if (stInterGrateControlPhaseInfo.rtime > 0)
									{
										if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            			{
                                			memset(&dctd,0,sizeof(dctd));
                                			dctd.mode = 29;//traffic control
                                			dctd.pattern = *(fcdata->patternid);
                                			dctd.lampcolor = 0x00;
                                			dctd.lamptime = stInterGrateControlPhaseInfo.rtime;
                                			dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                			dctd.stageline = stInterGrateControlPhaseInfo.stageline;
                                			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
                                			{
                                			#ifdef INTER_CONTROL_DEBUG
                                    			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                            			}
										#ifdef I_EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&dctd,0,sizeof(dctd));
                                            dctd.mode = 29;//traffic control
                                            dctd.pattern = *(fcdata->patternid);
                                            dctd.lampcolor = 0x00;
                                            dctd.lamptime = stInterGrateControlPhaseInfo.rtime;
                                            dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                            dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#endif
										fbdata[1] = 29;
                                		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                                		fbdata[3] = 0x00;
                                		fbdata[4] = stInterGrateControlPhaseInfo.rtime;
                                		if (!wait_write_serial(*(fcdata->fbserial)))
                                		{
                                    		if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                    		{
                                    		#ifdef INTER_CONTROL_DEBUG
                                        		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
                                        		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                        		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                        		{
                                        		#ifdef INTER_CONTROL_DEBUG
                                            		printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                        		#endif
                                        		}
                                    		}
                                		}
                                		else
                                		{
                                		#ifdef INTER_CONTROL_DEBUG
                                    		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                		#endif
                                		}
										sleep(stInterGrateControlPhaseInfo.rtime);
									}

									*(fcdata->slnum) += 1;
									*(fcdata->stageid) = \
									tscdata->timeconfig->TimeConfigList[rettl][*(fcdata->slnum)].StageId;
								if (0 == (tscdata->timeconfig->TimeConfigList[rettl][*(fcdata->slnum)].StageId))
									{
										*(fcdata->slnum) = 0;
										*(fcdata->stageid) = \
										tscdata->timeconfig->TimeConfigList[rettl][*(fcdata->slnum)].StageId;
									}
                                }//not have all red/yellow flash/close
                                else
                                {//have all red or yellow flash or all close
                                    dcred = 0;
                                    close = 0;
                                    if (1 == YellowFlashYes)
                                    {
                                        pthread_cancel(YellowFlashPid);
                                        pthread_join(YellowFlashPid,NULL);
                                        YellowFlashYes = 0;
                                    }
                                }//have all red or yellow flash or all close
                            }//not have phase control
							else if (0 != cp)
							{//phase control happen
								if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
								{//if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
									rpc = 0;
                                    rpi = tscdata->timeconfig->TimeConfigList[rettl][0].PhaseId;
                                    get_phase_id(rpi,&rpc);
									memset(nc,0,sizeof(nc));
                                    pcc = nc;
                                    for (i = 0; i < (tscdata->channel->FactChannelNum); i++)
                                    {
                                        if (0 == (tscdata->channel->ChannelList[i].ChannelId))
                                            break;
                                        if (rpc == (tscdata->channel->ChannelList[i].ChannelCtrlSrc))
                                        {
											#ifdef CLOSE_LAMP
											tclc = tscdata->channel->ChannelList[i].ChannelId;
											if ((tclc >= 0x05) && (tclc <= 0x0c))
											{
												if (*(fcdata->specfunc) & (0x01 << (tclc - 5)))
													continue;
											}
											#else
											if ((*(fcdata->specfunc)&0x10)&&(*(fcdata->specfunc)&0x20))
											{
												tclc = tscdata->channel->ChannelList[i].ChannelId;
												if(((5<=tclc)&&(tclc<=8)) || ((9<=tclc)&&(tclc<=12)))
													continue;
											}
											if ((*(fcdata->specfunc)&0x10)&&(!(*(fcdata->specfunc)&0x20)))
											{
												tclc = tscdata->channel->ChannelList[i].ChannelId;
												if ((5 <= tclc) && (tclc <= 8))
													continue;
											}
											if ((*(fcdata->specfunc)&0x20)&&(!(*(fcdata->specfunc)&0x10)))
											{
												tclc = tscdata->channel->ChannelList[i].ChannelId;
												if ((9 <= tclc) && (tclc <= 12))
													continue;
											}
											#endif
                                            *pcc = tscdata->channel->ChannelList[i].ChannelId;
                                            pcc++;
                                        }
                                    }
									memset(wcc,0,sizeof(wcc));
									memset(wnpcc,0,sizeof(wnpcc));
									memset(wpcc,0,sizeof(wpcc));
									z = 0;
									k = 0;
									s = 0;
									pce = 0;
									if ((0x5a <= cp) && (cp <= 0x5d))
                                    {//if ((0x5a <= cp) && (cp <= 0x5d))
										for (i = 0; i < MAX_CHANNEL; i++)
										{
											if (0 == lkch[cp-0x5a][i])
												break;
											ce = 0;
											for (j = 0; j < MAX_CHANNEL; j++)
											{
												if (0 == nc[j])
													break;
												if (nc[j] == lkch[cp-0x5a][i])
												{
													ce = 1;
													break;
												}
											}
											if (0 == ce)
											{
												wcc[z] = lkch[cp-0x5a][i];
												z++;
												if ((0x0d <= lkch[cp-0x5a][i]) && (lkch[cp-0x5a][i] <= 0x10))
												{
													wpcc[k] = lkch[cp-0x5a][i];
													k++;
													pce = 1;
													continue;
												}
												else
												{
													wnpcc[s] = lkch[cp-0x5a][i];
													s++;
													continue;
												}
											}//if (0 == ce)
										}								
									}//if ((0x5a <= cp) && (cp <= 0x5d))
                                   	if (0xC8 == cp)
									{//if (0xC8 == cp)
										for (i = 0; i < MAX_CHANNEL; i++)
                                        {
                                            if (*(fcdata->ccontrol) & (0x00000001 << i))
                                            {
                                                tclc = i + 1;
                                                #ifdef CLOSE_LAMP
                                                if ((tclc >= 0x05) && (tclc <= 0x0c))
                                                {
                                                    if (*(fcdata->specfunc) & (0x01 << (tclc - 5)))
                                                        continue;
                                                }
                                                #else
                                                if ((*(fcdata->specfunc)&0x10)&&(*(fcdata->specfunc)&0x20))
                                                {
                                                    if(((5<=tclc)&&(tclc<=8)) || ((9<=tclc)&&(tclc<=12)))
                                                        continue;
                                                }
                                                if ((*(fcdata->specfunc)&0x10)&&(!(*(fcdata->specfunc)&0x20)))
                                                {
                                                    if ((5 <= tclc) && (tclc <= 8))
                                                        continue;
                                                }
                                                if ((*(fcdata->specfunc)&0x20)&&(!(*(fcdata->specfunc)&0x10)))
                                                {
                                                    if ((9 <= tclc) && (tclc <= 12))
                                                        continue;
                                                }
                                                #endif
                                                ce = 0;
                                                for (j = 0; j < MAX_CHANNEL; j++)
                                                {
													if (0 == nc[j])
                                                        break;
                                                    if (nc[j] == tclc)
                                                    {
                                                        ce = 1;
                                                        break;
                                                    }
                                                }
                                                if (0 == ce)
                                                {
                                                    wcc[z] = i+1;
                                                    z++;
                                                    if ((0x0d <= tclc) && (tclc <= 0x10))
                                                    {
                                                        wpcc[k] = tclc;
                                                        k++;
                                                        pce = 1;
                                                        continue;
                                                    }
                                                    else
                                                    {
                                                        wnpcc[s] = tclc;
                                                        s++;
                                                        continue;
                                                    }
                                                }
                                            }
                                        }	
									}//if (0xC8 == cp)
 	
									if((0==wcc[0])&&((stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0)))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x5a <= cp) && (cp <= 0x5d))
                                                stStatuSinfo.conmode = cp;
                                            else
                                                stStatuSinfo.conmode = 29;
											stStatuSinfo.color = 0x02;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime + stInterGrateControlPhaseInfo.ytime + stInterGrateControlPhaseInfo.rtime;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
															
											stStatuSinfo.chans = 0;
            								memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            								csta = stStatuSinfo.csta;
            								for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == lkch[cp-0x5a][i])
                    								break;
                								stStatuSinfo.chans += 1;
                								tcsta = lkch[cp-0x5a][i];
                								tcsta <<= 2;
                								tcsta &= 0xfc;
                								tcsta |= 0x02; //00000010-green 
                								*csta = tcsta;
                								csta++;
            								}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("Info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}
									if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.gftime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x5a <= cp) && (cp <= 0x5d))
                                                stStatuSinfo.conmode = cp;
                                            else
                                                stStatuSinfo.conmode = 29;
											stStatuSinfo.color = 0x03;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
															
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == wcc[i])
                    								break;
                								for (j = 0; j < stStatuSinfo.chans; j++)
                								{
                    								if (0 == stStatuSinfo.csta[j])
                        								break;
                    								tcsta = stStatuSinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (wcc[i] == tcsta)
                    								{
                        								stStatuSinfo.csta[j] &= 0xfc;
														stStatuSinfo.csta[j] |= 0x03; //00000011-green flash
														break;
                    								}
                								}
            								}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("Info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively								
									}
									//green flash
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&dctd,0,sizeof(dctd));
										dctd.mode = 29;//traffic control
										dctd.pattern = *(fcdata->patternid);
										dctd.lampcolor = 0x02;
										dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
										dctd.phaseid = 0;
										dctd.stageline = stInterGrateControlPhaseInfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 29;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x02;
                                        dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
                                        dctd.phaseid = 0;
                                        dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = stInterGrateControlPhaseInfo.gftime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef INTER_CONTROL_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef INTER_CONTROL_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									if (stInterGrateControlPhaseInfo.gftime > 0)
									{
										ngf = 0;
										while (1)
										{
											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x03,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);
									
											ngf += 1;
											if (ngf >= stInterGrateControlPhaseInfo.gftime)
												break;
										}
									}//if (stInterGrateControlPhaseInfo.gftime > 0)
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef INTER_CONTROL_DEBUG
            								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.ytime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x5a <= cp) && (cp <= 0x5d))
                                                stStatuSinfo.conmode = cp;
                                            else
                                                stStatuSinfo.conmode = 29;
											stStatuSinfo.color = 0x01;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.ytime;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
															
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == wnpcc[i])
                    								break;
                								for (j = 0; j < stStatuSinfo.chans; j++)
                								{
                    								if (0 == stStatuSinfo.csta[j])
                        								break;
                    								tcsta = stStatuSinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (wnpcc[i] == tcsta)
                    								{
                        								stStatuSinfo.csta[j] &= 0xfc;
														stStatuSinfo.csta[j] |= 0x01; //00000001-yellow
														break;
                    								}
                								}
            								}
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == wpcc[i])
                                                	break;
                                            	for (j = 0; j < stStatuSinfo.chans; j++)
                                            	{
                                                	if (0 == stStatuSinfo.csta[j])
                                                    	break;
                                                	tcsta = stStatuSinfo.csta[j];
                                                	tcsta &= 0xfc;
                                                	tcsta >>= 2;
                                                	tcsta &= 0x3f;
                                                	if (wpcc[i] == tcsta)
                                                	{
                                                    	stStatuSinfo.csta[j] &= 0xfc;
														break;
                                                	}
                                            	}
                                        	}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}

									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            		{
                            			memset(&dctd,0,sizeof(dctd));
                            			dctd.mode = 29;//traffic control
                            			dctd.pattern = *(fcdata->patternid);
                            			dctd.lampcolor = 0x01;
                            			dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                            			dctd.phaseid = 0;
                            			dctd.stageline = stInterGrateControlPhaseInfo.stageline;
                            			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
                            			{
                            			#ifdef INTER_CONTROL_DEBUG
                               				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            			#endif
                            			}
                            		}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 29;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x01;
                                        dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                                        dctd.phaseid = 0;
                                        dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            		fbdata[3] = 0x01;
                            		fbdata[4] = stInterGrateControlPhaseInfo.ytime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                            			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                            			{
                            			#ifdef INTER_CONTROL_DEBUG
                               				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                			{
                                			#ifdef INTER_CONTROL_DEBUG
                                   				printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(stInterGrateControlPhaseInfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                                		printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
									if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.rtime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x5a <= cp) && (cp <= 0x5d))
                                                stStatuSinfo.conmode = cp;
                                            else
                                                stStatuSinfo.conmode = 29;
											stStatuSinfo.color = 0x00;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.rtime;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
															
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == wcc[i])
                                                	break;
                                            	for (j = 0; j < stStatuSinfo.chans; j++)
                                            	{
                                                	if (0 == stStatuSinfo.csta[j])
                                                    	break;
                                                	tcsta = stStatuSinfo.csta[j];
                                                	tcsta &= 0xfc;
                                                	tcsta >>= 2;
                                                	tcsta &= 0x3f;
                                                	if (wcc[i] == tcsta)
                                                	{
                                                    	stStatuSinfo.csta[j] &= 0xfc;
														break;
                                                	}
                                            	}
                                        	}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}
									*(fcdata->slnum) = 0;
                                    *(fcdata->stageid) = tscdata->timeconfig->TimeConfigList[rettl][0].StageId;
								}//if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
								else if ((0x01 <= cp) && (cp <= 0x20))
								{//else if ((0x01 <= cp) && (cp <= 0x20))
									unsigned char			tsl = 0;
									unsigned char			pexist = 0;
									for (i = 0; i < (tscdata->timeconfig->FactStageNum); i++)
									{
										if (0 == (tscdata->timeconfig->TimeConfigList[rettl][i].StageId))
											break;
										rpi = tscdata->timeconfig->TimeConfigList[rettl][i].PhaseId;
										rpc = 0;
										get_phase_id(rpi,&rpc);
										if (cp == rpc)
										{
											pexist = 1;	
											break;
										}
									}	
									if (1 == pexist)
									{//control phase belong to phase list;
										if (0 == tscdata->timeconfig->TimeConfigList[rettl][i+1].StageId)
										{
											*(fcdata->slnum) = 0;
											*(fcdata->stageid) = \
												tscdata->timeconfig->TimeConfigList[rettl][0].StageId;
											rpc = 0;
											rpi = tscdata->timeconfig->TimeConfigList[rettl][0].PhaseId;
											get_phase_id(rpi,&rpc);
										}
										else
										{
											*(fcdata->slnum) = i + 1;
											*(fcdata->stageid) = \
												tscdata->timeconfig->TimeConfigList[rettl][i+1].StageId;
											rpc = 0;
											rpi = tscdata->timeconfig->TimeConfigList[rettl][i+1].PhaseId;
											get_phase_id(rpi,&rpc);
										}
										memset(nc,0,sizeof(nc));
										pcc = nc;
										for (i = 0; i < (tscdata->channel->FactChannelNum); i++)
										{
											if (0 == (tscdata->channel->ChannelList[i].ChannelId))
												break;
											if (rpc == (tscdata->channel->ChannelList[i].ChannelCtrlSrc))
											{
												#ifdef CLOSE_LAMP
                                                tclc = tscdata->channel->ChannelList[i].ChannelId;
                                                if ((tclc >= 0x05) && (tclc <= 0x0c))
                                                {
                                                    if (*(fcdata->specfunc) & (0x01 << (tclc - 5)))
                                                        continue;
                                                }
												#else
												if ((*(fcdata->specfunc)&0x10)&&(*(fcdata->specfunc)&0x20))
            									{
                									tclc = tscdata->channel->ChannelList[i].ChannelId;
                									if(((5<=tclc)&&(tclc<=8)) || ((9<=tclc)&&(tclc<=12)))
                    									continue;
            									}
												if ((*(fcdata->specfunc)&0x10)&&(!(*(fcdata->specfunc)&0x20)))
												{
													tclc = tscdata->channel->ChannelList[i].ChannelId;
													if ((5 <= tclc) && (tclc <= 8))
														continue;
												}
												if ((*(fcdata->specfunc)&0x20)&&(!(*(fcdata->specfunc)&0x10)))
												{
													tclc = tscdata->channel->ChannelList[i].ChannelId;
                									if ((9 <= tclc) && (tclc <= 12))
                    									continue;
												}
                                                #endif
												*pcc = tscdata->channel->ChannelList[i].ChannelId;
												pcc++;
											}
										}
										memset(cc,0,sizeof(cc));
										pcc = cc;
										memset(wcc,0,sizeof(wcc));
										memset(wnpcc,0,sizeof(wnpcc));
										memset(wpcc,0,sizeof(wpcc));
										z = 0;
										k = 0;
										s = 0;
										pce = 0;
										for (i = 0; i < (tscdata->channel->FactChannelNum); i++)
										{
											if (0 == (tscdata->channel->ChannelList[i].ChannelId))
												break;
											if (cp == (tscdata->channel->ChannelList[i].ChannelCtrlSrc))
											{
												#ifdef CLOSE_LAMP
                                                tclc = tscdata->channel->ChannelList[i].ChannelId;
                                                if ((tclc >= 0x05) && (tclc <= 0x0c))
                                                {
                                                    if (*(fcdata->specfunc) & (0x01 << (tclc - 5)))
                                                        continue;
                                                }
												#else
												if ((*(fcdata->specfunc)&0x10)&&(*(fcdata->specfunc)&0x20))
            									{
                									tclc = tscdata->channel->ChannelList[i].ChannelId;
                									if(((5<=tclc)&&(tclc<=8)) || ((9<=tclc)&&(tclc<=12)))
                    									continue;
            									}
												if ((*(fcdata->specfunc)&0x10)&&(!(*(fcdata->specfunc)&0x20)))
												{
													tclc = tscdata->channel->ChannelList[i].ChannelId;
													if ((5 <= tclc) && (tclc <= 8))
														continue;
												}
												if ((*(fcdata->specfunc)&0x20)&&(!(*(fcdata->specfunc)&0x10)))
												{
													tclc = tscdata->channel->ChannelList[i].ChannelId;
                									if ((9 <= tclc) && (tclc <= 12))
                    									continue;
												}
                                                #endif
												*pcc = tscdata->channel->ChannelList[i].ChannelId;
												pcc++;
												ce = 0;
												for (j = 0; j < MAX_CHANNEL; j++)
												{
													if (0 == nc[j])
														break;
													if (nc[j] == tscdata->channel->ChannelList[i].ChannelId)
													{
														ce = 1;
														break;
													}
												}
												if (0 == ce)
												{
													wcc[z] = tscdata->channel->ChannelList[i].ChannelId;
													z++;
													if (3 == tscdata->channel->ChannelList[i].ChannelType)
													{
														wpcc[k] = tscdata->channel->ChannelList[i].ChannelId;
														k++;
														pce = 1;
														continue;
													}
													else
													{
														wnpcc[s] = tscdata->channel->ChannelList[i].ChannelId;
														s++;
														continue;
													}		
												}
											}
										}
										if((0==wcc[0])&&((stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0)))
										{
											if((!(*(fcdata->markbit)&0x1000))&&(!(*(fcdata->markbit)&0x8000)))
											{//report info to server actively
												stStatuSinfo.conmode = 29;
												stStatuSinfo.color = 0x02;
												stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime + stInterGrateControlPhaseInfo.ytime + stInterGrateControlPhaseInfo.rtime;
												stStatuSinfo.stage = 0;
												stStatuSinfo.cyclet = 0;
												stStatuSinfo.phase = 0;
												stStatuSinfo.phase |= (0x01 << (cp - 1));
																
												stStatuSinfo.chans = 0;
												memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
												csta = stStatuSinfo.csta;
												for (i = 0; i < MAX_CHANNEL; i++)
												{
													if (0 == cc[i])
														break;
													stStatuSinfo.chans += 1;
													tcsta = cc[i];
													tcsta <<= 2;
													tcsta &= 0xfc;
													tcsta |= 0x02; //00000010-green 
													*csta = tcsta;
													csta++;
												}
												memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
												memset(sibuf,0,sizeof(sibuf));
												if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
												{
												#ifdef INTER_CONTROL_DEBUG
													printf("Info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
												else
												{
													write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
												}
											}//report info to server actively
										}
										if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.gftime > 0))
										{
											if((!(*(fcdata->markbit)&0x1000))&&(!(*(fcdata->markbit)&0x8000)))
											{//report info to server actively
												stStatuSinfo.conmode = 29;
												stStatuSinfo.color = 0x03;
												stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime;
												stStatuSinfo.stage = 0;
												stStatuSinfo.cyclet = 0;
												stStatuSinfo.phase = 0;
												stStatuSinfo.phase |= (0x01 << (cp - 1));
																
												for (i = 0; i < MAX_CHANNEL; i++)
												{
													if (0 == wcc[i])
														break;
													for (j = 0; j < stStatuSinfo.chans; j++)
													{
														if (0 == stStatuSinfo.csta[j])
															break;
														tcsta = stStatuSinfo.csta[j];
														tcsta &= 0xfc;
														tcsta >>= 2;
														tcsta &= 0x3f;
														if (wcc[i] == tcsta)
														{
															stStatuSinfo.csta[j] &= 0xfc;
															stStatuSinfo.csta[j] |= 0x03; //00000011-green flash
															break;
														}
													}
												}
												memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
												memset(sibuf,0,sizeof(sibuf));
												if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
												{
												#ifdef INTER_CONTROL_DEBUG
													printf("Info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
												else
												{
													write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
												}
											}//report info to server actively								
										}
										//green flash
										if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
										{
											memset(&dctd,0,sizeof(dctd));
											dctd.mode = 29;//traffic control
											dctd.pattern = *(fcdata->patternid);
											dctd.lampcolor = 0x02;
											dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
											dctd.phaseid = cp;
											dctd.stageline = stInterGrateControlPhaseInfo.stageline;
											if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#ifdef I_EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&dctd,0,sizeof(dctd));
                                            dctd.mode = 29;//traffic control
                                            dctd.pattern = *(fcdata->patternid);
                                            dctd.lampcolor = 0x02;
                                            dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
                                            dctd.phaseid = cp;
                                            dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#endif
										fbdata[1] = 29;
										fbdata[2] = stInterGrateControlPhaseInfo.stageline;
										fbdata[3] = 0x02;
										fbdata[4] = stInterGrateControlPhaseInfo.gftime;
										if (!wait_write_serial(*(fcdata->fbserial)))
										{
											if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
												update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
												if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
												{
												#ifdef INTER_CONTROL_DEBUG
													printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
											}
										}
										else
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("face board serial port cannot write,Line:%d\n",__LINE__);
										#endif
										}
										if (stInterGrateControlPhaseInfo.gftime > 0)
										{
										ngf = 0;
										while (1)
										{
											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x03,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);
									
											ngf += 1;
											if (ngf >= stInterGrateControlPhaseInfo.gftime)
												break;
										}
										}//if (stInterGrateControlPhaseInfo.gftime > 0)
										if (1 == pce)
										{
											//current phase begin to yellow lamp
											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wnpcc,0x01, fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wpcc,0x00,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										else
										{
											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x01,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.ytime > 0))
										{
											if((!(*(fcdata->markbit)&0x1000))&&(!(*(fcdata->markbit)&0x8000)))
											{//report info to server actively
												stStatuSinfo.conmode = 29;
												stStatuSinfo.color = 0x01;
												stStatuSinfo.time = stInterGrateControlPhaseInfo.ytime;
												stStatuSinfo.stage = 0;
												stStatuSinfo.cyclet = 0;
												stStatuSinfo.phase = 0;
												stStatuSinfo.phase |= (0x01 << (cp - 1));
																
												for (i = 0; i < MAX_CHANNEL; i++)
												{
													if (0 == wnpcc[i])
														break;
													for (j = 0; j < stStatuSinfo.chans; j++)
													{
														if (0 == stStatuSinfo.csta[j])
															break;
														tcsta = stStatuSinfo.csta[j];
														tcsta &= 0xfc;
														tcsta >>= 2;
														tcsta &= 0x3f;
														if (wnpcc[i] == tcsta)
														{
															stStatuSinfo.csta[j] &= 0xfc;
															stStatuSinfo.csta[j] |= 0x01; //00000001-yellow
															break;
														}
													}
												}
												for (i = 0; i < MAX_CHANNEL; i++)
												{
													if (0 == wpcc[i])
														break;
													for (j = 0; j < stStatuSinfo.chans; j++)
													{
														if (0 == stStatuSinfo.csta[j])
															break;
														tcsta = stStatuSinfo.csta[j];
														tcsta &= 0xfc;
														tcsta >>= 2;
														tcsta &= 0x3f;
														if (wpcc[i] == tcsta)
														{
															stStatuSinfo.csta[j] &= 0xfc;
															break;
														}
													}
												}
												memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
												memset(sibuf,0,sizeof(sibuf));
												if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
												{
												#ifdef INTER_CONTROL_DEBUG
													printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
												else
												{
													write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
												}
											}//report info to server actively
										}

										if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
										{
											memset(&dctd,0,sizeof(dctd));
											dctd.mode = 29;//traffic control
											dctd.pattern = *(fcdata->patternid);
											dctd.lampcolor = 0x01;
											dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
											dctd.phaseid = cp;
											dctd.stageline = stInterGrateControlPhaseInfo.stageline;
											if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#ifdef I_EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&dctd,0,sizeof(dctd));
                                            dctd.mode = 29;//traffic control
                                            dctd.pattern = *(fcdata->patternid);
                                            dctd.lampcolor = 0x01;
                                            dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                                            dctd.phaseid = cp;
                                            dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#endif
										fbdata[1] = 29;
										fbdata[2] = stInterGrateControlPhaseInfo.stageline;
										fbdata[3] = 0x01;
										fbdata[4] = stInterGrateControlPhaseInfo.ytime;
										if (!wait_write_serial(*(fcdata->fbserial)))
										{
											if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
												update_event_list(fcdata->ec,fcdata->el,1,16, \
																		ct.tv_sec,fcdata->markbit);
												if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
												{
												#ifdef INTER_CONTROL_DEBUG
													printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
											}
										}
										else
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("face board serial port cannot write,Line:%d\n",__LINE__);
										#endif
										}
										sleep(stInterGrateControlPhaseInfo.ytime);

										//current phase begin to red lamp
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x00,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.rtime > 0))
										{
											if((!(*(fcdata->markbit)&0x1000))&&(!(*(fcdata->markbit)&0x8000)))
											{//report info to server actively
												stStatuSinfo.conmode = 29;
												stStatuSinfo.color = 0x00;
												stStatuSinfo.time = stInterGrateControlPhaseInfo.rtime;
												stStatuSinfo.stage = 0;
												stStatuSinfo.cyclet = 0;
												stStatuSinfo.phase = 0;
												stStatuSinfo.phase |= (0x01 << (cp - 1));
																
												for (i = 0; i < MAX_CHANNEL; i++)
												{
													if (0 == wcc[i])
														break;
													for (j = 0; j < stStatuSinfo.chans; j++)
													{
														if (0 == stStatuSinfo.csta[j])
															break;
														tcsta = stStatuSinfo.csta[j];
														tcsta &= 0xfc;
														tcsta >>= 2;
														tcsta &= 0x3f;
														if (wcc[i] == tcsta)
														{
															stStatuSinfo.csta[j] &= 0xfc;
															break;
														}
													}
												}
												memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
												memset(sibuf,0,sizeof(sibuf));
												if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
												{
												#ifdef INTER_CONTROL_DEBUG
													printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
												else
												{
													write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
												}
											}//report info to server actively
										}
									}//control phase belong to phase list;
									else
									{//control phase don't belong to phase list;
										rpc = 0;
										rpi = tscdata->timeconfig->TimeConfigList[rettl][0].PhaseId;
										get_phase_id(rpi,&rpc);

										memset(nc,0,sizeof(nc));
										pcc = nc;
										for (i = 0; i < (tscdata->channel->FactChannelNum); i++)
										{
											if (0 == (tscdata->channel->ChannelList[i].ChannelId))
												break;
											if (rpc == (tscdata->channel->ChannelList[i].ChannelCtrlSrc))
											{
												#ifdef CLOSE_LAMP
                                                tclc = tscdata->channel->ChannelList[i].ChannelId;
                                                if ((tclc >= 0x05) && (tclc <= 0x0c))
                                                {
                                                    if (*(fcdata->specfunc) & (0x01 << (tclc - 5)))
                                                        continue;
                                                }
												#else
												if ((*(fcdata->specfunc)&0x10)&&(*(fcdata->specfunc)&0x20))
            									{
                									tclc = tscdata->channel->ChannelList[i].ChannelId;
                									if(((5<=tclc)&&(tclc<=8)) || ((9<=tclc)&&(tclc<=12)))
                    									continue;
            									}
												if ((*(fcdata->specfunc)&0x10)&&(!(*(fcdata->specfunc)&0x20)))
												{
													tclc = tscdata->channel->ChannelList[i].ChannelId;
													if ((5 <= tclc) && (tclc <= 8))
														continue;
												}
												if ((*(fcdata->specfunc)&0x20)&&(!(*(fcdata->specfunc)&0x10)))
												{
													tclc = tscdata->channel->ChannelList[i].ChannelId;
                									if ((9 <= tclc) && (tclc <= 12))
                    									continue;
												}
                                                #endif
												*pcc = tscdata->channel->ChannelList[i].ChannelId;
												pcc++;
											}
										}
										memset(cc,0,sizeof(cc));
										pcc = cc;
										memset(wcc,0,sizeof(wcc));
										memset(wnpcc,0,sizeof(wnpcc));
										memset(wpcc,0,sizeof(wpcc));
										z = 0;
										k = 0;
										s = 0;
										pce = 0;
										for (i = 0; i < (tscdata->channel->FactChannelNum); i++)
										{
											if (0 == (tscdata->channel->ChannelList[i].ChannelId))
												break;
											if (cp == (tscdata->channel->ChannelList[i].ChannelCtrlSrc))
											{
												#ifdef CLOSE_LAMP
                                                tclc = tscdata->channel->ChannelList[i].ChannelId;
                                                if ((tclc >= 0x05) && (tclc <= 0x0c))
                                                {
                                                    if (*(fcdata->specfunc) & (0x01 << (tclc - 5)))
                                                        continue;
                                                }
												#else
												if ((*(fcdata->specfunc)&0x10)&&(*(fcdata->specfunc)&0x20))
            									{
                									tclc = tscdata->channel->ChannelList[i].ChannelId;
                									if(((5<=tclc)&&(tclc<=8)) || ((9<=tclc)&&(tclc<=12)))
                    									continue;
            									}
												if ((*(fcdata->specfunc)&0x10)&&(!(*(fcdata->specfunc)&0x20)))
												{
													tclc = tscdata->channel->ChannelList[i].ChannelId;
													if ((5 <= tclc) && (tclc <= 8))
														continue;
												}
												if ((*(fcdata->specfunc)&0x20)&&(!(*(fcdata->specfunc)&0x10)))
												{
													tclc = tscdata->channel->ChannelList[i].ChannelId;
                									if ((9 <= tclc) && (tclc <= 12))
                    									continue;
												}
                                                #endif
												*pcc = tscdata->channel->ChannelList[i].ChannelId;
												pcc++;
												ce = 0;
												for (j = 0; j < MAX_CHANNEL; j++)
												{
													if (0 == nc[j])
														break;
													if (nc[j] == tscdata->channel->ChannelList[i].ChannelId)
													{
														ce = 1;
														break;
													}
												}
												if (0 == ce)
												{
													wcc[z] = tscdata->channel->ChannelList[i].ChannelId;
													z++;
													if (3 == tscdata->channel->ChannelList[i].ChannelType)
													{
														wpcc[k] = tscdata->channel->ChannelList[i].ChannelId;
														k++;
														pce = 1;
														continue;
													}
													else
													{
														wnpcc[s] = tscdata->channel->ChannelList[i].ChannelId;
														s++;
														continue;
													}		
												}
											}
										}
										if((0==wcc[0])&&((stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0)))
										{
											if((!(*(fcdata->markbit)&0x1000))&&(!(*(fcdata->markbit)&0x8000)))
											{//report info to server actively
												stStatuSinfo.conmode = 29;
												stStatuSinfo.color = 0x02;
												stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime + stInterGrateControlPhaseInfo.ytime + stInterGrateControlPhaseInfo.rtime;
												stStatuSinfo.stage = 0;
												stStatuSinfo.cyclet = 0;
												stStatuSinfo.phase = 0;
												stStatuSinfo.phase |= (0x01 << (cp - 1));
																
												stStatuSinfo.chans = 0;
												memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
												csta = stStatuSinfo.csta;
												for (i = 0; i < MAX_CHANNEL; i++)
												{
													if (0 == cc[i])
														break;
													stStatuSinfo.chans += 1;
													tcsta = cc[i];
													tcsta <<= 2;
													tcsta &= 0xfc;
													tcsta |= 0x02; //00000010-green 
													*csta = tcsta;
													csta++;
												}
												memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
												memset(sibuf,0,sizeof(sibuf));
												if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
												{
												#ifdef INTER_CONTROL_DEBUG
													printf("Info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
												else
												{
													write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
												}
											}//report info to server actively
										}
										if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.gftime > 0))
										{
											if((!(*(fcdata->markbit)&0x1000))&&(!(*(fcdata->markbit)&0x8000)))
											{//report info to server actively
												stStatuSinfo.conmode = 29;
												stStatuSinfo.color = 0x03;
												stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime;
												stStatuSinfo.stage = 0;
												stStatuSinfo.cyclet = 0;
												stStatuSinfo.phase = 0;
												stStatuSinfo.phase |= (0x01 << (cp - 1));
																
												for (i = 0; i < MAX_CHANNEL; i++)
												{
													if (0 == wcc[i])
														break;
													for (j = 0; j < stStatuSinfo.chans; j++)
													{
														if (0 == stStatuSinfo.csta[j])
															break;
														tcsta = stStatuSinfo.csta[j];
														tcsta &= 0xfc;
														tcsta >>= 2;
														tcsta &= 0x3f;
														if (wcc[i] == tcsta)
														{
															stStatuSinfo.csta[j] &= 0xfc;
															stStatuSinfo.csta[j] |= 0x03; //00000011-green flash
															break;
														}
													}
												}
												memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
												memset(sibuf,0,sizeof(sibuf));
												if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
												{
												#ifdef INTER_CONTROL_DEBUG
													printf("Info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
												else
												{
													write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
												}
											}//report info to server actively								
										}
										//green flash
										if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
										{
											memset(&dctd,0,sizeof(dctd));
											dctd.mode = 29;//traffic control
											dctd.pattern = *(fcdata->patternid);
											dctd.lampcolor = 0x02;
											dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
											dctd.phaseid = cp;
											dctd.stageline = stInterGrateControlPhaseInfo.stageline;
											if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#ifdef I_EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&dctd,0,sizeof(dctd));
                                            dctd.mode = 29;//traffic control
                                            dctd.pattern = *(fcdata->patternid);
                                            dctd.lampcolor = 0x02;
                                            dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
                                            dctd.phaseid = cp;
                                            dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#endif
										fbdata[1] = 29;
										fbdata[2] = stInterGrateControlPhaseInfo.stageline;
										fbdata[3] = 0x02;
										fbdata[4] = stInterGrateControlPhaseInfo.gftime;
										if (!wait_write_serial(*(fcdata->fbserial)))
										{
											if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
												update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
												if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
												{
												#ifdef INTER_CONTROL_DEBUG
													printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
											}
										}
										else
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("face board serial port cannot write,Line:%d\n",__LINE__);
										#endif
										}
										if (stInterGrateControlPhaseInfo.gftime > 0)
										{
										ngf = 0;
										while (1)
										{
											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x03,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);
									
											ngf += 1;
											if (ngf >= stInterGrateControlPhaseInfo.gftime)
												break;
										}
										}//if (stInterGrateControlPhaseInfo.gftime > 0)
										if (1 == pce)
										{
											//current phase begin to yellow lamp
											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wnpcc,0x01, fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wpcc,0x00,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										else
										{
											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x01,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.ytime > 0))
										{
											if((!(*(fcdata->markbit)&0x1000))&&(!(*(fcdata->markbit)&0x8000)))
											{//report info to server actively
												stStatuSinfo.conmode = 29;
												stStatuSinfo.color = 0x01;
												stStatuSinfo.time = stInterGrateControlPhaseInfo.ytime;
												stStatuSinfo.stage = 0;
												stStatuSinfo.cyclet = 0;
												stStatuSinfo.phase = 0;
												stStatuSinfo.phase |= (0x01 << (cp - 1));
																
												for (i = 0; i < MAX_CHANNEL; i++)
												{
													if (0 == wnpcc[i])
														break;
													for (j = 0; j < stStatuSinfo.chans; j++)
													{
														if (0 == stStatuSinfo.csta[j])
															break;
														tcsta = stStatuSinfo.csta[j];
														tcsta &= 0xfc;
														tcsta >>= 2;
														tcsta &= 0x3f;
														if (wnpcc[i] == tcsta)
														{
															stStatuSinfo.csta[j] &= 0xfc;
															stStatuSinfo.csta[j] |= 0x01; //00000001-yellow
															break;
														}
													}
												}
												for (i = 0; i < MAX_CHANNEL; i++)
												{
													if (0 == wpcc[i])
														break;
													for (j = 0; j < stStatuSinfo.chans; j++)
													{
														if (0 == stStatuSinfo.csta[j])
															break;
														tcsta = stStatuSinfo.csta[j];
														tcsta &= 0xfc;
														tcsta >>= 2;
														tcsta &= 0x3f;
														if (wpcc[i] == tcsta)
														{
															stStatuSinfo.csta[j] &= 0xfc;
															break;
														}
													}
												}
												memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
												memset(sibuf,0,sizeof(sibuf));
												if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
												{
												#ifdef INTER_CONTROL_DEBUG
													printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
												else
												{
													write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
												}
											}//report info to server actively
										}

										if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
										{
											memset(&dctd,0,sizeof(dctd));
											dctd.mode = 29;//traffic control
											dctd.pattern = *(fcdata->patternid);
											dctd.lampcolor = 0x01;
											dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
											dctd.phaseid = cp;
											dctd.stageline = stInterGrateControlPhaseInfo.stageline;
											if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#ifdef I_EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&dctd,0,sizeof(dctd));
                                            dctd.mode = 29;//traffic control
                                            dctd.pattern = *(fcdata->patternid);
                                            dctd.lampcolor = 0x01;
                                            dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                                            dctd.phaseid = cp;
                                            dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#endif
										fbdata[1] = 29;
										fbdata[2] = stInterGrateControlPhaseInfo.stageline;
										fbdata[3] = 0x01;
										fbdata[4] = stInterGrateControlPhaseInfo.ytime;
										if (!wait_write_serial(*(fcdata->fbserial)))
										{
											if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
												update_event_list(fcdata->ec,fcdata->el,1,16, \
																		ct.tv_sec,fcdata->markbit);
												if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
												{
												#ifdef INTER_CONTROL_DEBUG
													printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
											}
										}
										else
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("face board serial port cannot write,Line:%d\n",__LINE__);
										#endif
										}
										sleep(stInterGrateControlPhaseInfo.ytime);

										//current phase begin to red lamp
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x00,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.rtime > 0))
										{
											if((!(*(fcdata->markbit)&0x1000))&&(!(*(fcdata->markbit)&0x8000)))
											{//report info to server actively
												stStatuSinfo.conmode = 29;
												stStatuSinfo.color = 0x00;
												stStatuSinfo.time = stInterGrateControlPhaseInfo.rtime;
												stStatuSinfo.stage = 0;
												stStatuSinfo.cyclet = 0;
												stStatuSinfo.phase = 0;
												stStatuSinfo.phase |= (0x01 << (cp - 1));
																
												for (i = 0; i < MAX_CHANNEL; i++)
												{
													if (0 == wcc[i])
														break;
													for (j = 0; j < stStatuSinfo.chans; j++)
													{
														if (0 == stStatuSinfo.csta[j])
															break;
														tcsta = stStatuSinfo.csta[j];
														tcsta &= 0xfc;
														tcsta >>= 2;
														tcsta &= 0x3f;
														if (wcc[i] == tcsta)
														{
															stStatuSinfo.csta[j] &= 0xfc;
															break;
														}
													}
												}
												memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
												memset(sibuf,0,sizeof(sibuf));
												if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
												{
												#ifdef INTER_CONTROL_DEBUG
													printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
												else
												{
													write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
												}
											}//report info to server actively
										}
										*(fcdata->slnum) = 0;
										*(fcdata->stageid) = \
											tscdata->timeconfig->TimeConfigList[rettl][0].StageId;
									}//control phase don't belong to phase list;
								}//else if ((0x01 <= cp) && (cp <= 0x20)) 
							}//if (0 != cp)
							new_all_red(&ardata);
									
							*(fcdata->contmode) = contmode; //restore control mode;
							cp = 0;
							ncmode = *(fcdata->contmode);
							*(fcdata->markbit2) &= 0xfff7;
							*(fcdata->fcontrol) = 0;
							*(fcdata->ccontrol) = 0;
							*(fcdata->trans) = 0;
							if (0 == InterGratedControlYes)
                            {
                                memset(&stInterGrateControlData,0,sizeof(stInterGrateControlData));
                                memset(&stInterGrateControlPhaseInfo,0,sizeof(stInterGrateControlPhaseInfo));
                                stInterGrateControlData.fd = fcdata;
                                stInterGrateControlData.td = tscdata;
                                stInterGrateControlData.pi = &stInterGrateControlPhaseInfo;
                                stInterGrateControlData.cs = chanstatus;
                                int dcret = pthread_create(&InterGratedControlPid,NULL, \
                                                    (void *)Start_Inter_Grate_Control,&stInterGrateControlData);
                                if (0 != dcret)
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    output_log("InterGrate control,create full InterGrate thread err");
                                #endif
                                    Inter_Control_End_Part_Child_Thread();
                                    return FAIL;
                                }
                                InterGratedControlYes = 1;
                            }	
							
							objecti[0].objectv[0] = 0xf3;
							factovs = 0;
							memset(cbuf,0,sizeof(cbuf));
							if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                            {
                            #ifdef INTER_CONTROL_DEBUG
                            	printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }
							if (!(*(fcdata->markbit) & 0x1000))
							{
                            	write(*(fcdata->sockfd->csockfd),cbuf,factovs);
							}
							gettimeofday(&ct,NULL);
                            update_event_list(fcdata->ec,fcdata->el,1,85,ct.tv_sec,fcdata->markbit);
							continue;
						}//net control will end
						if ((1 == netlock) && ((0xf0 != tcbuf[1]) || (0xf1 != tcbuf[1])))
						{//status of network control
							if (((0x5a <= tcbuf[1]) && (tcbuf[1] <= 0x5d)) || (0xC8 == tcbuf[1]))
							{//if (((0x5a <= tcbuf[1]) && (tcbuf[1] <= 0x5d)) || (0xC8 == tcbuf[1]))
								if ((1 == dcred) || (1 == YellowFlashYes) || (1 == close) || (1 == phcon))
                            	{//if ((1 == tcred) || (1 == tcyfyes) || (1 == close))
									fpc = 1;
									phcon = 0;
									cp = tcbuf[1];
                                	if (1 == YellowFlashYes)
                                	{
                                    	pthread_cancel(YellowFlashPid);
                                    	pthread_join(YellowFlashPid,NULL);
                                    	YellowFlashYes = 0;
                                	}
									if (1 != dcred)
									{
										new_all_red(&ardata);
									}
									dcred = 0;
									close = 0;
					//				#ifdef CLOSE_LAMP
                                    igc_set_lamp_color(*(fcdata->bbserial),clch,0x03);
									update_channel_status(fcdata->sockfd,chanstatus,clch,0x03,fcdata->markbit);
                    //                #endif
									if (0xC8 == tcbuf[1])
									{//if (0xC8 == tcbuf[1])
										memset(ccon,0,sizeof(ccon));
                                        j = 0;
                                        for (i = 0; i < MAX_CHANNEL; i++)
                                        {
                                            if (*(fcdata->ccontrol) & (0x00000001 << i))
                                            {
                                                #ifdef CLOSE_LAMP
                                                if (((i+1) >= 5) && ((i+1) <= 12))
                                                {
                                                    if (*(fcdata->specfunc) & (0x01 << ((i+1) - 5)))
                                                        continue;
                                                }
                                                #else
                                                if ((*(fcdata->specfunc)&0x10)&&(*(fcdata->specfunc)&0x20))
                                                {
                                                    tclc = i + 1;
                                                    if(((5<=tclc)&&(tclc<=8)) || ((9<=tclc)&&(tclc<=12)))
                                                        continue;
                                                }
                                                if ((*(fcdata->specfunc)&0x10)&&(!(*(fcdata->specfunc)&0x20)))
                                                {
                                                    tclc = i + 1;
                                                    if ((5 <= tclc) && (tclc <= 8))
                                                        continue;
                                                }
                                                if ((*(fcdata->specfunc)&0x20)&&(!(*(fcdata->specfunc)&0x10)))
                                                {
                                                    tclc = i + 1;
                                                    if ((9 <= tclc) && (tclc <= 12))
                                                        continue;
                                                }
												#endif
                                                ccon[j] = i + 1;
                                                j++;
                                            }
                                        }
                                        if(SUCCESS!=igc_set_lamp_color(*(fcdata->bbserial),ccon,0x02))
                                        {
                                        #ifdef INTER_CONTROL_DEBUG
                                            printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                            *(fcdata->markbit) |= 0x0800;
                                        }
                                        if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                                                            ccon,0x02,fcdata->markbit))
                                        {
                                        #ifdef INTER_CONTROL_DEBUG
                                            printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                        #endif
                                        }
                                        if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
                                        {//report info to server actively
                                            stStatuSinfo.conmode = 29;
                                            stStatuSinfo.color = 0x02;
                                            stStatuSinfo.time = 0;
                                            stStatuSinfo.stage = 0;
                                            stStatuSinfo.cyclet = 0;
                                            stStatuSinfo.phase = 0;
                                            stStatuSinfo.chans = 0;
                                            memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
                                            csta = stStatuSinfo.csta;
                                            for (i = 0; i < MAX_CHANNEL; i++)
											{
                                                if (0 == ccon[i])
                                                    break;
                                                stStatuSinfo.chans += 1;
                                                tcsta = ccon[i];
                                                tcsta <<= 2;
                                                tcsta &= 0xfc;
                                                tcsta |= 0x02; //00000010-green 
                                                *csta = tcsta;
                                                csta++;
                                            }

                                            memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
                                            memset(sibuf,0,sizeof(sibuf));
                                            if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
                                            {
                                            #ifdef INTER_CONTROL_DEBUG
                                                printf("status info pack err,File:%s,Line:%d\n", \
                                                        __FILE__,__LINE__);
                                            #endif
                                            }
                                            else
                                            {
                                                write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
                                            }
                                        }//report info to server actively
                                        gettimeofday(&ct,NULL);
                                        update_event_list(fcdata->ec,fcdata->el,1,127, \
                                                            ct.tv_sec,fcdata->markbit);	
										*(fcdata->fcontrol) = *(fcdata->ccontrol); //added by sk on 20180512
									}//if (0xC8 == tcbuf[1])
									if ((0x5a <= tcbuf[1]) && (tcbuf[1] <= 0x5d))
                                    {//if ((0x5a <= tcbuf[1]) && (tcbuf[1] <= 0x5d))	
										if(SUCCESS!=igc_set_lamp_color(*(fcdata->bbserial),lkch[cp-0x5a],0x02))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS!=update_channel_status(fcdata->sockfd,chanstatus, \
																		lkch[cp-0x5a],0x02,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = cp;
											stStatuSinfo.color = 0x02;
											stStatuSinfo.time = 0;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
										/*	
											stStatuSinfo.chans = 0;
											memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
											csta = stStatuSinfo.csta;
											for (i = 0; i < MAX_CHANNEL; i++)
											{
												if (0 == lkch[cp-0x5a][i])
													break;
												stStatuSinfo.chans += 1;
												tcsta = lkch[cp-0x5a][i];
												tcsta <<= 2;
												tcsta &= 0xfc;
												tcsta |= 0x02; //00000010-green 
												*csta = tcsta;
												csta++;
											}
										*/
											stStatuSinfo.chans = 0;
											memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
											csta = stStatuSinfo.csta;
											for (i = 0; i < MAX_CHANNEL; i++)
											{
												if (0 == lkch[cp-0x5a][i])
													break;
												stStatuSinfo.chans += 1;
												tcsta = lkch[cp-0x5a][i];
												tcsta <<= 2;
												tcsta &= 0xfc;
												tcsta |= 0x02; //00000010-green 
												*csta = tcsta;
												csta++;
											}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
											memset(sibuf,0,sizeof(sibuf));
											if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("status info pack err,File:%s,Line:%d\n", \
																				__FILE__,__LINE__);
											#endif
											}
											else
											{
												write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
											}
										}//report info to server actively
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,89,ct.tv_sec,fcdata->markbit);
									}//if ((0x5a <= tcbuf[1]) && (tcbuf[1] <= 0x5d))
                                	continue;
                            	}//if ((1 == tcred) || (1 == tcyfyes) || (1 == close))
								if (0 == fpc)
								{//phase control of first times
									fpc = 1;
									cp = stInterGrateControlPhaseInfo.phaseid;
								}//phase control of first times

								if ((cp == tcbuf[1]) && (0xC8 != cp))
								{
									if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
									{
										if ((0x5a <= cp) && (cp <= 0x5d))
                                            stStatuSinfo.conmode = cp;
                                        else
                                            stStatuSinfo.conmode = 29;
										stStatuSinfo.color = 0x02;
										stStatuSinfo.time = 0;
										stStatuSinfo.stage = 0;
										stStatuSinfo.cyclet = 0;
										stStatuSinfo.phase = 0;
								
										stStatuSinfo.chans = 0;
            							memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            							csta = stStatuSinfo.csta;
            							for (i = 0; i < MAX_CHANNEL; i++)
            							{
                							if (0 == lkch[cp-0x5a][i])
                    							break;
                							stStatuSinfo.chans += 1;
                							tcsta = lkch[cp-0x5a][i];
                							tcsta <<= 2;
                							tcsta &= 0xfc;
                							tcsta |= 0x02; //00000010-green 
                							*csta = tcsta;
                							csta++;
            							}
										memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
										memset(sibuf,0,sizeof(sibuf));
            							if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            							{
            							#ifdef INTER_CONTROL_DEBUG
                							printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            							#endif
            							}
            							else
            							{
                							write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            							}
									}

									objecti[0].objectv[0] = 0xf4;
                                    factovs = 0;
                                    memset(cbuf,0,sizeof(cbuf));
                                    if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                    if (!(*(fcdata->markbit) & 0x1000))
                                    {
                                        write(*(fcdata->sockfd->csockfd),cbuf,factovs);
                                    }
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,89,ct.tv_sec,fcdata->markbit);
									continue;
								}
								else
								{//control phase is not current phase
									if ((0x01 <= cp) && (cp <= 0x20))
									{//if ((0x01 <= cp) && (cp <= 0x20))
										memset(cc,0,sizeof(cc));
										pcc = cc;
										memset(wcc,0,sizeof(wcc));
										memset(wnpcc,0,sizeof(wnpcc));
										memset(wpcc,0,sizeof(wpcc));
										z = 0;
										k = 0;
										s = 0;
										pce = 0;
										for (i = 0; i < (tscdata->channel->FactChannelNum); i++)
										{
											if (0 == (tscdata->channel->ChannelList[i].ChannelId))
												break;
											if (cp == (tscdata->channel->ChannelList[i].ChannelCtrlSrc))
											{
												#ifdef CLOSE_LAMP
                                                tclc = tscdata->channel->ChannelList[i].ChannelId;
                                                if ((tclc >= 0x05) && (tclc <= 0x0c))
                                                {
                                                    if (*(fcdata->specfunc) & (0x01 << (tclc - 5)))
                                                        continue;
                                                }
												#else
												if ((*(fcdata->specfunc)&0x10)&&(*(fcdata->specfunc)&0x20))
            									{
                									tclc = tscdata->channel->ChannelList[i].ChannelId;
                									if(((5<=tclc)&&(tclc<=8)) || ((9<=tclc)&&(tclc<=12)))
                    									continue;
            									}
												if ((*(fcdata->specfunc)&0x10)&&(!(*(fcdata->specfunc)&0x20)))
												{
													tclc = tscdata->channel->ChannelList[i].ChannelId;
													if ((5 <= tclc) && (tclc <= 8))
														continue;
												}
												if ((*(fcdata->specfunc)&0x20)&&(!(*(fcdata->specfunc)&0x10)))
												{
													tclc = tscdata->channel->ChannelList[i].ChannelId;
                									if ((9 <= tclc) && (tclc <= 12))
                    									continue;
												}
                                                #endif
												*pcc = tscdata->channel->ChannelList[i].ChannelId;
												pcc++;
												ce = 0;
												if ((0x5a <= tcbuf[1]) && (tcbuf[1] <= 0x5d))
                                                {//if ((0x5a <= tcbuf[1]) && (tcbuf[1] <= 0x5d))
                                                    for (j = 0; j < MAX_CHANNEL; j++)
                                                    {
                                                        if (0 == lkch[tcbuf[1]-0x5a][j])
                                                            break;
                                                        if (lkch[tcbuf[1]-0x5a][j] \
                                                            == tscdata->channel->ChannelList[i].ChannelId)
                                                        {
                                                            ce = 1;
                                                            break;
                                                        }
                                                    }
                                                }//if ((0x5a <= tcbuf[1]) && (tcbuf[1] <= 0x5d))
                                                else if (0xC8 == tcbuf[1])
                                                {//else if (0xC8 == tcbuf[1])
                                                    tclc = tscdata->channel->ChannelList[i].ChannelId;
                                                    if (*(fcdata->ccontrol) & (0x00000001 << (tclc - 1)))
                                                        ce = 1;
                                                }//else if (0xC8 == tcbuf[1])
												if (0 == ce)
												{
													wcc[z] = tscdata->channel->ChannelList[i].ChannelId;
													z++;
													if (3 == tscdata->channel->ChannelList[i].ChannelType)
													{
														wpcc[k] = tscdata->channel->ChannelList[i].ChannelId;
														k++;
														pce = 1;
														continue;
													}
													else
													{
														wnpcc[s] = tscdata->channel->ChannelList[i].ChannelId;
														s++;
														continue;
													}		
												}
											}
										}
									}//if ((0x01 <= cp) && (cp <= 0x20))
									else if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
									{//else if ((0x5a <= cp) && (cp <= 0x5d))
										memset(cc,0,sizeof(cc));
                                        pcc = cc;
										if ((0x5a <= cp) && (cp <= 0x5d))
										{
											for (i = 0; i < MAX_CHANNEL; i++)
											{
												if (0 == lkch[cp-0x5a][i])
													break;
												*pcc = lkch[cp-0x5a][i];
												pcc++;
											}
										}
										else if (0xC8 == cp)
										{
											for (i = 0; i < MAX_CHANNEL; i++)
                                            {
                                                if (*(fcdata->fcontrol) & (0x00000001 << i))
                                                {
                                                    #ifdef CLOSE_LAMP
                                                    if (((i+1) >= 5) && ((i+1) <= 12))
                                                    {
                                                        if (*(fcdata->specfunc) & (0x01 << ((i+1) - 5)))
                                                            continue;
                                                    }
                                                    #else
                                                    if((*(fcdata->specfunc)&0x10)&&(*(fcdata->specfunc)&0x20))
                                                    {
                                                        tclc = i + 1;
                                                        if(((5<=tclc)&&(tclc<=8)) || ((9<=tclc)&&(tclc<=12)))
                                                            continue;
                                                    }
                                                  if((*(fcdata->specfunc)&0x10)&&(!(*(fcdata->specfunc)&0x20)))
                                                    {
                                                        tclc = i + 1;
                                                        if ((5 <= tclc) && (tclc <= 8))
                                                            continue;
                                                    }
                                                  if((*(fcdata->specfunc)&0x20)&&(!(*(fcdata->specfunc)&0x10)))
                                                    {
                                                        tclc = i + 1;
                                                        if ((9 <= tclc) && (tclc <= 12))
                                                            continue;
                                                    }
                                                    #endif
                                                    *pcc = i + 1;
													pcc++;
                                                }
                                            }
										}//else if (0xC8 == cp)
                                        memset(wcc,0,sizeof(wcc));
                                        memset(wnpcc,0,sizeof(wnpcc));
                                        memset(wpcc,0,sizeof(wpcc));
                                        z = 0;
                                        k = 0;
                                        s = 0;
                                        pce = 0;
                                        for (i = 0; i < MAX_CHANNEL; i++)
                                        {//for (i = 0; i < MAX_CHANNEL_LINE; i++)
                                            if (0 == cc[i])
                                                break;
                                            ce = 0;
											if ((0x5a <= tcbuf[1]) && (tcbuf[1] <= 0x5d))
                                            {//if ((0x5a <= tcbuf[1]) && (tcbuf[1] <= 0x5d))
                                                for (j = 0; j < MAX_CHANNEL; j++)
                                                {
                                                    if (0 == lkch[tcbuf[1]-0x5a][j])
                                                        break;
                                                    if (lkch[tcbuf[1]-0x5a][j] == cc[i])
                                                    {
                                                        ce = 1;
                                                        break;
                                                    }
                                                }
                                            }//if ((0x5a <= tcbuf[1]) && (tcbuf[1] <= 0x5d))
                                            else if (0xC8 == tcbuf[1])
                                            {//else if (0xC8 == tcbuf[1])
                                                if (*(fcdata->ccontrol) & (0x00000001 << (cc[i]-1)))
                                                    ce = 1;
                                            }//else if (0xC8 == tcbuf[1])
                                            if (0 == ce)
                                            {
                                                wcc[z] = cc[i];
                                                z++;
                                                if ((0x0d <= cc[i]) && (cc[i] <= 0x10))
                                                {
                                                    wpcc[k] = cc[i];
                                                    k++;
                                                    pce =1;
                                                    continue;
                                                }
                                                else
                                                {
                                                    wnpcc[s] = cc[i];
                                                    s++;
                                                    continue;
                                                }
                                            }
										}
									}//else if ((0x5a <= cp) && (cp <= 0x5d))
									if (0xC8 == tcbuf[1])
                                        *(fcdata->trans) |= 0x01;

									if ((0==wcc[0])&&((stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0)))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
                                                stStatuSinfo.conmode = /*29*/ncmode;
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
                                                stStatuSinfo.conmode = cp;
											else if (0xC8 == cp)
												stStatuSinfo.conmode = 29;
											stStatuSinfo.color = 0x02;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime + stInterGrateControlPhaseInfo.ytime + stInterGrateControlPhaseInfo.rtime;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                stStatuSinfo.phase = 0;
                                                stStatuSinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
                                            {
                                                stStatuSinfo.phase = 0;
                                            }
	
											stStatuSinfo.chans = 0;
                                        	memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
                                        	csta = stStatuSinfo.csta;
                                        	for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == cc[i])
                                                	break;
                                            	stStatuSinfo.chans += 1;
                                            	tcsta = cc[i];
                                            	tcsta <<= 2;
                                            	tcsta &= 0xfc;
                                            	tcsta |= 0x02;
                                            	*csta = tcsta;
                                            	csta++;
                                        	}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}
									if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.gftime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
                                                stStatuSinfo.conmode = /*29*/ncmode;
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
                                                stStatuSinfo.conmode = cp;
											else if (0xC8 == cp)
                                                stStatuSinfo.conmode = 29;
											stStatuSinfo.color = 0x03;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                stStatuSinfo.phase = 0;
                                                stStatuSinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
                                            {
                                                stStatuSinfo.phase = 0;
                                            }
	
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == wcc[i])
                                                	break;
                                            	for (j = 0; j < stStatuSinfo.chans; j++)
                                            	{
                                                	if (0 == stStatuSinfo.csta[j])
                                                    	break;
                                                	tcsta = stStatuSinfo.csta[j];
                                                	tcsta &= 0xfc;
                                                	tcsta >>= 2;
                                                	tcsta &= 0x3f;
                                                	if (wcc[i] == tcsta)
                                                	{
                                                    	stStatuSinfo.csta[j] &= 0xfc;
                                                    	stStatuSinfo.csta[j] |= 0x03; //00000001-green flash
														break;
                                                	}
                                            	}
                                        	}								
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}
									//green flash
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&dctd,0,sizeof(dctd));
										dctd.mode = 29;//traffic control
										dctd.pattern = *(fcdata->patternid);
										dctd.lampcolor = 0x02;
										dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
										dctd.phaseid = 0;
										dctd.stageline = stInterGrateControlPhaseInfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 29;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x02;
                                        dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
                                        dctd.phaseid = 0;
                                        dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = stInterGrateControlPhaseInfo.gftime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef INTER_CONTROL_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef INTER_CONTROL_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									if (stInterGrateControlPhaseInfo.gftime > 0)
									{
										ngf = 0;
										while (1)
										{
											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x03,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);
									
											ngf += 1;
											if (ngf >= stInterGrateControlPhaseInfo.gftime)
												break;
										}
									}//if (stInterGrateControlPhaseInfo.gftime > 0)
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef INTER_CONTROL_DEBUG
            								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}

									if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.ytime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
                                                stStatuSinfo.conmode = /*29*/ncmode;
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
                                                stStatuSinfo.conmode = cp;
											else if (0xC8 == cp)
                                                stStatuSinfo.conmode = 29;
											stStatuSinfo.color = 0x01;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.ytime;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                stStatuSinfo.phase = 0;
                                                stStatuSinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
                                            {
                                                stStatuSinfo.phase = 0;
                                            }	
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == wnpcc[i])
                    								break;
                								for (j = 0; j < stStatuSinfo.chans; j++)
                								{
                    								if (0 == stStatuSinfo.csta[j])
                        								break;
                    								tcsta = stStatuSinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (wnpcc[i] == tcsta)
                    								{
                        								stStatuSinfo.csta[j] &= 0xfc;
														stStatuSinfo.csta[j] |= 0x01; //00000001-yellow
														break;
                    								}
                								}
            								}
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == wpcc[i])
                                                	break;
                                            	for (j = 0; j < stStatuSinfo.chans; j++)
                                            	{
                                                	if (0 == stStatuSinfo.csta[j])
                                                    	break;
                                                	tcsta = stStatuSinfo.csta[j];
                                                	tcsta &= 0xfc;
                                                	tcsta >>= 2;
                                                	tcsta &= 0x3f;
                                                	if (wpcc[i] == tcsta)
                                                	{
                                                    	stStatuSinfo.csta[j] &= 0xfc;
														break;
                                                	}
                                            	}
                                        	}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}

									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            		{
                            			memset(&dctd,0,sizeof(dctd));
                            			dctd.mode = 29;//traffic control
                            			dctd.pattern = *(fcdata->patternid);
                            			dctd.lampcolor = 0x01;
                            			dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                            			dctd.phaseid = 0;
                            			dctd.stageline = stInterGrateControlPhaseInfo.stageline;
                            			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
                            			{
                            			#ifdef INTER_CONTROL_DEBUG
                               				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            			#endif
                            			}
                            		}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
                            			memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 29;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x01;
                                        dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                                        dctd.phaseid = 0;
                                        dctd.stageline = stInterGrateControlPhaseInfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            		fbdata[3] = 0x01;
                            		fbdata[4] = stInterGrateControlPhaseInfo.ytime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                            			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                            			{
                            			#ifdef INTER_CONTROL_DEBUG
                               				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                			{
                                			#ifdef INTER_CONTROL_DEBUG
                                   				printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(stInterGrateControlPhaseInfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                                		printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
									if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.rtime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
                                                stStatuSinfo.conmode = /*29*/ncmode;
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
                                                stStatuSinfo.conmode = cp;
											else if (0xC8 == cp)
                                                stStatuSinfo.conmode = 29;
											stStatuSinfo.color = 0x00;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.rtime;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                stStatuSinfo.phase = 0;
                                                stStatuSinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
                                            {
                                                stStatuSinfo.phase = 0;
                                            }
	
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == wcc[i])
                                                	break;
                                            	for (j = 0; j < stStatuSinfo.chans; j++)
                                            	{
                                                	if (0 == stStatuSinfo.csta[j])
                                                    	break;
                                                	tcsta = stStatuSinfo.csta[j];
                                                	tcsta &= 0xfc;
                                                	tcsta >>= 2;
                                                	tcsta &= 0x3f;
                                                	if (wcc[i] == tcsta)
                                                	{
                                                    	stStatuSinfo.csta[j] &= 0xfc;
														break;
                                                	}
                                            	}
                                        	}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}

									if (stInterGrateControlPhaseInfo.rtime > 0)
									{
										if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            			{
                                			memset(&dctd,0,sizeof(dctd));
                                			dctd.mode = 29;//network control
                                			dctd.pattern = *(fcdata->patternid);
                                			dctd.lampcolor = 0x00;
                                			dctd.lamptime = stInterGrateControlPhaseInfo.rtime;
                                			dctd.phaseid = 0;
                                			dctd.stageline = stInterGrateControlPhaseInfo.stageline;
                                			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
                                			{
                                			#ifdef INTER_CONTROL_DEBUG
                                   				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                            			}
										#ifdef I_EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&dctd,0,sizeof(dctd));
                                            dctd.mode = 29;//network control
                                            dctd.pattern = *(fcdata->patternid);
                                            dctd.lampcolor = 0x00;
                                            dctd.lamptime = stInterGrateControlPhaseInfo.rtime;
                                            dctd.phaseid = 0;
                                            dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#endif
										fbdata[1] = 29;
                                		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                                		fbdata[3] = 0x00;
                                		fbdata[4] = stInterGrateControlPhaseInfo.rtime;
                                		if (!wait_write_serial(*(fcdata->fbserial)))
                                		{
                                   			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                   			{
                                   			#ifdef INTER_CONTROL_DEBUG
                                       			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   			#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
                                       			update_event_list(fcdata->ec,fcdata->el,1,16, \
																		ct.tv_sec,fcdata->markbit);
                                       			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                       			{
                                       			#ifdef INTER_CONTROL_DEBUG
                                           			printf("gen err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                       			#endif
                                       			}
                                   			}
                                		}
                                		else
                                		{
                                		#ifdef INTER_CONTROL_DEBUG
                                   			printf("face board port cannot write,Line:%d\n",__LINE__);
                                		#endif
                                		}
										sleep(stInterGrateControlPhaseInfo.rtime);
									}
								
									if ((0x5a <= tcbuf[1]) && (tcbuf[1] <= 0x5d))
                                    {	
										if (SUCCESS != \
											igc_set_lamp_color(*(fcdata->bbserial),lkch[tcbuf[1]-0x5a],0x02))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
							
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																lkch[tcbuf[1]-0x5a],0x02,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										ncmode = *(fcdata->contmode);
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = tcbuf[1];
											stStatuSinfo.color = 0x02;
											stStatuSinfo.time = 0;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;

											stStatuSinfo.chans = 0;
											memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
											csta = stStatuSinfo.csta;
											for (i = 0; i < MAX_CHANNEL; i++)
											{
												if (0 == lkch[tcbuf[1]-0x5a][i])
													break;
												stStatuSinfo.chans += 1;
												tcsta = lkch[tcbuf[1]-0x5a][i];
												tcsta <<= 2;
												tcsta &= 0xfc;
												tcsta |= 0x02;
												*csta = tcsta;
												csta++;
											}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));					
											memset(sibuf,0,sizeof(sibuf));
											if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("status info pack err,File:%s,Line:%d\n", \
																			__FILE__,__LINE__);
											#endif
											}
											else
											{
												write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
											}
										}//report info to server actively
									}
									if (0xC8 == tcbuf[1])
									{
										memset(ccon,0,sizeof(ccon));
										j = 0;
										for (i = 0; i < MAX_CHANNEL; i++)
										{
											if (*(fcdata->ccontrol) & (0x00000001 << i))
											{
												#ifdef CLOSE_LAMP
												if (((i+1) >= 5) && ((i+1) <= 12))
												{
													if (*(fcdata->specfunc) & (0x01 << ((i+1) - 5)))
														continue;
												}
												#else
												if ((*(fcdata->specfunc)&0x10)&&(*(fcdata->specfunc)&0x20))
                                                {
                                                    tclc = i + 1; 
                                                    if(((5<=tclc)&&(tclc<=8)) || ((9<=tclc)&&(tclc<=12)))
                                                        continue;
                                                }
                                                if ((*(fcdata->specfunc)&0x10)&&(!(*(fcdata->specfunc)&0x20)))
                                                {
                                                    tclc = i + 1;
                                                    if ((5 <= tclc) && (tclc <= 8))
                                                        continue;
                                                }
                                                if ((*(fcdata->specfunc)&0x20)&&(!(*(fcdata->specfunc)&0x10)))
                                                {
                                                    tclc = i + 1;
                                                    if ((9 <= tclc) && (tclc <= 12))
                                                        continue;
                                                }
												#endif
												ccon[j] = i + 1;
												j++;
											}
										}
										*(fcdata->fcontrol) = *(fcdata->ccontrol);
										if(SUCCESS!=igc_set_lamp_color(*(fcdata->bbserial),ccon,0x02))
                                        {
                                        #ifdef INTER_CONTROL_DEBUG
                                            printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                            *(fcdata->markbit) |= 0x0800;
                                        }
                                        if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                                                    		ccon,0x02,fcdata->markbit))
                                        {
                                        #ifdef INTER_CONTROL_DEBUG
                                            printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                        #endif
                                        }
										ncmode = *(fcdata->contmode);
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = ncmode;
											stStatuSinfo.color = 0x02;
											stStatuSinfo.time = 0;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
											stStatuSinfo.chans = 0;
											memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
											csta = stStatuSinfo.csta;
											for (i = 0; i < MAX_CHANNEL; i++)
											{
												if (0 == ccon[i])
													break;
												stStatuSinfo.chans += 1;
												tcsta = ccon[i];
												tcsta <<= 2;
												tcsta &= 0xfc;
												tcsta |= 0x02; //00000010-green 
												*csta = tcsta;
												csta++;
											}

											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
											memset(sibuf,0,sizeof(sibuf));
											if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("status info pack err,File:%s,Line:%d\n", \
														__FILE__,__LINE__);
											#endif
											}
											else
											{
												write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
											}
										}//report info to server actively
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,127, \
															ct.tv_sec,fcdata->markbit);
									}//if (0xC8 == tcbuf[1])
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&dctd,0,sizeof(dctd));
										dctd.mode = 29;//traffic control
										dctd.pattern = *(fcdata->patternid);
										dctd.lampcolor = 0x02;
										dctd.lamptime = 0;
										dctd.phaseid = 0;
										dctd.stageline = stInterGrateControlPhaseInfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 29;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x02;
                                        dctd.lamptime = 0;
                                        dctd.phaseid = 0;
                                        dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = 0;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef INTER_CONTROL_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef INTER_CONTROL_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									if (0xC8 == tcbuf[1])
                                        *(fcdata->trans) = 0;
									cp = tcbuf[1];

									objecti[0].objectv[0] = 0xf4;
									factovs = 0;
									memset(cbuf,0,sizeof(cbuf));
									if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                            			printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
                            		}
									if (!(*(fcdata->markbit) & 0x1000))
									{
                            			write(*(fcdata->sockfd->csockfd),cbuf,factovs);
									}
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,89,ct.tv_sec,fcdata->markbit);							
									continue;	
								}//control phase is not current phase
							}//if (((0x5a <= tcbuf[1]) && (tcbuf[1] <= 0x5d)) || (0xC8 == tcbuf[1]))
							else if ((0x01 <= tcbuf[1]) && (tcbuf[1] <= 0x20))
							{//control someone phase
								//check phaseid whether exist in phase list;
								pex = 0;
								for (i = 0; i < (tscdata->phase->FactPhaseNum); i++)
								{
									if (0 == (tscdata->phase->PhaseList[i].PhaseId))
										break;
									if (tcbuf[1] == (tscdata->phase->PhaseList[i].PhaseId))
									{
										pex = 1;
										break;
									}	
								}
								if (0 == pex)
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("Not fit phase id,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									objecti[0].objectv[0] = 0xf4;
                                    factovs = 0;
                                    memset(cbuf,0,sizeof(cbuf));
                                    if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x00))
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                    if (!(*(fcdata->markbit) & 0x1000))
                                    {
                                        write(*(fcdata->sockfd->csockfd),cbuf,factovs);
                                    }
									continue;
								}
								//end check phaseid whether exist in phase list;

								if ((1 == dcred) || (1 == YellowFlashYes) || (1 == close) || (1 == phcon))
                            	{//if ((1 == tcred) || (1 == tcyfyes) || (1 == close))
									fpc = 1;
									phcon = 0;
									cp = tcbuf[1];
                                	if (1 == YellowFlashYes)
                                	{
                                    	pthread_cancel(YellowFlashPid);
                                    	pthread_join(YellowFlashPid,NULL);
                                    	YellowFlashYes = 0;
                                	}
									if (1 != dcred)
									{
										new_all_red(&ardata);
									}
									dcred = 0;
									close = 0;
					//				#ifdef CLOSE_LAMP
                                    igc_set_lamp_color(*(fcdata->bbserial),clch,0x03);
									update_channel_status(fcdata->sockfd,chanstatus,clch,0x03,fcdata->markbit);
                   //                 #endif
									memset(nc,0,sizeof(nc));
                                    pcc = nc;
                                    for (i = 0; i < (tscdata->channel->FactChannelNum); i++)
                                    {
                                        if (0 == (tscdata->channel->ChannelList[i].ChannelId))
                                            break;
                                        if (tcbuf[1] == (tscdata->channel->ChannelList[i].ChannelCtrlSrc))
                                        {
											#ifdef CLOSE_LAMP
											tclc = tscdata->channel->ChannelList[i].ChannelId;
											if ((tclc >= 0x05) && (tclc <= 0x0c))
											{
												if (*(fcdata->specfunc) & (0x01 << (tclc - 5)))
													continue;
											}
											#else
											if ((*(fcdata->specfunc)&0x10)&&(*(fcdata->specfunc)&0x20))
											{
												tclc = tscdata->channel->ChannelList[i].ChannelId;
												if(((5<=tclc)&&(tclc<=8)) || ((9<=tclc)&&(tclc<=12)))
													continue;
											}
											if ((*(fcdata->specfunc)&0x10)&&(!(*(fcdata->specfunc)&0x20)))
											{
												tclc = tscdata->channel->ChannelList[i].ChannelId;
												if ((5 <= tclc) && (tclc <= 8))
													continue;
											}
											if ((*(fcdata->specfunc)&0x20)&&(!(*(fcdata->specfunc)&0x10)))
											{
												tclc = tscdata->channel->ChannelList[i].ChannelId;
												if ((9 <= tclc) && (tclc <= 12))
													continue;
											}
											#endif
                                            *pcc = tscdata->channel->ChannelList[i].ChannelId;
                                            pcc++;
                                        }
                                    }
                                	if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),nc,0x02))
                                	{
                                	#ifdef INTER_CONTROL_DEBUG
                                    	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
                                    	*(fcdata->markbit) |= 0x0800;
                                	}
                                	if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                                        				nc,0x02,fcdata->markbit))
                                	{
                                	#ifdef INTER_CONTROL_DEBUG
                                    	printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                	#endif
                                	}
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										stStatuSinfo.conmode = 29;
										stStatuSinfo.color = 0x02;
										stStatuSinfo.time = 0;
										stStatuSinfo.stage = 0;
										stStatuSinfo.cyclet = 0;
										stStatuSinfo.phase = 0;
                                        stStatuSinfo.phase |= (0x01 << (cp - 1));														
									/*	
										stStatuSinfo.chans = 0;
            							memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            							csta = stStatuSinfo.csta;
            							for (i = 0; i < MAX_CHANNEL; i++)
            							{
                							stStatuSinfo.chans += 1;
                							tcsta = i + 1;
                							tcsta <<= 2;
                							tcsta &= 0xfc;
											for (j = 0; j < MAX_CHANNEL; j++)
											{
												if (0 == nc[j])
													break;
												if ((i+1) == nc[j])
												{
													tcsta |= 0x02;
													break;
												}
											}
                							*csta = tcsta;
                							csta++;
            							}
								*/
										stStatuSinfo.chans = 0;
										memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
                                        csta = stStatuSinfo.csta;
                                        for (i = 0; i < MAX_CHANNEL; i++)
                                        {
                                            if (0 == nc[i])
                                                break;
                                            stStatuSinfo.chans += 1;
                                            tcsta = nc[i];
                                            tcsta <<= 2;
                                            tcsta &= 0xfc;
                                            tcsta |= 0x02; //00000010-green 
                                            *csta = tcsta;
                                            csta++;
                                        }
										memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            							memset(sibuf,0,sizeof(sibuf));
            							if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            							{
            							#ifdef INTER_CONTROL_DEBUG
                							printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            							#endif
            							}
            							else
            							{
                							write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            							}
									}//report info to server actively
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,88,ct.tv_sec,fcdata->markbit);
                                	continue;
                            	}//if ((1 == tcred) || (1 == tcyfyes) || (1 == close))
								if (0 == fpc)
								{//phase control of first times
									fpc = 1;
									cp = stInterGrateControlPhaseInfo.phaseid;
								}//phase control of first times

								if (cp == tcbuf[1])
								{
									memset(cc,0,sizeof(cc));
									pcc = cc;
									for (i = 0; i < (tscdata->channel->FactChannelNum); i++)
									{
										if (0 == (tscdata->channel->ChannelList[i].ChannelId))
											break;
										if (cp == (tscdata->channel->ChannelList[i].ChannelCtrlSrc))
										{
											#ifdef CLOSE_LAMP
											tclc = tscdata->channel->ChannelList[i].ChannelId;
											if ((tclc >= 0x05) && (tclc <= 0x0c))
											{
												if (*(fcdata->specfunc) & (0x01 << (tclc - 5)))
													continue;
											}
											#else
											if ((*(fcdata->specfunc)&0x10)&&(*(fcdata->specfunc)&0x20))
											{
												tclc = tscdata->channel->ChannelList[i].ChannelId;
												if(((5<=tclc)&&(tclc<=8)) || ((9<=tclc)&&(tclc<=12)))
													continue;
											}
											if ((*(fcdata->specfunc)&0x10)&&(!(*(fcdata->specfunc)&0x20)))
											{
												tclc = tscdata->channel->ChannelList[i].ChannelId;
												if ((5 <= tclc) && (tclc <= 8))
													continue;
											}
											if ((*(fcdata->specfunc)&0x20)&&(!(*(fcdata->specfunc)&0x10)))
											{
												tclc = tscdata->channel->ChannelList[i].ChannelId;
												if ((9 <= tclc) && (tclc <= 12))
													continue;
											}
											#endif
											*pcc = tscdata->channel->ChannelList[i].ChannelId;
											pcc++;
										}
									}
									if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
									{
										stStatuSinfo.conmode = 29;
										stStatuSinfo.color = 0x02;
										stStatuSinfo.time = 0;
										stStatuSinfo.stage = 0;
										stStatuSinfo.cyclet = 0;
										stStatuSinfo.phase = 0;
										stStatuSinfo.phase |= (0x01 << (cp - 1));
								
										stStatuSinfo.chans = 0;
            							memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            							csta = stStatuSinfo.csta;
            							for (i = 0; i < MAX_CHANNEL; i++)
            							{
                							if (0 == cc[i])
                    							break;
                							stStatuSinfo.chans += 1;
                							tcsta = cc[i];
                							tcsta <<= 2;
                							tcsta &= 0xfc;
                							tcsta |= 0x02; //00000010-green 
                							*csta = tcsta;
                							csta++;
            							}
										memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
										memset(sibuf,0,sizeof(sibuf));
            							if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            							{
            							#ifdef INTER_CONTROL_DEBUG
                							printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            							#endif
            							}
            							else
            							{
                							write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            							}
									}

									objecti[0].objectv[0] = 0xf4;
                                    factovs = 0;
                                    memset(cbuf,0,sizeof(cbuf));
                                    if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                    if (!(*(fcdata->markbit) & 0x1000))
                                    {
                                        write(*(fcdata->sockfd->csockfd),cbuf,factovs);
                                    }
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,88,ct.tv_sec,fcdata->markbit);
									continue;
								}
								else
								{//control phase is not current phase
									memset(nc,0,sizeof(nc));
                                    pcc = nc;
                                    for (i = 0; i < (tscdata->channel->FactChannelNum); i++)
                                    {
                                        if (0 == (tscdata->channel->ChannelList[i].ChannelId))
                                            break;
                                        if (tcbuf[1] == (tscdata->channel->ChannelList[i].ChannelCtrlSrc))
                                        {
											#ifdef CLOSE_LAMP
											tclc = tscdata->channel->ChannelList[i].ChannelId;
											if ((tclc >= 0x05) && (tclc <= 0x0c))
											{
												if (*(fcdata->specfunc) & (0x01 << (tclc - 5)))
													continue;
											}
											#else
											if ((*(fcdata->specfunc)&0x10)&&(*(fcdata->specfunc)&0x20))
											{
												tclc = tscdata->channel->ChannelList[i].ChannelId;
												if(((5<=tclc)&&(tclc<=8)) || ((9<=tclc)&&(tclc<=12)))
													continue;
											}
											if ((*(fcdata->specfunc)&0x10)&&(!(*(fcdata->specfunc)&0x20)))
											{
												tclc = tscdata->channel->ChannelList[i].ChannelId;
												if ((5 <= tclc) && (tclc <= 8))
													continue;
											}
											if ((*(fcdata->specfunc)&0x20)&&(!(*(fcdata->specfunc)&0x10)))
											{
												tclc = tscdata->channel->ChannelList[i].ChannelId;
												if ((9 <= tclc) && (tclc <= 12))
													continue;
											}
											#endif
                                            *pcc = tscdata->channel->ChannelList[i].ChannelId;
                                            pcc++;
                                        }
                                    }
									if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
									{//if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
										memset(cc,0,sizeof(cc));
                                        pcc = cc;
										if ((0x5a <= cp) && (cp <= 0x5d))
                                        {
											for (i = 0; i < MAX_CHANNEL; i++)
											{
												if (0 == lkch[cp-0x5a][i])
													break;
												*pcc = lkch[cp-0x5a][i];
												pcc++;
											}
										}
										else if (0xC8 == cp)
										{//else if (0xC8 == cp)
											for (i = 0; i < MAX_CHANNEL; i++)
                                            {
                                                if (*(fcdata->ccontrol) & (0x00000001 << i))
                                                {
                                                    #ifdef CLOSE_LAMP
                                                    if (((i+1) >= 5) && ((i+1) <= 12))
                                                    {
                                                        if (*(fcdata->specfunc) & (0x01 << ((i+1) - 5)))
                                                            continue;
                                                    }
                                                    #else
                                                    if((*(fcdata->specfunc)&0x10)&&(*(fcdata->specfunc)&0x20))
                                                    {
                                                        tclc = i + 1;
                                                        if(((5<=tclc)&&(tclc<=8)) || ((9<=tclc)&&(tclc<=12)))
                                                            continue;
                                                    }
                                                  if((*(fcdata->specfunc)&0x10)&&(!(*(fcdata->specfunc)&0x20)))
                                                    {
                                                        tclc = i + 1;
                                                        if ((5 <= tclc) && (tclc <= 8))
                                                            continue;
                                                    }
                                                  if((*(fcdata->specfunc)&0x20)&&(!(*(fcdata->specfunc)&0x10)))
                                                    {
                                                        tclc = i + 1;
                                                        if ((9 <= tclc) && (tclc <= 12))
                                                            continue;
                                                    }
                                                    #endif
                                                    *pcc = i + 1;
													pcc++;
                                                }
                                            }
										}//else if (0xC8 == cp)
                                        memset(wcc,0,sizeof(wcc));
                                        memset(wnpcc,0,sizeof(wnpcc));
                                        memset(wpcc,0,sizeof(wpcc));
                                        z = 0;
                                        k = 0;
                                        s = 0;
                                        pce = 0;
                                        for (i = 0; i < MAX_CHANNEL; i++)
                                        {
                                            if (0 == cc[i])
                                                break;
                                            ce = 0;
											for (j = 0; j < MAX_CHANNEL; j++)
                                            {
                                                if (0 == nc[j])
                                                    break;
                                                if (nc[j] == cc[i])
                                                {
                                                    ce = 1;
                                                    break;
                                                }
                                            }
                                            if (0 == ce)
                                            {
                                                wcc[z] = cc[i];
                                                z++;
                                                if ((0x0d <= cc[i]) && (cc[i] <= 0x10))
                                                {
                                                    wpcc[k] = cc[i];
                                                    k++;
                                                    pce = 1;
                                                    continue;
                                                }
                                                else
                                                {
                                                    wnpcc[s] = cc[i];
                                                    s++;
                                                    continue;
                                                }
                                            }
										}
									}//if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
									else if ((0x01 <= cp) && (cp <= 0x20))
									{//else if ((0x01 <= cp) && (cp <= 0x20))
										memset(cc,0,sizeof(cc));
										pcc = cc;
										memset(wcc,0,sizeof(wcc));
										memset(wnpcc,0,sizeof(wnpcc));
										memset(wpcc,0,sizeof(wpcc));
										z = 0;
										k = 0;
										s = 0;
										pce = 0;
										for (i = 0; i < (tscdata->channel->FactChannelNum); i++)
										{
											if (0 == (tscdata->channel->ChannelList[i].ChannelId))
												break;
											if (cp == (tscdata->channel->ChannelList[i].ChannelCtrlSrc))
											{
												#ifdef CLOSE_LAMP
                                                tclc = tscdata->channel->ChannelList[i].ChannelId;
                                                if ((tclc >= 0x05) && (tclc <= 0x0c))
                                                {
                                                    if (*(fcdata->specfunc) & (0x01 << (tclc - 5)))
                                                        continue;
                                                }
												#else
												if ((*(fcdata->specfunc)&0x10)&&(*(fcdata->specfunc)&0x20))
            									{
                									tclc = tscdata->channel->ChannelList[i].ChannelId;
                									if(((5<=tclc)&&(tclc<=8)) || ((9<=tclc)&&(tclc<=12)))
                    									continue;
            									}
												if ((*(fcdata->specfunc)&0x10)&&(!(*(fcdata->specfunc)&0x20)))
												{
													tclc = tscdata->channel->ChannelList[i].ChannelId;
													if ((5 <= tclc) && (tclc <= 8))
														continue;
												}
												if ((*(fcdata->specfunc)&0x20)&&(!(*(fcdata->specfunc)&0x10)))
												{
													tclc = tscdata->channel->ChannelList[i].ChannelId;
                									if ((9 <= tclc) && (tclc <= 12))
                    									continue;
												}	
                                                #endif
												*pcc = tscdata->channel->ChannelList[i].ChannelId;
												pcc++;
												ce = 0;
												for (j = 0; j < MAX_CHANNEL; j++)
												{
													if (0 == nc[j])
														break;
													if (nc[j] == tscdata->channel->ChannelList[i].ChannelId)
													{
														ce = 1;
														break;
													}
												}
												if (0 == ce)
												{
													wcc[z] = tscdata->channel->ChannelList[i].ChannelId;
													z++;
													if (3 == tscdata->channel->ChannelList[i].ChannelType)
													{
														wpcc[k] = tscdata->channel->ChannelList[i].ChannelId;
														k++;
														pce = 1;
														continue;
													}
													else
													{
														wnpcc[s] = tscdata->channel->ChannelList[i].ChannelId;
														s++;
														continue;
													}		
												}
											}
										}
									}//else if ((0x01 <= cp) && (cp <= 0x20))

									if ((0==wcc[0])&&((stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0)))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
                                                stStatuSinfo.conmode = /*29*/ncmode;
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
                                                stStatuSinfo.conmode = cp;
											else if (0xC8 == cp)
                                                stStatuSinfo.conmode = 29;
											stStatuSinfo.color = 0x02;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime + stInterGrateControlPhaseInfo.ytime + stInterGrateControlPhaseInfo.rtime;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                stStatuSinfo.phase = 0;
                                                stStatuSinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
                                            {
                                                stStatuSinfo.phase = 0;
                                            }
	
											stStatuSinfo.chans = 0;
                                        	memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
                                        	csta = stStatuSinfo.csta;
                                        	for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == cc[i])
                                                	break;
                                            	stStatuSinfo.chans += 1;
                                            	tcsta = cc[i];
                                            	tcsta <<= 2;
                                            	tcsta &= 0xfc;
                                            	tcsta |= 0x02;
                                            	*csta = tcsta;
                                            	csta++;
                                        	}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}
									if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.gftime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
                                                stStatuSinfo.conmode = /*29*/ncmode;
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
                                                stStatuSinfo.conmode = cp;
											else if (0xC8 == cp)
                                                stStatuSinfo.conmode = 29;
											stStatuSinfo.color = 0x03;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                stStatuSinfo.phase = 0;
                                                stStatuSinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
                                            {
                                                stStatuSinfo.phase = 0;
                                            }
	
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == wcc[i])
                                                	break;
                                            	for (j = 0; j < stStatuSinfo.chans; j++)
                                            	{
                                                	if (0 == stStatuSinfo.csta[j])
                                                    	break;
                                                	tcsta = stStatuSinfo.csta[j];
                                                	tcsta &= 0xfc;
                                                	tcsta >>= 2;
                                                	tcsta &= 0x3f;
                                                	if (wcc[i] == tcsta)
                                                	{
                                                    	stStatuSinfo.csta[j] &= 0xfc;
                                                    	stStatuSinfo.csta[j] |= 0x03; //00000001-green flash
														break;
                                                	}
                                            	}
                                        	}								
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}
									//green flash
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&dctd,0,sizeof(dctd));
										dctd.mode = 29;//traffic control
										dctd.pattern = *(fcdata->patternid);
										dctd.lampcolor = 0x02;
										dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
										dctd.phaseid = cp;
										dctd.stageline = stInterGrateControlPhaseInfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 29;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x02;
                                        dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
                                        dctd.phaseid = cp;
                                        dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = stInterGrateControlPhaseInfo.gftime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef INTER_CONTROL_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef INTER_CONTROL_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									if (stInterGrateControlPhaseInfo.gftime > 0)
									{
										ngf = 0;
										while (1)
										{
											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x03,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);
									
											ngf += 1;
											if (ngf >= stInterGrateControlPhaseInfo.gftime)
												break;
										}
									}//if (stInterGrateControlPhaseInfo.gftime > 0)
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef INTER_CONTROL_DEBUG
            								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}

									if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.ytime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
                                                stStatuSinfo.conmode = /*29*/ncmode;
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
                                                stStatuSinfo.conmode = cp;
											else if (0xC8 == cp)
                                                stStatuSinfo.conmode = 29;
											stStatuSinfo.color = 0x01;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.ytime;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                stStatuSinfo.phase = 0;
                                                stStatuSinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
                                            {
                                                stStatuSinfo.phase = 0;
                                            }
	
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == wnpcc[i])
                    								break;
                								for (j = 0; j < stStatuSinfo.chans; j++)
                								{
                    								if (0 == stStatuSinfo.csta[j])
                        								break;
                    								tcsta = stStatuSinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (wnpcc[i] == tcsta)
                    								{
                        								stStatuSinfo.csta[j] &= 0xfc;
														stStatuSinfo.csta[j] |= 0x01; //00000001-yellow
														break;
                    								}
                								}
            								}
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == wpcc[i])
                                                	break;
                                            	for (j = 0; j < stStatuSinfo.chans; j++)
                                            	{
                                                	if (0 == stStatuSinfo.csta[j])
                                                    	break;
                                                	tcsta = stStatuSinfo.csta[j];
                                                	tcsta &= 0xfc;
                                                	tcsta >>= 2;
                                                	tcsta &= 0x3f;
                                                	if (wpcc[i] == tcsta)
                                                	{
                                                    	stStatuSinfo.csta[j] &= 0xfc;
														break;
                                                	}
                                            	}
                                        	}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}

									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            		{
                            			memset(&dctd,0,sizeof(dctd));
                            			dctd.mode = 29;//traffic control
                            			dctd.pattern = *(fcdata->patternid);
                            			dctd.lampcolor = 0x01;
                            			dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                            			dctd.phaseid = cp;
                            			dctd.stageline = stInterGrateControlPhaseInfo.stageline;
                            			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
                            			{
                            			#ifdef INTER_CONTROL_DEBUG
                               				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            			#endif
                            			}
                            		}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 29;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x01;
                                        dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                                        dctd.phaseid = cp;
                                        dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            		fbdata[3] = 0x01;
                            		fbdata[4] = stInterGrateControlPhaseInfo.ytime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                            			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                            			{
                            			#ifdef INTER_CONTROL_DEBUG
                               				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                			{
                                			#ifdef INTER_CONTROL_DEBUG
                                   				printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(stInterGrateControlPhaseInfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                                		printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
									if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.rtime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
                                                stStatuSinfo.conmode = /*29*/ncmode;
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
                                                stStatuSinfo.conmode = cp;
											else if (0xC8 == cp)
                                                stStatuSinfo.conmode = 29;
											stStatuSinfo.color = 0x00;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.rtime;
											stStatuSinfo.stage = 0;
											stStatuSinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                stStatuSinfo.phase = 0;
                                                stStatuSinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
                                            {
                                                stStatuSinfo.phase = 0;
                                            }	
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == wcc[i])
                                                	break;
                                            	for (j = 0; j < stStatuSinfo.chans; j++)
                                            	{
                                                	if (0 == stStatuSinfo.csta[j])
                                                    	break;
                                                	tcsta = stStatuSinfo.csta[j];
                                                	tcsta &= 0xfc;
                                                	tcsta >>= 2;
                                                	tcsta &= 0x3f;
                                                	if (wcc[i] == tcsta)
                                                	{
                                                    	stStatuSinfo.csta[j] &= 0xfc;
														break;
                                                	}
                                            	}
                                        	}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}

									if (stInterGrateControlPhaseInfo.rtime > 0)
									{
										if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            			{
                                			memset(&dctd,0,sizeof(dctd));
                                			dctd.mode = 29;//network control
                                			dctd.pattern = *(fcdata->patternid);
                                			dctd.lampcolor = 0x00;
                                			dctd.lamptime = stInterGrateControlPhaseInfo.rtime;
                                			dctd.phaseid = cp;
                                			dctd.stageline = stInterGrateControlPhaseInfo.stageline;
                                			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
                                			{
                                			#ifdef INTER_CONTROL_DEBUG
                                   				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                            			}
										#ifdef I_EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&dctd,0,sizeof(dctd));
                                            dctd.mode = 29;//network control
                                            dctd.pattern = *(fcdata->patternid);
                                            dctd.lampcolor = 0x00;
                                            dctd.lamptime = stInterGrateControlPhaseInfo.rtime;
                                            dctd.phaseid = cp;
                                            dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#endif
										fbdata[1] = 29;
                                		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                                		fbdata[3] = 0x00;
                                		fbdata[4] = stInterGrateControlPhaseInfo.rtime;
                                		if (!wait_write_serial(*(fcdata->fbserial)))
                                		{
                                   			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                   			{
                                   			#ifdef INTER_CONTROL_DEBUG
                                       			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   			#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
                                       			update_event_list(fcdata->ec,fcdata->el,1,16, \
																		ct.tv_sec,fcdata->markbit);
                                       			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                       			{
                                       			#ifdef INTER_CONTROL_DEBUG
                                           			printf("gen err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                       			#endif
                                       			}
                                   			}
                                		}
                                		else
                                		{
                                		#ifdef INTER_CONTROL_DEBUG
                                   			printf("face board port cannot write,Line:%d\n",__LINE__);
                                		#endif
                                		}
										sleep(stInterGrateControlPhaseInfo.rtime);
									}

									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),nc,0x02))
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			nc,0x02,fcdata->markbit))
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
                            		}
									ncmode = *(fcdata->contmode);
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										stStatuSinfo.conmode = /*29*/ncmode;
										stStatuSinfo.color = 0x02;
										stStatuSinfo.time = 0;
										stStatuSinfo.stage = 0;
										stStatuSinfo.cyclet = 0;
										stStatuSinfo.phase = 0;
										stStatuSinfo.phase |= (0x01 << (tcbuf[1] - 1));

										stStatuSinfo.chans = 0;
            							memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            							csta = stStatuSinfo.csta;
            							for (i = 0; i < MAX_CHANNEL; i++)
            							{
                							if (0 == nc[i])
                    							break;
                							stStatuSinfo.chans += 1;
                							tcsta = nc[i];
                							tcsta <<= 2;
                							tcsta &= 0xfc;
                							tcsta |= 0x02;
                							*csta = tcsta;
                							csta++;
            							}
										memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));					
            							memset(sibuf,0,sizeof(sibuf));
            							if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            							{
            							#ifdef INTER_CONTROL_DEBUG
                							printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            							#endif
            							}
            							else
            							{
                							write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            							}
									}//report info to server actively
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&dctd,0,sizeof(dctd));
										dctd.mode = 29;//traffic control
										dctd.pattern = *(fcdata->patternid);
										dctd.lampcolor = 0x02;
										dctd.lamptime = 0;
										dctd.phaseid = tcbuf[1];
										dctd.stageline = stInterGrateControlPhaseInfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 29;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x02;
                                        dctd.lamptime = 0;
                                        dctd.phaseid = tcbuf[1];
                                        dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = 0;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef INTER_CONTROL_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef INTER_CONTROL_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									cp = tcbuf[1];

									objecti[0].objectv[0] = 0xf4;
									factovs = 0;
									memset(cbuf,0,sizeof(cbuf));
									if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                            			printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
                            		}
									if (!(*(fcdata->markbit) & 0x1000))
									{
                            			write(*(fcdata->sockfd->csockfd),cbuf,factovs);
									}
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,88,ct.tv_sec,fcdata->markbit);							
									continue;	
								}//control phase is not current phase
							}//control someone phase
							else if(0x27 == tcbuf[1])
							{//net step by step
								fpc = 0;
								if(1 == bAllLampClose)
								{
									new_all_red(&ardata);
								}
								if ((1 == dcred) || (1 == close) || (1 == YellowFlashYes) || (1 == phcon))	
								{
									phcon = 0;
									dcred = 0;
									close = 0;
									if (1 == YellowFlashYes)
									{
										pthread_cancel(YellowFlashPid);
										pthread_join(YellowFlashPid,NULL);
										YellowFlashYes = 0;
									}
									if (1 != dcred)
									{
										new_all_red(&ardata);
									}
									dcred = 0;
									close = 0;
				//					#ifdef CLOSE_LAMP
                                    igc_set_lamp_color(*(fcdata->bbserial),clch,0x03);
									update_channel_status(fcdata->sockfd,chanstatus,clch,0x03,fcdata->markbit);
               //                     #endif
									if (0 == cp)
									{//not have phase control
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.chan,0x02))
                                		{
                                		#ifdef INTER_CONTROL_DEBUG
                                    		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
                                    		*(fcdata->markbit) |= 0x0800;
                                		}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    					stInterGrateControlPhaseInfo.chan,0x02,fcdata->markbit))
                                		{
                                		#ifdef INTER_CONTROL_DEBUG
                                    		printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               			#endif
                                		}

										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											stStatuSinfo.conmode = 29;
											stStatuSinfo.color = 0x02;
											stStatuSinfo.time = 0;
											stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
											stStatuSinfo.cyclet = 0;
											stStatuSinfo.phase = 0;
											stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
											/*				
											stStatuSinfo.chans = 0;
            								memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            								csta = stStatuSinfo.csta;
            								for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								stStatuSinfo.chans += 1;
                								tcsta = i + 1;
                								tcsta <<= 2;
                								tcsta &= 0xfc;
												for (j = 0; j < MAX_CHANNEL; j++)
												{
													if (0 == stInterGrateControlPhaseInfo.chan[j])
														break;
													if ((i+1) == stInterGrateControlPhaseInfo.chan[j])
													{
														tcsta |= 0x02;
														break;
													}
												}
                								*csta = tcsta;
                								csta++;
            								}*/
											stStatuSinfo.chans = 0;
											memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
                                            csta = stStatuSinfo.csta;
											for (i = 0; i < MAX_CHANNEL; i++)
											{
												if (0 == stInterGrateControlPhaseInfo.chan[i])
													break;
												stStatuSinfo.chans += 1;
												tcsta = stInterGrateControlPhaseInfo.chan[i];
												tcsta <<= 2;
												tcsta &= 0xfc;
												tcsta |= 0x02; //00000010-green 
												*csta = tcsta;
												csta++;
											}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//not have phase control
									else
									{//have phase control
										if ((0x5a <= cp) && (cp <= 0x5d))
										{//if ((0x5a <= cp) && (cp <= 0x5d))
											if (SUCCESS != \
												igc_set_lamp_color(*(fcdata->bbserial),lkch[cp-0x5a],0x02))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																		lkch[cp-0x5a],0x02,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("update err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											if((!(*(fcdata->markbit)&0x1000))&&(!(*(fcdata->markbit)&0x8000)))
											{//report info to server actively
												stStatuSinfo.conmode = cp;
												stStatuSinfo.color = 0x02;
												stStatuSinfo.time = 0;
												stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
												stStatuSinfo.cyclet = 0;
												stStatuSinfo.phase = 0;
											/*
												stStatuSinfo.chans = 0;
												memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
												csta = stStatuSinfo.csta;
												for (i = 0; i < MAX_CHANNEL; i++)
												{
													stStatuSinfo.chans += 1;
													tcsta = i + 1;
													tcsta <<= 2;
													tcsta &= 0xfc;
													for (j = 0; j < MAX_CHANNEL; j++)
													{
														if (0 == lkch[cp-0x5a][j])
															break;
														if ((i+1) == lkch[cp-0x5a][j])
														{
															tcsta |= 0x02;
															break;
														}
													}
													*csta = tcsta;
													csta++;
												}
												*/
												stStatuSinfo.chans = 0;
												memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
                                                csta = stStatuSinfo.csta;
												for (i = 0; i < MAX_CHANNEL; i++)
												{
													if (0 == lkch[cp-0x5a][i])
														break;
													stStatuSinfo.chans += 1;
													tcsta = lkch[cp-0x5a][i];
													tcsta <<= 2;
													tcsta &= 0xfc;
													tcsta |= 0x02; //00000010-green 
													*csta = tcsta;
													csta++;
												}
												memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
												memset(sibuf,0,sizeof(sibuf));
												if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
												{
												#ifdef INTER_CONTROL_DEBUG
													printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
												else
												{
													write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
												}
											}//report info to server actively
										}//if ((0x5a <= cp) && (cp <= 0x5d))
										else if (0xC8 == cp)
										{//else if (0xC8 == cp)
											memset(ccon,0,sizeof(ccon));
                                            j = 0;
                                            for (i = 0; i < MAX_CHANNEL; i++)
                                            {
                                                if (*(fcdata->ccontrol) & (0x00000001 << i))
                                                {
                                                    #ifdef CLOSE_LAMP
                                                    if (((i+1) >= 5) && ((i+1) <= 12))
                                                    {
                                                        if (*(fcdata->specfunc) & (0x01 << ((i+1) - 5)))
                                                            continue;
                                                    }
                                                    #else
                                                    if((*(fcdata->specfunc)&0x10)&&(*(fcdata->specfunc)&0x20))
                                                    {
                                                        tclc = i + 1;
                                                        if(((5<=tclc)&&(tclc<=8)) || ((9<=tclc)&&(tclc<=12)))
                                                            continue;
                                                    }
                                                   if((*(fcdata->specfunc)&0x10)&&(!(*(fcdata->specfunc)&0x20)))
                                                    {
                                                        tclc = i + 1;
                                                        if ((5 <= tclc) && (tclc <= 8))
                                                            continue;
                                                    }
                                                   if((*(fcdata->specfunc)&0x20)&&(!(*(fcdata->specfunc)&0x10)))
                                                    {
                                                        tclc = i + 1;
                                                        if ((9 <= tclc) && (tclc <= 12))
                                                            continue;
                                                    }
													#endif
                                                    ccon[j] = i + 1;
                                                    j++;
                                                }
                                            }
                                            if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),ccon,0x02))
                                            {
                                            #ifdef INTER_CONTROL_DEBUG
                                                printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                            #endif
                                                *(fcdata->markbit) |= 0x0800;
                                            }
                                            if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                                                                ccon,0x02,fcdata->markbit))
                                            {
                                            #ifdef INTER_CONTROL_DEBUG
                                                printf("update err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                            #endif
                                            }
                                            if((!(*(fcdata->markbit)&0x1000))&&(!(*(fcdata->markbit)&0x8000)))
                                            {//report info to server actively
                                                stStatuSinfo.conmode = 29;
                                                stStatuSinfo.color = 0x02;
                                                stStatuSinfo.time = 0;
                                                stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
                                                stStatuSinfo.cyclet = 0;
                                                stStatuSinfo.phase = 0;

                                                stStatuSinfo.chans = 0;
                                                memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
                                                csta = stStatuSinfo.csta;
												for (i = 0; i < MAX_CHANNEL; i++)
                                                {
                                                    if (0 == ccon[i])
                                                        break;
                                                    stStatuSinfo.chans += 1;
                                                    tcsta = ccon[i];
                                                    tcsta <<= 2;
                                                    tcsta &= 0xfc;
                                                    tcsta |= 0x02; //00000010-green 
                                                    *csta = tcsta;
                                                    csta++;
                                                }
                                                memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
                                                memset(sibuf,0,sizeof(sibuf));
                                                if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
                                                {
                                                #ifdef INTER_CONTROL_DEBUG
                                                    printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                                #endif
                                                }
                                                else
                                                {
                                                    write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
                                                }
                                            }//report info to server actively
										}//else if (0xC8 == cp)
										else if ((0x01 <= cp) && (cp <= 0x20))
										{//else if ((0x01 <= cp) && (cp <= 0x20))
											memset(cc,0,sizeof(cc));
											pcc = cc;
											for (i = 0; i < (tscdata->channel->FactChannelNum); i++)
											{
												if (0 == (tscdata->channel->ChannelList[i].ChannelId))
													break;
												if (cp == (tscdata->channel->ChannelList[i].ChannelCtrlSrc))
												{
													#ifdef CLOSE_LAMP
													tclc = tscdata->channel->ChannelList[i].ChannelId;
													if ((tclc >= 0x05) && (tclc <= 0x0c))
													{
														if (*(fcdata->specfunc) & (0x01 << (tclc - 5)))
															continue;
													}
													#else
													if ((*(fcdata->specfunc)&0x10)&&(*(fcdata->specfunc)&0x20))
													{
														tclc = tscdata->channel->ChannelList[i].ChannelId;
														if(((5<=tclc)&&(tclc<=8)) || ((9<=tclc)&&(tclc<=12)))
															continue;
													}
												if ((*(fcdata->specfunc)&0x10)&&(!(*(fcdata->specfunc)&0x20)))
													{
														tclc = tscdata->channel->ChannelList[i].ChannelId;
														if ((5 <= tclc) && (tclc <= 8))
															continue;
													}
												if ((*(fcdata->specfunc)&0x20)&&(!(*(fcdata->specfunc)&0x10)))
													{
														tclc = tscdata->channel->ChannelList[i].ChannelId;
														if ((9 <= tclc) && (tclc <= 12))
															continue;
													}
													#endif
													*pcc = tscdata->channel->ChannelList[i].ChannelId;
													pcc++;
												}
											}
											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),cc,0x02))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				cc,0x02,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("update err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											if((!(*(fcdata->markbit)&0x1000))&&(!(*(fcdata->markbit)&0x8000)))
											{//report info to server actively
												stStatuSinfo.conmode = 29;
												stStatuSinfo.color = 0x02;
												stStatuSinfo.time = 0;
												stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
												stStatuSinfo.cyclet = 0;
												stStatuSinfo.phase = 0;
												stStatuSinfo.phase |= (0x01 << (cp - 1));
											/*
												stStatuSinfo.chans = 0;
												memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
												csta = stStatuSinfo.csta;
												for (i = 0; i < MAX_CHANNEL; i++)
												{
													stStatuSinfo.chans += 1;
													tcsta = i + 1;
													tcsta <<= 2;
													tcsta &= 0xfc;
													for (j = 0; j < MAX_CHANNEL; j++)
													{
														if (0 == cc[j])
															break;
														if ((i+1) == cc[j])
														{
															tcsta |= 0x02;
															break;
														}
													}
													*csta = tcsta;
													csta++;
												}
											*/
												stStatuSinfo.chans = 0;
												memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
                                                csta = stStatuSinfo.csta;
												for (i = 0; i < MAX_CHANNEL; i++)
												{
													if (0 == cc[i])
														break;
													stStatuSinfo.chans += 1;
													tcsta = cc[i];
													tcsta <<= 2;
													tcsta &= 0xfc;
													tcsta |= 0x02; //00000010-green 
													*csta = tcsta;
													csta++;
												}
												memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
												memset(sibuf,0,sizeof(sibuf));
												if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
												{
												#ifdef INTER_CONTROL_DEBUG
													printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
												else
												{
													write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
												}
											}//report info to server actively
										}//else if ((0x01 <= cp) && (cp <= 0x20))
									}//have phase control
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,80,ct.tv_sec,fcdata->markbit);
									continue;
								}//if ((1 == dcred) || (1 == close) || (1 == YellowFlashYes))

								if (0 == cp)
								{
								if ((0==stInterGrateControlPhaseInfo.cchan[0])&&(stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0))
								{
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										stStatuSinfo.conmode = /*29*/ncmode;
										stStatuSinfo.color = 0x02;
										stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime + stInterGrateControlPhaseInfo.ytime + stInterGrateControlPhaseInfo.rtime;
										stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
										stStatuSinfo.cyclet = 0;
										stStatuSinfo.phase = 0;
										stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
															
										stStatuSinfo.chans = 0;
            							memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            							csta = stStatuSinfo.csta;
            							for (i = 0; i < MAX_CHANNEL; i++)
            							{
											if (0 == stInterGrateControlPhaseInfo.chan[i])
												break;
                							stStatuSinfo.chans += 1;
                							tcsta = stInterGrateControlPhaseInfo.chan[i];
                							tcsta <<= 2;
                							tcsta &= 0xfc;
											tcsta |= 0x02;
                							*csta = tcsta;
                							csta++;
            							}
										memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            							memset(sibuf,0,sizeof(sibuf));
            							if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            							{
            							#ifdef INTER_CONTROL_DEBUG
                							printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            							#endif
            							}
            							else
            							{
                							write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            							}
									}//report info to server actively
								}//if ((0==stInterGrateControlPhaseInfo.cchan[0])&&(stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0))

								if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.gftime > 0))
								{
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										stStatuSinfo.conmode = /*28*/ncmode;
										stStatuSinfo.color = 0x03;
										stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime;
										stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
										stStatuSinfo.cyclet = 0;
										stStatuSinfo.phase = 0;
										stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
									
										for (i = 0; i < MAX_CHANNEL; i++)
                                		{
                                			if (0 == stInterGrateControlPhaseInfo.cchan[i])
                                        		break;
                                    		for (j = 0; j < stStatuSinfo.chans; j++)
                                    		{
                                        		if (0 == stStatuSinfo.csta[j])
                                            		break;
                                        		tcsta = stStatuSinfo.csta[j];
                                        		tcsta &= 0xfc;
                                        		tcsta >>= 2;
                                        		tcsta &= 0x3f;
                                        		if (stInterGrateControlPhaseInfo.cchan[i] == tcsta)
                                        		{
                                            		stStatuSinfo.csta[j] &= 0xfc;
                                           			stStatuSinfo.csta[j] |= 0x03; //00000001-green flash
													break;
                                        		}
                                    		}
                                		}								
										memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            							memset(sibuf,0,sizeof(sibuf));
            							if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            							{
            							#ifdef INTER_CONTROL_DEBUG
                							printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            							#endif
            							}
            							else
            							{
                							write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            							}
									}//report info to server actively
								}//if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.gftime > 0))

								//current phase begin to green flash
								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
								{
									memset(&dctd,0,sizeof(dctd));
									dctd.mode = 29;//traffic control
									dctd.pattern = *(fcdata->patternid);
									dctd.lampcolor = 0x02;
									dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
									dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
									dctd.stageline = stInterGrateControlPhaseInfo.stageline;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef I_EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
								{
									memset(&dctd,0,sizeof(dctd));
                                    dctd.mode = 29;//traffic control
                                    dctd.pattern = *(fcdata->patternid);
                                    dctd.lampcolor = 0x02;
                                    dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
                                    dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                    dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
									if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#endif
								fbdata[1] = 29;
                            	fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            	fbdata[3] = 0x02;
                            	fbdata[4] = stInterGrateControlPhaseInfo.gftime;
                            	if (!wait_write_serial(*(fcdata->fbserial)))
                            	{
                                	if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                	{
                                	#ifdef INTER_CONTROL_DEBUG
                                    	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef INTER_CONTROL_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                	}
                            	}
                            	else
                            	{
                            	#ifdef INTER_CONTROL_DEBUG
                                	printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            	#endif
                            	}
								if (stInterGrateControlPhaseInfo.gftime > 0)
								{	
									ngf = 0;
									while (1)
									{
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cchan,0x03))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																stInterGrateControlPhaseInfo.cchan,0x03,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);

										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cchan,0x02))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			stInterGrateControlPhaseInfo.cchan,0x02,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);
									
										ngf += 1;
										if (ngf >= stInterGrateControlPhaseInfo.gftime)
											break;
									}
								}//if (stInterGrateControlPhaseInfo.gftime > 0)
								if (1 == (stInterGrateControlPhaseInfo.cpcexist))
								{
									//current phase begin to yellow lamp
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cnpchan,0x01))
        							{
        							#ifdef INTER_CONTROL_DEBUG
            							printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        							#endif
										*(fcdata->markbit) |= 0x0800;
        							}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
													stInterGrateControlPhaseInfo.cnpchan,0x01, fcdata->markbit))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cpchan,0x00))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																	stInterGrateControlPhaseInfo.cpchan,0x00,fcdata->markbit))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
								}
								else
								{
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cchan,0x01))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														stInterGrateControlPhaseInfo.cchan,0x01,fcdata->markbit))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
								}

								if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.ytime > 0))
								{
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										stStatuSinfo.conmode = /*29*/ncmode;
										stStatuSinfo.color = 0x01;
										stStatuSinfo.time = stInterGrateControlPhaseInfo.ytime;
										stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
										stStatuSinfo.cyclet = 0;
										stStatuSinfo.phase = 0;
										stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
															
										for (i = 0; i < MAX_CHANNEL; i++)
            							{
                							if (0 == stInterGrateControlPhaseInfo.cnpchan[i])
                    							break;
                							for (j = 0; j < stStatuSinfo.chans; j++)
                							{
                    							if (0 == stStatuSinfo.csta[j])
                        							break;
                    							tcsta = stStatuSinfo.csta[j];
                    							tcsta &= 0xfc;
                    							tcsta >>= 2;
                    							tcsta &= 0x3f;
                    							if (stInterGrateControlPhaseInfo.cnpchan[i] == tcsta)
                    							{
                        							stStatuSinfo.csta[j] &= 0xfc;
													stStatuSinfo.csta[j] |= 0x01; //00000001-yellow
													break;
                    							}
                							}
            							}
										for (i = 0; i < MAX_CHANNEL; i++)
                                		{
                                			if (0 == stInterGrateControlPhaseInfo.cpchan[i])
                                        		break;
                                    		for (j = 0; j < stStatuSinfo.chans; j++)
                                    		{
                                        		if (0 == stStatuSinfo.csta[j])
                                            		break;
                                        		tcsta = stStatuSinfo.csta[j];
                                        		tcsta &= 0xfc;
                                        		tcsta >>= 2;
                                        		tcsta &= 0x3f;
                                        		if (stInterGrateControlPhaseInfo.cpchan[i] == tcsta)
                                        		{
                                            		stStatuSinfo.csta[j] &= 0xfc;
													break;
                                        		}
                                    		}
                                		}
										memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            							memset(sibuf,0,sizeof(sibuf));
            							if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            							{
            							#ifdef INTER_CONTROL_DEBUG
                							printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            							#endif
            							}
            							else
            							{
                							write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            							}
									}//report info to server actively
								}//if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.ytime > 0))

								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            	{
                               		memset(&dctd,0,sizeof(dctd));
                               		dctd.mode = 29;//traffic control
                               		dctd.pattern = *(fcdata->patternid);
                               		dctd.lampcolor = 0x01;
                               		dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                               		dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                               		dctd.stageline = stInterGrateControlPhaseInfo.stageline;
                               		if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
                               		{
                               		#ifdef INTER_CONTROL_DEBUG
                                   		printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               		#endif
                               		}
                            	}
								#ifdef I_EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
								{
									memset(&dctd,0,sizeof(dctd));
                                    dctd.mode = 29;//traffic control
                                    dctd.pattern = *(fcdata->patternid);
                                    dctd.lampcolor = 0x01;
                                    dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                                    dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                    dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
									if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#endif
								fbdata[1] = 29;
                            	fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            	fbdata[3] = 0x01;
                            	fbdata[4] = stInterGrateControlPhaseInfo.ytime;
                            	if (!wait_write_serial(*(fcdata->fbserial)))
                            	{
                               		if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               		{
                               		#ifdef INTER_CONTROL_DEBUG
                                   		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               		#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
                                   		update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                   		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   		{
                                   		#ifdef INTER_CONTROL_DEBUG
                                       		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   		#endif
                                   		}
                               		}
                            	}
                            	else
                            	{
                            	#ifdef INTER_CONTROL_DEBUG
                               		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            	#endif
                            	}
								sleep(stInterGrateControlPhaseInfo.ytime);

								//current phase begin to red lamp
								if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cchan,0x00))
                            	{
                            	#ifdef INTER_CONTROL_DEBUG
                                	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            	#endif
									*(fcdata->markbit) |= 0x0800;
                            	}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															stInterGrateControlPhaseInfo.cchan,0x00,fcdata->markbit))
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}

								if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.rtime > 0))
								{
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										stStatuSinfo.conmode = /*29*/ncmode;
										stStatuSinfo.color = 0x00;
										stStatuSinfo.time = stInterGrateControlPhaseInfo.rtime;
										stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
										stStatuSinfo.cyclet = 0;
										stStatuSinfo.phase = 0;
										stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
															
										for (i = 0; i < MAX_CHANNEL; i++)
            							{
                							if (0 == stInterGrateControlPhaseInfo.cchan[i])
                    							break;
                							for (j = 0; j < stStatuSinfo.chans; j++)
                							{
                    							if (0 == stStatuSinfo.csta[j])
                        							break;
                    							tcsta = stStatuSinfo.csta[j];
                    							tcsta &= 0xfc;
                    							tcsta >>= 2;
                    							tcsta &= 0x3f;
                    							if (stInterGrateControlPhaseInfo.cchan[i] == tcsta)
                    							{
                        							stStatuSinfo.csta[j] &= 0xfc;
													break;
                    							}
                							}
            							}
										memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            							memset(sibuf,0,sizeof(sibuf));
            							if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            							{
            							#ifdef INTER_CONTROL_DEBUG
                							printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            							#endif
            							}
            							else
            							{
                							write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            							}
									}//report info to server actively
								}//if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.rtime > 0))

								if (stInterGrateControlPhaseInfo.rtime > 0)
								{
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            		{
                                		memset(&dctd,0,sizeof(dctd));
                                		dctd.mode = 29;//traffic control
                                		dctd.pattern = *(fcdata->patternid);
                                		dctd.lampcolor = 0x00;
                                		dctd.lamptime = stInterGrateControlPhaseInfo.rtime;
                                		dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                		dctd.stageline = stInterGrateControlPhaseInfo.stageline;
                                		if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
                                		{
                                		#ifdef INTER_CONTROL_DEBUG
                                    		printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                		#endif
                                		}
                            		}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 29;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x00;
                                        dctd.lamptime = stInterGrateControlPhaseInfo.rtime;
                                        dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                        dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                                	fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                                	fbdata[3] = 0x00;
                                	fbdata[4] = stInterGrateControlPhaseInfo.rtime;
                                	if (!wait_write_serial(*(fcdata->fbserial)))
                                	{
                                    	if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                    	{
                                    	#ifdef INTER_CONTROL_DEBUG
                                        	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                        	update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                        	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                        	{
                                        	#ifdef INTER_CONTROL_DEBUG
                                            	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        	#endif
                                        	}
                                    	}
                                	}
                                	else
                                	{
                                	#ifdef INTER_CONTROL_DEBUG
                                    	printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                	#endif
                                	}
									sleep(stInterGrateControlPhaseInfo.rtime);
								}

								*(fcdata->slnum) += 1;
								*(fcdata->stageid) = \
									tscdata->timeconfig->TimeConfigList[rettl][*(fcdata->slnum)].StageId;
								if (0 == (tscdata->timeconfig->TimeConfigList[rettl][*(fcdata->slnum)].StageId))
								{
									*(fcdata->slnum) = 0;
									*(fcdata->stageid) = \
										tscdata->timeconfig->TimeConfigList[rettl][*(fcdata->slnum)].StageId;
								}
								//get phase info of next phase
								memset(&stInterGrateControlPhaseInfo,0,sizeof(stInterGrateControlPhaseInfo));
        						if (SUCCESS != igc_get_phase_info(fcdata,tscdata,rettl,*(fcdata->slnum),&stInterGrateControlPhaseInfo))
        						{
        						#ifdef INTER_CONTROL_DEBUG
            						printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
									output_log("Timing control,get phase info err");
        						#endif
									Inter_Control_End_Part_Child_Thread();
            						return FAIL;
        						}
        						*(fcdata->phaseid) = 0;
        						*(fcdata->phaseid) |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));

								if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.chan,0x02))
                            	{
                            	#ifdef INTER_CONTROL_DEBUG
                                	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            	#endif
									*(fcdata->markbit) |= 0x0800;
                            	}
						
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																stInterGrateControlPhaseInfo.chan,0x02,fcdata->markbit))
                            	{
                            	#ifdef INTER_CONTROL_DEBUG
                                	printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            	#endif
                            	}
								ncmode = *(fcdata->contmode);
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									stStatuSinfo.conmode = /*29*/ncmode;
									stStatuSinfo.color = 0x02;
									stStatuSinfo.time = 0;
									stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
									stStatuSinfo.cyclet = 0;
									stStatuSinfo.phase = 0;
									stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
															
									stStatuSinfo.chans = 0;
            						memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            						csta = stStatuSinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
										if (0 == stInterGrateControlPhaseInfo.chan[i])
											break;
                						stStatuSinfo.chans += 1;
                						tcsta = stInterGrateControlPhaseInfo.chan[i];
                						tcsta <<= 2;
                						tcsta &= 0xfc;
										tcsta |= 0x02;
                						*csta = tcsta;
                						csta++;
            						}
									memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            						{
            						#ifdef INTER_CONTROL_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively
	
								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
								{
									memset(&dctd,0,sizeof(dctd));
									dctd.mode = 29;//traffic control
									dctd.pattern = *(fcdata->patternid);
									dctd.lampcolor = 0x02;
									dctd.lamptime = 0;
									dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
									dctd.stageline = stInterGrateControlPhaseInfo.stageline;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef I_EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
								{
									memset(&dctd,0,sizeof(dctd));
                                    dctd.mode = 29;//traffic control
                                    dctd.pattern = *(fcdata->patternid);
                                    dctd.lampcolor = 0x02;
                                    dctd.lamptime = 0;
                                    dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                    dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
									if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#endif
								fbdata[1] = 29;
                            	fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            	fbdata[3] = 0x02;
                            	fbdata[4] = 0;
                            	if (!wait_write_serial(*(fcdata->fbserial)))
                            	{
                                	if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                	{
                                	#ifdef INTER_CONTROL_DEBUG
                                    	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef INTER_CONTROL_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                	}
                            	}
                            	else
                            	{
                            	#ifdef INTER_CONTROL_DEBUG
                                	printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            	#endif
                            	}							
								}//0 == cp
								if (0 != cp)
								{
									if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
									{//if ((0x5a <= cp) && (cp <= 0x5d))
										*(fcdata->slnum) = 0;
										*(fcdata->stageid) = \
												tscdata->timeconfig->TimeConfigList[rettl][0].StageId;
										memset(&stInterGrateControlPhaseInfo,0,sizeof(stInterGrateControlPhaseInfo));
										if(SUCCESS != igc_get_phase_info(fcdata,tscdata,rettl,0,&stInterGrateControlPhaseInfo))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
											output_log("Timing control,get phase info err");
										#endif
											Inter_Control_End_Part_Child_Thread();
											return FAIL;
										}
										*(fcdata->phaseid) = 0;
										*(fcdata->phaseid) |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
										memset(cc,0,sizeof(cc));
                                        pcc = cc;
										if ((0x5a <= cp) && (cp <= 0x5d))
                                        {
											for (i = 0; i < MAX_CHANNEL; i++)
											{
												if (0 == lkch[cp-0x5a][i])
													break;
												*pcc = lkch[cp-0x5a][i];
												pcc++;
											}
										}
										else if (0xC8 == cp)
										{
											for (i = 0; i < MAX_CHANNEL; i++)
                                            {
                                                if (*(fcdata->ccontrol) & (0x00000001 << i))
                                                {
                                                    #ifdef CLOSE_LAMP
                                                    if (((i+1) >= 5) && ((i+1) <= 12))
                                                    {
                                                        if (*(fcdata->specfunc) & (0x01 << ((i+1) - 5)))
                                                            continue;
                                                    }
                                                    #else
                                                    if((*(fcdata->specfunc)&0x10)&&(*(fcdata->specfunc)&0x20))
                                                    {
                                                        tclc = i + 1;
                                                        if(((5<=tclc)&&(tclc<=8)) || ((9<=tclc)&&(tclc<=12)))
                                                            continue;
                                                    }
                                                   if((*(fcdata->specfunc)&0x10)&&(!(*(fcdata->specfunc)&0x20)))
                                                    {
                                                        tclc = i + 1;
                                                        if ((5 <= tclc) && (tclc <= 8))
                                                            continue;
                                                    }
                                                   if((*(fcdata->specfunc)&0x20)&&(!(*(fcdata->specfunc)&0x10)))
                                                    {
                                                        tclc = i + 1;
                                                        if ((9 <= tclc) && (tclc <= 12))
                                                            continue;
                                                    }
                                                    #endif
                                                    *pcc = i + 1;
													pcc++;
                                                }
                                            }
										}//else if (0xC8 == cp)
                                        memset(wcc,0,sizeof(wcc));
                                        memset(wnpcc,0,sizeof(wnpcc));
                                        memset(wpcc,0,sizeof(wpcc));
                                        z = 0;
                                        k = 0;
                                        s = 0;
                                        pce = 0;
                                        for (i = 0; i < MAX_CHANNEL; i++)
                                        {//for (i = 0; i < MAX_CHANNEL; i++)
                                            if (0 == cc[i])
                                                break;
                                            ce = 0;
											for (j = 0; j < MAX_CHANNEL; j++)
                                            {
                                                if (0 == stInterGrateControlPhaseInfo.chan[j])
                                                    break;
                                                if (stInterGrateControlPhaseInfo.chan[j] == cc[i])
                                                {
                                                    ce = 1;
                                                    break;
                                                }
                                            }
                                            if (0 == ce)
                                            {
                                                wcc[z] = cc[i];
                                                z++;
                                                if ((0x0d <= cc[i]) && (cc[i] <= 0x10))
                                                {
                                                    wpcc[k] = cc[i];
                                                    k++;
                                                    pce = 1;
                                                    continue;
                                                }
                                                else
                                                {
                                                    wnpcc[s] = cc[i];
                                                    s++;
                                                    continue;
                                                }
                                            }
										}
									}//if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
									else if ((0x01 <= cp) && (cp <= 0x20))
									{//else if ((0x01 <= cp) && (cp <= 0x20))
										for (i = 0; i < (tscdata->timeconfig->FactStageNum); i++)
										{
											if (0 == (tscdata->timeconfig->TimeConfigList[rettl][i].StageId))
												break;
											rpi = tscdata->timeconfig->TimeConfigList[rettl][i].PhaseId;
											rpc = 0;
											get_phase_id(rpi,&rpc);
											if (cp == rpc)
											{
												break;
											}
										}
										if (i != (tscdata->timeconfig->FactStageNum))
										{
											if (0 == (tscdata->timeconfig->TimeConfigList[rettl][i+1].StageId))
											{
												*(fcdata->slnum) = 0;
												*(fcdata->stageid) = \
												tscdata->timeconfig->TimeConfigList[rettl][0].StageId;
												memset(&stInterGrateControlPhaseInfo,0,sizeof(stInterGrateControlPhaseInfo));
												if(SUCCESS != igc_get_phase_info(fcdata,tscdata,rettl,0,&stInterGrateControlPhaseInfo))
												{
												#ifdef INTER_CONTROL_DEBUG
													printf("info err,File: %s,Line: %d\n",__FILE__,__LINE__);
													output_log("Timing control,get phase info err");
												#endif
													Inter_Control_End_Part_Child_Thread();
													return FAIL;
												}
												*(fcdata->phaseid) = 0;
												*(fcdata->phaseid) |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
											}
											else
											{
												*(fcdata->slnum) = i + 1;
												*(fcdata->stageid) = \
													tscdata->timeconfig->TimeConfigList[rettl][i+1].StageId;
												memset(&stInterGrateControlPhaseInfo,0,sizeof(stInterGrateControlPhaseInfo));
												if(SUCCESS!=igc_get_phase_info(fcdata,tscdata,rettl,i+1,&stInterGrateControlPhaseInfo))
												{
												#ifdef INTER_CONTROL_DEBUG
													printf("info err,File: %s,Line: %d\n",__FILE__,__LINE__);
													output_log("Timing control,get phase info err");
												#endif
													Inter_Control_End_Part_Child_Thread();
													return FAIL;
												}
												*(fcdata->phaseid) = 0;
												*(fcdata->phaseid) |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
											}
										}//if (i != (tscdata->timeconfig->FactStageNum))
										else
										{
											*(fcdata->slnum) = 0;
											*(fcdata->stageid) = \
													tscdata->timeconfig->TimeConfigList[rettl][0].StageId;
											memset(&stInterGrateControlPhaseInfo,0,sizeof(stInterGrateControlPhaseInfo));
											if(SUCCESS != igc_get_phase_info(fcdata,tscdata,rettl,0,&stInterGrateControlPhaseInfo))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
												output_log("Timing control,get phase info err");
											#endif
												Inter_Control_End_Part_Child_Thread();
												return FAIL;
											}
											*(fcdata->phaseid) = 0;
											*(fcdata->phaseid) |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
										}//if (i == (tscdata->timeconfig->FactStageNum))

										memset(cc,0,sizeof(cc));
										pcc = cc;
										memset(wcc,0,sizeof(wcc));
										memset(wnpcc,0,sizeof(wnpcc));
										memset(wpcc,0,sizeof(wpcc));
										z = 0;
										k = 0;
										s = 0;
										pce = 0;
										for (i = 0; i < (tscdata->channel->FactChannelNum); i++)
										{
											if (0 == (tscdata->channel->ChannelList[i].ChannelId))
												break;
											if (cp == (tscdata->channel->ChannelList[i].ChannelCtrlSrc))
											{
												#ifdef CLOSE_LAMP
                                                tclc = tscdata->channel->ChannelList[i].ChannelId;
                                                if ((tclc >= 0x05) && (tclc <= 0x0c))
                                                {
                                                    if (*(fcdata->specfunc) & (0x01 << (tclc - 5)))
                                                        continue;
                                                }
												#else
												if ((*(fcdata->specfunc)&0x10)&&(*(fcdata->specfunc)&0x20))
            									{
                									tclc = tscdata->channel->ChannelList[i].ChannelId;
                									if(((5<=tclc)&&(tclc<=8)) || ((9<=tclc)&&(tclc<=12)))
                    									continue;
            									}
												if ((*(fcdata->specfunc)&0x10)&&(!(*(fcdata->specfunc)&0x20)))
												{
													tclc = tscdata->channel->ChannelList[i].ChannelId;
													if ((5 <= tclc) && (tclc <= 8))
														continue;
												}
												if ((*(fcdata->specfunc)&0x20)&&(!(*(fcdata->specfunc)&0x10)))
												{
													tclc = tscdata->channel->ChannelList[i].ChannelId;
                									if ((9 <= tclc) && (tclc <= 12))
                    									continue;
												}
                                                #endif
												*pcc = tscdata->channel->ChannelList[i].ChannelId;
												pcc++;
												ce = 0;
												for (j = 0; j < MAX_CHANNEL; j++)
												{
													if (0 == stInterGrateControlPhaseInfo.chan[j])
														break;
												if (stInterGrateControlPhaseInfo.chan[j]==tscdata->channel->ChannelList[i].ChannelId)
													{
														ce = 1;
														break;
													}
												}
												if (0 == ce)
												{
													wcc[z] = tscdata->channel->ChannelList[i].ChannelId;
													z++;
													if (3 == tscdata->channel->ChannelList[i].ChannelType)
													{
														wpcc[k] = tscdata->channel->ChannelList[i].ChannelId;
														k++;
														pce = 1;
														continue;
													}
													else
													{
														wnpcc[s] = tscdata->channel->ChannelList[i].ChannelId;
														s++;
														continue;
													}		
												}
											}
										}
									}//else if ((0x01 <= cp) && (cp <= 0x20))

									if((0==wcc[0])&&(stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
                                                stStatuSinfo.conmode = /*29*/ncmode;
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
                                                stStatuSinfo.conmode = cp;
											else if (0xC8 == cp)
                                                stStatuSinfo.conmode = 29;
											stStatuSinfo.color = 0x02;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime + stInterGrateControlPhaseInfo.ytime + stInterGrateControlPhaseInfo.rtime;
											stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
											stStatuSinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                stStatuSinfo.phase = 0;
                                                stStatuSinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
                                            {
                                                stStatuSinfo.phase = 0;
                                            }
											stStatuSinfo.chans = 0;
            								memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            								csta = stStatuSinfo.csta;
            								for (i = 0; i < MAX_CHANNEL; i++)
            								{
												if (0 == cc[i])
													break;
                								stStatuSinfo.chans += 1;
                								tcsta = cc[i];
                								tcsta <<= 2;
                								tcsta &= 0xfc;
												tcsta |= 0x02;
                								*csta = tcsta;
                								csta++;
            								}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//info.cchan[0])&&(stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0))

									if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.gftime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
                                                stStatuSinfo.conmode = /*28*/ncmode;
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
                                                stStatuSinfo.conmode = cp;
											else if (0xC8 == cp)
                                                stStatuSinfo.conmode = 29;
											stStatuSinfo.color = 0x03;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime;
											stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
											stStatuSinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                stStatuSinfo.phase = 0;
                                                stStatuSinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
                                            {
                                                stStatuSinfo.phase = 0;
                                            }
	
											for (i = 0; i < MAX_CHANNEL; i++)
                                			{
                                				if (0 == wcc[i])
                                        			break;
                                    			for (j = 0; j < stStatuSinfo.chans; j++)
                                    			{
                                        			if (0 == stStatuSinfo.csta[j])
                                            			break;
                                        			tcsta = stStatuSinfo.csta[j];
                                        			tcsta &= 0xfc;
                                        			tcsta >>= 2;
                                        			tcsta &= 0x3f;
                                        			if (wcc[i] == tcsta)
                                        			{
                                            			stStatuSinfo.csta[j] &= 0xfc;
                                           				stStatuSinfo.csta[j] |= 0x03; //00000001-green flash
														break;
                                        			}
                                    			}
                                			}								
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.gftime > 0))

									//current phase begin to green flash
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&dctd,0,sizeof(dctd));
										dctd.mode = 29;//traffic control
										dctd.pattern = *(fcdata->patternid);
										dctd.lampcolor = 0x02;
										dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
										dctd.phaseid = cp;
										dctd.stageline = stInterGrateControlPhaseInfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 29;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x02;
                                        dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
                                        dctd.phaseid = cp;
                                        dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = stInterGrateControlPhaseInfo.gftime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                                		if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                		{
                                		#ifdef INTER_CONTROL_DEBUG
                                    		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                    		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                    		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                    		{
                                    		#ifdef INTER_CONTROL_DEBUG
                                        		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
                                    		}
                                		}
                            		}
                            		else
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                                		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									if (stInterGrateControlPhaseInfo.gftime > 0)
									{	
										ngf = 0;
										while (1)
										{
											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x03,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("updatechannelerr,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("setgreenlamperr,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x02,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("updatechannelerr,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);
									
											ngf += 1;
											if (ngf >= stInterGrateControlPhaseInfo.gftime)
												break;
										}
									}//if (stInterGrateControlPhaseInfo.gftime > 0)
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS!=igc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef INTER_CONTROL_DEBUG
            								printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														wcc,0x01,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}

									if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.ytime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
                                                stStatuSinfo.conmode = /*29*/ncmode;
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
                                                stStatuSinfo.conmode = cp;
											else if (0xC8 == cp)
                                                stStatuSinfo.conmode = 29;
											stStatuSinfo.color = 0x01;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.ytime;
											stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
											stStatuSinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                stStatuSinfo.phase = 0;
                                                stStatuSinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
                                            {
                                                stStatuSinfo.phase = 0;
                                            }
	
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == wnpcc[i])
                    								break;
                								for (j = 0; j < stStatuSinfo.chans; j++)
                								{
                    								if (0 == stStatuSinfo.csta[j])
                        								break;
                    								tcsta = stStatuSinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (wnpcc[i] == tcsta)
                    								{
                        								stStatuSinfo.csta[j] &= 0xfc;
														stStatuSinfo.csta[j] |= 0x01; //00000001-yellow
														break;
                    								}
                								}
            								}
											for (i = 0; i < MAX_CHANNEL; i++)
                                			{
                                				if (0 == wpcc[i])
                                        			break;
                                    			for (j = 0; j < stStatuSinfo.chans; j++)
                                    			{
                                        			if (0 == stStatuSinfo.csta[j])
                                            			break;
                                        			tcsta = stStatuSinfo.csta[j];
                                        			tcsta &= 0xfc;
                                        			tcsta >>= 2;
                                        			tcsta &= 0x3f;
                                        			if (wpcc[i] == tcsta)
                                        			{
                                            			stStatuSinfo.csta[j] &= 0xfc;
														break;
                                        			}
                                    			}
                                			}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.ytime > 0))

									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            		{
                               			memset(&dctd,0,sizeof(dctd));
                               			dctd.mode = 29;//traffic control
                               			dctd.pattern = *(fcdata->patternid);
                               			dctd.lampcolor = 0x01;
                               			dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                               			dctd.phaseid = cp;
                               			dctd.stageline = stInterGrateControlPhaseInfo.stageline;
                               			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
                               			{
                               			#ifdef INTER_CONTROL_DEBUG
                                   			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               			#endif
                               			}
                            		}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 29;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x01;
                                        dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                                        dctd.phaseid = cp;
                                        dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            		fbdata[3] = 0x01;
                            		fbdata[4] = stInterGrateControlPhaseInfo.ytime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef INTER_CONTROL_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef INTER_CONTROL_DEBUG
                                       			printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(stInterGrateControlPhaseInfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                                		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															wcc,0x00,fcdata->markbit))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}

									if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.rtime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
                                                stStatuSinfo.conmode = /*29*/ncmode;
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
                                                stStatuSinfo.conmode = cp;
											else if (0xC8 == cp)
                                                stStatuSinfo.conmode = 29;
											stStatuSinfo.color = 0x00;
											stStatuSinfo.time = stInterGrateControlPhaseInfo.rtime;
											stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
											stStatuSinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                stStatuSinfo.phase = 0;
                                                stStatuSinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
                                                stStatuSinfo.phase = 0;
	
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == wcc[i])
                    								break;
                								for (j = 0; j < stStatuSinfo.chans; j++)
                								{
                    								if (0 == stStatuSinfo.csta[j])
                        								break;
                    								tcsta = stStatuSinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (wcc[i] == tcsta)
                    								{
                        								stStatuSinfo.csta[j] &= 0xfc;
														break;
                    								}
                								}
            								}
											memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            								{
            								#ifdef INTER_CONTROL_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.rtime > 0))

									if (stInterGrateControlPhaseInfo.rtime > 0)
									{
										if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            			{
                                			memset(&dctd,0,sizeof(dctd));
                                			dctd.mode = 29;//traffic control
                                			dctd.pattern = *(fcdata->patternid);
                                			dctd.lampcolor = 0x00;
                                			dctd.lamptime = stInterGrateControlPhaseInfo.rtime;
                                			dctd.phaseid = cp;
                                			dctd.stageline = stInterGrateControlPhaseInfo.stageline;
                                			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
                                			{
                                			#ifdef INTER_CONTROL_DEBUG
                                    			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                            			}
										#ifdef I_EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&dctd,0,sizeof(dctd));
                                            dctd.mode = 29;//traffic control
                                            dctd.pattern = *(fcdata->patternid);
                                            dctd.lampcolor = 0x00;
                                            dctd.lamptime = stInterGrateControlPhaseInfo.rtime;
                                            dctd.phaseid = cp;
                                            dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#endif
										fbdata[1] = 29;
                                		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                                		fbdata[3] = 0x00;
                                		fbdata[4] = stInterGrateControlPhaseInfo.rtime;
                                		if (!wait_write_serial(*(fcdata->fbserial)))
                                		{
                                    		if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                    		{
                                    		#ifdef INTER_CONTROL_DEBUG
                                        		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
                                        		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                        		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                        		{
                                        		#ifdef INTER_CONTROL_DEBUG
                                            		printf("file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        		#endif
                                        		}
                                    		}
                                		}
                                		else
                                		{
                                		#ifdef INTER_CONTROL_DEBUG
                                    		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                		#endif
                                		}
										sleep(stInterGrateControlPhaseInfo.rtime);
									}
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.chan,0x02))
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			stInterGrateControlPhaseInfo.chan,0x02,fcdata->markbit))
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
                            		}
									ncmode = *(fcdata->contmode);
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										stStatuSinfo.conmode = /*29*/ncmode;
										stStatuSinfo.color = 0x02;
										stStatuSinfo.time = 0;
										stStatuSinfo.stage = 0;
										stStatuSinfo.cyclet = 0;
										stStatuSinfo.phase = 0;
										stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));

										stStatuSinfo.chans = 0;
            							memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            							csta = stStatuSinfo.csta;
            							for (i = 0; i < MAX_CHANNEL; i++)
            							{
                							if (0 == stInterGrateControlPhaseInfo.chan[i])
                    							break;
                							stStatuSinfo.chans += 1;
                							tcsta = stInterGrateControlPhaseInfo.chan[i];
                							tcsta <<= 2;
                							tcsta &= 0xfc;
                							tcsta |= 0x02;
                							*csta = tcsta;
                							csta++;
            							}
										memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            							memset(sibuf,0,sizeof(sibuf));
            							if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            							{
            							#ifdef INTER_CONTROL_DEBUG
                							printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            							#endif
            							}
            							else
            							{
                							write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            							}
									}//report info to server actively									
	
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&dctd,0,sizeof(dctd));
										dctd.mode = 29;//traffic control
										dctd.pattern = *(fcdata->patternid);
										dctd.lampcolor = 0x02;
										dctd.lamptime = 0;
										dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
										dctd.stageline = stInterGrateControlPhaseInfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
                                        dctd.mode = 29;//traffic control
                                        dctd.pattern = *(fcdata->patternid);
                                        dctd.lampcolor = 0x02;
                                        dctd.lamptime = 0;
                                        dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                        dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = stInterGrateControlPhaseInfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = 0;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef INTER_CONTROL_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef INTER_CONTROL_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef INTER_CONTROL_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
								}//0 != cp

								cp = 0;
								objecti[0].objectv[0] = 0x28;
                                factovs = 0;
                                memset(cbuf,0,sizeof(cbuf));
                                if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
                                if (!(*(fcdata->markbit) & 0x1000))
                                {
                                    write(*(fcdata->sockfd->csockfd),cbuf,factovs);
                                }
								gettimeofday(&ct,NULL);
                                update_event_list(fcdata->ec,fcdata->el,1,80,ct.tv_sec,fcdata->markbit);

                                continue;
							}//net step by step
							else if(0x21 == tcbuf[1])
							{//yellow flash
								dcred = 0;
								close = 0;
								cp = 0;
								if (0 == YellowFlashYes)
								{
									memset(&yfdata,0,sizeof(yfdata));
									yfdata.second = 0;
									yfdata.markbit = fcdata->markbit;
									yfdata.markbit2 = fcdata->markbit2;
									yfdata.serial = *(fcdata->bbserial);
									yfdata.sockfd = fcdata->sockfd;
									yfdata.cs = chanstatus;	
#ifdef FLASH_DEBUG
									char szInfo[32] = {0};
									char szInfoT[64] = {0};
									snprintf(szInfo,sizeof(szInfo)-1,"%s",__FUNCTION__);
									snprintf(szInfoT,sizeof(szInfoT)-1,"%d",__LINE__);
									tsc_save_eventlog(szInfo,szInfoT);
#endif
									int yfret = pthread_create(&YellowFlashPid,NULL,(void *)Inter_Control_Yellow_Flash,&yfdata);
									if (0 != yfret)
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
										output_log("InterGrate control,create yellow flash err");
									#endif
										Inter_Control_End_Part_Child_Thread();
										objecti[0].objectv[0] = 0x24;
                                		factovs = 0;
                                		memset(cbuf,0,sizeof(cbuf));
                                		if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x00))
                                		{
                                		#ifdef INTER_CONTROL_DEBUG
                                    		printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
                                		}
                                		if (!(*(fcdata->markbit) & 0x1000))
                                		{
                                    		write(*(fcdata->sockfd->csockfd),cbuf,factovs);
                                		}

										return FAIL;
									}
									YellowFlashYes = 1;
								}
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									stStatuSinfo.conmode = 29;
									stStatuSinfo.color = 0x05;
									stStatuSinfo.time = 0;
									stStatuSinfo.stage = 0;
									stStatuSinfo.cyclet = 0;
									stStatuSinfo.phase = 0;

									stStatuSinfo.chans = 0;
            						memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            						csta = stStatuSinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						stStatuSinfo.chans += 1;
                						tcsta = i+1;
                						tcsta <<= 2;
                						tcsta &= 0xfc;
                						*csta = tcsta;
                						csta++;
            						}
									memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));							
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            						{
            						#ifdef INTER_CONTROL_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively
								ncmode = *(fcdata->contmode);
								objecti[0].objectv[0] = 0x24;
                                factovs = 0;
                                memset(cbuf,0,sizeof(cbuf));
                                if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
                                if (!(*(fcdata->markbit) & 0x1000))
                                {
                                    write(*(fcdata->sockfd->csockfd),cbuf,factovs);
                                }
								gettimeofday(&ct,NULL);
                                update_event_list(fcdata->ec,fcdata->el,1,79,ct.tv_sec,fcdata->markbit);
								continue;
							}//yellow flash
							else if (0x22 == tcbuf[1])
							{//all red
								close = 0;
								cp = 0;
								if (1 == YellowFlashYes)
								{
									pthread_cancel(YellowFlashPid);
									pthread_join(YellowFlashPid,NULL);
									YellowFlashYes = 0;
								}
					
								if (0 == dcred)
								{	
									new_all_red(&ardata);
									dcred = 1;
								}
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									stStatuSinfo.conmode = 29;
									stStatuSinfo.color = 0x00;
									stStatuSinfo.time = 0;
									stStatuSinfo.stage = 0;
									stStatuSinfo.cyclet = 0;
									stStatuSinfo.phase = 0;

									stStatuSinfo.chans = 0;
            						memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            						csta = stStatuSinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						stStatuSinfo.chans += 1;
                						tcsta = i+1;
                						tcsta <<= 2;
                						tcsta &= 0xfc;
                						*csta = tcsta;
                						csta++;
            						}
									memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));						
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            						{
            						#ifdef INTER_CONTROL_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively
								ncmode = *(fcdata->contmode);
								objecti[0].objectv[0] = 0x25;
                                factovs = 0;
                                memset(cbuf,0,sizeof(cbuf));
                                if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
                                if (!(*(fcdata->markbit) & 0x1000))
                                {
                                    write(*(fcdata->sockfd->csockfd),cbuf,factovs);
                                }
								gettimeofday(&ct,NULL);
                                update_event_list(fcdata->ec,fcdata->el,1,78,ct.tv_sec,fcdata->markbit);							
								continue;
							}//all red
							else if (0x23 == tcbuf[1])
							{//close lamp
								dcred = 0;
								cp = 0;
								if (1 == YellowFlashYes)
                                {
                                    pthread_cancel(YellowFlashPid);
                                    pthread_join(YellowFlashPid,NULL);
                                    YellowFlashYes = 0;
                                }
								if (0 == close)
                                {
                                    new_all_close(&acdata);
                                    close = 1;
                                }
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									stStatuSinfo.conmode = 29;
									stStatuSinfo.color = 0x04;
									stStatuSinfo.time = 0;
									stStatuSinfo.stage = 0;
									stStatuSinfo.cyclet = 0;
									stStatuSinfo.phase = 0;

									stStatuSinfo.chans = 0;
            						memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            						csta = stStatuSinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						stStatuSinfo.chans += 1;
                						tcsta = i+1;
                						tcsta <<= 2;
                						tcsta &= 0xfc;
                						*csta = tcsta;
                						csta++;
            						}
									memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));						
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            						{
            						#ifdef INTER_CONTROL_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively
								ncmode = *(fcdata->contmode);
								objecti[0].objectv[0] = 0x26;
                                factovs = 0;
                                memset(cbuf,0,sizeof(cbuf));
                                if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
                                if (!(*(fcdata->markbit) & 0x1000))
                                {
                                    write(*(fcdata->sockfd->csockfd),cbuf,factovs);
                                }
								gettimeofday(&ct,NULL);
                                update_event_list(fcdata->ec,fcdata->el,1,81,ct.tv_sec,fcdata->markbit);
								continue;
							}//close lamp
						}//status of network control	
					}//net control is valid only when wireless and key control is invalid;
				}//network control
				else if ((0xDA == tcbuf[0]) && (0xED == tcbuf[2]))
				{//key traffic control
					if ((0 == wllock) && (0 == netlock) && (0 == wtlock))
					{//key lock or unlock is suessfully only when wireless lock or unlock is not valid;
						if (0x01 == tcbuf[1])
						{//lock or unlock
							if (0 == keylock)
							{
							#ifdef INTER_CONTROL_DEBUG
								printf("Prepare to lock,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								Get_Inter_Greate_Control_Status(&color,&leatime);
								if (2 != color)
								{//lamp color is not green
									struct timeval			spatime;
									unsigned char			endlock = 0;
									while(1)
									{//inline while loop
										spatime.tv_sec = 0;
										spatime.tv_usec = 200000;
										select(0,NULL,NULL,NULL,&spatime);
										memset(tcbuf,0,sizeof(tcbuf));
										read(*(fcdata->conpipe),tcbuf,sizeof(tcbuf));
										if ((0xDA == tcbuf[0]) && (0xED == tcbuf[2]))
										{
											if (0x01 == tcbuf[1])
											{
												endlock = 1;
												break;
											}
										}
										Get_Inter_Greate_Control_Status(&color,&leatime);
										if (2 == color)
										{
											break;
										}
										else
										{
											continue;
										}
									}//inline while loop
									if (1 == endlock)
									{
										continue;
									}
								}//lamp color is not green
#if 0
								//the following code is for WuXi check;
                                #ifdef INTER_CONTROL_DEBUG
                                printf("*******************leatime: %d,mingtime: %d,File:%s,Line:%d\n", \
                                        leatime,stInterGrateControlPhaseInfo.mingtime,__FILE__,__LINE__);
                                #endif
								if (leatime < stInterGrateControlPhaseInfo.mingtime)
								{
									unsigned char		cmt = stInterGrateControlPhaseInfo.mingtime - leatime;
									unsigned char		endlock = 0;
									while (1)
									{//2 while
										FD_ZERO(&nread);
        								FD_SET(*(fcdata->conpipe),&nread);
        								int max = *(fcdata->conpipe);
										mont.tv_sec = cmt;
										mont.tv_usec = 0;
										memset(&ltime,0,sizeof(struct timeval));
										gettimeofday(&ltime,NULL);
										int mret = select(max+1,&nread,NULL,NULL,&mont);
										if (mret < 0)
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											return FAIL;
										}
										if (0 == mret)
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("min green has ended,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											break;
										}
										if (mret > 0)
										{//mret > 0
											if (FD_ISSET(*(fcdata->conpipe),&nread))
											{
												memset(tcbuf,0,sizeof(tcbuf));
												read(*(fcdata->conpipe),tcbuf,sizeof(tcbuf));
												if ((0xDA == tcbuf[0]) && (0xED == tcbuf[2]))
												{
													if (0x01 == tcbuf[1])
													{
														endlock = 1;
														break;
													}
												}
											}//if (FD_ISSET(*(fcdata->conpipe),&nread))
											else
											{
												memset(&ct,0,sizeof(struct timeval));
												gettimeofday(&ct,NULL);	
												cmt = cmt - (ct.tv_sec - ltime.tv_sec);
												continue; 
											}
										}//mret > 0
									}//2 while
									if (1 == endlock)
										continue;
								}//if (leatime < stInterGrateControlPhaseInfo.mingtime)
								#ifdef INTER_CONTROL_DEBUG
								printf("end mintime pass,File: %s,LIne:%d\n",__FILE__,__LINE__);
								#endif
                                //end code of WuXi check;
#endif
								keylock = 1;
								dcred = 0;
								close = 0;
                                cktem = 0;
                                kstep = 0;
								Inter_Control_End_Child_Thread();//end man thread and its child thread

								*(fcdata->markbit2) |= 0x0002;
							#if 0
								new_all_red(&ardata);
							//	all_red(*(fcdata->bbserial),0);	
								if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.chan,0x02))
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									gettimeofday(&ct,NULL);
                					update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                					if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                					{
                					#ifdef INTER_CONTROL_DEBUG
                    					printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                					#endif
                					}
									*(fcdata->markbit) |= 0x0800;
								}
						#endif
						
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									stStatuSinfo.conmode = 28;
									if(0 == bAllLampClose)
									{
										stStatuSinfo.color = 0x02;
									}
									else
									{
										stStatuSinfo.color = 0x04;
									}
									stStatuSinfo.time = 0;
									stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
									stStatuSinfo.cyclet = 0;
									stStatuSinfo.phase = 0;
									stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
															
									stStatuSinfo.chans = 0;
            						memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            						csta = stStatuSinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						if (0 == stInterGrateControlPhaseInfo.chan[i])
                    						break;
                						stStatuSinfo.chans += 1;
                						tcsta = stInterGrateControlPhaseInfo.chan[i];
                						tcsta <<= 2;
                						tcsta &= 0xfc;
                						if(0 == bAllLampClose)
                						{
                							tcsta |= 0x02; //00000010-green
                						}
                						*csta = tcsta;
                						csta++;
            						}
									memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            						{
            						#ifdef INTER_CONTROL_DEBUG
                						printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively
					
								if (*(fcdata->auxfunc) & 0x01)
                                {//if (*(fcdata->auxfunc) & 0x01)
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),dcdownti,sizeof(dcdownti)) < 0)
                                        {
                                        #ifdef INTER_CONTROL_DEBUG
                                            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),dcedownti,sizeof(dcedownti)) < 0)
                                        {
                                        #ifdef INTER_CONTROL_DEBUG
                                            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (*(fcdata->auxfunc) & 0x01)
	
								*(fcdata->contmode) = 28;//traffic control mode
								//send current control info to face board
								fbdata[1] = 28;
								fbdata[2] = stInterGrateControlPhaseInfo.stageline;
								fbdata[3] = 2;
								fbdata[4] = 0;
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    	if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef INTER_CONTROL_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
									}
								}
								else
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
								dcecho[1] = 0x01;
								if (*(fcdata->markbit2) & 0x0800)
								{//comes from side door serial
									*(fcdata->markbit2) &= 0xf7ff;
									if (!wait_write_serial(*(fcdata->sdserial)))
                                    {
                                        if (write(*(fcdata->sdserial),dcecho,sizeof(dcecho)) < 0)
                                        {
                                        #ifdef INTER_CONTROL_DEBUG
                                            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                            *(fcdata->markbit) |= 0x0800;
                                        }
                                    }
                                    else
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                    #endif
                                    }
								}//comes from side door serial
								else
								{
									if (!wait_write_serial(*(fcdata->fbserial)))
									{
										if (write(*(fcdata->fbserial),dcecho,sizeof(dcecho)) < 0)
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
											update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
											if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																			fcdata->softevent,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
											}
										}
									}
									else
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
								}

								//send down time to configure tool
                            	if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            	{
                                	memset(&dctd,0,sizeof(dctd));
                                	dctd.mode = 28;
                                	dctd.pattern = *(fcdata->patternid);
                                	if(0 == bAllLampClose)
                                	{
                                		dctd.lampcolor = 0x02;
                                	}
									else
									{
										dctd.lampcolor = 0x03;
									}
                                	dctd.lamptime = 0;
                                	dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                	dctd.stageline = stInterGrateControlPhaseInfo.stageline;
                                	if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
                                	{
                                	#ifdef INTER_CONTROL_DEBUG
                                    	printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                	#endif
                                	}
                            	}
								#ifdef I_EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
								{
									memset(&dctd,0,sizeof(dctd));
                                    dctd.mode = 28;
                                    dctd.pattern = *(fcdata->patternid);
                                    if(0 == bAllLampClose)
                                	{
                                		dctd.lampcolor = 0x02;
                                	}
									else
									{
										dctd.lampcolor = 0x03;
									}
                                    dctd.lamptime = 0;
                                    dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                    dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
									if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#endif
								if(0 == bAllLampClose)
								{
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
		                                stInterGrateControlPhaseInfo.chan,0x02,fcdata->markbit))
		                            {
		                            #ifdef INTER_CONTROL_DEBUG 
		                                printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
		                            #endif
		                            }
								}
								else
								{
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
		                                stInterGrateControlPhaseInfo.chan,0x03,fcdata->markbit))
		                            {
		                            #ifdef INTER_CONTROL_DEBUG 
		                                printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
		                            #endif
		                            }
								}
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
                                {//report info to server actively
                                    memset(err,0,sizeof(err));
                                    gettimeofday(&ct,NULL);
                                    if (SUCCESS != err_report(err,ct.tv_sec,22,1))
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                    else
                                    { 
                                        write(*(fcdata->sockfd->csockfd),err,sizeof(err));
                                    }
									update_event_list(fcdata->ec,fcdata->el,1,27,ct.tv_sec,fcdata->markbit);
                                }//report info to server actively
								else
								{
									gettimeofday(&ct,NULL);
                                	update_event_list(fcdata->ec,fcdata->el,1,27,ct.tv_sec,fcdata->markbit);
								}
								continue;
							}//prepare to lock
							if (1 == keylock)
							{
							#ifdef INTER_CONTROL_DEBUG
                                printf("Prepare to unlock,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
								kstep = 0;
								cktem = 0;
								keylock = 0;
								dcred = 0;
								close = 0;
								if (1 == YellowFlashYes)
								{
									pthread_cancel(YellowFlashPid);
									pthread_join(YellowFlashPid,NULL);
									YellowFlashYes = 0;
								}
								if(1 == bAllLampClose)
								{
									friststartcontrol = 1;
								}
								dcecho[1] = 0x00;
								//tell face board that traffic control is canceled;
								if (*(fcdata->markbit2) & 0x0800)
                                {//comes from side door serial
                                    *(fcdata->markbit2) &= 0xf7ff;
                                    if (!wait_write_serial(*(fcdata->sdserial)))
                                    {
                                        if (write(*(fcdata->sdserial),dcecho,sizeof(dcecho)) < 0)
                                        {
                                        #ifdef INTER_CONTROL_DEBUG
                                            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                            *(fcdata->markbit) |= 0x0800;
                                        }
                                    }
                                    else
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                    #endif
                                    }
                                }//comes from side door serial
								else
								{
									if (!wait_write_serial(*(fcdata->fbserial)))
									{
										if (write(*(fcdata->fbserial),dcecho,sizeof(dcecho)) < 0)
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
											update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
											if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																			fcdata->softevent,fcdata->markbit))
											{
											#ifdef INTER_CONTROL_DEBUG
												printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
											}
										}
									}
									else
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
								}

								new_all_red(&ardata);
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									stStatuSinfo.conmode = 28;
									stStatuSinfo.color = 0x00;
									stStatuSinfo.time = 0;
									stStatuSinfo.stage = 0;
									stStatuSinfo.cyclet = 0;
									stStatuSinfo.phase = 0;
															
									stStatuSinfo.chans = 0;
            						memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            						csta = stStatuSinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						stStatuSinfo.chans += 1;
                						tcsta = i + 1;
                						tcsta <<= 2;
                						tcsta &= 0xfc;
                						*csta = tcsta;
                						csta++;
            						}
									memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            						{
            						#ifdef INTER_CONTROL_DEBUG
                						printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively
						//		all_red(*(fcdata->bbserial),0);
								*(fcdata->markbit2) &= 0xfffd;
								*(fcdata->contmode) = contmode;//restore control mode
								if (0 == InterGratedControlYes)
								{
									memset(&stInterGrateControlData,0,sizeof(stInterGrateControlData));
        							memset(&stInterGrateControlPhaseInfo,0,sizeof(stInterGrateControlPhaseInfo));
        							stInterGrateControlData.fd = fcdata;
        							stInterGrateControlData.td = tscdata;
        							stInterGrateControlData.pi = &stInterGrateControlPhaseInfo;
        							stInterGrateControlData.cs = chanstatus;
        							int dcret = pthread_create(&InterGratedControlPid,NULL, \
													(void *)Start_Inter_Grate_Control,&stInterGrateControlData);
        							if (0 != dcret)
        							{
        							#ifdef INTER_CONTROL_DEBUG
            							printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
										output_log("InterGrate control,create full InterGrate thread err");
        							#endif
										Inter_Control_End_Part_Child_Thread();
            							return FAIL;
        							}
									InterGratedControlYes = 1;
								}
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
                                {//report info to server actively
                                    memset(err,0,sizeof(err));
                                    gettimeofday(&ct,NULL);
                                    if (SUCCESS != err_report(err,ct.tv_sec,22,3))
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                    else
                                    {
                                        write(*(fcdata->sockfd->csockfd),err,sizeof(err));
                                    }
									update_event_list(fcdata->ec,fcdata->el,1,29,ct.tv_sec,fcdata->markbit);
                                }//report info to server actively
								else
								{
									gettimeofday(&ct,NULL);
                                	update_event_list(fcdata->ec,fcdata->el,1,29,ct.tv_sec,fcdata->markbit);
								}
								continue;
							}//prepare to unlock
						}//lock or unlock
						if (((0x11 <= tcbuf[1]) && (tcbuf[1] <= 0x18)) && (1 == keylock))	
						{//jump stage control
						#ifdef INTER_CONTROL_DEBUG
							printf("tcbuf[1]:%d,File:%s,Line:%d\n",tcbuf[1],__FILE__,__LINE__);
						#endif
							memset(&mdt,0,sizeof(markdata_c));
							mdt.redl = &dcred;
							mdt.closel = &close;
							mdt.rettl = &rettl;
							mdt.yfl = &YellowFlashYes;
							mdt.yfid = &YellowFlashPid;
							mdt.ardata = &ardata;
							mdt.fcdata = fcdata;
							mdt.tscdata = tscdata;
							mdt.chanstatus = chanstatus;
							mdt.sinfo = &stStatuSinfo;
							igc_jump_stage_control(&mdt,tcbuf[1],&stInterGrateControlPhaseInfo);	
						}//jump stage control
						if (((0x20 <= tcbuf[1]) && (tcbuf[1] <= 0x27)) && (1 == keylock))
                        {//direction control
                        #ifdef INTER_CONTROL_DEBUG
                            printf("tcbuf[1]:%d,cktem:%d,File:%s,Line:%d\n",tcbuf[1],cktem,__FILE__,__LINE__);
                        #endif
							if (cktem == tcbuf[1])
								continue;
							memset(&mdt,0,sizeof(markdata_c));
                            mdt.redl = &dcred;
                            mdt.closel = &close;
                            mdt.rettl = &rettl;
                            mdt.yfl = &YellowFlashYes;
                            mdt.yfid = &YellowFlashPid;
                            mdt.ardata = &ardata;
                            mdt.fcdata = fcdata;
                            mdt.tscdata = tscdata;
                            mdt.chanstatus = chanstatus;
                            mdt.sinfo = &stStatuSinfo;
							mdt.kstep = &kstep;
							igc_dirch_control(&mdt,cktem,tcbuf[1],dirch,&stInterGrateControlPhaseInfo);
							cktem = tcbuf[1];
                        }//direction control
						if ((0x02 == tcbuf[1]) && (1 == keylock))
						{//step by step
							kstep = 1;
							if (0 != cktem)
							{//front control is direction control
								memset(wcc,0,sizeof(wcc));
								memset(wnpcc,0,sizeof(wnpcc));
								memset(wpcc,0,sizeof(wpcc));
								z = 0;
								k = 0;
								s = 0;
								pce = 0;
								if(1 == bAllLampClose)
								{
									new_all_red(&ardata);
								}
								for (i = 0; i < MAX_CHANNEL; i++)
								{//for (i = 0; i < MAX_CHANNEL; i++)
									if (0 == dirch[cktem-0x20][i])
										break;
									ce = 0;
									for (j = 0; j < MAX_CHANNEL; j++)
									{//for (j = 0; j < MAX_CHANNEL; j++)
										if (0 == stInterGrateControlPhaseInfo.chan[j])
											break;
										if (dirch[cktem-0x20][i] == stInterGrateControlPhaseInfo.chan[j])
										{
											ce = 1;
											break;
										}
									}//for (j = 0; j < MAX_CHANNEL; j++)
									if (0 == ce)
									{
										wcc[z] = dirch[cktem-0x20][i];
										z++;
										if ((0x0d <= dirch[cktem-0x20][i]) && (dirch[cktem-0x20][i] <= 0x10))
										{
											wpcc[k] = dirch[cktem-0x20][i];
											k++;
											pce = 1;
										}
										else
										{
											wnpcc[s] = dirch[cktem-0x20][i];
											s++;
										}		
									}
								}//for (i = 0; i < MAX_CHANNEL; i++)
								
								if ((0==wcc[0])&&((stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0)))
								{
									if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
									{
										stStatuSinfo.conmode = 28;
										stStatuSinfo.color = 0x02;
										stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime + stInterGrateControlPhaseInfo.ytime + stInterGrateControlPhaseInfo.rtime;
										stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
										stStatuSinfo.cyclet = 0;
										stStatuSinfo.phase = 0;
										stStatuSinfo.chans = 0;
										memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
										csta = stStatuSinfo.csta;
										for (i = 0; i < MAX_CHANNEL; i++)
										{
											if (0 == dirch[cktem-0x20][i])
												break;
											stStatuSinfo.chans += 1;
											tcsta = dirch[cktem-0x20][i];
											tcsta <<= 2;
											tcsta &= 0xfc;
											tcsta |= 0x02; //00000010-green 
											*csta = tcsta;
											csta++;
										}
										memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
										memset(sibuf,0,sizeof(sibuf));
										if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										else
										{
											write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
										}
									}
								}//if ((0==wcc[0])&&((stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0)))
								if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.gftime > 0))	
								{			
									if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										stStatuSinfo.conmode = 28;
										stStatuSinfo.color = 0x03;
										stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime;
										stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
										stStatuSinfo.cyclet = 0;
										stStatuSinfo.phase = 0;
										for (i = 0; i < MAX_CHANNEL; i++)
										{
											if (0 == wcc[i])
												break;
											for (j = 0; j < stStatuSinfo.chans; j++)
											{
												if (0 == stStatuSinfo.csta[j])
													break;
												tcsta = stStatuSinfo.csta[j];
												tcsta &= 0xfc;
												tcsta >>= 2;
												tcsta &= 0x3f;
												if (wcc[i] == tcsta)
												{
													stStatuSinfo.csta[j] &= 0xfc;
													stStatuSinfo.csta[j] |= 0x03; //00000011-green flash
													break;
												}
											}
										}
										memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
										memset(sibuf,0,sizeof(sibuf));
										if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("status pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										else
										{
											write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
										}
									}//report info to server actively
								}//if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.gftime > 0))

								//green flash
								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
								{
									memset(&dctd,0,sizeof(dctd));
									dctd.mode = 28;//traffic control
									dctd.pattern = *(fcdata->patternid);
									dctd.lampcolor = 0x02;
									dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
									dctd.phaseid = 0;
									dctd.stageline = stInterGrateControlPhaseInfo.stageline;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef I_EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
								{
									memset(&dctd,0,sizeof(dctd));
                        			dctd.mode = 28;//traffic control
                        			dctd.pattern = *(fcdata->patternid);
                        			dctd.lampcolor = 0x02;
                        			dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
                        			dctd.phaseid = 0;
                        			dctd.stageline = stInterGrateControlPhaseInfo.stageline;	
									if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#endif
								fbdata[1] = 28;
								fbdata[2] = stInterGrateControlPhaseInfo.stageline;
								fbdata[3] = 0x02;
								fbdata[4] = stInterGrateControlPhaseInfo.gftime;
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
										if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																	fcdata->softevent,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
								}
								else
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("face board serial port cannot write,Line:%d\n",__LINE__);
								#endif
								}
								ngf = 0;
								if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.gftime > 0))
								{//if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.gftime > 0))
									while (1)
									{
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x03,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);
										
										ngf += 1;
										if (ngf >= stInterGrateControlPhaseInfo.gftime)
											break;
									}
								}//if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.gftime > 0))
								if (1 == pce)
								{
								//current phase begin to yellow lamp
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								else
								{
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}

								if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.ytime > 0))
								{
									if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										stStatuSinfo.conmode = 28;
										stStatuSinfo.color = 0x01;
										stStatuSinfo.time = stInterGrateControlPhaseInfo.ytime;
										stStatuSinfo.stage = 0;
										stStatuSinfo.cyclet = 0;
										stStatuSinfo.phase = 0;
														
										for (i = 0; i < MAX_CHANNEL; i++)
										{
											if (0 == wnpcc[i])
												break;
											for (j = 0; j < stStatuSinfo.chans; j++)
											{
												if (0 == stStatuSinfo.csta[j])
													break;
												tcsta = stStatuSinfo.csta[j];
												tcsta &= 0xfc;
												tcsta >>= 2;
												tcsta &= 0x3f;
												if (wnpcc[i] == tcsta)
												{
													stStatuSinfo.csta[j] &= 0xfc;
													stStatuSinfo.csta[j] |= 0x01; //00000001-yellow
													break;
												}
											}
										}
										for (i = 0; i < MAX_CHANNEL; i++)
										{
											if (0 == wpcc[i])
												break;
											for (j = 0; j < stStatuSinfo.chans; j++)
											{
												if (0 == stStatuSinfo.csta[j])
													break;
												tcsta = stStatuSinfo.csta[j];
												tcsta &= 0xfc;
												tcsta >>= 2;
												tcsta &= 0x3f;
												if (wpcc[i] == tcsta)
												{
													stStatuSinfo.csta[j] &= 0xfc;
													break;
												}
											}
										}
										memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
										memset(sibuf,0,sizeof(sibuf));
										if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("status pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										else
										{
											write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
										}
									}//report info to server actively
								}//if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.ytime > 0))

								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
								{
									memset(&dctd,0,sizeof(dctd));
									dctd.mode = 28;//traffic control
									dctd.pattern = *(fcdata->patternid);
									dctd.lampcolor = 0x01;
									dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
									dctd.phaseid = 0;
									dctd.stageline = stInterGrateControlPhaseInfo.stageline;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef I_EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
								{
									memset(&dctd,0,sizeof(dctd));
									dctd.mode = 28;//traffic control
									dctd.pattern = *(fcdata->patternid);
									dctd.lampcolor = 0x01;
									dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
									dctd.phaseid = 0;
									dctd.stageline = stInterGrateControlPhaseInfo.stageline;	
									if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#endif
								fbdata[1] = 28;
								fbdata[2] = stInterGrateControlPhaseInfo.stageline;
								fbdata[3] = 0x01;
								fbdata[4] = stInterGrateControlPhaseInfo.ytime;
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16, \
															ct.tv_sec,fcdata->markbit);
										if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																fcdata->softevent,fcdata->markbit))
										{	
										#ifdef INTER_CONTROL_DEBUG
											printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
								}
								else
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("face board serial port cannot write,Line:%d\n",__LINE__);
								#endif
								}
								sleep(stInterGrateControlPhaseInfo.ytime);

								//current phase begin to red lamp
								if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
									*(fcdata->markbit) |= 0x0800;
								}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																		wcc,0x00,fcdata->markbit))
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}

								if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.rtime > 0))
								{
									if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										stStatuSinfo.conmode = 28;
										stStatuSinfo.color = 0x00;
										stStatuSinfo.time = stInterGrateControlPhaseInfo.rtime;
										stStatuSinfo.stage = 0;
										stStatuSinfo.cyclet = 0;
										stStatuSinfo.phase = 0;
																	
										for (i = 0; i < MAX_CHANNEL; i++)
										{
											if (0 == wcc[i])
												break;
											for (j = 0; j < stStatuSinfo.chans; j++)
											{
												if (0 == stStatuSinfo.csta[j])
													break;
												tcsta = stStatuSinfo.csta[j];
												tcsta &= 0xfc;
												tcsta >>= 2;
												tcsta &= 0x3f;
												if (wcc[i] == tcsta)
												{
													stStatuSinfo.csta[j] &= 0xfc;
													break;
												}
											}
										}
										memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
										memset(sibuf,0,sizeof(sibuf));
										if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										else
										{
											write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
										}
									}//report info to server actively
								}//if ((0 != wcc[0]) && (stInterGrateControlPhaseInfo.rtime > 0))
								
								if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.chan,0x02))
                            	{
                            	#ifdef INTER_CONTROL_DEBUG
                                	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            	#endif
									*(fcdata->markbit) |= 0x0800;
                            	}
						
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																stInterGrateControlPhaseInfo.chan,0x02,fcdata->markbit))
                            	{
                            	#ifdef INTER_CONTROL_DEBUG
                                	printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            	#endif
                            	}

								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									stStatuSinfo.conmode = 28;
									stStatuSinfo.color = 0x02;
									stStatuSinfo.time = 0;
									stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
									stStatuSinfo.cyclet = 0;
									stStatuSinfo.phase = 0;
									stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
															
									stStatuSinfo.chans = 0;
            						memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            						csta = stStatuSinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
										if (0 == stInterGrateControlPhaseInfo.chan[i])
											break;
                						stStatuSinfo.chans += 1;
										tcsta = stInterGrateControlPhaseInfo.chan[i];
										tcsta <<= 2;
										tcsta &= 0xfc;
										tcsta |= 0x02;
										*csta = tcsta;
										csta++;
									}
									memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
									memset(sibuf,0,sizeof(sibuf));
									if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
									else
									{
										write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
									}
								}//report info to server actively
		
								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
								{
									memset(&dctd,0,sizeof(dctd));
									dctd.mode = 28;//traffic control
									dctd.pattern = *(fcdata->patternid);
									dctd.lampcolor = 0x02;
									dctd.lamptime = 0;
									dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
									dctd.stageline = stInterGrateControlPhaseInfo.stageline;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef I_EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
								{
									memset(&dctd,0,sizeof(dctd));
									dctd.mode = 28;//traffic control
									dctd.pattern = *(fcdata->patternid);
									dctd.lampcolor = 0x02;
									dctd.lamptime = 0;
									dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
									dctd.stageline = stInterGrateControlPhaseInfo.stageline;	
									if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#endif
								fbdata[1] = 28;
								fbdata[2] = stInterGrateControlPhaseInfo.stageline;
								fbdata[3] = 0x02;
								fbdata[4] = 0;
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
										if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																fcdata->softevent,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
								}
								else
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("face board serial port cannot write,Line:%d\n",__LINE__);
								#endif
								}							

								dcecho[1] = 0x02;
								if (*(fcdata->markbit2) & 0x1000)
								{
									*(fcdata->markbit2) &= 0xefff;
									if (!wait_write_serial(*(fcdata->sdserial)))
									{
										if (write(*(fcdata->sdserial),dcecho,sizeof(dcecho)) < 0)
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
									}
									else
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("face board serial port cannot write,Line:%d\n",__LINE__);
									#endif
									}	
								}//step by step comes from side door serial
								else
								{
									if (!wait_write_serial(*(fcdata->fbserial)))
									{
										if (write(*(fcdata->fbserial),dcecho,sizeof(dcecho)) < 0)
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
									}
									else
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("face board serial port cannot write,Line:%d\n",__LINE__);
									#endif
									}
								}							
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									memset(err,0,sizeof(err));
									gettimeofday(&ct,NULL);
									if (SUCCESS != err_report(err,ct.tv_sec,22,5))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
									else
									{
										write(*(fcdata->sockfd->csockfd),err,sizeof(err));
									}
									update_event_list(fcdata->ec,fcdata->el,1,31,ct.tv_sec,fcdata->markbit);
								}//report info to server actively
								else
								{
									gettimeofday(&ct,NULL);
									update_event_list(fcdata->ec,fcdata->el,1,31,ct.tv_sec,fcdata->markbit);
								}
								
								cktem = 0;
								continue;
							}//front control is direction control
							if ((1 == dcred) || (1 == YellowFlashYes) || (1 == close))
							{
								if (1 == close)
								{
									new_all_red(&ardata);
									close = 0;
								}
								dcred = 0;
								if (1 == YellowFlashYes)
                                {
                                    pthread_cancel(YellowFlashPid);
                                    pthread_join(YellowFlashPid,NULL);
                                    YellowFlashYes = 0;
									new_all_red(&ardata);
                                }
			//					#ifdef CLOSE_LAMP
                                igc_set_lamp_color(*(fcdata->bbserial),clch,0x03);
								update_channel_status(fcdata->sockfd,chanstatus,clch,0x03,fcdata->markbit);
            //                    #endif
								if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.chan,0x02))
								{
								#ifdef INTER_CONTROL_DEBUG
                                    printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                    gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                    *(fcdata->markbit) |= 0x0800;	
								}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    							stInterGrateControlPhaseInfo.chan,0x02,fcdata->markbit))
                                {
                                #ifdef INTER_CONTROL_DEBUG 
                                    printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
								dcecho[1] = 0x02;
								if (*(fcdata->markbit2) & 0x1000)
								{
									*(fcdata->markbit2) &= 0xefff;
									if (!wait_write_serial(*(fcdata->sdserial)))
									{
										if (write(*(fcdata->sdserial),dcecho,sizeof(dcecho)) < 0)
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
									}
									else
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("face board serial port cannot write,Line:%d\n",__LINE__);
									#endif
									}	
								}//step by step comes from side door serial
								else
								{
									if (!wait_write_serial(*(fcdata->fbserial)))
									{
										if (write(*(fcdata->fbserial),dcecho,sizeof(dcecho)) < 0)
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
									}
									else
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("face board serial port cannot write,Line:%d\n",__LINE__);
									#endif
									}
								}
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									stStatuSinfo.conmode = 28;
									stStatuSinfo.color = 0x02;
									stStatuSinfo.time = 0;
									stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
									stStatuSinfo.cyclet = 0;
									stStatuSinfo.phase = 0;
									stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
								/*						
									stStatuSinfo.chans = 0;
            						memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            						csta = stStatuSinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						stStatuSinfo.chans += 1;
                						tcsta = i + 1;
                						tcsta <<= 2;
                						tcsta &= 0xfc;
										for (j = 0; j < MAX_CHANNEL; j++)
										{
											if (0 == stInterGrateControlPhaseInfo.chan[j])
												break;
											if ((i+1) == stInterGrateControlPhaseInfo.chan[j])
											{
												tcsta |= 0x02;
												break;
											}
										}
                						*csta = tcsta;
                						csta++;
            						}
								*/
									stStatuSinfo.chans = 0;
									memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
                                    csta = stStatuSinfo.csta;
									for (i = 0; i < MAX_CHANNEL; i++)
									{
										if (0 == stInterGrateControlPhaseInfo.chan[i])
											break;
										stStatuSinfo.chans += 1;
										tcsta = stInterGrateControlPhaseInfo.chan[i];
										tcsta <<= 2;
										tcsta &= 0xfc;
										tcsta |= 0x02; //00000010-green 
										*csta = tcsta;
										csta++;
									}
									memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            						{
            						#ifdef INTER_CONTROL_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
                                    memset(err,0,sizeof(err));
                                    gettimeofday(&ct,NULL);
                                    if (SUCCESS != err_report(err,ct.tv_sec,22,5))
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                    else
                                    {
                                        write(*(fcdata->sockfd->csockfd),err,sizeof(err));
                                    }
									update_event_list(fcdata->ec,fcdata->el,1,31,ct.tv_sec,fcdata->markbit);
								}//report info to server actively
								else
								{
									gettimeofday(&ct,NULL);
                                	update_event_list(fcdata->ec,fcdata->el,1,31,ct.tv_sec,fcdata->markbit);
								}
								continue;
							}
#if 0
							//for WuXi check
							ct.tv_sec = 0;
                            ct.tv_usec = 0;
                            gettimeofday(&ct,NULL);
							if ((ct.tv_sec - wut.tv_sec) < stInterGrateControlPhaseInfo.mingtime)
							{
								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
								{
									memset(&dctd,0,sizeof(dctd));
									dctd.mode = 28;
									dctd.pattern = *(fcdata->patternid);
                                	dctd.lampcolor = 0x02;
                                	dctd.lamptime = stInterGrateControlPhaseInfo.mingtime - (ct.tv_sec - wut.tv_sec);
                                	dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                	dctd.stageline = stInterGrateControlPhaseInfo.stageline;	
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								fbdata[2] = stInterGrateControlPhaseInfo.stageline;
								fbdata[3] = 2;
                            	fbdata[4] = stInterGrateControlPhaseInfo.mingtime - (ct.tv_sec - wut.tv_sec);
                            	if (!wait_write_serial(*(fcdata->fbserial)))
                            	{
                                	if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                	{
                                	#ifdef INTER_CONTROL_DEBUG
                                    	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
										*(fcdata->markbit) |= 0x0800;
                                    	gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    	if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef INTER_CONTROL_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                	}
                            	}
                            	else
                            	{
                            	#ifdef INTER_CONTROL_DEBUG
                                	printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            	#endif
                            	}
								sleep(stInterGrateControlPhaseInfo.mingtime - (ct.tv_sec - wut.tv_sec));
							}
							//for WuXi Check
#endif
							if ((0==stInterGrateControlPhaseInfo.cchan[0])&&(stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0))
							{
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{
									stStatuSinfo.conmode = 28;
                                    stStatuSinfo.color = 0x02;
                                    stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime + stInterGrateControlPhaseInfo.ytime + stInterGrateControlPhaseInfo.rtime;
                                    stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
                                    stStatuSinfo.cyclet = 0;
                                    stStatuSinfo.phase = 0;
                                    stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));

                                    stStatuSinfo.chans = 0;
                                    memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
                                    csta = stStatuSinfo.csta;
									for (i = 0; i < MAX_CHANNEL; i++)
                                    {
                                        if (0 == stInterGrateControlPhaseInfo.chan[i])
                                            break;
                                        stStatuSinfo.chans += 1;
                                        tcsta = stInterGrateControlPhaseInfo.chan[i];
                                        tcsta <<= 2;
                                        tcsta &= 0xfc;
                                        tcsta |= 0x02;
                                        *csta = tcsta;
                                        csta++;
                                    }
									memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
                                    memset(sibuf,0,sizeof(sibuf));
                                    if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
                                    {
                                    #ifdef PERSON_DEBUG
                                        printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                    else
                                    {
                                        write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
                                    }
								}
							}
							if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.gftime > 0))
							{
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									stStatuSinfo.conmode = 28;
									stStatuSinfo.color = 0x03;
									stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime;
									stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
									stStatuSinfo.cyclet = 0;
									stStatuSinfo.phase = 0;
									stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
									
									for (i = 0; i < MAX_CHANNEL; i++)
                                	{
                                		if (0 == stInterGrateControlPhaseInfo.cchan[i])
                                        	break;
                                    	for (j = 0; j < stStatuSinfo.chans; j++)
                                    	{
                                        	if (0 == stStatuSinfo.csta[j])
                                            	break;
                                        	tcsta = stStatuSinfo.csta[j];
                                        	tcsta &= 0xfc;
                                        	tcsta >>= 2;
                                        	tcsta &= 0x3f;
                                        	if (stInterGrateControlPhaseInfo.cchan[i] == tcsta)
                                        	{
                                            	stStatuSinfo.csta[j] &= 0xfc;
                                           		stStatuSinfo.csta[j] |= 0x03; //00000001-green flash
												break;
                                        	}
                                    	}
                                	}								
									memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            						{
            						#ifdef INTER_CONTROL_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively
							}

							//send down time to configure tool
							if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
							{
								memset(&dctd,0,sizeof(dctd));
								dctd.mode = 28;
								dctd.pattern = *(fcdata->patternid);
                                dctd.lampcolor = 0x02;
                                dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
                                dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                dctd.stageline = stInterGrateControlPhaseInfo.stageline;	
								if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#ifdef I_EMBED_CONFIGURE_TOOL
                            if (*(fcdata->markbit2) & 0x0200)
                            {
                                memset(&dctd,0,sizeof(dctd));
                                dctd.mode = 28;
                                dctd.pattern = *(fcdata->patternid);
                                dctd.lampcolor = 0x02;
                                dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
                                dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                dctd.stageline = stInterGrateControlPhaseInfo.stageline;   
                                if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
                            }
                            #endif
							fbdata[1] = 28;
							fbdata[2] = stInterGrateControlPhaseInfo.stageline;
							fbdata[3] = 2;
                            fbdata[4] = stInterGrateControlPhaseInfo.gftime;
                            if (!wait_write_serial(*(fcdata->fbserial)))
                            {
                                if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									*(fcdata->markbit) |= 0x0800;
                                    gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                            }
                            else
                            {
                            #ifdef INTER_CONTROL_DEBUG
                                printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }
							if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
                            {//report info to server actively
                                memset(err,0,sizeof(err));
                                gettimeofday(&ct,NULL);
                                if (SUCCESS != err_report(err,ct.tv_sec,22,5))
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
                                else
                                {
                                    write(*(fcdata->sockfd->csockfd),err,sizeof(err));
                                }
								update_event_list(fcdata->ec,fcdata->el,1,31,ct.tv_sec,fcdata->markbit);
                            }//report info to server actively
							else
							{
								gettimeofday(&ct,NULL);
                                update_event_list(fcdata->ec,fcdata->el,1,31,ct.tv_sec,fcdata->markbit);
							}
							//begin to green flash
							if (stInterGrateControlPhaseInfo.gftime > 0)
							{
								ngf = 0;
								while (1)
								{//green flash loop
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cchan,0x03))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
										if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
										stInterGrateControlPhaseInfo.cchan,0x03,fcdata->markbit))
									{
									#ifdef INTER_CONTROL_DEBUG 
										printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
									to.tv_sec = 0;
									to.tv_usec = 500000;
									select(0,NULL,NULL,NULL,&to);
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cchan,0x02))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
										if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
										stInterGrateControlPhaseInfo.cchan,0x02,fcdata->markbit))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
									to.tv_sec = 0;
									to.tv_usec = 500000;
									select(0,NULL,NULL,NULL,&to);

									ngf += 1;
									if (ngf >= stInterGrateControlPhaseInfo.gftime)
										break;
								}//green flash loop
							}//if (stInterGrateControlPhaseInfo.gftime > 0)
							//end green flash

							//yellow lamp 
							if (1 == stInterGrateControlPhaseInfo.cpcexist)
							{//have person channels
								if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cnpchan,0x01))
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									gettimeofday(&ct,NULL);
                                   	update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                   	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                   	{
                                   	#ifdef INTER_CONTROL_DEBUG
                                       	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   	#endif
                                   	}
									*(fcdata->markbit) |= 0x0800;
								}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															stInterGrateControlPhaseInfo.cnpchan,0x01,fcdata->markbit))
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
								if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cpchan,0x00))
                               	{
                               	#ifdef INTER_CONTROL_DEBUG
                                   	printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               	#endif
									gettimeofday(&ct,NULL);
                                   	update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                   	if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                   	{
                                   	#ifdef INTER_CONTROL_DEBUG
                                       	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   	#endif
                                   	}
									*(fcdata->markbit) |= 0x0800;
                               	}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    							stInterGrateControlPhaseInfo.cpchan,0x00,fcdata->markbit))
                               	{
                               	#ifdef INTER_CONTROL_DEBUG
                                   	printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               	#endif
                               	}	
							}//have person channels
							else
							{//not have person channels
								if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cchan,0x01))
                               	{
                               	#ifdef INTER_CONTROL_DEBUG
                                   	printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               	#endif
									gettimeofday(&ct,NULL);
                                   	update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                   	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                   	{
                                   	#ifdef INTER_CONTROL_DEBUG
                                       	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   	#endif
                                   	}
									*(fcdata->markbit) |= 0x0800;
                               	}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                   									stInterGrateControlPhaseInfo.cchan,0x01,fcdata->markbit))
                               	{
                               	#ifdef INTER_CONTROL_DEBUG
                                   	printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               	#endif
                               	}
							}//not have person channels
							if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.ytime > 0))
							{
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									stStatuSinfo.conmode = 28;
									stStatuSinfo.color = 0x01;
									stStatuSinfo.time = stInterGrateControlPhaseInfo.ytime;
									stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
									stStatuSinfo.cyclet = 0;
									stStatuSinfo.phase = 0;
									stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
															
									for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						if (0 == stInterGrateControlPhaseInfo.cnpchan[i])
                    						break;
                						for (j = 0; j < stStatuSinfo.chans; j++)
                						{
                    						if (0 == stStatuSinfo.csta[j])
                        						break;
                    						tcsta = stStatuSinfo.csta[j];
                    						tcsta &= 0xfc;
                    						tcsta >>= 2;
                    						tcsta &= 0x3f;
                    						if (stInterGrateControlPhaseInfo.cnpchan[i] == tcsta)
                    						{
                        						stStatuSinfo.csta[j] &= 0xfc;
												stStatuSinfo.csta[j] |= 0x01; //00000001-yellow
												break;
                    						}
                						}
            						}
									for (i = 0; i < MAX_CHANNEL; i++)
                                	{
                                		if (0 == stInterGrateControlPhaseInfo.cpchan[i])
                                        	break;
                                    	for (j = 0; j < stStatuSinfo.chans; j++)
                                    	{
                                        	if (0 == stStatuSinfo.csta[j])
                                            	break;
                                        	tcsta = stStatuSinfo.csta[j];
                                        	tcsta &= 0xfc;
                                        	tcsta >>= 2;
                                        	tcsta &= 0x3f;
                                        	if (stInterGrateControlPhaseInfo.cpchan[i] == tcsta)
                                        	{
                                            	stStatuSinfo.csta[j] &= 0xfc;
												break;
                                        	}
                                    	}
                                	}
									memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            						{
            						#ifdef INTER_CONTROL_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively
							}

							if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            {
                               	memset(&dctd,0,sizeof(dctd));
                               	dctd.mode = 28;//traffic control
                               	dctd.pattern = *(fcdata->patternid);
                               	dctd.lampcolor = 0x01;
                               	dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                               	dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                               	dctd.stageline = stInterGrateControlPhaseInfo.stageline;
                               	if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
                               	{
                               	#ifdef INTER_CONTROL_DEBUG
                                   	printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               	#endif
                               	}
                            }
							#ifdef I_EMBED_CONFIGURE_TOOL
                            if (*(fcdata->markbit2) & 0x0200)
                            {
                                memset(&dctd,0,sizeof(dctd));
                                dctd.mode = 28;//traffic control
                                dctd.pattern = *(fcdata->patternid);
                                dctd.lampcolor = 0x01;
                                dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                                dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                dctd.stageline = stInterGrateControlPhaseInfo.stageline;  
                                if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
                            }
                            #endif
							fbdata[1] = 28;
							fbdata[2] = stInterGrateControlPhaseInfo.stageline;
							fbdata[3] = 1;
                            fbdata[4] = stInterGrateControlPhaseInfo.ytime;
                            if (!wait_write_serial(*(fcdata->fbserial)))
                            {
                               	if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               	{
                               	#ifdef INTER_CONTROL_DEBUG
                                   	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               	#endif
									*(fcdata->markbit) |= 0x0800;
                                   	gettimeofday(&ct,NULL);
                                   	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                   	if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                   	{
                                   	#ifdef INTER_CONTROL_DEBUG
                                       	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   	#endif
                                   	}
                               	}
                            }
                            else
                            {
                            #ifdef INTER_CONTROL_DEBUG
                               	printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }
							sleep(stInterGrateControlPhaseInfo.ytime);
							//end yellow lamp

							//red lamp
							if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cchan,0x00))
							{
							#ifdef INTER_CONTROL_DEBUG
								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								gettimeofday(&ct,NULL);
                                update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																fcdata->softevent,fcdata->markbit))
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
								*(fcdata->markbit) |= 0x0800;
							}
							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus,stInterGrateControlPhaseInfo.cchan,\
										0x00,fcdata->markbit))
							{
							#ifdef INTER_CONTROL_DEBUG
								printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}
							if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.rtime > 0))
							{
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									stStatuSinfo.conmode = 28;
									stStatuSinfo.color = 0x00;
									stStatuSinfo.time = stInterGrateControlPhaseInfo.rtime;
									stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
									stStatuSinfo.cyclet = 0;
									stStatuSinfo.phase = 0;
									stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
															
									for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						if (0 == stInterGrateControlPhaseInfo.cchan[i])
                    						break;
                						for (j = 0; j < stStatuSinfo.chans; j++)
                						{
                    						if (0 == stStatuSinfo.csta[j])
                        						break;
                    						tcsta = stStatuSinfo.csta[j];
                    						tcsta &= 0xfc;
                    						tcsta >>= 2;
                    						tcsta &= 0x3f;
                    						if (stInterGrateControlPhaseInfo.cchan[i] == tcsta)
                    						{
                        						stStatuSinfo.csta[j] &= 0xfc;
												break;
                    						}
                						}
            						}
									memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            						{
            						#ifdef INTER_CONTROL_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively
							}

							if (stInterGrateControlPhaseInfo.rtime > 0)
							{
								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                                {
                                    memset(&dctd,0,sizeof(dctd));
                                    dctd.mode = 28;//traffic control
                                    dctd.pattern = *(fcdata->patternid);
                                    dctd.lampcolor = 0x00;
                                    dctd.lamptime = stInterGrateControlPhaseInfo.rtime;
                                    dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                    dctd.stageline = stInterGrateControlPhaseInfo.stageline;
                                    if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
								#ifdef I_EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
								{
									memset(&dctd,0,sizeof(dctd));
                                    dctd.mode = 28;//traffic control
                                    dctd.pattern = *(fcdata->patternid);
                                    dctd.lampcolor = 0x00;
                                    dctd.lamptime = stInterGrateControlPhaseInfo.rtime;
                                    dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                    dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
									if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#endif
								fbdata[1] = 28;
								fbdata[2] = stInterGrateControlPhaseInfo.stageline;
								fbdata[3] = 0;
                            	fbdata[4] = stInterGrateControlPhaseInfo.rtime;
                            	if (!wait_write_serial(*(fcdata->fbserial)))
                            	{
                                	if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                	{
                                	#ifdef INTER_CONTROL_DEBUG
                                    	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
										*(fcdata->markbit) |= 0x0800;
                                    	gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    	if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef INTER_CONTROL_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                	}
                            	}
                            	else
                            	{
                            	#ifdef INTER_CONTROL_DEBUG
                                	printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            	#endif
                            	}
                                sleep(stInterGrateControlPhaseInfo.rtime);
							}	
							//end red lamp
							*(fcdata->slnum) += 1;
                            *(fcdata->stageid) = \
                                tscdata->timeconfig->TimeConfigList[rettl][*(fcdata->slnum)].StageId;
                            if (0 == (tscdata->timeconfig->TimeConfigList[rettl][*(fcdata->slnum)].StageId))
                            {
                                *(fcdata->slnum) = 0;
                                *(fcdata->stageid) = \
                                    tscdata->timeconfig->TimeConfigList[rettl][*(fcdata->slnum)].StageId;
                            }
							//get info of next phase
							memset(&stInterGrateControlPhaseInfo,0,sizeof(stInterGrateControlPhaseInfo));
							if (SUCCESS != igc_get_phase_info(fcdata,tscdata,rettl,*(fcdata->slnum),&stInterGrateControlPhaseInfo))
							{
							#ifdef INTER_CONTROL_DEBUG
								printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
								output_log("InterGrate control,get phase info err");
							#endif
								Inter_Control_End_Part_Child_Thread();
								return FAIL;
							}
							*(fcdata->phaseid) = 0;
							*(fcdata->phaseid) |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
							if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.chan,0x02))
                            {
                            #ifdef INTER_CONTROL_DEBUG
                                printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
								gettimeofday(&ct,NULL);
                                update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																fcdata->softevent,fcdata->markbit))
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
								*(fcdata->markbit) |= 0x0800;
                            }
							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus,stInterGrateControlPhaseInfo.chan,\
                                        0x02,fcdata->markbit))
                            {
                            #ifdef INTER_CONTROL_DEBUG
                                printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }

							if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
							{//report info to server actively
								stStatuSinfo.conmode = 28;
								stStatuSinfo.color = 0x02;
								stStatuSinfo.time = 0;
								stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
								stStatuSinfo.cyclet = 0;
								stStatuSinfo.phase = 0;
								stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
															
								stStatuSinfo.chans = 0;
            					memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            					csta = stStatuSinfo.csta;
            					for (i = 0; i < MAX_CHANNEL; i++)
            					{
									if (0 == stInterGrateControlPhaseInfo.chan[i])
										break;
                					stStatuSinfo.chans += 1;
                					tcsta = stInterGrateControlPhaseInfo.chan[i];
                					tcsta <<= 2;
                					tcsta &= 0xfc;
									tcsta |= 0x02;
                					*csta = tcsta;
                					csta++;
            					}
								memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            					memset(sibuf,0,sizeof(sibuf));
            					if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            					{
            					#ifdef INTER_CONTROL_DEBUG
                					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            					#endif
            					}
            					else
            					{
                					write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            					}
							}//report info to server actively

							if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
							{
								memset(&dctd,0,sizeof(dctd));
								dctd.mode = 28;
								dctd.pattern = *(fcdata->patternid);
                                dctd.lampcolor = 0x02;
                                dctd.lamptime = 0;
                                dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                dctd.stageline = stInterGrateControlPhaseInfo.stageline;	
								if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#ifdef I_EMBED_CONFIGURE_TOOL
                            if (*(fcdata->markbit2) & 0x0200)
                            {
                                memset(&dctd,0,sizeof(dctd));
                                dctd.mode = 28;
                                dctd.pattern = *(fcdata->patternid);
                                dctd.lampcolor = 0x02;
                                dctd.lamptime = 0;
                                dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                dctd.stageline = stInterGrateControlPhaseInfo.stageline;  
                                if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
                            }
                            #endif
							fbdata[1] = 28;
							fbdata[2] = stInterGrateControlPhaseInfo.stageline;
							fbdata[3] = 2;
                            fbdata[4] = 0;
                            if (!wait_write_serial(*(fcdata->fbserial)))
                            {
                                if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									*(fcdata->markbit) |= 0x0800;
                                    gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                            }
                            else
                            {
                            #ifdef INTER_CONTROL_DEBUG
                                printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }

                            dcecho[1] = 0x02;
							if (*(fcdata->markbit2) & 0x1000)
							{
								*(fcdata->markbit2) &= 0xefff;
								if (!wait_write_serial(*(fcdata->sdserial)))
                                {
                                    if (write(*(fcdata->sdserial),dcecho,sizeof(dcecho)) < 0)
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                        *(fcdata->markbit) |= 0x0800;
                                    }
                                }
                                else
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                #endif
                                }	
							}//step by step comes from side door serial
							else
							{
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),dcecho,sizeof(dcecho)) < 0)
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
										if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										*(fcdata->markbit) |= 0x0800;
									}
								}
								else
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("face board serial port cannot write,Line:%d\n",__LINE__);
								#endif
								}
							}

							continue;
						}//step by step
						if ((0x03 == tcbuf[1]) && (1 == keylock))
						{//yellow flash
							kstep = 0;
                            cktem = 0;
							dcred = 0;
							close = 0;
							if (0 == YellowFlashYes)
							{
								memset(&yfdata,0,sizeof(yfdata));
								yfdata.second = 0;
								yfdata.markbit = fcdata->markbit;
								yfdata.markbit2 = fcdata->markbit2;
								yfdata.serial = *(fcdata->bbserial);
								yfdata.sockfd = fcdata->sockfd;
								yfdata.cs = chanstatus;
#ifdef FLASH_DEBUG
								char szInfo[32] = {0};
								char szInfoT[64] = {0};
								snprintf(szInfo,sizeof(szInfo)-1,"%s",__FUNCTION__);
								snprintf(szInfoT,sizeof(szInfoT)-1,"%d",__LINE__);
								tsc_save_eventlog(szInfo,szInfoT);
#endif
								int yfret = pthread_create(&YellowFlashPid,NULL,(void *)Inter_Control_Yellow_Flash,&yfdata);
								if (0 != yfret)
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
									output_log("InterGrate control,create yellow flash err");
								#endif
									Inter_Control_End_Part_Child_Thread();
									return FAIL;
								}
								YellowFlashYes = 1;
							}
							dcecho[1] = 0x03;
							if (*(fcdata->markbit2) & 0x2000)
							{
								*(fcdata->markbit2) &= 0xdfff;
								if (!wait_write_serial(*(fcdata->sdserial)))
                                {
                                    if (write(*(fcdata->sdserial),dcecho,sizeof(dcecho)) < 0)
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                        *(fcdata->markbit) |= 0x0800;
                                    }
                                }
                                else
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                #endif
                                }
							}//yellow flash comes from side door serial;
							else
							{
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),dcecho,sizeof(dcecho)) < 0)
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
										if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										*(fcdata->markbit) |= 0x0800;
									}
								}
								else
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("face serial cannot write,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}

							if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
							{//report info to server actively
								stStatuSinfo.conmode = 28;
								stStatuSinfo.color = 0x05;
								stStatuSinfo.time = 0;
								stStatuSinfo.stage = 0;
								stStatuSinfo.cyclet = 0;
								stStatuSinfo.phase = 0;
															
								stStatuSinfo.chans = 0;
            					memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            					csta = stStatuSinfo.csta;
            					for (i = 0; i < MAX_CHANNEL; i++)
            					{
                					stStatuSinfo.chans += 1;
                					tcsta = i + 1;
                					tcsta <<= 2;
                					tcsta &= 0xfc;
                					*csta = tcsta;
                					csta++;
            					}
								memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            					memset(sibuf,0,sizeof(sibuf));
            					if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            					{
            					#ifdef INTER_CONTROL_DEBUG
                					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            					#endif
            					}
            					else
            					{
                					write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            					}
                                memset(err,0,sizeof(err));
                                gettimeofday(&ct,NULL);
                                if (SUCCESS != err_report(err,ct.tv_sec,22,7))
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
                                else
                                {
                                    write(*(fcdata->sockfd->csockfd),err,sizeof(err));
                                }
								update_event_list(fcdata->ec,fcdata->el,1,33,ct.tv_sec,fcdata->markbit);
							}//report info to server actively
							else
							{
								gettimeofday(&ct,NULL);
                            	update_event_list(fcdata->ec,fcdata->el,1,33,ct.tv_sec,fcdata->markbit);
							}
							continue;
						}//yellow flash
						if ((0x04 == tcbuf[1]) && (1 == keylock))
						{//all red
							kstep = 0;
                            cktem = 0;
							if (1 == YellowFlashYes)
							{
								pthread_cancel(YellowFlashPid);
								pthread_join(YellowFlashPid,NULL);
								YellowFlashYes = 0;
							}
							close = 0;
							if (0 == dcred)
							{
								new_all_red(&ardata);	
								dcred = 1;
							}
							dcecho[1] = 0x04;
							if (*(fcdata->markbit2) & 0x4000)
                            {
                                *(fcdata->markbit2) &= 0xbfff;
                                if (!wait_write_serial(*(fcdata->sdserial)))
                                {
                                    if (write(*(fcdata->sdserial),dcecho,sizeof(dcecho)) < 0)
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                        *(fcdata->markbit) |= 0x0800;
                                    }
                                }
                                else
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                #endif
                                }
                            }//all red comes from side door serial
							else
							{
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),dcecho,sizeof(dcecho)) < 0)
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
										if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										*(fcdata->markbit) |= 0x0800;
									}
								}
								else
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("face serial cannot write,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}

							if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
							{//report info to server actively
								stStatuSinfo.conmode = 28;
								stStatuSinfo.color = 0x00;
								stStatuSinfo.time = 0;
								stStatuSinfo.stage = 0;
								stStatuSinfo.cyclet = 0;
								stStatuSinfo.phase = 0;
															
								stStatuSinfo.chans = 0;
            					memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            					csta = stStatuSinfo.csta;
            					for (i = 0; i < MAX_CHANNEL; i++)
            					{
                					stStatuSinfo.chans += 1;
                					tcsta = i + 1;
                					tcsta <<= 2;
                					tcsta &= 0xfc;
                					*csta = tcsta;
                					csta++;
            					}
								memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            					memset(sibuf,0,sizeof(sibuf));
            					if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            					{
            					#ifdef INTER_CONTROL_DEBUG
                					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            					#endif
            					}
            					else
            					{
                					write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            					}
								memset(err,0,sizeof(err));
                                gettimeofday(&ct,NULL);
                                if (SUCCESS != err_report(err,ct.tv_sec,22,9))
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
                                else
                                {
                                    write(*(fcdata->sockfd->csockfd),err,sizeof(err));
                                }
								update_event_list(fcdata->ec,fcdata->el,1,35,ct.tv_sec,fcdata->markbit);
							}//report info to server actively
							else
							{
								gettimeofday(&ct,NULL);
                            	update_event_list(fcdata->ec,fcdata->el,1,35,ct.tv_sec,fcdata->markbit);
							}
							continue;
						}//all red
						if ((0x05 == tcbuf[1]) && (1 == keylock))
						{//all close
							kstep = 0;
                            cktem = 0;
							if (1 == YellowFlashYes)
							{
								pthread_cancel(YellowFlashPid);
								pthread_join(YellowFlashPid,NULL);
								YellowFlashYes = 0;
							}
							dcred = 0;
							if (0 == close)
							{
								new_all_close(&acdata);	
								close = 1;
							}
							dcecho[1] = 0x05;
							if (!wait_write_serial(*(fcdata->fbserial)))
							{
								if (write(*(fcdata->fbserial),dcecho,sizeof(dcecho)) < 0)
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
									*(fcdata->markbit) |= 0x0800;
								}
							}
							else
							{
							#ifdef INTER_CONTROL_DEBUG
								printf("face board serial cannot write,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
							if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
							{//report info to server actively
								stStatuSinfo.conmode = 28;
								stStatuSinfo.color = 0x04;
								stStatuSinfo.time = 0;
								stStatuSinfo.stage = 0;
								stStatuSinfo.cyclet = 0;
								stStatuSinfo.phase = 0;
															
								stStatuSinfo.chans = 0;
            					memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            					csta = stStatuSinfo.csta;
            					for (i = 0; i < MAX_CHANNEL; i++)
            					{
                					stStatuSinfo.chans += 1;
                					tcsta = i + 1;
                					tcsta <<= 2;
                					tcsta &= 0xfc;
                					*csta = tcsta;
                					csta++;
            					}
								memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            					memset(sibuf,0,sizeof(sibuf));
            					if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            					{
            					#ifdef INTER_CONTROL_DEBUG
                					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            					#endif
            					}
            					else
            					{
                					write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            					}
							#if 0
								memset(err,0,sizeof(err));
                                gettimeofday(&ct,NULL);
                                if (SUCCESS != err_report(err,ct.tv_sec,22,9))
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
                                else
                                {
                                    write(*(fcdata->sockfd->csockfd),err,sizeof(err));
                                }
								update_event_list(fcdata->ec,fcdata->el,1,35,ct.tv_sec,fcdata->markbit);
							#endif
							}//report info to server actively
							else
							{
							#if 0
								gettimeofday(&ct,NULL);
                            	update_event_list(fcdata->ec,fcdata->el,1,35,ct.tv_sec,fcdata->markbit);
							#endif
							}
							continue;
						}//all close
					}//key lock or unlock is suessfully only when wireless lock or unlock is not valid;
				}//key traffic control
				else if (!strncmp("WLTC1",tcbuf,5))
				{//wireless terminal traffic control
					if ((0 == keylock) && (0 == netlock) && (0 == wtlock))
                    {//wireless lock or unlock is suessfully only when key lock or unlock is not valid;
						unsigned char           wlbuf[5] = {0};
                        unsigned char           s = 0;
                        for (s = 5; ;s++)
                        {
                            if (('E' == tcbuf[s]) && ('N' == tcbuf[s+1]) && ('D' == tcbuf[s+2]))
                                break;
                            if ('\0' == tcbuf[s])
                                break;
                            if ((s - 5) > 4)
                                break;
                            wlbuf[s - 5] = tcbuf[s];
                        }
                        #ifdef INTER_CONTROL_DEBUG
                        printf("************wlbuf: %s,File: %s,Line: %d\n",wlbuf,__FILE__,__LINE__);
                        #endif
						if (!strcmp("SOCK",wlbuf))
						{//lock or unlock
							if (0 == wllock)
							{//prepare to lock
							#ifdef INTER_CONTROL_DEBUG
								printf("Prepare to lock,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								Get_Inter_Greate_Control_Status(&color,&leatime);
								if (2 != color)
								{//lamp color is not green
									struct timeval			spatime;
									unsigned char			endlock = 0;
									while(1)
									{//inline while loop
										spatime.tv_sec = 0;
										spatime.tv_usec = 200000;
										select(0,NULL,NULL,NULL,&spatime);
										memset(tcbuf,0,sizeof(tcbuf));
										read(*(fcdata->conpipe),tcbuf,sizeof(tcbuf));
									
										if (!strncmp("WLTC1",tcbuf,5))
                                        {
                                            memset(wlbuf,0,sizeof(wlbuf));
                                            s = 0;
                                            for (s = 5; ;s++)
                                            {
                                                if(('E'==tcbuf[s]) && ('N'==tcbuf[s+1]) && ('D'==tcbuf[s+2]))
                                                    break;
                                                if ('\0' == tcbuf[s])
                                                    break;
                                                if ((s - 5) > 4)
                                                    break;
                                                wlbuf[s - 5] = tcbuf[s];
                                            }
                                            if (!strcmp("SOCK",wlbuf))
                                            {
                                                endlock = 1;
                                                break;
                                            }
                                        }	
	
										Get_Inter_Greate_Control_Status(&color,&leatime);
										if (2 == color)
										{
											break;
										}
										else
										{
											continue;
										}
									}//inline while loop
									if (1 == endlock)
									{
										continue;
									}
								}//lamp color is not green
								
								wllock = 1;
								dcred = 0;
								close = 0;
								dircon = 0;
                                firdc = 1;
                                fdirn = 0;
                                cdirn = 0;
								Inter_Control_End_Child_Thread();//end man thread and its child thread
								*(fcdata->markbit2) |= 0x0010;
							#if 0
								new_all_red(&ardata);
							//	all_red(*(fcdata->bbserial),0);	
								if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.chan,0x02))
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									gettimeofday(&ct,NULL);
                					update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                					if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                					{
                					#ifdef INTER_CONTROL_DEBUG
                    					printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                					#endif
                					}
									*(fcdata->markbit) |= 0x0800;
								}
							#endif
						
								if (*(fcdata->auxfunc) & 0x01)
                                {//if (*(fcdata->auxfunc) & 0x01)
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),dcdownti,sizeof(dcdownti)) < 0)
                                        {
                                        #ifdef INTER_CONTROL_DEBUG
                                            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),dcedownti,sizeof(dcedownti)) < 0)
                                        {
                                        #ifdef INTER_CONTROL_DEBUG
                                            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (*(fcdata->auxfunc) & 0x01)
	
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									stStatuSinfo.conmode = 28;
									if(0 == bAllLampClose)
									{
										stStatuSinfo.color = 0x02;
									}
									else
									{
										stStatuSinfo.color = 0x04;
									}
									//stStatuSinfo.color = 0x02;
									stStatuSinfo.time = 0;
									stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
									stStatuSinfo.cyclet = 0;
									stStatuSinfo.phase = 0;
									stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
															
									stStatuSinfo.chans = 0;
            						memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            						csta = stStatuSinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						if (0 == stInterGrateControlPhaseInfo.chan[i])
                    						break;
                						stStatuSinfo.chans += 1;
                						tcsta = stInterGrateControlPhaseInfo.chan[i];
                						tcsta <<= 2;
                						tcsta &= 0xfc;
										if(0 == bAllLampClose)
										{
											tcsta |= 0x02; //00000010-green
										}
                						
                						*csta = tcsta;
                						csta++;
            						}
									memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            						{
            						#ifdef INTER_CONTROL_DEBUG
                						printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively
							
								*(fcdata->contmode) = 28;//traffic control mode
								//send current control info to face board
								fbdata[1] = 28;
								fbdata[2] = stInterGrateControlPhaseInfo.stageline;
								fbdata[3] = 2;
								fbdata[4] = 0;
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    	if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef INTER_CONTROL_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
									}
								}
								else
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
								if(0 == bAllLampClose)
								{
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
	                                    stInterGrateControlPhaseInfo.chan,0x02,fcdata->markbit))
	                                {
	                                #ifdef INTER_CONTROL_DEBUG 
	                                    printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
	                                #endif
	                                }
								}
								else
								{
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
	                                    stInterGrateControlPhaseInfo.chan,0x03,fcdata->markbit))
	                                {
	                                #ifdef INTER_CONTROL_DEBUG 
	                                    printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
	                                #endif
	                                }
								}
								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            	{
                                	memset(wltc,0,sizeof(wltc));
                                	strcpy(wltc,"SOCKBEGIN");
                                	write(*(fcdata->sockfd->ssockfd),wltc,sizeof(wltc));
									//send down time to configure tool
									memset(&dctd,0,sizeof(dctd));
									dctd.mode = 28;
									dctd.pattern = *(fcdata->patternid);
									if(0 == bAllLampClose)
									{
										dctd.lampcolor = 0x02;
									}
									else
									{
										dctd.lampcolor = 0x03;
									}
									dctd.lamptime = 0;
									dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
									dctd.stageline = stInterGrateControlPhaseInfo.stageline;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
									{ 
									#ifdef INTER_CONTROL_DEBUG
										printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
                            	}
								#ifdef I_EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
								{
									memset(&dctd,0,sizeof(dctd));
                                    dctd.mode = 28;
                                    dctd.pattern = *(fcdata->patternid);
                                    if(0 == bAllLampClose)
									{
										dctd.lampcolor = 0x02;
									}
									else
									{
										dctd.lampcolor = 0x03;
									}
                                    dctd.lamptime = 0;
                                    dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                    dctd.stageline = stInterGrateControlPhaseInfo.stageline;		
									if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#endif
								gettimeofday(&ct,NULL);
                                update_event_list(fcdata->ec,fcdata->el,1,90,ct.tv_sec,fcdata->markbit);

								continue;
							}//prepare to lock
							if (1 == wllock)
							{//prepare to unlock
							#ifdef INTER_CONTROL_DEBUG
                                printf("Prepare to unlock,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
								wllock = 0;
								dcred = 0;
								close = 0;
								dircon = 0;
                                firdc = 1;
                                fdirn = 0;
                                cdirn = 0;
								if(1 == bAllLampClose)
								{
									friststartcontrol = 1;
								}
								if (1 == YellowFlashYes)
								{
									pthread_cancel(YellowFlashPid);
									pthread_join(YellowFlashPid,NULL);
									YellowFlashYes = 0;
								}
								
								new_all_red(&ardata);
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									stStatuSinfo.conmode = 28;
									stStatuSinfo.color = 0x00;
									stStatuSinfo.time = 0;
									stStatuSinfo.stage = 0;
									stStatuSinfo.cyclet = 0;
									stStatuSinfo.phase = 0;
															
									stStatuSinfo.chans = 0;
            						memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            						csta = stStatuSinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						stStatuSinfo.chans += 1;
                						tcsta = i + 1;
                						tcsta <<= 2;
                						tcsta &= 0xfc;
                						*csta = tcsta;
                						csta++;
            						}
									memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            						{
            						#ifdef INTER_CONTROL_DEBUG
                						printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively
						//		all_red(*(fcdata->bbserial),0);
								*(fcdata->markbit2) &= 0xffef;
								*(fcdata->contmode) = contmode;//restore control mode
								if (0 == InterGratedControlYes)
								{
									memset(&stInterGrateControlData,0,sizeof(stInterGrateControlData));
        							memset(&stInterGrateControlPhaseInfo,0,sizeof(stInterGrateControlPhaseInfo));
        							stInterGrateControlData.fd = fcdata;
        							stInterGrateControlData.td = tscdata;
        							stInterGrateControlData.pi = &stInterGrateControlPhaseInfo;
        							stInterGrateControlData.cs = chanstatus;
        							int dcret = pthread_create(&InterGratedControlPid,NULL, \
													(void *)Start_Inter_Grate_Control,&stInterGrateControlData);
        							if (0 != dcret)
        							{
        							#ifdef INTER_CONTROL_DEBUG
            							printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
										output_log("InterGrate control,create full InterGrate thread err");
        							#endif
										Inter_Control_End_Part_Child_Thread();
            							return FAIL;
        							}
									InterGratedControlYes = 1;
								}
								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            	{
                                	memset(wltc,0,sizeof(wltc));
                                	strcpy(wltc,"SOCKEND");
                                	write(*(fcdata->sockfd->ssockfd),wltc,sizeof(wltc));
                            	}
								gettimeofday(&ct,NULL);
                                update_event_list(fcdata->ec,fcdata->el,1,91,ct.tv_sec,fcdata->markbit);

								continue;
							}//prepare to unlock
						}//lock or unlock
						if ((1 == wllock) && (!strcmp("STEP",wlbuf)))
						{//step by step
							firdc = 1;
                            cdirn = 0;
							if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            {
                                memset(wltc,0,sizeof(wltc));
                                strcpy(wltc,"STEPBEGIN");
                                write(*(fcdata->sockfd->ssockfd),wltc,sizeof(wltc));
                            }
							if(1 == bAllLampClose)
							{
								new_all_red(&ardata);
							}
							if ((1 == dcred) || (1 == YellowFlashYes) || (1 == close))
							{
								if (1 == close)
								{
									new_all_red(&ardata);
									close = 0;
								}
								dcred = 0;
								if (1 == YellowFlashYes)
                                {
                                    pthread_cancel(YellowFlashPid);
                                    pthread_join(YellowFlashPid,NULL);
                                    YellowFlashYes = 0;
									new_all_red(&ardata);
                                }
								
				//				#ifdef CLOSE_LAMP
                                igc_set_lamp_color(*(fcdata->bbserial),clch,0x03);
								update_channel_status(fcdata->sockfd,chanstatus,clch,0x03,fcdata->markbit);
                 //               #endif
								if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.chan,0x02))
								{
								#ifdef INTER_CONTROL_DEBUG
                                    printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                    gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                    *(fcdata->markbit) |= 0x0800;	
								}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    							stInterGrateControlPhaseInfo.chan,0x02,fcdata->markbit))
                                {
                                #ifdef INTER_CONTROL_DEBUG 
                                    printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									stStatuSinfo.conmode = 28;
									stStatuSinfo.color = 0x02;
									stStatuSinfo.time = 0;
									stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
									stStatuSinfo.cyclet = 0;
									stStatuSinfo.phase = 0;
									stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
									
									stStatuSinfo.chans = 0;
									memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
                                    csta = stStatuSinfo.csta;
									for (i = 0; i < MAX_CHANNEL; i++)
									{
										if (0 == stInterGrateControlPhaseInfo.chan[i])
											break;
										stStatuSinfo.chans += 1;
										tcsta = stInterGrateControlPhaseInfo.chan[i];
										tcsta <<= 2;
										tcsta &= 0xfc;
										tcsta |= 0x02; //00000010-green 
										*csta = tcsta;
										csta++;
									}
									memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            						{
            						#ifdef INTER_CONTROL_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively
	
								continue;
							}
				
							if ((1 == dircon) && (0 != fdirn))
                            {//direction control happen
								unsigned char			dcchan[MAX_CHANNEL] = {0};
								unsigned char			dcpchan[MAX_CHANNEL] = {0};
								unsigned char			dcnpchan[MAX_CHANNEL] = {0};
								unsigned char			dexist = 0;
								unsigned char			dpexist = 0;
								unsigned char			*pd = dcchan;

								s = 0;
								k = 0;
								for (i = 0; i < MAX_CHANNEL; i++)
								{//i
									if (0 == dirch[fdirn-1][i])
										break;
									dexist = 0;
									for (j = 0; j < MAX_CHANNEL; j++ )
									{//j
										if (0 == stInterGrateControlPhaseInfo.chan[j])
											break;
										if (dirch[fdirn-1][i] == stInterGrateControlPhaseInfo.chan[j])
										{
											dexist = 1;
											break;
										}			
									}//j
									if (0 == dexist)
									{//if (0 == exist)
										*pd = dirch[fdirn-1][i];
										pd++;
										for (j = 0; j < (tscdata->channel->FactChannelNum); j++)
										{//2j
											if (0 == (tscdata->channel->ChannelList[j].ChannelId))
												break;
											if(dirch[fdirn-1][i]==(tscdata->channel->ChannelList[j].ChannelId))
											{
												if (3 == tscdata->channel->ChannelList[j].ChannelType)
												{
													dcpchan[s] = dirch[fdirn-1][i];
													s++;
													dpexist = 1;		
												}
												else
												{
													dcnpchan[k] = dirch[fdirn-1][i];
													k++;
												}
												break;
											}
										}//2j
									}//if (0 == exist)
								}//i
								//end compute
								if (0 == dcchan[0])
								{
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										stStatuSinfo.conmode = 28;
										stStatuSinfo.color = 0x02;
										stStatuSinfo.time = 6;
										stStatuSinfo.stage = 0;
										stStatuSinfo.cyclet = 0;
										stStatuSinfo.phase = 0;
																
										stStatuSinfo.chans = 0;
										memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
										csta = stStatuSinfo.csta;
										for (i = 0; i < MAX_CHANNEL; i++)
										{
											if (0 == dcchan[i])
												break;
											stStatuSinfo.chans += 1;
											tcsta = dcchan[i];
											tcsta <<= 2;
											tcsta &= 0xfc;
											tcsta |= 0x02;
											*csta = tcsta;
											csta++;
										}
										memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
										memset(sibuf,0,sizeof(sibuf));
										if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										else
										{
											write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
										}
									}//report info to server actively
								}//if ((0==cchan[0])&&(stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0))

								if (0 != dcchan[0])
								{
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										stStatuSinfo.conmode = 28;
										stStatuSinfo.color = 0x03;
										stStatuSinfo.time = 3;
										stStatuSinfo.stage = 0;
										stStatuSinfo.cyclet = 0;
										stStatuSinfo.phase = 0;
										
										for (i = 0; i < MAX_CHANNEL; i++)
										{
											if (0 == dcchan[i])
												break;
											for (j = 0; j < stStatuSinfo.chans; j++)
											{
												if (0 == stStatuSinfo.csta[j])
													break;
												tcsta = stStatuSinfo.csta[j];
												tcsta &= 0xfc;
												tcsta >>= 2;
												tcsta &= 0x3f;
												if (dcchan[i] == tcsta)
												{
													stStatuSinfo.csta[j] &= 0xfc;
													stStatuSinfo.csta[j] |= 0x03; //00000001-green flash
													break;
												}
											}
										}								
										memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));	
										memset(sibuf,0,sizeof(sibuf));
										if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										else
										{
											write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
										}
									}//report info to server actively
								}//if ((0 != cchan[0]) && (stInterGrateControlPhaseInfo.gftime > 0))

								//current phase begin to green flash
								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
								{
									memset(&dctd,0,sizeof(dctd));
									dctd.mode = 28;//traffic control
									dctd.pattern = *(fcdata->patternid);
									dctd.lampcolor = 0x02;
									dctd.lamptime = 3;
									dctd.phaseid = 0;
									dctd.stageline = 0;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef I_EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
								{
									memset(&dctd,0,sizeof(dctd));
									dctd.mode = 28;//traffic control
									dctd.pattern = *(fcdata->patternid);
									dctd.lampcolor = 0x02;
									dctd.lamptime = 3;
									dctd.phaseid = 0;
									dctd.stageline = 0;	
									if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#endif
								fbdata[1] = 28;
								fbdata[2] = 0;
								fbdata[3] = 0x02;
								fbdata[4] = 3;
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
								}
								else
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("face board serial port cannot write,Line:%d\n",__LINE__);
								#endif
								}
								if (stInterGrateControlPhaseInfo.gftime > 0)
								{
									ngf = 0;
									while (1)
									{//green flash
										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),dcchan,0x03))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;	
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														dcchan,0x03,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);

										if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),dcchan,0x02))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														dcchan,0x02,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);
									
										ngf += 1;
										if (ngf >= 3)
											break;
									}//green flash
								}//if (stInterGrateControlPhaseInfo.gftime > 0)
								if (1 == dpexist)
								{
									//current phase begin to yellow lamp
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),dcnpchan,0x01))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														dcnpchan,0x01, fcdata->markbit))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),dcpchan,0x00))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																		dcpchan,0x00,fcdata->markbit))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
								}
								else
								{
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),dcchan,0x01))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															dcchan,0x01,fcdata->markbit))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
								}
								
								if (0 != dcchan[0])
								{
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										stStatuSinfo.conmode = 28;
										stStatuSinfo.color = 0x01;
										stStatuSinfo.time = 3;
										stStatuSinfo.stage = 0;
										stStatuSinfo.cyclet = 0;
										stStatuSinfo.phase = 0;
																
										for (i = 0; i < MAX_CHANNEL; i++)
										{
											if (0 == dcnpchan[i])
												break;
											for (j = 0; j < stStatuSinfo.chans; j++)
											{
												if (0 == stStatuSinfo.csta[j])
													break;
												tcsta = stStatuSinfo.csta[j];
												tcsta &= 0xfc;
												tcsta >>= 2;
												tcsta &= 0x3f;
												if (dcnpchan[i] == tcsta)
												{
													stStatuSinfo.csta[j] &= 0xfc;
													stStatuSinfo.csta[j] |= 0x01; //00000001-yellow
													break;
												}
											}
										}
										for (i = 0; i < MAX_CHANNEL; i++)
										{
											if (0 == dcpchan[i])
												break;
											for (j = 0; j < stStatuSinfo.chans; j++)
											{
												if (0 == stStatuSinfo.csta[j])
													break;
												tcsta = stStatuSinfo.csta[j];
												tcsta &= 0xfc;
												tcsta >>= 2;
												tcsta &= 0x3f;
												if (dcpchan[i] == tcsta)
												{
													stStatuSinfo.csta[j] &= 0xfc;
													break;
												}
											}
										}
										memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
										memset(sibuf,0,sizeof(sibuf));
										if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										else
										{
											write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
										}
									}//report info to server actively
								}//if ((0 != cchan[0]) && (stInterGrateControlPhaseInfo.ytime > 0))

								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
								{
									memset(&dctd,0,sizeof(dctd));
									dctd.mode = 28;//traffic control
									dctd.pattern = *(fcdata->patternid);
									dctd.lampcolor = 0x01;
									dctd.lamptime = 3;
									dctd.phaseid = 0;
									dctd.stageline = 0;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef I_EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
								{
									memset(&dctd,0,sizeof(dctd));
									dctd.mode = 28;//traffic control
									dctd.pattern = *(fcdata->patternid);
									dctd.lampcolor = 0x01;
									dctd.lamptime = 3;
									dctd.phaseid = 0;
									dctd.stageline = 0;	
									if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#endif
								fbdata[1] = 28;
								fbdata[2] = 0;
								fbdata[3] = 0x01;
								fbdata[4] = 3;
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
								}
								else
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("face board serial port cannot write,Line:%d\n",__LINE__);
								#endif
								}

								sleep(3);

								if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),dcchan,0x00))
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									*(fcdata->markbit) |= 0x0800;
								}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														dcchan,0x00,fcdata->markbit))
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
								
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															stInterGrateControlPhaseInfo.chan,0x02,fcdata->markbit))
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
								if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.chan,0x02))
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									*(fcdata->markbit) |= 0x0800;
									return FAIL;
								}
								else
								{//set success
									if ((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										stStatuSinfo.conmode = 28;
										stStatuSinfo.color = 0x02;
										stStatuSinfo.time = 0;
										stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
										stStatuSinfo.cyclet = 0;
										stStatuSinfo.phase = 0;
                                    	stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
										stStatuSinfo.chans = 0;
										memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
										csta = stStatuSinfo.csta;
										for (i = 0; i < MAX_CHANNEL; i++)
										{
											if (0 == stInterGrateControlPhaseInfo.chan[i])
												break;
											stStatuSinfo.chans += 1;
											tcsta = stInterGrateControlPhaseInfo.chan[i];
											tcsta <<= 2;
											tcsta &= 0xfc;
											tcsta |= 0x02; //00000010-green 
											*csta = tcsta;
											csta++;
										}
										memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
										memset(sibuf,0,sizeof(sibuf));
										if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										else
										{
											write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
										}
									}//report info to server actively

									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&dctd,0,sizeof(dctd));
										dctd.mode = 28;//traffic control
										dctd.pattern = *(fcdata->patternid);
										dctd.lampcolor = 0x02;
										dctd.lamptime = 0;
										dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                		dctd.stageline = stInterGrateControlPhaseInfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef I_EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&dctd,0,sizeof(dctd));
										dctd.mode = 28;//traffic control
										dctd.pattern = *(fcdata->patternid);
										dctd.lampcolor = 0x02;
										dctd.lamptime = 0;
										dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                		dctd.stageline = stInterGrateControlPhaseInfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
									fbdata[2] = stInterGrateControlPhaseInfo.stageline;
									fbdata[3] = 0x02;
									fbdata[4] = 0;
									if (!wait_write_serial(*(fcdata->fbserial)))
									{
										if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
									}
									else
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("face board serial port cannot write,Line:%d\n",__LINE__);
									#endif
									}							
								}//set success

                                dircon = 0;
								fdirn = 0;
                                continue;
                            }//diredtion control happen
	
							if ((0==stInterGrateControlPhaseInfo.cchan[0])&&((stInterGrateControlPhaseInfo.gftime>0)||(stInterGrateControlPhaseInfo.ytime>0)||(stInterGrateControlPhaseInfo.rtime>0)))
							{
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{
									stStatuSinfo.conmode = 28;
                                    stStatuSinfo.color = 0x02;
                                    stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime + stInterGrateControlPhaseInfo.ytime + stInterGrateControlPhaseInfo.rtime;
                                    stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
                                    stStatuSinfo.cyclet = 0;
                                    stStatuSinfo.phase = 0;
                                    stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));

                                    stStatuSinfo.chans = 0;
                                    memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
                                    csta = stStatuSinfo.csta;
									for (i = 0; i < MAX_CHANNEL; i++)
                                    {
                                        if (0 == stInterGrateControlPhaseInfo.chan[i])
                                            break;
                                        stStatuSinfo.chans += 1;
                                        tcsta = stInterGrateControlPhaseInfo.chan[i];
                                        tcsta <<= 2;
                                        tcsta &= 0xfc;
                                        tcsta |= 0x02;
                                        *csta = tcsta;
                                        csta++;
                                    }
									memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
                                    memset(sibuf,0,sizeof(sibuf));
                                    if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
                                    {
                                    #ifdef PERSON_DEBUG
                                        printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                    else
                                    {
                                        write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
                                    }
								}
							}
							if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.gftime > 0))	
							{
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									stStatuSinfo.conmode = 28;
									stStatuSinfo.color = 0x03;
									stStatuSinfo.time = stInterGrateControlPhaseInfo.gftime;
									stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
									stStatuSinfo.cyclet = 0;
									stStatuSinfo.phase = 0;
									stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
									
									for (i = 0; i < MAX_CHANNEL; i++)
                                	{
                                		if (0 == stInterGrateControlPhaseInfo.cchan[i])
                                        	break;
                                    	for (j = 0; j < stStatuSinfo.chans; j++)
                                    	{
                                        	if (0 == stStatuSinfo.csta[j])
                                            	break;
                                        	tcsta = stStatuSinfo.csta[j];
                                        	tcsta &= 0xfc;
                                        	tcsta >>= 2;
                                        	tcsta &= 0x3f;
                                        	if (stInterGrateControlPhaseInfo.cchan[i] == tcsta)
                                        	{
                                            	stStatuSinfo.csta[j] &= 0xfc;
                                            	stStatuSinfo.csta[j] |= 0x03; //00000001-green flash
												break;
                                        	}
                                    	}
                                	}								
									memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));			
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            						{
            						#ifdef INTER_CONTROL_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively
							}

							fbdata[1] = 28;	
							fbdata[2] = stInterGrateControlPhaseInfo.stageline;
							fbdata[3] = 2;
                            fbdata[4] = stInterGrateControlPhaseInfo.gftime;
                            if (!wait_write_serial(*(fcdata->fbserial)))
                            {
                                if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									*(fcdata->markbit) |= 0x0800;
                                    gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                            }
                            else
                            {
                            #ifdef INTER_CONTROL_DEBUG
                                printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }
							//begin to green flash
							if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
							{
								//send down time to configure tool
								memset(&dctd,0,sizeof(dctd));
								dctd.mode = 28;
								dctd.pattern = *(fcdata->patternid);
								dctd.lampcolor = 0x02;
								dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
								dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
								dctd.stageline = stInterGrateControlPhaseInfo.stageline;
								if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#ifdef I_EMBED_CONFIGURE_TOOL
                            if (*(fcdata->markbit2) & 0x0200)
                            {
                                memset(&dctd,0,sizeof(dctd));
                                dctd.mode = 28;
                                dctd.pattern = *(fcdata->patternid);
                                dctd.lampcolor = 0x02;
                                dctd.lamptime = stInterGrateControlPhaseInfo.gftime;
                                dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                dctd.stageline = stInterGrateControlPhaseInfo.stageline;   
                                if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
                            }
                            #endif
							if (stInterGrateControlPhaseInfo.gftime > 0)
							{
								ngf = 0;
								while (1)
								{//green flash loop
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cchan,0x03))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
										if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
										stInterGrateControlPhaseInfo.cchan,0x03,fcdata->markbit))
									{
									#ifdef INTER_CONTROL_DEBUG 
										printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
									to.tv_sec = 0;
									to.tv_usec = 500000;
									select(0,NULL,NULL,NULL,&to);
									if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cchan,0x02))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
										if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
										{
										#ifdef INTER_CONTROL_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
										stInterGrateControlPhaseInfo.cchan,0x02,fcdata->markbit))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
									to.tv_sec = 0;
									to.tv_usec = 500000;
									select(0,NULL,NULL,NULL,&to);

									ngf += 1;
									if (ngf >= stInterGrateControlPhaseInfo.gftime)
										break;
								}//green flash loop
							}//if (stInterGrateControlPhaseInfo.gftime > 0)
							//end green flash

							//yellow lamp 
							if (1 == stInterGrateControlPhaseInfo.cpcexist)
							{//have person channels
								if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cnpchan,0x01))
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									gettimeofday(&ct,NULL);
                                   	update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                   	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                   	{
                                   	#ifdef INTER_CONTROL_DEBUG
                                       	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   	#endif
                                   	}
									*(fcdata->markbit) |= 0x0800;
								}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															stInterGrateControlPhaseInfo.cnpchan,0x01,fcdata->markbit))
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
								if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cpchan,0x00))
                               	{
                               	#ifdef INTER_CONTROL_DEBUG
                                   	printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               	#endif
									gettimeofday(&ct,NULL);
                                   	update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                   	if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                   	{
                                   	#ifdef INTER_CONTROL_DEBUG
                                       	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   	#endif
                                   	}
									*(fcdata->markbit) |= 0x0800;
                               	}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    							stInterGrateControlPhaseInfo.cpchan,0x00,fcdata->markbit))
                               	{
                               	#ifdef INTER_CONTROL_DEBUG
                                   	printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               	#endif
                               	}	
							}//have person channels
							else
							{//not have person channels
								if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cchan,0x01))
                               	{
                               	#ifdef INTER_CONTROL_DEBUG
                                   	printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               	#endif
									gettimeofday(&ct,NULL);
                                   	update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                   	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                   	{
                                   	#ifdef INTER_CONTROL_DEBUG
                                       	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   	#endif
                                   	}
									*(fcdata->markbit) |= 0x0800;
                               	}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                   									stInterGrateControlPhaseInfo.cchan,0x01,fcdata->markbit))
                               	{
                               	#ifdef INTER_CONTROL_DEBUG
                                   	printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               	#endif
                               	}
							}//not have person channels
							if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.ytime > 0))
							{
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									stStatuSinfo.conmode = 28;
									stStatuSinfo.color = 0x01;
									stStatuSinfo.time = stInterGrateControlPhaseInfo.ytime;
									stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
									stStatuSinfo.cyclet = 0;
									stStatuSinfo.phase = 0;
									stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
															
									for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						if (0 == stInterGrateControlPhaseInfo.cnpchan[i])
                    						break;
                						for (j = 0; j < stStatuSinfo.chans; j++)
                						{
                    						if (0 == stStatuSinfo.csta[j])
                        						break;
                    						tcsta = stStatuSinfo.csta[j];
                    						tcsta &= 0xfc;
                    						tcsta >>= 2;
                    						tcsta &= 0x3f;
                    						if (stInterGrateControlPhaseInfo.cnpchan[i] == tcsta)
                    						{
                        						stStatuSinfo.csta[j] &= 0xfc;
												stStatuSinfo.csta[j] |= 0x01; //00000001-yellow
												break;
                    						}
                						}
            						}
									for (i = 0; i < MAX_CHANNEL; i++)
                                	{
                                		if (0 == stInterGrateControlPhaseInfo.cpchan[i])
                                        	break;
                                    	for (j = 0; j < stStatuSinfo.chans; j++)
                                    	{
                                        	if (0 == stStatuSinfo.csta[j])
                                            	break;
                                        	tcsta = stStatuSinfo.csta[j];
                                        	tcsta &= 0xfc;
                                        	tcsta >>= 2;
                                        	tcsta &= 0x3f;
                                        	if (stInterGrateControlPhaseInfo.cpchan[i] == tcsta)
                                        	{
                                            	stStatuSinfo.csta[j] &= 0xfc;
												break;
                                        	}
                                    	}
                                	}
									memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            						{
            						#ifdef INTER_CONTROL_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively
							}

							fbdata[1] = 28;	
							fbdata[2] = stInterGrateControlPhaseInfo.stageline;
							fbdata[3] = 1;
                            fbdata[4] = stInterGrateControlPhaseInfo.ytime;
                            if (!wait_write_serial(*(fcdata->fbserial)))
                            {
                               	if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               	{
                               	#ifdef INTER_CONTROL_DEBUG
                                   	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               	#endif
									*(fcdata->markbit) |= 0x0800;
                                   	gettimeofday(&ct,NULL);
                                   	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                   	if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                   	{
                                   	#ifdef INTER_CONTROL_DEBUG
                                       	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   	#endif
                                   	}
                               	}
                            }
                            else
                            {
                            #ifdef INTER_CONTROL_DEBUG
                               	printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }
							if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            {   
                                //send down time to configure tool
                                memset(&dctd,0,sizeof(dctd));
                                dctd.mode = 28;
                                dctd.pattern = *(fcdata->patternid);
                                dctd.lampcolor = 0x01;
                                dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                                dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                dctd.stageline = stInterGrateControlPhaseInfo.stageline;
                                if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
                            }
							#ifdef I_EMBED_CONFIGURE_TOOL
							if (*(fcdata->markbit2) & 0x0200)
							{
								memset(&dctd,0,sizeof(dctd));
                                dctd.mode = 28;
                                dctd.pattern = *(fcdata->patternid);
                                dctd.lampcolor = 0x01;
                                dctd.lamptime = stInterGrateControlPhaseInfo.ytime;
                                dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                dctd.stageline = stInterGrateControlPhaseInfo.stageline;	
								if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#endif
							sleep(stInterGrateControlPhaseInfo.ytime);
							//end yellow lamp

							//red lamp
							if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.cchan,0x00))
							{
							#ifdef INTER_CONTROL_DEBUG
								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								gettimeofday(&ct,NULL);
                                update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																fcdata->softevent,fcdata->markbit))
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
								*(fcdata->markbit) |= 0x0800;
							}
							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus,stInterGrateControlPhaseInfo.cchan,\
										0x00,fcdata->markbit))
							{
							#ifdef INTER_CONTROL_DEBUG
								printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}
							if ((0 != stInterGrateControlPhaseInfo.cchan[0]) && (stInterGrateControlPhaseInfo.rtime > 0))
							{
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									stStatuSinfo.conmode = 28;
									stStatuSinfo.color = 0x00;
									stStatuSinfo.time = stInterGrateControlPhaseInfo.rtime;
									stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
									stStatuSinfo.cyclet = 0;
									stStatuSinfo.phase = 0;
									stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
															
									for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						if (0 == stInterGrateControlPhaseInfo.cchan[i])
                    						break;
                						for (j = 0; j < stStatuSinfo.chans; j++)
                						{
                    						if (0 == stStatuSinfo.csta[j])
                        						break;
                    						tcsta = stStatuSinfo.csta[j];
                    						tcsta &= 0xfc;
                    						tcsta >>= 2;
                    						tcsta &= 0x3f;
                    						if (stInterGrateControlPhaseInfo.cchan[i] == tcsta)
                    						{
                        						stStatuSinfo.csta[j] &= 0xfc;
												break;
                    						}
                						}
            						}
									memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            						{
            						#ifdef INTER_CONTROL_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively
								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
								{   
									//send down time to configure tool
									memset(&dctd,0,sizeof(dctd));
									dctd.mode = 28;
									dctd.pattern = *(fcdata->patternid);
									dctd.lampcolor = 0x00;
									dctd.lamptime = stInterGrateControlPhaseInfo.rtime;
									dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
									dctd.stageline = stInterGrateControlPhaseInfo.stageline;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&dctd))
									{
									#ifdef INTER_CONTROL_DEBUG
										printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef I_EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
                                {
                                    //send down time to configure tool
                                   	memset(&dctd,0,sizeof(dctd));
                                    dctd.mode = 28;
                                    dctd.pattern = *(fcdata->patternid);
                                    dctd.lampcolor = 0x00;
                                    dctd.lamptime = stInterGrateControlPhaseInfo.rtime;
                                    dctd.phaseid = stInterGrateControlPhaseInfo.phaseid;
                                    dctd.stageline = stInterGrateControlPhaseInfo.stageline; 
                                    if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&dctd))
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }		
								#endif
							}
							if (stInterGrateControlPhaseInfo.rtime > 0)
							{
								fbdata[1] = 28;
								fbdata[2] = stInterGrateControlPhaseInfo.stageline;
								fbdata[3] = 0;
                            	fbdata[4] = stInterGrateControlPhaseInfo.rtime;
                            	if (!wait_write_serial(*(fcdata->fbserial)))
                            	{
                                	if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                	{
                                	#ifdef INTER_CONTROL_DEBUG
                                    	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
										*(fcdata->markbit) |= 0x0800;
                                    	gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    	if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef INTER_CONTROL_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                	}
                            	}
                            	else
                            	{
                            	#ifdef INTER_CONTROL_DEBUG
                                	printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            	#endif
                            	}
                                sleep(stInterGrateControlPhaseInfo.rtime);
							}	
							//end red lamp
							*(fcdata->slnum) += 1;
                            *(fcdata->stageid) = \
                                tscdata->timeconfig->TimeConfigList[rettl][*(fcdata->slnum)].StageId;
                            if (0 == (tscdata->timeconfig->TimeConfigList[rettl][*(fcdata->slnum)].StageId))
                            {
                                *(fcdata->slnum) = 0;
                                *(fcdata->stageid) = \
                                    tscdata->timeconfig->TimeConfigList[rettl][*(fcdata->slnum)].StageId;
                            }
							//get info of next phase
							memset(&stInterGrateControlPhaseInfo,0,sizeof(stInterGrateControlPhaseInfo));
							if (SUCCESS != igc_get_phase_info(fcdata,tscdata,rettl,*(fcdata->slnum),&stInterGrateControlPhaseInfo))
							{
							#ifdef INTER_CONTROL_DEBUG
								printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
								output_log("InterGrate control,get phase info err");
							#endif
								Inter_Control_End_Part_Child_Thread();
								return FAIL;
							}
							*(fcdata->phaseid) = 0;
							*(fcdata->phaseid) |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
							if (SUCCESS != igc_set_lamp_color(*(fcdata->bbserial),stInterGrateControlPhaseInfo.chan,0x02))
                            {
                            #ifdef INTER_CONTROL_DEBUG
                                printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
								gettimeofday(&ct,NULL);
                                update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																fcdata->softevent,fcdata->markbit))
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
								*(fcdata->markbit) |= 0x0800;
                            }
							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus,stInterGrateControlPhaseInfo.chan,\
                                        0x02,fcdata->markbit))
                            {
                            #ifdef INTER_CONTROL_DEBUG
                                printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }
							if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
							{//report info to server actively
								stStatuSinfo.conmode = 28;
								stStatuSinfo.color = 0x02;
								stStatuSinfo.time = 0;
								stStatuSinfo.stage = stInterGrateControlPhaseInfo.stageline;
								stStatuSinfo.cyclet = 0;
								stStatuSinfo.phase = 0;
								stStatuSinfo.phase |= (0x01 << (stInterGrateControlPhaseInfo.phaseid - 1));
															
								stStatuSinfo.chans = 0;
            					memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            					csta = stStatuSinfo.csta;
            					for (i = 0; i < MAX_CHANNEL; i++)
            					{
									if (0 == stInterGrateControlPhaseInfo.chan[i])
										break;	
                					stStatuSinfo.chans += 1;
                					tcsta = stInterGrateControlPhaseInfo.chan[i];
                					tcsta <<= 2;
                					tcsta &= 0xfc;
									tcsta |= 0x02;	
                					*csta = tcsta;
                					csta++;
            					}
								memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            					memset(sibuf,0,sizeof(sibuf));
            					if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            					{
            					#ifdef INTER_CONTROL_DEBUG
                					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            					#endif
            					}
            					else
            					{
                					write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            					}
							}//report info to server actively

							fbdata[1] = 28;
							fbdata[2] = stInterGrateControlPhaseInfo.stageline;
							fbdata[3] = 2;
                            fbdata[4] = 0;
                            if (!wait_write_serial(*(fcdata->fbserial)))
                            {
                                if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                {
                                #ifdef INTER_CONTROL_DEBUG
                                    printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									*(fcdata->markbit) |= 0x0800;
                                    gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef INTER_CONTROL_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                            }
                            else
                            {
                            #ifdef INTER_CONTROL_DEBUG
                                printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }

							gettimeofday(&ct,NULL);
                            update_event_list(fcdata->ec,fcdata->el,1,92,ct.tv_sec,fcdata->markbit);

							continue;
						}//step by step
						if ((1 == wllock) && (!strcmp("YF",wlbuf)))
						{//yellow flash
							dcred = 0;
							close = 0;
							dircon = 0;
                            firdc = 1;
                            fdirn = 0;
                            cdirn = 0;
							if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            {
                                memset(wltc,0,sizeof(wltc));
                                strcpy(wltc,"YFBEGIN");
                                write(*(fcdata->sockfd->ssockfd),wltc,sizeof(wltc));
                            }
							if (0 == YellowFlashYes)
							{
								memset(&yfdata,0,sizeof(yfdata));
								yfdata.second = 0;
								yfdata.markbit = fcdata->markbit;
								yfdata.markbit2 = fcdata->markbit2;
								yfdata.serial = *(fcdata->bbserial);
								yfdata.sockfd = fcdata->sockfd;
								yfdata.cs = chanstatus;
#ifdef FLASH_DEBUG
								char szInfo[32] = {0};
								char szInfoT[64] = {0};
								snprintf(szInfo,sizeof(szInfo)-1,"%s",__FUNCTION__);
								snprintf(szInfoT,sizeof(szInfoT)-1,"%d",__LINE__);
								tsc_save_eventlog(szInfo,szInfoT);
#endif
								int yfret = pthread_create(&YellowFlashPid,NULL,(void *)Inter_Control_Yellow_Flash,&yfdata);
								if (0 != yfret)
								{
								#ifdef INTER_CONTROL_DEBUG
									printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
									output_log("InterGrate control,create yellow flash err");
								#endif
									Inter_Control_End_Part_Child_Thread();
									return FAIL;
								}
								YellowFlashYes = 1;
							}
							
							if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
							{//report info to server actively
								stStatuSinfo.conmode = 28;
								stStatuSinfo.color = 0x05;
								stStatuSinfo.time = 0;
								stStatuSinfo.stage = 0;
								stStatuSinfo.cyclet = 0;
								stStatuSinfo.phase = 0;
															
								stStatuSinfo.chans = 0;
            					memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            					csta = stStatuSinfo.csta;
            					for (i = 0; i < MAX_CHANNEL; i++)
            					{
                					stStatuSinfo.chans += 1;
                					tcsta = i + 1;
                					tcsta <<= 2;
                					tcsta &= 0xfc;
                					*csta = tcsta;
                					csta++;
            					}
								memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            					memset(sibuf,0,sizeof(sibuf));
            					if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            					{
            					#ifdef INTER_CONTROL_DEBUG
                					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            					#endif
            					}
            					else
            					{
                					write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            					}
							}//report info to server actively
							gettimeofday(&ct,NULL);
                            update_event_list(fcdata->ec,fcdata->el,1,93,ct.tv_sec,fcdata->markbit);

							continue;
						}//yellow flash
						if ((1 == wllock) && (!strcmp("RED",wlbuf)))
						{//all red
							dircon = 0;
                            firdc = 1;
                            fdirn = 0;
                            cdirn = 0;
							if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            {
                                memset(wltc,0,sizeof(wltc));
                                strcpy(wltc,"REDBEGIN");
                                write(*(fcdata->sockfd->ssockfd),wltc,sizeof(wltc));
                            }
							if (1 == YellowFlashYes)
							{
								pthread_cancel(YellowFlashPid);
								pthread_join(YellowFlashPid,NULL);
								YellowFlashYes = 0;
							}
							close = 0;
							if (0 == dcred)
							{
								new_all_red(&ardata);	
								dcred = 1;
							}
							
							if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
							{//report info to server actively
								stStatuSinfo.conmode = 28;
								stStatuSinfo.color = 0x00;
								stStatuSinfo.time = 0;
								stStatuSinfo.stage = 0;
								stStatuSinfo.cyclet = 0;
								stStatuSinfo.phase = 0;
															
								stStatuSinfo.chans = 0;
            					memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            					csta = stStatuSinfo.csta;
            					for (i = 0; i < MAX_CHANNEL; i++)
            					{
                					stStatuSinfo.chans += 1;
                					tcsta = i + 1;
                					tcsta <<= 2;
                					tcsta &= 0xfc;
                					*csta = tcsta;
                					csta++;
            					}
								memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            					memset(sibuf,0,sizeof(sibuf));
            					if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            					{
            					#ifdef INTER_CONTROL_DEBUG
                					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            					#endif
            					}
            					else
            					{
                					write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            					}
							}//report info to server actively
							gettimeofday(&ct,NULL);
                            update_event_list(fcdata->ec,fcdata->el,1,94,ct.tv_sec,fcdata->markbit);

							continue;
						}//all red
						if ((1 == wllock) && (!strcmp("CLO",wlbuf)))
						{//all close
							dircon = 0;
                            firdc = 1;
                            fdirn = 0;
                            cdirn = 0;
							if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            {
                                memset(wltc,0,sizeof(wltc));
                                strcpy(wltc,"CLOBEGIN");
                                write(*(fcdata->sockfd->ssockfd),wltc,sizeof(wltc));
                            }
							if (1 == YellowFlashYes)
							{
								pthread_cancel(YellowFlashPid);
								pthread_join(YellowFlashPid,NULL);
								YellowFlashYes = 0;
							}
							dcred = 0;
							if (0 == close)
							{
								new_all_close(&acdata);	
								close = 1;
							}
							
							if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
							{//report info to server actively
								stStatuSinfo.conmode = 28;
								stStatuSinfo.color = 0x04;
								stStatuSinfo.time = 0;
								stStatuSinfo.stage = 0;
								stStatuSinfo.cyclet = 0;
								stStatuSinfo.phase = 0;
															
								stStatuSinfo.chans = 0;
            					memset(stStatuSinfo.csta,0,sizeof(stStatuSinfo.csta));
            					csta = stStatuSinfo.csta;
            					for (i = 0; i < MAX_CHANNEL; i++)
            					{
                					stStatuSinfo.chans += 1;
                					tcsta = i + 1;
                					tcsta <<= 2;
                					tcsta &= 0xfc;
                					*csta = tcsta;
                					csta++;
            					}
								memcpy(fcdata->sinfo,&stStatuSinfo,sizeof(statusinfo_t));
            					memset(sibuf,0,sizeof(sibuf));
            					if (SUCCESS != status_info_report(sibuf,&stStatuSinfo))
            					{
            					#ifdef INTER_CONTROL_DEBUG
                					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            					#endif
            					}
            					else
            					{
                					write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            					}
							}//report info to server actively
							continue;
						}//all close 
						if ((1 == wllock) && (!strncmp("SP",wlbuf,2)))
						{//stage phase
							unsigned char	staid = 0;
							if (!strcmp("SP11",wlbuf))
							{
								staid = 0x01; 
							}
							else if (!strcmp("SP12",wlbuf))
                            {
                                staid = 0x02;
                            }
							else if (!strcmp("SP13",wlbuf))
                            {
                                staid = 0x03;
                            }
							else if (!strcmp("SP14",wlbuf))
                            {
                                staid = 0x04;
                            }
							else if (!strcmp("SP15",wlbuf))
                            {
                                staid = 0x05;
                            }
							else if (!strcmp("SP16",wlbuf))
                            {
                                staid = 0x06;
                            }
							else if (!strcmp("SP17",wlbuf))
                            {
                                staid = 0x07;
                            }
							else if (!strcmp("SP18",wlbuf))
							{
								staid = 0x08;
							}
							memset(&mdt,0,sizeof(markdata_c));
							mdt.redl = &dcred;
							mdt.closel = &close;
							mdt.rettl = &rettl;
							mdt.dircon = &dircon;
							mdt.firdc = &firdc;
							mdt.yfl = &YellowFlashYes;
							mdt.yfid = &YellowFlashPid;
							mdt.ardata = &ardata;
							mdt.fcdata = fcdata;
							mdt.tscdata = tscdata;
							mdt.chanstatus = chanstatus;
							mdt.sinfo = &stStatuSinfo;
							igc_mobile_jp_control(&mdt,staid,&stInterGrateControlPhaseInfo,fdirch);
							dircon = 0;
                            firdc = 1;
							fdirn = 0;
							cdirn = 0;
							if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            {
                                memset(wltc,0,sizeof(wltc));
                                strncpy(wltc,wlbuf,4);
								strcat(wltc,"BEGIN");	
                                write(*(fcdata->sockfd->ssockfd),wltc,sizeof(wltc));
                            }
							continue;
						}//stage phase
						if ((1 == wllock) && (!strncmp("DIR",wlbuf,3)))
						{//Direction control
							dircon = 1;
							memset(cdirch,0,sizeof(cdirch));
							if (!strcmp("DIR1",wlbuf))
							{
								cdirn = 1;
								j = 0;
								for(i = 0; i < MAX_CHANNEL; i++)
								{
									if (0 == dirch[0][i])
										break;
									cdirch[j] = dirch[0][i];
									j++;
								}
							}
							else if (!strcmp("DIR2",wlbuf))
                            {
								cdirn = 2;
                                j = 0;
                                for(i = 0; i < MAX_CHANNEL; i++)
                                {
                                    if (0 == dirch[1][i])
                                        break;
                                    cdirch[j] = dirch[1][i];
                                    j++;
                                }
                            }						
							else if (!strcmp("DIR3",wlbuf))
                            {
								cdirn = 3;
                                j = 0;
                                for(i = 0; i < MAX_CHANNEL; i++)
                                {
                                    if (0 == dirch[2][i])
                                        break;
                                    cdirch[j] = dirch[2][i];
                                    j++;
                                }
                            }
							else if (!strcmp("DIR4",wlbuf))
                            {
								cdirn = 4;
                                j = 0;
                                for(i = 0; i < MAX_CHANNEL; i++)
                                {
                                    if (0 == dirch[3][i])
                                        break;
                                    cdirch[j] = dirch[3][i];
                                    j++;
                                }
                            }
							else if (!strcmp("DIR5",wlbuf))
                            {
								cdirn = 5;
                                j = 0;
                                for(i = 0; i < MAX_CHANNEL; i++)
                                {
                                    if (0 == dirch[4][i])
                                        break;
                                    cdirch[j] = dirch[4][i];
                                    j++;
                                }
                            }
							else if (!strcmp("DIR6",wlbuf))
                            {
								cdirn = 6;
                                j = 0;
                                for(i = 0; i < MAX_CHANNEL; i++)
                                {
                                    if (0 == dirch[5][i])
                                        break;
                                    cdirch[j] = dirch[5][i];
                                    j++;
                                }
                            }
							else if (!strcmp("DIR7",wlbuf))
                            {
								cdirn = 7;
                                j = 0;
                                for(i = 0; i < MAX_CHANNEL; i++)
                                {
                                    if (0 == dirch[6][i])
                                        break;
                                    cdirch[j] = dirch[6][i];
                                    j++;
                                }
                            }						
							else if (!strcmp("DIR8",wlbuf))
                            {
								cdirn = 8;
                                j = 0;
                                for(i = 0; i < MAX_CHANNEL; i++)
                                {
                                    if (0 == dirch[7][i])
                                        break;
                                    cdirch[j] = dirch[7][i];
                                    j++;
                                }
                            }

							if (fdirn == cdirn) //two dir control is same
							{
								continue;
							}	
							memset(&mdt,0,sizeof(markdata_c));
							mdt.redl = &dcred;
							mdt.closel = &close;
							mdt.rettl = &rettl;
							mdt.dircon = &dircon;
							mdt.firdc = &firdc;
							mdt.yfl = &YellowFlashYes;
							mdt.yfid = &YellowFlashPid;
							mdt.ardata = &ardata;
							mdt.fcdata = fcdata;
							mdt.tscdata = tscdata;
							mdt.chanstatus = chanstatus;
							mdt.sinfo = &stStatuSinfo;
							igc_mobile_direct_control(&mdt,&stInterGrateControlPhaseInfo,cdirch,fdirch);
							firdc = 0;
							fdirn = cdirn;
							cdirn = 0;
							memset(fdirch,0,sizeof(fdirch));
							for (i = 0; i < MAX_CHANNEL; i++)
							{
								if (0 == cdirch[i])
									break;
								fdirch[i] = cdirch[i];
							}
							if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            {
                                memset(wltc,0,sizeof(wltc));
                                strncpy(wltc,wlbuf,4);
                                strcat(wltc,"BEGIN");
                                write(*(fcdata->sockfd->ssockfd),wltc,sizeof(wltc));
                            }
							continue;
						}//Direction control
                    }//wireless lock or unlock is suessfully only when key lock or unlock is not valid;
				}//wireless terminal traffic control
			}//if (FD_ISSET(*(fcdata->conpipe),&nread))
		}//cpret > 0
	}//while loop
	
	return SUCCESS;	
}


