/*
 * File: mhd.c
 * Author: sk
 * Date: 20131125 
*/
#include "mhds.h"

static unsigned char		mahdyes = 0;
static pthread_t			mahdid;	
static unsigned char		madcpcyes = 0;
static pthread_t			madcpcid;
static unsigned char		mayfyes = 0;
static pthread_t			mayfid;
static unsigned char		ppmyes = 0;
static pthread_t			ppmid;
static unsigned char		cpdyes = 0;
static pthread_t			cpdid;
static pthread_t			rfpid;
static unsigned char        rettl = 0;
static struct timeval		gtime,gftime,ytime,rtime;
static phasedetect_t        phdetect[MAX_PHASE_LINE] = {0};
static phasedetect_t        *pphdetect = NULL;
static detectorpro_t        *dpro = NULL;
static statusinfo_t         sinfo;
static unsigned char		phcon = 0;

//#ifdef CHANNEL_YELLOW_FLASH
static pthread_t    yfcpid;
static cyellowflash_t      cyft;
//#endif

void madc_end_child_thread()
{
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
	if (1 == mayfyes)
	{
		pthread_cancel(mayfid);
		pthread_join(mayfid,NULL);
		mayfyes = 0;
	}
	if (1 == madcpcyes)
	{
		pthread_cancel(madcpcid);
		pthread_join(madcpcid,NULL);
		madcpcyes = 0;
	}
	if (1 == mahdyes)
	{
		pthread_cancel(mahdid);
		pthread_join(mahdid,NULL);
		mahdyes = 0;
	}
	#ifdef CHANNEL_YELLOW_FLASH
    if (1 == cyft.mark)
    {
        pthread_cancel(yfcpid);
        pthread_join(yfcpid,NULL);
        cyft.mark = 0;
        if (SUCCESS != madc_set_lamp_color(*(cyft.mah->fd->bbserial),cyft.chan,0x00))
        {
            printf("set channel color err,File: %s,Line: %d\n",__FILE__,__LINE__);
        }
        if (SUCCESS!=update_channel_status(cyft.mah->fd->sockfd,cyft.mah->cs,cyft.chan,0,cyft.mah->fd->markbit))
        {
            printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
        }
    }
    #endif
#if 0
	#ifdef CHANNEL_CLOSE
    if (SUCCESS != dc_set_lamp_color(*(cyft.dc->fd->bbserial),cyft.chan,0x00))
    {
        printf("set channel color err,File: %s,Line: %d\n",__FILE__,__LINE__);
    }
    if (SUCCESS!=update_channel_status(cyft.dc->fd->sockfd,cyft.dc->cs,cyft.chan,0,cyft.dc->fd->markbit))
    {
        printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
    }
    #endif
#endif

}
void madc_end_part_child_thread()
{
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
	if (1 == mayfyes)
    {
        pthread_cancel(mayfid);
        pthread_join(mayfid,NULL);
        mayfyes = 0;
    }
	if (1 == madcpcyes)
    {
        pthread_cancel(madcpcid);
        pthread_join(madcpcid,NULL);
        madcpcyes = 0;
    }
	#ifdef CHANNEL_YELLOW_FLASH
    if (1 == cyft.mark)
    {
        pthread_cancel(yfcpid);
        pthread_join(yfcpid,NULL);
        cyft.mark = 0;
        if (SUCCESS != madc_set_lamp_color(*(cyft.mah->fd->bbserial),cyft.chan,0x00))
        {
            printf("set channel color err,File: %s,Line: %d\n",__FILE__,__LINE__);
        }
        if (SUCCESS!=update_channel_status(cyft.mah->fd->sockfd,cyft.mah->cs,cyft.chan,0,cyft.mah->fd->markbit))
        {
            printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
        }
    }
    #endif
#if 0
	#ifdef CHANNEL_CLOSE
    if (SUCCESS != dc_set_lamp_color(*(cyft.dc->fd->bbserial),cyft.chan,0x00))
    {
        printf("set channel color err,File: %s,Line: %d\n",__FILE__,__LINE__);
    }
    if (SUCCESS!=update_channel_status(cyft.dc->fd->sockfd,cyft.dc->cs,cyft.chan,0,cyft.dc->fd->markbit))
    {
        printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
    }
    #endif
#endif
	
}

void madc_yellow_flash(void *arg)
{
    yfdata_t            *yfdata = arg;

    new_yellow_flash(yfdata);

    pthread_exit(NULL);
}

void madc_person_chan_greenflash(void *arg)
{
	mahpcdata_t				*mahpcdata = arg;
	unsigned char			i = 0;
	struct timeval			timeout;

	while (1)
	{
		if (SUCCESS != madc_set_lamp_color(*(mahpcdata->bbserial),mahpcdata->pchan,0x03))
		{
		#ifdef MAJOR_DEBUG
			printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			*(mahpcdata->markbit) |= 0x0800;
		}
		if (SUCCESS != update_channel_status(mahpcdata->sockfd,mahpcdata->cs,mahpcdata->pchan, \
											0x03,mahpcdata->markbit))
        {
        #ifdef MAJOR_DEBUG
            printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
        }
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;
		select(0,NULL,NULL,NULL,&timeout);
	
		if (SUCCESS != madc_set_lamp_color(*(mahpcdata->bbserial),mahpcdata->pchan,0x02))
		{
		#ifdef MAJOR_DEBUG
			printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			*(mahpcdata->markbit) |= 0x0800;
		}
		if (SUCCESS != update_channel_status(mahpcdata->sockfd,mahpcdata->cs,mahpcdata->pchan, \
											0x02,mahpcdata->markbit))
        {
        #ifdef MAJOR_DEBUG
            printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
        }
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;
		select(0,NULL,NULL,NULL,&timeout);		
	}//while loop

	pthread_exit(NULL);
}

void madc_monitor_pending_pipe(void *arg)
{//monitor detector to degrade when do not detect control;
	monpendphase_t			*mpphase = arg;
	fd_set					nRead;
	unsigned char			buf[256] = {0};
	unsigned char			*pbuf = buf;
	unsigned short			num = 0;
	unsigned short			mark = 0;
	unsigned char			i = 0, j = 0;
	unsigned char			dtype = 0;
	unsigned char			deteid = 0;
	unsigned char			mphase = 0;

	while (1)
	{//0
		FD_ZERO(&nRead);
		FD_SET(*(mpphase->pendpipe),&nRead);
		int fret = select(*(mpphase->pendpipe)+1,&nRead,NULL,NULL,NULL);
		if (fret < 0)
		{
		#ifdef MAJOR_DEBUG
			printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
			output_log("Detect control,select call err");
		#endif
			return;
		}
		if (fret > 0)
		{
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
							mark += 6;
							continue;//Fit data,directly break;
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
		}//if (fret > 0)
	}//0

	pthread_exit(NULL);
}//monitor detector to degrade when do not detect control;

void madc_count_phase_detector(void *arg)
{
	unsigned char				i = 0, j = 0, z = 0;
	struct timeval				ntime;
	unsigned int				leatime = 0;
	unsigned char				exist = 0;

	while (1)
	{//0
		sleep(INTERVALTIME);
		#ifdef MAJOR_DEBUG
		printf("Begin to count detector of phase,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif

		for (i = 0; i < MAX_PHASE_LINE; i++)
		{//for (i = 0; i < MAX_PHASE_LINE; i++)
			if (0 == phdetect[i].phaseid)
				break;
			if ((0x80 == phdetect[i].phasetype) || (0x04 == phdetect[i].phasetype))
                continue;
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
			
			if (phdetect[i].factnum >= phdetect[i].indetenum)
			{
				phdetect[i].validmark = 0;
			}
			else
			{
				phdetect[i].validmark = 1;
			}				
		}//for (i = 0; i < MAX_PHASE_LINE; i++)
	}//0	

	pthread_exit(NULL);
}

void start_major_half_detect(void *arg)
{
	mahdata_t				*mahdata = arg;
	unsigned char			tcline = 0;
	unsigned char			snum = 0;
	unsigned char			slnum = 0;
	unsigned char			i = 0,j = 0,z = 0;
	timedown_t				timedown;
	unsigned char			tphid = 0;
	unsigned char			tnum = 0;
	monpendphase_t			mpphase;	

	//  #ifdef CHANNEL_YELLOW_FLASH; 单通道黄闪和灭灯都可以使用这些变量
    unsigned char           cyfn = 0;
    unsigned int            cyfe = 0;
    unsigned char           cyfc[YFCHANNEL] = {0};
    unsigned char           *pcyfc =  NULL;
    //  #endif

	unsigned char           downti[8] = {0xA6,0xff,0xff,0xff,0xff,0x03,0,0xED};
	unsigned char           edownti[3] = {0xA5,0xA5,0xED};
	if (!wait_write_serial(*(mahdata->fd->bbserial)))
    {
    	if (write(*(mahdata->fd->bbserial),downti,sizeof(downti)) < 0)
        {
		#ifdef MAJOR_DEBUG
			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif       
        }
    }
    else
    {
    #ifdef MAJOR_DEBUG
    	printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
    }
    if (!wait_write_serial(*(mahdata->fd->bbserial)))
    {
    	if (write(*(mahdata->fd->bbserial),edownti,sizeof(edownti)) < 0)
        {
        #ifdef MAJOR_DEBUG
        	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
        }
    }
    else
    {
    #ifdef MAJOR_DEBUG
    	printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
    }

	if (SUCCESS != madc_get_timeconfig(mahdata->td,mahdata->fd->patternid,&tcline))
	{
	#ifdef MAJOR_DEBUG
		printf("Get time config err,File: %s,Line: %d\n",__FILE__,__LINE__);
		output_log("major half detect,Get timeconfig err");
	#endif
		madc_end_part_child_thread();
		//return;
		struct timeval				time;
		unsigned char				yferr[10] = {0};
		gettimeofday(&time,NULL);
		update_event_list(mahdata->fd->ec,mahdata->fd->el,1,11,time.tv_sec,mahdata->fd->markbit);
		if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
		{
			memset(yferr,0,sizeof(yferr));
			if (SUCCESS != err_report(yferr,time.tv_sec,1,11))
			{
				#ifdef MAJOR_DEBUG
				printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
			}
			else
			{
				write(*(mahdata->fd->sockfd->csockfd),yferr,sizeof(yferr));
			}
		}

		yfdata_t					yfdata;
		if (0 == mayfyes)
		{
			memset(&yfdata,0,sizeof(yfdata));
			yfdata.second = 0;
			yfdata.markbit = mahdata->fd->markbit;
			yfdata.markbit2 = mahdata->fd->markbit2;
			yfdata.serial = *(mahdata->fd->bbserial);
			yfdata.sockfd = mahdata->fd->sockfd;
			yfdata.cs = mahdata->cs;	
#ifdef FLASH_DEBUG
			char szInfo[32] = {0};
			char szInfoT[64] = {0};
			snprintf(szInfo,sizeof(szInfo)-1,"%s",__FUNCTION__);
			snprintf(szInfoT,sizeof(szInfoT)-1,"%d",__LINE__);
			tsc_save_eventlog(szInfo,szInfoT);
#endif
			int yfret = pthread_create(&mayfid,NULL,(void *)madc_yellow_flash,&yfdata);
			if (0 != yfret)
			{
				#ifdef MAJOR_DEBUG
				printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
				output_log("madc_yellow_flash,create yellow flash err");
				#endif
				madc_end_part_child_thread();
				return;
			}
			mayfyes = 1;
		}		
		while(1)
		{
			if (*(mahdata->fd->markbit) & 0x0001)
			{//end time of current pattern has arrived
				unsigned char           enddata[3] = {0xCC,0xDD,0xED};
				if (!wait_write_serial(*(mahdata->fd->synpipe)))
				{
					if (write(*(mahdata->fd->synpipe),enddata,sizeof(enddata)) < 0)
					{
						#ifdef FULL_DETECT_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						output_log("madc control,write synpipe err");
						#endif
						madc_end_part_child_thread();
						return;
					}
				}
				else
				{
					#ifdef FULL_DETECT_DEBUG
					printf("pipe cannot be written,File: %s,Line: %d\n",__FILE__,__LINE__);
					output_log("madc control,synpipe cannot write");
					#endif
					madc_end_part_child_thread();
					return;
				}
				sleep(5);//wait main module end own;	
			}//end time of current pattern has arrived
			sleep(10);
			continue;
		}

	}
	rettl = tcline;

	memset(phdetect,0,sizeof(phdetect));
	pphdetect = phdetect;
	j = 0;
	for (snum = 0; ;snum++)
	{//for (snum = 0; ;snum++)
		if (0 == mahdata->td->timeconfig->TimeConfigList[tcline][snum].StageId)
			break;
		get_phase_id(mahdata->td->timeconfig->TimeConfigList[tcline][snum].PhaseId,&tphid);
		for (i = 0; i < (mahdata->td->phase->FactPhaseNum); i++)
		{
			if (0 == (mahdata->td->phase->PhaseList[i].PhaseId))
				break;
			if (tphid == (mahdata->td->phase->PhaseList[i].PhaseId))
			{
				pphdetect->phaseid = tphid;
				pphdetect->phasetype = mahdata->td->phase->PhaseList[i].PhaseType;
				tnum = mahdata->td->phase->PhaseList[i].PhaseSpecFunc;
				tnum &= 0xe0;//get 5~7bit
				tnum >>= 5;
				tnum &= 0x07;
				pphdetect->indetenum = tnum; //invalid detector arrive the number,begin to degreade;
				pphdetect->validmark = 1;
				pphdetect->factnum = 0;
				memset(pphdetect->indetect,0,sizeof(pphdetect->indetect));
				memset(pphdetect->detectpro,0,sizeof(pphdetect->detectpro));
				if (0x20 == (pphdetect->phasetype))
				{//only flexible phase have mapped detector in major half detect;
					dpro = pphdetect->detectpro;
					for (z = 0; z < (mahdata->td->detector->FactDetectorNum); z++)
					{
						if (0 == (mahdata->td->detector->DetectorList[z].DetectorId))
							break;
						if ((pphdetect->phaseid) == (mahdata->td->detector->DetectorList[z].DetectorPhase))
						{//0
							dpro->deteid = mahdata->td->detector->DetectorList[z].DetectorId;
							dpro->validmark = 0;
							tnum = mahdata->td->detector->DetectorList[z].DetectorSpecFunc;
							tnum &= 0xfc; //get 2~7bit
							tnum >>= 2;
							tnum &= 0x3f;
							dpro->intime = tnum * 60; //invalid time of detector;
							memset(&(dpro->stime),0,sizeof(struct timeval));
							gettimeofday(&(dpro->stime),NULL);
							dpro++;	
						}//0
					}//for (z = 0; z < (mahdata->td->detector->FactDetectorNum); z++)
				}//only flexible phase have mapped detector in major half detect;
				pphdetect++;
				break;		
			}//if (tphid == (mahdata->td->phase->PhaseList[i].PhaseId))
		}//for (i = 0; i < (mahdata->td->phase->FactPhaseNum); i++)	
	}//for (snum = 0; ;snum++)
	
	if (0 == ppmyes)
	{//check the valid or invalid of detector when do not have detect control;
		memset(&mpphase,0,sizeof(monpendphase_t));
		mpphase.pendpipe = mahdata->fd->pendpipe;
		mpphase.detector = mahdata->td->detector;
		int ppmret = pthread_create(&ppmid,NULL,(void *)madc_monitor_pending_pipe,&mpphase);
		if (0 != ppmret)
		{
		#ifdef MAJOR_DEBUG
			printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
			output_log("major control,create monitor pending thread err");
		#endif
			madc_end_part_child_thread();
			return;
		}
		ppmyes = 1;
	}//check the valid or invalid of detector when do not have detect control;
	if (0 == cpdyes)
	{
		int cpdret = pthread_create(&cpdid,NULL,(void *)madc_count_phase_detector,NULL);
		if (0 != cpdret)
		{
		#ifdef MAJOR_DEBUG
			printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
			output_log("major control,create count phase thread err");
		#endif
			madc_end_part_child_thread();
			return;
		}
		cpdyes = 1;
	}
//data send of backup pattern control
	if (SUCCESS != mah_backup_pattern_data(*(mahdata->fd->bbserial),snum,phdetect))
	{
	#ifdef TIMEING_DEBUG
		printf("backup_pattern_data call err,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
	}
	//end data send
	slnum = *(mahdata->fd->slnum);
	*(mahdata->fd->stageid) = mahdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
	if (0 == (mahdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId))
	{
		slnum = 0;
		*(mahdata->fd->slnum) = 0;
		*(mahdata->fd->stageid) = mahdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
	}

	unsigned char				cycarr = 0;
	mahpinfo_t					*pinfo = mahdata->pi;
	mahpcdata_t					mahpcdata;
	unsigned char				gft = 0;//green flash time;
	struct timeval				timeout,mtime,nowtime,lasttime,ct;
	unsigned char				leatime = 0, mt = 0,bakv = 0;
	fd_set						nRead;
	unsigned char				sltime = 0;
	unsigned char				ffw = 0;
	unsigned char				buf[256] = {0};
	unsigned char				*pbuf = buf;
	unsigned short				num = 0,mark = 0;
	unsigned char				caryes = 0; //'1' means that vehicle pass detector;
	unsigned char				dtype = 0; //detector type;
	unsigned char				mintime = 0;
	unsigned char				concyc = 0;
	unsigned char               cycend[16] = "CYTGCYCLEENDEND";
	unsigned char               fbdata[6] = {0};
	fbdata[0] = 0xC1;
	fbdata[5] = 0xED;
	unsigned char				deteid = 0;
	unsigned char				mappid = 0;
	unsigned char				rft = 0;
	unsigned char				endahead = 0;
	unsigned char				endcyclen = 0;
	unsigned short				lefttime = 0;
	unsigned char				c1gt = 0;
	unsigned char				c1gmt = 0;
	unsigned char				c2gt = 0;
	unsigned char				c2gmt = 0;
	unsigned char				twocycend = 0;
	unsigned char				onecycend = 0;
	unsigned char				validmark = 0;

	unsigned char				sibuf[64] = {0};
//	statusinfo_t				sinfo;
	unsigned char               *csta = NULL;
    unsigned char               tcsta = 0;

	unsigned char				err1[10] = {0};//20220621	
	AdaptData_t                 adaptd;//20220621
    unsigned char               adabuf[432] = {0};//20220621

	memset(&adaptd,0,sizeof(AdaptData_t));
	adaptd.pattid = *(mahdata->fd->patternid);
	adaptd.contype = *(mahdata->fd->contmode);
	adaptd.stageid = *(mahdata->fd->stageid);


//	memset(&sinfo,0,sizeof(statusinfo_t));
	sinfo.conmode = *(mahdata->fd->contmode);
    sinfo.pattern = *(mahdata->fd->patternid);
    sinfo.cyclet = *(mahdata->fd->cyclet);

	dunhua_close_lamp(mahdata->fd,mahdata->cs);

	adaptd.phasenum = 0;//added on 20220620
    memset(adaptd.phi,0,sizeof(adaptd.phi));//added on 20220620
    unsigned char       ni = 0;//added on 20220620

	while (1)
	{//while loop
		if (1 == cycarr)
		{//if (1 == cycarr)
			cycarr = 0;

			if (*(mahdata->fd->markbit) & 0x0001)
			{//end time of current pattern has arrived
				unsigned char				syndata[3] = {0xCC,0xDD,0xED};
				#ifdef RED_FLASH
                sleep(1);//wait "rfpid" thread release resource; 
                #endif
				if (!wait_write_serial(*(mahdata->fd->synpipe)))
				{
					if (write(*(mahdata->fd->synpipe),syndata,sizeof(syndata)) < 0)
					{
					#ifdef MAJOR_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						output_log("Major half detect,write pipe err");
					#endif
						madc_end_part_child_thread();
						return;
					}
				}
				else
				{
				#ifdef MAJOR_DEBUG
					printf("Pipe cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
					output_log("Major half detect,pipe cannot write");
				#endif
					madc_end_part_child_thread();
					return;
				}
				sleep(5);//wait main process to end own;
			}//end time of current pattern has arrived

			//added on 20220620
            memset(adabuf,0,sizeof(adabuf));
			gettimeofday(&ct,NULL);
			adaptd.time = ct.tv_sec;
			for (i = 0; i < adaptd.phasenum; i++)
            {
                adaptd.cycle += adaptd.phi[i].pfactt;
            }
            if (SUCCESS != adapt_data_report(adabuf,&adaptd))
            {
            #ifdef MAJOR_DEBUG
                printf("adapt_data_report called err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
            else
            {
                write(*(mahdata->fd->sockfd->csockfd),adabuf,sizeof(adabuf));
            }
            adaptd.phasenum = 0;
			adaptd.cycle = 0;
			adaptd.time = 0;
            memset(adaptd.phi,0,sizeof(adaptd.phi));
            ni = 0;
            //end add
#if 0
			//send end mark of cycle to configure tool  
            if (write(*(mahdata->fd->sockfd->ssockfd),cycend,sizeof(cycend)) < 0)
            {
            #ifdef MAJOR_DEBUG
                printf("write error,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }			
#endif
			if ((*(mahdata->fd->markbit) & 0x0100) && (0 == endahead))
			{//current pattern transit ahead two cycles
				//Note,8th bit is clean 0 by main module
				if (0 != *(mahdata->fd->ncoordphase))
				{//next coordphase is not 0
					endahead = 1;
					endcyclen = 0;
					lefttime = 0;
					gettimeofday(&ct,NULL);
					lefttime = (unsigned char)((mahdata->fd->nst) - ct.tv_sec);
					#ifdef MAJOR_DEBUG
					printf("*****************lefttime: %d,File: %s,Line: %d\n",lefttime,__FILE__,__LINE__);
					#endif
					if (lefttime >= (*(mahdata->fd->cyclet)*3)/2)
					{//use two cyc to end pattern
						c1gt = (lefttime/2 + lefttime%2)/snum - 3 - 3;
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
						c1gt = lefttime/snum - 3 - 3;
						c1gmt = lefttime%snum;
						twocycend = 0;
						onecycend = 1;
					}//use one cyc to end pattern
					#ifdef MAJOR_DEBUG
					printf("c1gt:%d,c1gmt:%d,c2gt:%d,c2gmt:%d,twocycend:%d,onecycend:%d,Line:%d\n", \
							c1gt,c1gmt,c2gt,c2gmt,twocycend,onecycend,__LINE__);
					#endif
				}//next coordphase is not 0
			}//current pattern transit ahead two cycles			
			if (1 == endahead)
			{//if (1 == endahead)
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

		memset(pinfo,0,sizeof(mahpinfo_t));
		if (SUCCESS != madc_get_phase_info(mahdata->fd,mahdata->td,tcline,slnum,pinfo))
		{
		#ifdef MAJOR_DEBUG
			printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
			output_log("Major half detect,get phase info err");
		#endif
			madc_end_part_child_thread();
			return;
		}
		//added on 20220620
        adaptd.phasenum += 1;
        adaptd.phi[ni].pid = pinfo->phaseid;
        adaptd.phi[ni].pming = pinfo->mingtime;
        adaptd.phi[ni].pmaxg = pinfo->maxgtime1;
        //end add	

		#ifdef CHANNEL_YELLOW_FLASH	
		unsigned char           bcyex = 0;
        unsigned char           bcyft[MAX_CHANNEL] = {0};
        unsigned char           *pbcyft = bcyft;
		if (1 == cyft.mark)
		{
			cyfn = 0;
			cyfe = 0;
			for (cyfn = 0; cyfn < mahdata->td->phaseerror->FactPhaseErrorNum; cyfn++)
			{
				if (pinfo->phaseid  == mahdata->td->phaseerror->PhaseErrorList[cyfn].PhaseId)
				{
					cyfe = /*0xfffe1fff &*/ mahdata->td->phaseerror->PhaseErrorList[cyfn].PhaseConflict;
					break;
				}
			}

			cyft.mark = 0;
			for (i = 0; i < MAX_CHANNEL; i++)
			{
				if (0 == cyft.chan[i])
					break;
				bcyex = 0;
				if (cyfe & (0x01 << (cyft.chan[i] - 1)))
				{
					bcyex = 1;
				}
				if (0 == bcyex)
				{
					*pbcyft = cyft.chan[i];
					pbcyft++;
				}
			}
			
			pthread_cancel(yfcpid);
        	pthread_join(yfcpid,NULL);

			if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),bcyft,0x00))
			{
			#ifdef TIMING_DEBUG
				printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
			if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,bcyft,0x00,mahdata->fd->markbit))
			{
			#ifdef TIMING_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
		}//1 == cyft.mark
		#endif

		#ifdef CHANNEL_CLOSE
        unsigned char           bex = 0;
        unsigned char           bft[MAX_CHANNEL] = {0};
        unsigned char           *pbft = bft;
        if (1 == cyft.mark)
        {
            cyfn = 0;
            cyfe = 0;
            for (cyfn = 0; cyfn < mahdata->td->phaseerror->FactPhaseErrorNum; cyfn++)
            {
                if (pinfo->phaseid  == mahdata->td->phaseerror->PhaseErrorList[cyfn].PhaseId)
                {
                    cyfe = /*0xfffe1fff &*/ mahdata->td->phaseerror->PhaseErrorList[cyfn].PhaseConflict;
                    break;
                }
            }
            cyft.mark = 0;
			for (i = 0; i < MAX_CHANNEL; i++)
            {
                if (0 == cyft.chan[i])
                    break;
                bex = 0;
                if (cyfe & (0x01 << (cyft.chan[i] - 1)))
                {
                    bex = 1;
                }
                if (0 == bex)
                {
                    *pbft = cyft.chan[i];
                    pbft++;
                }
            }
			
			channel_close_end_report(mahdata->fd->sockfd->csockfd,bft);

            if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),bft,0x00))
            {
            #ifdef TIMING_DEBUG
                printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
            if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,bft,0x00,mahdata->fd->markbit))
            {
            #ifdef TIMING_DEBUG
                printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
		}//1 == cyft.mark
        #endif

		*(mahdata->fd->phaseid) = 0;
		*(mahdata->fd->phaseid) |= (0x01 << (pinfo->phaseid - 1));
		sinfo.stage = *(mahdata->fd->stageid);
        sinfo.phase = *(mahdata->fd->phaseid);

		if ((*(mahdata->fd->markbit) & 0x0100) && (1 == endahead))
		{//end pattern ahead two cycles
		#ifdef MAJOR_DEBUG
			printf("End pattern two cycles ahead,begin %dth cycle,Line:%d\n",endcyclen,__LINE__);
		#endif
			*(mahdata->fd->color) = 0x02;
			*(mahdata->fd->markbit) &= 0xfbff;
			if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->chan,0x02))
			{
			#ifdef MAJOR_DEBUG
				printf("Set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mahdata->fd->markbit) |= 0x0800;
				gettimeofday(&ct,NULL);
                update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15,ct.tv_sec,mahdata->fd->markbit);
                if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
													mahdata->fd->softevent,mahdata->fd->markbit))
                {
                #ifdef MAJOR_DEBUG
                    printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
			}
			#ifdef CHANNEL_YELLOW_FLASH
			cyfn = 0;
			cyfe = 0;
			for (cyfn = 0; cyfn < mahdata->td->phaseerror->FactPhaseErrorNum; cyfn++)
			{		
				if (pinfo->phaseid	== mahdata->td->phaseerror->PhaseErrorList[cyfn].PhaseId)
				{
					cyfe = /*0xfffe1fff &*/ mahdata->td->phaseerror->PhaseErrorList[cyfn].PhaseConflict;
					break;
				}
			}
			memset(cyfc,0,sizeof(cyfc));
			pcyfc = cyfc;
			if (cyfe > 0)
			{//exist channel yellow flash
				for (cyfn = 0; cyfn < YFCHANNEL; cyfn++)
				{
					if (cyfe & (0x01 << cyfn))
					{
						*pcyfc = cyfn + 1;
						pcyfc++;
					}
				}
				cyft.cyft = pinfo->gtime + pinfo->gftime + pinfo->ytime + pinfo->rtime -1;//for thread exit
				cyft.chan = cyfc;
				cyft.mah = mahdata;
				cyft.mark = 1;
				int yfarg = pthread_create(&yfcpid,NULL,(void *)mah_channel_yellow_flash,&cyft);
				if (0 != yfarg)
				{
				#ifdef TIMING_DEBUG
					printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					return;
				}
				else
				{//create channel yellow flash thread success;
				#ifdef TIMING_DEBUG
					printf("Create channel yellow flash thread success,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
					{//actively report is not probitted and connect successfully
						sinfo.time = pinfo->gtime + pinfo->gftime + pinfo->ytime + pinfo->rtime - 1;
						sinfo.conmode = *(mahdata->fd->contmode);
						sinfo.color = 0x05;//yellow flash
						sinfo.chans = 0;
						memset(sinfo.csta,0,sizeof(sinfo.csta));
						csta = sinfo.csta;
						for (i = 0; i < YFCHANNEL; i++)
						{
							if (0 == cyfc[i])
								break;
							sinfo.chans += 1;
							tcsta = cyfc[i];
							tcsta <<= 2;
							tcsta &= 0xfc;
							tcsta |= 0x00;
							*csta = tcsta;
							csta++;
						}
						memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
						memset(sibuf,0,sizeof(sibuf));
						if (SUCCESS != status_info_report(sibuf,&sinfo))	
						{
						#ifdef TIMING_DEBUG
							printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
						else
						{
							write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
						}
					}//actively report is not probitted and connect successfully
				}//create channel yellow flash thread success;
			}//exist channel yellow flash
			#endif

			#ifdef CHANNEL_CLOSE
			cyfn = 0;
			cyfe = 0;
			for (cyfn = 0; cyfn < mahdata->td->phaseerror->FactPhaseErrorNum; cyfn++)
			{
				if (pinfo->phaseid  == mahdata->td->phaseerror->PhaseErrorList[cyfn].PhaseId)
				{
					cyfe = /*0xfffe1fff &*/ mahdata->td->phaseerror->PhaseErrorList[cyfn].PhaseConflict;
					break;
				}
			}
			memset(cyfc,0,sizeof(cyfc));
			pcyfc = cyfc;
			if (cyfe > 0)
			{//exist channel close lamp
				for (cyfn = 0; cyfn < YFCHANNEL; cyfn++)
				{
					if (cyfe & (0x01 << cyfn))
					{
						*pcyfc = cyfn + 1;
						pcyfc++;
					}
				}
				cyft.cyft = pinfo->gtime + pinfo->gftime + pinfo->ytime + pinfo->rtime;
				cyft.chan = cyfc;
				cyft.mah = mahdata;
				cyft.mark = 1;

				if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),cyfc,0x03))
				{
				#ifdef TIMING_DEBUG
					printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,cyfc,0x03,mahdata->fd->markbit))
				{
				#ifdef TIMING_DEBUG
					printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				channel_close_begin_report(mahdata->fd->sockfd->csockfd,cyfc);
			}//exist channel close lamp
			#endif

			memset(&gtime,0,sizeof(gtime));
			gettimeofday(&gtime,NULL);
			memset(&gftime,0,sizeof(gftime));
			memset(&ytime,0,sizeof(ytime));
			memset(&rtime,0,sizeof(rtime));
			if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,pinfo->chan,0x02, \
													mahdata->fd->markbit))
			{
			#ifdef MAJOR_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
			if (0 == pinfo->cchan[0])
			{
				if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
				{//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
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
                        	sinfo.time = c2gt + c2gmt + 3 + 3;
                    	}
                    	else
                    	{
                        	sinfo.time = c2gt + 3 + 3;
                    	}
					}
					sinfo.color = 0x02;
					sinfo.conmode = *(mahdata->fd->contmode);//added on 20150529
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
					memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            		memset(sibuf,0,sizeof(sibuf));
            		if (SUCCESS != status_info_report(sibuf,&sinfo))
            		{
            		#ifdef MAJOR_DEBUG
                		printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            		#endif
            		}
            		else
            		{
                		write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            		}	
				}//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
			}
			else
			{
				if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
				{//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
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
					sinfo.conmode = *(mahdata->fd->contmode);//added on 20150529
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
					memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            		memset(sibuf,0,sizeof(sibuf));
            		if (SUCCESS != status_info_report(sibuf,&sinfo))
            		{
            		#ifdef MAJOR_DEBUG
                		printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            		#endif
            		}
            		else
            		{
                		write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            		}	
				}//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
			}

			if ((*(mahdata->fd->markbit) & 0x0002) && (*(mahdata->fd->markbit) & 0x0010))
			{//send down time data to configure tool
				memset(&timedown,0,sizeof(timedown));
				timedown.mode = *(mahdata->fd->contmode);
				timedown.pattern = *(mahdata->fd->patternid);
				timedown.lampcolor = 0x02;
				if (1 == endcyclen)
				{
					if (snum == (slnum + 1))
					{
						timedown.lamptime = c1gt + c1gmt + 3;//default 3 second green flash
					}
					else
					{
						timedown.lamptime = c1gt + 3;
					}
				}
				if (2 == endcyclen)
				{
					if (snum == (slnum + 1))
					{
						timedown.lamptime = c2gt + c2gmt + 3;
					}
					else
					{
						timedown.lamptime = c2gt + 3;
					}
				}
				timedown.phaseid = pinfo->phaseid;
				timedown.stageline = pinfo->stageline;
				if (SUCCESS != timedown_data_to_conftool(mahdata->fd->sockfd,&timedown))
				{
				#ifdef MAJOR_DEBUG
					printf("send time err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
			}//send down time data to configure tool
			#ifdef EMBED_CONFIGURE_TOOL
            if (*(mahdata->fd->markbit2) & 0x0200)
            {
                memset(&timedown,0,sizeof(timedown));
				timedown.mode = *(mahdata->fd->contmode);
				timedown.pattern = *(mahdata->fd->patternid);
				timedown.lampcolor = 0x02;
				if (1 == endcyclen)
				{
					if (snum == (slnum + 1))
					{
						timedown.lamptime = c1gt + c1gmt + 3;//default 3 second green flash
					}
					else
					{
						timedown.lamptime = c1gt + 3;
					}
				}
				if (2 == endcyclen)
				{
					if (snum == (slnum + 1))
					{
						timedown.lamptime = c2gt + c2gmt + 3;
					}
					else
					{
						timedown.lamptime = c2gt + 3;
					}
				}
				timedown.phaseid = pinfo->phaseid;
				timedown.stageline = pinfo->stageline;
                if (SUCCESS != timedown_data_to_embed(mahdata->fd->sockfd,&timedown))
                {
                #ifdef MAJOR_DEBUG
                    printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                #endif
                }
            }
            #endif

			//send down time data to face board
			if (*(mahdata->fd->contmode) < 27)
				fbdata[1] = *(mahdata->fd->contmode) + 1;
			else
				fbdata[1] = *(mahdata->fd->contmode);
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
						fbdata[4] = c1gt + c1gmt + 3;
					}
					else
					{
						fbdata[4] = c1gt + 3;
					}
				}
				if (2 == endcyclen)	
				{
					if (snum == (slnum + 1))
					{
						fbdata[4] = c2gt + c2gmt + 3;
					}
					else
					{
						fbdata[4] = c2gt + 3;
					}
				}
			}
			if (!wait_write_serial(*(mahdata->fd->fbserial)))
			{
				if (write(*(mahdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
				{
				#ifdef MAJOR_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mahdata->fd->markbit) |= 0x0800;
					gettimeofday(&ct,NULL);
                	update_event_list(mahdata->fd->ec,mahdata->fd->el,1,16,ct.tv_sec,mahdata->fd->markbit);
                	if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
														mahdata->fd->softevent,mahdata->fd->markbit))
                	{
                	#ifdef MAJOR_DEBUG
                    	printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                	}
				}
			}
			else
			{
			#ifdef MAJOR_DEBUG
				printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
			sendfaceInfoToBoard(mahdata->fd,fbdata);
			if (1 == endcyclen)
			{
				if (snum == (slnum + 1))
				{
			//		sleep(c1gt + c1gmt);
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
         //           sleep(c2gt + c2gmt);
					sltime = c2gt + c2gmt;
                }
                else
                {
        //            sleep(c2gt);
					sltime = c2gt;
                }
            }

			while (1)
			{//while loop
				FD_ZERO(&nRead);
				FD_SET(*(mahdata->fd->ffpipe),&nRead);
				lasttime.tv_sec = 0;
				lasttime.tv_usec = 0;
				gettimeofday(&lasttime,NULL);
				bakv = sltime;
				mtime.tv_sec = sltime;
				mtime.tv_usec = 0;
				int mret = select(*(mahdata->fd->ffpipe)+1,&nRead,NULL,NULL,&mtime);
				if (mret < 0)
				{
				#ifdef MAJOR_DEBUG
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
					if (FD_ISSET(*(mahdata->fd->ffpipe),&nRead))
					{
						memset(buf,0,sizeof(buf));
						read(*(mahdata->fd->ffpipe),buf,sizeof(buf));
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
			}//while loop

			//begin to green flash
			if (0 != pinfo->cchan[0])
			{
				if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
        		{//actively report is not probitted and connect successfully
					sinfo.time = 3;
					sinfo.color = 0x03;//green flash
					sinfo.conmode = *(mahdata->fd->contmode);//added on 20150529
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
					memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            		memset(sibuf,0,sizeof(sibuf));
            		if (SUCCESS != status_info_report(sibuf,&sinfo))
            		{
            		#ifdef MAJOR_DEBUG
                		printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            		#endif
            		}
            		else
            		{
                		write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            		}
				}//actively report is not probitted and connect successfully
			}

			memset(&gtime,0,sizeof(gtime));
            memset(&gftime,0,sizeof(gftime));
			gettimeofday(&gftime,NULL);
            memset(&ytime,0,sizeof(ytime));
            memset(&rtime,0,sizeof(rtime));
			*(mahdata->fd->markbit) |= 0x0400;
			gft = 0;
			if (pinfo->gftime > 0)
			{
				while (1)
				{
					if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->cchan,0x03))
					{
					#ifdef MAJOR_DEBUG
						printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						*(mahdata->fd->markbit) |= 0x0800;
						gettimeofday(&ct,NULL);
						update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15,ct.tv_sec,mahdata->fd->markbit);
						if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
															mahdata->fd->softevent,mahdata->fd->markbit))
						{
						#ifdef MAJOR_DEBUG
							printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
					}
					if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,pinfo->cchan,0x03, \
															mahdata->fd->markbit))
					{
					#ifdef MAJOR_DEBUG
						printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					timeout.tv_sec = 0;
					timeout.tv_usec = 500000;
					select(0,NULL,NULL,NULL,&timeout);
					if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->cchan,0x02))
					{
					#ifdef MAJOR_DEBUG
						printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						gettimeofday(&ct,NULL);
						update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15,ct.tv_sec,mahdata->fd->markbit);
						if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
															mahdata->fd->softevent,mahdata->fd->markbit))
						{
						#ifdef MAJOR_DEBUG
							printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
						*(mahdata->fd->markbit) |= 0x0800;
					}
					if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,pinfo->cchan,0x03, \
															mahdata->fd->markbit))
					{
					#ifdef MAJOR_DEBUG
						printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					timeout.tv_sec = 0;
					timeout.tv_usec = 500000;
					select(0,NULL,NULL,NULL,&timeout);
					
					gft += 1;
					if (gft >= 3)
						break;
				}//green flahs loop
			}//pinfo->gftime > 0
			//end green flash
			if (1 == phcon)
            {
                *(mahdata->fd->markbit) &= 0xfbff;
                memset(&gtime,0,sizeof(gtime));
                gettimeofday(&gtime,NULL);
                memset(&gftime,0,sizeof(gftime));
                memset(&ytime,0,sizeof(ytime));
                memset(&rtime,0,sizeof(rtime));
                phcon = 0;
                sleep(10);
            }

			#ifdef RED_FLASH	
			if (0 == mahdata->td->timeconfig->TimeConfigList[tcline][slnum+1].StageId)
			{
				rft = (mahdata->td->timeconfig->TimeConfigList[tcline][0].SpecFunc & 0xe0) >> 5;
			}
			else
			{
				rft = (mahdata->td->timeconfig->TimeConfigList[tcline][slnum+1].SpecFunc & 0xe0) >> 5;
			}
			if (rft > 0)
			{
				redflash_mah		mah;
				mah.tcline = tcline;
				mah.slnum = slnum;
				mah.snum =	snum;
				mah.rft = rft;
				mah.chan = pinfo->chan;
				mah.mah = mahdata;
				int rfarg = pthread_create(&rfpid,NULL,(void *)mah_red_flash,&mah);
				if (0 != rfarg)
				{
				#ifdef MAJOR_DEBUG
					printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					return;
				}
			}//if (rft > 0) 
			#endif

			//Begin to set yellow lamp
			if (1 == (pinfo->cpcexist))
			{//person channels exist in current phase
				if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->cpchan,0x00))
				{
				#ifdef MAJOR_DEBUG
					printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mahdata->fd->markbit) |= 0x0800;
					gettimeofday(&ct,NULL);
                	update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15,ct.tv_sec,mahdata->fd->markbit);
                	if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
														mahdata->fd->softevent,mahdata->fd->markbit))
                	{
                	#ifdef MAJOR_DEBUG
                    	printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                	}
				}
				if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,pinfo->cpchan,0x00, \
                                                   mahdata->fd->markbit))
                {
                #ifdef MAJOR_DEBUG
                    printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
				if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->cnpchan,0x01))
				{
				#ifdef MAJOR_DEBUG
                    printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
					*(mahdata->fd->markbit) |= 0x0800;
					gettimeofday(&ct,NULL);
                	update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15,ct.tv_sec,mahdata->fd->markbit);
                	if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
														mahdata->fd->softevent,mahdata->fd->markbit))
                	{
                	#ifdef MAJOR_DEBUG
                    	printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                	}
				}
				if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,pinfo->cnpchan,0x01, \
                                                   mahdata->fd->markbit))
                {
                #ifdef MAJOR_DEBUG
                    printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
			}//person channels exist in current phase
			else
			{//Not person channels exist in current phase
				if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->cchan,0x01))
				{
				#ifdef MAJOR_DEBUG
					printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mahdata->fd->markbit) |= 0x0800;
					gettimeofday(&ct,NULL);
                	update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15,ct.tv_sec,mahdata->fd->markbit);
                	if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
														mahdata->fd->softevent,mahdata->fd->markbit))
                	{
                	#ifdef MAJOR_DEBUG
                    	printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                	}
				}
				if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,pinfo->cchan,0x01, \
                                                   mahdata->fd->markbit))
                {
                #ifdef MAJOR_DEBUG
                    printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
			}//Not person channels exist in current phase

			if (0 != pinfo->cchan[0])
			{
				if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
        		{//actively report is not probitted and connect successfully
					sinfo.time = 3;
					sinfo.color = 0x01;
					sinfo.conmode = *(mahdata->fd->contmode);//added on 20150529
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
					memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            		memset(sibuf,0,sizeof(sibuf));
            		if (SUCCESS != status_info_report(sibuf,&sinfo))
            		{
            		#ifdef MAJOR_DEBUG
                		printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            		#endif
            		}
            		else
            		{
                		write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            		}
				}//actively report is not probitted and connect successfully
			}

			*(mahdata->fd->color) = 0x01;
			memset(&gtime,0,sizeof(gtime));
            memset(&gftime,0,sizeof(gftime));
            memset(&ytime,0,sizeof(ytime));
			gettimeofday(&ytime,NULL);
            memset(&rtime,0,sizeof(rtime));
			if ((*(mahdata->fd->markbit) & 0x0002) && (*(mahdata->fd->markbit) & 0x0010))
			{
				memset(&timedown,0,sizeof(timedown));
				timedown.mode = *(mahdata->fd->contmode);
				timedown.pattern = *(mahdata->fd->patternid);
				timedown.lampcolor = 0x01;
				timedown.lamptime = 3;
				timedown.phaseid = pinfo->phaseid;
				timedown.stageline = pinfo->stageline;
				if (SUCCESS != timedown_data_to_conftool(mahdata->fd->sockfd,&timedown))
				{
				#ifdef MAJOR_DEBUG
					printf("send time err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#ifdef EMBED_CONFIGURE_TOOL
            if (*(mahdata->fd->markbit2) & 0x0200)
            {
                memset(&timedown,0,sizeof(timedown));
                timedown.mode = *(mahdata->fd->contmode);
                timedown.pattern = *(mahdata->fd->patternid);
                timedown.lampcolor = 0x01;
                timedown.lamptime = 3;
                timedown.phaseid = pinfo->phaseid;
                timedown.stageline = pinfo->stageline;
                if (SUCCESS != timedown_data_to_embed(mahdata->fd->sockfd,&timedown))
                {
                #ifdef MAJOR_DEBUG
                    printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                #endif
                }
            }
            #endif
			if (*(mahdata->fd->contmode) < 27)
                fbdata[1] = *(mahdata->fd->contmode) + 1;
            else
                fbdata[1] = *(mahdata->fd->contmode);
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
			if (!wait_write_serial(*(mahdata->fd->fbserial)))
			{
				if (write(*(mahdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
				{
				#ifdef MAJOR_DEBUG
					printf("write err,File: %s,LIne: %d\n",__FILE__,__LINE__);
				#endif
					gettimeofday(&ct,NULL);
                	update_event_list(mahdata->fd->ec,mahdata->fd->el,1,16,ct.tv_sec,mahdata->fd->markbit);
                	if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
														mahdata->fd->softevent,mahdata->fd->markbit))
                	{
                	#ifdef MAJOR_DEBUG
                    	printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                	}
					*(mahdata->fd->markbit) |= 0x0800;
				}
			}
			else
			{
			#ifdef MAJOR_DEBUG
				printf("can not write,File: %s,LIne: %d\n",__FILE__,__LINE__);
			#endif
			}
			sendfaceInfoToBoard(mahdata->fd,fbdata);
			sleep(3);
			//end set yellow lamp
			//Begin to set red lamp
			if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->cchan,0x00))
			{
			#ifdef MAJOR_DEBUG
				printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				gettimeofday(&ct,NULL);
                update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15,ct.tv_sec,mahdata->fd->markbit);
                if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
												mahdata->fd->softevent,mahdata->fd->markbit))
                {
                #ifdef MAJOR_DEBUG
                    printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
				*(mahdata->fd->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,pinfo->cchan,0x00, \
                                            mahdata->fd->markbit))
            {
            #ifdef MAJOR_DEBUG
                printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
			*(mahdata->fd->color) = 0x00;

			if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
        	{//actively report is not probitted and connect successfully
				sinfo.time = 0;
				sinfo.color = 0x00;
				sinfo.conmode = *(mahdata->fd->contmode);//added on 20150529
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
				memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            	memset(sibuf,0,sizeof(sibuf));
            	if (SUCCESS != status_info_report(sibuf,&sinfo))
            	{
            	#ifdef MAJOR_DEBUG
                	printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            	#endif
            	}
            	else
            	{
                	write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            	}
			}//actively report is not probitted and connect successfully

			//end red lamp,red lamp time is default 0 in the last two cycles;
			if (1 == phcon)
            {
				*(mahdata->fd->markbit) &= 0xfbff;
                memset(&gtime,0,sizeof(gtime));
                gettimeofday(&gtime,NULL);
                memset(&gftime,0,sizeof(gftime));
                memset(&ytime,0,sizeof(ytime));
                memset(&rtime,0,sizeof(rtime));
                phcon = 0;
                sleep(10);
            }

			ni += 1;//20220621
			slnum += 1;
			*(mahdata->fd->slnum) = slnum;
			*(mahdata->fd->stageid) = mahdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
			if (0 == (mahdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId))
			{
				cycarr = 1;
				slnum = 0;
				*(mahdata->fd->slnum) = 0;
				*(mahdata->fd->stageid) = mahdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
			}

			#ifdef RED_FLASH
            if (rft > (pinfo->ytime + pinfo->rtime))
            {
                sleep(rft - (pinfo->ytime) - (pinfo->rtime));
            }
            #endif			

			continue;
		}//end pattern ahead two cycles
	
		/*********Not the last two cycles of ending pattern ahead******************/	
		if (0x20 == (pinfo->phasetype))	
		{//flexible phase
			validmark = 0;

			for (j = 0; j < MAX_PHASE_LINE; j++)
            {
                if (0 == phdetect[j].phaseid)
                    break;
                if (pinfo->phaseid == phdetect[j].phaseid)
                {
                    validmark = phdetect[j].validmark;
                    break;
                }
            }				

			if (0 == validmark)
			{
			#ifdef MAJOR_DEBUG
				printf("Begin to degrade timing control,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				//added on 20220621
				gettimeofday(&ct,NULL);
				update_event_list(mahdata->fd->ec,mahdata->fd->el,1,128,ct.tv_sec,mahdata->fd->markbit);
				memset(err1,0,sizeof(err1));
				*(mahdata->fd->phasedegrphid) |= (0x00000001 << (pinfo->phaseid -1));
				if (SUCCESS != err_report(err1,ct.tv_sec,21,(50+(pinfo->phaseid - 1))))
				{
				#ifdef MAJOR_DEBUG
					printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					write(*(mahdata->fd->sockfd->csockfd),err1,sizeof(err1));
				}
				//end add

				*(mahdata->fd->color) = 0x02;
				*(mahdata->fd->markbit) &= 0xfbff;
				if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->chan,0x02))
				{
				#ifdef MAJOR_DEBUG
					printf("set green err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mahdata->fd->markbit) |= 0x0800;
					gettimeofday(&ct,NULL);
                	update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15,ct.tv_sec,mahdata->fd->markbit);
                	if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
														mahdata->fd->softevent,mahdata->fd->markbit))
                	{
                	#ifdef MAJOR_DEBUG
                    	printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                	}
				}
				//20220621
				adaptd.phi[ni].paddt = 0;
				adaptd.phi[ni].pfactt = pinfo->gtime + pinfo->gftime + pinfo->ytime + pinfo->rtime;
				//end add
				#ifdef CHANNEL_YELLOW_FLASH
				cyfn = 0;
				cyfe = 0;
				for (cyfn = 0; cyfn < mahdata->td->phaseerror->FactPhaseErrorNum; cyfn++)
				{		
					if (pinfo->phaseid	== mahdata->td->phaseerror->PhaseErrorList[cyfn].PhaseId)
					{
						cyfe = /*0xfffe1fff &*/ mahdata->td->phaseerror->PhaseErrorList[cyfn].PhaseConflict;
						break;
					}
				}
				memset(cyfc,0,sizeof(cyfc));
				pcyfc = cyfc;
				if (cyfe > 0)
				{//exist channel yellow flash
					for (cyfn = 0; cyfn < YFCHANNEL; cyfn++)
					{
						if (cyfe & (0x01 << cyfn))
						{
							*pcyfc = cyfn + 1;
							pcyfc++;
						}
					}
					cyft.cyft = pinfo->gtime + pinfo->gftime + pinfo->ytime + pinfo->rtime -1;//for thread exit
					cyft.chan = cyfc;
					cyft.mah = mahdata;
					cyft.mark = 1;
					int yfarg = pthread_create(&yfcpid,NULL,(void *)mah_channel_yellow_flash,&cyft);
					if (0 != yfarg)
					{
					#ifdef TIMING_DEBUG
						printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						return;
					}
					else
					{//create channel yellow flash thread success;
					#ifdef TIMING_DEBUG
						printf("Create channel yellow flash thread success,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
						{//actively report is not probitted and connect successfully
							sinfo.time = pinfo->gtime + pinfo->gftime + pinfo->ytime + pinfo->rtime - 1;
							sinfo.conmode = *(mahdata->fd->contmode);
							sinfo.color = 0x05;//yellow flash
							sinfo.chans = 0;
							memset(sinfo.csta,0,sizeof(sinfo.csta));
							csta = sinfo.csta;
							for (i = 0; i < YFCHANNEL; i++)
							{
								if (0 == cyfc[i])
									break;
								sinfo.chans += 1;
								tcsta = cyfc[i];
								tcsta <<= 2;
								tcsta &= 0xfc;
								tcsta |= 0x00;
								*csta = tcsta;
								csta++;
							}
							memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
							memset(sibuf,0,sizeof(sibuf));
							if (SUCCESS != status_info_report(sibuf,&sinfo))	
							{
							#ifdef TIMING_DEBUG
								printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}
							else
							{
								write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
							}
						}//actively report is not probitted and connect successfully
					}//create channel yellow flash thread success;
				}//exist channel yellow flash
				#endif

				#ifdef CHANNEL_CLOSE
				cyfn = 0;
				cyfe = 0;
				for (cyfn = 0; cyfn < mahdata->td->phaseerror->FactPhaseErrorNum; cyfn++)
				{
					if (pinfo->phaseid  == mahdata->td->phaseerror->PhaseErrorList[cyfn].PhaseId)
					{
						cyfe = /*0xfffe1fff &*/ mahdata->td->phaseerror->PhaseErrorList[cyfn].PhaseConflict;
						break;
					}
				}
				memset(cyfc,0,sizeof(cyfc));
				pcyfc = cyfc;
				if (cyfe > 0)
				{//exist channel close lamp
					for (cyfn = 0; cyfn < YFCHANNEL; cyfn++)
					{
						if (cyfe & (0x01 << cyfn))
						{
							*pcyfc = cyfn + 1;
							pcyfc++;
						}
					}
					cyft.cyft = pinfo->gtime + pinfo->gftime + pinfo->ytime + pinfo->rtime;
					cyft.chan = cyfc;
					cyft.mah = mahdata;
					cyft.mark = 1;

					if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),cyfc,0x03))
					{
					#ifdef TIMING_DEBUG
						printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,cyfc,0x03,mahdata->fd->markbit))
					{
					#ifdef TIMING_DEBUG
						printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					channel_close_begin_report(mahdata->fd->sockfd->csockfd,cyfc);
				}//exist channel close lamp
				#endif			

				memset(&gtime,0,sizeof(gtime));
            	gettimeofday(&gtime,NULL);
            	memset(&gftime,0,sizeof(gftime));
            	memset(&ytime,0,sizeof(ytime));
            	memset(&rtime,0,sizeof(rtime));
				if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,pinfo->chan,0x02, \
												mahdata->fd->markbit))
				{
				#ifdef MAJOR_DEBUG
					printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}

				if (0 == pinfo->cchan[0])
				{
					if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
					{//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
						sinfo.time = pinfo->gtime + pinfo->gftime + pinfo->ytime + pinfo->rtime;	
						sinfo.color = 0x02;
						sinfo.conmode = *(mahdata->fd->contmode);//added on 20150529
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
						memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            			memset(sibuf,0,sizeof(sibuf));
            			if (SUCCESS != status_info_report(sibuf,&sinfo))
            			{
            			#ifdef MAJOR_DEBUG
                			printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            			#endif
            			}
            			else
            			{
                			write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            			}	
					}//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
				}
				else
				{
					if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
					{//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
						sinfo.time = pinfo->gtime + pinfo->gftime;	
						sinfo.color = 0x02;
						sinfo.conmode = *(mahdata->fd->contmode);//added on 20150529
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
						memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            			memset(sibuf,0,sizeof(sibuf));
            			if (SUCCESS != status_info_report(sibuf,&sinfo))
            			{
            			#ifdef MAJOR_DEBUG
                			printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            			#endif
            			}
            			else
            			{
                			write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            			}	
					}//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
				}

				if ((*(mahdata->fd->markbit) & 0x0002) && (*(mahdata->fd->markbit) & 0x0010))
				{
					memset(&timedown,0,sizeof(timedown));
					timedown.mode = *(mahdata->fd->contmode);
					timedown.pattern = *(mahdata->fd->patternid);
					timedown.lampcolor = 0x02;
					timedown.lamptime = pinfo->gtime + pinfo->gftime;
					timedown.phaseid = pinfo->phaseid;
					timedown.stageline = pinfo->stageline;
					if (SUCCESS != timedown_data_to_conftool(mahdata->fd->sockfd,&timedown))
					{
					#ifdef MAJOR_DEBUG
						printf("send time err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
				}
				#ifdef EMBED_CONFIGURE_TOOL
				if (*(mahdata->fd->markbit2) & 0x0200)
				{
					memset(&timedown,0,sizeof(timedown));
                    timedown.mode = *(mahdata->fd->contmode);
                    timedown.pattern = *(mahdata->fd->patternid);
                    timedown.lampcolor = 0x02;
                    timedown.lamptime = pinfo->gtime + pinfo->gftime;
                    timedown.phaseid = pinfo->phaseid;
                    timedown.stageline = pinfo->stageline;	
					if (SUCCESS != timedown_data_to_embed(mahdata->fd->sockfd,&timedown))
					{
					#ifdef MAJOR_DEBUG
						printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
				}
				#endif
				//send info to face board
				if (*(mahdata->fd->contmode) < 27)
                	fbdata[1] = *(mahdata->fd->contmode) + 1;
            	else
                	fbdata[1] = *(mahdata->fd->contmode);
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
				if (!wait_write_serial(*(mahdata->fd->fbserial)))
				{
					if (write(*(mahdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
					{
					#ifdef MAJOR_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						*(mahdata->fd->markbit) |= 0x0800;
					}
				}
				else
				{
				#ifdef MAJOR_DEBUG
					printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				sendfaceInfoToBoard(mahdata->fd,fbdata);
				if ((1 == (pinfo->cpcexist)) && ((pinfo->pctime) > 0))
				{//exist person channels
		//			sleep(pinfo->gtime - pinfo->pctime);
					sltime = pinfo->gtime - pinfo->pctime;
					ffw = 0;
					while (1)
					{//while loop
						FD_ZERO(&nRead);
						FD_SET(*(mahdata->fd->ffpipe),&nRead);
						lasttime.tv_sec = 0;
						lasttime.tv_usec = 0;
						gettimeofday(&lasttime,NULL);
						bakv = sltime;
						mtime.tv_sec = sltime;
						mtime.tv_usec = 0;
						int mret = select(*(mahdata->fd->ffpipe)+1,&nRead,NULL,NULL,&mtime);
						if (mret < 0)
						{
						#ifdef MAJOR_DEBUG
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
							if (FD_ISSET(*(mahdata->fd->ffpipe),&nRead))
							{
								memset(buf,0,sizeof(buf));
								read(*(mahdata->fd->ffpipe),buf,sizeof(buf));
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
					}//while loop

					if (0 == ffw)
					{
					#if 0
						if (0 == madcpcyes)
						{
							memset(&mahpcdata,0,sizeof(mahpcdata));
							mahpcdata.bbserial = mahdata->fd->bbserial;
							mahpcdata.pchan = pinfo->cpchan;
							mahpcdata.markbit = mahdata->fd->markbit;
							mahpcdata.sockfd = mahdata->fd->sockfd;
							mahpcdata.cs = mahdata->cs;
							mahpcdata.time = pinfo->pctime;
							int pcret=pthread_create(&madcpcid,NULL, \
													(void *)madc_person_chan_greenflash,&mahpcdata);
							if (0 != pcret)	
							{
							#ifdef MAJOR_DEBUG
								printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
								output_log("major half control,create person green flash err");
							#endif
								return;
							}
							madcpcyes = 1;
						}
					#endif
						if (0 == pinfo->cchan[0])
						{
							if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
							{//if((!(*(mahdata->fd->markbit) & 0x1000))&&(!(*(mahdata->fd->markbit) & 0x8000)))
								sinfo.time = pinfo->pctime + pinfo->gftime + pinfo->ytime + pinfo->rtime;	
								sinfo.conmode = *(mahdata->fd->contmode);//added on 20150529
								sinfo.color = 0x02;
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
								memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
								memset(sibuf,0,sizeof(sibuf));
            					if (SUCCESS != status_info_report(sibuf,&sinfo))
            					{
            					#ifdef MAJOR_DEBUG
                					printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            					#endif
            					}
            					else
            					{
                					write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            					}	
							}//if((!(*(mahdata->fd->markbit) & 0x1000))&&(!(*(mahdata->fd->markbit) & 0x8000)))
						}
						else
						{
							if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
							{//if((!(*(mahdata->fd->markbit) & 0x1000))&&(!(*(mahdata->fd->markbit) & 0x8000)))
								sinfo.time = pinfo->pctime + pinfo->gftime;	
								sinfo.conmode = *(mahdata->fd->contmode);//added on 20150529
								sinfo.color = 0x02;
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
								memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
								memset(sibuf,0,sizeof(sibuf));
            					if (SUCCESS != status_info_report(sibuf,&sinfo))
            					{
            					#ifdef MAJOR_DEBUG
                					printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            					#endif
            					}
            					else
            					{
                					write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            					}	
							}//if((!(*(mahdata->fd->markbit) & 0x1000))&&(!(*(mahdata->fd->markbit) & 0x8000)))
						}

				//		sleep(pinfo->pctime);
						sltime = pinfo->pctime;
						while (1)
						{//while loop
							FD_ZERO(&nRead);
							FD_SET(*(mahdata->fd->ffpipe),&nRead);
							lasttime.tv_sec = 0;
							lasttime.tv_usec = 0;
							gettimeofday(&lasttime,NULL);
							bakv = sltime;
							mtime.tv_sec = sltime;
							mtime.tv_usec = 0;
							int mret = select(*(mahdata->fd->ffpipe)+1,&nRead,NULL,NULL,&mtime);
							if (mret < 0)
							{
							#ifdef MAJOR_DEBUG
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
								if (FD_ISSET(*(mahdata->fd->ffpipe),&nRead))
								{
									memset(buf,0,sizeof(buf));
									read(*(mahdata->fd->ffpipe),buf,sizeof(buf));
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
						}//while loop

						if (1 == madcpcyes)
						{
							pthread_cancel(madcpcid);
							pthread_join(madcpcid,NULL);
							madcpcyes = 0;
						}
					}//ffw = 0
				}//exist person channels
				else
				{//not exist person channels
			//		sleep(pinfo->gtime);
					sltime = pinfo->gtime;
					while (1)
					{//while loop
						FD_ZERO(&nRead);
						FD_SET(*(mahdata->fd->ffpipe),&nRead);
						lasttime.tv_sec = 0;
						lasttime.tv_usec = 0;
						gettimeofday(&lasttime,NULL);
						bakv = sltime;
						mtime.tv_sec = sltime;
						mtime.tv_usec = 0;
						int mret = select(*(mahdata->fd->ffpipe)+1,&nRead,NULL,NULL,&mtime);
						if (mret < 0)
						{
						#ifdef MAJOR_DEBUG
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
							if (FD_ISSET(*(mahdata->fd->ffpipe),&nRead))
							{
								memset(buf,0,sizeof(buf));
								read(*(mahdata->fd->ffpipe),buf,sizeof(buf));
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
					}//while loop
				}//not exist person channels

				//begin to green flash
				if ((0 != pinfo->cchan[0]) && (pinfo->gftime > 0))
				{
					if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
					{//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
						sinfo.time = pinfo->gftime;	
						sinfo.conmode = *(mahdata->fd->contmode);//added on 20150529
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
						memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
						memset(sibuf,0,sizeof(sibuf));
            			if (SUCCESS != status_info_report(sibuf,&sinfo))
            			{
            			#ifdef MAJOR_DEBUG
                			printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            			#endif
            			}
            			else
            			{
                			write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            			}	
					}//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
				}

				memset(&gtime,0,sizeof(gtime));
            	memset(&gftime,0,sizeof(gftime));
				gettimeofday(&gftime,NULL);
            	memset(&ytime,0,sizeof(ytime));
            	memset(&rtime,0,sizeof(rtime));
				*(mahdata->fd->markbit) |= 0x0400;
				gft = 0;
				if (pinfo->gftime > 0)
				{
					while (1)
					{
						if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->cchan,0x03))
						{
						#ifdef MAJOR_DEBUG
							printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(mahdata->fd->markbit) |= 0x0800;
							gettimeofday(&ct,NULL);
							update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15, \
												ct.tv_sec,mahdata->fd->markbit);
							if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
															mahdata->fd->softevent,mahdata->fd->markbit))
							{
							#ifdef MAJOR_DEBUG
								printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}
						}
						if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs, \
														pinfo->cchan,0x03,mahdata->fd->markbit))
						{
						#ifdef MAJOR_DEBUG
							printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
						timeout.tv_sec = 0;
						timeout.tv_usec = 500000;
						select(0,NULL,NULL,NULL,&timeout);

						if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->cchan,0x02))
						{
						#ifdef MAJOR_DEBUG
							printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(mahdata->fd->markbit) |= 0x0800;
							gettimeofday(&ct,NULL);
							update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15, \
												ct.tv_sec,mahdata->fd->markbit);
							if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
															mahdata->fd->softevent,mahdata->fd->markbit))
							{
							#ifdef MAJOR_DEBUG
								printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}
						}
						if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs, \
														pinfo->cchan,0x02,mahdata->fd->markbit))
						{
						#ifdef MAJOR_DEBUG
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
				}//if (pinfo->gftime > 0)
				//end green flash
				if (1 == phcon)
                {
                    *(mahdata->fd->markbit) &= 0xfbff;
                    memset(&gtime,0,sizeof(gtime));
                    gettimeofday(&gtime,NULL);
                    memset(&gftime,0,sizeof(gftime));
                    memset(&ytime,0,sizeof(ytime));
                    memset(&rtime,0,sizeof(rtime));
                    phcon = 0;
                    sleep(10);
                }
				#ifdef RED_FLASH	
				if (0 == mahdata->td->timeconfig->TimeConfigList[tcline][slnum+1].StageId)
				{
					rft = (mahdata->td->timeconfig->TimeConfigList[tcline][0].SpecFunc & 0xe0) >> 5;
				}
				else
				{
					rft = (mahdata->td->timeconfig->TimeConfigList[tcline][slnum+1].SpecFunc & 0xe0) >> 5;
				}
				if (rft > 0)
				{
					redflash_mah		mah;
					mah.tcline = tcline;
					mah.slnum = slnum;
					mah.snum =	snum;
					mah.rft = rft;
					mah.chan = pinfo->chan;
					mah.mah = mahdata;
					int rfarg = pthread_create(&rfpid,NULL,(void *)mah_red_flash,&mah);
					if (0 != rfarg)
					{
					#ifdef MAJOR_DEBUG
						printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						return;
					}
				}//if (rft > 0) 
				#endif				

				//Begin to set yellow lamp
				if (1 == (pinfo->cpcexist))
				{
					if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->cpchan,0x00))
					{
					#ifdef MAJOR_DEBUG
						printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						*(mahdata->fd->markbit) |= 0x0800;
						gettimeofday(&ct,NULL);
                		update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15,ct.tv_sec,mahdata->fd->markbit);
                		if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
															mahdata->fd->softevent,mahdata->fd->markbit))
                		{
                		#ifdef MAJOR_DEBUG
                    		printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                		#endif
                		}
					}
					if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,pinfo->cpchan,0x00, \
                                                    mahdata->fd->markbit))
                	{
                	#ifdef MAJOR_DEBUG
                    	printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                	}
					if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->cnpchan,0x01))
					{
					#ifdef MAJOR_DEBUG
						printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						*(mahdata->fd->markbit) |= 0x0800;
						gettimeofday(&ct,NULL);
                		update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15,ct.tv_sec,mahdata->fd->markbit);
                		if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
															mahdata->fd->softevent,mahdata->fd->markbit))
                		{
                		#ifdef MAJOR_DEBUG
                    		printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                		#endif
                		}
					}
					if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,pinfo->cnpchan,0x01, \
                                                    mahdata->fd->markbit))
                	{
                	#ifdef MAJOR_DEBUG
                    	printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                	}
				}
				else
				{
					if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->cchan,0x01))
					{
					#ifdef MAJOR_DEBUG
						printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						*(mahdata->fd->markbit) |= 0x0800;
						gettimeofday(&ct,NULL);
                		update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15,ct.tv_sec,mahdata->fd->markbit);
                		if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
															mahdata->fd->softevent,mahdata->fd->markbit))
                		{
                		#ifdef MAJOR_DEBUG
                    		printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                		#endif
                		}
					}
					if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,pinfo->cchan,0x01, \
                                                    mahdata->fd->markbit))
                	{
                	#ifdef MAJOR_DEBUG
                    	printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                	}
				}

				if ((0 != pinfo->cchan[0]) && (pinfo->ytime > 0))
				{
					if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
					{//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
						sinfo.time = pinfo->ytime;	
						sinfo.conmode = *(mahdata->fd->contmode);//added on 20150529
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
						memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
						memset(sibuf,0,sizeof(sibuf));
            			if (SUCCESS != status_info_report(sibuf,&sinfo))
            			{
            			#ifdef MAJOR_DEBUG
                			printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            			#endif
            			}
            			else
            			{
                			write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            			}	
					}//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
				}

				*(mahdata->fd->color) = 0x01;
				memset(&gtime,0,sizeof(gtime));
            	memset(&gftime,0,sizeof(gftime));
            	memset(&ytime,0,sizeof(ytime));
				gettimeofday(&ytime,NULL);
            	memset(&rtime,0,sizeof(rtime));
				if ((*(mahdata->fd->markbit) & 0x0002) && (*(mahdata->fd->markbit) & 0x0010))
				{
					memset(&timedown,0,sizeof(timedown));
					timedown.mode = *(mahdata->fd->contmode);
					timedown.pattern = *(mahdata->fd->patternid);
					timedown.lampcolor = 0x01;
					timedown.lamptime = pinfo->ytime;
					timedown.phaseid = pinfo->phaseid;
					timedown.stageline = pinfo->stageline;
					if (SUCCESS != timedown_data_to_conftool(mahdata->fd->sockfd,&timedown))
					{
					#ifdef MAJOR_DEBUG
						printf("send time err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
				}
				#ifdef EMBED_CONFIGURE_TOOL
				if (*(mahdata->fd->markbit2) & 0x0200)
				{
					memset(&timedown,0,sizeof(timedown));
                    timedown.mode = *(mahdata->fd->contmode);
                    timedown.pattern = *(mahdata->fd->patternid);
                    timedown.lampcolor = 0x01;
                    timedown.lamptime = pinfo->ytime;
                    timedown.phaseid = pinfo->phaseid;
                    timedown.stageline = pinfo->stageline;	
					if (SUCCESS != timedown_data_to_embed(mahdata->fd->sockfd,&timedown))
					{
					#ifdef MAJOR_DEBUG
						printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
				}
				#endif
				if (*(mahdata->fd->contmode) < 27)
                	fbdata[1] = *(mahdata->fd->contmode) + 1;
            	else
                	fbdata[1] = *(mahdata->fd->contmode);
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
				if (!wait_write_serial(*(mahdata->fd->fbserial)))
				{
					if (write(*(mahdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
					{
					#ifdef MAJOR_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						*(mahdata->fd->markbit) |= 0x0800;
						gettimeofday(&ct,NULL);
                		update_event_list(mahdata->fd->ec,mahdata->fd->el,1,16,ct.tv_sec,mahdata->fd->markbit);
                		if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
															mahdata->fd->softevent,mahdata->fd->markbit))
                		{
                		#ifdef MAJOR_DEBUG
                    		printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                		#endif
                		}
					}
				}
				else
				{
				#ifdef MAJOR_DEBUG
					printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				sendfaceInfoToBoard(mahdata->fd,fbdata);
				sleep(pinfo->ytime);
				//End set yellow lamp
				//Begin to set red lamp
				if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->cchan,0x00))
				{
				#ifdef MAJOR_DEBUG
					printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mahdata->fd->markbit) |= 0x0800;
					gettimeofday(&ct,NULL);
                	update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15,ct.tv_sec,mahdata->fd->markbit);
                	if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
													mahdata->fd->softevent,mahdata->fd->markbit))
                	{
                	#ifdef MAJOR_DEBUG
                    	printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                	}
				}
				if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,pinfo->cchan,0x00, \
                                                mahdata->fd->markbit))
            	{
            	#ifdef MAJOR_DEBUG
                	printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
            	#endif
            	}

				if (0 != pinfo->cchan[0])
				{
					if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
					{//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
						sinfo.time = pinfo->rtime;	
						sinfo.conmode = *(mahdata->fd->contmode);//added on 20150529
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
						memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
						memset(sibuf,0,sizeof(sibuf));
            			if (SUCCESS != status_info_report(sibuf,&sinfo))
            			{
            			#ifdef MAJOR_DEBUG
                			printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            			#endif
            			}
            			else
            			{
                			write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            			}	
					}//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
				}

				*(mahdata->fd->color) = 0x00;
				if (pinfo->rtime > 0)
				{
					memset(&gtime,0,sizeof(gtime));
            		memset(&gftime,0,sizeof(gftime));
            		memset(&ytime,0,sizeof(ytime));
            		memset(&rtime,0,sizeof(rtime));
					gettimeofday(&rtime,NULL);
					if ((*(mahdata->fd->markbit) & 0x0002) && (*(mahdata->fd->markbit) & 0x0010))
					{
						memset(&timedown,0,sizeof(timedown));
						timedown.mode = *(mahdata->fd->contmode);
						timedown.pattern = *(mahdata->fd->patternid);
						timedown.lampcolor = 0x00;
						timedown.lamptime = pinfo->rtime;
						timedown.phaseid = pinfo->phaseid;
						timedown.stageline = pinfo->stageline;
						if (SUCCESS != timedown_data_to_conftool(mahdata->fd->sockfd,&timedown))
						{
						#ifdef MAJOR_DEBUG
							printf("send time err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
					}
					#ifdef EMBED_CONFIGURE_TOOL
					if (*(mahdata->fd->markbit2) & 0x0200)
					{
						memset(&timedown,0,sizeof(timedown));
                        timedown.mode = *(mahdata->fd->contmode);
                        timedown.pattern = *(mahdata->fd->patternid);
                        timedown.lampcolor = 0x00;
                        timedown.lamptime = pinfo->rtime;
                        timedown.phaseid = pinfo->phaseid;
                        timedown.stageline = pinfo->stageline;	
						if (SUCCESS != timedown_data_to_embed(mahdata->fd->sockfd,&timedown))
						{
						#ifdef MAJOR_DEBUG
							printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
					}
					#endif
					//send info to face board
					if (*(mahdata->fd->contmode) < 27)
                		fbdata[1] = *(mahdata->fd->contmode) + 1;
            		else
                		fbdata[1] = *(mahdata->fd->contmode);
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
					if (!wait_write_serial(*(mahdata->fd->fbserial)))
					{
						if (write(*(mahdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
						{
						#ifdef MAJOR_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(mahdata->fd->markbit) |= 0x0800;
							gettimeofday(&ct,NULL);
                			update_event_list(mahdata->fd->ec,mahdata->fd->el,1,16, \
												ct.tv_sec,mahdata->fd->markbit);
                			if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
																mahdata->fd->softevent,mahdata->fd->markbit))
                			{
                			#ifdef MAJOR_DEBUG
                    			printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                			#endif
                			}
						}
					}
					else
					{
					#ifdef MAJOR_DEBUG
						printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					sendfaceInfoToBoard(mahdata->fd,fbdata);
					sleep(pinfo->rtime);
				}//if (pinfo->rtime > 0)

				if (1 == phcon)
            	{
					*(mahdata->fd->markbit) &= 0xfbff;
                	memset(&gtime,0,sizeof(gtime));
                	gettimeofday(&gtime,NULL);
                	memset(&gftime,0,sizeof(gftime));
                	memset(&ytime,0,sizeof(ytime));
                	memset(&rtime,0,sizeof(rtime));
                	phcon = 0;
                	sleep(10);
            	}

				ni += 1;//20220622
				slnum += 1;
				*(mahdata->fd->slnum) = slnum;
				*(mahdata->fd->stageid) = mahdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
				if (0 == (mahdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId))
				{
					cycarr = 1;
					slnum = 0;
					*(mahdata->fd->slnum) = 0;
					*(mahdata->fd->stageid) = mahdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
				}
				#ifdef RED_FLASH
				if (rft > (pinfo->ytime + pinfo->rtime))
				{
					sleep(rft - (pinfo->ytime) - (pinfo->rtime));
				}
				#endif

				continue;
			}// if (0 == validmark)
			if (1 == validmark)
			{
				#ifdef MAJOR_DEBUG
					printf("Begin to pass flexible phase,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				//add 20220803 
				if (*(mahdata->fd->phasedegrphid) & (0x00000001 << (pinfo->phaseid - 1)))
				{
					*(mahdata->fd->phasedegrphid) &= (~(0x00000001 << (pinfo->phaseid - 1)));
					gettimeofday(&ct,NULL);
					update_event_list(mahdata->fd->ec,mahdata->fd->el,1,129,ct.tv_sec,mahdata->fd->markbit);
		
					memset(err1,0,sizeof(err1));
					if (SUCCESS != err_report(err1,ct.tv_sec,21,(82+(pinfo->phaseid - 1))))
					{
						#ifdef MAJOR_DEBUG
						printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
					}
					else
					{
						write(*(mahdata->fd->sockfd->csockfd),err1,sizeof(err1));
					}
				}

				//add 20220803 

				*(mahdata->fd->color) = 0x02;
				*(mahdata->fd->markbit) &= 0xfbff;
				if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->chan,0x02))
				{
				#ifdef MAJOR_DEBUG
					printf("set green err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mahdata->fd->markbit) |= 0x0800;
					gettimeofday(&ct,NULL);
                	update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15,ct.tv_sec,mahdata->fd->markbit);
                	if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
														mahdata->fd->softevent,mahdata->fd->markbit))
                	{
                	#ifdef MAJOR_DEBUG
                    	printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                	}
				}
				#ifdef CHANNEL_YELLOW_FLASH
				cyfn = 0;
				cyfe = 0;
				for (cyfn = 0; cyfn < mahdata->td->phaseerror->FactPhaseErrorNum; cyfn++)
				{		
					if (pinfo->phaseid	== mahdata->td->phaseerror->PhaseErrorList[cyfn].PhaseId)
					{
						cyfe = /*0xfffe1fff &*/ mahdata->td->phaseerror->PhaseErrorList[cyfn].PhaseConflict;
						break;
					}
				}
				memset(cyfc,0,sizeof(cyfc));
				pcyfc = cyfc;
				if (cyfe > 0)
				{//exist channel yellow flash
					for (cyfn = 0; cyfn < YFCHANNEL; cyfn++)
					{
						if (cyfe & (0x01 << cyfn))
						{
							*pcyfc = cyfn + 1;
							pcyfc++;
						}
					}
					cyft.cyft = pinfo->gtime + pinfo->gftime + pinfo->ytime + pinfo->rtime -1;//for thread exit
					cyft.chan = cyfc;
					cyft.mah = mahdata;
					cyft.mark = 1;
					int yfarg = pthread_create(&yfcpid,NULL,(void *)mah_channel_yellow_flash,&cyft);
					if (0 != yfarg)
					{
					#ifdef TIMING_DEBUG
						printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						return;
					}
					else
					{//create channel yellow flash thread success;
					#ifdef TIMING_DEBUG
						printf("Create channel yellow flash thread success,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
						{//actively report is not probitted and connect successfully
							sinfo.time = pinfo->gtime + pinfo->gftime + pinfo->ytime + pinfo->rtime - 1;
							sinfo.conmode = *(mahdata->fd->contmode);
							sinfo.color = 0x05;//yellow flash
							sinfo.chans = 0;
							memset(sinfo.csta,0,sizeof(sinfo.csta));
							csta = sinfo.csta;
							for (i = 0; i < YFCHANNEL; i++)
							{
								if (0 == cyfc[i])
									break;
								sinfo.chans += 1;
								tcsta = cyfc[i];
								tcsta <<= 2;
								tcsta &= 0xfc;
								tcsta |= 0x00;
								*csta = tcsta;
								csta++;
							}
							memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
							memset(sibuf,0,sizeof(sibuf));
							if (SUCCESS != status_info_report(sibuf,&sinfo))	
							{
							#ifdef TIMING_DEBUG
								printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}
							else
							{
								write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
							}
						}//actively report is not probitted and connect successfully
					}//create channel yellow flash thread success;
				}//exist channel yellow flash
				#endif

				#ifdef CHANNEL_CLOSE
				cyfn = 0;
				cyfe = 0;
				for (cyfn = 0; cyfn < mahdata->td->phaseerror->FactPhaseErrorNum; cyfn++)
				{
					if (pinfo->phaseid  == mahdata->td->phaseerror->PhaseErrorList[cyfn].PhaseId)
					{
						cyfe = /*0xfffe1fff &*/ mahdata->td->phaseerror->PhaseErrorList[cyfn].PhaseConflict;
						break;
					}
				}
				memset(cyfc,0,sizeof(cyfc));
				pcyfc = cyfc;
				if (cyfe > 0)
				{//exist channel close lamp
					for (cyfn = 0; cyfn < YFCHANNEL; cyfn++)
					{
						if (cyfe & (0x01 << cyfn))
						{
							*pcyfc = cyfn + 1;
							pcyfc++;
						}
					}
					cyft.cyft = pinfo->gtime + pinfo->gftime + pinfo->ytime + pinfo->rtime;
					cyft.chan = cyfc;
					cyft.mah = mahdata;
					cyft.mark = 1;

					if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),cyfc,0x03))
					{
					#ifdef TIMING_DEBUG
						printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,cyfc,0x03,mahdata->fd->markbit))
					{
					#ifdef TIMING_DEBUG
						printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					channel_close_begin_report(mahdata->fd->sockfd->csockfd,cyfc);
				}//exist channel close lamp
				#endif

				memset(&gtime,0,sizeof(gtime));
            	gettimeofday(&gtime,NULL);
            	memset(&gftime,0,sizeof(gftime));
            	memset(&ytime,0,sizeof(ytime));
            	memset(&rtime,0,sizeof(rtime));
				if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,pinfo->chan,0x02, \
												mahdata->fd->markbit))
				{
				#ifdef MAJOR_DEBUG
					printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				if (0 == pinfo->cchan[0])
				{
					if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
					{//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
						sinfo.time = pinfo->mingtime + pinfo->gftime + pinfo->ytime + pinfo->rtime;	
						sinfo.color = 0x02;
						sinfo.conmode = *(mahdata->fd->contmode);//added on 20150529
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
						memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            			memset(sibuf,0,sizeof(sibuf));
            			if (SUCCESS != status_info_report(sibuf,&sinfo))
            			{
            			#ifdef MAJOR_DEBUG
                			printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            			#endif
            			}
            			else
            			{
                			write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            			}	
					}//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
				}
				else
				{
					if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
					{//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
						sinfo.time = pinfo->mingtime + pinfo->gftime;	
						sinfo.color = 0x02;
						sinfo.conmode = *(mahdata->fd->contmode);//added on 20150529
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
						memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            			memset(sibuf,0,sizeof(sibuf));
            			if (SUCCESS != status_info_report(sibuf,&sinfo))
            			{
            			#ifdef MAJOR_DEBUG
                			printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            			#endif
            			}
            			else
            			{
                			write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            			}	
					}//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
				}

				if ((*(mahdata->fd->markbit) & 0x0002) && (*(mahdata->fd->markbit) & 0x0010))
				{
					memset(&timedown,0,sizeof(timedown));
					timedown.mode = *(mahdata->fd->contmode);
					timedown.pattern = *(mahdata->fd->patternid);
					timedown.lampcolor = 0x02;
					timedown.lamptime = pinfo->mingtime + pinfo->gftime;
					timedown.phaseid = pinfo->phaseid;
					timedown.stageline = pinfo->stageline;
					if (SUCCESS != timedown_data_to_conftool(mahdata->fd->sockfd,&timedown))
					{
					#ifdef MAJOR_DEBUG
						printf("send time err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
				}
				#ifdef EMBED_CONFIGURE_TOOL
				if (*(mahdata->fd->markbit2) & 0x0200)
				{
					memset(&timedown,0,sizeof(timedown));
                    timedown.mode = *(mahdata->fd->contmode);
                    timedown.pattern = *(mahdata->fd->patternid);
                    timedown.lampcolor = 0x02;
                    timedown.lamptime = pinfo->mingtime + pinfo->gftime;
                    timedown.phaseid = pinfo->phaseid;
                    timedown.stageline = pinfo->stageline;	
					if (SUCCESS != timedown_data_to_embed(mahdata->fd->sockfd,&timedown))
					{
					#ifdef MAJOR_DEBUG
						printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
				}
				#endif
				//send info to face board
				if (*(mahdata->fd->contmode) < 27)
                	fbdata[1] = *(mahdata->fd->contmode) + 1;
            	else
                	fbdata[1] = *(mahdata->fd->contmode);
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
					fbdata[4] = pinfo->mingtime + pinfo->gftime;
				}
				if (!wait_write_serial(*(mahdata->fd->fbserial)))
				{
					if (write(*(mahdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
					{
					#ifdef MAJOR_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						*(mahdata->fd->markbit) |= 0x0800;
						gettimeofday(&ct,NULL);
                		update_event_list(mahdata->fd->ec,mahdata->fd->el,1,16,ct.tv_sec,mahdata->fd->markbit);
                		if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
															mahdata->fd->softevent,mahdata->fd->markbit))
                		{
                		#ifdef MAJOR_DEBUG
                    		printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                		#endif
                		}
					}
				}
				else
				{
				#ifdef MAJOR_DEBUG
					printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				sendfaceInfoToBoard(mahdata->fd,fbdata);
				if (1 == (pinfo->cpcexist))
				{//exist person channels
			//		sleep(pinfo->mingtime - pinfo->pdelay);
					sltime = pinfo->mingtime - pinfo->ldeti;
					ffw = 0;
					while (1)
					{//while loop
						FD_ZERO(&nRead);
						FD_SET(*(mahdata->fd->ffpipe),&nRead);
						lasttime.tv_sec = 0;
						lasttime.tv_usec = 0;
						gettimeofday(&lasttime,NULL);
						bakv = sltime;
						mtime.tv_sec = sltime;
						mtime.tv_usec = 0;
						int mret = select(*(mahdata->fd->ffpipe)+1,&nRead,NULL,NULL,&mtime);
						if (mret < 0)
						{
						#ifdef MAJOR_DEBUG
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
							if (FD_ISSET(*(mahdata->fd->ffpipe),&nRead))
							{
								memset(buf,0,sizeof(buf));
								read(*(mahdata->fd->ffpipe),buf,sizeof(buf));
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
					}//while loop

					if (0 == ffw)
					{
					#if 0
						if (0 == madcpcyes)
						{
							memset(&mahpcdata,0,sizeof(mahpcdata));
							mahpcdata.bbserial = mahdata->fd->bbserial;
							mahpcdata.pchan = pinfo->cpchan;
							mahpcdata.markbit = mahdata->fd->markbit;
							mahpcdata.sockfd = mahdata->fd->sockfd;
							mahpcdata.cs = mahdata->cs;
							mahpcdata.time = pinfo->pctime;
							int pcret = pthread_create(&madcpcid,NULL, \
														(void *)madc_person_chan_greenflash,&mahpcdata);
							if (0 != pcret)	
							{
							#ifdef MAJOR_DEBUG
								printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
								output_log("major half detect,create person green flash err");
							#endif
								return;
							}
							madcpcyes = 1;
						}
					#endif
						if (0 == pinfo->cchan[0])
						{
							if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
                			{//if((!(*(mahdata->fd->markbit) & 0x1000))&&(!(*(mahdata->fd->markbit) & 0x8000)))
                    			sinfo.time = pinfo->ldeti + pinfo->gftime + pinfo->ytime + pinfo->rtime;
								sinfo.conmode = *(mahdata->fd->contmode);//added on 20150529
                    			sinfo.color = 0x02;
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
								memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
                    			memset(sibuf,0,sizeof(sibuf));
                    			if (SUCCESS != status_info_report(sibuf,&sinfo))
                    			{
                    			#ifdef MAJOR_DEBUG
                        			printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    			#endif
                    			}
								else
                    			{
                        			write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
                    			}
							}//if((!(*(mahdata->fd->markbit) & 0x1000))&&(!(*(mahdata->fd->markbit) & 0x8000)))
						}
						else
						{
							if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
                			{//if((!(*(mahdata->fd->markbit) & 0x1000))&&(!(*(mahdata->fd->markbit) & 0x8000)))
                    			sinfo.time = pinfo->ldeti + pinfo->gftime;
								sinfo.conmode = *(mahdata->fd->contmode);//added on 20150529
                    			sinfo.color = 0x02;
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
								memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
                    			memset(sibuf,0,sizeof(sibuf));
                    			if (SUCCESS != status_info_report(sibuf,&sinfo))
                    			{
                    			#ifdef MAJOR_DEBUG
                        			printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    			#endif
                    			}
								else
                    			{
                        			write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
                    			}
							}//if((!(*(mahdata->fd->markbit) & 0x1000))&&(!(*(mahdata->fd->markbit) & 0x8000)))
						}	
					}//0 == ffw
				}//exist person channels
				else
				{//not exist person channels
			//		sleep(pinfo->mingtime - pinfo->pdelay);
					sltime = pinfo->mingtime - pinfo->ldeti;
					ffw = 0;
					while (1)
					{//while loop
						FD_ZERO(&nRead);
						FD_SET(*(mahdata->fd->ffpipe),&nRead);
						lasttime.tv_sec = 0;
						lasttime.tv_usec = 0;
						gettimeofday(&lasttime,NULL);
						bakv = sltime;
						mtime.tv_sec = sltime;
						mtime.tv_usec = 0;
						int mret = select(*(mahdata->fd->ffpipe)+1,&nRead,NULL,NULL,&mtime);
						if (mret < 0)
						{
						#ifdef MAJOR_DEBUG
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
							if (FD_ISSET(*(mahdata->fd->ffpipe),&nRead))
							{
								memset(buf,0,sizeof(buf));
								read(*(mahdata->fd->ffpipe),buf,sizeof(buf));
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
					}//while loop
				}//not exist person channels

				if (0 == ffw)
				{
				#ifdef MAJOR_DEBUG
					printf("Begin monitor vehicle,time:%d,File:%s,Line:%d\n",pinfo->ldeti,__FILE__,__LINE__);
				#endif
					mt = pinfo->ldeti;
					mintime = pinfo->mingtime;
					int 	maxv = 0;
					while (1)
					{//inline while loop
						//clean pipe
                		while (1)
                		{
                    		memset(buf,0,sizeof(buf));
                    		if (read(*(mahdata->fd->flowpipe),buf,sizeof(buf)) <= 0)
                        		break;
                		}
						FD_ZERO(&nRead);
						FD_SET(*(mahdata->fd->flowpipe),&nRead);
						FD_SET(*(mahdata->fd->ffpipe),&nRead);
						lasttime.tv_sec = 0;
						lasttime.tv_usec = 0;
						gettimeofday(&lasttime,NULL);
						bakv = mt;
						mtime.tv_sec = mt;
						mtime.tv_usec = 0;
						#ifdef MAJOR_DEBUG
						printf("Begin to monitor vehicle,time:%d,File: %s,Line: %d\n",mt,__FILE__,__LINE__);	
						#endif
						maxv = MAX(*(mahdata->fd->flowpipe),*(mahdata->fd->ffpipe));
						int mret = select(maxv+1,&nRead,NULL,NULL,&mtime);
						if (mret < 0)
						{
						#ifdef MAJOR_DEBUG
							printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
							output_log("major half detect,select call err");
						#endif
							return;
						}
						if (0 == mret)
						{//time out
						#ifdef MAJOR_DEBUG
							printf("******************Time arrived,FIle: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							break;
						}//time out
						if (mret > 0)
						{//mret > 0
						#ifdef MAJOR_DEBUG
                        	printf("*********************num: %d,FIle: %s,Line: %d\n",num,__FILE__,__LINE__);
                        #endif
							if (FD_ISSET(*(mahdata->fd->ffpipe),&nRead))
							{
								num = 0;
                                memset(buf,0,sizeof(buf));
                                pbuf = buf;
                                num = read(*(mahdata->fd->ffpipe),buf,sizeof(buf));
								if (num > sizeof(buf))
                                	continue;
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
                        			mt -= leatime;
									if (mt > bakv)
										mt = bakv;
                        			continue;
								}
							}
							else if (FD_ISSET(*(mahdata->fd->flowpipe),&nRead))
							{
								num = 0;
								memset(buf,0,sizeof(buf));
								pbuf = buf;
								num = read(*(mahdata->fd->flowpipe),buf,sizeof(buf));
								if (num > sizeof(buf))
                                	continue;
								if (num > 0)
								{
									mark = 0;
									caryes = 0;
									while (1)
									{//inline 2th while loop
										if (mark >= num)
											break;
										if ((0xF1 == *(pbuf+mark)) && (0xED == *(pbuf+mark+5)))
										{
											deteid = *(pbuf+mark+2);
                                    		if (0x01 == *(pbuf+mark+1))
                                    		{//have vehicle pass
                                        		for (j = 0; j < (mahdata->td->detector->FactDetectorNum); j++)
                                        		{
                                            		if(0==(mahdata->td->detector->DetectorList[j].DetectorId))
                                                		break;
                                            	if(deteid==(mahdata->td->detector->DetectorList[j].DetectorId))
                                            		{
                                                	dtype = mahdata->td->detector->DetectorList[j].DetectorType;
                                                	if ((0x40 == dtype) || (0x80 == dtype) || (0x04 == dtype))
                                                	{//request or response detector
                                                    	mappid = \
                                                    	mahdata->td->detector->DetectorList[j].DetectorPhase;
                                                    	if (mappid == (pinfo->phaseid))
                                                    	{
                                                        	caryes = 1;
                                                        	break;
                                                    	}
                                                	}//request or response detector
                                                	else
                                                	{//unfit detector
                                                    	break;
                                                	}//unfit detector
                                            		}
                                        		}//for (j = 0; j < (dcdata->td->detector->FactDetectorNum); j++)
                                        		if (1 == caryes)
                                            		break;
                                    		}//have vehicle pass
                                    		mark += 6;
                                    		continue;//Fit data,break directly
										}//if ((0xF1 == *(pbuf+mark)) && (0xED == *(pbuf+mark+5)))
										else
										{
											mark += 1;
											continue;
										}
									}//inline 2th while loop
									if (1 == caryes)
									{//have vehicle pass the detector
										nowtime.tv_sec = 0;
										nowtime.tv_usec = 0;
										gettimeofday(&nowtime,NULL);
										leatime = nowtime.tv_sec - lasttime.tv_sec;
										mt -= leatime;
										if (mt > bakv)
											mt = bakv;
										if ((mintime + (pinfo->pdelay)) >= (pinfo->maxgtime1))
										{
										#ifdef MAJOR_DEBUG
											printf("Has arrived max green,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
											if ((!(*(mahdata->fd->markbit) & 0x1000)) && \
												(!(*(mahdata->fd->markbit) & 0x8000)))
                							{
												if (0 == pinfo->cchan[0])
												{
													sinfo.time = mt + (pinfo->maxgtime1) - mintime + \
																(pinfo->gftime) + pinfo->ytime + pinfo->rtime;
												}
												else
												{ 
                    								sinfo.time=mt+(pinfo->maxgtime1)-mintime+pinfo->gftime;
												}
												memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
                    							memset(sibuf,0,sizeof(sibuf));
                    							if (SUCCESS != status_info_report(sibuf,&sinfo))
                    							{
                    							#ifdef MAJOR_DEBUG
                        							printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    							#endif
                    							}
												else
                    							{
                        							write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
                    							}
											}
										if((*(mahdata->fd->markbit)&0x0002)&&(*(mahdata->fd->markbit)&0x0010))
											{
												memset(&timedown,0,sizeof(timedown));
												timedown.mode = *(mahdata->fd->contmode);
												timedown.pattern = *(mahdata->fd->patternid);
												timedown.lampcolor = 0x02;
												timedown.lamptime=mt+(pinfo->maxgtime1)-mintime+(pinfo->gftime);
												timedown.phaseid = pinfo->phaseid;
												timedown.stageline = pinfo->stageline;
										  if(SUCCESS!=timedown_data_to_conftool(mahdata->fd->sockfd,&timedown))
												{
												#ifdef MAJOR_DEBUG
													printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
											}
											#ifdef EMBED_CONFIGURE_TOOL
											if (*(mahdata->fd->markbit2) & 0x0200)
											{
												memset(&timedown,0,sizeof(timedown));
                                                timedown.mode = *(mahdata->fd->contmode);
                                                timedown.pattern = *(mahdata->fd->patternid);
                                                timedown.lampcolor = 0x02;
                                                timedown.lamptime=mt+(pinfo->maxgtime1)-mintime+(pinfo->gftime);
                                                timedown.phaseid = pinfo->phaseid;
                                                timedown.stageline = pinfo->stageline;
												if (SUCCESS != \
												timedown_data_to_embed(mahdata->fd->sockfd,&timedown))
												{
												#ifdef MAJOR_DEBUG
													printf("timedown fail,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
											}
										#endif
											//send info to face board
											if (*(mahdata->fd->contmode) < 27)
                								fbdata[1] = *(mahdata->fd->contmode) + 1;
            								else
                								fbdata[1] = *(mahdata->fd->contmode);
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
												fbdata[4] = mt+(pinfo->maxgtime1) - mintime + (pinfo->gftime);
											}
											if (!wait_write_serial(*(mahdata->fd->fbserial)))
											{
												if (write(*(mahdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
												{
												#ifdef MAJOR_DEBUG
													printf("write err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
													gettimeofday(&ct,NULL);
                									update_event_list(mahdata->fd->ec,mahdata->fd->el, \
																		1,16,ct.tv_sec,mahdata->fd->markbit);
                									if (SUCCESS != generate_event_file(mahdata->fd->ec,\
												mahdata->fd->el,mahdata->fd->softevent,mahdata->fd->markbit))
                									{
                									#ifdef MAJOR_DEBUG
                    									printf("generate file err,File: %s,Line: %d\n", \
																					__FILE__,__LINE__);
                									#endif
                									}
													*(mahdata->fd->markbit) |= 0x0800;
												}
											}
											else
											{
											#ifdef MAJOR_DEBUG
												printf("pipe cannot write,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											sendfaceInfoToBoard(mahdata->fd,fbdata);
											sleep(mt + (pinfo->maxgtime1) - mintime);
											//added on 20220620
                                        	adaptd.phi[ni].paddt += (pinfo->maxgtime1) - mintime;
                                        	//end add
											break;
										}//if ((mintime + (pinfo->pdelay)) >= (pinfo->maxgtime1))
										else
										{
											if ((!(*(mahdata->fd->markbit) & 0x1000)) && \
                                                (!(*(mahdata->fd->markbit) & 0x8000)))
                                        	{
												if (0 == pinfo->cchan[0])
												{
													sinfo.time = mt + (pinfo->pdelay) + pinfo->gftime + \
																	pinfo->ytime + pinfo->rtime;
												}
												else
												{
                                            		sinfo.time = mt + (pinfo->pdelay) + pinfo->gftime;
												}
												memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
                                            	memset(sibuf,0,sizeof(sibuf));
                                            	if (SUCCESS != status_info_report(sibuf,&sinfo))
                                            	{
                                            	#ifdef MAJOR_DEBUG
                                                	printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                            	#endif
                                            	}
                                            	else
                                            	{
                                                	write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
                                            	}
                                        	}
										if((*(mahdata->fd->markbit)&0x0002)&&(*(mahdata->fd->markbit)&0x0010))
											{
												memset(&timedown,0,sizeof(timedown));
												timedown.mode = *(mahdata->fd->contmode);
												timedown.pattern = *(mahdata->fd->patternid);
												timedown.lampcolor = 0x02;
												timedown.lamptime = mt + (pinfo->pdelay) + (pinfo->gftime);
												timedown.phaseid = pinfo->phaseid;
												timedown.stageline = pinfo->stageline;
										  if(SUCCESS!=timedown_data_to_conftool(mahdata->fd->sockfd,&timedown))
                                        		{
                                        		#ifdef MAJOR_DEBUG
                                            		printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                        		#endif
                                        		}
											}
											#ifdef EMBED_CONFIGURE_TOOL
											if (*(mahdata->fd->markbit2) & 0x0200)
											{
												memset(&timedown,0,sizeof(timedown));
                                                timedown.mode = *(mahdata->fd->contmode);
                                                timedown.pattern = *(mahdata->fd->patternid);
                                                timedown.lampcolor = 0x02;
                                                timedown.lamptime = mt + (pinfo->pdelay) + (pinfo->gftime);
                                                timedown.phaseid = pinfo->phaseid;
                                                timedown.stageline = pinfo->stageline;
												if (SUCCESS != \
												timedown_data_to_embed(mahdata->fd->sockfd,&timedown))
												{
												#ifdef MAJOR_DEBUG
													printf("timedown fail,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
											}
											#endif
											//send info to face board
											if (*(mahdata->fd->contmode) < 27)
                								fbdata[1] = *(mahdata->fd->contmode) + 1;
            								else
                								fbdata[1] = *(mahdata->fd->contmode);
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
												fbdata[4] = mt + (pinfo->pdelay) + (pinfo->gftime);
											}
											if (!wait_write_serial(*(mahdata->fd->fbserial)))
											{
												if (write(*(mahdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
												{
												#ifdef MAJOR_DEBUG
													printf("write err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
													gettimeofday(&ct,NULL);
                									update_event_list(mahdata->fd->ec,mahdata->fd->el,1,16, \
																		ct.tv_sec,mahdata->fd->markbit);
                									if (SUCCESS != generate_event_file(mahdata->fd->ec,\
												mahdata->fd->el,mahdata->fd->softevent,mahdata->fd->markbit))
                									{
                									#ifdef MAJOR_DEBUG
                    									printf("generate file err,File: %s,Line: %d\n", \
																						__FILE__,__LINE__);
                									#endif
                									}
													*(mahdata->fd->markbit) |= 0x0800;
												}
											}
											else
											{
											#ifdef MAJOR_DEBUG
												printf("pipe cannot write,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											sendfaceInfoToBoard(mahdata->fd,fbdata);
											sleep(mt);
											if (pinfo->pdelay > pinfo->ldeti)
											{
												mt = pinfo->pdelay - pinfo->ldeti;
												sleep(mt);
												mt = pinfo->ldeti;
											}
											else
											{	
												mt = pinfo->pdelay;
											}
											mintime += pinfo->pdelay;
											//added on 20220620
                                        	adaptd.phi[ni].paddt += pinfo->pdelay;
                                        	//end add
											continue;
										}//not arrive max time;
									}//have vehicle pass the detector
									else
									{//not have vehicle pass the detector
										nowtime.tv_sec = 0;
										nowtime.tv_usec = 0;
										gettimeofday(&nowtime,NULL);
										leatime = nowtime.tv_sec - lasttime.tv_sec;
										mt -= leatime;
										continue;
									}//not have vehicle pass the detector
								}//if (num > 0)
								else
								{
									nowtime.tv_sec = 0;
									nowtime.tv_usec = 0;
									gettimeofday(&nowtime,NULL);
									leatime = nowtime.tv_sec - lasttime.tv_sec;
									mt -= leatime;
									continue;
								}//if (num <= 0)
							}//else if (FD_ISSET(*(mahdata->fd->flowpipe),&nRead))
							else
							{
								nowtime.tv_sec = 0;
								nowtime.tv_usec = 0;
								gettimeofday(&nowtime,NULL);
								leatime = nowtime.tv_sec - lasttime.tv_sec;
								mt -= leatime;
								continue;	
							}
						}//mret > 0
					}//inline while loop
				}//0 == ffw

				//end child thread of person channels 
            	if (1 == madcpcyes)
            	{
                	pthread_cancel(madcpcid);
                	pthread_join(madcpcid,NULL);
                	madcpcyes = 0;
            	}	
				//added on 20220620
        		adaptd.phi[ni].pfactt = adaptd.phi[ni].paddt + pinfo->gtime + pinfo->gftime + pinfo->ytime + pinfo->rtime;
        		//end add

				//Begin to green flash
				if ((0 != pinfo->cchan[0]) && (pinfo->gftime > 0))	
				{
					if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
                	{//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
                   		sinfo.time = pinfo->gftime;
						sinfo.conmode = *(mahdata->fd->contmode);//added on 20150529
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
						memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
                    	memset(sibuf,0,sizeof(sibuf));
                   		if (SUCCESS != status_info_report(sibuf,&sinfo))
                   		{
                   		#ifdef MAJOR_DEBUG
                       		printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                   		#endif
                   		}
						else
                   		{
                       		write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
                   		}
					}//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
				}

				memset(&gtime,0,sizeof(gtime));
            	memset(&gftime,0,sizeof(gftime));
				gettimeofday(&gftime,NULL);
            	memset(&ytime,0,sizeof(ytime));
            	memset(&rtime,0,sizeof(rtime));
				*(mahdata->fd->markbit) |= 0x0400;
				gft = 0;
				if (pinfo->gftime > 0)
				{
					while (1)
					{
						if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->cchan,0x03))
						{
						#ifdef MAJOR_DEBUG
							printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							gettimeofday(&ct,NULL);
							update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15, \
												ct.tv_sec,mahdata->fd->markbit);
							if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
														mahdata->fd->softevent,mahdata->fd->markbit))
							{
							#ifdef MAJOR_DEBUG
								printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}
							*(mahdata->fd->markbit) |= 0x0800;	
						}
						if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs, \
															pinfo->cchan,0x03,mahdata->fd->markbit))
						{
						#ifdef MAJOR_DEBUG
							printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
						timeout.tv_sec = 0;
						timeout.tv_usec = 500000;
						select(0,NULL,NULL,NULL,&timeout);
						
						if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->cchan,0x02))
						{
						#ifdef MAJOR_DEBUG
							printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(mahdata->fd->markbit) |= 0x0800;	
							gettimeofday(&ct,NULL);
							update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15, \
													ct.tv_sec,mahdata->fd->markbit);
							if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
														mahdata->fd->softevent,mahdata->fd->markbit))
							{
							#ifdef MAJOR_DEBUG
								printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}
						}
						if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs, \
														pinfo->cchan,0x02,mahdata->fd->markbit))
						{
						#ifdef MAJOR_DEBUG
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
				}//if (pinfo->gftime > 0)
				//End green flash
				if (1 == phcon)
                {
                    *(mahdata->fd->markbit) &= 0xfbff;
                    memset(&gtime,0,sizeof(gtime));
                    gettimeofday(&gtime,NULL);
                    memset(&gftime,0,sizeof(gftime));
                    memset(&ytime,0,sizeof(ytime));
                    memset(&rtime,0,sizeof(rtime));
                    phcon = 0;
                    sleep(10);
                }
				#ifdef RED_FLASH	
				if (0 == mahdata->td->timeconfig->TimeConfigList[tcline][slnum+1].StageId)
				{
					rft = (mahdata->td->timeconfig->TimeConfigList[tcline][0].SpecFunc & 0xe0) >> 5;
				}
				else
				{
					rft = (mahdata->td->timeconfig->TimeConfigList[tcline][slnum+1].SpecFunc & 0xe0) >> 5;
				}
				if (rft > 0)
				{
					redflash_mah		mah;
					mah.tcline = tcline;
					mah.slnum = slnum;
					mah.snum =	snum;
					mah.rft = rft;
					mah.chan = pinfo->chan;
					mah.mah = mahdata;
					int rfarg = pthread_create(&rfpid,NULL,(void *)mah_red_flash,&mah);
					if (0 != rfarg)
					{
					#ifdef MAJOR_DEBUG
						printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						return;
					}
				}//if (rft > 0) 
				#endif

				//Begin to yellow lamp
				if (1 == (pinfo->cpcexist))
				{
					if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->cpchan,0x00))
					{
					#ifdef MAJOR_DEBUG
						printf("set lamp color ser,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						gettimeofday(&ct,NULL);
                		update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15,ct.tv_sec,mahdata->fd->markbit);
                		if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
																mahdata->fd->softevent,mahdata->fd->markbit))
                		{
                		#ifdef MAJOR_DEBUG
                    		printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                		#endif
                		}
						*(mahdata->fd->markbit) |= 0x0800;
					}
					if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,pinfo->cpchan,0x00, \
                                                    mahdata->fd->markbit))
                    {
                    #ifdef MAJOR_DEBUG
                        printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                    }
					if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->cnpchan,0x01))
					{
					#ifdef MAJOR_DEBUG
						printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						gettimeofday(&ct,NULL);
                		update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15,ct.tv_sec,mahdata->fd->markbit);
                		if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
															mahdata->fd->softevent,mahdata->fd->markbit))
                		{
                		#ifdef MAJOR_DEBUG
                    		printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                		#endif
                		}
						*(mahdata->fd->markbit) |= 0x0800;
					}
					if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,pinfo->cnpchan,0x01, \
                                                    mahdata->fd->markbit))
                    {
                    #ifdef MAJOR_DEBUG
                        printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                    }	
				}
				else
				{
					if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->cchan,0x01))
                    {
                    #ifdef MAJOR_DEBUG
                        printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
						gettimeofday(&ct,NULL);
                		update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15,ct.tv_sec,mahdata->fd->markbit);
                		if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
															mahdata->fd->softevent,mahdata->fd->markbit))
                		{
                		#ifdef MAJOR_DEBUG
                    		printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                		#endif
                		}
                        *(mahdata->fd->markbit) |= 0x0800;
                    }
					if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,pinfo->cchan,0x01, \
                                                    mahdata->fd->markbit))
                    {
                    #ifdef MAJOR_DEBUG
                        printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                    }
				}
				*(mahdata->fd->color) = 0x01;
				memset(&gtime,0,sizeof(gtime));
            	memset(&gftime,0,sizeof(gftime));
            	memset(&ytime,0,sizeof(ytime));
				gettimeofday(&ytime,NULL);
            	memset(&rtime,0,sizeof(rtime));

				if ((0 != pinfo->cchan[0]) && (pinfo->ytime > 0))
				{
					if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
                	{//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
                   		sinfo.time = pinfo->ytime;
						sinfo.conmode = *(mahdata->fd->contmode);//added on 20150529
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
						memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
                    	memset(sibuf,0,sizeof(sibuf));
                   		if (SUCCESS != status_info_report(sibuf,&sinfo))
                   		{
                   		#ifdef MAJOR_DEBUG
                       		printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                   		#endif
                   		}
						else
                   		{
                       		write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
                   		}
					}//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
				}

				if ((*(mahdata->fd->markbit) & 0x0002) && (*(mahdata->fd->markbit) & 0x0010))
				{
					memset(&timedown,0,sizeof(timedown));
					timedown.mode = *(mahdata->fd->contmode);
					timedown.pattern = *(mahdata->fd->patternid);
					timedown.lampcolor = 0x01;
					timedown.lamptime = pinfo->ytime;
					timedown.phaseid = pinfo->phaseid;
					timedown.stageline = pinfo->stageline;
					if (SUCCESS != timedown_data_to_conftool(mahdata->fd->sockfd,&timedown))
					{
					#ifdef MAJOR_DEBUG
						printf("send time err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
				}
				#ifdef EMBED_CONFIGURE_TOOL
				if (*(mahdata->fd->markbit2) & 0x0200)
				{
					memset(&timedown,0,sizeof(timedown));
                    timedown.mode = *(mahdata->fd->contmode);
                    timedown.pattern = *(mahdata->fd->patternid);
                    timedown.lampcolor = 0x01;
                    timedown.lamptime = pinfo->ytime;
                    timedown.phaseid = pinfo->phaseid;
                    timedown.stageline = pinfo->stageline;	
					if (SUCCESS != timedown_data_to_embed(mahdata->fd->sockfd,&timedown))
					{
					#ifdef MAJOR_DEBUG
						printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
				}
				#endif
				if (*(mahdata->fd->contmode) < 27)
                	fbdata[1] = *(mahdata->fd->contmode) + 1;
            	else
                	fbdata[1] = *(mahdata->fd->contmode);
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
				if (!wait_write_serial(*(mahdata->fd->fbserial)))
				{
					if (write(*(mahdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
					{
					#ifdef MAJOR_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						gettimeofday(&ct,NULL);
                		update_event_list(mahdata->fd->ec,mahdata->fd->el,1,16,ct.tv_sec,mahdata->fd->markbit);
                		if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
															mahdata->fd->softevent,mahdata->fd->markbit))
                		{
                		#ifdef MAJOR_DEBUG
                    		printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                		#endif
                		}
						*(mahdata->fd->markbit) &= 0x0800;
					}
				}
				else
				{
				#ifdef MAJOR_DEBUG
					printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				sendfaceInfoToBoard(mahdata->fd,fbdata);
				sleep(pinfo->ytime);
				//End yellow lamp

				//Begin to red lamp
				if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->cchan,0x00))
				{
				#ifdef MAJOR_DEBUG
					printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					gettimeofday(&ct,NULL);
                	update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15,ct.tv_sec,mahdata->fd->markbit);
                	if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
														mahdata->fd->softevent,mahdata->fd->markbit))
                	{
                	#ifdef MAJOR_DEBUG
                    	printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                	}
					*(mahdata->fd->markbit) &= 0x0800;
				}
				if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,pinfo->cchan,0x00, \
                                                    mahdata->fd->markbit))
                {
                #ifdef MAJOR_DEBUG
                    printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
				*(mahdata->fd->color) = 0x00;
			
				if (0 != pinfo->cchan[0])	
				{
					if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
                	{//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
                   		sinfo.time = pinfo->rtime;
						sinfo.conmode = *(mahdata->fd->contmode);//added on 20150529
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
						memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
                    	memset(sibuf,0,sizeof(sibuf));
                   		if (SUCCESS != status_info_report(sibuf,&sinfo))
                   		{
                   		#ifdef MAJOR_DEBUG
                       		printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                   		#endif
                   		}
						else
                   		{
                       		write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
                   		}
					}//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))				
				}

				if ((pinfo->rtime) > 0)
				{
					if ((*(mahdata->fd->markbit) & 0x0002) && (*(mahdata->fd->markbit) & 0x0010))
					{
						memset(&timedown,0,sizeof(timedown));
						timedown.mode = *(mahdata->fd->contmode);	
						timedown.pattern = *(mahdata->fd->patternid);
						timedown.lampcolor = 0x00;
						timedown.lamptime = pinfo->rtime;
						timedown.phaseid = pinfo->phaseid;
						timedown.stageline = pinfo->stageline;
						if (SUCCESS != timedown_data_to_conftool(mahdata->fd->sockfd,&timedown))
						{
						#ifdef MAJOR_DEBUG
							printf("send time err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
					}
					#ifdef EMBED_CONFIGURE_TOOL
					if (*(mahdata->fd->markbit2) & 0x0200)
					{
						memset(&timedown,0,sizeof(timedown));
                        timedown.mode = *(mahdata->fd->contmode);
                        timedown.pattern = *(mahdata->fd->patternid);
                        timedown.lampcolor = 0x00;
                        timedown.lamptime = pinfo->rtime;
                        timedown.phaseid = pinfo->phaseid;
                        timedown.stageline = pinfo->stageline;	
						if (SUCCESS != timedown_data_to_embed(mahdata->fd->sockfd,&timedown))
						{
						#ifdef MAJOR_DEBUG
							printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
					}
					#endif
					if (*(mahdata->fd->contmode) < 27)
                		fbdata[1] = *(mahdata->fd->contmode) + 1;
            		else
                		fbdata[1] = *(mahdata->fd->contmode);
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
					if (!wait_write_serial(*(mahdata->fd->fbserial)))
					{
						if (write(*(mahdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
						{
						#ifdef MAJOR_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							gettimeofday(&ct,NULL);
                			update_event_list(mahdata->fd->ec,mahdata->fd->el,1,16,ct.tv_sec, \
											mahdata->fd->markbit);
                			if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
																mahdata->fd->softevent,mahdata->fd->markbit))
                			{
                			#ifdef MAJOR_DEBUG
                    			printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                			#endif
                			}
							*(mahdata->fd->markbit) |= 0x0800;
						}
					}
					else
					{
					#ifdef MAJOR_DEBUG
						printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					sendfaceInfoToBoard(mahdata->fd,fbdata);
					sleep(pinfo->rtime);
				}
				//End red lamp

				if (1 == phcon)
            	{
					*(mahdata->fd->markbit) &= 0xfbff;
                	memset(&gtime,0,sizeof(gtime));
                	gettimeofday(&gtime,NULL);
                	memset(&gftime,0,sizeof(gftime));
                	memset(&ytime,0,sizeof(ytime));
                	memset(&rtime,0,sizeof(rtime));
                	phcon = 0;
                	sleep(10);
            	}

				ni += 1;//20220621
				slnum += 1;
				*(mahdata->fd->slnum) = slnum;
				*(mahdata->fd->stageid) = mahdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
				if (0 == (mahdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId))
				{
					cycarr = 1;
					slnum = 0;
					*(mahdata->fd->slnum) = 0;
					*(mahdata->fd->stageid) = mahdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
				}
				#ifdef RED_FLASH
				if (rft > (pinfo->ytime + pinfo->rtime))
				{
					sleep(rft - (pinfo->ytime) - (pinfo->rtime));
				}
				#endif

				continue;
			}//if (1 == validmark)
		}//flexible phase
		else if (0x80 == (pinfo->phasetype))
		{//fix phase
		#ifdef MAJOR_DEBUG
            printf("Begin to pass fix phase,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
			*(mahdata->fd->color) = 0x02;
			*(mahdata->fd->markbit) &= 0xfbff;
			if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->chan,0x02))
			{
			#ifdef MAJOR_DEBUG
				printf("set green err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				gettimeofday(&ct,NULL);
                update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15,ct.tv_sec,mahdata->fd->markbit);
                if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
													mahdata->fd->softevent,mahdata->fd->markbit))
                {
                #ifdef MAJOR_DEBUG
                    printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
				*(mahdata->fd->markbit) |= 0x0800;
			}
			//added on 20220620
			adaptd.phi[ni].paddt = 0;
            adaptd.phi[ni].pfactt = pinfo->fixgtime + pinfo->gftime + pinfo->ytime + pinfo->rtime;
            //end add

			#ifdef CHANNEL_YELLOW_FLASH
			cyfn = 0;
			cyfe = 0;
			for (cyfn = 0; cyfn < mahdata->td->phaseerror->FactPhaseErrorNum; cyfn++)
			{		
				if (pinfo->phaseid	== mahdata->td->phaseerror->PhaseErrorList[cyfn].PhaseId)
				{
					cyfe = /*0xfffe1fff &*/ mahdata->td->phaseerror->PhaseErrorList[cyfn].PhaseConflict;
					break;
				}
			}
			memset(cyfc,0,sizeof(cyfc));
			pcyfc = cyfc;
			if (cyfe > 0)
			{//exist channel yellow flash
				for (cyfn = 0; cyfn < YFCHANNEL; cyfn++)
				{
					if (cyfe & (0x01 << cyfn))
					{
						*pcyfc = cyfn + 1;
						pcyfc++;
					}
				}
				cyft.cyft = pinfo->gtime + pinfo->gftime + pinfo->ytime + pinfo->rtime -1;//for thread exit
				cyft.chan = cyfc;
				cyft.mah = mahdata;
				cyft.mark = 1;
				int yfarg = pthread_create(&yfcpid,NULL,(void *)mah_channel_yellow_flash,&cyft);
				if (0 != yfarg)
				{
				#ifdef TIMING_DEBUG
					printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					return;
				}
				else
				{//create channel yellow flash thread success;
				#ifdef TIMING_DEBUG
					printf("Create channel yellow flash thread success,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
					{//actively report is not probitted and connect successfully
						sinfo.time = pinfo->gtime + pinfo->gftime + pinfo->ytime + pinfo->rtime - 1;
						sinfo.conmode = *(mahdata->fd->contmode);
						sinfo.color = 0x05;//yellow flash
						sinfo.chans = 0;
						memset(sinfo.csta,0,sizeof(sinfo.csta));
						csta = sinfo.csta;
						for (i = 0; i < YFCHANNEL; i++)
						{
							if (0 == cyfc[i])
								break;
							sinfo.chans += 1;
							tcsta = cyfc[i];
							tcsta <<= 2;
							tcsta &= 0xfc;
							tcsta |= 0x00;
							*csta = tcsta;
							csta++;
						}
						memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
						memset(sibuf,0,sizeof(sibuf));
						if (SUCCESS != status_info_report(sibuf,&sinfo))	
						{
						#ifdef TIMING_DEBUG
							printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
						else
						{
							write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
						}
					}//actively report is not probitted and connect successfully
				}//create channel yellow flash thread success;
			}//exist channel yellow flash
			#endif

			#ifdef CHANNEL_CLOSE
			cyfn = 0;
			cyfe = 0;
			for (cyfn = 0; cyfn < mahdata->td->phaseerror->FactPhaseErrorNum; cyfn++)
			{
				if (pinfo->phaseid  == mahdata->td->phaseerror->PhaseErrorList[cyfn].PhaseId)
				{
					cyfe = /*0xfffe1fff &*/ mahdata->td->phaseerror->PhaseErrorList[cyfn].PhaseConflict;
					break;
				}
			}
			memset(cyfc,0,sizeof(cyfc));
			pcyfc = cyfc;
			if (cyfe > 0)
			{//exist channel close lamp
				for (cyfn = 0; cyfn < YFCHANNEL; cyfn++)
				{
					if (cyfe & (0x01 << cyfn))
					{
						*pcyfc = cyfn + 1;
						pcyfc++;
					}
				}
				cyft.cyft = pinfo->gtime + pinfo->gftime + pinfo->ytime + pinfo->rtime;
				cyft.chan = cyfc;
				cyft.mah = mahdata;
				cyft.mark = 1;

				if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),cyfc,0x03))
				{
				#ifdef TIMING_DEBUG
					printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,cyfc,0x03,mahdata->fd->markbit))
				{
				#ifdef TIMING_DEBUG
					printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				channel_close_begin_report(mahdata->fd->sockfd->csockfd,cyfc);
			}//exist channel close lamp
			#endif

			memset(&gtime,0,sizeof(gtime));
            gettimeofday(&gtime,NULL);
            memset(&gftime,0,sizeof(gftime));
            memset(&ytime,0,sizeof(ytime));
            memset(&rtime,0,sizeof(rtime));
			if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,pinfo->chan,0x02,mahdata->fd->markbit))
			{
			#ifdef MAJOR_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
			if (0 == pinfo->cchan[0])
			{
				if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
				{//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
					sinfo.time = pinfo->fixgtime + pinfo->gftime + pinfo->ytime + pinfo->rtime;	
					sinfo.conmode = *(mahdata->fd->contmode);//added on 20150529
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
					memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            		memset(sibuf,0,sizeof(sibuf));
            		if (SUCCESS != status_info_report(sibuf,&sinfo))
            		{
            		#ifdef MAJOR_DEBUG
                		printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            		#endif
            		}
            		else
            		{
                		write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            		}	
				}//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
			}
			else
			{
				if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
				{//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
					sinfo.time = pinfo->fixgtime + pinfo->gftime;	
					sinfo.color = 0x02;
					sinfo.conmode = *(mahdata->fd->contmode);//added on 20150529
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
					memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            		memset(sibuf,0,sizeof(sibuf));
            		if (SUCCESS != status_info_report(sibuf,&sinfo))
            		{
            		#ifdef MAJOR_DEBUG
                		printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            		#endif
            		}
            		else
            		{
                		write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            		}	
				}//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
			}

			if ((*(mahdata->fd->markbit) & 0x0002) && (*(mahdata->fd->markbit) & 0x0010))
			{
				memset(&timedown,0,sizeof(timedown));
				timedown.mode = *(mahdata->fd->contmode);
				timedown.pattern = *(mahdata->fd->patternid);
				timedown.lampcolor = 0x02;
				timedown.lamptime = pinfo->fixgtime + pinfo->gftime;
				timedown.phaseid = pinfo->phaseid;
				timedown.stageline = pinfo->stageline;
				if (SUCCESS != timedown_data_to_conftool(mahdata->fd->sockfd,&timedown))
				{
				#ifdef MAJOR_DEBUG
					printf("send time err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#ifdef EMBED_CONFIGURE_TOOL
			if (*(mahdata->fd->markbit2) & 0x0200)
			{
				memset(&timedown,0,sizeof(timedown));
                timedown.mode = *(mahdata->fd->contmode);
                timedown.pattern = *(mahdata->fd->patternid);
                timedown.lampcolor = 0x02;
                timedown.lamptime = pinfo->fixgtime + pinfo->gftime;
                timedown.phaseid = pinfo->phaseid;
                timedown.stageline = pinfo->stageline;	
				if (SUCCESS != timedown_data_to_embed(mahdata->fd->sockfd,&timedown))
				{
				#ifdef MAJOR_DEBUG
					printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#endif
			//send info to face board
			if (*(mahdata->fd->contmode) < 27)
                fbdata[1] = *(mahdata->fd->contmode) + 1;
            else
                fbdata[1] = *(mahdata->fd->contmode);
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
				fbdata[4] = pinfo->fixgtime + pinfo->gftime;
			}
			if (!wait_write_serial(*(mahdata->fd->fbserial)))
			{
				if (write(*(mahdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
				{
				#ifdef MAJOR_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					gettimeofday(&ct,NULL);
                	update_event_list(mahdata->fd->ec,mahdata->fd->el,1,16,ct.tv_sec,mahdata->fd->markbit);
                	if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
														mahdata->fd->softevent,mahdata->fd->markbit))
                	{
                	#ifdef MAJOR_DEBUG
                    	printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                	}
					*(mahdata->fd->markbit) |= 0x0800;
				}
			}
			else
			{
			#ifdef MAJOR_DEBUG
				printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
			sendfaceInfoToBoard(mahdata->fd,fbdata);
			if ((1 == (pinfo->cpcexist)) && ((pinfo->pctime) > 0))
			{//exist person channels
		//		sleep(pinfo->fixgtime - pinfo->pctime);
				sltime = pinfo->fixgtime - pinfo->pctime;
				ffw = 0;
				while (1)
				{//while loop
					FD_ZERO(&nRead);
					FD_SET(*(mahdata->fd->ffpipe),&nRead);
					lasttime.tv_sec = 0;
					lasttime.tv_usec = 0;
					gettimeofday(&lasttime,NULL);
					bakv = sltime;
					mtime.tv_sec = sltime;
					mtime.tv_usec = 0;
					int mret = select(*(mahdata->fd->ffpipe)+1,&nRead,NULL,NULL,&mtime);
					if (mret < 0)
					{
					#ifdef MAJOR_DEBUG
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
						if (FD_ISSET(*(mahdata->fd->ffpipe),&nRead))
						{
							memset(buf,0,sizeof(buf));
							read(*(mahdata->fd->ffpipe),buf,sizeof(buf));
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
				}//while loop
				
				if (0 == ffw)
				{
					if (0 == madcpcyes)
					{
						memset(&mahpcdata,0,sizeof(mahpcdata));
						mahpcdata.bbserial = mahdata->fd->bbserial;
						mahpcdata.pchan = pinfo->cpchan;
						mahpcdata.markbit = mahdata->fd->markbit;
						mahpcdata.sockfd = mahdata->fd->sockfd;
						mahpcdata.cs = mahdata->cs;
						mahpcdata.time = pinfo->pctime;
						int pcret=pthread_create(&madcpcid,NULL,(void *)madc_person_chan_greenflash,&mahpcdata);
						if (0 != pcret)	
						{
						#ifdef MAJOR_DEBUG
							printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
							output_log("major half detect,create person green flash err");
						#endif
							return;
						}
						madcpcyes = 1;
					}
					if (0 == pinfo->cchan[0])
					{
						if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
						{//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
							sinfo.time = pinfo->pctime + pinfo->gftime + pinfo->ytime + pinfo->rtime;	
							sinfo.conmode = *(mahdata->fd->contmode);//added on 20150529
							sinfo.color = 0x02;
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
							memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
							memset(sibuf,0,sizeof(sibuf));
            				if (SUCCESS != status_info_report(sibuf,&sinfo))
            				{
            				#ifdef MAJOR_DEBUG
               					printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            				#endif
            				}
            				else
            				{
               					write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            				}	
						}//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))	
					}
					else
					{
						if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
						{//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
							sinfo.time = pinfo->pctime + pinfo->gftime;	
							sinfo.conmode = *(mahdata->fd->contmode);//added on 20150529
							sinfo.color = 0x02;
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
							memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
							memset(sibuf,0,sizeof(sibuf));
            				if (SUCCESS != status_info_report(sibuf,&sinfo))
            				{
            				#ifdef MAJOR_DEBUG
               					printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            				#endif
            				}
            				else
            				{
               					write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            				}	
						}//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
					}

			//		sleep(pinfo->pctime);
					sltime = pinfo->pctime;
					while (1)
					{//while loop
						FD_ZERO(&nRead);
						FD_SET(*(mahdata->fd->ffpipe),&nRead);
						lasttime.tv_sec = 0;
						lasttime.tv_usec = 0;
						gettimeofday(&lasttime,NULL);
						bakv = sltime;
						mtime.tv_sec = sltime;
						mtime.tv_usec = 0;
						int mret = select(*(mahdata->fd->ffpipe)+1,&nRead,NULL,NULL,&mtime);
						if (mret < 0)
						{
						#ifdef MAJOR_DEBUG
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
							if (FD_ISSET(*(mahdata->fd->ffpipe),&nRead))
							{
								memset(buf,0,sizeof(buf));
								read(*(mahdata->fd->ffpipe),buf,sizeof(buf));
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
					}//while loop

					if (1 == madcpcyes)
					{
						pthread_cancel(madcpcid);
						pthread_join(madcpcid,NULL);
						madcpcyes = 0;
					}
				}//0 == ffw
			}//exist person channels
			else
			{//not exist person channels
		//		sleep(pinfo->fixgtime);
				sltime = pinfo->fixgtime;
				while (1)
				{//while loop
					FD_ZERO(&nRead);
					FD_SET(*(mahdata->fd->ffpipe),&nRead);
					lasttime.tv_sec = 0;
					lasttime.tv_usec = 0;
					gettimeofday(&lasttime,NULL);
					bakv = sltime;
					mtime.tv_sec = sltime;
					mtime.tv_usec = 0;
					int mret = select(*(mahdata->fd->ffpipe)+1,&nRead,NULL,NULL,&mtime);
					if (mret < 0)
					{
					#ifdef MAJOR_DEBUG
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
						if (FD_ISSET(*(mahdata->fd->ffpipe),&nRead))
						{
							memset(buf,0,sizeof(buf));
							read(*(mahdata->fd->ffpipe),buf,sizeof(buf));
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
				}//while loop
			}//not exist person channels

			//begin to green flash
			if ((0 != pinfo->cchan[0]) && (pinfo->gftime > 0))
			{
				if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
				{//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
					sinfo.time = pinfo->gftime;	
					sinfo.conmode = *(mahdata->fd->contmode);//added on 20150529
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
					memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
					memset(sibuf,0,sizeof(sibuf));
           			if (SUCCESS != status_info_report(sibuf,&sinfo))
           			{
           			#ifdef MAJOR_DEBUG
               			printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
           			#endif
           			}
           			else
           			{
               			write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
           			}	
				}//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
			}

			memset(&gtime,0,sizeof(gtime));
            memset(&gftime,0,sizeof(gftime));
			gettimeofday(&gftime,NULL);
            memset(&ytime,0,sizeof(ytime));
            memset(&rtime,0,sizeof(rtime));
			*(mahdata->fd->markbit) |= 0x0400;
			gft = 0;
			if (pinfo->gftime > 0)
			{
				while (1)
				{
					if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->cchan,0x03))
					{
					#ifdef MAJOR_DEBUG
						printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						gettimeofday(&ct,NULL);
						update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15,ct.tv_sec,mahdata->fd->markbit);
						if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
																mahdata->fd->softevent,mahdata->fd->markbit))
						{
						#ifdef MAJOR_DEBUG
							printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
						*(mahdata->fd->markbit) |= 0x0800;
					}
					if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,pinfo->cchan,0x03, \
														mahdata->fd->markbit))
					{
					#ifdef MAJOR_DEBUG
						printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					timeout.tv_sec = 0;
					timeout.tv_usec = 500000;
					select(0,NULL,NULL,NULL,&timeout);

					if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->cchan,0x02))
					{
					#ifdef MAJOR_DEBUG
						printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						gettimeofday(&ct,NULL);
						update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15,ct.tv_sec,mahdata->fd->markbit);
						if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
															mahdata->fd->softevent,mahdata->fd->markbit))
						{
						#ifdef MAJOR_DEBUG
							printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
						*(mahdata->fd->markbit) |= 0x0800;
					}
					if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,pinfo->cchan,0x02, \
														mahdata->fd->markbit))
					{
					#ifdef MAJOR_DEBUG
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
			}//if (pinfo->gftime > 0)
			//end green flash

			if (1 == phcon)
            {
                *(mahdata->fd->markbit) &= 0xfbff;
                memset(&gtime,0,sizeof(gtime));
                gettimeofday(&gtime,NULL);
                memset(&gftime,0,sizeof(gftime));
                memset(&ytime,0,sizeof(ytime));
                memset(&rtime,0,sizeof(rtime));
                phcon = 0;
                sleep(10);
            }
			#ifdef RED_FLASH	
			if (0 == mahdata->td->timeconfig->TimeConfigList[tcline][slnum+1].StageId)
			{
				rft = (mahdata->td->timeconfig->TimeConfigList[tcline][0].SpecFunc & 0xe0) >> 5;
			}
			else
			{
				rft = (mahdata->td->timeconfig->TimeConfigList[tcline][slnum+1].SpecFunc & 0xe0) >> 5;
			}
			if (rft > 0)
			{
				redflash_mah		mah;
				mah.tcline = tcline;
				mah.slnum = slnum;
				mah.snum =	snum;
				mah.rft = rft;
				mah.chan = pinfo->chan;
				mah.mah = mahdata;
				int rfarg = pthread_create(&rfpid,NULL,(void *)mah_red_flash,&mah);
				if (0 != rfarg)
				{
				#ifdef MAJOR_DEBUG
					printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					return;
				}
			}//if (rft > 0) 
			#endif

			//Begin to set yellow lamp
			if (1 == (pinfo->cpcexist))
			{
				if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->cpchan,0x00))
				{
				#ifdef MAJOR_DEBUG
					printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					gettimeofday(&ct,NULL);
                	update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15,ct.tv_sec,mahdata->fd->markbit);
                	if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
														mahdata->fd->softevent,mahdata->fd->markbit))
                	{
                	#ifdef MAJOR_DEBUG
                    	printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                	}
					*(mahdata->fd->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,pinfo->cpchan,0x00, \
                                                    mahdata->fd->markbit))
                {
                #ifdef MAJOR_DEBUG
                    printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
				if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->cnpchan,0x01))
				{
				#ifdef MAJOR_DEBUG
					printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					gettimeofday(&ct,NULL);
                	update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15,ct.tv_sec,mahdata->fd->markbit);
                	if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
														mahdata->fd->softevent,mahdata->fd->markbit))
                	{
                	#ifdef MAJOR_DEBUG
                    	printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                	}
					*(mahdata->fd->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,pinfo->cnpchan,0x01, \
                                                    mahdata->fd->markbit))
                {
                #ifdef MAJOR_DEBUG
                    printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
			}
			else
			{
				if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->cchan,0x01))
				{
				#ifdef MAJOR_DEBUG
					printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mahdata->fd->markbit) |= 0x0800;
					gettimeofday(&ct,NULL);
                	update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15,ct.tv_sec,mahdata->fd->markbit);
                	if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
													mahdata->fd->softevent,mahdata->fd->markbit))
                	{
                	#ifdef MAJOR_DEBUG
                    	printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                	}
				}
				if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,pinfo->cchan,0x01, \
                                                    mahdata->fd->markbit))
                {
                #ifdef MAJOR_DEBUG
                    printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
			}

			if ((0 != pinfo->cchan[0]) && (pinfo->ytime > 0))
			{
				if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
				{//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
					sinfo.time = pinfo->ytime;	
					sinfo.conmode = *(mahdata->fd->contmode);//added on 20150529
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
					memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
					memset(sibuf,0,sizeof(sibuf));
           			if (SUCCESS != status_info_report(sibuf,&sinfo))
           			{
           			#ifdef MAJOR_DEBUG
               			printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
           			#endif
           			}
           			else
           			{
               			write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
           			}	
				}//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
			}

			*(mahdata->fd->color) = 0x01;
			memset(&gtime,0,sizeof(gtime));
            memset(&gftime,0,sizeof(gftime));
            memset(&ytime,0,sizeof(ytime));
			gettimeofday(&ytime,NULL);
            memset(&rtime,0,sizeof(rtime));
			if ((*(mahdata->fd->markbit) & 0x0002) && (*(mahdata->fd->markbit) & 0x0010))
			{
				memset(&timedown,0,sizeof(timedown));
				timedown.mode = *(mahdata->fd->contmode);
				timedown.pattern = *(mahdata->fd->patternid);
				timedown.lampcolor = 0x01;
				timedown.lamptime = pinfo->ytime;
				timedown.phaseid = pinfo->phaseid;
				timedown.stageline = pinfo->stageline;
				if (SUCCESS != timedown_data_to_conftool(mahdata->fd->sockfd,&timedown))
				{
				#ifdef MAJOR_DEBUG
					printf("send time err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#ifdef EMBED_CONFIGURE_TOOL
			if (*(mahdata->fd->markbit2) & 0x0200)
			{
				memset(&timedown,0,sizeof(timedown));
                timedown.mode = *(mahdata->fd->contmode);
                timedown.pattern = *(mahdata->fd->patternid);
                timedown.lampcolor = 0x01;
                timedown.lamptime = pinfo->ytime;
                timedown.phaseid = pinfo->phaseid;
                timedown.stageline = pinfo->stageline;	
				if (SUCCESS != timedown_data_to_embed(mahdata->fd->sockfd,&timedown))
				{
				#ifdef MAJOR_DEBUG
					printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#endif
			if (*(mahdata->fd->contmode) < 27)
                fbdata[1] = *(mahdata->fd->contmode) + 1;
            else
                fbdata[1] = *(mahdata->fd->contmode);
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
			if (!wait_write_serial(*(mahdata->fd->fbserial)))
			{
				if (write(*(mahdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
				{
				#ifdef MAJOR_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mahdata->fd->markbit) |= 0x0800;
					gettimeofday(&ct,NULL);
                	update_event_list(mahdata->fd->ec,mahdata->fd->el,1,16,ct.tv_sec,mahdata->fd->markbit);
                	if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
												mahdata->fd->softevent,mahdata->fd->markbit))
                	{
                	#ifdef MAJOR_DEBUG
                    	printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                	}
				}
			}
			else
			{
			#ifdef MAJOR_DEBUG
				printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
			sendfaceInfoToBoard(mahdata->fd,fbdata);
			sleep(pinfo->ytime);
			//End set yellow lamp
			//Begin to set red lamp
			if (SUCCESS != madc_set_lamp_color(*(mahdata->fd->bbserial),pinfo->cchan,0x00))
			{
			#ifdef MAJOR_DEBUG
				printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mahdata->fd->markbit) |= 0x0800;
				gettimeofday(&ct,NULL);
                update_event_list(mahdata->fd->ec,mahdata->fd->el,1,15,ct.tv_sec,mahdata->fd->markbit);
                if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
											mahdata->fd->softevent,mahdata->fd->markbit))
                {
                #ifdef MAJOR_DEBUG
                    printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
			}
			if (SUCCESS != update_channel_status(mahdata->fd->sockfd,mahdata->cs,pinfo->cchan,0x00, \
                                                mahdata->fd->markbit))
            {
            #ifdef MAJOR_DEBUG
                printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }

			if (0 != pinfo->cchan[0])
			{
				if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
				{//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
					sinfo.time = pinfo->rtime;	
					sinfo.conmode = *(mahdata->fd->contmode);//added on 20150529
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
					memcpy(mahdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));		
					memset(sibuf,0,sizeof(sibuf));
            		if (SUCCESS != status_info_report(sibuf,&sinfo))
            		{
            		#ifdef MAJOR_DEBUG
               			printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            		#endif
            		}
            		else
            		{
               			write(*(mahdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            		}	
				}//if ((!(*(mahdata->fd->markbit) & 0x1000)) && (!(*(mahdata->fd->markbit) & 0x8000)))
			}

			*(mahdata->fd->color) = 0x00;
			if (pinfo->rtime > 0)
			{
				memset(&gtime,0,sizeof(gtime));
            	memset(&gftime,0,sizeof(gftime));
            	memset(&ytime,0,sizeof(ytime));
            	memset(&rtime,0,sizeof(rtime));
				gettimeofday(&rtime,NULL);
				if ((*(mahdata->fd->markbit) & 0x0002) && (*(mahdata->fd->markbit) & 0x0010))
				{
					memset(&timedown,0,sizeof(timedown));
					timedown.mode = *(mahdata->fd->contmode);
					timedown.pattern = *(mahdata->fd->patternid);
					timedown.lampcolor = 0x00;
					timedown.lamptime = pinfo->rtime;
					timedown.phaseid = pinfo->phaseid;
					timedown.stageline = pinfo->stageline;
					if (SUCCESS != timedown_data_to_conftool(mahdata->fd->sockfd,&timedown))
					{
					#ifdef MAJOR_DEBUG
						printf("send time err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
				}
				#ifdef EMBED_CONFIGURE_TOOL
				if (*(mahdata->fd->markbit2) & 0x0200)
				{
					memset(&timedown,0,sizeof(timedown));
                    timedown.mode = *(mahdata->fd->contmode);
                    timedown.pattern = *(mahdata->fd->patternid);
                    timedown.lampcolor = 0x00;
                    timedown.lamptime = pinfo->rtime;
                    timedown.phaseid = pinfo->phaseid;
                    timedown.stageline = pinfo->stageline;	
					if (SUCCESS != timedown_data_to_embed(mahdata->fd->sockfd,&timedown))
					{
					#ifdef MAJOR_DEBUG
						printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
				}
				#endif
				//send info to face board
				if (*(mahdata->fd->contmode) < 27)
                	fbdata[1] = *(mahdata->fd->contmode) + 1;
            	else
                	fbdata[1] = *(mahdata->fd->contmode);
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
				if (!wait_write_serial(*(mahdata->fd->fbserial)))
				{
					if (write(*(mahdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
					{
					#ifdef MAJOR_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						*(mahdata->fd->markbit) |= 0x0800;
						gettimeofday(&ct,NULL);
                		update_event_list(mahdata->fd->ec,mahdata->fd->el,1,16,ct.tv_sec,mahdata->fd->markbit);
                		if (SUCCESS != generate_event_file(mahdata->fd->ec,mahdata->fd->el,\
														mahdata->fd->softevent,mahdata->fd->markbit))
                		{
                		#ifdef MAJOR_DEBUG
                    		printf("generate file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                		#endif
                		}
					}
				}
				else
				{
				#ifdef MAJOR_DEBUG
					printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				sendfaceInfoToBoard(mahdata->fd,fbdata);
				sleep(pinfo->rtime);
			}//if (pinfo->rtime > 0)
			ni += 1;//20220621
			slnum += 1;
			*(mahdata->fd->slnum) = slnum;
			*(mahdata->fd->stageid) = mahdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
			if (0 == (mahdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId))
			{
				cycarr = 1;
				slnum = 0;
				*(mahdata->fd->slnum) = 0;
				*(mahdata->fd->stageid) = mahdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
			}
			#ifdef RED_FLASH
            if (rft > (pinfo->ytime + pinfo->rtime))
            {
                sleep(rft - (pinfo->ytime) - (pinfo->rtime));
            }
            #endif
			if (1 == phcon)
            {
				*(mahdata->fd->markbit) &= 0xfbff;
                memset(&gtime,0,sizeof(gtime));
                gettimeofday(&gtime,NULL);
                memset(&gftime,0,sizeof(gftime));
                memset(&ytime,0,sizeof(ytime));
                memset(&rtime,0,sizeof(rtime));
                phcon = 0;
                sleep(10);
            }

			continue;
		}//fix phase
		else
		{
		#ifdef MAJOR_DEBUG
			printf("Not have fit phase,please reset pattern,File:%s,Line: %d\n",__FILE__,__LINE__);
		#endif
			cycarr = 1;
			sleep(1);
			continue;
		}	
	}//while loop

	pthread_exit(NULL);
}

/*color: 0x00 means red,0x01 means yellow,0x02 means green,0x12 means green flash*/
int get_madc_status(unsigned char *color,unsigned char *leatime)
{
	if ((NULL == color) || (NULL == leatime))
    {
    #ifdef MAJOR_DEBUG
        printf("pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
        output_log("Major detect control,pointer address is null");
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

int major_half_detect(fcdata_t *fcdata,tscdata_t *tscdata,ChannelStatus_t *chanstatus)
{
	if ((NULL == fcdata) && (NULL == tscdata) && (NULL == chanstatus))
	{
	#ifdef MAJOR_DEBUG
		printf("pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
		output_log("major half detect,pointer address is null");
	#endif
		return MEMERR;
	}
	//Initial static variable
	mahdyes = 0;
	rettl = 0;
	madcpcyes = 0;
	mayfyes = 0;
	memset(&gtime,0,sizeof(gtime));
    memset(&gftime,0,sizeof(gftime));
    memset(&ytime,0,sizeof(ytime));
    memset(&rtime,0,sizeof(rtime));
	memset(&sinfo,0,sizeof(statusinfo_t));
	phcon = 0;
	memset(fcdata->sinfo,0,sizeof(statusinfo_t));
	//End static variable

	unsigned char			contmode = *(fcdata->contmode); //save control mode
	mahdata_t				mahdata;
	mahpinfo_t				pinfo;
	if (0 == mahdyes)
	{
		memset(&mahdata,0,sizeof(mahdata_t));
		memset(&pinfo,0,sizeof(pinfo));
		mahdata.fd = fcdata;
		mahdata.td = tscdata;
		mahdata.pi = &pinfo;
		mahdata.cs = chanstatus;
		int ret = pthread_create(&mahdid,NULL,(void *)start_major_half_detect,&mahdata);	
		if (0 != ret)
		{
		#ifdef MAJOR_DEBUG
			printf("create thread error,File: %s,Line: %d\n",__FILE__,__LINE__);
			output_log("major half detect,create major half detect thread err");
		#endif
			return FAIL;
		}
		mahdyes = 1;
	}

	sleep(1);
	//begin to monitor control pipe;
	unsigned char			cktem = 0;
	unsigned char			kstep = 0;
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
	unsigned char			keylock = 0;
	unsigned char			wllock = 0;
	unsigned char			tcbuf[32] = {0};
	unsigned char			color = 3; //lamp is default closed;
	unsigned char			leatime = 0;
	unsigned char			madcred = 0; //'1' means lamp has been status of all red
	timedown_t				madctd;
	unsigned char			ngf = 0;
	struct timeval			to,ct;
//	struct timeval			wut;
	struct timeval			mont,ltime;
	yfdata_t				yfdata;//data of yellow flash
	yfdata_t				ardata;//data of all red
	unsigned char			fbdata[6] = {0};
	fbdata[0] = 0xC1;
	fbdata[5] = 0xED;
	unsigned char			madcecho[3] = {0};//send traffic control info to face board
	madcecho[0] = 0xCA;
	madcecho[2] = 0xED;
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

	unsigned char				wtlock = 0;
	struct timeval				wtltime;
	unsigned char				pantn = 0;
	unsigned char               dirc = 0; //direct control
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
	
	unsigned char	ccon[32] = {0};
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
	unsigned char               ma_phase = 0;
	while (1)
	{//while loop
		FD_ZERO(&nread);
		FD_SET(*(fcdata->conpipe),&nread);
		int max = *(fcdata->conpipe);
#if 0
		//for WuXi check
		wut.tv_sec = 0;
		wut.tv_usec = 0;
		gettimeofday(&wut,NULL);
		//end WuXi check
#endif
		wtltime.tv_sec = RFIDT;
		wtltime.tv_usec = 0;
		int cpret = select(max+1,&nread,NULL,NULL,&wtltime);
		if (cpret < 0)
		{
		#ifdef MAJOR_DEBUG
			printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
			output_log("major half detect,select call err");
		#endif
			return FAIL;
		}
		if (0 == cpret)
		{//time out
			if (*(fcdata->markbit2) & 0x0100)
                continue; //rfid is controlling

			*(fcdata->markbit2) &= 0xfffe;
			if (1 == wtlock)
            {//if (1 == wtlock)
             //   pantn += 1;
            //    if (3 == pantn)
            //    {//wireless terminal has disconnected with signaler machine;
				if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
				{
					memset(wlinfo,0,sizeof(wlinfo));
					gettimeofday(&ct,NULL);
					if (SUCCESS != wireless_info_report(wlinfo,ct.tv_sec,fcdata->wlmark,0x20))
					{
					#ifdef MAJOR_DEBUG
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
				madcred = 0;
				close = 0;
				fpc = 0;
				pantn = 0;
				cp = 0;

				if (1 == mayfyes)
				{
					pthread_cancel(mayfid);
					pthread_join(mayfid,NULL);
					mayfyes = 0;
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
							#ifdef MAJOR_DEBUG
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
							#ifdef MAJOR_DEBUG
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
						memset(&madctd,0,sizeof(madctd));
						madctd.mode = 28;//traffic control
						madctd.pattern = *(fcdata->patternid);
						madctd.lampcolor = 0x02;
						madctd.lamptime = pinfo.gftime;
						madctd.phaseid = 0;
						madctd.stageline = pinfo.stageline;
						if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
						{
						#ifdef MAJOR_DEBUG
							printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
					}
					#ifdef EMBED_CONFIGURE_TOOL
					if (*(fcdata->markbit2) & 0x0200)
					{
						memset(&madctd,0,sizeof(madctd));
                        madctd.mode = 28;//traffic control
                        madctd.pattern = *(fcdata->patternid);
                        madctd.lampcolor = 0x02;
                        madctd.lamptime = pinfo.gftime;
                        madctd.phaseid = 0;
                        madctd.stageline = pinfo.stageline;	
						if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
						{
						#ifdef MAJOR_DEBUG
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
						#ifdef MAJOR_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(fcdata->markbit) |= 0x0800;
							gettimeofday(&ct,NULL);
							update_event_list(fcdata->ec,fcdata->el,1,16, \
													ct.tv_sec,fcdata->markbit);
							if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
														fcdata->softevent,fcdata->markbit))
							{
							#ifdef MAJOR_DEBUG
								printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
						}
					}
					else
					{
					#ifdef MAJOR_DEBUG
						printf("face board serial port cannot write,Line:%d\n",__LINE__);
					#endif
					}
					sendfaceInfoToBoard(fcdata,fbdata);
					ngf = 0;
					if (pinfo.gftime > 0)
					{
						while (1)
						{
							if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
							{
							#ifdef MAJOR_DEBUG
								printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								*(fcdata->markbit) |= 0x0800;
							}
							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																	wcc,0x03,fcdata->markbit))
							{
							#ifdef MAJOR_DEBUG
								printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
							to.tv_sec = 0;
							to.tv_usec = 500000;
							select(0,NULL,NULL,NULL,&to);
							if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
							{
							#ifdef MAJOR_DEBUG
								printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
								*(fcdata->markbit) |= 0x0800;
							}
							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																		wcc,0x02,fcdata->markbit))
							{
							#ifdef MAJOR_DEBUG
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
						if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
						{
						#ifdef MAJOR_DEBUG
							printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(fcdata->markbit) |= 0x0800;
						}
						if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																wnpcc,0x01, fcdata->markbit))
						{
						#ifdef MAJOR_DEBUG
							printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
						if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
						{
						#ifdef MAJOR_DEBUG
							printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(fcdata->markbit) |= 0x0800;
						}
						if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																wpcc,0x00,fcdata->markbit))
						{
						#ifdef MAJOR_DEBUG
							printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
					}
					else
					{
						if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
						{
						#ifdef MAJOR_DEBUG
							printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(fcdata->markbit) |= 0x0800;
						}
						if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																wcc,0x01,fcdata->markbit))
						{
						#ifdef MAJOR_DEBUG
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
							#ifdef MAJOR_DEBUG
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
						memset(&madctd,0,sizeof(madctd));
						madctd.mode = 28;//traffic control
						madctd.pattern = *(fcdata->patternid);
						madctd.lampcolor = 0x01;
						madctd.lamptime = pinfo.ytime;
						madctd.phaseid = 0;
						madctd.stageline = pinfo.stageline;
						if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
						{
						#ifdef MAJOR_DEBUG
							printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
					}
					#ifdef EMBED_CONFIGURE_TOOL
					if (*(fcdata->markbit2) & 0x0200)
					{
						memset(&madctd,0,sizeof(madctd));
                        madctd.mode = 28;//traffic control
                        madctd.pattern = *(fcdata->patternid);
                        madctd.lampcolor = 0x01;
                        madctd.lamptime = pinfo.ytime;
                        madctd.phaseid = 0;
                        madctd.stageline = pinfo.stageline;	
						if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
						{
						#ifdef MAJOR_DEBUG
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
						#ifdef MAJOR_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(fcdata->markbit) |= 0x0800;
							gettimeofday(&ct,NULL);
							update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
							if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																	fcdata->softevent,fcdata->markbit))
							{	
							#ifdef MAJOR_DEBUG
								printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
						}
					}
					else
					{
					#ifdef MAJOR_DEBUG
						printf("face board serial port cannot write,Line:%d\n",__LINE__);
					#endif
					}
					sendfaceInfoToBoard(fcdata,fbdata);
					sleep(pinfo.ytime);

					//current phase begin to red lamp
					if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
					{
					#ifdef MAJOR_DEBUG
						printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
						*(fcdata->markbit) |= 0x0800;
					}
					if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																		wcc,0x00,fcdata->markbit))
					{
					#ifdef MAJOR_DEBUG
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
							#ifdef MAJOR_DEBUG
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
				if (0 == mahdyes)
				{
					memset(&mahdata,0,sizeof(mahdata_t));
					memset(&pinfo,0,sizeof(pinfo));
					mahdata.fd = fcdata;
					mahdata.td = tscdata;
					mahdata.pi = &pinfo;
					mahdata.cs = chanstatus;
					int ret = pthread_create(&mahdid,NULL,(void *)start_major_half_detect,&mahdata);	
					if (0 != ret)
					{
					#ifdef MAJOR_DEBUG
						printf("create thread error,File: %s,Line: %d\n",__FILE__,__LINE__);
						output_log("major half detect,create major half detect thread err");
					#endif
						return FAIL;
					}
					mahdyes = 1;
				}
				
				continue;
       //         }//wireless terminal has disconnected with signaler machine;
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
                        continue;
                    }//pant
                    if ((0 == wllock) && (0 == keylock) && (0 == netlock))
                    {//terminal control is valid only when wireless and key and net control is invalid;
						if (0x04 == tcbuf[3])
                        {//control function
                            if ((0x01 == tcbuf[4]) && (0 == wtlock))
                            {//control will happen
								get_madc_status(&color,&leatime);
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
										
										get_madc_status(&color,&leatime);
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
                                        #ifdef MAJOR_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))			

								wtlock = 1;
								madcred = 0;
								close = 0;
								fpc = 0;
								cp = 0;
								dirc = 0;
								madc_end_child_thread();//end main thread and its child thread

								*(fcdata->markbit2) |= 0x0004;
							#if 0
								new_all_red(&ardata);
								if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
        						{
        						#ifdef MAJOR_DEBUG
            						printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        						#endif
									*(fcdata->markbit) |= 0x0800;
        						}
							#endif
								*(fcdata->contmode) = 28;//wireless terminal control mode
								
								if (*(fcdata->auxfunc) & 0x01)
                                {//if (*(fcdata->auxfunc) & 0x01)
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),dcdownti,sizeof(dcdownti)) < 0)
                                        {
                                        #ifdef FULL_DETECT_DEBUG
                                            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef FULL_DETECT_DEBUG
                                        printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),dcedownti,sizeof(dcedownti)) < 0)
                                        {
                                        #ifdef FULL_DETECT_DEBUG
                                            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef FULL_DETECT_DEBUG
                                        printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (*(fcdata->auxfunc) & 0x01)

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
            						#ifdef MAJOR_DEBUG
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
                                    #ifdef MAJOR_DEBUG
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
									#ifdef MAJOR_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
                						update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                						if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																fcdata->softevent,fcdata->markbit))
                						{
                						#ifdef MAJOR_DEBUG
                   							printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                						#endif
                						}
									}
								}
								else
								{
								#ifdef MAJOR_DEBUG
									printf("face board serial port cannot write,Line:%d\n",__LINE__);
								#endif
								}
								sendfaceInfoToBoard(fcdata,fbdata);
								//send down time to configure tool
                            	if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            	{
								#ifdef MAJOR_DEBUG
									printf("send control info to configure tool,File: %s,Line: %d\n", \
											__FILE__,__LINE__);
								#endif
                                	memset(&madctd,0,sizeof(madctd));
                                	madctd.mode = 28;
                                	madctd.pattern = *(fcdata->patternid);
                                	madctd.lampcolor = 0x02;
                                	madctd.lamptime = 0;
                                	madctd.phaseid = pinfo.phaseid;
                                	madctd.stageline = pinfo.stageline;
                                	if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
                                	{
                                	#ifdef MAJOR_DEBUG
                                    	printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                	#endif
                                	}
                            	}
								#ifdef EMBED_CONFIGURE_TOOL
                                if (*(fcdata->markbit2) & 0x0200)
                                {
                                    memset(&madctd,0,sizeof(madctd));
                                    madctd.mode = 28;
                                    madctd.pattern = *(fcdata->patternid);
                                    madctd.lampcolor = 0x02;
                                    madctd.lamptime = 0;
                                    madctd.phaseid = pinfo.phaseid;
                                    madctd.stageline = pinfo.stageline;
                                    if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                                #endif
                            	if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    				pinfo.chan,0x02,fcdata->markbit))
                            	{
                            	#ifdef MAJOR_DEBUG
                                	printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            	#endif
                            	}
							
								continue;
                            }//control will happen
                            else if ((0x00 == tcbuf[4]) && (1 == wtlock))
                            {//cancel control
								wtlock = 0;
								madcred = 0;
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
                                        #ifdef MAJOR_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
								if (1 == mayfyes)
								{
									pthread_cancel(mayfid);
									pthread_join(mayfid,NULL);
									mayfyes = 0;
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
            								#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
										memset(&madctd,0,sizeof(madctd));
										madctd.mode = 28;//traffic control
										madctd.pattern = *(fcdata->patternid);
										madctd.lampcolor = 0x02;
										madctd.lamptime = pinfo.gftime;
										madctd.phaseid = 0;
										madctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 28;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x02;
                                        madctd.lamptime = pinfo.gftime;
                                        madctd.phaseid = 0;
                                        madctd.stageline = pinfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                               			#ifdef MAJOR_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef MAJOR_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									ngf = 0;
									if (pinfo.gftime > 0)
									{
										while (1)
										{
											if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef MAJOR_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x03,fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef MAJOR_DEBUG
												printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
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
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef MAJOR_DEBUG
            								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef MAJOR_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef MAJOR_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
                            			memset(&madctd,0,sizeof(madctd));
                            			madctd.mode = 28;//traffic control
                            			madctd.pattern = *(fcdata->patternid);
                            			madctd.lampcolor = 0x01;
                            			madctd.lamptime = pinfo.ytime;
                            			madctd.phaseid = 0;
                            			madctd.stageline = pinfo.stageline;
                            			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
                            			{
                            			#ifdef MAJOR_DEBUG
                               				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            			#endif
                            			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 28;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x01;
                                        madctd.lamptime = pinfo.ytime;
                                        madctd.phaseid = 0;
                                        madctd.stageline = pinfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                            			#ifdef MAJOR_DEBUG
                               				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                			{
                                			#ifdef MAJOR_DEBUG
                                   				printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef MAJOR_DEBUG
                                		printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
								if (0 == mahdyes)
								{
									memset(&mahdata,0,sizeof(mahdata_t));
									memset(&pinfo,0,sizeof(pinfo));
									mahdata.fd = fcdata;
									mahdata.td = tscdata;
									mahdata.pi = &pinfo;
									mahdata.cs = chanstatus;
									int ret = pthread_create(&mahdid,NULL, \
												(void *)start_major_half_detect,&mahdata);	
									if (0 != ret)
									{
									#ifdef MAJOR_DEBUG
										printf("create thread error,File: %s,Line: %d\n",__FILE__,__LINE__);
										output_log("major half detect,create major half detect thread err");
									#endif
										return FAIL;
									}
									mahdyes = 1;
								}
								if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
								{
									memset(wlinfo,0,sizeof(wlinfo));
                                    gettimeofday(&ct,NULL);
                                    if (SUCCESS != wireless_info_report(wlinfo,ct.tv_sec,fcdata->wlmark,0x06))
                                    {
                                    #ifdef MAJOR_DEBUG
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
                                        #ifdef MAJOR_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))

								if ((1 == madcred) || (1 == mayfyes) || (1 == close))
                            	{//if ((1 == tcred) || (1 == tcyfyes) || (1 == close))
									dirc = tcbuf[4];
									if ((dirc < 5) && (dirc > 0x0c))
                                    {
                                        continue;
                                    }
									fpc = 1;
									cp = 0;
                                	if (1 == mayfyes)
                                	{
                                    	pthread_cancel(mayfid);
                                    	pthread_join(mayfid,NULL);
                                    	mayfyes = 0;
                                	}
									if (1 != madcred)
									{
										new_all_red(&ardata);
									}
									madcred = 0;
                                    close = 0;
					//				#ifdef CLOSE_LAMP
									madc_set_lamp_color(*(fcdata->bbserial),clch,0x03);
									update_channel_status(fcdata->sockfd,chanstatus,clch,0x03,fcdata->markbit);
					//				#endif
                                	if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),dirch[dirc-5],0x02))
                                	{
                                	#ifdef MAJOR_DEBUG
                                    	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
                                    	*(fcdata->markbit) |= 0x0800;
                                	}
                                	if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                                        				dirch[dirc-5],0x02,fcdata->markbit))
                                	{
                                	#ifdef MAJOR_DEBUG
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
            							#ifdef MAJOR_DEBUG
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
                                    	#ifdef MAJOR_DEBUG
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
                            	}//if ((1 == madcred) || (1 == mayfyes) || (1 == close))
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
            								#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
										memset(&madctd,0,sizeof(madctd));
										madctd.mode = 28;//traffic control
										madctd.pattern = *(fcdata->patternid);
										madctd.lampcolor = 0x02;
										madctd.lamptime = pinfo.gftime;
										madctd.phaseid = cp;
										madctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 28;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x02;
                                        madctd.lamptime = pinfo.gftime;
                                        madctd.phaseid = cp;
                                        madctd.stageline = pinfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                               			#ifdef MAJOR_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef MAJOR_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									ngf = 0;
									if (pinfo.gftime > 0)
									{
										while (1)
										{
											if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef MAJOR_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x03,fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef MAJOR_DEBUG
												printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
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
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef MAJOR_DEBUG
            								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef MAJOR_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef MAJOR_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
                            			memset(&madctd,0,sizeof(madctd));
                            			madctd.mode = 28;//traffic control
                            			madctd.pattern = *(fcdata->patternid);
                            			madctd.lampcolor = 0x01;
                            			madctd.lamptime = pinfo.ytime;
                            			madctd.phaseid = cp;
                            			madctd.stageline = pinfo.stageline;
                            			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
                            			{
                            			#ifdef MAJOR_DEBUG
                               				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            			#endif
                            			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 28;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x01;
                                        madctd.lamptime = pinfo.ytime;
                                        madctd.phaseid = cp;
                                        madctd.stageline = pinfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                            			#ifdef MAJOR_DEBUG
                               				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                			{
                                			#ifdef MAJOR_DEBUG
                                   				printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef MAJOR_DEBUG
                                		printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (pinfo.rtime > 0))									
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),dirch[dirc-5],0x02))
                            		{
                            		#ifdef MAJOR_DEBUG
                               			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																		dirch[dirc-5],0x02,fcdata->markbit))
                            		{
                            		#ifdef MAJOR_DEBUG
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
            							#ifdef MAJOR_DEBUG
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
										memset(&madctd,0,sizeof(madctd));
										madctd.mode = 28;//traffic control
										madctd.pattern = *(fcdata->patternid);
										madctd.lampcolor = 0x02;
										madctd.lamptime = 0;
										madctd.phaseid = 0;
										madctd.stageline = 0;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 28;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x02;
                                        madctd.lamptime = 0;
                                        madctd.phaseid = 0;
                                        madctd.stageline = 0;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                               			#ifdef MAJOR_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef MAJOR_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
										memset(&madctd,0,sizeof(madctd));
										madctd.mode = 28;//traffic control
										madctd.pattern = *(fcdata->patternid);
										madctd.lampcolor = 0x02;
										madctd.lamptime = pinfo.gftime;
										madctd.phaseid = 0;
										madctd.stageline = 0;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 28;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x02;
                                        madctd.lamptime = pinfo.gftime;
                                        madctd.phaseid = 0;
                                        madctd.stageline = 0;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                               			#ifdef MAJOR_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef MAJOR_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									ngf = 0;
									if (pinfo.gftime > 0)
									{
										while (1)
										{
											if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef MAJOR_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x03,fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef MAJOR_DEBUG
												printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
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
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef MAJOR_DEBUG
            								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef MAJOR_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef MAJOR_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
                            			memset(&madctd,0,sizeof(madctd));
                            			madctd.mode = 28;//traffic control
                            			madctd.pattern = *(fcdata->patternid);
                            			madctd.lampcolor = 0x01;
                            			madctd.lamptime = pinfo.ytime;
                            			madctd.phaseid = 0;
                            			madctd.stageline = 0;
                            			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
                            			{
                            			#ifdef MAJOR_DEBUG
                               				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            			#endif
                            			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 28;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x01;
                                        madctd.lamptime = pinfo.ytime;
                                        madctd.phaseid = 0;
                                        madctd.stageline = 0;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                            			#ifdef MAJOR_DEBUG
                               				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                			{
                                			#ifdef MAJOR_DEBUG
                                   				printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef MAJOR_DEBUG
                                		printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}//report info to server actively
									}//if ((0 != wcc[0]) && (pinfo.rtime > 0))									
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pnc,0x02))
                            		{
                            		#ifdef MAJOR_DEBUG
                               			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			pnc,0x02,fcdata->markbit))
                            		{
                            		#ifdef MAJOR_DEBUG
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
            							#ifdef MAJOR_DEBUG
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
										memset(&madctd,0,sizeof(madctd));
										madctd.mode = 28;//traffic control
										madctd.pattern = *(fcdata->patternid);
										madctd.lampcolor = 0x02;
										madctd.lamptime = 0;
										madctd.phaseid = 0;
										madctd.stageline = 0;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 28;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x02;
                                        madctd.lamptime = 0;
                                        madctd.phaseid = 0;
                                        madctd.stageline = 0;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                               			#ifdef MAJOR_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef MAJOR_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
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
                                   	#ifdef MAJOR_DEBUG
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
                                        #ifdef MAJOR_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
								if ((1 == madcred) || (1 == mayfyes) || (1 == close))
								{//if ((1 == madcred) || (1 == mayfyes) || (1 == close))
									if (1 == mayfyes)
                                    {
                                        pthread_cancel(mayfid);
                                        pthread_join(mayfid,NULL);
                                        mayfyes = 0;
                                    }
                                    if (1 != madcred)
                                    {
                                        new_all_red(&ardata);
                                    }
                                    madcred = 0;
                                    close = 0;
					//				#ifdef CLOSE_LAMP
                                    madc_set_lamp_color(*(fcdata->bbserial),clch,0x03);
									update_channel_status(fcdata->sockfd,chanstatus,clch,0x03,fcdata->markbit);
                   //                 #endif	
									if ((dirc < 0x05) || (dirc > 0x0c))
									{//not have phase control
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                                		{
                                		#ifdef MAJOR_DEBUG
                                    		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
                                    		*(fcdata->markbit) |= 0x0800;
                                		}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    									pinfo.chan,0x02,fcdata->markbit))
                                		{
                                		#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
										if(SUCCESS!=madc_set_lamp_color(*(fcdata->bbserial),dirch[dirc-5],0x02))
                                		{
                                		#ifdef MAJOR_DEBUG
                                    		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
                                    		*(fcdata->markbit) |= 0x0800;
                                		}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    									dirch[dirc-5],0x02,fcdata->markbit))
                                		{
                                		#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
                                    	#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
										memset(&madctd,0,sizeof(madctd));
										madctd.mode = 28;//traffic control
										madctd.pattern = *(fcdata->patternid);
										madctd.lampcolor = 0x02;
										madctd.lamptime = pinfo.gftime;
										madctd.phaseid = pinfo.phaseid;
										madctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 28;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x02;
                                        madctd.lamptime = pinfo.gftime;
                                        madctd.phaseid = pinfo.phaseid;
                                        madctd.stageline = pinfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                                		#ifdef MAJOR_DEBUG
                                    		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                    		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                    		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                    		{
                                    		#ifdef MAJOR_DEBUG
                                        		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
                                    		}
                                		}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
                                		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
	
									ngf = 0;
									if (pinfo.gftime > 0)
									{
									while (1)
									{
										if (SUCCESS!=madc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x03))
        								{
        								#ifdef MAJOR_DEBUG
            								printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															pinfo.cchan,0x03,fcdata->markbit))
        								{
        								#ifdef MAJOR_DEBUG
            								printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
        								#endif
        								}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);

										if (SUCCESS!=madc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x02))
        								{
        								#ifdef MAJOR_DEBUG
            								printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    									pinfo.cchan,0x02,fcdata->markbit))
                                		{
                                		#ifdef MAJOR_DEBUG
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
										if(SUCCESS!=madc_set_lamp_color(*(fcdata->bbserial),pinfo.cnpchan,0x01))
        								{
        								#ifdef MAJOR_DEBUG
            								printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
													pinfo.cnpchan,0x01, fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if(SUCCESS!=madc_set_lamp_color(*(fcdata->bbserial),pinfo.cpchan,0x00))
										{
										#ifdef MAJOR_DEBUG
											printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																	pinfo.cpchan,0x00,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if(SUCCESS!=madc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x01))
										{
										#ifdef MAJOR_DEBUG
											printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														pinfo.cchan,0x01,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
                               			memset(&madctd,0,sizeof(madctd));
                               			madctd.mode = 28;//traffic control
                               			madctd.pattern = *(fcdata->patternid);
                               			madctd.lampcolor = 0x01;
                               			madctd.lamptime = pinfo.ytime;
                               			madctd.phaseid = pinfo.phaseid;
                               			madctd.stageline = pinfo.stageline;
                               			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
                               			{
                               			#ifdef MAJOR_DEBUG
                                   			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               			#endif
                               			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 28;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x01;
                                        madctd.lamptime = pinfo.ytime;
                                        madctd.phaseid = pinfo.phaseid;
                                        madctd.stageline = pinfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                               			#ifdef MAJOR_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef MAJOR_DEBUG
                                       			printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x00))
                            		{
                            		#ifdef MAJOR_DEBUG
                                		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															pinfo.cchan,0x00,fcdata->markbit))
									{
									#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
                                			memset(&madctd,0,sizeof(madctd));
                                			madctd.mode = 28;//traffic control
                                			madctd.pattern = *(fcdata->patternid);
                                			madctd.lampcolor = 0x00;
                                			madctd.lamptime = pinfo.rtime;
                                			madctd.phaseid = pinfo.phaseid;
                                			madctd.stageline = pinfo.stageline;
                                			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
                                			{
                                			#ifdef MAJOR_DEBUG
                                    			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                            			}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&madctd,0,sizeof(madctd));
                                            madctd.mode = 28;//traffic control
                                            madctd.pattern = *(fcdata->patternid);
                                            madctd.lampcolor = 0x00;
                                            madctd.lamptime = pinfo.rtime;
                                            madctd.phaseid = pinfo.phaseid;
                                            madctd.stageline = pinfo.stageline;	
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
											{
											#ifdef MAJOR_DEBUG
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
                                    		#ifdef MAJOR_DEBUG
                                        		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
                                        		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                        		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                        		{
                                        		#ifdef MAJOR_DEBUG
                                            		printf("genfile err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                        		#endif
                                        		}
                                    		}
                                		}
                                		else
                                		{
                                		#ifdef MAJOR_DEBUG
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
									memset(&pinfo,0,sizeof(pinfo));
        						if(SUCCESS!=madc_get_phase_info(fcdata,tscdata,rettl,*(fcdata->slnum),&pinfo))
        							{
        							#ifdef MAJOR_DEBUG
            							printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
										output_log("Timing control,get phase info err");
        							#endif
										madc_end_part_child_thread();
            							return FAIL;
        							}
        							*(fcdata->phaseid) = 0;
        							*(fcdata->phaseid) |= (0x01 << (pinfo.phaseid - 1));

									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                            		{
                            		#ifdef MAJOR_DEBUG
                                		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																pinfo.chan,0x02,fcdata->markbit))
                            		{
                            		#ifdef MAJOR_DEBUG
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
            							#ifdef MAJOR_DEBUG
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
										memset(&madctd,0,sizeof(madctd));
										madctd.mode = 28;//traffic control
										madctd.pattern = *(fcdata->patternid);
										madctd.lampcolor = 0x02;
										madctd.lamptime = 0;
										madctd.phaseid = pinfo.phaseid;
										madctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 28;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x02;
                                        madctd.lamptime = 0;
                                        madctd.phaseid = pinfo.phaseid;
                                        madctd.stageline = pinfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                                		#ifdef MAJOR_DEBUG
                                    		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                    		{
                                    		#ifdef MAJOR_DEBUG
                                        		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
                                    		}
                                		}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
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
                                    memset(&pinfo,0,sizeof(pinfo));
                                    if(SUCCESS != madc_get_phase_info(fcdata,tscdata,rettl,0,&pinfo))
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        output_log("Timing control,get phase info err");
                                    #endif
                                        madc_end_part_child_thread();
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
            								#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
										memset(&madctd,0,sizeof(madctd));
										madctd.mode = 28;//traffic control
										madctd.pattern = *(fcdata->patternid);
										madctd.lampcolor = 0x02;
										madctd.lamptime = pinfo.gftime;
										madctd.phaseid = 0;
										madctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 28;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x02;
                                        madctd.lamptime = pinfo.gftime;
                                        madctd.phaseid = 0;
                                        madctd.stageline = pinfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                                		#ifdef MAJOR_DEBUG
                                    		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                    		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                    		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                    		{
                                    		#ifdef MAJOR_DEBUG
                                        		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
                                    		}
                                		}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
                                		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
	
									ngf = 0;
									if (pinfo.gftime > 0)
									{
										while (1)
										{
											if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef MAJOR_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x03,fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
												printf("updatechannelerr,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef MAJOR_DEBUG
												printf("setgreenlamperr,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x02,fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
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
										if (SUCCESS!=madc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef MAJOR_DEBUG
            								printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef MAJOR_DEBUG
											printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef MAJOR_DEBUG
											printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														wcc,0x01,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
                               			memset(&madctd,0,sizeof(madctd));
                               			madctd.mode = 28;//traffic control
                               			madctd.pattern = *(fcdata->patternid);
                               			madctd.lampcolor = 0x01;
                               			madctd.lamptime = pinfo.ytime;
                               			madctd.phaseid = 0;
                               			madctd.stageline = pinfo.stageline;
                               			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
                               			{
                               			#ifdef MAJOR_DEBUG
                                   			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               			#endif
                               			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 28;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x01;
                                        madctd.lamptime = pinfo.ytime;
                                        madctd.phaseid = 0;
                                        madctd.stageline = pinfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                               			#ifdef MAJOR_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef MAJOR_DEBUG
                                       			printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef MAJOR_DEBUG
                                		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															wcc,0x00,fcdata->markbit))
									{
									#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
                                			memset(&madctd,0,sizeof(madctd));
                                			madctd.mode = 28;//traffic control
                                			madctd.pattern = *(fcdata->patternid);
                                			madctd.lampcolor = 0x00;
                                			madctd.lamptime = pinfo.rtime;
                                			madctd.phaseid = 0;
                                			madctd.stageline = pinfo.stageline;
                                			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
                                			{
                                			#ifdef MAJOR_DEBUG
                                    			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                            			}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&madctd,0,sizeof(madctd));
                                            madctd.mode = 28;//traffic control
                                            madctd.pattern = *(fcdata->patternid);
                                            madctd.lampcolor = 0x00;
                                            madctd.lamptime = pinfo.rtime;
                                            madctd.phaseid = 0;
                                            madctd.stageline = pinfo.stageline;	
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
											{
											#ifdef MAJOR_DEBUG
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
                                    		#ifdef MAJOR_DEBUG
                                        		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
                                        		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                        		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                        		{
                                        		#ifdef MAJOR_DEBUG
                                            		printf("file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        		#endif
                                        		}
                                    		}
                                		}
                                		else
                                		{
                                		#ifdef MAJOR_DEBUG
                                    		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                		#endif
                                		}
										sleep(pinfo.rtime);
									}
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                            		{
                            		#ifdef MAJOR_DEBUG
                               			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			pinfo.chan,0x02,fcdata->markbit))
                            		{
                            		#ifdef MAJOR_DEBUG
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
            							#ifdef MAJOR_DEBUG
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
										memset(&madctd,0,sizeof(madctd));
										madctd.mode = 28;//traffic control
										madctd.pattern = *(fcdata->patternid);
										madctd.lampcolor = 0x02;
										madctd.lamptime = 0;
										madctd.phaseid = pinfo.phaseid;
										madctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 28;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x02;
                                        madctd.lamptime = 0;
                                        madctd.phaseid = pinfo.phaseid;
                                        madctd.stageline = pinfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                               			#ifdef MAJOR_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef MAJOR_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
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
                                    #ifdef MAJOR_DEBUG
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
								madcred = 0;
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
                                        #ifdef MAJOR_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
								if (0 == mayfyes)
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
									int yfret = pthread_create(&mayfid,NULL,(void *)madc_yellow_flash,&yfdata);
									if (0 != yfret)
									{
									#ifdef MAJOR_DEBUG
										printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
										output_log("Timing control,create yellow flash err");
									#endif
										madc_end_part_child_thread();

										return FAIL;
									}
									mayfyes = 1;
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
            						#ifdef MAJOR_DEBUG
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
                                    #ifdef MAJOR_DEBUG
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
								if (1 == mayfyes)
								{
									pthread_cancel(mayfid);
									pthread_join(mayfid,NULL);
									mayfyes = 0;
								}
					
								if (0 == madcred)
								{	
									new_all_red(&ardata);
									madcred = 1;
								}
							
								tpp.func[0] = tcbuf[4];
                                memset(terbuf,0,sizeof(terbuf));
                                if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
                                {
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),terbuf,sizeof(terbuf)) < 0)
                                        {
                                        #ifdef MAJOR_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))	
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
            						#ifdef MAJOR_DEBUG
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
                                    #ifdef MAJOR_DEBUG
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
								madcred = 0;
								cp = 0;
								if (1 == mayfyes)
                                {
                                    pthread_cancel(mayfid);
                                    pthread_join(mayfid,NULL);
                                    mayfyes = 0;
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
                                        #ifdef MAJOR_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
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
            						#ifdef MAJOR_DEBUG
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
                                    #ifdef MAJOR_DEBUG
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
							get_madc_status(&color,&leatime);
							ma_phase = 0;
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
											phcon = 0;
											endlock = 1;
											break;
										}
										else
										{
											ma_phase = tcbuf[1];
										}
									}
								
									get_madc_status(&color,&leatime);
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
							madcred = 0;
							close = 0;
							fpc = 0;
							cp = 0;
							madc_end_child_thread();//end main thread and its child thread

							*(fcdata->markbit2) |= 0x0008;
						#if 0
							new_all_red(&ardata);
						#endif
							if (0 == phcon)
							{
								if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
								{
								#ifdef MAJOR_DEBUG
									printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									*(fcdata->markbit) |= 0x0800;
								}
							}
						
							*(fcdata->markbit) |= 0x4000;								
							*(fcdata->contmode) = 29;//network control mode
					//		phcon = 0;
				
							if (*(fcdata->auxfunc) & 0x01)
							{//if (*(fcdata->auxfunc) & 0x01)
								if (!wait_write_serial(*(fcdata->bbserial)))
								{
									if (write(*(fcdata->bbserial),dcdownti,sizeof(dcdownti)) < 0)
									{
									#ifdef FULL_DETECT_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
								}
								else
								{
								#ifdef FULL_DETECT_DEBUG
									printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
								if (!wait_write_serial(*(fcdata->bbserial)))
								{
									if (write(*(fcdata->bbserial),dcedownti,sizeof(dcedownti)) < 0)
									{
									#ifdef FULL_DETECT_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
								}
								else
								{
								#ifdef FULL_DETECT_DEBUG
									printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
							}//if (*(fcdata->auxfunc) & 0x01)
			
							fbdata[1] = 29;
							fbdata[2] = pinfo.stageline;
							fbdata[3] = 0x02;
							fbdata[4] = 0;
							if (!wait_write_serial(*(fcdata->fbserial)))
							{
								if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
								{
								#ifdef MAJOR_DEBUG
									printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									*(fcdata->markbit) |= 0x0800;
									gettimeofday(&ct,NULL);
                					update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                					if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																fcdata->softevent,fcdata->markbit))
                					{
                					#ifdef MAJOR_DEBUG
                   						printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                					#endif
                					}
								}
							}
							else
							{
							#ifdef MAJOR_DEBUG
								printf("face board serial port cannot write,Line:%d\n",__LINE__);
							#endif
							}
							sendfaceInfoToBoard(fcdata,fbdata);	
							//send down time to configure tool
                            if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            {
							#ifdef MAJOR_DEBUG
								printf("send control info to configure tool,File: %s,Line: %d\n", \
											__FILE__,__LINE__);
							#endif
                                memset(&madctd,0,sizeof(madctd));
                                madctd.mode = 29;
                                madctd.pattern = *(fcdata->patternid);
                                madctd.lampcolor = 0x02;
                                madctd.lamptime = 0;
                                madctd.phaseid = pinfo.phaseid;
                                madctd.stageline = pinfo.stageline;
                                if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
                                {
                                #ifdef MAJOR_DEBUG
                                    printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
                            }
							#ifdef EMBED_CONFIGURE_TOOL
							if (*(fcdata->markbit2) & 0x0200)
							{
								memset(&madctd,0,sizeof(madctd));
                                madctd.mode = 29;
                                madctd.pattern = *(fcdata->patternid);
                                madctd.lampcolor = 0x02;
                                madctd.lamptime = 0;
                                madctd.phaseid = pinfo.phaseid;
                                madctd.stageline = pinfo.stageline;	
								if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
								{
								#ifdef MAJOR_DEBUG
									printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#endif
                            if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    pinfo.chan,0x02,fcdata->markbit))
                            {
                            #ifdef MAJOR_DEBUG
                                printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            #endif
                            }
							
							objecti[0].objectv[0] = 0xf2;
							factovs = 0;
							memset(cbuf,0,sizeof(cbuf));
							if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                            {
                            #ifdef MAJOR_DEBUG
                            	printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }
							if (!(*(fcdata->markbit) & 0x1000))
							{
                            	write(*(fcdata->sockfd->csockfd),cbuf,factovs);
							}
							gettimeofday(&ct,NULL);
                            update_event_list(fcdata->ec,fcdata->el,1,84,ct.tv_sec,fcdata->markbit);
                           	if(0 == ma_phase)
							{
								continue;
							}
							else
							{
								tcbuf[1] = ma_phase;
								madc_get_last_phaseinfo(fcdata,tscdata,rettl,&pinfo);
							}
						}//net control will happen
						if ((0xf0 == tcbuf[1]) && (1 == netlock))
                        {//has been status of net control
                            objecti[0].objectv[0] = 0xf2;
                            factovs = 0;
                            memset(cbuf,0,sizeof(cbuf));
                            if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                            {
                            #ifdef MAJOR_DEBUG
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
                            #ifdef MAJOR_DEBUG
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
				//			madcred = 0;
				//			close = 0;
							fpc = 0;
					/*
							if (1 == mayfyes)
							{
								pthread_cancel(mayfid);
								pthread_join(mayfid,NULL);
								mayfyes = 0;
							}
					*/
							*(fcdata->markbit) &= 0xbfff;
							
							if (0 == cp)
                            {//not have phase control
                                if ((0 == madcred) && (0 == mayfyes) && (0 == close))
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
            								#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
										memset(&madctd,0,sizeof(madctd));
										madctd.mode = 29;//traffic control
										madctd.pattern = *(fcdata->patternid);
										madctd.lampcolor = 0x02;
										madctd.lamptime = pinfo.gftime;
										madctd.phaseid = pinfo.phaseid;
										madctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 29;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x02;
                                        madctd.lamptime = pinfo.gftime;
                                        madctd.phaseid = pinfo.phaseid;
                                        madctd.stageline = pinfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                                		#ifdef MAJOR_DEBUG
                                    		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                    		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                    		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                    		{
                                    		#ifdef MAJOR_DEBUG
                                        		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
                                    		}
                                		}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
                                		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
	
									ngf = 0;
									if (pinfo.gftime > 0)
									{
									while (1)
									{
										if(SUCCESS!=madc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x03))
        								{
        								#ifdef MAJOR_DEBUG
            								printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															pinfo.cchan,0x03,fcdata->markbit))
        								{
        								#ifdef MAJOR_DEBUG
            								printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
        								#endif
        								}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);

										if(SUCCESS!=madc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x02))
        								{
        								#ifdef MAJOR_DEBUG
            								printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    									pinfo.cchan,0x02,fcdata->markbit))
                                		{
                                		#ifdef MAJOR_DEBUG
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
										if(SUCCESS!=madc_set_lamp_color(*(fcdata->bbserial),pinfo.cnpchan,0x01))
        								{
        								#ifdef MAJOR_DEBUG
            								printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
													pinfo.cnpchan,0x01, fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if(SUCCESS!=madc_set_lamp_color(*(fcdata->bbserial),pinfo.cpchan,0x00))
										{
										#ifdef MAJOR_DEBUG
											printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																	pinfo.cpchan,0x00,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if(SUCCESS!=madc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x01))
										{
										#ifdef MAJOR_DEBUG
											printf("set yellow lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														pinfo.cchan,0x01,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
                               			memset(&madctd,0,sizeof(madctd));
                               			madctd.mode = 29;//traffic control
                               			madctd.pattern = *(fcdata->patternid);
                               			madctd.lampcolor = 0x01;
                               			madctd.lamptime = pinfo.ytime;
                               			madctd.phaseid = pinfo.phaseid;
                               			madctd.stageline = pinfo.stageline;
                               			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
                               			{
                               			#ifdef MAJOR_DEBUG
                                   			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               			#endif
                               			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 29;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x01;
                                        madctd.lamptime = pinfo.ytime;
                                        madctd.phaseid = pinfo.phaseid;
                                        madctd.stageline = pinfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                               			#ifdef MAJOR_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef MAJOR_DEBUG
                                       			printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x00))
                            		{
                            		#ifdef MAJOR_DEBUG
                                		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															pinfo.cchan,0x00,fcdata->markbit))
									{
									#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
                                			memset(&madctd,0,sizeof(madctd));
                                			madctd.mode = 29;//traffic control
                                			madctd.pattern = *(fcdata->patternid);
                                			madctd.lampcolor = 0x00;
                                			madctd.lamptime = pinfo.rtime;
                                			madctd.phaseid = pinfo.phaseid;
                                			madctd.stageline = pinfo.stageline;
                                			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
                                			{
                                			#ifdef MAJOR_DEBUG
                                    			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                            			}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&madctd,0,sizeof(madctd));
                                            madctd.mode = 29;//traffic control
                                            madctd.pattern = *(fcdata->patternid);
                                            madctd.lampcolor = 0x00;
                                            madctd.lamptime = pinfo.rtime;
                                            madctd.phaseid = pinfo.phaseid;
                                            madctd.stageline = pinfo.stageline;	
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
											{
											#ifdef MAJOR_DEBUG
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
                                    		#ifdef MAJOR_DEBUG
                                        		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
                                        		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                        		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                        		{
                                        		#ifdef MAJOR_DEBUG
                                            		printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                        		#endif
                                        		}
                                    		}
                                		}
                                		else
                                		{
                                		#ifdef MAJOR_DEBUG
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
                                }//not have all red/yellow flash/close
                                else
                                {//have all red or yellow flash or all close
                                    madcred = 0;
                                    close = 0;
                                    if (1 == mayfyes)
                                    {
                                        pthread_cancel(mayfid);
                                        pthread_join(mayfid,NULL);
                                        mayfyes = 0;
                                    }
                                }//have all red or yellow flash or all close
                            }//not have phase control
							else if (0 != cp)
							{//phase control happen
								if (((0x5a <= cp) && (cp <= 0x5d)) || 0xC8 == cp)
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

									if((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
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
            								#ifdef MAJOR_DEBUG
                								printf("Info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
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
            								#ifdef MAJOR_DEBUG
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
										memset(&madctd,0,sizeof(madctd));
										madctd.mode = 29;//traffic control
										madctd.pattern = *(fcdata->patternid);
										madctd.lampcolor = 0x02;
										madctd.lamptime = pinfo.gftime;
										madctd.phaseid = 0;
										madctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 29;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x02;
                                        madctd.lamptime = pinfo.gftime;
                                        madctd.phaseid = 0;
                                        madctd.stageline = pinfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                               			#ifdef MAJOR_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef MAJOR_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									ngf = 0;
									if (pinfo.gftime > 0)
									{
										while (1)
										{
											if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef MAJOR_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x03,fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef MAJOR_DEBUG
												printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
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
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef MAJOR_DEBUG
            								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef MAJOR_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef MAJOR_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}

									if ((0 != wcc[0]) && (pinfo.ytime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
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
            								#ifdef MAJOR_DEBUG
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
                            			memset(&madctd,0,sizeof(madctd));
                            			madctd.mode = 29;//traffic control
                            			madctd.pattern = *(fcdata->patternid);
                            			madctd.lampcolor = 0x01;
                            			madctd.lamptime = pinfo.ytime;
                            			madctd.phaseid = 0;
                            			madctd.stageline = pinfo.stageline;
                            			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
                            			{
                            			#ifdef MAJOR_DEBUG
                               				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            			#endif
                            			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 29;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x01;
                                        madctd.lamptime = pinfo.ytime;
                                        madctd.phaseid = 0;
                                        madctd.stageline = pinfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                            			#ifdef MAJOR_DEBUG
                               				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                			{
                                			#ifdef MAJOR_DEBUG
                                   				printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef MAJOR_DEBUG
                                		printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef MAJOR_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}

									if ((0 != wcc[i]) && (pinfo.rtime > 0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
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
            								#ifdef MAJOR_DEBUG
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
												#ifdef MAJOR_DEBUG
													printf("Info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
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
												#ifdef MAJOR_DEBUG
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
											memset(&madctd,0,sizeof(madctd));
											madctd.mode = 29;//traffic control
											madctd.pattern = *(fcdata->patternid);
											madctd.lampcolor = 0x02;
											madctd.lamptime = pinfo.gftime;
											madctd.phaseid = cp;
											madctd.stageline = pinfo.stageline;
											if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
											{
											#ifdef MAJOR_DEBUG
												printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&madctd,0,sizeof(madctd));
                                            madctd.mode = 29;//traffic control
                                            madctd.pattern = *(fcdata->patternid);
                                            madctd.lampcolor = 0x02;
                                            madctd.lamptime = pinfo.gftime;
                                            madctd.phaseid = cp;
                                            madctd.stageline = pinfo.stageline;	
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
											{
											#ifdef MAJOR_DEBUG
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
											#ifdef MAJOR_DEBUG
												printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
												update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
												if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
												{
												#ifdef MAJOR_DEBUG
													printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
											}
										}
										else
										{
										#ifdef MAJOR_DEBUG
											printf("face board serial port cannot write,Line:%d\n",__LINE__);
										#endif
										}
										ngf = 0;
										if (pinfo.gftime > 0)
										{
										while (1)
										{
											if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef MAJOR_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x03,fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef MAJOR_DEBUG
												printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
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
											if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
											{
											#ifdef MAJOR_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wnpcc,0x01, fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
											{
											#ifdef MAJOR_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wpcc,0x00,fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										else
										{
											if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
											{
											#ifdef MAJOR_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x01,fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
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
												#ifdef MAJOR_DEBUG
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
											memset(&madctd,0,sizeof(madctd));
											madctd.mode = 29;//traffic control
											madctd.pattern = *(fcdata->patternid);
											madctd.lampcolor = 0x01;
											madctd.lamptime = pinfo.ytime;
											madctd.phaseid = cp;
											madctd.stageline = pinfo.stageline;
											if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
											{
											#ifdef MAJOR_DEBUG
												printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&madctd,0,sizeof(madctd));
                                            madctd.mode = 29;//traffic control
                                            madctd.pattern = *(fcdata->patternid);
                                            madctd.lampcolor = 0x01;
                                            madctd.lamptime = pinfo.ytime;
                                            madctd.phaseid = cp;
                                            madctd.stageline = pinfo.stageline;	
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
											{
											#ifdef MAJOR_DEBUG
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
											#ifdef MAJOR_DEBUG
												printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
												update_event_list(fcdata->ec,fcdata->el,1,16, \
																		ct.tv_sec,fcdata->markbit);
												if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
												{
												#ifdef MAJOR_DEBUG
													printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
											}
										}
										else
										{
										#ifdef MAJOR_DEBUG
											printf("face board serial port cannot write,Line:%d\n",__LINE__);
										#endif
										}
										sleep(pinfo.ytime);

										//current phase begin to red lamp
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
										{
										#ifdef MAJOR_DEBUG
											printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x00,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}

										if ((0 != wcc[i]) && (pinfo.rtime > 0))
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
												#ifdef MAJOR_DEBUG
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
												#ifdef MAJOR_DEBUG
													printf("Info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
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
												#ifdef MAJOR_DEBUG
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
											memset(&madctd,0,sizeof(madctd));
											madctd.mode = 29;//traffic control
											madctd.pattern = *(fcdata->patternid);
											madctd.lampcolor = 0x02;
											madctd.lamptime = pinfo.gftime;
											madctd.phaseid = cp;
											madctd.stageline = pinfo.stageline;
											if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
											{
											#ifdef MAJOR_DEBUG
												printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&madctd,0,sizeof(madctd));
                                            madctd.mode = 29;//traffic control
                                            madctd.pattern = *(fcdata->patternid);
                                            madctd.lampcolor = 0x02;
                                            madctd.lamptime = pinfo.gftime;
                                            madctd.phaseid = cp;
                                            madctd.stageline = pinfo.stageline;	
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
											{
											#ifdef MAJOR_DEBUG
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
											#ifdef MAJOR_DEBUG
												printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
												update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
												if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
												{
												#ifdef MAJOR_DEBUG
													printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
											}
										}
										else
										{
										#ifdef MAJOR_DEBUG
											printf("face board serial port cannot write,Line:%d\n",__LINE__);
										#endif
										}
										ngf = 0;
										if (pinfo.gftime > 0)
										{
										while (1)
										{
											if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef MAJOR_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x03,fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef MAJOR_DEBUG
												printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
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
											if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
											{
											#ifdef MAJOR_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wnpcc,0x01, fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
											{
											#ifdef MAJOR_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wpcc,0x00,fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										else
										{
											if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
											{
											#ifdef MAJOR_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x01,fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
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
												#ifdef MAJOR_DEBUG
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
											memset(&madctd,0,sizeof(madctd));
											madctd.mode = 29;//traffic control
											madctd.pattern = *(fcdata->patternid);
											madctd.lampcolor = 0x01;
											madctd.lamptime = pinfo.ytime;
											madctd.phaseid = cp;
											madctd.stageline = pinfo.stageline;
											if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
											{
											#ifdef MAJOR_DEBUG
												printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
										}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&madctd,0,sizeof(madctd));
                                            madctd.mode = 29;//traffic control
                                            madctd.pattern = *(fcdata->patternid);
                                            madctd.lampcolor = 0x01;
                                            madctd.lamptime = pinfo.ytime;
                                            madctd.phaseid = cp;
                                            madctd.stageline = pinfo.stageline;	
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
											{
											#ifdef MAJOR_DEBUG
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
											#ifdef MAJOR_DEBUG
												printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
												update_event_list(fcdata->ec,fcdata->el,1,16, \
																		ct.tv_sec,fcdata->markbit);
												if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
												{
												#ifdef MAJOR_DEBUG
													printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
												#endif
												}
											}
										}
										else
										{
										#ifdef MAJOR_DEBUG
											printf("face board serial port cannot write,Line:%d\n",__LINE__);
										#endif
										}
										sleep(pinfo.ytime);

										//current phase begin to red lamp
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
										{
										#ifdef MAJOR_DEBUG
											printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x00,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}

										if ((0 != wcc[i]) && (pinfo.rtime > 0))
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
												#ifdef MAJOR_DEBUG
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
							ncmode = *(fcdata->contmode);
							cp = 0;
							*(fcdata->markbit2) &= 0xfff7; 
							*(fcdata->fcontrol) = 0;
							*(fcdata->ccontrol) = 0;
							*(fcdata->trans) = 0; 
							if (0 == mahdyes)
                            {
                                memset(&mahdata,0,sizeof(mahdata_t));
                                memset(&pinfo,0,sizeof(pinfo));
                                mahdata.fd = fcdata;
                                mahdata.td = tscdata;
                                mahdata.pi = &pinfo;
                                mahdata.cs = chanstatus;
                                int ret=pthread_create(&mahdid,NULL,(void *)start_major_half_detect,&mahdata);
                                if (0 != ret)
                                {
                                #ifdef MAJOR_DEBUG
                                    printf("create thread error,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    output_log("major half detect,create major half detect thread err");
                                #endif
                                    return FAIL;
                                }
                                mahdyes = 1;
                            }

							objecti[0].objectv[0] = 0xf3;
							factovs = 0;
							memset(cbuf,0,sizeof(cbuf));
							if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                            {
                            #ifdef MAJOR_DEBUG
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
							{//if ((0x5a <= tcbuf[1]) && (tcbuf[1] <= 0x5d))
								if ((1 == madcred) || (1 == mayfyes) || (1 == close) || (1 == phcon))
                            	{//if ((1 == tcred) || (1 == tcyfyes) || (1 == close))
									fpc = 1;
									phcon = 0;
									cp = tcbuf[1];
                                	if (1 == mayfyes)
                                	{
                                    	pthread_cancel(mayfid);
                                    	pthread_join(mayfid,NULL);
                                    	mayfyes = 0;
                                	}
									if (1 != madcred)
									{
										new_all_red(&ardata);
									}
									madcred = 0;
                                    close = 0;
				//					#ifdef CLOSE_LAMP
                                    madc_set_lamp_color(*(fcdata->bbserial),clch,0x03);
									update_channel_status(fcdata->sockfd,chanstatus,clch,0x03,fcdata->markbit);
               //                     #endif
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
										if(SUCCESS!=madc_set_lamp_color(*(fcdata->bbserial),ccon,0x02))
                                        {
                                        #ifdef MAJOR_DEBUG
                                            printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                            *(fcdata->markbit) |= 0x0800;
                                        }
                                        if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                                                    		ccon,0x02,fcdata->markbit))
                                        {
                                        #ifdef MAJOR_DEBUG
                                            printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                        #endif
                                        }
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 29;//added by sk on 20180512
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
											#ifdef MAJOR_DEBUG
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
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial), \
														lkch[cp-0x5a],0x02))
										{
										#ifdef MAJOR_DEBUG
											printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																	lkch[cp-0x5a],0x02,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if ((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
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
											#ifdef MAJOR_DEBUG
												printf("statusinfopackerr,File:%s,Line:%d\n",__FILE__,__LINE__);
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
                            	}//if ((1 == madcred) || (1 == mayfyes) || (1 == close))
								if (0 == fpc)
								{//phase control of first times
									fpc = 1;
									cp = pinfo.phaseid;
								}//phase control of first times

								if ((cp == tcbuf[1]) && (0xC8 != cp))
								{
									if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
									{
										//added by sk on 20180512
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
            							#ifdef MAJOR_DEBUG
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
                                    #ifdef MAJOR_DEBUG
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
									if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
									{//if ((0x5a <= cp) && (cp <= 0x5d))
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
											}
											else if (0xC8 == tcbuf[1])
											{
												if (*(fcdata->ccontrol) & (0x00000001 << (cc[i]-1)))
                                                    ce = 1;
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
														if (lkch[tcbuf[1]-0x5a][j] == \
															tscdata->channel->ChannelList[i].ChannelId)
														{
															ce = 1;
															break;
														}
													}
												}
												else if (0xC8 == tcbuf[1])
												{
													tclc = tscdata->channel->ChannelList[i].ChannelId;
                                                    if (*(fcdata->ccontrol) & (0x00000001 << (tclc - 1)))
                                                        ce = 1;
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

									if (0xC8 == tcbuf[1])
										*(fcdata->trans) |= 0x01;

									if ((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
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
                                            	tcsta |= 0x02;
                                            	*csta = tcsta;
                                            	csta++;
                                        	}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
										memset(&madctd,0,sizeof(madctd));
										madctd.mode = 29;//traffic control
										madctd.pattern = *(fcdata->patternid);
										madctd.lampcolor = 0x02;
										madctd.lamptime = pinfo.gftime;
										madctd.phaseid = 0;
										madctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 29;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x02;
                                        madctd.lamptime = pinfo.gftime;
                                        madctd.phaseid = 0;
                                        madctd.stageline = pinfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                               			#ifdef MAJOR_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef MAJOR_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}

									ngf = 0;
									if (pinfo.gftime > 0)
									{
										while (1)
										{
											if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef MAJOR_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x03,fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef MAJOR_DEBUG
												printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
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
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef MAJOR_DEBUG
            								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef MAJOR_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef MAJOR_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
                            			memset(&madctd,0,sizeof(madctd));
                            			madctd.mode = 29;//traffic control
                            			madctd.pattern = *(fcdata->patternid);
                            			madctd.lampcolor = 0x01;
                            			madctd.lamptime = pinfo.ytime;
                            			madctd.phaseid = 0;
                            			madctd.stageline = pinfo.stageline;
                            			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
                            			{
                            			#ifdef MAJOR_DEBUG
                               				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            			#endif
                            			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 29;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x01;
                                        madctd.lamptime = pinfo.ytime;
                                        madctd.phaseid = 0;
                                        madctd.stageline = pinfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                            			#ifdef MAJOR_DEBUG
                               				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                			{
                                			#ifdef MAJOR_DEBUG
                                   				printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef MAJOR_DEBUG
                                		printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
                                			memset(&madctd,0,sizeof(madctd));
                                			madctd.mode = 29;//network control
                                			madctd.pattern = *(fcdata->patternid);
                                			madctd.lampcolor = 0x00;
                                			madctd.lamptime = pinfo.rtime;
                                			madctd.phaseid = 0;
                                			madctd.stageline = pinfo.stageline;
                                			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
                                			{
                                			#ifdef MAJOR_DEBUG
                                   				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                            			}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&madctd,0,sizeof(madctd));
                                            madctd.mode = 29;//network control
                                            madctd.pattern = *(fcdata->patternid);
                                            madctd.lampcolor = 0x00;
                                            madctd.lamptime = pinfo.rtime;
                                            madctd.phaseid = 0;
                                            madctd.stageline = pinfo.stageline;	
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
											{
											#ifdef MAJOR_DEBUG
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
                                   			#ifdef MAJOR_DEBUG
                                       			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   			#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
                                       			update_event_list(fcdata->ec,fcdata->el,1,16, \
																		ct.tv_sec,fcdata->markbit);
                                       			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                       			{
                                       			#ifdef MAJOR_DEBUG
                                           			printf("gen err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                       			#endif
                                       			}
                                   			}
                                		}
                                		else
                                		{
                                		#ifdef MAJOR_DEBUG
                                   			printf("face board port cannot write,Line:%d\n",__LINE__);
                                		#endif
                                		}
										sleep(pinfo.rtime);
									}
									if ((0x5a <= tcbuf[1]) && (tcbuf[1] <= 0x5d))
                                    {
										if (SUCCESS != \
											madc_set_lamp_color(*(fcdata->bbserial),lkch[tcbuf[1]-0x5a],0x02))
										{
										#ifdef MAJOR_DEBUG
											printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
							
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																lkch[tcbuf[1]-0x5a],0x02,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
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
											#ifdef MAJOR_DEBUG
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
                                        if(SUCCESS!=madc_set_lamp_color(*(fcdata->bbserial),ccon,0x02))
                                        {
                                        #ifdef MAJOR_DEBUG
                                            printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                            *(fcdata->markbit) |= 0x0800;
                                        }
                                        if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                                                            ccon,0x02,fcdata->markbit))
                                        {
                                        #ifdef MAJOR_DEBUG
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
                                            #ifdef MAJOR_DEBUG
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
										memset(&madctd,0,sizeof(madctd));
										madctd.mode = 29;//traffic control
										madctd.pattern = *(fcdata->patternid);
										madctd.lampcolor = 0x02;
										madctd.lamptime = 0;
										madctd.phaseid = 0;
										madctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 29;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x02;
                                        madctd.lamptime = 0;
                                        madctd.phaseid = 0;
                                        madctd.stageline = pinfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                               			#ifdef MAJOR_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef MAJOR_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
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
                            		#ifdef MAJOR_DEBUG
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
							}//if ((0x5a <= tcbuf[1]) && (tcbuf[1] <= 0x5d))
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
								#ifdef MAJOR_DEBUG
									printf("Not fit phase id,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									objecti[0].objectv[0] = 0xf4;
                                    factovs = 0;
                                    memset(cbuf,0,sizeof(cbuf));
                                    if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x00))
                                    {
                                    #ifdef MAJOR_DEBUG
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

								if ((1 == madcred) || (1 == mayfyes) || (1 == close) || (1 == phcon))
                            	{//if ((1 == tcred) || (1 == tcyfyes) || (1 == close))
									fpc = 1;
									phcon = 0;
									cp = tcbuf[1];
                                	if (1 == mayfyes)
                                	{
                                    	pthread_cancel(mayfid);
                                    	pthread_join(mayfid,NULL);
                                    	mayfyes = 0;
                                	}
									if (1 != madcred)
									{
										new_all_red(&ardata);
									}
									madcred = 0;
                                    close = 0;

			//						#ifdef CLOSE_LAMP
                                    madc_set_lamp_color(*(fcdata->bbserial),clch,0x03);
									update_channel_status(fcdata->sockfd,chanstatus,clch,0x03,fcdata->markbit);
            //                        #endif

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
                                	if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),nc,0x02))
                                	{
                                	#ifdef MAJOR_DEBUG
                                    	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
                                    	*(fcdata->markbit) |= 0x0800;
                                	}
                                	if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                                        				nc,0x02,fcdata->markbit))
                                	{
                                	#ifdef MAJOR_DEBUG
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
            							#ifdef MAJOR_DEBUG
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
                            	}//if ((1 == madcred) || (1 == mayfyes) || (1 == close))
								if (0 == fpc)
								{//phase control of first times
									fpc = 1;
									cp = pinfo.phaseid;
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
            							#ifdef MAJOR_DEBUG
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
                                    #ifdef MAJOR_DEBUG
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

									if ((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
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
                                            	tcsta |= 0x02;
                                            	*csta = tcsta;
                                            	csta++;
                                        	}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            								memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
										memset(&madctd,0,sizeof(madctd));
										madctd.mode = 29;//traffic control
										madctd.pattern = *(fcdata->patternid);
										madctd.lampcolor = 0x02;
										madctd.lamptime = pinfo.gftime;
										madctd.phaseid = cp;
										madctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 29;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x02;
                                        madctd.lamptime = pinfo.gftime;
                                        madctd.phaseid = cp;
                                        madctd.stageline = pinfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                               			#ifdef MAJOR_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef MAJOR_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}

									ngf = 0;
									if (pinfo.gftime > 0)
									{
										while (1)
										{
											if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
											{
											#ifdef MAJOR_DEBUG
												printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x03,fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
												printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											to.tv_sec = 0;
											to.tv_usec = 500000;
											select(0,NULL,NULL,NULL,&to);

											if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
											{
											#ifdef MAJOR_DEBUG
												printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
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
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef MAJOR_DEBUG
            								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef MAJOR_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef MAJOR_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
                            			memset(&madctd,0,sizeof(madctd));
                            			madctd.mode = 29;//traffic control
                            			madctd.pattern = *(fcdata->patternid);
                            			madctd.lampcolor = 0x01;
                            			madctd.lamptime = pinfo.ytime;
                            			madctd.phaseid = cp;
                            			madctd.stageline = pinfo.stageline;
                            			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
                            			{
                            			#ifdef MAJOR_DEBUG
                               				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            			#endif
                            			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 29;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x01;
                                        madctd.lamptime = pinfo.ytime;
                                        madctd.phaseid = cp;
                                        madctd.stageline = pinfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                            			#ifdef MAJOR_DEBUG
                               				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                			{
                                			#ifdef MAJOR_DEBUG
                                   				printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef MAJOR_DEBUG
                                		printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
                                			memset(&madctd,0,sizeof(madctd));
                                			madctd.mode = 29;//network control
                                			madctd.pattern = *(fcdata->patternid);
                                			madctd.lampcolor = 0x00;
                                			madctd.lamptime = pinfo.rtime;
                                			madctd.phaseid = cp;
                                			madctd.stageline = pinfo.stageline;
                                			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
                                			{
                                			#ifdef MAJOR_DEBUG
                                   				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                            			}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&madctd,0,sizeof(madctd));
                                            madctd.mode = 29;//network control
                                            madctd.pattern = *(fcdata->patternid);
                                            madctd.lampcolor = 0x00;
                                            madctd.lamptime = pinfo.rtime;
                                            madctd.phaseid = cp;
                                            madctd.stageline = pinfo.stageline;	
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
											{
											#ifdef MAJOR_DEBUG
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
                                   			#ifdef MAJOR_DEBUG
                                       			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   			#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
                                       			update_event_list(fcdata->ec,fcdata->el,1,16, \
																		ct.tv_sec,fcdata->markbit);
                                       			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                       			{
                                       			#ifdef MAJOR_DEBUG
                                           			printf("gen err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                       			#endif
                                       			}
                                   			}
                                		}
                                		else
                                		{
                                		#ifdef MAJOR_DEBUG
                                   			printf("face board port cannot write,Line:%d\n",__LINE__);
                                		#endif
                                		}
										sleep(pinfo.rtime);
									}

									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),nc,0x02))
                            		{
                            		#ifdef MAJOR_DEBUG
                               			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			nc,0x02,fcdata->markbit))
                            		{
                            		#ifdef MAJOR_DEBUG
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
            							#ifdef MAJOR_DEBUG
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
										memset(&madctd,0,sizeof(madctd));
										madctd.mode = 29;//traffic control
										madctd.pattern = *(fcdata->patternid);
										madctd.lampcolor = 0x02;
										madctd.lamptime = 0;
										madctd.phaseid = tcbuf[1];
										madctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 29;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x02;
                                        madctd.lamptime = 0;
                                        madctd.phaseid = tcbuf[1];
                                        madctd.stageline = pinfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                               			#ifdef MAJOR_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef MAJOR_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									cp = tcbuf[1];

									objecti[0].objectv[0] = 0xf4;
									factovs = 0;
									memset(cbuf,0,sizeof(cbuf));
									if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                            		{
                            		#ifdef MAJOR_DEBUG
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
								if ((1 == madcred) || (1 == close) || (1 == mayfyes) || (1 == phcon))
								{
                                	if (1 == mayfyes)
                                	{
                                    	pthread_cancel(mayfid);
                                    	pthread_join(mayfid,NULL);
                                    	mayfyes = 0;
                                	}
									if (1 != madcred)
									{
										new_all_red(&ardata);
									}
									phcon = 0;
									madcred = 0;
									close = 0;
						//			#ifdef CLOSE_LAMP
                                    madc_set_lamp_color(*(fcdata->bbserial),clch,0x03);
									update_channel_status(fcdata->sockfd,chanstatus,clch,0x03,fcdata->markbit);
                       //             #endif
									if (0 == cp)
									{//not have phase control
										if(SUCCESS!=madc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                                		{
                                		#ifdef MAJOR_DEBUG
                                    	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
                                    		*(fcdata->markbit) |= 0x0800;
                                		}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    					pinfo.chan,0x02,fcdata->markbit))
                                		{
                                		#ifdef MAJOR_DEBUG
                                    		printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               			#endif
                                		}

									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
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
            							#ifdef MAJOR_DEBUG
                							printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
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
												madc_set_lamp_color(*(fcdata->bbserial),lkch[cp-0x5a],0x02))
											{
											#ifdef MAJOR_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																		lkch[cp-0x5a],0x02,fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
												printf("update err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											if((!(*(fcdata->markbit)&0x1000))&&(!(*(fcdata->markbit)&0x8000)))
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
												#ifdef MAJOR_DEBUG
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
                                            if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),ccon,0x02))
                                            {
                                            #ifdef MAJOR_DEBUG
                                                printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                            #endif
                                                *(fcdata->markbit) |= 0x0800;
                                            }
                                            if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                                                                ccon,0x02,fcdata->markbit))
                                            {
                                            #ifdef MAJOR_DEBUG
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
                                                #ifdef MAJOR_DEBUG
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
											if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),cc,0x02))
											{
											#ifdef MAJOR_DEBUG
												printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				cc,0x02,fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
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
												#ifdef MAJOR_DEBUG
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
								}//if ((1 == madcred) || (1 == close) || (1 == mayfyes))


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
            							#ifdef MAJOR_DEBUG
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
            							#ifdef MAJOR_DEBUG
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
									memset(&madctd,0,sizeof(madctd));
									madctd.mode = 29;//traffic control
									madctd.pattern = *(fcdata->patternid);
									madctd.lampcolor = 0x02;
									madctd.lamptime = pinfo.gftime;
									madctd.phaseid = pinfo.phaseid;
									madctd.stageline = pinfo.stageline;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
									{
									#ifdef MAJOR_DEBUG
										printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef EMBED_CONFIGURE_TOOL
                                if (*(fcdata->markbit2) & 0x0200)
                                {
                                   	memset(&madctd,0,sizeof(madctd));
                                    madctd.mode = 29;//traffic control
                                    madctd.pattern = *(fcdata->patternid);
                                    madctd.lampcolor = 0x02;
                                    madctd.lamptime = pinfo.gftime;
                                    madctd.phaseid = pinfo.phaseid;
                                    madctd.stageline = pinfo.stageline; 
                                    if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
                                    {
                                    #ifdef MAJOR_DEBUG
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
                                	#ifdef MAJOR_DEBUG
                                    	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef MAJOR_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                	}
                            	}
                            	else
                            	{
                            	#ifdef MAJOR_DEBUG
                                	printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            	#endif
                            	}
	
								ngf = 0;
								if (pinfo.gftime > 0)
								{
								while (1)
								{
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x03))
        							{
        							#ifdef MAJOR_DEBUG
            							printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        							#endif
										*(fcdata->markbit) |= 0x0800;
        							}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															pinfo.cchan,0x03,fcdata->markbit))
        							{
        							#ifdef MAJOR_DEBUG
            							printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
        							#endif
        							}
									to.tv_sec = 0;
									to.tv_usec = 500000;
									select(0,NULL,NULL,NULL,&to);

									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x02))
        							{
        							#ifdef MAJOR_DEBUG
            							printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        							#endif
										*(fcdata->markbit) |= 0x0800;
        							}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    									pinfo.cchan,0x02,fcdata->markbit))
                                	{
                                	#ifdef MAJOR_DEBUG
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
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.cnpchan,0x01))
        							{
        							#ifdef MAJOR_DEBUG
            							printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        							#endif
										*(fcdata->markbit) |= 0x0800;
        							}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
													pinfo.cnpchan,0x01, fcdata->markbit))
									{
									#ifdef MAJOR_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.cpchan,0x00))
									{
									#ifdef MAJOR_DEBUG
										printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																	pinfo.cpchan,0x00,fcdata->markbit))
									{
									#ifdef MAJOR_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
								}
								else
								{
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x01))
									{
									#ifdef MAJOR_DEBUG
										printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														pinfo.cchan,0x01,fcdata->markbit))
									{
									#ifdef MAJOR_DEBUG
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
            							#ifdef MAJOR_DEBUG
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
                               		memset(&madctd,0,sizeof(madctd));
                               		madctd.mode = 29;//traffic control
                               		madctd.pattern = *(fcdata->patternid);
                               		madctd.lampcolor = 0x01;
                               		madctd.lamptime = pinfo.ytime;
                               		madctd.phaseid = pinfo.phaseid;
                               		madctd.stageline = pinfo.stageline;
                               		if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
                               		{
                               		#ifdef MAJOR_DEBUG
                                   		printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               		#endif
                               		}
                            	}
								#ifdef EMBED_CONFIGURE_TOOL
                                if (*(fcdata->markbit2) & 0x0200)
                                {
                                   	memset(&madctd,0,sizeof(madctd));
                                    madctd.mode = 29;//traffic control
                                    madctd.pattern = *(fcdata->patternid);
                                    madctd.lampcolor = 0x01;
                                    madctd.lamptime = pinfo.ytime;
                                    madctd.phaseid = pinfo.phaseid;
                                    madctd.stageline = pinfo.stageline; 
                                    if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
                                    {
                                    #ifdef MAJOR_DEBUG
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
                               		#ifdef MAJOR_DEBUG
                                   		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               		#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
                                   		update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                   		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   		{
                                   		#ifdef MAJOR_DEBUG
                                       		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   		#endif
                                   		}
                               		}
                            	}
                            	else
                            	{
                            	#ifdef MAJOR_DEBUG
                               		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            	#endif
                            	}
								sleep(pinfo.ytime);

								//current phase begin to red lamp
								if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x00))
                            	{
                            	#ifdef MAJOR_DEBUG
                                	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            	#endif
									*(fcdata->markbit) |= 0x0800;
                            	}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															pinfo.cchan,0x00,fcdata->markbit))
								{
								#ifdef MAJOR_DEBUG
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
            							#ifdef MAJOR_DEBUG
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
                                		memset(&madctd,0,sizeof(madctd));
                                		madctd.mode = 29;//traffic control
                                		madctd.pattern = *(fcdata->patternid);
                                		madctd.lampcolor = 0x00;
                                		madctd.lamptime = pinfo.rtime;
                                		madctd.phaseid = pinfo.phaseid;
                                		madctd.stageline = pinfo.stageline;
                                		if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
                                		{
                                		#ifdef MAJOR_DEBUG
                                    		printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                		#endif
                                		}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 29;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x00;
                                        madctd.lamptime = pinfo.rtime;
                                        madctd.phaseid = pinfo.phaseid;
                                        madctd.stageline = pinfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                                    	#ifdef MAJOR_DEBUG
                                        	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                        	update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                        	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                        	{
                                        	#ifdef MAJOR_DEBUG
                                            	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        	#endif
                                        	}
                                    	}
                                	}
                                	else
                                	{
                                	#ifdef MAJOR_DEBUG
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
        						if (SUCCESS!=madc_get_phase_info(fcdata,tscdata,rettl,*(fcdata->slnum),&pinfo))
        						{
        						#ifdef MAJOR_DEBUG
            						printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
									output_log("Timing control,get phase info err");
        						#endif
									madc_end_part_child_thread();
            						return FAIL;
        						}
        						*(fcdata->phaseid) = 0;
        						*(fcdata->phaseid) |= (0x01 << (pinfo.phaseid - 1));

								if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                            	{
                            	#ifdef MAJOR_DEBUG
                                	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            	#endif
									*(fcdata->markbit) |= 0x0800;
                            	}
						
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																pinfo.chan,0x02,fcdata->markbit))
                            	{
                            	#ifdef MAJOR_DEBUG
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
            						#ifdef MAJOR_DEBUG
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
									memset(&madctd,0,sizeof(madctd));
									madctd.mode = 29;//traffic control
									madctd.pattern = *(fcdata->patternid);
									madctd.lampcolor = 0x02;
									madctd.lamptime = 0;
									madctd.phaseid = pinfo.phaseid;
									madctd.stageline = pinfo.stageline;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
									{
									#ifdef MAJOR_DEBUG
										printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef EMBED_CONFIGURE_TOOL
                                if (*(fcdata->markbit2) & 0x0200)
                                {
                                   	memset(&madctd,0,sizeof(madctd));
                                    madctd.mode = 29;//traffic control
                                    madctd.pattern = *(fcdata->patternid);
                                    madctd.lampcolor = 0x02;
                                    madctd.lamptime = 0;
                                    madctd.phaseid = pinfo.phaseid;
                                    madctd.stageline = pinfo.stageline; 
                                    if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
                                    {
                                    #ifdef MAJOR_DEBUG
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
                                	#ifdef MAJOR_DEBUG
                                    	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef MAJOR_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                	}
                            	}
                            	else
                            	{
                            	#ifdef MAJOR_DEBUG
                                	printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            	#endif
                            	}
								}//0 == cp
								if (0 != cp)
								{
									if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
									{//if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
										*(fcdata->slnum) = 0;
										*(fcdata->stageid) = \
												tscdata->timeconfig->TimeConfigList[rettl][0].StageId;
										memset(&pinfo,0,sizeof(pinfo));
										if(SUCCESS!=madc_get_phase_info(fcdata,tscdata,rettl,0,&pinfo))
										{
										#ifdef MAJOR_DEBUG
											printf("phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											madc_end_part_child_thread();
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
												memset(&pinfo,0,sizeof(pinfo));
												if(SUCCESS!=madc_get_phase_info(fcdata,tscdata,rettl,0,&pinfo))
												{
												#ifdef MAJOR_DEBUG
													printf("info err,File: %s,Line: %d\n",__FILE__,__LINE__);
													output_log("Timing control,get phase info err");
												#endif
													madc_end_part_child_thread();
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
											if(SUCCESS!=madc_get_phase_info(fcdata,tscdata,rettl,i+1,&pinfo))
												{
												#ifdef MAJOR_DEBUG
													printf("info err,File: %s,Line: %d\n",__FILE__,__LINE__);
													output_log("Timing control,get phase info err");
												#endif
													madc_end_part_child_thread();
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
											if(SUCCESS != madc_get_phase_info(fcdata,tscdata,rettl,0,&pinfo))
											{
											#ifdef MAJOR_DEBUG
												printf("phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
												output_log("Timing control,get phase info err");
											#endif
												madc_end_part_child_thread();
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
											else if (0xC8 == cp)
                                                sinfo.conmode = 29;
											sinfo.color = 0x02;
											sinfo.time = pinfo.gftime + pinfo.ytime + pinfo.rtime;
											sinfo.stage = pinfo.stageline;
											sinfo.cyclet = 0;
											sinfo.chans = 0;
											if ((0x01 <= cp) && (cp <= 0x20))
                                            {
                                                sinfo.phase = 0;
                                                sinfo.phase |= (0x01 << (cp - 1));
                                            }
                                            else if (((0x5a <= cp) && (cp <= 0x5d)) || (0xC8 == cp))
                                            {
                                                sinfo.phase = 0;
                                            }
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
            								#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
										memset(&madctd,0,sizeof(madctd));
										madctd.mode = 29;//traffic control
										madctd.pattern = *(fcdata->patternid);
										madctd.lampcolor = 0x02;
										madctd.lamptime = pinfo.gftime;
										madctd.phaseid = cp;
										madctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 29;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x02;
                                        madctd.lamptime = pinfo.gftime;
                                        madctd.phaseid = cp;
                                        madctd.stageline = pinfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                                		#ifdef MAJOR_DEBUG
                                    		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                    		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                    		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                    		{
                                    		#ifdef MAJOR_DEBUG
                                        		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
                                    		}
                                		}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
                                		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
	
									ngf = 0;
									if (pinfo.gftime > 0)
									{
									while (1)
									{
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
        								{
        								#ifdef MAJOR_DEBUG
            								printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																		wcc,0x03,fcdata->markbit))
        								{
        								#ifdef MAJOR_DEBUG
            								printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
        								#endif
        								}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);

										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
        								{
        								#ifdef MAJOR_DEBUG
            								printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    										wcc,0x02,fcdata->markbit))
                                		{
                                		#ifdef MAJOR_DEBUG
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
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS!=madc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef MAJOR_DEBUG
            								printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef MAJOR_DEBUG
											printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef MAJOR_DEBUG
											printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														wcc,0x01,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
                               			memset(&madctd,0,sizeof(madctd));
                               			madctd.mode = 29;//traffic control
                               			madctd.pattern = *(fcdata->patternid);
                               			madctd.lampcolor = 0x01;
                               			madctd.lamptime = pinfo.ytime;
                               			madctd.phaseid = cp;
                               			madctd.stageline = pinfo.stageline;
                               			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
                               			{
                               			#ifdef MAJOR_DEBUG
                                   			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                               			#endif
                               			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 29;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x01;
                                        madctd.lamptime = pinfo.ytime;
                                        madctd.phaseid = cp;
                                        madctd.stageline = pinfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                               			#ifdef MAJOR_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef MAJOR_DEBUG
                                       			printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(pinfo.ytime);

									//current phase begin to red lamp
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef MAJOR_DEBUG
                                		printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															wcc,0x00,fcdata->markbit))
									{
									#ifdef MAJOR_DEBUG
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
            								#ifdef MAJOR_DEBUG
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
                                			memset(&madctd,0,sizeof(madctd));
                                			madctd.mode = 29;//traffic control
                                			madctd.pattern = *(fcdata->patternid);
                                			madctd.lampcolor = 0x00;
                                			madctd.lamptime = pinfo.rtime;
                                			madctd.phaseid = cp;
                                			madctd.stageline = pinfo.stageline;
                                			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
                                			{
                                			#ifdef MAJOR_DEBUG
                                    			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                            			}
										#ifdef EMBED_CONFIGURE_TOOL
										if (*(fcdata->markbit2) & 0x0200)
										{
											memset(&madctd,0,sizeof(madctd));
                                            madctd.mode = 29;//traffic control
                                            madctd.pattern = *(fcdata->patternid);
                                            madctd.lampcolor = 0x00;
                                            madctd.lamptime = pinfo.rtime;
                                            madctd.phaseid = cp;
                                            madctd.stageline = pinfo.stageline;	
											if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
											{
											#ifdef MAJOR_DEBUG
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
                                    		#ifdef MAJOR_DEBUG
                                        		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    		#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
                                        		update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                        		if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																			fcdata->softevent,fcdata->markbit))
                                        		{
                                        		#ifdef MAJOR_DEBUG
                                            		printf("file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        		#endif
                                        		}
                                    		}
                                		}
                                		else
                                		{
                                		#ifdef MAJOR_DEBUG
                                    		printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                		#endif
                                		}
										sleep(pinfo.rtime);
									}
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                            		{
                            		#ifdef MAJOR_DEBUG
                               			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			pinfo.chan,0x02,fcdata->markbit))
                            		{
                            		#ifdef MAJOR_DEBUG
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
            							#ifdef MAJOR_DEBUG
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
										memset(&madctd,0,sizeof(madctd));
										madctd.mode = 29;//traffic control
										madctd.pattern = *(fcdata->patternid);
										madctd.lampcolor = 0x02;
										madctd.lamptime = 0;
										madctd.phaseid = pinfo.phaseid;
										madctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
                                        madctd.mode = 29;//traffic control
                                        madctd.pattern = *(fcdata->patternid);
                                        madctd.lampcolor = 0x02;
                                        madctd.lamptime = 0;
                                        madctd.phaseid = pinfo.phaseid;
                                        madctd.stageline = pinfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
                               			#ifdef MAJOR_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef MAJOR_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef MAJOR_DEBUG
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
                                #ifdef MAJOR_DEBUG
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
								madcred = 0;
								close = 0;
								cp = 0;
								if (0 == mayfyes)
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
									int yfret = pthread_create(&mayfid,NULL,(void *)madc_yellow_flash,&yfdata);
									if (0 != yfret)
									{
									#ifdef MAJOR_DEBUG
										printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
										output_log("major detect control,create yellow flash err");
									#endif
										madc_end_part_child_thread();
										objecti[0].objectv[0] = 0x24;
                                		factovs = 0;
                                		memset(cbuf,0,sizeof(cbuf));
                                		if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x00))
                                		{
                                		#ifdef MAJOR_DEBUG
                                    		printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                		#endif
                                		}
                                		if (!(*(fcdata->markbit) & 0x1000))
                                		{
                                    		write(*(fcdata->sockfd->csockfd),cbuf,factovs);
                                		}
										return FAIL;
									}
									mayfyes = 1;
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
            						#ifdef MAJOR_DEBUG
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
                                #ifdef MAJOR_DEBUG
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
								if (1 == mayfyes)
								{
									pthread_cancel(mayfid);
									pthread_join(mayfid,NULL);
									mayfyes = 0;
								}
					
								if (0 == madcred)
								{	
									new_all_red(&ardata);
									madcred = 1;
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
            						#ifdef MAJOR_DEBUG
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
                                #ifdef MAJOR_DEBUG
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
								madcred = 0;
								cp = 0;
								if (1 == mayfyes)
                                {
                                    pthread_cancel(mayfid);
                                    pthread_join(mayfid,NULL);
                                    mayfyes = 0;
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
            						#ifdef MAJOR_DEBUG
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
                                #ifdef MAJOR_DEBUG
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
							{//prepare to lock
							#ifdef MAJOR_DEBUG
								printf("Begin to lock,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								get_madc_status(&color,&leatime);
								if (2 != color)	
								{
									struct timeval		spatime;
									unsigned char		endlock = 0;
									while (1)
									{
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
										get_madc_status(&color,&leatime);
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
								}//if (2 != color)

#if 0
								//the following code is for WuXi check;
                                #ifdef MAJOR_DEBUG
                                printf("*************leatime: %d,mingtime: %d,File:%s,Line:%d\n", \
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
										#ifdef MAJOR_DEBUG
											printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											return FAIL;
										}
										if (0 == mret)
										{
										#ifdef MAJOR_DEBUG
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
								#ifdef MAJOR_DEBUG
								printf("end mintime pass,File: %s,LIne:%d\n",__FILE__,__LINE__);
								#endif
                                //end code of WuXi check;
#endif
								keylock = 1;
								madcred = 0;
								close = 0;
                                cktem = 0;
                                kstep = 0;
								madc_end_child_thread();//end main thread and its child thread;

								*(fcdata->markbit2) |= 0x0002;
							#if 0
								new_all_red(&ardata);
								if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
								{
								#ifdef MAJOR_DEBUG
									printf("set lamp color err,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
									*(fcdata->markbit) |= 0x0800;
									gettimeofday(&ct,NULL);
									update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
									if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
									{
									#ifdef MAJOR_DEBUG
										printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
								}
							#endif
								if (*(fcdata->auxfunc) & 0x01)
                                {//if (*(fcdata->auxfunc) & 0x01)
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),dcdownti,sizeof(dcdownti)) < 0)
                                        {
                                        #ifdef FULL_DETECT_DEBUG
                                            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef FULL_DETECT_DEBUG
                                        printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),dcedownti,sizeof(dcedownti)) < 0)
                                        {
                                        #ifdef FULL_DETECT_DEBUG
                                            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef FULL_DETECT_DEBUG
                                        printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (*(fcdata->auxfunc) & 0x01)					
	
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
            						#ifdef MAJOR_DEBUG
                						printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively
							
								*(fcdata->contmode) = 28; //traffic control mode
								//send current control info to face board
								fbdata[1] = 28;
								fbdata[2] = pinfo.stageline;
								fbdata[3] = 2;
								fbdata[4] = 0;
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
									{
									#ifdef MAJOR_DEBUG
										printf("write face board err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
										if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
								}
								else
								{
								#ifdef MAJOR_DEBUG
									printf("cannot write,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
								sendfaceInfoToBoard(fcdata,fbdata);
								madcecho[1] = 0x01;
								if (*(fcdata->markbit2) & 0x0800)
								{//comes from side door serial
									*(fcdata->markbit2) &= 0xf7ff;
									if (!wait_write_serial(*(fcdata->sdserial)))
                                    {
                                        if (write(*(fcdata->sdserial),madcecho,sizeof(madcecho)) < 0)
                                        {
                                        #ifdef MAJOR_DEBUG
                                            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                            *(fcdata->markbit) |= 0x0800;
                                        }
                                    }
                                    else
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                    #endif
                                    }
								}//comes from side door serial
								else
								{
									if (!wait_write_serial(*(fcdata->fbserial)))
									{
										if (write(*(fcdata->fbserial),madcecho,sizeof(madcecho)) < 0)
										{
										#ifdef MAJOR_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
											update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
											if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																			fcdata->softevent,fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
												printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
											}
										}
									}
									else
									{
									#ifdef MAJOR_DEBUG
										printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
								}

								//send down time to configure tool
                            	if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            	{
                                	memset(&madctd,0,sizeof(madctd));
                                	madctd.mode = 28;
                                	madctd.pattern = *(fcdata->patternid);
                                	madctd.lampcolor = 0x02;
                                	madctd.lamptime = 0;
                                	madctd.phaseid = pinfo.phaseid;
                                	madctd.stageline = pinfo.stageline;
                                	if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
                                	{
                                	#ifdef MAJOR_DEBUG
                                    	printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                	#endif
                                	}
                            	}
								#ifdef EMBED_CONFIGURE_TOOL
                                if (*(fcdata->markbit2) & 0x0200)
                                {
                                   	memset(&madctd,0,sizeof(madctd));
                                    madctd.mode = 28;
                                    madctd.pattern = *(fcdata->patternid);
                                    madctd.lampcolor = 0x02;
                                    madctd.lamptime = 0;
                                    madctd.phaseid = pinfo.phaseid;
                                    madctd.stageline = pinfo.stageline; 
                                    if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                                #endif
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    pinfo.chan,0x02,fcdata->markbit))
                                {
                                #ifdef MAJOR_DEBUG 
                                    printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
                                {//report info to server actively
                                    memset(err,0,sizeof(err));
                                    gettimeofday(&ct,NULL);
                                    if (SUCCESS != err_report(err,ct.tv_sec,22,1))
                                    {
                                    #ifdef MAJOR_DEBUG
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
							#ifdef MAJOR_DEBUG
								printf("prepare to unlock,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								keylock = 0;
								madcred = 0;
								close = 0;
								cktem = 0;
								kstep = 0;
								if (1 == mayfyes)
								{
									pthread_cancel(mayfid);
									pthread_join(mayfid,NULL);
									mayfyes = 0;
								}
								madcecho[1] = 0x00;
								if (*(fcdata->markbit2) & 0x0800)
                                {//comes from side door serial
                                    *(fcdata->markbit2) &= 0xf7ff;
                                    if (!wait_write_serial(*(fcdata->sdserial)))
                                    {
                                        if (write(*(fcdata->sdserial),madcecho,sizeof(madcecho)) < 0)
                                        {
                                        #ifdef MAJOR_DEBUG
                                            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                            *(fcdata->markbit) |= 0x0800;
                                        }
                                    }
                                    else
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                    #endif
                                    }
                                }//comes from side door serial
								else
								{
									if (!wait_write_serial(*(fcdata->fbserial)))
									{
										if (write(*(fcdata->fbserial),madcecho,sizeof(madcecho)) < 0)
										{
										#ifdef MAJOR_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
											update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
											if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																			fcdata->softevent,fcdata->markbit))
											{
											#ifdef MAJOR_DEBUG
												printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
											}
										}
									}
									else
									{
									#ifdef MAJOR_DEBUG
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
            						#ifdef MAJOR_DEBUG
                						printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively

								*(fcdata->markbit2) &= 0xfffd;
								*(fcdata->contmode) = contmode; //restore control mode
								if (0 == mahdyes)
								{
									memset(&mahdata,0,sizeof(mahdata_t));
									memset(&pinfo,0,sizeof(pinfo));
									mahdata.fd = fcdata;
									mahdata.td = tscdata;
									mahdata.pi = &pinfo;
									mahdata.cs = chanstatus;
									int ret = pthread_create(&mahdid,NULL, \
												(void *)start_major_half_detect,&mahdata);	
									if (0 != ret)
									{
									#ifdef MAJOR_DEBUG
										printf("create thread error,File: %s,Line: %d\n",__FILE__,__LINE__);
										output_log("major half detect,create major half detect thread err");
									#endif
										return FAIL;
									}
									mahdyes = 1;
								}
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
                                {//report info to server actively
                                    memset(err,0,sizeof(err));
                                    gettimeofday(&ct,NULL);
                                    if (SUCCESS != err_report(err,ct.tv_sec,22,3))
                                    {
                                    #ifdef MAJOR_DEBUG
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
						#ifdef MAJOR_DEBUG
							printf("tcbuf[1]:%d,File:%s,Line:%d\n",tcbuf[1],__FILE__,__LINE__);
						#endif
							memset(&mdt,0,sizeof(markdata_c));
							mdt.redl = &madcred;
							mdt.closel = &close;
							mdt.rettl = &rettl;
							mdt.yfl = &mayfyes;
							mdt.yfid = &mayfid;
							mdt.ardata = &ardata;
							mdt.fcdata = fcdata;
							mdt.tscdata = tscdata;
							mdt.chanstatus = chanstatus;
							mdt.sinfo = &sinfo;
							madc_jump_stage_control(&mdt,tcbuf[1],&pinfo);	
						}//jump stage control
						if (((0x20 <= tcbuf[1]) && (tcbuf[1] <= 0x27)) && (1 == keylock))
                        {//direction control
                        #ifdef MAJOR_DEBUG
                            printf("tcbuf[1]:%d,cktem:%d,File:%s,Line:%d\n",tcbuf[1],cktem,__FILE__,__LINE__);
                        #endif
							if (cktem == tcbuf[1])
								continue;
							memset(&mdt,0,sizeof(markdata_c));
                            mdt.redl = &madcred;
                            mdt.closel = &close;
                            mdt.rettl = &rettl;
                            mdt.yfl = &mayfyes;
                            mdt.yfid = &mayfid;
                            mdt.ardata = &ardata;
                            mdt.fcdata = fcdata;
                            mdt.tscdata = tscdata;
                            mdt.chanstatus = chanstatus;
                            mdt.sinfo = &sinfo;
							mdt.kstep = &kstep;
							madc_dirch_control(&mdt,cktem,tcbuf[1],dirch,&pinfo);
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
										#ifdef MAJOR_DEBUG
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
										#ifdef MAJOR_DEBUG
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
									memset(&madctd,0,sizeof(madctd));
									madctd.mode = 28;//traffic control
									madctd.pattern = *(fcdata->patternid);
									madctd.lampcolor = 0x02;
									madctd.lamptime = pinfo.gftime;
									madctd.phaseid = 0;
									madctd.stageline = pinfo.stageline;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
									{
									#ifdef MAJOR_DEBUG
										printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
								{
									memset(&madctd,0,sizeof(madctd));
                        			madctd.mode = 28;//traffic control
                        			madctd.pattern = *(fcdata->patternid);
                        			madctd.lampcolor = 0x02;
                        			madctd.lamptime = pinfo.gftime;
                        			madctd.phaseid = 0;
                        			madctd.stageline = pinfo.stageline;	
									if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
									{
									#ifdef MAJOR_DEBUG
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
									#ifdef MAJOR_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
										if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																	fcdata->softevent,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
								}
								else
								{
								#ifdef MAJOR_DEBUG
									printf("face board serial port cannot write,Line:%d\n",__LINE__);
								#endif
								}
								ngf = 0;
								if ((0 != wcc[0]) && (pinfo.gftime > 0))
								{//if ((0 != wcc[0]) && (pinfo.gftime > 0))
									while (1)
									{
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
										{
										#ifdef MAJOR_DEBUG
											printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x03,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
										{
										#ifdef MAJOR_DEBUG
											printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																					wcc,0x02,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
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
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
									{
									#ifdef MAJOR_DEBUG
										printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
									{
									#ifdef MAJOR_DEBUG
										printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
									{
									#ifdef MAJOR_DEBUG
										printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
									{
									#ifdef MAJOR_DEBUG
										printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								else
								{
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
									{
									#ifdef MAJOR_DEBUG
										printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
									{
									#ifdef MAJOR_DEBUG
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
										#ifdef MAJOR_DEBUG
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
									memset(&madctd,0,sizeof(madctd));
									madctd.mode = 28;//traffic control
									madctd.pattern = *(fcdata->patternid);
									madctd.lampcolor = 0x01;
									madctd.lamptime = pinfo.ytime;
									madctd.phaseid = 0;
									madctd.stageline = pinfo.stageline;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
									{
									#ifdef MAJOR_DEBUG
										printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
								{
									memset(&madctd,0,sizeof(madctd));
									madctd.mode = 28;//traffic control
									madctd.pattern = *(fcdata->patternid);
									madctd.lampcolor = 0x01;
									madctd.lamptime = pinfo.ytime;
									madctd.phaseid = 0;
									madctd.stageline = pinfo.stageline;	
									if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
									{
									#ifdef MAJOR_DEBUG
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
									#ifdef MAJOR_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16, \
															ct.tv_sec,fcdata->markbit);
										if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																fcdata->softevent,fcdata->markbit))
										{	
										#ifdef MAJOR_DEBUG
											printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
								}
								else
								{
								#ifdef MAJOR_DEBUG
									printf("face board serial port cannot write,Line:%d\n",__LINE__);
								#endif
								}
								sleep(pinfo.ytime);

								//current phase begin to red lamp
								if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
								{
								#ifdef MAJOR_DEBUG
									printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
									*(fcdata->markbit) |= 0x0800;
								}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																		wcc,0x00,fcdata->markbit))
								{
								#ifdef MAJOR_DEBUG
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
										#ifdef MAJOR_DEBUG
											printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										else
										{
											write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
										}
									}//report info to server actively
								}//if ((0 != wcc[0]) && (pinfo.rtime > 0))
								
								if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                            	{
                            	#ifdef MAJOR_DEBUG
                                	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            	#endif
									*(fcdata->markbit) |= 0x0800;
                            	}
						
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																pinfo.chan,0x02,fcdata->markbit))
                            	{
                            	#ifdef MAJOR_DEBUG
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
									#ifdef MAJOR_DEBUG
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
									memset(&madctd,0,sizeof(madctd));
									madctd.mode = 28;//traffic control
									madctd.pattern = *(fcdata->patternid);
									madctd.lampcolor = 0x02;
									madctd.lamptime = 0;
									madctd.phaseid = pinfo.phaseid;
									madctd.stageline = pinfo.stageline;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
									{
									#ifdef MAJOR_DEBUG
										printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
								{
									memset(&madctd,0,sizeof(madctd));
									madctd.mode = 28;//traffic control
									madctd.pattern = *(fcdata->patternid);
									madctd.lampcolor = 0x02;
									madctd.lamptime = 0;
									madctd.phaseid = pinfo.phaseid;
									madctd.stageline = pinfo.stageline;	
									if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
									{
									#ifdef MAJOR_DEBUG
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
									#ifdef MAJOR_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
										if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																fcdata->softevent,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
								}
								else
								{
								#ifdef MAJOR_DEBUG
									printf("face board serial port cannot write,Line:%d\n",__LINE__);
								#endif
								}							

								madcecho[1] = 0x02;
								if (*(fcdata->markbit2) & 0x1000)
								{
									*(fcdata->markbit2) &= 0xefff;
									if (!wait_write_serial(*(fcdata->sdserial)))
									{
										if (write(*(fcdata->sdserial),madcecho,sizeof(madcecho)) < 0)
										{
										#ifdef MAJOR_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
									}
									else
									{
									#ifdef MAJOR_DEBUG
										printf("face board serial port cannot write,Line:%d\n",__LINE__);
									#endif
									}	
								}//step by step comes from side door serial
								else
								{
									if (!wait_write_serial(*(fcdata->fbserial)))
									{
										if (write(*(fcdata->fbserial),madcecho,sizeof(madcecho)) < 0)
										{
										#ifdef MAJOR_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
									}
									else
									{
									#ifdef MAJOR_DEBUG
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
									#ifdef MAJOR_DEBUG
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
							if ((1 == madcred) || (1 == mayfyes) || (1 == close))
							{
								if (1 == close)
								{
									new_all_red(&ardata);
									close = 0;
								}
								madcred = 0;
                            	if (1 == mayfyes)
                            	{
                                	pthread_cancel(mayfid);
                                	pthread_join(mayfid,NULL);
                                	mayfyes = 0;
									new_all_red(&ardata);
                            	}
				//				#ifdef CLOSE_LAMP
                                madc_set_lamp_color(*(fcdata->bbserial),clch,0x03);
								update_channel_status(fcdata->sockfd,chanstatus,clch,0x03,fcdata->markbit);
               //                 #endif
								if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                                {
                                #ifdef MAJOR_DEBUG
                                    printf("set lamp color err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                    *(fcdata->markbit) |= 0x0800;
                                    gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    			pinfo.chan,0x02,fcdata->markbit))
                                {
                                #ifdef MAJOR_DEBUG 
                                    printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
								madcecho[1] = 0x02;
								if (*(fcdata->markbit2) & 0x1000)
								{
									*(fcdata->markbit2) &= 0xefff;
									if (!wait_write_serial(*(fcdata->sdserial)))
									{
										if (write(*(fcdata->sdserial),madcecho,sizeof(madcecho)) < 0)
										{
										#ifdef MAJOR_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
									}
									else
									{
									#ifdef MAJOR_DEBUG
										printf("face board serial port cannot write,Line:%d\n",__LINE__);
									#endif
									}	
								}//step by step comes from side door serial
								else
								{
									if (!wait_write_serial(*(fcdata->fbserial)))
									{
										if (write(*(fcdata->fbserial),madcecho,sizeof(madcecho)) < 0)
										{
										#ifdef MAJOR_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
									}
									else
									{
									#ifdef MAJOR_DEBUG
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
            						#ifdef MAJOR_DEBUG
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
                                    #ifdef MAJOR_DEBUG
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
									memset(&madctd,0,sizeof(madctd));
									madctd.mode = 28;
									madctd.pattern = *(fcdata->patternid);
                                	madctd.lampcolor = 0x02;
                                	madctd.lamptime = pinfo.mingtime - (ct.tv_sec - wut.tv_sec);
                                	madctd.phaseid = pinfo.phaseid;
                                	madctd.stageline = pinfo.stageline;	
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
									{
									#ifdef MAJOR_DEBUG
										printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								fbdata[2] = pinfo.stageline;
								fbdata[3] = 2;
                            	fbdata[4] = pinfo.mingtime - (ct.tv_sec - wut.tv_sec);
                            	if (!wait_write_serial(*(fcdata->fbserial)))
                            	{
                                	if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                	{
                                	#ifdef MAJOR_DEBUG
                                    	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
										*(fcdata->markbit) |= 0x0800;
                                    	gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    	if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
															fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef MAJOR_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                	}
                            	}
                            	else
                            	{
                            	#ifdef MAJOR_DEBUG
                                	printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            	#endif
                            	}
								sleep(pinfo.mingtime - (ct.tv_sec - wut.tv_sec));
							} 
							//for WuXi check
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
            						#ifdef MAJOR_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively
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
            						#ifdef MAJOR_DEBUG
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
								memset(&madctd,0,sizeof(madctd));
								madctd.mode = 28;
								madctd.pattern = *(fcdata->patternid);
                                madctd.lampcolor = 0x02;
                                madctd.lamptime = pinfo.gftime;
                                madctd.phaseid = pinfo.phaseid;
                                madctd.stageline = pinfo.stageline;	
								if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
								{
								#ifdef MAJOR_DEBUG
									printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#ifdef EMBED_CONFIGURE_TOOL
							if (*(fcdata->markbit2) & 0x0200)
							{
								memset(&madctd,0,sizeof(madctd));
                                madctd.mode = 28;
                                madctd.pattern = *(fcdata->patternid);
                                madctd.lampcolor = 0x02;
                                madctd.lamptime = pinfo.gftime;
                                madctd.phaseid = pinfo.phaseid;
                                madctd.stageline = pinfo.stageline;	
								if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
								{
								#ifdef MAJOR_DEBUG
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
                                #ifdef MAJOR_DEBUG
                                    printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									*(fcdata->markbit) |= 0x0800;
                                    gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                            }
                            else
                            {
                            #ifdef MAJOR_DEBUG
                                printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }
							if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
                            {//report info to server actively
                                memset(err,0,sizeof(err));
                                gettimeofday(&ct,NULL);
                                if (SUCCESS != err_report(err,ct.tv_sec,22,5))
                                {
                                #ifdef MAJOR_DEBUG
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
							if (pinfo.gftime > 0)
							{
								while (1)
								{
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x03))
									{
									#ifdef MAJOR_DEBUG
										printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
										if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
										pinfo.cchan,0x03,fcdata->markbit))
									{
									#ifdef MAJOR_DEBUG 
										printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
									to.tv_sec = 0;
									to.tv_usec = 500000;
									select(0,NULL,NULL,NULL,&to);
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x02))
									{
									#ifdef MAJOR_DEBUG
										printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
										if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																			fcdata->softevent,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
										pinfo.cchan,0x02,fcdata->markbit))
									{
									#ifdef MAJOR_DEBUG
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
							//end green flash
							//begin to yellow lamp
							if (1 == pinfo.cpcexist)
							{
								if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.cnpchan,0x01))
								{
								#ifdef MAJOR_DEBUG
									printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
									*(fcdata->markbit) |= 0x0800;
								}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
									pinfo.cnpchan,0x01,fcdata->markbit))
								{
								#ifdef MAJOR_DEBUG
									printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
								if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.cpchan,0x00))
                                {
                                #ifdef MAJOR_DEBUG
                                    printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
									*(fcdata->markbit) |= 0x0800;
                                }
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    pinfo.cpchan,0x00,fcdata->markbit))
                                {
                                #ifdef MAJOR_DEBUG
                                    printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
							}
							else
							{
								if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x01))
                                {
                                #ifdef MAJOR_DEBUG
                                    printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
									*(fcdata->markbit) |= 0x0800;
                                }
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    pinfo.cchan,0x01,fcdata->markbit))
                                {
                                #ifdef MAJOR_DEBUG
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
            						#ifdef MAJOR_DEBUG
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
                                memset(&madctd,0,sizeof(madctd));
                                madctd.mode = 28;//traffic control
                                madctd.pattern = *(fcdata->patternid);
                                madctd.lampcolor = 0x01;
                                madctd.lamptime = pinfo.ytime;
                                madctd.phaseid = pinfo.phaseid;
                                madctd.stageline = pinfo.stageline;
                                if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
                                {
                                #ifdef MAJOR_DEBUG
                                    printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
                            }
							#ifdef EMBED_CONFIGURE_TOOL
							if (*(fcdata->markbit2) & 0x0200)
							{
								memset(&madctd,0,sizeof(madctd));
                                madctd.mode = 28;//traffic control
                                madctd.pattern = *(fcdata->patternid);
                                madctd.lampcolor = 0x01;
                                madctd.lamptime = pinfo.ytime;
                                madctd.phaseid = pinfo.phaseid;
                                madctd.stageline = pinfo.stageline;	
								if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
								{
								#ifdef MAJOR_DEBUG
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
                                #ifdef MAJOR_DEBUG
                                    printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									*(fcdata->markbit) |= 0x0800;
                                    gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                            }
                            else
                            {
                            #ifdef MAJOR_DEBUG
                                printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }
							sleep(pinfo.ytime);
							//end yellow lamp
							//red lamp
							if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x00))
							{
							#ifdef MAJOR_DEBUG
								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								gettimeofday(&ct,NULL);
                                update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																fcdata->softevent,fcdata->markbit))
                                {
                                #ifdef MAJOR_DEBUG
                                    printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
								*(fcdata->markbit) |= 0x0800;
							}
							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus,pinfo.cchan,\
										0x00,fcdata->markbit))
							{
							#ifdef MAJOR_DEBUG
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
            						#ifdef MAJOR_DEBUG
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
                                    memset(&madctd,0,sizeof(madctd));
                                    madctd.mode = 28;//traffic control
                                    madctd.pattern = *(fcdata->patternid);
                                    madctd.lampcolor = 0x00;
                                    madctd.lamptime = pinfo.rtime;
                                    madctd.phaseid = pinfo.phaseid;
                                    madctd.stageline = pinfo.stageline;
                                    if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
								#ifdef EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
								{
									memset(&madctd,0,sizeof(madctd));
                                    madctd.mode = 28;//traffic control
                                    madctd.pattern = *(fcdata->patternid);
                                    madctd.lampcolor = 0x00;
                                    madctd.lamptime = pinfo.rtime;
                                    madctd.phaseid = pinfo.phaseid;
                                    madctd.stageline = pinfo.stageline;	
									if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
									{
									#ifdef MAJOR_DEBUG
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
                                	#ifdef MAJOR_DEBUG
                                    	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
										*(fcdata->markbit) |= 0x0800;
                                    	gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    	if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef MAJOR_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                	}
                            	}
                            	else
                            	{
                            	#ifdef MAJOR_DEBUG
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
							if (SUCCESS != madc_get_phase_info(fcdata,tscdata,rettl,*(fcdata->slnum),&pinfo))
							{
							#ifdef MAJOR_DEBUG
								printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
								output_log("detect control,get phase info err");
							#endif
								madc_end_part_child_thread();
								return FAIL;
							}
							*(fcdata->phaseid) = 0;
							*(fcdata->phaseid) |= (0x01 << (pinfo.phaseid - 1));
							if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                            {
                            #ifdef MAJOR_DEBUG
                                printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
								gettimeofday(&ct,NULL);
                                update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																fcdata->softevent,fcdata->markbit))
                                {
                                #ifdef MAJOR_DEBUG
                                    printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
								*(fcdata->markbit) |= 0x0800;
                            }

							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus,pinfo.chan,\
                                        0x02,fcdata->markbit))
                            {
                            #ifdef MAJOR_DEBUG
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
            					#ifdef MAJOR_DEBUG
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
								memset(&madctd,0,sizeof(madctd));
								madctd.mode = 28;
								madctd.pattern = *(fcdata->patternid);
                                madctd.lampcolor = 0x02;
                                madctd.lamptime = 0;
                                madctd.phaseid = pinfo.phaseid;
                                madctd.stageline = pinfo.stageline;	
								if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
								{
								#ifdef MAJOR_DEBUG
									printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#ifdef EMBED_CONFIGURE_TOOL
                            if (*(fcdata->markbit2) & 0x0200)
                            {
								memset(&madctd,0,sizeof(madctd));
                                madctd.mode = 28;
                                madctd.pattern = *(fcdata->patternid);
                                madctd.lampcolor = 0x02;
                                madctd.lamptime = 0;
                                madctd.phaseid = pinfo.phaseid;
                                madctd.stageline = pinfo.stageline;
                                if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
                                {
                                #ifdef MAJOR_DEBUG
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
                                #ifdef MAJOR_DEBUG
                                    printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									*(fcdata->markbit) |= 0x0800;
                                    gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                            }
                            else
                            {
                            #ifdef MAJOR_DEBUG
                                printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }

                            madcecho[1] = 0x02;
							if (*(fcdata->markbit2) & 0x1000)
							{
								*(fcdata->markbit2) &= 0xefff;
								if (!wait_write_serial(*(fcdata->sdserial)))
                                {
                                    if (write(*(fcdata->sdserial),madcecho,sizeof(madcecho)) < 0)
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                        *(fcdata->markbit) |= 0x0800;
                                    }
                                }
                                else
                                {
                                #ifdef MAJOR_DEBUG
                                    printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                #endif
                                }	
							}//step by step comes from side door serial
							else
							{
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),madcecho,sizeof(madcecho)) < 0)
									{
									#ifdef MAJOR_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
										if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										*(fcdata->markbit) |= 0x0800;
									}
								}
								else
								{
								#ifdef MAJOR_DEBUG
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
							madcred = 0;
							close = 0;
							if (0 == mayfyes)
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
								int yfret = pthread_create(&mayfid,NULL,(void *)madc_yellow_flash,&yfdata);
								if (0 != yfret)
								{
								#ifdef MAJOR_DEBUG
									printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
									output_log("detect control,create yellow flash err");
								#endif
									madc_end_part_child_thread();
									return FAIL;
								}
								mayfyes = 1;
							}
							madcecho[1] = 0x03;
							if (*(fcdata->markbit2) & 0x2000)
							{
								*(fcdata->markbit2) &= 0xdfff;
								if (!wait_write_serial(*(fcdata->sdserial)))
                                {
                                    if (write(*(fcdata->sdserial),madcecho,sizeof(madcecho)) < 0)
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                        *(fcdata->markbit) |= 0x0800;
                                    }
                                }
                                else
                                {
                                #ifdef MAJOR_DEBUG
                                    printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                #endif
                                }
							}//yellow flash comes from side door serial;
							else
							{
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),madcecho,sizeof(madcecho)) < 0)
									{
									#ifdef MAJOR_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
										if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										*(fcdata->markbit) |= 0x0800;
									}
								}
								else
								{
								#ifdef MAJOR_DEBUG
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
            					#ifdef MAJOR_DEBUG
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
                                #ifdef MAJOR_DEBUG
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
							if (1 == mayfyes)
							{
								pthread_cancel(mayfid);
								pthread_join(mayfid,NULL);
								mayfyes = 0;
							}
							close = 0;
							if (0 == madcred)
							{
								new_all_red(&ardata);	
								madcred = 1;
							}
							madcecho[1] = 0x04;
							if (*(fcdata->markbit2) & 0x4000)
                            {
                                *(fcdata->markbit2) &= 0xbfff;
                                if (!wait_write_serial(*(fcdata->sdserial)))
                                {
                                    if (write(*(fcdata->sdserial),madcecho,sizeof(madcecho)) < 0)
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                        *(fcdata->markbit) |= 0x0800;
                                    }
                                }
                                else
                                {
                                #ifdef MAJOR_DEBUG
                                    printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                #endif
                                }
                            }//all red comes from side door serial
							else
							{
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),madcecho,sizeof(madcecho)) < 0)
									{
									#ifdef MAJOR_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
										if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										*(fcdata->markbit) |= 0x0800;
									}
								}
								else
								{
								#ifdef MAJOR_DEBUG
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
            					#ifdef MAJOR_DEBUG
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
                                #ifdef MAJOR_DEBUG
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
							if (1 == mayfyes)
							{
								pthread_cancel(mayfid);
								pthread_join(mayfid,NULL);
								mayfyes = 0;
							}
							madcred = 0;
							if (0 == close)
							{
								new_all_close(&acdata);	
								close = 1;
							}
							madcecho[1] = 0x05;
							if (!wait_write_serial(*(fcdata->fbserial)))
							{
								if (write(*(fcdata->fbserial),madcecho,sizeof(madcecho)) < 0)
								{
								#ifdef MAJOR_DEBUG
									printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
									*(fcdata->markbit) |= 0x0800;
								}
							}
							else
							{
							#ifdef MAJOR_DEBUG
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
            					#ifdef MAJOR_DEBUG
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
                                #ifdef MAJOR_DEBUG
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
                        #ifdef MAJOR_DEBUG
                        printf("************wlbuf: %s,File: %s,Line: %d\n",wlbuf,__FILE__,__LINE__);
                        #endif
						if (!strcmp("SOCK",wlbuf))
						{//lock or unlock
							if (0 == wllock)
							{//prepare to lock
							#ifdef MAJOR_DEBUG
								printf("Begin to lock,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								get_madc_status(&color,&leatime);
								if (2 != color)	
								{
									struct timeval		spatime;
									unsigned char		endlock = 0;
									while (1)
									{
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

										get_madc_status(&color,&leatime);
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
								}//if (2 != color)

								wllock = 1;
								madcred = 0;
								close = 0;
								dircon = 0;
                                firdc = 1;
                                fdirn = 0;
                                cdirn = 0;
								madc_end_child_thread();//end main thread and its child thread;
								*(fcdata->markbit2) |= 0x0010;
								
								if (*(fcdata->auxfunc) & 0x01)
                                {//if (*(fcdata->auxfunc) & 0x01)
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),dcdownti,sizeof(dcdownti)) < 0)
                                        {
                                        #ifdef FULL_DETECT_DEBUG
                                            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef FULL_DETECT_DEBUG
                                        printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),dcedownti,sizeof(dcedownti)) < 0)
                                        {
                                        #ifdef FULL_DETECT_DEBUG
                                            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef FULL_DETECT_DEBUG
                                        printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (*(fcdata->auxfunc) & 0x01)					
	
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
            						#ifdef MAJOR_DEBUG
                						printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively

								*(fcdata->contmode) = 28; //traffic control mode
								//send current control info to face board
								fbdata[1] = 28;
								fbdata[2] = pinfo.stageline;
								fbdata[3] = 2;
								fbdata[4] = 0;
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
									{
									#ifdef MAJOR_DEBUG
										printf("write face board err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
										if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
								}
								else
								{
								#ifdef MAJOR_DEBUG
									printf("cannot write,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
								sendfaceInfoToBoard(fcdata,fbdata);
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    pinfo.chan,0x02,fcdata->markbit))
                                {
                                #ifdef MAJOR_DEBUG 
                                    printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            	{
                                	memset(wltc,0,sizeof(wltc));
                                	strcpy(wltc,"SOCKBEGIN");
                                	write(*(fcdata->sockfd->ssockfd),wltc,sizeof(wltc));
                                    //send down time to configure tool
                                    memset(&madctd,0,sizeof(madctd));
                                    madctd.mode = 28;
                                    madctd.pattern = *(fcdata->patternid);
                                    madctd.lampcolor = 0x02;
                                    madctd.lamptime = 0;
                                    madctd.phaseid = pinfo.phaseid;
                                    madctd.stageline = pinfo.stageline;
                                    if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                            	}
								#ifdef EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
								{
									memset(&madctd,0,sizeof(madctd));
                                    madctd.mode = 28;
                                    madctd.pattern = *(fcdata->patternid);
                                    madctd.lampcolor = 0x02;
                                    madctd.lamptime = 0;
                                    madctd.phaseid = pinfo.phaseid;
                                    madctd.stageline = pinfo.stageline;
									if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
									{
									#ifdef MAJOR_DEBUG
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
							#ifdef MAJOR_DEBUG
								printf("prepare to unlock,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								wllock = 0;
								madcred = 0;
								close = 0;
								dircon = 0;
                                firdc = 1;
                                fdirn = 0;
                                cdirn = 0;
								if (1 == mayfyes)
								{
									pthread_cancel(mayfid);
									pthread_join(mayfid,NULL);
									mayfyes = 0;
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
            						#ifdef MAJOR_DEBUG
                						printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively

								*(fcdata->contmode) = contmode; //restore control mode
								*(fcdata->markbit2) &= 0xffef;
								if (0 == mahdyes)
								{
									memset(&mahdata,0,sizeof(mahdata_t));
									memset(&pinfo,0,sizeof(pinfo));
									mahdata.fd = fcdata;
									mahdata.td = tscdata;
									mahdata.pi = &pinfo;
									mahdata.cs = chanstatus;
									int ret = pthread_create(&mahdid,NULL, \
												(void *)start_major_half_detect,&mahdata);	
									if (0 != ret)
									{
									#ifdef MAJOR_DEBUG
										printf("create thread error,File: %s,Line: %d\n",__FILE__,__LINE__);
										output_log("major half detect,create major half detect thread err");
									#endif
										return FAIL;
									}
									mahdyes = 1;
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
							if ((1 == madcred) || (1 == mayfyes) || (1 == close))
							{
								if (1 == close)
								{
									close = 0;
									new_all_red(&ardata);
								}
								madcred = 0;
                            	if (1 == mayfyes)
                            	{
                                	pthread_cancel(mayfid);
                                	pthread_join(mayfid,NULL);
                                	mayfyes = 0;
									new_all_red(&ardata);
                            	}
					//			#ifdef CLOSE_LAMP
                                madc_set_lamp_color(*(fcdata->bbserial),clch,0x03);
								update_channel_status(fcdata->sockfd,chanstatus,clch,0x03,fcdata->markbit);
                   //             #endif
								if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                                {
                                #ifdef MAJOR_DEBUG
                                    printf("set lamp color err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                    *(fcdata->markbit) |= 0x0800;
                                    gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    			pinfo.chan,0x02,fcdata->markbit))
                                {
                                #ifdef MAJOR_DEBUG 
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
            						#ifdef MAJOR_DEBUG
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
										#ifdef MAJOR_DEBUG
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
										#ifdef MAJOR_DEBUG
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
									memset(&madctd,0,sizeof(madctd));
									madctd.mode = 28;//traffic control
									madctd.pattern = *(fcdata->patternid);
									madctd.lampcolor = 0x02;
									madctd.lamptime = 3;
									madctd.phaseid = 0;
									madctd.stageline = 0;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
									{
									#ifdef MAJOR_DEBUG
										printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
								{
									memset(&madctd,0,sizeof(madctd));
									madctd.mode = 28;//traffic control
									madctd.pattern = *(fcdata->patternid);
									madctd.lampcolor = 0x02;
									madctd.lamptime = 3;
									madctd.phaseid = 0;
									madctd.stageline = 0;	
									if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
									{
									#ifdef MAJOR_DEBUG
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
									#ifdef MAJOR_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
								}
								else
								{
								#ifdef MAJOR_DEBUG
									printf("face board serial port cannot write,Line:%d\n",__LINE__);
								#endif
								}

								ngf = 0;
								if (pinfo.gftime > 0)
								{
									while (1)
									{//green flash
										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),dcchan,0x03))
										{
										#ifdef MAJOR_DEBUG
											printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;	
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														dcchan,0x03,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);

										if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),dcchan,0x02))
										{
										#ifdef MAJOR_DEBUG
											printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														dcchan,0x02,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
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
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),dcnpchan,0x01))
									{
									#ifdef MAJOR_DEBUG
										printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														dcnpchan,0x01, fcdata->markbit))
									{
									#ifdef MAJOR_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),dcpchan,0x00))
									{
									#ifdef MAJOR_DEBUG
										printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																		dcpchan,0x00,fcdata->markbit))
									{
									#ifdef MAJOR_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
								}
								else
								{
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),dcchan,0x01))
									{
									#ifdef MAJOR_DEBUG
										printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															dcchan,0x01,fcdata->markbit))
									{
									#ifdef MAJOR_DEBUG
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
										#ifdef MAJOR_DEBUG
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
									memset(&madctd,0,sizeof(madctd));
									madctd.mode = 28;//traffic control
									madctd.pattern = *(fcdata->patternid);
									madctd.lampcolor = 0x01;
									madctd.lamptime = 3;
									madctd.phaseid = 0;
									madctd.stageline = 0;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
									{
									#ifdef MAJOR_DEBUG
										printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
								{
									memset(&madctd,0,sizeof(madctd));
									madctd.mode = 28;//traffic control
									madctd.pattern = *(fcdata->patternid);
									madctd.lampcolor = 0x01;
									madctd.lamptime = 3;
									madctd.phaseid = 0;
									madctd.stageline = 0;	
									if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
									{
									#ifdef MAJOR_DEBUG
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
									#ifdef MAJOR_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
									}
								}
								else
								{
								#ifdef MAJOR_DEBUG
									printf("face board serial port cannot write,Line:%d\n",__LINE__);
								#endif
								}

								sleep(3);

								if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),dcchan,0x00))
								{
								#ifdef MAJOR_DEBUG
									printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									*(fcdata->markbit) |= 0x0800;
								}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
														dcchan,0x00,fcdata->markbit))
								{
								#ifdef MAJOR_DEBUG
									printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
								
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
															pinfo.chan,0x02,fcdata->markbit))
								{
								#ifdef MAJOR_DEBUG
									printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
								if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
								{
								#ifdef MAJOR_DEBUG
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
										#ifdef MAJOR_DEBUG
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
										memset(&madctd,0,sizeof(madctd));
										madctd.mode = 28;//traffic control
										madctd.pattern = *(fcdata->patternid);
										madctd.lampcolor = 0x02;
										madctd.lamptime = 0;
										madctd.phaseid = pinfo.phaseid;
                                		madctd.stageline = pinfo.stageline;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&madctd,0,sizeof(madctd));
										madctd.mode = 28;//traffic control
										madctd.pattern = *(fcdata->patternid);
										madctd.lampcolor = 0x02;
										madctd.lamptime = 0;
										madctd.phaseid = pinfo.phaseid;
                                		madctd.stageline = pinfo.stageline;	
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
										{
										#ifdef MAJOR_DEBUG
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
										#ifdef MAJOR_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
									}
									else
									{
									#ifdef MAJOR_DEBUG
										printf("face board serial port cannot write,Line:%d\n",__LINE__);
									#endif
									}							
								}//set success
                                dircon = 0;
								fdirn = 0;
                                continue;
                            }//diredtion control happen

							if ((0==pinfo.cchan[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
							{
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{
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
            						#ifdef MAJOR_DEBUG
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
                                #ifdef MAJOR_DEBUG
                                    printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									*(fcdata->markbit) |= 0x0800;
                                    gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                            }
                            else
                            {
                            #ifdef MAJOR_DEBUG
                                printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }
							//begin to green flash
							if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
							{
								//send down time to configure tool
								memset(&madctd,0,sizeof(madctd));
								madctd.mode = 28;
								madctd.pattern = *(fcdata->patternid);
								madctd.lampcolor = 0x02;
								madctd.lamptime = pinfo.gftime;
								madctd.phaseid = pinfo.phaseid;
								madctd.stageline = pinfo.stageline;
								if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
								{ 
								#ifdef MAJOR_DEBUG
									printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#ifdef EMBED_CONFIGURE_TOOL
							if (*(fcdata->markbit2) & 0x0200)
							{
								memset(&madctd,0,sizeof(madctd));
                                madctd.mode = 28;
                                madctd.pattern = *(fcdata->patternid);
                                madctd.lampcolor = 0x02;
                                madctd.lamptime = pinfo.gftime;
                                madctd.phaseid = pinfo.phaseid;
                                madctd.stageline = pinfo.stageline;
								if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
								{
								#ifdef MAJOR_DEBUG
									printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#endif
							ngf = 0;
							if (pinfo.gftime > 0)
							{
								while (1)
								{
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x03))
									{
									#ifdef MAJOR_DEBUG
										printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
										if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
										pinfo.cchan,0x03,fcdata->markbit))
									{
									#ifdef MAJOR_DEBUG 
										printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
									to.tv_sec = 0;
									to.tv_usec = 500000;
									select(0,NULL,NULL,NULL,&to);
									if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x02))
									{
									#ifdef MAJOR_DEBUG
										printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
										if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																			fcdata->softevent,fcdata->markbit))
										{
										#ifdef MAJOR_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
										pinfo.cchan,0x02,fcdata->markbit))
									{
									#ifdef MAJOR_DEBUG
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
							//end green flash
							//begin to yellow lamp
							if (1 == pinfo.cpcexist)
							{
								if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.cnpchan,0x01))
								{
								#ifdef MAJOR_DEBUG
									printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
									*(fcdata->markbit) |= 0x0800;
								}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
									pinfo.cnpchan,0x01,fcdata->markbit))
								{
								#ifdef MAJOR_DEBUG
									printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
								if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.cpchan,0x00))
                                {
                                #ifdef MAJOR_DEBUG
                                    printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
									*(fcdata->markbit) |= 0x0800;
                                }
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    pinfo.cpchan,0x00,fcdata->markbit))
                                {
                                #ifdef MAJOR_DEBUG
                                    printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
							}
							else
							{
								if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x01))
                                {
                                #ifdef MAJOR_DEBUG
                                    printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
									*(fcdata->markbit) |= 0x0800;
                                }
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    pinfo.cchan,0x01,fcdata->markbit))
                                {
                                #ifdef MAJOR_DEBUG
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
            						#ifdef MAJOR_DEBUG
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
                                #ifdef MAJOR_DEBUG
                                    printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									*(fcdata->markbit) |= 0x0800;
                                    gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                            }
                            else
                            {
                            #ifdef MAJOR_DEBUG
                                printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }
							if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            {   
                                //send down time to configure tool
                                memset(&madctd,0,sizeof(madctd));
                                madctd.mode = 28;
                                madctd.pattern = *(fcdata->patternid);
                                madctd.lampcolor = 0x01;
                                madctd.lamptime = pinfo.ytime;
                                madctd.phaseid = pinfo.phaseid;
                                madctd.stageline = pinfo.stageline;
                                if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
                                { 
                                #ifdef MAJOR_DEBUG
                                    printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
                            }
							#ifdef EMBED_CONFIGURE_TOOL
							if (*(fcdata->markbit2) & 0x0200)
							{
								memset(&madctd,0,sizeof(madctd));
                                madctd.mode = 28;
                                madctd.pattern = *(fcdata->patternid);
                                madctd.lampcolor = 0x01;
                                madctd.lamptime = pinfo.ytime;
                                madctd.phaseid = pinfo.phaseid;
                                madctd.stageline = pinfo.stageline;
								if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
								{
								#ifdef MAJOR_DEBUG
									printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#endif
							sleep(pinfo.ytime);
							//end yellow lamp
							//red lamp
							if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x00))
							{
							#ifdef MAJOR_DEBUG
								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								gettimeofday(&ct,NULL);
                                update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																fcdata->softevent,fcdata->markbit))
                                {
                                #ifdef MAJOR_DEBUG
                                    printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
								*(fcdata->markbit) |= 0x0800;
							}
							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus,pinfo.cchan,\
										0x00,fcdata->markbit))
							{
							#ifdef MAJOR_DEBUG
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
            						#ifdef MAJOR_DEBUG
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
									memset(&madctd,0,sizeof(madctd));
									madctd.mode = 28;
									madctd.pattern = *(fcdata->patternid);
									madctd.lampcolor = 0x00;
									madctd.lamptime = pinfo.rtime;
									madctd.phaseid = pinfo.phaseid;
									madctd.stageline = pinfo.stageline;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&madctd))
									{ 
									#ifdef MAJOR_DEBUG
										printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
								{
									memset(&madctd,0,sizeof(madctd));
                                    madctd.mode = 28;
                                    madctd.pattern = *(fcdata->patternid);
                                    madctd.lampcolor = 0x00;
                                    madctd.lamptime = pinfo.rtime;
                                    madctd.phaseid = pinfo.phaseid;
                                    madctd.stageline = pinfo.stageline;	
									if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&madctd))
                                    { 
                                    #ifdef MAJOR_DEBUG
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
                                	#ifdef MAJOR_DEBUG
                                    	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
										*(fcdata->markbit) |= 0x0800;
                                    	gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    	if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef MAJOR_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                	}
                            	}
                            	else
                            	{
                            	#ifdef MAJOR_DEBUG
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
							if (SUCCESS != madc_get_phase_info(fcdata,tscdata,rettl,*(fcdata->slnum),&pinfo))
							{
							#ifdef MAJOR_DEBUG
								printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
								output_log("detect control,get phase info err");
							#endif
								madc_end_part_child_thread();
								return FAIL;
							}
							*(fcdata->phaseid) = 0;
							*(fcdata->phaseid) |= (0x01 << (pinfo.phaseid - 1));
							if (SUCCESS != madc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                            {
                            #ifdef MAJOR_DEBUG
                                printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
								gettimeofday(&ct,NULL);
                                update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																fcdata->softevent,fcdata->markbit))
                                {
                                #ifdef MAJOR_DEBUG
                                    printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
								*(fcdata->markbit) |= 0x0800;
                            }

							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus,pinfo.chan,\
                                        0x02,fcdata->markbit))
                            {
                            #ifdef MAJOR_DEBUG
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
            					#ifdef MAJOR_DEBUG
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
                                #ifdef MAJOR_DEBUG
                                    printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									*(fcdata->markbit) |= 0x0800;
                                    gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef MAJOR_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                            }
                            else
                            {
                            #ifdef MAJOR_DEBUG
                                printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }
                            
							gettimeofday(&ct,NULL);
                            update_event_list(fcdata->ec,fcdata->el,1,92,ct.tv_sec,fcdata->markbit);

							continue;
						}//step by step
						if ((1 == wllock) && (!strcmp("YF",wlbuf)))
						{//yellow flash
							madcred = 0;
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
							if (0 == mayfyes)
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
								int yfret = pthread_create(&mayfid,NULL,(void *)madc_yellow_flash,&yfdata);
								if (0 != yfret)
								{
								#ifdef MAJOR_DEBUG
									printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
									output_log("detect control,create yellow flash err");
								#endif
									madc_end_part_child_thread();
									return FAIL;
								}
								mayfyes = 1;
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
            					#ifdef MAJOR_DEBUG
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
							if (1 == mayfyes)
							{
								pthread_cancel(mayfid);
								pthread_join(mayfid,NULL);
								mayfyes = 0;
							}
							close = 0;
							if (0 == madcred)
							{
								new_all_red(&ardata);	
								madcred = 1;
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
            					memset(sibuf,0,sizeof(sibuf));
								memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            					if (SUCCESS != status_info_report(sibuf,&sinfo))
            					{
            					#ifdef MAJOR_DEBUG
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
							if (1 == mayfyes)
							{
								pthread_cancel(mayfid);
								pthread_join(mayfid,NULL);
								mayfyes = 0;
							}
							madcred = 0;
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
            					memset(sibuf,0,sizeof(sibuf));
								memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
            					if (SUCCESS != status_info_report(sibuf,&sinfo))
            					{
            					#ifdef MAJOR_DEBUG
                					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            					#endif
            					}
            					else
            					{
                					write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            					}
							}//report info to server actively
							continue;
						}//all red 
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
							mdt.redl = &madcred;
							mdt.closel = &close;
							mdt.rettl = &rettl;
							mdt.dircon = &dircon;
							mdt.firdc = &firdc;
							mdt.yfl = &mayfyes;
							mdt.yfid = &mayfid;
							mdt.ardata = &ardata;
							mdt.fcdata = fcdata;
							mdt.tscdata = tscdata;
							mdt.chanstatus = chanstatus;
							mdt.sinfo = &sinfo;
							madc_mobile_jp_control(&mdt,staid,&pinfo,fdirch);
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
							mdt.redl = &madcred;
							mdt.closel = &close;
							mdt.rettl = &rettl;
							mdt.dircon = &dircon;
							mdt.firdc = &firdc;
							mdt.yfl = &mayfyes;
							mdt.yfid = &mayfid;
							mdt.ardata = &ardata;
							mdt.fcdata = fcdata;
							mdt.tscdata = tscdata;
							mdt.chanstatus = chanstatus;
							mdt.sinfo = &sinfo;
							madc_mobile_direct_control(&mdt,&pinfo,cdirch,fdirch);
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
