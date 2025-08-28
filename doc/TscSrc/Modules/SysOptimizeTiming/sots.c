/* 
 * File:   sots.c
 * Author: sk
 *
 * Created on 20150914
 */

#include "sot.h"

int sot_get_phase_info(fcdata_t *fd,tscdata_t *td,unsigned char tcline,unsigned char slnum,sotpinfo_t *pinfo)
{
	if ((NULL == td) || (NULL == pinfo))
	{
	#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
		printf("pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char			phaseid = 0;
	unsigned char			i = 0,j = 0,z = 0,k = 0,s = 0,x = 0;
	unsigned char			nphaseid = 0;
	unsigned char			nchan[MAX_CHANNEL] = {0};
	unsigned char			exist = 0;
	unsigned char			tchan = 0;

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
			pinfo->pctime = td->phase->PhaseList[i].PhaseWalkClear;
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

int sot_get_timeconfig(tscdata_t *td,unsigned char *pattern,unsigned char *tcline)
{
	if ((NULL == tcline) || (NULL == td) || (NULL == pattern))
	{
	#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
		printf("pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	unsigned char			i = 0;
	unsigned char			tcid = 0;
	unsigned char			exit = 0;

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
	#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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

int sot_set_lamp_color(int serial,unsigned char *chan,unsigned char color)
{
	if (NULL == chan)
	{
	#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				return FAIL;
			}
		}
		else
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
	}
	else
	{
	#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
		printf("serial port cannot be written,File:%s,Line:%d\n",__FILE__,__LINE__);
	#endif
		return FAIL;
	}

	return SUCCESS;
}

int sot_send_traffic_para(unsigned char dn,int *csockfd,optidata_t *odc,sotphasedetect_t *spd)
{
	if ((NULL == odc) || (NULL == spd) || (NULL == csockfd))
	{
	#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
		printf("Pointer address err,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	
	unsigned char			i = 0,j = 0;
	double					fenzi = 0;
	double					fenmu = 1;
	double					gratio = 0; //green ratio	
	double					temp1 = 0;
	double					temp2 = 0;
	double					temp3 = 0;
	double					temp4 = 0;
	double					temp5 = 0;
	unsigned char			*reflags = odc->reflags;
	unsigned long			maxocctime = 0;
	unsigned int			maot = 0;

	if ((odc->opticycle) > 0)
	{
		odc->cycle = odc->opticycle;
	}

	odc->opticycle = 0;
	odc->phasenum = 0;
	if (1 == dn)
	{//one detector
		for (i = 0; i < MAX_PHASE_LINE; i++)
		{//0
			if (0 == spd[i].stageid)
				break;
			odc->phasenum += 1;
			spd[i].pt = 0;//phase time
			spd[i].pf = 0; //phase flow
			spd[i].kf = 0; //key road flow
			spd[i].kff = 0; //key road full flow
			spd[i].pfd = 0;//phase full degree
			spd[i].kd = 0; //key detector
			spd[i].kfr = 0; //key flow ratio
			spd[i].acs = 0; //ave car speed
			spd[i].acd = 0; //ave car delay
			spd[i].sct = 0; //stop car time
			spd[i].occrate = 0; //occupy rate
			spd[i].chw = 0; //car head way
			spd[i].pc = 0; //pass capacity
			spd[i].gtbak = spd[i].gtime;//bak green time;
			maxocctime = 0;
			maot = 0;
			for (j = 0; j < 10; j++)
			{//1
				if (0 == spd[i].sotdetectpro[j].deteid)
					break;
				if (spd[i].kf <= spd[i].sotdetectpro[j].icarnum)
				{
					spd[i].kf = spd[i].sotdetectpro[j].icarnum;
					spd[i].kd = spd[i].sotdetectpro[j].deteid;
					spd[i].kff = spd[i].sotdetectpro[j].fullflow;
				}
				if (maxocctime <= spd[i].sotdetectpro[j].maxocctime)
				{
					maxocctime = spd[i].sotdetectpro[j].maxocctime;
				}

				if (0 == spd[i].sotdetectpro[j].icarnum)
					spd[i].sotdetectpro[j].icarnum = 1;				
				if (maot <= ((spd[i].sotdetectpro[j].maxocctime)/(spd[i].sotdetectpro[j].icarnum)))
					maot = (spd[i].sotdetectpro[j].maxocctime)/(spd[i].sotdetectpro[j].icarnum);
				spd[i].pf += spd[i].sotdetectpro[j].icarnum;
				spd[i].pf += spd[i].sotdetectpro[j].redflow; 

				spd[i].sotdetectpro[j].icarnum = 0;
				spd[i].sotdetectpro[j].ocarnum = 0;
				spd[i].sotdetectpro[j].maxocctime = 0;
				spd[i].sotdetectpro[j].redflow = 0;	
			}//1
			spd[i].maot = maot;
			spd[i].occrate = \
			(double)maxocctime/(1000*(spd[i].gtime + spd[i].gftime + spd[i].ytime - spd[i].startdelay));		
            spd[i].pt = spd[i].syscoofixgreen +  spd[i].ytime + spd[i].startdelay + spd[i].rtime; 

			fenzi = (double)3600 * spd[i].kf;
			fenmu = (double)spd[i].kff * (double)spd[i].vgtime; 
			if ((-0.000001 < fenmu) && (fenmu < 0.000001))
				fenmu = 1;
			spd[i].pfd = fenzi/fenmu;

			fenzi = spd[i].kf * (double)3600;
			fenmu = spd[i].kff * (double)(odc->cycle);
			if ((-0.000001 < fenmu) && (fenmu < 0.000001))
				fenmu = 1;
			spd[i].kfr = fenzi/fenmu;

			
			fenzi = (double)3600 * spd[i].kf;
			if (spd[i].pfd < 1)
				fenmu = (double)spd[i].kff * 0.8;
			if (spd[i].pfd > 1)
				fenmu = (double)spd[i].kff * 0.6;
			if ((-0.000001 < fenmu) && (fenmu < 0.000001))
				fenmu = 1;
			spd[i].vgtime = (int)fenzi/fenmu;	
			if (spd[i].vgtime < spd[i].vmingtime)
				spd[i].vgtime = spd[i].vmingtime;
			if (spd[i].vgtime > spd[i].vmaxgtime)
				spd[i].vgtime = spd[i].vmaxgtime;
			
			spd[i].gtime = spd[i].vgtime + spd[i].startdelay - spd[i].ytime; 
			if (spd[i].optipt > 0)
				spd[i].curpt = spd[i].optipt;	
            spd[i].optipt =  spd[i].gtime +  spd[i].gftime + spd[i].ytime + spd[i].rtime;
			odc->opticycle += spd[i].optipt;
		}//0
		for (i = 0; i < MAX_PHASE_LINE; i++)
        {//1
            if (0 == spd[i].stageid)
                break;
			if ((-0.000001 < (odc->cycle)) && ((odc->cycle) < 0.000001))
                odc->cycle = 1;
			gratio = (double)(spd[i].syscoofixgreen)/(odc->cycle);
			if (spd[i].pfd < 1)
			{
				fenzi = (odc->cycle) * (1-gratio) * (1-gratio);
				fenmu = 2 * (1-gratio*spd[i].pfd);
				if ((-0.000001 < fenmu) && (fenmu < 0.000001))
                	fenmu = 1;
				temp1 = fenzi/fenmu;

				fenzi = spd[i].pfd * spd[i].pfd;
				fenmu = 2 * spd[i].kf * (1 - spd[i].pfd);
				if ((-0.000001 < fenmu) && (fenmu < 0.000001))
                    fenmu = 1;
				temp2 = fenzi/fenmu;
	
				temp4 = spd[i].kf * spd[i].kf;
				if ((-0.000001 < temp4) && (temp4 < 0.000001))
                    temp4 = 1;
				temp4 = (odc->cycle)/temp4;
				temp4 = powf(temp4,1.0/3);
				temp5 = 2.0 + 5.0 * gratio;
				temp5 = powf(spd[i].pfd,temp5);
				temp3 = 0.65 * temp4 * temp5;
				
				spd[i].acd = temp1 + temp2 - temp3;
			}
			else if (1 == spd[i].pfd)
			{
				fenzi = (odc->cycle) * (1-gratio) * (1-gratio);
				fenmu = 2 * (1-gratio*spd[i].pfd);
				if ((-0.000001 < fenmu) && (fenmu < 0.000001))
                    fenmu = 1;
				temp1 = fenzi/fenmu;
				spd[i].acd = temp1;
			}
			else if (spd[i].pfd > 1)
			{
				fenzi = (odc->cycle) * (1-gratio) * (1-gratio);
				fenmu = 2 * (1-gratio*spd[i].pfd);
				if ((-0.000001 < fenmu) && (fenmu < 0.000001))
                    fenmu = 1;
				temp1 = fenzi/fenmu;
				fenzi = odc->opticycle;
				fenmu = spd[i].kf - ((spd[i].syscoofixgreen * spd[i].kff)/3600);
				if ((-0.000001 < fenmu) && (fenmu < 0.000001))
                    fenmu = 1;
				temp2 = fenzi/fenmu;
				spd[i].acd = temp1 + temp2;	
			}
			
			if ((!(*reflags & 0x08)) && (!(*reflags & 0x04)))
			{//level 1 road
//				temp4 = (spd[i].kf*((double)3600/odc->cycle))/(double)1560;
				temp4 = (spd[i].kf*((double)3600/spd[i].curpt))/(double)1560;
		//		if (temp4 > 0.8)
		//		{
				temp5 = 1.726 + 3.15*(powf(temp4,3));
				fenzi = 66.5;
				fenmu = 1 + powf(temp4,temp5);
				if ((-0.000001 < fenmu) && (fenmu < 0.000001))
					fenmu = 1;
				spd[i].acs = fenzi/fenmu;
		//		}
		//		else
		//		{
		//			fenzi = 66.5;
		//			fenmu = 1 + 0.15*(powf(temp4,4));
		//			if ((-0.000001 < fenmu) && (fenmu < 0.000001))
        //                fenmu = 1;
		//			spd[i].acs = fenzi/fenmu;
		//		}
			}//level 1 road
			else if ((!(*reflags & 0x08)) && (*reflags & 0x04))
			{//level 2 road
//				temp4 = (spd[i].kf*((double)3600/odc->cycle))/(double)1450;
				temp4 = (spd[i].kf*((double)3600/spd[i].curpt))/(double)1450;
		//		if (temp4 > 0.8)
		//		{
				temp5 = 2.076 + 2.870*(powf(temp4,3));
				fenzi = 57.6;
				fenmu = 1 + powf(temp4,temp5);
				if ((-0.000001 < fenmu) && (fenmu < 0.000001))
					fenmu = 1;
				spd[i].acs = fenzi/fenmu;
		//		}
		//		else
		//		{
		//			fenzi = 57.6;
		//			fenmu = 1 + 0.15*(powf(temp4,4));
		//			if ((-0.000001 < fenmu) && (fenmu < 0.000001))
          //              fenmu = 1;
		//			spd[i].acs = fenzi/fenmu;
		//		}
			}//level 2 road
			else if ((*reflags & 0x08) && (!(*reflags & 0x04)))
			{//level 3 road
	//				temp4 = (spd[i].kf*((double)3600/odc->cycle))/(double)1450;
				temp4 = (spd[i].kf*((double)3600/spd[i].curpt))/(double)1450;
	//			if (temp4 > 0.8)
	//			{
				temp5 = 2.153 + 3.984*(powf(temp4,3));
				fenzi = 57.6;
				fenmu = 1 + powf(temp4,temp5);
				if ((-0.000001 < fenmu) && (fenmu < 0.000001))
					fenmu = 1;
				spd[i].acs = fenzi/fenmu;
	//			}
	//			else
	//			{
	//				fenzi = 57.6;
	//				fenmu = 1 + 0.15*(powf(temp4,4));
	//				if ((-0.000001 < fenmu) && (fenmu < 0.000001))
      //                  fenmu = 1;
	//				spd[i].acs = fenzi/fenmu;
	//			}
			}//level 3 road

			if ((-0.000001 < spd[i].kf) && (spd[i].kf < 0.000001))
            	spd[i].kf = 1;
			spd[i].chw = (spd[i].vgtime + spd[i].gftime)/spd[i].kf;
			
			fenzi = (odc->cycle) - (spd[i].syscoofixgreen);
			fenmu = (odc->cycle)*(1 - (spd[i].kfr));
			if ((-0.000001 < fenmu) && (fenmu < 0.000001))
            	fenmu = 1;
			spd[i].sct = 0.9 * (fenzi/fenmu);
			if (spd[i].sct > 3.0)
				spd[i].sct = 3.0;
			
			spd[i].pc = spd[i].pfd * spd[i].kff;
		}//1
	}//one detector
	else if (2 == dn)
	{//two detector
	#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
		printf("Not consider two detector,File:%s,Line:%d\n",__FILE__,__LINE__);
	#endif
		return SUCCESS;
	}//two detector
#if 0
	#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
	printf("patternid: %d,cycle: %d,phasenum: %d,opticycle: %d\n", \
				odc->patternid,odc->cycle,odc->phasenum,odc->opticycle);
	for(i = 0; i < MAX_PHASE_LINE; i++)
	{
		if (0 == spd[i].stageid)
			break;
		printf("i:%d,phaseid:%d,phasetime:%d,phaseflow:%d,keyroadflow:%d,phasefulldegree:%f,keydetector:%d, \
				keyflowratio:%f,avecarspeed:%d,avecardelay:%f,stopcartime:%f,occrate:%f,carheadway:%f, \
				passcapacity:%f,maot:%d\n",i,spd[i].phaseid,spd[i].pt,spd[i].pf,spd[i].kf,spd[i].pfd, \
				spd[i].kd,spd[i].kfr,spd[i].acs,spd[i].acd,spd[i].sct,spd[i].occrate,spd[i].chw, \
				spd[i].pc,spd[i].maot);	
	}
	#endif
#endif
	if ((!(*(odc->markbit) & 0x1000)) && (!(*(odc->markbit) & 0x8000)))
	{//if ((!(*(od->markbit) & 0x1000)) && (!(*(od->markbit) & 0x8000)))
		unsigned char				buf[256] = {0};
		unsigned char				*pbuf = buf;	
		unsigned short				tpt = 0;
		unsigned char				tpc = 0;

		*pbuf = 0x21;
		pbuf++;
		*pbuf = 0x83;
		pbuf++;
		*pbuf = 0xE3;
		pbuf++;
		*pbuf = 0x00;
		pbuf++;
		*pbuf = odc->patternid;
		pbuf++;
		
		tpt = odc->cycle;
		tpt &= 0xff00;
		tpt >>= 8;
		tpt &= 0x00ff;
		*pbuf = tpt;
		pbuf++;
		tpt = odc->cycle;
		tpt &= 0x00ff;
		*pbuf = tpt;
		pbuf++;	

		tpt = odc->opticycle;
        tpt &= 0xff00;
        tpt >>= 8;
        tpt &= 0x00ff;
        *pbuf = tpt;
        pbuf++;
        tpt = odc->opticycle;
        tpt &= 0x00ff;
        *pbuf = tpt;
        pbuf++;

		*pbuf = odc->phasenum;
		pbuf++;

		for (i = 0; i < MAX_PHASE_LINE; i++)
		{
			if (0 == spd[i].stageid)
				break;
			*pbuf = spd[i].phaseid;
			pbuf++;
			*pbuf = spd[i].pt;
			pbuf++;

			tpt = spd[i].pf;
			tpt &= 0xff00;
			tpt >>= 8;
			tpt &= 0x00ff;
			*pbuf = tpt;
			pbuf++;
			tpt = spd[i].pf;
			tpt &= 0x00ff;
			*pbuf = tpt;
			pbuf++;

			tpt = spd[i].kf;
			tpt &= 0xff00;
			tpt >>= 8;
			tpt &= 0x00ff;
			*pbuf = tpt;
			pbuf++;
			tpt = spd[i].kf;
			tpt &= 0x00ff;
			*pbuf = tpt;
			pbuf++;

			tpc = spd[i].pfd * 100;
			*pbuf = tpc;
			pbuf++;
						
			tpc = spd[i].kd;
			*pbuf = tpc;
			pbuf++;

			tpc = spd[i].kfr * 100;
			*pbuf = tpc;
			pbuf++;

			tpc = spd[i].acs;
			*pbuf = tpc;
			pbuf++;
		
			tpt = spd[i].acd * 100;
			tpt &= 0xff00;
			tpt >>= 8;
			tpt &= 0x00ff;
			*pbuf = tpt;
			pbuf++;
			tpt = spd[i].acd * 100;
			tpt &= 0x00ff;
			*pbuf = tpt;
			pbuf++;

			tpc = spd[i].sct * 10;
			*pbuf = tpc;
			pbuf++;

			tpc = spd[i].occrate * 100;
			*pbuf = tpc;
			pbuf++;

			tpt = spd[i].chw * 10;
			tpt &= 0xff00;
			tpt >>= 8;
			tpt &= 0x00ff;
			*pbuf = tpt;
			pbuf++;
			tpt = spd[i].chw * 10;
			tpt &= 0x00ff;
			*pbuf = tpt;
			pbuf++;

			tpt = spd[i].pc;
			tpt &= 0xff00;
			tpt >>= 8;
			tpt &= 0x00ff;
			*pbuf = tpt;
			pbuf++;
			tpt = spd[i].pc;
			tpt &= 0x00ff;
			*pbuf = tpt;
			pbuf++;
	
			*pbuf = spd[i].optipt;
            pbuf++;								
		}

		if (write(*csockfd,buf,sizeof(buf)) < 0)
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("write error,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}	
	}//if ((!(*(od->markbit) & 0x1000)) && (!(*(od->markbit) & 0x8000)))

	return SUCCESS;
}
int sot_backup_pattern_data(int serial,unsigned char snum,sotphasedetect_t *phd)
{
	if ((NULL == phd) || (0 == snum))
	{
	#ifdef TIMING_DEBUG
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
        #ifdef TIMING_DEBUG
            printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
            return FAIL;
        }
    }
    else
    {
    #ifdef TIMING_DEBUG
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
			#ifdef TIMING_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				return FAIL;
			}
		}
		else
		{
		#ifdef TIMING_DEBUG
			printf("serial port cannot be written,File:%s,Line:%d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
	}//for (i = 0;i < snum;i++)	

	return SUCCESS;
}
#if 0
int sot_send_traffic_para(unsigned char dn,int *csockfd,optidata_t *odc,sotphasedetect_t *spd)
{
	if ((NULL == odc) || (NULL == spd) || (NULL == csockfd))
	{
	#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
		printf("Pointer address err,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	
	unsigned char			i = 0,j = 0;
	double					fenzi = 0;
	double					fenmu = 1;
	double					gratio = 0; //green ratio	
	double					temp1 = 0;
	double					temp2 = 0;
	double					temp3 = 0;
	double					temp4 = 0;
	double					temp5 = 0;
	unsigned char			*reflags = odc->reflags;
	unsigned long			maxocctime = 0;
	unsigned int			maot = 0;

	if ((odc->opticycle) > 0)
	{
		odc->cycle = odc->opticycle;
	}

	odc->opticycle = 0;
	odc->phasenum = 0;
	if (1 == dn)
	{//one detector
		for (i = 0; i < MAX_PHASE_LINE; i++)
		{//0
			if (0 == spd[i].stageid)
				break;
			odc->phasenum += 1;
			spd[i].pt = 0;//phase time
			spd[i].pf = 0; //phase flow
			spd[i].kf = 0; //key road flow
			spd[i].kff = 0; //key road full flow
			spd[i].pfd = 0;//phase full degree
			spd[i].kd = 0; //key detector
			spd[i].kfr = 0; //key flow ratio
			spd[i].acs = 0; //ave car speed
			spd[i].acd = 0; //ave car delay
			spd[i].sct = 0; //stop car time
			spd[i].occrate = 0; //occupy rate
			spd[i].chw = 0; //car head way
			spd[i].pc = 0; //pass capacity
			spd[i].gtbak = spd[i].gtime;//bak green time;
			maxocctime = 0;
			maot = 0;
			for (j = 0; j < 10; j++)
			{//1
				if (0 == spd[i].sotdetectpro[j].deteid)
					break;
				if (spd[i].kf <= spd[i].sotdetectpro[j].icarnum)
				{
					spd[i].kf = spd[i].sotdetectpro[j].icarnum;
					spd[i].kd = spd[i].sotdetectpro[j].deteid;
					spd[i].kff = spd[i].sotdetectpro[j].fullflow;
				}
				if (maxocctime <= spd[i].sotdetectpro[j].maxocctime)
				{
					maxocctime = spd[i].sotdetectpro[j].maxocctime;
				}

				if (0 == spd[i].sotdetectpro[j].icarnum)
					spd[i].sotdetectpro[j].icarnum = 1;				
				if (maot <= ((spd[i].sotdetectpro[j].maxocctime)/(spd[i].sotdetectpro[j].icarnum)))
					maot = (spd[i].sotdetectpro[j].maxocctime)/(spd[i].sotdetectpro[j].icarnum);
				spd[i].pf += spd[i].sotdetectpro[j].icarnum;
				spd[i].pf += spd[i].sotdetectpro[j].redflow; 

				spd[i].sotdetectpro[j].icarnum = 0;
				spd[i].sotdetectpro[j].ocarnum = 0;
				spd[i].sotdetectpro[j].maxocctime = 0;
				spd[i].sotdetectpro[j].redflow = 0;	
			}//1
			spd[i].maot = maot;
			spd[i].occrate = \
				maxocctime/(1000*(spd[i].gtime + spd[i].gftime + spd[i].ytime - spd[i].startdelay));			

			fenzi = 3600 * spd[i].kf;
			fenmu = spd[i].kff * spd[i].vgtime; 
			if ((-0.000001 < fenmu) && (fenmu < 0.000001))
				fenmu = 1;
			spd[i].pfd = fenzi/fenmu;

			fenzi = spd[i].kf * 3600;
			fenmu = spd[i].kff * (odc->cycle);
			if ((-0.000001 < fenmu) && (fenmu < 0.000001))
				fenmu = 1;
			spd[i].kfr = fenzi/fenmu;

			
			fenzi = 3600 * spd[i].kf;
			if (spd[i].pfd < 1)
				fenmu = spd[i].kff * 0.8;
			if (spd[i].pfd > 1)
				fenmu = spd[i].kff * 0.6;
			if ((-0.000001 < fenmu) && (fenmu < 0.000001))
				fenmu = 1;
			spd[i].vgtime = fenzi/fenmu;	
			if (spd[i].vgtime < spd[i].vmingtime)
				spd[i].vgtime = spd[i].vmingtime;
			if (spd[i].vgtime > spd[i].vmaxgtime)
				spd[i].vgtime = spd[i].vmaxgtime;
			spd[i].gtime = spd[i].vgtime + spd[i].startdelay - spd[i].ytime; 	
			
			spd[i].pt = spd[i].gtime + spd[i].gftime + spd[i].ytime + spd[i].rtime;
			spd[i].optipt = spd[i].pt;
			odc->opticycle += spd[i].pt;
		}//0
		for (i = 0; i < MAX_PHASE_LINE; i++)
        {//1
            if (0 == spd[i].stageid)
                break;
			if ((-0.000001 < (odc->cycle)) && ((odc->cycle) < 0.000001))
                odc->cycle = 1;
			gratio = (spd[i].gtbak)/(odc->cycle);
			if (spd[i].pfd < 1)
			{
				fenzi = (odc->cycle) * (1-gratio) * (1-gratio);
				fenmu = 2 * (1-gratio*spd[i].pfd);
				if ((-0.000001 < fenmu) && (fenmu < 0.000001))
                	fenmu = 1;
				temp1 = fenzi/fenmu;

				fenzi = spd[i].pfd * spd[i].pfd;
				fenmu = 2 * spd[i].kf * (1 - spd[i].pfd);
				if ((-0.000001 < fenmu) && (fenmu < 0.000001))
                    fenmu = 1;
				temp2 = fenzi/fenmu;
	
				temp4 = spd[i].kf * spd[i].kf;
				if ((-0.000001 < temp4) && (temp4 < 0.000001))
                    temp4 = 1;
				temp4 = (odc->cycle)/temp4;
				temp4 = powf(temp4,1.0/3);
				temp5 = 2.0 + 5.0 * gratio;
				temp5 = powf(spd[i].pfd,temp5);
				temp3 = 0.65 * temp4 * temp5;
				
				spd[i].acd = temp1 + temp2 - temp3;
			}
			else if (1 == spd[i].pfd)
			{
				fenzi = (odc->cycle) * (1-gratio) * (1-gratio);
				fenmu = 2 * (1-gratio*spd[i].pfd);
				if ((-0.000001 < fenmu) && (fenmu < 0.000001))
                    fenmu = 1;
				temp1 = fenzi/fenmu;
				spd[i].acd = temp1;
			}
			else if (spd[i].pfd > 1)
			{
				fenzi = (odc->cycle) * (1-gratio) * (1-gratio);
				fenmu = 2 * (1-gratio*spd[i].pfd);
				if ((-0.000001 < fenmu) && (fenmu < 0.000001))
                    fenmu = 1;
				temp1 = fenzi/fenmu;
				fenzi = odc->opticycle;
				fenmu = spd[i].kf - ((spd[i].gtbak * spd[i].kff)/3600);
				if ((-0.000001 < fenmu) && (fenmu < 0.000001))
                    fenmu = 1;
				temp2 = fenzi/fenmu;
				spd[i].acd = temp1 + temp2;	
			}
			
			if ((!(*reflags & 0x08)) && (!(*reflags & 0x04)))
			{//level 1 road
				temp4 = (spd[i].kf)/1560;
				if (temp4 > 0.8)
				{
					temp5 = 1.726 + 3.15*(powf(temp4,3));
					fenzi = 66.5;
					fenmu = 1 + powf(temp4,temp5);
					if ((-0.000001 < fenmu) && (fenmu < 0.000001))
                    	fenmu = 1;
					spd[i].acs = fenzi/fenmu;
				}
				else
				{
					fenzi = 66.5;
					fenmu = 1 + 0.15*(powf(temp4,4));
					if ((-0.000001 < fenmu) && (fenmu < 0.000001))
                        fenmu = 1;
					spd[i].acs = fenzi/fenmu;
				}
			}//level 1 road
			else if ((!(*reflags & 0x08)) && (*reflags & 0x04))
			{//level 2 road
				temp4 = (spd[i].kf)/1450;
				if (temp4 > 0.8)
				{
					temp5 = 2.076 + 2.870*(powf(temp4,3));
					fenzi = 57.6;
					fenmu = 1 + powf(temp4,temp5);
					if ((-0.000001 < fenmu) && (fenmu < 0.000001))
                        fenmu = 1;
					spd[i].acs = fenzi/fenmu;
				}
				else
				{
					fenzi = 57.6;
					fenmu = 1 + 0.15*(powf(temp4,4));
					if ((-0.000001 < fenmu) && (fenmu < 0.000001))
                        fenmu = 1;
					spd[i].acs = fenzi/fenmu;
				}
			}//level 2 road
			else if ((*reflags & 0x08) && (!(*reflags & 0x04)))
			{//level 3 road
				temp4 = (spd[i].kf)/1450;
				if (temp4 > 0.8)
				{
					temp5 = 2.153 + 3.984*(powf(temp4,3));
					fenzi = 57.6;
					fenmu = 1 + powf(temp4,temp5);
					if ((-0.000001 < fenmu) && (fenmu < 0.000001))
                        fenmu = 1;
					spd[i].acs = fenzi/fenmu;
				}
				else
				{
					fenzi = 57.6;
					fenmu = 1 + 0.15*(powf(temp4,4));
					if ((-0.000001 < fenmu) && (fenmu < 0.000001))
                        fenmu = 1;
					spd[i].acs = fenzi/fenmu;
				}
			}//level 3 road

			if ((-0.000001 < spd[i].kf) && (spd[i].kf < 0.000001))
            	spd[i].kf = 1;
			spd[i].chw = (spd[i].vgtime + spd[i].gftime)/spd[i].kf;
			
			fenzi = (odc->cycle) - (spd[i].gtbak) - (spd[i].gftime);
			fenmu = (odc->cycle)*(1 - (spd[i].kfr));
			if ((-0.000001 < fenmu) && (fenmu < 0.000001))
            	fenmu = 1;
			spd[i].sct = 0.9 * (fenzi/fenmu);
			if (spd[i].sct > 3.0)
				spd[i].sct = 3.0;
			
			spd[i].pc = spd[i].pfd * spd[i].kff;
		}//1
	}//one detector
	else if (2 == dn)
	{//two detector
	#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
		printf("Not consider two detector,File:%s,Line:%d\n",__FILE__,__LINE__);
	#endif
		return SUCCESS;
	}//two detector

	#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
	printf("patternid: %d,cycle: %d,phasenum: %d,opticycle: %d\n", \
				odc->patternid,odc->cycle,odc->phasenum,odc->opticycle);
	for(i = 0; i < MAX_PHASE_LINE; i++)
	{
		if (0 == spd[i].stageid)
			break;
		printf("i:%d,phaseid:%d,phasetime:%d,phaseflow:%d,keyroadflow:%d,phasefulldegree:%f,keydetector:%d, \
				keyflowratio:%f,avecarspeed:%d,avecardelay:%f,stopcartime:%f,occrate:%f,carheadway:%f, \
				passcapacity:%f,maot:%d\n",i,spd[i].phaseid,spd[i].pt,spd[i].pf,spd[i].kf,spd[i].pfd, \
				spd[i].kd,spd[i].kfr,spd[i].acs,spd[i].acd,spd[i].sct,spd[i].occrate,spd[i].chw, \
				spd[i].pc,spd[i].maot);	
	}
	#endif

	if ((!(*(odc->markbit) & 0x1000)) && (!(*(odc->markbit) & 0x8000)))
	{//if ((!(*(od->markbit) & 0x1000)) && (!(*(od->markbit) & 0x8000)))
		unsigned char				buf[1024] = {0};
		unsigned char				*pbuf = buf;	
		unsigned short				tpt = 0;
		unsigned char				tpc = 0;

		*pbuf = 0x21;
		pbuf++;
		*pbuf = 0x83;
		pbuf++;
		*pbuf = 0xE3;
		pbuf++;
		*pbuf = 0x00;
		pbuf++;
		*pbuf = odc->patternid;
		pbuf++;
		
		tpt = odc->cycle;
		tpt &= 0xff00;
		tpt >>= 8;
		tpt &= 0x00ff;
		*pbuf = tpt;
		pbuf++;
		tpt = odc->cycle;
		tpt &= 0x00ff;
		*pbuf = tpt;
		pbuf++;	

		tpt = odc->opticycle;
        tpt &= 0xff00;
        tpt >>= 8;
        tpt &= 0x00ff;
        *pbuf = tpt;
        pbuf++;
        tpt = odc->opticycle;
        tpt &= 0x00ff;
        *pbuf = tpt;
        pbuf++;

		*pbuf = odc->phasenum;
		pbuf++;

		for (i = 0; i < MAX_PHASE_LINE; i++)
		{
			if (0 == spd[i].stageid)
				break;
			*pbuf = spd[i].phaseid;
			pbuf++;
			*pbuf = spd[i].pt;
			pbuf++;

			tpt = spd[i].pf;
			tpt &= 0xff00;
			tpt >>= 8;
			tpt &= 0x00ff;
			*pbuf = tpt;
			pbuf++;
			tpt = spd[i].pf;
			tpt &= 0x00ff;
			*pbuf = tpt;
			pbuf++;

			tpt = spd[i].kf;
			tpt &= 0xff00;
			tpt >>= 8;
			tpt &= 0x00ff;
			*pbuf = tpt;
			pbuf++;
			tpt = spd[i].kf;
			tpt &= 0x00ff;
			*pbuf = tpt;
			pbuf++;

			tpc = spd[i].pfd * 100;
			*pbuf = tpc;
			pbuf++;
						
			tpc = spd[i].kd;
			*pbuf = tpc;
			pbuf++;

			tpc = spd[i].kfr * 100;
			*pbuf = tpc;
			pbuf++;

			tpc = spd[i].acs;
			*pbuf = tpc;
			pbuf++;
		
			tpt = spd[i].acd * 100;
			tpt &= 0xff00;
			tpt >>= 8;
			tpt &= 0x00ff;
			*pbuf = tpt;
			pbuf++;
			tpt = spd[i].acd * 100;
			tpt &= 0x00ff;
			*pbuf = tpt;
			pbuf++;

			tpc = spd[i].sct * 10;
			*pbuf = tpc;
			pbuf++;

			tpc = spd[i].occrate * 100;
			*pbuf = tpc;
			pbuf++;

			tpt = spd[i].chw * 10;
			tpt &= 0xff00;
			tpt >>= 8;
			tpt &= 0x00ff;
			*pbuf = tpt;
			pbuf++;
			tpt = spd[i].chw * 10;
			tpt &= 0x00ff;
			*pbuf = tpt;
			pbuf++;

			tpt = spd[i].pc;
			tpt &= 0xff00;
			tpt >>= 8;
			tpt &= 0x00ff;
			*pbuf = tpt;
			pbuf++;
			tpt = spd[i].pc;
			tpt &= 0x00ff;
			*pbuf = tpt;
			pbuf++;
	
			*pbuf = spd[i].optipt;
            pbuf++;								
		}

		if (write(*csockfd,buf,sizeof(buf)) < 0)
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("write error,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}	
	}//if ((!(*(od->markbit) & 0x1000)) && (!(*(od->markbit) & 0x8000)))

	return SUCCESS;
}
#endif
int sot_dirch_control(markdata_t *mdt,unsigned char cktem,unsigned char ktem,unsigned char (*dirch)[8],sotpinfo_t *pinfo)
{
	if ((NULL == mdt) || (NULL == pinfo) || (NULL == *dirch))
	{
	#ifdef TIMING_DEBUG
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
		if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),dirch[ktem-0x20],0x02))
		{
		#ifdef TIMING_DEBUG
			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			*(mdt->fcdata->markbit) |= 0x0800;
			return FAIL;
		}
		if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
									dirch[ktem-0x20],0x02,mdt->fcdata->markbit))
		{
		#ifdef TIMING_DEBUG
			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}

		if (!wait_write_serial(*(mdt->fcdata->sdserial)))
		{
			if (write(*(mdt->fcdata->sdserial),echo,sizeof(echo)) < 0)
			{
			#ifdef TIMING_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			else
			{
			#ifdef TIMING_DEBUG
				printf("write success,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
		}
		else
		{
		#ifdef TIMING_DEBUG
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
			#ifdef TIMING_DEBUG
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
				#ifdef TIMING_DEBUG
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
				#ifdef TIMING_DEBUG
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
			#ifdef TIMING_DEBUG
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
			#ifdef TIMING_DEBUG
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
			#ifdef TIMING_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
		}
		else
		{
		#ifdef TIMING_DEBUG
			printf("face board serial port cannot write,Line:%d\n",__LINE__);
		#endif
		}

		ngf = 0;
		if ((0 != cchan[0]) && (pinfo->gftime > 0))
		{
			while (1)
			{//green flash
				if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x03))
				{
				#ifdef TIMING_DEBUG
					printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;	
				}
				if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
								cchan,0x03,mdt->fcdata->markbit))
				{
				#ifdef TIMING_DEBUG
					printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				to.tv_sec = 0;
				to.tv_usec = 500000;
				select(0,NULL,NULL,NULL,&to);

				if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x02))
				{
				#ifdef TIMING_DEBUG
					printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
								cchan,0x02,mdt->fcdata->markbit))
				{
				#ifdef TIMING_DEBUG
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
			if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cnpchan,0x01))
			{
			#ifdef TIMING_DEBUG
				printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
								cnpchan,0x01, mdt->fcdata->markbit))
			{
			#ifdef TIMING_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
			if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cpchan,0x00))
			{
			#ifdef TIMING_DEBUG
				printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
												cpchan,0x00,mdt->fcdata->markbit))
			{
			#ifdef TIMING_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
		}
		else
		{
			if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x01))
			{
			#ifdef TIMING_DEBUG
				printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
									cchan,0x01,mdt->fcdata->markbit))
			{
			#ifdef TIMING_DEBUG
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
				#ifdef TIMING_DEBUG
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
			#ifdef TIMING_DEBUG
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
			#ifdef TIMING_DEBUG
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
			#ifdef TIMING_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
		}
		else
		{
		#ifdef TIMING_DEBUG
			printf("face board serial port cannot write,Line:%d\n",__LINE__);
		#endif
		}
		sleep(pinfo->ytime);

		if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x00))
		{
		#ifdef TIMING_DEBUG
			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			*(mdt->fcdata->markbit) |= 0x0800;
		}
		if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
						cchan,0x00,mdt->fcdata->markbit))
		{
		#ifdef TIMING_DEBUG
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
				#ifdef TIMING_DEBUG
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
				#ifdef TIMING_DEBUG
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
				#ifdef TIMING_DEBUG
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
				#ifdef TIMING_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
			}
			else
			{
			#ifdef TIMING_DEBUG
				printf("face board serial port cannot write,Line:%d\n",__LINE__);
			#endif
			}
			sleep(pinfo->rtime);
		}
		
		if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),dirch[ktem-0x20],0x02))
		{
		#ifdef TIMING_DEBUG
			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			*(mdt->fcdata->markbit) |= 0x0800;
		}
		if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
						dirch[ktem-0x20],0x02,mdt->fcdata->markbit))
		{
		#ifdef TIMING_DEBUG
			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}
		if (!wait_write_serial(*(mdt->fcdata->sdserial)))
		{
			if (write(*(mdt->fcdata->sdserial),echo,sizeof(echo)) < 0)
			{
			#ifdef TIMING_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			else
			{
			#ifdef TIMING_DEBUG
				printf("write success,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
		}
		else
		{
		#ifdef TIMING_DEBUG
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
			#ifdef TIMING_DEBUG
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
			#ifdef TIMING_DEBUG
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
			#ifdef TIMING_DEBUG
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
			#ifdef TIMING_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
		}
		else
		{
		#ifdef TIMING_DEBUG
			printf("face board serial port cannot write,Line:%d\n",__LINE__);
		#endif
		}
	}//other condition

	return SUCCESS;
}
int sot_jump_stage_control(markdata_t *mdt,unsigned char staid,sotpinfo_t *pinfo)
{
	if (NULL == mdt)
	{
	#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("Stage id is not in TimeconfigList,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
		
		if (stageid == cstageid)
		{//Controlling stageid is current stageid;
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("Controlling stageid is current stageid,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),pinfo->chan,0x02))
            {
            #ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
                printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
                *(mdt->fcdata->markbit) |= 0x0800;
                return FAIL;
            }
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
                                pinfo->chan,0x02,mdt->fcdata->markbit))
            {
            #ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
                printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
			if (!wait_write_serial(*(mdt->fcdata->sdserial)))
			{
				if (write(*(mdt->fcdata->sdserial),echo,sizeof(echo)) < 0)
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
				else
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("write success,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
			}
			else
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef TIMING_DEBUG
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
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("Controlling stageid is not current stageid,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			memset(pinfo,0,sizeof(sotpinfo_t));
			if (SUCCESS != sot_get_phase_info(mdt->fcdata,mdt->tscdata,*(mdt->rettl),num,pinfo))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				return FAIL;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
                                pinfo->chan,0x02,mdt->fcdata->markbit))
            {
            #ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
                printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
			if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),pinfo->chan,0x02))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						*(mdt->fcdata->markbit) |= 0x0800;
					}
					else
                    {
                    #ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
                        printf("write success,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                    }
				}
				else
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef TIMING_DEBUG
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
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("Controlling stageid is current stageid,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			if (!wait_write_serial(*(mdt->fcdata->sdserial)))
			{
				if (write(*(mdt->fcdata->sdserial),echo,sizeof(echo)) < 0)
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
				else
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("write success,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
			}
			else
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("face board serial port cannot write,Line:%d\n",__LINE__);
			#endif
			}
			return SUCCESS;
		}
		else
		{//Controlling stageid is not current stageid
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
			}
			else
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("face board serial port cannot write,Line:%d\n",__LINE__);
			#endif
			}
			if (pinfo->gftime > 0)
			{
				ngf = 0;
				while (1)
				{//green flash
					if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x03))
					{
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
						printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						*(mdt->fcdata->markbit) |= 0x0800;	
					}
					if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
									cchan,0x03,mdt->fcdata->markbit))
					{
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
						printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					to.tv_sec = 0;
					to.tv_usec = 500000;
					select(0,NULL,NULL,NULL,&to);

					if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x02))
					{
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
						printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						*(mdt->fcdata->markbit) |= 0x0800;
					}
					if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
									cchan,0x02,mdt->fcdata->markbit))
					{
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
			}//if (pinfo->gftime > 0)
			if (1 == pcexist)
			{
				//current phase begin to yellow lamp
				if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cnpchan,0x01))
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
									cnpchan,0x01, mdt->fcdata->markbit))
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cpchan,0x00))
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
													cpchan,0x00,mdt->fcdata->markbit))
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
			}
			else
			{
				if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x01))
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
										cchan,0x01,mdt->fcdata->markbit))
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
			}
			else
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("face board serial port cannot write,Line:%d\n",__LINE__);
			#endif
			}

			sleep(pinfo->ytime);

			if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x00))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
							cchan,0x00,mdt->fcdata->markbit))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						*(mdt->fcdata->markbit) |= 0x0800;
					}
				}
				else
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("face board serial port cannot write,Line:%d\n",__LINE__);
				#endif
				}
				sleep(pinfo->rtime);
			}

			memset(pinfo,0,sizeof(sotpinfo_t));
			if (SUCCESS != sot_get_phase_info(mdt->fcdata,mdt->tscdata,*(mdt->rettl),num,pinfo))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				return FAIL;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
                            pinfo->chan,0x02,mdt->fcdata->markbit))
            {
            #ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
                printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
			if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),pinfo->chan,0x02))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						*(mdt->fcdata->markbit) |= 0x0800;
					}
					else
					{
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
						printf("write success,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
				}
				else
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef TIMING_DEBUG
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
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						*(mdt->fcdata->markbit) |= 0x0800;
					}
				}
				else
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("face board serial port cannot write,Line:%d\n",__LINE__);
				#endif
				}	

				return SUCCESS;
			}//set success
		}//Controlling stageid is not current stageid
	}//other condition
}

#ifdef RED_FLASH
void sot_red_flash(void *arg)
{
	redflash_sot			*rdt = arg;	

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

	if (0 == (rdt->sot->td->timeconfig->TimeConfigList[rdt->tcline][rdt->slnum+1].StageId))
	{
		get_phase_id(rdt->sot->td->timeconfig->TimeConfigList[rdt->tcline][0].PhaseId,&nphaseid);
	}
	else
	{
		get_phase_id(rdt->sot->td->timeconfig->TimeConfigList[rdt->tcline][rdt->slnum+1].PhaseId,&nphaseid);
	}
	

	for (i = 0; i < (rdt->sot->td->channel->FactChannelNum); i++)
	{
		if (0 == (rdt->sot->td->channel->ChannelList[i].ChannelId))
			break;
		if (nphaseid == (rdt->sot->td->channel->ChannelList[i].ChannelCtrlSrc))
		{
		#ifdef CLOSE_LAMP
            tchan = rdt->sot->td->channel->ChannelList[i].ChannelId;
            if ((tchan >= 0x05) && (tchan <= 0x0c))
            {
                if (*(rdt->sot->fd->specfunc) & (0x01 << (tchan - 5)))
                    continue;
            }
        #else
            if ((*(rdt->sot->fd->specfunc) & 0x10) && (*(rdt->sot->fd->specfunc) & 0x20))
            {
                tchan = rdt->sot->td->channel->ChannelList[i].ChannelId;
                if (((5 <= tchan) && (tchan <= 8)) || ((9 <= tchan) && (tchan <= 12)))
                    continue;
            }
            if ((*(rdt->sot->fd->specfunc) & 0x10) && (!(*(rdt->sot->fd->specfunc) & 0x20)))
            {
                tchan = rdt->sot->td->channel->ChannelList[i].ChannelId;
                if ((5 <= tchan) && (tchan <= 8))
                    continue;
            }
            if ((*(rdt->sot->fd->specfunc) & 0x20) && (!(*(rdt->sot->fd->specfunc) & 0x10)))
            {
                tchan = rdt->sot->td->channel->ChannelList[i].ChannelId;
                if ((9 <= tchan) && (tchan <= 12))
                    continue;
            }
        #endif	
			nchan[j] = rdt->sot->td->channel->ChannelList[i].ChannelId;
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
	if ( (!(*(rdt->sot->fd->markbit) & 0x1000)) && (!(*(rdt->sot->fd->markbit) & 0x8000)) )
    {
        red_flash_begin_report(rdt->sot->fd->sockfd->csockfd,cchan);
    }
	while (1)
	{//while
		if (SUCCESS != sot_set_lamp_color(*(rdt->sot->fd->bbserial),cchan,0x03))
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}		
		if (SUCCESS!=update_channel_status(rdt->sot->fd->sockfd,rdt->sot->cs,cchan,0x03,rdt->sot->fd->markbit))
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}
		to.tv_sec = 0;
		to.tv_usec = 500000;
		select(0,NULL,NULL,NULL,&to);		

		if (SUCCESS != sot_set_lamp_color(*(rdt->sot->fd->bbserial),cchan,0x00))
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}		
		if (SUCCESS!=update_channel_status(rdt->sot->fd->sockfd,rdt->sot->cs,cchan,0x00,rdt->sot->fd->markbit))
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
	if ( (!(*(rdt->sot->fd->markbit) & 0x1000)) && (!(*(rdt->sot->fd->markbit) & 0x8000)) )
    {
        red_flash_end_report(rdt->sot->fd->sockfd->csockfd);
    }	

	pthread_detach(pthread_self());
    pthread_exit(NULL);
}
#endif


//mobile achieve jump phase control
int sot_mobile_jp_control(markdata_t *mdt,unsigned char staid,sotpinfo_t *pinfo,unsigned char *dirch)
{
	if ((NULL == mdt) || (NULL == pinfo) || (NULL == dirch))
	{
	#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("Stage id is not in TimeconfigList,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
		
		if (stageid == cstageid)
		{//Controlling stageid is current stageid;
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("Controlling stageid is current stageid,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),pinfo->chan,0x02))
            {
            #ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
                printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
                *(mdt->fcdata->markbit) |= 0x0800;
                return FAIL;
            }
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
                                pinfo->chan,0x02,mdt->fcdata->markbit))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("Controlling stageid is not current stageid,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			memset(pinfo,0,sizeof(sotpinfo_t));
			if (SUCCESS != sot_get_phase_info(mdt->fcdata,mdt->tscdata,*(mdt->rettl),num,pinfo))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				return FAIL;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
                                pinfo->chan,0x02,mdt->fcdata->markbit))
            {   
            #ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
                printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
			if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),pinfo->chan,0x02))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
		}
		else
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("face board serial port cannot write,Line:%d\n",__LINE__);
		#endif
		}
		if (pinfo->gftime > 0)
		{
			ngf = 0;
			while (1)
			{//green flash
				if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x03))
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;	
				}
				if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
								cchan,0x03,mdt->fcdata->markbit))
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				to.tv_sec = 0;
				to.tv_usec = 500000;
				select(0,NULL,NULL,NULL,&to);

				if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x02))
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
								cchan,0x02,mdt->fcdata->markbit))
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
		}//if (pinfo->gftime > 0)
		if (1 == pcexist)
		{
			//current phase begin to yellow lamp
			if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cnpchan,0x01))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
								cnpchan,0x01, mdt->fcdata->markbit))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
			if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cpchan,0x00))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
												cpchan,0x00,mdt->fcdata->markbit))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
		}
		else
		{
			if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x01))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
									cchan,0x01,mdt->fcdata->markbit))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
		}
		else
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("face board serial port cannot write,Line:%d\n",__LINE__);
		#endif
		}

		sleep(3);

		if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x00))
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			*(mdt->fcdata->markbit) |= 0x0800;
		}
		if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
						cchan,0x00,mdt->fcdata->markbit))
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}

		memset(pinfo,0,sizeof(sotpinfo_t));
		if (SUCCESS != sot_get_phase_info(mdt->fcdata,mdt->tscdata,*(mdt->rettl),num,pinfo))
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
		if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
						pinfo->chan,0x02,mdt->fcdata->markbit))
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}
		if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),pinfo->chan,0x02))
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
			}
			else
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("Controlling stageid is current stageid,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return SUCCESS;
		}
		else
		{//Controlling stageid is not current stageid
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
			}
			else
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("face board serial port cannot write,Line:%d\n",__LINE__);
			#endif
			}
			if (pinfo->gftime > 0)
			{
				ngf = 0;
				while (1)
				{//green flash
					if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x03))
					{
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
						printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						*(mdt->fcdata->markbit) |= 0x0800;	
					}
					if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
									cchan,0x03,mdt->fcdata->markbit))
					{
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
						printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
					to.tv_sec = 0;
					to.tv_usec = 500000;
					select(0,NULL,NULL,NULL,&to);

					if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x02))
					{
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
						printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						*(mdt->fcdata->markbit) |= 0x0800;
					}
					if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
									cchan,0x02,mdt->fcdata->markbit))
					{
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
			}//if (pinfo->gftime > 0)
			if (1 == pcexist)
			{
				//current phase begin to yellow lamp
				if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cnpchan,0x01))
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
									cnpchan,0x01, mdt->fcdata->markbit))
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cpchan,0x00))
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
													cpchan,0x00,mdt->fcdata->markbit))
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
			}
			else
			{
				if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x01))
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
										cchan,0x01,mdt->fcdata->markbit))
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
			}
			else
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("face board serial port cannot write,Line:%d\n",__LINE__);
			#endif
			}

			sleep(pinfo->ytime);

			if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x00))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
							cchan,0x00,mdt->fcdata->markbit))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						*(mdt->fcdata->markbit) |= 0x0800;
					}
				}
				else
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("face board serial port cannot write,Line:%d\n",__LINE__);
				#endif
				}
				sleep(pinfo->rtime);
			}

			memset(pinfo,0,sizeof(sotpinfo_t));
			if (SUCCESS != sot_get_phase_info(mdt->fcdata,mdt->tscdata,*(mdt->rettl),num,pinfo))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("get phase info err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				return FAIL;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
                            pinfo->chan,0x02,mdt->fcdata->markbit))
            {
            #ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
                printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
			if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),pinfo->chan,0x02))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
					#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						*(mdt->fcdata->markbit) |= 0x0800;
					}
				}
				else
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("face board serial port cannot write,Line:%d\n",__LINE__);
				#endif
				}	

				return SUCCESS;
			}//set success
		}//Controlling stageid is not current stageid
	}//other condition
}

//mobile achieve direct control
int sot_mobile_direct_control(markdata_t *mdt,sotpinfo_t *pinfo,unsigned char *dirch,unsigned char *fdirch)
{
	if ((NULL == mdt) || (NULL == pinfo) || (NULL == dirch))
	{
	#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
		
		if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),dirch,0x02))
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			*(mdt->fcdata->markbit) |= 0x0800;
			return FAIL;
		}
		if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
							dirch,0x02,mdt->fcdata->markbit))
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
		}
		else
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("face board serial port cannot write,Line:%d\n",__LINE__);
		#endif
		}
		if (pinfo->gftime > 0)
		{
			ngf = 0;
			while (1)
			{//green flash
				if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x03))
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;	
				}
				if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
								cchan,0x03,mdt->fcdata->markbit))
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				to.tv_sec = 0;
				to.tv_usec = 500000;
				select(0,NULL,NULL,NULL,&to);

				if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x02))
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
								cchan,0x02,mdt->fcdata->markbit))
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
		}//if (pinfo->gftime > 0)
		if (1 == pcexist)
		{
			//current phase begin to yellow lamp
			if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cnpchan,0x01))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
								cnpchan,0x01, mdt->fcdata->markbit))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
			if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cpchan,0x00))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
												cpchan,0x00,mdt->fcdata->markbit))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
		}
		else
		{
			if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x01))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
									cchan,0x01,mdt->fcdata->markbit))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
		}
		else
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("face board serial port cannot write,Line:%d\n",__LINE__);
		#endif
		}

		sleep(pinfo->ytime);

		if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x00))
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			*(mdt->fcdata->markbit) |= 0x0800;
		}
		if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
						cchan,0x00,mdt->fcdata->markbit))
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
			}
			else
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("face board serial port cannot write,Line:%d\n",__LINE__);
			#endif
			}
			sleep(pinfo->rtime);
		}
	
		if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
						dirch,0x02,mdt->fcdata->markbit))
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}
		if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),dirch,0x02))
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
			}
			else
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
		}
		else
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("face board serial port cannot write,Line:%d\n",__LINE__);
		#endif
		}
		if (pinfo->gftime > 0)
		{
			ngf = 0;
			while (1)
			{//green flash
				if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x03))
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;	
				}
				if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
								cchan,0x03,mdt->fcdata->markbit))
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				to.tv_sec = 0;
				to.tv_usec = 500000;
				select(0,NULL,NULL,NULL,&to);

				if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x02))
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
				if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
								cchan,0x02,mdt->fcdata->markbit))
				{
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
		}//if (pinfo->gftime > 0)
		if (1 == pcexist)
		{
			//current phase begin to yellow lamp
			if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cnpchan,0x01))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
								cnpchan,0x01, mdt->fcdata->markbit))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
			if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cpchan,0x00))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("set red lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
												cpchan,0x00,mdt->fcdata->markbit))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
		}
		else
		{
			if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x01))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("set yellow lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
			if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
									cchan,0x01,mdt->fcdata->markbit))
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(mdt->fcdata->markbit) |= 0x0800;
			}
		}
		else
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("face board serial port cannot write,Line:%d\n",__LINE__);
		#endif
		}

		sleep(3);

		if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),cchan,0x00))
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			*(mdt->fcdata->markbit) |= 0x0800;
		}
		if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
						cchan,0x00,mdt->fcdata->markbit))
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}
		
		if (SUCCESS != update_channel_status(mdt->fcdata->sockfd,mdt->chanstatus, \
						dirch,0x02,mdt->fcdata->markbit))
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}
		if (SUCCESS != sot_set_lamp_color(*(mdt->fcdata->bbserial),dirch,0x02))
		{
		#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
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
				#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(mdt->fcdata->markbit) |= 0x0800;
				}
			}
			else
			{
			#ifdef SYSTEM_OPTIMIZE_TIMING_DEBUG
				printf("face board serial port cannot write,Line:%d\n",__LINE__);
			#endif
			}	

			return SUCCESS;
		}//set success
	}//other condition
}
int sot_get_last_phaseinfo(fcdata_t *fd,tscdata_t *td,unsigned char tcline,sotpinfo_t *pinfo)
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
	memset(pinfo,0,sizeof(sotpinfo_t));
	sot_get_phase_info(fd,td,tcline,*(fd->slnum),pinfo);
	return SUCCESS;
}

