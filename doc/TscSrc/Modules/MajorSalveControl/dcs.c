/* 
 * File:   tc.h
 * Author: sk
 *
 * Created on 2013-8-2
 */

#include "dcs.h"
int set_induction_screen(unsigned char *dchan,unsigned char *schan)
{
	unsigned char		*p = NULL;
	unsigned char		*p1 = NULL;
	unsigned char		*p2 = NULL;
	p2 = dchan;
	p = schan;
	while (1)
	{//while(1)
		if (0 == *p)
			break;
		if ((1 == *p) || (2 == *p) || (3 == *p) || (4 == *p))
		{
			p1 = schan;
			while (1)
			{
				if (0 == *p1)
				{
					break;
				}
				*p2 = *p1;
				p2++;	
				p1++;
			}
			if (1 == *p)
			{
				*p2 = 0x0c;
				p2++;
			}
			if (2 == *p)
			{
				*p2 = 0x0b;
				p2++;
			}
			if (3 == *p)
			{
				*p2 = 0x09;
				p2++;
			}
			if (4 == *p)
			{
				*p2 = 0x0a;
				p2++;
			}
			break;
		}
		p++;
	}//while(1)	

	return SUCCESS;
}


int ms_get_phase_info(fcdata_t *fd,tscdata_t *td,unsigned char tcline,unsigned char slnum,dcpinfo_t *pinfo)
{
	if ((NULL == td) || (NULL == pinfo) || (NULL == fd))
	{
	#ifdef FULL_DETECT_DEBUG
		printf("pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char			phaseid = 0;
	unsigned char			i = 0,j = 0,z = 0,k = 0,s = 0,x = 0;
	unsigned char			nphaseid = 0;
	unsigned char			nchan[MAX_CHANNEL] = {0};
	unsigned char			exist = 0;
	unsigned char 			tchan = 0;

	pinfo->stageline = td->timeconfig->TimeConfigList[tcline][slnum].StageId;
	get_phase_id(td->timeconfig->TimeConfigList[tcline][slnum].PhaseId,&phaseid);

	//get next pass phase id
	if (0 == (td->timeconfig->TimeConfigList[tcline][slnum+1].StageId))
	{
		get_phase_id(td->timeconfig->TimeConfigList[tcline][0].PhaseId,&nphaseid);
	}
	else
	{
		get_phase_id(td->timeconfig->TimeConfigList[tcline][slnum+1].PhaseId,&nphaseid);
	}
	//end get of next pass phase id

	pinfo->phaseid = phaseid;
	pinfo->ytime = td->timeconfig->TimeConfigList[tcline][slnum].YellowTime;
	pinfo->rtime = td->timeconfig->TimeConfigList[tcline][slnum].RedTime;
	for (i = 0; i < (td->phase->FactPhaseNum); i++)
	{
		if (0 == (td->phase->PhaseList[i].PhaseId))
			break;
		if (phaseid == (td->phase->PhaseList[i].PhaseId))
		{
			pinfo->gftime = td->phase->PhaseList[i].PhaseGreenFlash;
			pinfo->phasetype = td->phase->PhaseList[i].PhaseType;
			pinfo->mingtime = td->phase->PhaseList[i].PhaseMinGreen;
			pinfo->maxgtime1 = td->phase->PhaseList[i].PhaseMaxGreen1;
			pinfo->fixgtime = td->phase->PhaseList[i].PhaseFixGreen;
			pinfo->pgtime = td->phase->PhaseList[i].PhaseWalkGreen;
			pinfo->pctime = td->phase->PhaseList[i].PhaseWalkClear;
			pinfo->pdelay = td->phase->PhaseList[i].PhaseGreenDelay;
			pinfo->ldeti = td->phase->PhaseList[i].PhaseReserved & 0x0f;
			pinfo->gtime = td->timeconfig->TimeConfigList[tcline][slnum].GreenTime - pinfo->gftime;
			break;
		}
	}

	j = 0;
	z = 0;
	k = 0;
	s = 0;
	x = 0;
	for (i = 0; i < (td->channel->FactChannelNum); i++)
	{
		if (0 == (td->channel->ChannelList[i].ChannelId))
			break;
		if (nphaseid == (td->channel->ChannelList[i].ChannelCtrlSrc))
		{
		#ifdef CLOSE_LAMP
            tchan = td->channel->ChannelList[i].ChannelId;
            if ((tchan >= 0x05) && (tchan <= 0x0c))
            {
                if (*(fd->specfunc) & (0x01 << (tchan - 5)))
                    continue;
            }
        #else
			if ((*(fd->specfunc) & 0x10) && (*(fd->specfunc) & 0x20))
            {
                tchan = td->channel->ChannelList[i].ChannelId;
                if (((5 <= tchan) && (tchan <= 8)) || ((9 <= tchan) && (tchan <= 12)))
                    continue;
            }
			if ((*(fd->specfunc) & 0x10) && (!(*(fd->specfunc) & 0x20)))
			{
				tchan = td->channel->ChannelList[i].ChannelId;
				if ((5 <= tchan) && (tchan <= 8))
					continue;
			}
			if ((*(fd->specfunc) & 0x20) && (!(*(fd->specfunc) & 0x10)))
			{
				tchan = td->channel->ChannelList[i].ChannelId;
                if ((9 <= tchan) && (tchan <= 12))
                    continue;
			}
		#endif
			nchan[s] = td->channel->ChannelList[i].ChannelId;
			pinfo->npchan[s] = td->channel->ChannelList[i].ChannelId;
			s++;
		}
	}
	for (i = 0; i < (td->channel->FactChannelNum); i++)
	{//0000000000
		if (0 == (td->channel->ChannelList[i].ChannelId))
			break;
		if (phaseid == (td->channel->ChannelList[i].ChannelCtrlSrc))
		{
		#ifdef CLOSE_LAMP
            tchan = td->channel->ChannelList[i].ChannelId;
            if ((tchan >= 0x05) && (tchan <= 0x0c))
            {
                if (*(fd->specfunc) & (0x01 << (tchan - 5)))
                    continue;
            }
        #else
			if ((*(fd->specfunc) & 0x10) && (*(fd->specfunc) & 0x20))
            {
                tchan = td->channel->ChannelList[i].ChannelId;
				if (((5 <= tchan) && (tchan <= 8)) || ((9 <= tchan) && (tchan <= 12)))
                    continue;
            }
			if ((*(fd->specfunc) & 0x10) && (!(*(fd->specfunc) & 0x20)))
            {
                tchan = td->channel->ChannelList[i].ChannelId;
                if ((5 <= tchan) && (tchan <= 8))
                    continue;
            }
            if ((*(fd->specfunc) & 0x20) && (!(*(fd->specfunc) & 0x10)))
            {
                tchan = td->channel->ChannelList[i].ChannelId;
                if ((9 <= tchan) && (tchan <= 12))
                    continue;
            }
		#endif
			pinfo->chan[k] = td->channel->ChannelList[i].ChannelId;
			k++;
			exist = 0;
			for (s = 0; s < MAX_CHANNEL; s++)
			{
				if (0 == nchan[s])
					break;
				if (nchan[s] == (td->channel->ChannelList[i].ChannelId))
				{
					exist = 1;
					break;
				}
			}
			if (0 == exist)
			{//current channel is not exist in channls of next phase;
				pinfo->cchan[x] = td->channel->ChannelList[i].ChannelId;
				x++;
				if (3 == (td->channel->ChannelList[i].ChannelType))
				{
					pinfo->cpchan[z] = td->channel->ChannelList[i].ChannelId;
					z++;
					pinfo->cpcexist = 1;
					continue;
				}
				else
				{
					pinfo->cnpchan[j] = td->channel->ChannelList[i].ChannelId;
					j++;
					continue;
				}
			}//current channel is not exist in channls of next phase;
			else
			{
				continue;
			}	
		}
		else
		{
			continue;
		}
	}//000000000

	return SUCCESS;
}

int ms_get_timeconfig(tscdata_t *td,unsigned char *pattern,unsigned char *tcline)
{
	if ((NULL == tcline) || (NULL == td) || (NULL == pattern))
	{
	#ifdef FULL_DETECT_DEBUG
		printf("pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	unsigned char			i = 0;
	unsigned char			tcid = 0;
	unsigned char            exit = 0;

	for (i = 0; i < (td->pattern->FactPatternNum); i++)
	{
		if (0 == (td->pattern->PatternList[i].PatternId))
			break;
		if (*pattern == (td->pattern->PatternList[i].PatternId))
		{
			tcid = td->pattern->PatternList[i].TimeConfigId;
			break;
		}
	}
	if (0 == tcid)
	{
	#ifdef FULL_DETECT_DEBUG
		printf("Not have fit timeconfigid,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return FAIL;
	}
	else
	{
		for (i = 0; i < (td->timeconfig->FactTimeConfigNum); i++)
		{
			if (0 == (td->timeconfig->TimeConfigList[i][0].TimeConfigId))
				break;
			if (tcid == (td->timeconfig->TimeConfigList[i][0].TimeConfigId))
			{
				*tcline = i;
				exit = 1;
				break;
			}
		}
	}
	if(0 == exit)
		return FAIL;
	return SUCCESS;
}

int ms_set_lamp_color(int serial,unsigned char *chan,unsigned char color)
{
	if (NULL == chan)
	{
	#ifdef FULL_DETECT_DEBUG
		printf("pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	unsigned char			i = 0;
	unsigned char			sdata[4] = {0};
	sdata[0] = 0xAA;
	sdata[2] = color;
	sdata[3] = 0xED;
	
	for (i = 0; i < MAX_CHANNEL; i++)
	{
		if (0 == chan[i])
			break;
		sdata[1] = chan[i];
		if (!wait_write_serial(serial))
		{
			if (write(serial,sdata,sizeof(sdata)) < 0)
			{
			#ifdef FULL_DETECT_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				return FAIL;
			}
		}
		else
		{
		#ifdef FULL_DETECT_DEBUG
			printf("serial port cannot be written,File:%s,Line:%d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
	}
	//send end mark to serial port
	sdata[1] = 0;
	if (!wait_write_serial(serial))
	{
		if (write(serial,sdata,sizeof(sdata)) < 0)
		{
		#ifdef FULL_DETECT_DEBUG
			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
	}
	else
	{
	#ifdef FULL_DETECT_DEBUG
		printf("serial port cannot be written,File:%s,Line:%d\n",__FILE__,__LINE__);
	#endif
		return FAIL;
	}

	return SUCCESS;
}
int ms_backup_pattern_data(int serial,unsigned char snum,phasedetect_t *phd)
{
	if ((NULL == phd) || (0 == snum))
	{
	#ifdef FULL_DETECT_DEBUG
		printf("Pointer address err or stage is 0,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char	i = 0;
	unsigned int	temp = 0;
	unsigned char	bpd[6] = {0xA0,0,snum,0,0,0xED};
	//Firstly,send total stage number;
	if (!wait_write_serial(serial))
    {
        if (write(serial,bpd,sizeof(bpd)) < 0)
        {
        #ifdef FULL_DETECT_DEBUG
            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
            return FAIL;
        }
    }
    else
    {
    #ifdef FULL_DETECT_DEBUG
        printf("serial port cannot be written,File:%s,Line:%d\n",__FILE__,__LINE__);
    #endif
        return FAIL;
    }	

	//Secondly,begin to every stage data;
	
	for (i = 0;i < snum;i++)
	{//for (i = 0;i < snum;i++)
        memset(bpd,0,sizeof(bpd));
    	bpd[0] = 0xA0;
	    bpd[5] = 0xED; 
		if (0 == phd[i].stageid)
			break;
		bpd[1] = phd[i].stageid;
		temp = phd[i].chans;
		temp &= 0x000000ff;
		bpd[2] |= temp;
		temp = phd[i].chans;
		temp &= 0x0000ff00;
		temp >>= 8;
		temp &= 0x000000ff;
		bpd[3] |= temp;
		bpd[4] = phd[i].gtime + phd[i].gftime;
				
		if (!wait_write_serial(serial))
		{
			if (write(serial,bpd,sizeof(bpd)) < 0)
			{
			#ifdef FULL_DETECT_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				return FAIL;
			}
		}
		else
		{
		#ifdef FULL_DETECT_DEBUG
			printf("serial port cannot be written,File:%s,Line:%d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
	}//for (i = 0;i < snum;i++)	

	return SUCCESS;
}

int ms_dirch_control(markdata_c *mdt,unsigned char cktem,unsigned char ktem,unsigned char (*dirch)[8],dcpinfo_t *pinfo)
{
	if ((NULL == mdt) || (NULL == pinfo) || (NULL == *dirch))
	{
	#ifdef FULL_DETECT_DEBUG
        printf("pointer address error,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char			i = 0,j = 0,k = 0,z = 0;
	unsigned char           echo[3] = {0};
	echo[0] = 0xCA;
	echo[1] = ktem;
	echo[2] = 0xED;
	unsigned char			exist = 0;
	unsigned char			pcexist = 0;
	unsigned char           *csta = NULL;
    unsigned char           tcsta = 0;
	unsigned char           sibuf[64] = {0};
	unsigned char			step = 0;
    unsigned char           cchan[32] = {0};
    unsigned char           cpchan[32] = {0};
    unsigned char           cnpchan[32] = {0};
    unsigned char           *pchan = NULL;
	timedown_t              tctd;
	unsigned char           fbdata[6] = {0};
    fbdata[0] = 0xC1;
    fbdata[5] = 0xED;
	unsigned char			ngf = 0;
	struct timeval          to,ct;

	if ((1 == *(mdt->redl)) || (1 == *(mdt->yfl)) || (1 == *(mdt->closel)))
	{//all red or all close or yellow flash
		if (1 == *(mdt->yfl))
		{
			pthread_cancel(*(mdt->yfid));
			pthread_join(*(mdt->yfid),NULL);
			*(mdt->yfl) = 0;
			new_all_red(mdt->ardata);
		}
		*(mdt->redl) = 0;	
		if (1 == *(mdt->closel))
		{
			*(mdt->closel) = 0;
			new_all_red(mdt->ardata);
		}
		if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),dirch[ktem-0x20],0x02))
		{
		#ifdef FULL_DETECT_DEBUG
			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			*(mdt->fcdata->markbit) |= 0x0800;
			return FAIL;
		}
		if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
									dirch[ktem-0x20],0x02,mdt->fcdata->markbit))
		{
		#ifdef FULL_DETECT_DEBUG
			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}

		if (!wait_write_serial(*(mdt->fcdata->sdserial)))
		{
			if (write(*(mdt->fcdata->sdserial),echo,sizeof(echo)) < 0)
			{
			#ifdef FULL_DETECT_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			else
			{
			#ifdef FULL_DETECT_DEBUG
				printf("write success,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
		}
		else
		{
		#ifdef FULL_DETECT_DEBUG
			printf("face board serial port cannot write,Line:%d\n",__LINE__);
		#endif
		}
		if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
		{//report info to server actively
			mdt->sinfo->conmode = 28;
			mdt->sinfo->color = 0x02;
			mdt->sinfo->time = 0;
			mdt->sinfo->stage = pinfo->stageline;
			mdt->sinfo->cyclet = 0;
			mdt->sinfo->phase = 0;
			mdt->sinfo->chans = 0;
			memset(mdt->sinfo->csta,0,sizeof(mdt->sinfo->csta));
			csta = mdt->sinfo->csta;
			for (i = 0; i < MAX_CHANNEL; i++)
			{
				if (0 == dirch[ktem-0x20][i])
					break;
				mdt->sinfo->chans += 1;
				tcsta = dirch[ktem-0x20][i];
				tcsta <<= 2;
				tcsta &= 0xfc;
				tcsta |= 0x02; //00000010-green 
				*csta = tcsta;
				csta++;
			}
			memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));
			memset(sibuf,0,sizeof(sibuf));
			if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}
			else
			{
				write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
			}
		}//report info to server actively
	}//all red or all close or yellow flash
	else
	{//other condition
		memset(cchan,0,sizeof(cchan));
        memset(cpchan,0,sizeof(cpchan));
        memset(cnpchan,0,sizeof(cnpchan));
		pchan = cchan;
		k = 0;
		z = 0;
		pcexist = 0;
		if ((0 == *(mdt->kstep)) && (cktem > 0))
		{//front is not step control;
			for (i = 0; i < MAX_CHANNEL; i++)
			{//1
				if (0 == dirch[cktem-0x20][i])
					break;
				exist = 0;
				for (j = 0; j < MAX_CHANNEL; j++)
				{//2
					if (0 == dirch[ktem-0x20][j])
						break;
					if (dirch[cktem-0x20][i] == dirch[ktem-0x20][j])
					{
						exist = 1;
						break;
					}
				}//2
				if (0 == exist)
				{
					*pchan = dirch[cktem-0x20][i];
					pchan++;
					if ((13 <= dirch[cktem-0x20][i]) && (dirch[cktem-0x20][i] <= 16))
					{
						cpchan[k] = dirch[cktem-0x20][i];
						k++;
						pcexist = 1;
					}
					else
					{
						cnpchan[z] = dirch[cktem-0x20][i];
						z++;
					}
				}		
			}//1
		}//front is not step control;
		else if (0 == cktem) 
		{//front is step control or other;
			*(mdt->kstep) = 0;
			for (i = 0; i < MAX_CHANNEL; i++)
			{//1
				if (0 == pinfo->chan[i])
					break;
				exist = 0;
				for (j = 0; j < MAX_CHANNEL; j++)
				{//2
					if (0 == dirch[ktem-0x20][j])
						break;
					if (pinfo->chan[i] == dirch[ktem-0x20][j])
					{
						exist = 1;
						break;
					}
				}//2
				if (0 == exist)
				{
					*pchan = pinfo->chan[i];
					pchan++;
					if ((13 <= pinfo->chan[i]) && (pinfo->chan[i] <= 16))
					{
						cpchan[k] = pinfo->chan[i];
						k++;
						pcexist = 1;
					}
					else
					{
						cnpchan[z] = pinfo->chan[i];
						z++;
					}
				}		
			}//1
		}//front is step control or other;

		//Begin to green flash
		if ((0==cchan[0])&&(pinfo->gftime>0)||(pinfo->ytime>0)||(pinfo->rtime>0))
		{
			if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
			{//report info to server actively
				mdt->sinfo->conmode = 28;
				mdt->sinfo->color = 0x02;
				mdt->sinfo->time = pinfo->gftime + pinfo->ytime + pinfo->rtime;
				mdt->sinfo->stage = pinfo->stageline;
				mdt->sinfo->cyclet = 0;
				mdt->sinfo->phase = 0;
										
				mdt->sinfo->chans = 0;
				memset(mdt->sinfo->csta,0,sizeof(mdt->sinfo->csta));
				csta = mdt->sinfo->csta;
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == cchan[i])
						break;
					mdt->sinfo->chans += 1;
					tcsta = cchan[i];
					tcsta <<= 2;
					tcsta &= 0xfc;
					tcsta |= 0x02;
					*csta = tcsta;
					csta++;
				}
				memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));
				memset(sibuf,0,sizeof(sibuf));
				if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
				}
			}//report info to server actively
		}//if ((0==cchan[0])&&(pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0))

		if ((0 != cchan[0]) && (pinfo->gftime > 0))
		{
			if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
			{//report info to server actively
				mdt->sinfo->conmode = 28;
				mdt->sinfo->color = 0x03;
				mdt->sinfo->time = pinfo->gftime;
				mdt->sinfo->stage = pinfo->stageline;
				mdt->sinfo->cyclet = 0;
				mdt->sinfo->phase = 0;
				
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == cchan[i])
						break;
					for (j = 0; j < mdt->sinfo->chans; j++)
					{
						if (0 == mdt->sinfo->csta[j])
							break;
						tcsta = mdt->sinfo->csta[j];
						tcsta &= 0xfc;
						tcsta >>= 2;
						tcsta &= 0x3f;
						if (cchan[i] == tcsta)
						{
							mdt->sinfo->csta[j] &= 0xfc;
							mdt->sinfo->csta[j] |= 0x03; //00000001-green flash
							break;
						}
					}
				}								
				memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));	
				memset(sibuf,0,sizeof(sibuf));
				if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
				}
			}//report info to server actively
		}//if ((0 != cchan[0]) && (pinfo.gftime > 0))

		//current phase begin to green flash
		if ((*(mdt->fcdata->markbit) & 0x0002) && (*(mdt->fcdata->markbit) & 0x0010))
		{
			memset(&tctd,0,sizeof(tctd));
			tctd.mode = 28;//traffic control
			tctd.pattern = *(mdt->fcdata->patternid);
			tctd.lampcolor = 0x02;
			tctd.lamptime = pinfo->gftime;
			tctd.phaseid = 0;
			tctd.stageline = pinfo->stageline;
			if (SUCCESS != timedown_data_to_conftool(mdt->fcdata->sockfd,&tctd))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}
		}
		#ifdef EMBED_CONFIGURE_TOOL
		if (*(mdt->fcdata->markbit2) & 0x0200)
		{
			memset(&tctd,0,sizeof(tctd));
			tctd.mode = 28;//traffic control
			tctd.pattern = *(mdt->fcdata->patternid);
			tctd.lampcolor = 0x02;
			tctd.lamptime = pinfo->gftime;
			tctd.phaseid = 0;
			tctd.stageline = pinfo->stageline;	
			if (SUCCESS != timedown_data_to_embed(mdt->fcdata->sockfd,&tctd))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}
		}
		#endif
		fbdata[1] = 28;
		fbdata[2] = pinfo->stageline;
		fbdata[3] = 0x02;
		fbdata[4] = pinfo->gftime;
		if (!wait_write_serial(*(mdt->fcdata->fbserial)))
		{
			if (write(*(mdt->fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
			{
			#ifdef FULL_DETECT_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
		}
		else
		{
		#ifdef FULL_DETECT_DEBUG
			printf("face board serial port cannot write,Line:%d\n",__LINE__);
		#endif
		}

		ngf = 0;
		if ((0 != cchan[0]) && (pinfo->gftime > 0))
		{
			while (1)
			{//green flash
				if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x03))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;	
				}
				if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
								cchan,0x03,mdt->fcdata->markbit))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				to.tv_sec = 0;
				to.tv_usec = 500000;
				select(0,NULL,NULL,NULL,&to);

				if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x02))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
								cchan,0x02,mdt->fcdata->markbit))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				to.tv_sec = 0;
				to.tv_usec = 500000;
				select(0,NULL,NULL,NULL,&to);
			
				ngf += 1;
				if (ngf >= pinfo->gftime)
					break;
			}//green flash
		}//if ((0 != cchan[0]) && (pinfo->gftime > 0))
		//end green flash
		if (1 == pcexist)
		{
			//current phase begin to yellow lamp
			if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cnpchan,0x01))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
								cnpchan,0x01, mdt->fcdata->markbit))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
			if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cpchan,0x00))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
												cpchan,0x00,mdt->fcdata->markbit))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
		}
		else
		{
			if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x01))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
									cchan,0x01,mdt->fcdata->markbit))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
		}
		
		if ((0 != cchan[0]) && (pinfo->ytime > 0))
		{
			if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
			{//report info to server actively
				mdt->sinfo->conmode = 28;
				mdt->sinfo->color = 0x01;
				mdt->sinfo->time = pinfo->ytime;
				mdt->sinfo->stage = pinfo->stageline;
				mdt->sinfo->cyclet = 0;
				mdt->sinfo->phase = 0;
										
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == cnpchan[i])
						break;
					for (j = 0; j < mdt->sinfo->chans; j++)
					{
						if (0 == mdt->sinfo->csta[j])
							break;
						tcsta = mdt->sinfo->csta[j];
						tcsta &= 0xfc;
						tcsta >>= 2;
						tcsta &= 0x3f;
						if (cnpchan[i] == tcsta)
						{
							mdt->sinfo->csta[j] &= 0xfc;
							mdt->sinfo->csta[j] |= 0x01; //00000001-yellow
							break;
						}
					}
				}
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == cpchan[i])
						break;
					for (j = 0; j < mdt->sinfo->chans; j++)
					{
						if (0 == mdt->sinfo->csta[j])
							break;
						tcsta = mdt->sinfo->csta[j];
						tcsta &= 0xfc;
						tcsta >>= 2;
						tcsta &= 0x3f;
						if (cpchan[i] == tcsta)
						{
							mdt->sinfo->csta[j] &= 0xfc;
							break;
						}
					}
				}
				memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));
				memset(sibuf,0,sizeof(sibuf));
				if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
				}
			}//report info to server actively
		}//if ((0 != cchan[0]) && (pinfo.ytime > 0))

		if ((*(mdt->fcdata->markbit) & 0x0002) && (*(mdt->fcdata->markbit) & 0x0010))
		{
			memset(&tctd,0,sizeof(tctd));
			tctd.mode = 28;//traffic control
			tctd.pattern = *(mdt->fcdata->patternid);
			tctd.lampcolor = 0x01;
			tctd.lamptime = pinfo->ytime;
			tctd.phaseid = 0;
			tctd.stageline = pinfo->stageline;
			if (SUCCESS != timedown_data_to_conftool(mdt->fcdata->sockfd,&tctd))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}
		}
		#ifdef EMBED_CONFIGURE_TOOL
		if (*(mdt->fcdata->markbit2) & 0x0200)
		{
			memset(&tctd,0,sizeof(tctd));
			tctd.mode = 28;//traffic control
			tctd.pattern = *(mdt->fcdata->patternid);
			tctd.lampcolor = 0x01;
			tctd.lamptime = pinfo->ytime;
			tctd.phaseid = 0;
			tctd.stageline = pinfo->stageline;	
			if (SUCCESS != timedown_data_to_embed(mdt->fcdata->sockfd,&tctd))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}
		}
		#endif
		fbdata[1] = 28;
		fbdata[2] = pinfo->stageline;
		fbdata[3] = 0x01;
		fbdata[4] = pinfo->ytime;
		if (!wait_write_serial(*(mdt->fcdata->fbserial)))
		{
			if (write(*(mdt->fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
			{
			#ifdef FULL_DETECT_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
		}
		else
		{
		#ifdef FULL_DETECT_DEBUG
			printf("face board serial port cannot write,Line:%d\n",__LINE__);
		#endif
		}
		sleep(pinfo->ytime);

		if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x00))
		{
		#ifdef FULL_DETECT_DEBUG
			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			*(mdt->fcdata->markbit) |= 0x0800;
		}
		if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
						cchan,0x00,mdt->fcdata->markbit))
		{
		#ifdef FULL_DETECT_DEBUG
			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}
		if ((0 != cchan[0]) && (pinfo->rtime > 0))
		{
			if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
			{//report info to server actively
				mdt->sinfo->conmode = 28;
				mdt->sinfo->color = 0x00;
				mdt->sinfo->time = pinfo->rtime;
				mdt->sinfo->stage = pinfo->stageline;
				mdt->sinfo->cyclet = 0;
				mdt->sinfo->phase = 0;
										
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == cchan[i])
						break;
					for (j = 0; j < mdt->sinfo->chans; j++)
					{
						if (0 == mdt->sinfo->csta[j])
							break;
						tcsta = mdt->sinfo->csta[j];
						tcsta &= 0xfc;
						tcsta >>= 2;
						tcsta &= 0x3f;
						if (cchan[i] == tcsta)
						{
							mdt->sinfo->csta[j] &= 0xfc;
							break;
						}
					}
				}
				memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));	
				memset(sibuf,0,sizeof(sibuf));
				if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
				}
			}//report info to server actively
		}//if ((0 != cchan[0]) && (pinfo.rtime > 0))

		if (pinfo->rtime > 0)
		{
			if ((*(mdt->fcdata->markbit) & 0x0002) && (*(mdt->fcdata->markbit) & 0x0010))
			{
				memset(&tctd,0,sizeof(tctd));
				tctd.mode = 28;//traffic control
				tctd.pattern = *(mdt->fcdata->patternid);
				tctd.lampcolor = 0x00;
				tctd.lamptime = pinfo->rtime;
				tctd.phaseid = 0;
				tctd.stageline = pinfo->stageline;
				if (SUCCESS != timedown_data_to_conftool(mdt->fcdata->sockfd,&tctd))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#ifdef EMBED_CONFIGURE_TOOL
			if (*(mdt->fcdata->markbit2) & 0x0200)
			{
				memset(&tctd,0,sizeof(tctd));
				tctd.mode = 28;//traffic control
				tctd.pattern = *(mdt->fcdata->patternid);
				tctd.lampcolor = 0x00;
				tctd.lamptime = pinfo->rtime;
				tctd.phaseid = 0;
				tctd.stageline = pinfo->stageline;
				if (SUCCESS != timedown_data_to_embed(mdt->fcdata->sockfd,&tctd))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#endif
			fbdata[1] = 28;
			fbdata[2] = pinfo->stageline;
			fbdata[3] = 0x00;
			fbdata[4] = pinfo->rtime;
			if (!wait_write_serial(*(mdt->fcdata->fbserial)))
			{
				if (write(*(mdt->fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
				{
				#ifdef FULL_DETECT_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
			}
			else
			{
			#ifdef FULL_DETECT_DEBUG
				printf("face board serial port cannot write,Line:%d\n",__LINE__);
			#endif
			}
			sleep(pinfo->rtime);
		}
		
		if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),dirch[ktem-0x20],0x02))
		{
		#ifdef FULL_DETECT_DEBUG
			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			*(mdt->fcdata->markbit) |= 0x0800;
		}
		if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
						dirch[ktem-0x20],0x02,mdt->fcdata->markbit))
		{
		#ifdef FULL_DETECT_DEBUG
			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}
		if (!wait_write_serial(*(mdt->fcdata->sdserial)))
		{
			if (write(*(mdt->fcdata->sdserial),echo,sizeof(echo)) < 0)
			{
			#ifdef FULL_DETECT_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			else
			{
			#ifdef FULL_DETECT_DEBUG
				printf("write success,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
		}
		else
		{
		#ifdef FULL_DETECT_DEBUG
			printf("face board serial port cannot write,Line:%d\n",__LINE__);
		#endif
		}

		if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
		{//report info to server actively
			mdt->sinfo->conmode = 28;
			mdt->sinfo->color = 0x02;
			mdt->sinfo->time = 0;
			mdt->sinfo->stage = pinfo->stageline;
			mdt->sinfo->cyclet = 0;
			mdt->sinfo->phase = 0;
			mdt->sinfo->chans = 0;
			memset(mdt->sinfo->csta,0,sizeof(mdt->sinfo->csta));
			csta = mdt->sinfo->csta;
			for (i = 0; i < MAX_CHANNEL; i++)
			{
				if (0 == dirch[ktem-0x20][i])
					break;
				mdt->sinfo->chans += 1;
				tcsta = dirch[ktem-0x20][i];
				tcsta <<= 2;
				tcsta &= 0xfc;
				tcsta |= 0x02; //00000010-green 
				*csta = tcsta;
				csta++;
			}
			memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));
			memset(sibuf,0,sizeof(sibuf));
			if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}
			else
			{
				write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
			}
		}//report info to server actively

		if ((*(mdt->fcdata->markbit) & 0x0002) && (*(mdt->fcdata->markbit) & 0x0010))
		{
			memset(&tctd,0,sizeof(tctd));
			tctd.mode = 28;//traffic control
			tctd.pattern = *(mdt->fcdata->patternid);
			tctd.lampcolor = 0x02;
			tctd.lamptime = 0;
			tctd.phaseid = 0;
			tctd.stageline = pinfo->stageline;
			if (SUCCESS != timedown_data_to_conftool(mdt->fcdata->sockfd,&tctd))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}
		}
		#ifdef EMBED_CONFIGURE_TOOL
		if (*(mdt->fcdata->markbit2) & 0x0200)
		{
			memset(&tctd,0,sizeof(tctd));
			tctd.mode = 28;//traffic control
			tctd.pattern = *(mdt->fcdata->patternid);
			tctd.lampcolor = 0x02;
			tctd.lamptime = 0;
			tctd.phaseid = 0;
			tctd.stageline = pinfo->stageline;	
			if (SUCCESS != timedown_data_to_embed(mdt->fcdata->sockfd,&tctd))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}
		}
		#endif
		fbdata[1] = 28;
		fbdata[2] = pinfo->stageline;
		fbdata[3] = 0x02;
		fbdata[4] = 0;
		if (!wait_write_serial(*(mdt->fcdata->fbserial)))
		{
			if (write(*(mdt->fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
			{
			#ifdef FULL_DETECT_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
		}
		else
		{
		#ifdef FULL_DETECT_DEBUG
			printf("face board serial port cannot write,Line:%d\n",__LINE__);
		#endif
		}
	}//other condition

	return SUCCESS;
}

int ms_jump_stage_control(markdata_c *mdt,unsigned char staid,dcpinfo_t *pinfo)
{
	if (NULL == mdt)
	{
	#ifdef FULL_DETECT_DEBUG
		printf("pointer address error,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char			find = 0;
	unsigned char			nchan[32] = {0};
	unsigned char			cchan[32] = {0};
	unsigned char			cpchan[32] = {0};
	unsigned char			cnpchan[32] = {0};
	unsigned char			*pchan = NULL;
	unsigned char           *csta = NULL;
	unsigned char           tcsta = 0;
	unsigned char           sibuf[64] = {0};
	unsigned char           err[10] = {0};
	unsigned char			i = 0,j = 0,s = 0,k = 0;
	unsigned int			nphidi = 0;
	unsigned char			nphidc = 0;
	unsigned char			pcexist = 0;
	unsigned char			num = 0;
	unsigned char			echo[3] = {0};
	unsigned char			stageid = 0;
	unsigned char			cstageid = 0;
	unsigned char			ngf = 0;
	unsigned char			exist = 0;
	struct timeval			to,ct;
	echo[0] = 0xCA;
	echo[1] = staid;
	echo[2] = 0xED;
	unsigned char           fbdata[6] = {0};
    fbdata[0] = 0xC1;
    fbdata[5] = 0xED;
	timedown_t              tctd;

	stageid = staid & 0x0f;//hign 4 bit will be clean 0;
	cstageid = mdt->tscdata->timeconfig->TimeConfigList[*(mdt->rettl)][*(mdt->fcdata->slnum)].StageId;

	if ((1 == *(mdt->redl)) || (1 == *(mdt->yfl)) || (1 == *(mdt->closel)))
	{//all red or all close or yellow flash
		if (1 == *(mdt->yfl))
		{
			pthread_cancel(*(mdt->yfid));
			pthread_join(*(mdt->yfid),NULL);
			*(mdt->yfl) = 0;
			new_all_red(mdt->ardata);
		}
		*(mdt->redl) = 0;	
		if (1 == *(mdt->closel))
		{
			*(mdt->closel) = 0;
			new_all_red(mdt->ardata);
		}
		find = 0;
		for (i = 0; i < (mdt->tscdata->timeconfig->FactStageNum); i++)
		{//find whether stageid is in TimeConfigList;
			if (0 == (mdt->tscdata->timeconfig->TimeConfigList[*(mdt->rettl)][i].StageId))
				break;
			if (stageid == (mdt->tscdata->timeconfig->TimeConfigList[*(mdt->rettl)][i].StageId))
			{
				find = 1;
				num = i;
				break;
			}
		}//find whether stageid is in TimeConfigList;
		if (0 == find)
		{
		#ifdef FULL_DETECT_DEBUG
			printf("Stage id is not in TimeconfigList,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
		
		if (stageid == cstageid)
		{//Controlling stageid is current stageid;
		#ifdef FULL_DETECT_DEBUG
			printf("Controlling stageid is current stageid,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),pinfo->chan,0x02))
            {
            #ifdef FULL_DETECT_DEBUG
                printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
                *(mdt->fcdata->markbit) |= 0x0800;
                return FAIL;
            }
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
                                pinfo->chan,0x02,mdt->fcdata->markbit))
            {
            #ifdef FULL_DETECT_DEBUG
                printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
			if (!wait_write_serial(*(mdt->fcdata->sdserial)))
			{
				if (write(*(mdt->fcdata->sdserial),echo,sizeof(echo)) < 0)
				{
				#ifdef FULL_DETECT_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
				else
				{
				#ifdef FULL_DETECT_DEBUG
					printf("write success,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
			}
			else
			{
			#ifdef FULL_DETECT_DEBUG
				printf("face board serial port cannot write,Line:%d\n",__LINE__);
			#endif
			}
			if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
			{//report info to server actively
				mdt->sinfo->conmode = 28;
				mdt->sinfo->color = 0x02;
				mdt->sinfo->time = 0;
				mdt->sinfo->stage = pinfo->stageline;
				mdt->sinfo->cyclet = 0;
				mdt->sinfo->phase = 0;
				mdt->sinfo->phase |= (0x01 << (pinfo->phaseid - 1));
				mdt->sinfo->chans = 0;
				memset(mdt->sinfo->csta,0,sizeof(mdt->sinfo->csta));
				csta = mdt->sinfo->csta;
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == pinfo->chan[i])
						break;
					mdt->sinfo->chans += 1;
					tcsta = pinfo->chan[i];
					tcsta <<= 2;
					tcsta &= 0xfc;
					tcsta |= 0x02; //00000010-green 
					*csta = tcsta;
					csta++;
				}
				memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));
				memset(sibuf,0,sizeof(sibuf));
				if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
				}
			#if 0
				memset(err,0,sizeof(err));
				gettimeofday(&ct,NULL);
				if (SUCCESS != err_report(err,ct.tv_sec,22,5))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					write(*(mdt->fcdata->sockfd->csockfd),err,sizeof(err));
				}
				update_event_list(mdt->fcdata->ec,mdt->fcdata->el,1,31,ct.tv_sec,mdt->fcdata->markbit);
			#endif
			}//report info to server actively
			else
			{
			#if 0
				gettimeofday(&ct,NULL);
				update_event_list(mdt->fcdata->ec,mdt->fcdata->el,1,31,ct.tv_sec,mdt->fcdata->markbit);
			#endif
			}
		}//Controlling stageid is current stageid;
		else
		{//Controlling stageid is not current stageid;
		#ifdef FULL_DETECT_DEBUG
			printf("Controlling stageid is not current stageid,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			memset(pinfo,0,sizeof(dcpinfo_t));
			if (SUCCESS != ms_get_phase_info(mdt->fcdata,mdt->tscdata,*(mdt->rettl),num,pinfo))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				return FAIL;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
                                pinfo->chan,0x02,mdt->fcdata->markbit))
            {
            #ifdef FULL_DETECT_DEBUG
                printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
			if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),pinfo->chan,0x02))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
				return FAIL;
			}
			else
			{//set success
				*(mdt->fcdata->slnum) = num;
				*(mdt->fcdata->stageid) = pinfo->stageline;
				*(mdt->fcdata->phaseid) = 0;
				*(mdt->fcdata->phaseid) |= (0x01 << (pinfo->phaseid - 1));
				if (!wait_write_serial(*(mdt->fcdata->sdserial)))
				{
					if (write(*(mdt->fcdata->sdserial),echo,sizeof(echo)) < 0)
					{
					#ifdef FULL_DETECT_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						*(mdt->fcdata->markbit) |= 0x0800;
					}
					else
                    {
                    #ifdef FULL_DETECT_DEBUG
                        printf("write success,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                    }
				}
				else
				{
				#ifdef FULL_DETECT_DEBUG
					printf("face board serial port cannot write,Line:%d\n",__LINE__);
				#endif
				}

				if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
				{//report info to server actively
					mdt->sinfo->conmode = 28;
					mdt->sinfo->color = 0x02;
					mdt->sinfo->time = 0;
					mdt->sinfo->stage = pinfo->stageline;
					mdt->sinfo->cyclet = 0;
					mdt->sinfo->phase = 0;
					mdt->sinfo->phase |= (0x01 << (pinfo->phaseid - 1));
					mdt->sinfo->chans = 0;
					memset(mdt->sinfo->csta,0,sizeof(mdt->sinfo->csta));
					csta = mdt->sinfo->csta;
					for (i = 0; i < MAX_CHANNEL; i++)
					{
						if (0 == pinfo->chan[i])
							break;
						mdt->sinfo->chans += 1;
						tcsta = pinfo->chan[i];
						tcsta <<= 2;
						tcsta &= 0xfc;
						tcsta |= 0x02; //00000010-green 
						*csta = tcsta;
						csta++;
					}
					memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));
					memset(sibuf,0,sizeof(sibuf));
					if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
					{
					#ifdef FULL_DETECT_DEBUG
						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
					else
					{
						write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
					}
				#if 0
					memset(err,0,sizeof(err));
					gettimeofday(&ct,NULL);
					if (SUCCESS != err_report(err,ct.tv_sec,22,5))
					{
					#ifdef FULL_DETECT_DEBUG
						printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					else
					{
						write(*(mdt->fcdata->sockfd->csockfd),err,sizeof(err));
					}
					update_event_list(mdt->fcdata->ec,mdt->fcdata->el,1,31,ct.tv_sec,mdt->fcdata->markbit);
				#endif
				}//report info to server actively
				else
				{
				#if 0
					gettimeofday(&ct,NULL);
					update_event_list(mdt->fcdata->ec,mdt->fcdata->el,1,31,ct.tv_sec,mdt->fcdata->markbit);
				#endif
				}

			}//set success
		}//Controlling stageid is not current stageid;
		return SUCCESS;
	}//all red or all close or yellow flash
	else
	{//other condition
		if (stageid == cstageid)
		{
		#ifdef FULL_DETECT_DEBUG
			printf("Controlling stageid is current stageid,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			if (!wait_write_serial(*(mdt->fcdata->sdserial)))
			{
				if (write(*(mdt->fcdata->sdserial),echo,sizeof(echo)) < 0)
				{
				#ifdef FULL_DETECT_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
				else
				{
				#ifdef FULL_DETECT_DEBUG
					printf("write success,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
			}
			else
			{
			#ifdef FULL_DETECT_DEBUG
				printf("face board serial port cannot write,Line:%d\n",__LINE__);
			#endif
			}
			return SUCCESS;
		}
		else
		{//Controlling stageid is not current stageid
		#ifdef FULL_DETECT_DEBUG
			printf("Current stage begins to transit,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			find = 0;
			//begin to compute change channels
			for (i = 0; i < (mdt->tscdata->timeconfig->FactStageNum); i++)
			{//find whether stageid is in TimeConfigList;
				if (0 == (mdt->tscdata->timeconfig->TimeConfigList[*(mdt->rettl)][i].StageId))
					break;
				if (stageid == (mdt->tscdata->timeconfig->TimeConfigList[*(mdt->rettl)][i].StageId))
				{
					nphidi = mdt->tscdata->timeconfig->TimeConfigList[*(mdt->rettl)][i].PhaseId;
					get_phase_id(nphidi,&nphidc);
					find = 1;
					num = i;
					break;
				}
			}//find whether stageid is in TimeConfigList;
			if (0 == find)
			{
			#ifdef FULL_DETECT_DEBUG
				printf("Stage id is not in TimeconfigList,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				return FAIL;
			}
			pchan = nchan;	
			for (i = 0; i < (mdt->tscdata->channel->FactChannelNum); i++)
			{//1
				if (0 == (mdt->tscdata->channel->ChannelList[i].ChannelId))
					break;
				if (nphidc == (mdt->tscdata->channel->ChannelList[i].ChannelCtrlSrc))
				{
					*pchan = mdt->tscdata->channel->ChannelList[i].ChannelId;
					pchan++;
				}
			}//1
			pchan = cchan;
			s = 0;
			k = 0;
			pcexist = 0;
			for (i = 0; i < MAX_CHANNEL; i++)
			{//i
				if (0 == pinfo->chan[i])
					break;
				exist = 0;
				for (j = 0; j < MAX_CHANNEL; j++ )
				{//j
					if (0 == nchan[j])
						break;
					if (pinfo->chan[i] == nchan[j])
					{
						exist = 1;
						break;
					}			
				}//j
				if (0 == exist)
				{//if (0 == exist)
					*pchan = pinfo->chan[i];
					pchan++;
					for (j = 0; j < (mdt->tscdata->channel->FactChannelNum); j++)
					{//2j
						if (0 == (mdt->tscdata->channel->ChannelList[j].ChannelId))
							break;
						if (pinfo->chan[i] == (mdt->tscdata->channel->ChannelList[j].ChannelId))
						{
							if (3 == mdt->tscdata->channel->ChannelList[j].ChannelType)
							{
								cpchan[s] = pinfo->chan[i];
								s++;
								pcexist = 1;		
							}
							else
							{
								cnpchan[k] = pinfo->chan[i];
								k++;
							}
							break;
						}
					}//2j
				}//if (0 == exist)
			}//i
			//end compute
			if ((0==cchan[0])&&(pinfo->gftime>0)||(pinfo->ytime>0)||(pinfo->rtime>0))
			{
				if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
				{//report info to server actively
					mdt->sinfo->conmode = 28;
					mdt->sinfo->color = 0x02;
					mdt->sinfo->time = pinfo->gftime + pinfo->ytime + pinfo->rtime;
					mdt->sinfo->stage = pinfo->stageline;
					mdt->sinfo->cyclet = 0;
					mdt->sinfo->phase = 0;
					mdt->sinfo->phase |= (0x01 << (pinfo->phaseid - 1));
											
					mdt->sinfo->chans = 0;
					memset(mdt->sinfo->csta,0,sizeof(mdt->sinfo->csta));
					csta = mdt->sinfo->csta;
					for (i = 0; i < MAX_CHANNEL; i++)
					{
						if (0 == cchan[i])
							break;
						mdt->sinfo->chans += 1;
						tcsta = cchan[i];
						tcsta <<= 2;
						tcsta &= 0xfc;
						tcsta |= 0x02;
						*csta = tcsta;
						csta++;
					}
					memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));
					memset(sibuf,0,sizeof(sibuf));
					if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
					{
					#ifdef FULL_DETECT_DEBUG
						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
					else
					{
						write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
					}
				}//report info to server actively
			}//if ((0==cchan[0])&&(pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0))

			if ((0 != cchan[0]) && (pinfo->gftime > 0))
			{
				if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
				{//report info to server actively
					mdt->sinfo->conmode = 28;
					mdt->sinfo->color = 0x03;
					mdt->sinfo->time = pinfo->gftime;
					mdt->sinfo->stage = pinfo->stageline;
					mdt->sinfo->cyclet = 0;
					mdt->sinfo->phase = 0;
					mdt->sinfo->phase |= (0x01 << (pinfo->phaseid - 1));
					
					for (i = 0; i < MAX_CHANNEL; i++)
					{
						if (0 == cchan[i])
							break;
						for (j = 0; j < mdt->sinfo->chans; j++)
						{
							if (0 == mdt->sinfo->csta[j])
								break;
							tcsta = mdt->sinfo->csta[j];
							tcsta &= 0xfc;
							tcsta >>= 2;
							tcsta &= 0x3f;
							if (cchan[i] == tcsta)
							{
								mdt->sinfo->csta[j] &= 0xfc;
								mdt->sinfo->csta[j] |= 0x03; //00000001-green flash
								break;
							}
						}
					}								
					memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));	
					memset(sibuf,0,sizeof(sibuf));
					if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
					{
					#ifdef FULL_DETECT_DEBUG
						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
					else
					{
						write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
					}
				}//report info to server actively
			}//if ((0 != cchan[0]) && (pinfo.gftime > 0))

			//current phase begin to green flash
			if ((*(mdt->fcdata->markbit) & 0x0002) && (*(mdt->fcdata->markbit) & 0x0010))
			{
				memset(&tctd,0,sizeof(tctd));
				tctd.mode = 28;//traffic control
				tctd.pattern = *(mdt->fcdata->patternid);
				tctd.lampcolor = 0x02;
				tctd.lamptime = pinfo->gftime;
				tctd.phaseid = pinfo->phaseid;
				tctd.stageline = pinfo->stageline;
				if (SUCCESS != timedown_data_to_conftool(mdt->fcdata->sockfd,&tctd))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#ifdef EMBED_CONFIGURE_TOOL
			if (*(mdt->fcdata->markbit2) & 0x0200)
			{
				memset(&tctd,0,sizeof(tctd));
				tctd.mode = 28;//traffic control
				tctd.pattern = *(mdt->fcdata->patternid);
				tctd.lampcolor = 0x02;
				tctd.lamptime = pinfo->gftime;
				tctd.phaseid = pinfo->phaseid;
				tctd.stageline = pinfo->stageline;	
				if (SUCCESS != timedown_data_to_embed(mdt->fcdata->sockfd,&tctd))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#endif
			fbdata[1] = 28;
			fbdata[2] = pinfo->stageline;
			fbdata[3] = 0x02;
			fbdata[4] = pinfo->gftime;
			if (!wait_write_serial(*(mdt->fcdata->fbserial)))
			{
				if (write(*(mdt->fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
				{
				#ifdef FULL_DETECT_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
			}
			else
			{
			#ifdef FULL_DETECT_DEBUG
				printf("face board serial port cannot write,Line:%d\n",__LINE__);
			#endif
			}

			ngf = 0;
			while (1)
			{//green flash
				if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x03))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;	
				}
				if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
								cchan,0x03,mdt->fcdata->markbit))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				to.tv_sec = 0;
				to.tv_usec = 500000;
				select(0,NULL,NULL,NULL,&to);

				if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x02))
                {
                #ifdef FULL_DETECT_DEBUG
                    printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                    *(mdt->fcdata->markbit) |= 0x0800;
                }
                if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
                                cchan,0x02,mdt->fcdata->markbit))
                {
                #ifdef FULL_DETECT_DEBUG
                    printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
				to.tv_sec = 0;
				to.tv_usec = 500000;
				select(0,NULL,NULL,NULL,&to);
			
				ngf += 1;
				if (ngf >= pinfo->gftime)
					break;
			}//green flash

			if (1 == pcexist)
			{
				//current phase begin to yellow lamp
				if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cnpchan,0x01))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
									cnpchan,0x01, mdt->fcdata->markbit))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cpchan,0x00))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
													cpchan,0x00,mdt->fcdata->markbit))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
			}
			else
			{
				if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x01))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
										cchan,0x01,mdt->fcdata->markbit))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
			}
			
			if ((0 != cchan[0]) && (pinfo->ytime > 0))
			{
				if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
				{//report info to server actively
					mdt->sinfo->conmode = 28;
					mdt->sinfo->color = 0x01;
					mdt->sinfo->time = pinfo->ytime;
					mdt->sinfo->stage = pinfo->stageline;
					mdt->sinfo->cyclet = 0;
					mdt->sinfo->phase = 0;
					mdt->sinfo->phase |= (0x01 << (pinfo->phaseid - 1));
											
					for (i = 0; i < MAX_CHANNEL; i++)
					{
						if (0 == cnpchan[i])
							break;
						for (j = 0; j < mdt->sinfo->chans; j++)
						{
							if (0 == mdt->sinfo->csta[j])
								break;
							tcsta = mdt->sinfo->csta[j];
							tcsta &= 0xfc;
							tcsta >>= 2;
							tcsta &= 0x3f;
							if (cnpchan[i] == tcsta)
							{
								mdt->sinfo->csta[j] &= 0xfc;
								mdt->sinfo->csta[j] |= 0x01; //00000001-yellow
								break;
							}
						}
					}
					for (i = 0; i < MAX_CHANNEL; i++)
					{
						if (0 == cpchan[i])
							break;
						for (j = 0; j < mdt->sinfo->chans; j++)
						{
							if (0 == mdt->sinfo->csta[j])
								break;
							tcsta = mdt->sinfo->csta[j];
							tcsta &= 0xfc;
							tcsta >>= 2;
							tcsta &= 0x3f;
							if (cpchan[i] == tcsta)
							{
								mdt->sinfo->csta[j] &= 0xfc;
								break;
							}
						}
					}
					memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));
					memset(sibuf,0,sizeof(sibuf));
					if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
					{
					#ifdef FULL_DETECT_DEBUG
						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
					else
					{
						write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
					}
				}//report info to server actively
			}//if ((0 != cchan[0]) && (pinfo.ytime > 0))

			if ((*(mdt->fcdata->markbit) & 0x0002) && (*(mdt->fcdata->markbit) & 0x0010))
			{
				memset(&tctd,0,sizeof(tctd));
				tctd.mode = 28;//traffic control
				tctd.pattern = *(mdt->fcdata->patternid);
				tctd.lampcolor = 0x01;
				tctd.lamptime = pinfo->ytime;
				tctd.phaseid = pinfo->phaseid;
				tctd.stageline = pinfo->stageline;
				if (SUCCESS != timedown_data_to_conftool(mdt->fcdata->sockfd,&tctd))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#ifdef EMBED_CONFIGURE_TOOL
			if (*(mdt->fcdata->markbit2) & 0x0200)
			{
				memset(&tctd,0,sizeof(tctd));
				tctd.mode = 28;//traffic control
				tctd.pattern = *(mdt->fcdata->patternid);
				tctd.lampcolor = 0x01;
				tctd.lamptime = pinfo->ytime;
				tctd.phaseid = pinfo->phaseid;
				tctd.stageline = pinfo->stageline;	
				if (SUCCESS != timedown_data_to_embed(mdt->fcdata->sockfd,&tctd))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#endif
			fbdata[1] = 28;
			fbdata[2] = pinfo->stageline;
			fbdata[3] = 0x01;
			fbdata[4] = pinfo->ytime;
			if (!wait_write_serial(*(mdt->fcdata->fbserial)))
			{
				if (write(*(mdt->fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
				{
				#ifdef FULL_DETECT_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
			}
			else
			{
			#ifdef FULL_DETECT_DEBUG
				printf("face board serial port cannot write,Line:%d\n",__LINE__);
			#endif
			}

			sleep(pinfo->ytime);

			if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x00))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
							cchan,0x00,mdt->fcdata->markbit))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
			if ((0 != cchan[0]) && (pinfo->rtime > 0))
			{
				if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
				{//report info to server actively
					mdt->sinfo->conmode = 28;
					mdt->sinfo->color = 0x00;
					mdt->sinfo->time = pinfo->rtime;
					mdt->sinfo->stage = pinfo->stageline;
					mdt->sinfo->cyclet = 0;
					mdt->sinfo->phase = 0;
					mdt->sinfo->phase |= (0x01 << (pinfo->phaseid - 1));
											
					for (i = 0; i < MAX_CHANNEL; i++)
					{
						if (0 == cchan[i])
							break;
						for (j = 0; j < mdt->sinfo->chans; j++)
						{
							if (0 == mdt->sinfo->csta[j])
								break;
							tcsta = mdt->sinfo->csta[j];
							tcsta &= 0xfc;
							tcsta >>= 2;
							tcsta &= 0x3f;
							if (cchan[i] == tcsta)
							{
								mdt->sinfo->csta[j] &= 0xfc;
								break;
							}
						}
					}
					memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));	
					memset(sibuf,0,sizeof(sibuf));
					if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
					{
					#ifdef FULL_DETECT_DEBUG
						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
					else
					{
						write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
					}
				}//report info to server actively
			}//if ((0 != cchan[0]) && (pinfo.rtime > 0))

			if (pinfo->rtime > 0)
			{
				if ((*(mdt->fcdata->markbit) & 0x0002) && (*(mdt->fcdata->markbit) & 0x0010))
				{
					memset(&tctd,0,sizeof(tctd));
					tctd.mode = 28;//traffic control
					tctd.pattern = *(mdt->fcdata->patternid);
					tctd.lampcolor = 0x00;
					tctd.lamptime = pinfo->rtime;
					tctd.phaseid = pinfo->phaseid;
					tctd.stageline = pinfo->stageline;
					if (SUCCESS != timedown_data_to_conftool(mdt->fcdata->sockfd,&tctd))
					{
					#ifdef FULL_DETECT_DEBUG
						printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
				}
				#ifdef EMBED_CONFIGURE_TOOL
				if (*(mdt->fcdata->markbit2) & 0x0200)
				{
					memset(&tctd,0,sizeof(tctd));
					tctd.mode = 28;//traffic control
					tctd.pattern = *(mdt->fcdata->patternid);
					tctd.lampcolor = 0x00;
					tctd.lamptime = pinfo->rtime;
					tctd.phaseid = pinfo->phaseid;
					tctd.stageline = pinfo->stageline;
					if (SUCCESS != timedown_data_to_embed(mdt->fcdata->sockfd,&tctd))
					{
					#ifdef FULL_DETECT_DEBUG
						printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
				}
				#endif
				fbdata[1] = 28;
				fbdata[2] = pinfo->stageline;
				fbdata[3] = 0x00;
				fbdata[4] = pinfo->rtime;
				if (!wait_write_serial(*(mdt->fcdata->fbserial)))
				{
					if (write(*(mdt->fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
					{
					#ifdef FULL_DETECT_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						*(mdt->fcdata->markbit) |= 0x0800;
					}
				}
				else
				{
				#ifdef FULL_DETECT_DEBUG
					printf("face board serial port cannot write,Line:%d\n",__LINE__);
				#endif
				}
				sleep(pinfo->rtime);
			}


			memset(pinfo,0,sizeof(dcpinfo_t));
			if (SUCCESS != ms_get_phase_info(mdt->fcdata,mdt->tscdata,*(mdt->rettl),num,pinfo))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				return FAIL;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
                            pinfo->chan,0x02,mdt->fcdata->markbit))
            {
            #ifdef FULL_DETECT_DEBUG
                printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
			if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),pinfo->chan,0x02))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
				return FAIL;
			}
			else
			{//set success
				*(mdt->fcdata->slnum) = num;
				*(mdt->fcdata->stageid) = pinfo->stageline;
				*(mdt->fcdata->phaseid) = 0;
				*(mdt->fcdata->phaseid) |= (0x01 << (pinfo->phaseid - 1));

				if (!wait_write_serial(*(mdt->fcdata->sdserial)))
				{
					if (write(*(mdt->fcdata->sdserial),echo,sizeof(echo)) < 0)
					{
					#ifdef FULL_DETECT_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						*(mdt->fcdata->markbit) |= 0x0800;
					}
					else
					{
					#ifdef FULL_DETECT_DEBUG
						printf("write success,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
				}
				else
				{
				#ifdef FULL_DETECT_DEBUG
					printf("face board serial port cannot write,Line:%d\n",__LINE__);
				#endif
				}

				if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
				{//report info to server actively
					mdt->sinfo->conmode = 28;
					mdt->sinfo->color = 0x02;
					mdt->sinfo->time = 0;
					mdt->sinfo->stage = pinfo->stageline;
					mdt->sinfo->cyclet = 0;
					mdt->sinfo->phase = 0;
					mdt->sinfo->phase |= (0x01 << (pinfo->phaseid - 1));
					mdt->sinfo->chans = 0;
					memset(mdt->sinfo->csta,0,sizeof(mdt->sinfo->csta));
					csta = mdt->sinfo->csta;
					for (i = 0; i < MAX_CHANNEL; i++)
					{
						if (0 == pinfo->chan[i])
							break;
						mdt->sinfo->chans += 1;
						tcsta = pinfo->chan[i];
						tcsta <<= 2;
						tcsta &= 0xfc;
						tcsta |= 0x02; //00000010-green 
						*csta = tcsta;
						csta++;
					}
					memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));
					memset(sibuf,0,sizeof(sibuf));
					if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
					{
					#ifdef FULL_DETECT_DEBUG
						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
					else
					{
						write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
					}
				#if 0
					memset(err,0,sizeof(err));
					gettimeofday(&ct,NULL);
					if (SUCCESS != err_report(err,ct.tv_sec,22,5))
					{
					#ifdef FULL_DETECT_DEBUG
						printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					else
					{
						write(*(mdt->fcdata->sockfd->csockfd),err,sizeof(err));
					}
					update_event_list(mdt->fcdata->ec,mdt->fcdata->el,1,31,ct.tv_sec,mdt->fcdata->markbit);
				#endif
				}//report info to server actively
				else
				{
				#if 0
					gettimeofday(&ct,NULL);
					update_event_list(mdt->fcdata->ec,mdt->fcdata->el,1,31,ct.tv_sec,mdt->fcdata->markbit);
				#endif
				}

				if ((*(mdt->fcdata->markbit) & 0x0002) && (*(mdt->fcdata->markbit) & 0x0010))
				{
					memset(&tctd,0,sizeof(tctd));
					tctd.mode = 28;//traffic control
					tctd.pattern = *(mdt->fcdata->patternid);
					tctd.lampcolor = 0x02;
					tctd.lamptime = 0;
					tctd.phaseid = pinfo->phaseid;
					tctd.stageline = pinfo->stageline;
					if (SUCCESS != timedown_data_to_conftool(mdt->fcdata->sockfd,&tctd))
					{
					#ifdef FULL_DETECT_DEBUG
						printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
				}
				#ifdef EMBED_CONFIGURE_TOOL
				if (*(mdt->fcdata->markbit2) & 0x0200)
				{
					memset(&tctd,0,sizeof(tctd));
					tctd.mode = 28;//traffic control
					tctd.pattern = *(mdt->fcdata->patternid);
					tctd.lampcolor = 0x02;
					tctd.lamptime = 0;
					tctd.phaseid = pinfo->phaseid;
					tctd.stageline = pinfo->stageline;	
					if (SUCCESS != timedown_data_to_embed(mdt->fcdata->sockfd,&tctd))
					{
					#ifdef FULL_DETECT_DEBUG
						printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
				}
				#endif
				fbdata[1] = 28;
				fbdata[2] = pinfo->stageline;
				fbdata[3] = 0x02;
				fbdata[4] = 0;
				if (!wait_write_serial(*(mdt->fcdata->fbserial)))
				{
					if (write(*(mdt->fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
					{
					#ifdef FULL_DETECT_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						*(mdt->fcdata->markbit) |= 0x0800;
					}
				}
				else
				{
				#ifdef FULL_DETECT_DEBUG
					printf("face board serial port cannot write,Line:%d\n",__LINE__);
				#endif
				}	

				return SUCCESS;
			}//set success
		}//Controlling stageid is not current stageid
	}//other condition
}


#ifdef RED_FLASH
void ms_red_flash(void *arg)
{
	redflash_dc			*rdt = arg;	

	//Firstly,compute channels will be red flash;
	unsigned char			nphaseid = 0; //front phase id
	unsigned char			nchan[MAX_CHANNEL] = {0};
	unsigned char			i = 0,j = 0;
	unsigned char			cchan[MAX_CHANNEL] = {0};
	unsigned char			*pcchan = cchan;
	unsigned char			exist = 0;
	struct timeval			to;
	unsigned char			rftn = 0;
	unsigned char			tchan = 0;

	if (0 == (rdt->dc->td->timeconfig->TimeConfigList[rdt->tcline][rdt->slnum+1].StageId))
	{
		get_phase_id(rdt->dc->td->timeconfig->TimeConfigList[rdt->tcline][0].PhaseId,&nphaseid);
	}
	else
	{
		get_phase_id(rdt->dc->td->timeconfig->TimeConfigList[rdt->tcline][rdt->slnum+1].PhaseId,&nphaseid);
	}
	

	for (i = 0; i < (rdt->dc->td->channel->FactChannelNum); i++)
	{
		if (0 == (rdt->dc->td->channel->ChannelList[i].ChannelId))
			break;
		if (nphaseid == (rdt->dc->td->channel->ChannelList[i].ChannelCtrlSrc))
		{
		#ifdef CLOSE_LAMP
            tchan = rdt->dc->td->channel->ChannelList[i].ChannelId;
            if ((tchan >= 0x05) && (tchan <= 0x0c))
            {
                if (*(rdt->dc->fd->specfunc) & (0x01 << (tchan - 5)))
                    continue;
            }
        #else
            if ((*(rdt->dc->fd->specfunc) & 0x10) && (*(rdt->dc->fd->specfunc) & 0x20))
            {
                tchan = rdt->dc->td->channel->ChannelList[i].ChannelId;
                if (((5 <= tchan) && (tchan <= 8)) || ((9 <= tchan) && (tchan <= 12)))
                    continue;
            }
            if ((*(rdt->dc->fd->specfunc) & 0x10) && (!(*(rdt->dc->fd->specfunc) & 0x20)))
            {
                tchan = rdt->dc->td->channel->ChannelList[i].ChannelId;
                if ((5 <= tchan) && (tchan <= 8))
                    continue;
            }
            if ((*(rdt->dc->fd->specfunc) & 0x20) && (!(*(rdt->dc->fd->specfunc) & 0x10)))
            {
                tchan = rdt->dc->td->channel->ChannelList[i].ChannelId;
                if ((9 <= tchan) && (tchan <= 12))
                    continue;
            }
        #endif
			nchan[j] = rdt->dc->td->channel->ChannelList[i].ChannelId;
			j++;
		}
	}

	for (i = 0; i < MAX_CHANNEL; i++)
	{//i
		if (0 == nchan[i])
			break;
		exist = 0;
		for (j = 0; j < MAX_CHANNEL; j++)
		{//j
			if (0 == rdt->chan[j])
				break;
			if (nchan[i] == rdt->chan[j])
			{
				exist = 1;
				break;
			}
		}//j
		if (0 == exist)
		{
			*pcchan = nchan[i];
			pcchan++;
		}
	}//i

	//begin to red flash
	if ( (!(*(rdt->dc->fd->markbit) & 0x1000)) && (!(*(rdt->dc->fd->markbit) & 0x8000)) )
    {
        red_flash_begin_report(rdt->dc->fd->sockfd->csockfd,cchan);
    }
	while (1)
	{//while
		if (SUCCESS != ms_set_lamp_color(*(rdt->dc->fd->bbserial),cchan,0x03))
		{
		#ifdef FULL_DETECT_DEBUG
			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}		
		if (SUCCESS != update_channel_status(rdt->dc->fd->sockfd,rdt->dc->cs,cchan,0x03,rdt->dc->fd->markbit))
		{
		#ifdef FULL_DETECT_DEBUG
			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}
		to.tv_sec = 0;
		to.tv_usec = 500000;
		select(0,NULL,NULL,NULL,&to);		

		if (SUCCESS != ms_set_lamp_color(*(rdt->dc->fd->bbserial),cchan,0x00))
		{
		#ifdef FULL_DETECT_DEBUG
			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}		
		if (SUCCESS != update_channel_status(rdt->dc->fd->sockfd,rdt->dc->cs,cchan,0x00,rdt->dc->fd->markbit))
		{
		#ifdef FULL_DETECT_DEBUG
			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}
		to.tv_sec = 0;
		to.tv_usec = 500000;
		select(0,NULL,NULL,NULL,&to);

		rftn += 1;
		if (rftn >= (rdt->rft))
			break;	
	}//while
	if ( (!(*(rdt->dc->fd->markbit) & 0x1000)) && (!(*(rdt->dc->fd->markbit) & 0x8000)) )
    {
        red_flash_end_report(rdt->dc->fd->sockfd->csockfd);
    }	

	pthread_detach(pthread_self());
    pthread_exit(NULL);
}
#endif

//mobile achieve jump phase control
int ms_mobile_jp_control(markdata_c *mdt,unsigned char staid,dcpinfo_t *pinfo,unsigned char *dirch)
{
	if ((NULL == mdt) || (NULL == pinfo) || (NULL == dirch))
	{
	#ifdef FULL_DETECT_DEBUG
		printf("pointer address error,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char			find = 0;
	unsigned char			nchan[32] = {0};
	unsigned char			cchan[32] = {0};
	unsigned char			cpchan[32] = {0};
	unsigned char			cnpchan[32] = {0};
	unsigned char			*pchan = NULL;
	unsigned char           *csta = NULL;
	unsigned char           tcsta = 0;
	unsigned char           sibuf[64] = {0};
	unsigned char           err[10] = {0};
	unsigned char			i = 0,j = 0,s = 0,k = 0;
	unsigned int			nphidi = 0;
	unsigned char			nphidc = 0;
	unsigned char			pcexist = 0;
	unsigned char			num = 0;
	unsigned char			stageid = 0;
	unsigned char			cstageid = 0;
	unsigned char			ngf = 0;
	unsigned char			exist = 0;
	struct timeval			to,ct;
	unsigned char           fbdata[6] = {0};
    fbdata[0] = 0xC1;
    fbdata[5] = 0xED;
	timedown_t              tctd;

	stageid = staid;// & 0x0f;//hign 4 bit will be clean 0;
	cstageid = mdt->tscdata->timeconfig->TimeConfigList[*(mdt->rettl)][*(mdt->fcdata->slnum)].StageId;

	if ((1 == *(mdt->redl)) || (1 == *(mdt->yfl)) || (1 == *(mdt->closel)))
	{//all red or all close or yellow flash
		if (1 == *(mdt->yfl))
		{
			pthread_cancel(*(mdt->yfid));
			pthread_join(*(mdt->yfid),NULL);
			*(mdt->yfl) = 0;
			new_all_red(mdt->ardata);
		}
		*(mdt->redl) = 0;	
		if (1 == *(mdt->closel))
		{
			*(mdt->closel) = 0;
			new_all_red(mdt->ardata);
		}
		find = 0;
		for (i = 0; i < (mdt->tscdata->timeconfig->FactStageNum); i++)
		{//find whether stageid is in TimeConfigList;
			if (0 == (mdt->tscdata->timeconfig->TimeConfigList[*(mdt->rettl)][i].StageId))
				break;
			if (stageid == (mdt->tscdata->timeconfig->TimeConfigList[*(mdt->rettl)][i].StageId))
			{
				find = 1;
				num = i;
				break;
			}
		}//find whether stageid is in TimeConfigList;
		if (0 == find)
		{
		#ifdef FULL_DETECT_DEBUG
			printf("Stage id is not in TimeconfigList,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
		
		if (stageid == cstageid)
		{//Controlling stageid is current stageid;
		#ifdef FULL_DETECT_DEBUG
			printf("Controlling stageid is current stageid,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),pinfo->chan,0x02))
            {
            #ifdef FULL_DETECT_DEBUG
                printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
                *(mdt->fcdata->markbit) |= 0x0800;
                return FAIL;
            }
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
                                pinfo->chan,0x02,mdt->fcdata->markbit))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}

			if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
			{//report info to server actively
				mdt->sinfo->conmode = 28;
				mdt->sinfo->color = 0x02;
				mdt->sinfo->time = 0;
				mdt->sinfo->stage = pinfo->stageline;
				mdt->sinfo->cyclet = 0;
				mdt->sinfo->phase = 0;
				mdt->sinfo->phase |= (0x01 << (pinfo->phaseid - 1));
				mdt->sinfo->chans = 0;
				memset(mdt->sinfo->csta,0,sizeof(mdt->sinfo->csta));
				csta = mdt->sinfo->csta;
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == pinfo->chan[i])
						break;
					mdt->sinfo->chans += 1;
					tcsta = pinfo->chan[i];
					tcsta <<= 2;
					tcsta &= 0xfc;
					tcsta |= 0x02; //00000010-green 
					*csta = tcsta;
					csta++;
				}
				memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));
				memset(sibuf,0,sizeof(sibuf));
				if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
				}
			}//report info to server actively
		}//Controlling stageid is current stageid;
		else
		{//Controlling stageid is not current stageid;
		#ifdef FULL_DETECT_DEBUG
			printf("Controlling stageid is not current stageid,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			memset(pinfo,0,sizeof(dcpinfo_t));
			if (SUCCESS != ms_get_phase_info(mdt->fcdata,mdt->tscdata,*(mdt->rettl),num,pinfo))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				return FAIL;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
                                pinfo->chan,0x02,mdt->fcdata->markbit))
            {   
            #ifdef FULL_DETECT_DEBUG
                printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
			if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),pinfo->chan,0x02))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
				return FAIL;
			}
			else
			{//set success
				*(mdt->fcdata->slnum) = num;
				*(mdt->fcdata->stageid) = pinfo->stageline;
				*(mdt->fcdata->phaseid) = 0;
				*(mdt->fcdata->phaseid) |= (0x01 << (pinfo->phaseid - 1));

				if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
				{//report info to server actively
					mdt->sinfo->conmode = 28;
					mdt->sinfo->color = 0x02;
					mdt->sinfo->time = 0;
					mdt->sinfo->stage = pinfo->stageline;
					mdt->sinfo->cyclet = 0;
					mdt->sinfo->phase = 0;
					mdt->sinfo->phase |= (0x01 << (pinfo->phaseid - 1));
					mdt->sinfo->chans = 0;
					memset(mdt->sinfo->csta,0,sizeof(mdt->sinfo->csta));
					csta = mdt->sinfo->csta;
					for (i = 0; i < MAX_CHANNEL; i++)
					{
						if (0 == pinfo->chan[i])
							break;
						mdt->sinfo->chans += 1;
						tcsta = pinfo->chan[i];
						tcsta <<= 2;
						tcsta &= 0xfc;
						tcsta |= 0x02; //00000010-green 
						*csta = tcsta;
						csta++;
					}
					memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));
					memset(sibuf,0,sizeof(sibuf));
					if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
					{
					#ifdef FULL_DETECT_DEBUG
						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
					else
					{
						write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
					}
				}//report info to server actively
			}//set success
		}//Controlling stageid is not current stageid;
		return SUCCESS;
	}//all red or all close or yellow flash
	else if (1 == *(mdt->dircon))
	{//direction control happen
		find = 0;
		//begin to compute change channels
		for (i = 0; i < (mdt->tscdata->timeconfig->FactStageNum); i++)
		{//find whether stageid is in TimeConfigList;
			if (0 == (mdt->tscdata->timeconfig->TimeConfigList[*(mdt->rettl)][i].StageId))
				break;
			if (stageid == (mdt->tscdata->timeconfig->TimeConfigList[*(mdt->rettl)][i].StageId))
			{
				nphidi = mdt->tscdata->timeconfig->TimeConfigList[*(mdt->rettl)][i].PhaseId;
				get_phase_id(nphidi,&nphidc);
				find = 1;
				num = i;
				*(mdt->dircon) = 0;
				break;
			}
		}//find whether stageid is in TimeConfigList;
		if (0 == find)
		{
		#ifdef FULL_DETECT_DEBUG
			printf("Stage id is not in TimeconfigList,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
		pchan = nchan;	
		for (i = 0; i < (mdt->tscdata->channel->FactChannelNum); i++)
		{//1
			if (0 == (mdt->tscdata->channel->ChannelList[i].ChannelId))
				break;
			if (nphidc == (mdt->tscdata->channel->ChannelList[i].ChannelCtrlSrc))
			{
				*pchan = mdt->tscdata->channel->ChannelList[i].ChannelId;
				pchan++;
			}
		}//1
		pchan = cchan;
		s = 0;
		k = 0;
		pcexist = 0;
		for (i = 0; i < MAX_CHANNEL; i++)
		{//i
			if (0 == dirch[i])
				break;
			exist = 0;
			for (j = 0; j < MAX_CHANNEL; j++ )
			{//j
				if (0 == nchan[j])
					break;
				if (dirch[i] == nchan[j])
				{
					exist = 1;
					break;
				}			
			}//j
			if (0 == exist)
			{//if (0 == exist)
				*pchan = dirch[i];
				pchan++;
				for (j = 0; j < (mdt->tscdata->channel->FactChannelNum); j++)
				{//2j
					if (0 == (mdt->tscdata->channel->ChannelList[j].ChannelId))
						break;
					if (dirch[i] == (mdt->tscdata->channel->ChannelList[j].ChannelId))
					{
						if (3 == mdt->tscdata->channel->ChannelList[j].ChannelType)
						{
							cpchan[s] = dirch[i];
							s++;
							pcexist = 1;		
						}
						else
						{
							cnpchan[k] = dirch[i];
							k++;
						}
						break;
					}
				}//2j
			}//if (0 == exist)
		}//i
		if (0 == cchan[0])
		{
			if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
			{//report info to server actively
				mdt->sinfo->conmode = 28;
				mdt->sinfo->color = 0x02;
				mdt->sinfo->time = 6;//3sec gf + 3sec yellow + 0sec red
				mdt->sinfo->stage = 0;
				mdt->sinfo->cyclet = 0;
				mdt->sinfo->phase = 0;
										
				mdt->sinfo->chans = 0;
				memset(mdt->sinfo->csta,0,sizeof(mdt->sinfo->csta));
				csta = mdt->sinfo->csta;
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == cchan[i])
						break;
					mdt->sinfo->chans += 1;
					tcsta = cchan[i];
					tcsta <<= 2;
					tcsta &= 0xfc;
					tcsta |= 0x02;
					*csta = tcsta;
					csta++;
				}
				memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));
				memset(sibuf,0,sizeof(sibuf));
				if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
				}
			}//report info to server actively
		}//if ((0==cchan[0])&&(pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0))

		if (0 != cchan[0])
		{
			if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
			{//report info to server actively
				mdt->sinfo->conmode = 28;
				mdt->sinfo->color = 0x03;
				mdt->sinfo->time = 3;//default 3 sec gf
				mdt->sinfo->stage = 0;
				mdt->sinfo->cyclet = 0;
				mdt->sinfo->phase = 0;
				
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == cchan[i])
						break;
					for (j = 0; j < mdt->sinfo->chans; j++)
					{
						if (0 == mdt->sinfo->csta[j])
							break;
						tcsta = mdt->sinfo->csta[j];
						tcsta &= 0xfc;
						tcsta >>= 2;
						tcsta &= 0x3f;
						if (cchan[i] == tcsta)
						{
							mdt->sinfo->csta[j] &= 0xfc;
							mdt->sinfo->csta[j] |= 0x03; //00000001-green flash
							break;
						}
					}
				}								
				memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));	
				memset(sibuf,0,sizeof(sibuf));
				if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
				}
			}//report info to server actively
		}//if ((0 != cchan[0]) && (pinfo.gftime > 0))

		//current phase begin to green flash
		if ((*(mdt->fcdata->markbit) & 0x0002) && (*(mdt->fcdata->markbit) & 0x0010))
		{
			memset(&tctd,0,sizeof(tctd));
			tctd.mode = 28;//traffic control
			tctd.pattern = *(mdt->fcdata->patternid);
			tctd.lampcolor = 0x02;
			tctd.lamptime = 3;//default 3 sec gf;
			tctd.phaseid = 0;
			tctd.stageline = 0;
			if (SUCCESS != timedown_data_to_conftool(mdt->fcdata->sockfd,&tctd))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}
		}
		#ifdef EMBED_CONFIGURE_TOOL
		if (*(mdt->fcdata->markbit2) & 0x0200)
		{
			memset(&tctd,0,sizeof(tctd));
			tctd.mode = 28;//traffic control
			tctd.pattern = *(mdt->fcdata->patternid);
			tctd.lampcolor = 0x02;
			tctd.lamptime = 3;
			tctd.phaseid = 0;
			tctd.stageline = 0;	
			if (SUCCESS != timedown_data_to_embed(mdt->fcdata->sockfd,&tctd))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}
		}
		#endif
		fbdata[1] = 28;
		fbdata[2] = 0;
		fbdata[3] = 0x02;
		fbdata[4] = 3;
		if (!wait_write_serial(*(mdt->fcdata->fbserial)))
		{
			if (write(*(mdt->fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
			{
			#ifdef FULL_DETECT_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
		}
		else
		{
		#ifdef FULL_DETECT_DEBUG
			printf("face board serial port cannot write,Line:%d\n",__LINE__);
		#endif
		}

		ngf = 0;
		while (1)
		{//green flash
			if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x03))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;	
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
							cchan,0x03,mdt->fcdata->markbit))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
			to.tv_sec = 0;
			to.tv_usec = 500000;
			select(0,NULL,NULL,NULL,&to);

			if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x02))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
							cchan,0x02,mdt->fcdata->markbit))
			{
			#ifdef FULL_DETECT_DEBUG
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

		if (1 == pcexist)
		{
			//current phase begin to yellow lamp
			if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cnpchan,0x01))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
								cnpchan,0x01, mdt->fcdata->markbit))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
			if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cpchan,0x00))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
												cpchan,0x00,mdt->fcdata->markbit))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
		}
		else
		{
			if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x01))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
									cchan,0x01,mdt->fcdata->markbit))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
		}
		
		if (0 != cchan[0])
		{
			if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
			{//report info to server actively
				mdt->sinfo->conmode = 28;
				mdt->sinfo->color = 0x01;
				mdt->sinfo->time = 0;
				mdt->sinfo->stage = 0;
				mdt->sinfo->cyclet = 0;
				mdt->sinfo->phase = 0;
										
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == cnpchan[i])
						break;
					for (j = 0; j < mdt->sinfo->chans; j++)
					{
						if (0 == mdt->sinfo->csta[j])
							break;
						tcsta = mdt->sinfo->csta[j];
						tcsta &= 0xfc;
						tcsta >>= 2;
						tcsta &= 0x3f;
						if (cnpchan[i] == tcsta)
						{
							mdt->sinfo->csta[j] &= 0xfc;
							mdt->sinfo->csta[j] |= 0x01; //00000001-yellow
							break;
						}
					}
				}
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == cpchan[i])
						break;
					for (j = 0; j < mdt->sinfo->chans; j++)
					{
						if (0 == mdt->sinfo->csta[j])
							break;
						tcsta = mdt->sinfo->csta[j];
						tcsta &= 0xfc;
						tcsta >>= 2;
						tcsta &= 0x3f;
						if (cpchan[i] == tcsta)
						{
							mdt->sinfo->csta[j] &= 0xfc;
							break;
						}
					}
				}
				memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));
				memset(sibuf,0,sizeof(sibuf));
				if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
				}
			}//report info to server actively
		}//if ((0 != cchan[0]) && (pinfo.ytime > 0))

		if ((*(mdt->fcdata->markbit) & 0x0002) && (*(mdt->fcdata->markbit) & 0x0010))
		{
			memset(&tctd,0,sizeof(tctd));
			tctd.mode = 28;//traffic control
			tctd.pattern = *(mdt->fcdata->patternid);
			tctd.lampcolor = 0x01;
			tctd.lamptime = 3;
			tctd.phaseid = 0;
			tctd.stageline = 0;
			if (SUCCESS != timedown_data_to_conftool(mdt->fcdata->sockfd,&tctd))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}
		}
		#ifdef EMBED_CONFIGURE_TOOL
		if (*(mdt->fcdata->markbit2) & 0x0200)
		{
			memset(&tctd,0,sizeof(tctd));
			tctd.mode = 28;//traffic control
			tctd.pattern = *(mdt->fcdata->patternid);
			tctd.lampcolor = 0x01;
			tctd.lamptime = 3;
			tctd.phaseid = 0;
			tctd.stageline = 0;	
			if (SUCCESS != timedown_data_to_embed(mdt->fcdata->sockfd,&tctd))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}
		}
		#endif
		fbdata[1] = 28;
		fbdata[2] = 0;
		fbdata[3] = 0x01;
		fbdata[4] = 3;
		if (!wait_write_serial(*(mdt->fcdata->fbserial)))
		{
			if (write(*(mdt->fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
			{
			#ifdef FULL_DETECT_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
		}
		else
		{
		#ifdef FULL_DETECT_DEBUG
			printf("face board serial port cannot write,Line:%d\n",__LINE__);
		#endif
		}

		sleep(3);

		if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x00))
		{
		#ifdef FULL_DETECT_DEBUG
			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			*(mdt->fcdata->markbit) |= 0x0800;
		}
		if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
						cchan,0x00,mdt->fcdata->markbit))
		{
		#ifdef FULL_DETECT_DEBUG
			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}

		memset(pinfo,0,sizeof(dcpinfo_t));
		if (SUCCESS != ms_get_phase_info(mdt->fcdata,mdt->tscdata,*(mdt->rettl),num,pinfo))
		{
		#ifdef FULL_DETECT_DEBUG
			printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
		if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
						pinfo->chan,0x02,mdt->fcdata->markbit))
		{
		#ifdef FULL_DETECT_DEBUG
			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}
		if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),pinfo->chan,0x02))
		{
		#ifdef FULL_DETECT_DEBUG
			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			*(mdt->fcdata->markbit) |= 0x0800;
			return FAIL;
		}
		else
		{//set success
			*(mdt->fcdata->slnum) = num;
			*(mdt->fcdata->stageid) = pinfo->stageline;
			*(mdt->fcdata->phaseid) = 0;
			*(mdt->fcdata->phaseid) |= (0x01 << (pinfo->phaseid - 1));

			if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
			{//report info to server actively
				mdt->sinfo->conmode = 28;
				mdt->sinfo->color = 0x02;
				mdt->sinfo->time = 0;
				mdt->sinfo->stage = pinfo->stageline;
				mdt->sinfo->cyclet = 0;
				mdt->sinfo->phase = 0;
				mdt->sinfo->phase |= (0x01 << (pinfo->phaseid - 1));
				mdt->sinfo->chans = 0;
				memset(mdt->sinfo->csta,0,sizeof(mdt->sinfo->csta));
				csta = mdt->sinfo->csta;
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == pinfo->chan[i])
						break;
					mdt->sinfo->chans += 1;
					tcsta = pinfo->chan[i];
					tcsta <<= 2;
					tcsta &= 0xfc;
					tcsta |= 0x02; //00000010-green 
					*csta = tcsta;
					csta++;
				}
				memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));
				memset(sibuf,0,sizeof(sibuf));
				if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
				}
			}//report info to server actively

			if ((*(mdt->fcdata->markbit) & 0x0002) && (*(mdt->fcdata->markbit) & 0x0010))
			{
				memset(&tctd,0,sizeof(tctd));
				tctd.mode = 28;//traffic control
				tctd.pattern = *(mdt->fcdata->patternid);
				tctd.lampcolor = 0x02;
				tctd.lamptime = 0;
				tctd.phaseid = pinfo->phaseid;
				tctd.stageline = pinfo->stageline;
				if (SUCCESS != timedown_data_to_conftool(mdt->fcdata->sockfd,&tctd))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#ifdef EMBED_CONFIGURE_TOOL
			if (*(mdt->fcdata->markbit2) & 0x0200)
			{
				memset(&tctd,0,sizeof(tctd));
				tctd.mode = 28;//traffic control
				tctd.pattern = *(mdt->fcdata->patternid);
				tctd.lampcolor = 0x02;
				tctd.lamptime = 0;
				tctd.phaseid = pinfo->phaseid;
				tctd.stageline = pinfo->stageline;	
				if (SUCCESS != timedown_data_to_embed(mdt->fcdata->sockfd,&tctd))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#endif
			fbdata[1] = 28;
			fbdata[2] = pinfo->stageline;
			fbdata[3] = 0x02;
			fbdata[4] = 0;
			if (!wait_write_serial(*(mdt->fcdata->fbserial)))
			{
				if (write(*(mdt->fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
				{
				#ifdef FULL_DETECT_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
			}
			else
			{
			#ifdef FULL_DETECT_DEBUG
				printf("face board serial port cannot write,Line:%d\n",__LINE__);
			#endif
			}	

			return SUCCESS;
		}//set success
	}//direction control happen
	else
	{//other condition
		if (stageid == cstageid)
		{
		#ifdef FULL_DETECT_DEBUG
			printf("Controlling stageid is current stageid,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return SUCCESS;
		}
		else
		{//Controlling stageid is not current stageid
		#ifdef FULL_DETECT_DEBUG
			printf("Current stage begins to transit,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			find = 0;
			//begin to compute change channels
			for (i = 0; i < (mdt->tscdata->timeconfig->FactStageNum); i++)
			{//find whether stageid is in TimeConfigList;
				if (0 == (mdt->tscdata->timeconfig->TimeConfigList[*(mdt->rettl)][i].StageId))
					break;
				if (stageid == (mdt->tscdata->timeconfig->TimeConfigList[*(mdt->rettl)][i].StageId))
				{
					nphidi = mdt->tscdata->timeconfig->TimeConfigList[*(mdt->rettl)][i].PhaseId;
					get_phase_id(nphidi,&nphidc);
					find = 1;
					num = i;
					break;
				}
			}//find whether stageid is in TimeConfigList;
			if (0 == find)
			{
			#ifdef FULL_DETECT_DEBUG
				printf("Stage id is not in TimeconfigList,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				return FAIL;
			}
			pchan = nchan;	
			for (i = 0; i < (mdt->tscdata->channel->FactChannelNum); i++)
			{//1
				if (0 == (mdt->tscdata->channel->ChannelList[i].ChannelId))
					break;
				if (nphidc == (mdt->tscdata->channel->ChannelList[i].ChannelCtrlSrc))
				{
					*pchan = mdt->tscdata->channel->ChannelList[i].ChannelId;
					pchan++;
				}
			}//1
			pchan = cchan;
			s = 0;
			k = 0;
			pcexist = 0;
			for (i = 0; i < MAX_CHANNEL; i++)
			{//i
				if (0 == pinfo->chan[i])
					break;
				exist = 0;
				for (j = 0; j < MAX_CHANNEL; j++ )
				{//j
					if (0 == nchan[j])
						break;
					if (pinfo->chan[i] == nchan[j])
					{
						exist = 1;
						break;
					}			
				}//j
				if (0 == exist)
				{//if (0 == exist)
					*pchan = pinfo->chan[i];
					pchan++;
					for (j = 0; j < (mdt->tscdata->channel->FactChannelNum); j++)
					{//2j
						if (0 == (mdt->tscdata->channel->ChannelList[j].ChannelId))
							break;
						if (pinfo->chan[i] == (mdt->tscdata->channel->ChannelList[j].ChannelId))
						{
							if (3 == mdt->tscdata->channel->ChannelList[j].ChannelType)
							{
								cpchan[s] = pinfo->chan[i];
								s++;
								pcexist = 1;		
							}
							else
							{
								cnpchan[k] = pinfo->chan[i];
								k++;
							}
							break;
						}
					}//2j
				}//if (0 == exist)
			}//i
			//end compute
			if ((0==cchan[0])&&(pinfo->gftime>0)||(pinfo->ytime>0)||(pinfo->rtime>0))
			{
				if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
				{//report info to server actively
					mdt->sinfo->conmode = 28;
					mdt->sinfo->color = 0x02;
					mdt->sinfo->time = pinfo->gftime + pinfo->ytime + pinfo->rtime;
					mdt->sinfo->stage = pinfo->stageline;
					mdt->sinfo->cyclet = 0;
					mdt->sinfo->phase = 0;
					mdt->sinfo->phase |= (0x01 << (pinfo->phaseid - 1));
											
					mdt->sinfo->chans = 0;
					memset(mdt->sinfo->csta,0,sizeof(mdt->sinfo->csta));
					csta = mdt->sinfo->csta;
					for (i = 0; i < MAX_CHANNEL; i++)
					{
						if (0 == cchan[i])
							break;
						mdt->sinfo->chans += 1;
						tcsta = cchan[i];
						tcsta <<= 2;
						tcsta &= 0xfc;
						tcsta |= 0x02;
						*csta = tcsta;
						csta++;
					}
					memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));
					memset(sibuf,0,sizeof(sibuf));
					if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
					{
					#ifdef FULL_DETECT_DEBUG
						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
					else
					{
						write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
					}
				}//report info to server actively
			}//if ((0==cchan[0])&&(pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0))

			if ((0 != cchan[0]) && (pinfo->gftime > 0))
			{
				if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
				{//report info to server actively
					mdt->sinfo->conmode = 28;
					mdt->sinfo->color = 0x03;
					mdt->sinfo->time = pinfo->gftime;
					mdt->sinfo->stage = pinfo->stageline;
					mdt->sinfo->cyclet = 0;
					mdt->sinfo->phase = 0;
					mdt->sinfo->phase |= (0x01 << (pinfo->phaseid - 1));
					
					for (i = 0; i < MAX_CHANNEL; i++)
					{
						if (0 == cchan[i])
							break;
						for (j = 0; j < mdt->sinfo->chans; j++)
						{
							if (0 == mdt->sinfo->csta[j])
								break;
							tcsta = mdt->sinfo->csta[j];
							tcsta &= 0xfc;
							tcsta >>= 2;
							tcsta &= 0x3f;
							if (cchan[i] == tcsta)
							{
								mdt->sinfo->csta[j] &= 0xfc;
								mdt->sinfo->csta[j] |= 0x03; //00000001-green flash
								break;
							}
						}
					}								
					memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));	
					memset(sibuf,0,sizeof(sibuf));
					if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
					{
					#ifdef FULL_DETECT_DEBUG
						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
					else
					{
						write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
					}
				}//report info to server actively
			}//if ((0 != cchan[0]) && (pinfo.gftime > 0))

			//current phase begin to green flash
			if ((*(mdt->fcdata->markbit) & 0x0002) && (*(mdt->fcdata->markbit) & 0x0010))
			{
				memset(&tctd,0,sizeof(tctd));
				tctd.mode = 28;//traffic control
				tctd.pattern = *(mdt->fcdata->patternid);
				tctd.lampcolor = 0x02;
				tctd.lamptime = pinfo->gftime;
				tctd.phaseid = pinfo->phaseid;
				tctd.stageline = pinfo->stageline;
				if (SUCCESS != timedown_data_to_conftool(mdt->fcdata->sockfd,&tctd))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#ifdef EMBED_CONFIGURE_TOOL
			if (*(mdt->fcdata->markbit2) & 0x0200)
			{
				memset(&tctd,0,sizeof(tctd));
				tctd.mode = 28;//traffic control
				tctd.pattern = *(mdt->fcdata->patternid);
				tctd.lampcolor = 0x02;
				tctd.lamptime = pinfo->gftime;
				tctd.phaseid = pinfo->phaseid;
				tctd.stageline = pinfo->stageline;	
				if (SUCCESS != timedown_data_to_embed(mdt->fcdata->sockfd,&tctd))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#endif
			fbdata[1] = 28;
			fbdata[2] = pinfo->stageline;
			fbdata[3] = 0x02;
			fbdata[4] = pinfo->gftime;
			if (!wait_write_serial(*(mdt->fcdata->fbserial)))
			{
				if (write(*(mdt->fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
				{
				#ifdef FULL_DETECT_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
			}
			else
			{
			#ifdef FULL_DETECT_DEBUG
				printf("face board serial port cannot write,Line:%d\n",__LINE__);
			#endif
			}

			ngf = 0;
			while (1)
			{//green flash
				if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x03))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;	
				}
				if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
								cchan,0x03,mdt->fcdata->markbit))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				to.tv_sec = 0;
				to.tv_usec = 500000;
				select(0,NULL,NULL,NULL,&to);

				if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x02))
                {
                #ifdef FULL_DETECT_DEBUG
                    printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                    *(mdt->fcdata->markbit) |= 0x0800;
                }
                if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
                                cchan,0x02,mdt->fcdata->markbit))
                {
                #ifdef FULL_DETECT_DEBUG
                    printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
				to.tv_sec = 0;
				to.tv_usec = 500000;
				select(0,NULL,NULL,NULL,&to);
			
				ngf += 1;
				if (ngf >= pinfo->gftime)
					break;
			}//green flash

			if (1 == pcexist)
			{
				//current phase begin to yellow lamp
				if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cnpchan,0x01))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
									cnpchan,0x01, mdt->fcdata->markbit))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cpchan,0x00))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
													cpchan,0x00,mdt->fcdata->markbit))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
			}
			else
			{
				if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x01))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
										cchan,0x01,mdt->fcdata->markbit))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
			}
			
			if ((0 != cchan[0]) && (pinfo->ytime > 0))
			{
				if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
				{//report info to server actively
					mdt->sinfo->conmode = 28;
					mdt->sinfo->color = 0x01;
					mdt->sinfo->time = pinfo->ytime;
					mdt->sinfo->stage = pinfo->stageline;
					mdt->sinfo->cyclet = 0;
					mdt->sinfo->phase = 0;
					mdt->sinfo->phase |= (0x01 << (pinfo->phaseid - 1));
											
					for (i = 0; i < MAX_CHANNEL; i++)
					{
						if (0 == cnpchan[i])
							break;
						for (j = 0; j < mdt->sinfo->chans; j++)
						{
							if (0 == mdt->sinfo->csta[j])
								break;
							tcsta = mdt->sinfo->csta[j];
							tcsta &= 0xfc;
							tcsta >>= 2;
							tcsta &= 0x3f;
							if (cnpchan[i] == tcsta)
							{
								mdt->sinfo->csta[j] &= 0xfc;
								mdt->sinfo->csta[j] |= 0x01; //00000001-yellow
								break;
							}
						}
					}
					for (i = 0; i < MAX_CHANNEL; i++)
					{
						if (0 == cpchan[i])
							break;
						for (j = 0; j < mdt->sinfo->chans; j++)
						{
							if (0 == mdt->sinfo->csta[j])
								break;
							tcsta = mdt->sinfo->csta[j];
							tcsta &= 0xfc;
							tcsta >>= 2;
							tcsta &= 0x3f;
							if (cpchan[i] == tcsta)
							{
								mdt->sinfo->csta[j] &= 0xfc;
								break;
							}
						}
					}
					memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));
					memset(sibuf,0,sizeof(sibuf));
					if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
					{
					#ifdef FULL_DETECT_DEBUG
						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
					else
					{
						write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
					}
				}//report info to server actively
			}//if ((0 != cchan[0]) && (pinfo.ytime > 0))

			if ((*(mdt->fcdata->markbit) & 0x0002) && (*(mdt->fcdata->markbit) & 0x0010))
			{
				memset(&tctd,0,sizeof(tctd));
				tctd.mode = 28;//traffic control
				tctd.pattern = *(mdt->fcdata->patternid);
				tctd.lampcolor = 0x01;
				tctd.lamptime = pinfo->ytime;
				tctd.phaseid = pinfo->phaseid;
				tctd.stageline = pinfo->stageline;
				if (SUCCESS != timedown_data_to_conftool(mdt->fcdata->sockfd,&tctd))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#ifdef EMBED_CONFIGURE_TOOL
			if (*(mdt->fcdata->markbit2) & 0x0200)
			{
				memset(&tctd,0,sizeof(tctd));
				tctd.mode = 28;//traffic control
				tctd.pattern = *(mdt->fcdata->patternid);
				tctd.lampcolor = 0x01;
				tctd.lamptime = pinfo->ytime;
				tctd.phaseid = pinfo->phaseid;
				tctd.stageline = pinfo->stageline;	
				if (SUCCESS != timedown_data_to_embed(mdt->fcdata->sockfd,&tctd))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#endif
			fbdata[1] = 28;
			fbdata[2] = pinfo->stageline;
			fbdata[3] = 0x01;
			fbdata[4] = pinfo->ytime;
			if (!wait_write_serial(*(mdt->fcdata->fbserial)))
			{
				if (write(*(mdt->fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
				{
				#ifdef FULL_DETECT_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
			}
			else
			{
			#ifdef FULL_DETECT_DEBUG
				printf("face board serial port cannot write,Line:%d\n",__LINE__);
			#endif
			}

			sleep(pinfo->ytime);

			if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x00))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
							cchan,0x00,mdt->fcdata->markbit))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
			if ((0 != cchan[0]) && (pinfo->rtime > 0))
			{
				if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
				{//report info to server actively
					mdt->sinfo->conmode = 28;
					mdt->sinfo->color = 0x00;
					mdt->sinfo->time = pinfo->rtime;
					mdt->sinfo->stage = pinfo->stageline;
					mdt->sinfo->cyclet = 0;
					mdt->sinfo->phase = 0;
					mdt->sinfo->phase |= (0x01 << (pinfo->phaseid - 1));
											
					for (i = 0; i < MAX_CHANNEL; i++)
					{
						if (0 == cchan[i])
							break;
						for (j = 0; j < mdt->sinfo->chans; j++)
						{
							if (0 == mdt->sinfo->csta[j])
								break;
							tcsta = mdt->sinfo->csta[j];
							tcsta &= 0xfc;
							tcsta >>= 2;
							tcsta &= 0x3f;
							if (cchan[i] == tcsta)
							{
								mdt->sinfo->csta[j] &= 0xfc;
								break;
							}
						}
					}
					memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));	
					memset(sibuf,0,sizeof(sibuf));
					if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
					{
					#ifdef FULL_DETECT_DEBUG
						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
					else
					{
						write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
					}
				}//report info to server actively
			}//if ((0 != cchan[0]) && (pinfo.rtime > 0))

			if (pinfo->rtime > 0)
			{
				if ((*(mdt->fcdata->markbit) & 0x0002) && (*(mdt->fcdata->markbit) & 0x0010))
				{
					memset(&tctd,0,sizeof(tctd));
					tctd.mode = 28;//traffic control
					tctd.pattern = *(mdt->fcdata->patternid);
					tctd.lampcolor = 0x00;
					tctd.lamptime = pinfo->rtime;
					tctd.phaseid = pinfo->phaseid;
					tctd.stageline = pinfo->stageline;
					if (SUCCESS != timedown_data_to_conftool(mdt->fcdata->sockfd,&tctd))
					{
					#ifdef FULL_DETECT_DEBUG
						printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
				}
				#ifdef EMBED_CONFIGURE_TOOL
				if (*(mdt->fcdata->markbit2) & 0x0200)
				{
					memset(&tctd,0,sizeof(tctd));
					tctd.mode = 28;//traffic control
					tctd.pattern = *(mdt->fcdata->patternid);
					tctd.lampcolor = 0x00;
					tctd.lamptime = pinfo->rtime;
					tctd.phaseid = pinfo->phaseid;
					tctd.stageline = pinfo->stageline;
					if (SUCCESS != timedown_data_to_embed(mdt->fcdata->sockfd,&tctd))
					{
					#ifdef FULL_DETECT_DEBUG
						printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
				}
				#endif
				fbdata[1] = 28;
				fbdata[2] = pinfo->stageline;
				fbdata[3] = 0x00;
				fbdata[4] = pinfo->rtime;
				if (!wait_write_serial(*(mdt->fcdata->fbserial)))
				{
					if (write(*(mdt->fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
					{
					#ifdef FULL_DETECT_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						*(mdt->fcdata->markbit) |= 0x0800;
					}
				}
				else
				{
				#ifdef FULL_DETECT_DEBUG
					printf("face board serial port cannot write,Line:%d\n",__LINE__);
				#endif
				}
				sleep(pinfo->rtime);
			}

			memset(pinfo,0,sizeof(dcpinfo_t));
			if (SUCCESS != ms_get_phase_info(mdt->fcdata,mdt->tscdata,*(mdt->rettl),num,pinfo))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				return FAIL;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
                            pinfo->chan,0x02,mdt->fcdata->markbit))
            {
            #ifdef FULL_DETECT_DEBUG
                printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
			if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),pinfo->chan,0x02))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
				return FAIL;
			}
			else
			{//set success
				*(mdt->fcdata->slnum) = num;
				*(mdt->fcdata->stageid) = pinfo->stageline;
				*(mdt->fcdata->phaseid) = 0;
				*(mdt->fcdata->phaseid) |= (0x01 << (pinfo->phaseid - 1));

				if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
				{//report info to server actively
					mdt->sinfo->conmode = 28;
					mdt->sinfo->color = 0x02;
					mdt->sinfo->time = 0;
					mdt->sinfo->stage = pinfo->stageline;
					mdt->sinfo->cyclet = 0;
					mdt->sinfo->phase = 0;
					mdt->sinfo->phase |= (0x01 << (pinfo->phaseid - 1));
					mdt->sinfo->chans = 0;
					memset(mdt->sinfo->csta,0,sizeof(mdt->sinfo->csta));
					csta = mdt->sinfo->csta;
					for (i = 0; i < MAX_CHANNEL; i++)
					{
						if (0 == pinfo->chan[i])
							break;
						mdt->sinfo->chans += 1;
						tcsta = pinfo->chan[i];
						tcsta <<= 2;
						tcsta &= 0xfc;
						tcsta |= 0x02; //00000010-green 
						*csta = tcsta;
						csta++;
					}
					memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));
					memset(sibuf,0,sizeof(sibuf));
					if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
					{
					#ifdef FULL_DETECT_DEBUG
						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
					else
					{
						write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
					}
				}//report info to server actively

				if ((*(mdt->fcdata->markbit) & 0x0002) && (*(mdt->fcdata->markbit) & 0x0010))
				{
					memset(&tctd,0,sizeof(tctd));
					tctd.mode = 28;//traffic control
					tctd.pattern = *(mdt->fcdata->patternid);
					tctd.lampcolor = 0x02;
					tctd.lamptime = 0;
					tctd.phaseid = pinfo->phaseid;
					tctd.stageline = pinfo->stageline;
					if (SUCCESS != timedown_data_to_conftool(mdt->fcdata->sockfd,&tctd))
					{
					#ifdef FULL_DETECT_DEBUG
						printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
				}
				#ifdef EMBED_CONFIGURE_TOOL
				if (*(mdt->fcdata->markbit2) & 0x0200)
				{
					memset(&tctd,0,sizeof(tctd));
					tctd.mode = 28;//traffic control
					tctd.pattern = *(mdt->fcdata->patternid);
					tctd.lampcolor = 0x02;
					tctd.lamptime = 0;
					tctd.phaseid = pinfo->phaseid;
					tctd.stageline = pinfo->stageline;	
					if (SUCCESS != timedown_data_to_embed(mdt->fcdata->sockfd,&tctd))
					{
					#ifdef FULL_DETECT_DEBUG
						printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
					#endif
					}
				}
				#endif
				fbdata[1] = 28;
				fbdata[2] = pinfo->stageline;
				fbdata[3] = 0x02;
				fbdata[4] = 0;
				if (!wait_write_serial(*(mdt->fcdata->fbserial)))
				{
					if (write(*(mdt->fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
					{
					#ifdef FULL_DETECT_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						*(mdt->fcdata->markbit) |= 0x0800;
					}
				}
				else
				{
				#ifdef FULL_DETECT_DEBUG
					printf("face board serial port cannot write,Line:%d\n",__LINE__);
				#endif
				}	

				return SUCCESS;
			}//set success
		}//Controlling stageid is not current stageid
	}//other condition
}

//mobile achieve direct control
int ms_mobile_direct_control(markdata_c *mdt,dcpinfo_t *pinfo,unsigned char *dirch,unsigned char *fdirch)
{
	if ((NULL == mdt) || (NULL == pinfo) || (NULL == dirch))
	{
	#ifdef FULL_DETECT_DEBUG
		printf("pointer address error,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char			find = 0;
	unsigned char			nchan[32] = {0};
	unsigned char			cchan[32] = {0};
	unsigned char			cpchan[32] = {0};
	unsigned char			cnpchan[32] = {0};
	unsigned char			*pchan = NULL;
	unsigned char           *csta = NULL;
	unsigned char           tcsta = 0;
	unsigned char           sibuf[64] = {0};
	unsigned char           err[10] = {0};
	unsigned char			i = 0,j = 0,s = 0,k = 0;
	unsigned int			nphidi = 0;
	unsigned char			nphidc = 0;
	unsigned char			pcexist = 0;
	unsigned char			num = 0;
	unsigned char			cstageid = 0;
	unsigned char			ngf = 0;
	unsigned char			exist = 0;
	struct timeval			to,ct;
	unsigned char           fbdata[6] = {0};
    fbdata[0] = 0xC1;
    fbdata[5] = 0xED;
	timedown_t              tctd;

	cstageid = mdt->tscdata->timeconfig->TimeConfigList[*(mdt->rettl)][*(mdt->fcdata->slnum)].StageId;

	if ((1 == *(mdt->redl)) || (1 == *(mdt->yfl)) || (1 == *(mdt->closel)))
	{//all red or all close or yellow flash
		if (1 == *(mdt->yfl))
		{
			pthread_cancel(*(mdt->yfid));
			pthread_join(*(mdt->yfid),NULL);
			*(mdt->yfl) = 0;
			new_all_red(mdt->ardata);
		}
		*(mdt->redl) = 0;	
		if (1 == *(mdt->closel))
		{
			*(mdt->closel) = 0;
			new_all_red(mdt->ardata);
		}
		
		if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),dirch,0x02))
		{
		#ifdef FULL_DETECT_DEBUG
			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			*(mdt->fcdata->markbit) |= 0x0800;
			return FAIL;
		}
		if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
							dirch,0x02,mdt->fcdata->markbit))
		{
		#ifdef FULL_DETECT_DEBUG
			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}

		if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
		{//report info to server actively
			mdt->sinfo->conmode = 28;
			mdt->sinfo->color = 0x02;
			mdt->sinfo->time = 0;
			mdt->sinfo->stage = 0;
			mdt->sinfo->cyclet = 0;
			mdt->sinfo->phase = 0;
			mdt->sinfo->chans = 0;
			memset(mdt->sinfo->csta,0,sizeof(mdt->sinfo->csta));
			csta = mdt->sinfo->csta;
			for (i = 0; i < MAX_CHANNEL; i++)
			{
				if (0 == dirch[i])
					break;
				mdt->sinfo->chans += 1;
				tcsta = dirch[i];
				tcsta <<= 2;
				tcsta &= 0xfc;
				tcsta |= 0x02; //00000010-green 
				*csta = tcsta;
				csta++;
			}
			memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));
			memset(sibuf,0,sizeof(sibuf));
			if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}
			else
			{
				write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
			}
		}//report info to server actively
		return SUCCESS;
	}//all red or all close or yellow flash
	else if (1 == *(mdt->firdc))
	{//first direct control means step or stage control switch to direction control
		pchan = cchan;
		s = 0;
		k = 0;
		pcexist = 0;
		for (i = 0; i < MAX_CHANNEL; i++)
		{//i
			if (0 == pinfo->chan[i])
				break;
			exist = 0;
			for (j = 0; j < MAX_CHANNEL; j++ )
			{//j
				if (0 == dirch[j])
					break;
				if (pinfo->chan[i] == dirch[j])
				{
					exist = 1;
					break;
				}			
			}//j
			if (0 == exist)
			{//if (0 == exist)
				*pchan = pinfo->chan[i];
				pchan++;
				for (j = 0; j < (mdt->tscdata->channel->FactChannelNum); j++)
				{//2j
					if (0 == (mdt->tscdata->channel->ChannelList[j].ChannelId))
						break;
					if (pinfo->chan[i] == (mdt->tscdata->channel->ChannelList[j].ChannelId))
					{
						if (3 == mdt->tscdata->channel->ChannelList[j].ChannelType)
						{
							cpchan[s] = pinfo->chan[i];
							s++;
							pcexist = 1;		
						}
						else
						{
							cnpchan[k] = pinfo->chan[i];
							k++;
						}
						break;
					}
				}//2j
			}//if (0 == exist)
		}//i
		
		if (0 == cchan[0])
		{
			if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
			{//report info to server actively
				mdt->sinfo->conmode = 28;
				mdt->sinfo->color = 0x02;
				mdt->sinfo->time = pinfo->gftime + pinfo->ytime + pinfo->rtime;
				mdt->sinfo->stage = pinfo->stageline;
                mdt->sinfo->cyclet = 0;
                mdt->sinfo->phase = 0;
                mdt->sinfo->phase |= (0x01 << (pinfo->phaseid - 1));
										
				mdt->sinfo->chans = 0;
				memset(mdt->sinfo->csta,0,sizeof(mdt->sinfo->csta));
				csta = mdt->sinfo->csta;
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == cchan[i])
						break;
					mdt->sinfo->chans += 1;
					tcsta = cchan[i];
					tcsta <<= 2;
					tcsta &= 0xfc;
					tcsta |= 0x02;
					*csta = tcsta;
					csta++;
				}
				memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));
				memset(sibuf,0,sizeof(sibuf));
				if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
				}
			}//report info to server actively
		}//if ((0==cchan[0])&&(pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0))

		if (0 != cchan[0])
		{
			if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
			{//report info to server actively
				mdt->sinfo->conmode = 28;
				mdt->sinfo->color = 0x03;
				mdt->sinfo->time = pinfo->gftime;
				mdt->sinfo->stage = pinfo->stageline;
                mdt->sinfo->cyclet = 0;
                mdt->sinfo->phase = 0;
                mdt->sinfo->phase |= (0x01 << (pinfo->phaseid - 1));
				
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == cchan[i])
						break;
					for (j = 0; j < mdt->sinfo->chans; j++)
					{
						if (0 == mdt->sinfo->csta[j])
							break;
						tcsta = mdt->sinfo->csta[j];
						tcsta &= 0xfc;
						tcsta >>= 2;
						tcsta &= 0x3f;
						if (cchan[i] == tcsta)
						{
							mdt->sinfo->csta[j] &= 0xfc;
							mdt->sinfo->csta[j] |= 0x03; //00000001-green flash
							break;
						}
					}
				}								
				memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));	
				memset(sibuf,0,sizeof(sibuf));
				if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
				}
			}//report info to server actively
		}//if ((0 != cchan[0]) && (pinfo.gftime > 0))

		//current phase begin to green flash
		if ((*(mdt->fcdata->markbit) & 0x0002) && (*(mdt->fcdata->markbit) & 0x0010))
		{
			memset(&tctd,0,sizeof(tctd));
			tctd.mode = 28;//traffic control
			tctd.pattern = *(mdt->fcdata->patternid);
			tctd.lampcolor = 0x02;
			tctd.lamptime = pinfo->gftime;
			tctd.phaseid = pinfo->phaseid;
			tctd.stageline = pinfo->stageline;
			if (SUCCESS != timedown_data_to_conftool(mdt->fcdata->sockfd,&tctd))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}
		}
		#ifdef EMBED_CONFIGURE_TOOL
		if (*(mdt->fcdata->markbit2) & 0x0200)
		{
			memset(&tctd,0,sizeof(tctd));
			tctd.mode = 28;//traffic control
			tctd.pattern = *(mdt->fcdata->patternid);
			tctd.lampcolor = 0x02;
			tctd.lamptime = pinfo->gftime;
			tctd.phaseid = pinfo->phaseid;
			tctd.stageline = pinfo->stageline;	
			if (SUCCESS != timedown_data_to_embed(mdt->fcdata->sockfd,&tctd))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}
		}
		#endif
		fbdata[1] = 28;
		fbdata[2] = pinfo->stageline;
		fbdata[3] = 0x02;
		fbdata[4] = pinfo->gftime;
		if (!wait_write_serial(*(mdt->fcdata->fbserial)))
		{
			if (write(*(mdt->fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
			{
			#ifdef FULL_DETECT_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
		}
		else
		{
		#ifdef FULL_DETECT_DEBUG
			printf("face board serial port cannot write,Line:%d\n",__LINE__);
		#endif
		}

		ngf = 0;
		while (1)
		{//green flash
			if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x03))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;	
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
							cchan,0x03,mdt->fcdata->markbit))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
			to.tv_sec = 0;
			to.tv_usec = 500000;
			select(0,NULL,NULL,NULL,&to);

			if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x02))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
							cchan,0x02,mdt->fcdata->markbit))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
			to.tv_sec = 0;
			to.tv_usec = 500000;
			select(0,NULL,NULL,NULL,&to);
		
			ngf += 1;
			if (ngf >= pinfo->gftime)
				break;
		}//green flash

		if (1 == pcexist)
		{
			//current phase begin to yellow lamp
			if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cnpchan,0x01))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
								cnpchan,0x01, mdt->fcdata->markbit))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
			if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cpchan,0x00))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
												cpchan,0x00,mdt->fcdata->markbit))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
		}
		else
		{
			if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x01))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
									cchan,0x01,mdt->fcdata->markbit))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
		}
		
		if (0 != cchan[0])
		{
			if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
			{//report info to server actively
				mdt->sinfo->conmode = 28;
				mdt->sinfo->color = 0x01;
				mdt->sinfo->time = pinfo->ytime;
				mdt->sinfo->stage = pinfo->stageline;
                mdt->sinfo->cyclet = 0;
                mdt->sinfo->phase = 0;
                mdt->sinfo->phase |= (0x01 << (pinfo->phaseid - 1));
										
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == cnpchan[i])
						break;
					for (j = 0; j < mdt->sinfo->chans; j++)
					{
						if (0 == mdt->sinfo->csta[j])
							break;
						tcsta = mdt->sinfo->csta[j];
						tcsta &= 0xfc;
						tcsta >>= 2;
						tcsta &= 0x3f;
						if (cnpchan[i] == tcsta)
						{
							mdt->sinfo->csta[j] &= 0xfc;
							mdt->sinfo->csta[j] |= 0x01; //00000001-yellow
							break;
						}
					}
				}
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == cpchan[i])
						break;
					for (j = 0; j < mdt->sinfo->chans; j++)
					{
						if (0 == mdt->sinfo->csta[j])
							break;
						tcsta = mdt->sinfo->csta[j];
						tcsta &= 0xfc;
						tcsta >>= 2;
						tcsta &= 0x3f;
						if (cpchan[i] == tcsta)
						{
							mdt->sinfo->csta[j] &= 0xfc;
							break;
						}
					}
				}
				memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));
				memset(sibuf,0,sizeof(sibuf));
				if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
				}
			}//report info to server actively
		}//if ((0 != cchan[0]) && (pinfo.ytime > 0))

		if ((*(mdt->fcdata->markbit) & 0x0002) && (*(mdt->fcdata->markbit) & 0x0010))
		{
			memset(&tctd,0,sizeof(tctd));
			tctd.mode = 28;//traffic control
			tctd.pattern = *(mdt->fcdata->patternid);
			tctd.lampcolor = 0x01;
			tctd.lamptime = pinfo->ytime;
			tctd.phaseid = pinfo->phaseid;
			tctd.stageline = pinfo->stageline;
			if (SUCCESS != timedown_data_to_conftool(mdt->fcdata->sockfd,&tctd))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}
		}
		#ifdef EMBED_CONFIGURE_TOOL
		if (*(mdt->fcdata->markbit2) & 0x0200)
		{
			memset(&tctd,0,sizeof(tctd));
			tctd.mode = 28;//traffic control
			tctd.pattern = *(mdt->fcdata->patternid);
			tctd.lampcolor = 0x01;
			tctd.lamptime = pinfo->ytime;
			tctd.phaseid = pinfo->phaseid;
			tctd.stageline = pinfo->stageline;	
			if (SUCCESS != timedown_data_to_embed(mdt->fcdata->sockfd,&tctd))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}
		}
		#endif
		fbdata[1] = 28;
		fbdata[2] = pinfo->stageline;
		fbdata[3] = 0x01;
		fbdata[4] = pinfo->ytime;
		if (!wait_write_serial(*(mdt->fcdata->fbserial)))
		{
			if (write(*(mdt->fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
			{
			#ifdef FULL_DETECT_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
		}
		else
		{
		#ifdef FULL_DETECT_DEBUG
			printf("face board serial port cannot write,Line:%d\n",__LINE__);
		#endif
		}

		sleep(pinfo->ytime);

		if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x00))
		{
		#ifdef FULL_DETECT_DEBUG
			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			*(mdt->fcdata->markbit) |= 0x0800;
		}
		if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
						cchan,0x00,mdt->fcdata->markbit))
		{
		#ifdef FULL_DETECT_DEBUG
			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}
	
		if ((0 != cchan[0]) && (pinfo->rtime > 0))
		{
			if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
			{//report info to server actively
				mdt->sinfo->conmode = 28;
				mdt->sinfo->color = 0x00;
				mdt->sinfo->time = pinfo->rtime;
				mdt->sinfo->stage = pinfo->stageline;
				mdt->sinfo->cyclet = 0;
				mdt->sinfo->phase = 0;
				mdt->sinfo->phase |= (0x01 << (pinfo->phaseid - 1));
										
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == cchan[i])
						break;
					for (j = 0; j < mdt->sinfo->chans; j++)
					{
						if (0 == mdt->sinfo->csta[j])
							break;
						tcsta = mdt->sinfo->csta[j];
						tcsta &= 0xfc;
						tcsta >>= 2;
						tcsta &= 0x3f;
						if (cchan[i] == tcsta)
						{
							mdt->sinfo->csta[j] &= 0xfc;
							break;
						}
					}
				}
				memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));	
				memset(sibuf,0,sizeof(sibuf));
				if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
				}
			}//report info to server actively
		}//if ((0 != cchan[0]) && (pinfo.rtime > 0))

		if (pinfo->rtime > 0)
		{
			if ((*(mdt->fcdata->markbit) & 0x0002) && (*(mdt->fcdata->markbit) & 0x0010))
			{
				memset(&tctd,0,sizeof(tctd));
				tctd.mode = 28;//traffic control
				tctd.pattern = *(mdt->fcdata->patternid);
				tctd.lampcolor = 0x00;
				tctd.lamptime = pinfo->rtime;
				tctd.phaseid = pinfo->phaseid;
				tctd.stageline = pinfo->stageline;
				if (SUCCESS != timedown_data_to_conftool(mdt->fcdata->sockfd,&tctd))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#ifdef EMBED_CONFIGURE_TOOL
			if (*(mdt->fcdata->markbit2) & 0x0200)
			{
				memset(&tctd,0,sizeof(tctd));
				tctd.mode = 28;//traffic control
				tctd.pattern = *(mdt->fcdata->patternid);
				tctd.lampcolor = 0x00;
				tctd.lamptime = pinfo->rtime;
				tctd.phaseid = pinfo->phaseid;
				tctd.stageline = pinfo->stageline;
				if (SUCCESS != timedown_data_to_embed(mdt->fcdata->sockfd,&tctd))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#endif
			fbdata[1] = 28;
			fbdata[2] = pinfo->stageline;
			fbdata[3] = 0x00;
			fbdata[4] = pinfo->rtime;
			if (!wait_write_serial(*(mdt->fcdata->fbserial)))
			{
				if (write(*(mdt->fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
				{
				#ifdef FULL_DETECT_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
			}
			else
			{
			#ifdef FULL_DETECT_DEBUG
				printf("face board serial port cannot write,Line:%d\n",__LINE__);
			#endif
			}
			sleep(pinfo->rtime);
		}
	
		if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
						dirch,0x02,mdt->fcdata->markbit))
		{
		#ifdef FULL_DETECT_DEBUG
			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}
		if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),dirch,0x02))
		{
		#ifdef FULL_DETECT_DEBUG
			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			*(mdt->fcdata->markbit) |= 0x0800;
			return FAIL;
		}
		else
		{//set success
			if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
			{//report info to server actively
				mdt->sinfo->conmode = 28;
				mdt->sinfo->color = 0x02;
				mdt->sinfo->time = 0;
				mdt->sinfo->stage = 0;
				mdt->sinfo->cyclet = 0;
				mdt->sinfo->phase = 0;
				mdt->sinfo->chans = 0;
				memset(mdt->sinfo->csta,0,sizeof(mdt->sinfo->csta));
				csta = mdt->sinfo->csta;
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == dirch[i])
						break;
					mdt->sinfo->chans += 1;
					tcsta = dirch[i];
					tcsta <<= 2;
					tcsta &= 0xfc;
					tcsta |= 0x02; //00000010-green 
					*csta = tcsta;
					csta++;
				}
				memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));
				memset(sibuf,0,sizeof(sibuf));
				if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
				}
			}//report info to server actively

			if ((*(mdt->fcdata->markbit) & 0x0002) && (*(mdt->fcdata->markbit) & 0x0010))
			{
				memset(&tctd,0,sizeof(tctd));
				tctd.mode = 28;//traffic control
				tctd.pattern = *(mdt->fcdata->patternid);
				tctd.lampcolor = 0x02;
				tctd.lamptime = 0;
				tctd.phaseid = 0;
				tctd.stageline = 0;
				if (SUCCESS != timedown_data_to_conftool(mdt->fcdata->sockfd,&tctd))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#ifdef EMBED_CONFIGURE_TOOL
			if (*(mdt->fcdata->markbit2) & 0x0200)
			{
				memset(&tctd,0,sizeof(tctd));
				tctd.mode = 28;//traffic control
				tctd.pattern = *(mdt->fcdata->patternid);
				tctd.lampcolor = 0x02;
				tctd.lamptime = 0;
				tctd.phaseid = 0;
				tctd.stageline = 0;	
				if (SUCCESS != timedown_data_to_embed(mdt->fcdata->sockfd,&tctd))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#endif
			fbdata[1] = 28;
			fbdata[2] = 0;
			fbdata[3] = 0x02;
			fbdata[4] = 0;
			if (!wait_write_serial(*(mdt->fcdata->fbserial)))
			{
				if (write(*(mdt->fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
				{
				#ifdef FULL_DETECT_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
			}
			else
			{
			#ifdef FULL_DETECT_DEBUG
				printf("face board serial port cannot write,Line:%d\n",__LINE__);
			#endif
			}	
			return SUCCESS;
		}//set success
	}//first direct control
	else
	{//other condition
		//begin to compute change channels
		pchan = cchan;
		s = 0;
		k = 0;
		pcexist = 0;
		for (i = 0; i < MAX_CHANNEL; i++)
		{//i
			if (0 == fdirch[i])
				break;
			exist = 0;
			for (j = 0; j < MAX_CHANNEL; j++ )
			{//j
				if (0 == dirch[j])
					break;
				if (fdirch[i] == dirch[j])
				{
					exist = 1;
					break;
				}			
			}//j
			if (0 == exist)
			{//if (0 == exist)
				*pchan = fdirch[i];
				pchan++;
				for (j = 0; j < (mdt->tscdata->channel->FactChannelNum); j++)
				{//2j
					if (0 == (mdt->tscdata->channel->ChannelList[j].ChannelId))
						break;
					if (fdirch[i] == (mdt->tscdata->channel->ChannelList[j].ChannelId))
					{
						if (3 == mdt->tscdata->channel->ChannelList[j].ChannelType)
						{
							cpchan[s] = fdirch[i];
							s++;
							pcexist = 1;		
						}
						else
						{
							cnpchan[k] = fdirch[i];
							k++;
						}
						break;
					}
				}//2j
			}//if (0 == exist)
		}//i
		//end compute
		if (0 == cchan[0])
		{
			if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
			{//report info to server actively
				mdt->sinfo->conmode = 28;
				mdt->sinfo->color = 0x02;
				mdt->sinfo->time = 6;
				mdt->sinfo->stage = 0;
				mdt->sinfo->cyclet = 0;
				mdt->sinfo->phase = 0;
										
				mdt->sinfo->chans = 0;
				memset(mdt->sinfo->csta,0,sizeof(mdt->sinfo->csta));
				csta = mdt->sinfo->csta;
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == cchan[i])
						break;
					mdt->sinfo->chans += 1;
					tcsta = cchan[i];
					tcsta <<= 2;
					tcsta &= 0xfc;
					tcsta |= 0x02;
					*csta = tcsta;
					csta++;
				}
				memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));
				memset(sibuf,0,sizeof(sibuf));
				if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
				}
			}//report info to server actively
		}//if ((0==cchan[0])&&(pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0))

		if (0 != cchan[0])
		{
			if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
			{//report info to server actively
				mdt->sinfo->conmode = 28;
				mdt->sinfo->color = 0x03;
				mdt->sinfo->time = 3;
				mdt->sinfo->stage = 0;
				mdt->sinfo->cyclet = 0;
				mdt->sinfo->phase = 0;
				
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == cchan[i])
						break;
					for (j = 0; j < mdt->sinfo->chans; j++)
					{
						if (0 == mdt->sinfo->csta[j])
							break;
						tcsta = mdt->sinfo->csta[j];
						tcsta &= 0xfc;
						tcsta >>= 2;
						tcsta &= 0x3f;
						if (cchan[i] == tcsta)
						{
							mdt->sinfo->csta[j] &= 0xfc;
							mdt->sinfo->csta[j] |= 0x03; //00000001-green flash
							break;
						}
					}
				}								
				memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));	
				memset(sibuf,0,sizeof(sibuf));
				if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
				}
			}//report info to server actively
		}//if ((0 != cchan[0]) && (pinfo.gftime > 0))

		//current phase begin to green flash
		if ((*(mdt->fcdata->markbit) & 0x0002) && (*(mdt->fcdata->markbit) & 0x0010))
		{
			memset(&tctd,0,sizeof(tctd));
			tctd.mode = 28;//traffic control
			tctd.pattern = *(mdt->fcdata->patternid);
			tctd.lampcolor = 0x02;
			tctd.lamptime = 3;
			tctd.phaseid = 0;
			tctd.stageline = 0;
			if (SUCCESS != timedown_data_to_conftool(mdt->fcdata->sockfd,&tctd))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}
		}
		#ifdef EMBED_CONFIGURE_TOOL
		if (*(mdt->fcdata->markbit2) & 0x0200)
		{
			memset(&tctd,0,sizeof(tctd));
			tctd.mode = 28;//traffic control
			tctd.pattern = *(mdt->fcdata->patternid);
			tctd.lampcolor = 0x02;
			tctd.lamptime = 3;
			tctd.phaseid = 0;
			tctd.stageline = 0;	
			if (SUCCESS != timedown_data_to_embed(mdt->fcdata->sockfd,&tctd))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}
		}
		#endif
		fbdata[1] = 28;
		fbdata[2] = 0;
		fbdata[3] = 0x02;
		fbdata[4] = 3;
		if (!wait_write_serial(*(mdt->fcdata->fbserial)))
		{
			if (write(*(mdt->fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
			{
			#ifdef FULL_DETECT_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
		}
		else
		{
		#ifdef FULL_DETECT_DEBUG
			printf("face board serial port cannot write,Line:%d\n",__LINE__);
		#endif
		}

		ngf = 0;
		while (1)
		{//green flash
			if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x03))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;	
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
							cchan,0x03,mdt->fcdata->markbit))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
			to.tv_sec = 0;
			to.tv_usec = 500000;
			select(0,NULL,NULL,NULL,&to);

			if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x02))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
							cchan,0x02,mdt->fcdata->markbit))
			{
			#ifdef FULL_DETECT_DEBUG
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

		if (1 == pcexist)
		{
			//current phase begin to yellow lamp
			if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cnpchan,0x01))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
								cnpchan,0x01, mdt->fcdata->markbit))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
			if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cpchan,0x00))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
												cpchan,0x00,mdt->fcdata->markbit))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
		}
		else
		{
			if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x01))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
									cchan,0x01,mdt->fcdata->markbit))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
		}
		
		if (0 != cchan[0])
		{
			if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
			{//report info to server actively
				mdt->sinfo->conmode = 28;
				mdt->sinfo->color = 0x01;
				mdt->sinfo->time = 3;
				mdt->sinfo->stage = 3;
				mdt->sinfo->cyclet = 0;
				mdt->sinfo->phase = 0;
										
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == cnpchan[i])
						break;
					for (j = 0; j < mdt->sinfo->chans; j++)
					{
						if (0 == mdt->sinfo->csta[j])
							break;
						tcsta = mdt->sinfo->csta[j];
						tcsta &= 0xfc;
						tcsta >>= 2;
						tcsta &= 0x3f;
						if (cnpchan[i] == tcsta)
						{
							mdt->sinfo->csta[j] &= 0xfc;
							mdt->sinfo->csta[j] |= 0x01; //00000001-yellow
							break;
						}
					}
				}
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == cpchan[i])
						break;
					for (j = 0; j < mdt->sinfo->chans; j++)
					{
						if (0 == mdt->sinfo->csta[j])
							break;
						tcsta = mdt->sinfo->csta[j];
						tcsta &= 0xfc;
						tcsta >>= 2;
						tcsta &= 0x3f;
						if (cpchan[i] == tcsta)
						{
							mdt->sinfo->csta[j] &= 0xfc;
							break;
						}
					}
				}
				memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));
				memset(sibuf,0,sizeof(sibuf));
				if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
				}
			}//report info to server actively
		}//if ((0 != cchan[0]) && (pinfo.ytime > 0))

		if ((*(mdt->fcdata->markbit) & 0x0002) && (*(mdt->fcdata->markbit) & 0x0010))
		{
			memset(&tctd,0,sizeof(tctd));
			tctd.mode = 28;//traffic control
			tctd.pattern = *(mdt->fcdata->patternid);
			tctd.lampcolor = 0x01;
			tctd.lamptime = 3;
			tctd.phaseid = 0;
			tctd.stageline = 0;
			if (SUCCESS != timedown_data_to_conftool(mdt->fcdata->sockfd,&tctd))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}
		}
		#ifdef EMBED_CONFIGURE_TOOL
		if (*(mdt->fcdata->markbit2) & 0x0200)
		{
			memset(&tctd,0,sizeof(tctd));
			tctd.mode = 28;//traffic control
			tctd.pattern = *(mdt->fcdata->patternid);
			tctd.lampcolor = 0x01;
			tctd.lamptime = 3;
			tctd.phaseid = 0;
			tctd.stageline = 0;	
			if (SUCCESS != timedown_data_to_embed(mdt->fcdata->sockfd,&tctd))
			{
			#ifdef FULL_DETECT_DEBUG
				printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}
		}
		#endif
		fbdata[1] = 28;
		fbdata[2] = 0;
		fbdata[3] = 0x01;
		fbdata[4] = 3;
		if (!wait_write_serial(*(mdt->fcdata->fbserial)))
		{
			if (write(*(mdt->fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
			{
			#ifdef FULL_DETECT_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
		}
		else
		{
		#ifdef FULL_DETECT_DEBUG
			printf("face board serial port cannot write,Line:%d\n",__LINE__);
		#endif
		}

		sleep(3);

		if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x00))
		{
		#ifdef FULL_DETECT_DEBUG
			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			*(mdt->fcdata->markbit) |= 0x0800;
		}
		if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
						cchan,0x00,mdt->fcdata->markbit))
		{
		#ifdef FULL_DETECT_DEBUG
			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}
		
		if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
						dirch,0x02,mdt->fcdata->markbit))
		{
		#ifdef FULL_DETECT_DEBUG
			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}
		if (SUCCESS != ms_set_lamp_color(*(mdt->fcdata->bbserial),dirch,0x02))
		{
		#ifdef FULL_DETECT_DEBUG
			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			*(mdt->fcdata->markbit) |= 0x0800;
			return FAIL;
		}
		else
		{//set success
			if ((!(*(mdt->fcdata->markbit) & 0x1000)) && (!(*(mdt->fcdata->markbit) & 0x8000)))
			{//report info to server actively
				mdt->sinfo->conmode = 28;
				mdt->sinfo->color = 0x02;
				mdt->sinfo->time = 0;
				mdt->sinfo->stage = 0;
				mdt->sinfo->cyclet = 0;
				mdt->sinfo->phase = 0;
				mdt->sinfo->chans = 0;
				memset(mdt->sinfo->csta,0,sizeof(mdt->sinfo->csta));
				csta = mdt->sinfo->csta;
				for (i = 0; i < MAX_CHANNEL; i++)
				{
					if (0 == dirch[i])
						break;
					mdt->sinfo->chans += 1;
					tcsta = dirch[i];
					tcsta <<= 2;
					tcsta &= 0xfc;
					tcsta |= 0x02; //00000010-green 
					*csta = tcsta;
					csta++;
				}
				memcpy(mdt->fcdata->sinfo,mdt->sinfo,sizeof(statusinfo_t));
				memset(sibuf,0,sizeof(sibuf));
				if (SUCCESS != status_info_report(sibuf,mdt->sinfo))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
				else
				{
					write(*(mdt->fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
				}
			}//report info to server actively

			if ((*(mdt->fcdata->markbit) & 0x0002) && (*(mdt->fcdata->markbit) & 0x0010))
			{
				memset(&tctd,0,sizeof(tctd));
				tctd.mode = 28;//traffic control
				tctd.pattern = *(mdt->fcdata->patternid);
				tctd.lampcolor = 0x02;
				tctd.lamptime = 0;
				tctd.phaseid = 0;
				tctd.stageline = 0;
				if (SUCCESS != timedown_data_to_conftool(mdt->fcdata->sockfd,&tctd))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#ifdef EMBED_CONFIGURE_TOOL
			if (*(mdt->fcdata->markbit2) & 0x0200)
			{
				memset(&tctd,0,sizeof(tctd));
				tctd.mode = 28;//traffic control
				tctd.pattern = *(mdt->fcdata->patternid);
				tctd.lampcolor = 0x02;
				tctd.lamptime = 0;
				tctd.phaseid = 0;
				tctd.stageline = 0;	
				if (SUCCESS != timedown_data_to_embed(mdt->fcdata->sockfd,&tctd))
				{
				#ifdef FULL_DETECT_DEBUG
					printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
				#endif
				}
			}
			#endif
			fbdata[1] = 28;
			fbdata[2] = 0;
			fbdata[3] = 0x02;
			fbdata[4] = 0;
			if (!wait_write_serial(*(mdt->fcdata->fbserial)))
			{
				if (write(*(mdt->fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
				{
				#ifdef FULL_DETECT_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
			}
			else
			{
			#ifdef FULL_DETECT_DEBUG
				printf("face board serial port cannot write,Line:%d\n",__LINE__);
			#endif
			}	

			return SUCCESS;
		}//set success
	}//other condition
}

void ms_channel_yellow_flash(void *arg)
{
	cyellowflash_t			*cyf = arg;
	unsigned char			num = 0;
	struct timeval          to;

	while (1)
	{//while
		if (SUCCESS != ms_set_lamp_color(*(cyf->dc->fd->bbserial),cyf->chan,0x03))
		{
		#ifdef TIMING_DEBUG
			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}		
		if (SUCCESS != update_channel_status(cyf->dc->fd->sockfd,cyf->dc->cs,cyf->chan,0x03,cyf->dc->fd->markbit))
		{
		#ifdef TIMING_DEBUG
			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}
		to.tv_sec = 0;
		to.tv_usec = 500000;
		select(0,NULL,NULL,NULL,&to);		

		if (SUCCESS != ms_set_lamp_color(*(cyf->dc->fd->bbserial),cyf->chan,0x01))
		{
		#ifdef TIMING_DEBUG
			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}		
		if (SUCCESS != update_channel_status(cyf->dc->fd->sockfd,cyf->dc->cs,cyf->chan,0x01,cyf->dc->fd->markbit))
		{
		#ifdef TIMING_DEBUG
			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}
		to.tv_sec = 0;
		to.tv_usec = 500000;
		select(0,NULL,NULL,NULL,&to);

	#if 0
		num += 1;
		if (num >= (cyf->cyft))
			break;	
	#endif
	}//while

//	pthread_detach(pthread_self());
//	cyf->mark = 0;
    pthread_exit(NULL);
}
int ms_get_last_phaseinfo(fcdata_t *fd,tscdata_t *td,unsigned char tcline,dcpinfo_t *pinfo)
{
	if((NULL == fd) || (td == NULL)|| (pinfo == NULL))
	{
		#ifdef FULL_DETECT_DEBUG
		printf("pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		return MEMERR;
	}

	unsigned char  i = 0;
	if(*(fd->slnum) == 0)
	{
		while(1)
		{
			if (0 == td->timeconfig->TimeConfigList[tcline][i].StageId)
			break;
			i++;
		}
		*(fd->slnum) = i;
	}
	else
	{
		*(fd->slnum) = *(fd->slnum) -1;
	}
	memset(pinfo,0,sizeof(dcpinfo_t));
	ms_get_phase_info(fd,td,tcline,*(fd->slnum),pinfo);
	return SUCCESS;
}

