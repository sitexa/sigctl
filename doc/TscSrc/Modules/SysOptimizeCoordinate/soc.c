/* 
 * File:   soc.c
 * Author: sk
 *
 * Created on 20150630
 */

#include "socs.h"
static struct timeval			gtime,gftime,ytime,rtime;
static int						socyes = 0;
static pthread_t				socpid;
static int						socyfyes = 0;
static pthread_t				socyfpid;
static int						socpcyes = 0;
static pthread_t				socpcpid;
static unsigned char			socrettl = 0;
static socphasedetect_t			phdetect[MAX_PHASE_LINE] = {0};
static socphasedetect_t			*pphdetect = NULL;
static socdetectorpro_t         *dpro = NULL;
static statusinfo_t             sinfo;
static unsigned char			phcon = 0; //value is '1' means have phase control;
static unsigned char            curpid = 0;
static unsigned char            ppmyes = 0;
static pthread_t                ppmid;
static pthread_t				rfpid;

static void soc_yellow_flash(void *arg);

void soc_end_part_child_thread()
{
    if (1 == socyfyes)
    {
        pthread_cancel(socyfpid);
        pthread_join(socyfpid,NULL);
        socyfyes = 0;
    }
	if (1 == ppmyes)
    {
        pthread_cancel(ppmid);
        pthread_join(ppmid,NULL);
        ppmyes = 0;
    }
    if (1 == socpcyes)
    {
        pthread_cancel(socpcpid);
        pthread_join(socpcpid,NULL);
        socpcyes = 0;
    }
}

void soc_end_child_thread()
{
    if (1 == socyes)
    {
        pthread_cancel(socpid);
        pthread_join(socpid,NULL);
        socyes = 0;
    }
    if (1 == socyfyes)
    {
        pthread_cancel(socyfpid);
        pthread_join(socyfpid,NULL);
        socyfyes = 0;
    }
	if (1 == ppmyes)
    {
        pthread_cancel(ppmid);
        pthread_join(ppmid,NULL);
        ppmyes = 0;
    }
    if (1 == socpcyes)
    {
        pthread_cancel(socpcpid);
        pthread_join(socpcpid,NULL);
        socpcyes = 0;
    }
}

void soc_monitor_pending_pipe(void *arg)
{
	sleep(2);
	//only after wait 2s,'curpid' variable should have value; 
	socmonpendphase_t			*mpphase = arg;
	fd_set						nRead;
	unsigned char				buf[256] = {0};
	unsigned char				*pbuf = buf;
	unsigned short				num = 0;
	unsigned short				mark = 0;	
	unsigned char				i = 0, j = 0;
	unsigned char				dtype = 0; //detector type
	unsigned char				deteid = 0;//detector id;
	unsigned char				mphase = 0;
	unsigned short				itime = 0;
	unsigned short				otime = 0;
	unsigned short				ttime = 0;
	unsigned char				curpyes = 0;
	unsigned char				mappid = 0;
	unsigned char				EndCompute = 0;

	while (1)
	{//while loop
		FD_ZERO(&nRead);
		FD_SET(*(mpphase->pendpipe),&nRead);
		int fret = select(*(mpphase->pendpipe)+1,&nRead,NULL,NULL,NULL);
		if (fret < 0)
		{
		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
			printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return;
		}
		if (fret > 0)
		{//if (fret > 0)
			if (FD_ISSET(*(mpphase->pendpipe),&nRead))
			{
				memset(buf,0,sizeof(buf));
				pbuf = buf;
				num = read(*(mpphase->pendpipe),buf,sizeof(buf)); 
				if (num > sizeof(buf))
                    continue;
				if (num > 0)
				{
					mark = 0;
					while (1)
					{//inline while loop
						if (mark >= num)
							break;
						if ((0xF1 == *(pbuf+mark)) && (0xED == *(pbuf+mark+5)))
						{
							deteid = *(pbuf+mark+2);
							if (1 == *(pbuf+mark+1))
							{//if (1 == *(pbuf+mark+1))
								itime = 0;
								itime |= *(pbuf+mark+3);
								itime <<= 8;
								itime |= *(pbuf+mark+4);

								for (i = 0; i < MAX_PHASE_LINE; i++)
                                {
                                    if (0 == phdetect[i].phaseid)
                                        break;
                                    for (j = 0; j < 10; j++)
                                    {
                                        if (0 == phdetect[i].socdetectpro[j].deteid)
                                            break;
                                        if (deteid == phdetect[i].socdetectpro[j].deteid)
                                        {
											phdetect[i].socdetectpro[j].icarnum += 1;
                                        }
                                    }
                                }

								curpyes = 0;
                                for (j = 0; j < (mpphase->detector->FactDetectorNum); j++)
                                {
                                    if (0 == (mpphase->detector->DetectorList[j].DetectorId))
                                        break;
                                    if (deteid == (mpphase->detector->DetectorList[j].DetectorId))
                                    {
                                        dtype = mpphase->detector->DetectorList[j].DetectorType;
                                        if ((0x40 == dtype) || (0x80 == dtype) || (0x04 == dtype))
                                        {//request or response detector or bus detector
                                            mappid = mpphase->detector->DetectorList[j].DetectorPhase;
                                            if (mappid == curpid)
                                            {
                                                curpyes = 1;
                                                break;
                                            }
                                        }//request or response detector
                                        else
                                        {//unfit detector
											curpyes = 1; //lest the following code is run
                                            break;
                                        }//unfit detector
                                    }
                                }//for (j = 0; j < (dcdata->td->detector->FactDetectorNum); j++) 
								if (0 == curpyes)
								{//0 == curpyes
									for (i = 0; i < MAX_PHASE_LINE; i++)
                                    {
                                    	if (0 == phdetect[i].phaseid)
                                            break;
										if (curpid != phdetect[i].phaseid)
										{
											for (j = 0; j < 10; j++)
                                            {
                                            	if (0 == phdetect[i].socdetectpro[j].deteid)
                                                	break;
												if (deteid == phdetect[i].socdetectpro[j].deteid)
                                                {
													if (0 == phdetect[i].socdetectpro[j].redflow)
                                                		phdetect[i].socdetectpro[j].redflow += 1;
													break;	
                                            	}
											}//j = 0; j < 10; j++
										}//if (curpid != phdetect[i].phaseid)
									}//i = 0; i < MAX_PHASE_LINE; i++
								}//0 == curpyes
							}//if (1 == *(pbuf+mark+1))
							if (2 == *(pbuf+mark+1))
							{
								otime = 0;
								otime |= *(pbuf+mark+3);
								otime <<= 8;
								otime |= *(pbuf+mark+4);
//								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
  //                              printf("curpid: %d,leave time: %d,File: %s,Line: %d\n", \
	//									curpid,otime,__FILE__,__LINE__);
      //                      	#endif	
								//Not consider whether the detector is valid or not when vehicle leave detector 
								//Because has been considered when vehicle enter detector
								curpyes = 0;	
								for (j = 0; j < (mpphase->detector->FactDetectorNum); j++)
								{
                                	if (0 == (mpphase->detector->DetectorList[j].DetectorId))
                                    	break;
                                	if (deteid == (mpphase->detector->DetectorList[j].DetectorId))
                                	{
                                    	dtype = mpphase->detector->DetectorList[j].DetectorType;
                                        if ((0x40 == dtype) || (0x80 == dtype) || (0x04 == dtype))
                                        {//request or response detector or bus detector
                                        	mappid = mpphase->detector->DetectorList[j].DetectorPhase;
                                            if (mappid == curpid)
                                            {
                                          		curpyes = 1;
                                                break;
                                            }
                                        }//request or response detector
                                        else
                                        {//unfit detector
                                            break;
                                        }//unfit detector
                                    }
                                }//for (j = 0; j < (dcdata->td->detector->FactDetectorNum); j++)	
								if (1 == curpyes)
								{
									EndCompute = 0;
									for (i = 0; i < MAX_PHASE_LINE; i++)
                                    {
                                    	if (0 == phdetect[i].phaseid)
                                            break;
										if (curpid == phdetect[i].phaseid)
										{
											for (j = 0; j < 10; j++)
                                            {
                                            	if (0 == phdetect[i].socdetectpro[j].deteid)
                                                	break;
												if (deteid == phdetect[i].socdetectpro[j].deteid)
                                                {
                                                	if (0 == phdetect[i].socdetectpro[j].notfirstdata)
                                                    {
                                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                                    	printf("This is the first data will be ignored\n");
                                                    #endif
                                                        phdetect[i].socdetectpro[j].notfirstdata = 1;
                                                        EndCompute = 1;
                                                        break;
                                                    }
                                                    if (1 == phdetect[i].socdetectpro[j].notfirstdata)
                                                    {
                                                //    	PhaseCarRoad[i].CarRoadNum[j].MaxOccTime += MaxOccTime;
                                                //        PhaseCarRoad[i].CarRoadNum[j].CarNum += 1;
														//compute maxocctime and output car num
														ttime = 0;
														if (otime >= itime)
														{
															ttime = otime - itime;
														#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
															printf("ttime: %d,File: %s,Line: %d\n", ttime, \
																	__FILE__,__LINE__);
														#endif
															phdetect[i].socdetectpro[j].maxocctime += ttime;
														}
														else
														{
															ttime = 0xffff - itime;
															ttime += otime;
														#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                                            printf("ttime: %d,File: %s,Line: %d\n", ttime, \
                                                                    __FILE__,__LINE__);
                                                        #endif
															phdetect[i].socdetectpro[j].maxocctime += ttime;
														}
														phdetect[i].socdetectpro[j].ocarnum += 1;
                                                        EndCompute = 1;
                                                        break;
                                                    }
                                            	}
											}//j = 0; j < 10; j++
											if (1 == EndCompute)
												break;
										}//if (curpid == phdetect[i].phaseid)
									}//i = 0; i < MAX_PHASE_LINE; i++
								}//if (1 == curpyes)
							}//if (2 == *(pbuf+mark+1))
							mark += 6;
							continue;
						}
						else
						{
							mark += 1;
							continue;
						}
					}//inline while loop
				}//if (num > 0)
				else
				{
					continue;
				}
			}//if (FD_ISSET(*(mpphase->pendpipe),&nRead))
			else
			{
				continue;
			}
		}//if (fret > 0)
	}//while loop

	pthread_exit(NULL);
}

void soc_person_chan_greenflash(void *arg)
{
	socpcdata_t			*pcdata = arg;
	struct timeval		timeout;	
	unsigned char		num = 0;
	
	while(1)
	{
		//close
		if (SUCCESS != soc_set_lamp_color(*(pcdata->bbserial),pcdata->pchan,0x03))
        {
        #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
			*(pcdata->markbit) |= 0x0800;
        }
		if (SUCCESS != update_channel_status(pcdata->sockfd,pcdata->cs,pcdata->pchan,0x03,pcdata->markbit))
        {
        #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
        #endif
        }
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;
		select(0,NULL,NULL,NULL,&timeout);
		//green
		if (SUCCESS != soc_set_lamp_color(*(pcdata->bbserial),pcdata->pchan,0x02))
        {
        #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
			*(pcdata->markbit) |= 0x0800;
        }
		if (SUCCESS != update_channel_status(pcdata->sockfd,pcdata->cs,pcdata->pchan,0x02,pcdata->markbit))
        {
        #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
        #endif
        }
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;
		select(0,NULL,NULL,NULL,&timeout);

		num += 1;
		if (num >= (pcdata->time))
			break;
	}//while loop

	pthread_exit(NULL);
}


void start_optimize_coordinate_control(void *arg)
{
	socdata_t				*tcdata = arg;
	unsigned char			tcline = 0;
	unsigned char			slnum = 0;
	timedown_t				timedown;
	unsigned char			snum = 0;//the max stage value
	unsigned char			contm = *(tcdata->fd->contmode);
	int                     i = 0,j = 0,z = 0,k = 0,s = 0;
	unsigned char			tphid = 0;
	fd_set					nRead;
	unsigned char			sltime = 0;
	struct timeval			mctime,lasttime,nowtime;
	unsigned char			reatime = 0;
	unsigned char			recbuf[16] = {0};
	unsigned char			ffw = 0;
	unsigned char			temp = 0;
	unsigned char			tclc1 = 0;
	socmonpendphase_t       mpphase;

	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
	printf("Begin to system optimize coordinate control,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
	
	if (SUCCESS != soc_get_timeconfig(tcdata->td,tcdata->fd->patternid,&tcline))
	{
	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
		printf("get tcline err,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		soc_end_part_child_thread();
		//return;

		struct timeval				time;
		unsigned char				yferr[10] = {0};
		gettimeofday(&time,NULL);
		update_event_list(tcdata->fd->ec,tcdata->fd->el,1,11,time.tv_sec,tcdata->fd->markbit);
		if ((!(*(tcdata->fd->markbit) & 0x1000)) && (!(*(tcdata->fd->markbit) & 0x8000)))
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
				write(*(tcdata->fd->sockfd->csockfd),yferr,sizeof(yferr));
			}
		}

		yfdata_t					yfdata;
		if (0 == socyfyes)
		{
			memset(&yfdata,0,sizeof(yfdata));
			yfdata.second = 0;
			yfdata.markbit = tcdata->fd->markbit;
			yfdata.markbit2 = tcdata->fd->markbit2;
			yfdata.serial = *(tcdata->fd->bbserial);
			yfdata.sockfd = tcdata->fd->sockfd;
			yfdata.cs = tcdata->cs;		
#ifdef FLASH_DEBUG
			char szInfo[32] = {0};
			char szInfoT[64] = {0};
			snprintf(szInfo,sizeof(szInfo)-1,"%s",__FUNCTION__);
			snprintf(szInfoT,sizeof(szInfoT)-1,"%d",__LINE__);
			tsc_save_eventlog(szInfo,szInfoT);
#endif
			int yfret = pthread_create(&socyfpid,NULL,(void *)soc_yellow_flash,&yfdata);
			if (0 != yfret)
			{
				#ifdef TIMING_DEBUG
				printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
				output_log("soc_yellow_flash control,create yellow flash err");
				#endif
				soc_end_part_child_thread();
				return;
			}
			socyfyes = 1;
		}		
		while(1)
		{
			if (*(tcdata->fd->markbit) & 0x0001)
			{//end time of current pattern has arrived
				unsigned char           enddata[3] = {0xCC,0xDD,0xED};
				if (!wait_write_serial(*(tcdata->fd->synpipe)))
				{
					if (write(*(tcdata->fd->synpipe),enddata,sizeof(enddata)) < 0)
					{
				#ifdef FULL_DETECT_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
						soc_end_part_child_thread();
						return;
					}
				}
				else
				{
			#ifdef FULL_DETECT_DEBUG
					printf("pipe cannot be written,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
					soc_end_part_child_thread();
					return;
				}
				sleep(5);//wait main module end own;	
			}//end time of current pattern has arrived
			sleep(10);
			continue;
		}

	}

	socrettl = tcline;
//	memset(phdetect,0,sizeof(socphasedetect_t)*MAX_PHASE_LINE);
	memset(phdetect,0,sizeof(phdetect));
    pphdetect = phdetect;
    j = 0;
	//compute total stage num of current pattern;
	for (snum = 0; ;snum++)
	{
		if (0 == (tcdata->td->timeconfig->TimeConfigList[tcline][snum].StageId))
			break;
		pphdetect->stageid = tcdata->td->timeconfig->TimeConfigList[tcline][snum].StageId;
		get_phase_id(tcdata->td->timeconfig->TimeConfigList[tcline][snum].PhaseId,&tphid);

		pphdetect->phaseid = tphid;
		pphdetect->gtime = tcdata->td->timeconfig->TimeConfigList[tcline][snum].GreenTime;
		pphdetect->syscoofixgreen = pphdetect->gtime;
        pphdetect->ytime = tcdata->td->timeconfig->TimeConfigList[tcline][snum].YellowTime;
        pphdetect->rtime = tcdata->td->timeconfig->TimeConfigList[tcline][snum].RedTime;
		pphdetect->curpt = pphdetect->gtime + pphdetect->ytime + pphdetect->rtime;
		temp = tcdata->td->timeconfig->TimeConfigList[tcline][snum].SpecFunc;
        temp &= 0x1e;
        temp = temp >> 1;
        temp &= 0x0f;
        pphdetect->startdelay = temp;

		//get channls of phase 
		for (i = 0; i < (tcdata->td->channel->FactChannelNum); i++)
		{
			if (0 == (tcdata->td->channel->ChannelList[i].ChannelId))
				break;
			if ((pphdetect->phaseid) == (tcdata->td->channel->ChannelList[i].ChannelCtrlSrc))
			{
				#ifdef CLOSE_LAMP
                tclc1 = tcdata->td->channel->ChannelList[i].ChannelId;
                if ((tclc1 >= 0x05) && (tclc1 <= 0x0c))
                {
                    if (*(tcdata->fd->specfunc) & (0x01 << (tclc1 - 5)))
                        continue;
                }
				#else
				if ((*(tcdata->fd->specfunc)&0x10)&&(*(tcdata->fd->specfunc)&0x20))
				{
					tclc1 = tcdata->td->channel->ChannelList[i].ChannelId;
					if(((5<=tclc1)&&(tclc1<=8)) || ((9<=tclc1)&&(tclc1<=12)))
						continue;
				}
				if ((*(tcdata->fd->specfunc)&0x10)&&(!(*(tcdata->fd->specfunc)&0x20)))
				{
					tclc1 = tcdata->td->channel->ChannelList[i].ChannelId;
					if ((5 <= tclc1) && (tclc1 <= 8))
						continue;
				}
				if ((*(tcdata->fd->specfunc)&0x20)&&(!(*(tcdata->fd->specfunc)&0x10)))
				{
					tclc1 = tcdata->td->channel->ChannelList[i].ChannelId;
					if ((9 <= tclc1) && (tclc1 <= 12))
						continue;
				}
                #endif
				pphdetect->chans |= (0x00000001 << ((tcdata->td->channel->ChannelList[i].ChannelId) - 1)); 
				continue;
			}
		}

		for (i = 0; i < (tcdata->td->phase->FactPhaseNum); i++)
		{//for (i = 0; i < (dcdata->td->phase->FactPhaseNum); i++)
			if (0 == (tcdata->td->phase->PhaseList[i].PhaseId))
				break;
			if (tphid == (tcdata->td->phase->PhaseList[i].PhaseId))
			{
				pphdetect->phasetype = tcdata->td->phase->PhaseList[i].PhaseType;
				pphdetect->fixgtime = tcdata->td->phase->PhaseList[i].PhaseFixGreen;
				pphdetect->mingtime = tcdata->td->phase->PhaseList[i].PhaseMinGreen;
				pphdetect->maxgtime = tcdata->td->phase->PhaseList[i].PhaseMaxGreen1;
				pphdetect->gftime = tcdata->td->phase->PhaseList[i].PhaseGreenFlash;
				pphdetect->gtime -= pphdetect->gftime;//green time is not include green flash
				pphdetect->pgtime = tcdata->td->phase->PhaseList[i].PhaseWalkGreen;
				pphdetect->pctime = tcdata->td->phase->PhaseList[i].PhaseWalkClear;
				pphdetect->vmingtime = pphdetect->mingtime + pphdetect->ytime - pphdetect->startdelay;
				pphdetect->vmaxgtime = pphdetect->maxgtime + pphdetect->ytime - pphdetect->startdelay; 
				memset(pphdetect->socdetectpro,0,sizeof(pphdetect->socdetectpro));
				dpro = pphdetect->socdetectpro;
				for (z = 0; z < (tcdata->td->detector->FactDetectorNum); z++)
				{//1
					if (0 == (tcdata->td->detector->DetectorList[z].DetectorId))
						break;
					if ((pphdetect->phaseid) == (tcdata->td->detector->DetectorList[z].DetectorPhase))
					{
						temp = tcdata->td->detector->DetectorList[z].DetectorSpecFunc;
						if (0x00 == (temp & 0x02)) //It is not a key car road;
                        	continue;

						dpro->deteid = tcdata->td->detector->DetectorList[z].DetectorId;
						dpro->icarnum = 0;
						dpro->ocarnum = 0;
						dpro->fullflow = tcdata->td->detector->DetectorList[z].DetectorFlow;
						dpro->maxocctime = 0;
						dpro->aveocctime = 0;
						dpro->fulldegree = 0.0;

						dpro++;
					}
				}//1
				break;
			}//if (tphid == (sadata->td->phase->PhaseList[i].PhaseId))
		}//for (i = 0; i < (dcdata->td->phase->FactPhaseNum); i++)
		pphdetect++;
	}//for (snum = 0; ;snum++)

	if (0 == ppmyes)
    {//monitor pending pipe or check the valid or invalid of detector when do not have detect control;
        memset(&mpphase,0,sizeof(socmonpendphase_t));
        mpphase.pendpipe = tcdata->fd->pendpipe;
        mpphase.detector = tcdata->td->detector;
        int ppmret = pthread_create(&ppmid,NULL,(void *)soc_monitor_pending_pipe,&mpphase);
        if (0 != ppmret)
        {
        #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
            soc_end_part_child_thread();
            return;
        }
        ppmyes = 1;
    }//monitor pending pipe or check the valid or invalid of detector when do not have detect control;
//data send of backup pattern control
	if (SUCCESS != soc_backup_pattern_data(*(tcdata->fd->bbserial),snum,phdetect))
	{
	#ifdef TIMEING_DEBUG
		printf("backup_pattern_data call err,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
	}
	//end data send
	slnum = *(tcdata->fd->slnum);
	*(tcdata->fd->stageid) = tcdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
	if (0 == (tcdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId))
	{
		slnum = 0;
		*(tcdata->fd->slnum) = slnum;
		*(tcdata->fd->stageid) = tcdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
	}

	unsigned char				cycarr = 0;
	socpinfo_t					*pinfo = tcdata->pi;
	socpcdata_t					pcdata;
	struct timeval				tctout;
	unsigned char				fbdata[6] = {0};
	unsigned char				concyc = 0;
	fbdata[0] = 0xC1;
	fbdata[5] = 0xED;
//	unsigned char				cycend[16] = "CYTGCYCLEENDEND";

	unsigned int				lefttime = 0;
	struct timeval				tv;
	struct timeval				tel;
	long						ct = 0;
	unsigned char				endahead = 0;
	unsigned char				c1gt = 0;
	unsigned char				c1gmt = 0;
	unsigned char				c2gt = 0;
	unsigned char				c2gmt = 0;
	unsigned char				endcyclen = 0;
	unsigned char				twocycend = 0;
	unsigned char				onecycend = 0;

	unsigned char				offset = 0;
	unsigned char				twocycadj = 0;
	unsigned char				onecycadj = 0;
	unsigned char				add = 0;
	unsigned char				minus = 0;
	unsigned char				adjcyclen = 0;
	unsigned char				phaseadj = 0;
	unsigned char				c1gat = 0;
	unsigned char				c1gamt = 0;
	unsigned char				c2gat = 0;
	unsigned char				c2gamt = 0;
	unsigned char				rft = 0;
	unsigned char				sibuf[64] = {0};
//	statusinfo_t				sinfo;
	unsigned char				*csta = NULL;
	unsigned char				tcsta = 0;
//	memset(&sinfo,0,sizeof(statusinfo_t));
	sinfo.conmode = *(tcdata->fd->contmode);
	sinfo.pattern = *(tcdata->fd->patternid);
	sinfo.cyclet = *(tcdata->fd->cyclet);

	unsigned char           downti[8] = {0xA6,0,0,0,0,0,0,0xED};
    unsigned char           edownti[3] = {0xA5,0xA5,0xED};
	unsigned int			tchans = 0;
	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
	printf("sinfo.cyclet: %d,contmode:%d,File: %s,Line: %d,****************\n", \
			sinfo.cyclet,*(tcdata->fd->contmode),__FILE__,__LINE__);
	#endif

	if ((0 != *(tcdata->fd->coordphase))&&(0 == phaseadj)&&(0 == endahead))
	{
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		gettimeofday(&tv,NULL);
		ct = tv.tv_sec;	
		offset = (ct - (tcdata->fd->st)) % ((long)(*(tcdata->fd->cyclet)));
		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
		printf("cyclet: %d,File:%s,Line: %d\n",*(tcdata->fd->cyclet),__FILE__,__LINE__);
		printf("***st:%d,ct:%d,offset: %d,File: %s,Line: %d\n",tcdata->fd->st,ct,offset,__FILE__,__LINE__);	
		#endif
		phaseadj = 1;
		if (offset >= (*(tcdata->fd->cyclet)/2))
		{
			add = 1;
			minus = 0;
			offset = *(tcdata->fd->cyclet) - offset;
			if (offset >= (*(tcdata->fd->cyclet)/4))
			{
				twocycadj = 1;
				onecycadj = 0;
				c1gat = (offset/2 + offset%2)/snum;
				c1gamt = (offset/2 + offset%2)%snum;
				c2gat = (offset/2)/snum;
				c2gamt = (offset/2)%snum;
			}
			else
			{
				twocycadj = 0;
				onecycadj = 1;
				c2gat = 0;
				c2gamt = 0;
				c1gat = offset/snum;
				c1gamt = offset%snum;
			}
		}
		else
		{
			add = 0;
			minus = 1;
			if (offset >= (*(tcdata->fd->cyclet)/4))
			{//use two cycles to minus
				twocycadj = 1;
				onecycadj = 0;
				c1gat = (offset/2 + offset%2)/snum;
				c1gamt = (offset/2 + offset%2)%snum;
				c2gat = (offset/2)/snum;
				c2gamt = (offset/2)%snum;
			}//use two cycles to minus
			else
			{//use one cycle to minus
				twocycadj = 0;
				onecycadj = 1;
				c2gat = 0;
				c2gamt = 0;
				c1gat = offset/snum;
				c1gamt = offset%snum;
			}//use one cycle to minus
		}
	}//if ((4 == *(tcdata->fd->contmode)) && (0 != *(tcdata->fd->coordphase)))

	if ((1==phaseadj)&&(0!=*(tcdata->fd->coordphase))&&(0==endahead))
	{
		adjcyclen += 1;
		cycarr = 0;
	}

	if (*(tcdata->fd->auxfunc) & 0x01)
	{//if (*(tcdata->fd->auxfunc) & 0x01)
		if ((30 != *(tcdata->fd->contmode)) && (31 != *(tcdata->fd->contmode)))
		{
			for (i = 0;i < MAX_PHASE_LINE;i++)
			{
				if (0 == phdetect[i].stageid)
					break;
				if (*(tcdata->fd->stageid) == phdetect[i].stageid)
				{
					break;
				}
			}
			for (j = 1; j < (MAX_CHANNEL+1); j++)
			{
				if (phdetect[i].chans & (0x00000001 << (j -1)))
					continue;
				for (z = 0; z < MAX_PHASE_LINE; z++)
				{
					if (0 == phdetect[z].stageid)
						break;
					if (z == i)
						continue;
					downti[6] = 0;
					if (phdetect[z].chans & (0x00000001 << (j - 1)))
					{
						k = i;
						if (z > i)
						{
							while (k != z)
							{
								downti[6] += \
								phdetect[k].gtime+phdetect[k].gftime+phdetect[k].ytime+phdetect[k].rtime;

								if((0!=*(tcdata->fd->coordphase))&&(1==phaseadj))
								{
									if (0 == endahead)
									{
										if (1 == adjcyclen)
										{
											if ((1 == add) && (0 == minus))
											{
												downti[6] += c1gat;
											}
											if ((0 == add) && (1 == minus))
											{
												downti[6] -= c1gat;
											}
										}
										if (2 == adjcyclen)
										{
											if ((1 == add) && (0 == minus))
											{
												downti[6] += c2gat;
											}
											if ((0 == add) && (1 == minus))
											{
												downti[6] -= c2gat;
											}
										}			
									}
								}//if((4=->fd->contmode))&&(0!=*(tcdata->fd->coordphase))&&(1==phaseadj))
								k++;
							}
						}
						if (z < i)
						{
							while (k != z)
							{
								if (0 == phdetect[k].stageid)
								{
									if((0!=*(tcdata->fd->coordphase))&&(1==phaseadj))
									{
										if (0 == endahead)
										{
											if (1 == adjcyclen)
											{
												if ((1 == add) && (0 == minus))
												{
													downti[6] += c1gamt;
												}
												if ((0 == add) && (1 == minus))
												{
													downti[6] -= c1gamt;
												}
											}
											if (2 == adjcyclen)
											{
												if ((1 == add) && (0 == minus))
												{
													downti[6] += c2gamt;
												}
												if ((0 == add) && (1 == minus))
												{
													downti[6] -= c2gamt;
												}
											}			
										}
									}//if((4==*(tcdata->fd->contmode))&&(0!=*(tcdata->fd->coordphase))&&
									k = 0;
									continue;
								}
								
								downti[6] += \
								phdetect[k].gtime+phdetect[k].gftime+phdetect[k].ytime+phdetect[k].rtime;
								
								if((0!=*(tcdata->fd->coordphase))&&(1==phaseadj))
								{
									if (0 == endahead)
									{
										if (1 == adjcyclen)
										{
											if ((1 == add) && (0 == minus))
											{
												downti[6] += c1gat;
											}
											if ((0 == add) && (1 == minus))
											{
												downti[6] -= c1gat;
											}
										}
										if (2 == adjcyclen)
										{
											if ((1 == add) && (0 == minus))
											{
												downti[6] += c2gat;
											}
											if ((0 == add) && (1 == minus))
											{
												downti[6] -= c2gat;
											}
										}			
									}
								}//if((4==*(>contmode))&&(0!=*(tcdata->fd->coordphase))&&(1==phaseadj))
								k++;
							}
						}
						tchans = 0;
						tchans |= (0x00000001 << (j -1));
						downti[1] = 0;
						downti[1] |= (tchans & 0x000000ff);
						downti[2] = 0;
						downti[2] |= (((tchans & 0x0000ff00) >> 8) & 0x000000ff);
						downti[3] = 0;
						downti[3] |= (((tchans & 0x00ff0000) >> 16) & 0x000000ff);
						downti[4] = 0;
						downti[4] |= (((tchans & 0xff000000) >> 24) & 0x000000ff);
						downti[5] = 0;//red color
						if (!wait_write_serial(*(tcdata->fd->bbserial)))
						{
							if (write(*(tcdata->fd->bbserial),downti,sizeof(downti)) < 0)
							{
							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
								printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}
						}
						else
						{
						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
							printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
						break;
					}//if (phdetect[z].chans & (0x00000001 << (j - 1)))
				}//for (z = 0; z < MAX_PHASE_LINE; z++)
			}//for (j = 1; j < (MAX_CHANNEL+1); j++)
		}//if ((30 != *(tcdata->fd->contmode)) && (31 != *(tcdata->fd->contmode)))
	}//if (*(tcdata->fd->auxfunc) & 0x01)

	//added on 20150728 
    optidata_t                  odc;//added on 20150728
    memset(&odc,0,sizeof(optidata_t));
    odc.patternid = *(tcdata->fd->patternid);
    odc.phasenum = 0;
    odc.cycle = *(tcdata->fd->cyclet);
    odc.opticycle = 0;       
    odc.markbit = tcdata->fd->markbit;
    odc.markbit2 = tcdata->fd->markbit2;
    odc.reflags = tcdata->fd->reflags;
    //end add

	dunhua_close_lamp(tcdata->fd,tcdata->cs);

	while (1)
	{//while loop
		if (1 == cycarr)
		{//one cycle arrived
			cycarr = 0;

			if (*(tcdata->fd->markbit) & 0x0001)
			{//end time of current pattern has arrived
				unsigned char			syndata[3] = {0xCC,0xDD,0xED};
				#ifdef RED_FLASH
                sleep(1);//wait "rfpid" thread release resource; 
                #endif
				if (!wait_write_serial(*(tcdata->fd->synpipe)))
				{
					if (write(*(tcdata->fd->synpipe),syndata,sizeof(syndata)) < 0)
					{
					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						soc_end_part_child_thread();
						return;
					}
				}
				else
				{
				#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
					printf("the pipe cannot be written,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
					soc_end_part_child_thread();
					return;
				}
				//Note,0bit is clean '0' by main module;
				sleep(5); //wait main module end own;
			}//end time of current pattern has arrived

			//send end mark of cycle to configure tool	

#if 0
			if (write(*(tcdata->fd->sockfd->ssockfd),cycend,sizeof(cycend)) < 0)
    		{
    		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
        		printf("send data to configure tool error,File: %s,Line: %d\n",__FILE__,__LINE__);
    		#endif
    		}			
#endif	
			if ((*(tcdata->fd->markbit) & 0x0100) && (0 == endahead))
			{//current pattern prepare to transit ahead two cycles;
				//Note,8bit is clean '0' by main module;
				if (0 != *(tcdata->fd->ncoordphase))
				{//next coordphase is not 0
					endahead = 1;
					endcyclen = 0;
					phaseadj = 0; //last two cycles,do not need phase offset adjust;
					lefttime = 0;
                    ct = 0;
                    tv.tv_sec = 0;
                    tv.tv_usec = 0;
                    gettimeofday(&tv,NULL);
                    ct = tv.tv_sec;
					lefttime = (unsigned int)((tcdata->fd->nst) - ct);

					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
					printf("********************lefttime: %d,File: %s,Line: %d\n",lefttime,__FILE__,__LINE__);
					#endif
					//greenflash/yellow/red is default 3/3/0 respectively;
					if (lefttime >= (*(tcdata->fd->cyclet)*3)/2)
					{//use two cycles to end current pattern;
						c1gt = (lefttime/2 + lefttime%2)/snum - 3 -3;
						c1gmt = (lefttime/2 + lefttime%2)%snum;
						c2gt = (lefttime/2)/snum - 3 - 3;
						c2gmt = (lefttime/2)%snum;
						twocycend = 1;
						onecycend = 0;
					}//use two cycles to end current pattern;
					else
					{//use one cycle to end current pattern
						c2gmt = 0;
						c2gt = 0;
						c1gt = lefttime/snum - 3 -3;
						c1gmt = lefttime%snum;
						twocycend = 0;
						onecycend = 1;
					}//use one cycle to end current pattern
					//greenflash/yellow/red is default 3/3/0 respectively;
				#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
				printf("c1gt:%d,c1gmt:%d,c2gt:%d,c2gmt:%d,twocycend:%d,onecycend:%d,Line:%d\n", \
					c1gt,c1gmt,c2gt,c2gmt,twocycend,onecycend,__LINE__);
				#endif
				}//next coordphase is not 0;
			}//current pattern prepare to transit ahead two cycles;
			if (1 == endahead)
			{
				endcyclen += 1;
				if (1 == endcyclen)
				{
					sinfo.cyclet = (c1gt + 3 + 3)*snum + c1gmt;
				}	
				if (3 == endcyclen)
				{
					endcyclen = 2;
					tctout.tv_sec = 0;
					tctout.tv_usec = 200000;
					select(0,NULL,NULL,NULL,&tctout);
					cycarr = 1;
					continue;
				}
				if (2 == endcyclen)
				{
					if ((1 == onecycend) && (0 == twocycend))
					{
						endcyclen = 2;
						tctout.tv_sec = 0;
						tctout.tv_usec = 200000;
						select(0,NULL,NULL,NULL,&tctout);
						cycarr = 1;
						continue;
					}
					sinfo.cyclet = (c2gt + 3 + 3)*snum + c2gmt;
				}
				if (*(tcdata->fd->auxfunc) & 0x01)
				{//if (*(tcdata->fd->auxfunc) & 0x01)
					if ((30 != *(tcdata->fd->contmode)) && (31 != *(tcdata->fd->contmode)))
					{
						for (i = 0;i < MAX_PHASE_LINE;i++)
						{
							if (0 == phdetect[i].stageid)
								break;
							if (*(tcdata->fd->stageid) == phdetect[i].stageid)
							{
								break;
							}
						}
						for (j = 1; j < (MAX_CHANNEL+1); j++)
						{
							if (phdetect[i].chans & (0x00000001 << (j -1)))
								continue;
							for (z = 0; z < MAX_PHASE_LINE; z++)
							{
								if (0 == phdetect[z].stageid)
									break;
								if (z == i)
									continue;
								downti[6] = 0;
								if (phdetect[z].chans & (0x00000001 << (j - 1)))
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
											if (0 == phdetect[k].stageid)
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
									if (!wait_write_serial(*(tcdata->fd->bbserial)))
									{
										if (write(*(tcdata->fd->bbserial),downti,sizeof(downti)) < 0)
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
									break;
								}//if (phdetect[z].chans & (0x00000001 << (j - 1)))
							}//for (z = 0; z < MAX_PHASE_LINE; z++)
						}//for (j = 1; j < (MAX_CHANNEL+1); j++)
					}//if ((30 != *(sadata->fd->contmode)) && (31 != *(sadata->fd->contmode)))
				}//if (*(tcdata->fd->auxfunc) & 0x01)
			}//if (1 == endahead)
			if((0!=*(tcdata->fd->coordphase))&&(0==phaseadj)&&(0==endahead))
			{//phase offset adjust;
				adjcyclen = 0;
				phaseadj = 1;
				tv.tv_sec = 0;
        		tv.tv_usec = 0;

        		gettimeofday(&tv,NULL);
        		ct = tv.tv_sec;
				offset = (ct - (tcdata->fd->st)) % ((long)(*(tcdata->fd->cyclet)));
				#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
				printf("cyclet: %d,Line: %d\n",*(tcdata->fd->cyclet),__LINE__);
        		printf("st:%d,ct:%d,offset:%d,File:%s,Line:%d\n",tcdata->fd->st,ct,offset,__FILE__,__LINE__);
        		#endif
				if (offset >= (*(tcdata->fd->cyclet)/2))
				{
					add = 1;
            		minus = 0;
            		offset = *(tcdata->fd->cyclet) - offset;
            		if (offset >= (*(tcdata->fd->cyclet)/4))
            		{//use two cycles to add
                		twocycadj = 1;
                		onecycadj = 0;
                		c1gat = (offset/2 + offset%2)/snum;
                		c1gamt = (offset/2 + offset%2)%snum;
                		c2gat = (offset/2)/snum;
                		c2gamt = (offset/2)%snum;
            		}//use two cycles to add
            		else
            		{//use one cycle to add
                		twocycadj = 0;
                		onecycadj = 1;
                		c2gat = 0;
                		c2gamt = 0;
                		c1gat = offset/snum;
                		c1gamt = offset%snum;
            		}//use one cycle to add
				}
				else
				{
					add = 0;
            		minus = 1;
            		if (offset >= (*(tcdata->fd->cyclet)/4))
            		{//use two cycles to minus
                		twocycadj = 1;
                		onecycadj = 0;
                		c1gat = (offset/2 + offset%2)/snum;
                		c1gamt = (offset/2 + offset%2)%snum;
                		c2gat = (offset/2)/snum;
                		c2gamt = (offset/2)%snum;
            		}//use two cycles to minus
            		else
            		{//use one cycle to minus
                		twocycadj = 0;
                		onecycadj = 1;
                		c2gat = 0;
                		c2gamt = 0;
                		c1gat = offset/snum;
                		c1gamt = offset%snum;
            		}//use one cycle to minus
				}
			}//phase offset adjust;
			if((1==phaseadj)&&(0!=*(tcdata->fd->coordphase))&&(0==endahead))
			{
				adjcyclen += 1;
				if (1 == adjcyclen)
				{
					if ((1 == add) && (0 == minus))
					{
						sinfo.cyclet = *(tcdata->fd->cyclet) + c1gat + c1gamt;
					}
					if ((0 == add) && (1 == minus))
					{
						sinfo.cyclet = *(tcdata->fd->cyclet) - c1gat - c1gamt;
					}
				}
				if (3 == adjcyclen)
				{
					adjcyclen = 0;
					phaseadj = 0;
				}
				if (2 == adjcyclen)
				{
					if ((0 == twocycadj) && (1 == onecycadj))
					{
						adjcyclen = 0;
						phaseadj = 0;
					}
					if ((1 == twocycadj) && (0 == onecycadj))
					{
						if ((1 == add) && (0 == minus))
                    	{
							sinfo.cyclet = *(tcdata->fd->cyclet) + c2gat + c2gamt;
                    	}
                    	if ((0 == add) && (1 == minus))
                    	{
							sinfo.cyclet = *(tcdata->fd->cyclet) - c2gat - c2gamt;
                    	}
					}
				}

				if (*(tcdata->fd->auxfunc) & 0x01)
				{//if (*(tcdata->fd->auxfunc) & 0x01)
					if ((30 != *(tcdata->fd->contmode)) && (31 != *(tcdata->fd->contmode)))
					{
						for (i = 0;i < MAX_PHASE_LINE;i++)
						{
							if (0 == phdetect[i].stageid)
								break;
							if (*(tcdata->fd->stageid) == phdetect[i].stageid)
							{
								break;
							}
						}
						for (j = 1; j < (MAX_CHANNEL+1); j++)
						{
							if (phdetect[i].chans & (0x00000001 << (j -1)))
								continue;
							for (z = 0; z < MAX_PHASE_LINE; z++)
							{
								if (0 == phdetect[z].stageid)
									break;
								if (z == i)
									continue;
								downti[6] = 0;
								if (phdetect[z].chans & (0x00000001 << (j - 1)))
								{
									k = i;
									if (z > i)
									{
										while (k != z)
										{
											
											downti[6] += \
												phdetect[k].gtime + phdetect[k].gftime + \
												phdetect[k].ytime + phdetect[k].rtime;
											
											if (1 == adjcyclen)
											{
												if ((1 == add) && (0 == minus))
												{
													downti[6] += c1gat;
												}
												if ((0 == add) && (1 == minus))
												{
													downti[6] -= c1gat;
												}
											}
											if (2 == adjcyclen)
											{
												if ((1 == add) && (0 == minus))
												{
													downti[6] += c2gat;
												}
												if ((0 == add) && (1 == minus))
												{
													downti[6] -= c2gat;
												}
											}			
											k++;
										}
									}
									if (z < i)
									{
										while (k != z)
										{
											if (0 == phdetect[k].stageid)
											{
												if (1 == adjcyclen)
												{
													if ((1 == add) && (0 == minus))
													{
														downti[6] += c1gamt;
													}
													if ((0 == add) && (1 == minus))
													{
														downti[6] -= c1gamt;
													}
												}
												if (2 == adjcyclen)
												{
													if ((1 == add) && (0 == minus))
													{
														downti[6] += c2gamt;
													}
													if ((0 == add) && (1 == minus))
													{
														downti[6] -= c2gamt;
													}
												}			
												k = 0;
												continue;
											}
											
											downti[6] += \
												phdetect[k].gtime + phdetect[k].gftime + \
												phdetect[k].ytime + phdetect[k].rtime;
											
											if (1 == adjcyclen)
											{
												if ((1 == add) && (0 == minus))
												{
													downti[6] += c1gat;
												}
												if ((0 == add) && (1 == minus))
												{
													downti[6] -= c1gat;
												}
											}
											if (2 == adjcyclen)
											{
												if ((1 == add) && (0 == minus))
												{
													downti[6] += c2gat;
												}
												if ((0 == add) && (1 == minus))
												{
													downti[6] -= c2gat;
												}
											}				
											k++;
										}
									}
									tchans = 0;
									tchans |= (0x00000001 << (j -1));
									downti[1] = 0;
									downti[1] |= (tchans & 0x000000ff);
									downti[2] = 0;
									downti[2] |= (((tchans & 0x0000ff00) >> 8) & 0x000000ff);
									downti[3] = 0;
									downti[3] |= (((tchans & 0x00ff0000) >> 16) & 0x000000ff);
									downti[4] = 0;
									downti[4] |= (((tchans & 0xff000000) >> 24) & 0x000000ff);
									downti[5] = 0;//red color
									if (!wait_write_serial(*(tcdata->fd->bbserial)))
									{
										if (write(*(tcdata->fd->bbserial),downti,sizeof(downti)) < 0)
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
									break;
								}//if (phdetect[z].chans & (0x00000001 << (j - 1)))
							}//for (z = 0; z < MAX_PHASE_LINE; z++)
						}//for (j = 1; j < (MAX_CHANNEL+1); j++)
					}//if ((30 != *(tcdata->fd->contmode)) && (31 != *(tcdata->fd->contmode)))
				}//if (*(tcdata->fd->auxfunc) & 0x01)
			}//if ((1 == phaseadj) && (4 == *(tcdata->fd->contmode)) && (0 != *(tcdata->fd->coordphase)))
			if (*(tcdata->fd->reflags) & 0x10)
			{
				odc.opticycle = sinfo.cyclet;
				if (SUCCESS != soc_send_traffic_para(1,tcdata->fd->sockfd->csockfd,&odc,phdetect))
				{
				#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
					printf("soc_send_traffic_para call err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
	//              odc.opticycle = sinfo.cyclet;
			}	
		}//one cycle arrived

		memset(pinfo,0,sizeof(socpinfo_t));
		if (SUCCESS != soc_get_phase_info(tcdata->fd,tcdata->td,tcline,slnum,pinfo))	
		{
		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
			printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			soc_end_part_child_thread();
			return;
		}
	
		curpid = pinfo->phaseid;	
		*(tcdata->fd->phaseid) = 0;
		*(tcdata->fd->phaseid) |= (0x01 << (pinfo->phaseid - 1));
		*(tcdata->fd->markbit) &= 0xfbff;
		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
		printf("Begin pass phase: %d,File: %s,Line: %d\n",pinfo->phaseid,__FILE__,__LINE__);
		#endif
		*(tcdata->fd->color) = 0x02;
		sinfo.stage = *(tcdata->fd->stageid);
        sinfo.phase = *(tcdata->fd->phaseid);
		if (SUCCESS != soc_set_lamp_color(*(tcdata->fd->bbserial),pinfo->chan,0x02))
		{
		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			gettimeofday(&tel,NULL);
			update_event_list(tcdata->fd->ec,tcdata->fd->el,1,15,tel.tv_sec,tcdata->fd->markbit);
            if (SUCCESS != generate_event_file(tcdata->fd->ec,tcdata->fd->el, \
												tcdata->fd->softevent,tcdata->fd->markbit))
            {
            #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
			*(tcdata->fd->markbit) |= 0x0800;
		}
		memset(&gtime,0,sizeof(gtime));
		gettimeofday(&gtime,NULL);
		memset(&gftime,0,sizeof(gftime));
		memset(&ytime,0,sizeof(ytime));
		memset(&rtime,0,sizeof(rtime));

		if (SUCCESS!=update_channel_status(tcdata->fd->sockfd,tcdata->cs,pinfo->chan,0x02,tcdata->fd->markbit))
		{
		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
			printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
		#endif
		}

		if (0 == pinfo->cchan[0])
		{//only send one phase status,time is green+greenflash+yellow+red
			if ((!(*(tcdata->fd->markbit) & 0x1000)) && (!(*(tcdata->fd->markbit) & 0x8000)))
			{//actively report is not probitted and connect successfully
				if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
        		{//if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
					if (c1gt > 0)
					{//if (c1gt > 0)
           				if (1 == endcyclen)
           				{
               				if (snum == (slnum + 1))
               				{
                   				sinfo.time = c1gt + c1gmt + 3 + 3;//3sec green flash and 3sec yellow
               				}//last a phase of stage
               				else
               				{
                   				sinfo.time = c1gt + 3 + 3;
               				}
           				}
           				if (2 == endcyclen)
           				{
               				if (snum == (slnum + 1))
               				{
                   				sinfo.time = c2gt + c2gmt + 3 + 3;
               				}
               				else
               				{
                   				sinfo.time = c2gt + 3 + 3;
               				}
           				}
					}//if (c1gt > 0)
       			}//if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
				else
				{
					if ((0 != *(tcdata->fd->coordphase)) && (1 == phaseadj))
					{//phase offset adjust
						if (1 == adjcyclen)
						{//1th adjust
							if ((1 == add) && (0 == minus))
							{
								if (snum == (slnum + 1))
								{
									sinfo.time = pinfo->gtime + c1gat + c1gamt + pinfo->gftime + \
												pinfo->ytime + pinfo->rtime;
								}
								else
								{
									sinfo.time = pinfo->gtime + c1gat + pinfo->gftime + \
												pinfo->ytime + pinfo->rtime;
								}
							}
							if ((0 == add) && (1 == minus))
							{
								if (snum == (slnum + 1))
								{
									sinfo.time = pinfo->gtime + pinfo->gftime + pinfo->ytime + pinfo->rtime - \
												c1gat - c1gamt;
								}
								else
								{
									sinfo.time = pinfo->gtime + pinfo->gftime + pinfo->ytime + \
												pinfo->rtime - c1gat;
								}
							}
						}//1th adjust
						if (2 == adjcyclen)
						{//2th adjust
							if ((1 == add) && (0 == minus))
							{
								if (snum == (slnum + 1))
								{
									sinfo.time = pinfo->gtime + c2gat + c2gamt + pinfo->gftime + \
												pinfo->ytime + pinfo->rtime;
								}
								else
								{
									sinfo.time = pinfo->gtime + c2gat + pinfo->gftime + \
												pinfo->ytime + pinfo->rtime;
								}
							}
							if ((0 == add) && (1 == minus))
							{
								if (snum == (slnum + 1))
                       			{
									sinfo.time = pinfo->gtime + pinfo->gftime + pinfo->ytime + \
												pinfo->rtime - c2gat - c2gamt;
                       			}
                       			else
                       			{
									sinfo.time = pinfo->gtime + pinfo->gftime + pinfo->ytime + \
												pinfo->rtime- c2gat;
                       			}
							}
						}//2th adjust
					}//phase offset adjust
					else
					{//normal phase pass time
						sinfo.time = pinfo->gtime + pinfo->gftime + pinfo->ytime + pinfo->rtime;
					}//normal phase pass time
				}
				sinfo.conmode = *(tcdata->fd->contmode);//added on 20150529
				sinfo.color = 0x02;
				sinfo.chans = 0;
				memset(sinfo.csta,0,sizeof(sinfo.csta));
				csta = sinfo.csta;
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == pinfo->chan[i])
						break;
					sinfo.chans += 1;
					tcsta = pinfo->chan[i];
					tcsta <<= 2;
					tcsta &= 0xfc;
					tcsta |= 0x02;
					*csta = tcsta;
					csta++;
				}
				memcpy(tcdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
				memset(sibuf,0,sizeof(sibuf));
				if (SUCCESS != status_info_report(sibuf,&sinfo))	
				{
				#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
					printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				else
            	{
                	write(*(tcdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            	}
			}//actively report is not probitted and connect successfully
		}//only send one phase status,time is green+greenflash+yellow+red
		else
		{//0 != pinfo->cchan[0]
			if ((!(*(tcdata->fd->markbit) & 0x1000)) && (!(*(tcdata->fd->markbit) & 0x8000)))
			{//actively report is not probitted and connect successfully
				if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
        		{//if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
					if (c1gt > 0)
					{//if (c1gt > 0)
           				if (1 == endcyclen)
           				{
               				if (snum == (slnum + 1))
               				{
                   				sinfo.time = c1gt + c1gmt + 3;
               				}//last a phase of stage
               				else
               				{
                   				sinfo.time = c1gt + 3;
               				}
           				}
           				if (2 == endcyclen)
           				{
               				if (snum == (slnum + 1))
               				{
                   				sinfo.time = c2gt + c2gmt + 3;
               				}
               				else
               				{
                   				sinfo.time = c2gt + 3;
               				}
           				}
					}//if (c1gt > 0)
       			}//if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
				else
				{
					if ((0 != *(tcdata->fd->coordphase)) && (1 == phaseadj))
					{//phase offset adjust
						if (1 == adjcyclen)
						{//1th adjust
							if ((1 == add) && (0 == minus))
							{
								if (snum == (slnum + 1))
								{
									sinfo.time = pinfo->gtime + c1gat + c1gamt + pinfo->gftime;
								}
								else
								{
									sinfo.time = pinfo->gtime + c1gat + pinfo->gftime;
								}
							}
							if ((0 == add) && (1 == minus))
							{
								if (snum == (slnum + 1))
								{
									sinfo.time = pinfo->gtime - c1gat - c1gamt + pinfo->gftime;
								}
								else
								{
									sinfo.time = pinfo->gtime - c1gat + pinfo->gftime;
								}
							}
						}//1th adjust
						if (2 == adjcyclen)
						{//2th adjust
							if ((1 == add) && (0 == minus))
							{
								if (snum == (slnum + 1))
								{
									sinfo.time = pinfo->gtime + c2gat + c2gamt + pinfo->gftime;
								}
								else
								{
									sinfo.time = pinfo->gtime + c2gat + pinfo->gftime;
								}
							}
							if ((0 == add) && (1 == minus))
							{
								if (snum == (slnum + 1))
                       			{
									sinfo.time = pinfo->gtime - c2gat - c2gamt + pinfo->gftime;
                       			}
                       			else
                       			{
									sinfo.time = pinfo->gtime - c2gat + pinfo->gftime;
                       			}
							}
						}//2th adjust
					}//phase offset adjust
					else
					{//normal phase pass time
						sinfo.time = pinfo->gtime + pinfo->gftime;
					}//normal phase pass time
				}
				sinfo.conmode = *(tcdata->fd->contmode);//added on 20150529
				sinfo.color = 0x02;
				sinfo.chans = 0;
				memset(sinfo.csta,0,sizeof(sinfo.csta));
				csta = sinfo.csta;
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == pinfo->chan[i])
						break;
					sinfo.chans += 1;
					tcsta = pinfo->chan[i];
					tcsta <<= 2;
					tcsta &= 0xfc;
					tcsta |= 0x02;
					*csta = tcsta;
					csta++;
				}
				memcpy(tcdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
				memset(sibuf,0,sizeof(sibuf));
				if (SUCCESS != status_info_report(sibuf,&sinfo))	
				{
				#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
					printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				else
            	{
				#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
					printf("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x,Line:%d\n", \
							sibuf[0],sibuf[1],sibuf[2],sibuf[3],sibuf[4],sibuf[5],sibuf[6],sibuf[7], \
							sibuf[8],__LINE__);
				#endif
                	write(*(tcdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            	}
			}//actively report is not probitted and connect successfully
		}//0 != pinfo->cchan[0]
		
		if (*(tcdata->fd->auxfunc) & 0x01)
		{//if (*(tcdata->fd->auxfunc) & 0x01)
			unsigned char			jishu = 0;
			if ((30 != *(tcdata->fd->contmode)) && (31 != *(tcdata->fd->contmode)))
			{
				for (j = 0; j < MAX_CHANNEL; j++)
				{//for (j = 0; j < MAX_CHANNEL; j++)
					if (0 == (pinfo->chan[j]))
						break;
					downti[6] = 0;
					jishu = 0;
					for (z = 0; z < MAX_PHASE_LINE; z++)
					{//for (z = 0; z < MAX_PHASE_LINE; z++)
						if (0 == phdetect[z].stageid)
							break;
						if (phdetect[z].stageid == (pinfo->stageline))
						{
							jishu += 1;
							if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
							{//the last two cycles
								if (1 == endcyclen)
								{
									downti[6] += c1gt + 3; 
								}
								if (2 == endcyclen)
								{
									downti[6] += c2gt + 3;
								}
							}//the last two cycles
							else
							{//not the last two cycles
								
								downti[6] += phdetect[z].gtime + phdetect[z].gftime;
								
								if((0!=*(tcdata->fd->coordphase))&&(1==phaseadj))
								{//phase offset adjust
									if (1 == adjcyclen)
									{
										if ((1 == add) && (0 == minus))
										{
											downti[6] += c1gat;	
										}
										if ((0 == add) && (1 == minus))
										{
											downti[6] -= c1gat;
										}
									}
									if (2 == adjcyclen)
									{
										if ((1 == add) && (0 == minus))
										{
											downti[6] += c2gat;
										}
										if ((0 == add) && (1 == minus))
										{
											downti[6] -= c2gat;
										}
									}	
								}//phase offset adjust
							}//not the last two cycles	
							break;
						}//if (phdetect[z].stageid == (pinfo->stageline))	
					}//for (z = 0; z < MAX_PHASE_LINE; z++)
					k = z + 1;
					s = z; //backup 'z';
					while (1)
					{
						if (0 == phdetect[k].stageid)
						{
							if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
							{
								if (1 == endcyclen)
								{
									downti[6] += c1gmt;
								}
								if (2 == endcyclen)
								{
									downti[6] += c2gmt;
								}
							}
							else
							{//not the last two cycles
								if((0!=*(tcdata->fd->coordphase))&&(1==phaseadj))
								{//phase offset adjust
									if (1 == adjcyclen)
									{
										if ((1 == add) && (0 == minus))
										{
											downti[6] += c1gamt;
										}
										if ((0 == add) && (1 == minus))
										{
											downti[6] -= c1gamt;
										}
									}
									if (2 == adjcyclen)
									{
										if ((1 == add) && (0 == minus))
										{
											downti[6] += c2gamt;
										}
										if ((0 == add) && (1 == minus))
										{
											downti[6] -= c2gamt;
										}
									}
								}//phase offset adjust
							}//not the last two cycles	
							k = 0;
							continue;
						}
						if (k == s)
							break;
						if (phdetect[k].chans & (0x00000001 << (pinfo->chan[j] - 1)))
						{
							if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
							{	
								downti[6] += 3;//default 3 second yellow lamp
								if (1 == endcyclen)
									downti[6] += c1gt + 3;
								if (2 == endcyclen)
									downti[6] += c2gt + 3;
							}
							else
							{//not the last two cycles
								downti[6] += phdetect[z].ytime + phdetect[z].rtime;

								downti[6] += phdetect[k].gtime + phdetect[k].gftime;
								
								if((0!=*(tcdata->fd->coordphase))&&(1==phaseadj))
								{//phase offset adjust
									if (1 == adjcyclen)
									{
										if ((1 == add) && (0 == minus))
										{
											downti[6] += c1gat;
										}
										if ((0 == add) && (1 == minus))
										{
											downti[6] -= c1gat;
										}
									}
									if (2 == adjcyclen)
									{
										if ((1 == add) && (0 == minus))
										{
											downti[6] += c2gat;
										}
										if ((0 == add) && (1 == minus))
										{
											downti[6] -= c2gat;
										}
									}
								}//phase offset adjust
							}//not the last two cycles
							jishu += 1;
						}
						else
						{
							break;
						}
						z = k;
						k++;
					}
					if (snum == jishu)
					{
						downti[6] = 0xff;
					}//the channel is included in all phase;
					tchans = 0;
					tchans |= (0x00000001 << ((pinfo->chan[j])-1));
					downti[1] = 0;
					downti[1] |= (tchans & 0x000000ff);
					downti[2] = 0;
					downti[2] |= (((tchans & 0x0000ff00) >> 8) & 0x000000ff);
					downti[3] = 0;
					downti[3] |= (((tchans & 0x00ff0000) >> 16) & 0x000000ff);
					downti[4] = 0;
					downti[4] |= (((tchans & 0xff000000) >> 24) & 0x000000ff);
					downti[5] = 0x02;
					if (!wait_write_serial(*(tcdata->fd->bbserial)))
					{
						if (write(*(tcdata->fd->bbserial),downti,sizeof(downti)) < 0)
						{
						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
					}
					else
					{
					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
						printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
				}//for (j = 0; j < MAX_CHANNEL; j++)
				if (!wait_write_serial(*(tcdata->fd->bbserial)))
				{
					if (write(*(tcdata->fd->bbserial),edownti,sizeof(edownti)) < 0)
					{
					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
				}
				else
				{
				#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
					printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
			}
		}//if (*(tcdata->fd->auxfunc) & 0x01)

		if ((*(tcdata->fd->markbit) & 0x0002) && (*(tcdata->fd->markbit) & 0x0010))
		{
			memset(&timedown,0,sizeof(timedown));
			timedown.mode = *(tcdata->fd->contmode);
			timedown.pattern = *(tcdata->fd->patternid);
			timedown.lampcolor = 0x02;
			if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
			{
				if (1 == endcyclen)
				{
					if (snum == (slnum + 1))
					{
						timedown.lamptime = c1gt + c1gmt + 3;//default 3 sec green flash
					}//last a phase of stage
					else
					{
						timedown.lamptime = c1gt + 3;//default 3 sec green flash
					}
				}
				if (2 == endcyclen)
				{
					if (snum == (slnum + 1))
					{
						timedown.lamptime = c2gt + c2gmt + 3;//default 3 sec green flash
					}
					else
					{
						timedown.lamptime = c2gt + 3;//default 3 sec green flash
					}
				}
			}
			else
			{//not the last two cycles
				if ((0 != *(tcdata->fd->coordphase)) && (1 == phaseadj))
				{//phase offset adjust
					if (1 == adjcyclen)
					{//1th adjust
						if ((1 == add) && (0 == minus))
						{
							if (snum == (slnum + 1))
							{
								timedown.lamptime = pinfo->gtime + pinfo->gftime + c1gat + c1gamt;
							}
							else
							{
								timedown.lamptime = pinfo->gtime + pinfo->gftime + c1gat;
							}
						}
						if ((0 == add) && (1 == minus))
						{
							if (snum == (slnum + 1))
							{
								timedown.lamptime = pinfo->gtime + pinfo->gftime - c1gat - c1gamt;
							}
							else
							{
								timedown.lamptime = pinfo->gtime + pinfo->gftime - c1gat;
							}
						}
					}//1th adjust
					if (2 == adjcyclen)
					{//2th adjust
						if ((1 == add) && (0 == minus))
						{
							if (snum == (slnum + 1))
							{
								timedown.lamptime = pinfo->gtime + pinfo->gftime + c2gat + c2gamt;
							}
							else
							{
								timedown.lamptime = pinfo->gtime + pinfo->gftime + c2gat;
							}
						}
						if ((0 == add) && (1 == minus))
						{
							if (snum == (slnum + 1))
                            {
								timedown.lamptime = pinfo->gtime + pinfo->gftime - c2gat - c2gamt;
                            }
                            else
                            {
								timedown.lamptime = pinfo->gtime + pinfo->gftime - c2gat;
                            }
						}
					}//2th adjust
				}//phase offset adjust
				else
				{//normal phase run time
					timedown.lamptime = pinfo->gtime + pinfo->gftime;
				}//normal phase run time
			}//not the last two cycles
			timedown.phaseid = pinfo->phaseid;
			timedown.stageline = pinfo->stageline;
			if (SUCCESS != timedown_data_to_conftool(tcdata->fd->sockfd,&timedown))
			{
			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}
		}
		#ifdef EMBED_CONFIGURE_TOOL
		if (*(tcdata->fd->markbit2) & 0x0200)
		{
			memset(&timedown,0,sizeof(timedown));
			timedown.mode = *(tcdata->fd->contmode);
			timedown.pattern = *(tcdata->fd->patternid);
			timedown.lampcolor = 0x02;
			if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
			{
				if (1 == endcyclen)
				{
					if (snum == (slnum + 1))
					{
						timedown.lamptime = c1gt + c1gmt + 3;//default 3 sec green flash
					}//last a phase of stage
					else
					{
						timedown.lamptime = c1gt + 3;//default 3 sec green flash
					}
				}
				if (2 == endcyclen)
				{
					if (snum == (slnum + 1))
					{
						timedown.lamptime = c2gt + c2gmt + 3;//default 3 sec green flash
					}
					else
					{
						timedown.lamptime = c2gt + 3;//default 3 sec green flash
					}
				}
			}
			else
			{//not the last two cycles
				if ((0 != *(tcdata->fd->coordphase)) && (1 == phaseadj))
				{//phase offset adjust
					if (1 == adjcyclen)
					{//1th adjust
						if ((1 == add) && (0 == minus))
						{
							if (snum == (slnum + 1))
							{
								timedown.lamptime = pinfo->gtime + pinfo->gftime + c1gat + c1gamt;
							}
							else
							{
								timedown.lamptime = pinfo->gtime + pinfo->gftime + c1gat;
							}
						}
						if ((0 == add) && (1 == minus))
						{
							if (snum == (slnum + 1))
							{
								timedown.lamptime = pinfo->gtime + pinfo->gftime - c1gat - c1gamt;
							}
							else
							{
								timedown.lamptime = pinfo->gtime + pinfo->gftime - c1gat;
							}
						}
					}//1th adjust
					if (2 == adjcyclen)
					{//2th adjust
						if ((1 == add) && (0 == minus))
						{
							if (snum == (slnum + 1))
							{
								timedown.lamptime = pinfo->gtime + pinfo->gftime + c2gat + c2gamt;
							}
							else
							{
								timedown.lamptime = pinfo->gtime + pinfo->gftime + c2gat;
							}
						}
						if ((0 == add) && (1 == minus))
						{
							if (snum == (slnum + 1))
                            {
								timedown.lamptime = pinfo->gtime + pinfo->gftime - c2gat - c2gamt;
                            }
                            else
                            {
								timedown.lamptime = pinfo->gtime + pinfo->gftime - c2gat;
                            }
						}
					}//2th adjust
				}//phase offset adjust
				else
				{//normal phase run time
					timedown.lamptime = pinfo->gtime + pinfo->gftime;
				}//normal phase run time
			}//not the last two cycles
			timedown.phaseid = pinfo->phaseid;
			timedown.stageline = pinfo->stageline;	
			if (SUCCESS != timedown_data_to_embed(tcdata->fd->sockfd,&timedown))
			{
			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
				printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}
		}
		#endif	
	
		//send info of control to face board;
		if (*(tcdata->fd->contmode) < 27)
			fbdata[1] = *(tcdata->fd->contmode) + 1;
		else
			fbdata[1] = *(tcdata->fd->contmode);
		if ((30 == fbdata[1]) || (31 == fbdata[1]))
		{
			fbdata[2] = 0;
			fbdata[3] = 0;
			fbdata[4] = 0;
		}
		else
		{
			fbdata[2] = pinfo->stageline;
			fbdata[3] = 0x02;
			if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
        	{
            	if (1 == endcyclen)
            	{
                	if (snum == (slnum + 1))
                	{
                    	fbdata[4] = c1gt + c1gmt + 3;//default 3 sec green flash
                	}//last a phase of stage
                	else
                	{
                    	fbdata[4] = c1gt + 3;//default 3 sec green flash
                	}
            	}
            	if (2 == endcyclen)
            	{
                	if (snum == (slnum + 1))
                	{
                    	fbdata[4] = c2gt + c2gmt + 3;//default 3 sec green flash
                	}
                	else
                	{
                    	fbdata[4] = c2gt + 3;//default 3 sec green flash
                	}
            	}
        	}
			else
			{
				if ((0 != *(tcdata->fd->coordphase)) && (1 == phaseadj))
				{//phase offset adjust
					if (1 == adjcyclen)
					{//1th adjust
						if ((1 == add) && (0 == minus))
						{
							if (snum == (slnum + 1))
							{
								fbdata[4] = pinfo->gtime + pinfo->gftime + c1gat + c1gamt;
							}
							else
							{
								fbdata[4] = pinfo->gtime + pinfo->gftime + c1gat;
							}
						}
						if ((0 == add) && (1 == minus))
						{
							if (snum == (slnum + 1))
							{
								fbdata[4] = pinfo->gtime + pinfo->gftime - c1gat - c1gamt;
							}
							else
							{
								fbdata[4] = pinfo->gtime + pinfo->gftime - c1gat;
							}
						}
					}//1th adjust
					if (2 == adjcyclen)
					{//2th adjust
						if ((1 == add) && (0 == minus))
						{
							if (snum == (slnum + 1))
							{
								fbdata[4] = pinfo->gtime + pinfo->gftime + c2gat + c2gamt;
							}
							else
							{
								fbdata[4] = pinfo->gtime + pinfo->gftime + c2gat;
							}
						}
						if ((0 == add) && (1 == minus))
						{
							if (snum == (slnum + 1))
                        	{
								fbdata[4] = pinfo->gtime + pinfo->gftime - c2gat - c2gamt;
                        	}
                        	else
                        	{
								fbdata[4] = pinfo->gtime + pinfo->gftime - c2gat;
                        	}
						}
					}//2th adjust
				}//phase offset adjust
				else
				{//normal phase pass time
					fbdata[4] = pinfo->gtime + pinfo->gftime;
				}//normal phase pass time
			}
		}
		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
		printf("send face board time: %d,File: %s,Line: %d\n",fbdata[4],__FILE__,__LINE__);
		#endif
		if (!wait_write_serial(*(tcdata->fd->fbserial)))
		{
			if (write(*(tcdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
			{
			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(tcdata->fd->markbit) |= 0x0800;
				gettimeofday(&tel,NULL);
            	update_event_list(tcdata->fd->ec,tcdata->fd->el,1,16,tel.tv_sec,tcdata->fd->markbit);
            	if (SUCCESS != generate_event_file(tcdata->fd->ec,tcdata->fd->el, \
													tcdata->fd->softevent,tcdata->fd->markbit))
            	{
            	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
            	#endif
            	}
			}
		}
		else
		{
		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
			printf("face board serial cannot write,File:%s,Line:%d\n",__FILE__,__LINE__);
		#endif
		}
		sendfaceInfoToBoard(tcdata->fd,fbdata);
		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
		printf("Begin to compute sleep time,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		sltime = 0;
		if ((1 == pinfo->cpcexist) && ((pinfo->pctime) > 0))
		{//the phase do have person type channel
			if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
			{
			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
				printf("next coordinate control,endcyclen:%d,slnum:%d,Line:%d\n",endcyclen,slnum,__LINE__);
			#endif
				if (1 == endcyclen)
                {
                    if (snum == (slnum + 1))
                    {
                  //      sleep(c1gt + c1gmt);
						sltime = c1gt + c1gmt;
                    }//last a phase of stage
                    else
                    {
                 //       sleep(c1gt);
						sltime = c1gt;
                    }
                }
                if (2 == endcyclen)
                {
                    if (snum == (slnum + 1))
                    {
               //         sleep(c2gt + c2gmt);
						sltime = c2gt + c2gmt;
                    }
                    else
                    {
                //        sleep(c2gt);
						sltime = c2gt;
                    }
                }
				while (1)
				{//while (1)
					FD_ZERO(&nRead);
                	FD_SET(*(tcdata->fd->ffpipe),&nRead);	
					reatime = 0;
					mctime.tv_sec = sltime;
					mctime.tv_usec = 0;
					lasttime.tv_sec = 0;
					lasttime.tv_usec = 0;
					gettimeofday(&lasttime,NULL);
					int slret = select(*(tcdata->fd->ffpipe)+1,&nRead,NULL,NULL,&mctime);
					if (slret < 0)
					{
					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
						printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						break;			
					}
					if (0 == slret)
					{
						break;
					}
					if (slret > 0)
					{
						if (FD_ISSET(*(tcdata->fd->ffpipe),&nRead))
						{
							memset(recbuf,0,sizeof(recbuf));
							read(*(tcdata->fd->ffpipe),recbuf,sizeof(recbuf));
							if (!strncmp(recbuf,"fastforward",11))
							{
								break;
							}
							else
							{
								nowtime.tv_sec = 0;
								nowtime.tv_usec = 0;
								gettimeofday(&nowtime,NULL);
								reatime = nowtime.tv_sec - lasttime.tv_sec;
								sltime -= reatime;
								continue;			
							}	
						}
						else
						{
							nowtime.tv_sec = 0;
							nowtime.tv_usec = 0;
							gettimeofday(&nowtime,NULL);
							reatime = nowtime.tv_sec - lasttime.tv_sec;
							sltime -= reatime;
							continue;
						}
					}//if (slret > 0)
				}//while (1)
			}//the last two cycles
			else
			{//not the last two cycles
				if ((0 != *(tcdata->fd->coordphase)) && (1 == phaseadj))
				{//phase offset adjust
					if (1 == adjcyclen)
					{//1th adjust
						if ((1 == add) && (0 == minus))
						{
							if (snum == (slnum + 1))
							{
						//		sleep(pinfo->gtime - pinfo->pctime + c1gat + c1gamt);
								sltime = pinfo->gtime - pinfo->pctime + c1gat + c1gamt;
							}
							else
							{
						//		sleep(pinfo->gtime - pinfo->pctime + c1gat);
								sltime = pinfo->gtime - pinfo->pctime + c1gat;
							}
						}
						if ((0 == add) && (1 == minus))
						{
							if (snum == (slnum + 1))
							{
						//		sleep(pinfo->gtime - pinfo->pctime - c1gat - c1gamt);
								sltime = pinfo->gtime - pinfo->pctime - c1gat - c1gamt;
							}
							else
							{
						//		sleep(pinfo->gtime - pinfo->pctime - c1gat);
								sltime = pinfo->gtime - pinfo->pctime - c1gat;
							}
						}
					}//1th adjust
					if (2 == adjcyclen)
					{//2th adjust
						if ((1 == add) && (0 == minus))
						{
							if (snum == (slnum + 1))
							{
					//			sleep(pinfo->gtime - pinfo->pctime + c2gat + c2gamt);
								sltime = pinfo->gtime - pinfo->pctime + c2gat + c2gamt;
							}
							else
							{
					//			sleep(pinfo->gtime - pinfo->pctime + c2gat);
								sltime = pinfo->gtime - pinfo->pctime + c2gat;
							}
						}
						if ((0 == add) && (1 == minus))
						{
							if (snum == (slnum + 1))
                        	{
					//			sleep(pinfo->gtime - pinfo->pctime - c2gat - c2gamt);
								sltime = pinfo->gtime - pinfo->pctime - c2gat - c2gamt;
                        	}
                        	else
                        	{
					//			sleep(pinfo->gtime - pinfo->pctime - c2gat);
								sltime = pinfo->gtime - pinfo->pctime - c2gat;
                        	}
						}
					}//2th adjust
				}//phase offset adjust
				else
				{//normal phase run time
			//		sleep(pinfo->gtime - pinfo->pctime);
					sltime = pinfo->gtime - pinfo->pctime;
				}//normal phase run time
				#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
				printf("sltime:%d,c1gat:%d,c1gamt:%d,c2gat:%d,c2gamt:%d,add:%d,minus:%d,File: %s,Line: %d\n", \
					sltime,c1gat,c1gamt,c2gat,c2gamt,add,minus,__FILE__,__LINE__);
				#endif
				ffw = 0;
				while (1)
				{//while (1)
					FD_ZERO(&nRead);
                	FD_SET(*(tcdata->fd->ffpipe),&nRead);	
					reatime = 0;
					mctime.tv_sec = sltime;
					mctime.tv_usec = 0;
					lasttime.tv_sec = 0;
					lasttime.tv_usec = 0;
					gettimeofday(&lasttime,NULL);
					int slret = select(*(tcdata->fd->ffpipe)+1,&nRead,NULL,NULL,&mctime);
					if (slret < 0)
					{
					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
						printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						break;			
					}
					if (0 == slret)
					{
						break;
					}
					if (slret > 0)
					{
						if (FD_ISSET(*(tcdata->fd->ffpipe),&nRead))
						{
							memset(recbuf,0,sizeof(recbuf));
							read(*(tcdata->fd->ffpipe),recbuf,sizeof(recbuf));
							if (!strncmp(recbuf,"fastforward",11))
							{
								ffw = 1;
								break;
							}
							else
							{
								nowtime.tv_sec = 0;
								nowtime.tv_usec = 0;
								gettimeofday(&nowtime,NULL);
								reatime = nowtime.tv_sec - lasttime.tv_sec;
								sltime -= reatime;
								continue;			
							}	
						}
						else
						{
							nowtime.tv_sec = 0;
							nowtime.tv_usec = 0;
							gettimeofday(&nowtime,NULL);
							reatime = nowtime.tv_sec - lasttime.tv_sec;
							sltime -= reatime;
							continue;
						}
					}//if (slret > 0)
				}//while (1)

				if (0 == ffw)
				{
					if (0 == socpcyes)
					{
						memset(&pcdata,0,sizeof(pcdata));
						pcdata.bbserial = tcdata->fd->bbserial;
						pcdata.pchan = pinfo->cpchan;
						pcdata.markbit = tcdata->fd->markbit;
						pcdata.sockfd = tcdata->fd->sockfd;
						pcdata.cs = tcdata->cs;
						pcdata.time = pinfo->pctime; 
						int pcret = pthread_create(&socpcpid,NULL,(void *)soc_person_chan_greenflash,&pcdata);
						if (0 != pcret)
						{
						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
							printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							soc_end_part_child_thread();
							return;
						}
						socpcyes = 1;
					}
				
					if (0 == pinfo->cchan[0])
					{//if (0 == pinfo->cchan[0])
						if ((!(*(tcdata->fd->markbit) & 0x1000)) && (!(*(tcdata->fd->markbit) & 0x8000)))
						{//actively report is not probitted and connect successfully
                			sinfo.time = pinfo->pctime + pinfo->gftime + pinfo->ytime + pinfo->rtime;
							sinfo.color = 0x02;
							sinfo.conmode = *(tcdata->fd->contmode);//added on 20150529
							for (i = 0; i < MAX_CHANNEL; i++)
							{
								if (0 == pinfo->cpchan[i])
									break;
								for (j = 0; j < sinfo.chans; j++)
								{
									if (0 == sinfo.csta[j])
										break;
									tcsta = sinfo.csta[j];
									tcsta &= 0xfc;
									tcsta >>= 2;
									tcsta &= 0x3f;
									if (tcsta == pinfo->cpchan[i])
									{
										sinfo.csta[j] &= 0xfc;
										sinfo.csta[j] |= 0x03;
										break;
									}	
								}
							}
							memcpy(tcdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
							memset(sibuf,0,sizeof(sibuf));
							if (SUCCESS != status_info_report(sibuf,&sinfo))	
							{
							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
								printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}
							else
            				{
                				write(*(tcdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            				}	
						}//actively report is not probitted and connect successfully
					}//if (0 == pinfo->cchan[0])
					else
					{//0 != pinfo->cchan[0]
						if ((!(*(tcdata->fd->markbit) & 0x1000)) && (!(*(tcdata->fd->markbit) & 0x8000)))
						{//actively report is not probitted and connect successfully
                			sinfo.time = pinfo->pctime + pinfo->gftime;
							sinfo.color = 0x02;
							sinfo.conmode = *(tcdata->fd->contmode);//added on 20150529
							for (i = 0; i < MAX_CHANNEL; i++)
							{
								if (0 == pinfo->cpchan[i])
									break;
								for (j = 0; j < sinfo.chans; j++)
								{
									if (0 == sinfo.csta[j])
										break;
									tcsta = sinfo.csta[j];
									tcsta &= 0xfc;
									tcsta >>= 2;
									tcsta &= 0x3f;
									if (tcsta == pinfo->cpchan[i])
									{
										sinfo.csta[j] &= 0xfc;
										sinfo.csta[j] |= 0x03;
										break;
									}	
								}
							}
							memcpy(tcdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
							memset(sibuf,0,sizeof(sibuf));
							if (SUCCESS != status_info_report(sibuf,&sinfo))	
							{
							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
								printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}
							else
            				{
							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                            printf("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x,Line:%d\n", \
                                    sibuf[0],sibuf[1],sibuf[2],sibuf[3],sibuf[4],sibuf[5],sibuf[6],sibuf[7], \
                                    sibuf[8],__LINE__);
                            #endif
                				write(*(tcdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            				}	
						}//actively report is not probitted and connect successfully
					}//0 != pinfo->cchan[0]
				
			//		sleep(pinfo->pctime);
					sltime = pinfo->pctime;
					while (1)
					{//while (1)
						FD_ZERO(&nRead);
                		FD_SET(*(tcdata->fd->ffpipe),&nRead);	
						reatime = 0;
						mctime.tv_sec = sltime;
						mctime.tv_usec = 0;
						lasttime.tv_sec = 0;
						lasttime.tv_usec = 0;
						gettimeofday(&lasttime,NULL);
						int slret = select(*(tcdata->fd->ffpipe)+1,&nRead,NULL,NULL,&mctime);
						if (slret < 0)
						{
						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
							printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							break;			
						}
						if (0 == slret)
						{
							break;
						}
						if (slret > 0)
						{
							if (FD_ISSET(*(tcdata->fd->ffpipe),&nRead))
							{
								memset(recbuf,0,sizeof(recbuf));
								read(*(tcdata->fd->ffpipe),recbuf,sizeof(recbuf));
								if (!strncmp(recbuf,"fastforward",11))
								{
									break;
								}
								else
								{
									nowtime.tv_sec = 0;
									nowtime.tv_usec = 0;
									gettimeofday(&nowtime,NULL);
									reatime = nowtime.tv_sec - lasttime.tv_sec;
									sltime -= reatime;
									continue;			
								}	
							}
							else
							{
								nowtime.tv_sec = 0;
								nowtime.tv_usec = 0;
								gettimeofday(&nowtime,NULL);
								reatime = nowtime.tv_sec - lasttime.tv_sec;
								sltime -= reatime;
								continue;
							}
						}//if (slret > 0)
					}//while (1)
					if (1 == socpcyes)
					{
						pthread_cancel(socpcpid);
						pthread_join(socpcpid,NULL);
						socpcyes = 0;
					}
				}//0 == ffw
			}//not the last two cycles	
		}//the phase do have person type channel
		else
		{//the phase do not have person type channel;
			if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
            {
			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                printf("next coordinate control,endcyclen:%d,slnum:%d,Line:%d\n",endcyclen,slnum,__LINE__);
            #endif
                if (1 == endcyclen)
                {
                    if (snum == (slnum + 1))
                    {
				//		sleep(c1gt + c1gmt);
						sltime = c1gt + c1gmt;
                    }//last a phase of stage
                    else
                    {
				//		sleep(c1gt);
						sltime = c1gt;
                    }
                }
                if (2 == endcyclen)
                {
                    if (snum == (slnum + 1))
                    {
				//		sleep(c2gt + c2gmt);
						sltime = c2gt + c2gmt;
                    }
                    else
                    {
				//		sleep(c2gt);
						sltime = c2gt;
                    }
                }
				while (1)
				{//while (1)
					FD_ZERO(&nRead);
                	FD_SET(*(tcdata->fd->ffpipe),&nRead);	
					reatime = 0;
					mctime.tv_sec = sltime;
					mctime.tv_usec = 0;
					lasttime.tv_sec = 0;
					lasttime.tv_usec = 0;
					gettimeofday(&lasttime,NULL);
					int slret = select(*(tcdata->fd->ffpipe)+1,&nRead,NULL,NULL,&mctime);
					if (slret < 0)
					{
					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
						printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						break;			
					}
					if (0 == slret)
					{
						break;
					}
					if (slret > 0)
					{
						if (FD_ISSET(*(tcdata->fd->ffpipe),&nRead))
						{
							memset(recbuf,0,sizeof(recbuf));
							read(*(tcdata->fd->ffpipe),recbuf,sizeof(recbuf));
							if (!strncmp(recbuf,"fastforward",11))
							{
								break;
							}
							else
							{
								nowtime.tv_sec = 0;
								nowtime.tv_usec = 0;
								gettimeofday(&nowtime,NULL);
								reatime = nowtime.tv_sec - lasttime.tv_sec;
								sltime -= reatime;
								continue;			
							}	
						}
						else
						{
							nowtime.tv_sec = 0;
							nowtime.tv_usec = 0;
							gettimeofday(&nowtime,NULL);
							reatime = nowtime.tv_sec - lasttime.tv_sec;
							sltime -= reatime;
							continue;
						}
					}//if (slret > 0)
				}//while (1)				
            }
			else
			{
				if ((0 != *(tcdata->fd->coordphase)) && (1 == phaseadj))
				{//phase offset adjust
					if (1 == adjcyclen)
					{//1th adjust
						if ((1 == add) && (0 == minus))
						{
							if (snum == (slnum + 1))
							{
					//			sleep(pinfo->gtime + c1gat + c1gamt);
								sltime = pinfo->gtime + c1gat + c1gamt;
							}
							else
							{
					//			sleep(pinfo->gtime + c1gat);
								sltime = pinfo->gtime + c1gat;
							}
						}
						if ((0 == add) && (1 == minus))
						{
							if (snum == (slnum + 1))
							{
					//			sleep(pinfo->gtime - c1gat - c1gamt);
								sltime = pinfo->gtime - c1gat - c1gamt;
							}
							else
							{
					//			sleep(pinfo->gtime - c1gat);
								sltime = pinfo->gtime - c1gat;
							}
						}
					}//1th adjust
					if (2 == adjcyclen)
					{//2th adjust
						if ((1 == add) && (0 == minus))
						{
							if (snum == (slnum + 1))
							{
					//			sleep(pinfo->gtime + c2gat + c2gamt);
								sltime = pinfo->gtime + c2gat + c2gamt;
							}
							else
							{
					//			sleep(pinfo->gtime + c2gat);
								sltime = pinfo->gtime + c2gat;
							}
						}
						if ((0 == add) && (1 == minus))
						{
							if (snum == (slnum + 1))
                        	{
					//			sleep(pinfo->gtime - c2gat - c2gamt);
								sltime = pinfo->gtime - c2gat - c2gamt;
                        	}
                        	else
                        	{
					//			sleep(pinfo->gtime - c2gat);
								sltime = pinfo->gtime - c2gat;
                        	}
						}
					}//2th adjust
				}//phase offset adjust
				else
				{//normal phase run time
				//	sleep(pinfo->gtime);
					sltime = pinfo->gtime;
				}//normal phase run time
				#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
				printf("sltime:%d,c1gat:%d,c1gamt:%d,c2gat:%d,c2gamt:%d,add:%d,minus:%d,File: %s,Line: %d\n", \
                    sltime,c1gat,c1gamt,c2gat,c2gamt,add,minus,__FILE__,__LINE__);	
				#endif
				while (1)
				{//while (1)
					FD_ZERO(&nRead);
                	FD_SET(*(tcdata->fd->ffpipe),&nRead);	
					reatime = 0;
					mctime.tv_sec = sltime;
					mctime.tv_usec = 0;
					lasttime.tv_sec = 0;
					lasttime.tv_usec = 0;
					gettimeofday(&lasttime,NULL);
					int slret = select(*(tcdata->fd->ffpipe)+1,&nRead,NULL,NULL,&mctime);
					if (slret < 0)
					{
					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
						printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						break;			
					}
					if (0 == slret)
					{
						break;
					}
					if (slret > 0)
					{
						if (FD_ISSET(*(tcdata->fd->ffpipe),&nRead))
						{
							memset(recbuf,0,sizeof(recbuf));
							read(*(tcdata->fd->ffpipe),recbuf,sizeof(recbuf));
							if (!strncmp(recbuf,"fastforward",11))
							{
								break;
							}
							else
							{
								nowtime.tv_sec = 0;
								nowtime.tv_usec = 0;
								gettimeofday(&nowtime,NULL);
								reatime = nowtime.tv_sec - lasttime.tv_sec;
								sltime -= reatime;
								continue;			
							}	
						}
						else
						{
							nowtime.tv_sec = 0;
							nowtime.tv_usec = 0;
							gettimeofday(&nowtime,NULL);
							reatime = nowtime.tv_sec - lasttime.tv_sec;
							sltime -= reatime;
							continue;
						}
					}//if (slret > 0)
				}//while (1)								
			}
		}//the phase do not have person type channel;

		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
		printf("Begin to green flash,File: %s,LIne: %d\n",__FILE__,__LINE__);
		#endif
		//green flash
		if (0 != pinfo->cchan[0])
		{//change channels exist
			if ((pinfo->gftime > 0) || ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead)))
			{//if ((pinfo->gftime > 0) || ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))) 
				if ((!(*(tcdata->fd->markbit) & 0x1000)) && (!(*(tcdata->fd->markbit) & 0x8000)))
				{//actively report is not probitted and connect successfully
					if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
					{
						sinfo.time = 3;//default 3 sec green flash
					}
					else
					{
           				sinfo.time =  pinfo->gftime;
					}
					sinfo.conmode = *(tcdata->fd->contmode);//added on 20150529
					sinfo.color = 0x03;
					for (i = 0; i < MAX_CHANNEL; i++)
					{
						if (0 == pinfo->cchan[i])
							break;
						for (j = 0; j < sinfo.chans; j++)
						{
							if (0 == sinfo.csta[j])
								break;
							tcsta = sinfo.csta[j];
							tcsta &= 0xfc;
							tcsta >>= 2;
							tcsta &= 0x3f;
							if (tcsta == pinfo->cchan[i])
							{
								sinfo.csta[j] &= 0xfc;
								sinfo.csta[j] |= 0x03;
								break;
							}	
						}
					}
					memcpy(tcdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
					memset(sibuf,0,sizeof(sibuf));
					if (SUCCESS != status_info_report(sibuf,&sinfo))	
					{
					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
						printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					else
            		{
					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                    	printf("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x,Line:%d\n", \
                            sibuf[0],sibuf[1],sibuf[2],sibuf[3],sibuf[4],sibuf[5],sibuf[6],sibuf[7], \
                            sibuf[8],__LINE__);
                    #endif
            			write(*(tcdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            		}	
				}//actively report is not probitted and connect successfully
			}//if ((pinfo->gftime > 0) || ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead)))
		}//change channels exist 

		memset(&gtime,0,sizeof(gtime));
        memset(&gftime,0,sizeof(gftime));
		gettimeofday(&gftime,NULL);
        memset(&ytime,0,sizeof(ytime));
        memset(&rtime,0,sizeof(rtime));
		*(tcdata->fd->markbit) |= 0x0400;
		unsigned char		tn = 0;
		while(1)
		{
			//close
			if (SUCCESS != soc_set_lamp_color(*(tcdata->fd->bbserial),pinfo->cchan,0x03))
        	{
        	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        	#endif
				gettimeofday(&tel,NULL);
            	update_event_list(tcdata->fd->ec,tcdata->fd->el,1,15,tel.tv_sec,tcdata->fd->markbit);
            	if (SUCCESS != generate_event_file(tcdata->fd->ec,tcdata->fd->el, \
													tcdata->fd->softevent,tcdata->fd->markbit))
            	{
            	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
            	#endif
            	}
				*(tcdata->fd->markbit) |= 0x0800;
        	}
			if (SUCCESS != update_channel_status(tcdata->fd->sockfd,tcdata->cs,pinfo->cchan,0x03, \
				tcdata->fd->markbit))
        	{
        	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            	printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
        	#endif
        	}
			tctout.tv_sec = 0;
			tctout.tv_usec = 500000;
			select(0,NULL,NULL,NULL,&tctout);

			//green
			if (SUCCESS != soc_set_lamp_color(*(tcdata->fd->bbserial),pinfo->cchan,0x02))
        	{
        	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        	#endif
				gettimeofday(&tel,NULL);
            	update_event_list(tcdata->fd->ec,tcdata->fd->el,1,15,tel.tv_sec,tcdata->fd->markbit);
            	if (SUCCESS != generate_event_file(tcdata->fd->ec,tcdata->fd->el, \
													tcdata->fd->softevent,tcdata->fd->markbit))
            	{
            	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
            	#endif
            	}
				*(tcdata->fd->markbit) |= 0x0800;
        	}
			if (SUCCESS != update_channel_status(tcdata->fd->sockfd,tcdata->cs,pinfo->cchan,0x02, \
                tcdata->fd->markbit))
            {
            #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
            #endif
            }
            tctout.tv_sec = 0;
            tctout.tv_usec = 500000;
            select(0,NULL,NULL,NULL,&tctout);
			
			tn += 1;
			if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
			{
				if (tn >= 3)
					break;
			}
			else
			{
				if (tn >= pinfo->gftime)
					break;
			}
		}
		//end green flash

		if (1 == phcon)
        {
            *(tcdata->fd->markbit) &= 0xfbff;
            memset(&gtime,0,sizeof(gtime));
            gettimeofday(&gtime,NULL);
            memset(&gftime,0,sizeof(gftime));
            memset(&ytime,0,sizeof(ytime));
            memset(&rtime,0,sizeof(rtime));
            phcon = 0;
            sleep(10);
        }

		#ifdef RED_FLASH	
		if (0 == tcdata->td->timeconfig->TimeConfigList[tcline][slnum+1].StageId)
		{
			rft = (tcdata->td->timeconfig->TimeConfigList[tcline][0].SpecFunc & 0xe0) >> 5;
		}
		else
		{
			rft = (tcdata->td->timeconfig->TimeConfigList[tcline][slnum+1].SpecFunc & 0xe0) >> 5;
		}
		if (rft > 0)
		{
			redflash_soc		soc;
			soc.tcline = tcline;
			soc.slnum = slnum;
			soc.snum =	snum;
			soc.rft = rft;
			soc.chan = pinfo->chan;
			soc.soc = tcdata;
			int rfarg = pthread_create(&rfpid,NULL,(void *)soc_red_flash,&soc);
			if (0 != rfarg)
			{
			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
				printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				return;
			}
		}//if (rft > 0) 
		#endif

		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
		printf("Begin to yellow lamp,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		//yellow lamp
		if (1 == (pinfo->cpcexist))
		{//current change channels exist person channels;
			if (SUCCESS != soc_set_lamp_color(*(tcdata->fd->bbserial),pinfo->cnpchan,0x01))
			{
			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
				printf("set yellow lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
				gettimeofday(&tel,NULL);
           		update_event_list(tcdata->fd->ec,tcdata->fd->el,1,15,tel.tv_sec,tcdata->fd->markbit);
           		if (SUCCESS != generate_event_file(tcdata->fd->ec,tcdata->fd->el, \
													tcdata->fd->softevent,tcdata->fd->markbit))
           		{
           		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
               		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
           		#endif
           		}
				*(tcdata->fd->markbit) |= 0x0800;
			}
			if(SUCCESS != update_channel_status(tcdata->fd->sockfd,tcdata->cs,pinfo->cnpchan, \
				0x01,tcdata->fd->markbit))
			{
			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
				printf("update channels err,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}
			
			if (SUCCESS != soc_set_lamp_color(*(tcdata->fd->bbserial),pinfo->cpchan,0x00))
			{
			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
				printf("set red lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
				gettimeofday(&tel,NULL);
           		update_event_list(tcdata->fd->ec,tcdata->fd->el,1,15,tel.tv_sec,tcdata->fd->markbit);
           		if (SUCCESS != generate_event_file(tcdata->fd->ec,tcdata->fd->el, \
													tcdata->fd->softevent,tcdata->fd->markbit))
           		{
           		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
               		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
           		#endif
           		}
				*(tcdata->fd->markbit) |= 0x0800;
			}
			if(SUCCESS != update_channel_status(tcdata->fd->sockfd,tcdata->cs,pinfo->cpchan, \
					0x00,tcdata->fd->markbit))
			{
			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
				printf("update channels err,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}

			if (*(tcdata->fd->auxfunc) & 0x01)
			{//if (*(tcdata->fd->auxfunc) & 0x01)
				if ((30 != *(tcdata->fd->contmode)) && (31 != *(tcdata->fd->contmode)))
				{//11
					tchans = 0;
					for (j = 0; j < MAX_CHANNEL; j++)
					{
						if (0 == (pinfo->cnpchan[j]))
							break;
						tchans |= (0x00000001 << ((pinfo->cnpchan[j]) - 1));
					}
					downti[1] = 0;
					downti[1] |= (tchans & 0x000000ff);
					downti[2] = 0;
					downti[2] |= (((tchans & 0x0000ff00) >> 8) & 0x000000ff);
					downti[3] = 0;
					downti[3] |= (((tchans & 0x00ff0000) >> 16) & 0x000000ff);
					downti[4] = 0;
					downti[4] |= (((tchans & 0xff000000) >> 24) & 0x000000ff);	
					downti[5] = 0x01;
					if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
					{//the last two cycles
						downti[6] = 3;
					}//the last two cycles
					else
					{//not the last two cycles
						downti[6] = pinfo->ytime;
					}//not the last two cycles
					if (!wait_write_serial(*(tcdata->fd->bbserial)))
					{
						if (write(*(tcdata->fd->bbserial),downti,sizeof(downti)) < 0)
						{
						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
					}
					else
					{
					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
						printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					for (j = 0; j < MAX_CHANNEL; j++)
					{//2
						if (0 == pinfo->cpchan[j])
							break;
						downti[6] = 0;
						for (z = 0; z < MAX_PHASE_LINE; z++)
						{//for (z = 0; z < MAX_PHASE_LINE; z++)
							if (0 == phdetect[z].stageid)
								break;
							if (phdetect[z].stageid == (pinfo->stageline))
							{
								if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
								{
									downti[6] += 3;
								}
								else
								{
									downti[6] += phdetect[z].ytime + phdetect[z].rtime;
								}
								break;
							}//if (phdetect[z].stageid == (pinfo->stageline))	
						}//for (z = 0; z < MAX_PHASE_LINE; z++)
						k = z + 1;
						s = z; //backup 'z'
						while (1)
						{
							if (0 == phdetect[k].stageid)
							{
								if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
								{
									if (1 == endcyclen)
										downti[6] += c1gmt;
									if (2 == endcyclen)
										downti[6] += c2gmt;
								}
								else
								{
									if((0!=*(tcdata->fd->coordphase))&&(1==phaseadj))
									{
										if (1 == adjcyclen)
										{
											if ((1 == add) && (0 == minus))
											{
												downti[6] += c1gamt;
											}
											if ((0 == add) && (1 == minus))
											{
												downti[6] -= c1gamt;
											}
										}
										if (2 == adjcyclen)
										{
											if ((1 == add) && (0 == minus))
											{
												downti[6] += c2gamt;
											}
											if ((0 == add) && (1 == minus))
											{
												downti[6] -= c2gamt;
											}
										}
									}
								}
								k = 0;
								continue;
							}
							if (k == s)
								break;
							if (phdetect[k].chans & (0x00000001 << (pinfo->cpchan[j] - 1)))
							{
								break;
							}
							else
							{
								if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
								{//the last two cycles
									if (1 == endcyclen)
										downti[6] += c1gt + 3 + 3;
									if (2 == endcyclen)
										downti[6] += c2gt + 3 + 3;
								}//the last two cycles
								else
								{
									downti[6] += \
									phdetect[k].gtime + phdetect[k].gftime + \
									phdetect[k].ytime + phdetect[k].rtime;
									if((0!=*(tcdata->fd->coordphase))&&(1==phaseadj))
									{//phase offset adjust
										if (1 == adjcyclen)
										{
											if ((1 == add) && (0 == minus))
											{
												downti[6] += c1gat;
											}
											if ((0 == add) && (1 == minus))
											{
												downti[6] -= c1gat;
											}
										}
										if (2 == adjcyclen)
										{
											if ((1 == add) && (0 == minus))
											{
												downti[6] += c2gat;
											}
											if ((0 == add) && (1 == minus))
											{
												downti[6] -= c2gat;
											}
										}
									}//phase offset adjust
								}
							}
							z = k;
							k++;
						}
						tchans = 0;
						tchans |= (0x00000001 << (pinfo->cpchan[j] - 1));
						downti[1] = 0;
						downti[1] |= (tchans & 0x000000ff);
						downti[2] = 0;
						downti[2] |= (((tchans & 0x0000ff00) >> 8) & 0x000000ff);
						downti[3] = 0;
						downti[3] |= (((tchans & 0x00ff0000) >> 16) & 0x000000ff);
						downti[4] = 0;
						downti[4] |= (((tchans & 0xff000000) >> 24) & 0x000000ff);	
						downti[5] = 0x00;
						if (!wait_write_serial(*(tcdata->fd->bbserial)))
						{
							if (write(*(tcdata->fd->bbserial),downti,sizeof(downti)) < 0)
							{
							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
								printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}
						}
						else
						{
						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
							printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}	
					}//2
					if (!wait_write_serial(*(tcdata->fd->bbserial)))
					{
						if (write(*(tcdata->fd->bbserial),edownti,sizeof(edownti)) < 0)
						{
						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
					}
					else
					{
					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
						printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}	
				}//11
			}//if (*(tcdata->fd->auxfunc) & 0x01)
		}//current change channels exist person channels;
		else
		{//current change channels do not exist person channels;
			if (SUCCESS != soc_set_lamp_color(*(tcdata->fd->bbserial),pinfo->cchan,0x01))
			{
			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
				printf("set yellow err,File: %s,Line:%d\n",__FILE__,__LINE__);
			#endif
				gettimeofday(&tel,NULL);
           		update_event_list(tcdata->fd->ec,tcdata->fd->el,1,15,tel.tv_sec,tcdata->fd->markbit);
           		if (SUCCESS != generate_event_file(tcdata->fd->ec,tcdata->fd->el, \
													tcdata->fd->softevent,tcdata->fd->markbit))
           		{
           		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
               		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
           		#endif
           		}
				*(tcdata->fd->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(tcdata->fd->sockfd,tcdata->cs,pinfo->cchan, \
				0x01,tcdata->fd->markbit))
			{
			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
				printf("update channels err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
			if (*(tcdata->fd->auxfunc) & 0x01)
			{//if (*(tcdata->fd->auxfunc) & 0x01)
				if ((30 != *(tcdata->fd->contmode)) && (31 != *(tcdata->fd->contmode)))
				{//11
					tchans = 0;
					for (j = 0; j < MAX_CHANNEL; j++)
					{
						if (0 == (pinfo->cchan[j]))
							break;
						tchans |= (0x00000001 << ((pinfo->cchan[j]) - 1));
					}
					downti[1] = 0;
					downti[1] |= (tchans & 0x000000ff);
					downti[2] = 0;
					downti[2] |= (((tchans & 0x0000ff00) >> 8) & 0x000000ff);
					downti[3] = 0;
					downti[3] |= (((tchans & 0x00ff0000) >> 16) & 0x000000ff);
					downti[4] = 0;
					downti[4] |= (((tchans & 0xff000000) >> 24) & 0x000000ff);	
					downti[5] = 0x01;
					if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
						downti[6] = 3;
					else
						downti[6] = pinfo->ytime;
					if (!wait_write_serial(*(tcdata->fd->bbserial)))
					{
						if (write(*(tcdata->fd->bbserial),downti,sizeof(downti)) < 0)
						{
						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
					}
					else
					{
					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
						printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					if (!wait_write_serial(*(tcdata->fd->bbserial)))
					{
						if (write(*(tcdata->fd->bbserial),edownti,sizeof(edownti)) < 0)
						{
						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
					}
					else
					{
					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
						printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
				}//11
			}//if (*(tcdata->fd->auxfunc) & 0x01)
		}//current change channels do not exist person channels;

		if (0 != pinfo->cchan[0])
        {//change channels exist
            if ((pinfo->ytime > 0) || ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead)))
            {//if ((pinfo->gftime > 0) || ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead)))
				if ((!(*(tcdata->fd->markbit) & 0x1000)) && (!(*(tcdata->fd->markbit) & 0x8000)))
				{//actively report is not probitted and connect successfully
					if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
            		{
                		sinfo.time = 3;
            		}
            		else
            		{
                		sinfo.time = pinfo->ytime;
            		}
					sinfo.conmode = *(tcdata->fd->contmode);//added on 20150529	
					sinfo.color = 0x01;
					for (i = 0; i < MAX_CHANNEL; i++)
					{
						if (0 == pinfo->cnpchan[i])
							break;
						for (j = 0; j < sinfo.chans; j++)
						{
							if (0 == sinfo.csta[j])
								break;
							tcsta = sinfo.csta[j];
							tcsta &= 0xfc;
							tcsta >>= 2;
							tcsta &= 0x3f;
							if (tcsta == pinfo->cnpchan[i])
							{
								sinfo.csta[j] &= 0xfc;
								sinfo.csta[j] |= 0x01;
								break;
							}	
						}
					}
					for (i = 0; i < MAX_CHANNEL; i++)
            		{
                		if (0 == pinfo->cpchan[i])
                    		break;
                		for (j = 0; j < sinfo.chans; j++)
                		{
                    		if (0 == sinfo.csta[j])
                        		break;
                    		tcsta = sinfo.csta[j];
                    		tcsta &= 0xfc;
                    		tcsta >>= 2;
                    		tcsta &= 0x3f;
                    		if (tcsta == pinfo->cpchan[i])
                    		{
                        		sinfo.csta[j] &= 0xfc;
								break;
                    		}
                		}
            		}
					memcpy(tcdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
					memset(sibuf,0,sizeof(sibuf));
					if (SUCCESS != status_info_report(sibuf,&sinfo))	
					{
					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
						printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					else
            		{
					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                    	printf("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x,Line:%d\n", \
                            sibuf[0],sibuf[1],sibuf[2],sibuf[3],sibuf[4],sibuf[5],sibuf[6],sibuf[7], \
                            sibuf[8],__LINE__);
                    #endif
            			write(*(tcdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            		}
				}//actively report is not probitted and connect successfully		
			}
		}//change channels exist

		if ((*(tcdata->fd->markbit) & 0x0002) && (*(tcdata->fd->markbit) & 0x0010))
		{
			memset(&timedown,0,sizeof(timedown));
        	timedown.mode = *(tcdata->fd->contmode);
        	timedown.pattern = *(tcdata->fd->patternid);
        	timedown.lampcolor = 0x01;
			if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
			{
				timedown.lamptime = 3;
			}
			else
			{
        		timedown.lamptime = pinfo->ytime;
			}
        	timedown.phaseid = pinfo->phaseid;
        	timedown.stageline = pinfo->stageline;
        	if (SUCCESS != timedown_data_to_conftool(tcdata->fd->sockfd,&timedown))
        	{
        	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
           		printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
        	#endif
        	}
		}
		#ifdef EMBED_CONFIGURE_TOOL
		if (*(tcdata->fd->markbit2) & 0x0200)
		{
			memset(&timedown,0,sizeof(timedown));
            timedown.mode = *(tcdata->fd->contmode);
            timedown.pattern = *(tcdata->fd->patternid);
            timedown.lampcolor = 0x01;
            if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
            {
                timedown.lamptime = 3;
            }
            else
            {
                timedown.lamptime = pinfo->ytime;
            }
            timedown.phaseid = pinfo->phaseid;
            timedown.stageline = pinfo->stageline;		
			if (SUCCESS != timedown_data_to_embed(tcdata->fd->sockfd,&timedown))
			{
			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
				printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}
		}
		#endif
		//send control info to face board
		if (*(tcdata->fd->contmode) < 27)
            fbdata[1] = *(tcdata->fd->contmode) + 1;
        else
            fbdata[1] = *(tcdata->fd->contmode);
		if ((30 == fbdata[1]) || (31 == fbdata[1]))
		{
			fbdata[2] = 0;
			fbdata[3] = 0;
			fbdata[4] = 0;
		}
		else
		{
			fbdata[2] = pinfo->stageline;
			fbdata[3] = 0x01;
			if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
			{
        		fbdata[4] = 3;
			}
			else
			{
				fbdata[4] = pinfo->ytime;
			}
		}
        if (!wait_write_serial(*(tcdata->fd->fbserial)))
        {
           	if (write(*(tcdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
           	{
           	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
               	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
           	#endif
				*(tcdata->fd->markbit) |= 0x0800;
				gettimeofday(&tel,NULL);
           		update_event_list(tcdata->fd->ec,tcdata->fd->el,1,16,tel.tv_sec,tcdata->fd->markbit);
           		if (SUCCESS != generate_event_file(tcdata->fd->ec,tcdata->fd->el, \
													tcdata->fd->softevent,tcdata->fd->markbit))
           		{
           		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
               		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
           		#endif
           		}
           	}
        }
        else
        {
        #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
           	printf("face serial cannot write,File:%s,Line:%d\n",__FILE__,__LINE__);
        #endif
        }
		sendfaceInfoToBoard(tcdata->fd,fbdata);
		*(tcdata->fd->color) = 0x01;
		memset(&gtime,0,sizeof(gtime));
        memset(&gftime,0,sizeof(gftime));
        memset(&ytime,0,sizeof(ytime));
		gettimeofday(&ytime,NULL);
        memset(&rtime,0,sizeof(rtime));
		if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
			sleep(3);
		else
			sleep(pinfo->ytime);

		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
		printf("Begin to red lamp,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		//red lamp
		//all channels will be set red	
		if (SUCCESS != soc_set_lamp_color(*(tcdata->fd->bbserial),pinfo->cchan,0x00))
        {
        #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
			gettimeofday(&tel,NULL);
            update_event_list(tcdata->fd->ec,tcdata->fd->el,1,15,tel.tv_sec,tcdata->fd->markbit);
            if (SUCCESS != generate_event_file(tcdata->fd->ec,tcdata->fd->el, \
													tcdata->fd->softevent,tcdata->fd->markbit))
            {
            #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
			*(tcdata->fd->markbit) |= 0x0800;
        }
		if(SUCCESS!=update_channel_status(tcdata->fd->sockfd,tcdata->cs,pinfo->cchan,0x00,tcdata->fd->markbit))
        {   
        #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
        #endif 
        }
		*(tcdata->fd->color) = 0x00;

		if (0 != pinfo->cchan[0])
        {//change channels exist
			if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
				pinfo->rtime = 0;//red time is default 0 at the two cycle of end ahead

     //       if (pinfo->rtime > 0)
    //        {
				if ((!(*(tcdata->fd->markbit) & 0x1000)) && (!(*(tcdata->fd->markbit) & 0x8000)))
				{//actively report is not probitted and connect successfully
					if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
            		{
                		sinfo.time = 0;
            		}
            		else
            		{
                		sinfo.time = pinfo->rtime;
            		}	
					sinfo.color = 0x00;
					sinfo.conmode = *(tcdata->fd->contmode);//added on 20150529
					for (i = 0; i < MAX_CHANNEL; i++)
					{
						if (0 == pinfo->cchan[i])
							break;
						for (j = 0; j < sinfo.chans; j++)
						{
							if (0 == sinfo.csta[j])
								break;
							tcsta = sinfo.csta[j];
							tcsta &= 0xfc;
							tcsta >>= 2;
							tcsta &= 0x3f;
							if (tcsta == pinfo->cchan[i])
							{
								sinfo.csta[j] &= 0xfc;
								break;
							}	
						}
					}
					memcpy(tcdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
					memset(sibuf,0,sizeof(sibuf));
					if (SUCCESS != status_info_report(sibuf,&sinfo))	
					{
					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
						printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					else
            		{
					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                    	printf("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x,Line:%d\n", \
                            sibuf[0],sibuf[1],sibuf[2],sibuf[3],sibuf[4],sibuf[5],sibuf[6],sibuf[7], \
                            sibuf[8],__LINE__);
                    #endif
            			write(*(tcdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            		}
				}//actively report is not probitted and connect successfully
		//	}
		}//change channels exist

		if (*(tcdata->fd->auxfunc) & 0x01)
		{//if (*(tcdata->fd->auxfunc) & 0x01)
			if ((30 != *(tcdata->fd->contmode)) && (31 != *(tcdata->fd->contmode)))
			{
				for (j = 0; j < MAX_CHANNEL; j++)
				{//2
					if (0 == pinfo->cchan[j])
						break;
					downti[6] = 0;
					for (z = 0; z < MAX_PHASE_LINE; z++)
					{//for (z = 0; z < MAX_PHASE_LINE; z++)
						if (0 == phdetect[z].stageid)
							break;
						if (phdetect[z].stageid == (pinfo->stageline))
						{
							if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
								downti[6] += 0;
							else
								downti[6] += phdetect[z].rtime;
							break;
						}//if (phdetect[z].stageid == (pinfo->stageline))	
					}//for (z = 0; z < MAX_PHASE_LINE; z++)
					k = z + 1;
					s = z; //backup 'z'
					while (1)
					{
						if (0 == phdetect[k].stageid)
						{
							if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
							{
								if (1 == endcyclen)
									downti[6] += c1gmt;
								if (2 == endcyclen)
									downti[6] += c2gmt;
							}
							else
							{
								if((0!=*(tcdata->fd->coordphase))&&(1==phaseadj))
								{
									if (1 == adjcyclen)
									{
										if ((1 == add) && (0 == minus))
										{
											downti[6] += c1gamt;
										}
										if ((0 == add) && (1 == minus))
										{
											downti[6] -= c1gamt;
										}
									}
									if (2 == adjcyclen)
									{
										if ((1 == add) && (0 == minus))
										{
											downti[6] += c2gamt;
										}
										if ((0 == add) && (1 == minus))
										{
											downti[6] -= c2gamt;
										}
									}
								}
							}
							k = 0;
							continue;
						}
						if (k == s)
							break;
						if (phdetect[k].chans & (0x00000001 << (pinfo->cchan[j] - 1)))
						{
							break;
						}
						else
						{
							if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
							{
								if (1 == endcyclen)
									downti[6] += c1gt + 3 + 3;
								if (2 == endcyclen)
									downti[6] += c2gt + 3 + 3;
							}
							else
							{
								downti[6] += \
								phdetect[k].gtime+phdetect[k].gftime+phdetect[k].ytime+phdetect[k].rtime;
								if((0!=*(tcdata->fd->coordphase))&&(1==phaseadj))
								{
									if (1 == adjcyclen)
									{
										if ((1 == add) && (0 == minus))
										{
											downti[6] += c1gat;
										}
										if ((0 == add) && (1 == minus))
										{
											downti[6] -= c1gat;
										}
									}
									if (2 == adjcyclen)
									{
										if ((1 == add) && (0 == minus))
										{
											downti[6] += c2gat;
										}
										if ((0 == add) && (1 == minus))
										{
											downti[6] -= c2gat;
										}
									}
								}
							}
						}
						z = k;
						k++;
					}
					tchans = 0;
					tchans |= (0x00000001 << (pinfo->cchan[j] - 1));
					downti[1] = 0;
					downti[1] |= (tchans & 0x000000ff);
					downti[2] = 0;
					downti[2] |= (((tchans & 0x0000ff00) >> 8) & 0x000000ff);
					downti[3] = 0;
					downti[3] |= (((tchans & 0x00ff0000) >> 16) & 0x000000ff);
					downti[4] = 0;
					downti[4] |= (((tchans & 0xff000000) >> 24) & 0x000000ff);	
					downti[5] = 0x00;
					if (!wait_write_serial(*(tcdata->fd->bbserial)))
					{
						if (write(*(tcdata->fd->bbserial),downti,sizeof(downti)) < 0)
						{
						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
					}
					else
					{
					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
						printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}	
				}//2
			}
		}//if (*(tcdata->fd->auxfunc) & 0x01)

		//time of red lamp is default 0 in last two cycles 
		if ((*(tcdata->fd->markbit) & 0x0100) && (1 == endahead))
		{
		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
			printf("red lamp default is 0,do nothing,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}
		else
		{//not the last two cycles
			if (pinfo->rtime > 0)
			{
				if (*(tcdata->fd->auxfunc) & 0x01)
				{//if (*(tcdata->fd->auxfunc) & 0x01)
					if (!wait_write_serial(*(tcdata->fd->bbserial)))
					{
						if (write(*(tcdata->fd->bbserial),edownti,sizeof(edownti)) < 0)
						{
						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
					}
					else
					{
					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
						printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
                }//if (*(tcdata->fd->auxfunc) & 0x01)

				if ((*(tcdata->fd->markbit) & 0x0002) && (*(tcdata->fd->markbit) & 0x0010))
				{
					memset(&timedown,0,sizeof(timedown));
        			timedown.mode = *(tcdata->fd->contmode);
        			timedown.pattern = *(tcdata->fd->patternid);
        			timedown.lampcolor = 0x00;
        			timedown.lamptime = pinfo->rtime;
        			timedown.phaseid = pinfo->phaseid;
        			timedown.stageline = pinfo->stageline;
        			if (SUCCESS != timedown_data_to_conftool(tcdata->fd->sockfd,&timedown))
        			{
        			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
        			#endif
        			}
				}
				#ifdef EMBED_CONFIGURE_TOOL
				if (*(tcdata->fd->markbit2) & 0x0200)
				{
					memset(&timedown,0,sizeof(timedown));
                    timedown.mode = *(tcdata->fd->contmode);
                    timedown.pattern = *(tcdata->fd->patternid);
                    timedown.lampcolor = 0x00;
                    timedown.lamptime = pinfo->rtime;
                    timedown.phaseid = pinfo->phaseid;
                    timedown.stageline = pinfo->stageline;	
					if (SUCCESS != timedown_data_to_embed(tcdata->fd->sockfd,&timedown))
					{
					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
						printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
				}
				#endif
				if (*(tcdata->fd->contmode) < 27)
            		fbdata[1] = *(tcdata->fd->contmode) + 1;
        		else
            		fbdata[1] = *(tcdata->fd->contmode);
				if ((30 == fbdata[1]) || (31 == fbdata[1]))
				{
					fbdata[2] = 0;
					fbdata[3] = 0;
					fbdata[4] = 0;
				}
				else
				{
					fbdata[2] = pinfo->stageline;
					fbdata[3] = 0x00;
        			fbdata[4] = pinfo->rtime;
				}
        		if (!wait_write_serial(*(tcdata->fd->fbserial)))
        		{
            		if (write(*(tcdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
            		{
            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
            		#endif
						*(tcdata->fd->markbit) |= 0x0800;
						gettimeofday(&tel,NULL);
                		update_event_list(tcdata->fd->ec,tcdata->fd->el,1,16,tel.tv_sec,tcdata->fd->markbit);
                		if (SUCCESS != generate_event_file(tcdata->fd->ec,tcdata->fd->el, \
																tcdata->fd->softevent,tcdata->fd->markbit))
                		{
                		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                    		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                		#endif
                		}
            		}
        		}
        		else
        		{
        		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            		printf("face serial cannot write,File:%s,Line:%d\n",__FILE__,__LINE__);
        		#endif
        		}

				memset(&gtime,0,sizeof(gtime));
        		memset(&gftime,0,sizeof(gftime));
        		memset(&ytime,0,sizeof(ytime));
        		memset(&rtime,0,sizeof(rtime));
				gettimeofday(&rtime,NULL);
				sendfaceInfoToBoard(tcdata->fd,fbdata);
				sleep(pinfo->rtime);
			}
		}//not the last two cycles

		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
		printf("Begin to next phase,FIle: %s,LIne: %d\n",__FILE__,__LINE__);
		#endif
		slnum += 1;
		*(tcdata->fd->slnum) = slnum;
		*(tcdata->fd->stageid) = tcdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
		if (0 == (tcdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId))
		{
			slnum = 0;
			*(tcdata->fd->slnum) = slnum;
			*(tcdata->fd->stageid) = tcdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
			cycarr = 1;
		}
		#ifdef RED_FLASH
		if (rft > (pinfo->ytime + pinfo->rtime))
		{
			sleep(rft - (pinfo->ytime) - (pinfo->rtime));
		}
		#endif
		if (1 == phcon)
		{
			*(tcdata->fd->markbit) &= 0xfbff;
			memset(&gtime,0,sizeof(gtime));
        	gettimeofday(&gtime,NULL);
        	memset(&gftime,0,sizeof(gftime));
        	memset(&ytime,0,sizeof(ytime));
        	memset(&rtime,0,sizeof(rtime));
			phcon = 0;
			sleep(10);
		}
		continue;
	}//while loop

	pthread_exit(NULL);
}

/*color: 0x00 means red,0x01 means yellow,0x02 means green,0x12 means green flash*/
int get_soc_status(unsigned char *color,unsigned char *leatime)
{
	if ((NULL == color) || (NULL == leatime))
	{
	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
		printf("pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	struct timeval			nowtime;
	nowtime.tv_sec = 0;
	nowtime.tv_usec = 0;
	gettimeofday(&nowtime,NULL);
	if (0 != rtime.tv_sec)
	{
		*leatime = nowtime.tv_sec - rtime.tv_sec;
		*color = 0x00;
	}
	if (0 != ytime.tv_sec)
	{
		*leatime = nowtime.tv_sec - ytime.tv_sec;
		*color = 0x01;
	}
	if (0 != gftime.tv_sec)
	{
		*leatime = nowtime.tv_sec - gftime.tv_sec;
		*color = 0x12;
	}
	if (0 != gtime.tv_sec)
	{
		*leatime = nowtime.tv_sec - gtime.tv_sec;
		*color = 0x02;
	}

	return SUCCESS;
}

static void soc_yellow_flash(void *arg)
{
//	int		*bbserial = arg;
	
//	all_yellow_flash(*bbserial,0);
	yfdata_t			*yfdata = arg;
	new_yellow_flash(yfdata);

	pthread_exit(NULL);
}

int optimize_coordinate_control(fcdata_t *fcdata, tscdata_t *tscdata,ChannelStatus_t *chanstatus)
{
	if ((NULL == fcdata) || (NULL == tscdata) || (NULL == chanstatus))
	{
	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
		printf("pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	//initial static variable
	socrettl = 0;
	socyes = 0;
	socyfyes = 0;
	socpcyes = 0;
	memset(&gtime,0,sizeof(gtime));
	memset(&gftime,0,sizeof(gftime));
	memset(&ytime,0,sizeof(ytime));
	memset(&rtime,0,sizeof(rtime));
	memset(&sinfo,0,sizeof(statusinfo_t));
	phcon = 0;
	memset(fcdata->sinfo,0,sizeof(statusinfo_t));
	//end initial static variable

	socdata_t				tcdata;
	socpinfo_t				pinfo;
	unsigned char			contmode = *(fcdata->contmode);//save control mode
	if (0 == socyes)
	{
		memset(&tcdata,0,sizeof(tcdata));
		memset(&pinfo,0,sizeof(socpinfo_t));
		tcdata.fd = fcdata;
		tcdata.td = tscdata;
		tcdata.cs = chanstatus;
		tcdata.pi = &pinfo;
		int tcret = pthread_create(&socpid,NULL,(void *)start_optimize_coordinate_control,&tcdata);
		if (0 != tcret)
		{
		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
			printf("create thread err,File:%s,Line:%d\n",__FILE__,__LINE__);
		#endif
			soc_end_part_child_thread();
			return FAIL;
		}
		socyes = 1;
	}

	sleep(1);

	//Begin to check if there have manual traffic control;
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

	unsigned char           downti[8] = {0xA6,0xff,0xff,0xff,0xff,0x03,0,0xED};
    unsigned char           edownti[3] = {0xA5,0xA5,0xED};
	unsigned char			tcred = 0;//'1' means lamps have been status of all red; 
	fd_set					nread;
	unsigned char			tcbuf[32] = {0};
	unsigned char			keylock = 0; //key lock,'1' means lock,'0' means unlock;	
	unsigned char			wllock = 0; //wireless terminal lock, '1' means lock,'0' means unlock;
	unsigned char			color = 3;// default lamp is closed;
	unsigned char			leatime = 0;//lease time;
	struct timeval			to,ct;
//	struct timeval			wut;
	struct timeval          mont,ltime;
	unsigned char			ngf = 0;
	timedown_t				tctd;
	yfdata_t				yfdata;
	yfdata_t				ardata;
	unsigned char           fbdata[6] = {0};
	fbdata[0] = 0xC1;
	fbdata[5] = 0xED;
	unsigned char			tcecho[3] = {0};
	tcecho[0] = 0xCA;
	tcecho[2] = 0xED;
	memset(&ardata,0,sizeof(ardata));
	ardata.second = 0;
	ardata.markbit = fcdata->markbit;
	ardata.markbit2 = fcdata->markbit2;
	ardata.serial = *(fcdata->bbserial); 
	ardata.sockfd = fcdata->sockfd;
	ardata.cs = chanstatus;

	unsigned char               sibuf[64] = {0};
//	statusinfo_t                sinfo;
//    memset(&sinfo,0,sizeof(statusinfo_t));
	unsigned char               *csta = NULL;
    unsigned char               tcsta = 0;
    sinfo.pattern = *(fcdata->patternid);
	unsigned char				ncmode = *(fcdata->contmode);

	unsigned char				wtlock = 0; //wireless terminal lock	
	struct timeval				wtltime;
	unsigned char				pantn = 0; //number of pant;
	unsigned char           	dirc = 0; //direct control
	unsigned char				kstep = 0;
	unsigned char				cktem = 0;
	//channes of eight direction mapping;
    //left and right lamp all close
	unsigned char 		dirch1[8][8] = {{1,3,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
                                    {2,4,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
                                    {1,0,0,0,0,0,0,0},{2,0,0,0,0,0,0,0},
                                    {3,0,0,0,0,0,0,0},{4,0,0,0,0,0,0,0}};

	unsigned char       lkch1[4][8] = {{1,3,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
                                        {2,4,0,0,0,0,0,0},{0,0,0,0,0,0,0,0}};
	unsigned char		clch1[12] = {5,6,7,8,9,10,11,12,17,18,19,20};
	//only close left lamp
	unsigned char 		dirch2[8][8] = {{1,3,9,11,0,0,0,0},{9,11,0,0,0,0,0,0},
                                      {2,4,10,12,0,0,0,0},{10,12,0,0,0,0,0,0},
                                      {1,9,0,0,0,0,0,0},{2,10,0,0,0,0,0,0},
                                      {3,11,0,0,0,0,0,0},{4,12,0,0,0,0,0,0}};

	unsigned char       lkch2[4][8] = {{1,3,9,11,0,0,0,0},{9,11,0,0,0,0,0,0},
                                        {2,4,10,12,0,0,0,0},{10,12,0,0,0,0,0,0}};
	unsigned char		clch2[12] = {5,6,7,8,17,18,19,20,0,0,0,0};
	//only close right lamp
	unsigned char 		dirch3[8][8] = {{1,3,0,0,0,0,0,0},{5,7,17,19,0,0,0,0},
                                      {2,4,0,0,0,0,0,0},{6,8,18,20,0,0,0,0},
                                      {1,5,17,0,0,0,0,0},{2,6,18,0,0,0,0,0},
                                      {3,7,19,0,0,0,0,0},{4,8,20,0,0,0,0,0}};

	unsigned char       lkch3[4][8] = {{1,3,0,0,0,0,0,0},{5,7,17,19,0,0,0,0},
                                        {2,4,0,0,0,0,0,0},{6,8,18,20,0,0,0,0}};
	unsigned char		clch3[12] = {9,10,11,12,0,0,0,0,0,0,0,0};
	//Not close left and right lamp
	unsigned char		dirch0[8][8] = {{1,3,9,11,0,0,0,0},{5,7,9,11,17,19,0,0},
                                      {2,4,10,12,0,0,0,0},{6,8,10,12,18,20,0,0},
                                      {1,5,9,17,0,0,0,0},{2,6,10,18,0,0,0,0},
                                      {3,7,11,19,0,0,0,0},{4,8,12,20,0,0,0,0}};
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
	unsigned char				wlinfo[13] = {0};
	terminalpp_t				tpp;
	unsigned char           	terbuf[11] = {0};
	tpp.head = 0xA9;
	tpp.size = 4;
	tpp.id = *(fcdata->lip);
	tpp.type = 0x04;
	tpp.func[0] = 0; 
	tpp.func[1] = 0;
	tpp.func[2] = 0;
	tpp.func[3] = 0;
	tpp.end = 0xED;

	markdata_t					mdt;
	unsigned char               dircon = 0;//mobile direction control
    unsigned char               firdc = 1;//mobile first direction control
    unsigned char               fdirch[MAX_CHANNEL] = {0};//front direction control channel
    unsigned char               fdirn = 0;
    unsigned char               cdirch[MAX_CHANNEL] = {0};//current direction control channel
    unsigned char               cdirn = 0;
	unsigned char				tclc = 0;
	unsigned char				ccon[MAX_CHANNEL] = {0};
	unsigned char               soc_phase = 0;
	while (1)
	{//while loop
	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
		printf("Reading pipe,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		FD_ZERO(&nread);
		FD_SET(*(fcdata->conpipe),&nread);
		int max = *(fcdata->conpipe);
		wtltime.tv_sec = RFIDT;
		wtltime.tv_usec = 0;
#if 0
		//WuXi code
		wut.tv_sec = 0;
		wut.tv_usec = 0;
		gettimeofday(&wut,NULL);
		//end WuXi code
#endif
		int cpret = select(max+1,&nread,NULL,NULL,&wtltime);
		if (cpret < 0)
		{
		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
			printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			soc_end_part_child_thread();
			return FAIL;
		}
		if (0 == cpret)
		{//time out
			if (*(fcdata->markbit2) & 0x0100)
				continue;  //rfid is controlling
			*(fcdata->markbit2) &= 0xfffe;
			if (1 == wtlock)
			{//if (1 == wtlock)
			//	pantn += 1;
			//	if (2 == pantn)
			//	{//wireless terminal has disconnected with signaler machine;
				if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
				{
					memset(wlinfo,0,sizeof(wlinfo));
					gettimeofday(&ct,NULL);
					if (SUCCESS != wireless_info_report(wlinfo,ct.tv_sec,fcdata->wlmark,0x20))
					{
					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
				tcred = 0;
				close = 0;
				fpc = 0;
				pantn = 0;
				cp = 0;

				if (1 == socyfyes)
				{
					pthread_cancel(socyfpid);
					pthread_join(socyfpid,NULL);
					socyfyes = 0;
				}
						
				if ((dirc >= 0x05) && (dirc <= 0x0c))
				{//direct control happen
					rpc = 0;
					rpi = tscdata->timeconfig->TimeConfigList[socrettl][0].PhaseId;
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

					if ((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
					{
						if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
						{
							sinfo.conmode = 28;
							sinfo.color = 0x02;
							sinfo.time = pinfo.gftime + pinfo.ytime + pinfo.rtime;
							sinfo.stage = 0;
							sinfo.cyclet = 0;
							sinfo.phase = 0;
							sinfo.chans = 0;
							memset(sinfo.csta,0,sizeof(sinfo.csta));
							csta = sinfo.csta;
							for (i = 0; i < MAX_CHANNEL; i++)
							{
								if (0 == dirch[dirc-5][i])
									break;
								sinfo.chans += 1;
								tcsta = dirch[dirc-5][i];
								tcsta <<= 2;
								tcsta &= 0xfc;
								tcsta |= 0x02; //00000010-green 
								*csta = tcsta;
								csta++;
							}
							memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
							memset(sibuf,0,sizeof(sibuf));
							if (SUCCESS != status_info_report(sibuf,&sinfo))
							{
							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
							else
							{
								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
							}
						}
					}//if ((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
					if ((0 != wcc[0]) && (pinfo.gftime > 0))	
					{			
						if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
						{//report info to server actively
							sinfo.conmode = 28;
							sinfo.color = 0x03;
							sinfo.time = pinfo.gftime;
							sinfo.stage = 0;
							sinfo.cyclet = 0;
							sinfo.phase = 0;
														
							for (i = 0; i < MAX_CHANNEL; i++)
							{
								if (0 == wcc[i])
									break;
								for (j = 0; j < sinfo.chans; j++)
								{
									if (0 == sinfo.csta[j])
										break;
									tcsta = sinfo.csta[j];
									tcsta &= 0xfc;
									tcsta >>= 2;
									tcsta &= 0x3f;
									if (wcc[i] == tcsta)
									{
										sinfo.csta[j] &= 0xfc;
										sinfo.csta[j] |= 0x03; //00000011-green flash
										break;
									}
								}
							}
							memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
							memset(sibuf,0,sizeof(sibuf));
							if (SUCCESS != status_info_report(sibuf,&sinfo))
							{
							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
								printf("status pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
							else
							{
								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
							}
						}//report info to server actively
					}//if ((0 != wcc[0]) && (pinfo.gftime > 0))

					//green flash
					if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
					{
						memset(&tctd,0,sizeof(tctd));
						tctd.mode = 28;//traffic control
						tctd.pattern = *(fcdata->patternid);
						tctd.lampcolor = 0x02;
						tctd.lamptime = pinfo.gftime;
						tctd.phaseid = 0;
						tctd.stageline = pinfo.stageline;
						if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
						{
						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
							printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
					}
					#ifdef EMBED_CONFIGURE_TOOL
					if (*(fcdata->markbit2) & 0x0200)
					{
						memset(&tctd,0,sizeof(tctd));
                        tctd.mode = 28;//traffic control
                        tctd.pattern = *(fcdata->patternid);
                        tctd.lampcolor = 0x02;
                        tctd.lamptime = pinfo.gftime;
                        tctd.phaseid = 0;
                        tctd.stageline = pinfo.stageline;
						if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
						{
						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
							printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
					}
					#endif
					fbdata[1] = 28;
					fbdata[2] = pinfo.stageline;
					fbdata[3] = 0x02;
					fbdata[4] = pinfo.gftime;
					if (!wait_write_serial(*(fcdata->fbserial)))
					{
						if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
						{
						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(fcdata->markbit) |= 0x0800;
							gettimeofday(&ct,NULL);
							update_event_list(fcdata->ec,fcdata->el,1,16, \
													ct.tv_sec,fcdata->markbit);
							if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
														fcdata->softevent,fcdata->markbit))
							{
							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
								printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
						}
					}
					else
					{
					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
						printf("face board serial port cannot write,Line:%d\n",__LINE__);
					#endif
					}
					sendfaceInfoToBoard(fcdata,fbdata);
					if (pinfo.gftime > 0)
					{
						ngf = 0;
						while (1)
						{
							if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
							{
							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
								printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								*(fcdata->markbit) |= 0x0800;
							}
							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																	wcc,0x03,fcdata->markbit))
							{
							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
								printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
							to.tv_sec = 0;
							to.tv_usec = 500000;
							select(0,NULL,NULL,NULL,&to);
							if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
							{
							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
								printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
								*(fcdata->markbit) |= 0x0800;
							}
							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																		wcc,0x02,fcdata->markbit))
							{
							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
								printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
							to.tv_sec = 0;
							to.tv_usec = 500000;
							select(0,NULL,NULL,NULL,&to);
							
							ngf += 1;
							if (ngf >= pinfo.gftime)
								break;
						}
					}//if (pinfo.gftime > 0)
					if (1 == pce)
					{
					//current phase begin to yellow lamp
						if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
						{
						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
							printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(fcdata->markbit) |= 0x0800;
						}
						if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																wnpcc,0x01, fcdata->markbit))
						{
						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
							printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
						if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
						{
						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
							printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(fcdata->markbit) |= 0x0800;
						}
						if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																wpcc,0x00,fcdata->markbit))
						{
						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
							printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
					}
					else
					{
						if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
						{
						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
							printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(fcdata->markbit) |= 0x0800;
						}
						if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																wcc,0x01,fcdata->markbit))
						{
						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
							printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
					}

					if ((0 != wcc[0]) && (pinfo.ytime > 0))
					{
						if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
						{//report info to server actively
							sinfo.conmode = 28;
							sinfo.color = 0x01;
							sinfo.time = pinfo.ytime;
							sinfo.stage = 0;
							sinfo.cyclet = 0;
							sinfo.phase = 0;
											
							for (i = 0; i < MAX_CHANNEL; i++)
							{
								if (0 == wnpcc[i])
									break;
								for (j = 0; j < sinfo.chans; j++)
								{
									if (0 == sinfo.csta[j])
										break;
									tcsta = sinfo.csta[j];
									tcsta &= 0xfc;
									tcsta >>= 2;
									tcsta &= 0x3f;
									if (wnpcc[i] == tcsta)
									{
										sinfo.csta[j] &= 0xfc;
										sinfo.csta[j] |= 0x01; //00000001-yellow
										break;
									}
								}
							}
							for (i = 0; i < MAX_CHANNEL; i++)
							{
								if (0 == wpcc[i])
									break;
								for (j = 0; j < sinfo.chans; j++)
								{
									if (0 == sinfo.csta[j])
										break;
									tcsta = sinfo.csta[j];
									tcsta &= 0xfc;
									tcsta >>= 2;
									tcsta &= 0x3f;
									if (wpcc[i] == tcsta)
									{
										sinfo.csta[j] &= 0xfc;
										break;
									}
								}
							}
							memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
							memset(sibuf,0,sizeof(sibuf));
							if (SUCCESS != status_info_report(sibuf,&sinfo))
							{
							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
								printf("status pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
							else
							{
								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
							}
						}//report info to server actively
					}//if ((0 != wcc[0]) && (pinfo.ytime > 0))

					if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
					{
						memset(&tctd,0,sizeof(tctd));
						tctd.mode = 28;//traffic control
						tctd.pattern = *(fcdata->patternid);
						tctd.lampcolor = 0x01;
						tctd.lamptime = pinfo.ytime;
						tctd.phaseid = 0;
						tctd.stageline = pinfo.stageline;
						if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
						{
						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
							printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
					}
					#ifdef EMBED_CONFIGURE_TOOL
					if (*(fcdata->markbit2) & 0x0200)
					{
						memset(&tctd,0,sizeof(tctd));
                        tctd.mode = 28;//traffic control
                        tctd.pattern = *(fcdata->patternid);
                        tctd.lampcolor = 0x01;
                        tctd.lamptime = pinfo.ytime;
                        tctd.phaseid = 0;
                        tctd.stageline = pinfo.stageline;
						if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
						{
						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
							printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
					}
					#endif
					fbdata[1] = 28;
					fbdata[2] = pinfo.stageline;
					fbdata[3] = 0x01;
					fbdata[4] = pinfo.ytime;
					if (!wait_write_serial(*(fcdata->fbserial)))
					{
						if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
						{
						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(fcdata->markbit) |= 0x0800;
							gettimeofday(&ct,NULL);
							update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
							if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																	fcdata->softevent,fcdata->markbit))
							{	
							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
								printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
						}
					}
					else
					{
					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
						printf("face board serial port cannot write,Line:%d\n",__LINE__);
					#endif
					}
					sendfaceInfoToBoard(fcdata,fbdata);
					sleep(pinfo.ytime);

					//current phase begin to red lamp
					if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
					{
					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
						printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
						*(fcdata->markbit) |= 0x0800;
					}
					if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															wcc,0x00,fcdata->markbit))
					{
					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
						printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}

					if ((0 != wcc[0]) && (pinfo.rtime > 0))
					{
						if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
						{//report info to server actively
							sinfo.conmode = 28;
							sinfo.color = 0x00;
							sinfo.time = pinfo.rtime;
							sinfo.stage = 0;
							sinfo.cyclet = 0;
							sinfo.phase = 0;
														
							for (i = 0; i < MAX_CHANNEL; i++)
							{
								if (0 == wcc[i])
									break;
								for (j = 0; j < sinfo.chans; j++)
								{
									if (0 == sinfo.csta[j])
										break;
									tcsta = sinfo.csta[j];
									tcsta &= 0xfc;
									tcsta >>= 2;
									tcsta &= 0x3f;
									if (wcc[i] == tcsta)
									{
										sinfo.csta[j] &= 0xfc;
										break;
									}
								}
							}
							memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
							memset(sibuf,0,sizeof(sibuf));
							if (SUCCESS != status_info_report(sibuf,&sinfo))
							{
							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
							else
							{
								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
							}
						}//report info to server actively
					}//if ((0 != wcc[0]) && (pinfo.rtime > 0))
					*(fcdata->slnum) = 0;
					*(fcdata->stageid) = tscdata->timeconfig->TimeConfigList[socrettl][0].StageId;
				}//if (0 != directc)
				new_all_red(&ardata);

				*(fcdata->contmode) = contmode; //restore control mode;
				cp = 0;
				dirc = 0;
				*(fcdata->markbit2) &= 0xfffb;
				if (0 == socyes)
				{
					memset(&tcdata,0,sizeof(tcdata));
					memset(&pinfo,0,sizeof(pinfo));
					tcdata.fd = fcdata;
					tcdata.td = tscdata;
					tcdata.cs = chanstatus;
					tcdata.pi = &pinfo;
					int tcret = pthread_create(&socpid,NULL,(void *)start_optimize_coordinate_control,&tcdata);
					if (0 != tcret)
					{
					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
						printf("create thread err,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
						soc_end_part_child_thread();
						return FAIL;
					}
					socyes = 1;
				}
				continue;
		//		}//wireless terminal has disconnected with signaler machine;
			}//if (1 == wtlock)
			continue;
		}//time out
		if (cpret > 0)
		{//cpret > 0
			if (FD_ISSET(*(fcdata->conpipe),&nread))
			{
				memset(tcbuf,0,sizeof(tcbuf));
				read(*(fcdata->conpipe),tcbuf,sizeof(tcbuf));
				if ((0xB9 == tcbuf[0]) && ((0xED == tcbuf[8]) || (0xED == tcbuf[4]) || (0xED == tcbuf[5])))
				{//wireless terminal control
					pantn = 0;
					if (0x02 == tcbuf[3])
					{//pant
						continue;
					}//pant
					if ((0 == wllock) && (0 == keylock) && (0 == netlock))
					{//terminal control is valid only when wireless and key and net control is invalid;
						if (0x04 == tcbuf[3])
						{//control function
							if ((0x01 == tcbuf[4]) && (0 == wtlock))
							{//control will happen
								get_soc_status(&color,&leatime);
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
										
										get_soc_status(&color,&leatime);
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
                                        #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))

								wtlock = 1;
								tcred = 0;
								close = 0;
								fpc = 0;
								cp = 0;
								dirc = 0;
								soc_end_child_thread();//end main thread and its child thread
					//			new_all_red(&ardata);
								*(fcdata->markbit2) |= 0x0004;
						#if 0
								if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
        						{
        						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            						printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        						#endif
									*(fcdata->markbit) |= 0x0800;
        						}
						#endif
								*(fcdata->contmode) = 28;//wireless terminal control mode
						
								fbdata[1] = 28;
								fbdata[2] = pinfo.stageline;
								fbdata[3] = 0x02;
								fbdata[4] = 0;
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
                						update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                						if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																fcdata->softevent,fcdata->markbit))
                						{
                						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                   							printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                						#endif
                						}
									}
								}
								else
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("face board serial port cannot write,Line:%d\n",__LINE__);
								#endif
								}
								sendfaceInfoToBoard(fcdata,fbdata);
						//		#ifdef TIMING_DOWN_TIME
								if (*(fcdata->auxfunc) & 0x01)
								{//if (*(fcdata->auxfunc) & 0x01)
									if (!wait_write_serial(*(fcdata->bbserial)))
									{
										if (write(*(fcdata->bbserial),downti,sizeof(downti)) < 0)
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
									if (!wait_write_serial(*(fcdata->bbserial)))
									{
										if (write(*(fcdata->bbserial),edownti,sizeof(edownti)) < 0)
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
								}//if (*(fcdata->auxfunc) & 0x01)	
						//		#endif							

								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									sinfo.conmode = 28;
									sinfo.color = 0x02;
									sinfo.time = 0;
									sinfo.stage = pinfo.stageline;
									sinfo.cyclet = 0;
									sinfo.phase = 0;
									sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
															
									sinfo.chans = 0;
            						memset(sinfo.csta,0,sizeof(sinfo.csta));
            						csta = sinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						if (0 == pinfo.chan[i])
                    						break;
                						sinfo.chans += 1;
                						tcsta = pinfo.chan[i];
                						tcsta <<= 2;
                						tcsta &= 0xfc;
                						tcsta |= 0x02; //00000010-green
                						*csta = tcsta;
                						csta++;
            						}
									memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&sinfo))
            						{
            						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
	
								//send down time to configure tool
                            	if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            	{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("send control info to configure tool,File: %s,Line: %d\n", \
											__FILE__,__LINE__);
								#endif
                                	memset(&tctd,0,sizeof(tctd));
                                	tctd.mode = 28;
                                	tctd.pattern = *(fcdata->patternid);
                                	tctd.lampcolor = 0x02;
                                	tctd.lamptime = 0;
                                	tctd.phaseid = pinfo.phaseid;
                                	tctd.stageline = pinfo.stageline;
                                	if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
                                	{
                                	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    	printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                	#endif
                                	}
                            	}
								#ifdef EMBED_CONFIGURE_TOOL
                                if (*(fcdata->markbit2) & 0x0200)
                                {
									memset(&tctd,0,sizeof(tctd));
                                    tctd.mode = 28;
                                    tctd.pattern = *(fcdata->patternid);
                                    tctd.lampcolor = 0x02;
                                    tctd.lamptime = 0;
                                    tctd.phaseid = pinfo.phaseid;
                                    tctd.stageline = pinfo.stageline;
                                    if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
                                    {
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                                #endif
                            	if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    				pinfo.chan,0x02,fcdata->markbit))
                            	{
                            	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                	printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            	#endif
                            	}
									
								continue;
							}//control will happen
							else if ((0x00 == tcbuf[4]) && (1 == wtlock))
							{//cancel control
								wtlock = 0;
								tcred = 0;
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
                                        #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
								if (1 == socyfyes)
								{
									pthread_cancel(socyfpid);
									pthread_join(socyfpid,NULL);
									socyfyes = 0;
								}
							
								if ((dirc >= 5) && (dirc <= 0x0c))
								{//direction control happen
									rpc = 0;
                                	rpi = tscdata->timeconfig->TimeConfigList[socrettl][0].PhaseId;
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

									if ((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{
											sinfo.conmode = 28;
											sinfo.color = 0x02;
											sinfo.time = pinfo.gftime + pinfo.ytime + pinfo.rtime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
											sinfo.chans = 0;
            								memset(sinfo.csta,0,sizeof(sinfo.csta));
            								csta = sinfo.csta;
            								for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == dirch[dirc-5][i])
                    								break;
                								sinfo.chans += 1;
                								tcsta = dirch[dirc-5][i];
                								tcsta <<= 2;
                								tcsta &= 0xfc;
                								tcsta |= 0x02; //00000010-green 
                								*csta = tcsta;
                								csta++;
            								}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
											memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}
									}//if ((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
									if ((0 != wcc[0]) && (pinfo.gftime > 0))	
									{			
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 28;
											sinfo.color = 0x03;
											sinfo.time = pinfo.gftime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
															
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == wcc[i])
                    								break;
                								for (j = 0; j < sinfo.chans; j++)
                								{
                    								if (0 == sinfo.csta[j])
                        								break;
                    								tcsta = sinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (wcc[i] == tcsta)
                    								{
                        								sinfo.csta[j] &= 0xfc;
														sinfo.csta[j] |= 0x03; //00000011-green flash
														break;
                    								}
                								}
            								}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("status pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (pinfo.gftime > 0))
	
									//green flash
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&tctd,0,sizeof(tctd));
										tctd.mode = 28;//traffic control
										tctd.pattern = *(fcdata->patternid);
										tctd.lampcolor = 0x02;
										tctd.lamptime = pinfo.gftime;
										tctd.phaseid = 0;
										tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 28;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x02;
                                        tctd.lamptime = pinfo.gftime;
                                        tctd.phaseid = 0;
                                        tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = pinfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = pinfo.gftime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									if (pinfo.gftime > 0)
									{
										ngf = 0;
										while (1)
										{
											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x03,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);
									
											ngf += 1;
											if (ngf >= pinfo.gftime)
												break;
										}
									}//if (pinfo.gftime > 0)
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}

									if ((0 != wcc[0]) && (pinfo.ytime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 28;
											sinfo.color = 0x01;
											sinfo.time = pinfo.ytime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
															
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == wnpcc[i])
                    								break;
                								for (j = 0; j < sinfo.chans; j++)
                								{
                    								if (0 == sinfo.csta[j])
                        								break;
                    								tcsta = sinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (wnpcc[i] == tcsta)
                    								{
                        								sinfo.csta[j] &= 0xfc;
														sinfo.csta[j] |= 0x01; //00000001-yellow
														break;
                    								}
                								}
            								}
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == wpcc[i])
                                                	break;
                                            	for (j = 0; j < sinfo.chans; j++)
                                            	{
                                                	if (0 == sinfo.csta[j])
                                                    	break;
                                                	tcsta = sinfo.csta[j];
                                                	tcsta &= 0xfc;
                                                	tcsta >>= 2;
                                                	tcsta &= 0x3f;
                                                	if (wpcc[i] == tcsta)
                                                	{
                                                    	sinfo.csta[j] &= 0xfc;
														break;
                                                	}
                                            	}
                                        	}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("status pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (pinfo.ytime > 0))

									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            		{
                            			memset(&tctd,0,sizeof(tctd));
                            			tctd.mode = 28;//traffic control
                            			tctd.pattern = *(fcdata->patternid);
                            			tctd.lampcolor = 0x01;
                            			tctd.lamptime = pinfo.ytime;
                            			tctd.phaseid = 0;
                            			tctd.stageline = pinfo.stageline;
                            			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
                            			{
                            			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            			#endif
                            			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 28;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x01;
                                        tctd.lamptime = pinfo.ytime;
                                        tctd.phaseid = 0;
                                        tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = pinfo.stageline;
                            		fbdata[3] = 0x01;
                            		fbdata[4] = pinfo.ytime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                            			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                            			{
                            			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                			{
                                			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   				printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                		printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}

									if ((0 != wcc[0]) && (pinfo.rtime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 28;
											sinfo.color = 0x00;
											sinfo.time = pinfo.rtime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
															
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == wcc[i])
                                                	break;
                                            	for (j = 0; j < sinfo.chans; j++)
                                            	{
                                                	if (0 == sinfo.csta[j])
                                                    	break;
                                                	tcsta = sinfo.csta[j];
                                                	tcsta &= 0xfc;
                                                	tcsta >>= 2;
                                                	tcsta &= 0x3f;
                                                	if (wcc[i] == tcsta)
                                                	{
                                                    	sinfo.csta[j] &= 0xfc;
														break;
                                                	}
                                            	}
                                        	}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (pinfo.rtime > 0))
									*(fcdata->slnum) = 0;
                                    *(fcdata->stageid)=tscdata->timeconfig->TimeConfigList[socrettl][0].StageId;
								}//direction control happen
								
								new_all_red(&ardata);

								*(fcdata->contmode) = contmode; //restore control mode;
								cp = 0;
								dirc = 0;
								*(fcdata->markbit2) &= 0xfffb;
								if (0 == socyes)
								{
									memset(&tcdata,0,sizeof(tcdata));
									memset(&pinfo,0,sizeof(pinfo));
									tcdata.fd = fcdata;
									tcdata.td = tscdata;
									tcdata.cs = chanstatus;
									tcdata.pi = &pinfo;
									int tcret = pthread_create(&socpid,NULL, \
												(void *)start_optimize_coordinate_control,&tcdata);
									if (0 != tcret)
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("create thread err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
										soc_end_part_child_thread();
										return FAIL;
									}
									socyes = 1;
								}
								if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
								{
									memset(wlinfo,0,sizeof(wlinfo));
                                    gettimeofday(&ct,NULL);
                                    if (SUCCESS != wireless_info_report(wlinfo,ct.tv_sec,fcdata->wlmark,0x06))
                                    {
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("write base board serial err,File: %s,Line: %d\n", \
														__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("base board serial cannot write,File: %s,Line: %d\n", \
												__FILE__,__LINE__);
									#endif
									}
								}//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))

								if ((1 == tcred) || (1 == socyfyes) || (1 == close))
                            	{//if ((1 == tcred) || (1 == tcyfyes) || (1 == close))
									dirc = tcbuf[4];
									if ((dirc < 5) && (dirc > 0x0c))
                                    {
                                        continue;
                                    }

									fpc = 1;
									cp = 0;
                                	if (1 == socyfyes)
                                	{
                                    	pthread_cancel(socyfpid);
                                    	pthread_join(socyfpid,NULL);
                                    	socyfyes = 0;
                                	}
									if (1 != tcred)
									{
										new_all_red(&ardata);
									}
									tcred = 0;
                                    close = 0;
					//				#ifdef CLOSE_LAMP
									soc_set_lamp_color(*(fcdata->bbserial),clch,0x03);
									update_channel_status(fcdata->sockfd,chanstatus,clch,0x03,fcdata->markbit);
					//				#endif
									
                                	if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),dirch[dirc-5],0x02))
                                	{
                                	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
                                    	*(fcdata->markbit) |= 0x0800;
                                	}
                                	if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                                        				dirch[dirc-5],0x02,fcdata->markbit))
                                	{
                                	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    	printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                	#endif
                                	}
						
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										sinfo.conmode = 28;
										sinfo.color = 0x02;
										sinfo.time = 0;
										sinfo.stage = 0;
										sinfo.cyclet = 0;
										sinfo.phase = 0;
								/*
										sinfo.chans = 0;
            							memset(sinfo.csta,0,sizeof(sinfo.csta));
            							csta = sinfo.csta;
            							for (i = 0; i < MAX_CHANNEL; i++)
            							{
                							sinfo.chans += 1;
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
										sinfo.chans = 0;
										memset(sinfo.csta,0,sizeof(sinfo.csta));
                                        csta = sinfo.csta;
										for (i = 0; i < MAX_CHANNEL; i++)
                                        {
                                            if (0 == dirch[dirc-5][i])
                                                break;
                                            sinfo.chans += 1;
                                            tcsta = dirch[dirc-5][i];
                                            tcsta <<= 2;
                                            tcsta &= 0xfc;
                                            tcsta |= 0x02; //00000010-green 
                                            *csta = tcsta;
                                            csta++;
                                        }

										memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            							memset(sibuf,0,sizeof(sibuf));
            							if (SUCCESS != status_info_report(sibuf,&sinfo))
            							{
            							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
                                    	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
                            	}//if ((1 == tcred) || (1 == tcyfyes) || (1 == close))
								if (0 == fpc)
								{//if (0 == fpc)
									fpc = 1;
									cp = pinfo.phaseid;
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
									if ((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{
											sinfo.conmode = 28;
											sinfo.color = 0x02;
											sinfo.time = pinfo.gftime + pinfo.ytime + pinfo.rtime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
											sinfo.phase |= (0x01 << (cp - 1));
											sinfo.chans = 0;
            								memset(sinfo.csta,0,sizeof(sinfo.csta));
            								csta = sinfo.csta;
            								for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == cc[i])
                    								break;
                								sinfo.chans += 1;
                								tcsta = cc[i];
                								tcsta <<= 2;
                								tcsta &= 0xfc;
                								tcsta |= 0x02; //00000010-green 
                								*csta = tcsta;
                								csta++;
            								}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
											memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}
									}//if ((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
									if ((0 != wcc[0]) && (pinfo.gftime > 0))	
									{			
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 28;
											sinfo.color = 0x03;
											sinfo.time = pinfo.gftime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
											sinfo.phase |= (0x01 << (cp - 1));
	
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == wcc[i])
                    								break;
                								for (j = 0; j < sinfo.chans; j++)
                								{
                    								if (0 == sinfo.csta[j])
                        								break;
                    								tcsta = sinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (wcc[i] == tcsta)
                    								{
                        								sinfo.csta[j] &= 0xfc;
														sinfo.csta[j] |= 0x03; //00000011-green flash
														break;
                    								}
                								}
            								}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("status pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (pinfo.gftime > 0))
	
									//green flash
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&tctd,0,sizeof(tctd));
										tctd.mode = 28;//traffic control
										tctd.pattern = *(fcdata->patternid);
										tctd.lampcolor = 0x02;
										tctd.lamptime = pinfo.gftime;
										tctd.phaseid = cp;
										tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 28;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x02;
                                        tctd.lamptime = pinfo.gftime;
                                        tctd.phaseid = cp;
                                        tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = pinfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = pinfo.gftime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									if (pinfo.gftime > 0)
									{
										ngf = 0;
										while (1)
										{
											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x03,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);
									
											ngf += 1;
											if (ngf >= pinfo.gftime)
												break;
										}
									}//if (pinfo.gftime > 0)
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}

									if ((0 != wcc[0]) && (pinfo.ytime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 28;
											sinfo.color = 0x01;
											sinfo.time = pinfo.ytime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
											sinfo.phase |= (0x01 << (cp - 1));
				
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == wnpcc[i])
                    								break;
                								for (j = 0; j < sinfo.chans; j++)
                								{
                    								if (0 == sinfo.csta[j])
                        								break;
                    								tcsta = sinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (wnpcc[i] == tcsta)
                    								{
                        								sinfo.csta[j] &= 0xfc;
														sinfo.csta[j] |= 0x01; //00000001-yellow
														break;
                    								}
                								}
            								}
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == wpcc[i])
                                                	break;
                                            	for (j = 0; j < sinfo.chans; j++)
                                            	{
                                                	if (0 == sinfo.csta[j])
                                                    	break;
                                                	tcsta = sinfo.csta[j];
                                                	tcsta &= 0xfc;
                                                	tcsta >>= 2;
                                                	tcsta &= 0x3f;
                                                	if (wpcc[i] == tcsta)
                                                	{
                                                    	sinfo.csta[j] &= 0xfc;
														break;
                                                	}
                                            	}
                                        	}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("status pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (pinfo.ytime > 0))

									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            		{
                            			memset(&tctd,0,sizeof(tctd));
                            			tctd.mode = 28;//traffic control
                            			tctd.pattern = *(fcdata->patternid);
                            			tctd.lampcolor = 0x01;
                            			tctd.lamptime = pinfo.ytime;
                            			tctd.phaseid = cp;
                            			tctd.stageline = pinfo.stageline;
                            			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
                            			{
                            			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            			#endif
                            			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 28;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x01;
                                        tctd.lamptime = pinfo.ytime;
                                        tctd.phaseid = cp;
                                        tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = pinfo.stageline;
                            		fbdata[3] = 0x01;
                            		fbdata[4] = pinfo.ytime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                            			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                            			{
                            			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                			{
                                			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   				printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                		printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}

									if ((0 != wcc[0]) && (pinfo.rtime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 28;
											sinfo.color = 0x00;
											sinfo.time = pinfo.rtime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
											sinfo.phase |= (0x01 << (cp - 1));
	
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == wcc[i])
                                                	break;
                                            	for (j = 0; j < sinfo.chans; j++)
                                            	{
                                                	if (0 == sinfo.csta[j])
                                                    	break;
                                                	tcsta = sinfo.csta[j];
                                                	tcsta &= 0xfc;
                                                	tcsta >>= 2;
                                                	tcsta &= 0x3f;
                                                	if (wcc[i] == tcsta)
                                                	{
                                                    	sinfo.csta[j] &= 0xfc;
														break;
                                                	}
                                            	}
                                        	}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (pinfo.rtime > 0))									
									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),dirch[dirc-5],0x02))
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																		dirch[dirc-5],0x02,fcdata->markbit))
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
                            		}
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										sinfo.conmode = 28;
										sinfo.color = 0x02;
										sinfo.time = 0;
										sinfo.stage = 0;
										sinfo.cyclet = 0;
										sinfo.phase = 0;

										sinfo.chans = 0;
            							memset(sinfo.csta,0,sizeof(sinfo.csta));
            							csta = sinfo.csta;
            							for (i = 0; i < MAX_CHANNEL; i++)
            							{
                							if (0 == dirch[dirc-5][i])
                    							break;
                							sinfo.chans += 1;
                							tcsta = dirch[dirc-5][i];
                							tcsta <<= 2;
                							tcsta &= 0xfc;
                							tcsta |= 0x02;
                							*csta = tcsta;
                							csta++;
            							}
										memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));					
            							memset(sibuf,0,sizeof(sibuf));
            							if (SUCCESS != status_info_report(sibuf,&sinfo))
            							{
            							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
										memset(&tctd,0,sizeof(tctd));
										tctd.mode = 28;//traffic control
										tctd.pattern = *(fcdata->patternid);
										tctd.lampcolor = 0x02;
										tctd.lamptime = 0;
										tctd.phaseid = 0;
										tctd.stageline = 0;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 28;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x02;
                                        tctd.lamptime = 0;
                                        tctd.phaseid = 0;
                                        tctd.stageline = 0;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = pinfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = 0;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
									
									if ((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{
											sinfo.conmode = 28;
											sinfo.color = 0x02;
											sinfo.time = pinfo.gftime + pinfo.ytime + pinfo.rtime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
											sinfo.chans = 0;
            								memset(sinfo.csta,0,sizeof(sinfo.csta));
            								csta = sinfo.csta;
            								for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == cc[i])
                    								break;
                								sinfo.chans += 1;
                								tcsta = cc[i];
                								tcsta <<= 2;
                								tcsta &= 0xfc;
                								tcsta |= 0x02; //00000010-green 
                								*csta = tcsta;
                								csta++;
            								}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
											memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}
									}//if ((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
									if ((0 != wcc[0]) && (pinfo.gftime > 0))	
									{			
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 28;
											sinfo.color = 0x03;
											sinfo.time = pinfo.gftime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
	
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == wcc[i])
                    								break;
                								for (j = 0; j < sinfo.chans; j++)
                								{
                    								if (0 == sinfo.csta[j])
                        								break;
                    								tcsta = sinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (wcc[i] == tcsta)
                    								{
                        								sinfo.csta[j] &= 0xfc;
														sinfo.csta[j] |= 0x03; //00000011-green flash
														break;
                    								}
                								}
            								}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("status pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (pinfo.gftime > 0))
	
									//green flash
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&tctd,0,sizeof(tctd));
										tctd.mode = 28;//traffic control
										tctd.pattern = *(fcdata->patternid);
										tctd.lampcolor = 0x02;
										tctd.lamptime = pinfo.gftime;
										tctd.phaseid = 0;
										tctd.stageline = 0;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 28;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x02;
                                        tctd.lamptime = pinfo.gftime;
                                        tctd.phaseid = 0;
                                        tctd.stageline = 0;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = 0;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = pinfo.gftime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									if (pinfo.gftime > 0)
									{
										ngf = 0;
										while (1)
										{
											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x03,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);
									
											ngf += 1;
											if (ngf >= pinfo.gftime)
												break;
										}
									}//if (pinfo.gftime > 0)
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}

									if ((0 != wcc[0]) && (pinfo.ytime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 28;
											sinfo.color = 0x01;
											sinfo.time = pinfo.ytime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
				
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == wnpcc[i])
                    								break;
                								for (j = 0; j < sinfo.chans; j++)
                								{
                    								if (0 == sinfo.csta[j])
                        								break;
                    								tcsta = sinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (wnpcc[i] == tcsta)
                    								{
                        								sinfo.csta[j] &= 0xfc;
														sinfo.csta[j] |= 0x01; //00000001-yellow
														break;
                    								}
                								}
            								}
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == wpcc[i])
                                                	break;
                                            	for (j = 0; j < sinfo.chans; j++)
                                            	{
                                                	if (0 == sinfo.csta[j])
                                                    	break;
                                                	tcsta = sinfo.csta[j];
                                                	tcsta &= 0xfc;
                                                	tcsta >>= 2;
                                                	tcsta &= 0x3f;
                                                	if (wpcc[i] == tcsta)
                                                	{
                                                    	sinfo.csta[j] &= 0xfc;
														break;
                                                	}
                                            	}
                                        	}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("status pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (pinfo.ytime > 0))

									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            		{
                            			memset(&tctd,0,sizeof(tctd));
                            			tctd.mode = 28;//traffic control
                            			tctd.pattern = *(fcdata->patternid);
                            			tctd.lampcolor = 0x01;
                            			tctd.lamptime = pinfo.ytime;
                            			tctd.phaseid = 0;
                            			tctd.stageline = 0;
                            			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
                            			{
                            			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            			#endif
                            			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 28;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x01;
                                        tctd.lamptime = pinfo.ytime;
                                        tctd.phaseid = 0;
                                        tctd.stageline = 0;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = 0;
                            		fbdata[3] = 0x01;
                            		fbdata[4] = pinfo.ytime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                            			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                            			{
                            			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                			{
                                			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   				printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                		printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}

									if ((0 != wcc[0]) && (pinfo.rtime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 28;
											sinfo.color = 0x00;
											sinfo.time = pinfo.rtime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
	
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == wcc[i])
                                                	break;
                                            	for (j = 0; j < sinfo.chans; j++)
                                            	{
                                                	if (0 == sinfo.csta[j])
                                                    	break;
                                                	tcsta = sinfo.csta[j];
                                                	tcsta &= 0xfc;
                                                	tcsta >>= 2;
                                                	tcsta &= 0x3f;
                                                	if (wcc[i] == tcsta)
                                                	{
                                                    	sinfo.csta[j] &= 0xfc;
														break;
                                                	}
                                            	}
                                        	}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (pinfo.rtime > 0))									
									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pnc,0x02))
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			pnc,0x02,fcdata->markbit))
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
                            		}
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										sinfo.conmode = 28;
										sinfo.color = 0x02;
										sinfo.time = 0;
										sinfo.stage = 0;
										sinfo.cyclet = 0;
										sinfo.phase = 0;

										sinfo.chans = 0;
            							memset(sinfo.csta,0,sizeof(sinfo.csta));
            							csta = sinfo.csta;
            							for (i = 0; i < MAX_CHANNEL; i++)
            							{
                							if (0 == pnc[i])
                    							break;
                							sinfo.chans += 1;
                							tcsta = pnc[i];
                							tcsta <<= 2;
                							tcsta &= 0xfc;
                							tcsta |= 0x02;
                							*csta = tcsta;
                							csta++;
            							}
										memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));				
            							memset(sibuf,0,sizeof(sibuf));
            							if (SUCCESS != status_info_report(sibuf,&sinfo))
            							{
            							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
										memset(&tctd,0,sizeof(tctd));
										tctd.mode = 28;//traffic control
										tctd.pattern = *(fcdata->patternid);
										tctd.lampcolor = 0x02;
										tctd.lamptime = 0;
										tctd.phaseid = 0;
										tctd.stageline = 0;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 28;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x02;
                                        tctd.lamptime = 0;
                                        tctd.phaseid = 0;
                                        tctd.stageline = 0;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = pinfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = 0;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
                                    if (SUCCESS != wireless_info_report(wlinfo,ct.tv_sec,fcdata->wlmark,dce))
                                    {
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
							{//terminal step by step
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
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("write base board serial err,File: %s,Line: %d\n", \
														__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("base board serial cannot write,File: %s,Line: %d\n", \
												__FILE__,__LINE__);
									#endif
									}
								}//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))

								if ((1 == tcred) || (1 == socyfyes) || (1 == close))
								{//if ((1 == tcred) || (1 == tcyfyes) || (1 == close))
									if (1 == socyfyes)
                                    {
                                        pthread_cancel(socyfpid);
                                        pthread_join(socyfpid,NULL);
                                        socyfyes = 0;
                                    }
                                    if (1 != tcred)
                                    {
                                        new_all_red(&ardata);
                                    }
                                    tcred = 0;
                                    close = 0;
					//				#ifdef CLOSE_LAMP
                                    soc_set_lamp_color(*(fcdata->bbserial),clch,0x03);
									update_channel_status(fcdata->sockfd,chanstatus,clch,0x03,fcdata->markbit);
                   //                 #endif	
									if ((dirc < 0x05) || (dirc > 0x0c))
									{//not have phase control
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                                		{
                                		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
                                    		*(fcdata->markbit) |= 0x0800;
                                		}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    									pinfo.chan,0x02,fcdata->markbit))
                                		{
                                		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    		printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               			#endif
                                		}

										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 28;
											sinfo.color = 0x02;
											sinfo.time = 0;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
											sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
										/*	
											sinfo.chans = 0;
            								memset(sinfo.csta,0,sizeof(sinfo.csta));
            								csta = sinfo.csta;
            								for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								sinfo.chans += 1;
                								tcsta = i + 1;
                								tcsta <<= 2;
                								tcsta &= 0xfc;
												for (j = 0; j < MAX_CHANNEL; j++)
												{
													if (0 == pinfo.chan[j])
														break;
													if ((i+1) == pinfo.chan[j])
													{
														tcsta |= 0x02;
														break;
													}
												}
                								*csta = tcsta;
                								csta++;
            								}
										*/
											sinfo.chans = 0;
											memset(sinfo.csta,0,sizeof(sinfo.csta));
                                            csta = sinfo.csta;
											for (i = 0; i < MAX_CHANNEL; i++)
											{
												if (0 == pinfo.chan[i])
													break;
												sinfo.chans += 1;
												tcsta = pinfo.chan[i];
												tcsta <<= 2;
												tcsta &= 0xfc;
												tcsta |= 0x02; //00000010-green 
												*csta = tcsta;
												csta++;
											}

											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
										if (SUCCESS!=soc_set_lamp_color(*(fcdata->bbserial),dirch[dirc-5],0x02))
                                		{
                                		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
                                    		*(fcdata->markbit) |= 0x0800;
                                		}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    									dirch[dirc-5],0x02,fcdata->markbit))
                                		{
                                		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    		printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               			#endif
                                		}

										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 28;
											sinfo.color = 0x02;
											sinfo.time = 0;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
										/*					
											sinfo.chans = 0;
            								memset(sinfo.csta,0,sizeof(sinfo.csta));
            								csta = sinfo.csta;
            								for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								sinfo.chans += 1;
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
											sinfo.chans = 0;
											memset(sinfo.csta,0,sizeof(sinfo.csta));
                                            csta = sinfo.csta;
											for (i = 0; i < MAX_CHANNEL; i++)
											{
												if (0 == dirch[dirc-5][i])
													break;
												sinfo.chans += 1;
												tcsta = dirch[dirc-5][i];
												tcsta <<= 2;
												tcsta &= 0xfc;
												tcsta |= 0x02; //00000010-green 
												*csta = tcsta;
												csta++;
											}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
                                    	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
								}//if ((1 == tcred) || (1 == tcyfyes) || (1 == close))
								if ((dirc < 0x05) || (dirc > 0x0c))
								{//not have direction control
									if((0==pinfo.cchan[0])&&(pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 28;
											sinfo.color = 0x02;
											sinfo.time = pinfo.gftime + pinfo.ytime + pinfo.rtime;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
											sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
											sinfo.chans = 0;
            								memset(sinfo.csta,0,sizeof(sinfo.csta));
            								csta = sinfo.csta;
            								for (i = 0; i < MAX_CHANNEL; i++)
            								{
												if (0 == pinfo.chan[i])
													break;
                								sinfo.chans += 1;
                								tcsta = pinfo.chan[i];
                								tcsta <<= 2;
                								tcsta &= 0xfc;
												tcsta |= 0x02;
                								*csta = tcsta;
                								csta++;
            								}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//info.cchan[0])&&(pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0))

									if ((0 != pinfo.cchan[0]) && (pinfo.gftime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 28;
											sinfo.color = 0x03;
											sinfo.time = pinfo.gftime;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
											sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
									
											for (i = 0; i < MAX_CHANNEL; i++)
                                			{
                                				if (0 == pinfo.cchan[i])
                                        			break;
                                    			for (j = 0; j < sinfo.chans; j++)
                                    			{
                                        			if (0 == sinfo.csta[j])
                                            			break;
                                        			tcsta = sinfo.csta[j];
                                        			tcsta &= 0xfc;
                                        			tcsta >>= 2;
                                        			tcsta &= 0x3f;
                                        			if (pinfo.cchan[i] == tcsta)
                                        			{
                                            			sinfo.csta[j] &= 0xfc;
                                           				sinfo.csta[j] |= 0x03; //00000001-green flash
														break;
                                        			}
                                    			}
                                			}								
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));	
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != pinfo.cchan[0]) && (pinfo.gftime > 0))

									//current phase begin to green flash
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&tctd,0,sizeof(tctd));
										tctd.mode = 28;//traffic control
										tctd.pattern = *(fcdata->patternid);
										tctd.lampcolor = 0x02;
										tctd.lamptime = pinfo.gftime;
										tctd.phaseid = pinfo.phaseid;
										tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 28;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x02;
                                        tctd.lamptime = pinfo.gftime;
                                        tctd.phaseid = pinfo.phaseid;
                                        tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = pinfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = pinfo.gftime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                                		if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                		{
                                		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                    		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                    		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                    		{
                                    		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
                                    		}
                                		}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									if (pinfo.gftime > 0)
									{	
									ngf = 0;
									while (1)
									{
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x03))
        								{
        								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            								printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															pinfo.cchan,0x03,fcdata->markbit))
        								{
        								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            								printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
        								#endif
        								}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);

										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x02))
        								{
        								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            								printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    									pinfo.cchan,0x02,fcdata->markbit))
                                		{
                                		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    		printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                		#endif
                                		}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);
								
										ngf += 1;
										if (ngf >= pinfo.gftime)
											break;
									}
									}//if (pinfo.gftime > 0)
									if (1 == (pinfo.cpcexist))
									{
										//current phase begin to yellow lamp
										if (SUCCESS!=soc_set_lamp_color(*(fcdata->bbserial),pinfo.cnpchan,0x01))
        								{
        								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            								printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
													pinfo.cnpchan,0x01, fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if(SUCCESS!=soc_set_lamp_color(*(fcdata->bbserial),pinfo.cpchan,0x00))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																	pinfo.cpchan,0x00,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x01))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														pinfo.cchan,0x01,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}

									if ((0 != pinfo.cchan[0]) && (pinfo.ytime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 28;
											sinfo.color = 0x01;
											sinfo.time = pinfo.ytime;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
											sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
															
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == pinfo.cnpchan[i])
                    								break;
                								for (j = 0; j < sinfo.chans; j++)
                								{
                    								if (0 == sinfo.csta[j])
                        								break;
                    								tcsta = sinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (pinfo.cnpchan[i] == tcsta)
                    								{
                        								sinfo.csta[j] &= 0xfc;
														sinfo.csta[j] |= 0x01; //00000001-yellow
														break;
                    								}
                								}
            								}
											for (i = 0; i < MAX_CHANNEL; i++)
                                			{
                                				if (0 == pinfo.cpchan[i])
                                        			break;
                                    			for (j = 0; j < sinfo.chans; j++)
                                    			{
                                        			if (0 == sinfo.csta[j])
                                            			break;
                                        			tcsta = sinfo.csta[j];
                                        			tcsta &= 0xfc;
                                        			tcsta >>= 2;
                                        			tcsta &= 0x3f;
                                        			if (pinfo.cpchan[i] == tcsta)
                                        			{
                                            			sinfo.csta[j] &= 0xfc;
														break;
                                        			}
                                    			}
                                			}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != pinfo.cchan[0]) && (pinfo.ytime > 0))

									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            		{
                               			memset(&tctd,0,sizeof(tctd));
                               			tctd.mode = 28;//traffic control
                               			tctd.pattern = *(fcdata->patternid);
                               			tctd.lampcolor = 0x01;
                               			tctd.lamptime = pinfo.ytime;
                               			tctd.phaseid = pinfo.phaseid;
                               			tctd.stageline = pinfo.stageline;
                               			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
                               			{
                               			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               			#endif
                               			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 28;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x01;
                                        tctd.lamptime = pinfo.ytime;
                                        tctd.phaseid = pinfo.phaseid;
                                        tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = pinfo.stageline;
                            		fbdata[3] = 0x01;
                            		fbdata[4] = pinfo.ytime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                       			printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x00))
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															pinfo.cchan,0x00,fcdata->markbit))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}

									if ((0 != pinfo.cchan[0]) && (pinfo.rtime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 28;
											sinfo.color = 0x00;
											sinfo.time = pinfo.rtime;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
											sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
															
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == pinfo.cchan[i])
                    								break;
                								for (j = 0; j < sinfo.chans; j++)
                								{
                    								if (0 == sinfo.csta[j])
                        								break;
                    								tcsta = sinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (pinfo.cchan[i] == tcsta)
                    								{
                        								sinfo.csta[j] &= 0xfc;
														break;
                    								}
                								}
            								}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));	
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != pinfo.cchan[0]) && (pinfo.rtime > 0))

									if (pinfo.rtime > 0)
									{
										if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            			{
                                			memset(&tctd,0,sizeof(tctd));
                                			tctd.mode = 28;//traffic control
                                			tctd.pattern = *(fcdata->patternid);
                                			tctd.lampcolor = 0x00;
                                			tctd.lamptime = pinfo.rtime;
                                			tctd.phaseid = pinfo.phaseid;
                                			tctd.stageline = pinfo.stageline;
                                			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
                                			{
                                			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                            			}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&tctd,0,sizeof(tctd));
                                            tctd.mode = 28;//traffic control
                                            tctd.pattern = *(fcdata->patternid);
                                            tctd.lampcolor = 0x00;
                                            tctd.lamptime = pinfo.rtime;
                                            tctd.phaseid = pinfo.phaseid;
                                            tctd.stageline = pinfo.stageline;
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#endif
										fbdata[1] = 28;
                                		fbdata[2] = pinfo.stageline;
                                		fbdata[3] = 0x00;
                                		fbdata[4] = pinfo.rtime;
                                		if (!wait_write_serial(*(fcdata->fbserial)))
                                		{
                                    		if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                    		{
                                    		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
                                        		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                        		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                        		{
                                        		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                            		printf("genfile err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                        		#endif
                                        		}
                                    		}
                                		}
                                		else
                                		{
                                		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                		#endif
                                		}
										sleep(pinfo.rtime);
									}

									*(fcdata->slnum) += 1;
									*(fcdata->stageid) = \
										tscdata->timeconfig->TimeConfigList[socrettl][*(fcdata->slnum)].StageId;
							if (0==(tscdata->timeconfig->TimeConfigList[socrettl][*(fcdata->slnum)].StageId))
									{
										*(fcdata->slnum) = 0;
										*(fcdata->stageid) = \
										tscdata->timeconfig->TimeConfigList[socrettl][*(fcdata->slnum)].StageId;
									}
									//get phase info of next phase
									memset(&pinfo,0,sizeof(socpinfo_t));
        							if (SUCCESS != \
										soc_get_phase_info(fcdata,tscdata,socrettl,*(fcdata->slnum),&pinfo))
        							{
        							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            							printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
        							#endif
										soc_end_part_child_thread();
            							return FAIL;
        							}
        							*(fcdata->phaseid) = 0;
        							*(fcdata->phaseid) |= (0x01 << (pinfo.phaseid - 1));

									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																pinfo.chan,0x02,fcdata->markbit))
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                		printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
                            		}
									ncmode = *(fcdata->contmode);
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										sinfo.conmode = 28;
										sinfo.color = 0x02;
										sinfo.time = 0;
										sinfo.stage = pinfo.stageline;
										sinfo.cyclet = 0;
										sinfo.phase = 0;
										sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
															
										sinfo.chans = 0;
            							memset(sinfo.csta,0,sizeof(sinfo.csta));
            							csta = sinfo.csta;
            							for (i = 0; i < MAX_CHANNEL; i++)
            							{
											if (0 == pinfo.chan[i])
												break;
                							sinfo.chans += 1;
                							tcsta = pinfo.chan[i];
                							tcsta <<= 2;
                							tcsta &= 0xfc;
											tcsta |= 0x02;
                							*csta = tcsta;
                							csta++;
            							}
										memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            							memset(sibuf,0,sizeof(sibuf));
            							if (SUCCESS != status_info_report(sibuf,&sinfo))
            							{
            							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
										memset(&tctd,0,sizeof(tctd));
										tctd.mode = 28;//traffic control
										tctd.pattern = *(fcdata->patternid);
										tctd.lampcolor = 0x02;
										tctd.lamptime = 0;
										tctd.phaseid = pinfo.phaseid;
										tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 28;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x02;
                                        tctd.lamptime = 0;
                                        tctd.phaseid = pinfo.phaseid;
                                        tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = pinfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = 0;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                                		if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                		{
                                		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                    		{
                                    		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
                                    		}
                                		}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
								}// not have direction control
								else
								{//have direction control
									//default return to first stage;
									*(fcdata->slnum) = 0;
									*(fcdata->stageid) = \
                                               tscdata->timeconfig->TimeConfigList[socrettl][0].StageId;
                                    memset(&pinfo,0,sizeof(socpinfo_t));
                                    if(SUCCESS != soc_get_phase_info(fcdata,tscdata,socrettl,0,&pinfo))
                                    {
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        printf("phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                        soc_end_part_child_thread();
                                        return FAIL;
                                    }
                                    *(fcdata->phaseid) = 0;
                                    *(fcdata->phaseid) |= (0x01 << (pinfo.phaseid - 1));

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
											if (0 == pinfo.chan[j])
												break;
											if (dirch[dirc-5][i] == pinfo.chan[j])
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

									if((0==wcc[0])&&(pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 28;
											sinfo.color = 0x02;
											sinfo.time = pinfo.gftime + pinfo.ytime + pinfo.rtime;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
											sinfo.chans = 0;
            								memset(sinfo.csta,0,sizeof(sinfo.csta));
            								csta = sinfo.csta;
            								for (i = 0; i < MAX_CHANNEL; i++)
            								{
												if (0 == dirch[dirc-5][i])
													break;
                								sinfo.chans += 1;
                								tcsta = dirch[dirc-5][i];
                								tcsta <<= 2;
                								tcsta &= 0xfc;
												tcsta |= 0x02;
                								*csta = tcsta;
                								csta++;
            								}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//info.cchan[0])&&(pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0))

									if ((0 != wcc[0]) && (pinfo.gftime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 28;
											sinfo.color = 0x03;
											sinfo.time = pinfo.gftime;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
									
											for (i = 0; i < MAX_CHANNEL; i++)
                                			{
                                				if (0 == wcc[i])
                                        			break;
                                    			for (j = 0; j < sinfo.chans; j++)
                                    			{
                                        			if (0 == sinfo.csta[j])
                                            			break;
                                        			tcsta = sinfo.csta[j];
                                        			tcsta &= 0xfc;
                                        			tcsta >>= 2;
                                        			tcsta &= 0x3f;
                                        			if (wcc[i] == tcsta)
                                        			{
                                            			sinfo.csta[j] &= 0xfc;
                                           				sinfo.csta[j] |= 0x03; //00000001-green flash
														break;
                                        			}
                                    			}
                                			}								
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));	
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != pinfo.cchan[0]) && (pinfo.gftime > 0))

									//current phase begin to green flash
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&tctd,0,sizeof(tctd));
										tctd.mode = 28;//traffic control
										tctd.pattern = *(fcdata->patternid);
										tctd.lampcolor = 0x02;
										tctd.lamptime = pinfo.gftime;
										tctd.phaseid = 0;
										tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 28;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x02;
                                        tctd.lamptime = pinfo.gftime;
                                        tctd.phaseid = 0;
                                        tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = pinfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = pinfo.gftime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                                		if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                		{
                                		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                    		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                    		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                    		{
                                    		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
                                    		}
                                		}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									if (pinfo.gftime > 0)
									{	
										ngf = 0;
										while (1)
										{
											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x03,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("updatechannelerr,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("setgreenlamperr,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x02,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("updatechannelerr,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);
									
											ngf += 1;
											if (ngf >= pinfo.gftime)
												break;
										}
									}//if (pinfo.gftime > 0)
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS!=soc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            								printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														wcc,0x01,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}

									if ((0 != wcc[0]) && (pinfo.ytime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 28;
											sinfo.color = 0x01;
											sinfo.time = pinfo.ytime;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
															
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == wnpcc[i])
                    								break;
                								for (j = 0; j < sinfo.chans; j++)
                								{
                    								if (0 == sinfo.csta[j])
                        								break;
                    								tcsta = sinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (wnpcc[i] == tcsta)
                    								{
                        								sinfo.csta[j] &= 0xfc;
														sinfo.csta[j] |= 0x01; //00000001-yellow
														break;
                    								}
                								}
            								}
											for (i = 0; i < MAX_CHANNEL; i++)
                                			{
                                				if (0 == wpcc[i])
                                        			break;
                                    			for (j = 0; j < sinfo.chans; j++)
                                    			{
                                        			if (0 == sinfo.csta[j])
                                            			break;
                                        			tcsta = sinfo.csta[j];
                                        			tcsta &= 0xfc;
                                        			tcsta >>= 2;
                                        			tcsta &= 0x3f;
                                        			if (wpcc[i] == tcsta)
                                        			{
                                            			sinfo.csta[j] &= 0xfc;
														break;
                                        			}
                                    			}
                                			}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != pinfo.cchan[0]) && (pinfo.ytime > 0))

									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            		{
                               			memset(&tctd,0,sizeof(tctd));
                               			tctd.mode = 28;//traffic control
                               			tctd.pattern = *(fcdata->patternid);
                               			tctd.lampcolor = 0x01;
                               			tctd.lamptime = pinfo.ytime;
                               			tctd.phaseid = 0;
                               			tctd.stageline = pinfo.stageline;
                               			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
                               			{
                               			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               			#endif
                               			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 28;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x01;
                                        tctd.lamptime = pinfo.ytime;
                                        tctd.phaseid = 0;
                                        tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = pinfo.stageline;
                            		fbdata[3] = 0x01;
                            		fbdata[4] = pinfo.ytime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                       			printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															wcc,0x00,fcdata->markbit))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}

									if ((0 != wcc[0]) && (pinfo.rtime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 28;
											sinfo.color = 0x00;
											sinfo.time = pinfo.rtime;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
															
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == wcc[i])
                    								break;
                								for (j = 0; j < sinfo.chans; j++)
                								{
                    								if (0 == sinfo.csta[j])
                        								break;
                    								tcsta = sinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (wcc[i] == tcsta)
                    								{
                        								sinfo.csta[j] &= 0xfc;
														break;
                    								}
                								}
            								}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));	
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != pinfo.cchan[0]) && (pinfo.rtime > 0))

									if (pinfo.rtime > 0)
									{
										if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            			{
                                			memset(&tctd,0,sizeof(tctd));
                                			tctd.mode = 28;//traffic control
                                			tctd.pattern = *(fcdata->patternid);
                                			tctd.lampcolor = 0x00;
                                			tctd.lamptime = pinfo.rtime;
                                			tctd.phaseid = 0;
                                			tctd.stageline = pinfo.stageline;
                                			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
                                			{
                                			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                            			}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&tctd,0,sizeof(tctd));
                                            tctd.mode = 28;//traffic control
                                            tctd.pattern = *(fcdata->patternid);
                                            tctd.lampcolor = 0x00;
                                            tctd.lamptime = pinfo.rtime;
                                            tctd.phaseid = 0;
                                            tctd.stageline = pinfo.stageline;
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#endif
										fbdata[1] = 28;
                                		fbdata[2] = pinfo.stageline;
                                		fbdata[3] = 0x00;
                                		fbdata[4] = pinfo.rtime;
                                		if (!wait_write_serial(*(fcdata->fbserial)))
                                		{
                                    		if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                    		{
                                    		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
                                        		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                        		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                        		{
                                        		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                            		printf("file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        		#endif
                                        		}
                                    		}
                                		}
                                		else
                                		{
                                		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                		#endif
                                		}
										sleep(pinfo.rtime);
									}
									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			pinfo.chan,0x02,fcdata->markbit))
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
                            		}
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										sinfo.conmode = 28;
										sinfo.color = 0x02;
										sinfo.time = 0;
										sinfo.stage = 0;
										sinfo.cyclet = 0;
										sinfo.phase = 0;
										sinfo.phase |= (0x01 << (pinfo.phaseid - 1));

										sinfo.chans = 0;
            							memset(sinfo.csta,0,sizeof(sinfo.csta));
            							csta = sinfo.csta;
            							for (i = 0; i < MAX_CHANNEL; i++)
            							{
                							if (0 == pinfo.chan[i])
                    							break;
                							sinfo.chans += 1;
                							tcsta = pinfo.chan[i];
                							tcsta <<= 2;
                							tcsta &= 0xfc;
                							tcsta |= 0x02;
                							*csta = tcsta;
                							csta++;
            							}
										memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));					
            							memset(sibuf,0,sizeof(sibuf));
            							if (SUCCESS != status_info_report(sibuf,&sinfo))
            							{
            							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
										memset(&tctd,0,sizeof(tctd));
										tctd.mode = 28;//traffic control
										tctd.pattern = *(fcdata->patternid);
										tctd.lampcolor = 0x02;
										tctd.lamptime = 0;
										tctd.phaseid = pinfo.phaseid;
										tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 28;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x02;
                                        tctd.lamptime = 0;
                                        tctd.phaseid = pinfo.phaseid;
                                        tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = pinfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = 0;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
							}//terminal step by step
							else if ((0x03 == tcbuf[4]) && (1 == wtlock))
							{//yellow flash
								dirc = 0;
								tcred = 0;
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
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("write base board serial err,File: %s,Line: %d\n", \
														__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("base board serial cannot write,File: %s,Line: %d\n", \
												__FILE__,__LINE__);
									#endif
									}
								}//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))

								if (0 == socyfyes)
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
									int yfret = pthread_create(&socyfpid,NULL,(void *)soc_yellow_flash,&yfdata);
									if (0 != yfret)
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										soc_end_part_child_thread();

										return FAIL;
									}
									socyfyes = 1;
								}

								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									sinfo.conmode = 28;
									sinfo.color = 0x05;
									sinfo.time = 0;
									sinfo.stage = 0;
									sinfo.cyclet = 0;
									sinfo.phase = 0;

									sinfo.chans = 0;
            						memset(sinfo.csta,0,sizeof(sinfo.csta));
            						csta = sinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						sinfo.chans += 1;
                						tcsta = i+1;
                						tcsta <<= 2;
                						tcsta &= 0xfc;
                						*csta = tcsta;
                						csta++;
            						}
									memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));						
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&sinfo))
            						{
            						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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

								tpp.func[0] = tcbuf[4];
								memset(terbuf,0,sizeof(terbuf));
								if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
								{
									if (!wait_write_serial(*(fcdata->bbserial)))
									{
										if (write(*(fcdata->bbserial),terbuf,sizeof(terbuf)) < 0)
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("write base board serial err,File: %s,Line: %d\n", \
														__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("base board serial cannot write,File: %s,Line: %d\n", \
												__FILE__,__LINE__);
									#endif
									}
								}//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))

								if (1 == socyfyes)
								{
									pthread_cancel(socyfpid);
									pthread_join(socyfpid,NULL);
									socyfyes = 0;
								}
					
								if (0 == tcred)
								{	
									new_all_red(&ardata);
									tcred = 1;
								}
								
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									sinfo.conmode = 28;
									sinfo.color = 0x00;
									sinfo.time = 0;
									sinfo.stage = 0;
									sinfo.cyclet = 0;
									sinfo.phase = 0;

									sinfo.chans = 0;
            						memset(sinfo.csta,0,sizeof(sinfo.csta));
            						csta = sinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						sinfo.chans += 1;
                						tcsta = i+1;
                						tcsta <<= 2;
                						tcsta &= 0xfc;
                						*csta = tcsta;
                						csta++;
            						}
									memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));					
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&sinfo))
            						{
            						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
								tcred = 0;
								cp = 0;

								tpp.func[0] = tcbuf[4];
								memset(terbuf,0,sizeof(terbuf));
								if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
								{
									if (!wait_write_serial(*(fcdata->bbserial)))
									{
										if (write(*(fcdata->bbserial),terbuf,sizeof(terbuf)) < 0)
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("write base board serial err,File: %s,Line: %d\n", \
														__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("base board serial cannot write,File: %s,Line: %d\n", \
												__FILE__,__LINE__);
									#endif
									}
								}//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))

								if (1 == socyfyes)
                                {
                                    pthread_cancel(socyfpid);
                                    pthread_join(socyfpid,NULL);
                                    socyfyes = 0;
                                }
								if (0 == close)
                                {
                                    new_all_close(&acdata);
                                    close = 1;
                                }

								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									sinfo.conmode = 28;
									sinfo.color = 0x04;
									sinfo.time = 0;
									sinfo.stage = 0;
									sinfo.cyclet = 0;
									sinfo.phase = 0;

									sinfo.chans = 0;
            						memset(sinfo.csta,0,sizeof(sinfo.csta));
            						csta = sinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						sinfo.chans += 1;
                						tcsta = i+1;
                						tcsta <<= 2;
                						tcsta &= 0xfc;
                						*csta = tcsta;
                						csta++;
            						}
									memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));					
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&sinfo))
            						{
            						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
							get_soc_status(&color,&leatime);
							soc_phase = 0;
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
											soc_phase = tcbuf[1];
										}
									}
								
									get_soc_status(&color,&leatime);
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
							tcred = 0;
							close = 0;
							fpc = 0;
							cp = 0;
							soc_end_child_thread();//end main thread and its child thread
				//			new_all_red(&ardata);
							*(fcdata->markbit2) |= 0x0008;
							if (0 == phcon)
							{
								if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
        						{
        						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            						printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        						#endif
									*(fcdata->markbit) |= 0x0800;
        						}	
							}
					
							*(fcdata->markbit) |= 0x4000;								
							*(fcdata->contmode) = 29;//network control mode
					//		phcon = 0;
						#if 0	
							if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
							{//report info to server actively
								sinfo.conmode = 29;
								sinfo.color = 0x02;
								sinfo.time = 0;
								sinfo.stage = pinfo.stageline;
								sinfo.cyclet = 0;
								sinfo.phase = 0;
								sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
															
								sinfo.chans = 0;
            					memset(sinfo.csta,0,sizeof(sinfo.csta));
            					csta = sinfo.csta;
            					for (i = 0; i < MAX_CHANNEL; i++)
            					{
                					if (0 == pinfo.chan[i])
                    					break;
                					sinfo.chans += 1;
                					tcsta = pinfo.chan[i];
                					tcsta <<= 2;
                					tcsta &= 0xfc;
                					tcsta |= 0x02; //00000010-green
                					*csta = tcsta;
                					csta++;
            					}
								memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            					memset(sibuf,0,sizeof(sibuf));
            					if (SUCCESS != status_info_report(sibuf,&sinfo))
            					{
            					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                					printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            					#endif
            					}
            					else
            					{
                					write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            					}
							}//report info to server actively
						#endif
							fbdata[1] = 29;
							fbdata[2] = pinfo.stageline;
							fbdata[3] = 0x02;
							fbdata[4] = 0;
							if (!wait_write_serial(*(fcdata->fbserial)))
							{
								if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									*(fcdata->markbit) |= 0x0800;
									gettimeofday(&ct,NULL);
                					update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                					if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																fcdata->softevent,fcdata->markbit))
                					{
                					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                   						printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                					#endif
                					}
								}
							}
							else
							{
							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
								printf("face board serial port cannot write,Line:%d\n",__LINE__);
							#endif
							}
							sendfaceInfoToBoard(fcdata,fbdata);
					//		#ifdef TIMING_DOWN_TIME
							if (*(fcdata->auxfunc) & 0x01)
							{//if (*(fcdata->auxfunc) & 0x01)
								if (!wait_write_serial(*(fcdata->bbserial)))
								{
									if (write(*(fcdata->bbserial),downti,sizeof(downti)) < 0)
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
								}
								else
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
								if (!wait_write_serial(*(fcdata->bbserial)))
								{
									if (write(*(fcdata->bbserial),edownti,sizeof(edownti)) < 0)
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
								}
								else
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
							}//if (*(fcdata->auxfunc) & 0x01)	
						//	#endif							
	
							//send down time to configure tool
                            if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            {
							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
								printf("send control info to configure tool,File: %s,Line: %d\n", \
											__FILE__,__LINE__);
							#endif
                                memset(&tctd,0,sizeof(tctd));
                                tctd.mode = 29;
                                tctd.pattern = *(fcdata->patternid);
                                tctd.lampcolor = 0x02;
                                tctd.lamptime = 0;
                                tctd.phaseid = pinfo.phaseid;
                                tctd.stageline = pinfo.stageline;
                                if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
                                {
                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
                            }
							#ifdef EMBED_CONFIGURE_TOOL
							if (*(fcdata->markbit2) & 0x0200)
							{
								memset(&tctd,0,sizeof(tctd));
                                tctd.mode = 29;
                                tctd.pattern = *(fcdata->patternid);
                                tctd.lampcolor = 0x02;
                                tctd.lamptime = 0;
                                tctd.phaseid = pinfo.phaseid;
                                tctd.stageline = pinfo.stageline;
								if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#endif
                            if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    pinfo.chan,0x02,fcdata->markbit))
                            {
                            #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            #endif
                            }
							
							objecti[0].objectv[0] = 0xf2;
							factovs = 0;
							memset(cbuf,0,sizeof(cbuf));
							if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                            {
                            #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                            	printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }
							if (!(*(fcdata->markbit) & 0x1000))
							{
                            	write(*(fcdata->sockfd->csockfd),cbuf,factovs);
							}
							gettimeofday(&ct,NULL);
                            update_event_list(fcdata->ec,fcdata->el,1,84,ct.tv_sec,fcdata->markbit);

                           	if(0 == soc_phase)
							{
								continue;
							}
							else
							{
								tcbuf[1] = soc_phase;
								soc_get_last_phaseinfo(fcdata,tscdata,socrettl,&pinfo);
							}
						}//net control will happen
						if ((0xf0 == tcbuf[1]) && (1 == netlock))
						{//has been status of net control
							objecti[0].objectv[0] = 0xf2;
                            factovs = 0;
                            memset(cbuf,0,sizeof(cbuf));
                            if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                            {
                            #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
                            #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
							fpc = 0;
						#if 0
							tcred = 0;
							close = 0;
							if (1 == tcyfyes)
							{
								pthread_cancel(tcyfpid);
								pthread_join(tcyfpid,NULL);
								tcyfyes = 0;
							}
						#endif
							*(fcdata->markbit) &= 0xbfff;
							if (0 == cp)
							{//not have phase control
								if ((0 == tcred) && (0 == socyfyes) && (0 == close))
								{//not have all red/yellow flash/close
									if((0==pinfo.cchan[0])&&(pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 29;
											sinfo.color = 0x02;
											sinfo.time = pinfo.gftime + pinfo.ytime + pinfo.rtime;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
											sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
											sinfo.chans = 0;
            								memset(sinfo.csta,0,sizeof(sinfo.csta));
            								csta = sinfo.csta;
            								for (i = 0; i < MAX_CHANNEL; i++)
            								{
												if (0 == pinfo.chan[i])
													break;
                								sinfo.chans += 1;
                								tcsta = pinfo.chan[i];
                								tcsta <<= 2;
                								tcsta &= 0xfc;
												tcsta |= 0x02;
                								*csta = tcsta;
                								csta++;
            								}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//info.cchan[0])&&(pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0))

									if ((0 != pinfo.cchan[0]) && (pinfo.gftime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 29;
											sinfo.color = 0x03;
											sinfo.time = pinfo.gftime;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
											sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
									
											for (i = 0; i < MAX_CHANNEL; i++)
                                			{
                                				if (0 == pinfo.cchan[i])
                                        			break;
                                    			for (j = 0; j < sinfo.chans; j++)
                                    			{
                                        			if (0 == sinfo.csta[j])
                                            			break;
                                        			tcsta = sinfo.csta[j];
                                        			tcsta &= 0xfc;
                                        			tcsta >>= 2;
                                        			tcsta &= 0x3f;
                                        			if (pinfo.cchan[i] == tcsta)
                                        			{
                                            			sinfo.csta[j] &= 0xfc;
                                           				sinfo.csta[j] |= 0x03; //00000001-green flash
														break;
                                        			}
                                    			}
                                			}								
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));	
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != pinfo.cchan[0]) && (pinfo.gftime > 0))

									//current phase begin to green flash
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&tctd,0,sizeof(tctd));
										tctd.mode = 29;//traffic control
										tctd.pattern = *(fcdata->patternid);
										tctd.lampcolor = 0x02;
										tctd.lamptime = pinfo.gftime;
										tctd.phaseid = pinfo.phaseid;
										tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 29;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x02;
                                        tctd.lamptime = pinfo.gftime;
                                        tctd.phaseid = pinfo.phaseid;
                                        tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = pinfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = pinfo.gftime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                                		if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                		{
                                		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                    		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                    		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                    		{
                                    		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
                                    		}
                                		}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									if (pinfo.gftime > 0)
									{	
									ngf = 0;
									while (1)
									{
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x03))
        								{
        								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            								printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															pinfo.cchan,0x03,fcdata->markbit))
        								{
        								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            								printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
        								#endif
        								}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);

										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x02))
        								{
        								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            								printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    									pinfo.cchan,0x02,fcdata->markbit))
                                		{
                                		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    		printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                		#endif
                                		}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);
								
										ngf += 1;
										if (ngf >= pinfo.gftime)
											break;
									}
									}//if (pinfo.gftime > 0)
									if (1 == (pinfo.cpcexist))
									{
										//current phase begin to yellow lamp
										if(SUCCESS!=soc_set_lamp_color(*(fcdata->bbserial),pinfo.cnpchan,0x01))
        								{
        								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            								printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
													pinfo.cnpchan,0x01, fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if(SUCCESS!=soc_set_lamp_color(*(fcdata->bbserial),pinfo.cpchan,0x00))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																	pinfo.cpchan,0x00,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if(SUCCESS!=soc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x01))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														pinfo.cchan,0x01,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}

									if ((0 != pinfo.cchan[0]) && (pinfo.ytime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 29;
											sinfo.color = 0x01;
											sinfo.time = pinfo.ytime;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
											sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
															
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == pinfo.cnpchan[i])
                    								break;
                								for (j = 0; j < sinfo.chans; j++)
                								{
                    								if (0 == sinfo.csta[j])
                        								break;
                    								tcsta = sinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (pinfo.cnpchan[i] == tcsta)
                    								{
                        								sinfo.csta[j] &= 0xfc;
														sinfo.csta[j] |= 0x01; //00000001-yellow
														break;
                    								}
                								}
            								}
											for (i = 0; i < MAX_CHANNEL; i++)
                                			{
                                				if (0 == pinfo.cpchan[i])
                                        			break;
                                    			for (j = 0; j < sinfo.chans; j++)
                                    			{
                                        			if (0 == sinfo.csta[j])
                                            			break;
                                        			tcsta = sinfo.csta[j];
                                        			tcsta &= 0xfc;
                                        			tcsta >>= 2;
                                        			tcsta &= 0x3f;
                                        			if (pinfo.cpchan[i] == tcsta)
                                        			{
                                            			sinfo.csta[j] &= 0xfc;
														break;
                                        			}
                                    			}
                                			}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != pinfo.cchan[0]) && (pinfo.ytime > 0))

									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            		{
                               			memset(&tctd,0,sizeof(tctd));
                               			tctd.mode = 29;//traffic control
                               			tctd.pattern = *(fcdata->patternid);
                               			tctd.lampcolor = 0x01;
                               			tctd.lamptime = pinfo.ytime;
                               			tctd.phaseid = pinfo.phaseid;
                               			tctd.stageline = pinfo.stageline;
                               			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
                               			{
                               			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               			#endif
                               			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 29;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x01;
                                        tctd.lamptime = pinfo.ytime;
                                        tctd.phaseid = pinfo.phaseid;
                                        tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = pinfo.stageline;
                            		fbdata[3] = 0x01;
                            		fbdata[4] = pinfo.ytime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                       			printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x00))
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															pinfo.cchan,0x00,fcdata->markbit))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}

									if ((0 != pinfo.cchan[0]) && (pinfo.rtime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 29;
											sinfo.color = 0x00;
											sinfo.time = pinfo.rtime;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
											sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
															
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == pinfo.cchan[i])
                    								break;
                								for (j = 0; j < sinfo.chans; j++)
                								{
                    								if (0 == sinfo.csta[j])
                        								break;
                    								tcsta = sinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (pinfo.cchan[i] == tcsta)
                    								{
                        								sinfo.csta[j] &= 0xfc;
														break;
                    								}
                								}
            								}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));	
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != pinfo.cchan[0]) && (pinfo.rtime > 0))

									if (pinfo.rtime > 0)
									{
										if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            			{
                                			memset(&tctd,0,sizeof(tctd));
                                			tctd.mode = 29;//traffic control
                                			tctd.pattern = *(fcdata->patternid);
                                			tctd.lampcolor = 0x00;
                                			tctd.lamptime = pinfo.rtime;
                                			tctd.phaseid = pinfo.phaseid;
                                			tctd.stageline = pinfo.stageline;
                                			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
                                			{
                                			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                            			}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&tctd,0,sizeof(tctd));
                                            tctd.mode = 29;//traffic control
                                            tctd.pattern = *(fcdata->patternid);
                                            tctd.lampcolor = 0x00;
                                            tctd.lamptime = pinfo.rtime;
                                            tctd.phaseid = pinfo.phaseid;
                                            tctd.stageline = pinfo.stageline;
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#endif
										fbdata[1] = 29;
                                		fbdata[2] = pinfo.stageline;
                                		fbdata[3] = 0x00;
                                		fbdata[4] = pinfo.rtime;
                                		if (!wait_write_serial(*(fcdata->fbserial)))
                                		{
                                    		if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                    		{
                                    		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
                                        		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                        		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                        		{
                                        		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                            		printf("genfile err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                        		#endif
                                        		}
                                    		}
                                		}
                                		else
                                		{
                                		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                		#endif
                                		}
										sleep(pinfo.rtime);
									}

									*(fcdata->slnum) += 1;
									*(fcdata->stageid) = \
									tscdata->timeconfig->TimeConfigList[socrettl][*(fcdata->slnum)].StageId;
							if(0==(tscdata->timeconfig->TimeConfigList[socrettl][*(fcdata->slnum)].StageId))
									{
										*(fcdata->slnum) = 0;
										*(fcdata->stageid) = \
										tscdata->timeconfig->TimeConfigList[socrettl][*(fcdata->slnum)].StageId;
									}
								}//not have all red/yellow flash/close
								else
								{//have all red or yellow flash or all close
									tcred = 0;
                            		close = 0;
                            		if (1 == socyfyes)
                            		{
                                		pthread_cancel(socyfpid);
                                		pthread_join(socyfpid,NULL);
                                		socyfyes = 0;
                            		}
								}//have all red or yellow flash or all close
							}//not have phase control
							else if (0 != cp)
							{//phase control happen
								if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
								{//if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
									rpc = 0;
									rpi = tscdata->timeconfig->TimeConfigList[socrettl][0].PhaseId;
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
										{//for (i = 0; i < MAX_CHANNEL_LINE; i++)
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
												if ((0x0d <= lkch[cp-0x5a][i]) && (lkch[cp-0x5a][i]<= 0x10))
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
											}
										}//for (i = 0; i < MAX_CHANNEL_LINE; i++)								
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
	
									if ((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
									{
										if((!(*(fcdata->markbit)&0x1000))&&(!(*(fcdata->markbit)&0x8000)))
										{
											if ((0x5a <= cp) && (cp <= 0x5d))
                                                sinfo.conmode = cp;
                                            else
                                                sinfo.conmode = 29;
											sinfo.color = 0x02;
											sinfo.time = pinfo.gftime + pinfo.ytime + pinfo.rtime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
											sinfo.chans = 0;
											memset(sinfo.csta,0,sizeof(sinfo.csta));
											csta = sinfo.csta;
											for (i = 0; i < MAX_CHANNEL; i++)
											{
												if (0 == lkch[cp-0x5a][i])
													break;
												sinfo.chans += 1;
												tcsta = lkch[cp-0x5a][i];
												tcsta <<= 2;
												tcsta &= 0xfc;
												tcsta |= 0x02; //00000010-green 
												*csta = tcsta;
												csta++;
											}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
											memset(sibuf,0,sizeof(sibuf));
											if (SUCCESS != status_info_report(sibuf,&sinfo))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											else
											{
												write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
											}
										}
									}//if ((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
									if ((0 != wcc[0]) && (pinfo.gftime > 0))	
									{			
										if((!(*(fcdata->markbit)&0x1000))&&(!(*(fcdata->markbit)&0x8000)))
										{//report info to server actively
											if ((0x5a <= cp) && (cp <= 0x5d))
                                                sinfo.conmode = cp;
                                            else
                                                sinfo.conmode = 29;
											sinfo.color = 0x03;
											sinfo.time = pinfo.gftime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
															
											for (i = 0; i < MAX_CHANNEL; i++)
											{
												if (0 == wcc[i])
													break;
												for (j = 0; j < sinfo.chans; j++)
												{
													if (0 == sinfo.csta[j])
														break;
													tcsta = sinfo.csta[j];
													tcsta &= 0xfc;
													tcsta >>= 2;
													tcsta &= 0x3f;
													if (wcc[i] == tcsta)
													{
														sinfo.csta[j] &= 0xfc;
														sinfo.csta[j] |= 0x03; //00000011-green flash
														break;
													}
												}
											}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
											memset(sibuf,0,sizeof(sibuf));
											if (SUCCESS != status_info_report(sibuf,&sinfo))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("status err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											else
											{
												write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
											}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (pinfo.gftime > 0))
	
									//green flash
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&tctd,0,sizeof(tctd));
										tctd.mode = 29;//traffic control
										tctd.pattern = *(fcdata->patternid);
										tctd.lampcolor = 0x02;
										tctd.lamptime = pinfo.gftime;
										tctd.phaseid = 0;
										tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 29;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x02;
                                        tctd.lamptime = pinfo.gftime;
                                        tctd.phaseid = 0;
                                        tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
									fbdata[2] = pinfo.stageline;
									fbdata[3] = 0x02;
									fbdata[4] = pinfo.gftime;
									if (!wait_write_serial(*(fcdata->fbserial)))
									{
										if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
											update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
											if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
									}
									else
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("face board serial port cannot write,Line:%d\n",__LINE__);
									#endif
									}
									if (pinfo.gftime > 0)
									{
										ngf = 0;
										while (1)
										{
											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x03,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);
									
											ngf += 1;
											if (ngf >= pinfo.gftime)
												break;
										}
									}//if (pinfo.gftime > 0)
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}

									if ((0 != wcc[0]) && (pinfo.ytime > 0))
									{
										if((!(*(fcdata->markbit)&0x1000))&&(!(*(fcdata->markbit)&0x8000)))
										{//report info to server actively
											if ((0x5a <= cp) && (cp <= 0x5d))
                                                sinfo.conmode = cp;
                                            else
                                                sinfo.conmode = 29;
											sinfo.color = 0x01;
											sinfo.time = pinfo.ytime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
															
											for (i = 0; i < MAX_CHANNEL; i++)
											{
												if (0 == wnpcc[i])
													break;
												for (j = 0; j < sinfo.chans; j++)
												{
													if (0 == sinfo.csta[j])
														break;
													tcsta = sinfo.csta[j];
													tcsta &= 0xfc;
													tcsta >>= 2;
													tcsta &= 0x3f;
													if (wnpcc[i] == tcsta)
													{
														sinfo.csta[j] &= 0xfc;
														sinfo.csta[j] |= 0x01; //00000001-yellow
														break;
													}
												}
											}
											for (i = 0; i < MAX_CHANNEL; i++)
											{
												if (0 == wpcc[i])
													break;
												for (j = 0; j < sinfo.chans; j++)
												{
													if (0 == sinfo.csta[j])
														break;
													tcsta = sinfo.csta[j];
													tcsta &= 0xfc;
													tcsta >>= 2;
													tcsta &= 0x3f;
													if (wpcc[i] == tcsta)
													{
														sinfo.csta[j] &= 0xfc;
														break;
													}
												}
											}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
											memset(sibuf,0,sizeof(sibuf));
											if (SUCCESS != status_info_report(sibuf,&sinfo))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("status err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											else
											{
												write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
											}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (pinfo.ytime > 0))

									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&tctd,0,sizeof(tctd));
										tctd.mode = 29;//traffic control
										tctd.pattern = *(fcdata->patternid);
										tctd.lampcolor = 0x01;
										tctd.lamptime = pinfo.ytime;
										tctd.phaseid = 0;
										tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 29;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x01;
                                        tctd.lamptime = pinfo.ytime;
                                        tctd.phaseid = 0;
                                        tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
									fbdata[2] = pinfo.stageline;
									fbdata[3] = 0x01;
									fbdata[4] = pinfo.ytime;
									if (!wait_write_serial(*(fcdata->fbserial)))
									{
										if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
											update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
											if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
									}
									else
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("face board serial port cannot write,Line:%d\n",__LINE__);
									#endif
									}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}

									if ((0 != wcc[0]) && (pinfo.rtime > 0))
									{
										if((!(*(fcdata->markbit)&0x1000))&&(!(*(fcdata->markbit)&0x8000)))
										{//report info to server actively
											if ((0x5a <= cp) && (cp <= 0x5d))
                                                sinfo.conmode = cp;
                                            else
                                                sinfo.conmode = 29;
											sinfo.color = 0x00;
											sinfo.time = pinfo.rtime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
															
											for (i = 0; i < MAX_CHANNEL; i++)
											{
												if (0 == wcc[i])
													break;
												for (j = 0; j < sinfo.chans; j++)
												{
													if (0 == sinfo.csta[j])
														break;
													tcsta = sinfo.csta[j];
													tcsta &= 0xfc;
													tcsta >>= 2;
													tcsta &= 0x3f;
													if (wcc[i] == tcsta)
													{
														sinfo.csta[j] &= 0xfc;
														break;
													}
												}
											}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
											memset(sibuf,0,sizeof(sibuf));
											if (SUCCESS != status_info_report(sibuf,&sinfo))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											else
											{
												write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
											}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (pinfo.rtime > 0))
									*(fcdata->slnum) = 0;
									*(fcdata->stageid) = \
									tscdata->timeconfig->TimeConfigList[socrettl][0].StageId;
								}//if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp)) 
								else if ((0x01 <= cp) && (cp <= 0x20))
								{//else if ((0x01 <= cp) && (cp <= 0x20))
									unsigned char			tsl = 0;
									unsigned char			pexist = 0;
									for (i = 0; i < (tscdata->timeconfig->FactStageNum); i++)
									{
										if (0 == (tscdata->timeconfig->TimeConfigList[socrettl][i].StageId))
											break;
										rpi = tscdata->timeconfig->TimeConfigList[socrettl][i].PhaseId;
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
										if (0 == tscdata->timeconfig->TimeConfigList[socrettl][i+1].StageId)
										{
											*(fcdata->slnum) = 0;
											*(fcdata->stageid) = \
												tscdata->timeconfig->TimeConfigList[socrettl][0].StageId;
											rpc = 0;
											rpi = tscdata->timeconfig->TimeConfigList[socrettl][0].PhaseId;
											get_phase_id(rpi,&rpc);
										}
										else
										{
											*(fcdata->slnum) = i + 1;
											*(fcdata->stageid) = \
												tscdata->timeconfig->TimeConfigList[socrettl][i+1].StageId;
											rpc = 0;
											rpi = tscdata->timeconfig->TimeConfigList[socrettl][i+1].PhaseId;
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
									
										if ((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
										{
											if((!(*(fcdata->markbit)&0x1000))&&(!(*(fcdata->markbit)&0x8000)))
											{
												sinfo.conmode = 29;
												sinfo.color = 0x02;
												sinfo.time = pinfo.gftime + pinfo.ytime + pinfo.rtime;
												sinfo.stage = 0;
												sinfo.cyclet = 0;
												sinfo.phase = 0;
												sinfo.phase |= (0x01 << (cp - 1));
												sinfo.chans = 0;
												memset(sinfo.csta,0,sizeof(sinfo.csta));
												csta = sinfo.csta;
												for (i = 0; i < MAX_CHANNEL; i++)
												{
													if (0 == cc[i])
														break;
													sinfo.chans += 1;
													tcsta = cc[i];
													tcsta <<= 2;
													tcsta &= 0xfc;
													tcsta |= 0x02; //00000010-green 
													*csta = tcsta;
													csta++;
												}
												memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
												memset(sibuf,0,sizeof(sibuf));
												if (SUCCESS != status_info_report(sibuf,&sinfo))
												{
												#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
													printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
												else
												{
													write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
												}
											}
										}//if((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
										if ((0 != wcc[0]) && (pinfo.gftime > 0))	
										{			
											if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit)&0x8000)))
											{//report info to server actively
												sinfo.conmode = 29;
												sinfo.color = 0x03;
												sinfo.time = pinfo.gftime;
												sinfo.stage = 0;
												sinfo.cyclet = 0;
												sinfo.phase = 0;
												sinfo.phase |= (0x01 << (cp - 1));
																
												for (i = 0; i < MAX_CHANNEL; i++)
												{
													if (0 == wcc[i])
														break;
													for (j = 0; j < sinfo.chans; j++)
													{
														if (0 == sinfo.csta[j])
															break;
														tcsta = sinfo.csta[j];
														tcsta &= 0xfc;
														tcsta >>= 2;
														tcsta &= 0x3f;
														if (wcc[i] == tcsta)
														{
															sinfo.csta[j] &= 0xfc;
															sinfo.csta[j] |= 0x03; //00000011-green flash
															break;
														}
													}
												}
												memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
												memset(sibuf,0,sizeof(sibuf));
												if (SUCCESS != status_info_report(sibuf,&sinfo))
												{
												#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
													printf("status err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
												else
												{
													write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
												}
											}//report info to server actively
										}//if ((0 != wcc[0]) && (pinfo.gftime > 0))
		
										//green flash
										if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
										{
											memset(&tctd,0,sizeof(tctd));
											tctd.mode = 29;//traffic control
											tctd.pattern = *(fcdata->patternid);
											tctd.lampcolor = 0x02;
											tctd.lamptime = pinfo.gftime;
											tctd.phaseid = cp;
											tctd.stageline = pinfo.stageline;
											if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&tctd,0,sizeof(tctd));
                                            tctd.mode = 29;//traffic control
                                            tctd.pattern = *(fcdata->patternid);
                                            tctd.lampcolor = 0x02;
                                            tctd.lamptime = pinfo.gftime;
                                            tctd.phaseid = cp;
                                            tctd.stageline = pinfo.stageline;
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#endif
										fbdata[1] = 29;
										fbdata[2] = pinfo.stageline;
										fbdata[3] = 0x02;
										fbdata[4] = pinfo.gftime;
										if (!wait_write_serial(*(fcdata->fbserial)))
										{
											if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
												update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
												if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
												{
												#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
													printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
											}
										}
										else
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("face board serial port cannot write,Line:%d\n",__LINE__);
										#endif
										}
										if (pinfo.gftime > 0)
										{
										ngf = 0;
										while (1)
										{
											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x03,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);
									
											ngf += 1;
											if (ngf >= pinfo.gftime)
												break;
										}
										}//if (pinfo.gftime > 0)
										if (1 == pce)
										{
											//current phase begin to yellow lamp
											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wnpcc,0x01, fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wpcc,0x00,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										else
										{
											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x01,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}

										if ((0 != wcc[0]) && (pinfo.ytime > 0))
										{
											if((!(*(fcdata->markbit)&0x1000))&&(!(*(fcdata->markbit)&0x8000)))
											{//report info to server actively
												sinfo.conmode = 29;
												sinfo.color = 0x01;
												sinfo.time = pinfo.ytime;
												sinfo.stage = 0;
												sinfo.cyclet = 0;
												sinfo.phase = 0;
												sinfo.phase |= (0x01 << (cp - 1));
																
												for (i = 0; i < MAX_CHANNEL; i++)
												{
													if (0 == wnpcc[i])
														break;
													for (j = 0; j < sinfo.chans; j++)
													{
														if (0 == sinfo.csta[j])
															break;
														tcsta = sinfo.csta[j];
														tcsta &= 0xfc;
														tcsta >>= 2;
														tcsta &= 0x3f;
														if (wnpcc[i] == tcsta)
														{
															sinfo.csta[j] &= 0xfc;
															sinfo.csta[j] |= 0x01; //00000001-yellow
															break;
														}
													}
												}
												for (i = 0; i < MAX_CHANNEL; i++)
												{
													if (0 == wpcc[i])
														break;
													for (j = 0; j < sinfo.chans; j++)
													{
														if (0 == sinfo.csta[j])
															break;
														tcsta = sinfo.csta[j];
														tcsta &= 0xfc;
														tcsta >>= 2;
														tcsta &= 0x3f;
														if (wpcc[i] == tcsta)
														{
															sinfo.csta[j] &= 0xfc;
															break;
														}
													}
												}
												memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
												memset(sibuf,0,sizeof(sibuf));
												if (SUCCESS != status_info_report(sibuf,&sinfo))
												{
												#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
													printf("status err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
												else
												{
													write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
												}
											}//report info to server actively
										}//if ((0 != wcc[0]) && (pinfo.ytime > 0))

										if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
										{
											memset(&tctd,0,sizeof(tctd));
											tctd.mode = 29;//traffic control
											tctd.pattern = *(fcdata->patternid);
											tctd.lampcolor = 0x01;
											tctd.lamptime = pinfo.ytime;
											tctd.phaseid = cp;
											tctd.stageline = pinfo.stageline;
											if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&tctd,0,sizeof(tctd));
                                            tctd.mode = 29;//traffic control
                                            tctd.pattern = *(fcdata->patternid);
                                            tctd.lampcolor = 0x01;
                                            tctd.lamptime = pinfo.ytime;
                                            tctd.phaseid = cp;
                                            tctd.stageline = pinfo.stageline;
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#endif
										fbdata[1] = 29;
										fbdata[2] = pinfo.stageline;
										fbdata[3] = 0x01;
										fbdata[4] = pinfo.ytime;
										if (!wait_write_serial(*(fcdata->fbserial)))
										{
											if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
												update_event_list(fcdata->ec,fcdata->el,1,16, \
																		ct.tv_sec,fcdata->markbit);
												if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
												{
												#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
													printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
											}
										}
										else
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("face board serial port cannot write,Line:%d\n",__LINE__);
										#endif
										}
										sleep(pinfo.ytime);

										//current phase begin to red lamp
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x00,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}

										if ((0 != wcc[0]) && (pinfo.rtime > 0))
										{
											if((!(*(fcdata->markbit)&0x1000))&&(!(*(fcdata->markbit)&0x8000)))
											{//report info to server actively
												sinfo.conmode = 29;
												sinfo.color = 0x00;
												sinfo.time = pinfo.rtime;
												sinfo.stage = 0;
												sinfo.cyclet = 0;
												sinfo.phase = 0;
												sinfo.phase |= (0x01 << (cp - 1));
																
												for (i = 0; i < MAX_CHANNEL; i++)
												{
													if (0 == wcc[i])
														break;
													for (j = 0; j < sinfo.chans; j++)
													{
														if (0 == sinfo.csta[j])
															break;
														tcsta = sinfo.csta[j];
														tcsta &= 0xfc;
														tcsta >>= 2;
														tcsta &= 0x3f;
														if (wcc[i] == tcsta)
														{
															sinfo.csta[j] &= 0xfc;
															break;
														}
													}
												}
												memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
												memset(sibuf,0,sizeof(sibuf));
												if (SUCCESS != status_info_report(sibuf,&sinfo))
												{
												#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
													printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
												else
												{
													write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
												}
											}//report info to server actively
										}//if ((0 != wcc[0]) && (pinfo.rtime > 0))
									}//control phase belong to phase list;
									else
									{//control phase don't belong to phase list;
										rpc = 0;
										rpi = tscdata->timeconfig->TimeConfigList[socrettl][0].PhaseId;
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
									
										if ((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
										{
											if((!(*(fcdata->markbit)&0x1000))&&(!(*(fcdata->markbit)&0x8000)))
											{
												sinfo.conmode = 29;
												sinfo.color = 0x02;
												sinfo.time = pinfo.gftime + pinfo.ytime + pinfo.rtime;
												sinfo.stage = 0;
												sinfo.cyclet = 0;
												sinfo.phase = 0;
												sinfo.phase |= (0x01 << (cp - 1));
												sinfo.chans = 0;
												memset(sinfo.csta,0,sizeof(sinfo.csta));
												csta = sinfo.csta;
												for (i = 0; i < MAX_CHANNEL; i++)
												{
													if (0 == cc[i])
														break;
													sinfo.chans += 1;
													tcsta = cc[i];
													tcsta <<= 2;
													tcsta &= 0xfc;
													tcsta |= 0x02; //00000010-green 
													*csta = tcsta;
													csta++;
												}
												memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
												memset(sibuf,0,sizeof(sibuf));
												if (SUCCESS != status_info_report(sibuf,&sinfo))
												{
												#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
													printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
												else
												{
													write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
												}
											}
										}//if((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
										if ((0 != wcc[0]) && (pinfo.gftime > 0))	
										{			
											if((!(*(fcdata->markbit)&0x1000))&&(!(*(fcdata->markbit)&0x8000)))
											{//report info to server actively
												sinfo.conmode = 29;
												sinfo.color = 0x03;
												sinfo.time = pinfo.gftime;
												sinfo.stage = 0;
												sinfo.cyclet = 0;
												sinfo.phase = 0;
												sinfo.phase |= (0x01 << (cp - 1));
																
												for (i = 0; i < MAX_CHANNEL; i++)
												{
													if (0 == wcc[i])
														break;
													for (j = 0; j < sinfo.chans; j++)
													{
														if (0 == sinfo.csta[j])
															break;
														tcsta = sinfo.csta[j];
														tcsta &= 0xfc;
														tcsta >>= 2;
														tcsta &= 0x3f;
														if (wcc[i] == tcsta)
														{
															sinfo.csta[j] &= 0xfc;
															sinfo.csta[j] |= 0x03; //00000011-green flash
															break;
														}
													}
												}
												memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
												memset(sibuf,0,sizeof(sibuf));
												if (SUCCESS != status_info_report(sibuf,&sinfo))
												{
												#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
													printf("status err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
												else
												{
													write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
												}
											}//report info to server actively
										}//if ((0 != wcc[0]) && (pinfo.gftime > 0))
		
										//green flash
										if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
										{
											memset(&tctd,0,sizeof(tctd));
											tctd.mode = 29;//traffic control
											tctd.pattern = *(fcdata->patternid);
											tctd.lampcolor = 0x02;
											tctd.lamptime = pinfo.gftime;
											tctd.phaseid = cp;
											tctd.stageline = pinfo.stageline;
											if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&tctd,0,sizeof(tctd));
                                            tctd.mode = 29;//traffic control
                                            tctd.pattern = *(fcdata->patternid);
                                            tctd.lampcolor = 0x02;
                                            tctd.lamptime = pinfo.gftime;
                                            tctd.phaseid = cp;
                                            tctd.stageline = pinfo.stageline;
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#endif
										fbdata[1] = 29;
										fbdata[2] = pinfo.stageline;
										fbdata[3] = 0x02;
										fbdata[4] = pinfo.gftime;
										if (!wait_write_serial(*(fcdata->fbserial)))
										{
											if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
												update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
												if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
												{
												#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
													printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
											}
										}
										else
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("face board serial port cannot write,Line:%d\n",__LINE__);
										#endif
										}
										if (pinfo.gftime > 0)
										{
										ngf = 0;
										while (1)
										{
											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x03,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);
									
											ngf += 1;
											if (ngf >= pinfo.gftime)
												break;
										}
										}//if (pinfo.gftime > 0)
										if (1 == pce)
										{
											//current phase begin to yellow lamp
											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wnpcc,0x01, fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wpcc,0x00,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										else
										{
											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x01,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}

										if ((0 != wcc[0]) && (pinfo.ytime > 0))
										{
											if((!(*(fcdata->markbit)&0x1000))&&(!(*(fcdata->markbit)&0x8000)))
											{//report info to server actively
												sinfo.conmode = 29;
												sinfo.color = 0x01;
												sinfo.time = pinfo.ytime;
												sinfo.stage = 0;
												sinfo.cyclet = 0;
												sinfo.phase = 0;
												sinfo.phase |= (0x01 << (cp - 1));
																
												for (i = 0; i < MAX_CHANNEL; i++)
												{
													if (0 == wnpcc[i])
														break;
													for (j = 0; j < sinfo.chans; j++)
													{
														if (0 == sinfo.csta[j])
															break;
														tcsta = sinfo.csta[j];
														tcsta &= 0xfc;
														tcsta >>= 2;
														tcsta &= 0x3f;
														if (wnpcc[i] == tcsta)
														{
															sinfo.csta[j] &= 0xfc;
															sinfo.csta[j] |= 0x01; //00000001-yellow
															break;
														}
													}
												}
												for (i = 0; i < MAX_CHANNEL; i++)
												{
													if (0 == wpcc[i])
														break;
													for (j = 0; j < sinfo.chans; j++)
													{
														if (0 == sinfo.csta[j])
															break;
														tcsta = sinfo.csta[j];
														tcsta &= 0xfc;
														tcsta >>= 2;
														tcsta &= 0x3f;
														if (wpcc[i] == tcsta)
														{
															sinfo.csta[j] &= 0xfc;
															break;
														}
													}
												}
												memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
												memset(sibuf,0,sizeof(sibuf));
												if (SUCCESS != status_info_report(sibuf,&sinfo))
												{
												#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
													printf("status err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
												else
												{
													write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
												}
											}//report info to server actively
										}//if ((0 != wcc[0]) && (pinfo.ytime > 0))

										if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
										{
											memset(&tctd,0,sizeof(tctd));
											tctd.mode = 29;//traffic control
											tctd.pattern = *(fcdata->patternid);
											tctd.lampcolor = 0x01;
											tctd.lamptime = pinfo.ytime;
											tctd.phaseid = cp;
											tctd.stageline = pinfo.stageline;
											if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&tctd,0,sizeof(tctd));
                                            tctd.mode = 29;//traffic control
                                            tctd.pattern = *(fcdata->patternid);
                                            tctd.lampcolor = 0x01;
                                            tctd.lamptime = pinfo.ytime;
                                            tctd.phaseid = cp;
                                            tctd.stageline = pinfo.stageline;
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#endif
										fbdata[1] = 29;
										fbdata[2] = pinfo.stageline;
										fbdata[3] = 0x01;
										fbdata[4] = pinfo.ytime;
										if (!wait_write_serial(*(fcdata->fbserial)))
										{
											if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
												update_event_list(fcdata->ec,fcdata->el,1,16, \
																		ct.tv_sec,fcdata->markbit);
												if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
												{
												#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
													printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
											}
										}
										else
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("face board serial port cannot write,Line:%d\n",__LINE__);
										#endif
										}
										sleep(pinfo.ytime);

										//current phase begin to red lamp
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x00,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}

										if ((0 != wcc[0]) && (pinfo.rtime > 0))
										{
											if((!(*(fcdata->markbit)&0x1000))&&(!(*(fcdata->markbit)&0x8000)))
											{//report info to server actively
												sinfo.conmode = 29;
												sinfo.color = 0x00;
												sinfo.time = pinfo.rtime;
												sinfo.stage = 0;
												sinfo.cyclet = 0;
												sinfo.phase = 0;
												sinfo.phase |= (0x01 << (cp - 1));
																
												for (i = 0; i < MAX_CHANNEL; i++)
												{
													if (0 == wcc[i])
														break;
													for (j = 0; j < sinfo.chans; j++)
													{
														if (0 == sinfo.csta[j])
															break;
														tcsta = sinfo.csta[j];
														tcsta &= 0xfc;
														tcsta >>= 2;
														tcsta &= 0x3f;
														if (wcc[i] == tcsta)
														{
															sinfo.csta[j] &= 0xfc;
															break;
														}
													}
												}
												memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
												memset(sibuf,0,sizeof(sibuf));
												if (SUCCESS != status_info_report(sibuf,&sinfo))
												{
												#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
													printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
												else
												{
													write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
												}
											}//report info to server actively
										}//if ((0 != wcc[0]) && (pinfo.rtime > 0))
										*(fcdata->slnum) = 0;
										*(fcdata->stageid) = \
										tscdata->timeconfig->TimeConfigList[socrettl][0].StageId;
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
							if (0 == socyes)
							{
								memset(&tcdata,0,sizeof(tcdata));
								memset(&pinfo,0,sizeof(pinfo));
								tcdata.fd = fcdata;
								tcdata.td = tscdata;
								tcdata.cs = chanstatus;
								tcdata.pi = &pinfo;
								int tcret = pthread_create(&socpid,NULL, \
									(void *)start_optimize_coordinate_control,&tcdata);
								if (0 != tcret)
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("create thread err,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
									soc_end_part_child_thread();
									return FAIL;
								}
								socyes = 1;
							}
							objecti[0].objectv[0] = 0xf3;
							factovs = 0;
							memset(cbuf,0,sizeof(cbuf));
							if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                            {
                            #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
								if ((1 == tcred) || (1 == socyfyes) || (1 == close) || (1 == phcon))
                            	{//if ((1 == tcred) || (1 == tcyfyes) || (1 == close))
									fpc = 1;
									phcon = 0;
									cp = tcbuf[1];
                                	if (1 == socyfyes)
                                	{
                                    	pthread_cancel(socyfpid);
                                    	pthread_join(socyfpid,NULL);
                                    	socyfyes = 0;
                                	}
									if (1 != tcred)
                                    {
                                        new_all_red(&ardata);
                                    }
                                    tcred = 0;
                                    close = 0;
							//		#ifdef CLOSE_LAMP
                                    soc_set_lamp_color(*(fcdata->bbserial),clch,0x03);
									update_channel_status(fcdata->sockfd,chanstatus,clch,0x03,fcdata->markbit);
                            //        #endif
									if (0xC8 == tcbuf[1])
									{// if (0xC8 == tcbuf[1])
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
                                        if(SUCCESS!=soc_set_lamp_color(*(fcdata->bbserial),ccon,0x02))
                                        {
                                        #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                            printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                            *(fcdata->markbit) |= 0x0800;
                                        }
                                        if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                                                            ccon,0x02,fcdata->markbit))
                                        {
                                        #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                            printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                        #endif
                                        }
                                        if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
                                        {//report info to server actively
                                            sinfo.conmode = 29;
                                            sinfo.color = 0x02;
                                            sinfo.time = 0;
                                            sinfo.stage = 0;
                                            sinfo.cyclet = 0;
                                            sinfo.phase = 0;
                                            sinfo.chans = 0;
                                            memset(sinfo.csta,0,sizeof(sinfo.csta));
                                            csta = sinfo.csta;
                                            for (i = 0; i < MAX_CHANNEL; i++)
											{
                                                if (0 == ccon[i])
                                                    break;
                                                sinfo.chans += 1;
                                                tcsta = ccon[i];
                                                tcsta <<= 2;
                                                tcsta &= 0xfc;
                                                tcsta |= 0x02; //00000010-green 
                                                *csta = tcsta;
                                                csta++;
                                            }

                                            memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
                                            memset(sibuf,0,sizeof(sibuf));
                                            if (SUCCESS != status_info_report(sibuf,&sinfo))
                                            {
                                            #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
									}// if (0xC8 == tcbuf[1])
									if ((0x5a <= tcbuf[1]) && (tcbuf[1] <= 0x5d))
                                    {//if ((0x5a <= tcbuf[1]) && (tcbuf[1] <= 0x5d))	
										if(SUCCESS!=soc_set_lamp_color(*(fcdata->bbserial),lkch[cp-0x5a],0x02))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																		lkch[cp-0x5a],0x02,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
							
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = cp;
											sinfo.color = 0x02;
											sinfo.time = 0;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
									/*
											sinfo.chans = 0;
											memset(sinfo.csta,0,sizeof(sinfo.csta));
											csta = sinfo.csta;
											for (i = 0; i < MAX_CHANNEL; i++)
											{
												if (0 == lkch[cp-0x5a][i])
													break;
												sinfo.chans += 1;
												tcsta = lkch[cp-0x5a][i];
												tcsta <<= 2;
												tcsta &= 0xfc;
												tcsta |= 0x02; //00000010-green 
												*csta = tcsta;
												csta++;
											}
									*/
											sinfo.chans = 0;
											memset(sinfo.csta,0,sizeof(sinfo.csta));
											csta = sinfo.csta;
											for (i = 0; i < MAX_CHANNEL; i++)
											{
												if (0 == lkch[cp-0x5a][i])
													break;
												sinfo.chans += 1;
												tcsta = lkch[cp-0x5a][i];
												tcsta <<= 2;
												tcsta &= 0xfc;
												tcsta |= 0x02; //00000010-green 
												*csta = tcsta;
												csta++;
											}

											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
											memset(sibuf,0,sizeof(sibuf));
											if (SUCCESS != status_info_report(sibuf,&sinfo))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
										update_event_list(fcdata->ec,fcdata->el,1,89,ct.tv_sec,fcdata->markbit);									}//if ((0x5a <= tcbuf[1]) && (tcbuf[1] <= 0x5d))
                                	continue;
                            	}//if ((1 == tcred) || (1 == tcyfyes) || (1 == close))
								if (0 == fpc)
								{//phase control of first times
									fpc = 1;
									cp = pinfo.phaseid;
								}//phase control of first times

								if ((cp == tcbuf[1]) && (0xC8 != cp))
								{
									if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
									{
										if ((0x5a <= cp) && (cp <= 0x5d))
                                            sinfo.conmode = cp;
                                        else
                                            sinfo.conmode = 29;
										sinfo.color = 0x02;
										sinfo.time = 0;
										sinfo.stage = 0;
										sinfo.cyclet = 0;
										sinfo.phase = 0;
								
										sinfo.chans = 0;
            							memset(sinfo.csta,0,sizeof(sinfo.csta));
            							csta = sinfo.csta;
            							for (i = 0; i < MAX_CHANNEL; i++)
            							{
                							if (0 == lkch[cp-0x5a][i])
                    							break;
                							sinfo.chans += 1;
                							tcsta = lkch[cp-0x5a][i];
                							tcsta <<= 2;
                							tcsta &= 0xfc;
                							tcsta |= 0x02; //00000010-green 
                							*csta = tcsta;
                							csta++;
            							}
										memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));	
										memset(sibuf,0,sizeof(sibuf));
            							if (SUCCESS != status_info_report(sibuf,&sinfo))
            							{
            							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
										}//for (i = 0; i < MAX_CHANNEL_LINE; i++)
									}//else if ((0x5a <= cp) && (cp <= 0x5d))
									if (0xC8 == tcbuf[1])
                                        *(fcdata->trans) |= 0x01;

									if ((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{
											if ((0x01 <= cp) && (cp <= 0x20))
												sinfo.conmode = /*29*/ncmode;
											else if ((0x5a <= cp) && (cp <= 0x5d))
												sinfo.conmode = cp;
											else if (0xC8 == cp)
												sinfo.conmode = 29;
											sinfo.color = 0x02;
											sinfo.time = pinfo.gftime + pinfo.ytime + pinfo.rtime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
											{
												sinfo.phase = 0;
												sinfo.phase |= (0x01 << (cp - 1));
											}
											else if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
											{
												sinfo.phase = 0;
											}
											sinfo.chans = 0;
            								memset(sinfo.csta,0,sizeof(sinfo.csta));
            								csta = sinfo.csta;
            								for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == cc[i])
                    								break;
                								sinfo.chans += 1;
                								tcsta = cc[i];
                								tcsta <<= 2;
                								tcsta &= 0xfc;
                								tcsta |= 0x02; //00000010-green 
                								*csta = tcsta;
                								csta++;
            								}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));	
											memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}
									}//if((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
						
									if ((0 != wcc[0]) && (pinfo.gftime > 0))
									{	
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
												sinfo.conmode = /*29*/ncmode;
											else if ((0x5a <= cp) && (cp <= 0x5d))
												sinfo.conmode = cp;
											else if (0xC8 == cp)
                                                sinfo.conmode = 29;
											sinfo.color = 0x03;
											sinfo.time = pinfo.gftime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
											{
												sinfo.phase = 0;
												sinfo.phase |= (0x01 << (cp - 1));
											}
											else if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
											{
												sinfo.phase = 0;
											}
									
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == wcc[i])
                                                	break;
                                            	for (j = 0; j < sinfo.chans; j++)
                                            	{
                                                	if (0 == sinfo.csta[j])
                                                    	break;
                                                	tcsta = sinfo.csta[j];
                                                	tcsta &= 0xfc;
                                                	tcsta >>= 2;
                                                	tcsta &= 0x3f;
                                                	if (wcc[i] == tcsta)
                                                	{
                                                    	sinfo.csta[j] &= 0xfc;
                                                    	sinfo.csta[j] |= 0x03; //00000001-green flash
														break;
                                                	}
                                            	}
                                        	}								
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));	
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively										
									}//if ((0 != wcc[0]) && (pinfo.gftime > 0))

									//green flash
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&tctd,0,sizeof(tctd));
										tctd.mode = 29;//traffic control
										tctd.pattern = *(fcdata->patternid);
										tctd.lampcolor = 0x02;
										tctd.lamptime = pinfo.gftime;
										tctd.phaseid = 0;
										tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 29;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x02;
                                        tctd.lamptime = pinfo.gftime;
                                        tctd.phaseid = 0;
                                        tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = pinfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = pinfo.gftime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									if (pinfo.gftime > 0)
									{
										ngf = 0;
										while (1)
										{
											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x03,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);
									
											ngf += 1;
											if (ngf >= pinfo.gftime)
												break;
										}
									}//if (pinfo.gftime > 0)
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}

									if ((0 != wcc[0]) && (pinfo.ytime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
												sinfo.conmode = /*29*/ncmode;
											else if ((0x5a <= cp) && (cp <= 0x5d))
												sinfo.conmode = cp;
											else if (0xC8 == cp)
                                                sinfo.conmode = 29;
											sinfo.color = 0x01;
											sinfo.time = pinfo.ytime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
											{
												sinfo.phase = 0;
												sinfo.phase |= (0x01 << (cp - 1));
											}
											else if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
											{
												sinfo.phase = 0;
											}			
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == wnpcc[i])
                    								break;
                								for (j = 0; j < sinfo.chans; j++)
                								{
                    								if (0 == sinfo.csta[j])
                        								break;
                    								tcsta = sinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (wnpcc[i] == tcsta)
                    								{
                        								sinfo.csta[j] &= 0xfc;
														sinfo.csta[j] |= 0x01; //00000001-yellow
														break;
                    								}
                								}
            								}
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == wpcc[i])
                                                	break;
                                            	for (j = 0; j < sinfo.chans; j++)
                                            	{
                                                	if (0 == sinfo.csta[j])
                                                    	break;
                                                	tcsta = sinfo.csta[j];
                                                	tcsta &= 0xfc;
                                                	tcsta >>= 2;
                                                	tcsta &= 0x3f;
                                                	if (wpcc[i] == tcsta)
                                                	{
                                                    	sinfo.csta[j] &= 0xfc;
														break;
                                                	}
                                            	}
                                        	}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (pinfo.ytime > 0))

									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            		{
                            			memset(&tctd,0,sizeof(tctd));
                            			tctd.mode = 29;//traffic control
                            			tctd.pattern = *(fcdata->patternid);
                            			tctd.lampcolor = 0x01;
                            			tctd.lamptime = pinfo.ytime;
                            			tctd.phaseid = 0;
                            			tctd.stageline = pinfo.stageline;
                            			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
                            			{
                            			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            			#endif
                            			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 29;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x01;
                                        tctd.lamptime = pinfo.ytime;
                                        tctd.phaseid = 0;
                                        tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = pinfo.stageline;
                            		fbdata[3] = 0x01;
                            		fbdata[4] = pinfo.ytime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                            			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                            			{
                            			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                			{
                                			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   				printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                		printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}

									if ((0 != wcc[0]) && (pinfo.rtime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
												sinfo.conmode = /*29*/ncmode;
											else if ((0x5a <= cp) && (cp <= 0x5d))
												sinfo.conmode = cp;
											else if (0xC8 == cp)
                                                sinfo.conmode = 29;
											sinfo.color = 0x00;
											sinfo.time = pinfo.rtime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
											{
												sinfo.phase = 0;
												sinfo.phase |= (0x01 << (cp - 1));
											}
											else if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
											{
												sinfo.phase = 0;
											}			
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == wcc[i])
                                                	break;
                                            	for (j = 0; j < sinfo.chans; j++)
                                            	{
                                                	if (0 == sinfo.csta[j])
                                                    	break;
                                                	tcsta = sinfo.csta[j];
                                                	tcsta &= 0xfc;
                                                	tcsta >>= 2;
                                                	tcsta &= 0x3f;
                                                	if (wcc[i] == tcsta)
                                                	{
                                                    	sinfo.csta[j] &= 0xfc;
														break;
                                                	}
                                            	}
                                        	}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (pinfo.rtime > 0))

									if (pinfo.rtime > 0)
									{
										if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            			{
                                			memset(&tctd,0,sizeof(tctd));
                                			tctd.mode = 29;//network control
                                			tctd.pattern = *(fcdata->patternid);
                                			tctd.lampcolor = 0x00;
                                			tctd.lamptime = pinfo.rtime;
                                			tctd.phaseid = 0;
                                			tctd.stageline = pinfo.stageline;
                                			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
                                			{
                                			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                            			}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&tctd,0,sizeof(tctd));
                                            tctd.mode = 29;//network control
                                            tctd.pattern = *(fcdata->patternid);
                                            tctd.lampcolor = 0x00;
                                            tctd.lamptime = pinfo.rtime;
                                            tctd.phaseid = 0;
                                            tctd.stageline = pinfo.stageline;
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#endif
										fbdata[1] = 29;
                                		fbdata[2] = pinfo.stageline;
                                		fbdata[3] = 0x00;
                                		fbdata[4] = pinfo.rtime;
                                		if (!wait_write_serial(*(fcdata->fbserial)))
                                		{
                                   			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                   			{
                                   			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                       			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   			#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
                                       			update_event_list(fcdata->ec,fcdata->el,1,16, \
																		ct.tv_sec,fcdata->markbit);
                                       			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                       			{
                                       			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                           			printf("gen err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                       			#endif
                                       			}
                                   			}
                                		}
                                		else
                                		{
                                		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   			printf("face board port cannot write,Line:%d\n",__LINE__);
                                		#endif
                                		}
										sleep(pinfo.rtime);
									}

									if ((0x5a <= tcbuf[1]) && (tcbuf[1] <= 0x5d))
                                    {	
										if (SUCCESS != \
											soc_set_lamp_color(*(fcdata->bbserial),lkch[tcbuf[1]-0x5a],0x02))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
							
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																lkch[tcbuf[1]-0x5a],0x02,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										ncmode = *(fcdata->contmode);
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = tcbuf[1];
											sinfo.color = 0x02;
											sinfo.time = 0;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											sinfo.phase = 0;

											sinfo.chans = 0;
											memset(sinfo.csta,0,sizeof(sinfo.csta));
											csta = sinfo.csta;
											for (i = 0; i < MAX_CHANNEL; i++)
											{
												if (0 == lkch[tcbuf[1]-0x5a][i])
													break;
												sinfo.chans += 1;
												tcsta = lkch[tcbuf[1]-0x5a][i];
												tcsta <<= 2;
												tcsta &= 0xfc;
												tcsta |= 0x02;
												*csta = tcsta;
												csta++;
											}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));				
											memset(sibuf,0,sizeof(sibuf));
											if (SUCCESS != status_info_report(sibuf,&sinfo))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
										if(SUCCESS!=soc_set_lamp_color(*(fcdata->bbserial),ccon,0x02))
                                        {
                                        #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                            printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                            *(fcdata->markbit) |= 0x0800;
                                        }
                                        if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                                                    		ccon,0x02,fcdata->markbit))
                                        {
                                        #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                            printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                        #endif
                                        }
										ncmode = *(fcdata->contmode);
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = ncmode;
											sinfo.color = 0x02;
											sinfo.time = 0;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
											sinfo.chans = 0;
											memset(sinfo.csta,0,sizeof(sinfo.csta));
											csta = sinfo.csta;
											for (i = 0; i < MAX_CHANNEL; i++)
											{
												if (0 == ccon[i])
													break;
												sinfo.chans += 1;
												tcsta = ccon[i];
												tcsta <<= 2;
												tcsta &= 0xfc;
												tcsta |= 0x02; //00000010-green 
												*csta = tcsta;
												csta++;
											}

											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
											memset(sibuf,0,sizeof(sibuf));
											if (SUCCESS != status_info_report(sibuf,&sinfo))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
										memset(&tctd,0,sizeof(tctd));
										tctd.mode = 29;//traffic control
										tctd.pattern = *(fcdata->patternid);
										tctd.lampcolor = 0x02;
										tctd.lamptime = 0;
										tctd.phaseid = 0;
										tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 29;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x02;
                                        tctd.lamptime = 0;
                                        tctd.phaseid = 0;
                                        tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = pinfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = 0;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
							{//if ((0x01 <= tcbuf[1]) && (tcbuf[1] <= 0x20))
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
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("Not fit phase id,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									objecti[0].objectv[0] = 0xf4;
                                    factovs = 0;
                                    memset(cbuf,0,sizeof(cbuf));
                                    if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x00))
                                    {
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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

								if ((1 == tcred) || (1 == socyfyes) || (1 == close) || (1 == phcon))
                            	{//if ((1 == tcred) || (1 == tcyfyes) || (1 == close))
									fpc = 1;
									phcon = 0;
									cp = tcbuf[1];
                                	if (1 == socyfyes)
                                	{
                                    	pthread_cancel(socyfpid);
                                    	pthread_join(socyfpid,NULL);
                                    	socyfyes = 0;
                                	}
									if (1 != tcred)
                                    {
                                        new_all_red(&ardata);
                                    }
                                    tcred = 0;
                                    close = 0;
					//				#ifdef CLOSE_LAMP
                                    soc_set_lamp_color(*(fcdata->bbserial),clch,0x03);
									update_channel_status(fcdata->sockfd,chanstatus,clch,0x03,fcdata->markbit);
                    //                #endif
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
                                	if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),nc,0x02))
                                	{
                                	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
                                    	*(fcdata->markbit) |= 0x0800;
                                	}
                                	if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                                        				nc,0x02,fcdata->markbit))
                                	{
                                	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    	printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                	#endif
                                	}
						
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										sinfo.conmode = 29;
										sinfo.color = 0x02;
										sinfo.time = 0;
										sinfo.stage = 0;
										sinfo.cyclet = 0;
							//			sinfo.phase = cp;
										sinfo.phase = 0;
										sinfo.phase |= (0x01 << (cp - 1));
							/*	
										sinfo.chans = 0;
            							memset(sinfo.csta,0,sizeof(sinfo.csta));
            							csta = sinfo.csta;
            							for (i = 0; i < MAX_CHANNEL; i++)
            							{
                							sinfo.chans += 1;
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
										sinfo.chans = 0;
										memset(sinfo.csta,0,sizeof(sinfo.csta));
                                        csta = sinfo.csta;
										for (i = 0; i < MAX_CHANNEL; i++)
                                        {
                                            if (0 == nc[i])
                                                break;
                                            sinfo.chans += 1;
                                            tcsta = nc[i];
                                            tcsta <<= 2;
                                            tcsta &= 0xfc;
                                            tcsta |= 0x02; //00000010-green 
                                            *csta = tcsta;
                                            csta++;
                                        }
										memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            							memset(sibuf,0,sizeof(sibuf));
            							if (SUCCESS != status_info_report(sibuf,&sinfo))
            							{
            							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
									cp = pinfo.phaseid;
								}//phase control of first times

								if (cp == tcbuf[1])
								{//control phase is current phase;
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
										sinfo.conmode = 29;
										sinfo.color = 0x02;
										sinfo.time = 0;
										sinfo.stage = 0;
										sinfo.cyclet = 0;
										sinfo.phase = 0;
										sinfo.phase |= (0x01 << (cp - 1));
								
										sinfo.chans = 0;
            							memset(sinfo.csta,0,sizeof(sinfo.csta));
            							csta = sinfo.csta;
            							for (i = 0; i < MAX_CHANNEL; i++)
            							{
                							if (0 == cc[i])
                    							break;
                							sinfo.chans += 1;
                							tcsta = cc[i];
                							tcsta <<= 2;
                							tcsta &= 0xfc;
                							tcsta |= 0x02; //00000010-green 
                							*csta = tcsta;
                							csta++;
            							}
										memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));	
										memset(sibuf,0,sizeof(sibuf));
            							if (SUCCESS != status_info_report(sibuf,&sinfo))
            							{
            							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
								}//control phase is current phase
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
										}//if ((0x5a <= cp) && (cp <= 0x5d))
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
						#if 0	
									sinfo.chans = 0;
                                    memset(sinfo.csta,0,sizeof(sinfo.csta));
                                    csta = sinfo.csta;
                                    for (i = 0; i < MAX_CHANNEL; i++)
                                    {
                                        if (0 == cc[i])
                                            break;
                                        sinfo.chans += 1;
                                        tcsta = cc[i];
                                        tcsta <<= 2;
                                        tcsta &= 0xfc;
                                        tcsta |= 0x02; //00000010-green 
                                        *csta = tcsta;
                                    	csta++;
                                    }
						#endif	
									if ((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{
											if ((0x01 <= cp) && (cp <= 0x20))	
												sinfo.conmode = /*29*/ncmode;
											else if ((0x5a <= cp) && (cp <= 0x5d))
												sinfo.conmode = cp;
											else if (0xC8 == cp)
                                                sinfo.conmode = 29;
											sinfo.color = 0x02;
											sinfo.time = pinfo.gftime + pinfo.ytime + pinfo.rtime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
											{
												sinfo.phase = 0;
												sinfo.phase |= (0x01 << (cp - 1));
											}
											else if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
											{
												sinfo.phase = 0;
											}
											sinfo.chans = 0;
            								memset(sinfo.csta,0,sizeof(sinfo.csta));
            								csta = sinfo.csta;
            								for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == cc[i])
                    								break;
                								sinfo.chans += 1;
                								tcsta = cc[i];
                								tcsta <<= 2;
                								tcsta &= 0xfc;
                								tcsta |= 0x02; //00000010-green 
                								*csta = tcsta;
                								csta++;
            								}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));	
											memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}
									}//if((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
						
									if ((0 != wcc[0]) && (pinfo.gftime > 0))
									{	
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
                                                sinfo.conmode = /*29*/ncmode;
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
                                                sinfo.conmode = cp;
											else if (0xC8 == cp)
                                                sinfo.conmode = 29;
											sinfo.color = 0x03;
											sinfo.time = pinfo.gftime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                sinfo.phase = 0;
                                                sinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
                                            {
                                                sinfo.phase = 0;
                                            }								
	
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == wcc[i])
                                                	break;
                                            	for (j = 0; j < sinfo.chans; j++)
                                            	{
                                                	if (0 == sinfo.csta[j])
                                                    	break;
                                                	tcsta = sinfo.csta[j];
                                                	tcsta &= 0xfc;
                                                	tcsta >>= 2;
                                                	tcsta &= 0x3f;
                                                	if (wcc[i] == tcsta)
                                                	{
                                                    	sinfo.csta[j] &= 0xfc;
                                                    	sinfo.csta[j] |= 0x03; //00000001-green flash
														break;
                                                	}
                                            	}
                                        	}								
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));	
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively										
									}//if ((0 != wcc[0]) && (pinfo.gftime > 0))

									//green flash
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&tctd,0,sizeof(tctd));
										tctd.mode = 29;//traffic control
										tctd.pattern = *(fcdata->patternid);
										tctd.lampcolor = 0x02;
										tctd.lamptime = pinfo.gftime;
										tctd.phaseid = cp;
										tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 29;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x02;
                                        tctd.lamptime = pinfo.gftime;
                                        tctd.phaseid = cp;
                                        tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = pinfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = pinfo.gftime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									if (pinfo.gftime > 0)
									{
										ngf = 0;
										while (1)
										{
											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x03,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);
									
											ngf += 1;
											if (ngf >= pinfo.gftime)
												break;
										}
									}//if (pinfo.gftime > 0)
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}

									if ((0 != wcc[0]) && (pinfo.ytime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
												sinfo.conmode = /*29*/ncmode;
											else if ((0x5a <= cp) && (cp <= 0x5d))
												sinfo.conmode = cp;
											else if (0xC8 == cp)
                                                sinfo.conmode = 29;
											sinfo.color = 0x01;
											sinfo.time = pinfo.ytime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                sinfo.phase = 0;
                                                sinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
                                            {
                                                sinfo.phase = 0;
                                            }											
				
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == wnpcc[i])
                    								break;
                								for (j = 0; j < sinfo.chans; j++)
                								{
                    								if (0 == sinfo.csta[j])
                        								break;
                    								tcsta = sinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (wnpcc[i] == tcsta)
                    								{
                        								sinfo.csta[j] &= 0xfc;
														sinfo.csta[j] |= 0x01; //00000001-yellow
														break;
                    								}
                								}
            								}
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == wpcc[i])
                                                	break;
                                            	for (j = 0; j < sinfo.chans; j++)
                                            	{
                                                	if (0 == sinfo.csta[j])
                                                    	break;
                                                	tcsta = sinfo.csta[j];
                                                	tcsta &= 0xfc;
                                                	tcsta >>= 2;
                                                	tcsta &= 0x3f;
                                                	if (wpcc[i] == tcsta)
                                                	{
                                                    	sinfo.csta[j] &= 0xfc;
														break;
                                                	}
                                            	}
                                        	}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (pinfo.ytime > 0))

									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            		{
                            			memset(&tctd,0,sizeof(tctd));
                            			tctd.mode = 29;//traffic control
                            			tctd.pattern = *(fcdata->patternid);
                            			tctd.lampcolor = 0x01;
                            			tctd.lamptime = pinfo.ytime;
                            			tctd.phaseid = cp;
                            			tctd.stageline = pinfo.stageline;
                            			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
                            			{
                            			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            			#endif
                            			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 29;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x01;
                                        tctd.lamptime = pinfo.ytime;
                                        tctd.phaseid = cp;
                                        tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = pinfo.stageline;
                            		fbdata[3] = 0x01;
                            		fbdata[4] = pinfo.ytime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                            			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                            			{
                            			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                			{
                                			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   				printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                		printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}

									if ((0 != wcc[0]) && (pinfo.rtime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
												sinfo.conmode = /*29*/ncmode;
											else if ((0x5a <= cp) && (cp <= 0x5d))
												sinfo.conmode = cp;
											else if (0xC8 == cp)
                                                sinfo.conmode = 29;
											sinfo.color = 0x00;
											sinfo.time = pinfo.rtime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                sinfo.phase = 0;
                                                sinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
                                            {
                                                sinfo.phase = 0;
                                            }											
				
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == wcc[i])
                                                	break;
                                            	for (j = 0; j < sinfo.chans; j++)
                                            	{
                                                	if (0 == sinfo.csta[j])
                                                    	break;
                                                	tcsta = sinfo.csta[j];
                                                	tcsta &= 0xfc;
                                                	tcsta >>= 2;
                                                	tcsta &= 0x3f;
                                                	if (wcc[i] == tcsta)
                                                	{
                                                    	sinfo.csta[j] &= 0xfc;
														break;
                                                	}
                                            	}
                                        	}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (pinfo.rtime > 0))

									if (pinfo.rtime > 0)
									{
										if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            			{
                                			memset(&tctd,0,sizeof(tctd));
                                			tctd.mode = 29;//network control
                                			tctd.pattern = *(fcdata->patternid);
                                			tctd.lampcolor = 0x00;
                                			tctd.lamptime = pinfo.rtime;
                                			tctd.phaseid = cp;
                                			tctd.stageline = pinfo.stageline;
                                			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
                                			{
                                			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                            			}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&tctd,0,sizeof(tctd));
                                            tctd.mode = 29;//network control
                                            tctd.pattern = *(fcdata->patternid);
                                            tctd.lampcolor = 0x00;
                                            tctd.lamptime = pinfo.rtime;
                                            tctd.phaseid = cp;
                                            tctd.stageline = pinfo.stageline;
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#endif
										fbdata[1] = 29;
                                		fbdata[2] = pinfo.stageline;
                                		fbdata[3] = 0x00;
                                		fbdata[4] = pinfo.rtime;
                                		if (!wait_write_serial(*(fcdata->fbserial)))
                                		{
                                   			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                   			{
                                   			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                       			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   			#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
                                       			update_event_list(fcdata->ec,fcdata->el,1,16, \
																		ct.tv_sec,fcdata->markbit);
                                       			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                       			{
                                       			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                           			printf("gen err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                       			#endif
                                       			}
                                   			}
                                		}
                                		else
                                		{
                                		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   			printf("face board port cannot write,Line:%d\n",__LINE__);
                                		#endif
                                		}
										sleep(pinfo.rtime);
									}

									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),nc,0x02))
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			nc,0x02,fcdata->markbit))
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
                            		}
									ncmode = *(fcdata->contmode);
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										sinfo.conmode = /*29*/ncmode;
										sinfo.color = 0x02;
										sinfo.time = 0;
										sinfo.stage = 0;
										sinfo.cyclet = 0;
										sinfo.phase = 0;
										sinfo.phase |= (0x01 << (tcbuf[1] - 1));

										sinfo.chans = 0;
            							memset(sinfo.csta,0,sizeof(sinfo.csta));
            							csta = sinfo.csta;
            							for (i = 0; i < MAX_CHANNEL; i++)
            							{
                							if (0 == nc[i])
                    							break;
                							sinfo.chans += 1;
                							tcsta = nc[i];
                							tcsta <<= 2;
                							tcsta &= 0xfc;
                							tcsta |= 0x02;
                							*csta = tcsta;
                							csta++;
            							}
										memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));				
            							memset(sibuf,0,sizeof(sibuf));
            							if (SUCCESS != status_info_report(sibuf,&sinfo))
            							{
            							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
										memset(&tctd,0,sizeof(tctd));
										tctd.mode = 29;//traffic control
										tctd.pattern = *(fcdata->patternid);
										tctd.lampcolor = 0x02;
										tctd.lamptime = 0;
										tctd.phaseid = tcbuf[1];
										tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 29;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x02;
                                        tctd.lamptime = 0;
                                        tctd.phaseid = tcbuf[1];
                                        tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = pinfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = 0;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									cp = tcbuf[1];

									objecti[0].objectv[0] = 0xf4;
									factovs = 0;
									memset(cbuf,0,sizeof(cbuf));
									if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
							}//else if ((0x01 <= tcbuf[1]) && (tcbuf[1] <= 0x20))
							else if (0x27 == tcbuf[1])
							{//net step by step
								fpc = 0;
								if ((1 == tcred) || (1 == socyfyes) || (1 == close) || (1 == phcon))
								{
                                	if (1 == socyfyes)
                                	{
                                    	pthread_cancel(socyfpid);
                                    	pthread_join(socyfpid,NULL);
                                    	socyfyes = 0;
                                	}
									if (1 != tcred)
									{
										new_all_red(&ardata);
									}
									phcon = 0;
									tcred = 0;
									close = 0;
					//				#ifdef CLOSE_LAMP
                                    soc_set_lamp_color(*(fcdata->bbserial),clch,0x03);
									update_channel_status(fcdata->sockfd,chanstatus,clch,0x03,fcdata->markbit);
                    //                #endif
									if (0 == cp)
									{//not have phase control
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                                		{
                                		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
                                    		*(fcdata->markbit) |= 0x0800;
                                		}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    									pinfo.chan,0x02,fcdata->markbit))
                                		{
                                		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    		printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               			#endif
                                		}

										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 29;
											sinfo.color = 0x02;
											sinfo.time = 0;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
											sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
											
										/*				
											sinfo.chans = 0;
            								memset(sinfo.csta,0,sizeof(sinfo.csta));
            								csta = sinfo.csta;
            								for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								sinfo.chans += 1;
                								tcsta = i + 1;
                								tcsta <<= 2;
                								tcsta &= 0xfc;
												for (j = 0; j < MAX_CHANNEL; j++)
												{
													if (0 == pinfo.chan[j])
														break;
													if ((i+1) == pinfo.chan[j])
													{
														tcsta |= 0x02;
														break;
													}
												}
                								*csta = tcsta;
                								csta++;
            								}
										*/
											sinfo.chans = 0;
											memset(sinfo.csta,0,sizeof(sinfo.csta));
                                            csta = sinfo.csta;
											for (i = 0; i < MAX_CHANNEL; i++)
                                        	{
                                            	if (0 == pinfo.chan[i])
                                                	break;
                                            	sinfo.chans += 1;
                                            	tcsta = pinfo.chan[i];
                                            	tcsta <<= 2;
                                            	tcsta &= 0xfc;
                                            	tcsta |= 0x02; //00000010-green 
                                            	*csta = tcsta;
                                            	csta++;
                                        	}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
											if (SUCCESS != 
												soc_set_lamp_color(*(fcdata->bbserial),lkch[cp-0x5a],0x02))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																		lkch[cp-0x5a],0x02,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("update err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}

										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
											{//report info to server actively
												sinfo.conmode = cp;
												sinfo.color = 0x02;
												sinfo.time = 0;
												sinfo.stage = pinfo.stageline;
												sinfo.cyclet = 0;
												sinfo.phase = 0;
												/*			
												sinfo.chans = 0;
												memset(sinfo.csta,0,sizeof(sinfo.csta));
												csta = sinfo.csta;
												for (i = 0; i < MAX_CHANNEL; i++)
												{
													sinfo.chans += 1;
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
												sinfo.chans = 0;
												memset(sinfo.csta,0,sizeof(sinfo.csta));
                                                csta = sinfo.csta;
												for (i = 0; i < MAX_CHANNEL; i++)
                                        		{
                                            		if (0 == lkch[cp-0x5a][i])
                                                		break;
                                            		sinfo.chans += 1;
                                            		tcsta = lkch[cp-0x5a][i];
                                            		tcsta <<= 2;
                                            		tcsta &= 0xfc;
                                            		tcsta |= 0x02; //00000010-green 
                                            		*csta = tcsta;
                                            		csta++;
                                        		}
												memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
												memset(sibuf,0,sizeof(sibuf));
												if (SUCCESS != status_info_report(sibuf,&sinfo))
												{
												#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
                                            if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),ccon,0x02))
                                            {
                                            #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                                printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                            #endif
                                                *(fcdata->markbit) |= 0x0800;
                                            }
                                            if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                                                                ccon,0x02,fcdata->markbit))
                                            {
                                            #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                                printf("update err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                            #endif
                                            }
                                            if((!(*(fcdata->markbit)&0x1000))&&(!(*(fcdata->markbit)&0x8000)))
                                            {//report info to server actively
                                                sinfo.conmode = 29;
                                                sinfo.color = 0x02;
                                                sinfo.time = 0;
                                                sinfo.stage = pinfo.stageline;
                                                sinfo.cyclet = 0;
                                                sinfo.phase = 0;

                                                sinfo.chans = 0;
                                                memset(sinfo.csta,0,sizeof(sinfo.csta));
                                                csta = sinfo.csta;
												for (i = 0; i < MAX_CHANNEL; i++)
                                                {
                                                    if (0 == ccon[i])
                                                        break;
                                                    sinfo.chans += 1;
                                                    tcsta = ccon[i];
                                                    tcsta <<= 2;
                                                    tcsta &= 0xfc;
                                                    tcsta |= 0x02; //00000010-green 
                                                    *csta = tcsta;
                                                    csta++;
                                                }
                                                memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
                                                memset(sibuf,0,sizeof(sibuf));
                                                if (SUCCESS != status_info_report(sibuf,&sinfo))
                                                {
                                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),cc,0x02))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				cc,0x02,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("update err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}

										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
											{//report info to server actively
												sinfo.conmode = 29;
												sinfo.color = 0x02;
												sinfo.time = 0;
												sinfo.stage = pinfo.stageline;
												sinfo.cyclet = 0;
												sinfo.phase = 0;
												sinfo.phase |= (0x01 << (cp - 1));
											/*				
												sinfo.chans = 0;
												memset(sinfo.csta,0,sizeof(sinfo.csta));
												csta = sinfo.csta;
												for (i = 0; i < MAX_CHANNEL; i++)
												{
													sinfo.chans += 1;
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
												sinfo.chans = 0;
												memset(sinfo.csta,0,sizeof(sinfo.csta));
                                                csta = sinfo.csta;
												for (i = 0; i < MAX_CHANNEL; i++)
                                        		{
                                            		if (0 == cc[i])
                                                		break;
                                            		sinfo.chans += 1;
                                            		tcsta = cc[i];
                                            		tcsta <<= 2;
                                            		tcsta &= 0xfc;
                                            		tcsta |= 0x02; //00000010-green 
                                            		*csta = tcsta;
                                            		csta++;
                                        		}
												memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
												memset(sibuf,0,sizeof(sibuf));
												if (SUCCESS != status_info_report(sibuf,&sinfo))
												{
												#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
								}//if ((1 == tcred) || (1 == tcyfyes) || (1 == close))

								if (0 == cp)
								{
									if((0==pinfo.cchan[0])&&(pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = /*29*/ncmode;
											sinfo.color = 0x02;
											sinfo.time = pinfo.gftime + pinfo.ytime + pinfo.rtime;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
											sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
											sinfo.chans = 0;
            								memset(sinfo.csta,0,sizeof(sinfo.csta));
            								csta = sinfo.csta;
            								for (i = 0; i < MAX_CHANNEL; i++)
            								{
												if (0 == pinfo.chan[i])
													break;
                								sinfo.chans += 1;
                								tcsta = pinfo.chan[i];
                								tcsta <<= 2;
                								tcsta &= 0xfc;
												tcsta |= 0x02;
                								*csta = tcsta;
                								csta++;
            								}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//info.cchan[0])&&(pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0))

									if ((0 != pinfo.cchan[0]) && (pinfo.gftime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = /*28*/ncmode;
											sinfo.color = 0x03;
											sinfo.time = pinfo.gftime;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
											sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
									
											for (i = 0; i < MAX_CHANNEL; i++)
                                			{
                                				if (0 == pinfo.cchan[i])
                                        			break;
                                    			for (j = 0; j < sinfo.chans; j++)
                                    			{
                                        			if (0 == sinfo.csta[j])
                                            			break;
                                        			tcsta = sinfo.csta[j];
                                        			tcsta &= 0xfc;
                                        			tcsta >>= 2;
                                        			tcsta &= 0x3f;
                                        			if (pinfo.cchan[i] == tcsta)
                                        			{
                                            			sinfo.csta[j] &= 0xfc;
                                           				sinfo.csta[j] |= 0x03; //00000001-green flash
														break;
                                        			}
                                    			}
                                			}								
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));	
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != pinfo.cchan[0]) && (pinfo.gftime > 0))

									//current phase begin to green flash
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&tctd,0,sizeof(tctd));
										tctd.mode = 29;//traffic control
										tctd.pattern = *(fcdata->patternid);
										tctd.lampcolor = 0x02;
										tctd.lamptime = pinfo.gftime;
										tctd.phaseid = pinfo.phaseid;
										tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 29;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x02;
                                        tctd.lamptime = pinfo.gftime;
                                        tctd.phaseid = pinfo.phaseid;
                                        tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = pinfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = pinfo.gftime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                                		if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                		{
                                		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                    		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                    		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                    		{
                                    		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
                                    		}
                                		}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									if (pinfo.gftime > 0)
									{	
									ngf = 0;
									while (1)
									{
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x03))
        								{
        								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            								printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															pinfo.cchan,0x03,fcdata->markbit))
        								{
        								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            								printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
        								#endif
        								}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);

										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x02))
        								{
        								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            								printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    									pinfo.cchan,0x02,fcdata->markbit))
                                		{
                                		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    		printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                		#endif
                                		}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);
								
										ngf += 1;
										if (ngf >= pinfo.gftime)
											break;
									}
									}//if (pinfo.gftime > 0)
									if (1 == (pinfo.cpcexist))
									{
										//current phase begin to yellow lamp
										if(SUCCESS!=soc_set_lamp_color(*(fcdata->bbserial),pinfo.cnpchan,0x01))
        								{
        								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            								printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
													pinfo.cnpchan,0x01, fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if(SUCCESS!=soc_set_lamp_color(*(fcdata->bbserial),pinfo.cpchan,0x00))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																	pinfo.cpchan,0x00,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if(SUCCESS!=soc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x01))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														pinfo.cchan,0x01,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}

									if ((0 != pinfo.cchan[0]) && (pinfo.ytime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = /*29*/ncmode;
											sinfo.color = 0x01;
											sinfo.time = pinfo.ytime;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
											sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
															
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == pinfo.cnpchan[i])
                    								break;
                								for (j = 0; j < sinfo.chans; j++)
                								{
                    								if (0 == sinfo.csta[j])
                        								break;
                    								tcsta = sinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (pinfo.cnpchan[i] == tcsta)
                    								{
                        								sinfo.csta[j] &= 0xfc;
														sinfo.csta[j] |= 0x01; //00000001-yellow
														break;
                    								}
                								}
            								}
											for (i = 0; i < MAX_CHANNEL; i++)
                                			{
                                				if (0 == pinfo.cpchan[i])
                                        			break;
                                    			for (j = 0; j < sinfo.chans; j++)
                                    			{
                                        			if (0 == sinfo.csta[j])
                                            			break;
                                        			tcsta = sinfo.csta[j];
                                        			tcsta &= 0xfc;
                                        			tcsta >>= 2;
                                        			tcsta &= 0x3f;
                                        			if (pinfo.cpchan[i] == tcsta)
                                        			{
                                            			sinfo.csta[j] &= 0xfc;
														break;
                                        			}
                                    			}
                                			}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != pinfo.cchan[0]) && (pinfo.ytime > 0))

									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            		{
                               			memset(&tctd,0,sizeof(tctd));
                               			tctd.mode = 29;//traffic control
                               			tctd.pattern = *(fcdata->patternid);
                               			tctd.lampcolor = 0x01;
                               			tctd.lamptime = pinfo.ytime;
                               			tctd.phaseid = pinfo.phaseid;
                               			tctd.stageline = pinfo.stageline;
                               			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
                               			{
                               			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               			#endif
                               			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 29;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x01;
                                        tctd.lamptime = pinfo.ytime;
                                        tctd.phaseid = pinfo.phaseid;
                                        tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = pinfo.stageline;
                            		fbdata[3] = 0x01;
                            		fbdata[4] = pinfo.ytime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                       			printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x00))
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															pinfo.cchan,0x00,fcdata->markbit))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}

									if ((0 != pinfo.cchan[0]) && (pinfo.rtime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = /*29*/ncmode;
											sinfo.color = 0x00;
											sinfo.time = pinfo.rtime;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											sinfo.phase = 0;
											sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
															
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == pinfo.cchan[i])
                    								break;
                								for (j = 0; j < sinfo.chans; j++)
                								{
                    								if (0 == sinfo.csta[j])
                        								break;
                    								tcsta = sinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (pinfo.cchan[i] == tcsta)
                    								{
                        								sinfo.csta[j] &= 0xfc;
														break;
                    								}
                								}
            								}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));	
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != pinfo.cchan[0]) && (pinfo.rtime > 0))

									if (pinfo.rtime > 0)
									{
										if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            			{
                                			memset(&tctd,0,sizeof(tctd));
                                			tctd.mode = 29;//traffic control
                                			tctd.pattern = *(fcdata->patternid);
                                			tctd.lampcolor = 0x00;
                                			tctd.lamptime = pinfo.rtime;
                                			tctd.phaseid = pinfo.phaseid;
                                			tctd.stageline = pinfo.stageline;
                                			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
                                			{
                                			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                            			}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&tctd,0,sizeof(tctd));
                                            tctd.mode = 29;//traffic control
                                            tctd.pattern = *(fcdata->patternid);
                                            tctd.lampcolor = 0x00;
                                            tctd.lamptime = pinfo.rtime;
                                            tctd.phaseid = pinfo.phaseid;
                                            tctd.stageline = pinfo.stageline;
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#endif
										fbdata[1] = 29;
                                		fbdata[2] = pinfo.stageline;
                                		fbdata[3] = 0x00;
                                		fbdata[4] = pinfo.rtime;
                                		if (!wait_write_serial(*(fcdata->fbserial)))
                                		{
                                    		if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                    		{
                                    		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
                                        		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                        		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                        		{
                                        		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                            		printf("genfile err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                        		#endif
                                        		}
                                    		}
                                		}
                                		else
                                		{
                                		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                		#endif
                                		}
										sleep(pinfo.rtime);
									}

									*(fcdata->slnum) += 1;
									*(fcdata->stageid) = \
									tscdata->timeconfig->TimeConfigList[socrettl][*(fcdata->slnum)].StageId;
							if(0==(tscdata->timeconfig->TimeConfigList[socrettl][*(fcdata->slnum)].StageId))
									{
										*(fcdata->slnum) = 0;
										*(fcdata->stageid) = \
										tscdata->timeconfig->TimeConfigList[socrettl][*(fcdata->slnum)].StageId;
									}
									//get phase info of next phase
									memset(&pinfo,0,sizeof(socpinfo_t));
        							if (SUCCESS != \
										soc_get_phase_info(fcdata,tscdata,socrettl,*(fcdata->slnum),&pinfo))
        							{
        							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            							printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
        							#endif
										soc_end_part_child_thread();
            							return FAIL;
        							}
        							*(fcdata->phaseid) = 0;
        							*(fcdata->phaseid) |= (0x01 << (pinfo.phaseid - 1));

									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																pinfo.chan,0x02,fcdata->markbit))
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                		printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
                            		}
									ncmode = *(fcdata->contmode);
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										sinfo.conmode = /*29*/ncmode;
										sinfo.color = 0x02;
										sinfo.time = 0;
										sinfo.stage = pinfo.stageline;
										sinfo.cyclet = 0;
										sinfo.phase = 0;
										sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
															
										sinfo.chans = 0;
            							memset(sinfo.csta,0,sizeof(sinfo.csta));
            							csta = sinfo.csta;
            							for (i = 0; i < MAX_CHANNEL; i++)
            							{
											if (0 == pinfo.chan[i])
												break;
                							sinfo.chans += 1;
                							tcsta = pinfo.chan[i];
                							tcsta <<= 2;
                							tcsta &= 0xfc;
											tcsta |= 0x02;
                							*csta = tcsta;
                							csta++;
            							}
										memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            							memset(sibuf,0,sizeof(sibuf));
            							if (SUCCESS != status_info_report(sibuf,&sinfo))
            							{
            							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
										memset(&tctd,0,sizeof(tctd));
										tctd.mode = 29;//traffic control
										tctd.pattern = *(fcdata->patternid);
										tctd.lampcolor = 0x02;
										tctd.lamptime = 0;
										tctd.phaseid = pinfo.phaseid;
										tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 29;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x02;
                                        tctd.lamptime = 0;
                                        tctd.phaseid = pinfo.phaseid;
                                        tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = pinfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = 0;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                                		if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                		{
                                		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                    		{
                                    		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
                                    		}
                                		}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
								}// 0 == cp
								if (0 != cp)
								{//if (0 != cp)
									if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
									{//if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
										*(fcdata->slnum) = 0;
										*(fcdata->stageid) = \
												tscdata->timeconfig->TimeConfigList[socrettl][0].StageId;
										memset(&pinfo,0,sizeof(socpinfo_t));
										if(SUCCESS != soc_get_phase_info(fcdata,tscdata,socrettl,0,&pinfo))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											soc_end_part_child_thread();
											return FAIL;
										}
										*(fcdata->phaseid) = 0;
										*(fcdata->phaseid) |= (0x01 << (pinfo.phaseid - 1));

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
												if (0 == pinfo.chan[j])
													break;
												if (pinfo.chan[j] == cc[i])	
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
										}//for (i = 0; i < MAX_CHANNEL; i++)
									}//if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
									else if ((0x01 <= cp) && (cp <= 0x20))
									{//else if ((0x01 <= cp) && (cp <= 0x20))
										for (i = 0; i < (tscdata->timeconfig->FactStageNum); i++)
										{
											if (0 == (tscdata->timeconfig->TimeConfigList[socrettl][i].StageId))
												break;
											rpi = tscdata->timeconfig->TimeConfigList[socrettl][i].PhaseId;
											rpc = 0;
											get_phase_id(rpi,&rpc);
											if (cp == rpc)
											{
												break;
											}
										}
										if (i != (tscdata->timeconfig->FactStageNum))
										{
											if(0==(tscdata->timeconfig->TimeConfigList[socrettl][i+1].StageId))
											{
												*(fcdata->slnum) = 0;
												*(fcdata->stageid) = \
												tscdata->timeconfig->TimeConfigList[socrettl][0].StageId;
												memset(&pinfo,0,sizeof(socpinfo_t));
												if(SUCCESS != \
													soc_get_phase_info(fcdata,tscdata,socrettl,0,&pinfo))
												{
												#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
													printf("info err,File: %s,Line: %d\n",__FILE__,__LINE__);
												#endif
													soc_end_part_child_thread();
													return FAIL;
												}
												*(fcdata->phaseid) = 0;
												*(fcdata->phaseid) |= (0x01 << (pinfo.phaseid - 1));
											}
											else
											{
												*(fcdata->slnum) = i + 1;
												*(fcdata->stageid) = \
													tscdata->timeconfig->TimeConfigList[socrettl][i+1].StageId;
												memset(&pinfo,0,sizeof(socpinfo_t));
												if(SUCCESS != \
													soc_get_phase_info(fcdata,tscdata,socrettl,i+1,&pinfo))
												{
												#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
													printf("info err,File: %s,Line: %d\n",__FILE__,__LINE__);
												#endif
													soc_end_part_child_thread();
													return FAIL;
												}
												*(fcdata->phaseid) = 0;
												*(fcdata->phaseid) |= (0x01 << (pinfo.phaseid - 1));
											}
										}//if (i != (tscdata->timeconfig->FactStageNum))
										else
										{
											*(fcdata->slnum) = 0;
											*(fcdata->stageid) = \
													tscdata->timeconfig->TimeConfigList[socrettl][0].StageId;
											memset(&pinfo,0,sizeof(socpinfo_t));
											if(SUCCESS != soc_get_phase_info(fcdata,tscdata,socrettl,0,&pinfo))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												soc_end_part_child_thread();
												return FAIL;
											}
											*(fcdata->phaseid) = 0;
											*(fcdata->phaseid) |= (0x01 << (pinfo.phaseid - 1));
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
													if (0 == pinfo.chan[j])
														break;
													if (pinfo.chan[j] == \
														tscdata->channel->ChannelList[i].ChannelId)
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

									if((0==wcc[0])&&(pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
												sinfo.conmode = /*29*/ncmode;
											else if ((0x5a <= cp) && (cp <= 0x5d))
												sinfo.conmode = cp;
											else if (0xC8 == cp)
                                                sinfo.conmode = 29;
											sinfo.color = 0x02;
											sinfo.time = pinfo.gftime + pinfo.ytime + pinfo.rtime;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
											{
												sinfo.phase = 0;
												sinfo.phase |= (0x01 << (cp - 1));
											}
											else if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
											{
												sinfo.phase = 0;
											}
											sinfo.chans = 0;
            								memset(sinfo.csta,0,sizeof(sinfo.csta));
            								csta = sinfo.csta;
            								for (i = 0; i < MAX_CHANNEL; i++)
            								{
												if (0 == cc[i])
													break;
                								sinfo.chans += 1;
                								tcsta = cc[i];
                								tcsta <<= 2;
                								tcsta &= 0xfc;
												tcsta |= 0x02;
                								*csta = tcsta;
                								csta++;
            								}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//info.cchan[0])&&(pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0))

									if ((0 != wcc[0]) && (pinfo.gftime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
												sinfo.conmode = /*28*/ncmode;
											else if ((0x5a <= cp) && (cp <= 0x5d))
												sinfo.conmode = cp;
											else if (0xC8 == cp)
                                                sinfo.conmode = 29;
											sinfo.color = 0x03;
											sinfo.time = pinfo.gftime;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
											{
												sinfo.phase = 0;
												sinfo.phase |= (0x01 << (cp - 1));
											}
											else if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
											{
												sinfo.phase = 0;
											}
											for (i = 0; i < MAX_CHANNEL; i++)
                                			{
                                				if (0 == wcc[i])
                                        			break;
                                    			for (j = 0; j < sinfo.chans; j++)
                                    			{
                                        			if (0 == sinfo.csta[j])
                                            			break;
                                        			tcsta = sinfo.csta[j];
                                        			tcsta &= 0xfc;
                                        			tcsta >>= 2;
                                        			tcsta &= 0x3f;
                                        			if (wcc[i] == tcsta)
                                        			{
                                            			sinfo.csta[j] &= 0xfc;
                                           				sinfo.csta[j] |= 0x03; //00000001-green flash
														break;
                                        			}
                                    			}
                                			}								
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));	
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != pinfo.cchan[0]) && (pinfo.gftime > 0))

									//current phase begin to green flash
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
									{
										memset(&tctd,0,sizeof(tctd));
										tctd.mode = 29;//traffic control
										tctd.pattern = *(fcdata->patternid);
										tctd.lampcolor = 0x02;
										tctd.lamptime = pinfo.gftime;
										tctd.phaseid = cp;
										tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 29;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x02;
                                        tctd.lamptime = pinfo.gftime;
                                        tctd.phaseid = cp;
                                        tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = pinfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = pinfo.gftime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                                		if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                		{
                                		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                    		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                    		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                    		{
                                    		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
                                    		}
                                		}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									if (pinfo.gftime > 0)
									{	
										ngf = 0;
										while (1)
										{
											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x03,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("updatechannelerr,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("setgreenlamperr,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x02,fcdata->markbit))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("updatechannelerr,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);
									
											ngf += 1;
											if (ngf >= pinfo.gftime)
												break;
										}
									}//if (pinfo.gftime > 0)
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS!=soc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            								printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														wcc,0x01,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}

									if ((0 != wcc[0]) && (pinfo.ytime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
												sinfo.conmode = /*29*/ncmode;
											else if ((0x5a <= cp) && (cp <= 0x5d))
												sinfo.conmode = cp;
											else if (0xC8 == cp)
                                                sinfo.conmode = 29;
											sinfo.color = 0x01;
											sinfo.time = pinfo.ytime;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
											{
												sinfo.phase = 0;
												sinfo.phase |= (0x01 << (cp - 1));
											}
											else if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
											{
												sinfo.phase = 0;
											}
															
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == wnpcc[i])
                    								break;
                								for (j = 0; j < sinfo.chans; j++)
                								{
                    								if (0 == sinfo.csta[j])
                        								break;
                    								tcsta = sinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (wnpcc[i] == tcsta)
                    								{
                        								sinfo.csta[j] &= 0xfc;
														sinfo.csta[j] |= 0x01; //00000001-yellow
														break;
                    								}
                								}
            								}
											for (i = 0; i < MAX_CHANNEL; i++)
                                			{
                                				if (0 == wpcc[i])
                                        			break;
                                    			for (j = 0; j < sinfo.chans; j++)
                                    			{
                                        			if (0 == sinfo.csta[j])
                                            			break;
                                        			tcsta = sinfo.csta[j];
                                        			tcsta &= 0xfc;
                                        			tcsta >>= 2;
                                        			tcsta &= 0x3f;
                                        			if (wpcc[i] == tcsta)
                                        			{
                                            			sinfo.csta[j] &= 0xfc;
														break;
                                        			}
                                    			}
                                			}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != pinfo.cchan[0]) && (pinfo.ytime > 0))

									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            		{
                               			memset(&tctd,0,sizeof(tctd));
                               			tctd.mode = 29;//traffic control
                               			tctd.pattern = *(fcdata->patternid);
                               			tctd.lampcolor = 0x01;
                               			tctd.lamptime = pinfo.ytime;
                               			tctd.phaseid = cp;
                               			tctd.stageline = pinfo.stageline;
                               			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
                               			{
                               			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               			#endif
                               			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 29;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x01;
                                        tctd.lamptime = pinfo.ytime;
                                        tctd.phaseid = cp;
                                        tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = pinfo.stageline;
                            		fbdata[3] = 0x01;
                            		fbdata[4] = pinfo.ytime;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                       			printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															wcc,0x00,fcdata->markbit))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}

									if ((0 != wcc[0]) && (pinfo.rtime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
												sinfo.conmode = /*29*/ncmode;
											else if ((0x5a <= cp) && (cp <= 0x5d))	
												sinfo.conmode = cp;
											else if (0xC8 == cp)
                                                sinfo.conmode = 29;
											sinfo.color = 0x00;
											sinfo.time = pinfo.rtime;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
											{
												sinfo.phase = 0;
												sinfo.phase |= (0x01 << (cp - 1));
											}
											else if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
												sinfo.phase = 0;
			
											for (i = 0; i < MAX_CHANNEL; i++)
            								{
                								if (0 == wcc[i])
                    								break;
                								for (j = 0; j < sinfo.chans; j++)
                								{
                    								if (0 == sinfo.csta[j])
                        								break;
                    								tcsta = sinfo.csta[j];
                    								tcsta &= 0xfc;
                    								tcsta >>= 2;
                    								tcsta &= 0x3f;
                    								if (wcc[i] == tcsta)
                    								{
                        								sinfo.csta[j] &= 0xfc;
														break;
                    								}
                								}
            								}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));	
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != pinfo.cchan[0]) && (pinfo.rtime > 0))

									if (pinfo.rtime > 0)
									{
										if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            			{
                                			memset(&tctd,0,sizeof(tctd));
                                			tctd.mode = 29;//traffic control
                                			tctd.pattern = *(fcdata->patternid);
                                			tctd.lampcolor = 0x00;
                                			tctd.lamptime = pinfo.rtime;
                                			tctd.phaseid = cp;
                                			tctd.stageline = pinfo.stageline;
                                			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
                                			{
                                			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                            			}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&tctd,0,sizeof(tctd));
                                            tctd.mode = 29;//traffic control
                                            tctd.pattern = *(fcdata->patternid);
                                            tctd.lampcolor = 0x00;
                                            tctd.lamptime = pinfo.rtime;
                                            tctd.phaseid = cp;
                                            tctd.stageline = pinfo.stageline;
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
											{
											#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
												printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#endif
										fbdata[1] = 29;
                                		fbdata[2] = pinfo.stageline;
                                		fbdata[3] = 0x00;
                                		fbdata[4] = pinfo.rtime;
                                		if (!wait_write_serial(*(fcdata->fbserial)))
                                		{
                                    		if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                    		{
                                    		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
                                        		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                        		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                        		{
                                        		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                            		printf("file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        		#endif
                                        		}
                                    		}
                                		}
                                		else
                                		{
                                		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                		#endif
                                		}
										sleep(pinfo.rtime);
									}
									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			pinfo.chan,0x02,fcdata->markbit))
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
                            		}
									ncmode = *(fcdata->contmode);
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										sinfo.conmode = /*29*/ncmode;
										sinfo.color = 0x02;
										sinfo.time = 0;
										sinfo.stage = 0;
										sinfo.cyclet = 0;
										sinfo.phase = 0;
										sinfo.phase |= (0x01 << (pinfo.phaseid - 1));

										sinfo.chans = 0;
            							memset(sinfo.csta,0,sizeof(sinfo.csta));
            							csta = sinfo.csta;
            							for (i = 0; i < MAX_CHANNEL; i++)
            							{
                							if (0 == pinfo.chan[i])
                    							break;
                							sinfo.chans += 1;
                							tcsta = pinfo.chan[i];
                							tcsta <<= 2;
                							tcsta &= 0xfc;
                							tcsta |= 0x02;
                							*csta = tcsta;
                							csta++;
            							}
										memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));				
            							memset(sibuf,0,sizeof(sibuf));
            							if (SUCCESS != status_info_report(sibuf,&sinfo))
            							{
            							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
										memset(&tctd,0,sizeof(tctd));
										tctd.mode = 29;//traffic control
										tctd.pattern = *(fcdata->patternid);
										tctd.lampcolor = 0x02;
										tctd.lamptime = 0;
										tctd.phaseid = pinfo.phaseid;
										tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
                                        tctd.mode = 29;//traffic control
                                        tctd.pattern = *(fcdata->patternid);
                                        tctd.lampcolor = 0x02;
                                        tctd.lamptime = 0;
                                        tctd.phaseid = pinfo.phaseid;
                                        tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = pinfo.stageline;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = 0;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}	
								}//if (0 != cp)		
					
								cp = 0;
								objecti[0].objectv[0] = 0x28;
                                factovs = 0;
                                memset(cbuf,0,sizeof(cbuf));
                                if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                                {
                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
								tcred = 0;
								close = 0;
								cp = 0;
								if (0 == socyfyes)
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
									int yfret = pthread_create(&socyfpid,NULL,(void *)soc_yellow_flash,&yfdata);
									if (0 != yfret)
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										soc_end_part_child_thread();
										objecti[0].objectv[0] = 0x24;
                                		factovs = 0;
                                		memset(cbuf,0,sizeof(cbuf));
                                		if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x00))
                                		{
                                		#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    		printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
                                		}
                                		if (!(*(fcdata->markbit) & 0x1000))
                                		{
                                    		write(*(fcdata->sockfd->csockfd),cbuf,factovs);
                                		}

										return FAIL;
									}
									socyfyes = 1;
								}

								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									sinfo.conmode = 29;
									sinfo.color = 0x05;
									sinfo.time = 0;
									sinfo.stage = 0;
									sinfo.cyclet = 0;
									sinfo.phase = 0;

									sinfo.chans = 0;
            						memset(sinfo.csta,0,sizeof(sinfo.csta));
            						csta = sinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						sinfo.chans += 1;
                						tcsta = i+1;
                						tcsta <<= 2;
                						tcsta &= 0xfc;
                						*csta = tcsta;
                						csta++;
            						}
									memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));						
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&sinfo))
            						{
            						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
								if (1 == socyfyes)
								{
									pthread_cancel(socyfpid);
									pthread_join(socyfpid,NULL);
									socyfyes = 0;
								}
					
								if (0 == tcred)
								{	
									new_all_red(&ardata);
									tcred = 1;
								}
								
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									sinfo.conmode = 29;
									sinfo.color = 0x00;
									sinfo.time = 0;
									sinfo.stage = 0;
									sinfo.cyclet = 0;
									sinfo.phase = 0;

									sinfo.chans = 0;
            						memset(sinfo.csta,0,sizeof(sinfo.csta));
            						csta = sinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						sinfo.chans += 1;
                						tcsta = i+1;
                						tcsta <<= 2;
                						tcsta &= 0xfc;
                						*csta = tcsta;
                						csta++;
            						}
									memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));						
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&sinfo))
            						{
            						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
								tcred = 0;
								cp = 0;
								if (1 == socyfyes)
                                {
                                    pthread_cancel(socyfpid);
                                    pthread_join(socyfpid,NULL);
                                    socyfyes = 0;
                                }
								if (0 == close)
                                {
                                    new_all_close(&acdata);
                                    close = 1;
                                }

								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									sinfo.conmode = 29;
									sinfo.color = 0x04;
									sinfo.time = 0;
									sinfo.stage = 0;
									sinfo.cyclet = 0;
									sinfo.phase = 0;

									sinfo.chans = 0;
            						memset(sinfo.csta,0,sizeof(sinfo.csta));
            						csta = sinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						sinfo.chans += 1;
                						tcsta = i+1;
                						tcsta <<= 2;
                						tcsta &= 0xfc;
                						*csta = tcsta;
                						csta++;
            						}
									memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));						
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&sinfo))
            						{
            						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
				{
					if ((0 == wllock) && (0 == netlock) && (0 == wtlock))
					{//Key control is valid only when wireless terminal control is not valid;
						if (0x01 == tcbuf[1])
						{//if (0x01 == tcbuf[1]),lock or unlock
							if (0 == keylock)
							{//if (0 == keylock)
								get_soc_status(&color,&leatime);
								if (0x02 != color)
								{//if (0x02 != color)
									struct timeval          spacetime;
									unsigned char			endlock = 0;
									while (1)
									{//while loop,000000
										spacetime.tv_sec = 0;
										spacetime.tv_usec = 200000;
										select(0,NULL,NULL,NULL,&spacetime);

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
										get_soc_status(&color,&leatime);
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
#if 0
								//the following code is for WuXi check;
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
								printf("********************leatime: %d,mingtime: %d,File:%s,Line:%d\n", \
										leatime,pinfo.mingtime,__FILE__,__LINE__);
								#endif
								if (leatime < pinfo.mingtime)
								{
									unsigned char		cmt = pinfo.mingtime - leatime;
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
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											return FAIL;
										}
										if (0 == mret)
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
								}//if (leatime < pinfo.mingtime)
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
								printf("end mintime pass,File: %s,LIne:%d\n",__FILE__,__LINE__);
								#endif
								//end code of WuXi check;	
#endif
								keylock = 1;
                                tcred = 0;
								close = 0;
                                cktem = 0;
                                kstep = 0;
								soc_end_child_thread();//end main thread and its child thread
					//			new_all_red(&ardata);
								*(fcdata->markbit2) |= 0x0002;
							#if 0
								if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
        						{
        						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            						printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        						#endif
									*(fcdata->markbit) |= 0x0800;
        						}
							#endif

								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									sinfo.conmode = 28;
									sinfo.color = 0x02;
									sinfo.time = 0;
									sinfo.stage = pinfo.stageline;
									sinfo.cyclet = 0;
									sinfo.phase = 0;
									sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
															
									sinfo.chans = 0;
            						memset(sinfo.csta,0,sizeof(sinfo.csta));
            						csta = sinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						if (0 == pinfo.chan[i])
                    						break;
                						sinfo.chans += 1;
                						tcsta = pinfo.chan[i];
                						tcsta <<= 2;
                						tcsta &= 0xfc;
                						tcsta |= 0x02; //00000010-green
                						*csta = tcsta;
                						csta++;
            						}
									memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&sinfo))
            						{
            						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                						printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively

								*(fcdata->contmode) = 28;//traffic control mode
								fbdata[1] = 28;
								fbdata[2] = pinfo.stageline;
								fbdata[3] = 0x02;
								fbdata[4] = 0;
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
                						update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                						if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                						{
                						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                    						printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                						#endif
                						}
									}
								}
								else
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("face board serial port cannot write,Line:%d\n",__LINE__);
								#endif
								}
								sendfaceInfoToBoard(fcdata,fbdata);
								tcecho[1] = 0x01;
								if (*(fcdata->markbit2) & 0x0800)
								{//comes from side door serial
									*(fcdata->markbit2) &= 0xf7ff;
									if (!wait_write_serial(*(fcdata->sdserial)))
                                    {
                                        if (write(*(fcdata->sdserial),tcecho,sizeof(tcecho)) < 0)
                                        {
                                        #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                            *(fcdata->markbit) |= 0x0800;
                                        }
                                    }
                                    else
                                    {
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                    #endif
                                    }
								}//comes from side door serial
								else
								{
									if (!wait_write_serial(*(fcdata->fbserial)))
									{
										if (write(*(fcdata->fbserial),tcecho,sizeof(tcecho)) < 0)
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
									}
									else
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("face board serial port cannot write,Line:%d\n",__LINE__);
									#endif
									}
								}

						//		#ifdef TIMING_DOWN_TIME
								if (*(fcdata->auxfunc) & 0x01)
								{//if (*(fcdata->auxfunc) & 0x01)
									if (!wait_write_serial(*(fcdata->bbserial)))
									{
										if (write(*(fcdata->bbserial),downti,sizeof(downti)) < 0)
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
									if (!wait_write_serial(*(fcdata->bbserial)))
									{
										if (write(*(fcdata->bbserial),edownti,sizeof(edownti)) < 0)
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG 
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
								}//if (*(fcdata->auxfunc) & 0x01)	
						//		#endif

								//send down time to configure tool
                                if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                                {
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("send control info to configure tool,File: %s,Line: %d\n", \
											__FILE__,__LINE__);
								#endif
                                    memset(&tctd,0,sizeof(tctd));
                                    tctd.mode = 28;
                                    tctd.pattern = *(fcdata->patternid);
                                    tctd.lampcolor = 0x02;
                                    tctd.lamptime = 0;
                                    tctd.phaseid = pinfo.phaseid;
                                    tctd.stageline = pinfo.stageline;
                                    if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
                                    {
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
								#ifdef EMBED_CONFIGURE_TOOL
                                if (*(fcdata->markbit2) & 0x0200)
                                {
									memset(&tctd,0,sizeof(tctd));
                                    tctd.mode = 28;
                                    tctd.pattern = *(fcdata->patternid);
                                    tctd.lampcolor = 0x02;
                                    tctd.lamptime = 0;
                                    tctd.phaseid = pinfo.phaseid;
                                    tctd.stageline = pinfo.stageline;
                                    if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
                                    {
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                                #endif
                                if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    pinfo.chan,0x02,fcdata->markbit))
                                {
                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
                                {//report info to server actively
									memset(err,0,sizeof(err));
									gettimeofday(&ct,NULL);
                					if (SUCCESS != err_report(err,ct.tv_sec,22,1))
                					{
                					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
							}//if (0 == keylock)
						
							if (1 == keylock)
							{//if (1 == keylock)
								kstep = 0;
								cktem = 0;
								keylock = 0;
								tcred = 0;
								close = 0;
								if (1 == socyfyes)
								{
									pthread_cancel(socyfpid);
									pthread_join(socyfpid,NULL);
									socyfyes = 0;
								}
								tcecho[1] = 0x00;
								if (*(fcdata->markbit2) & 0x0800)
                                {//comes from side door serial
                                    *(fcdata->markbit2) &= 0xf7ff;
                                    if (!wait_write_serial(*(fcdata->sdserial)))
                                    {
                                        if (write(*(fcdata->sdserial),tcecho,sizeof(tcecho)) < 0)
                                        {
                                        #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                            *(fcdata->markbit) |= 0x0800;
                                        }
                                    }
                                    else
                                    {
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                    #endif
                                    }
                                }//comes from side door serial
								else
								{
									if (!wait_write_serial(*(fcdata->fbserial)))
									{
										if (write(*(fcdata->fbserial),tcecho,sizeof(tcecho)) < 0)
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
									}
									else
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("face board serial port cannot write,Line:%d\n",__LINE__);
									#endif
									}
								}

								new_all_red(&ardata);	
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									sinfo.conmode = 28;
									sinfo.color = 0x00;
									sinfo.time = 0;
									sinfo.stage = 0;
									sinfo.cyclet = 0;
									sinfo.phase = 0;
															
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
									memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&sinfo))
            						{
            						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                						printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}

                                    memset(err,0,sizeof(err));
                                    gettimeofday(&ct,NULL);
                                    if (SUCCESS != err_report(err,ct.tv_sec,22,3))
                                    {
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
							//	all_red(*(fcdata->bbserial),0);
								*(fcdata->contmode) = contmode; //restore control mode;
								*(fcdata->markbit2) &= 0xfffd;
								if (0 == socyes)
								{
									memset(&tcdata,0,sizeof(tcdata));
									memset(&pinfo,0,sizeof(pinfo));
									tcdata.fd = fcdata;
									tcdata.td = tscdata;
									tcdata.cs = chanstatus;
									tcdata.pi = &pinfo;
									int tcret = pthread_create(&socpid,NULL, \
										(void *)start_optimize_coordinate_control,&tcdata);
									if (0 != tcret)
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("create thread err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
										soc_end_part_child_thread();
										return FAIL;
									}
									socyes = 1;
								}
								continue;
							}//if (1 == keylock)
						}//if (0x01 == tcbuf[1]),lock or unlock
						if (((0x11 <= tcbuf[1]) && (tcbuf[1] <= 0x18)) && (1 == keylock))	
						{//jump stage control
						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
							printf("tcbuf[1]:%d,File:%s,Line:%d\n",tcbuf[1],__FILE__,__LINE__);
						#endif
							memset(&mdt,0,sizeof(markdata_t));
							mdt.redl = &tcred;
							mdt.closel = &close;
							mdt.rettl = &socrettl;
							mdt.yfl = &socyfyes;
							mdt.yfid = &socyfpid;
							mdt.ardata = &ardata;
							mdt.fcdata = fcdata;
							mdt.tscdata = tscdata;
							mdt.chanstatus = chanstatus;
							mdt.sinfo = &sinfo;
							soc_jump_stage_control(&mdt,tcbuf[1],&pinfo);	
						}//jump stage control
						if (((0x20 <= tcbuf[1]) && (tcbuf[1] <= 0x27)) && (1 == keylock))
                        {//direction control
                        #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                            printf("tcbuf[1]:%d,cktem:%d,File:%s,Line:%d\n",tcbuf[1],cktem,__FILE__,__LINE__);
                        #endif
							if (cktem == tcbuf[1])
								continue;
							memset(&mdt,0,sizeof(markdata_t));
                            mdt.redl = &tcred;
                            mdt.closel = &close;
                            mdt.rettl = &socrettl;
                            mdt.yfl = &socyfyes;
                            mdt.yfid = &socyfpid;
                            mdt.ardata = &ardata;
                            mdt.fcdata = fcdata;
                            mdt.tscdata = tscdata;
                            mdt.chanstatus = chanstatus;
                            mdt.sinfo = &sinfo;
							mdt.kstep = &kstep;
							soc_dirch_control(&mdt,cktem,tcbuf[1],dirch,&pinfo);
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
								for (i = 0; i < MAX_CHANNEL; i++)
								{//for (i = 0; i < MAX_CHANNEL; i++)
									if (0 == dirch[cktem-0x20][i])
										break;
									ce = 0;
									for (j = 0; j < MAX_CHANNEL; j++)
									{//for (j = 0; j < MAX_CHANNEL; j++)
										if (0 == pinfo.chan[j])
											break;
										if (dirch[cktem-0x20][i] == pinfo.chan[j])
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
								
								if ((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
								{
									if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
									{
										sinfo.conmode = 28;
										sinfo.color = 0x02;
										sinfo.time = pinfo.gftime + pinfo.ytime + pinfo.rtime;
										sinfo.stage = pinfo.stageline;
										sinfo.cyclet = 0;
										sinfo.phase = 0;
										sinfo.chans = 0;
										memset(sinfo.csta,0,sizeof(sinfo.csta));
										csta = sinfo.csta;
										for (i = 0; i < MAX_CHANNEL; i++)
										{
											if (0 == dirch[cktem-0x20][i])
												break;
											sinfo.chans += 1;
											tcsta = dirch[cktem-0x20][i];
											tcsta <<= 2;
											tcsta &= 0xfc;
											tcsta |= 0x02; //00000010-green 
											*csta = tcsta;
											csta++;
										}
										memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
										memset(sibuf,0,sizeof(sibuf));
										if (SUCCESS != status_info_report(sibuf,&sinfo))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										else
										{
											write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
										}
									}
								}//if ((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
								if ((0 != wcc[0]) && (pinfo.gftime > 0))	
								{			
									if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										sinfo.conmode = 28;
										sinfo.color = 0x03;
										sinfo.time = pinfo.gftime;
										sinfo.stage = pinfo.stageline;
										sinfo.cyclet = 0;
										sinfo.phase = 0;
										for (i = 0; i < MAX_CHANNEL; i++)
										{
											if (0 == wcc[i])
												break;
											for (j = 0; j < sinfo.chans; j++)
											{
												if (0 == sinfo.csta[j])
													break;
												tcsta = sinfo.csta[j];
												tcsta &= 0xfc;
												tcsta >>= 2;
												tcsta &= 0x3f;
												if (wcc[i] == tcsta)
												{
													sinfo.csta[j] &= 0xfc;
													sinfo.csta[j] |= 0x03; //00000011-green flash
													break;
												}
											}
										}
										memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
										memset(sibuf,0,sizeof(sibuf));
										if (SUCCESS != status_info_report(sibuf,&sinfo))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("status pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										else
										{
											write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
										}
									}//report info to server actively
								}//if ((0 != wcc[0]) && (pinfo.gftime > 0))

								//green flash
								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
								{
									memset(&tctd,0,sizeof(tctd));
									tctd.mode = 28;//traffic control
									tctd.pattern = *(fcdata->patternid);
									tctd.lampcolor = 0x02;
									tctd.lamptime = pinfo.gftime;
									tctd.phaseid = 0;
									tctd.stageline = pinfo.stageline;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
								{
									memset(&tctd,0,sizeof(tctd));
                        			tctd.mode = 28;//traffic control
                        			tctd.pattern = *(fcdata->patternid);
                        			tctd.lampcolor = 0x02;
                        			tctd.lamptime = pinfo.gftime;
                        			tctd.phaseid = 0;
                        			tctd.stageline = pinfo.stageline;	
									if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#endif
								fbdata[1] = 28;
								fbdata[2] = pinfo.stageline;
								fbdata[3] = 0x02;
								fbdata[4] = pinfo.gftime;
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
										if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																	fcdata->softevent,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
								}
								else
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("face board serial port cannot write,Line:%d\n",__LINE__);
								#endif
								}
								ngf = 0;
								if ((0 != wcc[0]) && (pinfo.gftime > 0))
								{//if ((0 != wcc[0]) && (pinfo.gftime > 0))
									while (1)
									{
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x03,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);
										
										ngf += 1;
										if (ngf >= pinfo.gftime)
											break;
									}
								}//if ((0 != wcc[0]) && (pinfo.gftime > 0))
								if (1 == pce)
								{
								//current phase begin to yellow lamp
									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								else
								{
									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}

								if ((0 != wcc[0]) && (pinfo.ytime > 0))
								{
									if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										sinfo.conmode = 28;
										sinfo.color = 0x01;
										sinfo.time = pinfo.ytime;
										sinfo.stage = 0;
										sinfo.cyclet = 0;
										sinfo.phase = 0;
														
										for (i = 0; i < MAX_CHANNEL; i++)
										{
											if (0 == wnpcc[i])
												break;
											for (j = 0; j < sinfo.chans; j++)
											{
												if (0 == sinfo.csta[j])
													break;
												tcsta = sinfo.csta[j];
												tcsta &= 0xfc;
												tcsta >>= 2;
												tcsta &= 0x3f;
												if (wnpcc[i] == tcsta)
												{
													sinfo.csta[j] &= 0xfc;
													sinfo.csta[j] |= 0x01; //00000001-yellow
													break;
												}
											}
										}
										for (i = 0; i < MAX_CHANNEL; i++)
										{
											if (0 == wpcc[i])
												break;
											for (j = 0; j < sinfo.chans; j++)
											{
												if (0 == sinfo.csta[j])
													break;
												tcsta = sinfo.csta[j];
												tcsta &= 0xfc;
												tcsta >>= 2;
												tcsta &= 0x3f;
												if (wpcc[i] == tcsta)
												{
													sinfo.csta[j] &= 0xfc;
													break;
												}
											}
										}
										memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
										memset(sibuf,0,sizeof(sibuf));
										if (SUCCESS != status_info_report(sibuf,&sinfo))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("status pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										else
										{
											write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
										}
									}//report info to server actively
								}//if ((0 != wcc[0]) && (pinfo.ytime > 0))

								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
								{
									memset(&tctd,0,sizeof(tctd));
									tctd.mode = 28;//traffic control
									tctd.pattern = *(fcdata->patternid);
									tctd.lampcolor = 0x01;
									tctd.lamptime = pinfo.ytime;
									tctd.phaseid = 0;
									tctd.stageline = pinfo.stageline;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
								{
									memset(&tctd,0,sizeof(tctd));
									tctd.mode = 28;//traffic control
									tctd.pattern = *(fcdata->patternid);
									tctd.lampcolor = 0x01;
									tctd.lamptime = pinfo.ytime;
									tctd.phaseid = 0;
									tctd.stageline = pinfo.stageline;	
									if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#endif
								fbdata[1] = 28;
								fbdata[2] = pinfo.stageline;
								fbdata[3] = 0x01;
								fbdata[4] = pinfo.ytime;
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16, \
															ct.tv_sec,fcdata->markbit);
										if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																fcdata->softevent,fcdata->markbit))
										{	
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
								}
								else
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("face board serial port cannot write,Line:%d\n",__LINE__);
								#endif
								}
								sleep(pinfo.ytime);

								//current phase begin to red lamp
								if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
									*(fcdata->markbit) |= 0x0800;
								}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																		wcc,0x00,fcdata->markbit))
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}

								if ((0 != wcc[0]) && (pinfo.rtime > 0))
								{
									if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										sinfo.conmode = 28;
										sinfo.color = 0x00;
										sinfo.time = pinfo.rtime;
										sinfo.stage = 0;
										sinfo.cyclet = 0;
										sinfo.phase = 0;
																	
										for (i = 0; i < MAX_CHANNEL; i++)
										{
											if (0 == wcc[i])
												break;
											for (j = 0; j < sinfo.chans; j++)
											{
												if (0 == sinfo.csta[j])
													break;
												tcsta = sinfo.csta[j];
												tcsta &= 0xfc;
												tcsta >>= 2;
												tcsta &= 0x3f;
												if (wcc[i] == tcsta)
												{
													sinfo.csta[j] &= 0xfc;
													break;
												}
											}
										}
										memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
										memset(sibuf,0,sizeof(sibuf));
										if (SUCCESS != status_info_report(sibuf,&sinfo))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										else
										{
											write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
										}
									}//report info to server actively
								}//if ((0 != wcc[0]) && (pinfo.rtime > 0))
								
								if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                            	{
                            	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            	#endif
									*(fcdata->markbit) |= 0x0800;
                            	}
						
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																pinfo.chan,0x02,fcdata->markbit))
                            	{
                            	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                	printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            	#endif
                            	}

								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									sinfo.conmode = 28;
									sinfo.color = 0x02;
									sinfo.time = 0;
									sinfo.stage = pinfo.stageline;
									sinfo.cyclet = 0;
									sinfo.phase = 0;
									sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
															
									sinfo.chans = 0;
            						memset(sinfo.csta,0,sizeof(sinfo.csta));
            						csta = sinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
										if (0 == pinfo.chan[i])
											break;
                						sinfo.chans += 1;
										tcsta = pinfo.chan[i];
										tcsta <<= 2;
										tcsta &= 0xfc;
										tcsta |= 0x02;
										*csta = tcsta;
										csta++;
									}
									memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
									memset(sibuf,0,sizeof(sibuf));
									if (SUCCESS != status_info_report(sibuf,&sinfo))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
									memset(&tctd,0,sizeof(tctd));
									tctd.mode = 28;//traffic control
									tctd.pattern = *(fcdata->patternid);
									tctd.lampcolor = 0x02;
									tctd.lamptime = 0;
									tctd.phaseid = pinfo.phaseid;
									tctd.stageline = pinfo.stageline;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
								{
									memset(&tctd,0,sizeof(tctd));
									tctd.mode = 28;//traffic control
									tctd.pattern = *(fcdata->patternid);
									tctd.lampcolor = 0x02;
									tctd.lamptime = 0;
									tctd.phaseid = pinfo.phaseid;
									tctd.stageline = pinfo.stageline;	
									if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#endif
								fbdata[1] = 28;
								fbdata[2] = pinfo.stageline;
								fbdata[3] = 0x02;
								fbdata[4] = 0;
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
										if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																fcdata->softevent,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
								}
								else
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("face board serial port cannot write,Line:%d\n",__LINE__);
								#endif
								}							

								tcecho[1] = 0x02;
								if (*(fcdata->markbit2) & 0x1000)
								{
									*(fcdata->markbit2) &= 0xefff;
									if (!wait_write_serial(*(fcdata->sdserial)))
									{
										if (write(*(fcdata->sdserial),tcecho,sizeof(tcecho)) < 0)
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
									}
									else
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("face board serial port cannot write,Line:%d\n",__LINE__);
									#endif
									}	
								}//step by step comes from side door serial
								else
								{
									if (!wait_write_serial(*(fcdata->fbserial)))
									{
										if (write(*(fcdata->fbserial),tcecho,sizeof(tcecho)) < 0)
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
									}
									else
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
							if ((1 == tcred) || (1 == socyfyes) || (1 == close))
							{
								if (1 == close)
								{
									new_all_red(&ardata);
									close = 0;
								}
								tcred = 0;
                                if (1 == socyfyes)
                                {
                                    pthread_cancel(socyfpid);
                                    pthread_join(socyfpid,NULL);
                                    socyfyes = 0;
									new_all_red(&ardata);
                                }
		//						#ifdef CLOSE_LAMP
                                soc_set_lamp_color(*(fcdata->bbserial),clch,0x03);
								update_channel_status(fcdata->sockfd,chanstatus,clch,0x03,fcdata->markbit);
        //                        #endif
								if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                                {
                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                    *(fcdata->markbit) |= 0x0800;
                                }
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    					pinfo.chan,0x02,fcdata->markbit))
                                {
                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
								tcecho[1] = 0x02;
								if (*(fcdata->markbit2) & 0x1000)
								{
									*(fcdata->markbit2) &= 0xefff;
									if (!wait_write_serial(*(fcdata->sdserial)))
									{
										if (write(*(fcdata->sdserial),tcecho,sizeof(tcecho)) < 0)
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
									}
									else
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("face board serial port cannot write,Line:%d\n",__LINE__);
									#endif
									}	
								}//step by step comes from side door serial
								else
								{
									if (!wait_write_serial(*(fcdata->fbserial)))
									{
										if (write(*(fcdata->fbserial),tcecho,sizeof(tcecho)) < 0)
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
									}
									else
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("face board serial port cannot write,Line:%d\n",__LINE__);
									#endif
									}
								}
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									sinfo.conmode = 28;
									sinfo.color = 0x02;
									sinfo.time = 0;
									sinfo.stage = pinfo.stageline;
									sinfo.cyclet = 0;
									sinfo.phase = 0;
									sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
								/*						
									sinfo.chans = 0;
            						memset(sinfo.csta,0,sizeof(sinfo.csta));
            						csta = sinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						sinfo.chans += 1;
                						tcsta = i + 1;
                						tcsta <<= 2;
                						tcsta &= 0xfc;
										for (j = 0; j < MAX_CHANNEL; j++)
										{
											if (0 == pinfo.chan[j])
												break;
											if ((i+1) == pinfo.chan[j])
											{
												tcsta |= 0x02;
												break;
											}
										}
                						*csta = tcsta;
                						csta++;
            						}
								*/
									sinfo.chans = 0;
									memset(sinfo.csta,0,sizeof(sinfo.csta));
                                    csta = sinfo.csta;
									for (i = 0; i < MAX_CHANNEL; i++)
									{
										if (0 == pinfo.chan[i])
											break;
										sinfo.chans += 1;
										tcsta = pinfo.chan[i];
										tcsta <<= 2;
										tcsta &= 0xfc;
										tcsta |= 0x02; //00000010-green 
										*csta = tcsta;
										csta++;
									}
									memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&sinfo))
            						{
            						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
							if ((ct.tv_sec - wut.tv_sec) < pinfo.mingtime)
							{
								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
								{
									memset(&tctd,0,sizeof(tctd));
									tctd.mode = 28;//traffic control
									tctd.pattern = *(fcdata->patternid);
									tctd.lampcolor = 0x02;
									tctd.lamptime = pinfo.mingtime - (ct.tv_sec - wut.tv_sec);
									tctd.phaseid = pinfo.phaseid;
									tctd.stageline = pinfo.stageline;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
                            	fbdata[2] = pinfo.stageline;
                            	fbdata[3] = 0x02;
                            	fbdata[4] = pinfo.mingtime - (ct.tv_sec - wut.tv_sec);
                            	if (!wait_write_serial(*(fcdata->fbserial)))
                            	{
                                	if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                	{
                                	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                	}
                            	}
                            	else
                            	{
                            	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                	printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            	#endif
                            	}
								sleep(pinfo.mingtime - (ct.tv_sec - wut.tv_sec));
							} 
							//for WuXi Check
#endif
							if ((0==pinfo.cchan[0])&&(pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0))
							{
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									sinfo.conmode = 28;
									sinfo.color = 0x02;
									sinfo.time = pinfo.gftime + pinfo.ytime + pinfo.rtime;
									sinfo.stage = pinfo.stageline;
									sinfo.cyclet = 0;
									sinfo.phase = 0;
									sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
															
									sinfo.chans = 0;
            						memset(sinfo.csta,0,sizeof(sinfo.csta));
            						csta = sinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
										if (0 == pinfo.chan[i])
											break;
                						sinfo.chans += 1;
                						tcsta = pinfo.chan[i];
                						tcsta <<= 2;
                						tcsta &= 0xfc;
										tcsta |= 0x02;
                						*csta = tcsta;
                						csta++;
            						}
									memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&sinfo))
            						{
            						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively
							}//if ((0==pinfo.cchan[0])&&(pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0))

							if ((0 != pinfo.cchan[0]) && (pinfo.gftime > 0))
							{
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									sinfo.conmode = 28;
									sinfo.color = 0x03;
									sinfo.time = pinfo.gftime;
									sinfo.stage = pinfo.stageline;
									sinfo.cyclet = 0;
									sinfo.phase = 0;
									sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
									
									for (i = 0; i < MAX_CHANNEL; i++)
                                	{
                                		if (0 == pinfo.cchan[i])
                                        	break;
                                    	for (j = 0; j < sinfo.chans; j++)
                                    	{
                                        	if (0 == sinfo.csta[j])
                                            	break;
                                        	tcsta = sinfo.csta[j];
                                        	tcsta &= 0xfc;
                                        	tcsta >>= 2;
                                        	tcsta &= 0x3f;
                                        	if (pinfo.cchan[i] == tcsta)
                                        	{
                                            	sinfo.csta[j] &= 0xfc;
                                           		sinfo.csta[j] |= 0x03; //00000001-green flash
												break;
                                        	}
                                    	}
                                	}								
									memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));	
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&sinfo))
            						{
            						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively
							}//if ((0 != pinfo.cchan[0]) && (pinfo.gftime > 0))

							//current phase begin to green flash
							if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
							{
								memset(&tctd,0,sizeof(tctd));
								tctd.mode = 28;//traffic control
								tctd.pattern = *(fcdata->patternid);
								tctd.lampcolor = 0x02;
								tctd.lamptime = pinfo.gftime;
								tctd.phaseid = pinfo.phaseid;
								tctd.stageline = pinfo.stageline;
								if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#ifdef EMBED_CONFIGURE_TOOL
							if (*(fcdata->markbit2) & 0x0200)
							{
								memset(&tctd,0,sizeof(tctd));
                                tctd.mode = 28;//traffic control
                                tctd.pattern = *(fcdata->patternid);
                                tctd.lampcolor = 0x02;
                                tctd.lamptime = pinfo.gftime;
                                tctd.phaseid = pinfo.phaseid;
                                tctd.stageline = pinfo.stageline;
								if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#endif
							fbdata[1] = 28;
                            fbdata[2] = pinfo.stageline;
                            fbdata[3] = 0x02;
                            fbdata[4] = pinfo.gftime;
                            if (!wait_write_serial(*(fcdata->fbserial)))
                            {
                                if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                {
                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									*(fcdata->markbit) |= 0x0800;
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                            }
                            else
                            {
                            #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            #endif
                            }
							if (pinfo.gftime > 0)
							{	
								ngf = 0;
								while (1)
								{
									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x03))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
										pinfo.cchan,0x03,fcdata->markbit))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
									to.tv_sec = 0;
									to.tv_usec = 500000;
									select(0,NULL,NULL,NULL,&to);

									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x02))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
										pinfo.cchan,0x02,fcdata->markbit))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
									to.tv_sec = 0;
									to.tv_usec = 500000;
									select(0,NULL,NULL,NULL,&to);
									
									ngf += 1;
									if (ngf >= pinfo.gftime)
										break;
								}
							}//if (pinfo.gftime > 0)
							if (1 == (pinfo.cpcexist))
							{
								//current phase begin to yellow lamp
								if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.cnpchan,0x01))
        						{
        						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            						printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        						#endif
									*(fcdata->markbit) |= 0x0800;
        						}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
													pinfo.cnpchan,0x01, fcdata->markbit))
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
								if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.cpchan,0x00))
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									*(fcdata->markbit) |= 0x0800;
								}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																	pinfo.cpchan,0x00,fcdata->markbit))
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
							}
							else
							{
								if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x01))
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									*(fcdata->markbit) |= 0x0800;
								}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														pinfo.cchan,0x01,fcdata->markbit))
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
							}

							if ((0 != pinfo.cchan[0]) && (pinfo.ytime > 0))
							{
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									sinfo.conmode = 28;
									sinfo.color = 0x01;
									sinfo.time = pinfo.ytime;
									sinfo.stage = pinfo.stageline;
									sinfo.cyclet = 0;
									sinfo.phase = 0;
									sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
															
									for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						if (0 == pinfo.cnpchan[i])
                    						break;
                						for (j = 0; j < sinfo.chans; j++)
                						{
                    						if (0 == sinfo.csta[j])
                        						break;
                    						tcsta = sinfo.csta[j];
                    						tcsta &= 0xfc;
                    						tcsta >>= 2;
                    						tcsta &= 0x3f;
                    						if (pinfo.cnpchan[i] == tcsta)
                    						{
                        						sinfo.csta[j] &= 0xfc;
												sinfo.csta[j] |= 0x01; //00000001-yellow
												break;
                    						}
                						}
            						}
									for (i = 0; i < MAX_CHANNEL; i++)
                                	{
                                		if (0 == pinfo.cpchan[i])
                                        	break;
                                    	for (j = 0; j < sinfo.chans; j++)
                                    	{
                                        	if (0 == sinfo.csta[j])
                                            	break;
                                        	tcsta = sinfo.csta[j];
                                        	tcsta &= 0xfc;
                                        	tcsta >>= 2;
                                        	tcsta &= 0x3f;
                                        	if (pinfo.cpchan[i] == tcsta)
                                        	{
                                            	sinfo.csta[j] &= 0xfc;
												break;
                                        	}
                                    	}
                                	}
									memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&sinfo))
            						{
            						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively
							}//if ((0 != pinfo.cchan[0]) && (pinfo.ytime > 0))

							if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            {
                               	memset(&tctd,0,sizeof(tctd));
                               	tctd.mode = 28;//traffic control
                               	tctd.pattern = *(fcdata->patternid);
                               	tctd.lampcolor = 0x01;
                               	tctd.lamptime = pinfo.ytime;
                               	tctd.phaseid = pinfo.phaseid;
                               	tctd.stageline = pinfo.stageline;
                               	if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
                               	{
                               	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   	printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               	#endif
                               	}
                            }
							#ifdef EMBED_CONFIGURE_TOOL
							if (*(fcdata->markbit2) & 0x0200)
							{
								memset(&tctd,0,sizeof(tctd));
                                tctd.mode = 28;//traffic control
                                tctd.pattern = *(fcdata->patternid);
                                tctd.lampcolor = 0x01;
                                tctd.lamptime = pinfo.ytime;
                                tctd.phaseid = pinfo.phaseid;
                                tctd.stageline = pinfo.stageline;
								if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#endif
							fbdata[1] = 28;
                            fbdata[2] = pinfo.stageline;
                            fbdata[3] = 0x01;
                            fbdata[4] = pinfo.ytime;
                            if (!wait_write_serial(*(fcdata->fbserial)))
                            {
                               	if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               	{
                               	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               	#endif
									*(fcdata->markbit) |= 0x0800;
									gettimeofday(&ct,NULL);
                                   	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                   	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   	{
                                   	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                       	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   	#endif
                                   	}
                               	}
                            }
                            else
                            {
                            #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               	printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            #endif
                            }
							sleep(pinfo.ytime);

							//current phase begin to red lamp
							if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x00))
                            {
                            #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
								*(fcdata->markbit) |= 0x0800;
                            }
							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus,pinfo.cchan,0x00, \
								fcdata->markbit))
							{
							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
								printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}

							if ((0 != pinfo.cchan[0]) && (pinfo.rtime > 0))
							{
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									sinfo.conmode = 28;
									sinfo.color = 0x00;
									sinfo.time = pinfo.rtime;
									sinfo.stage = pinfo.stageline;
									sinfo.cyclet = 0;
									sinfo.phase = 0;
									sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
															
									for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						if (0 == pinfo.cchan[i])
                    						break;
                						for (j = 0; j < sinfo.chans; j++)
                						{
                    						if (0 == sinfo.csta[j])
                        						break;
                    						tcsta = sinfo.csta[j];
                    						tcsta &= 0xfc;
                    						tcsta >>= 2;
                    						tcsta &= 0x3f;
                    						if (pinfo.cchan[i] == tcsta)
                    						{
                        						sinfo.csta[j] &= 0xfc;
												break;
                    						}
                						}
            						}
									memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));	
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&sinfo))
            						{
            						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively
							}//if ((0 != pinfo.cchan[0]) && (pinfo.rtime > 0))

							if (pinfo.rtime > 0)
							{
								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            	{
                                	memset(&tctd,0,sizeof(tctd));
                                	tctd.mode = 28;//traffic control
                                	tctd.pattern = *(fcdata->patternid);
                                	tctd.lampcolor = 0x00;
                                	tctd.lamptime = pinfo.rtime;
                                	tctd.phaseid = pinfo.phaseid;
                                	tctd.stageline = pinfo.stageline;
                                	if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
                                	{
                                	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    	printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                	#endif
                                	}
                            	}
								#ifdef EMBED_CONFIGURE_TOOL
                                if (*(fcdata->markbit2) & 0x0200)
                                {
									memset(&tctd,0,sizeof(tctd));
                                    tctd.mode = 28;//traffic control
                                    tctd.pattern = *(fcdata->patternid);
                                    tctd.lampcolor = 0x00;
                                    tctd.lamptime = pinfo.rtime;
                                    tctd.phaseid = pinfo.phaseid;
                                    tctd.stageline = pinfo.stageline;
                                    if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
                                    {
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                                #endif
								fbdata[1] = 28;
                                fbdata[2] = pinfo.stageline;
                                fbdata[3] = 0x00;
                                fbdata[4] = pinfo.rtime;
                                if (!wait_write_serial(*(fcdata->fbserial)))
                                {
                                    if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                    {
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
                                        update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                        if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                        {
                                        #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                            printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                }
                                else
                                {
                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                #endif
                                }
								sleep(pinfo.rtime);
							}

							*(fcdata->slnum) += 1;
							*(fcdata->stageid) = \
								tscdata->timeconfig->TimeConfigList[socrettl][*(fcdata->slnum)].StageId;
							if (0 == (tscdata->timeconfig->TimeConfigList[socrettl][*(fcdata->slnum)].StageId))
							{
								*(fcdata->slnum) = 0;
								*(fcdata->stageid) = \
									tscdata->timeconfig->TimeConfigList[socrettl][*(fcdata->slnum)].StageId;
							}
							//get phase info of next phase
							memset(&pinfo,0,sizeof(socpinfo_t));
        					if (SUCCESS != soc_get_phase_info(fcdata,tscdata,socrettl,*(fcdata->slnum),&pinfo))
        					{
        					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            					printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
        					#endif
								soc_end_part_child_thread();
            					return FAIL;
        					}
        					*(fcdata->phaseid) = 0;
        					*(fcdata->phaseid) |= (0x01 << (pinfo.phaseid - 1));

							if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                            {
                            #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
								*(fcdata->markbit) |= 0x0800;
                            }
						
							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus,pinfo.chan,0x02, \
                                fcdata->markbit))
                            {
                            #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }

							if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
							{//report info to server actively
								sinfo.conmode = 28;
								sinfo.color = 0x02;
								sinfo.time = 0;
								sinfo.stage = pinfo.stageline;
								sinfo.cyclet = 0;
								sinfo.phase = 0;
								sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
															
								sinfo.chans = 0;
            					memset(sinfo.csta,0,sizeof(sinfo.csta));
            					csta = sinfo.csta;
            					for (i = 0; i < MAX_CHANNEL; i++)
            					{
									if (0 == pinfo.chan[i])
										break;
                					sinfo.chans += 1;
                					tcsta = pinfo.chan[i];
                					tcsta <<= 2;
                					tcsta &= 0xfc;
									tcsta |= 0x02;
                					*csta = tcsta;
                					csta++;
            					}
								memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            					memset(sibuf,0,sizeof(sibuf));
            					if (SUCCESS != status_info_report(sibuf,&sinfo))
            					{
            					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
								memset(&tctd,0,sizeof(tctd));
								tctd.mode = 28;//traffic control
								tctd.pattern = *(fcdata->patternid);
								tctd.lampcolor = 0x02;
								tctd.lamptime = 0;
								tctd.phaseid = pinfo.phaseid;
								tctd.stageline = pinfo.stageline;
								if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#ifdef EMBED_CONFIGURE_TOOL
							if (*(fcdata->markbit2) & 0x0200)
							{
								memset(&tctd,0,sizeof(tctd));
                                tctd.mode = 28;//traffic control
                                tctd.pattern = *(fcdata->patternid);
                                tctd.lampcolor = 0x02;
                                tctd.lamptime = 0;
                                tctd.phaseid = pinfo.phaseid;
                                tctd.stageline = pinfo.stageline;
								if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#endif
							fbdata[1] = 28;
                            fbdata[2] = pinfo.stageline;
                            fbdata[3] = 0x02;
                            fbdata[4] = 0;
                            if (!wait_write_serial(*(fcdata->fbserial)))
                            {
                                if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                {
                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									*(fcdata->markbit) |= 0x0800;
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                            }
                            else
                            {
                            #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            #endif
                            }							

							tcecho[1] = 0x02;
							if (*(fcdata->markbit2) & 0x1000)
							{
								*(fcdata->markbit2) &= 0xefff;
								if (!wait_write_serial(*(fcdata->sdserial)))
                                {
                                    if (write(*(fcdata->sdserial),tcecho,sizeof(tcecho)) < 0)
                                    {
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                        *(fcdata->markbit) |= 0x0800;
                                    }
                                }
                                else
                                {
                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                #endif
                                }	
							}//step by step comes from side door serial
							else
							{
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),tcecho,sizeof(tcecho)) < 0)
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
								}
								else
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
						}//step by step
						if ((0x03 == tcbuf[1]) && (1 == keylock))
						{//yellow flash
							kstep = 0;
                            cktem = 0;
							tcred = 0;
							close = 0;
							if (0 == socyfyes)
							{
							//	int		*bbserial = fcdata->bbserial;
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
								int yfret = pthread_create(&socyfpid,NULL,(void *)soc_yellow_flash,&yfdata);
								if (0 != yfret)
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									soc_end_part_child_thread();
									return FAIL;
								}
								socyfyes = 1;
							}
							tcecho[1] = 0x03;
							if (*(fcdata->markbit2) & 0x2000)
							{
								*(fcdata->markbit2) &= 0xdfff;
								if (!wait_write_serial(*(fcdata->sdserial)))
                                {
                                    if (write(*(fcdata->sdserial),tcecho,sizeof(tcecho)) < 0)
                                    {
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                        *(fcdata->markbit) |= 0x0800;
                                    }
                                }
                                else
                                {
                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                #endif
                                }
							}//yellow flash comes from side door serial;
							else
							{
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),tcecho,sizeof(tcecho)) < 0)
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
								}
								else
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("face board serial port cannot write,Line:%d\n",__LINE__);
								#endif
								}
							}

							if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
							{//report info to server actively
								sinfo.conmode = 28;
								sinfo.color = 0x05;
								sinfo.time = 0;
								sinfo.stage = 0;
								sinfo.cyclet = 0;
								sinfo.phase = 0;
															
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
								memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            					memset(sibuf,0,sizeof(sibuf));
            					if (SUCCESS != status_info_report(sibuf,&sinfo))
            					{
            					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
							if (1 == socyfyes)
							{
								pthread_cancel(socyfpid);
								pthread_join(socyfpid,NULL);
								socyfyes = 0;
							}
							close = 0;	
							if (0 == tcred)
							{	
								new_all_red(&ardata);
						//		all_red(*(fcdata->bbserial),0);
								tcred = 1;
							}						
							tcecho[1] = 0x04;
							if (*(fcdata->markbit2) & 0x4000)
                            {
                                *(fcdata->markbit2) &= 0xbfff;
                                if (!wait_write_serial(*(fcdata->sdserial)))
                                {
                                    if (write(*(fcdata->sdserial),tcecho,sizeof(tcecho)) < 0)
                                    {
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                        *(fcdata->markbit) |= 0x0800;
                                    }
                                }
                                else
                                {
                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                #endif
                                }
                            }//all red comes from side door serial
							else
							{
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),tcecho,sizeof(tcecho)) < 0)
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
								}
								else
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("face board serial port cannot write,Line:%d\n",__LINE__);
								#endif
								}
							}

							if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
							{//report info to server actively
								sinfo.conmode = 28;
								sinfo.color = 0x00;
								sinfo.time = 0;
								sinfo.stage = 0;
								sinfo.cyclet = 0;
								sinfo.phase = 0;
															
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
								memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            					memset(sibuf,0,sizeof(sibuf));
            					if (SUCCESS != status_info_report(sibuf,&sinfo))
            					{
            					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
							if (1 == socyfyes)
							{
								pthread_cancel(socyfpid);
								pthread_join(socyfpid,NULL);
								socyfyes = 0;
							}
							tcred = 0;	
							if (0 == close)
							{	
								new_all_close(&acdata);
								close = 1;
							}						
							tcecho[1] = 0x05;
							if (!wait_write_serial(*(fcdata->fbserial)))
                            {
                                if (write(*(fcdata->fbserial),tcecho,sizeof(tcecho)) < 0)
                                {
                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									*(fcdata->markbit) |= 0x0800;
                                }
                            }
                            else
                            {
                            #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            #endif
                            }

							if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
							{//report info to server actively
								sinfo.conmode = 28;
								sinfo.color = 0x04;
								sinfo.time = 0;
								sinfo.stage = 0;
								sinfo.cyclet = 0;
								sinfo.phase = 0;
															
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
								memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            					memset(sibuf,0,sizeof(sibuf));
            					if (SUCCESS != status_info_report(sibuf,&sinfo))
            					{
            					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
						}//all red
					}//Key control is valid only when wireless terminal control is not valid;
				}//if ((0xDA == tcbuf[0]) && (0xED == tcbuf[2]))
				else if (!strncmp("WLTC1",tcbuf,5))
				{//data from wireless terminal
					if ((0 == keylock) && (0 == netlock) && (0 == wtlock))
					{//wireless control is valid only when key control is not valid
						unsigned char			wlbuf[5] = {0};
						unsigned char			s = 0;
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
						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
						printf("************wlbuf: %s,File: %s,Line: %d\n",wlbuf,__FILE__,__LINE__);	
						#endif	
						if (!strcmp("SOCK",wlbuf))
						{//lock or unlock
							if (0 == wllock)
							{//lock will happen
								get_soc_status(&color,&leatime);
								if (0x02 != color)
								{//if (0x02 != color)
									struct timeval          spacetime;
									unsigned char			endlock = 0;
									while (1)
									{//while loop,000000
										spacetime.tv_sec = 0;
										spacetime.tv_usec = 200000;
										select(0,NULL,NULL,NULL,&spacetime);

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
										get_soc_status(&color,&leatime);
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
								
								wllock = 1;
                                tcred = 0;
								close = 0;
								dircon = 0;
                                firdc = 1;
                                fdirn = 0;
                                cdirn = 0;
								soc_end_child_thread();//end main thread and its child thread
								*(fcdata->markbit2) |= 0x0010;
						
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									sinfo.conmode = 28;
									sinfo.color = 0x02;
									sinfo.time = 0;
									sinfo.stage = pinfo.stageline;
									sinfo.cyclet = 0;
									sinfo.phase = 0;
									sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
															
									sinfo.chans = 0;
            						memset(sinfo.csta,0,sizeof(sinfo.csta));
            						csta = sinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						if (0 == pinfo.chan[i])
                    						break;
                						sinfo.chans += 1;
                						tcsta = pinfo.chan[i];
                						tcsta <<= 2;
                						tcsta &= 0xfc;
                						tcsta |= 0x02; //00000010-green
                						*csta = tcsta;
                						csta++;
            						}
									memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&sinfo))
            						{
            						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                						printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively

								*(fcdata->contmode) = 28;//traffic control mode
								fbdata[1] = 28;
								fbdata[2] = pinfo.stageline;
								fbdata[3] = 0x02;
								fbdata[4] = 0;
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
                						update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
									}
								}
								else
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("face board serial port cannot write,Line:%d\n",__LINE__);
								#endif
								}
								sendfaceInfoToBoard(fcdata,fbdata);
						//		#ifdef TIMING_DOWN_TIME
								if (*(fcdata->auxfunc) & 0x01)
								{//if (*(fcdata->auxfunc) & 0x01)
									if (!wait_write_serial(*(fcdata->bbserial)))
									{
										if (write(*(fcdata->bbserial),downti,sizeof(downti)) < 0)
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
									if (!wait_write_serial(*(fcdata->bbserial)))
									{
										if (write(*(fcdata->bbserial),edownti,sizeof(edownti)) < 0)
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG 
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}	
								}//if (*(fcdata->auxfunc) & 0x01)
						//		#endif
	
                                if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    pinfo.chan,0x02,fcdata->markbit))
                                {
                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                                {
                                    memset(wltc,0,sizeof(wltc));
                                    strcpy(wltc,"SOCKBEGIN");
                                    write(*(fcdata->sockfd->ssockfd),wltc,sizeof(wltc));

									//send down time to configure tool
                                    memset(&tctd,0,sizeof(tctd));
                                    tctd.mode = 28;
                                    tctd.pattern = *(fcdata->patternid);
                                    tctd.lampcolor = 0x02;
                                    tctd.lamptime = 0;
                                    tctd.phaseid = pinfo.phaseid;
                                    tctd.stageline = pinfo.stageline;
                                    if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
                                    {
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
								#ifdef EMBED_CONFIGURE_TOOL
                                if (*(fcdata->markbit2) & 0x0200)
                                {
									memset(&tctd,0,sizeof(tctd));
                                    tctd.mode = 28;
                                    tctd.pattern = *(fcdata->patternid);
                                    tctd.lampcolor = 0x02;
                                    tctd.lamptime = 0;
                                    tctd.phaseid = pinfo.phaseid;
                                    tctd.stageline = pinfo.stageline;
                                    if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
                                    {
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                                #endif							
								gettimeofday(&ct,NULL);
                                update_event_list(fcdata->ec,fcdata->el,1,90,ct.tv_sec,fcdata->markbit);
	
								continue;
							}//lock will happen
							if (1 == wllock)
							{//unlock will happen
								wllock = 0;
								tcred = 0;
								close = 0;
								dircon = 0;
                                firdc = 1;
                                fdirn = 0;
                                cdirn = 0;
								if (1 == socyfyes)
								{
									pthread_cancel(socyfpid);
									pthread_join(socyfpid,NULL);
									socyfyes = 0;
								}
								
								new_all_red(&ardata);	
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									sinfo.conmode = 28;
									sinfo.color = 0x00;
									sinfo.time = 0;
									sinfo.stage = 0;
									sinfo.cyclet = 0;
									sinfo.phase = 0;
															
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
									memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&sinfo))
            						{
            						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                						printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively

								*(fcdata->markbit2) &= 0xffef;
							//	all_red(*(fcdata->bbserial),0);
								*(fcdata->contmode) = contmode; //restore control mode;
								if (0 == socyes)
								{
									memset(&tcdata,0,sizeof(tcdata));
									memset(&pinfo,0,sizeof(pinfo));
									tcdata.fd = fcdata;
									tcdata.td = tscdata;
									tcdata.cs = chanstatus;
									tcdata.pi = &pinfo;
									int tcret=pthread_create(&socpid,NULL, \
										(void *)start_optimize_coordinate_control,&tcdata);
									if (0 != tcret)
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("create thread err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
										soc_end_part_child_thread();
										return FAIL;
									}
									socyes = 1;
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
							}//unlock will happen
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

							if ((1 == tcred) || (1 == socyfyes) || (1 == close))
							{
								if (1 == close)
								{
									close = 0;
									new_all_red(&ardata);
								}
								tcred = 0;
                                if (1 == socyfyes)
                                {
                                    pthread_cancel(socyfpid);
                                    pthread_join(socyfpid,NULL);
                                    socyfyes = 0;
									new_all_red(&ardata);
                                }
			//					#ifdef CLOSE_LAMP
                                soc_set_lamp_color(*(fcdata->bbserial),clch,0x03);
								update_channel_status(fcdata->sockfd,chanstatus,clch,0x03,fcdata->markbit);
            //                    #endif
								if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                                {
                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                    *(fcdata->markbit) |= 0x0800;
                                }
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    					pinfo.chan,0x02,fcdata->markbit))
                                {
                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }

								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									sinfo.conmode = 28;
									sinfo.color = 0x02;
									sinfo.time = 0;
									sinfo.stage = pinfo.stageline;
									sinfo.cyclet = 0;
									sinfo.phase = 0;
									sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
								/*							
									sinfo.chans = 0;
            						memset(sinfo.csta,0,sizeof(sinfo.csta));
            						csta = sinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						sinfo.chans += 1;
                						tcsta = i + 1;
                						tcsta <<= 2;
                						tcsta &= 0xfc;
										for (j = 0; j < MAX_CHANNEL; j++)
										{
											if (0 == pinfo.chan[j])
												break;
											if ((i+1) == pinfo.chan[j])
											{
												tcsta |= 0x02;
												break;
											}
										}
                						*csta = tcsta;
                						csta++;
            						}
								*/
									sinfo.chans = 0;
									memset(sinfo.csta,0,sizeof(sinfo.csta));
                                    csta = sinfo.csta;
									for (i = 0; i < MAX_CHANNEL; i++)
									{
										if (0 == pinfo.chan[i])
											break;
										sinfo.chans += 1;
										tcsta = pinfo.chan[i];
										tcsta <<= 2;
										tcsta &= 0xfc;
										tcsta |= 0x02; //00000010-green 
										*csta = tcsta;
										csta++;
									}

									memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&sinfo))
            						{
            						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
										if (0 == pinfo.chan[j])
											break;
										if (dirch[fdirn-1][i] == pinfo.chan[j])
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
										sinfo.conmode = 28;
										sinfo.color = 0x02;
										sinfo.time = 6;
										sinfo.stage = 0;
										sinfo.cyclet = 0;
										sinfo.phase = 0;
																
										sinfo.chans = 0;
										memset(sinfo.csta,0,sizeof(sinfo.csta));
										csta = sinfo.csta;
										for (i = 0; i < MAX_CHANNEL; i++)
										{
											if (0 == dcchan[i])
												break;
											sinfo.chans += 1;
											tcsta = dcchan[i];
											tcsta <<= 2;
											tcsta &= 0xfc;
											tcsta |= 0x02;
											*csta = tcsta;
											csta++;
										}
										memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
										memset(sibuf,0,sizeof(sibuf));
										if (SUCCESS != status_info_report(sibuf,&sinfo))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										else
										{
											write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
										}
									}//report info to server actively
								}//if ((0==cchan[0])&&(pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0))

								if (0 != dcchan[0])
								{
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										sinfo.conmode = 28;
										sinfo.color = 0x03;
										sinfo.time = 3;
										sinfo.stage = 0;
										sinfo.cyclet = 0;
										sinfo.phase = 0;
										
										for (i = 0; i < MAX_CHANNEL; i++)
										{
											if (0 == dcchan[i])
												break;
											for (j = 0; j < sinfo.chans; j++)
											{
												if (0 == sinfo.csta[j])
													break;
												tcsta = sinfo.csta[j];
												tcsta &= 0xfc;
												tcsta >>= 2;
												tcsta &= 0x3f;
												if (dcchan[i] == tcsta)
												{
													sinfo.csta[j] &= 0xfc;
													sinfo.csta[j] |= 0x03; //00000001-green flash
													break;
												}
											}
										}								
										memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));	
										memset(sibuf,0,sizeof(sibuf));
										if (SUCCESS != status_info_report(sibuf,&sinfo))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										else
										{
											write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
										}
									}//report info to server actively
								}//if ((0 != cchan[0]) && (pinfo.gftime > 0))

								//current phase begin to green flash
								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
								{
									memset(&tctd,0,sizeof(tctd));
									tctd.mode = 28;//traffic control
									tctd.pattern = *(fcdata->patternid);
									tctd.lampcolor = 0x02;
									tctd.lamptime = 3;
									tctd.phaseid = 0;
									tctd.stageline = 0;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
								{
									memset(&tctd,0,sizeof(tctd));
									tctd.mode = 28;//traffic control
									tctd.pattern = *(fcdata->patternid);
									tctd.lampcolor = 0x02;
									tctd.lamptime = 3;
									tctd.phaseid = 0;
									tctd.stageline = 0;	
									if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
								}
								else
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("face board serial port cannot write,Line:%d\n",__LINE__);
								#endif
								}
								if (pinfo.gftime > 0)
								{
									ngf = 0;
									while (1)
									{//green flash
										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),dcchan,0x03))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;	
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														dcchan,0x03,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);

										if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),dcchan,0x02))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														dcchan,0x02,fcdata->markbit))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
								}//if (pinfo.gftime > 0)
								if (1 == dpexist)
								{
									//current phase begin to yellow lamp
									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),dcnpchan,0x01))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														dcnpchan,0x01, fcdata->markbit))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),dcpchan,0x00))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																		dcpchan,0x00,fcdata->markbit))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
								}
								else
								{
									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),dcchan,0x01))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															dcchan,0x01,fcdata->markbit))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
								}
								
								if (0 != dcchan[0])
								{
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										sinfo.conmode = 28;
										sinfo.color = 0x01;
										sinfo.time = 3;
										sinfo.stage = 0;
										sinfo.cyclet = 0;
										sinfo.phase = 0;
																
										for (i = 0; i < MAX_CHANNEL; i++)
										{
											if (0 == dcnpchan[i])
												break;
											for (j = 0; j < sinfo.chans; j++)
											{
												if (0 == sinfo.csta[j])
													break;
												tcsta = sinfo.csta[j];
												tcsta &= 0xfc;
												tcsta >>= 2;
												tcsta &= 0x3f;
												if (dcnpchan[i] == tcsta)
												{
													sinfo.csta[j] &= 0xfc;
													sinfo.csta[j] |= 0x01; //00000001-yellow
													break;
												}
											}
										}
										for (i = 0; i < MAX_CHANNEL; i++)
										{
											if (0 == dcpchan[i])
												break;
											for (j = 0; j < sinfo.chans; j++)
											{
												if (0 == sinfo.csta[j])
													break;
												tcsta = sinfo.csta[j];
												tcsta &= 0xfc;
												tcsta >>= 2;
												tcsta &= 0x3f;
												if (dcpchan[i] == tcsta)
												{
													sinfo.csta[j] &= 0xfc;
													break;
												}
											}
										}
										memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
										memset(sibuf,0,sizeof(sibuf));
										if (SUCCESS != status_info_report(sibuf,&sinfo))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										else
										{
											write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
										}
									}//report info to server actively
								}//if ((0 != cchan[0]) && (pinfo.ytime > 0))

								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
								{
									memset(&tctd,0,sizeof(tctd));
									tctd.mode = 28;//traffic control
									tctd.pattern = *(fcdata->patternid);
									tctd.lampcolor = 0x01;
									tctd.lamptime = 3;
									tctd.phaseid = 0;
									tctd.stageline = 0;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
								{
									memset(&tctd,0,sizeof(tctd));
									tctd.mode = 28;//traffic control
									tctd.pattern = *(fcdata->patternid);
									tctd.lampcolor = 0x01;
									tctd.lamptime = 3;
									tctd.phaseid = 0;
									tctd.stageline = 0;	
									if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
								}
								else
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("face board serial port cannot write,Line:%d\n",__LINE__);
								#endif
								}

								sleep(3);

								if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),dcchan,0x00))
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									*(fcdata->markbit) |= 0x0800;
								}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														dcchan,0x00,fcdata->markbit))
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
								
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															pinfo.chan,0x02,fcdata->markbit))
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
								if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									*(fcdata->markbit) |= 0x0800;
									return FAIL;
								}
								else
								{//set success
									if ((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										sinfo.conmode = 28;
										sinfo.color = 0x02;
										sinfo.time = 0;
										sinfo.stage = pinfo.stageline;
										sinfo.cyclet = 0;
										sinfo.phase = 0;
                                    	sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
										sinfo.chans = 0;
										memset(sinfo.csta,0,sizeof(sinfo.csta));
										csta = sinfo.csta;
										for (i = 0; i < MAX_CHANNEL; i++)
										{
											if (0 == pinfo.chan[i])
												break;
											sinfo.chans += 1;
											tcsta = pinfo.chan[i];
											tcsta <<= 2;
											tcsta &= 0xfc;
											tcsta |= 0x02; //00000010-green 
											*csta = tcsta;
											csta++;
										}
										memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
										memset(sibuf,0,sizeof(sibuf));
										if (SUCCESS != status_info_report(sibuf,&sinfo))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
										memset(&tctd,0,sizeof(tctd));
										tctd.mode = 28;//traffic control
										tctd.pattern = *(fcdata->patternid);
										tctd.lampcolor = 0x02;
										tctd.lamptime = 0;
										tctd.phaseid = pinfo.phaseid;
                                		tctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&tctd,0,sizeof(tctd));
										tctd.mode = 28;//traffic control
										tctd.pattern = *(fcdata->patternid);
										tctd.lampcolor = 0x02;
										tctd.lamptime = 0;
										tctd.phaseid = pinfo.phaseid;
                                		tctd.stageline = pinfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
									fbdata[2] = pinfo.stageline;
									fbdata[3] = 0x02;
									fbdata[4] = 0;
									if (!wait_write_serial(*(fcdata->fbserial)))
									{
										if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
										{
										#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
									}
									else
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("face board serial port cannot write,Line:%d\n",__LINE__);
									#endif
									}							
								}//set success
                                dircon = 0;
								fdirn = 0;
                                continue;
                            }//diredtion control happen

							if ((0==pinfo.cchan[0])&&(pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0))
							{
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									sinfo.conmode = 28;
									sinfo.color = 0x02;
									sinfo.time = pinfo.gftime + pinfo.ytime + pinfo.rtime;
									sinfo.stage = pinfo.stageline;
									sinfo.cyclet = 0;
									sinfo.phase = 0;
									sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
															
									sinfo.chans = 0;
            						memset(sinfo.csta,0,sizeof(sinfo.csta));
            						csta = sinfo.csta;
            						for (i = 0; i < MAX_CHANNEL; i++)
            						{
										if (0 == pinfo.chan[i])
											break;
                						sinfo.chans += 1;
                						tcsta = pinfo.chan[i];
                						tcsta <<= 2;
                						tcsta &= 0xfc;
										tcsta |= 0x02;
                						*csta = tcsta;
                						csta++;
            						}
									memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&sinfo))
            						{
            						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively
							}//if ((0==pinfo.cchan[0])&&(pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0))

							if ((0 != pinfo.cchan[0]) && (pinfo.gftime > 0))
							{
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									sinfo.conmode = 28;
									sinfo.color = 0x03;
									sinfo.time = pinfo.gftime;
									sinfo.stage = pinfo.stageline;
									sinfo.cyclet = 0;
									sinfo.phase = 0;
									sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
									
									for (i = 0; i < MAX_CHANNEL; i++)
                                	{
                                		if (0 == pinfo.cchan[i])
                                        	break;
                                    	for (j = 0; j < sinfo.chans; j++)
                                    	{
                                        	if (0 == sinfo.csta[j])
                                            	break;
                                        	tcsta = sinfo.csta[j];
                                        	tcsta &= 0xfc;
                                        	tcsta >>= 2;
                                        	tcsta &= 0x3f;
                                        	if (pinfo.cchan[i] == tcsta)
                                        	{
                                            	sinfo.csta[j] &= 0xfc;
                                            	sinfo.csta[j] |= 0x03; //00000001-green flash
												break;
                                        	}
                                    	}
                                	}								
									memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));	
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&sinfo))
            						{
            						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively
							}//if ((0 != pinfo.cchan[0]) && (pinfo.gftime > 0))

							//current phase begin to green flash
							fbdata[1] = 28;
                            fbdata[2] = pinfo.stageline;
                            fbdata[3] = 0x02;
                            fbdata[4] = pinfo.gftime;
                            if (!wait_write_serial(*(fcdata->fbserial)))
                            {
                                if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                {
                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									*(fcdata->markbit) |= 0x0800;
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                }
                            }
                            else
                            {
                            #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            #endif
                            }

							if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
							{
								//send down time to configure tool
								memset(&tctd,0,sizeof(tctd));
								tctd.mode = 28;
								tctd.pattern = *(fcdata->patternid);
								tctd.lampcolor = 0x02;
								tctd.lamptime = pinfo.gftime;
								tctd.phaseid = pinfo.phaseid;
								tctd.stageline = pinfo.stageline;
								if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#ifdef EMBED_CONFIGURE_TOOL
							if (*(fcdata->markbit2) & 0x0200)
							{
								memset(&tctd,0,sizeof(tctd));
                                tctd.mode = 28;
                                tctd.pattern = *(fcdata->patternid);
                                tctd.lampcolor = 0x02;
                                tctd.lamptime = pinfo.gftime;
                                tctd.phaseid = pinfo.phaseid;
                                tctd.stageline = pinfo.stageline;
								if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#endif
							if (pinfo.gftime > 0)
							{
								ngf = 0;
								while (1)
								{
									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x03))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
										pinfo.cchan,0x03,fcdata->markbit))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
									to.tv_sec = 0;
									to.tv_usec = 500000;
									select(0,NULL,NULL,NULL,&to);

									if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x02))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
										pinfo.cchan,0x02,fcdata->markbit))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
									to.tv_sec = 0;
									to.tv_usec = 500000;
									select(0,NULL,NULL,NULL,&to);
									
									ngf += 1;
									if (ngf >= pinfo.gftime)
										break;
								}
							}//if (pinfo.gftime > 0)
							if (1 == (pinfo.cpcexist))
							{
								//current phase begin to yellow lamp
								if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.cnpchan,0x01))
        						{
        						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            						printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        						#endif
									*(fcdata->markbit) |= 0x0800;
        						}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
													pinfo.cnpchan,0x01, fcdata->markbit))
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
								if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.cpchan,0x00))
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									*(fcdata->markbit) |= 0x0800;
								}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																	pinfo.cpchan,0x00,fcdata->markbit))
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
							}
							else
							{
								if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x01))
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									*(fcdata->markbit) |= 0x0800;
								}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														pinfo.cchan,0x01,fcdata->markbit))
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
							}

							if ((0 != pinfo.cchan[0]) && (pinfo.ytime > 0))
							{
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									sinfo.conmode = 28;
									sinfo.color = 0x01;
									sinfo.time = pinfo.ytime;
									sinfo.stage = pinfo.stageline;
									sinfo.cyclet = 0;
									sinfo.phase = 0;
									sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
															
									for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						if (0 == pinfo.cnpchan[i])
                    						break;
                						for (j = 0; j < sinfo.chans; j++)
                						{
                    						if (0 == sinfo.csta[j])
                        						break;
                    						tcsta = sinfo.csta[j];
                    						tcsta &= 0xfc;
                    						tcsta >>= 2;
                    						tcsta &= 0x3f;
                    						if (pinfo.cnpchan[i] == tcsta)
                    						{
                        						sinfo.csta[j] &= 0xfc;
												sinfo.csta[j] |= 0x01; //00000001-yellow
												break;
                    						}
                						}
            						}
									for (i = 0; i < MAX_CHANNEL; i++)
                                	{
                                		if (0 == pinfo.cpchan[i])
                                        	break;
                                    	for (j = 0; j < sinfo.chans; j++)
                                    	{
                                        	if (0 == sinfo.csta[j])
                                            	break;
                                        	tcsta = sinfo.csta[j];
                                        	tcsta &= 0xfc;
                                        	tcsta >>= 2;
                                        	tcsta &= 0x3f;
                                        	if (pinfo.cpchan[i] == tcsta)
                                        	{
                                            	sinfo.csta[j] &= 0xfc;
												break;
                                        	}
                                    	}
                                	}
									memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&sinfo))
            						{
            						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively
							}//if ((0 != pinfo.cchan[0]) && (pinfo.ytime > 0))

							fbdata[1] = 28;
							fbdata[2] = pinfo.stageline;
                            fbdata[3] = 0x01;
                            fbdata[4] = pinfo.ytime;
                            if (!wait_write_serial(*(fcdata->fbserial)))
                            {
                               	if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               	{
                               	#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                   	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               	#endif
									*(fcdata->markbit) |= 0x0800;
									gettimeofday(&ct,NULL);
                                   	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                               	}
                            }
                            else
                            {
                            #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                               	printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            #endif
                            }
							if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            {   
                                //send down time to configure tool
                                memset(&tctd,0,sizeof(tctd));
                                tctd.mode = 28;
                                tctd.pattern = *(fcdata->patternid);
                                tctd.lampcolor = 0x01;
                                tctd.lamptime = pinfo.ytime;
                                tctd.phaseid = pinfo.phaseid;
                                tctd.stageline = pinfo.stageline;
                                if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
                                {
                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
                            }
							#ifdef EMBED_CONFIGURE_TOOL
							if (*(fcdata->markbit2) & 0x0200)
							{
								memset(&tctd,0,sizeof(tctd));
                                tctd.mode = 28;
                                tctd.pattern = *(fcdata->patternid);
                                tctd.lampcolor = 0x01;
                                tctd.lamptime = pinfo.ytime;
                                tctd.phaseid = pinfo.phaseid;
                                tctd.stageline = pinfo.stageline;
								if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#endif
							sleep(pinfo.ytime);

							//current phase begin to red lamp
							if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x00))
                            {
                            #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
								*(fcdata->markbit) |= 0x0800;
                            }
							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus,pinfo.cchan,0x00, \
								fcdata->markbit))
							{
							#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
								printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}

							if ((0 != pinfo.cchan[0]) && (pinfo.rtime > 0))
							{
								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
								{   
									//send down time to configure tool
									memset(&tctd,0,sizeof(tctd));
									tctd.mode = 28;
									tctd.pattern = *(fcdata->patternid);
									tctd.lampcolor = 0x00;
									tctd.lamptime = pinfo.rtime;
									tctd.phaseid = pinfo.phaseid;
									tctd.stageline = pinfo.stageline;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&tctd))
									{
									#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
										printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef EMBED_CONFIGURE_TOOL
                                if (*(fcdata->markbit2) & 0x0200)
                                {
                                 	memset(&tctd,0,sizeof(tctd));
                                    tctd.mode = 28;
                                    tctd.pattern = *(fcdata->patternid);
                                    tctd.lampcolor = 0x00;
                                    tctd.lamptime = pinfo.rtime;
                                    tctd.phaseid = pinfo.phaseid;
                                    tctd.stageline = pinfo.stageline;  
                                    if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&tctd))
                                    {
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                                #endif
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									sinfo.conmode = 28;
									sinfo.color = 0x00;
									sinfo.time = pinfo.rtime;
									sinfo.stage = pinfo.stageline;
									sinfo.cyclet = 0;
									sinfo.phase = 0;
									sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
															
									for (i = 0; i < MAX_CHANNEL; i++)
            						{
                						if (0 == pinfo.cchan[i])
                    						break;
                						for (j = 0; j < sinfo.chans; j++)
                						{
                    						if (0 == sinfo.csta[j])
                        						break;
                    						tcsta = sinfo.csta[j];
                    						tcsta &= 0xfc;
                    						tcsta >>= 2;
                    						tcsta &= 0x3f;
                    						if (pinfo.cchan[i] == tcsta)
                    						{
                        						sinfo.csta[j] &= 0xfc;
												break;
                    						}
                						}
            						}
									memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));	
            						memset(sibuf,0,sizeof(sibuf));
            						if (SUCCESS != status_info_report(sibuf,&sinfo))
            						{
            						#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively
							}//if ((0 != pinfo.cchan[0]) && (pinfo.rtime > 0))

							if (pinfo.rtime > 0)
							{
								fbdata[1] = 28;
                                fbdata[2] = pinfo.stageline;
                                fbdata[3] = 0x00;
                                fbdata[4] = pinfo.rtime;
                                if (!wait_write_serial(*(fcdata->fbserial)))
                                {
                                    if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                    {
                                    #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                        printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
                                        update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    }
                                }
                                else
                                {
                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                #endif
                                }
								sleep(pinfo.rtime);
							}

							*(fcdata->slnum) += 1;
							*(fcdata->stageid) = \
								tscdata->timeconfig->TimeConfigList[socrettl][*(fcdata->slnum)].StageId;
							if (0 == (tscdata->timeconfig->TimeConfigList[socrettl][*(fcdata->slnum)].StageId))
							{
								*(fcdata->slnum) = 0;
								*(fcdata->stageid) = \
									tscdata->timeconfig->TimeConfigList[socrettl][*(fcdata->slnum)].StageId;
							}
							//get phase info of next phase
							memset(&pinfo,0,sizeof(socpinfo_t));
        					if (SUCCESS != soc_get_phase_info(fcdata,tscdata,socrettl,*(fcdata->slnum),&pinfo))
        					{
        					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
            					printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
        					#endif
								soc_end_part_child_thread();
            					return FAIL;
        					}
        					*(fcdata->phaseid) = 0;
        					*(fcdata->phaseid) |= (0x01 << (pinfo.phaseid - 1));

							if (SUCCESS != soc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                            {
                            #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
								*(fcdata->markbit) |= 0x0800;
                            }
						
							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus,pinfo.chan,0x02, \
                                fcdata->markbit))
                            {
                            #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }

							if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
							{//report info to server actively
								sinfo.conmode = 28;
								sinfo.color = 0x02;
								sinfo.time = 0;
								sinfo.stage = pinfo.stageline;
								sinfo.cyclet = 0;
								sinfo.phase = 0;
								sinfo.phase |= (0x01 << (pinfo.phaseid - 1));
															
								sinfo.chans = 0;
            					memset(sinfo.csta,0,sizeof(sinfo.csta));
            					csta = sinfo.csta;
            					for (i = 0; i < MAX_CHANNEL; i++)
            					{
									if (0 == pinfo.chan[i])
										break;	
                					sinfo.chans += 1;
                					tcsta = pinfo.chan[i];
                					tcsta <<= 2;
                					tcsta &= 0xfc;
									tcsta |= 0x02;	
                					*csta = tcsta;
                					csta++;
            					}
								memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            					memset(sibuf,0,sizeof(sibuf));
            					if (SUCCESS != status_info_report(sibuf,&sinfo))
            					{
            					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            					#endif
            					}
            					else
            					{
                					write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            					}
							}//report info to server actively

							fbdata[1] = 28;	
                            fbdata[2] = pinfo.stageline;
                            fbdata[3] = 0x02;
                            fbdata[4] = 0;
                            if (!wait_write_serial(*(fcdata->fbserial)))
                            {
                                if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                {
                                #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                    printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									*(fcdata->markbit) |= 0x0800;
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                }
                            }
                            else
                            {
                            #ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
                                printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            #endif
                            }							

							gettimeofday(&ct,NULL);
                            update_event_list(fcdata->ec,fcdata->el,1,92,ct.tv_sec,fcdata->markbit);
							continue;
						}//step by step
						if ((1 == wllock) && (!strcmp("YF",wlbuf)))
						{//yellow flash
							tcred = 0;
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

							if (0 == socyfyes)
							{
							//	int		*bbserial = fcdata->bbserial;
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
								int yfret = pthread_create(&socyfpid,NULL,(void *)soc_yellow_flash,&yfdata);
								if (0 != yfret)
								{
								#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
									printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									soc_end_part_child_thread();
									return FAIL;
								}
								socyfyes = 1;
							}

							if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
							{//report info to server actively
								sinfo.conmode = 28;
								sinfo.color = 0x05;
								sinfo.time = 0;
								sinfo.stage = 0;
								sinfo.cyclet = 0;
								sinfo.phase = 0;
															
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
								memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            					memset(sibuf,0,sizeof(sibuf));
            					if (SUCCESS != status_info_report(sibuf,&sinfo))
            					{
            					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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

							if (1 == socyfyes)
							{
								pthread_cancel(socyfpid);
								pthread_join(socyfpid,NULL);
								socyfyes = 0;
							}
							close = 0;	
							if (0 == tcred)
							{	
								new_all_red(&ardata);
								tcred = 1;
							}						

							if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
							{//report info to server actively
								sinfo.conmode = 28;
								sinfo.color = 0x00;
								sinfo.time = 0;
								sinfo.stage = 0;
								sinfo.cyclet = 0;
								sinfo.phase = 0;
															
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
								memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            					memset(sibuf,0,sizeof(sibuf));
            					if (SUCCESS != status_info_report(sibuf,&sinfo))
            					{
            					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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

							if (1 == socyfyes)
							{
								pthread_cancel(socyfpid);
								pthread_join(socyfpid,NULL);
								socyfyes = 0;
							}
							tcred = 0;	
							if (0 == close)
							{	
								new_all_close(&acdata);
								close = 1;
							}						

							if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
							{//report info to server actively
								sinfo.conmode = 28;
								sinfo.color = 0x04;
								sinfo.time = 0;
								sinfo.stage = 0;
								sinfo.cyclet = 0;
								sinfo.phase = 0;
															
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
								memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            					memset(sibuf,0,sizeof(sibuf));
            					if (SUCCESS != status_info_report(sibuf,&sinfo))
            					{
            					#ifdef SYSTEM_OPTIMIZE_COORD_DEBUG
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
							memset(&mdt,0,sizeof(markdata_t));
							mdt.redl = &tcred;
							mdt.closel = &close;
							mdt.rettl = &socrettl;
							mdt.dircon = &dircon;
							mdt.firdc = &firdc;
							mdt.yfl = &socyfyes;
							mdt.yfid = &socyfpid;
							mdt.ardata = &ardata;
							mdt.fcdata = fcdata;
							mdt.tscdata = tscdata;
							mdt.chanstatus = chanstatus;
							mdt.sinfo = &sinfo;
							soc_mobile_jp_control(&mdt,staid,&pinfo,fdirch);
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
							memset(&mdt,0,sizeof(markdata_t));
							mdt.redl = &tcred;
							mdt.closel = &close;
							mdt.rettl = &socrettl;
							mdt.dircon = &dircon;
							mdt.firdc = &firdc;
							mdt.yfl = &socyfyes;
							mdt.yfid = &socyfpid;
							mdt.ardata = &ardata;
							mdt.fcdata = fcdata;
							mdt.tscdata = tscdata;
							mdt.chanstatus = chanstatus;
							mdt.sinfo = &sinfo;
							soc_mobile_direct_control(&mdt,&pinfo,cdirch,fdirch);
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
					}//wireless control is valid only when key control is not valid
				}//data from wireless terminal	
			}//if (FD_ISSET(*(fcdata->conpipe),&nread))
		}//cpret > 0
	}//while loop	

	return SUCCESS;
}


