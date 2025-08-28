/*
 *	File:	rcc.c
 *	Author:	sk
 *	Date:	20160629
*/
#include "rcc.h"

static struct timeval			gtime,gftime,ytime,rtime;
static unsigned char			rccyes = 0;
static pthread_t				rccid;
static unsigned char			rccyfyes = 0;
static pthread_t				rccyfid;
static unsigned char			rccpcyes = 0;
static pthread_t				rccpcid;
static unsigned char			ppmyes = 0;
static pthread_t				ppmid;
static unsigned char			cpdyes = 0;
static pthread_t				cpdid;
static unsigned char			rettl = 0;

static phasedetect_t			phdetect[MAX_PHASE_LINE] = {0};
static phasedetect_t			*pphdetect = NULL;
static detectorpro_t			*dpro = NULL;
static unsigned char			degrade = 0;
static statusinfo_t				sinfo;
static unsigned char			phcon = 0;
		
void rcc_end_child_thread()
{
	if (1 == rccyfyes)
	{
		pthread_cancel(rccyfid);
		pthread_join(rccyfid,NULL);
		rccyfyes = 0;
	}
	if (1 == rccpcyes)
	{
		pthread_cancel(rccpcid);
		pthread_join(rccpcid,NULL);
		rccpcyes = 0;
	}
	if (1 == ppmyes)
	{
		pthread_cancel(ppmid);
		pthread_join(ppmid,NULL);
		ppmyes = 0;
	}
	if (1 == cpdyes)
	{
		pthread_cancel(cpdid);
		pthread_join(cpdid,NULL);
		cpdyes = 0;
	}
	if (1 == rccyes)
	{
		pthread_cancel(rccid);
		pthread_join(rccid,NULL);
		rccyes = 0;
	}
}

void rcc_end_part_child_thread()
{
	if (1 == rccyfyes)
	{
		pthread_cancel(rccyfid);
		pthread_join(rccyfid,NULL);
		rccyfyes = 0;
	}
	if (1 == rccpcyes)
	{
		pthread_cancel(rccpcid);
		pthread_join(rccpcid,NULL);
		rccpcyes = 0;
	}
	if (1 == ppmyes)
	{
		pthread_cancel(ppmid);
		pthread_join(ppmid,NULL);
		ppmyes = 0;
	}
	if (1 == cpdyes)
	{
		pthread_cancel(cpdid);
		pthread_join(cpdid,NULL);
		cpdyes = 0;
	}
}

void rcc_yellow_flash(void *arg)
{
    yfdata_t            *yfdata = arg;

    new_yellow_flash(yfdata);

    pthread_exit(NULL);
}

void rcc_monitor_pending_pipe(void *arg)
{
	monpendphase_t      *mpphase = arg;
	fd_set 				nRead;
	unsigned char		buf[256] = {0};
	unsigned char		*pbuf = buf;
	unsigned short		num = 0;
	unsigned short		mark = 0;
	unsigned char		i = 0, j = 0;
	unsigned char		dtype = 0; //detector type
	unsigned char		deteid = 0;//detector id;
	unsigned char		mphase = 0;

	while (1)
	{//while loop
		FD_ZERO(&nRead);
		FD_SET(*(mpphase->pendpipe),&nRead);
		int fret = select(*(mpphase->pendpipe)+1,&nRead,NULL,NULL,NULL);
		if (fret < 0)
		{
		#ifdef RCC_DEBUG
			printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return;
		}
		if (fret > 0)	
		{//0
			if (FD_ISSET(*(mpphase->pendpipe),&nRead))
			{//1
				memset(buf,0,sizeof(buf));
				pbuf = buf;
				num = read(*(mpphase->pendpipe),buf,sizeof(buf));
				if (num > sizeof(buf))
                    continue;
				if (num > 0)
				{//2
					mark = 0;
					while (1)
					{//3
						if (mark >= num)
							break;
						if ((0xF1 == *(pbuf+mark)) && (0xED == *(pbuf+mark+5)))
						{
							deteid = *(pbuf+mark+2);
							if (3 == *(pbuf+mark+1))
							{//detector report error
								for (i = 0; i < MAX_PHASE_LINE; i++)
								{
									if (0 == phdetect[i].phaseid)
										break;
									for (j = 0; j < 10; j++)
									{
										if (0 == phdetect[i].detectpro[j].deteid)
											break;
										if (deteid == phdetect[i].detectpro[j].deteid)
										{
											phdetect[i].detectpro[j].validmark = 0;
											phdetect[i].detectpro[j].err = 1;
											phdetect[i].detectpro[j].stime.tv_sec = 0;
											phdetect[i].detectpro[j].stime.tv_usec = 0;
											gettimeofday(&(phdetect[i].detectpro[j].stime),NULL);
										}
									}
								}
							}//detector report error
							if (4 == *(pbuf+mark+1))
							{//person type detector
								for (i = 0; i < MAX_PHASE_LINE; i++)
                                {
                                    if (0 == phdetect[i].phaseid)
                                        break;
                                    for (j = 0; j < 10; j++)
                                    {
                                        if (0 == phdetect[i].detectpro[j].deteid)
                                            break;
                                        if (deteid == phdetect[i].detectpro[j].deteid)
                                        {
                                            phdetect[i].detectpro[j].validmark = 1;
											phdetect[i].detectpro[j].err = 0;
											phdetect[i].detectpro[j].stime.tv_sec = 0;
                                            phdetect[i].detectpro[j].stime.tv_usec = 0;
                                            gettimeofday(&(phdetect[i].detectpro[j].stime),NULL);
                                        }
                                    }
                                }
							}//person type detector
#if 0
							if (5 == *(pbuf+mark+1))
							{//bus proprity detector
								for (i = 0; i < MAX_PHASE_LINE; i++)
                                {
                                    if (0 == phdetect[i].phaseid)
                                        break;
                                    for (j = 0; j < 10; j++)
                                    {
                                        if (0 == phdetect[i].detectpro[j].deteid)
                                            break;
                                        if (deteid == phdetect[i].detectpro[j].deteid)
                                        {
                                            phdetect[i].detectpro[j].validmark = 1;
											phdetect[i].detectpro[j].err = 0;
											phdetect[i].detectpro[j].stime.tv_sec = 0;
                                            phdetect[i].detectpro[j].stime.tv_usec = 0;
                                            gettimeofday(&(phdetect[i].detectpro[j].stime),NULL);
                                        }
                                    }
                                }
							}//bus proprity detector
							if (1 == *(pbuf+mark+1))
							{//if (1 == *(pbuf+mark+1))
								for (i = 0; i < MAX_PHASE_LINE; i++)
                                {
                                    if (0 == phdetect[i].phaseid)
                                        break;
                                    for (j = 0; j < 10; j++)
                                    {
                                        if (0 == phdetect[i].detectpro[j].deteid)
                                            break;
                                        if (deteid == phdetect[i].detectpro[j].deteid)
                                        {
                                            phdetect[i].detectpro[j].validmark = 1;
											phdetect[i].detectpro[j].err = 0;
											phdetect[i].detectpro[j].stime.tv_sec = 0;
                                            phdetect[i].detectpro[j].stime.tv_usec = 0;
                                            gettimeofday(&(phdetect[i].detectpro[j].stime),NULL);
                                        }
                                    }
                                }							
							}//if (1 == *(pbuf+mark+1))
#endif
							mark += 6;
							continue;//Fir data,break directly
						}
						else
						{
							mark += 1;
							continue;
						}
					}//3
					continue;
				}//2
				else
				{
					continue;
				}
			}//1
			else
			{
				continue;
			}
		}//0
	}//while loop

	pthread_exit(NULL);
}

void rcc_count_phase_detector(void *arg)
{
	unsigned char			i = 0, j = 0, z = 0;
	struct timeval			ntime;
	unsigned int			leatime = 0;
	unsigned char			exist = 0;

	while (1)
	{//while loop
		sleep(INTERVALTIME);
		#ifdef RCC_DEBUG
		printf("Begin to count detector of phase,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif

		for (i = 0; i < MAX_PHASE_LINE; i++)
		{//for (i = 0; i < MAX_PHASE_LINE; i++)
			if (0 == phdetect[i].phaseid)
				break;
//			if ((0x80 == phdetect[i].phasetype) || (0x04 == phdetect[i].phasetype))
//                continue;
			for (j = 0; j < 10; j++)
			{//for (j = 0; j < 10; j++)
				if (0 == phdetect[i].detectpro[j].deteid)
					break;
				if (1 == phdetect[i].detectpro[j].err)
				{//detector do report err own
					exist = 0;
					for (z = 0; z < 10; z++)
					{
						if (phdetect[i].indetect[z] == phdetect[i].detectpro[j].deteid)
						{
							exist = 1;
							break;
						}
					}
					if (0 == exist)
					{
						for (z = 0; z < 10; z++)
						{
							if (0 == phdetect[i].indetect[z])
							{
								phdetect[i].indetect[z] = phdetect[i].detectpro[j].deteid;
								phdetect[i].factnum += 1;
								break;
							}
						}
					}
				}//detector do report err own
				else
				{//detector do not report err own;
					if (0 == phdetect[i].detectpro[j].validmark) 
					{//the detector is invalid;
						ntime.tv_sec = 0;
						ntime.tv_usec = 0;
						gettimeofday(&ntime,NULL);
						leatime = ntime.tv_sec - (phdetect[i].detectpro[j].stime.tv_sec);
						if (leatime >= phdetect[i].detectpro[j].intime)
						{
							exist = 0;
							for (z = 0; z < 10; z++)
							{
								if (phdetect[i].indetect[z] == phdetect[i].detectpro[j].deteid)
								{
									exist = 1;
									break;
								}
							}
							if (0 == exist)
							{
								for (z = 0; z < 10; z++)
                        		{
                            		if (0 == phdetect[i].indetect[z])
                            		{
                                		phdetect[i].indetect[z] = phdetect[i].detectpro[j].deteid;
                                		phdetect[i].factnum += 1;
                                		break;
                            		}
                        		}
							}
						}//if (leatime >= phdetect[i].detectpro[j].intime)		
					}//the detector is invalid
					else
					{//the detector is valid
						for (z = 0; z < 10; z++)
                        {
                            if (phdetect[i].indetect[z] == phdetect[i].detectpro[j].deteid)
                            {
                                phdetect[i].indetect[z] = 0;
                                if (phdetect[i].factnum > 0)
                                    phdetect[i].factnum -= 1;
                                break;
                            }
                        }
                        phdetect[i].detectpro[j].validmark = 0;
					}//the detector is valid
				}//detector do not report err own;
			}//for (j = 0; j < 10; j++)
			
			if ((phdetect[i].factnum >= phdetect[i].indetenum) && (0x04 == phdetect[i].phasetype))
			{
				phdetect[i].validmark = 0;
				degrade = 1;
			}
			else
			{
				phdetect[i].validmark = 1;
				degrade = 0;
			}				
		}//for (i = 0; i < MAX_PHASE_LINE; i++)
	}//while loop

	pthread_exit(NULL);
}

void rcc_person_chan_greenflash(void *arg)
{
	rccpcdata_t				*rccpcdata = arg;
	unsigned char			i = 0;
	struct timeval			timeout;

	while (1)
	{
		if (SUCCESS != rcc_set_lamp_color(*(rccpcdata->bbserial),rccpcdata->pchan,0x03))
		{
		#ifdef RCC_DEBUG
			printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			*(rccpcdata->markbit) |= 0x0800;
		}
		if (SUCCESS != update_channel_status(rccpcdata->sockfd,rccpcdata->cs,rccpcdata->pchan, \
							0x03,rccpcdata->markbit))
		{
		#ifdef RCC_DEBUG
			printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;
		select(0,NULL,NULL,NULL,&timeout);

		if (SUCCESS != rcc_set_lamp_color(*(rccpcdata->bbserial),rccpcdata->pchan,0x02))
		{
		#ifdef RCC_DEBUG
			printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			*(rccpcdata->markbit) |= 0x0800;
		}
		if (SUCCESS != update_channel_status(rccpcdata->sockfd,rccpcdata->cs,rccpcdata->pchan, \
                            0x02,rccpcdata->markbit))
		{
		#ifdef RCC_DEBUG
			printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;
		select(0,NULL,NULL,NULL,&timeout);
	}

	pthread_exit(NULL);
}


void start_rail_crossing_control(void *arg)
{
	rccdata_t				*rccdata = arg;
	unsigned char			tcline = 0;
	unsigned char			snum = 0;//max number of stage
	unsigned char			slnum = 0;
	unsigned char			i = 0,j = 0,z = 0;
	unsigned char			tphid = 0;
	unsigned char			tnum = 0;
	unsigned char			personpid = 0;
	timedown_t				timedown;
	monpendphase_t			mpphase;

	unsigned char           downti[8] = {0xA6,0xff,0xff,0xff,0xff,0x03,0,0xED};
	unsigned char           edownti[3] = {0xA5,0xA5,0xED};
	if (!wait_write_serial(*(rccdata->fd->bbserial)))
    {
    	if (write(*(rccdata->fd->bbserial),downti,sizeof(downti)) < 0)
        {
		#ifdef RCC_DEBUG
			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif       
        }
    }
    else
    {
    #ifdef RCC_DEBUG
    	printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
    }
    if (!wait_write_serial(*(rccdata->fd->bbserial)))
    {
    	if (write(*(rccdata->fd->bbserial),edownti,sizeof(edownti)) < 0)
        {
        #ifdef RCC_DEBUG
        	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
        }
    }
    else
    {
    #ifdef RCC_DEBUG
    	printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
    }

	if (SUCCESS != rcc_get_timeconfig(rccdata->td,rccdata->fd->patternid,&tcline))
    {
    #ifdef RCC_DEBUG
        printf("rcc_get_timeconfig err,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
        rcc_end_part_child_thread();
       // return;

		struct timeval				time;
		unsigned char				yferr[10] = {0};
		gettimeofday(&time,NULL);
		update_event_list(rccdata->fd->ec,rccdata->fd->el,1,11,time.tv_sec,rccdata->fd->markbit);
		if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
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
				write(*(rccdata->fd->sockfd->csockfd),yferr,sizeof(yferr));
			}
		}

		yfdata_t					yfdata;
		if (0 == rccyfyes)
		{
			memset(&yfdata,0,sizeof(yfdata));
			yfdata.second = 0;
			yfdata.markbit = rccdata->fd->markbit;
			yfdata.markbit2 = rccdata->fd->markbit2;
			yfdata.serial = *(rccdata->fd->bbserial);
			yfdata.sockfd = rccdata->fd->sockfd;
			yfdata.cs = rccdata->cs;
#ifdef FLASH_DEBUG
			char szInfo[32] = {0};
			char szInfoT[64] = {0};
			snprintf(szInfo,sizeof(szInfo)-1,"%s",__FUNCTION__);
			snprintf(szInfoT,sizeof(szInfoT)-1,"%d",__LINE__);
			tsc_save_eventlog(szInfo,szInfoT);
#endif
			int yfret = pthread_create(&rccyfid,NULL,(void *)rcc_yellow_flash,&yfdata);
			if (0 != yfret)
			{
		#ifdef TIMING_DEBUG
				printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
				output_log("rcc_yellow_flash control,create yellow flash err");
		#endif
				rcc_end_part_child_thread();
				return;
			}
			rccyfyes = 1;
		}		
		while(1)
		{
			if (*(rccdata->fd->markbit) & 0x0001)
			{//end time of current pattern has arrived
				unsigned char           enddata[3] = {0xCC,0xDD,0xED};
				if (!wait_write_serial(*(rccdata->fd->synpipe)))
				{
					if (write(*(rccdata->fd->synpipe),enddata,sizeof(enddata)) < 0)
					{
				#ifdef FULL_DETECT_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
						rcc_end_part_child_thread();
						return;
					}
				}
				else
				{
			#ifdef FULL_DETECT_DEBUG
					printf("pipe cannot be written,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
					rcc_end_part_child_thread();
					return;
				}
				sleep(5);//wait main module end own;	
			}//end time of current pattern has arrived
			sleep(10);
			continue;
		}

    }
    rettl = tcline;
	//Get max value of stage,all phase and its maped detector,
	memset(phdetect,0,sizeof(phdetect));
	pphdetect = phdetect;
	for (snum = 0; ;snum++)
	{
		if (0 == rccdata->td->timeconfig->TimeConfigList[tcline][snum].StageId)
			break;
		get_phase_id(rccdata->td->timeconfig->TimeConfigList[tcline][snum].PhaseId,&tphid);
		for (i = 0; i < (rccdata->td->phase->FactPhaseNum); i++)
		{
			if (0 == (rccdata->td->phase->PhaseList[i].PhaseId))
				break;
			if (tphid == (rccdata->td->phase->PhaseList[i].PhaseId))
			{
				pphdetect->phaseid = tphid;
				pphdetect->phasetype = rccdata->td->phase->PhaseList[i].PhaseType;
				if (0x04 == (rccdata->td->phase->PhaseList[i].PhaseType))
					personpid = rccdata->td->phase->PhaseList[i].PhaseId;
				tnum = rccdata->td->phase->PhaseList[i].PhaseSpecFunc;
				tnum &= 0xe0;//get 5~7bit
				tnum >>= 5;
				tnum &= 0x07; 
				pphdetect->indetenum = tnum;//invalid detector arrive the number,begin to degrade;
				pphdetect->validmark = 1;
				pphdetect->factnum = 0;
				memset(pphdetect->indetect,0,sizeof(pphdetect->indetect));
				memset(pphdetect->detectpro,0,sizeof(pphdetect->detectpro));
				if (0x04 == (pphdetect->phasetype))
				{//only person phase map detector in person pass streel control	
					dpro = pphdetect->detectpro;
					for (z = 0; z < (rccdata->td->detector->FactDetectorNum); z++)
					{//1
						if (0 == (rccdata->td->detector->DetectorList[z].DetectorId))
							break;
						if ((pphdetect->phaseid) == (rccdata->td->detector->DetectorList[z].DetectorPhase))
						{
							dpro->deteid = rccdata->td->detector->DetectorList[z].DetectorId;
							dpro->detetype = rccdata->td->detector->DetectorList[z].DetectorType;
							dpro->validmark = 0;
							dpro->err = 0;
							tnum = rccdata->td->detector->DetectorList[z].DetectorSpecFunc;
							tnum &= 0xfc;//get 2~7bit
							tnum >>= 2;
							tnum &= 0x3f;
							dpro->intime = tnum * 60; //seconds;
							memset(&(dpro->stime),0,sizeof(struct timeval));
							gettimeofday(&(dpro->stime),NULL);
							dpro++;
						}
					}//1
				}//only person phase map detector in person pass streel control
				pphdetect++;
				break;
			}
		}//for (i = 0; i < (rccdata->td->phase->FactPhaseNum); i++)
	}//for (snum = 0; ;snum++)
#if 0	
	if (0 == ppmyes)
	{//monitor pending pipe to check the valid or invalid of detector when do not have detector control
		memset(&mpphase,0,sizeof(monpendphase_t));
		mpphase.pendpipe = rccdata->fd->pendpipe;
		mpphase.detector = rccdata->td->detector;
		int ppmret = pthread_create(&ppmid,NULL,(void *)rcc_monitor_pending_pipe,&mpphase);
		if (0 != ppmret)
		{
		#ifdef RCC_DEBUG
			printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			rcc_end_part_child_thread();
			return;
		}
		ppmyes = 1;
	}//monitor pending pipe to check the valid or invalid of detector when do not have detector control	
	if (0 == cpdyes)
	{
		int cpdret = pthread_create(&cpdid,NULL,(void *)rcc_count_phase_detector,NULL);
		if (0 != cpdret)
		{
		#ifdef RCC_DEBUG
			printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			rcc_end_part_child_thread();
			return;
		}
		cpdyes = 1;
	}
#endif

	slnum = *(rccdata->fd->slnum);
	*(rccdata->fd->stageid) = rccdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
	if (0 == (rccdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId))
	{
		slnum = 0;
		*(rccdata->fd->slnum) = 0;
		*(rccdata->fd->stageid) = rccdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
	}

	unsigned char				cycarr = 0;
	rccpinfo_t					*pinfo = rccdata->pi;	
	rccpcdata_t					rccpcdata;
	unsigned char				gft = 0; //green flash time;
	struct timeval				timeout,mtime,nowtime,lasttime,ct;
	unsigned char				leatime = 0,mt = 0;
	fd_set						nRead;
	unsigned char				sltime = 0;
	unsigned char				ffw = 0;
	unsigned char				buf[256] = {0};
	unsigned char				*pbuf = buf;
	unsigned short				num = 0, mark = 0;
	unsigned char				personyes = 0;//'1' means that person press button;
	unsigned char				dtype = 0; //the type of person button;
	unsigned char				concyc = 0;
	unsigned char				mappid = 0;//phase id of person mapping;
	unsigned char				deteid = 0;
	unsigned char				fbdata[6] = {0};
	fbdata[0] = 0xC1;
	fbdata[5] = 0xED;

	unsigned char               endahead = 0;
	unsigned char               endcyclen = 0;
	unsigned int                lefttime = 0;
	unsigned char               c1gt = 0;
	unsigned char               c1gmt = 0;
	unsigned char               c2gt = 0;
	unsigned char               c2gmt = 0;
	unsigned char               twocycend = 0;
	unsigned char               onecycend = 0;
	unsigned char               validmark = 0; 

	unsigned char				sibuf[64] = {0};
//	statusinfo_t				sinfo;
	unsigned char               *csta = NULL;
    unsigned char               tcsta = 0;
//	memset(&sinfo,0,sizeof(statusinfo_t));
	sinfo.conmode = *(rccdata->fd->contmode);
    sinfo.pattern = *(rccdata->fd->patternid);
    sinfo.cyclet = *(rccdata->fd->cyclet);

	dunhua_close_lamp(rccdata->fd,rccdata->cs);

	while (1)
	{//while loop
		if (1 == cycarr)
		{
			cycarr = 0;

			if (*(rccdata->fd->markbit) & 0x0001)
			{//end time of current pattern has arrived
				unsigned char			syndata[3] = {0xCC,0xDD,0xED};
				if (!wait_write_serial(*(rccdata->fd->synpipe)))
				{
					if (write(*(rccdata->fd->synpipe),syndata,sizeof(syndata)) < 0)
					{
					#ifdef RCC_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						rcc_end_part_child_thread();
						return;
					}
				}
				else
				{
				#ifdef RCC_DEBUG
					printf("pipe cannot be written,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					rcc_end_part_child_thread();
					return;
				}
				//Note, 0th bit is clean 0 by main module
				sleep(5);//wait main module end own;
			}//end time of current pattern has arrived

			if ((*(rccdata->fd->markbit) & 0x0100) && (0 == endahead))
			{//current pattern transit ahead two cycles
				//Note, 8th bit is clean 0 by main module
				if (0 != *(rccdata->fd->ncoordphase))
				{//next coordphase is not 0
					endahead = 1;
					endcyclen = 0;
					lefttime = 0;
					gettimeofday(&ct,NULL);
					lefttime = (unsigned int)((rccdata->fd->nst) - ct.tv_sec);
					#ifdef RCC_DEBUG
					printf("*****************lefttime: %d,File: %s,Line: %d\n",lefttime,__FILE__,__LINE__);
					#endif
					if (lefttime >= (*(rccdata->fd->cyclet)*3)/2)
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
					#ifdef RCC_DEBUG
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
                    sinfo.cyclet = (c1gt + 3 + 3)*snum + c1gmt;
                }
				if (3 == endcyclen)
				{
					endcyclen = 2;
					ct.tv_sec = 0;
					ct.tv_usec = 200000;
					select(0,NULL,NULL,NULL,&ct);
					cycarr = 1;
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
						cycarr = 1;
						continue;
					}
					sinfo.cyclet = (c2gt + 3 + 3)*snum + c2gmt;
				}	
			}//if (1 == endahead)
		}//if (1 == cycarr)
		
		memset(pinfo,0,sizeof(rccpinfo_t));
		if (SUCCESS != rcc_get_phase_info(rccdata->fd,rccdata->td,tcline,slnum,pinfo))
		{
		#ifdef RCC_DEBUG
			printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			rcc_end_part_child_thread();
			return;
		}
		*(rccdata->fd->phaseid) = 0;
		*(rccdata->fd->phaseid) |= (0x01 << (pinfo->phaseid - 1));
		sinfo.stage = *(rccdata->fd->stageid);
        sinfo.phase = *(rccdata->fd->phaseid);
		if ((*(rccdata->fd->markbit) & 0x0100) && (1 == endahead))
		{//end pattern ahead two cycles
			#ifdef RCC_DEBUG
			printf("End pattern two cycles AHEAD, begin %d cycle,Line:%d\n",endcyclen,__LINE__);
			#endif
			*(rccdata->fd->color) = 0x02;
			*(rccdata->fd->markbit) &= 0xfbff;
			if (SUCCESS != rcc_set_lamp_color(*(rccdata->fd->bbserial),pinfo->chan,0x02))
           	{
           	#ifdef RCC_DEBUG
               	printf("set green err,File: %s,Line: %d\n",__FILE__,__LINE__);
           	#endif
               	gettimeofday(&ct,NULL);
               	update_event_list(rccdata->fd->ec,rccdata->fd->el,1,15,ct.tv_sec,rccdata->fd->markbit);
               	if (SUCCESS != generate_event_file(rccdata->fd->ec,rccdata->fd->el,\
														rccdata->fd->softevent,rccdata->fd->markbit))
               	{
               	#ifdef RCC_DEBUG
                   	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
               	#endif
               	}
				*(rccdata->fd->markbit) |= 0x0800;
           	}
           	memset(&gtime,0,sizeof(gtime));
           	gettimeofday(&gtime,NULL);
           	memset(&gftime,0,sizeof(gftime));
           	memset(&ytime,0,sizeof(ytime));
           	memset(&rtime,0,sizeof(rtime));

           	if (SUCCESS != update_channel_status(rccdata->fd->sockfd,rccdata->cs,pinfo->chan,0x02, \
													rccdata->fd->markbit))
            {
            #ifdef RCC_DEBUG
               	printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
            #endif
            }
			if (0 == pinfo->cchan[0])
			{
				if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
				{//if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
					if (1 == endcyclen)
					{
						if (snum == (slnum + 1))
                    	{
                        	sinfo.time = c1gt + c1gmt + 3 + 3;
                    	}
                    	else
                    	{
                        	sinfo.time = c1gt + 3 + 3;
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
					sinfo.color = 0x02;
					sinfo.conmode = *(rccdata->fd->contmode);//added on 20150529
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
					memcpy(rccdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            		memset(sibuf,0,sizeof(sibuf));
            		if (SUCCESS != status_info_report(sibuf,&sinfo))
            		{
            		#ifdef RCC_DEBUG
                		printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            		#endif
            		}
            		else
            		{
                		write(*(rccdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            		}	
				}//if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
			}
			else
			{
				if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
				{//if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
					if (1 == endcyclen)
					{
						if (snum == (slnum + 1))
                    	{
                        	sinfo.time = c1gt + c1gmt + 3;
                    	}
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
					sinfo.color = 0x02;
					sinfo.conmode = *(rccdata->fd->contmode);//added on 20150529
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
					memcpy(rccdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            		memset(sibuf,0,sizeof(sibuf));
            		if (SUCCESS != status_info_report(sibuf,&sinfo))
            		{
            		#ifdef RCC_DEBUG
                		printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            		#endif
            		}
            		else
            		{
                		write(*(rccdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            		}	
				}//if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
			}

			if ((*(rccdata->fd->markbit) & 0x0002) && (*(rccdata->fd->markbit) & 0x0010))
            {//send down time data to configure tool
               	memset(&timedown,0,sizeof(timedown));
               	timedown.mode = *(rccdata->fd->contmode);
               	timedown.pattern = *(rccdata->fd->patternid);
               	timedown.lampcolor = 0x02;
				if (1 == endcyclen)
				{
					if (snum == (slnum + 1))
					{
						if (0x04 == (pinfo->phasetype))
							timedown.lamptime = c1gt + c1gmt + 3 + 3; //3 sec gf + 3sec yellow lamp
						else
							timedown.lamptime = c1gt + c1gmt + 3;//default 3 second green flash
					}
					else
					{
						if (0x04 == (pinfo->phasetype))
							timedown.lamptime = c1gt + 3 + 3;
						else
							timedown.lamptime = c1gt + 3;
					}
				}
				if (2 == endcyclen)
				{
					if (snum == (slnum + 1))
					{
						if (0x04 == (pinfo->phasetype))
							timedown.lamptime = c2gt + c2gmt + 3 + 3;
						else
							timedown.lamptime = c2gt + c2gmt + 3;
					}
					else
					{
						if (0x04 == (pinfo->phasetype))
							timedown.lamptime = c2gt + 3 + 3;
						else
							timedown.lamptime = c2gt + 3;
					}
				}
				timedown.phaseid = pinfo->phaseid;
               	timedown.stageline = pinfo->stageline;
               	if (SUCCESS != timedown_data_to_conftool(rccdata->fd->sockfd,&timedown))
               	{
               	#ifdef RCC_DEBUG
                   	printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
               	#endif
               	}
			}//send down time data to configure tool
			#ifdef EMBED_CONFIGURE_TOOL
			if (*(rccdata->fd->markbit2) & 0x0200)
			{   
				memset(&timedown,0,sizeof(timedown));
               	timedown.mode = *(rccdata->fd->contmode);
               	timedown.pattern = *(rccdata->fd->patternid);
               	timedown.lampcolor = 0x02;
				if (1 == endcyclen)
				{
					if (snum == (slnum + 1))
					{
						if (0x04 == (pinfo->phasetype))
							timedown.lamptime = c1gt + c1gmt + 3 + 3; //3 sec gf + 3sec yellow lamp
						else
							timedown.lamptime = c1gt + c1gmt + 3;//default 3 second green flash
					}
					else
					{
						if (0x04 == (pinfo->phasetype))
							timedown.lamptime = c1gt + 3 + 3;
						else
							timedown.lamptime = c1gt + 3;
					}
				}
				if (2 == endcyclen)
				{
					if (snum == (slnum + 1))
					{
						if (0x04 == (pinfo->phasetype))
							timedown.lamptime = c2gt + c2gmt + 3 + 3;
						else
							timedown.lamptime = c2gt + c2gmt + 3;
					}
					else
					{
						if (0x04 == (pinfo->phasetype))
							timedown.lamptime = c2gt + 3 + 3;
						else
							timedown.lamptime = c2gt + 3;
					}
				}
				timedown.phaseid = pinfo->phaseid;
               	timedown.stageline = pinfo->stageline;	 
				if (SUCCESS != timedown_data_to_embed(rccdata->fd->sockfd,&timedown))
				{
				#ifdef RCC_DEBUG
					printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#endif

			//send down time data to face board
			if (*(rccdata->fd->contmode) < 27)
				fbdata[1] = *(rccdata->fd->contmode) + 1;
			else
				fbdata[1] = *(rccdata->fd->contmode);
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
				if (1 == endcyclen)
				{
					if (snum == (slnum + 1))
					{
						if (0x04 == (pinfo->phasetype))
							fbdata[4] = c1gt + c1gmt +3 + 3;
						else
							fbdata[4] = c1gt + c1gmt + 3;
					}
					else
					{
						if (0x04 == (pinfo->phasetype))
							fbdata[4] = c1gt + 3 + 3;
						else
							fbdata[4] = c1gt + 3;
					}
				}
				if (2 == endcyclen)
				{
					if (snum == (slnum + 1))
					{
						if (0x04 == (pinfo->phasetype))
							fbdata[4] = c2gt + c2gmt + 3 + 3;
						else
							fbdata[4] = c2gt + c2gmt + 3;
					}
					else
					{
						if (0x04 == (pinfo->phasetype))
							fbdata[4] = c2gt + 3 + 3;
						else
							fbdata[4] = c2gt + 3;
					}
				}
			}
			if (!wait_write_serial(*(rccdata->fd->fbserial)))
            {
               	if (write(*(rccdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
               	{
               	#ifdef RCC_DEBUG
                   	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
               	#endif
					*(rccdata->fd->markbit) |= 0x0800;
                   	gettimeofday(&ct,NULL);
                   	update_event_list(rccdata->fd->ec,rccdata->fd->el,1,16,ct.tv_sec,rccdata->fd->markbit);
                   	if (SUCCESS != generate_event_file(rccdata->fd->ec,rccdata->fd->el,\
														rccdata->fd->softevent,rccdata->fd->markbit))
                   	{
                   	#ifdef RCC_DEBUG
                       	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                   	#endif
                   	}
               	}
            }
            else
            {
            #ifdef RCC_DEBUG
               	printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
			sendfaceInfoToBoard(rccdata->fd,fbdata);
			if (1 == endcyclen)
			{
				if (snum == (slnum + 1))
				{
		//			sleep(c1gt + c1gmt);
					sltime = c1gt + c1gmt;
				}
				else
				{
		//			sleep(c1gt);
					sltime = c1gt;
				}
			}
			if (2 == endcyclen)
			{
				if (snum == (slnum + 1))
				{
		//			sleep(c2gt + c2gmt);
					sltime = c2gt + c2gmt;
				}
				else
				{
		//			sleep(c2gt);
					sltime = c2gt;
				}
			}

			while (1)
			{//while (1)
				FD_ZERO(&nRead);
				FD_SET(*(rccdata->fd->ffpipe),&nRead);	
				lasttime.tv_sec = 0;
				lasttime.tv_usec = 0;
				gettimeofday(&lasttime,NULL);
				mtime.tv_sec = sltime;
				mtime.tv_usec = 0;
				int mret = select(*(rccdata->fd->ffpipe)+1,&nRead,NULL,NULL,&mtime);
				if (mret < 0)
				{
				#ifdef RCC_DEBUG
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
					if (FD_ISSET(*(rccdata->fd->ffpipe),&nRead))
					{
						memset(buf,0,sizeof(buf));
						read(*(rccdata->fd->ffpipe),buf,sizeof(buf));
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
                        continue;
					}
				}	
			}//while (1)

			//begin to green flash				
			if (0 != pinfo->cchan[0])
			{
				if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
        		{//actively report is not probitted and connect successfully
					if (0x04 == (pinfo->phasetype))
					{
						sinfo.time = 6;
					}
					else
					{
						sinfo.time = 3;
					}
					sinfo.conmode = *(rccdata->fd->contmode);//added on 20150529
					sinfo.color = 0x03;//green flash
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
					memcpy(rccdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            		memset(sibuf,0,sizeof(sibuf));
            		if (SUCCESS != status_info_report(sibuf,&sinfo))
            		{
            		#ifdef RCC_DEBUG
                		printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            		#endif
            		}
            		else
            		{
                		write(*(rccdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            		}
				}//actively report is not probitted and connect successfully
			}

			memset(&gtime,0,sizeof(gtime));
			memset(&gftime,0,sizeof(gftime));
			gettimeofday(&gftime,NULL);
			memset(&ytime,0,sizeof(ytime));
			memset(&rtime,0,sizeof(rtime));
			*(rccdata->fd->markbit) |= 0x0400;
			gft = 0;
			while (1)
			{
				if (SUCCESS != rcc_set_lamp_color(*(rccdata->fd->bbserial),pinfo->cchan,0x03))
				{
				#ifdef RCC_DEBUG
					printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					gettimeofday(&ct,NULL);
               		update_event_list(rccdata->fd->ec,rccdata->fd->el,1,15,ct.tv_sec,rccdata->fd->markbit);
               		if (SUCCESS != generate_event_file(rccdata->fd->ec,rccdata->fd->el,\
														rccdata->fd->softevent,rccdata->fd->markbit))
               		{
               		#ifdef RCC_DEBUG
                   		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
               		#endif
               		}
					*(rccdata->fd->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(rccdata->fd->sockfd,rccdata->cs,pinfo->cchan,0x03, \
												rccdata->fd->markbit))
				{
				#ifdef RCC_DEBUG
					printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}	
				timeout.tv_sec = 0;
				timeout.tv_usec = 500000;
				select(0,NULL,NULL,NULL,&timeout);
				if (SUCCESS != rcc_set_lamp_color(*(rccdata->fd->bbserial),pinfo->cchan,0x02))
				{
				#ifdef RCC_DEBUG
					printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					gettimeofday(&ct,NULL);
                	update_event_list(rccdata->fd->ec,rccdata->fd->el,1,15,ct.tv_sec,rccdata->fd->markbit);
                	if (SUCCESS != generate_event_file(rccdata->fd->ec,rccdata->fd->el,\
														rccdata->fd->softevent,rccdata->fd->markbit))
                	{
                	#ifdef RCC_DEBUG
                   		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                	}
					*(rccdata->fd->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(rccdata->fd->sockfd,rccdata->cs,pinfo->cchan,0x02, \
                                                   rccdata->fd->markbit))
                {
                #ifdef RCC_DEBUG
                   	printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
				timeout.tv_sec = 0;
				timeout.tv_usec = 500000;
				select(0,NULL,NULL,NULL,&timeout);
				gft += 1;

				if (0x04 == (pinfo->phasetype))
				{
					if (gft >= 6)//here add 3 sec yellow lamp(because person phase do not have yellow lamp)
						break;
				}
				else
				{
					if (gft >= 3)
						break;
				}	
			}	
			//end green flash
		
			if (1 == phcon)
            {
                *(rccdata->fd->markbit) &= 0xfbff;
                memset(&gtime,0,sizeof(gtime));
                gettimeofday(&gtime,NULL);
                memset(&gftime,0,sizeof(gftime));
                memset(&ytime,0,sizeof(ytime));
                memset(&rtime,0,sizeof(rtime));
                phcon = 0;
                sleep(10);
            }
	
			if (0x04 != (pinfo->phasetype))
			{//person phase do not have yellow lamp
				//Begin to set yellow lamp 
				if (1 == (pinfo->cpcexist))
				{//person channels exist in current phase
					//all person channels will be set red lamp;
					if (SUCCESS != rcc_set_lamp_color(*(rccdata->fd->bbserial),pinfo->cpchan,0x00))
					{
					#ifdef RCC_DEBUG
						printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						gettimeofday(&ct,NULL);
               			update_event_list(rccdata->fd->ec,rccdata->fd->el,1,15,ct.tv_sec,rccdata->fd->markbit);
               			if (SUCCESS != generate_event_file(rccdata->fd->ec,rccdata->fd->el,\
															rccdata->fd->softevent,rccdata->fd->markbit))
               			{
               			#ifdef RCC_DEBUG
                   			printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
               			#endif
               			}
						*(rccdata->fd->markbit) |= 0x0800;
					}
					if (SUCCESS != update_channel_status(rccdata->fd->sockfd,rccdata->cs,pinfo->cpchan,0x00, \
                                                   rccdata->fd->markbit))
               		{
               		#ifdef RCC_DEBUG
                   		printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
               		#endif
               		}	
					//other change channels will be set yellow lamp;
					if (SUCCESS != rcc_set_lamp_color(*(rccdata->fd->bbserial),pinfo->cnpchan,0x01))
					{
					#ifdef RCC_DEBUG
						printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						gettimeofday(&ct,NULL);
                   		update_event_list(rccdata->fd->ec,rccdata->fd->el,1,15,ct.tv_sec,rccdata->fd->markbit);
                   		if (SUCCESS != generate_event_file(rccdata->fd->ec,rccdata->fd->el,\
																rccdata->fd->softevent,rccdata->fd->markbit))
                   		{
                   		#ifdef RCC_DEBUG
                       		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                   		#endif
                   		}
						*(rccdata->fd->markbit) |= 0x0800;
					}
					if (SUCCESS != update_channel_status(rccdata->fd->sockfd,rccdata->cs,pinfo->cnpchan,0x01, \
                                                   rccdata->fd->markbit))
               		{
               		#ifdef RCC_DEBUG
                   		printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
               		#endif
               		}
				}//person channels exist in current phase
				else
				{//Not person channels in current phase
					if (SUCCESS != rcc_set_lamp_color(*(rccdata->fd->bbserial),pinfo->cchan,0x01))
					{
					#ifdef RCC_DEBUG
						printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						gettimeofday(&ct,NULL);
                   		update_event_list(rccdata->fd->ec,rccdata->fd->el,1,15,ct.tv_sec,rccdata->fd->markbit);
                   		if (SUCCESS != generate_event_file(rccdata->fd->ec,rccdata->fd->el,\
															rccdata->fd->softevent,rccdata->fd->markbit))
                   		{
                   		#ifdef RCC_DEBUG
                       		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                   		#endif
                   		}
						*(rccdata->fd->markbit) |= 0x0800;
					}
					if (SUCCESS != update_channel_status(rccdata->fd->sockfd,rccdata->cs,pinfo->cchan,0x01, \
                                                   rccdata->fd->markbit))
               		{
               		#ifdef RCC_DEBUG
                   		printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
               		#endif
               		}
				}//Not person channels in current phase

				if (0 != pinfo->cchan[0])
				{
					if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
        			{//actively report is not probitted and connect successfully
						sinfo.time = 3;
						sinfo.color = 0x01;
						sinfo.conmode = *(rccdata->fd->contmode);//added on 20150529
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
						memcpy(rccdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            			memset(sibuf,0,sizeof(sibuf));
            			if (SUCCESS != status_info_report(sibuf,&sinfo))
            			{
            			#ifdef RCC_DEBUG
                			printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            			#endif
            			}
            			else
            			{
                			write(*(rccdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            			}
					}//actively report is not probitted and connect successfully			
				}

				if ((*(rccdata->fd->markbit) & 0x0002) && (*(rccdata->fd->markbit) & 0x0010))
           		{
               		memset(&timedown,0,sizeof(timedown));
               		timedown.mode = *(rccdata->fd->contmode);
               		timedown.pattern = *(rccdata->fd->patternid);
               		timedown.lampcolor = 0x01;
					timedown.lamptime = 3;	
               		timedown.phaseid = pinfo->phaseid;
               		timedown.stageline = pinfo->stageline;
               		if (SUCCESS != timedown_data_to_conftool(rccdata->fd->sockfd,&timedown))
               		{
               		#ifdef RCC_DEBUG
                   		printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
               		#endif
               		}
           		}
				#ifdef EMBED_CONFIGURE_TOOL
                if (*(rccdata->fd->markbit2) & 0x0200)
                {   
                    memset(&timedown,0,sizeof(timedown));
                    timedown.mode = *(rccdata->fd->contmode);
                    timedown.pattern = *(rccdata->fd->patternid);
                    timedown.lampcolor = 0x01;
                    timedown.lamptime = 3;
                    timedown.phaseid = pinfo->phaseid;
                    timedown.stageline = pinfo->stageline;
                    if (SUCCESS != timedown_data_to_embed(rccdata->fd->sockfd,&timedown))
                    {
                    #ifdef RCC_DEBUG
                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                    #endif
                    }
                }
                #endif
				//send info to face board
				if (*(rccdata->fd->contmode) < 27)
                	fbdata[1] = *(rccdata->fd->contmode) + 1;
            	else
                	fbdata[1] = *(rccdata->fd->contmode);
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
					fbdata[4] = 3;
				}
           		if (!wait_write_serial(*(rccdata->fd->fbserial)))
           		{
		       		if (write(*(rccdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
               		{
               		#ifdef RCC_DEBUG
                   		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
               		#endif
						*(rccdata->fd->markbit) |= 0x0800;
                   		gettimeofday(&ct,NULL);
                   		update_event_list(rccdata->fd->ec,rccdata->fd->el,1,16,ct.tv_sec,rccdata->fd->markbit);
                   		if (SUCCESS != generate_event_file(rccdata->fd->ec,rccdata->fd->el,\
															rccdata->fd->softevent,rccdata->fd->markbit))
                   		{
                   		#ifdef RCC_DEBUG
                       		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                   		#endif
                   		}
               		}
           		}
           		else
           		{
           		#ifdef RCC_DEBUG
               		printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
           		#endif
           		}
				sendfaceInfoToBoard(rccdata->fd,fbdata);
				*(rccdata->fd->color) = 0x01;
				memset(&gtime,0,sizeof(gtime));
           		memset(&gftime,0,sizeof(gftime));
           		memset(&ytime,0,sizeof(ytime));
				gettimeofday(&ytime,NULL);
           		memset(&rtime,0,sizeof(rtime));
				sleep(3);
				//end set yellow lamp
			}//person phase do not have yellow lamp

			//Begin to set red lamp
			if (SUCCESS != rcc_set_lamp_color(*(rccdata->fd->bbserial),pinfo->cchan,0x00))
			{
			#ifdef RCC_DEBUG
				printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				gettimeofday(&ct,NULL);
               	update_event_list(rccdata->fd->ec,rccdata->fd->el,1,15,ct.tv_sec,rccdata->fd->markbit);
               	if (SUCCESS != generate_event_file(rccdata->fd->ec,rccdata->fd->el,\
													rccdata->fd->softevent,rccdata->fd->markbit))
               	{
               	#ifdef RCC_DEBUG
                   	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
               	#endif
               	}
				*(rccdata->fd->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(rccdata->fd->sockfd,rccdata->cs,pinfo->cchan,0x00, \
											rccdata->fd->markbit))
           	{
           	#ifdef RCC_DEBUG
               	printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
           	#endif
           	}
			*(rccdata->fd->color) = 0x00;

			if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
        	{//actively report is not probitted and connect successfully
				sinfo.time = 0;
				sinfo.color = 0x00;
				sinfo.conmode = *(rccdata->fd->contmode);//added on 20150529
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
				memcpy(rccdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            	memset(sibuf,0,sizeof(sibuf));
            	if (SUCCESS != status_info_report(sibuf,&sinfo))
            	{
            	#ifdef RCC_DEBUG
                	printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            	#endif
            	}
            	else
            	{
                	write(*(rccdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            	}
			}//actively report is not probitted and connect successfully

			//end red lamp,red lamp time is default 0 in the last two cycles;
			if (1 == phcon)
            {
				*(rccdata->fd->markbit) &= 0xfbff;
                memset(&gtime,0,sizeof(gtime));
                gettimeofday(&gtime,NULL);
                memset(&gftime,0,sizeof(gftime));
                memset(&ytime,0,sizeof(ytime));
                memset(&rtime,0,sizeof(rtime));
                phcon = 0;
                sleep(10);
            }

			slnum += 1;
			*(rccdata->fd->slnum) = slnum;
			*(rccdata->fd->stageid) = rccdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
			if (0 == (rccdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId))
			{
				cycarr = 1;
				slnum = 0;
				*(rccdata->fd->slnum) = 0;
				*(rccdata->fd->stageid) = rccdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
			}				
			continue;
		}//end pattern ahead two cycles		
		if (0x04 == (pinfo->phasetype))
		{//person phase
#if 0
		#ifdef RCC_DEBUG
			printf("**********Begin to pass person phase,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		    *(rccdata->fd->color) = 0x02;
			*(rccdata->fd->markbit) &= 0xfbff;	
			if (SUCCESS != rcc_set_lamp_color(*(rccdata->fd->bbserial),pinfo->chan,0x02))
			{
			#ifdef RCC_DEBUG
				printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				gettimeofday(&ct,NULL);
				update_event_list(rccdata->fd->ec,rccdata->fd->el,1,15,ct.tv_sec,rccdata->fd->markbit);
				if (SUCCESS != generate_event_file(rccdata->fd->ec,rccdata->fd->el,\
													rccdata->fd->softevent,rccdata->fd->markbit))
				{
				#ifdef RCC_DEBUG
					printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				*(rccdata->fd->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(rccdata->fd->sockfd,rccdata->cs,pinfo->chan,0x02, \
												rccdata->fd->markbit))
			{
			#ifdef RCC_DEBUG
				printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
			memset(&gtime,0,sizeof(gtime));
			gettimeofday(&gtime,NULL);
			memset(&gftime,0,sizeof(gftime));
			memset(&ytime,0,sizeof(ytime));
			memset(&rtime,0,sizeof(rtime));
			if (0 == pinfo->cchan[0])
			{
				if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
				{//if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
					sinfo.time = pinfo->pgtime + pinfo->pctime + pinfo->rtime;	
					sinfo.color = 0x02;
					sinfo.conmode = *(rccdata->fd->contmode);//added on 20150529
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
					memcpy(rccdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            		memset(sibuf,0,sizeof(sibuf));
            		if (SUCCESS != status_info_report(sibuf,&sinfo))
            		{
            		#ifdef RCC_DEBUG
                		printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            		#endif
            		}
            		else
            		{
                		write(*(rccdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            		}	
				}//if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
			}
			else
			{
				if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
				{//if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
					sinfo.time = pinfo->pgtime;	
					sinfo.color = 0x02;
					sinfo.conmode = *(rccdata->fd->contmode);//added on 20150529
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
					memcpy(rccdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            		memset(sibuf,0,sizeof(sibuf));
            		if (SUCCESS != status_info_report(sibuf,&sinfo))
            		{
            		#ifdef RCC_DEBUG
                		printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            		#endif
            		}
            		else
            		{
                		write(*(rccdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            		}	
				}//if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
			}

			if ((*(rccdata->fd->markbit) & 0x0002) && (*(rccdata->fd->markbit) & 0x0010))
			{
				memset(&timedown,0,sizeof(timedown));
				timedown.mode = *(rccdata->fd->contmode);
				timedown.pattern = *(rccdata->fd->patternid);
				timedown.lampcolor = 0x02;
				timedown.lamptime = pinfo->pgtime + pinfo->pctime;
				timedown.phaseid = pinfo->phaseid;
				timedown.stageline = pinfo->stageline;
				if (SUCCESS != timedown_data_to_conftool(rccdata->fd->sockfd,&timedown))
				{
				#ifdef RCC_DEBUG
					printf("send time err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#ifdef EMBED_CONFIGURE_TOOL
			if (*(rccdata->fd->markbit2) & 0x0200)
			{   
				memset(&timedown,0,sizeof(timedown));
                timedown.mode = *(rccdata->fd->contmode);
                timedown.pattern = *(rccdata->fd->patternid);
                timedown.lampcolor = 0x02;
                timedown.lamptime = pinfo->pgtime + pinfo->pctime;
                timedown.phaseid = pinfo->phaseid;
                timedown.stageline = pinfo->stageline;	 
				if (SUCCESS != timedown_data_to_embed(rccdata->fd->sockfd,&timedown))
				{
				#ifdef RCC_DEBUG
					printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#endif
			//send time data to face board;
			if (*(rccdata->fd->contmode) < 27)
                fbdata[1] = *(rccdata->fd->contmode) + 1;
            else
                fbdata[1] = *(rccdata->fd->contmode);
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
				fbdata[4] = pinfo->pgtime + pinfo->pctime;
			}
			if (!wait_write_serial(*(rccdata->fd->fbserial))) 
			{
				if (write(*(rccdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
				{
				#ifdef RCC_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(rccdata->fd->markbit) |= 0x0800;
					gettimeofday(&ct,NULL);
					update_event_list(rccdata->fd->ec,rccdata->fd->el,1,16,ct.tv_sec,rccdata->fd->markbit);
					if (SUCCESS != generate_event_file(rccdata->fd->ec,rccdata->fd->el,\
															rccdata->fd->softevent,rccdata->fd->markbit))
					{
					#ifdef RCC_DEBUG
						printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
				}
			}
			else
			{
			#ifdef RCC_DEBUG
				printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
		//	sleep(pinfo->pgtime);
			sltime = pinfo->pgtime;
			while (1)
			{//while (1)
				FD_ZERO(&nRead);
				FD_SET(*(rccdata->fd->ffpipe),&nRead);	
				lasttime.tv_sec = 0;
				lasttime.tv_usec = 0;
				gettimeofday(&lasttime,NULL);
				mtime.tv_sec = sltime;
				mtime.tv_usec = 0;
				int mret = select(*(rccdata->fd->ffpipe)+1,&nRead,NULL,NULL,&mtime);
				if (mret < 0)
				{
				#ifdef RCC_DEBUG
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
					if (FD_ISSET(*(rccdata->fd->ffpipe),&nRead))
					{
						memset(buf,0,sizeof(buf));
						read(*(rccdata->fd->ffpipe),buf,sizeof(buf));
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
                        continue;
					}
				}	
			}//while (1)

			//green flash of person phase
			if ((0 != pinfo->cchan[0]) && (pinfo->pctime > 0))
			{
				if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
				{//if ((!(*(ppsdata->fd->markbit) & 0x1000)) && (!(*(ppsdata->fd->markbit) & 0x8000)))
					sinfo.time = pinfo->pctime;	
					sinfo.conmode = *(rccdata->fd->contmode);//added on 20150529
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
					memcpy(rccdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
					memset(sibuf,0,sizeof(sibuf));
					if (SUCCESS != status_info_report(sibuf,&sinfo))
					{
					#ifdef RCC_DEBUG
						printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					else
					{
						write(*(rccdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
					}	
				}//if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
			}

			memset(&gtime,0,sizeof(gtime));
			memset(&gftime,0,sizeof(gftime));
			gettimeofday(&gftime,NULL);
			memset(&ytime,0,sizeof(ytime));
			memset(&rtime,0,sizeof(rtime));
			gft = 0;
			*(rccdata->fd->markbit) |= 0x0400;
			while (1)
			{
				if (SUCCESS != rcc_set_lamp_color(*(rccdata->fd->bbserial),pinfo->cchan,0x03))
				{
				#ifdef RCC_DEBUG
					printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					gettimeofday(&ct,NULL);
                    update_event_list(rccdata->fd->ec,rccdata->fd->el,1,15,ct.tv_sec,rccdata->fd->markbit);
                    if (SUCCESS != generate_event_file(rccdata->fd->ec,rccdata->fd->el,\
															rccdata->fd->softevent,rccdata->fd->markbit))
                    {
                    #ifdef RCC_DEBUG
                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                    }
					*(rccdata->fd->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(rccdata->fd->sockfd,rccdata->cs,pinfo->cchan,0x03, \
                                                    rccdata->fd->markbit))
            	{
            	#ifdef RCC_DEBUG
                	printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
            	#endif
            	}
				timeout.tv_sec = 0;
				timeout.tv_usec = 500000;
				select(0,NULL,NULL,NULL,&timeout);

				if (SUCCESS != rcc_set_lamp_color(*(rccdata->fd->bbserial),pinfo->cchan,0x02))
				{
				#ifdef RCC_DEBUG
					printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					gettimeofday(&ct,NULL);
                    update_event_list(rccdata->fd->ec,rccdata->fd->el,1,15,ct.tv_sec,rccdata->fd->markbit);
                    if (SUCCESS != generate_event_file(rccdata->fd->ec,rccdata->fd->el,\
														rccdata->fd->softevent,rccdata->fd->markbit))
                    {
                    #ifdef RCC_DEBUG
                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                    }
					*(rccdata->fd->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(rccdata->fd->sockfd,rccdata->cs,pinfo->cchan,0x02, \
                                                    rccdata->fd->markbit))
                {
                #ifdef RCC_DEBUG
                    printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }	
				timeout.tv_sec = 0;
				timeout.tv_usec = 500000;
				select(0,NULL,NULL,NULL,&timeout);

				gft += 1;
				if (gft >= (pinfo->pctime))
					break;
			}
			//end green flash

			if (1 == phcon)
            {
                *(rccdata->fd->markbit) &= 0xfbff;
                memset(&gtime,0,sizeof(gtime));
                gettimeofday(&gtime,NULL);
                memset(&gftime,0,sizeof(gftime));
                memset(&ytime,0,sizeof(ytime));
                memset(&rtime,0,sizeof(rtime));
                phcon = 0;
                sleep(10);
            }

			//red lamp of person phase,note:person channel do not have yellow color;
			if (SUCCESS != rcc_set_lamp_color(*(rccdata->fd->bbserial),pinfo->cchan,0x00))
			{
			#ifdef RCC_DEBUG
				printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				gettimeofday(&ct,NULL);
                update_event_list(rccdata->fd->ec,rccdata->fd->el,1,15,ct.tv_sec,rccdata->fd->markbit);
                if (SUCCESS != generate_event_file(rccdata->fd->ec,rccdata->fd->el,\
													rccdata->fd->softevent,rccdata->fd->markbit))
                {
                #ifdef RCC_DEBUG
                    printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
				*(rccdata->fd->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(rccdata->fd->sockfd,rccdata->cs,pinfo->cchan,0x00, \
                                                    rccdata->fd->markbit))
            {
            #ifdef RCC_DEBUG
                printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
			*(rccdata->fd->color) = 0x00;

			if (0 != pinfo->cchan[0])
			{
				if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
				{//if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
					sinfo.time = pinfo->rtime;	
					sinfo.conmode = *(rccdata->fd->contmode);//added on 20150529
					sinfo.color = 0x00;
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
					memcpy(rccdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
					memset(sibuf,0,sizeof(sibuf));
            		if (SUCCESS != status_info_report(sibuf,&sinfo))
            		{
            		#ifdef RCC_DEBUG
                		printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            		#endif
            		}
            		else
            		{
                		write(*(rccdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            		}	
				}//if ((!(*(ppsdata->fd->markbit) & 0x1000)) && (!(*(ppsdata->fd->markbit) & 0x8000)))
			}

			if (pinfo->rtime > 0)
			{
				memset(&gtime,0,sizeof(gtime));
            	memset(&gftime,0,sizeof(gftime));
            	memset(&ytime,0,sizeof(ytime));
            	memset(&rtime,0,sizeof(rtime));
            	gettimeofday(&rtime,NULL);

				if ((*(rccdata->fd->markbit) & 0x0002) && (*(rccdata->fd->markbit) & 0x0010))
            	{
               		memset(&timedown,0,sizeof(timedown));
               		timedown.mode = *(rccdata->fd->contmode);
               		timedown.pattern = *(rccdata->fd->patternid);
               		timedown.lampcolor = 0x00;
               		timedown.lamptime = pinfo->rtime;
               		timedown.phaseid = pinfo->phaseid;
               		timedown.stageline = pinfo->stageline;
               		if (SUCCESS != timedown_data_to_conftool(rccdata->fd->sockfd,&timedown))
               		{
               		#ifdef RCC_DEBUG
                   		printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
               		#endif
               		}
            	}
				#ifdef EMBED_CONFIGURE_TOOL
                if (*(rccdata->fd->markbit2) & 0x0200)
                {   
                    memset(&timedown,0,sizeof(timedown));
                    timedown.mode = *(rccdata->fd->contmode);
                    timedown.pattern = *(rccdata->fd->patternid);
                    timedown.lampcolor = 0x00;
                    timedown.lamptime = pinfo->rtime;
                    timedown.phaseid = pinfo->phaseid;
                    timedown.stageline = pinfo->stageline; 
                    if (SUCCESS != timedown_data_to_embed(rccdata->fd->sockfd,&timedown))
                    {
                    #ifdef RCC_DEBUG
                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                    #endif
                    }
                }
                #endif

				//send info to face board
				if (*(rccdata->fd->contmode) < 27)
                	fbdata[1] = *(rccdata->fd->contmode) + 1;
            	else
                	fbdata[1] = *(rccdata->fd->contmode);
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
           		if (!wait_write_serial(*(rccdata->fd->fbserial)))
           		{
               		if (write(*(rccdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
               		{
               		#ifdef RCC_DEBUG
                   		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
               		#endif
						*(rccdata->fd->markbit) |= 0x0800;
                   		gettimeofday(&ct,NULL);
                   		update_event_list(rccdata->fd->ec,rccdata->fd->el,1,16,ct.tv_sec,rccdata->fd->markbit);
                   		if (SUCCESS != generate_event_file(rccdata->fd->ec,rccdata->fd->el,\
																rccdata->fd->softevent,rccdata->fd->markbit))
                   		{
                   		#ifdef RCC_DEBUG
                       		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                   		#endif
                   		}
               		}
           		}
           		else
           		{
           		#ifdef RCC_DEBUG
               		printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
           		#endif
           		}
			
				sleep(pinfo->rtime);
			}
			//end red lamp
#endif
			if (1 == phcon)
            {
				*(rccdata->fd->markbit) &= 0xfbff;
                memset(&gtime,0,sizeof(gtime));
                gettimeofday(&gtime,NULL);
                memset(&gftime,0,sizeof(gftime));
                memset(&ytime,0,sizeof(ytime));
                memset(&rtime,0,sizeof(rtime));
                phcon = 0;
                sleep(10);
            }

			slnum += 1;
			*(rccdata->fd->slnum) = slnum;
			*(rccdata->fd->stageid) = rccdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
			if (0 == (rccdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId))
			{
				cycarr = 1;
				slnum = 0;
				*(rccdata->fd->slnum) = 0;
				*(rccdata->fd->stageid) = rccdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
			}

			continue;
		}//person phase
		else if (0x01 == (pinfo->phasetype))
		{//vehicle phase
		#ifdef RCC_DEBUG
			printf("*************Begin to pass vehicle phase,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			unsigned char		leave = 0;
			*(rccdata->fd->color) = 0x02;
			*(rccdata->fd->markbit) &= 0xfbff;
			if (SUCCESS != rcc_set_lamp_color(*(rccdata->fd->bbserial),pinfo->chan,0x02))
			{
			#ifdef RCC_DEBUG
				printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				gettimeofday(&ct,NULL);
                update_event_list(rccdata->fd->ec,rccdata->fd->el,1,15,ct.tv_sec,rccdata->fd->markbit);
                if (SUCCESS != generate_event_file(rccdata->fd->ec,rccdata->fd->el,\
														rccdata->fd->softevent,rccdata->fd->markbit))
                {
                #ifdef RCC_DEBUG
                    printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
                *(rccdata->fd->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(rccdata->fd->sockfd,rccdata->cs,pinfo->chan,0x02, \
                                                    rccdata->fd->markbit))
            {
            #ifdef RCC_DEBUG
                printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
            memset(&gtime,0,sizeof(gtime));
            gettimeofday(&gtime,NULL);
            memset(&gftime,0,sizeof(gftime));
            memset(&ytime,0,sizeof(ytime));
            memset(&rtime,0,sizeof(rtime));

			if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
			{//if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
				sinfo.time = pinfo->gtime + pinfo->gftime;	
				sinfo.color = 0x02;
				sinfo.conmode = *(rccdata->fd->contmode);//added on 20150529
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
				memcpy(rccdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
				memset(sibuf,0,sizeof(sibuf));
				if (SUCCESS != status_info_report(sibuf,&sinfo))
				{	
				#ifdef RCC_DEBUG
					printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					write(*(rccdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
				}	
			}//if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))	

			if ((*(rccdata->fd->markbit) & 0x0002) && (*(rccdata->fd->markbit) & 0x0010))
            {
                memset(&timedown,0,sizeof(timedown));
                timedown.mode = *(rccdata->fd->contmode);
                timedown.pattern = *(rccdata->fd->patternid);
                timedown.lampcolor = 0x02;
                timedown.lamptime = pinfo->gtime + pinfo->gftime;
                timedown.phaseid = pinfo->phaseid;
                timedown.stageline = pinfo->stageline;
                if (SUCCESS != timedown_data_to_conftool(rccdata->fd->sockfd,&timedown))
                {
                #ifdef RCC_DEBUG
                    printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                #endif
                }
            }
			#ifdef EMBED_CONFIGURE_TOOL
			if (*(rccdata->fd->markbit2) & 0x0200)
			{   
				memset(&timedown,0,sizeof(timedown));
                timedown.mode = *(rccdata->fd->contmode);
                timedown.pattern = *(rccdata->fd->patternid);
                timedown.lampcolor = 0x02;
                timedown.lamptime = pinfo->gtime + pinfo->gftime;
                timedown.phaseid = pinfo->phaseid;
                timedown.stageline = pinfo->stageline;	 
				if (SUCCESS != timedown_data_to_embed(rccdata->fd->sockfd,&timedown))
				{
				#ifdef RCC_DEBUG
					printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#endif
			//send info to face board
			if (*(rccdata->fd->contmode) < 27)
                fbdata[1] = *(rccdata->fd->contmode) + 1;
            else
                fbdata[1] = *(rccdata->fd->contmode);
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
            	fbdata[4] = pinfo->gtime + pinfo->gftime;
			}
            if (!wait_write_serial(*(rccdata->fd->fbserial)))
            {
                if (write(*(rccdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
                {
                #ifdef RCC_DEBUG
                    printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
					*(rccdata->fd->markbit) |= 0x0800;
                    gettimeofday(&ct,NULL);
                    update_event_list(rccdata->fd->ec,rccdata->fd->el,1,16,ct.tv_sec,rccdata->fd->markbit);
                    if (SUCCESS != generate_event_file(rccdata->fd->ec,rccdata->fd->el,\
															rccdata->fd->softevent,rccdata->fd->markbit))
                    {
                    #ifdef RCC_DEBUG
                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                    }
                }
            }
            else
            {
            #ifdef RCC_DEBUG
                printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
			sendfaceInfoToBoard(rccdata->fd,fbdata);
			//clean pipe
			while (1)
			{
				memset(buf,0,sizeof(buf));
				if (read(*(rccdata->fd->flowpipe),buf,sizeof(buf)) <= 0)
					break;
			}//end clean pipe
			#ifdef RCC_DEBUG
			printf("Begin to monitor vehicle,time: %d,File: %s,Line: %d\n",pinfo->gtime,__FILE__,__LINE__);
			#endif
			mt = pinfo->gtime;
			int maxv = 0;
			personyes = 0;
			while (1)
			{//monitor loop
				FD_ZERO(&nRead);
				FD_SET(*(rccdata->fd->flowpipe),&nRead);
//				FD_SET(*(rccdata->fd->ffpipe),&nRead);
				lasttime.tv_sec = 0;
				lasttime.tv_usec = 0;
				gettimeofday(&lasttime,NULL);
				mtime.tv_sec = mt;
				mtime.tv_usec = 0;
//				maxv = MAX(*(rccdata->fd->flowpipe),*(rccdata->fd->ffpipe));
				maxv = *(rccdata->fd->flowpipe);
				int mret = select(maxv+1,&nRead,NULL,NULL,&mtime);
				if (mret < 0)
				{
				#ifdef RCC_DEBUG
					printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					rcc_end_part_child_thread();
					return;
				}
				if (0 == mret)
				{//time out
					if (*(rccdata->fd->markbit) & 0x0001)
					{
			//			cycarr = 1;
						leave = 1;
						break;
					}
					if ((*(rccdata->fd->markbit) & 0x0100) && (0 == endahead))
					{
						leave = 1;
                        break;
					}
					
					break;
				}//time out
				if (mret > 0)
				{//have person request
					if (FD_ISSET(*(rccdata->fd->flowpipe),&nRead))
					{
						memset(buf,0,sizeof(buf));
						pbuf = buf;
						num = read(*(rccdata->fd->flowpipe),buf,sizeof(buf));
						if (num > sizeof(buf))
                        	continue;
						if (num > 0)
						{
							mark = 0;
							personyes = 0;
							while (1)
							{//inline while loop
								if (mark >= num)
									break;
								if ((0xF1 == *(pbuf+mark)) && (0xED == *(pbuf+mark+5)))
                                {
                                    if (0x04 == *(pbuf+mark+1))
                                    {//have person request
										deteid = *(pbuf+mark+2);
                                    	if (49 == deteid)
										{//control of all red
											personyes = 1;
											break;
										}//control of all red    
                                    }//have person request
                                    mark += 6;
                                    continue;//Fit data,break directly
                                }
                                else
                                {
                                    mark += 1;
                                    continue;
                                }
							}//inline while loop
							if (1 == personyes)
							{
								break;
							}//if (1 == personyes)
							else
							{
								nowtime.tv_sec = 0;
								nowtime.tv_usec = 0;
								gettimeofday(&nowtime,NULL);
								leatime = nowtime.tv_sec - lasttime.tv_sec;
								mt -= leatime;
								continue;
							}
						}//num >= 0
						else
						{//num < 0
							nowtime.tv_sec = 0;
							nowtime.tv_usec = 0;
							gettimeofday(&nowtime,NULL);
							leatime = nowtime.tv_sec - lasttime.tv_sec;
							mt -= leatime;
							continue;
						}//num < 0
					}//else if (FD_ISSET(*(rccdata->fd->flowpipe),&nRead))
					else
					{
						nowtime.tv_sec = 0;
						nowtime.tv_usec = 0;
						gettimeofday(&nowtime,NULL);
						leatime = nowtime.tv_sec - lasttime.tv_sec;
						mt -= leatime;
						continue;
					}
				}//have person request	
			}//monitor loop of person button

			#ifdef RCC_DEBUG	
			printf("begin to green flash,personyes: %d,FIle:%s,LIne:%d\n",personyes,__FILE__,__LINE__);
			#endif
			//begin to green flash
			if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
			{//if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
				sinfo.time = pinfo->gftime;	
				sinfo.color = 0x03;
				sinfo.conmode = *(rccdata->fd->contmode);//added on 20150529
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == pinfo->chan[i])
						break;
					for (j = 0; j < sinfo.chans; j++)
					{
						if (0 == sinfo.csta[j])
							break;
						tcsta = sinfo.csta[j];
						tcsta &= 0xfc;
						tcsta >>= 2;
						tcsta &= 0x3f;
						if (tcsta == pinfo->chan[i])
						{
							sinfo.csta[j] &= 0xfc;
							sinfo.csta[j] |= 0x03;
							break;
						}
					}
				}
				memcpy(rccdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
				memset(sibuf,0,sizeof(sibuf));
				if (SUCCESS != status_info_report(sibuf,&sinfo))
				{
				#ifdef RCC_DEBUG
					printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					write(*(rccdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
				}	
			}//if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))

			memset(&gtime,0,sizeof(gtime));
			memset(&gftime,0,sizeof(gftime));
			gettimeofday(&gftime,NULL);
			memset(&ytime,0,sizeof(ytime));
			memset(&rtime,0,sizeof(rtime));
			*(rccdata->fd->markbit) |= 0x0400;
			gft = 0;
			while (1)
			{
				if (SUCCESS != rcc_set_lamp_color(*(rccdata->fd->bbserial),pinfo->chan,0x03))
				{
				#ifdef RCC_DEBUG
					printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					gettimeofday(&ct,NULL);
                    update_event_list(rccdata->fd->ec,rccdata->fd->el,1,15,ct.tv_sec,rccdata->fd->markbit);
                    if (SUCCESS != generate_event_file(rccdata->fd->ec,rccdata->fd->el,\
															rccdata->fd->softevent,rccdata->fd->markbit))
                    {
                    #ifdef RCC_DEBUG
                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                    }
					*(rccdata->fd->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(rccdata->fd->sockfd,rccdata->cs,pinfo->chan,0x03, \
                                                    rccdata->fd->markbit))
            	{
            	#ifdef RCC_DEBUG
                	printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
            	#endif
            	}
				timeout.tv_sec = 0;
				timeout.tv_usec = 500000;
				select(0,NULL,NULL,NULL,&timeout);

				if (SUCCESS != rcc_set_lamp_color(*(rccdata->fd->bbserial),pinfo->chan,0x02))
				{
				#ifdef RCC_DEBUG
					printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					gettimeofday(&ct,NULL);
                    update_event_list(rccdata->fd->ec,rccdata->fd->el,1,15,ct.tv_sec,rccdata->fd->markbit);
                    if (SUCCESS != generate_event_file(rccdata->fd->ec,rccdata->fd->el,\
														rccdata->fd->softevent,rccdata->fd->markbit))
                    {
                    #ifdef RCC_DEBUG
                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                    }
					*(rccdata->fd->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(rccdata->fd->sockfd,rccdata->cs,pinfo->chan,0x02, \
                                                    rccdata->fd->markbit))
                {
                #ifdef RCC_DEBUG
                    printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }	
				timeout.tv_sec = 0;
				timeout.tv_usec = 500000;
				select(0,NULL,NULL,NULL,&timeout);

				gft += 1;
				if (gft >= (pinfo->gftime))
					break;
			}
			//end green flash

			if (1 == phcon)
            {
                *(rccdata->fd->markbit) &= 0xfbff;
                memset(&gtime,0,sizeof(gtime));
                gettimeofday(&gtime,NULL);
                memset(&gftime,0,sizeof(gftime));
                memset(&ytime,0,sizeof(ytime));
                memset(&rtime,0,sizeof(rtime));
                phcon = 0;
                sleep(10);
            }
			#ifdef RCC_DEBUG 
            printf("begin to yellow lamp,FIle:%s,LIne:%d\n",__FILE__,__LINE__);
            #endif
			//begin yellow lamp
			if (1 == (pinfo->pcexist))
			{
				if (SUCCESS != rcc_set_lamp_color(*(rccdata->fd->bbserial),pinfo->pchan,0x00))
				{
				#ifdef RCC_DEBUG
					printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					gettimeofday(&ct,NULL);
                	update_event_list(rccdata->fd->ec,rccdata->fd->el,1,15,ct.tv_sec,rccdata->fd->markbit);
                	if (SUCCESS != generate_event_file(rccdata->fd->ec,rccdata->fd->el,\
														rccdata->fd->softevent,rccdata->fd->markbit))
                	{
                	#ifdef RCC_DEBUG
                   		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                	}
					*(rccdata->fd->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(rccdata->fd->sockfd,rccdata->cs,pinfo->pchan,0x00, \
                                                   rccdata->fd->markbit))
            	{
            	#ifdef RCC_DEBUG
                	printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
            	#endif
            	}
				if (SUCCESS != rcc_set_lamp_color(*(rccdata->fd->bbserial),pinfo->npchan,0x01))
                {
                #ifdef RCC_DEBUG
                    printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                    gettimeofday(&ct,NULL);
                    update_event_list(rccdata->fd->ec,rccdata->fd->el,1,15,ct.tv_sec,rccdata->fd->markbit);
                    if (SUCCESS != generate_event_file(rccdata->fd->ec,rccdata->fd->el,\
															rccdata->fd->softevent,rccdata->fd->markbit))
                    {
                    #ifdef RCC_DEBUG
                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                    }
                    *(rccdata->fd->markbit) |= 0x0800;
                }
                if (SUCCESS != update_channel_status(rccdata->fd->sockfd,rccdata->cs,pinfo->npchan,0x01, \
                                                    rccdata->fd->markbit))
                {
                #ifdef RCC_DEBUG
                    printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }		
			}
			else
			{
				if (SUCCESS != rcc_set_lamp_color(*(rccdata->fd->bbserial),pinfo->chan,0x01))
                {
                #ifdef RCC_DEBUG
                    printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                    gettimeofday(&ct,NULL);
                    update_event_list(rccdata->fd->ec,rccdata->fd->el,1,15,ct.tv_sec,rccdata->fd->markbit);
                    if (SUCCESS != generate_event_file(rccdata->fd->ec,rccdata->fd->el,\
														rccdata->fd->softevent,rccdata->fd->markbit))
                    {
                    #ifdef RCC_DEBUG
                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                    }
                    *(rccdata->fd->markbit) |= 0x0800;
                }
                if (SUCCESS != update_channel_status(rccdata->fd->sockfd,rccdata->cs,pinfo->chan,0x01, \
                                                    rccdata->fd->markbit))
                {
                #ifdef RCC_DEBUG
                    printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
			}
			*(rccdata->fd->color) = 0x01;
			memset(&gtime,0,sizeof(gtime));
            memset(&gftime,0,sizeof(gftime));
            memset(&ytime,0,sizeof(ytime));
			gettimeofday(&ytime,NULL);
            memset(&rtime,0,sizeof(rtime));

			if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
			{//if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
				sinfo.time = pinfo->ytime;	
				sinfo.color = 0x01;
				sinfo.conmode = *(rccdata->fd->contmode);//added on 20150529
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == pinfo->npchan[i])
						break;
					for (j = 0; j < sinfo.chans; j++)
					{
						if (0 == sinfo.csta[j])
							break;
						tcsta = sinfo.csta[j];
						tcsta &= 0xfc;
						tcsta >>= 2;
						tcsta &= 0x3f;
						if (tcsta == pinfo->npchan[i])
						{
							sinfo.csta[j] &= 0xfc;
							sinfo.csta[j] |= 0x01;
							break;
						}
					}
				}
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == pinfo->pchan[i])
						break;
					for (j = 0; j < sinfo.chans; j++)
					{
						if (0 == sinfo.csta[j])
							break;
						tcsta = sinfo.csta[j];
						tcsta &= 0xfc;
						tcsta >>= 2;
						tcsta &= 0x3f;
						if (tcsta == pinfo->pchan[i])
						{
							sinfo.csta[j] &= 0xfc;
							break;
						}
					}
				}
				memcpy(rccdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
				memset(sibuf,0,sizeof(sibuf));
				if (SUCCESS != status_info_report(sibuf,&sinfo))
				{
				#ifdef RCC_DEBUG
					printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					write(*(rccdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
				}	
			}//if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))

			if ((*(rccdata->fd->markbit) & 0x0002) && (*(rccdata->fd->markbit) & 0x0010))
            {
            	memset(&timedown,0,sizeof(timedown));
            	timedown.mode = *(rccdata->fd->contmode);
            	timedown.pattern = *(rccdata->fd->patternid);
            	timedown.lampcolor = 0x01;
                timedown.lamptime = pinfo->ytime;
               	timedown.phaseid = pinfo->phaseid;
               	timedown.stageline = pinfo->stageline;
               	if (SUCCESS != timedown_data_to_conftool(rccdata->fd->sockfd,&timedown))
               	{
               	#ifdef RCC_DEBUG
                	printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
               	#endif
               	}
            }
			#ifdef EMBED_CONFIGURE_TOOL
			if (*(rccdata->fd->markbit2) & 0x0200)
			{   
				memset(&timedown,0,sizeof(timedown));
                timedown.mode = *(rccdata->fd->contmode);
                timedown.pattern = *(rccdata->fd->patternid);
                timedown.lampcolor = 0x01;
                timedown.lamptime = pinfo->ytime;
                timedown.phaseid = pinfo->phaseid;
                timedown.stageline = pinfo->stageline;	 
				if (SUCCESS != timedown_data_to_embed(rccdata->fd->sockfd,&timedown))
				{
				#ifdef RCC_DEBUG
					printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#endif
			//send info to face board
			if (*(rccdata->fd->contmode) < 27)
                fbdata[1] = *(rccdata->fd->contmode) + 1;
            else
                fbdata[1] = *(rccdata->fd->contmode);
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
           		fbdata[4] = pinfo->ytime;
			}
           	if (!wait_write_serial(*(rccdata->fd->fbserial)))
           	{
             	if (write(*(rccdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
             	{
             	#ifdef RCC_DEBUG
               		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
             	#endif
					*(rccdata->fd->markbit) |= 0x0800;
                   	gettimeofday(&ct,NULL);
                   	update_event_list(rccdata->fd->ec,rccdata->fd->el,1,16,ct.tv_sec,rccdata->fd->markbit);
                   	if (SUCCESS != generate_event_file(rccdata->fd->ec,rccdata->fd->el,\
														rccdata->fd->softevent,rccdata->fd->markbit))
                   	{
                   	#ifdef RCC_DEBUG
                    	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                   	#endif
                   	}
               	}
           	}
           	else
           	{
           	#ifdef RCC_DEBUG
            	printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
           	#endif
           	}
			sendfaceInfoToBoard(rccdata->fd,fbdata);
			sleep(pinfo->ytime);
			//end yellow lamp

			#ifdef RCC_DEBUG 
            printf("begin to red lamp,FIle:%s,LIne:%d\n",__FILE__,__LINE__);
            #endif
			//begin to red lamp
			if (SUCCESS != rcc_set_lamp_color(*(rccdata->fd->bbserial),pinfo->chan,0x00))
			{
			#ifdef RCC_DEBUG
				printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				gettimeofday(&ct,NULL);
                update_event_list(rccdata->fd->ec,rccdata->fd->el,1,15,ct.tv_sec,rccdata->fd->markbit);
                if (SUCCESS != generate_event_file(rccdata->fd->ec,rccdata->fd->el,\
													rccdata->fd->softevent,rccdata->fd->markbit))
                {
                #ifdef RCC_DEBUG
                    printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
				*(rccdata->fd->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(rccdata->fd->sockfd,rccdata->cs,pinfo->chan,0x00, \
                                                    rccdata->fd->markbit))
            {
            #ifdef RCC_DEBUG
                printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }

			if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
			{//if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))
				sinfo.time = pinfo->rtime;	
				sinfo.color = 0x00;
				sinfo.conmode = *(rccdata->fd->contmode);//added on 20150529
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == pinfo->chan[i])
						break;
					for (j = 0; j < sinfo.chans; j++)
					{
						if (0 == sinfo.csta[j])
							break;
						tcsta = sinfo.csta[j];
						tcsta &= 0xfc;
						tcsta >>= 2;
						tcsta &= 0x3f;
						if (tcsta == pinfo->chan[i])
						{
							sinfo.csta[j] &= 0xfc;
							break;
						}
					}
				}
				memcpy(rccdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
				memset(sibuf,0,sizeof(sibuf));
				if (SUCCESS != status_info_report(sibuf,&sinfo))
				{
				#ifdef RCC_DEBUG
					printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					write(*(rccdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
				}	
			}//if ((!(*(rccdata->fd->markbit) & 0x1000)) && (!(*(rccdata->fd->markbit) & 0x8000)))			

			*(rccdata->fd->color) = 0x00;

			memset(&gtime,0,sizeof(gtime));
			memset(&gftime,0,sizeof(gftime));
			memset(&ytime,0,sizeof(ytime));
			memset(&rtime,0,sizeof(rtime));
			gettimeofday(&rtime,NULL);

			if ((*(rccdata->fd->markbit) & 0x0002) && (*(rccdata->fd->markbit) & 0x0010))
			{
				memset(&timedown,0,sizeof(timedown));
				timedown.mode = *(rccdata->fd->contmode);
				timedown.pattern = *(rccdata->fd->patternid);
				timedown.lampcolor = 0x00;
				timedown.lamptime = pinfo->rtime;
				timedown.phaseid = pinfo->phaseid;
				timedown.stageline = pinfo->stageline;
				if (SUCCESS != timedown_data_to_conftool(rccdata->fd->sockfd,&timedown))
				{
				#ifdef RCC_DEBUG
					printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#ifdef EMBED_CONFIGURE_TOOL
			if (*(rccdata->fd->markbit2) & 0x0200)
			{   
				memset(&timedown,0,sizeof(timedown));
				timedown.mode = *(rccdata->fd->contmode);
				timedown.pattern = *(rccdata->fd->patternid);
				timedown.lampcolor = 0x00;
				timedown.lamptime = pinfo->rtime;
				timedown.phaseid = pinfo->phaseid;
				timedown.stageline = pinfo->stageline; 
				if (SUCCESS != timedown_data_to_embed(rccdata->fd->sockfd,&timedown))
				{
				#ifdef RCC_DEBUG
					printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#endif

			//send info to face board
			if (*(rccdata->fd->contmode) < 27)
				fbdata[1] = *(rccdata->fd->contmode) + 1;
			else
				fbdata[1] = *(rccdata->fd->contmode);
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
			if (!wait_write_serial(*(rccdata->fd->fbserial)))
			{
				if (write(*(rccdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
				{
				#ifdef RCC_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(rccdata->fd->markbit) |= 0x0800;
					gettimeofday(&ct,NULL);
					update_event_list(rccdata->fd->ec,rccdata->fd->el,1,16,ct.tv_sec,rccdata->fd->markbit);
					if (SUCCESS != generate_event_file(rccdata->fd->ec,rccdata->fd->el,\
											rccdata->fd->softevent,rccdata->fd->markbit))
					{
					#ifdef RCC_DEBUG
						printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
				}
			}
			else
			{
			#ifdef RCC_DEBUG
				printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
			sendfaceInfoToBoard(rccdata->fd,fbdata);
			sleep(pinfo->rtime);

			#ifdef RCC_DEBUG
			printf("**********************personyes: %d,File: %s,Line: %d\n",personyes,__FILE__,__LINE__);
			#endif

			if ((0 == personyes) && (0 == leave))
			{//if ((0 == personyes) && (0 == leave))
				memset(buf,0,sizeof(buf));
				pbuf = buf;
				num = read(*(rccdata->fd->flowpipe),buf,sizeof(buf));
				if ((num > 0) && (num < sizeof(buf)))
				{//if ((num > 0) && (num < sizeof(buf)))
					mark = 0;
					while (1)
					{//inline while loop
						if (mark >= num)
							break;
						if ((0xF1 == *(pbuf+mark)) && (0xED == *(pbuf+mark+5)))
						{
							if (0x04 == *(pbuf+mark+1))
							{//have person request
								deteid = *(pbuf+mark+2);
								if (49 == deteid)
								{//control of all red
									personyes = 1;
									break;
								}//control of all red    
							}//have person request
							mark += 6;
							continue;//Fit data,break directly
						}
						else
						{
							mark += 1;
							continue;
						}
					}//inline while loop
				}//if ((num > 0) && (num < sizeof(buf)))
			}//if ((0 == personyes) && (0 == leave))

			if ((0 == leave) && (1 == personyes))
			{//if ((0 == leave) && (1 == personyes))
				while (1)
				{//monitor loop
					FD_ZERO(&nRead);
					FD_SET(*(rccdata->fd->flowpipe),&nRead);
					maxv = *(rccdata->fd->flowpipe);
					int mret = select(maxv+1,&nRead,NULL,NULL,NULL);
					if (mret < 0)
					{
					#ifdef RCC_DEBUG
						printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						rcc_end_part_child_thread();
						return;
					}
					if (0 == mret)
					{
						break;
					}
					if (mret > 0)
					{//have person request
						if (FD_ISSET(*(rccdata->fd->flowpipe),&nRead))
						{
							memset(buf,0,sizeof(buf));
							pbuf = buf;
							num = read(*(rccdata->fd->flowpipe),buf,sizeof(buf));
							if (num > sizeof(buf))
                                continue;
							if (num > 0)
							{
								mark = 0;
								personyes = 0;
								while (1)
								{//inline while loop
									if (mark >= num)
										break;
									if ((0xF1 == *(pbuf+mark)) && (0xED == *(pbuf+mark+5)))
									{
										if (0x04 == *(pbuf+mark+1))
										{//have person request
											deteid = *(pbuf+mark+2);
											if (50 == deteid)
											{//cancel control
												personyes = 1;
												break;
											}//cancel control    
										}//have person request
										mark += 6;
										continue;//Fit data,break directly
									}
									else
									{
										mark += 1;
										continue;
									}
								}//inline while loop
								if (1 == personyes)
								{
									break;
								}//if (1 == personyes)
								else
								{
									nowtime.tv_sec = 0;
									nowtime.tv_usec = 0;
									gettimeofday(&nowtime,NULL);
									leatime = nowtime.tv_sec - lasttime.tv_sec;
									mt -= leatime;
									continue;
								}
							}//num >= 0
							else
							{//num < 0
								nowtime.tv_sec = 0;
								nowtime.tv_usec = 0;
								gettimeofday(&nowtime,NULL);
								leatime = nowtime.tv_sec - lasttime.tv_sec;
								mt -= leatime;
								continue;
							}//num < 0
						}//if (FD_ISSET(*(rccdata->fd->flowpipe),&nRead))
						else
						{
							nowtime.tv_sec = 0;
							nowtime.tv_usec = 0;
							gettimeofday(&nowtime,NULL);
							leatime = nowtime.tv_sec - lasttime.tv_sec;
							mt -= leatime;
							continue;
						}
					}//have person request	
				}//monitor loop of person button
			}//if ((0 == leave) && (1 == personyes))	

			if (1 == phcon)
            {
				*(rccdata->fd->markbit) &= 0xfbff;
                memset(&gtime,0,sizeof(gtime));
                gettimeofday(&gtime,NULL);
                memset(&gftime,0,sizeof(gftime));
                memset(&ytime,0,sizeof(ytime));
                memset(&rtime,0,sizeof(rtime));
                phcon = 0;
                sleep(10);
            }

			leave = 0;
			personyes = 0;
			slnum += 1;
			*(rccdata->fd->slnum) = slnum;
			*(rccdata->fd->stageid) = rccdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
			if (0 == (rccdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId))
			{
				cycarr = 1;
				slnum = 0;
				*(rccdata->fd->slnum) = 0;
				*(rccdata->fd->stageid) = rccdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
			}
 
			continue;
		}//vehicle phase
		else
		{//not fit phase type
		#ifdef RCC_DEBUG
			printf("Not have fit phase type,please reset pattern,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			fbdata[0] = 0xC1;
			fbdata[5] = 0xED;
			fbdata[1] = 2;
			fbdata[2] = 0;
			fbdata[3] = 0x01;
			fbdata[4] = 0;
			sendfaceInfoToBoard(rccdata->fd,fbdata);
			cycarr = 1;
			sleep(1);
			continue;
		}//not fit phase type
	}//while loop

	return;
}

int get_rcc_status(unsigned char *color,unsigned char *leatime)
{
	if ((NULL == color) || (NULL == leatime))
    {
    #ifdef RCC_DEBUG
        printf("pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
        return MEMERR;
    }

    struct timeval          ntime;
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

int rail_crossing_control(fcdata_t *fcdata, tscdata_t *tscdata,ChannelStatus_t *chanstatus)
{
	if ((NULL == fcdata) || (NULL == tscdata) || (NULL == chanstatus))
	{
	#ifdef RCC_DEBUG
		printf("pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}	

	//Initial static variable
	rccyes = 0;
	rccyfyes = 0;
	rccpcyes = 0;
	ppmyes = 0;
	cpdyes = 0;
	rettl = 0;
	memset(&gtime,0,sizeof(gtime));
	memset(&gftime,0,sizeof(gftime));
	memset(&ytime,0,sizeof(ytime));
	memset(&rtime,0,sizeof(rtime));
	degrade = 0;
	memset(&sinfo,0,sizeof(statusinfo_t));
	phcon = 0;
	memset(fcdata->sinfo,0,sizeof(statusinfo_t));
	//End static variable

	rccdata_t				rccdata;
	rccpinfo_t				pinfo;
	unsigned char			contmode = *(fcdata->contmode);
	if (0 == rccyes)
	{
		memset(&rccdata,0,sizeof(rccdata_t));
		memset(&pinfo,0,sizeof(rccpinfo_t));
		rccdata.fd = fcdata;
		rccdata.td = tscdata;
		rccdata.pi = &pinfo;
		rccdata.cs = chanstatus;
		int gtret = pthread_create(&rccid,NULL,(void *)start_rail_crossing_control,&rccdata);
		if (0 != gtret)
		{
		#ifdef RCC_DEBUG
			printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
		rccyes = 1;
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

	fd_set					nread;
	unsigned char			keylock = 0;//key lock or unlock
	unsigned char			wllock = 0;//wireless terminal lock or unlock
	unsigned char			gtbuf[32] = {0};
	unsigned char			color = 3; //lamp is default closed;
	unsigned char			leatime = 0;
	unsigned char			gtred = 0;//'1' means lamp has been status of all red
	timedown_t				gttd;
	unsigned char			ngf = 0;
	struct timeval			to,ct;
//	struct timeval			wut;
	struct timeval			mont,ltime;
	yfdata_t				yfdata;//data of yellow flash
	yfdata_t				ardata;//data of all red
	unsigned char			fbdata[6] = {0};
	fbdata[0] = 0xC1;
	fbdata[5] = 0xED;
	unsigned char			gtecho[3] = {0};//send traffic control info to face board;
	gtecho[0] = 0xCA;
	gtecho[2] = 0xED;
	memset(&ardata,0,sizeof(ardata));
	ardata.second = 0;
	ardata.markbit = fcdata->markbit;
	ardata.markbit2 = fcdata->markbit2;
	ardata.serial = *(fcdata->bbserial);
	ardata.sockfd = fcdata->sockfd;
	ardata.cs = chanstatus;

	unsigned char               sibuf[64] = {0};
	unsigned char               *csta = NULL;
    unsigned char               tcsta = 0;
    sinfo.pattern = *(fcdata->patternid);
	unsigned char               ncmode = *(fcdata->contmode);

	unsigned char				wtlock = 0;
	struct timeval				wtltime;
	unsigned char				pantn = 0;

	unsigned char               dirc = 0; //direct control
    //channes of eight direction mapping;
    unsigned char       dirch[8][5] = {{0x01,0x03,0x09,0x0b,0x00},{0x05,0x07,0x09,0x0b,0x00},
                                      {0x02,0x04,0x0a,0x0c,0x00},{0x06,0x08,0x0a,0x0c,0x00},
                                      {0x01,0x05,0x09,0x00,0x00},{0x02,0x06,0x0a,0x00,0x00},
                                      {0x03,0x07,0x0b,0x00,0x00},{0x04,0x08,0x0c,0x00,0x00}};
	
	unsigned char       lkch[4][5] = {{0x01,0x03,0x09,0x0b,0x00},{0x05,0x07,0x09,0x0b,0x00},
                                        {0x02,0x04,0x0a,0x0c,0x00},{0x06,0x08,0x0a,0x0c,0x00}};	

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
	markdata_c				mdt;
	unsigned char           rail_phase = 0;
	while (1)
	{//0
		FD_ZERO(&nread);
		FD_SET(*(fcdata->conpipe),&nread);
		wtltime.tv_sec = RFIDT;
		wtltime.tv_usec = 0;
		int cpret = select(*(fcdata->conpipe)+1,&nread,NULL,NULL,&wtltime);
		if (cpret < 0)
		{
		#ifdef RCC_DEBUG
			printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			rcc_end_part_child_thread();
			return FAIL;
		}
		if (0 == cpret)
		{//time out
			if (*(fcdata->markbit2) & 0x0100)
                continue; //rfid is controlling
			*(fcdata->markbit2) &= 0xfffe;
			if (1 == wtlock)
            {//if (1 == wtlock)
          //      pantn += 1;
          //      if (3 == pantn)
          //      {//wireless terminal has disconnected with signaler machine;
				if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
				{
					memset(wlinfo,0,sizeof(wlinfo));
					gettimeofday(&ct,NULL);
					if (SUCCESS != wireless_info_report(wlinfo,ct.tv_sec,fcdata->wlmark,0x20))
					{
					#ifdef RCC_DEBUG
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
				gtred = 0;
				close = 0;
				fpc = 0;
				pantn = 0;
				cp = 0;

				if (1 == rccyfyes)
				{
					pthread_cancel(rccyfid);
					pthread_join(rccyfid,NULL);
					rccyfyes = 0;
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
							#ifdef RCC_DEBUG
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
							#ifdef RCC_DEBUG
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
						memset(&gttd,0,sizeof(gttd));
						gttd.mode = 28;//traffic control
						gttd.pattern = *(fcdata->patternid);
						gttd.lampcolor = 0x02;
						gttd.lamptime = pinfo.gftime;
						gttd.phaseid = 0;
						gttd.stageline = pinfo.stageline;
						if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
						{
						#ifdef RCC_DEBUG
							printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
					}
					#ifdef EMBED_CONFIGURE_TOOL
					if (*(fcdata->markbit2) & 0x0200)
					{
						memset(&gttd,0,sizeof(gttd));
                        gttd.mode = 28;//traffic control
                        gttd.pattern = *(fcdata->patternid);
                        gttd.lampcolor = 0x02;
                        gttd.lamptime = pinfo.gftime;
                        gttd.phaseid = 0;
                        gttd.stageline = pinfo.stageline;  
						if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
						{
						#ifdef RCC_DEBUG
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
						#ifdef RCC_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(fcdata->markbit) |= 0x0800;
							gettimeofday(&ct,NULL);
							update_event_list(fcdata->ec,fcdata->el,1,16, \
													ct.tv_sec,fcdata->markbit);
							if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
														fcdata->softevent,fcdata->markbit))
							{
							#ifdef RCC_DEBUG
								printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
						}
					}
					else
					{
					#ifdef RCC_DEBUG
						printf("face board serial port cannot write,Line:%d\n",__LINE__);
					#endif
					}
					sendfaceInfoToBoard(fcdata,fbdata);
					ngf = 0;
					while (1)
					{
						if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
						{
						#ifdef RCC_DEBUG
							printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(fcdata->markbit) |= 0x0800;
						}
						if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																wcc,0x03,fcdata->markbit))
						{
						#ifdef RCC_DEBUG
							printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
						to.tv_sec = 0;
						to.tv_usec = 500000;
						select(0,NULL,NULL,NULL,&to);
						if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
						{
						#ifdef RCC_DEBUG
							printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
							*(fcdata->markbit) |= 0x0800;
						}
						if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																	wcc,0x02,fcdata->markbit))
						{
						#ifdef RCC_DEBUG
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
					if (1 == pce)
					{
					//current phase begin to yellow lamp
						if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
						{
						#ifdef RCC_DEBUG
							printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(fcdata->markbit) |= 0x0800;
						}
						if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																wnpcc,0x01, fcdata->markbit))
						{
						#ifdef RCC_DEBUG
							printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
						if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
						{
						#ifdef RCC_DEBUG
							printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(fcdata->markbit) |= 0x0800;
						}
						if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																wpcc,0x00,fcdata->markbit))
						{
						#ifdef RCC_DEBUG
							printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
					}
					else
					{
						if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
						{
						#ifdef RCC_DEBUG
							printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(fcdata->markbit) |= 0x0800;
						}
						if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																wcc,0x01,fcdata->markbit))
						{
						#ifdef RCC_DEBUG
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
							#ifdef RCC_DEBUG
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
						memset(&gttd,0,sizeof(gttd));
						gttd.mode = 28;//traffic control
						gttd.pattern = *(fcdata->patternid);
						gttd.lampcolor = 0x01;
						gttd.lamptime = pinfo.ytime;
						gttd.phaseid = 0;
						gttd.stageline = pinfo.stageline;
						if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
						{
						#ifdef RCC_DEBUG
							printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
					}
					#ifdef EMBED_CONFIGURE_TOOL
					if (*(fcdata->markbit2) & 0x0200)
					{
						memset(&gttd,0,sizeof(gttd));
                        gttd.mode = 28;//traffic control
                        gttd.pattern = *(fcdata->patternid);
                        gttd.lampcolor = 0x01;
                        gttd.lamptime = pinfo.ytime;
                        gttd.phaseid = 0;
                        gttd.stageline = pinfo.stageline;  
						if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
						{
						#ifdef RCC_DEBUG
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
						#ifdef RCC_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(fcdata->markbit) |= 0x0800;
							gettimeofday(&ct,NULL);
							update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
							if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																	fcdata->softevent,fcdata->markbit))
							{	
							#ifdef RCC_DEBUG
								printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
						}
					}
					else
					{
					#ifdef RCC_DEBUG
						printf("face board serial port cannot write,Line:%d\n",__LINE__);
					#endif
					}
					sendfaceInfoToBoard(fcdata,fbdata);
					sleep(pinfo.ytime);

					//current phase begin to red lamp
					if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
					{
					#ifdef RCC_DEBUG
						printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
						*(fcdata->markbit) |= 0x0800;
					}
					if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															wcc,0x00,fcdata->markbit))
					{
					#ifdef RCC_DEBUG
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
							#ifdef RCC_DEBUG
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
					*(fcdata->stageid) = tscdata->timeconfig->TimeConfigList[rettl][0].StageId;
				}//if (0 != directc)
				new_all_red(&ardata);

				*(fcdata->contmode) = contmode; //restore control mode;
				cp = 0;
				dirc = 0;
				*(fcdata->markbit2) &= 0xfffb;
				if (0 == rccyes)
				{
					memset(&rccdata,0,sizeof(rccdata_t));
					memset(&pinfo,0,sizeof(rccpinfo_t));
					rccdata.fd = fcdata;
					rccdata.td = tscdata;
					rccdata.pi = &pinfo;
					rccdata.cs = chanstatus;
					int gtret = pthread_create(&rccid,NULL,(void *)start_rail_crossing_control,&rccdata);
					if (0 != gtret)
					{
					#ifdef RCC_DEBUG
						printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						return FAIL;
					}
					rccyes = 1;
				}
				continue;
            }//if (1 == wtlock)
            continue;
		}//time out
		if (cpret > 0)
		{//if (cpret > 0)
			if (FD_ISSET(*(fcdata->conpipe),&nread))
			{
				memset(gtbuf,0,sizeof(gtbuf));
				read(*(fcdata->conpipe),gtbuf,sizeof(gtbuf));
				if ((0xB9==gtbuf[0]) && ((0xED==gtbuf[8]) || (0xED==gtbuf[4]) || (0xED==gtbuf[5])))
                {//wireless terminal control
					pantn = 0;
                    if (0x02 == gtbuf[3])
                    {//pant
                        continue;
                    }//pant
                    if ((0 == wllock) && (0 == keylock) && (0 == netlock))
                    {//terminal control is valid only when wireless and key and net control is invalid;
						if (0x04 == gtbuf[3])
                        {//control function
                            if ((0x01 == gtbuf[4]) && (0 == wtlock))
                            {//control will happen
								get_rcc_status(&color,&leatime);
								if (0x02 != color)
								{
									struct timeval          spacetime;
									unsigned char           endlock = 0;
									
									while (1)
									{//while loop,000000
										spacetime.tv_sec = 0;
										spacetime.tv_usec = 200000;
										select(0,NULL,NULL,NULL,&spacetime);

										memset(gtbuf,0,sizeof(gtbuf));
										read(*(fcdata->conpipe),gtbuf,sizeof(gtbuf));
										if ((0xB9 == gtbuf[0]) && (0xED == gtbuf[5]))
										{
											if (0x00 == gtbuf[4])
											{
												endlock = 1;
												break;
											}
										}
										
										get_rcc_status(&color,&leatime);
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
								tpp.func[0] = gtbuf[4];
                                memset(terbuf,0,sizeof(terbuf));
                                if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
                                {
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),terbuf,sizeof(terbuf)) < 0)
                                        {
                                        #ifdef RCC_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
								wtlock = 1;
								gtred = 0;
								close = 0;
								fpc = 0;
								cp = 0;
								dirc = 0;
								rcc_end_child_thread();//end main thread and its child thread

								*(fcdata->markbit2) |= 0x0004;
							
								*(fcdata->contmode) = 28;//wireless terminal control mode

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
            						#ifdef RCC_DEBUG
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
                                    #ifdef RCC_DEBUG
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
								fbdata[2] = pinfo.stageline;
								fbdata[3] = 0x02;
								fbdata[4] = 0;
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
									{
									#ifdef RCC_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
                						update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                						if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																fcdata->softevent,fcdata->markbit))
                						{
                						#ifdef RCC_DEBUG
                   							printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                						#endif
                						}
									}
								}
								else
								{
								#ifdef RCC_DEBUG
									printf("face board serial port cannot write,Line:%d\n",__LINE__);
								#endif
								}
								sendfaceInfoToBoard(fcdata,fbdata);
								//send down time to configure tool
                            	if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            	{
								#ifdef RCC_DEBUG
									printf("send control info to configure tool,File: %s,Line: %d\n", \
											__FILE__,__LINE__);
								#endif
                                	memset(&gttd,0,sizeof(gttd));
                                	gttd.mode = 28;
                                	gttd.pattern = *(fcdata->patternid);
                                	gttd.lampcolor = 0x02;
                                	gttd.lamptime = 0;
                                	gttd.phaseid = pinfo.phaseid;
                                	gttd.stageline = pinfo.stageline;
                                	if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
                                	{
                                	#ifdef RCC_DEBUG
                                    	printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                	#endif
                                	}
                            	}
								#ifdef EMBED_CONFIGURE_TOOL
                                if (*(fcdata->markbit2) & 0x0200)
                                {
                                 	memset(&gttd,0,sizeof(gttd));
                                    gttd.mode = 28;
                                    gttd.pattern = *(fcdata->patternid);
                                    gttd.lampcolor = 0x02;
                                    gttd.lamptime = 0;
                                    gttd.phaseid = pinfo.phaseid;
                                    gttd.stageline = pinfo.stageline; 
                                    if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                                #endif
                            	if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    				pinfo.chan,0x02,fcdata->markbit))
                            	{
                            	#ifdef RCC_DEBUG
                                	printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            	#endif
                            	}
							
								continue;
                            }//control will happen
                            else if ((0x00 == gtbuf[4]) && (1 == wtlock))
                            {//cancel control
								wtlock = 0;
								gtred = 0;
								close = 0;
								fpc = 0;
								cp = 0;
								tpp.func[0] = gtbuf[4];
                                memset(terbuf,0,sizeof(terbuf));
                                if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
                                {
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),terbuf,sizeof(terbuf)) < 0)
                                        {
                                        #ifdef RCC_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
								if (1 == rccyfyes)
								{
									pthread_cancel(rccyfid);
									pthread_join(rccyfid,NULL);
									rccyfyes = 0;
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
            								#ifdef RCC_DEBUG
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
            								#ifdef RCC_DEBUG
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
										memset(&gttd,0,sizeof(gttd));
										gttd.mode = 28;//traffic control
										gttd.pattern = *(fcdata->patternid);
										gttd.lampcolor = 0x02;
										gttd.lamptime = pinfo.gftime;
										gttd.phaseid = 0;
										gttd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 28;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x02;
                                        gttd.lamptime = pinfo.gftime;
                                        gttd.phaseid = 0;
                                        gttd.stageline = pinfo.stageline;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                               			#ifdef RCC_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef RCC_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									ngf = 0;
									while (1)
									{
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
       									{
       									#ifdef RCC_DEBUG
           									printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
       									#endif
											*(fcdata->markbit) |= 0x0800;
       									}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x03,fcdata->markbit))
       									{
        								#ifdef RCC_DEBUG
            								printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
        								#endif
        								}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);

										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
        								{
        								#ifdef RCC_DEBUG
            								printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    											wcc,0x02,fcdata->markbit))
                                		{
                                		#ifdef RCC_DEBUG
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
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef RCC_DEBUG
            								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef RCC_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef RCC_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef RCC_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef RCC_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef RCC_DEBUG
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
            								#ifdef RCC_DEBUG
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
                            			memset(&gttd,0,sizeof(gttd));
                            			gttd.mode = 28;//traffic control
                            			gttd.pattern = *(fcdata->patternid);
                            			gttd.lampcolor = 0x01;
                            			gttd.lamptime = pinfo.ytime;
                            			gttd.phaseid = 0;
                            			gttd.stageline = pinfo.stageline;
                            			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
                            			{
                            			#ifdef RCC_DEBUG
                               				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            			#endif
                            			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 28;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x01;
                                        gttd.lamptime = pinfo.ytime;
                                        gttd.phaseid = 0;
                                        gttd.stageline = pinfo.stageline;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                            			#ifdef RCC_DEBUG
                               				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                			{
                                			#ifdef RCC_DEBUG
                                   				printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef RCC_DEBUG
                                		printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef RCC_DEBUG
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
            								#ifdef RCC_DEBUG
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
                                    *(fcdata->stageid) = tscdata->timeconfig->TimeConfigList[rettl][0].StageId;
								}//direction control happen
								
								new_all_red(&ardata);

								*(fcdata->contmode) = contmode; //restore control mode;
								cp = 0;
								dirc = 0;
								*(fcdata->markbit2) &= 0xfffb;
								if (0 == rccyes)
								{
									memset(&rccdata,0,sizeof(rccdata_t));
									memset(&pinfo,0,sizeof(rccpinfo_t));
									rccdata.fd = fcdata;
									rccdata.td = tscdata;
									rccdata.pi = &pinfo;
									rccdata.cs = chanstatus;
									int gtret = \
									pthread_create(&rccid,NULL,(void *)start_rail_crossing_control,&rccdata);
									if (0 != gtret)
									{
									#ifdef RCC_DEBUG
										printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										return FAIL;
									}
									rccyes = 1;
								}
								if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
								{
									memset(wlinfo,0,sizeof(wlinfo));
                                    gettimeofday(&ct,NULL);
                                    if (SUCCESS != wireless_info_report(wlinfo,ct.tv_sec,fcdata->wlmark,0x06))
                                    {
                                    #ifdef RCC_DEBUG
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
                            else if (((0x05 <= gtbuf[4]) && (gtbuf[4] <= 0x0c)) && (1 == wtlock))
                            {//direction control
								tpp.func[0] = gtbuf[4];
                                memset(terbuf,0,sizeof(terbuf));
                                if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
                                {
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),terbuf,sizeof(terbuf)) < 0)
                                        {
                                        #ifdef RCC_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
								if ((1 == gtred) || (1 == rccyfyes) || (1 == close))
                            	{//if ((1 == tcred) || (1 == tcyfyes) || (1 == close))
									dirc = gtbuf[4];
									if ((dirc < 5) && (dirc > 0x0c))
                                    {
                                        continue;
                                    }
									fpc = 1;
									cp = 0;
                                	if (1 == rccyfyes)
                                	{
                                    	pthread_cancel(rccyfid);
                                    	pthread_join(rccyfid,NULL);
                                    	rccyfyes = 0;
                                	}
									if (1 != gtred)
									{
										new_all_red(&ardata);
									}
									gtred = 0;
                                    close = 0;

                                	if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),dirch[dirc-5],0x02))
                                	{
                                	#ifdef RCC_DEBUG
                                    	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
                                    	*(fcdata->markbit) |= 0x0800;
                                	}
                                	if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                                        				dirch[dirc-5],0x02,fcdata->markbit))
                                	{
                                	#ifdef RCC_DEBUG
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
            							#ifdef RCC_DEBUG
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
									if (0x05 == gtbuf[4])
									{
										dce = 0x10;
										val = 52;
									}
									else if (0x06 == gtbuf[4])
									{
										dce = 0x12;
										val = 54;
									}
									else if (0x07 == gtbuf[4])
									{
										dce = 0x14;
										val = 56;
									}
									else if (0x08 == gtbuf[4])
									{
                                    	dce = 0x16;
										val = 58;
									}
									else if (0x09 == gtbuf[4])
									{
                                    	dce = 0x18;
										val = 60;
									}
									else if (0x0a == gtbuf[4])
									{
                                    	dce = 0x1a;
										val = 62;
									}
									else if (0x0b == gtbuf[4])
									{
                                    	dce = 0x1c;
										val = 64;
									}
									else if (0x0c == gtbuf[4])
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
                                   		#ifdef RCC_DEBUG
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
                            	}//if ((1 == gtred) || (1 == gtyfyes) || (1 == close))
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
                            				*pcc = tscdata->channel->ChannelList[i].ChannelId;
                                			pcc++;
                            			}
                        			}
									dirc = gtbuf[4];
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
            								#ifdef RCC_DEBUG
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
            								#ifdef RCC_DEBUG
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
										memset(&gttd,0,sizeof(gttd));
										gttd.mode = 28;//traffic control
										gttd.pattern = *(fcdata->patternid);
										gttd.lampcolor = 0x02;
										gttd.lamptime = pinfo.gftime;
										gttd.phaseid = cp;
										gttd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 28;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x02;
                                        gttd.lamptime = pinfo.gftime;
                                        gttd.phaseid = cp;
                                        gttd.stageline = pinfo.stageline;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                               			#ifdef RCC_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef RCC_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									ngf = 0;
									while (1)
									{
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
       									{
       									#ifdef RCC_DEBUG
           									printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
       									#endif
											*(fcdata->markbit) |= 0x0800;
       									}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x03,fcdata->markbit))
       									{
        								#ifdef RCC_DEBUG
            								printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
        								#endif
        								}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);

										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
        								{
        								#ifdef RCC_DEBUG
            								printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    											wcc,0x02,fcdata->markbit))
                                		{
                                		#ifdef RCC_DEBUG
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
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef RCC_DEBUG
            								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef RCC_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef RCC_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef RCC_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef RCC_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef RCC_DEBUG
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
            								#ifdef RCC_DEBUG
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
                            			memset(&gttd,0,sizeof(gttd));
                            			gttd.mode = 28;//traffic control
                            			gttd.pattern = *(fcdata->patternid);
                            			gttd.lampcolor = 0x01;
                            			gttd.lamptime = pinfo.ytime;
                            			gttd.phaseid = cp;
                            			gttd.stageline = pinfo.stageline;
                            			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
                            			{
                            			#ifdef RCC_DEBUG
                               				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            			#endif
                            			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 28;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x01;
                                        gttd.lamptime = pinfo.ytime;
                                        gttd.phaseid = cp;
                                        gttd.stageline = pinfo.stageline;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                            			#ifdef RCC_DEBUG
                               				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                			{
                                			#ifdef RCC_DEBUG
                                   				printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef RCC_DEBUG
                                		printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef RCC_DEBUG
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
            								#ifdef RCC_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (pinfo.rtime > 0))									
									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),dirch[dirc-5],0x02))
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																		dirch[dirc-5],0x02,fcdata->markbit))
                            		{
                            		#ifdef RCC_DEBUG
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
            							#ifdef RCC_DEBUG
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
										memset(&gttd,0,sizeof(gttd));
										gttd.mode = 28;//traffic control
										gttd.pattern = *(fcdata->patternid);
										gttd.lampcolor = 0x02;
										gttd.lamptime = 0;
										gttd.phaseid = 0;
										gttd.stageline = 0;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 28;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x02;
                                        gttd.lamptime = 0;
                                        gttd.phaseid = 0;
                                        gttd.stageline = 0;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                               			#ifdef RCC_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef RCC_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									cp = 0;
								}//if (0 == fpc)
								else if (1 == fpc)
								{//else if (1 == fpc)
									if (dirc == gtbuf[4])
									{//control is current phase
										continue;
									}//control is current phase
									unsigned char		*pcc = dirch[dirc-5];
									dirc = gtbuf[4];
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
            								#ifdef RCC_DEBUG
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
            								#ifdef RCC_DEBUG
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
										memset(&gttd,0,sizeof(gttd));
										gttd.mode = 28;//traffic control
										gttd.pattern = *(fcdata->patternid);
										gttd.lampcolor = 0x02;
										gttd.lamptime = pinfo.gftime;
										gttd.phaseid = 0;
										gttd.stageline = 0;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 28;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x02;
                                        gttd.lamptime = pinfo.gftime;
                                        gttd.phaseid = 0;
                                        gttd.stageline = 0;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                               			#ifdef RCC_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef RCC_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									ngf = 0;
									while (1)
									{
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
       									{
       									#ifdef RCC_DEBUG
           									printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
       									#endif
											*(fcdata->markbit) |= 0x0800;
       									}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x03,fcdata->markbit))
       									{
        								#ifdef RCC_DEBUG
            								printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
        								#endif
        								}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);

										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
        								{
        								#ifdef RCC_DEBUG
            								printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    											wcc,0x02,fcdata->markbit))
                                		{
                                		#ifdef RCC_DEBUG
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
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef RCC_DEBUG
            								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef RCC_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef RCC_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef RCC_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef RCC_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef RCC_DEBUG
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
            								#ifdef RCC_DEBUG
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
                            			memset(&gttd,0,sizeof(gttd));
                            			gttd.mode = 28;//traffic control
                            			gttd.pattern = *(fcdata->patternid);
                            			gttd.lampcolor = 0x01;
                            			gttd.lamptime = pinfo.ytime;
                            			gttd.phaseid = 0;
                            			gttd.stageline = 0;
                            			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
                            			{
                            			#ifdef RCC_DEBUG
                               				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            			#endif
                            			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 28;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x01;
                                        gttd.lamptime = pinfo.ytime;
                                        gttd.phaseid = 0;
                                        gttd.stageline = 0;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                            			#ifdef RCC_DEBUG
                               				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                			{
                                			#ifdef RCC_DEBUG
                                   				printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef RCC_DEBUG
                                		printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef RCC_DEBUG
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
            								#ifdef RCC_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (pinfo.rtime > 0))									
									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pnc,0x02))
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			pnc,0x02,fcdata->markbit))
                            		{
                            		#ifdef RCC_DEBUG
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
            							#ifdef RCC_DEBUG
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
										memset(&gttd,0,sizeof(gttd));
										gttd.mode = 28;//traffic control
										gttd.pattern = *(fcdata->patternid);
										gttd.lampcolor = 0x02;
										gttd.lamptime = 0;
										gttd.phaseid = 0;
										gttd.stageline = 0;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 28;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x02;
                                        gttd.lamptime = 0;
                                        gttd.phaseid = 0;
                                        gttd.stageline = 0;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                               			#ifdef RCC_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef RCC_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
									}
								}//else if (1 == fpc)
								unsigned char				dce = 0;
								unsigned char				val = 0;
								if (0x05 == gtbuf[4])
								{
									dce = 0x10;
									val = 52;
								}
								else if (0x06 == gtbuf[4])
								{
									dce = 0x12;
									val = 54;
								}
								else if (0x07 == gtbuf[4])
								{
									dce = 0x14;
									val = 56;
								}
								else if (0x08 == gtbuf[4])
								{
                                   	dce = 0x16;
									val = 58;
								}
								else if (0x09 == gtbuf[4])
								{
                                   	dce = 0x18;
									val = 60;
								}
								else if (0x0a == gtbuf[4])
								{
                                   	dce = 0x1a;
									val = 62;
								}
								else if (0x0b == gtbuf[4])
								{
                                   	dce = 0x1c;
									val = 64;
								}
								else if (0x0c == gtbuf[4])
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
                                	#ifdef RCC_DEBUG
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
                            else if ((0x02 == gtbuf[4]) && (1 == wtlock))
                            {//step by step
								cp = 0;
								fpc = 0;
								tpp.func[0] = gtbuf[4];
                                memset(terbuf,0,sizeof(terbuf));
                                if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
                                {
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),terbuf,sizeof(terbuf)) < 0)
                                        {
                                        #ifdef RCC_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
								if ((1 == gtred) || (1 == rccyfyes) || (1 == close))
								{//if ((1 == gtred) || (1 == gtyfyes) || (1 == close))
									if (1 == rccyfyes)
                                    {
                                        pthread_cancel(rccyfid);
                                        pthread_join(rccyfid,NULL);
                                        rccyfyes = 0;
                                    }
                                    if (1 != gtred)
                                    {
                                        new_all_red(&ardata);
                                    }
                                    gtred = 0;
                                    close = 0;
									
									if ((dirc < 0x05) || (dirc > 0x0c))
									{//not have phase control
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                                		{
                                		#ifdef RCC_DEBUG
                                    		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
                                    		*(fcdata->markbit) |= 0x0800;
                                		}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    									pinfo.chan,0x02,fcdata->markbit))
                                		{
                                		#ifdef RCC_DEBUG
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
            								#ifdef RCC_DEBUG
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
										if (SUCCESS!=rcc_set_lamp_color(*(fcdata->bbserial),dirch[dirc-5],0x02))
                                		{
                                		#ifdef RCC_DEBUG
                                    		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
                                    		*(fcdata->markbit) |= 0x0800;
                                		}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    									dirch[dirc-5],0x02,fcdata->markbit))
                                		{
                                		#ifdef RCC_DEBUG
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
            								#ifdef RCC_DEBUG
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
                                    	#ifdef RCC_DEBUG
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
								}//if ((1 == gtred) || (1 == gtyfyes) || (1 == close))

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
            								#ifdef RCC_DEBUG
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
            								#ifdef RCC_DEBUG
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
										memset(&gttd,0,sizeof(gttd));
										gttd.mode = 28;//traffic control
										gttd.pattern = *(fcdata->patternid);
										gttd.lampcolor = 0x02;
										gttd.lamptime = pinfo.gftime;
										gttd.phaseid = pinfo.phaseid;
										gttd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 28;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x02;
                                        gttd.lamptime = pinfo.gftime;
                                        gttd.phaseid = pinfo.phaseid;
                                        gttd.stageline = pinfo.stageline;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                                		#ifdef RCC_DEBUG
                                    		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                    		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                    		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                    		{
                                    		#ifdef RCC_DEBUG
                                        		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
                                    		}
                                		}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
                                		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
	
									ngf = 0;
									while (1)
									{
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x03))
        								{
        								#ifdef RCC_DEBUG
            								printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															pinfo.cchan,0x03,fcdata->markbit))
        								{
        								#ifdef RCC_DEBUG
            								printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
        								#endif
        								}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);

										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x02))
        								{
        								#ifdef RCC_DEBUG
            								printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    									pinfo.cchan,0x02,fcdata->markbit))
                                		{
                                		#ifdef RCC_DEBUG
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
									if (1 == (pinfo.cpcexist))
									{
										//current phase begin to yellow lamp
										if (SUCCESS!=rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cnpchan,0x01))
        								{
        								#ifdef RCC_DEBUG
            								printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
													pinfo.cnpchan,0x01, fcdata->markbit))
										{
										#ifdef RCC_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cpchan,0x00))
										{
										#ifdef RCC_DEBUG
											printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																	pinfo.cpchan,0x00,fcdata->markbit))
										{
										#ifdef RCC_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x01))
										{
										#ifdef RCC_DEBUG
											printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														pinfo.cchan,0x01,fcdata->markbit))
										{
										#ifdef RCC_DEBUG
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
            								#ifdef RCC_DEBUG
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
                               			memset(&gttd,0,sizeof(gttd));
                               			gttd.mode = 28;//traffic control
                               			gttd.pattern = *(fcdata->patternid);
                               			gttd.lampcolor = 0x01;
                               			gttd.lamptime = pinfo.ytime;
                               			gttd.phaseid = pinfo.phaseid;
                               			gttd.stageline = pinfo.stageline;
                               			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
                               			{
                               			#ifdef RCC_DEBUG
                                   			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               			#endif
                               			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 28;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x01;
                                        gttd.lamptime = pinfo.ytime;
                                        gttd.phaseid = pinfo.phaseid;
                                        gttd.stageline = pinfo.stageline;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                               			#ifdef RCC_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef RCC_DEBUG
                                       			printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x00))
                            		{
                            		#ifdef RCC_DEBUG
                                		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															pinfo.cchan,0x00,fcdata->markbit))
									{
									#ifdef RCC_DEBUG
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
            								#ifdef RCC_DEBUG
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
                                			memset(&gttd,0,sizeof(gttd));
                                			gttd.mode = 28;//traffic control
                                			gttd.pattern = *(fcdata->patternid);
                                			gttd.lampcolor = 0x00;
                                			gttd.lamptime = pinfo.rtime;
                                			gttd.phaseid = pinfo.phaseid;
                                			gttd.stageline = pinfo.stageline;
                                			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
                                			{
                                			#ifdef RCC_DEBUG
                                    			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                            			}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&gttd,0,sizeof(gttd));
                                            gttd.mode = 28;//traffic control
                                            gttd.pattern = *(fcdata->patternid);
                                            gttd.lampcolor = 0x00;
                                            gttd.lamptime = pinfo.rtime;
                                            gttd.phaseid = pinfo.phaseid;
                                            gttd.stageline = pinfo.stageline;  
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
											{
											#ifdef RCC_DEBUG
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
                                    		#ifdef RCC_DEBUG
                                        		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
                                        		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                        		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                        		{
                                        		#ifdef RCC_DEBUG
                                            		printf("genfile err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                        		#endif
                                        		}
                                    		}
                                		}
                                		else
                                		{
                                		#ifdef RCC_DEBUG
                                    		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                		#endif
                                		}
										sleep(pinfo.rtime);
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
									memset(&pinfo,0,sizeof(rccpinfo_t));
        						if(SUCCESS!=rcc_get_phase_info(fcdata,tscdata,rettl,*(fcdata->slnum),&pinfo))
        							{
        							#ifdef RCC_DEBUG
            							printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
        							#endif
										rcc_end_part_child_thread();
            							return FAIL;
        							}
        							*(fcdata->phaseid) = 0;
        							*(fcdata->phaseid) |= (0x01 << (pinfo.phaseid - 1));

									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                            		{
                            		#ifdef RCC_DEBUG
                                		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																pinfo.chan,0x02,fcdata->markbit))
                            		{
                            		#ifdef RCC_DEBUG
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
            							#ifdef RCC_DEBUG
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
										memset(&gttd,0,sizeof(gttd));
										gttd.mode = 28;//traffic control
										gttd.pattern = *(fcdata->patternid);
										gttd.lampcolor = 0x02;
										gttd.lamptime = 0;
										gttd.phaseid = pinfo.phaseid;
										gttd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 28;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x02;
                                        gttd.lamptime = 0;
                                        gttd.phaseid = pinfo.phaseid;
                                        gttd.stageline = pinfo.stageline;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                                		#ifdef RCC_DEBUG
                                    		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                    		{
                                    		#ifdef RCC_DEBUG
                                        		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
                                    		}
                                		}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
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
                                    memset(&pinfo,0,sizeof(rccpinfo_t));
                                    if(SUCCESS != rcc_get_phase_info(fcdata,tscdata,rettl,0,&pinfo))
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                        rcc_end_part_child_thread();
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
            								#ifdef RCC_DEBUG
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
            								#ifdef RCC_DEBUG
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
										memset(&gttd,0,sizeof(gttd));
										gttd.mode = 28;//traffic control
										gttd.pattern = *(fcdata->patternid);
										gttd.lampcolor = 0x02;
										gttd.lamptime = pinfo.gftime;
										gttd.phaseid = 0;
										gttd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 28;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x02;
                                        gttd.lamptime = pinfo.gftime;
                                        gttd.phaseid = 0;
                                        gttd.stageline = pinfo.stageline;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                                		#ifdef RCC_DEBUG
                                    		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                    		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                    		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                    		{
                                    		#ifdef RCC_DEBUG
                                        		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
                                    		}
                                		}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
                                		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
	
									ngf = 0;
									while (1)
									{
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
        								{
        								#ifdef RCC_DEBUG
            								printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																		wcc,0x03,fcdata->markbit))
        								{
        								#ifdef RCC_DEBUG
            								printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
        								#endif
        								}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);

										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
        								{
        								#ifdef RCC_DEBUG
            								printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    										wcc,0x02,fcdata->markbit))
                                		{
                                		#ifdef RCC_DEBUG
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
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS!=rcc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef RCC_DEBUG
            								printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef RCC_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef RCC_DEBUG
											printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef RCC_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef RCC_DEBUG
											printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														wcc,0x01,fcdata->markbit))
										{
										#ifdef RCC_DEBUG
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
            								#ifdef RCC_DEBUG
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
                               			memset(&gttd,0,sizeof(gttd));
                               			gttd.mode = 28;//traffic control
                               			gttd.pattern = *(fcdata->patternid);
                               			gttd.lampcolor = 0x01;
                               			gttd.lamptime = pinfo.ytime;
                               			gttd.phaseid = 0;
                               			gttd.stageline = pinfo.stageline;
                               			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
                               			{
                               			#ifdef RCC_DEBUG
                                   			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               			#endif
                               			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 28;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x01;
                                        gttd.lamptime = pinfo.ytime;
                                        gttd.phaseid = 0;
                                        gttd.stageline = pinfo.stageline;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                               			#ifdef RCC_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef RCC_DEBUG
                                       			printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef RCC_DEBUG
                                		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															wcc,0x00,fcdata->markbit))
									{
									#ifdef RCC_DEBUG
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
            								#ifdef RCC_DEBUG
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
                                			memset(&gttd,0,sizeof(gttd));
                                			gttd.mode = 28;//traffic control
                                			gttd.pattern = *(fcdata->patternid);
                                			gttd.lampcolor = 0x00;
                                			gttd.lamptime = pinfo.rtime;
                                			gttd.phaseid = 0;
                                			gttd.stageline = pinfo.stageline;
                                			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
                                			{
                                			#ifdef RCC_DEBUG
                                    			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                            			}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&gttd,0,sizeof(gttd));
                                            gttd.mode = 28;//traffic control
                                            gttd.pattern = *(fcdata->patternid);
                                            gttd.lampcolor = 0x00;
                                            gttd.lamptime = pinfo.rtime;
                                            gttd.phaseid = 0;
                                            gttd.stageline = pinfo.stageline;  
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
											{
											#ifdef RCC_DEBUG
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
                                    		#ifdef RCC_DEBUG
                                        		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
                                        		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                        		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                        		{
                                        		#ifdef RCC_DEBUG
                                            		printf("file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        		#endif
                                        		}
                                    		}
                                		}
                                		else
                                		{
                                		#ifdef RCC_DEBUG
                                    		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                		#endif
                                		}
										sleep(pinfo.rtime);
									}
									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			pinfo.chan,0x02,fcdata->markbit))
                            		{
                            		#ifdef RCC_DEBUG
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
            							#ifdef RCC_DEBUG
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
										memset(&gttd,0,sizeof(gttd));
										gttd.mode = 28;//traffic control
										gttd.pattern = *(fcdata->patternid);
										gttd.lampcolor = 0x02;
										gttd.lamptime = 0;
										gttd.phaseid = pinfo.phaseid;
										gttd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 28;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x02;
                                        gttd.lamptime = 0;
                                        gttd.phaseid = pinfo.phaseid;
                                        gttd.stageline = pinfo.stageline;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                               			#ifdef RCC_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef RCC_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
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
                                    #ifdef RCC_DEBUG
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
                            else if ((0x03 == gtbuf[4]) && (1 == wtlock))
                            {//yellow flash
								dirc = 0;
								gtred = 0;
								close = 0;
								cp = 0;
								tpp.func[0] = gtbuf[4];
                                memset(terbuf,0,sizeof(terbuf));
                                if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
                                {
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),terbuf,sizeof(terbuf)) < 0)
                                        {
                                        #ifdef RCC_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
								if (0 == rccyfyes)
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
									int yfret = pthread_create(&rccyfid,NULL,(void *)rcc_yellow_flash,&yfdata);
									if (0 != yfret)
									{
									#ifdef RCC_DEBUG
										printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										rcc_end_part_child_thread();

										return FAIL;
									}
									rccyfyes = 1;
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
            						#ifdef RCC_DEBUG
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
                                    #ifdef RCC_DEBUG
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
                            else if ((0x04 == gtbuf[4]) && (1 == wtlock))
                            {//all red
								dirc = 0;
								close = 0;
								cp = 0;
								tpp.func[0] = gtbuf[4];
                                memset(terbuf,0,sizeof(terbuf));
                                if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
                                {
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),terbuf,sizeof(terbuf)) < 0)
                                        {
                                        #ifdef RCC_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
								if (1 == rccyfyes)
								{
									pthread_cancel(rccyfid);
									pthread_join(rccyfid,NULL);
									rccyfyes = 0;
								}
					
								if (0 == gtred)
								{	
									new_all_red(&ardata);
									gtred = 1;
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
            						#ifdef RCC_DEBUG
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
                                    #ifdef RCC_DEBUG
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
                            else if ((0x0d == gtbuf[4]) && (1 == wtlock))
                            {//close
								dirc = 0;
								gtred = 0;
								cp = 0;
								tpp.func[0] = gtbuf[4];
                                memset(terbuf,0,sizeof(terbuf));
                                if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
                                {
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),terbuf,sizeof(terbuf)) < 0)
                                        {
                                        #ifdef RCC_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
								if (1 == rccyfyes)
                                {
                                    pthread_cancel(rccyfid);
                                    pthread_join(rccyfid,NULL);
                                    rccyfyes = 0;
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
            						#ifdef RCC_DEBUG
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
                                    #ifdef RCC_DEBUG
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
				else if ((0xCC == gtbuf[0]) && (0xED == gtbuf[2]))
				{//network control
					if ((0 == wllock) && (0 == keylock) && (0 == wtlock))
					{//net control is valid only when wireless and key control is invalid;
						if ((0xf0 == gtbuf[1]) && (0 == netlock))
						{//net control will happen
							get_rcc_status(&color,&leatime);
							rail_phase = 0;
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

									memset(gtbuf,0,sizeof(gtbuf));
									read(*(fcdata->conpipe),gtbuf,sizeof(gtbuf));
									if ((0xCC == gtbuf[0]) && (0xED == gtbuf[2]))
									{
										if (0xf1 == gtbuf[1])
										{
											endlock = 1;
											phcon = 0;
											break;
										}
										else
										{
											rail_phase = gtbuf[1];
										}
									}
									get_rcc_status(&color,&leatime);
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
							gtred = 0;
							close = 0;
							fpc = 0;
							cp = 0;
							rcc_end_child_thread();//end main thread and its child thread
						
							*(fcdata->markbit2) |= 0x0008;
						#if 0
							new_all_red(&ardata);
						#endif
							if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
        					{
        					#ifdef RCC_DEBUG
            					printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        					#endif
								*(fcdata->markbit) |= 0x0800;
        					}
						
							*(fcdata->markbit) |= 0x4000;								
							*(fcdata->contmode) = 29;//network control mode
							phcon = 0;
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
            					#ifdef RCC_DEBUG
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
								#ifdef RCC_DEBUG
									printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									*(fcdata->markbit) |= 0x0800;
									gettimeofday(&ct,NULL);
                					update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                					if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																fcdata->softevent,fcdata->markbit))
                					{
                					#ifdef RCC_DEBUG
                   						printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                					#endif
                					}
								}
							}
							else
							{
							#ifdef RCC_DEBUG
								printf("face board serial port cannot write,Line:%d\n",__LINE__);
							#endif
							}
							sendfaceInfoToBoard(fcdata,fbdata);
							//send down time to configure tool
                            if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            {
							#ifdef RCC_DEBUG
								printf("send control info to configure tool,File: %s,Line: %d\n", \
											__FILE__,__LINE__);
							#endif
                                memset(&gttd,0,sizeof(gttd));
                                gttd.mode = 29;
                                gttd.pattern = *(fcdata->patternid);
                                gttd.lampcolor = 0x02;
                                gttd.lamptime = 0;
                                gttd.phaseid = pinfo.phaseid;
                                gttd.stageline = pinfo.stageline;
                                if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
                                {
                                #ifdef RCC_DEBUG
                                    printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
                            }
							#ifdef EMBED_CONFIGURE_TOOL
							if (*(fcdata->markbit2) & 0x0200)
							{
								memset(&gttd,0,sizeof(gttd));
                                gttd.mode = 29;
                                gttd.pattern = *(fcdata->patternid);
                                gttd.lampcolor = 0x02;
                                gttd.lamptime = 0;
                                gttd.phaseid = pinfo.phaseid;
                                gttd.stageline = pinfo.stageline;  
								if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
								{
								#ifdef RCC_DEBUG
									printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#endif
                            if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    pinfo.chan,0x02,fcdata->markbit))
                            {
                            #ifdef RCC_DEBUG
                                printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            #endif
                            }
							
							objecti[0].objectv[0] = 0xf2;
							factovs = 0;
							memset(cbuf,0,sizeof(cbuf));
							if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                            {
                            #ifdef RCC_DEBUG
                            	printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }
							if (!(*(fcdata->markbit) & 0x1000))
							{
                            	write(*(fcdata->sockfd->csockfd),cbuf,factovs);
							}
							gettimeofday(&ct,NULL);
                            update_event_list(fcdata->ec,fcdata->el,1,84,ct.tv_sec,fcdata->markbit);
                           	if(0 == rail_phase)
							{
								continue;
							}
							else
							{
								gtbuf[1] = rail_phase;
								rcc_get_last_phaseinfo(fcdata,tscdata,rettl,&pinfo);
							}
						}//net control will happen
						if ((0xf0 == gtbuf[1]) && (1 == netlock))
                        {//has been status of net control
                            objecti[0].objectv[0] = 0xf2;
                            factovs = 0;
                            memset(cbuf,0,sizeof(cbuf));
                            if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                            {
                            #ifdef RCC_DEBUG
                                printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }
                            if (!(*(fcdata->markbit) & 0x1000))
                            {
                                write(*(fcdata->sockfd->csockfd),cbuf,factovs);
                            }
                        }//has been status of net control
						if ((0xf1 == gtbuf[1]) && (0 == netlock))
                        {//not fit control or restrore
                            objecti[0].objectv[0] = 0xf3;
                            factovs = 0;
                            memset(cbuf,0,sizeof(cbuf));
                            if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x00))
                            {
                            #ifdef RCC_DEBUG
                                printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }
                            if (!(*(fcdata->markbit) & 0x1000))
                            {
                                write(*(fcdata->sockfd->csockfd),cbuf,factovs);
                            }
                        }//not fit control or restrore
						if ((0xf1 == gtbuf[1]) && (1 == netlock))
						{//net control will end
							netlock = 0;
					//		gtred = 0;
					//		close = 0;
							fpc = 0;
					/*
							if (1 == gtyfyes)
							{
								pthread_cancel(gtyfid);
								pthread_join(gtyfid,NULL);
								gtyfyes = 0;
							}
						*/
							*(fcdata->markbit) &= 0xbfff;
							if (0 == cp)
                            {//not have phase control
                                if ((0 == gtred) && (0 == rccyfyes) && (0 == close))
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
            								#ifdef RCC_DEBUG
                								printf("Info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0==pinfo.cchan[0])&&(pinfo.gftime>0)
									//||(pinfo.ytime>0)||(pinfo.rtime>0))
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
            								#ifdef RCC_DEBUG
                								printf("Info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
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
										memset(&gttd,0,sizeof(gttd));
										gttd.mode = 29;//traffic control
										gttd.pattern = *(fcdata->patternid);
										gttd.lampcolor = 0x02;
										gttd.lamptime = pinfo.gftime;
										gttd.phaseid = pinfo.phaseid;
										gttd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 29;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x02;
                                        gttd.lamptime = pinfo.gftime;
                                        gttd.phaseid = pinfo.phaseid;
                                        gttd.stageline = pinfo.stageline;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                                		#ifdef RCC_DEBUG
                                    		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                    		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                    		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                    		{
                                    		#ifdef RCC_DEBUG
                                        		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
                                    		}
                                		}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
                                		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
	
									ngf = 0;
									while (1)
									{
										if(SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x03))
        								{
        								#ifdef RCC_DEBUG
            								printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															pinfo.cchan,0x03,fcdata->markbit))
        								{
        								#ifdef RCC_DEBUG
            								printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
        								#endif
        								}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);

										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x02))
        								{
        								#ifdef RCC_DEBUG
            								printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    									pinfo.cchan,0x02,fcdata->markbit))
                                		{
                                		#ifdef RCC_DEBUG
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
									if (1 == (pinfo.cpcexist))
									{
										//current phase begin to yellow lamp
										if(SUCCESS!=rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cnpchan,0x01))
        								{
        								#ifdef RCC_DEBUG
            								printf("set yellow lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
													pinfo.cnpchan,0x01, fcdata->markbit))
										{
										#ifdef RCC_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if(SUCCESS!=rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cpchan,0x00))
										{
										#ifdef RCC_DEBUG
											printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																	pinfo.cpchan,0x00,fcdata->markbit))
										{
										#ifdef RCC_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x01))
										{
										#ifdef RCC_DEBUG
											printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														pinfo.cchan,0x01,fcdata->markbit))
										{
										#ifdef RCC_DEBUG
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
            								#ifdef RCC_DEBUG
                								printf("Info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
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
                               			memset(&gttd,0,sizeof(gttd));
                               			gttd.mode = 29;//traffic control
                               			gttd.pattern = *(fcdata->patternid);
                               			gttd.lampcolor = 0x01;
                               			gttd.lamptime = pinfo.ytime;
                               			gttd.phaseid = pinfo.phaseid;
                               			gttd.stageline = pinfo.stageline;
                               			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
                               			{
                               			#ifdef RCC_DEBUG
                                   			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               			#endif
                               			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 29;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x01;
                                        gttd.lamptime = pinfo.ytime;
                                        gttd.phaseid = pinfo.phaseid;
                                        gttd.stageline = pinfo.stageline;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                               			#ifdef RCC_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef RCC_DEBUG
                                       			printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x00))
                            		{
                            		#ifdef RCC_DEBUG
                                		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															pinfo.cchan,0x00,fcdata->markbit))
									{
									#ifdef RCC_DEBUG
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
            								#ifdef RCC_DEBUG
                								printf("Info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
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
                                			memset(&gttd,0,sizeof(gttd));
                                			gttd.mode = 29;//traffic control
                                			gttd.pattern = *(fcdata->patternid);
                                			gttd.lampcolor = 0x00;
                                			gttd.lamptime = pinfo.rtime;
                                			gttd.phaseid = pinfo.phaseid;
                                			gttd.stageline = pinfo.stageline;
                                			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
                                			{
                                			#ifdef RCC_DEBUG
                                    			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                            			}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&gttd,0,sizeof(gttd));
                                            gttd.mode = 29;//traffic control
                                            gttd.pattern = *(fcdata->patternid);
                                            gttd.lampcolor = 0x00;
                                            gttd.lamptime = pinfo.rtime;
                                            gttd.phaseid = pinfo.phaseid;
                                            gttd.stageline = pinfo.stageline;  
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
											{
											#ifdef RCC_DEBUG
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
                                    		#ifdef RCC_DEBUG
                                        		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
                                        		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                        		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                        		{
                                        		#ifdef RCC_DEBUG
                                            		printf("File err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        		#endif
                                        		}
                                    		}
                                		}
                                		else
                                		{
                                		#ifdef RCC_DEBUG
                                    		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                		#endif
                                		}
										sleep(pinfo.rtime);
									}

									*(fcdata->slnum) += 1;
									*(fcdata->stageid) = \
									tscdata->timeconfig->TimeConfigList[rettl][*(fcdata->slnum)].StageId;
								if(0 == (tscdata->timeconfig->TimeConfigList[rettl][*(fcdata->slnum)].StageId))
									{
										*(fcdata->slnum) = 0;
										*(fcdata->stageid) = \
										tscdata->timeconfig->TimeConfigList[rettl][*(fcdata->slnum)].StageId;
									}
                                }//not have all red/yellow flash/close
                                else
                                {//have all red or yellow flash or all close
                                    gtred = 0;
                                    close = 0;
                                    if (1 == rccyfyes)
                                    {
                                        pthread_cancel(rccyfid);
                                        pthread_join(rccyfid,NULL);
                                        rccyfyes = 0;
                                    }
                                }//have all red or yellow flash or all close
                            }//not have phase control
							else if (0 != cp)
							{//phase control happen
								if ((0x5a <= cp) && (cp <= 0x5d))
								{//if ((0x5a <= cp) && (cp <= 0x5d))
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
							
									if((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = cp;
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
            								#ifdef RCC_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}
									if ((0 != wcc[0]) && (pinfo.gftime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = cp;
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
            								#ifdef RCC_DEBUG
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
										memset(&gttd,0,sizeof(gttd));
										gttd.mode = 29;//traffic control
										gttd.pattern = *(fcdata->patternid);
										gttd.lampcolor = 0x02;
										gttd.lamptime = pinfo.gftime;
										gttd.phaseid = 0;
										gttd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 29;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x02;
                                        gttd.lamptime = pinfo.gftime;
                                        gttd.phaseid = 0;
                                        gttd.stageline = pinfo.stageline;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                               			#ifdef RCC_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef RCC_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									ngf = 0;
									while (1)
									{
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
       									{
       									#ifdef RCC_DEBUG
           									printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
       									#endif
											*(fcdata->markbit) |= 0x0800;
       									}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x03,fcdata->markbit))
       									{
        								#ifdef RCC_DEBUG
            								printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
        								#endif
        								}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);

										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
        								{
        								#ifdef RCC_DEBUG
            								printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    											wcc,0x02,fcdata->markbit))
                                		{
                                		#ifdef RCC_DEBUG
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
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef RCC_DEBUG
            								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef RCC_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef RCC_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef RCC_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef RCC_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef RCC_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}

									if ((0 != wcc[0]) && (pinfo.ytime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = cp;
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
            								#ifdef RCC_DEBUG
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
                            			memset(&gttd,0,sizeof(gttd));
                            			gttd.mode = 29;//traffic control
                            			gttd.pattern = *(fcdata->patternid);
                            			gttd.lampcolor = 0x01;
                            			gttd.lamptime = pinfo.ytime;
                            			gttd.phaseid = 0;
                            			gttd.stageline = pinfo.stageline;
                            			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
                            			{
                            			#ifdef RCC_DEBUG
                               				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            			#endif
                            			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 29;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x01;
                                        gttd.lamptime = pinfo.ytime;
                                        gttd.phaseid = 0;
                                        gttd.stageline = pinfo.stageline;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                            			#ifdef RCC_DEBUG
                               				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                			{
                                			#ifdef RCC_DEBUG
                                   				printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef RCC_DEBUG
                                		printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef RCC_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}

									if ((0 != wcc[0]) && (pinfo.rtime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = cp;
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
            								#ifdef RCC_DEBUG
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
								}//if ((0x5a <= cp) && (cp <= 0x5d))
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
								
										if((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
										{
											if((!(*(fcdata->markbit)&0x1000))&&(!(*(fcdata->markbit)&0x8000)))
											{//report info to server actively
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
												#ifdef RCC_DEBUG
													printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
												else
												{
													write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
												}
											}//report info to server actively
										}
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
												#ifdef RCC_DEBUG
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
											memset(&gttd,0,sizeof(gttd));
											gttd.mode = 29;//traffic control
											gttd.pattern = *(fcdata->patternid);
											gttd.lampcolor = 0x02;
											gttd.lamptime = pinfo.gftime;
											gttd.phaseid = cp;
											gttd.stageline = pinfo.stageline;
											if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
											{
											#ifdef RCC_DEBUG
												printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&gttd,0,sizeof(gttd));
                                            gttd.mode = 29;//traffic control
                                            gttd.pattern = *(fcdata->patternid);
                                            gttd.lampcolor = 0x02;
                                            gttd.lamptime = pinfo.gftime;
                                            gttd.phaseid = cp;
                                            gttd.stageline = pinfo.stageline;  
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
											{
											#ifdef RCC_DEBUG
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
											#ifdef RCC_DEBUG
												printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
												update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
												if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
												{
												#ifdef RCC_DEBUG
													printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
											}
										}
										else
										{
										#ifdef RCC_DEBUG
											printf("face board serial port cannot write,Line:%d\n",__LINE__);
										#endif
										}
										ngf = 0;
										while (1)
										{
											if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef RCC_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x03,fcdata->markbit))
											{
											#ifdef RCC_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef RCC_DEBUG
												printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
											{
											#ifdef RCC_DEBUG
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
										if (1 == pce)
										{
											//current phase begin to yellow lamp
											if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
											{
											#ifdef RCC_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wnpcc,0x01, fcdata->markbit))
											{
											#ifdef RCC_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
											{
											#ifdef RCC_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wpcc,0x00,fcdata->markbit))
											{
											#ifdef RCC_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										else
										{
											if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
											{
											#ifdef RCC_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x01,fcdata->markbit))
											{
											#ifdef RCC_DEBUG
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
												#ifdef RCC_DEBUG
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
											memset(&gttd,0,sizeof(gttd));
											gttd.mode = 29;//traffic control
											gttd.pattern = *(fcdata->patternid);
											gttd.lampcolor = 0x01;
											gttd.lamptime = pinfo.ytime;
											gttd.phaseid = cp;
											gttd.stageline = pinfo.stageline;
											if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
											{
											#ifdef RCC_DEBUG
												printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&gttd,0,sizeof(gttd));
                                            gttd.mode = 29;//traffic control
                                            gttd.pattern = *(fcdata->patternid);
                                            gttd.lampcolor = 0x01;
                                            gttd.lamptime = pinfo.ytime;
                                            gttd.phaseid = cp;
                                            gttd.stageline = pinfo.stageline;  
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
											{
											#ifdef RCC_DEBUG
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
											#ifdef RCC_DEBUG
												printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
												update_event_list(fcdata->ec,fcdata->el,1,16, \
																		ct.tv_sec,fcdata->markbit);
												if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
												{
												#ifdef RCC_DEBUG
													printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
											}
										}
										else
										{
										#ifdef RCC_DEBUG
											printf("face board serial port cannot write,Line:%d\n",__LINE__);
										#endif
										}
										sleep(pinfo.ytime);

										//current phase begin to red lamp
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
										{
										#ifdef RCC_DEBUG
											printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x00,fcdata->markbit))
										{
										#ifdef RCC_DEBUG
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
												#ifdef RCC_DEBUG
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
								
										if((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
										{
											if((!(*(fcdata->markbit)&0x1000))&&(!(*(fcdata->markbit)&0x8000)))
											{//report info to server actively
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
												#ifdef RCC_DEBUG
													printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
												else
												{
													write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
												}
											}//report info to server actively
										}
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
												#ifdef RCC_DEBUG
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
											memset(&gttd,0,sizeof(gttd));
											gttd.mode = 29;//traffic control
											gttd.pattern = *(fcdata->patternid);
											gttd.lampcolor = 0x02;
											gttd.lamptime = pinfo.gftime;
											gttd.phaseid = cp;
											gttd.stageline = pinfo.stageline;
											if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
											{
											#ifdef RCC_DEBUG
												printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&gttd,0,sizeof(gttd));
                                            gttd.mode = 29;//traffic control
                                            gttd.pattern = *(fcdata->patternid);
                                            gttd.lampcolor = 0x02;
                                            gttd.lamptime = pinfo.gftime;
                                            gttd.phaseid = cp;
                                            gttd.stageline = pinfo.stageline;  
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
											{
											#ifdef RCC_DEBUG
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
											#ifdef RCC_DEBUG
												printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
												update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
												if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
												{
												#ifdef RCC_DEBUG
													printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
											}
										}
										else
										{
										#ifdef RCC_DEBUG
											printf("face board serial port cannot write,Line:%d\n",__LINE__);
										#endif
										}
										ngf = 0;
										while (1)
										{
											if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef RCC_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x03,fcdata->markbit))
											{
											#ifdef RCC_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef RCC_DEBUG
												printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
											{
											#ifdef RCC_DEBUG
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
										if (1 == pce)
										{
											//current phase begin to yellow lamp
											if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
											{
											#ifdef RCC_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wnpcc,0x01, fcdata->markbit))
											{
											#ifdef RCC_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
											{
											#ifdef RCC_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wpcc,0x00,fcdata->markbit))
											{
											#ifdef RCC_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										else
										{
											if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
											{
											#ifdef RCC_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x01,fcdata->markbit))
											{
											#ifdef RCC_DEBUG
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
												#ifdef RCC_DEBUG
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
											memset(&gttd,0,sizeof(gttd));
											gttd.mode = 29;//traffic control
											gttd.pattern = *(fcdata->patternid);
											gttd.lampcolor = 0x01;
											gttd.lamptime = pinfo.ytime;
											gttd.phaseid = cp;
											gttd.stageline = pinfo.stageline;
											if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
											{
											#ifdef RCC_DEBUG
												printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&gttd,0,sizeof(gttd));
                                            gttd.mode = 29;//traffic control
                                            gttd.pattern = *(fcdata->patternid);
                                            gttd.lampcolor = 0x01;
                                            gttd.lamptime = pinfo.ytime;
                                            gttd.phaseid = cp;
                                            gttd.stageline = pinfo.stageline;  
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
											{
											#ifdef RCC_DEBUG
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
											#ifdef RCC_DEBUG
												printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
												update_event_list(fcdata->ec,fcdata->el,1,16, \
																		ct.tv_sec,fcdata->markbit);
												if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
												{
												#ifdef RCC_DEBUG
													printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
											}
										}
										else
										{
										#ifdef RCC_DEBUG
											printf("face board serial port cannot write,Line:%d\n",__LINE__);
										#endif
										}
										sleep(pinfo.ytime);

										//current phase begin to red lamp
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
										{
										#ifdef RCC_DEBUG
											printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x00,fcdata->markbit))
										{
										#ifdef RCC_DEBUG
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
												#ifdef RCC_DEBUG
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
						#if 0
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
            						#ifdef RCC_DEBUG
                						printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively	
							}
						#endif
							*(fcdata->contmode) = contmode; //restore control mode;
							ncmode = *(fcdata->contmode);
							*(fcdata->markbit2) &= 0xfff7;
							cp = 0;
							if (0 == rccyes)
    						{
        						memset(&rccdata,0,sizeof(rccdata_t));
        						memset(&pinfo,0,sizeof(rccpinfo_t));
        						rccdata.fd = fcdata;
        						rccdata.td = tscdata;
        						rccdata.pi = &pinfo;
        						rccdata.cs = chanstatus;
        						int gtret = \
								pthread_create(&rccid,NULL,(void *)start_rail_crossing_control,&rccdata);
        						if (0 != gtret)
        						{
        						#ifdef RCC_DEBUG
            						printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
        						#endif
            						return FAIL;
        						}
        						rccyes = 1;
    						}
							
							objecti[0].objectv[0] = 0xf3;
							factovs = 0;
							memset(cbuf,0,sizeof(cbuf));
							if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                            {
                            #ifdef RCC_DEBUG
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
						if ((1 == netlock) && ((0xf0 != gtbuf[1]) || (0xf1 != gtbuf[1])))
						{//status of network control
							if ((0x5a <= gtbuf[1]) && (gtbuf[1] <= 0x5d))
							{//if ((0x5a <= gtbuf[1]) && (gtbuf[1] <= 0x5d))
								if ((1 == gtred) || (1 == rccyfyes) || (1 == close))
                            	{//if ((1 == tcred) || (1 == tcyfyes) || (1 == close))
									fpc = 1;
									cp = gtbuf[1];
                                	if (1 == rccyfyes)
                                	{
                                    	pthread_cancel(rccyfid);
                                    	pthread_join(rccyfid,NULL);
                                    	rccyfyes = 0;
                                	}
									if (1 != gtred)
									{
										new_all_red(&ardata);
									}
									gtred = 0;
									close = 0;

                                	if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),lkch[cp-0x5a],0x02))
                                	{
                                	#ifdef RCC_DEBUG
                                    	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
                                    	*(fcdata->markbit) |= 0x0800;
                                	}
                                	if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                                        				lkch[cp-0x5a],0x02,fcdata->markbit))
                                	{
                                	#ifdef RCC_DEBUG
                                    	printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                	#endif
                                	}
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
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
            							#ifdef RCC_DEBUG
                							printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            							#endif
            							}
            							else
            							{
                							write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            							}
									}//report info to server actively
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,89,ct.tv_sec,fcdata->markbit);
                                	continue;
                            	}//if ((1 == tcred) || (1 == tcyfyes) || (1 == close))
								if (0 == fpc)
								{//phase control of first times
									fpc = 1;
									cp = pinfo.phaseid;
								}//phase control of first times

								if (cp == gtbuf[1])
								{
									if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
									{
										sinfo.conmode = cp;
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
            							#ifdef RCC_DEBUG
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
                                    #ifdef RCC_DEBUG
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
												*pcc = tscdata->channel->ChannelList[i].ChannelId;
												pcc++;
												ce = 0;
												for (j = 0; j < MAX_CHANNEL; j++)
												{
													if (0 == lkch[gtbuf[1]-0x5a][j])
													break;
													if (lkch[gtbuf[1]-0x5a][j] == \
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
									}//if ((0x01 <= cp) && (cp <= 0x20))
                                    else if ((0x5a <= cp) && (cp <= 0x5d))
                                    {//else if ((0x5a <= cp) && (cp <= 0x5d))
										memset(cc,0,sizeof(cc));
                                        pcc = cc;
                                        for (i = 0; i < MAX_CHANNEL; i++)
                                        {
                                            if (0 == lkch[cp-0x5a][i])
                                                break;
                                            *pcc = lkch[cp-0x5a][i];
                                            pcc++;
                                        }
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
											for (j = 0; j < MAX_CHANNEL; j++)
                                            {
                                                if (0 == lkch[gtbuf[1]-0x5a][j])
                                                    break;
                                                if (lkch[gtbuf[1]-0x5a][j] == cc[i])
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
 
									if ((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
                                                sinfo.conmode = /*29*/ncmode;
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
                                                sinfo.conmode = cp;
											sinfo.color = 0x02;
											sinfo.time = pinfo.gftime + pinfo.ytime + pinfo.rtime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                sinfo.phase = 0;
                                                sinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
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
            								#ifdef RCC_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}
									if ((0 != wcc[0]) && (pinfo.gftime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
                                                sinfo.conmode = /*29*/ncmode;
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
                                                sinfo.conmode = cp;
											sinfo.color = 0x03;
											sinfo.time = pinfo.gftime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                sinfo.phase = 0;
                                                sinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
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
            								#ifdef RCC_DEBUG
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
										memset(&gttd,0,sizeof(gttd));
										gttd.mode = 29;//traffic control
										gttd.pattern = *(fcdata->patternid);
										gttd.lampcolor = 0x02;
										gttd.lamptime = pinfo.gftime;
										gttd.phaseid = 0;
										gttd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 29;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x02;
                                        gttd.lamptime = pinfo.gftime;
                                        gttd.phaseid = 0;
                                        gttd.stageline = pinfo.stageline;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                               			#ifdef RCC_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef RCC_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}

									ngf = 0;
									while (1)
									{
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
       									{
       									#ifdef RCC_DEBUG
           									printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
       									#endif
											*(fcdata->markbit) |= 0x0800;
       									}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x03,fcdata->markbit))
       									{
        								#ifdef RCC_DEBUG
            								printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
        								#endif
        								}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);

										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
        								{
        								#ifdef RCC_DEBUG
            								printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    											wcc,0x02,fcdata->markbit))
                                		{
                                		#ifdef RCC_DEBUG
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
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef RCC_DEBUG
            								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef RCC_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef RCC_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef RCC_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef RCC_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef RCC_DEBUG
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
											sinfo.color = 0x01;
											sinfo.time = pinfo.ytime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                sinfo.phase = 0;
                                                sinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
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
            								#ifdef RCC_DEBUG
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
                            			memset(&gttd,0,sizeof(gttd));
                            			gttd.mode = 29;//traffic control
                            			gttd.pattern = *(fcdata->patternid);
                            			gttd.lampcolor = 0x01;
                            			gttd.lamptime = pinfo.ytime;
                            			gttd.phaseid = 0;
                            			gttd.stageline = pinfo.stageline;
                            			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
                            			{
                            			#ifdef RCC_DEBUG
                               				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            			#endif
                            			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 29;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x01;
                                        gttd.lamptime = pinfo.ytime;
                                        gttd.phaseid = 0;
                                        gttd.stageline = pinfo.stageline;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                            			#ifdef RCC_DEBUG
                               				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                			{
                                			#ifdef RCC_DEBUG
                                   				printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef RCC_DEBUG
                                		printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef RCC_DEBUG
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
											sinfo.color = 0x00;
											sinfo.time = pinfo.rtime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                sinfo.phase = 0;
                                                sinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
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
            								#ifdef RCC_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}

									if (pinfo.rtime > 0)
									{
										if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            			{
                                			memset(&gttd,0,sizeof(gttd));
                                			gttd.mode = 29;//network control
                                			gttd.pattern = *(fcdata->patternid);
                                			gttd.lampcolor = 0x00;
                                			gttd.lamptime = pinfo.rtime;
                                			gttd.phaseid = 0;
                                			gttd.stageline = pinfo.stageline;
                                			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
                                			{
                                			#ifdef RCC_DEBUG
                                   				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                            			}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&gttd,0,sizeof(gttd));
                                            gttd.mode = 29;//network control
                                            gttd.pattern = *(fcdata->patternid);
                                            gttd.lampcolor = 0x00;
                                            gttd.lamptime = pinfo.rtime;
                                            gttd.phaseid = 0;
                                            gttd.stageline = pinfo.stageline;  
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
											{
											#ifdef RCC_DEBUG
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
                                   			#ifdef RCC_DEBUG
                                       			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   			#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
                                       			update_event_list(fcdata->ec,fcdata->el,1,16, \
																		ct.tv_sec,fcdata->markbit);
                                       			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                       			{
                                       			#ifdef RCC_DEBUG
                                           			printf("gen err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                       			#endif
                                       			}
                                   			}
                                		}
                                		else
                                		{
                                		#ifdef RCC_DEBUG
                                   			printf("face board port cannot write,Line:%d\n",__LINE__);
                                		#endif
                                		}
										sleep(pinfo.rtime);
									}

									if (SUCCESS != \
											rcc_set_lamp_color(*(fcdata->bbserial),lkch[gtbuf[1]-0x5a],0x02))
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																lkch[gtbuf[1]-0x5a],0x02,fcdata->markbit))
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
                            		}
									ncmode = *(fcdata->contmode);
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										sinfo.conmode = gtbuf[1];
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
                							if (0 == lkch[gtbuf[1]-0x5a][i])
                    							break;
                							sinfo.chans += 1;
                							tcsta = lkch[gtbuf[1]-0x5a][i];
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
            							#ifdef RCC_DEBUG
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
										memset(&gttd,0,sizeof(gttd));
										gttd.mode = 29;//traffic control
										gttd.pattern = *(fcdata->patternid);
										gttd.lampcolor = 0x02;
										gttd.lamptime = 0;
										gttd.phaseid = 0;
										gttd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 29;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x02;
                                        gttd.lamptime = 0;
                                        gttd.phaseid = 0;
                                        gttd.stageline = pinfo.stageline;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                               			#ifdef RCC_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef RCC_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									cp = gtbuf[1];

									objecti[0].objectv[0] = 0xf4;
									factovs = 0;
									memset(cbuf,0,sizeof(cbuf));
									if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                            		{
                            		#ifdef RCC_DEBUG
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
							}//if ((0x5a <= gtbuf[1]) && (gtbuf[1] <= 0x5d))
							else if ((0x01 <= gtbuf[1]) && (gtbuf[1] <= 0x20))
							{//control someone phase
								//check phaseid whether exist in phase list;
								pex = 0;
								for (i = 0; i < (tscdata->phase->FactPhaseNum); i++)
								{
									if (0 == (tscdata->phase->PhaseList[i].PhaseId))
										break;
									if (gtbuf[1] == (tscdata->phase->PhaseList[i].PhaseId))
									{
										pex = 1;
										break;
									}	
								}
								if (0 == pex)
								{
								#ifdef RCC_DEBUG
									printf("Not fit phase id,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									objecti[0].objectv[0] = 0xf4;
                                    factovs = 0;
                                    memset(cbuf,0,sizeof(cbuf));
                                    if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x00))
                                    {
                                    #ifdef RCC_DEBUG
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

								if ((1 == gtred) || (1 == rccyfyes) || (1 == close))
                            	{//if ((1 == tcred) || (1 == tcyfyes) || (1 == close))
									fpc = 1;
									cp = gtbuf[1];
                                	if (1 == rccyfyes)
                                	{
                                    	pthread_cancel(rccyfid);
                                    	pthread_join(rccyfid,NULL);
                                    	rccyfyes = 0;
                                	}
									if (1 != gtred)
									{
										new_all_red(&ardata);
									}
									gtred = 0;
									close = 0;

									memset(nc,0,sizeof(nc));
                                    pcc = nc;
                                    for (i = 0; i < (tscdata->channel->FactChannelNum); i++)
                                    {
                                        if (0 == (tscdata->channel->ChannelList[i].ChannelId))
                                            break;
                                        if (gtbuf[1] == (tscdata->channel->ChannelList[i].ChannelCtrlSrc))
                                        {
                                            *pcc = tscdata->channel->ChannelList[i].ChannelId;
                                            pcc++;
                                        }
                                    }
                                	if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),nc,0x02))
                                	{
                                	#ifdef RCC_DEBUG
                                    	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
                                    	*(fcdata->markbit) |= 0x0800;
                                	}
                                	if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                                        				nc,0x02,fcdata->markbit))
                                	{
                                	#ifdef RCC_DEBUG
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
            							#ifdef RCC_DEBUG
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

								if (cp == gtbuf[1])
								{
									memset(cc,0,sizeof(cc));
									pcc = cc;
									for (i = 0; i < (tscdata->channel->FactChannelNum); i++)
									{
										if (0 == (tscdata->channel->ChannelList[i].ChannelId))
											break;
										if (cp == (tscdata->channel->ChannelList[i].ChannelCtrlSrc))
										{
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
            							#ifdef RCC_DEBUG
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
                                    #ifdef RCC_DEBUG
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
                                        if (gtbuf[1] == (tscdata->channel->ChannelList[i].ChannelCtrlSrc))
                                        {
                                            *pcc = tscdata->channel->ChannelList[i].ChannelId;
                                            pcc++;
                                        }
                                    }
									if ((0x5a <= cp) && (cp <= 0x5d))
									{//if ((0x5a <= cp) && (cp <= 0x5d))
										memset(cc,0,sizeof(cc));
                                        pcc = cc;
                                        for (i = 0; i < MAX_CHANNEL; i++)
                                        {
                                            if (0 == lkch[cp-0x5a][i])
                                                break;
                                            *pcc = lkch[cp-0x5a][i];
                                            pcc++;
                                        }
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
									}//if ((0x5a <= cp) && (cp <= 0x5d))
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

									if ((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
                                                sinfo.conmode = /*29*/ncmode;
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
                                                sinfo.conmode = cp;
											sinfo.color = 0x02;
											sinfo.time = pinfo.gftime + pinfo.ytime + pinfo.rtime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                sinfo.phase = 0;
                                                sinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
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
            								#ifdef RCC_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}
									if ((0 != wcc[0]) && (pinfo.gftime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
                                                sinfo.conmode = /*29*/ncmode;
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
                                                sinfo.conmode = cp;
											sinfo.color = 0x03;
											sinfo.time = pinfo.gftime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                sinfo.phase = 0;
                                                sinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
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
            								#ifdef RCC_DEBUG
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
										memset(&gttd,0,sizeof(gttd));
										gttd.mode = 29;//traffic control
										gttd.pattern = *(fcdata->patternid);
										gttd.lampcolor = 0x02;
										gttd.lamptime = pinfo.gftime;
										gttd.phaseid = cp;
										gttd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 29;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x02;
                                        gttd.lamptime = pinfo.gftime;
                                        gttd.phaseid = cp;
                                        gttd.stageline = pinfo.stageline;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                               			#ifdef RCC_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef RCC_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}

									ngf = 0;
									while (1)
									{
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
       									{
       									#ifdef RCC_DEBUG
           									printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
       									#endif
											*(fcdata->markbit) |= 0x0800;
       									}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x03,fcdata->markbit))
       									{
        								#ifdef RCC_DEBUG
            								printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
        								#endif
        								}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);

										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
        								{
        								#ifdef RCC_DEBUG
            								printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    											wcc,0x02,fcdata->markbit))
                                		{
                                		#ifdef RCC_DEBUG
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
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef RCC_DEBUG
            								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef RCC_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef RCC_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef RCC_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef RCC_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef RCC_DEBUG
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
											sinfo.color = 0x01;
											sinfo.time = pinfo.ytime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                sinfo.phase = 0;
                                                sinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
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
            								#ifdef RCC_DEBUG
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
                            			memset(&gttd,0,sizeof(gttd));
                            			gttd.mode = 29;//traffic control
                            			gttd.pattern = *(fcdata->patternid);
                            			gttd.lampcolor = 0x01;
                            			gttd.lamptime = pinfo.ytime;
                            			gttd.phaseid = cp;
                            			gttd.stageline = pinfo.stageline;
                            			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
                            			{
                            			#ifdef RCC_DEBUG
                               				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            			#endif
                            			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 29;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x01;
                                        gttd.lamptime = pinfo.ytime;
                                        gttd.phaseid = cp;
                                        gttd.stageline = pinfo.stageline;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                            			#ifdef RCC_DEBUG
                               				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                			{
                                			#ifdef RCC_DEBUG
                                   				printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef RCC_DEBUG
                                		printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef RCC_DEBUG
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
											sinfo.color = 0x00;
											sinfo.time = pinfo.rtime;
											sinfo.stage = 0;
											sinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                sinfo.phase = 0;
                                                sinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
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
            								#ifdef RCC_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}

									if (pinfo.rtime > 0)
									{
										if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            			{
                                			memset(&gttd,0,sizeof(gttd));
                                			gttd.mode = 29;//network control
                                			gttd.pattern = *(fcdata->patternid);
                                			gttd.lampcolor = 0x00;
                                			gttd.lamptime = pinfo.rtime;
                                			gttd.phaseid = cp;
                                			gttd.stageline = pinfo.stageline;
                                			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
                                			{
                                			#ifdef RCC_DEBUG
                                   				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                            			}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&gttd,0,sizeof(gttd));
                                            gttd.mode = 29;//network control
                                            gttd.pattern = *(fcdata->patternid);
                                            gttd.lampcolor = 0x00;
                                            gttd.lamptime = pinfo.rtime;
                                            gttd.phaseid = cp;
                                            gttd.stageline = pinfo.stageline;  
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
											{
											#ifdef RCC_DEBUG
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
                                   			#ifdef RCC_DEBUG
                                       			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   			#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
                                       			update_event_list(fcdata->ec,fcdata->el,1,16, \
																		ct.tv_sec,fcdata->markbit);
                                       			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                       			{
                                       			#ifdef RCC_DEBUG
                                           			printf("gen err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                       			#endif
                                       			}
                                   			}
                                		}
                                		else
                                		{
                                		#ifdef RCC_DEBUG
                                   			printf("face board port cannot write,Line:%d\n",__LINE__);
                                		#endif
                                		}
										sleep(pinfo.rtime);
									}

									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),nc,0x02))
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			nc,0x02,fcdata->markbit))
                            		{
                            		#ifdef RCC_DEBUG
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
										sinfo.phase |= (0x01 << (gtbuf[1] - 1));

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
            							#ifdef RCC_DEBUG
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
										memset(&gttd,0,sizeof(gttd));
										gttd.mode = 29;//traffic control
										gttd.pattern = *(fcdata->patternid);
										gttd.lampcolor = 0x02;
										gttd.lamptime = 0;
										gttd.phaseid = gtbuf[1];
										gttd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 29;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x02;
                                        gttd.lamptime = 0;
                                        gttd.phaseid = gtbuf[1];
                                        gttd.stageline = pinfo.stageline;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                               			#ifdef RCC_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef RCC_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									cp = gtbuf[1];

									objecti[0].objectv[0] = 0xf4;
									factovs = 0;
									memset(cbuf,0,sizeof(cbuf));
									if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                            		{
                            		#ifdef RCC_DEBUG
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
							else if(0x27 == gtbuf[1])
							{//net step by step
								fpc = 0;
								if ((1 == gtred) || (1 == close) || (1 == rccyfyes))
								{
									if (1 == rccyfyes)
									{
										pthread_cancel(rccyfid);
										pthread_join(rccyfid,NULL);
										rccyfyes = 0;
									}
									if (1 != gtred)
									{
										new_all_red(&ardata);
									}
									gtred = 0;
									close = 0;
									if (0 == cp)
									{//not have phase control
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                                		{
                                		#ifdef RCC_DEBUG
                                    		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
                                    		*(fcdata->markbit) |= 0x0800;
                                		}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    								pinfo.chan,0x02,fcdata->markbit))
                                		{
                                		#ifdef RCC_DEBUG
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
            								#ifdef RCC_DEBUG
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
												rcc_set_lamp_color(*(fcdata->bbserial),lkch[cp-0x5a],0x02))
                                        	{
                                        	#ifdef RCC_DEBUG
                                            	printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        	#endif
                                            	*(fcdata->markbit) |= 0x0800;
                                        	}
                                        	if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                                                          lkch[cp-0x5a],0x02,fcdata->markbit))
                                        	{
                                        	#ifdef RCC_DEBUG
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
                                            	#ifdef RCC_DEBUG
                                                	printf("info err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                            	#endif
                                            	}
                                            	else
                                            	{
                                                	write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
                                            	}
											}//report info to server actively	
										}//if ((0x5a <= cp) && (cp <= 0x5d))
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
                                                	*pcc = tscdata->channel->ChannelList[i].ChannelId;
                                                	pcc++;
                                            	}
                                        	}
                                        	if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),cc,0x02))
                                        	{
                                        	#ifdef RCC_DEBUG
                                            	printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        	#endif
                                            	*(fcdata->markbit) |= 0x0800;
                                        	}
                                        	if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                                                            cc,0x02,fcdata->markbit))
                                        	{
                                        	#ifdef RCC_DEBUG
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

                                            	sinfo.chans = 0;
                                            	memset(sinfo.csta,0,sizeof(sinfo.csta));
                                            	csta = sinfo.csta;
						/*
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
                                            	#ifdef RCC_DEBUG
                                                	printf("info err,File:%s,Line:%d\n",__FILE__,__LINE__);
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
								}//if ((1 == gtred) || (1 == close) || (1 == gtyfyes))
							
								if (0 == cp)
								{	
								if ((0==pinfo.cchan[0])&&(pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0))
								{
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
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
            							#ifdef RCC_DEBUG
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
            							#ifdef RCC_DEBUG
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
									memset(&gttd,0,sizeof(gttd));
									gttd.mode = 29;//traffic control
									gttd.pattern = *(fcdata->patternid);
									gttd.lampcolor = 0x02;
									gttd.lamptime = pinfo.gftime;
									gttd.phaseid = pinfo.phaseid;
									gttd.stageline = pinfo.stageline;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
									{
									#ifdef RCC_DEBUG
										printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef EMBED_CONFIGURE_TOOL
                                if (*(fcdata->markbit2) & 0x0200)
                                {
                                 	memset(&gttd,0,sizeof(gttd));
                                    gttd.mode = 29;//traffic control
                                    gttd.pattern = *(fcdata->patternid);
                                    gttd.lampcolor = 0x02;
                                    gttd.lamptime = pinfo.gftime;
                                    gttd.phaseid = pinfo.phaseid;
                                    gttd.stageline = pinfo.stageline; 
                                    if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
                                    {
                                    #ifdef RCC_DEBUG
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
                                	#ifdef RCC_DEBUG
                                    	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef RCC_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                	}
                            	}
                            	else
                            	{
                            	#ifdef RCC_DEBUG
                                	printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            	#endif
                            	}
	
								ngf = 0;
								while (1)
								{
									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x03))
        							{
        							#ifdef RCC_DEBUG
            							printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        							#endif
										*(fcdata->markbit) |= 0x0800;
        							}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															pinfo.cchan,0x03,fcdata->markbit))
        							{
        							#ifdef RCC_DEBUG
            							printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
        							#endif
        							}
									to.tv_sec = 0;
									to.tv_usec = 500000;
									select(0,NULL,NULL,NULL,&to);

									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x02))
        							{
        							#ifdef RCC_DEBUG
            							printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        							#endif
										*(fcdata->markbit) |= 0x0800;
        							}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    									pinfo.cchan,0x02,fcdata->markbit))
                                	{
                                	#ifdef RCC_DEBUG
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
								if (1 == (pinfo.cpcexist))
								{
									//current phase begin to yellow lamp
									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cnpchan,0x01))
        							{
        							#ifdef RCC_DEBUG
            							printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        							#endif
										*(fcdata->markbit) |= 0x0800;
        							}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
													pinfo.cnpchan,0x01, fcdata->markbit))
									{
									#ifdef RCC_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cpchan,0x00))
									{
									#ifdef RCC_DEBUG
										printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																	pinfo.cpchan,0x00,fcdata->markbit))
									{
									#ifdef RCC_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
								}
								else
								{
									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x01))
									{
									#ifdef RCC_DEBUG
										printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														pinfo.cchan,0x01,fcdata->markbit))
									{
									#ifdef RCC_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
								}

								if ((0 != pinfo.cchan[0]) && (pinfo.ytime > 0))
								{
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
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
            							#ifdef RCC_DEBUG
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
                               		memset(&gttd,0,sizeof(gttd));
                               		gttd.mode = 29;//traffic control
                               		gttd.pattern = *(fcdata->patternid);
                               		gttd.lampcolor = 0x01;
                               		gttd.lamptime = pinfo.ytime;
                               		gttd.phaseid = pinfo.phaseid;
                               		gttd.stageline = pinfo.stageline;
                               		if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
                               		{
                               		#ifdef RCC_DEBUG
                                   		printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               		#endif
                               		}
                            	}
								#ifdef EMBED_CONFIGURE_TOOL
                                if (*(fcdata->markbit2) & 0x0200)
                                {
                                 	memset(&gttd,0,sizeof(gttd));
                                    gttd.mode = 29;//traffic control
                                    gttd.pattern = *(fcdata->patternid);
                                    gttd.lampcolor = 0x01;
                                    gttd.lamptime = pinfo.ytime;
                                    gttd.phaseid = pinfo.phaseid;
                                    gttd.stageline = pinfo.stageline; 
                                    if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
                                    {
                                    #ifdef RCC_DEBUG
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
                               		#ifdef RCC_DEBUG
                                   		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               		#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
                                   		update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                   		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   		{
                                   		#ifdef RCC_DEBUG
                                       		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   		#endif
                                   		}
                               		}
                            	}
                            	else
                            	{
                            	#ifdef RCC_DEBUG
                               		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            	#endif
                            	}
								sleep(pinfo.ytime);

								//current phase begin to red lamp
								if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x00))
                            	{
                            	#ifdef RCC_DEBUG
                                	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            	#endif
									*(fcdata->markbit) |= 0x0800;
                            	}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															pinfo.cchan,0x00,fcdata->markbit))
								{
								#ifdef RCC_DEBUG
									printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}

								if ((0 != pinfo.cchan[0]) && (pinfo.rtime > 0))
								{
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
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
            							#ifdef RCC_DEBUG
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
                                		memset(&gttd,0,sizeof(gttd));
                                		gttd.mode = 29;//traffic control
                                		gttd.pattern = *(fcdata->patternid);
                                		gttd.lampcolor = 0x00;
                                		gttd.lamptime = pinfo.rtime;
                                		gttd.phaseid = pinfo.phaseid;
                                		gttd.stageline = pinfo.stageline;
                                		if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
                                		{
                                		#ifdef RCC_DEBUG
                                    		printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                		#endif
                                		}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 29;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x00;
                                        gttd.lamptime = pinfo.rtime;
                                        gttd.phaseid = pinfo.phaseid;
                                        gttd.stageline = pinfo.stageline;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                                    	#ifdef RCC_DEBUG
                                        	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                        	update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                        	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                        	{
                                        	#ifdef RCC_DEBUG
                                            	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        	#endif
                                        	}
                                    	}
                                	}
                                	else
                                	{
                                	#ifdef RCC_DEBUG
                                    	printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                	#endif
                                	}
									sleep(pinfo.rtime);
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
								memset(&pinfo,0,sizeof(pinfo));
        						if (SUCCESS != rcc_get_phase_info(fcdata,tscdata,rettl,*(fcdata->slnum),&pinfo))
        						{
        						#ifdef RCC_DEBUG
            						printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
									output_log("Timing control,get phase info err");
        						#endif
									rcc_end_part_child_thread();
            						return FAIL;
        						}
        						*(fcdata->phaseid) = 0;
        						*(fcdata->phaseid) |= (0x01 << (pinfo.phaseid - 1));

								if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                            	{
                            	#ifdef RCC_DEBUG
                                	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            	#endif
									*(fcdata->markbit) |= 0x0800;
                            	}
						
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																pinfo.chan,0x02,fcdata->markbit))
                            	{
                            	#ifdef RCC_DEBUG
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
            						#ifdef RCC_DEBUG
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
									memset(&gttd,0,sizeof(gttd));
									gttd.mode = 29;//traffic control
									gttd.pattern = *(fcdata->patternid);
									gttd.lampcolor = 0x02;
									gttd.lamptime = 0;
									gttd.phaseid = pinfo.phaseid;
									gttd.stageline = pinfo.stageline;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
									{
									#ifdef RCC_DEBUG
										printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef EMBED_CONFIGURE_TOOL
                                if (*(fcdata->markbit2) & 0x0200)
                                {
                                 	memset(&gttd,0,sizeof(gttd));
                                    gttd.mode = 29;//traffic control
                                    gttd.pattern = *(fcdata->patternid);
                                    gttd.lampcolor = 0x02;
                                    gttd.lamptime = 0;
                                    gttd.phaseid = pinfo.phaseid;
                                    gttd.stageline = pinfo.stageline; 
                                    if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
                                    {
                                    #ifdef RCC_DEBUG
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
                                	#ifdef RCC_DEBUG
                                    	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef RCC_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                	}
                            	}
                            	else
                            	{
                            	#ifdef RCC_DEBUG
                                	printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            	#endif
                            	}							
								}//0 == cp
								if (0 != cp)
								{
									if ((0x5a <= cp) && (cp <= 0x5d))
									{//if ((0x5a <= cp) && (cp <= 0x5d))
										*(fcdata->slnum) = 0;
										*(fcdata->stageid) = \
												tscdata->timeconfig->TimeConfigList[rettl][0].StageId;
										memset(&pinfo,0,sizeof(pinfo));
										if(SUCCESS!=rcc_get_phase_info(fcdata,tscdata,rettl,0,&pinfo))
										{
										#ifdef RCC_DEBUG
											printf("phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
											output_log("Timing control,get phase info err");
										#endif
											rcc_end_part_child_thread();
											return FAIL;
										}
										*(fcdata->phaseid) = 0;
										*(fcdata->phaseid) |= (0x01 << (pinfo.phaseid - 1));	
										memset(cc,0,sizeof(cc));
                                        pcc = cc;
                                        for (i = 0; i < MAX_CHANNEL; i++)
                                        {
                                            if (0 == lkch[cp-0x5a][i])
                                                break;
                                            *pcc = lkch[cp-0x5a][i];
                                            pcc++;
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
										}	
									}//if ((0x5a <= cp) && (cp <= 0x5d))
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
												memset(&pinfo,0,sizeof(pinfo));
												if(SUCCESS!=rcc_get_phase_info(fcdata,tscdata,rettl,0,&pinfo))
												{
												#ifdef RCC_DEBUG
													printf("info err,File: %s,Line: %d\n",__FILE__,__LINE__);
													output_log("Timing control,get phase info err");
												#endif
													rcc_end_part_child_thread();
													return FAIL;
												}
												*(fcdata->phaseid) = 0;
												*(fcdata->phaseid) |= (0x01 << (pinfo.phaseid - 1));
											}
											else
											{
												*(fcdata->slnum) = i + 1;
												*(fcdata->stageid) = \
													tscdata->timeconfig->TimeConfigList[rettl][i+1].StageId;
												memset(&pinfo,0,sizeof(pinfo));
												if(SUCCESS!=rcc_get_phase_info(fcdata,tscdata,rettl,i+1,&pinfo))
												{
												#ifdef RCC_DEBUG
													printf("info err,File: %s,Line: %d\n",__FILE__,__LINE__);
													output_log("Timing control,get phase info err");
												#endif
													rcc_end_part_child_thread();
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
													tscdata->timeconfig->TimeConfigList[rettl][0].StageId;
											memset(&pinfo,0,sizeof(pinfo));
											if(SUCCESS != rcc_get_phase_info(fcdata,tscdata,rettl,0,&pinfo))
											{
											#ifdef RCC_DEBUG
												printf("phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
												output_log("Timing control,get phase info err");
											#endif
												rcc_end_part_child_thread();
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
												*pcc = tscdata->channel->ChannelList[i].ChannelId;
												pcc++;
												ce = 0;
												for (j = 0; j < MAX_CHANNEL; j++)
												{
													if (0 == pinfo.chan[j])
														break;
												if (pinfo.chan[j]==tscdata->channel->ChannelList[i].ChannelId)
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
											sinfo.color = 0x02;
											sinfo.time = pinfo.gftime + pinfo.ytime + pinfo.rtime;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                sinfo.phase = 0;
                                                sinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
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
            								#ifdef RCC_DEBUG
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
											sinfo.color = 0x03;
											sinfo.time = pinfo.gftime;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                sinfo.phase = 0;
                                                sinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
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
            								#ifdef RCC_DEBUG
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
										memset(&gttd,0,sizeof(gttd));
										gttd.mode = 29;//traffic control
										gttd.pattern = *(fcdata->patternid);
										gttd.lampcolor = 0x02;
										gttd.lamptime = pinfo.gftime;
										gttd.phaseid = cp;
										gttd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 29;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x02;
                                        gttd.lamptime = pinfo.gftime;
                                        gttd.phaseid = cp;
                                        gttd.stageline = pinfo.stageline;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                                		#ifdef RCC_DEBUG
                                    		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                    		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                    		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                    		{
                                    		#ifdef RCC_DEBUG
                                        		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
                                    		}
                                		}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
                                		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
	
									ngf = 0;
									while (1)
									{
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
        								{
        								#ifdef RCC_DEBUG
            								printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																		wcc,0x03,fcdata->markbit))
        								{
        								#ifdef RCC_DEBUG
            								printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
        								#endif
        								}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);

										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
        								{
        								#ifdef RCC_DEBUG
            								printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    										wcc,0x02,fcdata->markbit))
                                		{
                                		#ifdef RCC_DEBUG
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
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS!=rcc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef RCC_DEBUG
            								printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef RCC_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef RCC_DEBUG
											printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef RCC_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef RCC_DEBUG
											printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														wcc,0x01,fcdata->markbit))
										{
										#ifdef RCC_DEBUG
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
											sinfo.color = 0x01;
											sinfo.time = pinfo.ytime;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                sinfo.phase = 0;
                                                sinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
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
            								#ifdef RCC_DEBUG
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
                               			memset(&gttd,0,sizeof(gttd));
                               			gttd.mode = 29;//traffic control
                               			gttd.pattern = *(fcdata->patternid);
                               			gttd.lampcolor = 0x01;
                               			gttd.lamptime = pinfo.ytime;
                               			gttd.phaseid = cp;
                               			gttd.stageline = pinfo.stageline;
                               			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
                               			{
                               			#ifdef RCC_DEBUG
                                   			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               			#endif
                               			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 29;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x01;
                                        gttd.lamptime = pinfo.ytime;
                                        gttd.phaseid = cp;
                                        gttd.stageline = pinfo.stageline;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                               			#ifdef RCC_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef RCC_DEBUG
                                       			printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef RCC_DEBUG
                                		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															wcc,0x00,fcdata->markbit))
									{
									#ifdef RCC_DEBUG
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
											sinfo.color = 0x00;
											sinfo.time = pinfo.rtime;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                sinfo.phase = 0;
                                                sinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
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
            								#ifdef RCC_DEBUG
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
                                			memset(&gttd,0,sizeof(gttd));
                                			gttd.mode = 29;//traffic control
                                			gttd.pattern = *(fcdata->patternid);
                                			gttd.lampcolor = 0x00;
                                			gttd.lamptime = pinfo.rtime;
                                			gttd.phaseid = cp;
                                			gttd.stageline = pinfo.stageline;
                                			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
                                			{
                                			#ifdef RCC_DEBUG
                                    			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                            			}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&gttd,0,sizeof(gttd));
                                            gttd.mode = 29;//traffic control
                                            gttd.pattern = *(fcdata->patternid);
                                            gttd.lampcolor = 0x00;
                                            gttd.lamptime = pinfo.rtime;
                                            gttd.phaseid = cp;
                                            gttd.stageline = pinfo.stageline;  
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
											{
											#ifdef RCC_DEBUG
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
                                    		#ifdef RCC_DEBUG
                                        		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
                                        		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                        		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                        		{
                                        		#ifdef RCC_DEBUG
                                            		printf("file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        		#endif
                                        		}
                                    		}
                                		}
                                		else
                                		{
                                		#ifdef RCC_DEBUG
                                    		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                		#endif
                                		}
										sleep(pinfo.rtime);
									}
									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                            		{
                            		#ifdef RCC_DEBUG
                               			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			pinfo.chan,0x02,fcdata->markbit))
                            		{
                            		#ifdef RCC_DEBUG
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
            							#ifdef RCC_DEBUG
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
										memset(&gttd,0,sizeof(gttd));
										gttd.mode = 29;//traffic control
										gttd.pattern = *(fcdata->patternid);
										gttd.lampcolor = 0x02;
										gttd.lamptime = 0;
										gttd.phaseid = pinfo.phaseid;
										gttd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&gttd,0,sizeof(gttd));
                                        gttd.mode = 29;//traffic control
                                        gttd.pattern = *(fcdata->patternid);
                                        gttd.lampcolor = 0x02;
                                        gttd.lamptime = 0;
                                        gttd.phaseid = pinfo.phaseid;
                                        gttd.stageline = pinfo.stageline;  
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
										{
										#ifdef RCC_DEBUG
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
                               			#ifdef RCC_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef RCC_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef RCC_DEBUG
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
                                #ifdef RCC_DEBUG
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
							else if(0x21 == gtbuf[1])
							{//yellow flash
								gtred = 0;
								close = 0;
								cp = 0;
								if (0 == rccyfyes)
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
									int yfret = pthread_create(&rccyfid,NULL,(void *)rcc_yellow_flash,&yfdata);
									if (0 != yfret)
									{
									#ifdef RCC_DEBUG
										printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										rcc_end_part_child_thread();
										objecti[0].objectv[0] = 0x24;
                                		factovs = 0;
                                		memset(cbuf,0,sizeof(cbuf));
                                		if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x00))
                                		{
                                		#ifdef RCC_DEBUG
                                    		printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
                                		}
                                		if (!(*(fcdata->markbit) & 0x1000))
                                		{
                                    		write(*(fcdata->sockfd->csockfd),cbuf,factovs);
                                		}
										return FAIL;
									}
									rccyfyes = 1;
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
            						#ifdef RCC_DEBUG
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
                                #ifdef RCC_DEBUG
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
							else if (0x22 == gtbuf[1])
							{//all red
								close = 0;
								cp = 0;
								if (1 == rccyfyes)
								{
									pthread_cancel(rccyfid);
									pthread_join(rccyfid,NULL);
									rccyfyes = 0;
								}
					
								if (0 == gtred)
								{	
									new_all_red(&ardata);
									gtred = 1;
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
            						#ifdef RCC_DEBUG
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
                                #ifdef RCC_DEBUG
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
							else if (0x23 == gtbuf[1])
							{//close lamp
								gtred = 0;
								cp = 0;
								if (1 == rccyfyes)
                                {
                                    pthread_cancel(rccyfid);
                                    pthread_join(rccyfid,NULL);
                                    rccyfyes = 0;
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
            						#ifdef RCC_DEBUG
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
                                #ifdef RCC_DEBUG
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
				else if ((0xDA == gtbuf[0]) && (0xED == gtbuf[2]))
				{//key traffic control
					if ((0 == wllock) && (0 == netlock) && (0 == wtlock))
					{//key lock or unlock is valid only when wireless lock or unlock is invalid
						if (0x01 == gtbuf[1])
						{//lock or unlock
							if (0 == keylock)
							{//prepare to lock
							#ifdef RCC_DEBUG
								printf("Prepare to lock,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								get_rcc_status(&color,&leatime);
								if (2 != color)
								{//lamp color is not green
									struct timeval			spatime;
									unsigned char			endlock = 0;
									while (1)
									{//inline while loop
										spatime.tv_sec = 0;
										spatime.tv_usec = 200000;
										select(0,NULL,NULL,NULL,&spatime);
										memset(gtbuf,0,sizeof(gtbuf));
										read(*(fcdata->conpipe),gtbuf,sizeof(gtbuf));
										if ((0xDA == gtbuf[0]) && (0xED == gtbuf[2]))
										{
											if (0x01 == gtbuf[1])
											{
												endlock = 1;
												break;
											}
										}
										get_rcc_status(&color,&leatime);
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
								keylock = 1;
								gtred = 0;
								close = 0;
								rcc_end_child_thread();//end main thread and its child thread

								*(fcdata->markbit2) |= 0x0002;
							#if 0
								new_all_red(&ardata);
								if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
								{
								#ifdef RCC_DEBUG
									printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									gettimeofday(&ct,NULL);
									update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
									if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																			fcdata->softevent,fcdata->markbit))
									{
									#ifdef RCC_DEBUG
										printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
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
            						#ifdef RCC_DEBUG
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
								fbdata[2] = pinfo.stageline;
								fbdata[3] = 2;
								fbdata[4] = 0;
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
									{
									#ifdef RCC_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    	if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef RCC_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
									}
								}
								else
								{
								#ifdef RCC_DEBUG
									printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
								sendfaceInfoToBoard(fcdata,fbdata);
								gtecho[1] = 0x01;
								if (*(fcdata->markbit2) & 0x0800)
								{//comes from side door serial
									*(fcdata->markbit2) &= 0xf7ff;
									if (!wait_write_serial(*(fcdata->sdserial)))
                                    {
                                        if (write(*(fcdata->sdserial),gtecho,sizeof(gtecho)) < 0)
                                        {
                                        #ifdef RCC_DEBUG
                                            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                            *(fcdata->markbit) |= 0x0800;
                                        }
                                    }
                                    else
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                    #endif
                                    }
								}//comes from side door serial
								else
								{
									if (!wait_write_serial(*(fcdata->fbserial)))
									{
										if (write(*(fcdata->fbserial),gtecho,sizeof(gtecho)) < 0)
										{
										#ifdef RCC_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
											update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
											if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																			fcdata->softevent,fcdata->markbit))
											{
											#ifdef RCC_DEBUG
												printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
											}
										}
									}
									else
									{
									#ifdef RCC_DEBUG
										printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
								}
								//send down time to configure tool
                            	if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            	{
                                	memset(&gttd,0,sizeof(gttd));
                                	gttd.mode = 28;
                                	gttd.pattern = *(fcdata->patternid);
                                	gttd.lampcolor = 0x02;
                                	gttd.lamptime = 0;
                                	gttd.phaseid = pinfo.phaseid;
                                	gttd.stageline = pinfo.stageline;
                                	if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
                                	{
                                	#ifdef RCC_DEBUG
                                    	printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                	#endif
                                	}
                            	}
								#ifdef EMBED_CONFIGURE_TOOL
                                if (*(fcdata->markbit2) & 0x0200)
                                {
                                 	memset(&gttd,0,sizeof(gttd));
                                    gttd.mode = 28;
                                    gttd.pattern = *(fcdata->patternid);
                                    gttd.lampcolor = 0x02;
                                    gttd.lamptime = 0;
                                    gttd.phaseid = pinfo.phaseid;
                                    gttd.stageline = pinfo.stageline; 
                                    if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                                #endif
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    pinfo.chan,0x02,fcdata->markbit))
                                {
                                #ifdef RCC_DEBUG 
                                    printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
                                {//report info to server actively
                                    memset(err,0,sizeof(err));
                                    gettimeofday(&ct,NULL);
                                    if (SUCCESS != err_report(err,ct.tv_sec,22,1))
                                    {
                                    #ifdef RCC_DEBUG
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
							{//prepare to unlock
							#ifdef RCC_DEBUG
								printf("prepare to unlock,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								keylock = 0;
								gtred = 0;
								close = 0;
								if (1 == rccyfyes)
								{
									pthread_cancel(rccyfid);
									pthread_join(rccyfid,NULL);
									rccyfyes = 0;
								}
								gtecho[1] = 0x00;
								if (*(fcdata->markbit2) & 0x0800)
                                {//comes from side door serial
                                    *(fcdata->markbit2) &= 0xf7ff;
                                    if (!wait_write_serial(*(fcdata->sdserial)))
                                    {
                                        if (write(*(fcdata->sdserial),gtecho,sizeof(gtecho)) < 0)
                                        {
                                        #ifdef RCC_DEBUG
                                            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                            *(fcdata->markbit) |= 0x0800;
                                        }
                                    }
                                    else
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                    #endif
                                    }
                                }//comes from side door serial
								{
									if (!wait_write_serial(*(fcdata->fbserial)))
									{
										if (write(*(fcdata->fbserial),gtecho,sizeof(gtecho)) < 0)
										{
										#ifdef RCC_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
											update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
											if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
											{
											#ifdef RCC_DEBUG
												printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
											}
										}
									}
									else
									{
									#ifdef RCC_DEBUG
										printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
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
            						#ifdef RCC_DEBUG
                						printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively

								*(fcdata->contmode) = contmode;//restore control mode
								*(fcdata->markbit2) &= 0xfffd;
								if (0 == rccyes)
								{
									memset(&rccdata,0,sizeof(rccdata_t));
									memset(&pinfo,0,sizeof(rccpinfo_t));
									rccdata.fd = fcdata;
									rccdata.td = tscdata;
									rccdata.pi = &pinfo;
									rccdata.cs = chanstatus;
									int gtret = \
									pthread_create(&rccid,NULL,(void *)start_rail_crossing_control,&rccdata);
									if (0 != gtret)
									{
									#ifdef RCC_DEBUG
										printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										return FAIL;
									}
									rccyes = 1;
								}
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
                                {//report info to server actively
                                    memset(err,0,sizeof(err));
                                    gettimeofday(&ct,NULL);
                                    if (SUCCESS != err_report(err,ct.tv_sec,22,3))
                                    {
                                    #ifdef RCC_DEBUG
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
						if (((0x11 <= gtbuf[1]) && (gtbuf[1] <= 0x18)) && (1 == keylock))	
						{//jump stage control
						#ifdef RCC_DEBUG
							printf("gtbuf[1]:%d,File:%s,Line:%d\n",gtbuf[1],__FILE__,__LINE__);
						#endif
							memset(&mdt,0,sizeof(markdata_c));
							mdt.redl = &gtred;
							mdt.closel = &close;
							mdt.rettl = &rettl;
							mdt.yfl = &rccyfyes;
							mdt.yfid = &rccyfid;
							mdt.ardata = &ardata;
							mdt.fcdata = fcdata;
							mdt.tscdata = tscdata;
							mdt.chanstatus = chanstatus;
							mdt.sinfo = &sinfo;
							rcc_jump_stage_control(&mdt,gtbuf[1],&pinfo);	
						}//jump stage control
						if ((0x02 == gtbuf[1]) && (1 == keylock))
						{//step by step
							if ((1 == gtred) || (1 ==rccyfyes) || (1 == close))
							{
								if (1 == close)
								{
									new_all_red(&ardata);
									close = 0;
								}
								gtred = 0;
								if (1 == rccyfyes)
								{
									pthread_cancel(rccyfid);
									pthread_join(rccyfid,NULL);
									rccyfyes = 0;
									new_all_red(&ardata);
								}
								if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
								{
								#ifdef RCC_DEBUG
									printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									gettimeofday(&ct,NULL);
									update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
									if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                    *(fcdata->markbit) |= 0x0800;
								}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    				pinfo.chan,0x02,fcdata->markbit))
                                {
                                #ifdef RCC_DEBUG 
                                    printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
								gtecho[1] = 0x02;
								if (*(fcdata->markbit2) & 0x1000)
								{
									*(fcdata->markbit2) &= 0xefff;
									if (!wait_write_serial(*(fcdata->sdserial)))
									{
										if (write(*(fcdata->sdserial),gtecho,sizeof(gtecho)) < 0)
										{
										#ifdef RCC_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
									}
									else
									{
									#ifdef RCC_DEBUG
										printf("face board serial port cannot write,Line:%d\n",__LINE__);
									#endif
									}	
								}//step by step comes from side door serial
								else
								{
									if (!wait_write_serial(*(fcdata->fbserial)))
									{
										if (write(*(fcdata->fbserial),gtecho,sizeof(gtecho)) < 0)
										{
										#ifdef RCC_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											gettimeofday(&ct,NULL);
											update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
											if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																				fcdata->softevent,fcdata->markbit))
											{
											#ifdef RCC_DEBUG
												printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
											}
											*(fcdata->markbit) |= 0x0800;
										}
									}
									else
									{
									#ifdef RCC_DEBUG
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
            						#ifdef RCC_DEBUG
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
                                    #ifdef RCC_DEBUG
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
							}//if ((1 == gtred) || (1 == gtyfyes))

							if ((0==pinfo.cchan[0])&&(pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0))
							{
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{
									sinfo.conmode = 28;
                                    sinfo.color = 0x02;
									if (0x04 == pinfo.phasetype)
                                        sinfo.time = pinfo.gftime + pinfo.rtime;
                                    else
                                        sinfo.time = pinfo.gftime + pinfo.ytime + pinfo.rtime;
                           //         sinfo.time = pinfo.gftime + pinfo.ytime + pinfo.rtime;
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
            						#ifdef RCC_DEBUG
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
								memset(&gttd,0,sizeof(gttd));
								gttd.mode = 28;
								gttd.pattern = *(fcdata->patternid);
                                gttd.lampcolor = 0x02;
                                gttd.lamptime = pinfo.gftime;
                                gttd.phaseid = pinfo.phaseid;
                                gttd.stageline = pinfo.stageline;	
								if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
								{
								#ifdef RCC_DEBUG
									printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#ifdef EMBED_CONFIGURE_TOOL
							if (*(fcdata->markbit2) & 0x0200)
							{
								memset(&gttd,0,sizeof(gttd));
                                gttd.mode = 28;
                                gttd.pattern = *(fcdata->patternid);
                                gttd.lampcolor = 0x02;
                                gttd.lamptime = pinfo.gftime;
                                gttd.phaseid = pinfo.phaseid;
                                gttd.stageline = pinfo.stageline;  
								if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
								{
								#ifdef RCC_DEBUG
									printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#endif
							fbdata[1] = 28;
							fbdata[2] = pinfo.stageline;
							fbdata[3] = 2;
                            fbdata[4] = pinfo.gftime;
                            if (!wait_write_serial(*(fcdata->fbserial)))
                            {
                                if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                {
                                #ifdef RCC_DEBUG
                                    printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									*(fcdata->markbit) |= 0x0800;
                                    gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                            }
                            else
                            {
                            #ifdef RCC_DEBUG
                                printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }

							if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
                            {//report info to server actively
                                memset(err,0,sizeof(err));
                                gettimeofday(&ct,NULL);
                                if (SUCCESS != err_report(err,ct.tv_sec,22,5))
                                {
                                #ifdef RCC_DEBUG
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
							ngf = 0;
							while (1)
							{//green flash loop
								if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x03))
								{
								#ifdef RCC_DEBUG
									printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
									*(fcdata->markbit) |= 0x0800;
								}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    pinfo.cchan,0x03,fcdata->markbit))
                                {
                                #ifdef RCC_DEBUG 
                                    printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
								to.tv_sec = 0;
								to.tv_usec = 500000;
								select(0,NULL,NULL,NULL,&to);
								if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x02))
                                {
                                #ifdef RCC_DEBUG
                                    printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
									*(fcdata->markbit) |= 0x0800;
                                }
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    pinfo.cchan,0x02,fcdata->markbit))
                                {
                                #ifdef RCC_DEBUG
                                    printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
                                to.tv_sec = 0;
                                to.tv_usec = 500000;
                                select(0,NULL,NULL,NULL,&to);

								ngf += 1;
								if (ngf >= pinfo.gftime)
									break;
							}//green flash loop
							//end green flash

							//yellow lamp 
							if (0x04 != pinfo.phasetype)
							{//person phase do not have yellow lamp status
								if (1 == pinfo.cpcexist)
								{//have person channels
									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cnpchan,0x01))
									{
									#ifdef RCC_DEBUG
										printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                    	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef RCC_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
										pinfo.cnpchan,0x01,fcdata->markbit))
									{
									#ifdef RCC_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cpchan,0x00))
                                	{
                                	#ifdef RCC_DEBUG
                                    	printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
										gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                    	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef RCC_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
										*(fcdata->markbit) |= 0x0800;
                                	}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    		pinfo.cpchan,0x00,fcdata->markbit))
                                	{
                                	#ifdef RCC_DEBUG
                                    	printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
                                	}	
								}//have person channels
								else
								{//not have person channels
									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x01))
                                	{
                                	#ifdef RCC_DEBUG
                                    	printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
										gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                    	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef RCC_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
										*(fcdata->markbit) |= 0x0800;
                                	}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    						pinfo.cchan,0x01,fcdata->markbit))
                                	{
                                	#ifdef RCC_DEBUG
                                    	printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
                                	}
								}//not have person channels

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
            							#ifdef RCC_DEBUG
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
                                	memset(&gttd,0,sizeof(gttd));
                                	gttd.mode = 28;//traffic control
                                	gttd.pattern = *(fcdata->patternid);
                                	gttd.lampcolor = 0x01;
                                	gttd.lamptime = pinfo.ytime;
                                	gttd.phaseid = pinfo.phaseid;
                                	gttd.stageline = pinfo.stageline;
                                	if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
                                	{
                                	#ifdef RCC_DEBUG
                                    	printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                	#endif
                                	}
                            	}
								#ifdef EMBED_CONFIGURE_TOOL
                                if (*(fcdata->markbit2) & 0x0200)
                                {
                                 	memset(&gttd,0,sizeof(gttd));
                                    gttd.mode = 28;//traffic control
                                    gttd.pattern = *(fcdata->patternid);
                                    gttd.lampcolor = 0x01;
                                    gttd.lamptime = pinfo.ytime;
                                    gttd.phaseid = pinfo.phaseid;
                                    gttd.stageline = pinfo.stageline; 
                                    if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                                #endif
								fbdata[1] = 28;
								fbdata[2] = pinfo.stageline;
								fbdata[3] = 1;
                            	fbdata[4] = pinfo.ytime;
                            	if (!wait_write_serial(*(fcdata->fbserial)))
                            	{
                                	if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                	{
                                	#ifdef RCC_DEBUG
                                    	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
										*(fcdata->markbit) |= 0x0800;
                                    	gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef RCC_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                	}
                            	}
                            	else
                            	{
                            	#ifdef RCC_DEBUG
                                	printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            	#endif
                            	}
								sleep(pinfo.ytime);
								//end yellow lamp
							}//person phase do not have yellow lamp status

							//red lamp
							if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x00))
							{
							#ifdef RCC_DEBUG
								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								gettimeofday(&ct,NULL);
                                update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																fcdata->softevent,fcdata->markbit))
                                {
                                #ifdef RCC_DEBUG
                                    printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
								*(fcdata->markbit) |= 0x0800;
							}
							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus,pinfo.cchan,\
										0x00,fcdata->markbit))
							{
							#ifdef RCC_DEBUG
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
            						#ifdef RCC_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively
							}

							if (pinfo.rtime > 0)
							{
								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                                {
                                    memset(&gttd,0,sizeof(gttd));
                                    gttd.mode = 28;//traffic control
                                    gttd.pattern = *(fcdata->patternid);
                                    gttd.lampcolor = 0x00;
                                    gttd.lamptime = pinfo.rtime;
                                    gttd.phaseid = pinfo.phaseid;
                                    gttd.stageline = pinfo.stageline;
                                    if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
								#ifdef EMBED_CONFIGURE_TOOL
                                if (*(fcdata->markbit2) & 0x0200)
                                {
                                 	memset(&gttd,0,sizeof(gttd));
                                    gttd.mode = 28;//traffic control
                                    gttd.pattern = *(fcdata->patternid);
                                    gttd.lampcolor = 0x00;
                                    gttd.lamptime = pinfo.rtime;
                                    gttd.phaseid = pinfo.phaseid;
                                    gttd.stageline = pinfo.stageline; 
                                    if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                                #endif
								fbdata[1] = 28;
								fbdata[2] = pinfo.stageline;
								fbdata[3] = 0;
                            	fbdata[4] = pinfo.rtime;
                            	if (!wait_write_serial(*(fcdata->fbserial)))
                            	{
                                	if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                	{
                                	#ifdef RCC_DEBUG
                                    	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
										*(fcdata->markbit) |= 0x0800;
                                    	gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    	if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef RCC_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                	}
                            	}
                            	else
                            	{
                            	#ifdef RCC_DEBUG
                                	printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            	#endif
                            	}
                                sleep(pinfo.rtime);
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
							memset(&pinfo,0,sizeof(pinfo));
							if (SUCCESS != rcc_get_phase_info(fcdata,tscdata,rettl,*(fcdata->slnum),&pinfo))
							{
							#ifdef RCC_DEBUG
								printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
								output_log("green trigger,get phase info err");
							#endif
								rcc_end_part_child_thread();
								return FAIL;
							}
							*(fcdata->phaseid) = 0;
							*(fcdata->phaseid) |= (0x01 << (pinfo.phaseid - 1));
							if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                            {
                            #ifdef RCC_DEBUG
                                printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
								gettimeofday(&ct,NULL);
                                update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el, \
																fcdata->softevent,fcdata->markbit))
                                {
                                #ifdef RCC_DEBUG
                                    printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
								*(fcdata->markbit) |= 0x0800;
                            }

							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus,pinfo.chan,\
                                        0x02,fcdata->markbit))
                            {
                            #ifdef RCC_DEBUG
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
            					#ifdef RCC_DEBUG
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
								memset(&gttd,0,sizeof(gttd));
								gttd.mode = 28;
								gttd.pattern = *(fcdata->patternid);
                                gttd.lampcolor = 0x02;
                                gttd.lamptime = 0;
                                gttd.phaseid = pinfo.phaseid;
                                gttd.stageline = pinfo.stageline;	
								if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
								{
								#ifdef RCC_DEBUG
									printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#ifdef EMBED_CONFIGURE_TOOL
							if (*(fcdata->markbit2) & 0x0200)
							{
								memset(&gttd,0,sizeof(gttd));
                                gttd.mode = 28;
                                gttd.pattern = *(fcdata->patternid);
                                gttd.lampcolor = 0x02;
                                gttd.lamptime = 0;
                                gttd.phaseid = pinfo.phaseid;
                                gttd.stageline = pinfo.stageline;  
								if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
								{
								#ifdef RCC_DEBUG
									printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#endif
							fbdata[1] = 28;
							fbdata[2] = pinfo.stageline;
							fbdata[3] = 2;
                            fbdata[4] = 0;
                            if (!wait_write_serial(*(fcdata->fbserial)))
                            {
                                if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                {
                                #ifdef RCC_DEBUG
                                    printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									*(fcdata->markbit) |= 0x0800;
                                    gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                            }
                            else
                            {
                            #ifdef RCC_DEBUG
                                printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }

                            gtecho[1] = 0x02;
							if (*(fcdata->markbit2) & 0x1000)
							{
								*(fcdata->markbit2) &= 0xefff;
								if (!wait_write_serial(*(fcdata->sdserial)))
                                {
                                    if (write(*(fcdata->sdserial),gtecho,sizeof(gtecho)) < 0)
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                        *(fcdata->markbit) |= 0x0800;
                                    }
                                }
                                else
                                {
                                #ifdef RCC_DEBUG
                                    printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                #endif
                                }	
							}//step by step comes from side door serial
							else
							{
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),gtecho,sizeof(gtecho)) < 0)
									{
									#ifdef RCC_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
										if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																			fcdata->softevent,fcdata->markbit))
										{
										#ifdef RCC_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										*(fcdata->markbit) |= 0x0800;
									}
								}
								else
								{
								#ifdef RCC_DEBUG
									printf("face board serial port cannot write,Line:%d\n",__LINE__);
								#endif
								}
							}
							continue;
						}//step by step
						if ((0x03 == gtbuf[1]) && (1 == keylock))
						{//yellow flash
							gtred = 0;
							close = 0;
							if (0 == rccyfyes)
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
								int yfret = pthread_create(&rccyfid,NULL,(void *)rcc_yellow_flash,&yfdata);
								if (0 != yfret)
								{
								#ifdef RCC_DEBUG
									printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									rcc_end_part_child_thread();
									return FAIL;
								}
								rccyfyes = 1;
							}
							gtecho[1] = 0x03;
							if (*(fcdata->markbit2) & 0x2000)
							{
								*(fcdata->markbit2) &= 0xdfff;
								if (!wait_write_serial(*(fcdata->sdserial)))
                                {
                                    if (write(*(fcdata->sdserial),gtecho,sizeof(gtecho)) < 0)
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                        *(fcdata->markbit) |= 0x0800;
                                    }
                                }
                                else
                                {
                                #ifdef RCC_DEBUG
                                    printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                #endif
                                }
							}//yellow flash comes from side door serial;
							else
							{
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),gtecho,sizeof(gtecho)) < 0)
									{
									#ifdef RCC_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
										if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
										{
										#ifdef RCC_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										*(fcdata->markbit) |= 0x0800;
									}
								}
								else
								{
								#ifdef RCC_DEBUG
									printf("face serial cannot write,File:%s,Line:%d\n",__FILE__,__LINE__);
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
            					#ifdef RCC_DEBUG
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
                                #ifdef RCC_DEBUG
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
						if ((0x04 == gtbuf[1]) && (1 == keylock))
						{//all red
							if (1 == rccyfyes)
							{
								pthread_cancel(rccyfid);
								pthread_join(rccyfid,NULL);
								rccyfyes = 0;
							}
							close = 0;
							if (0 == gtred)
							{
								new_all_red(&ardata);	
								gtred = 1;
							}
							gtecho[1] = 0x04;
							if (*(fcdata->markbit2) & 0x4000)
                            {
                                *(fcdata->markbit2) &= 0xbfff;
                                if (!wait_write_serial(*(fcdata->sdserial)))
                                {
                                    if (write(*(fcdata->sdserial),gtecho,sizeof(gtecho)) < 0)
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                        *(fcdata->markbit) |= 0x0800;
                                    }
                                }
                                else
                                {
                                #ifdef RCC_DEBUG
                                    printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                #endif
                                }
                            }//all red comes from side door serial
							else
							{
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),gtecho,sizeof(gtecho)) < 0)
									{
									#ifdef RCC_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
										if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
										{
										#ifdef RCC_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										*(fcdata->markbit) |= 0x0800;
									}
								}
								else
								{
								#ifdef RCC_DEBUG
									printf("face serial cannot write,File:%s,Line:%d\n",__FILE__,__LINE__);
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
            					#ifdef RCC_DEBUG
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
                                #ifdef RCC_DEBUG
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
						if ((0x05 == gtbuf[1]) && (1 == keylock))
						{//all close
							if (1 == rccyfyes)
							{
								pthread_cancel(rccyfid);
								pthread_join(rccyfid,NULL);
								rccyfyes = 0;
							}
							gtred = 0;
							if (0 == close)
							{
								new_all_close(&acdata);	
								close = 1;
							}
							gtecho[1] = 0x05;
							if (!wait_write_serial(*(fcdata->fbserial)))
							{
								if (write(*(fcdata->fbserial),gtecho,sizeof(gtecho)) < 0)
								{
								#ifdef RCC_DEBUG
									printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
									*(fcdata->markbit) |= 0x0800;
								}
							}
							else
							{
							#ifdef RCC_DEBUG
								printf("face board serial cannot write,File:%s,Line:%d\n",__FILE__,__LINE__);
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
            					#ifdef RCC_DEBUG
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
                                #ifdef RCC_DEBUG
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
					}//key lock or unlock is valid only when wireless lock or unlock is invalid
				}//key traffic control
				else if (!strncmp("WLTC1",gtbuf,5))
				{//wireless terminal traffic control
					if ((0 == keylock) && (0 == netlock) && (0 == wtlock))
                    {//wireless lock or unlock is suessfully only when key lock or unlock is not valid;
						unsigned char           wlbuf[5] = {0};
                        unsigned char           s = 0;
                        for (s = 5; ;s++)
                        {
                            if (('E' == gtbuf[s]) && ('N' == gtbuf[s+1]) && ('D' == gtbuf[s+2]))
                                break;
                            if ('\0' == gtbuf[s])
                                break;
                            if ((s - 5) > 4)
                                break;
                            wlbuf[s - 5] = gtbuf[s];
                        }
                        #ifdef RCC_DEBUG
                        printf("************wlbuf: %s,File: %s,Line: %d\n",wlbuf,__FILE__,__LINE__);
                        #endif
						if (!strcmp("SOCK",wlbuf))
						{//lock or unlock
							if (0 == wllock)
							{//prepare to lock
							#ifdef RCC_DEBUG
								printf("Prepare to lock,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								get_rcc_status(&color,&leatime);
								if (2 != color)
								{//lamp color is not green
									struct timeval			spatime;
									unsigned char			endlock = 0;
									while (1)
									{//inline while loop
										spatime.tv_sec = 0;
										spatime.tv_usec = 200000;
										select(0,NULL,NULL,NULL,&spatime);
										memset(gtbuf,0,sizeof(gtbuf));
										read(*(fcdata->conpipe),gtbuf,sizeof(gtbuf));									
										if (!strncmp("WLTC1",gtbuf,5))
                                        {
                                            memset(wlbuf,0,sizeof(wlbuf));
                                            s = 0;
                                            for (s = 5; ;s++)
                                            {
                                                if(('E'==gtbuf[s]) && ('N'==gtbuf[s+1]) && ('D'==gtbuf[s+2]))
                                                    break;
                                                if ('\0' == gtbuf[s])
                                                    break;
                                                if ((s - 5) > 4)
                                                    break;
                                                wlbuf[s - 5] = gtbuf[s];
                                            }
                                            if (!strcmp("SOCK",wlbuf))
                                            {
                                                endlock = 1;
                                                break;
                                            }
                                        }								
	
										get_rcc_status(&color,&leatime);
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
								gtred = 0;
								close = 0;
								rcc_end_child_thread();//end main thread and its child thread
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
            						#ifdef RCC_DEBUG
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
								fbdata[2] = pinfo.stageline;
								fbdata[3] = 2;
								fbdata[4] = 0;
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
									{
									#ifdef RCC_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    	if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef RCC_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
									}
								}
								else
								{
								#ifdef RCC_DEBUG
									printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
								sendfaceInfoToBoard(fcdata,fbdata);
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    pinfo.chan,0x02,fcdata->markbit))
                                {
                                #ifdef RCC_DEBUG 
                                    printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                                {
                                    memset(wltc,0,sizeof(wltc));
                                    strcpy(wltc,"SOCKBEGIN");
                                    write(*(fcdata->sockfd->ssockfd),wltc,sizeof(wltc));
									//send down time to configure tool
									memset(&gttd,0,sizeof(gttd));
									gttd.mode = 28;
									gttd.pattern = *(fcdata->patternid);
									gttd.lampcolor = 0x02;
									gttd.lamptime = 0;
									gttd.phaseid = pinfo.phaseid;
									gttd.stageline = pinfo.stageline;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
									{
									#ifdef RCC_DEBUG
										printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
                                }
								#ifdef EMBED_CONFIGURE_TOOL
                                if (*(fcdata->markbit2) & 0x0200)
                                {
                                 	memset(&gttd,0,sizeof(gttd));
                                    gttd.mode = 28;
                                    gttd.pattern = *(fcdata->patternid);
                                    gttd.lampcolor = 0x02;
                                    gttd.lamptime = 0;
                                    gttd.phaseid = pinfo.phaseid;
                                    gttd.stageline = pinfo.stageline; 
                                    if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
                                    {
                                    #ifdef RCC_DEBUG
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
							#ifdef RCC_DEBUG
								printf("prepare to unlock,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								wllock = 0;
								gtred = 0;
								close = 0;
								if (1 == rccyfyes)
								{
									pthread_cancel(rccyfid);
									pthread_join(rccyfid,NULL);
									rccyfyes = 0;
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
            						#ifdef RCC_DEBUG
                						printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively					

								*(fcdata->contmode) = contmode;//restore control mode
								*(fcdata->markbit2) &= 0xffef;
								if (0 == rccyes)
								{
									memset(&rccdata,0,sizeof(rccdata_t));
									memset(&pinfo,0,sizeof(rccpinfo_t));
									rccdata.fd = fcdata;
									rccdata.td = tscdata;
									rccdata.pi = &pinfo;
									rccdata.cs = chanstatus;
									int gtret = \
									pthread_create(&rccid,NULL,(void *)start_rail_crossing_control,&rccdata);
									if (0 != gtret)
									{
									#ifdef RCC_DEBUG
										printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										return FAIL;
									}
									rccyes = 1;
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
							if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            {
                                memset(wltc,0,sizeof(wltc));
                                strcpy(wltc,"STEPBEGIN");
                                write(*(fcdata->sockfd->ssockfd),wltc,sizeof(wltc));
                            }
							if ((1 == gtred) || (1 ==rccyfyes) || (1 == close))
							{
								if (1 == close)
								{
									close = 0;
									new_all_red(&ardata);
								}
								gtred = 0;
								if (1 == rccyfyes)
								{
									pthread_cancel(rccyfid);
									pthread_join(rccyfid,NULL);
									rccyfyes = 0;
									new_all_red(&ardata);
								}
								if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
								{
								#ifdef RCC_DEBUG
									printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									gettimeofday(&ct,NULL);
									update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
									if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                    *(fcdata->markbit) |= 0x0800;
								}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    				pinfo.chan,0x02,fcdata->markbit))
                                {
                                #ifdef RCC_DEBUG 
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
            						#ifdef RCC_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively

								continue;
							}//if ((1 == gtred) || (1 == gtyfyes))

							if ((0==pinfo.cchan[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
							{
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{
									sinfo.conmode = 28;
                                    sinfo.color = 0x02;
									if (0x04 == pinfo.phasetype)
                                        sinfo.time = pinfo.gftime + pinfo.rtime;
                                    else
                                        sinfo.time = pinfo.gftime + pinfo.ytime + pinfo.rtime;
                              //      sinfo.time = pinfo.gftime + pinfo.ytime + pinfo.rtime;
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
            						#ifdef RCC_DEBUG
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
							fbdata[2] = pinfo.stageline;
							fbdata[3] = 2;
                            fbdata[4] = pinfo.gftime;
                            if (!wait_write_serial(*(fcdata->fbserial)))
                            {
                                if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                {
                                #ifdef RCC_DEBUG
                                    printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									*(fcdata->markbit) |= 0x0800;
                                    gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                            }
                            else
                            {
                            #ifdef RCC_DEBUG
                                printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }
							//begin to green flash
							if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
							{
								//send down time to configure tool
								memset(&gttd,0,sizeof(gttd));
								gttd.mode = 28;
								gttd.pattern = *(fcdata->patternid);
								gttd.lampcolor = 0x02;
								gttd.lamptime = pinfo.gftime;
								gttd.phaseid = pinfo.phaseid;
								gttd.stageline = pinfo.stageline;
								if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
								{
								#ifdef RCC_DEBUG
									printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#ifdef EMBED_CONFIGURE_TOOL
							if (*(fcdata->markbit2) & 0x0200)
							{
								memset(&gttd,0,sizeof(gttd));
                                gttd.mode = 28;
                                gttd.pattern = *(fcdata->patternid);
                                gttd.lampcolor = 0x02;
                                gttd.lamptime = pinfo.gftime;
                                gttd.phaseid = pinfo.phaseid;
                                gttd.stageline = pinfo.stageline;  
								if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
								{
								#ifdef RCC_DEBUG
									printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#endif
							ngf = 0;
							while (1)
							{//green flash loop
								if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x03))
								{
								#ifdef RCC_DEBUG
									printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
									*(fcdata->markbit) |= 0x0800;
								}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    pinfo.cchan,0x03,fcdata->markbit))
                                {
                                #ifdef RCC_DEBUG 
                                    printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
								to.tv_sec = 0;
								to.tv_usec = 500000;
								select(0,NULL,NULL,NULL,&to);
								if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x02))
                                {
                                #ifdef RCC_DEBUG
                                    printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
									*(fcdata->markbit) |= 0x0800;
                                }
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    pinfo.cchan,0x02,fcdata->markbit))
                                {
                                #ifdef RCC_DEBUG
                                    printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
                                to.tv_sec = 0;
                                to.tv_usec = 500000;
                                select(0,NULL,NULL,NULL,&to);

								ngf += 1;
								if (ngf >= pinfo.gftime)
									break;
							}//green flash loop
							//end green flash

							//yellow lamp 
							if (0x04 != pinfo.phasetype)
							{//person phase do not have yellow lamp status
								if (1 == pinfo.cpcexist)
								{//have person channels
									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cnpchan,0x01))
									{
									#ifdef RCC_DEBUG
										printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                    	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef RCC_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
										pinfo.cnpchan,0x01,fcdata->markbit))
									{
									#ifdef RCC_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cpchan,0x00))
                                	{
                                	#ifdef RCC_DEBUG
                                    	printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
										gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                    	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef RCC_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
										*(fcdata->markbit) |= 0x0800;
                                	}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    		pinfo.cpchan,0x00,fcdata->markbit))
                                	{
                                	#ifdef RCC_DEBUG
                                    	printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
                                	}	
								}//have person channels
								else
								{//not have person channels
									if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x01))
                                	{
                                	#ifdef RCC_DEBUG
                                    	printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
										gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                    	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef RCC_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
										*(fcdata->markbit) |= 0x0800;
                                	}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    						pinfo.cchan,0x01,fcdata->markbit))
                                	{
                                	#ifdef RCC_DEBUG
                                    	printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
                                	}
								}//not have person channels

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
            							#ifdef RCC_DEBUG
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
								fbdata[2] = pinfo.stageline;
								fbdata[3] = 1;
                            	fbdata[4] = pinfo.ytime;
                            	if (!wait_write_serial(*(fcdata->fbserial)))
                            	{
                                	if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                	{
                                	#ifdef RCC_DEBUG
                                    	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
										*(fcdata->markbit) |= 0x0800;
                                    	gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef RCC_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                	}
                            	}
                            	else
                            	{
                            	#ifdef RCC_DEBUG
                                	printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            	#endif
                            	}
								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
								{   
									//send down time to configure tool
									memset(&gttd,0,sizeof(gttd));
									gttd.mode = 28;
									gttd.pattern = *(fcdata->patternid);
									gttd.lampcolor = 0x01;
									gttd.lamptime = pinfo.ytime;
									gttd.phaseid = pinfo.phaseid;
									gttd.stageline = pinfo.stageline;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
									{
									#ifdef RCC_DEBUG
										printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef EMBED_CONFIGURE_TOOL
                                if (*(fcdata->markbit2) & 0x0200)
                                {
                                  	memset(&gttd,0,sizeof(gttd));
                                    gttd.mode = 28;
                                    gttd.pattern = *(fcdata->patternid);
                                    gttd.lampcolor = 0x01;
                                    gttd.lamptime = pinfo.ytime;
                                    gttd.phaseid = pinfo.phaseid;
                                    gttd.stageline = pinfo.stageline;
                                    if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                                #endif
								sleep(pinfo.ytime);
								//end yellow lamp
							}//person phase do not have yellow lamp status

							//red lamp
							if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x00))
							{
							#ifdef RCC_DEBUG
								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								gettimeofday(&ct,NULL);
                                update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																fcdata->softevent,fcdata->markbit))
                                {
                                #ifdef RCC_DEBUG
                                    printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
								*(fcdata->markbit) |= 0x0800;
							}
							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus,pinfo.cchan,\
										0x00,fcdata->markbit))
							{
							#ifdef RCC_DEBUG
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
            						#ifdef RCC_DEBUG
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
                                    memset(&gttd,0,sizeof(gttd));
                                    gttd.mode = 28;
                                    gttd.pattern = *(fcdata->patternid);
                                    gttd.lampcolor = 0x00;
                                    gttd.lamptime = pinfo.rtime;
                                    gttd.phaseid = pinfo.phaseid;
                                    gttd.stageline = pinfo.stageline;
                                    if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&gttd))
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
								#ifdef EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
								{   
									memset(&gttd,0,sizeof(gttd));
                                    gttd.mode = 28;
                                    gttd.pattern = *(fcdata->patternid);
                                    gttd.lampcolor = 0x00;
                                    gttd.lamptime = pinfo.rtime;
                                    gttd.phaseid = pinfo.phaseid;
                                    gttd.stageline = pinfo.stageline; 
									if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&gttd))
									{
									#ifdef RCC_DEBUG
										printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#endif								

							}

							if (pinfo.rtime > 0)
							{
								fbdata[1] = 28;
								fbdata[2] = pinfo.stageline;
								fbdata[3] = 0;
                            	fbdata[4] = pinfo.rtime;
                            	if (!wait_write_serial(*(fcdata->fbserial)))
                            	{
                                	if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                	{
                                	#ifdef RCC_DEBUG
                                    	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
										*(fcdata->markbit) |= 0x0800;
                                    	gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    	if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef RCC_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                	}
                            	}
                            	else
                            	{
                            	#ifdef RCC_DEBUG
                                	printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            	#endif
                            	}
								
                                sleep(pinfo.rtime);
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
							memset(&pinfo,0,sizeof(pinfo));
							if (SUCCESS != rcc_get_phase_info(fcdata,tscdata,rettl,*(fcdata->slnum),&pinfo))
							{
							#ifdef RCC_DEBUG
								printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
								output_log("green trigger,get phase info err");
							#endif
								rcc_end_part_child_thread();
								return FAIL;
							}
							*(fcdata->phaseid) = 0;
							*(fcdata->phaseid) |= (0x01 << (pinfo.phaseid - 1));
							if (SUCCESS != rcc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                            {
                            #ifdef RCC_DEBUG
                                printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
								gettimeofday(&ct,NULL);
                                update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el, \
																fcdata->softevent,fcdata->markbit))
                                {
                                #ifdef RCC_DEBUG
                                    printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
								*(fcdata->markbit) |= 0x0800;
                            }

							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus,pinfo.chan,\
                                        0x02,fcdata->markbit))
                            {
                            #ifdef RCC_DEBUG
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
            					#ifdef RCC_DEBUG
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
							fbdata[3] = 2;
                            fbdata[4] = 0;
                            if (!wait_write_serial(*(fcdata->fbserial)))
                            {
                                if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                {
                                #ifdef RCC_DEBUG
                                    printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									*(fcdata->markbit) |= 0x0800;
                                    gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef RCC_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                            }
                            else
                            {
                            #ifdef RCC_DEBUG
                                printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }

							gettimeofday(&ct,NULL);
                            update_event_list(fcdata->ec,fcdata->el,1,92,ct.tv_sec,fcdata->markbit);
							continue;
						}//step by step
						if ((1 == wllock) && (!strcmp("YF",wlbuf)))
						{//yellow flash
							gtred = 0;
							close = 0;
							if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            {
                                memset(wltc,0,sizeof(wltc));
                                strcpy(wltc,"YFBEGIN");
                                write(*(fcdata->sockfd->ssockfd),wltc,sizeof(wltc));
                            }
							if (0 == rccyfyes)
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
								int yfret = pthread_create(&rccyfid,NULL,(void *)rcc_yellow_flash,&yfdata);
								if (0 != yfret)
								{
								#ifdef RCC_DEBUG
									printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									rcc_end_part_child_thread();
									return FAIL;
								}
								rccyfyes = 1;
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
            					#ifdef RCC_DEBUG
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
							if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            {
                                memset(wltc,0,sizeof(wltc));
                                strcpy(wltc,"REDBEGIN");
                                write(*(fcdata->sockfd->ssockfd),wltc,sizeof(wltc));
                            }
							if (1 == rccyfyes)
							{
								pthread_cancel(rccyfid);
								pthread_join(rccyfid,NULL);
								rccyfyes = 0;
							}
							close = 0;
							if (0 == gtred)
							{
								new_all_red(&ardata);	
								gtred = 1;
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
            					#ifdef RCC_DEBUG
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
							if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            {
                                memset(wltc,0,sizeof(wltc));
                                strcpy(wltc,"CLOBEGIN");
                                write(*(fcdata->sockfd->ssockfd),wltc,sizeof(wltc));
                            }
							if (1 == rccyfyes)
							{
								pthread_cancel(rccyfid);
								pthread_join(rccyfid,NULL);
								rccyfyes = 0;
							}
							gtred = 0;
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
            					#ifdef RCC_DEBUG
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
                    }//wireless lock or unlock is suessfully only when key lock or unlock is not valid;
				}//wireless terminal traffic control
			}//if (FD_ISSET(*(fcdata->conpipe),&nread))
		}//if (cpret > 0)
	}//0

	return SUCCESS;
}
