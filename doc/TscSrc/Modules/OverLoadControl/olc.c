/*
 *	File:	olc.c
 *	Author:	sk
 *	Date:	20161219
*/
#include "olc.h"

static struct timeval			gtime,gftime,ytime,rtime;
static unsigned char			ocyes = 0;
static pthread_t				ocid;
static unsigned char			ocyfyes = 0;
static pthread_t				ocyfid;
static unsigned char			ocpcyes = 0;
static pthread_t				ocpcid;
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
		
void oc_end_child_thread()
{
	if (1 == ocyfyes)
	{
		pthread_cancel(ocyfid);
		pthread_join(ocyfid,NULL);
		ocyfyes = 0;
	}
	if (1 == ocpcyes)
	{
		pthread_cancel(ocpcid);
		pthread_join(ocpcid,NULL);
		ocpcyes = 0;
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
	if (1 == ocyes)
	{
		pthread_cancel(ocid);
		pthread_join(ocid,NULL);
		ocyes = 0;
	}
}

void oc_end_part_child_thread()
{
	if (1 == ocyfyes)
	{
		pthread_cancel(ocyfid);
		pthread_join(ocyfid,NULL);
		ocyfyes = 0;
	}
	if (1 == ocpcyes)
	{
		pthread_cancel(ocpcid);
		pthread_join(ocpcid,NULL);
		ocpcyes = 0;
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

void oc_yellow_flash(void *arg)
{
    yfdata_t            *yfdata = arg;

    new_yellow_flash(yfdata);

    pthread_exit(NULL);
}

void oc_person_chan_greenflash(void *arg)
{
	ocpcdata_t				*ocpcdata = arg;
	unsigned char			i = 0;
	struct timeval			timeout;

	while (1)
	{
		if (SUCCESS != oc_set_lamp_color(*(ocpcdata->bbserial),ocpcdata->pchan,0x03))
		{
		#ifdef OC_DEBUG
			printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			*(ocpcdata->markbit) |= 0x0800;
		}
		if (SUCCESS != update_channel_status(ocpcdata->sockfd,ocpcdata->cs,ocpcdata->pchan, \
							0x03,ocpcdata->markbit))
		{
		#ifdef OC_DEBUG
			printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;
		select(0,NULL,NULL,NULL,&timeout);

		if (SUCCESS != oc_set_lamp_color(*(ocpcdata->bbserial),ocpcdata->pchan,0x02))
		{
		#ifdef OC_DEBUG
			printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			*(ocpcdata->markbit) |= 0x0800;
		}
		if (SUCCESS != update_channel_status(ocpcdata->sockfd,ocpcdata->cs,ocpcdata->pchan, \
                            0x02,ocpcdata->markbit))
		{
		#ifdef OC_DEBUG
			printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;
		select(0,NULL,NULL,NULL,&timeout);
	}

	pthread_exit(NULL);
}


void start_overload_control(void *arg)
{
	ocdata_t				*ocdata = arg;
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
	if (!wait_write_serial(*(ocdata->fd->bbserial)))
    {
    	if (write(*(ocdata->fd->bbserial),downti,sizeof(downti)) < 0)
        {
		#ifdef OC_DEBUG
			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif       
        }
    }
    else
    {
    #ifdef OC_DEBUG
    	printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
    }
    if (!wait_write_serial(*(ocdata->fd->bbserial)))
    {
    	if (write(*(ocdata->fd->bbserial),edownti,sizeof(edownti)) < 0)
        {
        #ifdef OC_DEBUG
        	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
        }
    }
    else
    {
    #ifdef OC_DEBUG
    	printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
    }

	if (SUCCESS != oc_get_timeconfig(ocdata->td,ocdata->fd->patternid,&tcline))
    {
    #ifdef OC_DEBUG
        printf("oc_get_timeconfig err,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
        oc_end_part_child_thread();
       // return;

		struct timeval				time;
		unsigned char				yferr[10] = {0};
		gettimeofday(&time,NULL);
		update_event_list(ocdata->fd->ec,ocdata->fd->el,1,11,time.tv_sec,ocdata->fd->markbit);
		if ((!(*(ocdata->fd->markbit) & 0x1000)) && (!(*(ocdata->fd->markbit) & 0x8000)))
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
				write(*(ocdata->fd->sockfd->csockfd),yferr,sizeof(yferr));
			}
		}

		yfdata_t					yfdata;
		if (0 == ocyfyes)
		{
			memset(&yfdata,0,sizeof(yfdata));
			yfdata.second = 0;
			yfdata.markbit = ocdata->fd->markbit;
			yfdata.markbit2 = ocdata->fd->markbit2;
			yfdata.serial = *(ocdata->fd->bbserial);
			yfdata.sockfd = ocdata->fd->sockfd;
			yfdata.cs = ocdata->cs;		
#ifdef FLASH_DEBUG
			char szInfo[32] = {0};
			char szInfoT[64] = {0};
			snprintf(szInfo,sizeof(szInfo)-1,"%s",__FUNCTION__);
			snprintf(szInfoT,sizeof(szInfoT)-1,"%d",__LINE__);
			tsc_save_eventlog(szInfo,szInfoT);
#endif
			int yfret = pthread_create(&ocyfid,NULL,(void *)oc_yellow_flash,&yfdata);
			if (0 != yfret)
			{
		#ifdef TIMING_DEBUG
				printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
				output_log("oc_yellow_flash control,create yellow flash err");
		#endif
				oc_end_part_child_thread();
				return;
			}
			ocyfyes = 1;
		}		
		while(1)
		{
			if (*(ocdata->fd->markbit) & 0x0001)
			{//end time of current pattern has arrived
				unsigned char           enddata[3] = {0xCC,0xDD,0xED};
				if (!wait_write_serial(*(ocdata->fd->synpipe)))
				{
					if (write(*(ocdata->fd->synpipe),enddata,sizeof(enddata)) < 0)
					{
				#ifdef FULL_DETECT_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
						oc_end_part_child_thread();
						return;
					}
				}
				else
				{
			#ifdef FULL_DETECT_DEBUG
					printf("pipe cannot be written,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
					oc_end_part_child_thread();
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
		if (0 == ocdata->td->timeconfig->TimeConfigList[tcline][snum].StageId)
			break;
		get_phase_id(ocdata->td->timeconfig->TimeConfigList[tcline][snum].PhaseId,&tphid);
		for (i = 0; i < (ocdata->td->phase->FactPhaseNum); i++)
		{
			if (0 == (ocdata->td->phase->PhaseList[i].PhaseId))
				break;
			if (tphid == (ocdata->td->phase->PhaseList[i].PhaseId))
			{
				pphdetect->phaseid = tphid;
				pphdetect->phasetype = ocdata->td->phase->PhaseList[i].PhaseType;
				if (0x04 == (ocdata->td->phase->PhaseList[i].PhaseType))
					personpid = ocdata->td->phase->PhaseList[i].PhaseId;
				tnum = ocdata->td->phase->PhaseList[i].PhaseSpecFunc;
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
					for (z = 0; z < (ocdata->td->detector->FactDetectorNum); z++)
					{//1
						if (0 == (ocdata->td->detector->DetectorList[z].DetectorId))
							break;
						if ((pphdetect->phaseid) == (ocdata->td->detector->DetectorList[z].DetectorPhase))
						{
							dpro->deteid = ocdata->td->detector->DetectorList[z].DetectorId;
							dpro->detetype = ocdata->td->detector->DetectorList[z].DetectorType;
							dpro->validmark = 0;
							dpro->err = 0;
							tnum = ocdata->td->detector->DetectorList[z].DetectorSpecFunc;
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
		}//for (i = 0; i < (ocdata->td->phase->FactPhaseNum); i++)
	}//for (snum = 0; ;snum++)

	slnum = /**(ocdata->fd->slnum)*/0;
	*(ocdata->fd->slnum) = 0;
	*(ocdata->fd->stageid) = ocdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId;

	unsigned char				cycarr = 0;
	ocpinfo_t					*pinfo = ocdata->pi;	
	ocpcdata_t					ocpcdata;
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
	sinfo.conmode = *(ocdata->fd->contmode);
    sinfo.pattern = *(ocdata->fd->patternid);
    sinfo.cyclet = *(ocdata->fd->cyclet);

	while (1)
	{//while loop
		if (1 == cycarr)
		{
			cycarr = 0;

			if (*(ocdata->fd->markbit) & 0x0001)
			{//end time of current pattern has arrived
				unsigned char			syndata[3] = {0xCC,0xDD,0xED};
				if (!wait_write_serial(*(ocdata->fd->synpipe)))
				{
					if (write(*(ocdata->fd->synpipe),syndata,sizeof(syndata)) < 0)
					{
					#ifdef OC_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						oc_end_part_child_thread();
						return;
					}
				}
				else
				{
				#ifdef OC_DEBUG
					printf("pipe cannot be written,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					oc_end_part_child_thread();
					return;
				}
				//Note, 0th bit is clean 0 by main module
				sleep(5);//wait main module end own;
			}//end time of current pattern has arrived

			if ((*(ocdata->fd->markbit) & 0x0100) && (0 == endahead))
			{//current pattern transit ahead two cycles
				//Note, 8th bit is clean 0 by main module
				if (0 != *(ocdata->fd->ncoordphase))
				{//next coordphase is not 0
					endahead = 1;
					endcyclen = 0;
					lefttime = 0;
					gettimeofday(&ct,NULL);
					lefttime = (unsigned int)((ocdata->fd->nst) - ct.tv_sec);
					#ifdef OC_DEBUG
					printf("*****************lefttime: %d,File: %s,Line: %d\n",lefttime,__FILE__,__LINE__);
					#endif
					if (lefttime >= (*(ocdata->fd->cyclet)*3)/2)
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
					#ifdef OC_DEBUG
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
		
		memset(pinfo,0,sizeof(ocpinfo_t));
		if (SUCCESS != oc_get_phase_info(ocdata->fd,ocdata->td,tcline,slnum,pinfo))
		{
		#ifdef OC_DEBUG
			printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			oc_end_part_child_thread();
			return;
		}
		*(ocdata->fd->phaseid) = 0;
		*(ocdata->fd->phaseid) |= (0x01 << (pinfo->phaseid - 1));
		sinfo.stage = *(ocdata->fd->stageid);
        sinfo.phase = *(ocdata->fd->phaseid);
		if ((*(ocdata->fd->markbit) & 0x0100) && (1 == endahead))
		{//end pattern ahead two cycles
			#ifdef OC_DEBUG
			printf("End pattern two cycles AHEAD, begin %d cycle,Line:%d\n",endcyclen,__LINE__);
			#endif
			*(ocdata->fd->color) = 0x02;
			*(ocdata->fd->markbit) &= 0xfbff;
			if (SUCCESS != oc_set_lamp_color(*(ocdata->fd->bbserial),pinfo->chan,0x02))
           	{
           	#ifdef OC_DEBUG
               	printf("set green err,File: %s,Line: %d\n",__FILE__,__LINE__);
           	#endif
               	gettimeofday(&ct,NULL);
               	update_event_list(ocdata->fd->ec,ocdata->fd->el,1,15,ct.tv_sec,ocdata->fd->markbit);
               	if (SUCCESS != generate_event_file(ocdata->fd->ec,ocdata->fd->el,\
														ocdata->fd->softevent,ocdata->fd->markbit))
               	{
               	#ifdef OC_DEBUG
                   	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
               	#endif
               	}
				*(ocdata->fd->markbit) |= 0x0800;
           	}
           	memset(&gtime,0,sizeof(gtime));
           	gettimeofday(&gtime,NULL);
           	memset(&gftime,0,sizeof(gftime));
           	memset(&ytime,0,sizeof(ytime));
           	memset(&rtime,0,sizeof(rtime));

           	if (SUCCESS != update_channel_status(ocdata->fd->sockfd,ocdata->cs,pinfo->chan,0x02, \
													ocdata->fd->markbit))
            {
            #ifdef OC_DEBUG
               	printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
            #endif
            }
			if (0 == pinfo->cchan[0])
			{
				if ((!(*(ocdata->fd->markbit) & 0x1000)) && (!(*(ocdata->fd->markbit) & 0x8000)))
				{//if ((!(*(ocdata->fd->markbit) & 0x1000)) && (!(*(ocdata->fd->markbit) & 0x8000)))
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
					sinfo.conmode = *(ocdata->fd->contmode);//added on 20150529
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
					memcpy(ocdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            		memset(sibuf,0,sizeof(sibuf));
            		if (SUCCESS != status_info_report(sibuf,&sinfo))
            		{
            		#ifdef OC_DEBUG
                		printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            		#endif
            		}
            		else
            		{
                		write(*(ocdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            		}	
				}//if ((!(*(ocdata->fd->markbit) & 0x1000)) && (!(*(ocdata->fd->markbit) & 0x8000)))
			}
			else
			{
				if ((!(*(ocdata->fd->markbit) & 0x1000)) && (!(*(ocdata->fd->markbit) & 0x8000)))
				{//if ((!(*(ocdata->fd->markbit) & 0x1000)) && (!(*(ocdata->fd->markbit) & 0x8000)))
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
					sinfo.conmode = *(ocdata->fd->contmode);//added on 20150529
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
					memcpy(ocdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            		memset(sibuf,0,sizeof(sibuf));
            		if (SUCCESS != status_info_report(sibuf,&sinfo))
            		{
            		#ifdef OC_DEBUG
                		printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            		#endif
            		}
            		else
            		{
                		write(*(ocdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            		}	
				}//if ((!(*(ocdata->fd->markbit) & 0x1000)) && (!(*(ocdata->fd->markbit) & 0x8000)))
			}

			if ((*(ocdata->fd->markbit) & 0x0002) && (*(ocdata->fd->markbit) & 0x0010))
            {//send down time data to configure tool
               	memset(&timedown,0,sizeof(timedown));
               	timedown.mode = *(ocdata->fd->contmode);
               	timedown.pattern = *(ocdata->fd->patternid);
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
               	if (SUCCESS != timedown_data_to_conftool(ocdata->fd->sockfd,&timedown))
               	{
               	#ifdef OC_DEBUG
                   	printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
               	#endif
               	}
			}//send down time data to configure tool
			#ifdef EMBED_CONFIGURE_TOOL
			if (*(ocdata->fd->markbit2) & 0x0200)
			{   
				memset(&timedown,0,sizeof(timedown));
               	timedown.mode = *(ocdata->fd->contmode);
               	timedown.pattern = *(ocdata->fd->patternid);
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
				if (SUCCESS != timedown_data_to_embed(ocdata->fd->sockfd,&timedown))
				{
				#ifdef OC_DEBUG
					printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#endif

			//send down time data to face board
			if (*(ocdata->fd->contmode) < 27)
				fbdata[1] = *(ocdata->fd->contmode) + 1;
			else
				fbdata[1] = *(ocdata->fd->contmode);
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
			if (!wait_write_serial(*(ocdata->fd->fbserial)))
            {
               	if (write(*(ocdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
               	{
               	#ifdef OC_DEBUG
                   	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
               	#endif
					*(ocdata->fd->markbit) |= 0x0800;
                   	gettimeofday(&ct,NULL);
                   	update_event_list(ocdata->fd->ec,ocdata->fd->el,1,16,ct.tv_sec,ocdata->fd->markbit);
                   	if (SUCCESS != generate_event_file(ocdata->fd->ec,ocdata->fd->el,\
														ocdata->fd->softevent,ocdata->fd->markbit))
                   	{
                   	#ifdef OC_DEBUG
                       	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                   	#endif
                   	}
               	}
            }
            else
            {
            #ifdef OC_DEBUG
               	printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
			sendfaceInfoToBoard(ocdata->fd,fbdata);
			if (1 == endcyclen)
			{
				if (snum == (slnum + 1))
				{
					sltime = c1gt + c1gmt;
				}
				else
				{
					sltime = c1gt;
				}
			}
			if (2 == endcyclen)
			{
				if (snum == (slnum + 1))
				{
					sltime = c2gt + c2gmt;
				}
				else
				{
					sltime = c2gt;
				}
			}

			while (1)
			{//while (1)
				FD_ZERO(&nRead);
				FD_SET(*(ocdata->fd->ffpipe),&nRead);	
				lasttime.tv_sec = 0;
				lasttime.tv_usec = 0;
				gettimeofday(&lasttime,NULL);
				mtime.tv_sec = sltime;
				mtime.tv_usec = 0;
				int mret = select(*(ocdata->fd->ffpipe)+1,&nRead,NULL,NULL,&mtime);
				if (mret < 0)
				{
				#ifdef OC_DEBUG
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
					if (FD_ISSET(*(ocdata->fd->ffpipe),&nRead))
					{
						memset(buf,0,sizeof(buf));
						read(*(ocdata->fd->ffpipe),buf,sizeof(buf));
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
				if ((!(*(ocdata->fd->markbit) & 0x1000)) && (!(*(ocdata->fd->markbit) & 0x8000)))
        		{//actively report is not probitted and connect successfully
					if (0x04 == (pinfo->phasetype))
					{
						sinfo.time = 6;
					}
					else
					{
						sinfo.time = 3;
					}
					sinfo.conmode = *(ocdata->fd->contmode);//added on 20150529
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
					memcpy(ocdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            		memset(sibuf,0,sizeof(sibuf));
            		if (SUCCESS != status_info_report(sibuf,&sinfo))
            		{
            		#ifdef OC_DEBUG
                		printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            		#endif
            		}
            		else
            		{
                		write(*(ocdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            		}
				}//actively report is not probitted and connect successfully
			}

			memset(&gtime,0,sizeof(gtime));
			memset(&gftime,0,sizeof(gftime));
			gettimeofday(&gftime,NULL);
			memset(&ytime,0,sizeof(ytime));
			memset(&rtime,0,sizeof(rtime));
			*(ocdata->fd->markbit) |= 0x0400;
			gft = 0;
			while (1)
			{
				if (SUCCESS != oc_set_lamp_color(*(ocdata->fd->bbserial),pinfo->cchan,0x03))
				{
				#ifdef OC_DEBUG
					printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					gettimeofday(&ct,NULL);
               		update_event_list(ocdata->fd->ec,ocdata->fd->el,1,15,ct.tv_sec,ocdata->fd->markbit);
               		if (SUCCESS != generate_event_file(ocdata->fd->ec,ocdata->fd->el,\
														ocdata->fd->softevent,ocdata->fd->markbit))
               		{
               		#ifdef OC_DEBUG
                   		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
               		#endif
               		}
					*(ocdata->fd->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(ocdata->fd->sockfd,ocdata->cs,pinfo->cchan,0x03, \
												ocdata->fd->markbit))
				{
				#ifdef OC_DEBUG
					printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}	
				timeout.tv_sec = 0;
				timeout.tv_usec = 500000;
				select(0,NULL,NULL,NULL,&timeout);
				if (SUCCESS != oc_set_lamp_color(*(ocdata->fd->bbserial),pinfo->cchan,0x02))
				{
				#ifdef OC_DEBUG
					printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					gettimeofday(&ct,NULL);
                	update_event_list(ocdata->fd->ec,ocdata->fd->el,1,15,ct.tv_sec,ocdata->fd->markbit);
                	if (SUCCESS != generate_event_file(ocdata->fd->ec,ocdata->fd->el,\
														ocdata->fd->softevent,ocdata->fd->markbit))
                	{
                	#ifdef OC_DEBUG
                   		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                	}
					*(ocdata->fd->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(ocdata->fd->sockfd,ocdata->cs,pinfo->cchan,0x02, \
                                                   ocdata->fd->markbit))
                {
                #ifdef OC_DEBUG
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
                *(ocdata->fd->markbit) &= 0xfbff;
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
					if (SUCCESS != oc_set_lamp_color(*(ocdata->fd->bbserial),pinfo->cpchan,0x00))
					{
					#ifdef OC_DEBUG
						printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						gettimeofday(&ct,NULL);
               			update_event_list(ocdata->fd->ec,ocdata->fd->el,1,15,ct.tv_sec,ocdata->fd->markbit);
               			if (SUCCESS != generate_event_file(ocdata->fd->ec,ocdata->fd->el,\
															ocdata->fd->softevent,ocdata->fd->markbit))
               			{
               			#ifdef OC_DEBUG
                   			printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
               			#endif
               			}
						*(ocdata->fd->markbit) |= 0x0800;
					}
					if (SUCCESS != update_channel_status(ocdata->fd->sockfd,ocdata->cs,pinfo->cpchan,0x00, \
                                                   ocdata->fd->markbit))
               		{
               		#ifdef OC_DEBUG
                   		printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
               		#endif
               		}	
					//other change channels will be set yellow lamp;
					if (SUCCESS != oc_set_lamp_color(*(ocdata->fd->bbserial),pinfo->cnpchan,0x01))
					{
					#ifdef OC_DEBUG
						printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						gettimeofday(&ct,NULL);
                   		update_event_list(ocdata->fd->ec,ocdata->fd->el,1,15,ct.tv_sec,ocdata->fd->markbit);
                   		if (SUCCESS != generate_event_file(ocdata->fd->ec,ocdata->fd->el,\
																ocdata->fd->softevent,ocdata->fd->markbit))
                   		{
                   		#ifdef OC_DEBUG
                       		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                   		#endif
                   		}
						*(ocdata->fd->markbit) |= 0x0800;
					}
					if (SUCCESS != update_channel_status(ocdata->fd->sockfd,ocdata->cs,pinfo->cnpchan,0x01, \
                                                   ocdata->fd->markbit))
               		{
               		#ifdef OC_DEBUG
                   		printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
               		#endif
               		}
				}//person channels exist in current phase
				else
				{//Not person channels in current phase
					if (SUCCESS != oc_set_lamp_color(*(ocdata->fd->bbserial),pinfo->cchan,0x01))
					{
					#ifdef OC_DEBUG
						printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						gettimeofday(&ct,NULL);
                   		update_event_list(ocdata->fd->ec,ocdata->fd->el,1,15,ct.tv_sec,ocdata->fd->markbit);
                   		if (SUCCESS != generate_event_file(ocdata->fd->ec,ocdata->fd->el,\
															ocdata->fd->softevent,ocdata->fd->markbit))
                   		{
                   		#ifdef OC_DEBUG
                       		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                   		#endif
                   		}
						*(ocdata->fd->markbit) |= 0x0800;
					}
					if (SUCCESS != update_channel_status(ocdata->fd->sockfd,ocdata->cs,pinfo->cchan,0x01, \
                                                   ocdata->fd->markbit))
               		{
               		#ifdef OC_DEBUG
                   		printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
               		#endif
               		}
				}//Not person channels in current phase

				if (0 != pinfo->cchan[0])
				{
					if ((!(*(ocdata->fd->markbit) & 0x1000)) && (!(*(ocdata->fd->markbit) & 0x8000)))
        			{//actively report is not probitted and connect successfully
						sinfo.time = 3;
						sinfo.color = 0x01;
						sinfo.conmode = *(ocdata->fd->contmode);//added on 20150529
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
						memcpy(ocdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            			memset(sibuf,0,sizeof(sibuf));
            			if (SUCCESS != status_info_report(sibuf,&sinfo))
            			{
            			#ifdef OC_DEBUG
                			printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            			#endif
            			}
            			else
            			{
                			write(*(ocdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            			}
					}//actively report is not probitted and connect successfully			
				}

				if ((*(ocdata->fd->markbit) & 0x0002) && (*(ocdata->fd->markbit) & 0x0010))
           		{
               		memset(&timedown,0,sizeof(timedown));
               		timedown.mode = *(ocdata->fd->contmode);
               		timedown.pattern = *(ocdata->fd->patternid);
               		timedown.lampcolor = 0x01;
					timedown.lamptime = 3;	
               		timedown.phaseid = pinfo->phaseid;
               		timedown.stageline = pinfo->stageline;
               		if (SUCCESS != timedown_data_to_conftool(ocdata->fd->sockfd,&timedown))
               		{
               		#ifdef OC_DEBUG
                   		printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
               		#endif
               		}
           		}
				#ifdef EMBED_CONFIGURE_TOOL
                if (*(ocdata->fd->markbit2) & 0x0200)
                {   
                    memset(&timedown,0,sizeof(timedown));
                    timedown.mode = *(ocdata->fd->contmode);
                    timedown.pattern = *(ocdata->fd->patternid);
                    timedown.lampcolor = 0x01;
                    timedown.lamptime = 3;
                    timedown.phaseid = pinfo->phaseid;
                    timedown.stageline = pinfo->stageline;
                    if (SUCCESS != timedown_data_to_embed(ocdata->fd->sockfd,&timedown))
                    {
                    #ifdef OC_DEBUG
                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                    #endif
                    }
                }
                #endif
				//send info to face board
				if (*(ocdata->fd->contmode) < 27)
                	fbdata[1] = *(ocdata->fd->contmode) + 1;
            	else
                	fbdata[1] = *(ocdata->fd->contmode);
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
           		if (!wait_write_serial(*(ocdata->fd->fbserial)))
           		{
		       		if (write(*(ocdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
               		{
               		#ifdef OC_DEBUG
                   		printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
               		#endif
						*(ocdata->fd->markbit) |= 0x0800;
                   		gettimeofday(&ct,NULL);
                   		update_event_list(ocdata->fd->ec,ocdata->fd->el,1,16,ct.tv_sec,ocdata->fd->markbit);
                   		if (SUCCESS != generate_event_file(ocdata->fd->ec,ocdata->fd->el,\
															ocdata->fd->softevent,ocdata->fd->markbit))
                   		{
                   		#ifdef OC_DEBUG
                       		printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                   		#endif
                   		}
               		}
           		}
           		else
           		{
           		#ifdef OC_DEBUG
               		printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
           		#endif
           		}
				sendfaceInfoToBoard(ocdata->fd,fbdata);
				*(ocdata->fd->color) = 0x01;
				memset(&gtime,0,sizeof(gtime));
           		memset(&gftime,0,sizeof(gftime));
           		memset(&ytime,0,sizeof(ytime));
				gettimeofday(&ytime,NULL);
           		memset(&rtime,0,sizeof(rtime));
				sleep(3);
				//end set yellow lamp
			}//person phase do not have yellow lamp

			//Begin to set red lamp
			if (SUCCESS != oc_set_lamp_color(*(ocdata->fd->bbserial),pinfo->cchan,0x00))
			{
			#ifdef OC_DEBUG
				printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				gettimeofday(&ct,NULL);
               	update_event_list(ocdata->fd->ec,ocdata->fd->el,1,15,ct.tv_sec,ocdata->fd->markbit);
               	if (SUCCESS != generate_event_file(ocdata->fd->ec,ocdata->fd->el,\
													ocdata->fd->softevent,ocdata->fd->markbit))
               	{
               	#ifdef OC_DEBUG
                   	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
               	#endif
               	}
				*(ocdata->fd->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(ocdata->fd->sockfd,ocdata->cs,pinfo->cchan,0x00, \
											ocdata->fd->markbit))
           	{
           	#ifdef OC_DEBUG
               	printf("update chan err,File: %s,Line: %d\n",__FILE__,__LINE__);
           	#endif
           	}
			*(ocdata->fd->color) = 0x00;

			if ((!(*(ocdata->fd->markbit) & 0x1000)) && (!(*(ocdata->fd->markbit) & 0x8000)))
        	{//actively report is not probitted and connect successfully
				sinfo.time = 0;
				sinfo.color = 0x00;
				sinfo.conmode = *(ocdata->fd->contmode);//added on 20150529
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
				memcpy(ocdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            	memset(sibuf,0,sizeof(sibuf));
            	if (SUCCESS != status_info_report(sibuf,&sinfo))
            	{
            	#ifdef OC_DEBUG
                	printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            	#endif
            	}
            	else
            	{
                	write(*(ocdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            	}
			}//actively report is not probitted and connect successfully

			//end red lamp,red lamp time is default 0 in the last two cycles;
			if (1 == phcon)
            {
				*(ocdata->fd->markbit) &= 0xfbff;
                memset(&gtime,0,sizeof(gtime));
                gettimeofday(&gtime,NULL);
                memset(&gftime,0,sizeof(gftime));
                memset(&ytime,0,sizeof(ytime));
                memset(&rtime,0,sizeof(rtime));
                phcon = 0;
                sleep(10);
            }

			slnum += 1;
			*(ocdata->fd->slnum) = slnum;
			*(ocdata->fd->stageid) = ocdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
			if (0 == (ocdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId))
			{
				cycarr = 1;
				slnum = 0;
				*(ocdata->fd->slnum) = 0;
				*(ocdata->fd->stageid) = ocdata->td->timeconfig->TimeConfigList[tcline][slnum].StageId;
			}				
			continue;
		}//end pattern ahead two cycles		

		if (0x01 == (pinfo->phasetype))
		{//vehicle phase
			*(ocdata->fd->color) = 0x02;
			*(ocdata->fd->markbit) &= 0xfbff;
			if (SUCCESS != oc_set_lamp_color(*(ocdata->fd->bbserial),pinfo->chan,0x02))
			{
			#ifdef OC_DEBUG
				printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				gettimeofday(&ct,NULL);
                update_event_list(ocdata->fd->ec,ocdata->fd->el,1,15,ct.tv_sec,ocdata->fd->markbit);
                if (SUCCESS != generate_event_file(ocdata->fd->ec,ocdata->fd->el,\
														ocdata->fd->softevent,ocdata->fd->markbit))
                {
                #ifdef OC_DEBUG
                    printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
                *(ocdata->fd->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(ocdata->fd->sockfd,ocdata->cs,pinfo->chan,0x02, \
                                                    ocdata->fd->markbit))
            {
            #ifdef OC_DEBUG
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
				if ((!(*(ocdata->fd->markbit) & 0x1000)) && (!(*(ocdata->fd->markbit) & 0x8000)))
				{//if ((!(*(ocdata->fd->markbit) & 0x1000)) && (!(*(ocdata->fd->markbit) & 0x8000)))
					sinfo.time = pinfo->mingtime + pinfo->gftime + pinfo->ytime + pinfo->rtime;	
					sinfo.color = 0x02;
					sinfo.conmode = *(ocdata->fd->contmode);//added on 20150529
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
					memcpy(ocdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            		memset(sibuf,0,sizeof(sibuf));
            		if (SUCCESS != status_info_report(sibuf,&sinfo))
            		{	
            		#ifdef OC_DEBUG
                		printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            		#endif
            		}
            		else
            		{
                		write(*(ocdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            		}	
				}//if ((!(*(ocdata->fd->markbit) & 0x1000)) && (!(*(ocdata->fd->markbit) & 0x8000)))
			}
			else
			{
				if ((!(*(ocdata->fd->markbit) & 0x1000)) && (!(*(ocdata->fd->markbit) & 0x8000)))
				{//if ((!(*(ocdata->fd->markbit) & 0x1000)) && (!(*(ocdata->fd->markbit) & 0x8000)))
					sinfo.time = pinfo->mingtime + pinfo->gftime;	
					sinfo.color = 0x02;
					sinfo.conmode = *(ocdata->fd->contmode);//added on 20150529
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
					memcpy(ocdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            		memset(sibuf,0,sizeof(sibuf));
            		if (SUCCESS != status_info_report(sibuf,&sinfo))
            		{	
            		#ifdef OC_DEBUG
                		printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            		#endif
            		}
            		else
            		{
                		write(*(ocdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            		}	
				}//if ((!(*(ocdata->fd->markbit) & 0x1000)) && (!(*(ocdata->fd->markbit) & 0x8000)))	
			}

			if ((*(ocdata->fd->markbit) & 0x0002) && (*(ocdata->fd->markbit) & 0x0010))
            {
                memset(&timedown,0,sizeof(timedown));
                timedown.mode = *(ocdata->fd->contmode);
                timedown.pattern = *(ocdata->fd->patternid);
                timedown.lampcolor = 0x02;
                timedown.lamptime = pinfo->mingtime + pinfo->gftime;
                timedown.phaseid = pinfo->phaseid;
                timedown.stageline = pinfo->stageline;
                if (SUCCESS != timedown_data_to_conftool(ocdata->fd->sockfd,&timedown))
                {
                #ifdef OC_DEBUG
                    printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                #endif
                }
            }
			#ifdef EMBED_CONFIGURE_TOOL
			if (*(ocdata->fd->markbit2) & 0x0200)
			{   
				memset(&timedown,0,sizeof(timedown));
                timedown.mode = *(ocdata->fd->contmode);
                timedown.pattern = *(ocdata->fd->patternid);
                timedown.lampcolor = 0x02;
                timedown.lamptime = pinfo->mingtime + pinfo->gftime;
                timedown.phaseid = pinfo->phaseid;
                timedown.stageline = pinfo->stageline;	 
				if (SUCCESS != timedown_data_to_embed(ocdata->fd->sockfd,&timedown))
				{
				#ifdef OC_DEBUG
					printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#endif
			//send info to face board
			if (*(ocdata->fd->contmode) < 27)
                fbdata[1] = *(ocdata->fd->contmode) + 1;
            else
                fbdata[1] = *(ocdata->fd->contmode);
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
            if (!wait_write_serial(*(ocdata->fd->fbserial)))
            {
                if (write(*(ocdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
                {
                #ifdef OC_DEBUG
                    printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
					*(ocdata->fd->markbit) |= 0x0800;
                    gettimeofday(&ct,NULL);
                    update_event_list(ocdata->fd->ec,ocdata->fd->el,1,16,ct.tv_sec,ocdata->fd->markbit);
                    if (SUCCESS != generate_event_file(ocdata->fd->ec,ocdata->fd->el,\
															ocdata->fd->softevent,ocdata->fd->markbit))
                    {
                    #ifdef OC_DEBUG
                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                    }
                }
            }
            else
            {
            #ifdef OC_DEBUG
                printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
			sendfaceInfoToBoard(ocdata->fd,fbdata);
			mt = pinfo->mingtime;
			while (1)
			{//monitor loop
			#ifdef OC_DEBUG
            	printf("*************Begin to monitor flowpipe,File: %s,Line: %d\n",__FILE__,__LINE__);
        	#endif
				FD_ZERO(&nRead);
				FD_SET(*(ocdata->fd->flowpipe),&nRead);
				lasttime.tv_sec = 0;
				lasttime.tv_usec = 0;
				gettimeofday(&lasttime,NULL);
				mtime.tv_sec = mt;
				mtime.tv_usec = 0;
				int mret = select(*(ocdata->fd->flowpipe)+1,&nRead,NULL,NULL,&mtime);
				if (mret < 0)
				{
				#ifdef OC_DEBUG
					printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					oc_end_part_child_thread();
					return;
				}
				if (0 == mret)
				{//time out
					if (*(ocdata->fd->markbit) & 0x0001)
					{
						cycarr = 1;
						break;
					}
					if ((*(ocdata->fd->markbit) & 0x0100) && (0 == endahead))
					{
						cycarr = 1;
                        break;
					}
					mt = pinfo->mingtime;
					if ((!(*(ocdata->fd->markbit) & 0x1000)) && (!(*(ocdata->fd->markbit) & 0x8000)))
					{//if((!(*(ocdata->fd->markbit) & 0x1000))&&(!(*(ocdata->fd->markbit) & 0x8000)))
						if (0 == pinfo->cchan[0])
						{
							sinfo.time = pinfo->mingtime + pinfo->gftime + pinfo->ytime + pinfo->rtime;
						}
						else
						{
							sinfo.time = pinfo->mingtime + pinfo->gftime;	
						}
						memcpy(ocdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
						memset(sibuf,0,sizeof(sibuf));
            			if (SUCCESS != status_info_report(sibuf,&sinfo))
            			{
            			#ifdef OC_DEBUG
                			printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            			#endif
            			}
            			else
            			{
                			write(*(ocdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            			}	
					}//if((!(*(ocdata->fd->markbit) & 0x1000))&&(!(*(ocdata->fd->markbit) & 0x8000)))				
					if ((*(ocdata->fd->markbit) & 0x0002) && (*(ocdata->fd->markbit) & 0x0010))
            		{
                		memset(&timedown,0,sizeof(timedown));
                		timedown.mode = *(ocdata->fd->contmode);
                		timedown.pattern = *(ocdata->fd->patternid);
               			timedown.lampcolor = 0x02;
               			timedown.lamptime = (pinfo->mingtime) + (pinfo->gftime);
               			timedown.phaseid = pinfo->phaseid;
               			timedown.stageline = pinfo->stageline;
               			if (SUCCESS != timedown_data_to_conftool(ocdata->fd->sockfd,&timedown))
               			{
               			#ifdef OC_DEBUG
                   			printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
               			#endif
                		}
            		}
					#ifdef EMBED_CONFIGURE_TOOL
					if (*(ocdata->fd->markbit2) & 0x0200)
					{   
						memset(&timedown,0,sizeof(timedown));
                        timedown.mode = *(ocdata->fd->contmode);
                        timedown.pattern = *(ocdata->fd->patternid);
                        timedown.lampcolor = 0x02;
                        timedown.lamptime = (pinfo->mingtime) + (pinfo->gftime);
                        timedown.phaseid = pinfo->phaseid;
                        timedown.stageline = pinfo->stageline;	 
						if (SUCCESS != timedown_data_to_embed(ocdata->fd->sockfd,&timedown))
						{
						#ifdef OC_DEBUG
							printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
					}
					#endif
					//send info to face board
					if (*(ocdata->fd->contmode) < 27)
                		fbdata[1] = *(ocdata->fd->contmode) + 1;
            		else
                		fbdata[1] = *(ocdata->fd->contmode);
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
            			fbdata[4] = (pinfo->mingtime) + (pinfo->gftime);
					}
            		if (!wait_write_serial(*(ocdata->fd->fbserial)))
            		{
                		if (write(*(ocdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
                		{
                		#ifdef OC_DEBUG
                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                		#endif
							*(ocdata->fd->markbit) |= 0x0800;
                   			gettimeofday(&ct,NULL);
                   			update_event_list(ocdata->fd->ec,ocdata->fd->el,1,16, \
															ct.tv_sec,ocdata->fd->markbit);
                   			if (SUCCESS != generate_event_file(ocdata->fd->ec,ocdata->fd->el, \
															ocdata->fd->softevent,ocdata->fd->markbit))
                   			{
                   			#ifdef OC_DEBUG
                   				printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                   			#endif
                   			}
                		}
            		}
            		else
            		{
            		#ifdef OC_DEBUG
                		printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
            		#endif
            		}
					sendfaceInfoToBoard(ocdata->fd,fbdata);
					continue;
				}//time out
			}//monitor loop 
		}//vehicle phase
		else
		{//not fit phase type
		#ifdef OC_DEBUG
			printf("Not have fit phase type,please reset pattern,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			fbdata[0] = 0xC1;
			fbdata[5] = 0xED;
			fbdata[1] = 2;
			fbdata[2] = 0;
			fbdata[3] = 0x01;
			fbdata[4] = 0;
			sendfaceInfoToBoard(ocdata->fd,fbdata);
			cycarr = 1;
			sleep(1);
			continue;
		}//not fit phase type
	}//while loop

	return;
}

int get_oc_status(unsigned char *color,unsigned char *leatime)
{
	if ((NULL == color) || (NULL == leatime))
    {
    #ifdef OC_DEBUG
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

int overload_control(fcdata_t *fcdata, tscdata_t *tscdata,ChannelStatus_t *chanstatus)
{
	if ((NULL == fcdata) || (NULL == tscdata) || (NULL == chanstatus))
	{
	#ifdef OC_DEBUG
		printf("pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
		output_log("green trigger,pointer address is null");
	#endif
		return MEMERR;
	}	

	//Initial static variable
	ocyes = 0;
	ocyfyes = 0;
	ocpcyes = 0;
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

	ocdata_t				ocdata;
	ocpinfo_t				pinfo;
	unsigned char			contmode = *(fcdata->contmode);
	if (0 == ocyes)
	{
		memset(&ocdata,0,sizeof(ocdata_t));
		memset(&pinfo,0,sizeof(ocpinfo_t));
		ocdata.fd = fcdata;
		ocdata.td = tscdata;
		ocdata.pi = &pinfo;
		ocdata.cs = chanstatus;
		int gtret = pthread_create(&ocid,NULL,(void *)start_overload_control,&ocdata);
		if (0 != gtret)
		{
		#ifdef OC_DEBUG
			printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
		ocyes = 1;
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
//	statusinfo_t                sinfo;
//    memset(&sinfo,0,sizeof(statusinfo_t));
	unsigned char               *csta = NULL;
    unsigned char               tcsta = 0;
    sinfo.pattern = *(fcdata->patternid);
	unsigned char               ncmode = *(fcdata->contmode);

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
	unsigned char				jsc = 0;//jump stage control 
	unsigned int				totime = 0;
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
/*
	if (SUCCESS != oc_read_timeout(&totime))
	{
	#ifdef OC_DEBUG
		printf("get time err,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
	}
*/
	unsigned char		tclc = 0;
	int 			cpret = 0;
	while (1)
	{//0
	#ifdef OC_DEBUG
		printf("Begin to monitor control pipe,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		FD_ZERO(&nread);
		FD_SET(*(fcdata->conpipe),&nread);
		if (pinfo.maxgtime2 > 0)
		{
			wtltime.tv_sec = pinfo.maxgtime2;
			wtltime.tv_usec = 0;
			cpret = select(*(fcdata->conpipe)+1,&nread,NULL,NULL,&wtltime);
		}
		else
		{
			cpret = select(*(fcdata->conpipe)+1,&nread,NULL,NULL,NULL);
		}
		if (cpret < 0)
		{
		#ifdef OC_DEBUG
			printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			oc_end_part_child_thread();
			return FAIL;
		}
		if (0 == cpret)
		{//time out
			if (*(fcdata->markbit2) & 0x0100)
                continue; //rfid is controlling
			*(fcdata->markbit2) &= 0xfffe;
			if (1 == keylock)
			{//1 == keylock
			#ifdef OC_DEBUG
				printf("*****************prepare to unlock,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				keylock = 0;
				gtred = 0;
				close = 0;
				if (1 == ocyfyes)
				{
					pthread_cancel(ocyfid);
					pthread_join(ocyfid,NULL);
					ocyfyes = 0;
				}
				gtecho[1] = 0x00;
				
				if (!wait_write_serial(*(fcdata->fbserial)))
				{
					if (write(*(fcdata->fbserial),gtecho,sizeof(gtecho)) < 0)
					{
					#ifdef OC_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						*(fcdata->markbit) |= 0x0800;
						gettimeofday(&ct,NULL);
						update_event_list(fcdata->ec,fcdata->el,1,16, \
											ct.tv_sec,fcdata->markbit);
						if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
															fcdata->softevent,fcdata->markbit))
						{
						#ifdef OC_DEBUG
							printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
					}
				}
				else
				{
				#ifdef OC_DEBUG
					printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}

				if (1 == jsc)
				{//stage control happen
					jsc = 0;
					unsigned char			ocnphid = 0;
					unsigned char			ochan[32] = {0};
					unsigned char			occhan[32] = {0};
					unsigned char			ocpchan[32] = {0};
					unsigned char			ocnpchan[32] = {0};
					unsigned char			*pochan = ochan;
					unsigned char			oci = 0, ocj = 0,ocs = 0,ock = 0;
					unsigned char			exist = 0,pcexist = 0;
					get_phase_id(tscdata->timeconfig->TimeConfigList[rettl][0].PhaseId,&ocnphid);
					for (oci = 0; oci < tscdata->channel->FactChannelNum; oci++)
					{
						if (0 == tscdata->channel->ChannelList[oci].ChannelId)
							break;
						if (ocnphid == tscdata->channel->ChannelList[oci].ChannelCtrlSrc)
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
							*pochan = tscdata->channel->ChannelList[oci].ChannelId;
							pochan++;
						}	
					}
					pochan = occhan;
					for (oci = 0; oci < MAX_CHANNEL; oci++)
					{//for (oci = 0; oci < MAX_CHANNEL; oci++)
						if (0 == pinfo.chan[oci])
							break;
						exist = 0;
						for (ocj = 0; ocj < MAX_CHANNEL; ocj++)
						{//for (ocj = 0; ocj < MAX_CHANNEL; ocj++)
							if (0 == ochan[ocj])
								break;
							if (pinfo.chan[oci] == ochan[ocj])
							{
								exist = 1;
								break;
							}
						}//for (ocj = 0; ocj < MAX_CHANNEL; ocj++)
						if (0 == exist)
						{//if (0 == exist)
							*pochan = pinfo.chan[oci];
							pochan++;
							for (ocj = 0; ocj < tscdata->channel->FactChannelNum; ocj++)
							{//2j
								if (0 == tscdata->channel->ChannelList[ocj].ChannelId)
									break;
								if (pinfo.chan[oci] == tscdata->channel->ChannelList[ocj].ChannelId)
								{
									if (3 == tscdata->channel->ChannelList[ocj].ChannelType)
									{
										pcexist = 1;
										ocpchan[ocs] = pinfo.chan[oci];
										ocs++;
									}
									else
									{
										ocnpchan[ock] = pinfo.chan[oci];
										ock++;
									}
								}
							}//2j
						}//if (0 == exist)
					}//for (oci = 0; oci < MAX_CHANNEL; oci++)

					if ((0 == occhan[0])&&(pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0))
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
							for (oci = 0; oci < MAX_CHANNEL; oci++)
							{
								if (0 == ochan[oci])
									break;
								sinfo.chans += 1;
								tcsta = ochan[oci];
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
							#ifdef OC_DEBUG
								printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
							#endif
							}
							else
							{
								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
							}
						}
					}
					if ((0 != occhan[0]) && (pinfo.gftime > 0))
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
							
							for (oci = 0; oci < MAX_CHANNEL; oci++)
							{
								if (0 == occhan[oci])
									break;
								for (ocj = 0; ocj < sinfo.chans; ocj++)
								{
									if (0 == sinfo.csta[ocj])
										break;
									tcsta = sinfo.csta[ocj];
									tcsta &= 0xfc;
									tcsta >>= 2;
									tcsta &= 0x3f;
									if (occhan[oci] == tcsta)
									{
										sinfo.csta[ocj] &= 0xfc;
										sinfo.csta[ocj] |= 0x03; //00000001-green flash
										break;
									}
								}
							}								
							memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));		
							memset(sibuf,0,sizeof(sibuf));
							if (SUCCESS != status_info_report(sibuf,&sinfo))
							{
							#ifdef OC_DEBUG
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
						#ifdef OC_DEBUG
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
						#ifdef OC_DEBUG
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
						#ifdef OC_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(fcdata->markbit) |= 0x0800;
							gettimeofday(&ct,NULL);
							update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
							if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
														fcdata->softevent,fcdata->markbit))
							{
							#ifdef OC_DEBUG
								printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}
						}
					}
					else
					{
					#ifdef OC_DEBUG
						printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					sendfaceInfoToBoard(fcdata,fbdata);
					if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
					{//report info to server actively
						memset(err,0,sizeof(err));
						gettimeofday(&ct,NULL);
						if (SUCCESS != err_report(err,ct.tv_sec,22,5))
						{
						#ifdef OC_DEBUG
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
						if (SUCCESS != oc_set_lamp_color(*(fcdata->bbserial),occhan,0x03))
						{
						#ifdef OC_DEBUG
							printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							gettimeofday(&ct,NULL);
							update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
							if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																fcdata->softevent,fcdata->markbit))
							{
							#ifdef OC_DEBUG
								printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}
							*(fcdata->markbit) |= 0x0800;
						}
						if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
							occhan,0x03,fcdata->markbit))
						{
						#ifdef OC_DEBUG 
							printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
						#endif
						}
						to.tv_sec = 0;
						to.tv_usec = 500000;
						select(0,NULL,NULL,NULL,&to);
						if (SUCCESS != oc_set_lamp_color(*(fcdata->bbserial),occhan,0x02))
						{
						#ifdef OC_DEBUG
							printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							gettimeofday(&ct,NULL);
							update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
							if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
															fcdata->softevent,fcdata->markbit))
							{
							#ifdef OC_DEBUG
								printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}
							*(fcdata->markbit) |= 0x0800;
						}
						if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
							occhan,0x02,fcdata->markbit))
						{
						#ifdef OC_DEBUG
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
						if (1 == pcexist)
						{//have person channels
							if (SUCCESS != oc_set_lamp_color(*(fcdata->bbserial),ocnpchan,0x01))
							{
							#ifdef OC_DEBUG
								printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								gettimeofday(&ct,NULL);
								update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
								if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																fcdata->softevent,fcdata->markbit))
								{
								#ifdef OC_DEBUG
									printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
								*(fcdata->markbit) |= 0x0800;
							}
							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
								ocnpchan,0x01,fcdata->markbit))
							{
							#ifdef OC_DEBUG
								printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}
							if (SUCCESS != oc_set_lamp_color(*(fcdata->bbserial),ocpchan,0x00))
							{
							#ifdef OC_DEBUG
								printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								gettimeofday(&ct,NULL);
								update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
								if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
															fcdata->softevent,fcdata->markbit))
								{
								#ifdef OC_DEBUG
									printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
								*(fcdata->markbit) |= 0x0800;
							}
							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
									ocpchan,0x00,fcdata->markbit))
							{
							#ifdef OC_DEBUG
								printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}	
						}//have person channels
						else
						{//not have person channels
							if (SUCCESS != oc_set_lamp_color(*(fcdata->bbserial),occhan,0x01))
							{
							#ifdef OC_DEBUG
								printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								gettimeofday(&ct,NULL);
								update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
								if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																fcdata->softevent,fcdata->markbit))
								{
								#ifdef OC_DEBUG
									printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
								*(fcdata->markbit) |= 0x0800;
							}
							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
													occhan,0x01,fcdata->markbit))
							{
							#ifdef OC_DEBUG
								printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}
						}//not have person channels

						if ((0 != occhan[0]) && (pinfo.ytime > 0))
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
													
								for (oci = 0; oci < MAX_CHANNEL; oci++)
								{
									if (0 == ocnpchan[i])
										break;
									for (ocj = 0; ocj < sinfo.chans; ocj++)
									{
										if (0 == sinfo.csta[ocj])
											break;
										tcsta = sinfo.csta[ocj];
										tcsta &= 0xfc;
										tcsta >>= 2;
										tcsta &= 0x3f;
										if (ocnpchan[oci] == tcsta)
										{
											sinfo.csta[ocj] &= 0xfc;
											sinfo.csta[ocj] |= 0x01; //00000001-yellow
											break;
										}
									}
								}
								for (oci = 0; oci < MAX_CHANNEL; oci++)
								{
									if (0 == ocpchan[oci])
										break;
									for (ocj = 0; ocj < sinfo.chans; ocj++)
									{
										if (0 == sinfo.csta[ocj])
											break;
										tcsta = sinfo.csta[ocj];
										tcsta &= 0xfc;
										tcsta >>= 2;
										tcsta &= 0x3f;
										if (ocpchan[oci] == tcsta)
										{
											sinfo.csta[ocj] &= 0xfc;
											break;
										}
									}
								}
								memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
								memset(sibuf,0,sizeof(sibuf));
								if (SUCCESS != status_info_report(sibuf,&sinfo))
								{
								#ifdef OC_DEBUG
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
							#ifdef OC_DEBUG
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
							#ifdef OC_DEBUG
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
							#ifdef OC_DEBUG
								printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								*(fcdata->markbit) |= 0x0800;
								gettimeofday(&ct,NULL);
								update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
								if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
															fcdata->softevent,fcdata->markbit))
								{
								#ifdef OC_DEBUG
									printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
							}
						}
						else
						{
						#ifdef OC_DEBUG
							printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
						sendfaceInfoToBoard(fcdata,fbdata);
						sleep(pinfo.ytime);
						//end yellow lamp
					}//person phase do not have yellow lamp status

					//red lamp
					if (SUCCESS != oc_set_lamp_color(*(fcdata->bbserial),occhan,0x00))
					{
					#ifdef OC_DEBUG
						printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						gettimeofday(&ct,NULL);
						update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
						if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
														fcdata->softevent,fcdata->markbit))
						{
						#ifdef OC_DEBUG
							printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
						*(fcdata->markbit) |= 0x0800;
					}
					if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus,occhan,\
								0x00,fcdata->markbit))
					{
					#ifdef OC_DEBUG
						printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}

					if ((0 != occhan[0]) && (pinfo.rtime > 0))
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
													
							for (oci = 0; oci < MAX_CHANNEL; oci++)
							{
								if (0 == pinfo.cchan[oci])
									break;
								for (ocj = 0; ocj < sinfo.chans; ocj++)
								{
									if (0 == sinfo.csta[ocj])
										break;
									tcsta = sinfo.csta[ocj];
									tcsta &= 0xfc;
									tcsta >>= 2;
									tcsta &= 0x3f;
									if (pinfo.cchan[oci] == tcsta)
									{
										sinfo.csta[ocj] &= 0xfc;
										break;
									}
								}
							}
							memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
							memset(sibuf,0,sizeof(sibuf));
							if (SUCCESS != status_info_report(sibuf,&sinfo))
							{
							#ifdef OC_DEBUG
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
							#ifdef OC_DEBUG
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
							#ifdef OC_DEBUG
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
							#ifdef OC_DEBUG
								printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								*(fcdata->markbit) |= 0x0800;
								gettimeofday(&ct,NULL);
								update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
								if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																fcdata->softevent,fcdata->markbit))
								{
								#ifdef OC_DEBUG
									printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
							}
						}
						else
						{
						#ifdef OC_DEBUG
							printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
						}
						sendfaceInfoToBoard(fcdata,fbdata);
						sleep(pinfo.rtime);
					}	
				}//stage control happen

//				new_all_red(&ardata);
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
					#ifdef OC_DEBUG
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
				if (0 == ocyes)
				{
					memset(&ocdata,0,sizeof(ocdata_t));
					memset(&pinfo,0,sizeof(ocpinfo_t));
					ocdata.fd = fcdata;
					ocdata.td = tscdata;
					ocdata.pi = &pinfo;
					ocdata.cs = chanstatus;
					int ret=pthread_create(&ocid,NULL,(void *)start_overload_control,&ocdata);
					if (0 != ret)
					{
					#ifdef OC_DEBUG
						printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
						output_log("green trigger,create green trigger thread err");
					#endif
						return FAIL;
					}
					ocyes = 1;
				}
				if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
				{//report info to server actively
					memset(err,0,sizeof(err));
					gettimeofday(&ct,NULL);
					if (SUCCESS != err_report(err,ct.tv_sec,22,3))
					{
					#ifdef OC_DEBUG
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
			}//1 == keylock
            continue;
		}//time out
		if (cpret > 0)
		{//if (cpret > 0)
			if (FD_ISSET(*(fcdata->conpipe),&nread))
			{
				memset(gtbuf,0,sizeof(gtbuf));
				read(*(fcdata->conpipe),gtbuf,sizeof(gtbuf));
				if ((0xDA == gtbuf[0]) && (0xED == gtbuf[2]))
				{//key traffic control
					if ((0 == wllock) && (0 == netlock) && (0 == wtlock))
					{//key lock or unlock is valid only when wireless lock or unlock is invalid
						if (0x01 == gtbuf[1])
						{//lock or unlock
							if (0 == keylock)
							{//prepare to lock
							#ifdef OC_DEBUG
								printf("*************Prepare to lock,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								get_oc_status(&color,&leatime);
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
										get_oc_status(&color,&leatime);
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
								oc_end_child_thread();//end main thread and its child thread
								*(fcdata->markbit2) |= 0x0002;
								
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
            						#ifdef OC_DEBUG
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
									#ifdef OC_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    	if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef OC_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
									}
								}
								else
								{
								#ifdef OC_DEBUG
									printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
								sendfaceInfoToBoard(fcdata,fbdata);
								gtecho[1] = 0x01;
								
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),gtecho,sizeof(gtecho)) < 0)
									{
									#ifdef OC_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16, \
															ct.tv_sec,fcdata->markbit);
										if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
										{
										#ifdef OC_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
								}
								else
								{
								#ifdef OC_DEBUG
									printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
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
                                	#ifdef OC_DEBUG
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
                                    #ifdef OC_DEBUG
                                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                                #endif
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    pinfo.chan,0x02,fcdata->markbit))
                                {
                                #ifdef OC_DEBUG 
                                    printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
                                {//report info to server actively
                                    memset(err,0,sizeof(err));
                                    gettimeofday(&ct,NULL);
                                    if (SUCCESS != err_report(err,ct.tv_sec,22,1))
                                    {
                                    #ifdef OC_DEBUG
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
							#ifdef OC_DEBUG
								printf("prepare to unlock,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								keylock = 0;
								gtred = 0;
								close = 0;
								if (1 == ocyfyes)
								{
									pthread_cancel(ocyfid);
									pthread_join(ocyfid,NULL);
									ocyfyes = 0;
								}
								gtecho[1] = 0x00;
								
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),gtecho,sizeof(gtecho)) < 0)
									{
									#ifdef OC_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16, \
															ct.tv_sec,fcdata->markbit);
										if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																			fcdata->softevent,fcdata->markbit))
										{
										#ifdef OC_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
								}
								else
								{
								#ifdef OC_DEBUG
									printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}

								if (1 == jsc)
								{//stage control happen
									jsc = 0;
									unsigned char			ocnphid = 0;
									unsigned char			ochan[32] = {0};
									unsigned char			occhan[32] = {0};
									unsigned char			ocpchan[32] = {0};
									unsigned char			ocnpchan[32] = {0};
									unsigned char			*pochan = ochan;
									unsigned char			oci = 0, ocj = 0,ocs = 0,ock = 0;
									unsigned char			exist = 0,pcexist = 0;
								get_phase_id(tscdata->timeconfig->TimeConfigList[rettl][0].PhaseId,&ocnphid);
									for (oci = 0; oci < tscdata->channel->FactChannelNum; oci++)
									{
										if (0 == tscdata->channel->ChannelList[oci].ChannelId)
											break;
										if (ocnphid == tscdata->channel->ChannelList[oci].ChannelCtrlSrc)
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
											*pochan = tscdata->channel->ChannelList[oci].ChannelId;
											pochan++;
										}	
									}
									pochan = occhan;
									for (oci = 0; oci < MAX_CHANNEL; oci++)
									{//for (oci = 0; oci < MAX_CHANNEL; oci++)
										if (0 == pinfo.chan[oci])
											break;
										exist = 0;
										for (ocj = 0; ocj < MAX_CHANNEL; ocj++)
										{//for (ocj = 0; ocj < MAX_CHANNEL; ocj++)
											if (0 == ochan[ocj])
												break;
											if (pinfo.chan[oci] == ochan[ocj])
											{
												exist = 1;
												break;
											}
										}//for (ocj = 0; ocj < MAX_CHANNEL; ocj++)
										if (0 == exist)
										{//if (0 == exist)
											*pochan = pinfo.chan[oci];
											pochan++;
											for (ocj = 0; ocj < tscdata->channel->FactChannelNum; ocj++)
											{//2j
												if (0 == tscdata->channel->ChannelList[ocj].ChannelId)
													break;
											if (pinfo.chan[oci] == tscdata->channel->ChannelList[ocj].ChannelId)
												{
													if (3 == tscdata->channel->ChannelList[ocj].ChannelType)
													{
														pcexist = 1;
														ocpchan[ocs] = pinfo.chan[oci];
														ocs++;
													}
													else
													{
														ocnpchan[ock] = pinfo.chan[oci];
														ock++;
													}
												}
											}//2j
										}//if (0 == exist)
									}//for (oci = 0; oci < MAX_CHANNEL; oci++)

									if ((0 == occhan[0])&&(pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0))
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
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
											for (oci = 0; oci < MAX_CHANNEL; oci++)
											{
												if (0 == ochan[oci])
													break;
												sinfo.chans += 1;
												tcsta = ochan[oci];
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
											#ifdef OC_DEBUG
											printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
											#endif
											}
											else
											{
												write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
											}
										}
									}
									if ((0 != occhan[0]) && (pinfo.gftime > 0))
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
											
											for (oci = 0; oci < MAX_CHANNEL; oci++)
											{
												if (0 == occhan[oci])
													break;
												for (ocj = 0; ocj < sinfo.chans; ocj++)
												{
													if (0 == sinfo.csta[ocj])
														break;
													tcsta = sinfo.csta[ocj];
													tcsta &= 0xfc;
													tcsta >>= 2;
													tcsta &= 0x3f;
													if (occhan[oci] == tcsta)
													{
														sinfo.csta[ocj] &= 0xfc;
														sinfo.csta[ocj] |= 0x03; //00000001-green flash
														break;
													}
												}
											}								
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));		
											memset(sibuf,0,sizeof(sibuf));
											if (SUCCESS != status_info_report(sibuf,&sinfo))
											{
											#ifdef OC_DEBUG
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
										#ifdef OC_DEBUG
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
										#ifdef OC_DEBUG
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
										#ifdef OC_DEBUG
											printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
											if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
											{
											#ifdef OC_DEBUG
												printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
											}
										}
									}
									else
									{
									#ifdef OC_DEBUG
										printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}

									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										memset(err,0,sizeof(err));
										gettimeofday(&ct,NULL);
										if (SUCCESS != err_report(err,ct.tv_sec,22,5))
										{
										#ifdef OC_DEBUG
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
										if (SUCCESS != oc_set_lamp_color(*(fcdata->bbserial),occhan,0x03))
										{
										#ifdef OC_DEBUG
											printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
											if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
											{
											#ifdef OC_DEBUG
												printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
											}
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
											occhan,0x03,fcdata->markbit))
										{
										#ifdef OC_DEBUG 
											printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);
										if (SUCCESS != oc_set_lamp_color(*(fcdata->bbserial),occhan,0x02))
										{
										#ifdef OC_DEBUG
											printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
											if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																			fcdata->softevent,fcdata->markbit))
											{
											#ifdef OC_DEBUG
												printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
											}
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
											occhan,0x02,fcdata->markbit))
										{
										#ifdef OC_DEBUG
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
										if (1 == pcexist)
										{//have person channels
											if (SUCCESS != oc_set_lamp_color(*(fcdata->bbserial),ocnpchan,0x01))
											{
											#ifdef OC_DEBUG
												printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
												if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
												{
												#ifdef OC_DEBUG
												printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
												#endif
												}
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
												ocnpchan,0x01,fcdata->markbit))
											{
											#ifdef OC_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
											}
											if (SUCCESS != oc_set_lamp_color(*(fcdata->bbserial),ocpchan,0x00))
											{
											#ifdef OC_DEBUG
												printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
												if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																			fcdata->softevent,fcdata->markbit))
												{
												#ifdef OC_DEBUG
												printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
												#endif
												}
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
													ocpchan,0x00,fcdata->markbit))
											{
											#ifdef OC_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
											}	
										}//have person channels
										else
										{//not have person channels
											if (SUCCESS != oc_set_lamp_color(*(fcdata->bbserial),occhan,0x01))
											{
											#ifdef OC_DEBUG
												printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
												if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
												{
												#ifdef OC_DEBUG
												printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
												#endif
												}
												*(fcdata->markbit) |= 0x0800;
											}
											if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																	occhan,0x01,fcdata->markbit))
											{
											#ifdef OC_DEBUG
											printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
											}
										}//not have person channels

										if ((0 != occhan[0]) && (pinfo.ytime > 0))
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
																	
												for (oci = 0; oci < MAX_CHANNEL; oci++)
												{
													if (0 == ocnpchan[i])
														break;
													for (ocj = 0; ocj < sinfo.chans; ocj++)
													{
														if (0 == sinfo.csta[ocj])
															break;
														tcsta = sinfo.csta[ocj];
														tcsta &= 0xfc;
														tcsta >>= 2;
														tcsta &= 0x3f;
														if (ocnpchan[oci] == tcsta)
														{
															sinfo.csta[ocj] &= 0xfc;
															sinfo.csta[ocj] |= 0x01; //00000001-yellow
															break;
														}
													}
												}
												for (oci = 0; oci < MAX_CHANNEL; oci++)
												{
													if (0 == ocpchan[oci])
														break;
													for (ocj = 0; ocj < sinfo.chans; ocj++)
													{
														if (0 == sinfo.csta[ocj])
															break;
														tcsta = sinfo.csta[ocj];
														tcsta &= 0xfc;
														tcsta >>= 2;
														tcsta &= 0x3f;
														if (ocpchan[oci] == tcsta)
														{
															sinfo.csta[ocj] &= 0xfc;
															break;
														}
													}
												}
												memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
												memset(sibuf,0,sizeof(sibuf));
												if (SUCCESS != status_info_report(sibuf,&sinfo))
												{
												#ifdef OC_DEBUG
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
											#ifdef OC_DEBUG
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
											#ifdef OC_DEBUG
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
											#ifdef OC_DEBUG
												printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
												if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																			fcdata->softevent,fcdata->markbit))
												{
												#ifdef OC_DEBUG
												printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
												#endif
												}
											}
										}
										else
										{
										#ifdef OC_DEBUG
											printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										sleep(pinfo.ytime);
										//end yellow lamp
									}//person phase do not have yellow lamp status

									//red lamp
									if (SUCCESS != oc_set_lamp_color(*(fcdata->bbserial),occhan,0x00))
									{
									#ifdef OC_DEBUG
										printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
										if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
										{
										#ifdef OC_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus,occhan,\
												0x00,fcdata->markbit))
									{
									#ifdef OC_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}

									if ((0 != occhan[0]) && (pinfo.rtime > 0))
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
																	
											for (oci = 0; oci < MAX_CHANNEL; oci++)
											{
												if (0 == pinfo.cchan[oci])
													break;
												for (ocj = 0; ocj < sinfo.chans; ocj++)
												{
													if (0 == sinfo.csta[ocj])
														break;
													tcsta = sinfo.csta[ocj];
													tcsta &= 0xfc;
													tcsta >>= 2;
													tcsta &= 0x3f;
													if (pinfo.cchan[oci] == tcsta)
													{
														sinfo.csta[ocj] &= 0xfc;
														break;
													}
												}
											}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
											memset(sibuf,0,sizeof(sibuf));
											if (SUCCESS != status_info_report(sibuf,&sinfo))
											{
											#ifdef OC_DEBUG
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
											#ifdef OC_DEBUG
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
											#ifdef OC_DEBUG
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
											#ifdef OC_DEBUG
												printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
											#endif
												*(fcdata->markbit) |= 0x0800;
												gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
												if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
												{
												#ifdef OC_DEBUG
												printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
												#endif
												}
											}
										}
										else
										{
										#ifdef OC_DEBUG
											printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										sleep(pinfo.rtime);
									}	
								}//stage control happen

//								new_all_red(&ardata);
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
            						#ifdef OC_DEBUG
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
								if (0 == ocyes)
								{
									memset(&ocdata,0,sizeof(ocdata_t));
									memset(&pinfo,0,sizeof(ocpinfo_t));
									ocdata.fd = fcdata;
									ocdata.td = tscdata;
									ocdata.pi = &pinfo;
									ocdata.cs = chanstatus;
									int ret=pthread_create(&ocid,NULL,(void *)start_overload_control,&ocdata);
									if (0 != ret)
									{
									#ifdef OC_DEBUG
										printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
										output_log("green trigger,create green trigger thread err");
									#endif
										return FAIL;
									}
									ocyes = 1;
								}
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
                                {//report info to server actively
                                    memset(err,0,sizeof(err));
                                    gettimeofday(&ct,NULL);
                                    if (SUCCESS != err_report(err,ct.tv_sec,22,3))
                                    {
                                    #ifdef OC_DEBUG
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
						#ifdef OC_DEBUG
							printf("**************gtbuf[1]:%d,File:%s,Line:%d\n",gtbuf[1],__FILE__,__LINE__);
						#endif
							memset(&mdt,0,sizeof(markdata_c));
							mdt.redl = &gtred;
							mdt.closel = &close;
							mdt.rettl = &rettl;
							mdt.yfl = &ocyfyes;
							mdt.yfid = &ocyfid;
							mdt.ardata = &ardata;
							mdt.fcdata = fcdata;
							mdt.tscdata = tscdata;
							mdt.chanstatus = chanstatus;
							mdt.sinfo = &sinfo;
							oc_jump_stage_control(&mdt,gtbuf[1],&pinfo);	
							jsc = 1;
						}//jump stage control
						if ((0x02 == gtbuf[1]) && (1 == keylock))
						{//step by step
							if ((1 == gtred) || (1 ==ocyfyes) || (1 == close))
							{
								if (1 == close)
								{
									new_all_red(&ardata);
									close = 0;
								}
								gtred = 0;
								if (1 == ocyfyes)
								{
									pthread_cancel(ocyfid);
									pthread_join(ocyfid,NULL);
									ocyfyes = 0;
									new_all_red(&ardata);
								}
					//			#ifdef CLOSE_LAMP
                                oc_set_lamp_color(*(fcdata->bbserial),clch,0x03);
								update_channel_status(fcdata->sockfd,chanstatus,clch,0x03,fcdata->markbit);
                    //            #endif
								if (SUCCESS != oc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
								{
								#ifdef OC_DEBUG
									printf("set lamp color err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									gettimeofday(&ct,NULL);
									update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
									if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef OC_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                    *(fcdata->markbit) |= 0x0800;
								}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    				pinfo.chan,0x02,fcdata->markbit))
                                {
                                #ifdef OC_DEBUG 
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
            						#ifdef OC_DEBUG
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
                                    #ifdef OC_DEBUG
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
            						#ifdef OC_DEBUG
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
								#ifdef OC_DEBUG
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
								#ifdef OC_DEBUG
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
                                #ifdef OC_DEBUG
                                    printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									*(fcdata->markbit) |= 0x0800;
                                    gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef OC_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                            }
                            else
                            {
                            #ifdef OC_DEBUG
                                printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }

							if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
                            {//report info to server actively
                                memset(err,0,sizeof(err));
                                gettimeofday(&ct,NULL);
                                if (SUCCESS != err_report(err,ct.tv_sec,22,5))
                                {
                                #ifdef OC_DEBUG
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
								if (SUCCESS != oc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x03))
								{
								#ifdef OC_DEBUG
									printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef OC_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
									*(fcdata->markbit) |= 0x0800;
								}
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    pinfo.cchan,0x03,fcdata->markbit))
                                {
                                #ifdef OC_DEBUG 
                                    printf("update channel err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
								to.tv_sec = 0;
								to.tv_usec = 500000;
								select(0,NULL,NULL,NULL,&to);
								if (SUCCESS != oc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x02))
                                {
                                #ifdef OC_DEBUG
                                    printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef OC_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
									*(fcdata->markbit) |= 0x0800;
                                }
								if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    pinfo.cchan,0x02,fcdata->markbit))
                                {
                                #ifdef OC_DEBUG
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
									if (SUCCESS != oc_set_lamp_color(*(fcdata->bbserial),pinfo.cnpchan,0x01))
									{
									#ifdef OC_DEBUG
										printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                    	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef OC_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
										*(fcdata->markbit) |= 0x0800;
									}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
										pinfo.cnpchan,0x01,fcdata->markbit))
									{
									#ifdef OC_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}
									if (SUCCESS != oc_set_lamp_color(*(fcdata->bbserial),pinfo.cpchan,0x00))
                                	{
                                	#ifdef OC_DEBUG
                                    	printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
										gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                    	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef OC_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
										*(fcdata->markbit) |= 0x0800;
                                	}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    		pinfo.cpchan,0x00,fcdata->markbit))
                                	{
                                	#ifdef OC_DEBUG
                                    	printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
                                	}	
								}//have person channels
								else
								{//not have person channels
									if (SUCCESS != oc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x01))
                                	{
                                	#ifdef OC_DEBUG
                                    	printf("set color err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
										gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                    	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef OC_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
										*(fcdata->markbit) |= 0x0800;
                                	}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                    						pinfo.cchan,0x01,fcdata->markbit))
                                	{
                                	#ifdef OC_DEBUG
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
            							#ifdef OC_DEBUG
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
                                	#ifdef OC_DEBUG
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
                                    #ifdef OC_DEBUG
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
                                	#ifdef OC_DEBUG
                                    	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
										*(fcdata->markbit) |= 0x0800;
                                    	gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    	if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef OC_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                	}
                            	}
                            	else
                            	{
                            	#ifdef OC_DEBUG
                                	printf("can't write,File: %s,Line: %d\n",__FILE__,__LINE__);
                            	#endif
                            	}
								sleep(pinfo.ytime);
								//end yellow lamp
							}//person phase do not have yellow lamp status

							//red lamp
							if (SUCCESS != oc_set_lamp_color(*(fcdata->bbserial),pinfo.cchan,0x00))
							{
							#ifdef OC_DEBUG
								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
								gettimeofday(&ct,NULL);
                                update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																fcdata->softevent,fcdata->markbit))
                                {
                                #ifdef OC_DEBUG
                                    printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
								*(fcdata->markbit) |= 0x0800;
							}
							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus,pinfo.cchan,\
										0x00,fcdata->markbit))
							{
							#ifdef OC_DEBUG
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
            						#ifdef OC_DEBUG
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
                                    #ifdef OC_DEBUG
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
                                    #ifdef OC_DEBUG
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
                                	#ifdef OC_DEBUG
                                    	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
										*(fcdata->markbit) |= 0x0800;
                                    	gettimeofday(&ct,NULL);
                                    	update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    	if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
                                    	{
                                    	#ifdef OC_DEBUG
                                        	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                	}
                            	}
                            	else
                            	{
                            	#ifdef OC_DEBUG
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
							if (SUCCESS != oc_get_phase_info(fcdata,tscdata,rettl,*(fcdata->slnum),&pinfo))
							{
							#ifdef OC_DEBUG
								printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
								output_log("green trigger,get phase info err");
							#endif
								oc_end_part_child_thread();
								return FAIL;
							}
							*(fcdata->phaseid) = 0;
							*(fcdata->phaseid) |= (0x01 << (pinfo.phaseid - 1));
							if (SUCCESS != oc_set_lamp_color(*(fcdata->bbserial),pinfo.chan,0x02))
                            {
                            #ifdef OC_DEBUG
                                printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
								gettimeofday(&ct,NULL);
                                update_event_list(fcdata->ec,fcdata->el,1,15,ct.tv_sec,fcdata->markbit);
                                if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el, \
																fcdata->softevent,fcdata->markbit))
                                {
                                #ifdef OC_DEBUG
                                    printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
								*(fcdata->markbit) |= 0x0800;
                            }

							if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus,pinfo.chan,\
                                        0x02,fcdata->markbit))
                            {
                            #ifdef OC_DEBUG
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
            					#ifdef OC_DEBUG
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
								#ifdef OC_DEBUG
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
								#ifdef OC_DEBUG
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
                                #ifdef OC_DEBUG
                                    printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
									*(fcdata->markbit) |= 0x0800;
                                    gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS!=generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef OC_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                            }
                            else
                            {
                            #ifdef OC_DEBUG
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
                                    #ifdef OC_DEBUG
                                        printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                        *(fcdata->markbit) |= 0x0800;
                                    }
                                }
                                else
                                {
                                #ifdef OC_DEBUG
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
									#ifdef OC_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
										if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																			fcdata->softevent,fcdata->markbit))
										{
										#ifdef OC_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										*(fcdata->markbit) |= 0x0800;
									}
								}
								else
								{
								#ifdef OC_DEBUG
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
							if (0 == ocyfyes)
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
								int yfret = pthread_create(&ocyfid,NULL,(void *)oc_yellow_flash,&yfdata);
								if (0 != yfret)
								{
								#ifdef OC_DEBUG
									printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
									output_log("green trigger,create yellow flash err");
								#endif
									oc_end_part_child_thread();
									return FAIL;
								}
								ocyfyes = 1;
							}
							gtecho[1] = 0x03;
							if (*(fcdata->markbit2) & 0x2000)
							{
								*(fcdata->markbit2) &= 0xdfff;
								if (!wait_write_serial(*(fcdata->sdserial)))
                                {
                                    if (write(*(fcdata->sdserial),gtecho,sizeof(gtecho)) < 0)
                                    {
                                    #ifdef OC_DEBUG
                                        printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                        *(fcdata->markbit) |= 0x0800;
                                    }
                                }
                                else
                                {
                                #ifdef OC_DEBUG
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
									#ifdef OC_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
										if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
										{
										#ifdef OC_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										*(fcdata->markbit) |= 0x0800;
									}
								}
								else
								{
								#ifdef OC_DEBUG
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
            					#ifdef OC_DEBUG
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
                                #ifdef OC_DEBUG
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
							if (1 == ocyfyes)
							{
								pthread_cancel(ocyfid);
								pthread_join(ocyfid,NULL);
								ocyfyes = 0;
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
                                    #ifdef OC_DEBUG
                                        printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                        *(fcdata->markbit) |= 0x0800;
                                    }
                                }
                                else
                                {
                                #ifdef OC_DEBUG
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
									#ifdef OC_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
										if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																	fcdata->softevent,fcdata->markbit))
										{
										#ifdef OC_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
										*(fcdata->markbit) |= 0x0800;
									}
								}
								else
								{
								#ifdef OC_DEBUG
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
            					#ifdef OC_DEBUG
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
                                #ifdef OC_DEBUG
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
							if (1 == ocyfyes)
							{
								pthread_cancel(ocyfid);
								pthread_join(ocyfid,NULL);
								ocyfyes = 0;
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
								#ifdef OC_DEBUG
									printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
                                    if (SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																fcdata->softevent,fcdata->markbit))
                                    {
                                    #ifdef OC_DEBUG
                                        printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
									*(fcdata->markbit) |= 0x0800;
								}
							}
							else
							{
							#ifdef OC_DEBUG
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
            					#ifdef OC_DEBUG
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
                                #ifdef OC_DEBUG
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
			}//if (FD_ISSET(*(fcdata->conpipe),&nread))
		}//if (cpret > 0)
	}//0

	return SUCCESS;
}
