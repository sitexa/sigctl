/*
 * File:		sc.c
 * Author:		sk
 * Created on 20131112	
*/
#include "scs.h"
static unsigned char			scyes = 0;
static pthread_t				scid;
static unsigned char			yfyes = 0;
static pthread_t				yfid;
static unsigned char			ar = 0;
static unsigned char			ayf = 0;
static unsigned char			ac = 0;
static unsigned char			cmbak = 0;
static statusinfo_t             sinfo;
static unsigned char			netlockbak = 0;

void sc_end_child_thread()
{
	if (1 == scyes)
	{
		pthread_cancel(scid);
		pthread_join(scid,NULL);
		scyes = 0;
	}
	if (1 == yfyes)
	{
		pthread_cancel(yfid);
		pthread_join(yfid,NULL);
		yfyes = 0;
	}
}

void sc_end_part_child_thread()
{
	if (1 == yfyes)
    {
        pthread_cancel(yfid);
        pthread_join(yfid,NULL);
        yfyes = 0;
    }
}

void start_same_control(void *arg)
{
	scdata_t				*scdata = arg;
	unsigned char			data[4] = {0};
	struct timeval			timeout,ct,tel;
	unsigned char			i = 0;
	timedown_t				td;
	unsigned char			fbdata[6] = {0};
	unsigned char   		concyc = 0;
	unsigned char           cycend[16] = "CYTGCYCLEENDEND";

	unsigned char				sibuf[64] = {0};
//	statusinfo_t				sinfo;
	unsigned char               *csta = NULL;
    unsigned char               tcsta = 0;
//	memset(&sinfo,0,sizeof(statusinfo_t));
	sinfo.conmode = *(scdata->fd->contmode);
    sinfo.pattern = *(scdata->fd->patternid);
    sinfo.cyclet = 0;

	unsigned char           downti[8] = {0xA6,0xff,0xff,0xff,0xff,0x03,0,0xED};
	unsigned char           edownti[3] = {0xA5,0xA5,0xED};
	if (!wait_write_serial(*(scdata->fd->bbserial)))
    {
    	if (write(*(scdata->fd->bbserial),downti,sizeof(downti)) < 0)
        {
		#ifdef SAME_DEBUG
			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif       
        }
    }
    else
    {
    #ifdef SAME_DEBUG
    	printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
    }
    if (!wait_write_serial(*(scdata->fd->bbserial)))
    {
    	if (write(*(scdata->fd->bbserial),edownti,sizeof(edownti)) < 0)
        {
        #ifdef SAME_DEBUG
        	printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
        }
    }
    else
    {
    #ifdef SAME_DEBUG
    	printf("face board cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
    }

	cmbak = *(scdata->fd->contmode);
	fbdata[0] = 0xC1;
	fbdata[5] = 0xED;
	data[0] = 0xAA;
	data[1] = 0xFF;
	data[3] = 0xED;
	memset(&td,0,sizeof(td));
	fbdata[1] = 2;
	fbdata[2] = 0;
	fbdata[3] = 0x01;
	fbdata[4] = 0;
	sendfaceInfoToBoard(scdata->fd,fbdata);

	
	if (1 == *(scdata->fd->contmode))
	{//close lamp
	#ifdef SAME_DEBUG
		printf("Close all lamp,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		*(scdata->fd->color) = 0x03;
		data[2] = 0x03;
        if (!wait_write_serial(*(scdata->fd->bbserial)))
        {
            if (write(*(scdata->fd->bbserial),data,sizeof(data)) < 0)
            {
            #ifdef SAME_DEBUG
                printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
				*(scdata->fd->markbit) &= 0x0800;
            }
        }
        else
        {
        #ifdef SAME_DEBUG
            printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
        }
		//update channel status
        for (i = 0; i < MAX_CHANNEL_STATUS; i++)
        {
            scdata->cs->ChannelStatusList[i].ChannelStatusReds = 0;
            scdata->cs->ChannelStatusList[i].ChannelStatusYellows = 0;
            scdata->cs->ChannelStatusList[i].ChannelStatusGreens = 0;
        }
		//send data of channels to conf tool
		if ((!(*(scdata->fd->markbit) & 0x1000)) && (!(*(scdata->fd->markbit) & 0x8000)))
		{//if ((!(*(scdata->fd->markbit) & 0x1000)) && (!(*(scdata->fd->markbit) & 0x8000)))
			sinfo.time = 0;	
			sinfo.color = 0x04;
			sinfo.conmode = *(scdata->fd->contmode);//added on 20150529
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
			memcpy(scdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            memset(sibuf,0,sizeof(sibuf));
            if (SUCCESS != status_info_report(sibuf,&sinfo))
            {
            #ifdef SAME_DEBUG
                printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
            else
            {
               	write(*(scdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            }	
		}//if ((!(*(scdata->fd->markbit) & 0x1000)) && (!(*(scdata->fd->markbit) & 0x8000)))

        unsigned char acdata[8] = {'C','Y','T','F',0x03,'E','N','D'};
        if ((*(scdata->fd->markbit) & 0x0010) && (*(scdata->fd->markbit) & 0x0002))
        {
            if (write(*(scdata->fd->sockfd->ssockfd),acdata,sizeof(acdata)) < 0)
            {
            #ifdef SAME_DEBUG
                printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
            #endif
            }
			td.mode = 1;
        	td.pattern = *(scdata->fd->patternid);
        	td.lampcolor = 0x03;
        	td.lamptime = 0;
        	td.phaseid = 0;
        	td.stageline = 0;
        	if (SUCCESS != timedown_data_to_conftool(scdata->fd->sockfd,&td))
        	{
        	#ifdef SAME_DEBUG
            	printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
        	#endif
        	}
        }
		#ifdef EMBED_CONFIGURE_TOOL
		if (*(scdata->fd->markbit2) & 0x0200)
        {
            if (write(*(scdata->fd->sockfd->lhsockfd),acdata,sizeof(acdata)) < 0)
            {
            #ifdef SAME_DEBUG
                printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
            #endif
            }
            td.mode = 1;
            td.pattern = *(scdata->fd->patternid);
            td.lampcolor = 0x03;
            td.lamptime = 0;
            td.phaseid = 0;
            td.stageline = 0;
            if (SUCCESS != timedown_data_to_embed(scdata->fd->sockfd,&td))
            {
            #ifdef SAME_DEBUG
                printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
            #endif
            }
        }
		#endif

		ac = 1;
		ayf = 0;
		ar = 0;

		if (*(scdata->fd->contmode) < 27)
            fbdata[1] = *(scdata->fd->contmode) + 1;
        else
            fbdata[1] = *(scdata->fd->contmode);
		if ((30 == fbdata[1]) || (31 == fbdata[1]))
        {
            fbdata[2] = 0;
            fbdata[3] = 0;
            fbdata[4] = 0;
        }
        else
		{
			fbdata[2] = 0;
			fbdata[3] = 0x03;
			fbdata[4] = 0;
		}
		if (!wait_write_serial(*(scdata->fd->fbserial)))
		{
			if (write(*(scdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
			{
			#ifdef SAME_DEBUG
				printf("write err,File: %s,Line:%d\n",__FILE__,__LINE__);
			#endif
				*(scdata->fd->markbit) |= 0x0800;
				gettimeofday(&ct,NULL);
				update_event_list(scdata->fd->ec,scdata->fd->el,1,16,ct.tv_sec,scdata->fd->markbit);
                if(SUCCESS != generate_event_file(scdata->fd->ec,scdata->fd->el, \
										scdata->fd->softevent,scdata->fd->markbit))
                {
                #ifdef SAME_DEBUG
                    printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }	
			}
		}
		else
		{
		#ifdef SAME_DEBUG
			printf("face serial cannot write,File:%s,Line:%d\n",__FILE__,__LINE__);
		#endif
		}	
	}//close lamp
	else if (2 == *(scdata->fd->contmode))
	{//yellow flash
	#ifdef SAME_DEBUG
		printf("All yellow flash,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		unsigned char   yfdata[8] = {0};
        yfdata[0] = 'C';
        yfdata[1] = 'Y';
        yfdata[2] = 'T';
        yfdata[3] = 'F';
        yfdata[5] = 'E';
        yfdata[6] = 'N';
        yfdata[7] = 'D';
		ac = 0;
		ayf = 1;
		ar = 0;
	
		if (*(scdata->fd->contmode) < 27)
            fbdata[1] = *(scdata->fd->contmode) + 1;
        else
            fbdata[1] = *(scdata->fd->contmode);
		if ((30 == fbdata[1]) || (31 == fbdata[1]))
        {
            fbdata[2] = 0;
            fbdata[3] = 0;
            fbdata[4] = 0;
        }
        else
		{
       		fbdata[2] = 0;
       		fbdata[3] = 0;
       		fbdata[4] = 0;
		}
       	if (!wait_write_serial(*(scdata->fd->fbserial)))
       	{
           	if (write(*(scdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
           	{
           	#ifdef SAME_DEBUG
               	printf("write err,File: %s,Line:%d\n",__FILE__,__LINE__);
           	#endif
               	*(scdata->fd->markbit) |= 0x0800;
               	gettimeofday(&ct,NULL);
               	update_event_list(scdata->fd->ec,scdata->fd->el,1,16,ct.tv_sec,scdata->fd->markbit);
               	if(SUCCESS != generate_event_file(scdata->fd->ec,scdata->fd->el, \
										scdata->fd->softevent,scdata->fd->markbit))
                {
                #ifdef SAME_DEBUG
                   	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
            }
        }
        else
        {
        #ifdef SAME_DEBUG
           	printf("face serial cannot write,File:%s,Line:%d\n",__FILE__,__LINE__);
        #endif
        }

		if ((!(*(scdata->fd->markbit) & 0x1000)) && (!(*(scdata->fd->markbit) & 0x8000)))
		{//if ((!(*(scdata->fd->markbit) & 0x1000)) && (!(*(scdata->fd->markbit) & 0x8000)))
			sinfo.time = 0;	
			sinfo.color = 0x05;
			sinfo.conmode = *(scdata->fd->contmode);//added on 20150529
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
			memcpy(scdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            memset(sibuf,0,sizeof(sibuf));
            if (SUCCESS != status_info_report(sibuf,&sinfo))
            {
            #ifdef SAME_DEBUG
                printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
            else
            {
               	write(*(scdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            }	
		}//if ((!(*(scdata->fd->markbit) & 0x1000)) && (!(*(scdata->fd->markbit) & 0x8000)))

		*(scdata->fd->color) = 0x01;	
		while (1)
		{//while loop
			if (*(scdata->fd->markbit) & 0x0080)
			{//lamp err
				concyc += 1;
				if (1 == concyc)
				{//lamp err happen for the first time
				#ifdef SAME_DEBUG
					printf("First lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					unsigned char			recover[3] = {0xA4,0xA4,0xED};
					if (!wait_write_serial(*(scdata->fd->bbserial)))
					{
						if (write(*(scdata->fd->bbserial),recover,sizeof(recover)) < 0)
						{
						#ifdef SAME_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(scdata->fd->markbit) |= 0x0800;
							gettimeofday(&tel,NULL);
							update_event_list(scdata->fd->ec,scdata->fd->el,1,15, \
												tel.tv_sec,scdata->fd->markbit);
                            if (SUCCESS != generate_event_file(scdata->fd->ec,scdata->fd->el,\
																scdata->fd->softevent,scdata->fd->markbit))
                            {
                            #ifdef SAME_DEBUG
                            	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }
						}
						else
						{//write successfully;
							*(scdata->fd->markbit) &= 0xff7f;
							*(scdata->fd->contmode) = cmbak;	
						}//write successfully;
					}
					else
					{
					#ifdef SAME_DEBUG
						printf("serial cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
				}//lamp err happen for the first time
				if (2 == concyc)
				{
				#ifdef SAME_DEBUG
					printf("Continus two lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
			}//lamp err
			else
			{//not have lamp err
				concyc = 0;
			}//not have lamp err

			if (*(scdata->fd->markbit) & 0x0001)
            {//end time of current pattern has arrived
				unsigned char       syndata[3] = {0xCC,0xDD,0xED};
                if (!wait_write_serial(*(scdata->fd->synpipe)))
                {
                    if (write(*(scdata->fd->synpipe),syndata,sizeof(syndata)) < 0)
                    {
                    #ifdef SAME_DEBUG
                        printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
						sc_end_part_child_thread(); 
                    }   
                }
                else
                {
                #ifdef SAME_DEBUG
                    printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
                sleep(5); //wait main module end own
            }//end time of current pattern has arrived
#if 0
			//send end mark of cycle to configure tool  
            if (write(*(scdata->fd->sockfd->ssockfd),cycend,sizeof(cycend)) < 0)
            {
            #ifdef SAME_DEBUG
                printf("write error,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
#endif
			data[2] = 0x03;
			if (!wait_write_serial(*(scdata->fd->bbserial)))
			{
				if (write(*(scdata->fd->bbserial),data,sizeof(data)) < 0)
				{
				#ifdef SAME_DEBUG
					printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					*(scdata->fd->markbit) &= 0x0800;
				}
			}
			else
			{
			#ifdef SAME_DEBUG
				printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			}
			//update channel status
            for (i = 0; i < MAX_CHANNEL_STATUS; i++)
            {
                scdata->cs->ChannelStatusList[i].ChannelStatusReds = 0;
                scdata->cs->ChannelStatusList[i].ChannelStatusYellows = 0;
                scdata->cs->ChannelStatusList[i].ChannelStatusGreens = 0;
            }
			//send data of channels to conf tool
			if ((*(scdata->fd->markbit) & 0x0010) && (*(scdata->fd->markbit) & 0x0002))
        	{
				yfdata[4] = 0x03;
            	if (write(*(scdata->fd->sockfd->ssockfd),yfdata,sizeof(yfdata)) < 0)
            	{
            	#ifdef SAME_DEBUG
                	printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
            	#endif
            	}
				if ((*(scdata->fd->markbit2) & 0x0002) || (*(scdata->fd->markbit2) & 0x0004) || \
					(*(scdata->fd->markbit2) & 0x0010))
					td.mode = 28;
				else if (*(scdata->fd->markbit2) & 0x0008)
					td.mode = 29;
				else
					td.mode = 2;
            	td.pattern = *(scdata->fd->patternid);
            	td.lampcolor = 0x03;
            	td.lamptime = 0;
            	td.phaseid = 0;
            	td.stageline = 0;
            	if (SUCCESS != timedown_data_to_conftool(scdata->fd->sockfd,&td))
            	{
            	#ifdef SAME_DEBUG
                	printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
            	#endif
            	}
        	}
			#ifdef EMBED_CONFIGURE_TOOL
			if (*(scdata->fd->markbit2) & 0x0200)
        	{
				yfdata[4] = 0x03;
            	if (write(*(scdata->fd->sockfd->lhsockfd),yfdata,sizeof(yfdata)) < 0)
            	{
            	#ifdef SAME_DEBUG
                	printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
            	#endif
            	}
				if ((*(scdata->fd->markbit2) & 0x0002) || (*(scdata->fd->markbit2) & 0x0004) || \
					(*(scdata->fd->markbit2) & 0x0010))
					td.mode = 28;
				else if (*(scdata->fd->markbit2) & 0x0008)
					td.mode = 29;
				else
					td.mode = 2;
            	td.pattern = *(scdata->fd->patternid);
            	td.lampcolor = 0x03;
            	td.lamptime = 0;
            	td.phaseid = 0;
            	td.stageline = 0;
            	if (SUCCESS != timedown_data_to_embed(scdata->fd->sockfd,&td))
            	{
            	#ifdef SAME_DEBUG
                	printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
            	#endif
            	}
        	}
			#endif
#if 0
			if (*(scdata->fd->contmode) < 27)
                fbdata[1] = *(scdata->fd->contmode) + 1;
            else
                fbdata[1] = *(scdata->fd->contmode);
			if ((30 == fbdata[1]) || (31 == fbdata[1]))
            {
                fbdata[2] = 0;
                fbdata[3] = 0;
                fbdata[4] = 0;
            }
            else
			{
        		fbdata[2] = 0;
        		fbdata[3] = 0x03;
        		fbdata[4] = 0;
			}
        	if (!wait_write_serial(*(scdata->fd->fbserial)))
        	{
            	if (write(*(scdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
            	{
            	#ifdef SAME_DEBUG
                	printf("write err,File: %s,Line:%d\n",__FILE__,__LINE__);
            	#endif
                	*(scdata->fd->markbit) |= 0x0800;
                	gettimeofday(&ct,NULL);
                	update_event_list(scdata->fd->ec,scdata->fd->el,1,16,ct.tv_sec,scdata->fd->markbit);
                	if(SUCCESS != generate_event_file(scdata->fd->ec,scdata->fd->el,\
														scdata->fd->softevent,scdata->fd->markbit))
                	{
                	#ifdef SAME_DEBUG
                    	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                	}
            	}
        	}
        	else
        	{
        	#ifdef SAME_DEBUG
            	printf("face serial cannot write,File:%s,Line:%d\n",__FILE__,__LINE__);
        	#endif
        	}
#endif
			timeout.tv_sec = 0;
			timeout.tv_usec = 500000;
			select(0,NULL,NULL,NULL,&timeout);

			data[2] = 0x01;
			if (!wait_write_serial(*(scdata->fd->bbserial)))
            {
                if (write(*(scdata->fd->bbserial),data,sizeof(data)) < 0)
                {
                #ifdef SAME_DEBUG
                    printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
					*(scdata->fd->markbit) &= 0x0800;
                }
            }
            else
            {
            #ifdef SAME_DEBUG
                printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
			//update channel status
            for (i = 0; i < MAX_CHANNEL_STATUS; i++)
            {
                scdata->cs->ChannelStatusList[i].ChannelStatusReds = 0;
                scdata->cs->ChannelStatusList[i].ChannelStatusYellows = 0xff;
                scdata->cs->ChannelStatusList[i].ChannelStatusGreens = 0;
            }
			//send data of channels to conf tool
            if ((*(scdata->fd->markbit) & 0x0010) && (*(scdata->fd->markbit) & 0x0002))
            {
                yfdata[4] = 0x01;
                if (write(*(scdata->fd->sockfd->ssockfd),yfdata,sizeof(yfdata)) < 0)
                {
                #ifdef SAME_DEBUG
                    printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
                #endif
                }
				if ((*(scdata->fd->markbit2) & 0x0002) || (*(scdata->fd->markbit2) & 0x0004) || \
                    (*(scdata->fd->markbit2) & 0x0010))
                    td.mode = 28;
                else if (*(scdata->fd->markbit2) & 0x0008)
                    td.mode = 29;
                else
                    td.mode = 2;
            	td.pattern = *(scdata->fd->patternid);
            	td.lampcolor = 0x01;
            	td.lamptime = 0;
            	td.phaseid = 0;
            	td.stageline = 0;
            	if (SUCCESS != timedown_data_to_conftool(scdata->fd->sockfd,&td))
            	{
            	#ifdef SAME_DEBUG
                	printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
            	#endif
            	}
            }
			#ifdef EMBED_CONFIGURE_TOOL
			if (*(scdata->fd->markbit2) & 0x0200)
            {
                yfdata[4] = 0x01;
                if (write(*(scdata->fd->sockfd->lhsockfd),yfdata,sizeof(yfdata)) < 0)
                {
                #ifdef SAME_DEBUG
                    printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
                #endif
                }
				if ((*(scdata->fd->markbit2) & 0x0002) || (*(scdata->fd->markbit2) & 0x0004) || \
                    (*(scdata->fd->markbit2) & 0x0010))
                    td.mode = 28;
                else if (*(scdata->fd->markbit2) & 0x0008)
                    td.mode = 29;
                else
                    td.mode = 2;
            	td.pattern = *(scdata->fd->patternid);
            	td.lampcolor = 0x01;
            	td.lamptime = 0;
            	td.phaseid = 0;
            	td.stageline = 0;
            	if (SUCCESS != timedown_data_to_embed(scdata->fd->sockfd,&td))
            	{
            	#ifdef SAME_DEBUG
                	printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
            	#endif
            	}
            }
			#endif
#if 0
			if (*(scdata->fd->contmode) < 27)
				fbdata[1] = *(scdata->fd->contmode) + 1;
			else
				fbdata[1] = *(scdata->fd->contmode);
			if ((30 == fbdata[1]) || (31 == fbdata[1]))
			{
				fbdata[2] = 0;
				fbdata[3] = 0;
				fbdata[4] = 0;
			}
			else
			{
        		fbdata[2] = 0;
        		fbdata[3] = 0x01;
        		fbdata[4] = 0;
			}
        	if (!wait_write_serial(*(scdata->fd->fbserial)))
        	{
            	if (write(*(scdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
            	{
            	#ifdef SAME_DEBUG
                	printf("write err,File: %s,Line:%d\n",__FILE__,__LINE__);
            	#endif
                	*(scdata->fd->markbit) |= 0x0800;
                	gettimeofday(&ct,NULL);
                	update_event_list(scdata->fd->ec,scdata->fd->el,1,16,ct.tv_sec,scdata->fd->markbit);
                	if(SUCCESS != generate_event_file(scdata->fd->ec,scdata->fd->el,\
														scdata->fd->softevent,scdata->fd->markbit))
                	{
                	#ifdef SAME_DEBUG
                    	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                	}
            	}
        	}
        	else
        	{
        	#ifdef SAME_DEBUG
            	printf("face serial cannot write,File:%s,Line:%d\n",__FILE__,__LINE__);
        	#endif
        	}
#endif
			timeout.tv_sec = 0;
            timeout.tv_usec = 500000;
            select(0,NULL,NULL,NULL,&timeout);
		}//while loop
	}//yellow flash
	else if (3 == *(scdata->fd->contmode))
	{//all red
	#ifdef SAME_DEBUG
		printf("All red,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		*(scdata->fd->color) = 0x00;
		data[2] = 0x00;
        if (!wait_write_serial(*(scdata->fd->bbserial)))
        {
            if (write(*(scdata->fd->bbserial),data,sizeof(data)) < 0)
            {
            #ifdef SAME_DEBUG
                printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
				*(scdata->fd->markbit) &= 0x0800;
            }
        }
        else
        {
        #ifdef SAME_DEBUG
            printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
        }
		//update channel status
        for (i = 0; i < MAX_CHANNEL_STATUS; i++)
        {
            scdata->cs->ChannelStatusList[i].ChannelStatusReds = 0xff;
            scdata->cs->ChannelStatusList[i].ChannelStatusYellows = 0;
            scdata->cs->ChannelStatusList[i].ChannelStatusGreens = 0;
        }
		
		if ((!(*(scdata->fd->markbit) & 0x1000)) && (!(*(scdata->fd->markbit) & 0x8000)))
		{//if ((!(*(scdata->fd->markbit) & 0x1000)) && (!(*(scdata->fd->markbit) & 0x8000)))
			sinfo.time = 0;	
			sinfo.color = 0x00;
			sinfo.conmode = *(scdata->fd->contmode);//added on 20150529
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
			memcpy(scdata->fd->sinfo,&sinfo,sizeof(statusinfo_t));
            if (SUCCESS != status_info_report(sibuf,&sinfo))
            {
            #ifdef SAME_DEBUG
                printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
            }
            else
            {
               	write(*(scdata->fd->sockfd->csockfd),sibuf,sizeof(sibuf));
            }	
		}//if ((!(*(scdata->fd->markbit) & 0x1000)) && (!(*(scdata->fd->markbit) & 0x8000)))

		//send data of channels to conf tool
		unsigned char ardata[8] = {'C','Y','T','F',0x00,'E','N','D'};
		if ((*(scdata->fd->markbit) & 0x0010) && (*(scdata->fd->markbit) & 0x0002))
    	{
        	if (write(*(scdata->fd->sockfd->ssockfd),ardata,sizeof(ardata)) < 0)
        	{
        	#ifdef SAME_DEBUG
            	printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
        	#endif
        	}
			td.mode = 3;
        	td.pattern = *(scdata->fd->patternid);
        	td.lampcolor = 0x00;
        	td.lamptime = 0;
        	td.phaseid = 0;
        	td.stageline = 0;
        	if (SUCCESS != timedown_data_to_conftool(scdata->fd->sockfd,&td))
        	{
        	#ifdef SAME_DEBUG
            	printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
        	#endif
        	}
    	}
		#ifdef EMBED_CONFIGURE_TOOL
		if (*(scdata->fd->markbit2) & 0x0200)
		{
			if (write(*(scdata->fd->sockfd->lhsockfd),ardata,sizeof(ardata)) < 0)
            {
            #ifdef SAME_DEBUG
                printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
            #endif
            }
            td.mode = 3;
            td.pattern = *(scdata->fd->patternid);
            td.lampcolor = 0x00;
            td.lamptime = 0;
            td.phaseid = 0;
            td.stageline = 0;
            if (SUCCESS != timedown_data_to_embed(scdata->fd->sockfd,&td))
            {
            #ifdef SAME_DEBUG
                printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
            #endif
            }
		}
		#endif

		ac = 0;
		ayf = 0;
		ar = 1;
		if (*(scdata->fd->contmode) < 27)
			fbdata[1] = *(scdata->fd->contmode) + 1;
		else
			fbdata[1] = *(scdata->fd->contmode);
		if ((30 == fbdata[1]) || (31 == fbdata[1]))
        {
            fbdata[2] = 0;
            fbdata[3] = 0;
            fbdata[4] = 0;
        }
        else
		{
        	fbdata[2] = 0;
        	fbdata[3] = 0x00;
        	fbdata[4] = 0;
		}
        if (!wait_write_serial(*(scdata->fd->fbserial)))
        {
            if (write(*(scdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
            {
            #ifdef SAME_DEBUG
                printf("write err,File: %s,Line:%d\n",__FILE__,__LINE__);
            #endif
                *(scdata->fd->markbit) |= 0x0800;
                gettimeofday(&ct,NULL);
                update_event_list(scdata->fd->ec,scdata->fd->el,1,16,ct.tv_sec,scdata->fd->markbit);
                if(SUCCESS != generate_event_file(scdata->fd->ec,scdata->fd->el, \
									scdata->fd->softevent,scdata->fd->markbit))
                {
                #ifdef SAME_DEBUG
                    printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
            }
        }
        else
        {
        #ifdef SAME_DEBUG
            printf("face serial cannot write,File:%s,Line:%d\n",__FILE__,__LINE__);
        #endif
        }
	}//all red	

	if ((1 == *(scdata->fd->contmode)) || (3 == *(scdata->fd->contmode)))
	{//monitor request, Note:yellow flash has done request in its interior;
		while (1)
		{//while loop
			sleep(1);
			
			if (*(scdata->fd->markbit) & 0x0080)
			{//lamp err
				concyc += 1;
				if (1 == concyc)
				{//lamp err happen for the first time
				#ifdef SAME_DEBUG
					printf("First lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					unsigned char			recover[3] = {0xA4,0xA4,0xED};
					if (!wait_write_serial(*(scdata->fd->bbserial)))
					{
						if (write(*(scdata->fd->bbserial),recover,sizeof(recover)) < 0)
						{
						#ifdef SAME_DEBUG
							printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
						#endif
							*(scdata->fd->markbit) |= 0x0800;
							gettimeofday(&tel,NULL);
							update_event_list(scdata->fd->ec,scdata->fd->el,1,15, \
												tel.tv_sec,scdata->fd->markbit);
                            if (SUCCESS != generate_event_file(scdata->fd->ec,scdata->fd->el,\
																scdata->fd->softevent,scdata->fd->markbit))
                            {
                            #ifdef SAME_DEBUG
                            	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }
						}
						else
						{//write successfully;
							*(scdata->fd->markbit) &= 0xff7f;
							*(scdata->fd->contmode) = cmbak;	
						}//write successfully;
					}
					else
					{
					#ifdef SAME_DEBUG
						printf("serial cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
					}
				}//lamp err happen for the first time
				if (2 == concyc)
				{
				#ifdef SAME_DEBUG
					printf("Continus two lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
			}//lamp err
			else
			{//not have lamp err
				concyc = 0;
			}//not have lamp err
	

			if (*(scdata->fd->markbit) & 0x0001)
			{//end time of current pattern has arrived
				unsigned char		syndata[3] = {0xCC,0xDD,0xED};
				if (!wait_write_serial(*(scdata->fd->synpipe)))
				{
					if (write(*(scdata->fd->synpipe),syndata,sizeof(syndata)) < 0)
					{
					#ifdef SAME_DEBUG
						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						sc_end_part_child_thread();
					}
				}
				else
				{
				#ifdef SAME_DEBUG
					printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
				sleep(5); //wait main module end own
			}//end time of current pattern has arrived	
		}//while loop
	}//monitor request, Note:yellow flash has done request in its interior;

	pthread_exit(NULL);
}

void sc_yellow_flash(void *arg)
{
	scyfdata_t		*scyfdata = arg;
	unsigned char   yfdata[8] = {0};
    yfdata[0] = 'C';
    yfdata[1] = 'Y';
    yfdata[2] = 'T';
    yfdata[3] = 'F';
    yfdata[5] = 'E';
    yfdata[6] = 'N';
    yfdata[7] = 'D';
	unsigned char	data[4] = {0};
	data[0] = 0xAA;
    data[1] = 0xFF;
    data[3] = 0xED;
	struct timeval		to,ct;
	unsigned char		i = 0;
	timedown_t			td;
	unsigned char		fbdata[6] = {0};

	fbdata[0] = 0xC1;
	fbdata[5] = 0xED;
	memset(&td,0,sizeof(td));
	while (1)
	{//while loop
		data[2] = 0x03;
		if (!wait_write_serial(*(scyfdata->fd->bbserial)))
		{
			if (write(*(scyfdata->fd->bbserial),data,sizeof(data)) < 0)
			{
			#ifdef SAME_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(scyfdata->fd->markbit) &= 0x0800;
			}
		}
		else
		{
		#ifdef SAME_DEBUG
			printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}
		//update channel status
        for (i = 0; i < MAX_CHANNEL_STATUS; i++)
        {
            scyfdata->cs->ChannelStatusList[i].ChannelStatusReds = 0;
            scyfdata->cs->ChannelStatusList[i].ChannelStatusYellows = 0;
            scyfdata->cs->ChannelStatusList[i].ChannelStatusGreens = 0;
        }
		//send data of channels to conf tool
		if ((*(scyfdata->fd->markbit) & 0x0010) && (*(scyfdata->fd->markbit) & 0x0002))
        {
			yfdata[4] = 0x03;
           	if (write(*(scyfdata->fd->sockfd->ssockfd),yfdata,sizeof(yfdata)) < 0)
            {
            #ifdef SAME_DEBUG
               	printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
            #endif
            }

			if (0 == netlockbak)
				td.mode = 28;
			else
				td.mode = 29;
        	td.pattern = *(scyfdata->fd->patternid);
        	td.lampcolor = 0x03;
        	td.lamptime = 0;
        	td.phaseid = 0;
        	td.stageline = 0;
        	if (SUCCESS != timedown_data_to_conftool(scyfdata->fd->sockfd,&td))
        	{
        	#ifdef SAME_DEBUG
            	printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
        	#endif
        	}
        }
		#ifdef EMBED_CONFIGURE_TOOL
		if (*(scyfdata->fd->markbit2) & 0x0200)
		{
			yfdata[4] = 0x03;
            if (write(*(scyfdata->fd->sockfd->lhsockfd),yfdata,sizeof(yfdata)) < 0)
            {
            #ifdef SAME_DEBUG
                printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
            #endif
            }

            if (0 == netlockbak)
                td.mode = 28;
            else
                td.mode = 29;
            td.pattern = *(scyfdata->fd->patternid);
            td.lampcolor = 0x03;
            td.lamptime = 0;
            td.phaseid = 0;
            td.stageline = 0;
            if (SUCCESS != timedown_data_to_embed(scyfdata->fd->sockfd,&td))
            {
            #ifdef SAME_DEBUG
                printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
            #endif
            }	
		}//if (*(scyfdata->fd->markbit2) & 0x0200)
		#endif
#if 0
		fbdata[1] = 28;
        fbdata[2] = 0;
        fbdata[3] = 0x03;
        fbdata[4] = 0;
        if (!wait_write_serial(*(scyfdata->fd->fbserial)))
        {
            if (write(*(scyfdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
            {
            #ifdef SAME_DEBUG
                printf("write err,File: %s,Line:%d\n",__FILE__,__LINE__);
            #endif
                *(scyfdata->fd->markbit) |= 0x0800;
                gettimeofday(&ct,NULL);
                update_event_list(scyfdata->fd->ec,scyfdata->fd->el,1,16,ct.tv_sec,scyfdata->fd->markbit);
                if(SUCCESS != generate_event_file(scyfdata->fd->ec,scyfdata->fd->el,\
													scyfdata->fd->softevent,scyfdata->fd->markbit))
                {
                #ifdef SAME_DEBUG
                    printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
            }
        }
        else
        {
        #ifdef SAME_DEBUG
            printf("face serial cannot write,File:%s,Line:%d\n",__FILE__,__LINE__);
        #endif
        }
#endif
		to.tv_sec = 0;
		to.tv_usec = 500000;
		select(0,NULL,NULL,NULL,&to);

		data[2] = 0x01;
		if (!wait_write_serial(*(scyfdata->fd->bbserial)))
        {
            if (write(*(scyfdata->fd->bbserial),data,sizeof(data)) < 0)
            {
            #ifdef SAME_DEBUG
                printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
				*(scyfdata->fd->markbit) &= 0x0800;
            }
        }
        else
        {
        #ifdef SAME_DEBUG
            printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
        }
		//update channel status
        for (i = 0; i < MAX_CHANNEL_STATUS; i++)
        {
            scyfdata->cs->ChannelStatusList[i].ChannelStatusReds = 0;
            scyfdata->cs->ChannelStatusList[i].ChannelStatusYellows = 0xff;
            scyfdata->cs->ChannelStatusList[i].ChannelStatusGreens = 0;
        }
		//send data of channels to conf tool
        if ((*(scyfdata->fd->markbit) & 0x0010) && (*(scyfdata->fd->markbit) & 0x0002))
        {
            yfdata[4] = 0x01;
            if (write(*(scyfdata->fd->sockfd->ssockfd),yfdata,sizeof(yfdata)) < 0)
            {
            #ifdef SAME_DEBUG
                printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
            #endif
            }
			if (0 == netlockbak)
				td.mode = 28;
			else
				td.mode = 29;
        	td.pattern = *(scyfdata->fd->patternid);
        	td.lampcolor = 0x01;
        	td.lamptime = 0;
        	td.phaseid = 0;
        	td.stageline = 0;
        	if (SUCCESS != timedown_data_to_conftool(scyfdata->fd->sockfd,&td))
        	{
        	#ifdef SAME_DEBUG
            	printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
        	#endif
        	}
        }
		#ifdef EMBED_CONFIGURE_TOOL
		if (*(scyfdata->fd->markbit2) & 0x0200)
		{
			yfdata[4] = 0x01;
			if (write(*(scyfdata->fd->sockfd->lhsockfd),yfdata,sizeof(yfdata)) < 0)
			{
			#ifdef SAME_DEBUG
				printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}

			if (0 == netlockbak)
				td.mode = 28;
			else
				td.mode = 29;
			td.pattern = *(scyfdata->fd->patternid);
			td.lampcolor = 0x03;
			td.lamptime = 0;
			td.phaseid = 0;
			td.stageline = 0;
			if (SUCCESS != timedown_data_to_embed(scyfdata->fd->sockfd,&td))
			{
			#ifdef SAME_DEBUG
				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
			}	
		}//if (*(scyfdata->fd->markbit2) & 0x0200)
		#endif
#if 0
		fbdata[1] = 28;
        fbdata[2] = 0;
        fbdata[3] = 0x03;
        fbdata[4] = 0;
        if (!wait_write_serial(*(scyfdata->fd->fbserial)))
        {
            if (write(*(scyfdata->fd->fbserial),fbdata,sizeof(fbdata)) < 0)
            {
            #ifdef SAME_DEBUG
                printf("write err,File: %s,Line:%d\n",__FILE__,__LINE__);
            #endif
                *(scyfdata->fd->markbit) |= 0x0800;
                gettimeofday(&ct,NULL);
                update_event_list(scyfdata->fd->ec,scyfdata->fd->el,1,16,ct.tv_sec,scyfdata->fd->markbit);
                if(SUCCESS != generate_event_file(scyfdata->fd->ec,scyfdata->fd->el,\
													scyfdata->fd->softevent,scyfdata->fd->markbit))
                {
                #ifdef SAME_DEBUG
                    printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                }
            }
        }
        else
        {
        #ifdef SAME_DEBUG
            printf("face serial cannot write,File:%s,Line:%d\n",__FILE__,__LINE__);
        #endif
        }
#endif
		to.tv_sec = 0;
        to.tv_usec = 500000;
        select(0,NULL,NULL,NULL,&to);
	}//while loop

	pthread_exit(NULL);
}

int same_control(fcdata_t *fcdata,tscdata_t *tscdata,ChannelStatus_t *chanstatus)
{
	if ((NULL == fcdata) || (NULL == tscdata) ||(NULL== chanstatus))
	{
	#ifdef SAME_DEBUG
		printf("pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
		output_log("same control,pointer address is null");
	#endif
		return MEMERR;
	}
	scdata_t			scdata;
	//initial static variable
	scyes = 0;
	yfyes = 0;
	ar = 0;
	ayf = 0;
	ac = 0;
	cmbak = 0;
	netlockbak = 0;
	memset(&sinfo,0,sizeof(statusinfo_t));
	*(fcdata->markbit) &= 0xfbff;
	memset(fcdata->sinfo,0,sizeof(statusinfo_t));
	//end initial static variable

	if (0 == scyes)
	{
		memset(&scdata,0,sizeof(scdata));
		scdata.fd = fcdata;
		scdata.td = tscdata;
		scdata.cs = chanstatus;
		int scret = pthread_create(&scid,NULL,(void *)start_same_control,&scdata);
		if (0 != scret)
		{
		#ifdef SAME_DEBUG
			printf("create thread error,File: %s,Line: %d\n",__FILE__,__LINE__);
			output_log("same control,create same control thread err");
		#endif
			return FAIL;
		}
		scyes = 1;
	}

	sleep(1);
	
	fd_set					nread;
	unsigned char			scbuf[32] = {0};
	unsigned char			keylock = 0;
	unsigned char			wllock = 0;
	unsigned char			netlock = 0;
	scyfdata_t				scyfdata;
	unsigned char			i = 0,j = 0,z = 0,k = 0,s = 0;
	timedown_t				td;
	struct timeval			ct,to;
	unsigned char			fbdata[6] = {0};
	fbdata[0] = 0xC1;
	fbdata[5] = 0xED;
	yfdata_t                ardata;
	memset(&ardata,0,sizeof(ardata));
    ardata.second = 0;
    ardata.markbit = fcdata->markbit;
	ardata.markbit2 = fcdata->markbit2;
    ardata.serial = *(fcdata->bbserial);
    ardata.sockfd = fcdata->sockfd;
    ardata.cs = chanstatus;	
	yfdata_t                acdata;
    memset(&acdata,0,sizeof(acdata));
    acdata.second = 0;
    acdata.markbit = fcdata->markbit;
	acdata.markbit2 = fcdata->markbit2;
    acdata.serial = *(fcdata->bbserial);
    acdata.sockfd = fcdata->sockfd;
    acdata.cs = chanstatus;
	unsigned char               *csta = NULL;
    unsigned char               tcsta = 0;
	unsigned char               sibuf[64] = {0};
	unsigned short          factovs = 0;
    unsigned char           cbuf[1024] = {0};
	infotype_t              itype;
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
	unsigned char			pex = 0;
	unsigned char			fpc = 0;
	unsigned char			cp = 0;
	unsigned char           cc[32] = {0};//channels of current phase;
    unsigned char           nc[32] = {0};//channels of next phase;
    unsigned char           wcc[32] = {0};//channels of will change;
    unsigned char           wnpcc[32] = {0};//not person channels of will change
    unsigned char           wpcc[32] = {0};//person channels of will change
	unsigned char           *pcc = NULL;
	unsigned char			pce = 0;
	unsigned char			ce = 0;
	timedown_t              sctd;
	unsigned char			ngf = 0;

	unsigned char			wtlock = 0;
	struct timeval			wtltime;
	unsigned char			pantn = 0;
	unsigned char          	dirc = 0; //direct control
	//channes of eight direction mapping;
    unsigned char    	dirch[8][5] = {{0x01,0x03,0x09,0x0b,0x00},{0x05,0x07,0x09,0x0b,0x00},
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

	unsigned char			ccon[32] = {0};

	while (1)
	{//while loop
	#ifdef SAME_DEBUG
		printf("Reading pipe,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		FD_ZERO(&nread);
		FD_SET(*(fcdata->conpipe),&nread);
		wtltime.tv_sec = RFIDT;
		wtltime.tv_usec = 0;
		int max = *(fcdata->conpipe);
		int cpret = select(max+1,&nread,NULL,NULL,&wtltime);
		if (cpret < 0)
		{
		#ifdef SAME_DEBUG
			printf("select call err,File: %s,Line: %d\n",__FILE__,__LINE__);
			output_log("same control,select call err");
		#endif
			sc_end_part_child_thread();
			return FAIL;	
		}
		if (0 == cpret)
		{//time out
			if (*(fcdata->markbit2) & 0x0100)
                continue; //rfid is controlling
			*(fcdata->markbit2) &= 0xfffe;
			if (1 == wtlock)
            {//if (1 == wtlock)
            //    pantn += 1;
            //    if (3 == pantn)
           //     {//wireless terminal has disconnected with signaler machine;
				if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
				{
					memset(wlinfo,0,sizeof(wlinfo));
					gettimeofday(&ct,NULL);
					if (SUCCESS != wireless_info_report(wlinfo,ct.tv_sec,fcdata->wlmark,0x20))
					{
					#ifdef SAME_DEBUG
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
				ar = 0;
				ac = 0;
				ayf = 0;
				fpc = 0;
				dirc = 0;
				cp = 0;
				pantn = 0;
				if (1 == yfyes)
				{
					pthread_cancel(yfid);
					pthread_join(yfid,NULL);
					yfyes = 0;
				}
				*(fcdata->markbit2) &= 0xfffb;	
				if (0 == scyes)
				{
					memset(&scdata,0,sizeof(scdata));
					scdata.fd = fcdata;
					scdata.td = tscdata;
					scdata.cs = chanstatus;
					int scret = pthread_create(&scid,NULL,(void *)start_same_control,&scdata);
					if (0 != scret)
					{
					#ifdef SAME_DEBUG
						printf("create thread error,File: %s,Line: %d\n",__FILE__,__LINE__);
						output_log("same control,create same control thread err");
					#endif
						return FAIL;
					}
					scyes = 1;
				}		
				continue;
          //      }//wireless terminal has disconnected with signaler machine;
            }//if (1 == wtlock)
            continue;
		}//time out
		if (cpret > 0)
		{//cpret > 0
			if (FD_ISSET(*(fcdata->conpipe),&nread))
			{
				memset(scbuf,0,sizeof(scbuf));
				read(*(fcdata->conpipe),scbuf,sizeof(scbuf));
				if ((0xB9==scbuf[0]) && ((0xED==scbuf[8]) || (0xED==scbuf[4]) || (0xED==scbuf[5])))
                {//wireless terminal control
					pantn = 0;
                    if (0x02 == scbuf[3])
                    {//pant
                        continue;
                    }//pant
                    if ((0 == wllock) && (0 == keylock) && (0 == netlock))
                    {//terminal control is valid only when wireless and key and net control is invalid;
						if (0x04 == scbuf[3])
                        {//control function
                            if ((0x01 == scbuf[4]) && (0 == wtlock))
                            {//control will happen
								*(fcdata->markbit2) |= 0x0004;
								wtlock = 1;

								tpp.func[0] = scbuf[4];
                                memset(terbuf,0,sizeof(terbuf));
                                if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
                                {
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {
                                        if (write(*(fcdata->bbserial),terbuf,sizeof(terbuf)) < 0)
                                        {
                                        #ifdef SAME_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef SAME_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									sinfo.conmode = 28;
									if (1 == *(fcdata->contmode))
										sinfo.color = 0x04;
									if (2 == *(fcdata->contmode))
										sinfo.color = 0x05;
									if (3 == *(fcdata->contmode))
										sinfo.color = 0;
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
            						#ifdef SAME_DEBUG
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
                                    #ifdef SAME_DEBUG
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
                                fbdata[2] = 0;
                                fbdata[3] = 0;
                                fbdata[4] = 0;
                                if (!wait_write_serial(*(fcdata->fbserial)))
                                {
                                    if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                    {
                                    #ifdef SAME_DEBUG
                                        printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                                else
                                {
                                #ifdef SAME_DEBUG
                                    printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                #endif
                                }

								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                                {
                                #ifdef SAME_DEBUG
                                    printf("send control info to configure tool,File: %s,Line: %d\n", \
                                            __FILE__,__LINE__);
                                #endif
                                    memset(&sctd,0,sizeof(sctd));
                                    sctd.mode = 28;
                                    sctd.pattern = *(fcdata->patternid);
                                    sctd.lampcolor = 0;
                                    sctd.lamptime = 0;
                                    sctd.phaseid = 0;
                                    sctd.stageline = 0;
                                    if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&sctd))
                                    {
                                    #ifdef SAME_DEBUG
                                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
								#ifdef EMBED_CONFIGURE_TOOL
                                if (*(fcdata->markbit2) & 0x0200)
                                {
                                    memset(&sctd,0,sizeof(sctd));
                                    sctd.mode = 28;
                                    sctd.pattern = *(fcdata->patternid);
                                    sctd.lampcolor = 0;
                                    sctd.lamptime = 0;
                                    sctd.phaseid = 0;
                                    sctd.stageline = 0; 
                                    if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&sctd))
                                    {
                                    #ifdef SAME_DEBUG
                                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                                #endif
								continue;
                            }//control will happen
                            else if ((0x00 == scbuf[4]) && (1 == wtlock))
                            {//cancel control
								if (1 == yfyes)
								{
									pthread_cancel(yfid);
									pthread_join(yfid,NULL);
									yfyes = 0;
								}
								fpc = 0;
								cp = 0;
								ac = 0;
								ar = 0;
								ayf = 0;
								dirc = 0;
								wtlock = 0;
								*(fcdata->markbit2) &= 0xfffb;

								tpp.func[0] = scbuf[4];
                                memset(terbuf,0,sizeof(terbuf));
                                if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
                                {   
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {   
                                        if (write(*(fcdata->bbserial),terbuf,sizeof(terbuf)) < 0)
                                        {
                                        #ifdef SAME_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef SAME_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
								if (1 == scyes)
								{
									pthread_cancel(scid);
									pthread_join(scid,NULL);
									scyes = 0;
								}
								if (0 == scyes)
    							{
        							memset(&scdata,0,sizeof(scdata));
        							scdata.fd = fcdata;
        							scdata.td = tscdata;
        							scdata.cs = chanstatus;
        							int scret = pthread_create(&scid,NULL,(void *)start_same_control,&scdata);
        							if (0 != scret)
        							{
        							#ifdef SAME_DEBUG
            							printf("create thread error,File: %s,Line: %d\n",__FILE__,__LINE__);
										output_log("same control,create same control thread err");
        							#endif
            							return FAIL;
        							}
        							scyes = 1;
    							}
								if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
								{
									memset(wlinfo,0,sizeof(wlinfo));
                                    gettimeofday(&ct,NULL);
                                    if (SUCCESS != wireless_info_report(wlinfo,ct.tv_sec,fcdata->wlmark,0x06))
                                    {
                                    #ifdef SAME_DEBUG
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
                            else if (((0x05 <= scbuf[4]) && (scbuf[4] <= 0x0c)) && (1 == wtlock))
                            {//direction control
								tpp.func[0] = scbuf[4];
                                memset(terbuf,0,sizeof(terbuf));
                                if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
                                {   
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {   
                                        if (write(*(fcdata->bbserial),terbuf,sizeof(terbuf)) < 0)
                                        {
                                        #ifdef SAME_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef SAME_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))

								if ((1 == ar) || (1 == ayf) || (1 == ac))
                            	{//if ((1 == ar) || (1 == ayf) || (1 == ac))
									dirc = scbuf[4];
									if ((dirc < 5) && (dirc > 0x0c))
                                    {
                                        continue;
                                    }

									fpc = 1;
									cp = 0;
                                	if (1 == yfyes)
                                	{
                                    	pthread_cancel(yfid);
                                    	pthread_join(yfid,NULL);
                                    	yfyes = 0;
                                	}
									if (1 == scyes)
									{
										pthread_cancel(scid);
                                        pthread_join(scid,NULL);
                                        scyes = 0;
									}
									if (1 != ar)
									{
										new_all_red(&ardata);
									}
									ar = 0;
                                    ac = 0;
									ayf = 0;

                                	if (SUCCESS != sc_set_lamp_color(*(fcdata->bbserial),dirch[dirc-5],0x02))
                                	{
                                	#ifdef SAME_DEBUG
                                    	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
                                    	*(fcdata->markbit) |= 0x0800;
                                	}
                                	if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                                        				dirch[dirc-5],0x02,fcdata->markbit))
                                	{
                                	#ifdef SAME_DEBUG
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
            							#ifdef SAME_DEBUG
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
									if (0x05 == scbuf[4])
									{
										dce = 0x10;
										val = 52;
									}
									else if (0x06 == scbuf[4])
									{
										dce = 0x12;
										val = 54;
									}
									else if (0x07 == scbuf[4])
									{
										dce = 0x14;
										val = 56;
									}
									else if (0x08 == scbuf[4])
									{
                                    	dce = 0x16;
										val = 58;
									}
									else if (0x09 == scbuf[4])
									{
                                    	dce = 0x18;
										val = 60;
									}
									else if (0x0a == scbuf[4])
									{
                                    	dce = 0x1a;
										val = 62;
									}
									else if (0x0b == scbuf[4])
									{
                                    	dce = 0x1c;
										val = 64;
									}
									else if (0x0c == scbuf[4])
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
                                    	#ifdef SAME_DEBUG
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
                            	}//if ((1 == ar) || (1 == ayf) || (1 == ac))

								if (0 == fpc)
								{
									if (1 == yfyes)
                                    {
                                        pthread_cancel(yfid);
                                        pthread_join(yfid,NULL);
                                        yfyes = 0;
                                    }
									if (1 == scyes)
									{
										pthread_cancel(scid);
                                        pthread_join(scid,NULL);
                                        scyes = 0;
									}
									new_all_red(&ardata);
									dirc = scbuf[4];
									fpc = 1;

                                	if (SUCCESS != sc_set_lamp_color(*(fcdata->bbserial),dirch[dirc-5],0x02))
                                	{
                                	#ifdef SAME_DEBUG
                                    	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
                                    	*(fcdata->markbit) |= 0x0800;
                                	}
                                	if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                                        				dirch[dirc-5],0x02,fcdata->markbit))
                                	{
                                	#ifdef SAME_DEBUG
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
            							#ifdef SAME_DEBUG
                							printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            							#endif
            							}
            							else
            							{
                							write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            							}
									}//report info to server actively
									continue;
								}//if (0 == fpc)
								else if (1 == fpc)
								{//else if (1 == fpc)
									if (dirc == scbuf[4])
									{//control is current phase
										continue;
									}//control is current phase
									unsigned char		*pcc = dirch[dirc-5];
									dirc = scbuf[4];
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
									
									if (0==wcc[0])
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{
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
            								#ifdef SAME_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}
									}//if ((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
									if (0 != wcc[0])	
									{			
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 28;
											sinfo.color = 0x03;
											sinfo.time = 3;
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
            								#ifdef SAME_DEBUG
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
										memset(&sctd,0,sizeof(sctd));
										sctd.mode = 28;//traffic control
										sctd.pattern = *(fcdata->patternid);
										sctd.lampcolor = 0x02;
										sctd.lamptime = 3;
										sctd.phaseid = 0;
										sctd.stageline = 0;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&sctd))
										{
										#ifdef SAME_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&sctd,0,sizeof(sctd));
                                        sctd.mode = 28;//traffic control
                                        sctd.pattern = *(fcdata->patternid);
                                        sctd.lampcolor = 0x02;
                                        sctd.lamptime = 3;
                                        sctd.phaseid = 0;
                                        sctd.stageline = 0;	 
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&sctd))
										{
										#ifdef SAME_DEBUG
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
                               			#ifdef SAME_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef SAME_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SAME_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									ngf = 0;
									while (1)
									{
										if (SUCCESS != sc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
										{
										#ifdef SAME_DEBUG
											printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x03,fcdata->markbit))
										{
										#ifdef SAME_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);

										if (SUCCESS != sc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
										{
										#ifdef SAME_DEBUG
											printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x02,fcdata->markbit))
										{
										#ifdef SAME_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);
								
										ngf += 1;
										if (ngf >= 3)
											break;
									}
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS != sc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef SAME_DEBUG
            								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef SAME_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != sc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef SAME_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef SAME_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != sc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef SAME_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef SAME_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}

									if (0 != wcc[0])
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											sinfo.conmode = 28;
											sinfo.color = 0x01;
											sinfo.time = 3;
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
            								#ifdef SAME_DEBUG
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
                            			memset(&sctd,0,sizeof(sctd));
                            			sctd.mode = 28;//traffic control
                            			sctd.pattern = *(fcdata->patternid);
                            			sctd.lampcolor = 0x01;
                            			sctd.lamptime = 3;
                            			sctd.phaseid = 0;
                            			sctd.stageline = 0;
                            			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&sctd))
                            			{
                            			#ifdef SAME_DEBUG
                               				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            			#endif
                            			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&sctd,0,sizeof(sctd));
                                        sctd.mode = 28;//traffic control
                                        sctd.pattern = *(fcdata->patternid);
                                        sctd.lampcolor = 0x01;
                                        sctd.lamptime = 3;
                                        sctd.phaseid = 0;
                                        sctd.stageline = 0;	 
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&sctd))
										{
										#ifdef SAME_DEBUG
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
                            			#ifdef SAME_DEBUG
                               				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                			{
                                			#ifdef SAME_DEBUG
                                   				printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SAME_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(3);

									//current phase begin to red lamp
									if (SUCCESS != sc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef SAME_DEBUG
                                		printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef SAME_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}

									if (SUCCESS != sc_set_lamp_color(*(fcdata->bbserial),pnc,0x02))
                            		{
                            		#ifdef SAME_DEBUG
                               			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			pnc,0x02,fcdata->markbit))
                            		{
                            		#ifdef SAME_DEBUG
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
            							#ifdef SAME_DEBUG
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
										memset(&sctd,0,sizeof(sctd));
										sctd.mode = 28;//traffic control
										sctd.pattern = *(fcdata->patternid);
										sctd.lampcolor = 0x02;
										sctd.lamptime = 0;
										sctd.phaseid = 0;
										sctd.stageline = 0;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&sctd))
										{
										#ifdef SAME_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&sctd,0,sizeof(sctd));
                                        sctd.mode = 28;//traffic control
                                        sctd.pattern = *(fcdata->patternid);
                                        sctd.lampcolor = 0x02;
                                        sctd.lamptime = 0;
                                        sctd.phaseid = 0;
                                        sctd.stageline = 0;	 
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&sctd))
										{
										#ifdef SAME_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 28;
                            		fbdata[2] = 0;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = 0;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef SAME_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef SAME_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SAME_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
									}
								}//else if (1 == fpc)
								unsigned char				dce = 0;
								unsigned char				val = 0;
								if (0x05 == scbuf[4])
								{
									dce = 0x10;
									val = 52;
								}
								else if (0x06 == scbuf[4])
								{
									dce = 0x12;
									val = 54;
								}
								else if (0x07 == scbuf[4])
								{
									dce = 0x14;
									val = 56;
								}
								else if (0x08 == scbuf[4])
								{
                                   	dce = 0x16;
									val = 58;
								}
								else if (0x09 == scbuf[4])
								{
                                   	dce = 0x18;
									val = 60;
								}
								else if (0x0a == scbuf[4])
								{
                                   	dce = 0x1a;
									val = 62;
								}
								else if (0x0b == scbuf[4])
								{
                                   	dce = 0x1c;
									val = 64;
								}
								else if (0x0c == scbuf[4])
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
                                   	#ifdef SAME_DEBUG
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
                            else if ((0x02 == scbuf[4]) && (1 == wtlock))
                            {//step by step
                            }//step by step
                            else if ((0x03 == scbuf[4]) && (1 == wtlock))
                            {//yellow flash
								tpp.func[0] = scbuf[4];
                                memset(terbuf,0,sizeof(terbuf));
                                if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
                                {   
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {   
                                        if (write(*(fcdata->bbserial),terbuf,sizeof(terbuf)) < 0)
                                        {
                                        #ifdef SAME_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef SAME_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
								
								if (1 == ayf)
                                {//has been yellow flash status
                                    ac = 0;
                                    ar = 0;
                                    cp = 0;
                                    dirc = 0;
                                }//has been yellow flash status
                                else
                                {//create yellow flash	
									if (1 == scyes)
									{
										pthread_cancel(scid);
										pthread_join(scid,NULL);
										scyes = 0;
									}
									if (0 == yfyes)
									{
										memset(&scyfdata,0,sizeof(scyfdata));
										scyfdata.fd = fcdata;
										scyfdata.cs = chanstatus;	
										int yfret=pthread_create(&yfid,NULL,(void *)sc_yellow_flash,&scyfdata);
										if (0 != yfret)
										{
										#ifdef SAME_DEBUG
											printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											continue;
										}
										yfyes = 1;
									}
									ayf = 1;
									ac = 0;
									ar = 0;
									cp = 0;
									dirc = 0;
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
            						#ifdef SAME_DEBUG
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
                                    #ifdef SAME_DEBUG
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
                            else if ((0x04 == scbuf[4]) && (1 == wtlock))
                            {//all red
								tpp.func[0] = scbuf[4];
                                memset(terbuf,0,sizeof(terbuf));
                                if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
                                {   
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {   
                                        if (write(*(fcdata->bbserial),terbuf,sizeof(terbuf)) < 0)
                                        {
                                        #ifdef SAME_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef SAME_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))

								if (1 == ar)
                                {//has been all red status
                                    ayf = 0;
                                    ac = 0;
                                    cp = 0;
                                    dirc = 0;
                                }//has been all red status
                                else
                                {//set all red
									if (1 == scyes)
									{
										pthread_cancel(scid);
										pthread_join(scid,NULL);
										scyes = 0;
									}
									if (1 == yfyes)
									{
										pthread_cancel(yfid);
										pthread_join(yfid,NULL);
										yfyes = 0;
									}
									ar = 1;
									ac = 0;
									ayf = 0;
									cp = 0;
									dirc = 0;
									new_all_red(&ardata);	
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
            						#ifdef SAME_DEBUG
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
                                    #ifdef SAME_DEBUG
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
                            else if ((0x0d == scbuf[4]) && (1 == wtlock))
                            {//close
								tpp.func[0] = scbuf[4];
                                memset(terbuf,0,sizeof(terbuf));
                                if (SUCCESS == terminal_protol_pack(terbuf,&tpp))
                                {   
                                    if (!wait_write_serial(*(fcdata->bbserial)))
                                    {   
                                        if (write(*(fcdata->bbserial),terbuf,sizeof(terbuf)) < 0)
                                        {
                                        #ifdef SAME_DEBUG
                                            printf("write base board serial err,File: %s,Line: %d\n", \
                                                        __FILE__,__LINE__);
                                        #endif
                                        }
                                    }
                                    else
                                    {
                                    #ifdef SAME_DEBUG
                                        printf("base board serial cannot write,File: %s,Line: %d\n", \
                                                __FILE__,__LINE__);
                                    #endif
                                    }
                                }//if (SUCCESS == terminal_protol_pack(terbuf,&tpp))

								if (1 == ac)
                                {//has been all close status
                                    ayf = 0;
                                    ar = 0;
                                    cp = 0;
                                    dirc = 0;
                                }//has been all close status
                                else
                                {//set all close
									if (1 == scyes)
									{
										pthread_cancel(scid);
										pthread_join(scid,NULL);
										scyes = 0;
									}
									if (1 == yfyes)
									{
										pthread_cancel(yfid);
										pthread_join(yfid,NULL);
										yfyes = 0;
									}
									ar = 0;
									ac = 1;
									ayf = 0;
									cp = 0;
									dirc = 0;
									new_all_close(&acdata);	
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
            						#ifdef SAME_DEBUG
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
                                    #ifdef SAME_DEBUG
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
				else if ((0xCC == scbuf[0]) && (0xED == scbuf[2]))
				{//network control
					if ((0 == wllock) && (0 == keylock) && (0 == wtlock))
					{//net control is valid only when wireless and key control is invalid;
						if ((0xf0 == scbuf[1]) && (0 == netlock))
						{//net control will happen
					//		sc_end_child_thread();
					//		new_all_red(&ardata);
							*(fcdata->markbit2) |= 0x0008;
							*(fcdata->markbit) |= 0x4000;
							netlock = 1;
							netlockbak = netlock;
							objecti[0].objectv[0] = 0xf2;
                            factovs = 0;
                            memset(cbuf,0,sizeof(cbuf));
                            if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                            {
                            #ifdef SAME_DEBUG
                                printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }
                            if (!(*(fcdata->markbit) & 0x1000))
                            {
                                write(*(fcdata->sockfd->csockfd),cbuf,factovs);
                            }

							sinfo.conmode = 29;
							if (1 == *(fcdata->contmode))
								sinfo.color = 0x04;
							if (2 == *(fcdata->contmode))
								sinfo.color = 0x05;
							if (3 == *(fcdata->contmode))
								sinfo.color = 0;
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
							#ifdef SAME_DEBUG
								printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
							#endif
							}
							else
							{
								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
							}

							fbdata[1] = 29;
							fbdata[2] = 0;
							fbdata[3] = 0;
							fbdata[4] = 0;
							if (!wait_write_serial(*(fcdata->fbserial)))
							{
								if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
								{
								#ifdef SAME_DEBUG
									printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
								}
							}
							else
							{
							#ifdef SAME_DEBUG
								printf("face board serial port cannot write,Line:%d\n",__LINE__);
							#endif
							}

							if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
							{
							#ifdef SAME_DEBUG
								printf("send control info to configure tool,File: %s,Line: %d\n", \
										__FILE__,__LINE__);
							#endif
								memset(&sctd,0,sizeof(sctd));
								sctd.mode = 29;
								sctd.pattern = *(fcdata->patternid);
								sctd.lampcolor = 0;
								sctd.lamptime = 0;
								sctd.phaseid = 0;
								sctd.stageline = 0;
								if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&sctd))
								{
								#ifdef SAME_DEBUG
									printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#ifdef EMBED_CONFIGURE_TOOL
							if (*(fcdata->markbit2) & 0x0200)
							{
								memset(&sctd,0,sizeof(sctd));
                                sctd.mode = 29;
                                sctd.pattern = *(fcdata->patternid);
                                sctd.lampcolor = 0;
                                sctd.lamptime = 0;
                                sctd.phaseid = 0;
                                sctd.stageline = 0;	 
								if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&sctd))
								{
								#ifdef SAME_DEBUG
									printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}
							#endif

							gettimeofday(&ct,NULL);
                            update_event_list(fcdata->ec,fcdata->el,1,84,ct.tv_sec,fcdata->markbit);
							continue;
						}//net control will happen
						if ((0xf0 == scbuf[1]) && (1 == netlock))
						{//has been status of net control
							objecti[0].objectv[0] = 0xf2;
                            factovs = 0;
                            memset(cbuf,0,sizeof(cbuf));
                            if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                            {
                            #ifdef SAME_DEBUG
                                printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }
                            if (!(*(fcdata->markbit) & 0x1000))
                            {
                                write(*(fcdata->sockfd->csockfd),cbuf,factovs);
                            }
							continue;
						}//has been status of net control
						if ((0xf1 == scbuf[1]) && (1 == netlock))
                        {//net control will end
							if (1 == yfyes)
							{
								pthread_cancel(yfid);
								pthread_join(yfid,NULL);
								yfyes = 0;
							}
							fpc = 0;
							cp = 0;
							ac = 0;
							ar = 0;
							ayf = 0;
							*(fcdata->markbit2) &= 0xfff7;
							*(fcdata->markbit) &= 0xbfff;
							if (1 == scyes)
							{
								pthread_cancel(scid);
								pthread_join(scid,NULL);
								scyes = 0;
							}
							if (0 == scyes)
    						{
        						memset(&scdata,0,sizeof(scdata));
        						scdata.fd = fcdata;
        						scdata.td = tscdata;
        						scdata.cs = chanstatus;
        						int scret = pthread_create(&scid,NULL,(void *)start_same_control,&scdata);
        						if (0 != scret)
        						{
        						#ifdef SAME_DEBUG
            						printf("create thread error,File: %s,Line: %d\n",__FILE__,__LINE__);
									output_log("same control,create same control thread err");
        						#endif
            						return FAIL;
        						}
        						scyes = 1;
    						}
							netlock = 0;
							netlockbak = netlock;
							objecti[0].objectv[0] = 0xf3;
                            factovs = 0;
                            memset(cbuf,0,sizeof(cbuf));
                            if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                            {
                            #ifdef SAME_DEBUG
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
						if ((0xf1 == scbuf[1]) && (0 == netlock))
						{//not fit control or restrore
							objecti[0].objectv[0] = 0xf3;
                            factovs = 0;
                            memset(cbuf,0,sizeof(cbuf));
                            if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x00))
                            {
                            #ifdef SAME_DEBUG
                                printf("pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            #endif
                            }
                            if (!(*(fcdata->markbit) & 0x1000))
                            {
                                write(*(fcdata->sockfd->csockfd),cbuf,factovs);
                            }
						}//not fit control or restrore
						if ((1 == netlock) && ((0xf0 != scbuf[1]) || (0xf1 != scbuf[1])))
                        {//status of network control
							if ((0x5a <= scbuf[1]) && (scbuf[1] <= 0x5d))
							{//if ((0x5a <= scbuf[1]) && (scbuf[1] <= 0x5d))
								if ((1 == ar) || (1 == ayf) || (1 == ac))
                            	{//if ((1 == ar) || (1 == ayf) || (1 == ac))
									fpc = 1;
									cp = scbuf[1];
                                	if (1 == yfyes)
                                	{
                                    	pthread_cancel(yfid);
                                    	pthread_join(yfid,NULL);
                                    	yfyes = 0;
                                	}
									if (1 == scyes)
                                    {
                                        pthread_cancel(scid);
                                        pthread_join(scid,NULL);
                                        scyes = 0;
                                    }
									if (1 != ar)
                                    {
                                        new_all_red(&ardata);
                                    }
                                    ar = 0;
                                    ac = 0;
									ayf = 0;

                                	if (SUCCESS != sc_set_lamp_color(*(fcdata->bbserial),lkch[cp-0x5a],0x02))
                                	{
                                	#ifdef SAME_DEBUG
                                    	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
                                    	*(fcdata->markbit) |= 0x0800;
                                	}
                                	if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                                        			lkch[cp-0x5a],0x02,fcdata->markbit))
                                	{
                                	#ifdef SAME_DEBUG
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
            							#ifdef SAME_DEBUG
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
                                    	memset(&sctd,0,sizeof(sctd));
                                    	sctd.mode = 29;//traffic control
                                    	sctd.pattern = *(fcdata->patternid);
                                    	sctd.lampcolor = 0x02;
                                    	sctd.lamptime = 0;
                                    	sctd.phaseid = 0;
                                    	sctd.stageline = 0;
                                    	if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&sctd))
                                    	{
                                    	#ifdef SAME_DEBUG
                                        	printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                	}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&sctd,0,sizeof(sctd));
                                        sctd.mode = 29;//traffic control
                                        sctd.pattern = *(fcdata->patternid);
                                        sctd.lampcolor = 0x02;
                                        sctd.lamptime = 0;
                                        sctd.phaseid = 0;
                                        sctd.stageline = 0;	 
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&sctd))
										{
										#ifdef SAME_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,89,ct.tv_sec,fcdata->markbit);							
                                	continue;
                            	}//if ((1 == ar) || (1 == ayf) || (1 == ac))

								if (0 == fpc)
								{//phase control of first times
									if (1 == yfyes)
                                    {
                                        pthread_cancel(yfid);
                                        pthread_join(yfid,NULL);
                                        yfyes = 0;
                                    }
									if (1 == scyes)
									{
										pthread_cancel(scid);
                                        pthread_join(scid,NULL);
                                        scyes = 0;
									}
									new_all_red(&ardata);
									cp = scbuf[1];
									fpc = 1;
									
                                	if (SUCCESS != sc_set_lamp_color(*(fcdata->bbserial),lkch[cp-0x5a],0x02))
                                	{
                                	#ifdef SAME_DEBUG
                                    	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
                                    	*(fcdata->markbit) |= 0x0800;
                                	}
                                	if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                                        			lkch[cp-0x5a],0x02,fcdata->markbit))
                                	{
                                	#ifdef SAME_DEBUG
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
            							#ifdef SAME_DEBUG
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
                                    	memset(&sctd,0,sizeof(sctd));
                                    	sctd.mode = 29;//traffic control
                                    	sctd.pattern = *(fcdata->patternid);
                                    	sctd.lampcolor = 0x02;
                                    	sctd.lamptime = 0;
                                    	sctd.phaseid = 0;
                                    	sctd.stageline = 0;
                                    	if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&sctd))
                                    	{
                                    	#ifdef SAME_DEBUG
                                        	printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                	}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&sctd,0,sizeof(sctd));
                                        sctd.mode = 29;//traffic control
                                        sctd.pattern = *(fcdata->patternid);
                                        sctd.lampcolor = 0x02;
                                        sctd.lamptime = 0;
                                        sctd.phaseid = 0;
                                        sctd.stageline = 0;	 
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&sctd))
										{
										#ifdef SAME_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif								
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,89,ct.tv_sec,fcdata->markbit);
									continue;
								}//phase control of first times

								if (cp == scbuf[1])
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
            							#ifdef SAME_DEBUG
                							printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            							#endif
            							}
            							else
            							{
                							write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            							}
									}
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                                	{
                                    	memset(&sctd,0,sizeof(sctd));
                                    	sctd.mode = 29;//traffic control
                                    	sctd.pattern = *(fcdata->patternid);
                                    	sctd.lampcolor = 0x02;
                                    	sctd.lamptime = 0;
                                    	sctd.phaseid = 0;
                                    	sctd.stageline = 0;
                                    	if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&sctd))
                                    	{
                                    	#ifdef SAME_DEBUG
                                        	printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                	}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&sctd,0,sizeof(sctd));
                                        sctd.mode = 29;//traffic control
                                        sctd.pattern = *(fcdata->patternid);
                                        sctd.lampcolor = 0x02;
                                        sctd.lamptime = 0;
                                        sctd.phaseid = 0;
                                        sctd.stageline = 0;	 
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&sctd))
										{
										#ifdef SAME_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif

									objecti[0].objectv[0] = 0xf4;
                                    factovs = 0;
                                    memset(cbuf,0,sizeof(cbuf));
                                    if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                                    {
                                    #ifdef SAME_DEBUG
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
                                        {//for (i = 0; i < MAX_CHANNEL_LINE; i++)
                                            if (0 == cc[i])
                                                break;
                                            ce = 0;
											for (j = 0; j < MAX_CHANNEL; j++)
                                            {
                                                if (0 == lkch[scbuf[1]-0x5a][j])
                                                    break;
                                                if (lkch[scbuf[1]-0x5a][j] == cc[i])
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
													if (0 == lkch[scbuf[1]-0x5a][j])
														break;
													if (lkch[scbuf[1]-0x5a][j] == \
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
									
									if (0 == wcc[0])
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{
											if ((0x01 <= cp) && (cp <= 0x20))
                                                sinfo.conmode = 29;
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
                                                sinfo.conmode = cp;
											sinfo.color = 0x02;
											sinfo.time = 6;//3 sec gftime and 3 sec yellow
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
                								tcsta |= 0x02; //00000010-green 
                								*csta = tcsta;
                								csta++;
            								}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
											memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SAME_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}
									}//if((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
						
									if (0 != wcc[0])
									{	
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
                                                sinfo.conmode = 29;
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
                                                sinfo.conmode = cp;
											sinfo.color = 0x03;
											sinfo.time = 3;//default 3 sec green flash
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
            								#ifdef SAME_DEBUG
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
										memset(&sctd,0,sizeof(sctd));
										sctd.mode = 29;//traffic control
										sctd.pattern = *(fcdata->patternid);
										sctd.lampcolor = 0x02;
										sctd.lamptime = 3;
										sctd.phaseid = 0;
										sctd.stageline = 0;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&sctd))
										{
										#ifdef SAME_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&sctd,0,sizeof(sctd));
                                        sctd.mode = 29;//traffic control
                                        sctd.pattern = *(fcdata->patternid);
                                        sctd.lampcolor = 0x02;
                                        sctd.lamptime = 3;
                                        sctd.phaseid = 0;
                                        sctd.stageline = 0;	 
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&sctd))
										{
										#ifdef SAME_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = 0;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = 3;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef SAME_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef SAME_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SAME_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}

									ngf = 0;
									while (1)
									{
										if (SUCCESS != sc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
										{
										#ifdef SAME_DEBUG
											printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x03,fcdata->markbit))
										{
										#ifdef SAME_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);

										if (SUCCESS != sc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
										{
										#ifdef SAME_DEBUG
											printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x02,fcdata->markbit))
										{
										#ifdef SAME_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);
								
										ngf += 1;
										if (ngf >= 3)
											break;
									}
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS != sc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef SAME_DEBUG
            								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef SAME_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != sc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef SAME_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef SAME_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != sc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef SAME_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef SAME_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}

									if (0 != wcc[0])
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
                                                sinfo.conmode = 29;
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
                                                sinfo.conmode = cp;
											sinfo.color = 0x01;
											sinfo.time = 3;
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
            								#ifdef SAME_DEBUG
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
                            			memset(&sctd,0,sizeof(sctd));
                            			sctd.mode = 29;//traffic control
                            			sctd.pattern = *(fcdata->patternid);
                            			sctd.lampcolor = 0x01;
                            			sctd.lamptime = 3;
                            			sctd.phaseid = 0;
                            			sctd.stageline = 3;
                            			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&sctd))
                            			{
                            			#ifdef SAME_DEBUG
                               				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            			#endif
                            			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&sctd,0,sizeof(sctd));
                                        sctd.mode = 29;//traffic control
                                        sctd.pattern = *(fcdata->patternid);
                                        sctd.lampcolor = 0x01;
                                        sctd.lamptime = 3;
                                        sctd.phaseid = 0;
                                        sctd.stageline = 3;	 
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&sctd))
										{
										#ifdef SAME_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = 0;
                            		fbdata[3] = 0x01;
                            		fbdata[4] = 3;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                            			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                            			{
                            			#ifdef SAME_DEBUG
                               				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                			{
                                			#ifdef SAME_DEBUG
                                   				printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SAME_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(3);

									//current phase begin to red lamp
									if (SUCCESS != sc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef SAME_DEBUG
                                		printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef SAME_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}

									if (SUCCESS != \
										sc_set_lamp_color(*(fcdata->bbserial),lkch[scbuf[1]-0x5a],0x02))
                            		{
                            		#ifdef SAME_DEBUG
                               			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																lkch[scbuf[1]-0x5a],0x02,fcdata->markbit))
                            		{
                            		#ifdef SAME_DEBUG
                               			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
                            		}
									if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
									{//report info to server actively
										sinfo.conmode = scbuf[1];
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
                							if (0 == lkch[scbuf[1]-0x5a][i])
                    							break;
                							sinfo.chans += 1;
                							tcsta = lkch[scbuf[1]-0x5a][i];
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
            							#ifdef SAME_DEBUG
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
										memset(&sctd,0,sizeof(sctd));
										sctd.mode = 29;//traffic control
										sctd.pattern = *(fcdata->patternid);
										sctd.lampcolor = 0x02;
										sctd.lamptime = 0;
										sctd.phaseid = 0;
										sctd.stageline = 0;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&sctd))
										{
										#ifdef SAME_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&sctd,0,sizeof(sctd));
                                        sctd.mode = 29;//traffic control
                                        sctd.pattern = *(fcdata->patternid);
                                        sctd.lampcolor = 0x02;
                                        sctd.lamptime = 0;
                                        sctd.phaseid = 0;
                                        sctd.stageline = 0;	 
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&sctd))
										{
										#ifdef SAME_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = 0;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = 0;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef SAME_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef SAME_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SAME_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									cp = scbuf[1];

									objecti[0].objectv[0] = 0xf4;
									factovs = 0;
									memset(cbuf,0,sizeof(cbuf));
									if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                            		{
                            		#ifdef SAME_DEBUG
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
							}//if ((0x5a <= scbuf[1]) && (scbuf[1] <= 0x5d))
							else if ((0x01 <= scbuf[1]) && (scbuf[1] <= 0x20))
							{//control someone phase
								//check phaseid whether exist in phase list;
								pex = 0;
								for (i = 0; i < (tscdata->phase->FactPhaseNum); i++)
								{
									if (0 == (tscdata->phase->PhaseList[i].PhaseId))
										break;
									if (scbuf[1] == (tscdata->phase->PhaseList[i].PhaseId))
									{
										pex = 1;
										break;
									}	
								}
								if (0 == pex)
								{
								#ifdef SAME_DEBUG
									printf("Not fit phase id,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									objecti[0].objectv[0] = 0xf4;
                                    factovs = 0;
                                    memset(cbuf,0,sizeof(cbuf));
                                    if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x00))
                                    {
                                    #ifdef SAME_DEBUG
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

								if ((1 == ar) || (1 == ayf) || (1 == ac))
                            	{//if ((1 == ar) || (1 == ayf) || (1 == ac))
									fpc = 1;
									cp = scbuf[1];
                                	if (1 == yfyes)
                                	{
                                    	pthread_cancel(yfid);
                                    	pthread_join(yfid,NULL);
                                    	yfyes = 0;
                                	}
									if (1 == scyes)
                                    {
                                        pthread_cancel(scid);
                                        pthread_join(scid,NULL);
                                        scyes = 0;
                                    }
									if (1 != ar)
                                    {
                                        new_all_red(&ardata);
                                    }
                                    ar = 0;
                                    ac = 0;
									ayf = 0;

									memset(nc,0,sizeof(nc));
                                    pcc = nc;
                                    for (i = 0; i < (tscdata->channel->FactChannelNum); i++)
                                    {
                                        if (0 == (tscdata->channel->ChannelList[i].ChannelId))
                                            break;
                                        if (scbuf[1] == (tscdata->channel->ChannelList[i].ChannelCtrlSrc))
                                        {
                                            *pcc = tscdata->channel->ChannelList[i].ChannelId;
                                            pcc++;
                                        }
                                    }
                                	if (SUCCESS != sc_set_lamp_color(*(fcdata->bbserial),nc,0x02))
                                	{
                                	#ifdef SAME_DEBUG
                                    	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
                                    	*(fcdata->markbit) |= 0x0800;
                                	}
                                	if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                                        				nc,0x02,fcdata->markbit))
                                	{
                                	#ifdef SAME_DEBUG
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
            							#ifdef SAME_DEBUG
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
                                    	memset(&sctd,0,sizeof(sctd));
                                    	sctd.mode = 29;//traffic control
                                    	sctd.pattern = *(fcdata->patternid);
                                    	sctd.lampcolor = 0x02;
                                    	sctd.lamptime = 0;
                                    	sctd.phaseid = cp;
                                    	sctd.stageline = 0;
                                    	if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&sctd))
                                    	{
                                    	#ifdef SAME_DEBUG
                                        	printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                	}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&sctd,0,sizeof(sctd));
                                        sctd.mode = 29;//traffic control
                                        sctd.pattern = *(fcdata->patternid);
                                        sctd.lampcolor = 0x02;
                                        sctd.lamptime = 0;
                                        sctd.phaseid = cp;
                                        sctd.stageline = 0;	 
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&sctd))
										{
										#ifdef SAME_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,88,ct.tv_sec,fcdata->markbit);							
                                	continue;
                            	}//if ((1 == ar) || (1 == ayf) || (1 == ac))

								if (0 == fpc)
								{//phase control of first times
									if (1 == yfyes)
                                    {
                                        pthread_cancel(yfid);
                                        pthread_join(yfid,NULL);
                                        yfyes = 0;
                                    }
									if (1 == scyes)
									{
										pthread_cancel(scid);
                                        pthread_join(scid,NULL);
                                        scyes = 0;
									}
									new_all_red(&ardata);
									cp = scbuf[1];
									fpc = 1;
									memset(nc,0,sizeof(nc));
                                    pcc = nc;
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
                                	if (SUCCESS != sc_set_lamp_color(*(fcdata->bbserial),nc,0x02))
                                	{
                                	#ifdef SAME_DEBUG
                                    	printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                	#endif
                                    	*(fcdata->markbit) |= 0x0800;
                                	}
                                	if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
                                                        				nc,0x02,fcdata->markbit))
                                	{
                                	#ifdef SAME_DEBUG
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
            							#ifdef SAME_DEBUG
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
                                    	memset(&sctd,0,sizeof(sctd));
                                    	sctd.mode = 29;//traffic control
                                    	sctd.pattern = *(fcdata->patternid);
                                    	sctd.lampcolor = 0x02;
                                    	sctd.lamptime = 0;
                                    	sctd.phaseid = cp;
                                    	sctd.stageline = 0;
                                    	if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&sctd))
                                    	{
                                    	#ifdef SAME_DEBUG
                                        	printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                	}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&sctd,0,sizeof(sctd));
                                        sctd.mode = 29;//traffic control
                                        sctd.pattern = *(fcdata->patternid);
                                        sctd.lampcolor = 0x02;
                                        sctd.lamptime = 0;
                                        sctd.phaseid = cp;
                                        sctd.stageline = 0;	 
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&sctd))
										{
										#ifdef SAME_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif								
									gettimeofday(&ct,NULL);
                                    update_event_list(fcdata->ec,fcdata->el,1,88,ct.tv_sec,fcdata->markbit);
									continue;
								}//phase control of first times

								if (cp == scbuf[1])
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
            							#ifdef SAME_DEBUG
                							printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            							#endif
            							}
            							else
            							{
                							write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            							}
									}
									if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                                	{
                                    	memset(&sctd,0,sizeof(sctd));
                                    	sctd.mode = 29;//traffic control
                                    	sctd.pattern = *(fcdata->patternid);
                                    	sctd.lampcolor = 0x02;
                                    	sctd.lamptime = 0;
                                    	sctd.phaseid = cp;
                                    	sctd.stageline = 0;
                                    	if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&sctd))
                                    	{
                                    	#ifdef SAME_DEBUG
                                        	printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    	#endif
                                    	}
                                	}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&sctd,0,sizeof(sctd));
                                        sctd.mode = 29;//traffic control
                                        sctd.pattern = *(fcdata->patternid);
                                        sctd.lampcolor = 0x02;
                                        sctd.lamptime = 0;
                                        sctd.phaseid = cp;
                                        sctd.stageline = 0;	 
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&sctd))
										{
										#ifdef SAME_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									objecti[0].objectv[0] = 0xf4;
                                    factovs = 0;
                                    memset(cbuf,0,sizeof(cbuf));
                                    if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                                    {
                                    #ifdef SAME_DEBUG
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
                                        if (scbuf[1] == (tscdata->channel->ChannelList[i].ChannelCtrlSrc))
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
									
									if (0 == wcc[0])
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{
											if ((0x01 <= cp) && (cp <= 0x20))
                                                sinfo.conmode = 29;
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
                                                sinfo.conmode = cp;
											sinfo.color = 0x02;
											sinfo.time = 6;//3 sec gftime and 3 sec yellow
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
                								tcsta |= 0x02; //00000010-green 
                								*csta = tcsta;
                								csta++;
            								}
											memcpy(fcdata->sinfo,&sinfo,sizeof(statusinfo_t));
											memset(sibuf,0,sizeof(sibuf));
            								if (SUCCESS != status_info_report(sibuf,&sinfo))
            								{
            								#ifdef SAME_DEBUG
                								printf("info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            								#endif
            								}
            								else
            								{
                								write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            								}
										}
									}//if((0==wcc[0])&&((pinfo.gftime>0)||(pinfo.ytime>0)||(pinfo.rtime>0)))
						
									if (0 != wcc[0])
									{	
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
                                                sinfo.conmode = 29;
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
                                                sinfo.conmode = cp;
											sinfo.color = 0x03;
											sinfo.time = 3;//default 3 sec green flash
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
            								#ifdef SAME_DEBUG
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
										memset(&sctd,0,sizeof(sctd));
										sctd.mode = 29;//traffic control
										sctd.pattern = *(fcdata->patternid);
										sctd.lampcolor = 0x02;
										sctd.lamptime = 3;
										sctd.phaseid = cp;
										sctd.stageline = 0;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&sctd))
										{
										#ifdef SAME_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&sctd,0,sizeof(sctd));
                                        sctd.mode = 29;//traffic control
                                        sctd.pattern = *(fcdata->patternid);
                                        sctd.lampcolor = 0x02;
                                        sctd.lamptime = 3;
                                        sctd.phaseid = cp;
                                        sctd.stageline = 0;	 
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&sctd))
										{
										#ifdef SAME_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = 0;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = 3;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef SAME_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef SAME_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SAME_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}

									ngf = 0;
									while (1)
									{
										if (SUCCESS != sc_set_lamp_color(*(fcdata->bbserial),wcc,0x03))
										{
										#ifdef SAME_DEBUG
											printf("close lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x03,fcdata->markbit))
										{
										#ifdef SAME_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);

										if (SUCCESS != sc_set_lamp_color(*(fcdata->bbserial),wcc,0x02))
										{
										#ifdef SAME_DEBUG
											printf("set lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																				wcc,0x02,fcdata->markbit))
										{
										#ifdef SAME_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										to.tv_sec = 0;
										to.tv_usec = 500000;
										select(0,NULL,NULL,NULL,&to);
								
										ngf += 1;
										if (ngf >= 3)
											break;
									}
									if (1 == pce)
									{
										//current phase begin to yellow lamp
										if (SUCCESS != sc_set_lamp_color(*(fcdata->bbserial),wnpcc,0x01))
        								{
        								#ifdef SAME_DEBUG
            								printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
        								#endif
											*(fcdata->markbit) |= 0x0800;
        								}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wnpcc,0x01, fcdata->markbit))
										{
										#ifdef SAME_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
										if (SUCCESS != sc_set_lamp_color(*(fcdata->bbserial),wpcc,0x00))
										{
										#ifdef SAME_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wpcc,0x00,fcdata->markbit))
										{
										#ifdef SAME_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									else
									{
										if (SUCCESS != sc_set_lamp_color(*(fcdata->bbserial),wcc,0x01))
										{
										#ifdef SAME_DEBUG
											printf("set lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											*(fcdata->markbit) |= 0x0800;
										}
										if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x01,fcdata->markbit))
										{
										#ifdef SAME_DEBUG
											printf("update chan err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}

									if (0 != wcc[0])
									{
										if((!(*(fcdata->markbit) & 0x1000))&&(!(*(fcdata->markbit) & 0x8000)))
										{//report info to server actively
											if ((0x01 <= cp) && (cp <= 0x20))
                                                sinfo.conmode = 29;
                                            else if ((0x5a <= cp) && (cp <= 0x5d))
                                                sinfo.conmode = cp;
											sinfo.color = 0x01;
											sinfo.time = 3;
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
            								#ifdef SAME_DEBUG
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
                            			memset(&sctd,0,sizeof(sctd));
                            			sctd.mode = 29;//traffic control
                            			sctd.pattern = *(fcdata->patternid);
                            			sctd.lampcolor = 0x01;
                            			sctd.lamptime = 3;
                            			sctd.phaseid = cp;
                            			sctd.stageline = 3;
                            			if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&sctd))
                            			{
                            			#ifdef SAME_DEBUG
                               				printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            			#endif
                            			}
                            		}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&sctd,0,sizeof(sctd));
                                        sctd.mode = 29;//traffic control
                                        sctd.pattern = *(fcdata->patternid);
                                        sctd.lampcolor = 0x01;
                                        sctd.lamptime = 3;
                                        sctd.phaseid = cp;
                                        sctd.stageline = 3;	 
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&sctd))
										{
										#ifdef SAME_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = 0;
                            		fbdata[3] = 0x01;
                            		fbdata[4] = 3;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                            			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                            			{
                            			#ifdef SAME_DEBUG
                               				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                			{
                                			#ifdef SAME_DEBUG
                                   				printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                			#endif
                                			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SAME_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									sleep(3);

									//current phase begin to red lamp
									if (SUCCESS != sc_set_lamp_color(*(fcdata->bbserial),wcc,0x00))
                            		{
                            		#ifdef SAME_DEBUG
                                		printf("set green lamp err,File:%s,Line:%d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			wcc,0x00,fcdata->markbit))
									{
									#ifdef SAME_DEBUG
										printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
									}

									if (SUCCESS != sc_set_lamp_color(*(fcdata->bbserial),nc,0x02))
                            		{
                            		#ifdef SAME_DEBUG
                               			printf("set green lamp err,File: %s,Line: %d\n",__FILE__,__LINE__);
                            		#endif
										*(fcdata->markbit) |= 0x0800;
                            		}
						
									if (SUCCESS != update_channel_status(fcdata->sockfd,chanstatus, \
																			nc,0x02,fcdata->markbit))
                            		{
                            		#ifdef SAME_DEBUG
                               			printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
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
										sinfo.phase |= (0x01 << (scbuf[1] - 1));

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
            							#ifdef SAME_DEBUG
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
										memset(&sctd,0,sizeof(sctd));
										sctd.mode = 29;//traffic control
										sctd.pattern = *(fcdata->patternid);
										sctd.lampcolor = 0x02;
										sctd.lamptime = 0;
										sctd.phaseid = scbuf[1];
										sctd.stageline = 0;
										if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&sctd))
										{
										#ifdef SAME_DEBUG
											printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#ifdef EMBED_CONFIGURE_TOOL
									if (*(fcdata->markbit2) & 0x0200)
									{
										memset(&sctd,0,sizeof(sctd));
                                        sctd.mode = 29;//traffic control
                                        sctd.pattern = *(fcdata->patternid);
                                        sctd.lampcolor = 0x02;
                                        sctd.lamptime = 0;
                                        sctd.phaseid = scbuf[1];
                                        sctd.stageline = 0;	 
										if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&sctd))
										{
										#ifdef SAME_DEBUG
											printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
										#endif
										}
									}
									#endif
									fbdata[1] = 29;
                            		fbdata[2] = 0;
                            		fbdata[3] = 0x02;
                            		fbdata[4] = 0;
                            		if (!wait_write_serial(*(fcdata->fbserial)))
                            		{
                               			if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                               			{
                               			#ifdef SAME_DEBUG
                                   			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                               			#endif
											*(fcdata->markbit) |= 0x0800;
											gettimeofday(&ct,NULL);
                                   			update_event_list(fcdata->ec,fcdata->el,1,16, \
																	ct.tv_sec,fcdata->markbit);
                                   			if(SUCCESS!=generate_event_file(fcdata->ec,fcdata->el, \
																		fcdata->softevent,fcdata->markbit))
                                   			{
                                   			#ifdef SAME_DEBUG
                                       			printf("gen file err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                   			#endif
                                   			}
                               			}
                            		}
                            		else
                            		{
                            		#ifdef SAME_DEBUG
                               			printf("face board serial port cannot write,Line:%d\n",__LINE__);
                            		#endif
                            		}
									cp = scbuf[1];

									objecti[0].objectv[0] = 0xf4;
									factovs = 0;
									memset(cbuf,0,sizeof(cbuf));
									if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                            		{
                            		#ifdef SAME_DEBUG
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
							else if (0x27 == scbuf[1])
							{//net step by step
							}//net step by step
							else if(0x21 == scbuf[1])
							{//yellow flash
								if (1 == ayf)
                                {//has been yellow flash status
                                    ac = 0;
                                    ar = 0;
                                    cp = 0;
                                }//has been yellow flash status
                                else
                                {//create yellow flash	
									if (1 == scyes)
									{
										pthread_cancel(scid);
										pthread_join(scid,NULL);
										scyes = 0;
									}
									if (0 == yfyes)
									{
										memset(&scyfdata,0,sizeof(scyfdata));
										scyfdata.fd = fcdata;
										scyfdata.cs = chanstatus;	
										int yfret=pthread_create(&yfid,NULL,(void *)sc_yellow_flash,&scyfdata);
										if (0 != yfret)
										{
										#ifdef SAME_DEBUG
											printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
											continue;
										}
										yfyes = 1;
									}
									ayf = 1;
									ac = 0;
									ar = 0;
									cp = 0;
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
            						#ifdef SAME_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively

								objecti[0].objectv[0] = 0x24;
                                factovs = 0;
                                memset(cbuf,0,sizeof(cbuf));
                                if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                                {
                                #ifdef SAME_DEBUG
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
							else if (0x22 == scbuf[1])
							{//all red
								if (1 == ar)
                                {//has been all red status
                                    ayf = 0;
                                    ac = 0;
                                    cp = 0;
                                }//has been all red status
                                else
                                {//set all red
									if (1 == scyes)
									{
										pthread_cancel(scid);
										pthread_join(scid,NULL);
										scyes = 0;
									}
									if (1 == yfyes)
									{
										pthread_cancel(yfid);
										pthread_join(yfid,NULL);
										yfyes = 0;
									}
									ar = 1;
									ac = 0;
									ayf = 0;
									cp = 0;
									new_all_red(&ardata);	
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
            						#ifdef SAME_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively

								objecti[0].objectv[0] = 0x25;
                                factovs = 0;
                                memset(cbuf,0,sizeof(cbuf));
                                if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                                {
                                #ifdef SAME_DEBUG
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
							else if (0x23 == scbuf[1])
							{//close lamp
								if (1 == ac)
                                {//has been all close status
                                    ayf = 0;
                                    ar = 0;
                                    cp = 0;
                                }//has been all close status
                                else
                                {//set all close
									if (1 == scyes)
									{
										pthread_cancel(scid);
										pthread_join(scid,NULL);
										scyes = 0;
									}
									if (1 == yfyes)
									{
										pthread_cancel(yfid);
										pthread_join(yfid,NULL);
										yfyes = 0;
									}
									ar = 0;
									ac = 1;
									ayf = 0;
									cp = 0;
									new_all_close(&acdata);	
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
            						#ifdef SAME_DEBUG
                						printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively

								objecti[0].objectv[0] = 0x26;
                                factovs = 0;
                                memset(cbuf,0,sizeof(cbuf));
                                if (SUCCESS != control_data_pack(cbuf,&itype,objecti,&factovs,0x01))
                                {
                                #ifdef SAME_DEBUG
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
				else if ((0xDA == scbuf[0]) && (0xED == scbuf[2]))
				{//data from key
					if ((0 == wllock) && (0 == netlock) && (0 == wtlock))
					{//key control is valid only when wireless control is not valid;
						if (0x01 == scbuf[1])
						{//lock or unlock
							if (0 == keylock)
							{//lock will happen
					//			sc_end_child_thread();
					//			new_all_red(&ardata);
								*(fcdata->markbit2) |= 0x0002;
								keylock = 1;

								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									sinfo.conmode = 28;
									if (1 == *(fcdata->contmode))
										sinfo.color = 0x04;
									if (2 == *(fcdata->contmode))
										sinfo.color = 0x05;
									if (3 == *(fcdata->contmode))
										sinfo.color = 0;
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
            						#ifdef SAME_DEBUG
                						printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively

								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
                                {//report info to server actively
                                    memset(err,0,sizeof(err));
                                    gettimeofday(&ct,NULL);
                                    if (SUCCESS != err_report(err,ct.tv_sec,22,1))
                                    {
                                    #ifdef SAME_DEBUG
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

								fbdata[1] = 28;
                                fbdata[2] = 0;
                                fbdata[3] = 0;
                                fbdata[4] = 0;
                                if (!wait_write_serial(*(fcdata->fbserial)))
                                {
                                    if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                    {
                                    #ifdef SAME_DEBUG
                                        printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                                else
                                {
                                #ifdef SAME_DEBUG
                                    printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                #endif
                                }

								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                                {
                                #ifdef SAME_DEBUG
                                    printf("send control info to configure tool,File: %s,Line: %d\n", \
                                            __FILE__,__LINE__);
                                #endif
                                    memset(&sctd,0,sizeof(sctd));
                                    sctd.mode = 28;
                                    sctd.pattern = *(fcdata->patternid);
                                    sctd.lampcolor = 0;
                                    sctd.lamptime = 0;
                                    sctd.phaseid = 0;
                                    sctd.stageline = 0;
                                    if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&sctd))
                                    {
                                    #ifdef SAME_DEBUG
                                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
								#ifdef EMBED_CONFIGURE_TOOL
                                if (*(fcdata->markbit2) & 0x0200)
                                {
                                    memset(&sctd,0,sizeof(sctd));
                                    sctd.mode = 28;
                                    sctd.pattern = *(fcdata->patternid);
                                    sctd.lampcolor = 0;
                                    sctd.lamptime = 0;
                                    sctd.phaseid = 0;
                                    sctd.stageline = 0; 
                                    if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&sctd))
                                    {
                                    #ifdef SAME_DEBUG
                                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                                #endif

								continue;
							}//lock will happen
							if (1 == keylock)
							{//unlock will happen
								ac = 0;
								ar = 0;
								ayf = 0;
								if (1 == yfyes)
								{
									pthread_cancel(yfid);
									pthread_join(yfid,NULL);
									yfyes = 0;
								}
								*(fcdata->markbit2) &= 0xfffd;
								if (1 == scyes)
								{
									pthread_cancel(scid);
									pthread_join(scid,NULL);
									scyes = 0;
								}
								if (0 == scyes)
    							{
        							memset(&scdata,0,sizeof(scdata));
        							scdata.fd = fcdata;
        							scdata.td = tscdata;
        							scdata.cs = chanstatus;
        							int scret = pthread_create(&scid,NULL,(void *)start_same_control,&scdata);
        							if (0 != scret)
        							{
        							#ifdef SAME_DEBUG
            							printf("create thread error,File: %s,Line: %d\n",__FILE__,__LINE__);
										output_log("same control,create same control thread err");
        							#endif
            							return FAIL;
        							}
        							scyes = 1;
    							}
								keylock = 0;
								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
                                {//report info to server actively
                                    memset(err,0,sizeof(err));
                                    gettimeofday(&ct,NULL);
                                    if (SUCCESS != err_report(err,ct.tv_sec,22,3))
                                    {
                                    #ifdef SAME_DEBUG
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
							}//unlock will happen
						}//lock or unlock
						if ((1 == keylock) && (2 == scbuf[1]))
						{//step by step
							continue;
						}//step by step
						if ((1 == keylock) && (3 == scbuf[1]))
                        {//yellow flash
							if (1 == ayf)
							{//has been yellow flash status
								ac = 0;
								ar = 0;
								continue;
							}//has been yellow flash status
							else
							{//create yellow flash
								if (1 == scyes)
								{
									pthread_cancel(scid);
									pthread_join(scid,NULL);
									scyes = 0;
								}
								if (0 == yfyes)
								{
									memset(&scyfdata,0,sizeof(scyfdata));
									scyfdata.fd = fcdata;
									scyfdata.cs = chanstatus;	
									int yfret = pthread_create(&yfid,NULL,(void *)sc_yellow_flash,&scyfdata);
									if (0 != yfret)
									{
									#ifdef SAME_DEBUG
										printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										continue;
									}
									yfyes = 1;
								}
								ayf = 1;
								ac = 0;
								ar = 0;
							}//create yellow flash
							if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
                            {
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
                                #ifdef SAME_DEBUG
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
                                #ifdef SAME_DEBUG
                                    printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
                                else
                                {
                                    write(*(fcdata->sockfd->csockfd),err,sizeof(err));
                                }
								update_event_list(fcdata->ec,fcdata->el,1,33,ct.tv_sec,fcdata->markbit);
							}
							else
							{
								gettimeofday(&ct,NULL);
                            	update_event_list(fcdata->ec,fcdata->el,1,33,ct.tv_sec,fcdata->markbit);
					 		}
							continue;
                        }//yellow flash
						if ((1 == keylock) && (4 == scbuf[1]))
                        {//all red
							if (1 == ar)
                            {//has been all red status
                                ayf = 0;
                                ac = 0;
                                continue;
                            }//has been all red status
                            else
                            {//set all red
								if (1 == scyes)
								{
									pthread_cancel(scid);
									pthread_join(scid,NULL);
									scyes = 0;
								}
								if (1 == yfyes)
								{
									pthread_cancel(yfid);
									pthread_join(yfid,NULL);
									yfyes = 0;
								}
								ar = 1;
								ac = 0;
								ayf = 0;
								unsigned char           data[4] = {0xAA,0xFF,0x00,0xED};
								if (!wait_write_serial(*(fcdata->bbserial)))
								{
									if (write(*(fcdata->bbserial),data,sizeof(data)) < 0)
									{
									#ifdef SAME_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) &= 0x0800;
										continue;
									}
								}
								else
								{
								#ifdef SAME_DEBUG
									printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									continue;
								}
								//update channel status
								for (i = 0; i < MAX_CHANNEL_STATUS; i++)
								{
									chanstatus->ChannelStatusList[i].ChannelStatusReds = 0xff;
									chanstatus->ChannelStatusList[i].ChannelStatusYellows = 0;
									chanstatus->ChannelStatusList[i].ChannelStatusGreens = 0;
								}
								//send data of channels to conf tool
								if ((*(fcdata->markbit) & 0x0010) && (*(fcdata->markbit) & 0x0002))
								{
									unsigned char ardata[8] = {'C','Y','T','F',0x00,'E','N','D'};
									if (write(*(fcdata->sockfd->ssockfd),ardata,sizeof(ardata)) < 0)
									{
									#ifdef SAME_DEBUG
										printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								
									td.mode = 28;
									td.pattern = *(fcdata->patternid);
									td.lampcolor = 0x00;
									td.lamptime = 0;
									td.phaseid = 0;
									td.stageline = 0;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&td))
									{
									#ifdef SAME_DEBUG
										printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef EMBED_CONFIGURE_TOOL
                                if (*(fcdata->markbit2) & 0x0200)
                                {
                                    unsigned char ardata[8] = {'C','Y','T','F',0x00,'E','N','D'};
                                    if (write(*(fcdata->sockfd->lhsockfd),ardata,sizeof(ardata)) < 0)
                                    {
                                    #ifdef SAME_DEBUG
                                        printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }

                                    td.mode = 28;
                                    td.pattern = *(fcdata->patternid);
                                    td.lampcolor = 0x00;
                                    td.lamptime = 0;
                                    td.phaseid = 0;
                                    td.stageline = 0;
                                    if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&td))
                                    {
                                    #ifdef SAME_DEBUG
                                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                                #endif
								fbdata[1] = 28;
								fbdata[2] = 0;
								fbdata[3] = 0x00;
								fbdata[4] = 0;
								if (!wait_write_serial(*(fcdata->fbserial)))
								{
									if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
									{
									#ifdef SAME_DEBUG
										printf("write err,File: %s,Line:%d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) |= 0x0800;
										gettimeofday(&ct,NULL);
										update_event_list(fcdata->ec,fcdata->el,1,16,ct.tv_sec,fcdata->markbit);
										if(SUCCESS != generate_event_file(fcdata->ec,fcdata->el,\
																		fcdata->softevent,fcdata->markbit))
										{
										#ifdef SAME_DEBUG
											printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
										#endif
										}
									}
								}
								else
								{
								#ifdef SAME_DEBUG
									printf("face serial cannot write,File:%s,Line:%d\n",__FILE__,__LINE__);
								#endif
								}
							}//set all red
	
							if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
                            {
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
                                #ifdef SAME_DEBUG
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
                                #ifdef SAME_DEBUG
                                    printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
                                else
                                {
                                    write(*(fcdata->sockfd->csockfd),err,sizeof(err));
                                }
								update_event_list(fcdata->ec,fcdata->el,1,35,ct.tv_sec,fcdata->markbit);
							}
							else
							{
								gettimeofday(&ct,NULL);
                            	update_event_list(fcdata->ec,fcdata->el,1,35,ct.tv_sec,fcdata->markbit);
							}
							continue;
                        }//all red
						if ((1 == keylock) && (5 == scbuf[1]))
                        {//all close
							if (1 == ac)
                            {//has been all close status
                                ayf = 0;
                                ar = 0;
                                continue;
                            }//has been all red status
                            else
                            {//set all close
								if (1 == scyes)
								{
									pthread_cancel(scid);
									pthread_join(scid,NULL);
									scyes = 0;
								}
								if (1 == yfyes)
								{
									pthread_cancel(yfid);
									pthread_join(yfid,NULL);
									yfyes = 0;
								}
								ar = 0;
								ac = 1;
								ayf = 0;
								unsigned char           data[4] = {0xAA,0xFF,0x03,0xED};
								if (!wait_write_serial(*(fcdata->bbserial)))
								{
									if (write(*(fcdata->bbserial),data,sizeof(data)) < 0)
									{
									#ifdef SAME_DEBUG
										printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										*(fcdata->markbit) &= 0x0800;
										continue;
									}
								}
								else
								{
								#ifdef SAME_DEBUG
									printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
								#endif
									continue;
								}
								//update channel status
								for (i = 0; i < MAX_CHANNEL_STATUS; i++)
								{
									chanstatus->ChannelStatusList[i].ChannelStatusReds = 0;
									chanstatus->ChannelStatusList[i].ChannelStatusYellows = 0;
									chanstatus->ChannelStatusList[i].ChannelStatusGreens = 0;
								}
								//send data of channels to conf tool
								if ((*(fcdata->markbit) & 0x0010) && (*(fcdata->markbit) & 0x0002))
								{
									unsigned char ardata[8] = {'C','Y','T','F',0x03,'E','N','D'};
									if (write(*(fcdata->sockfd->ssockfd),ardata,sizeof(ardata)) < 0)
									{
									#ifdef SAME_DEBUG
										printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								
									td.mode = 28;
									td.pattern = *(fcdata->patternid);
									td.lampcolor = 0x03;
									td.lamptime = 0;
									td.phaseid = 0;
									td.stageline = 0;
									if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&td))
									{
									#ifdef SAME_DEBUG
										printf("send time err,File:%s,Line:%d\n",__FILE__,__LINE__);
									#endif
									}
								}
								#ifdef EMBED_CONFIGURE_TOOL
                                if (*(fcdata->markbit2) & 0x0200)
                                {
                                    unsigned char ardata[8] = {'C','Y','T','F',0x03,'E','N','D'};
                                    if (write(*(fcdata->sockfd->lhsockfd),ardata,sizeof(ardata)) < 0)
                                    {
                                    #ifdef SAME_DEBUG
                                        printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }

                                    td.mode = 28;
                                    td.pattern = *(fcdata->patternid);
                                    td.lampcolor = 0x03;
                                    td.lamptime = 0;
                                    td.phaseid = 0;
                                    td.stageline = 0;
                                    if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&td))
                                    {
                                    #ifdef SAME_DEBUG
                                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                                #endif
							}//set all close
	
							if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
                            {
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
                                #ifdef SAME_DEBUG
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
                                #ifdef SAME_DEBUG
                                    printf("err_report call err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                #endif
                                }
                                else
                                {
                                    write(*(fcdata->sockfd->csockfd),err,sizeof(err));
                                }
								update_event_list(fcdata->ec,fcdata->el,1,35,ct.tv_sec,fcdata->markbit);
							#endif
							}
							else
							{
							#if 0
								gettimeofday(&ct,NULL);
                            	update_event_list(fcdata->ec,fcdata->el,1,35,ct.tv_sec,fcdata->markbit);
							#endif
							}
							continue;
                        }//all close
					}//key control is valid only when wireless control is not valid;
				}//data from key;
				else if (!strncmp("WLTC1",scbuf,5))
				{//wireless terminal traffic control
					if ((0 == keylock) && (0 == netlock) && (0 == wtlock))
                    {//wireless lock or unlock is suessfully only when key lock or unlock is not valid;
						unsigned char           wlbuf[5] = {0};
                        unsigned char           s = 0;
                        for (s = 5; ;s++)
                        {
                            if (('E' == scbuf[s]) && ('N' == scbuf[s+1]) && ('D' == scbuf[s+2]))
                                break;
                            if ('\0' == scbuf[s])
                                break;
                            if ((s - 5) > 4)
                                break;
                            wlbuf[s - 5] = scbuf[s];
                        }
                        #ifdef SAME_DEBUG
                        printf("************wlbuf: %s,File: %s,Line: %d\n",wlbuf,__FILE__,__LINE__);
                        #endif
						if (!strcmp("SOCK",wlbuf))
						{//lock or unlock
							if (0 == wllock)
							{//prepare to lock
								*(fcdata->markbit2) |= 0x0010;
								wllock = 1;
					//			sc_end_child_thread();
					//			new_all_red(&ardata);
								if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            	{
                                	memset(wltc,0,sizeof(wltc));
                                	strcpy(wltc,"SOCKBEGIN");
                                	write(*(fcdata->sockfd->ssockfd),wltc,sizeof(wltc));
                                    //send down time to configure tool
                                    memset(&sctd,0,sizeof(sctd));
                                    sctd.mode = 28;
                                    sctd.pattern = *(fcdata->patternid);
                                    sctd.lampcolor = 0;
                                    sctd.lamptime = 0;
                                    sctd.phaseid = 0;
                                    sctd.stageline = 0;
                                    if (SUCCESS != timedown_data_to_conftool(fcdata->sockfd,&sctd))
                                    {
                                    #ifdef SAME_DEBUG
                                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                            	}
								#ifdef EMBED_CONFIGURE_TOOL
								if (*(fcdata->markbit2) & 0x0200)
                                {
                                    memset(&sctd,0,sizeof(sctd));
                                    sctd.mode = 28;
                                    sctd.pattern = *(fcdata->patternid);
                                    sctd.lampcolor = 0;
                                    sctd.lamptime = 0;
                                    sctd.phaseid = 0;
                                    sctd.stageline = 0; 
                                    if (SUCCESS != timedown_data_to_embed(fcdata->sockfd,&sctd))
                                    {
                                    #ifdef SAME_DEBUG
                                        printf("time down fail,File:%s,Line:%d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                                #endif

								gettimeofday(&ct,NULL);
                                update_event_list(fcdata->ec,fcdata->el,1,90,ct.tv_sec,fcdata->markbit);

								if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
								{//report info to server actively
									sinfo.conmode = 28;
									if (1 == *(fcdata->contmode))
										sinfo.color = 0x04;
									if (2 == *(fcdata->contmode))
										sinfo.color = 0x05;
									if (3 == *(fcdata->contmode))
										sinfo.color = 0;
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
            						#ifdef SAME_DEBUG
                						printf("status info pack err,File: %s,Line: %d\n",__FILE__,__LINE__);
            						#endif
            						}
            						else
            						{
                						write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
            						}
								}//report info to server actively

								fbdata[1] = 28;
                                fbdata[2] = 0;
                                fbdata[3] = 0;
                                fbdata[4] = 0;
                                if (!wait_write_serial(*(fcdata->fbserial)))
                                {
                                    if (write(*(fcdata->fbserial),fbdata,sizeof(fbdata)) < 0)
                                    {
                                    #ifdef SAME_DEBUG
                                        printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
                                    #endif
                                    }
                                }
                                else
                                {
                                #ifdef SAME_DEBUG
                                    printf("face board serial port cannot write,Line:%d\n",__LINE__);
                                #endif
                                }								

								continue;
							}//prepare to lock
							if (1 == wllock)
							{//prepare to unlock
								ac = 0;
								ayf = 0;
								ar = 0;
								if (1 == yfyes)
								{
									pthread_cancel(yfid);
									pthread_join(yfid,NULL);
									yfyes = 0;
								}
								*(fcdata->markbit2) &= 0xffef;
								if (1 == scyes)
                                {
                                    pthread_cancel(scid);
                                    pthread_join(scid,NULL);
                                    scyes = 0;
                                }
								if (0 == scyes)
    							{
        							memset(&scdata,0,sizeof(scdata));
        							scdata.fd = fcdata;
        							scdata.td = tscdata;
        							scdata.cs = chanstatus;
        							int scret = pthread_create(&scid,NULL,(void *)start_same_control,&scdata);
        							if (0 != scret)
        							{
        							#ifdef SAME_DEBUG
            							printf("create thread error,File: %s,Line: %d\n",__FILE__,__LINE__);
										output_log("same control,create same control thread err");
        							#endif
            							return FAIL;
        							}
        							scyes = 1;
    							}
								wllock = 0;
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
							continue;
						}//step by step
						if ((1 == wllock) && (!strcmp("YF",wlbuf)))
						{//yellow flash
							if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            {
                                memset(wltc,0,sizeof(wltc));
                                strcpy(wltc,"YFBEGIN");
                                write(*(fcdata->sockfd->ssockfd),wltc,sizeof(wltc));
                            }
							if (1 == ayf)
							{//has been yellow flash status
								ac = 0;
								ar = 0;
								continue;
							}//has been yellow flash status
							else
							{//create yellow flash
								if (1 == scyes)
								{
									pthread_cancel(scid);
									pthread_join(scid,NULL);
									scyes = 0;
								}
								if (0 == yfyes)
								{
									memset(&scyfdata,0,sizeof(scyfdata));
									scyfdata.fd = fcdata;
									scyfdata.cs = chanstatus;	
									int yfret = pthread_create(&yfid,NULL,(void *)sc_yellow_flash,&scyfdata);
									if (0 != yfret)
									{
									#ifdef SAME_DEBUG
										printf("create thread err,File: %s,Line: %d\n",__FILE__,__LINE__);
									#endif
										continue;
									}
									yfyes = 1;
								}
								ayf = 1;
								ac = 0;
								ar = 0;
							}//create yellow flash
							if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
                            {
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
                                #ifdef SAME_DEBUG
                                    printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
                                else
                                {
                                    write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
                                }
							}
							gettimeofday(&ct,NULL);
                            update_event_list(fcdata->ec,fcdata->el,1,93,ct.tv_sec,fcdata->markbit);
							continue;
						}//yellow flash
						if ((1 == wllock) && (!strcmp("RED",wlbuf)))
						{//all red
							unsigned char	ctdata[8] = {'C','Y','T','F',0x00,'E','N','D'};
							if ((*(fcdata->markbit) & 0x0002) && (*(fcdata->markbit) & 0x0010))
                            {
                                memset(wltc,0,sizeof(wltc));
                                strcpy(wltc,"REDBEGIN");
                                write(*(fcdata->sockfd->ssockfd),wltc,sizeof(wltc));
                            }
							if (1 == ar)
							{//has been all red status
								ayf = 0;
								ac = 0;
								continue;
							}//has been all red status
							else
							{//set all red
								if (1 == scyes)
								{
									pthread_cancel(scid);
									pthread_join(scid,NULL);
									scyes = 0;
								}
								if (1 == yfyes)
								{
									pthread_cancel(yfid);
									pthread_join(yfid,NULL);
									yfyes = 0;
								}
								ar = 1;
								ac = 0;
								ayf = 0;
								unsigned char           data[4] = {0xAA,0xFF,0x00,0xED};
								if (!wait_write_serial(*(fcdata->bbserial)))
        						{
            						if (write(*(fcdata->bbserial),data,sizeof(data)) < 0)
            						{
            						#ifdef SAME_DEBUG
                						printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
            						#endif
										*(fcdata->markbit) &= 0x0800;
                						continue;
            						}
        						}
        						else
        						{
        						#ifdef SAME_DEBUG
            						printf("can not write,File: %s,Line: %d\n",__FILE__,__LINE__);
        						#endif
            						continue;
        						}
								//update channel status
        						for (i = 0; i < MAX_CHANNEL_STATUS; i++)
        						{
            						chanstatus->ChannelStatusList[i].ChannelStatusReds = 0xff;
            						chanstatus->ChannelStatusList[i].ChannelStatusYellows = 0;
            						chanstatus->ChannelStatusList[i].ChannelStatusGreens = 0;
        						}
							}//set all red
							if ((!(*(fcdata->markbit) & 0x1000)) && (!(*(fcdata->markbit) & 0x8000)))
                            {
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
                                #ifdef SAME_DEBUG
                                    printf("status info pack err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
                                else
                                {
                                    write(*(fcdata->sockfd->csockfd),sibuf,sizeof(sibuf));
                                }
							}
							if ((*(fcdata->markbit) & 0x0010) && (*(fcdata->markbit) & 0x0002))
    						{
        						if (write(*(fcdata->sockfd->ssockfd),ctdata,sizeof(ctdata)) < 0)
        						{
        						#ifdef SAME_DEBUG
            						printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
        						#endif
        						}
    						}
							#ifdef EMBED_CONFIGURE_TOOL
							if (*(fcdata->markbit) & 0x0200)
							{
								if (write(*(fcdata->sockfd->lhsockfd),ctdata,sizeof(ctdata)) < 0)
                                {
                                #ifdef SAME_DEBUG
                                    printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
                                #endif
                                }
							}
							#endif
							gettimeofday(&ct,NULL);
                            update_event_list(fcdata->ec,fcdata->el,1,94,ct.tv_sec,fcdata->markbit);
							continue;
						}//all red 
                    }//wireless lock or unlock is suessfully only when key lock or unlock is not valid;
				}//wireless terminal traffic control
			}//if (FD_ISSET(*(fcdata->conpipe),&nread))
		}//cpret > 0
	}//while loop

	return SUCCESS;
}
