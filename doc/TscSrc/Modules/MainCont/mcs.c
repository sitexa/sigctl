/*
 * File:   mcs.c
 * Author: sk
 *
 * Created on 2013年6月1日, 上午10:29
 */

#include "mcs.h"

/*open serial port*/
int mc_open_serial_port(int *serial)
{
	if (NULL == serial)
	{
	#ifdef MAIN_DEBUG
		printf("pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	int 	i = 0;	
	const unsigned char *dev[5] = {"/dev/ttyS1","/dev/ttyS3","/dev/ttyS4","/dev/ttyS5","/dev/ttyS2"};
	
	for (i = 0; i < 5; i++)
	{
		serial[i] = open(dev[i],O_RDWR|O_NOCTTY|O_NDELAY);
		if (-1 == serial[i])
		{
		#ifdef MAIN_DEBUG
			printf("open serial[%d] error,File: %s,Line: %d\n",i,__FILE__,__LINE__);
		#endif
			return FAIL;
		}
	}

	return SUCCESS;
}

int mc_set_serial_port_gps(int *serial)
{
	struct termios new_opt;
	int status;
	int	i = 0;

	//get the current config -> new_opt
	bzero(&new_opt, sizeof(new_opt));

	cfsetispeed(&new_opt,B4800);
	cfsetospeed(&new_opt,B4800);
	new_opt.c_cflag |= CLOCAL;

	new_opt.c_cflag |= CREAD;

	new_opt.c_cflag |= HUPCL;
	new_opt.c_cflag &=~CRTSCTS;

	new_opt.c_cflag &=~CSIZE;
	new_opt.c_cflag |=CS8;

	//setup parity
	new_opt.c_cflag &= ~PARENB;   /* Clear parity enable */
	new_opt.c_iflag &= ~INPCK;     /* Enable parity checking */



	
	new_opt.c_cflag &=~CSTOPB;

	new_opt.c_lflag &= ~(ICANON | ECHO | ISIG);				/*Input*/
	new_opt.c_oflag &= ~OPOST;								/*Output*/

	new_opt.c_cc[VMIN]=1;

	new_opt.c_cc[VTIME]=1;

    tcflush(serial[3],TCIFLUSH);
    if (0 != (tcsetattr(serial[3],TCSANOW,&new_opt)))
    {
    #ifdef MAIN_DEBUG
        printf("set serial error,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
        return FAIL;
    }

	return status;
}
int mc_set_serial_port_new(int *serial)
{
	struct termios new_opt;
	int	i = 0;

	//get the current config -> new_opt
	bzero(&new_opt, sizeof(new_opt));

	cfsetispeed(&new_opt,B115200);
	cfsetospeed(&new_opt,B115200);
	new_opt.c_cflag |= CLOCAL;

	new_opt.c_cflag |= CREAD;

	new_opt.c_cflag |= HUPCL;
	new_opt.c_cflag &=~CRTSCTS;

	new_opt.c_cflag &=~CSIZE;
	new_opt.c_cflag |=CS8;

	//setup parity
	new_opt.c_cflag &= ~PARENB;   /* Clear parity enable */
	new_opt.c_iflag &= ~INPCK;     /* Enable parity checking */



	
	new_opt.c_cflag &=~CSTOPB;

	new_opt.c_lflag &= ~(ICANON | ECHO | ISIG);				/*Input*/
	new_opt.c_oflag &= ~OPOST;								/*Output*/

	new_opt.c_cc[VMIN]=1;

	new_opt.c_cc[VTIME]=1;

	for (i = 0; i < 5; i++)
    {
        if (3 == i)
            continue;
        tcflush(serial[i],TCIFLUSH);
        if (0 != (tcsetattr(serial[i],TCSANOW,&new_opt)))
        {
        #ifdef MAIN_DEBUG
            printf("set serial error,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
            return FAIL;
        }
    }
	cfsetispeed(&new_opt,B4800);
    cfsetospeed(&new_opt,B4800);
	tcflush(serial[3],TCIFLUSH);
    if (0 != (tcsetattr(serial[3],TCSANOW,&new_opt)))
    {
    #ifdef MAIN_DEBUG
        printf("set serial error,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
        return FAIL;
    }

	return SUCCESS;
}

/*set serial port*/
int mc_set_serial_port(int *serial)
{
	if (NULL == serial)
	{
	#ifdef MAIN_DEBUG
		printf("pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	struct termios		newtio;
	int					i = 0;

	memset(&newtio,0,sizeof(struct termios));
	cfsetispeed(&newtio, B115200);
	cfsetospeed(&newtio, B115200);
	newtio.c_cflag &= ~PARENB;
	newtio.c_cflag &= ~CSTOPB;
	newtio.c_cflag &= ~CSIZE;
	newtio.c_cflag |= CS8;
//	tcflush(serial,TCIFLUSH);

	for (i = 0; i < 5; i++)
	{
		if (3 == i)
			continue;
		tcflush(serial[i],TCIFLUSH);
		if (0 != (tcsetattr(serial[i],TCSANOW,&newtio)))
		{
		#ifdef MAIN_DEBUG
			printf("set serial error,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
	}
	
	//set GPS com
	memset(&newtio,0,sizeof(struct termios));
    cfsetispeed(&newtio, B4800);
    cfsetospeed(&newtio, B4800);
    newtio.c_cflag &= ~PARENB;
    newtio.c_cflag &= ~CSTOPB;
    newtio.c_cflag &= ~CSIZE;
    newtio.c_cflag |= CS8;
	tcflush(serial[3],TCIFLUSH);
	if (0 != (tcsetattr(serial[3],TCSANOW,&newtio)))
    {
    #ifdef MAIN_DEBUG
        printf("set serial error,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
        return FAIL;
    }
	//end set GPS com

	return SUCCESS;
}

/*close serial port*/
int mc_close_serial_port(int *serial)
{
	if (NULL == serial)
	{
	#ifdef MAIN_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	int 			ret = 0;
	int				i = 0;
	
	for (i = 0; i < 5; i++)
	{
		ret = close(serial[i]);
		if (-1 == ret)
		{
		#ifdef MAIN_DEBUG
			printf("close serial port error,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
	}
	return SUCCESS;
}

/*cm:control mode;pid:pattern id;pid:phase id;*/
int mc_lights_status(unsigned char *dstr,unsigned char cm,unsigned char pid,unsigned int phaseid,ChannelStatus_t *cs)
{
	if ((NULL == dstr) || (NULL == cs))
	{
	#ifdef MAIN_DEBUG
		printf("pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	int			tpid = 0;
	
	*dstr = 'C';dstr++;
	*dstr = 'Y';dstr++;
	*dstr = 'T';dstr++;
	*dstr = '3';dstr++;
	*dstr = '4';dstr++;
	*dstr = '1';dstr++;
	*dstr = cs->ChannelStatusList[0].ChannelStatusReds;dstr++;
	*dstr = cs->ChannelStatusList[0].ChannelStatusYellows;dstr++;
	*dstr = cs->ChannelStatusList[0].ChannelStatusGreens;dstr++;
	*dstr = '2';dstr++;
	*dstr = cs->ChannelStatusList[1].ChannelStatusReds;dstr++;
	*dstr = cs->ChannelStatusList[1].ChannelStatusYellows;dstr++;
	*dstr = cs->ChannelStatusList[1].ChannelStatusGreens;dstr++;
	*dstr = '3';dstr++;
	*dstr = cs->ChannelStatusList[2].ChannelStatusReds;dstr++;
	*dstr = cs->ChannelStatusList[2].ChannelStatusYellows;dstr++;
	*dstr = cs->ChannelStatusList[2].ChannelStatusGreens;dstr++;
	*dstr = '4';dstr++;
	*dstr = cs->ChannelStatusList[3].ChannelStatusReds;dstr++;	
	*dstr = cs->ChannelStatusList[3].ChannelStatusYellows;dstr++;
	*dstr = cs->ChannelStatusList[3].ChannelStatusGreens;dstr++;
	*dstr = cm;dstr++;
	*dstr = pid;dstr++;

	tpid = phaseid;
	tpid &= 0x000000ff;
	*dstr |= tpid;dstr++;
	tpid = phaseid;
	tpid &= 0x0000ff00;
	tpid >>= 8;
	*dstr |= tpid;dstr++;
	tpid = phaseid;
	tpid &= 0x00ff0000;
	tpid >>= 16;
	*dstr |= tpid;dstr++;
	tpid = phaseid;
	tpid &= 0xff000000;
	tpid >>= 24;
	*dstr |= tpid;dstr++;
	
	*dstr = 'E';dstr++;
	*dstr = 'N';dstr++;
	*dstr = 'D';dstr++;

	return SUCCESS;
}

int netdata_combine(unsigned char *dstr,int nlen,unsigned char *stream)
{
	if ((NULL == dstr) || (NULL == stream))
	{
	#ifdef MAIN_DEBUG
		printf("pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char				*tdstr = dstr;
	int							tnlen = 0;
	int							i = 0;

	*tdstr = 0x21;
	tdstr++;
	*tdstr = 0x84;
	tdstr++;
	*tdstr = 0xD8;
	tdstr++;
	*tdstr = 0x00;
	tdstr++;

	tnlen = nlen;
	tnlen &= 0xff000000;
	tnlen >>= 24;
	tnlen &= 0x000000ff;
	*tdstr |= tnlen;
	tdstr++;

	tnlen = nlen;
	tnlen &= 0x00ff0000;
	tnlen >>= 16;
	tnlen &= 0x000000ff;
	*tdstr |= tnlen;
	tdstr++;

	tnlen = nlen;
	tnlen &= 0x0000ff00;
	tnlen >>= 8;
	tnlen &= 0x000000ff;
	*tdstr |= tnlen;
	tdstr++;

	tnlen = nlen;
	tnlen &= 0x000000ff;
	*tdstr |= tnlen;
	tdstr++;

	for (i = 0; i < nlen; i++)
	{
		*tdstr = *stream;
		stream++;
		tdstr++;
	}

	return SUCCESS;
}

int mc_combine_str(unsigned char *dstr, unsigned char *head, int datalen, unsigned char *stream, int slen, unsigned char *end)
{
	if ((NULL == dstr) || (NULL == head) || (NULL == stream) || (NULL == end))
	{
	#ifdef MAIN_DEBUG
		printf("pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	unsigned char				temps[4] = {0};
	int							tempi = 0;
	unsigned char				*tstr = dstr;
	int							len = 0;
	int 						i = 0;

	len = strlen(head);
	for (i = 0; i < len; i++)
	{
		*tstr = *head;
		tstr++;
		head++;
	}
	tempi = datalen;
	tempi &= 0x000000ff;
	temps[0] |= tempi;
	tempi = datalen;
	tempi &= 0x0000ff00;
	tempi >>= 8;
	temps[1] |= tempi;
	tempi = datalen;
	tempi &= 0x00ff0000;
	tempi >>= 16;
	temps[2] |= tempi;
	tempi = datalen;
	tempi &= 0xff000000;
	tempi >>= 24;
	temps[3] |= tempi;

	for (i = 0; i < 4; i++)
	{
		*tstr = temps[i];
		tstr++;
	}

	for (i = 0; i < slen; i++)
	{
		*tstr = *stream;
		tstr++;
		stream++;
	}
	
	len = strlen(end);
	for (i = 0; i < len; i++)
	{
		*tstr = *end;
		tstr++;
		end++;
	}

	return SUCCESS;
}

int get_last_ip_field(int *lip)
{
	if (NULL == lip)
	{
	#ifdef MAIN_DEBUG
		printf("Pointer address err,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	FILE                *ipfp = NULL;
	unsigned char		ip[20] = {0};
	unsigned char		msg[64] = {0};
	unsigned char       *tem1 = NULL;
	unsigned char       *tem2 = NULL;
	int					pos = 0;

	ipfp = fopen(IPFILE,"rb");
	if (NULL == ipfp)
	{
	#ifdef MAIN_DEBUG
		printf("open file err,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	else
	{//if (NULL != ipfp)
		while (1)
		{//while (1)
			memset(msg,0,sizeof(msg));
			if (NULL == fgets(msg,sizeof(msg),ipfp))
				break;
            if (SUCCESS == find_out_child_str(msg,"IPAddress",&pos))
            {
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
                break;
            }
		}//while (1)
		fclose(ipfp);
		
		unsigned char       section1[4] = {0};
        unsigned char       section2[4] = {0};
        unsigned char       section3[4] = {0};
        unsigned char       section4[4] = {0};
        if (SUCCESS != get_network_section(ip,section1,section2,section3,section4))
        { 
        #ifdef MAIN_DEBUG 
        	printf("get section err,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
			return FAIL;
        }
		string_to_digit(section4,lip);	
	}//if (NULL != ipfp)	

	return SUCCESS;
}

int signaler_status_report(driboardstatus_t *dbstatus,int *csockfd)
{
	if ((NULL == dbstatus) || (NULL == csockfd))
	{
	#ifdef MAIN_DEBUG
		printf("Pointer address err,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR; 
	}

	unsigned char 			i = 0;
	unsigned char           err[10] = {0};
	struct timeval			tv;
	unsigned char			dribid = 0;

	gettimeofday(&tv,NULL);
	for (i = 0; i < DRIBOARDNUM; i++)
	{
//		printf("***************************dribid:%d,status:%d,Line:%d\n", \
//				dbstatus->driboardstatusList[i].dribid,dbstatus->driboardstatusList[i].status,__LINE__);
		if ((0 == (dbstatus->driboardstatusList[i].dribid)) || (0 == (dbstatus->driboardstatusList[i].status)))
			continue;

		dribid = dbstatus->driboardstatusList[i].dribid;
//		printf("*******dribid:%d,status:%d,Line:%d\n", dribid,dbstatus->driboardstatusList[i].status,__LINE__);
		if (0 == (dbstatus->driboardstatusList[i].status))
		{//uninstall status
			memset(err,0,sizeof(err));
			if (SUCCESS != err_report(err,tv.tv_sec,19,dribid))
			{
			#ifdef MAIN_DEBUG
				printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
			#endif
			}
			else
			{
				write(*csockfd,err,sizeof(err));
			}	
		}//uninstall status
		else if(1 == (dbstatus->driboardstatusList[i].status))
		{//start-up
			memset(err,0,sizeof(err));
			if (SUCCESS != err_report(err,tv.tv_sec,19,dribid+16))
			{
			#ifdef MAIN_DEBUG
				printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
			#endif
			}
			else
			{
				write(*csockfd,err,sizeof(err));
			}
		}//start-up
		else if (2 == (dbstatus->driboardstatusList[i].status))
		{//normal
			memset(err,0,sizeof(err));
			if (SUCCESS != err_report(err,tv.tv_sec,19,dribid+32))
			{
			#ifdef MAIN_DEBUG
				printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
			#endif
			}
			else
			{
				write(*csockfd,err,sizeof(err));
			}
		}//normal
		else if (3 == (dbstatus->driboardstatusList[i].status))
		{//yellow flash since err
			memset(err,0,sizeof(err));
			if (SUCCESS != err_report(err,tv.tv_sec,19,dribid+48))
			{
			#ifdef MAIN_DEBUG
				printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
			#endif
			}
			else
			{
				write(*csockfd,err,sizeof(err));
			}
		}//yellow flash since err
		else if (4 == (dbstatus->driboardstatusList[i].status))
		{//yellow flash since close
			memset(err,0,sizeof(err));
			if (SUCCESS != err_report(err,tv.tv_sec,19,dribid+64))
			{
			#ifdef MAIN_DEBUG
				printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
			#endif
			}
			else
			{
				write(*csockfd,err,sizeof(err));
			}
		}//yellow flash since close
		else if (5 == (dbstatus->driboardstatusList[i].status))
		{//yellow flash since control
			memset(err,0,sizeof(err));
			if (SUCCESS != err_report(err,tv.tv_sec,19,dribid+80))
			{
			#ifdef MAIN_DEBUG
				printf("err_report call err,File: %s,Line: %d\n", __FILE__,__LINE__);
			#endif
			}
			else
			{
				write(*csockfd,err,sizeof(err));
			}
		}//yellow flash since control
	}//for (i = 0; i < DRIBOARDNUM; i++)

	return SUCCESS;
}


int pattern_switch(Pattern_t *pattern,Pattern1_t *pattern1)
{
	if ((NULL == pattern) || (NULL == pattern1))
	{
	#ifdef MAIN_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	unsigned char			i = 0;

	pattern->FactPatternNum = pattern1->FactPatternNum1;
	for (i = 0; i < pattern1->FactPatternNum1; i++)
	{
		if (0 == pattern1->PatternList1[i].PatternId1)
			break;
		pattern->PatternList[i].PatternId = pattern1->PatternList1[i].PatternId1;
		pattern->PatternList[i].CycleTime = pattern1->PatternList1[i].CycleTime1;
		pattern->PatternList[i].PhaseOffset = pattern1->PatternList1[i].PhaseOffset1;
		pattern->PatternList[i].CoordPhase = pattern1->PatternList1[i].CoordPhase1;
		pattern->PatternList[i].TimeConfigId = pattern1->PatternList1[i].TimeConfigId1;
	}

	return SUCCESS;
}

int start_localhost_client(lhdata_t *lhdata)
{
	if (NULL == lhdata)
	{
	#ifdef MAIN_DEBUG
		printf("pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
/*
	#ifdef MAIN_DEBUG
	printf("lhsocked: %d,File: %s,Line: %d\n",*(lhdata->lhsocked),__FILE__,__LINE__);
	#endif
	fd_set					lhRead;
	struct timeval			lhmt,ptt;
	unsigned char			revbuf[256] = {0};
	
	while(1)
	{//1
		if ((1 == *(lhdata->newip)) || (1 == *(lhdata->neterr)))
		{
			*(lhdata->newip) = 0;
			*(lhdata->neterr) = 0;
			break;
		}
		FD_ZERO(&lhRead);
		FD_SET(*(lhdata->lhsocked),&lhRead);
		lhmt.tv_sec = 15;
		lhmt.tv_usec = 0;
		int lhret = select(*(lhdata->lhsocked)+1,&lhRead,NULL,NULL,&lhmt);
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
			*(lhdata->neterr) = 1;

			if (*(lhdata->markbit2) & 0x0010)
			{//configure tool is control machine;
				*(lhdata->markbit2) &= 0xffef;
				unsigned char           lhbuf[13] = {0};
				unsigned char           lhpd[32] = {0};
				//clean pipe
				while (1)
				{
					memset(lhpd,0,sizeof(lhpd));
					if (read(*(lhdata->conpipe0),lhpd,sizeof(lhpd)) <= 0)
						break;
				}
				strcpy(lhbuf,"WLTC1SOCKEND");
				//end clean pipe            
				if (!wait_write_serial(*(lhdata->conpipe1)))
				{
					if (write(*(lhdata->conpipe1),lhbuf,sizeof(lhbuf)) < 0)
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
		}//lhret == 0
		if (lhret > 0)
		{//lhret > 0
			memset(revbuf,0,sizeof(revbuf));
			if (read(*(lhdata->lhsocked),revbuf,sizeof(revbuf)) <= 0)
			{
			#ifdef MAIN_DEBUG
				printf("Since client exit,continue to listen,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				if (*(lhdata->markbit2) & 0x0010)
				{//configure tool is control machine;
					*(lhdata->markbit2) &= 0xffef;
					unsigned char           lhbuf[13] = {0};
					unsigned char           lhpd[32] = {0};
					//clean pipe
					while (1)
					{
						memset(lhpd,0,sizeof(lhpd));
						if (read(*(lhdata->conpipe0),lhpd,sizeof(lhpd)) <= 0)
							break;
					}
					strcpy(lhbuf,"WLTC1SOCKEND");
					//end clean pipe            
					if (!wait_write_serial(*(lhdata->conpipe1)))
					{
						if (write(*(lhdata->conpipe1),lhbuf,sizeof(lhbuf)) < 0)
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
			#ifdef MAIN_DEBUG
			printf("Reading network port,revbuf: %s,File: %s,Line: %d\n",revbuf,__FILE__,__LINE__);
			#endif
			if (!strcmp(revbuf,"IAMALIVE"))
			{
				continue;	
			}
		
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

				if (write(*(lhdata->lhsocked),configys,sizeof(configys)) < 0)
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
					read(*(lhdata->lhsocked),&revdata,1);
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
					write(*(lhdata->lhsocked),"CONFIGER",9);
					continue;
				}
				#ifdef MAIN_DEBUG
				printf("This is our data,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				write(*(lhdata->lhsocked),"CONFIGOK",9);

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
		}//netret > 0
		
	}//1,2th while loop
*/	
	return SUCCESS;
}

int check_stageid_valid(unsigned char stageid,unsigned char patternid,tscdata_t *tscdata)
{
	if (NULL == tscdata)
	{
	#ifdef MAIN_DEBUG
		printf("Pointer address err,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR; 
	}
	if ((0 == stageid) || (0 == patternid))
	{
	#ifdef MAIN_DEBUG
		printf("stageid or patternid is 0,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return FAIL;
	}
	unsigned char		i = 0,j = 0;
	unsigned char		tcid = 0;
	unsigned char		exist = 0;

	for (i = 0; i < (tscdata->pattern->FactPatternNum); i++)
	{//for (i = 0; i < (tscdata->pattern->FactPatternNum); i++)
		if (0 == (tscdata->pattern->PatternList[i].PatternId))
			break;
		if (patternid == (tscdata->pattern->PatternList[i].PatternId))
		{
			tcid = tscdata->pattern->PatternList[i].TimeConfigId;
			break;
		}
	}//for (i = 0; i < (tscdata->pattern->FactPatternNum); i++)
	if (0 == tcid)
	{
	#ifdef MAIN_DEBUG
		printf("Not have fit time configid,File:%s,Line:%d\n",__FILE__,__LINE__);
	#endif
		return FAIL;
	}
	else
	{//0 != tcid
	#ifdef MAIN_DEBUG
		printf("Time config id: %d,File: %s,Line: %d\n",tcid,__FILE__,__LINE__);
	#endif
		exist = 0;
		for (i = 0; i < (tscdata->timeconfig->FactTimeConfigNum); i++)
		{//1
			if (0 == (tscdata->timeconfig->TimeConfigList[i][0].TimeConfigId))
				break;
			if (tcid == (tscdata->timeconfig->TimeConfigList[i][0].TimeConfigId))
			{//2
				for (j = 0; j < (tscdata->timeconfig->FactStageNum); j++)
				{//3
					if (0 == (tscdata->timeconfig->TimeConfigList[i][j].StageId))
						break;
					if (stageid == (tscdata->timeconfig->TimeConfigList[i][j].StageId))
					{
						exist = 1;
						break;
					}
				}//3
				break;
			}//2
		}//1
	}//0 != tcid	

	if (1 == exist)
	{
		return SUCCESS;
	}
	else
	{
		return FAIL;
	}
}


