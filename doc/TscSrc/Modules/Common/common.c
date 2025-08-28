/* 
 * File:   common.c
 * Author: sk
 *
 * Created on 2013年5月22日, 下午4:33
 */
#include "../../Inc/mydef.h"
//#include "../MainCont/mc.h"仿真测试

#define MSGTXT "./data/msg.txt"

#define OBJECTNUM 8 
static object_t object[OBJECTNUM] = {{0x8D,9,5,1,2,1,4,1,0,0,0,0,0,0,0}, \
									 {0x8E,8,8,1,1,1,1,1,1,1,1,0,0,0,0}, \
									 {0xC0,7,5,1,2,2,1,1,0,0,0,0,0,0,0}, \
									 {0xC1,10,7,1,1,4,1,1,1,1,0,0,0,0,0}, \
									 {0x95,12,12,1,1,1,1,1,1,1,1,1,1,1,1}, \
									 {0x97,5,2,1,4,0,0,0,0,0,0,0,0,0,0}, \
									 {0xB0,4,4,1,1,1,1,0,0,0,0,0,0,0,0}, \
									 {0x9F,9,8,1,1,1,1,1,1,2,1,0,0,0,0}};
/*Generate lamp err status file*/
int generate_lamp_err_file(lamperrtype_t *let,unsigned short *markbit)
{
	if ((NULL == let) || (NULL == markbit))
	{
	#ifdef COMMON_DEBUG
        printf("pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
        return MEMERR;
	}
	FILE		*lfp = NULL;
	
	lfp = fopen("./data/lamperr.dat","wb+");
    if (NULL == lfp)
    {
    #ifdef COMMON_DEBUG
        printf("open file err,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
        return FAIL;
    }
    fwrite(let,sizeof(lamperrtype_t),1,lfp);
    fclose(lfp);
	*markbit &= 0xdfff;

	return SUCCESS;
}

/*Generate event text file*/
int generate_event_file(EventClass_t *ec,EventLog_t *el,unsigned char **softevent,unsigned short *markbit)
{
	if ((NULL == el) || (NULL == *softevent) || (NULL == markbit) || (NULL == ec))
	{
	#ifdef COMMON_DEBUG
		printf("pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	FILE				*fp = NULL;
	FILE				*bfp = NULL;
	unsigned char		buf[256] = {0}; 
	unsigned char		tbuf[64] = {0};
	unsigned char		cbuf[64] = {0};
	unsigned char		i = 0;
	int 				j = 0;
	time_t				et = 0;
	struct tm 			*p = NULL;


#if 0
	if (0 == access("./data/event.txt",F_OK))
	{
	#ifdef COMMON_DEBUG
		printf("delete old event.txt,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		system("rm -f ./data/event.txt");
	}
#endif
	//Firstly,generate a bin file
	bfp = fopen("./data/event.dat","wb+");
	if (NULL == bfp)
	{
	#ifdef COMMON_DEBUG
        printf("open file err,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
		return FAIL;
	}
	fwrite(ec,sizeof(EventClass_t),1,bfp);
	fwrite(el,sizeof(EventLog_t),1,bfp);
	fclose(bfp);

	//secondly,generate a text file;
	fp = fopen("./data/event.txt","wb+");
	if (NULL == fp)
	{
	#ifdef COMMON_DEBUG
		printf("open file err,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return FAIL;
	}
	*markbit &= 0xfdff;
	memset(buf,0,sizeof(buf));
	sprintf(buf,"%s   %s          %s               %s\n","流水号","事件类型","发生时间","事件描述");
	fputs(buf,fp);
	for (i = 1; i <= 20; i++)
	{//20 type event
		if (1 == i)
		{//software event
			for (j = 0;j < 255;j++)
			{
				if (0 == (el->EventLogList[j].EventLogId))
				{
					break;
				}
				et = el->EventLogList[j].EventLogTime;
				p = localtime(&et);
				memset(tbuf,0,sizeof(tbuf));
				sprintf(tbuf,"%d-%d-%d/%d:%d:%d",p->tm_year+1900,p->tm_mon+1,p->tm_mday, \
						p->tm_hour,p->tm_min,p->tm_sec);
				memset(buf,0,sizeof(buf));
				sprintf(buf,"  %d         %d          %s         %s\n",el->EventLogList[j].EventLogId, \
						el->EventLogList[j].EventClassId,tbuf,softevent[el->EventLogList[j].EventLogValue-1]);
				fputs(buf,fp);
			}
		}//software event;
		else if (2 == i)
		{//green conflict restore
			for (j = 0;j < 255;j++)
			{
				if (0 == (el->EventLogList[255+j].EventLogId))
				{
					break;
				}
				et = el->EventLogList[255+j].EventLogTime;
				p = localtime(&et);
				memset(tbuf,0,sizeof(tbuf));
				sprintf(tbuf,"%d-%d-%d/%d:%d:%d",p->tm_year+1900,p->tm_mon+1,p->tm_mday, \
						p->tm_hour,p->tm_min,p->tm_sec);
				memset(buf,0,sizeof(buf));
				sprintf(buf,"  %d         %d          %s         %s\n",el->EventLogList[255+j].EventLogId, \
                el->EventLogList[255+j].EventClassId,tbuf,"绿冲突恢复 （通道号无效）");
				fputs(buf,fp);
			}
		}//green conflict restore
		else if (3 == i)
		{//green conflict
			for (j = 0;j < 255;j++)
            {
                if (0 == (el->EventLogList[510+j].EventLogId))
                {
                    break;
                }
                et = el->EventLogList[510+j].EventLogTime;
                p = localtime(&et);
                memset(tbuf,0,sizeof(tbuf));
                sprintf(tbuf,"%d-%d-%d/%d:%d:%d",p->tm_year+1900,p->tm_mon+1,p->tm_mday, \
                        p->tm_hour,p->tm_min,p->tm_sec);
                memset(buf,0,sizeof(buf));
                sprintf(buf,"  %d         %d          %s         %s\n",el->EventLogList[510+j].EventLogId, \
                el->EventLogList[510+j].EventClassId,tbuf,"绿冲突 （通道号无效）");
                fputs(buf,fp);
            }
		}//green conflict
		else if (4 == i)
		{//红绿同亮恢复
			for (j = 0;j < 255;j++)
			{
				if (0 == (el->EventLogList[765+j].EventLogId))
				{
					break;
				}
				et = el->EventLogList[765+j].EventLogTime;
				p = localtime(&et);
				memset(tbuf,0,sizeof(tbuf));
				sprintf(tbuf,"%d-%d-%d/%d:%d:%d",p->tm_year+1900,p->tm_mon+1,p->tm_mday, \
						p->tm_hour,p->tm_min,p->tm_sec);
				memset(cbuf,0,sizeof(cbuf));
				sprintf(cbuf,"通道 %d 红绿同亮恢复",el->EventLogList[765+j].EventLogValue);
				memset(buf,0,sizeof(buf));
				sprintf(buf,"  %d         %d          %s         %s\n",el->EventLogList[765+j].EventLogId, \
						el->EventLogList[765+j].EventClassId,tbuf,cbuf);
				fputs(buf,fp);
			}
		}//红绿同亮恢复
		else if (5 == i)
		{//红灯不亮恢复
			for (j = 0;j < 255;j++)
			{
				if (0 == (el->EventLogList[1020+j].EventLogId))
				{
					break;
				}
				et = el->EventLogList[1020+j].EventLogTime;
				p = localtime(&et);
				memset(tbuf,0,sizeof(tbuf));
				sprintf(tbuf,"%d-%d-%d/%d:%d:%d",p->tm_year+1900,p->tm_mon+1,p->tm_mday, \
						p->tm_hour,p->tm_min,p->tm_sec);
				memset(cbuf,0,sizeof(cbuf));
				sprintf(cbuf,"通道 %d 红灯不亮恢复",el->EventLogList[1020+j].EventLogValue);
				memset(buf,0,sizeof(buf));
				sprintf(buf,"  %d         %d          %s         %s\n",el->EventLogList[1020+j].EventLogId, \
                        el->EventLogList[1020+j].EventClassId,tbuf,cbuf);
                fputs(buf,fp);
			}
		}//红灯不亮恢复
		else if (6 == i)
		{//红灯误亮恢复
			for (j = 0;j < 255;j++)
			{
				if (0 == (el->EventLogList[1275+j].EventLogId))
				{
					break;
				}
				et = el->EventLogList[1275+j].EventLogTime;
				p = localtime(&et);
				memset(tbuf,0,sizeof(tbuf));
				sprintf(tbuf,"%d-%d-%d/%d:%d:%d",p->tm_year+1900,p->tm_mon+1,p->tm_mday, \
						p->tm_hour,p->tm_min,p->tm_sec);
				memset(cbuf,0,sizeof(cbuf));
				sprintf(cbuf,"通道 %d 红灯误亮恢复",el->EventLogList[1275+j].EventLogValue);
				memset(buf,0,sizeof(buf));
				sprintf(buf,"  %d         %d          %s         %s\n",el->EventLogList[1275+j].EventLogId, \
						el->EventLogList[1275+j].EventClassId,tbuf,cbuf);
				fputs(buf,fp);
			}
		}//红灯误亮恢复
		else if (7 == i)
		{//黄灯不亮恢复
			for (j = 0;j < 255;j++)
			{
				if (0 == (el->EventLogList[1530+j].EventLogId))
				{
					break;
				}
				et = el->EventLogList[1530+j].EventLogTime;
				p = localtime(&et);
				memset(tbuf,0,sizeof(tbuf));
				sprintf(tbuf,"%d-%d-%d/%d:%d:%d",p->tm_year+1900,p->tm_mon+1,p->tm_mday, \
						p->tm_hour,p->tm_min,p->tm_sec);
				memset(cbuf,0,sizeof(cbuf));
				sprintf(cbuf,"通道 %d  黄灯不亮恢复",el->EventLogList[1530+j].EventLogValue);
				memset(buf,0,sizeof(buf));
				sprintf(buf,"  %d         %d          %s         %s\n",el->EventLogList[1530+j].EventLogId, \
						el->EventLogList[1530+j].EventClassId,tbuf,cbuf);
				fputs(buf,fp);
			}
		}//黄灯不亮恢复
		else if (8 == i)
		{//黄灯误亮恢复
			for (j = 0;j < 255;j++)
			{
				if (0 == (el->EventLogList[1785+j].EventLogId))
				{
					break;
				}
				et = el->EventLogList[1785+j].EventLogTime;
				p = localtime(&et);
				memset(tbuf,0,sizeof(tbuf));
				sprintf(tbuf,"%d-%d-%d/%d:%d:%d",p->tm_year+1900,p->tm_mon+1,p->tm_mday, \
						p->tm_hour,p->tm_min,p->tm_sec);
				memset(cbuf,0,sizeof(cbuf));
				sprintf(cbuf,"通道 %d 黄灯误亮恢复",el->EventLogList[1785+j].EventLogValue);
				memset(buf,0,sizeof(buf));
				sprintf(buf,"  %d         %d          %s         %s\n",el->EventLogList[1785+j].EventLogId, \
						el->EventLogList[1785+j].EventClassId,tbuf,cbuf);
				fputs(buf,fp);
			}
		}//黄灯误亮恢复
		else if (9 == i)
		{//绿灯不亮恢复
			for (j = 0;j < 255;j++)
            {
                if (0 == (el->EventLogList[2040+j].EventLogId))
                {
                    break;
                }
                et = el->EventLogList[2040+j].EventLogTime;
                p = localtime(&et);
                memset(tbuf,0,sizeof(tbuf));
                sprintf(tbuf,"%d-%d-%d/%d:%d:%d",p->tm_year+1900,p->tm_mon+1,p->tm_mday, \
                        p->tm_hour,p->tm_min,p->tm_sec);
                memset(cbuf,0,sizeof(cbuf));
                sprintf(cbuf,"通道 %d 绿灯不亮恢复",el->EventLogList[2040+j].EventLogValue);
                memset(buf,0,sizeof(buf));
                sprintf(buf,"  %d         %d          %s         %s\n",el->EventLogList[2040+j].EventLogId, \
                        el->EventLogList[2040+j].EventClassId,tbuf,cbuf);
                fputs(buf,fp);
            }
		}//绿灯不亮恢复
		else if (10 == i)
		{//绿灯误亮恢复
			for (j = 0;j < 255;j++)
            {
                if (0 == (el->EventLogList[2295+j].EventLogId))
                {
                    break;
                }
                et = el->EventLogList[2295+j].EventLogTime;
                p = localtime(&et);
                memset(tbuf,0,sizeof(tbuf));
                sprintf(tbuf,"%d-%d-%d/%d:%d:%d",p->tm_year+1900,p->tm_mon+1,p->tm_mday, \
                        p->tm_hour,p->tm_min,p->tm_sec);
                memset(cbuf,0,sizeof(cbuf));
                sprintf(cbuf,"通道 %d 绿灯误亮恢复",el->EventLogList[2295+j].EventLogValue);
                memset(buf,0,sizeof(buf));
                sprintf(buf,"  %d         %d          %s         %s\n",el->EventLogList[2295+j].EventLogId, \
                        el->EventLogList[2295+j].EventClassId,tbuf,cbuf);
                fputs(buf,fp);
            }
		}//绿灯误亮恢复
		else if (11 == i)
		{//红绿同亮
			for (j = 0;j < 255;j++)
			{
				if (0 == (el->EventLogList[2550+j].EventLogId))
				{
					break;
				}
				et = el->EventLogList[2550+j].EventLogTime;
				p = localtime(&et);
				memset(tbuf,0,sizeof(tbuf));
				sprintf(tbuf,"%d-%d-%d/%d:%d:%d",p->tm_year+1900,p->tm_mon+1,p->tm_mday, \
						p->tm_hour,p->tm_min,p->tm_sec);
				memset(cbuf,0,sizeof(cbuf));
				sprintf(cbuf,"通道 %d 红绿同亮",el->EventLogList[2550+j].EventLogValue);
				memset(buf,0,sizeof(buf));
				sprintf(buf,"  %d         %d          %s         %s\n",el->EventLogList[2550+j].EventLogId, \
						el->EventLogList[2550+j].EventClassId,tbuf,cbuf);
				fputs(buf,fp);
			}
		}//红绿同亮
		else if (12 == i)
		{//红灯不亮
			for (j = 0;j < 255;j++)
            {
                if (0 == (el->EventLogList[2805+j].EventLogId))
                {
                    break;
                }
                et = el->EventLogList[2805+j].EventLogTime;
                p = localtime(&et);
                memset(tbuf,0,sizeof(tbuf));
                sprintf(tbuf,"%d-%d-%d/%d:%d:%d",p->tm_year+1900,p->tm_mon+1,p->tm_mday, \
                        p->tm_hour,p->tm_min,p->tm_sec);
                memset(cbuf,0,sizeof(cbuf));
                sprintf(cbuf,"通道 %d 红灯不亮",el->EventLogList[2805+j].EventLogValue);
                memset(buf,0,sizeof(buf));
                sprintf(buf,"  %d         %d          %s         %s\n",el->EventLogList[2805+j].EventLogId, \
                        el->EventLogList[2805+j].EventClassId,tbuf,cbuf);
                fputs(buf,fp);
            }
		}//红灯不亮
		else if (13 == i)
		{//红灯误亮
			for (j = 0;j < 255;j++)
            {
                if (0 == (el->EventLogList[3060+j].EventLogId))
                {
                    break;
                }
                et = el->EventLogList[3060+j].EventLogTime;
                p = localtime(&et);
                memset(tbuf,0,sizeof(tbuf));
                sprintf(tbuf,"%d-%d-%d/%d:%d:%d",p->tm_year+1900,p->tm_mon+1,p->tm_mday, \
                        p->tm_hour,p->tm_min,p->tm_sec);
                memset(cbuf,0,sizeof(cbuf));
                sprintf(cbuf,"通道 %d 红灯误亮",el->EventLogList[3060+j].EventLogValue);
                memset(buf,0,sizeof(buf));
                sprintf(buf,"  %d         %d          %s         %s\n",el->EventLogList[3060+j].EventLogId, \
                        el->EventLogList[3060+j].EventClassId,tbuf,cbuf);
                fputs(buf,fp);
            }
		}//红灯误亮
		else if (14 == i)
		{//黄灯不亮
			for (j = 0;j < 255;j++)
            {
                if (0 == (el->EventLogList[3315+j].EventLogId))
                {
                    break;
                }
                et = el->EventLogList[3315+j].EventLogTime;
                p = localtime(&et);
                memset(tbuf,0,sizeof(tbuf));
                sprintf(tbuf,"%d-%d-%d/%d:%d:%d",p->tm_year+1900,p->tm_mon+1,p->tm_mday, \
                        p->tm_hour,p->tm_min,p->tm_sec);
                memset(cbuf,0,sizeof(cbuf));
                sprintf(cbuf,"通道 %d 黄灯不亮",el->EventLogList[3315+j].EventLogValue);
                memset(buf,0,sizeof(buf));
                sprintf(buf,"  %d         %d          %s         %s\n",el->EventLogList[3315+j].EventLogId, \
                        el->EventLogList[3315+j].EventClassId,tbuf,cbuf);
                fputs(buf,fp);
            }
		}//黄灯不亮
		else if (15 == i)
		{//黄灯误亮
			for (j = 0;j < 255;j++)
            {
                if (0 == (el->EventLogList[3570+j].EventLogId))
                {
                    break;
                }
                et = el->EventLogList[3570+j].EventLogTime;
                p = localtime(&et);
                memset(tbuf,0,sizeof(tbuf));
                sprintf(tbuf,"%d-%d-%d/%d:%d:%d",p->tm_year+1900,p->tm_mon+1,p->tm_mday, \
                        p->tm_hour,p->tm_min,p->tm_sec);
                memset(cbuf,0,sizeof(cbuf));
                sprintf(cbuf,"通道 %d 黄灯误亮",el->EventLogList[3570+j].EventLogValue);
                memset(buf,0,sizeof(buf));
                sprintf(buf,"  %d         %d          %s         %s\n",el->EventLogList[3570+j].EventLogId, \
                        el->EventLogList[3570+j].EventClassId,tbuf,cbuf);
                fputs(buf,fp);
            }
		}//黄灯误亮
		else if (16 == i)
		{//绿灯不亮
			for (j = 0;j < 255;j++)
            {
                if (0 == (el->EventLogList[3825+j].EventLogId))
                {
                    break;
                }
                et = el->EventLogList[3825+j].EventLogTime;
                p = localtime(&et);
                memset(tbuf,0,sizeof(tbuf));
                sprintf(tbuf,"%d-%d-%d/%d:%d:%d",p->tm_year+1900,p->tm_mon+1,p->tm_mday, \
                        p->tm_hour,p->tm_min,p->tm_sec);
                memset(cbuf,0,sizeof(cbuf));
                sprintf(cbuf,"通道 %d 绿灯不亮",el->EventLogList[3825+j].EventLogValue);
                memset(buf,0,sizeof(buf));
                sprintf(buf,"  %d         %d          %s         %s\n",el->EventLogList[3825+j].EventLogId, \
                        el->EventLogList[3825+j].EventClassId,tbuf,cbuf);
                fputs(buf,fp);
            }
		}//绿灯不亮
		else if (17 == i)
		{//绿灯误亮
			for (j = 0;j < 255;j++)
            {
                if (0 == (el->EventLogList[4080+j].EventLogId))
                {
                    break;
                }
                et = el->EventLogList[4080+j].EventLogTime;
                p = localtime(&et);
                memset(tbuf,0,sizeof(tbuf));
                sprintf(tbuf,"%d-%d-%d/%d:%d:%d",p->tm_year+1900,p->tm_mon+1,p->tm_mday, \
                        p->tm_hour,p->tm_min,p->tm_sec);
                memset(cbuf,0,sizeof(cbuf));
                sprintf(cbuf,"通道 %d 绿灯误亮",el->EventLogList[4080+j].EventLogValue);
                memset(buf,0,sizeof(buf));
                sprintf(buf,"  %d         %d          %s         %s\n",el->EventLogList[4080+j].EventLogId, \
                        el->EventLogList[4080+j].EventClassId,tbuf,cbuf);
                fputs(buf,fp);
            }
		}//绿灯误亮
		else if (18 == i)
		{//CAN 通讯事件
			for (j = 0; j < 255; j++)
			{
				if (0 == (el->EventLogList[4335+j].EventLogId))
				{
					break;
				}
				et = el->EventLogList[4335+j].EventLogTime;
				p = localtime(&et);
				memset(tbuf,0,sizeof(tbuf));
				sprintf(tbuf,"%d-%d-%d/%d:%d:%d",p->tm_year+1900,p->tm_mon+1,p->tm_mday, \
						p->tm_hour,p->tm_min,p->tm_sec);
				unsigned char				telid = 0;
				telid = el->EventLogList[4335+j].EventLogValue/*EventLogId*/;
				memset(cbuf,0,sizeof(cbuf));
				if(33 == telid)
				{//main board err
					sprintf(cbuf,"%s","主控板故障");
				}//main board err
				else if(34 == telid)
				{//main board normal
					sprintf(cbuf,"%s","主控板正常");
				}//main board normal
				else if ((1 <= telid) && (telid <= 16))
				{//drive board err
					sprintf(cbuf,"驱动板 %d 故障",telid);	
				}//drive board err
				else if((17 <= telid) && (telid <= 32))
				{//drive board normal
					sprintf(cbuf,"驱动板 %d 正常",telid-16);
				}//drive board normal
				memset(buf,0,sizeof(buf));
				sprintf(buf,"  %d         %d          %s         %s\n",el->EventLogList[4335+j].EventLogId, \
                        el->EventLogList[4335+j].EventClassId,tbuf,cbuf);
				fputs(buf,fp);
			}
		}//CAN 通讯事件
		else if (19 == i)
		{//驱动板事件
			for (j = 0; j < 255; j++)
			{
				if (0 == (el->EventLogList[4590+j].EventLogId))
				{
					break;
				}
				et = el->EventLogList[4590+j].EventLogTime;
				p = localtime(&et);
				memset(tbuf,0,sizeof(tbuf));
				sprintf(tbuf,"%d-%d-%d/%d:%d:%d",p->tm_year+1900,p->tm_mon+1,p->tm_mday, \
                        p->tm_hour,p->tm_min,p->tm_sec);
				unsigned char               telid = 0;
                telid = el->EventLogList[4590+j].EventLogValue/*EventLogId*/;
				memset(cbuf,0,sizeof(cbuf));
				if ((telid >= 1) && (telid <= 16))
				{//drive board do not install;
					sprintf(cbuf,"驱动板 %d 未安装",telid);
				}//drive board do not install;
				else if ((telid >= 17) && (telid <= 32))
				{//drive board start
					sprintf(cbuf,"驱动板 %d 启动",telid-16);
				}//drive board start
				else if ((telid >= 33) && (telid <= 48))
				{//drive board normal
					sprintf(cbuf,"驱动板 %d 正常",telid-32);
				}//drive board normal
				else if ((telid >= 49) && (telid <= 64))
				{//drive board err yellow falsh
					sprintf(cbuf,"驱动板 %d 故障黄闪",telid-48);
				}//drive board err yellow falsh
				else if ((telid >= 65) && (telid <= 80))
				{//drive board single yellow flash (close)
					sprintf(cbuf,"驱动板 %d 独立黄闪（灭）",telid-64);
				}//drive board single yellow flash (close)
				else if ((telid >= 81) && (telid <= 96))
				{//drive board single yellow flash (control)
					sprintf(cbuf,"驱动板 %d 独立黄闪（控制）",telid-80);
				}//drive board single yellow flash (control)
				memset(buf,0,sizeof(buf));
				sprintf(buf,"  %d         %d          %s         %s\n",el->EventLogList[4590+j].EventLogId, \
                        el->EventLogList[4590+j].EventClassId,tbuf,cbuf);
                fputs(buf,fp); 
			}
		}//驱动板事件
		else if (20 == i)
		{//车辆检测器故障
			for (j = 0;j < 255;j++)
            {
                if (0 == (el->EventLogList[4845+j].EventLogId))
                {
                    break;
                }
                et = el->EventLogList[4845+j].EventLogTime;
                p = localtime(&et);
                memset(tbuf,0,sizeof(tbuf));
                sprintf(tbuf,"%d-%d-%d/%d:%d:%d",p->tm_year+1900,p->tm_mon+1,p->tm_mday, \
                        p->tm_hour,p->tm_min,p->tm_sec);
                memset(cbuf,0,sizeof(cbuf));
                sprintf(cbuf,"检测器 %d 故障",el->EventLogList[4845+j].EventLogValue);
                memset(buf,0,sizeof(buf));
                sprintf(buf,"  %d         %d          %s         %s\n",el->EventLogList[4845+j].EventLogId, \
                        el->EventLogList[4845+j].EventClassId,tbuf,cbuf);
                fputs(buf,fp);
            }
		}//车辆检测器故障
		else
		{
		#ifdef COMMON_DEBUG
			printf("Not fit event type,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			continue;
		}
	}//20 type event;
	fclose(fp);

	return SUCCESS;
}

/*Get current time of system*/
struct tm *get_system_time(void)
{
	time_t				s;

	s = time((time_t *)NULL);
	return localtime(&s);
}

/*Modify the pointed content of text file*/
void modify_content_value(char *filename,char *str,int value)
{
	if ((NULL == filename) || (NULL == str))
	{
	#ifdef COMMON_DEBUG
		printf("pointer address error,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return;
	}

	FILE				*fp = NULL;
	char				msg[50] = {0};
	char				item[40] = {0};
	long				offset = 0;

	fp = fopen(filename,"r+");
	if (NULL == fp)
	{
	#ifdef COMMON_DEBUG
		printf("Open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return;
	}

	while (NULL != fgets(msg,sizeof(msg),fp))
	{//0
		sscanf(msg,"%s%d",item,&value);
		if (0 != strcmp(str,item))
		{
			offset += strlen(msg);
		}
		else
		{//1
			fseek(fp,offset,SEEK_SET);
			memset(msg,0,sizeof(msg));
			sprintf(msg,"%s %d",item,value);
			fputs(msg,fp);
			break;
		}//1
	}//0
	fclose(fp);
	return;
}

/*output debug message to pointed file;*/
void output_log(char *str)
{
	if (NULL == str)
	{
	#ifdef COMMON_DEBUG
		printf("pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return;
	}

	time_t				s;
	struct tm			*p;
	FILE				*fp;

	fp = fopen(MSGTXT,"a+");
	if (NULL == fp)
	{
	#ifdef COMMON_DEBUG
		printf("open file error,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return;
	}
	
	s = time((time_t*)NULL);
	p = localtime(&s);
	fprintf(fp,"%d-%d-%d %d:%d:%d : %s\n",1900+p->tm_year,1+p->tm_mon,p->tm_mday, \
			p->tm_hour,p->tm_min,p->tm_sec,str);
	fclose(fp);
}

#ifdef FLASH_DEBUG
/**********************************EventLog***********************************/
/*
* 函数原型：tsc_save_eventlog
* 功能描述：保存运行事件日志
* 输入参数：pType：事件日志类型指针
          nFlag：事件日志内容标识
          pInfo：事件日志内容指针
* 输出参数：无
* 返 回 值：无
*/
void tsc_save_eventlog(char *pType, char *pInfo)
{
  time_t tmsec;
  time(&tmsec);
  struct tm *pcurtime;
  pcurtime = localtime(&tmsec);

  FILE *pf;
  struct stat statbuf;
  char strbuf[255] = {0};

  if (stat("./tsc_yellowlog.txt",&statbuf) < 0)
    pf = fopen("./tsc_yellowlog.txt","a");
  else
  {
    if (statbuf.st_size > 2*1024*1024)
      pf = fopen("./tsc_yellowlog.txt","w");
    else
      pf = fopen("./tsc_yellowlog.txt","a");
  }
  if (pf)
  {
      sprintf(strbuf,"%04d-%02d-%02d %02d:%02d:%02d\t[%s]\t%s\n",pcurtime->tm_year+1900,pcurtime->tm_mon+1,pcurtime->tm_mday,pcurtime->tm_hour,pcurtime->tm_min,pcurtime->tm_sec,pType,pInfo);
    if (fputs(strbuf,pf) < 0)
      perror("fputs[EventLog]");
    fclose(pf);
  }
  else
    perror("fopen[EventLog]");
}
#endif

/*wait serial until it can be written*/
int wait_write_serial(int serial)
{
	fd_set				WriteFd;
	int					ret;

	FD_ZERO(&WriteFd);
	FD_SET(serial,&WriteFd);
	ret = select(serial+1,NULL,&WriteFd,NULL,NULL);
	if (ret < 0)
	{
	#ifdef COMMON_DEBUG
		printf("serial is not writeable,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return FAIL;
	}
	if (ret > 0)
	{
		if (FD_ISSET(serial,&WriteFd))
		{
			return SUCCESS;
		}
		else
		{
			return FAIL;
		}
	}
}

/*get phase ids through phase num*/
int get_phase_ids(unsigned int	Iphaseid,unsigned char *phaseid)
{
	if (NULL == phaseid)
	{
	#ifdef COMMON_DEBUG
		printf("pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	
	int					i = 0;
	unsigned char		*bakpid = phaseid;

	for (i = 0; i < 32; i++)
	{
#if 0
		if (31 == i)
		{
			if (Iphaseid & 0x80000000)
			{
				*bakpid = i + 1;
				bakpid++;
			}
			break;
		}
#endif
		if (Iphaseid & (0x01 << i))
		{
			*bakpid = i + 1;
			bakpid++;
		}
		else
		{
			continue;
		}
	}
	return SUCCESS;
}

/*get single phase id throuth phase num*/
int get_phase_id(unsigned int Iphaseid,unsigned char *phaseid)
{
	if (NULL == phaseid)
	{
	#ifdef COMMON_DEBUG
		printf("pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	int 				i = 0;
	
	for (i = 0; i < 32; i++)
	{
#if 0
		if (31 == i)
		{
			if (Iphaseid & 0x80000000)
			{
				*phaseid = i + 1;
			}
			break;
		}
#endif
		if (Iphaseid & (0x01 << i))
		{
			*phaseid = i + 1;
			break;
		}
		else
		{
			continue;
		}
	}
	return SUCCESS;
}

/*return 0 if success,or return -1 or -2*/
int find_out_child_str(const char *source,const char *childstr,int *pos)
{
	if ((NULL == source) || (NULL == childstr) || (NULL == pos))
	{
	#ifdef COMMON_DEBUG
		printf("pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	const char			*s = source;
	const char			*c = childstr;
	unsigned char		mark = 0;
	*pos = 0;

	while (1)
	{//0
		if (*source == *childstr)
		{
			s = source;
			c = childstr;
			mark = 0;
			while (1)
			{//1
				s++;
				c++;
				if (*c == '\0')
				{
					break;
				}
				if (*s != *c)
				{
					source++;
					*pos += 1;
					mark = 1;
					c = childstr;
					break;
				}	
			}//1
			if (0 == mark)
			{
				return SUCCESS;
			}
		}
		else
		{
			mark = 1;
			source++;
			*pos += 1;
			if (*source == '\0')
			{
				break;
			}
		}
	}//0	

	if (1 == mark)
		return FAIL;
	else
		return SUCCESS;
}

int update_channel_status(sockfd_t *sockfd,ChannelStatus_t *cs,unsigned char *chanid,unsigned char mark,unsigned short *markbit)
{
	if ((NULL == cs) || (NULL == chanid) || (NULL == sockfd) || (NULL == markbit))
	{
	#ifdef COMMON_DEBUG
		printf("cs is NULL or chanid is 0,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	int						i = 0;
	unsigned char			data[9] = {0};

	data[0] = 'C';
	data[1] = 'Y';
	data[2] = 'T';
	data[3] = '1';
	data[5] = mark;
	data[6] = 'E';
	data[7] = 'N';
	data[8] = 'D';
	for (i = 0; i < MAX_CHANNEL; i++ )
	{
		if (0 == chanid[i])
			break;

		if ((*markbit & 0x0010) && (*markbit & 0x0002))
        {//start monitor
            data[4] = chanid[i];
            if (write(*(sockfd->ssockfd),data,sizeof(data)) < 0)
            {
            #ifdef COMMON_DEBUG
                printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
            #endif
            }
        }//start monitor
		#ifdef EMBED_CONFIGURE_TOOL
		data[4] = chanid[i];
		write(*(sockfd->lhsockfd),data,sizeof(data));
		#endif
		if ((chanid[i] >= 1) && (chanid[i] <= 8))
		{
			if (3 == mark)
			{
				cs->ChannelStatusList[0].ChannelStatusReds &= (~(0x01 << (chanid[i] - 1)));
				cs->ChannelStatusList[0].ChannelStatusYellows &= (~(0x01 << (chanid[i] - 1)));
				cs->ChannelStatusList[0].ChannelStatusGreens &= (~(0x01 << (chanid[i] - 1)));
			}
			if (0 == mark)
			{
				cs->ChannelStatusList[0].ChannelStatusReds |= (0x01 << (chanid[i] - 1));
				cs->ChannelStatusList[0].ChannelStatusYellows &= (~(0x01 << (chanid[i] - 1)));
				cs->ChannelStatusList[0].ChannelStatusGreens &= (~(0x01 << (chanid[i] - 1)));
			}
			if (1 == mark)
			{
				cs->ChannelStatusList[0].ChannelStatusReds &= (~(0x01 << (chanid[i] - 1)));
				cs->ChannelStatusList[0].ChannelStatusYellows |= (0x01 << (chanid[i] - 1));
				cs->ChannelStatusList[0].ChannelStatusGreens &= (~(0x01 << (chanid[i] - 1)));
			}
			if (2 == mark)
			{
				cs->ChannelStatusList[0].ChannelStatusReds &= (~(0x01 << (chanid[i] - 1)));
				cs->ChannelStatusList[0].ChannelStatusYellows &= (~(0x01 << (chanid[i] - 1)));
				cs->ChannelStatusList[0].ChannelStatusGreens |= (0x01 << (chanid[i] - 1));
			}
		}
		if ((chanid[i] >= 9) && (chanid[i] <= 16))
		{
			if (3 == mark)
        	{
            	cs->ChannelStatusList[1].ChannelStatusReds &= (~(0x01 << (chanid[i] - 9)));
            	cs->ChannelStatusList[1].ChannelStatusYellows &= (~(0x01 << (chanid[i] - 9)));
            	cs->ChannelStatusList[1].ChannelStatusGreens &= (~(0x01 << (chanid[i] - 9)));
        	}
        	if (0 == mark)
        	{
            	cs->ChannelStatusList[1].ChannelStatusReds |= (0x01 << (chanid[i] - 9));
            	cs->ChannelStatusList[1].ChannelStatusYellows &= (~(0x01 << (chanid[i] - 9)));
            	cs->ChannelStatusList[1].ChannelStatusGreens &= (~(0x01 << (chanid[i] - 9)));
        	}
        	if (1 == mark)
        	{
            	cs->ChannelStatusList[1].ChannelStatusReds &= (~(0x01 << (chanid[i] - 9)));
            	cs->ChannelStatusList[1].ChannelStatusYellows |= (0x01 << (chanid[i] - 9));
            	cs->ChannelStatusList[1].ChannelStatusGreens &= (~(0x01 << (chanid[i] - 9)));
        	}
        	if (2 == mark)
        	{
            	cs->ChannelStatusList[1].ChannelStatusReds &= (~(0x01 << (chanid[i] - 9)));
            	cs->ChannelStatusList[1].ChannelStatusYellows &= (~(0x01 << (chanid[i] - 9)));
            	cs->ChannelStatusList[1].ChannelStatusGreens |= (0x01 << (chanid[i] - 9));
        	}
		}
		if ((chanid[i] >= 17) && (chanid[i] <= 24))
		{
			if (3 == mark)
        	{
            	cs->ChannelStatusList[2].ChannelStatusReds &= (~(0x01 << (chanid[i] - 17)));
            	cs->ChannelStatusList[2].ChannelStatusYellows &= (~(0x01 << (chanid[i] - 17)));
            	cs->ChannelStatusList[2].ChannelStatusGreens &= (~(0x01 << (chanid[i] - 17)));
        	}
        	if (0 == mark)
        	{
            	cs->ChannelStatusList[2].ChannelStatusReds |= (0x01 << (chanid[i] - 17));
            	cs->ChannelStatusList[2].ChannelStatusYellows &= (~(0x01 << (chanid[i] - 17)));
            	cs->ChannelStatusList[2].ChannelStatusGreens &= (~(0x01 << (chanid[i] - 17)));
        	}
        	if (1 == mark)
        	{
            	cs->ChannelStatusList[2].ChannelStatusReds &= (~(0x01 << (chanid[i] - 17)));
            	cs->ChannelStatusList[2].ChannelStatusYellows |= (0x01 << (chanid[i] - 17));
            	cs->ChannelStatusList[2].ChannelStatusGreens &= (~(0x01 << (chanid[i] - 17)));
        	}
        	if (2 == mark)
        	{
            	cs->ChannelStatusList[2].ChannelStatusReds &= (~(0x01 << (chanid[i] - 17)));
            	cs->ChannelStatusList[2].ChannelStatusYellows &= (~(0x01 << (chanid[i] - 17)));
            	cs->ChannelStatusList[2].ChannelStatusGreens |= (0x01 << (chanid[i] - 17));
        	}
		}
		if ((chanid[i] >= 25) && (chanid[i] <= 32))
		{
			if (3 == mark)
        	{
            	cs->ChannelStatusList[3].ChannelStatusReds &= (~(0x01 << (chanid[i] - 25)));
            	cs->ChannelStatusList[3].ChannelStatusYellows &= (~(0x01 << (chanid[i] - 25)));
            	cs->ChannelStatusList[3].ChannelStatusGreens &= (~(0x01 << (chanid[i] - 25)));
        	}
        	if (0 == mark)
        	{
            	cs->ChannelStatusList[3].ChannelStatusReds |= (0x01 << (chanid[i] - 25));
            	cs->ChannelStatusList[3].ChannelStatusYellows &= (~(0x01 << (chanid[i] - 25)));
            	cs->ChannelStatusList[3].ChannelStatusGreens &= (~(0x01 << (chanid[i] - 25)));
        	}
        	if (1 == mark)
        	{
            	cs->ChannelStatusList[3].ChannelStatusReds &= (~(0x01 << (chanid[i] - 25)));
            	cs->ChannelStatusList[3].ChannelStatusYellows |= (0x01 << (chanid[i] - 25));
            	cs->ChannelStatusList[3].ChannelStatusGreens &= (~(0x01 << (chanid[i] - 25)));
        	}
        	if (2 == mark)
        	{
            	cs->ChannelStatusList[3].ChannelStatusReds &= (~(0x01 << (chanid[i] - 25)));
            	cs->ChannelStatusList[3].ChannelStatusYellows &= (~(0x01 << (chanid[i] - 25)));
            	cs->ChannelStatusList[3].ChannelStatusGreens |= (0x01 << (chanid[i] - 25));
        	}
		}
	}//for loop
	return SUCCESS;
}

int update_lamp_status(lamperrtype_t *let,unsigned char chanid,unsigned char etype, \
						unsigned emark,unsigned short *markbit)
{
	if (NULL == let)
	{
	#ifdef COMMON_DEBUG
		printf("pointer address err,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char				i = 0;

	for (i = 0; i < MAX_CHANNEL_EX; i++)
	{//for (i = 0; i < MAX_CHANNEL_EX; i++)
		if (0 == (let->lamperrList[i].chanid))
			break;
		if (1 == etype)//green conflict do not send configure tool
			break;
		if (chanid == (let->lamperrList[i].chanid))
		{//if (chanid == (let->lamperrList[i].chanid))
			if (0 == emark)
			{//restore
/*
				if (1 == etype)
				{//green lamp conflict
					let->lamperrList[i].etype = 2;
				}//green lamp conflict
*/
				if (2 == etype)
				{//red and green are light
					let->lamperrList[i].etype = 4;
				}//red and green are light
				if (3 == etype)
				{//red lamp not light
					let->lamperrList[i].etype = 5;
				}//red lamp not light
				if (4 == etype)
                {//red lamp err light
                    let->lamperrList[i].etype = 6;
                }//red lamp err light
				if (5 == etype)
                {//yellow lamp not light
                    let->lamperrList[i].etype = 7;
                }//yellow lamp not light
				if (6 == etype)
                {//yellow lamp err light
                    let->lamperrList[i].etype = 8;
                }//yellow lamp err light
				if (7 == etype)
                {//green lamp not light
                    let->lamperrList[i].etype = 9;
                }//green lamp not light
                if (8 == etype)
                {//green lamp err light
                    let->lamperrList[i].etype = 10;
                }//green lamp err light
			}//restore
			if (1 == emark)
			{//err
/*
				if (1 == etype)
				{//green lamp conflict
					let->lamperrList[i].etype = 3;
				}//green lamp conflict
*/
				if (2 == etype)
				{//red and green are light
					let->lamperrList[i].etype = 11;
				}//red and green are light
				if (3 == etype)
				{//red lamp not light
					let->lamperrList[i].etype = 12;
				}//red lamp not light
				if (4 == etype)
                {//red lamp err light
                    let->lamperrList[i].etype = 13;
                }//red lamp err light
				if (5 == etype)
                {//yellow lamp not light
                    let->lamperrList[i].etype = 14;
                }//yellow lamp not light
				if (6 == etype)
                {//yellow lamp err light
                    let->lamperrList[i].etype = 15;
                }//yellow lamp err light
				if (7 == etype)
                {//green lamp not light
                    let->lamperrList[i].etype = 16;
                }//green lamp not light
                if (8 == etype)
                {//green lamp err light
                    let->lamperrList[i].etype = 17;
                }//green lamp err light
			}//err
			break;
		}//if (chanid == (let->lamperrList[i].chanid))
	}//for (i = 0; i < MAX_CHANNEL_EX; i++)
	*markbit |= 0x2000;

	return SUCCESS;
}

int update_event_list(EventClass_t *ec,EventLog_t *el,unsigned char ecid,unsigned char elv,unsigned int time ,
						unsigned short *markbit)	
{
	if ((NULL == ec) || (NULL == el) || (NULL == markbit))
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char				factecn = 0;
	unsigned char				i = 0;

	for (i = 0; i < MAX_EVENTCLASS_LINE; i++)
	{
		if (ecid == (ec->EventClassList[i].EventClassId))
		{
			if ((ec->EventClassList[i].EventClassRowNum) >= 255)
				ec->EventClassList[i].EventClassRowNum = 1;
			else
				ec->EventClassList[i].EventClassRowNum += 1;
			break;
		}
	}
	if (i == MAX_EVENTCLASS_LINE)
	{
	#ifdef COMMON_DEBUG
		printf("Not have valid event class ID,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return FAIL;
	}
	factecn = ec->EventClassList[i].EventClassRowNum;
	el->EventLogList[(ecid-1)*255+factecn-1].EventClassId = ecid; 
	el->EventLogList[(ecid-1)*255+factecn-1].EventLogId = factecn; //1~255
	el->EventLogList[(ecid-1)*255+factecn-1].EventLogTime = time;
	el->EventLogList[(ecid-1)*255+factecn-1].EventLogValue = elv;	
	*markbit |= 0x0200;
	return SUCCESS;
}

#if 0
int all_red(int serial,unsigned char second)
{
	unsigned char				data[4] = {0};
	
	data[0] = 0xAA;
	data[1] = 0xFF;
	data[2] = 0x00;
	data[3] = 0xED;

	if (!wait_write_serial(serial))
	{
		if (write(serial,data,sizeof(data)) < 0)
		{
		#ifdef COMMON_DEBUG
			printf("write serial error,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
	}
	else
	{
	#ifdef COMMON_DEBUG
		printf("serial can not be written,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return FAIL;
	}

	if (second > 0)
		sleep(second);	
	
	return SUCCESS;	
}
#endif

//all close
int new_all_close(yfdata_t *acdata)
{
	if (NULL == acdata)
	{
	#ifdef COMMON_DEBUG
		printf("pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	unsigned char			data[4] = {0xAA,0xFF,0x03,0xED};
	unsigned char			ctdata[8] = {'C','Y','T','F',0x03,'E','N','D'};
	unsigned char			i = 0;

	if (!wait_write_serial(acdata->serial))
    {
        if (write(acdata->serial,data,sizeof(data)) < 0)
        {
        #ifdef COMMON_DEBUG
            printf("write serial error,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
			*(acdata->markbit) |= 0x0800;
        }
    }
    else
    {
    #ifdef COMMON_DEBUG
        printf("serial can not be written,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
    }

	if ((*(acdata->markbit) & 0x0010) && (*(acdata->markbit) & 0x0002))
	{
		if (write(*(acdata->sockfd->ssockfd),ctdata,sizeof(ctdata)) < 0)
        {
        #ifdef COMMON_DEBUG
            printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
        #endif
        }
	}
	#ifdef EMBED_CONFIGURE_TOOL
	if ((*(acdata->markbit2) & 0x0200))
    {
        if (write(*(acdata->sockfd->lhsockfd),ctdata,sizeof(ctdata)) < 0)
        {
        #ifdef COMMON_DEBUG
            printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
        #endif
        }
    }
	#endif
	for (i = 0; i < MAX_CHANNEL_STATUS; i++)
    {
        acdata->cs->ChannelStatusList[i].ChannelStatusReds = 0;
        acdata->cs->ChannelStatusList[i].ChannelStatusYellows = 0;
        acdata->cs->ChannelStatusList[i].ChannelStatusGreens = 0;
    }
	
	if ((acdata->second) > 0)
		sleep(acdata->second);

	return SUCCESS;
}

//all red
int new_all_red(yfdata_t *ardata)
{
	if (NULL == ardata)
	{
	#ifdef COMMON_DEBUG
		printf("pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	unsigned char				data[4] = {0xAA,0xFF,0x00,0xED};
	unsigned char				ctdata[8] = {'C','Y','T','F',0x00,'E','N','D'};
	unsigned char				i = 0;

	if (!wait_write_serial(ardata->serial))
    {
        if (write(ardata->serial,data,sizeof(data)) < 0)
        {
        #ifdef COMMON_DEBUG
            printf("write serial error,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
			*(ardata->markbit) |= 0x0800;
        }
    }
    else
    {
    #ifdef COMMON_DEBUG
        printf("serial can not be written,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
    }

	if ((*(ardata->markbit) & 0x0010) && (*(ardata->markbit) & 0x0002))
	{
		if (write(*(ardata->sockfd->ssockfd),ctdata,sizeof(ctdata)) < 0)
        {
        #ifdef COMMON_DEBUG
            printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
        #endif
        }
	}
	#ifdef EMBED_CONFIGURE_TOOL
	if ((*(ardata->markbit2) & 0x0200))
    {
        if (write(*(ardata->sockfd->lhsockfd),ctdata,sizeof(ctdata)) < 0)
        {
        #ifdef COMMON_DEBUG
            printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
        #endif
        }
    }
	#endif
	for (i = 0; i < MAX_CHANNEL_STATUS; i++)
    {
        ardata->cs->ChannelStatusList[i].ChannelStatusReds = 0xff;
        ardata->cs->ChannelStatusList[i].ChannelStatusYellows = 0;
        ardata->cs->ChannelStatusList[i].ChannelStatusGreens = 0;
    }

	if ((ardata->second) > 0)
		sleep(ardata->second);

	return SUCCESS;
}

//yellow flash will be limitless if yfdata->second is 0
int new_yellow_flash(yfdata_t *yfdata)
{
	if (NULL == yfdata)
	{
	#ifdef COMMON_DEBUG
		printf("pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
#ifdef FLASH_DEBUG
	char szInfo[32] = {0};
	char szInfoT[64] = {0};
	snprintf(szInfo,sizeof(szInfo)-1,"%s",__FUNCTION__);
	snprintf(szInfoT,sizeof(szInfoT)-1,"%d",__LINE__);
	tsc_save_eventlog(szInfo,szInfoT);
#endif

	unsigned char				data[4] = {0};
	unsigned char				ctdata[8] = {0};
	unsigned char				num = 0;
	struct timeval				timeout;
	unsigned char				i = 0;

	data[0] = 0xAA;
	data[1] = 0xFF;
	data[3] = 0xED;
	ctdata[0] = 'C';
	ctdata[1] = 'Y';
	ctdata[2] = 'T';
	ctdata[3] = 'F';
    ctdata[5] = 'E';
    ctdata[6] = 'N';
    ctdata[7] = 'D';
	
	unsigned char               fbdata[6] = {0};
	fbdata[0] = 0xC1;
	fbdata[5] = 0xED;
	fbdata[1] = 2;
	fbdata[2] = 0;
	fbdata[3] = 0x01;
	fbdata[4] = 0;

	if (!wait_write_serial(yfdata->serial))
	{
		if (write(yfdata->serial,fbdata,6) < 0)
		{
		#ifdef MAIN_DEBUG
			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			*(yfdata->markbit) |= 0x0800;
		}
	}
	else
	{
	#ifdef MAIN_DEBUG
		printf("cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
	}
	while (1)
	{//while loop
		data[2] = 0x03;
		if (!wait_write_serial(yfdata->serial))
		{
			if (write(yfdata->serial,data,sizeof(data)) < 0)
			{
			#ifdef COMMON_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				*(yfdata->markbit) |= 0x0800;
			}
		}
		else
		{
		#ifdef COMMON_DEBUG
			printf("cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		}
		if ((*(yfdata->markbit) & 0x0010) && (*(yfdata->markbit) & 0x0002))
        {//start monitor
           	ctdata[4] = 3;
            if (write(*(yfdata->sockfd->ssockfd),ctdata,sizeof(ctdata)) < 0)
            {
            #ifdef COMMON_DEBUG
               	printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
            #endif
            }
        }//start monitor
		#ifdef EMBED_CONFIGURE_TOOL
		if ((*(yfdata->markbit2) & 0x0200))
        {//start monitor
            ctdata[4] = 3;
            if (write(*(yfdata->sockfd->lhsockfd),ctdata,sizeof(ctdata)) < 0)
            {
            #ifdef COMMON_DEBUG
                printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
            #endif
            }
        }//start monitor
		#endif
		for (i = 0; i < MAX_CHANNEL_STATUS; i++)
		{
			yfdata->cs->ChannelStatusList[i].ChannelStatusReds = 0;
			yfdata->cs->ChannelStatusList[i].ChannelStatusYellows = 0;
			yfdata->cs->ChannelStatusList[i].ChannelStatusGreens = 0;	
		}
		
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;
		select(0,NULL,NULL,NULL,&timeout);

		data[2] = 0x01;
        if (!wait_write_serial(yfdata->serial))
        {
            if (write(yfdata->serial,data,sizeof(data)) < 0)
            {
            #ifdef COMMON_DEBUG
                printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
            #endif
				*(yfdata->markbit) |= 0x0800;
            }
        }
        else
        {
        #ifdef COMMON_DEBUG
            printf("cannot write,File: %s,Line: %d\n",__FILE__,__LINE__);
        #endif
        }
		if ((*(yfdata->markbit) & 0x0010) && (*(yfdata->markbit) & 0x0002))
        {//start monitor
            ctdata[4] = 1;
            if (write(*(yfdata->sockfd->ssockfd),ctdata,sizeof(ctdata)) < 0)
            {
            #ifdef COMMON_DEBUG
                printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
            #endif
            }
        }//start monitor
		#ifdef EMBED_CONFIGURE_TOOL
		if ((*(yfdata->markbit2) & 0x0200))
        {//start monitor
            ctdata[4] = 1;
            if (write(*(yfdata->sockfd->lhsockfd),ctdata,sizeof(ctdata)) < 0)
            {
            #ifdef COMMON_DEBUG
                printf("send data err,File:%s,Line:%d\n",__FILE__,__LINE__);
            #endif
            }
        }//start monitor
		#endif
        for (i = 0; i < MAX_CHANNEL_STATUS; i++)
        {
            yfdata->cs->ChannelStatusList[i].ChannelStatusReds = 0;
            yfdata->cs->ChannelStatusList[i].ChannelStatusYellows = 0xff;
            yfdata->cs->ChannelStatusList[i].ChannelStatusGreens = 0;
        }
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;
        select(0,NULL,NULL,NULL,&timeout);
		
		if ((yfdata->second) > 0)
		{
			num += 1;
			if (num >= (yfdata->second))
				break;
		}
	}//while loop

	return SUCCESS;
}

#if 0
/*yellow flash will be limitless if 'second' is 0*/
int all_yellow_flash(int serial,unsigned char second)
{
	unsigned char				data[4] = {0};
	unsigned char				num = 0;
	struct timeval				timeout;

	data[0] = 0xAA;
	data[1] = 0xFF;
	data[3] = 0xED;	
	while(1)
	{
		data[2] = 0x03; //turn off
		if (!wait_write_serial(serial))
		{
			if (write(serial,data,sizeof(data)) < 0)
			{
			#ifdef COMMON_DEBUG
				printf("write serial error,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
				return FAIL;	
			}		
		}
		else
		{
		#ifdef COMMON_DEBUG
			printf("the serial can not be written,File:%s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;
		select(0,NULL,NULL,NULL,&timeout);
		
		data[2] = 0x01;//yellow
		if (!wait_write_serial(serial))
		{
			if (write(serial,data,sizeof(data)) < 0)
			{
			#ifdef COMMON_DEBUG
				printf("write serial error,File:%s,Line:%d\n",__FILE__,__LINE__);
			#endif
				return FAIL;	
			}
		}
		else
		{
		#ifdef COMMON_DEBUG
			printf("the serial can not be written,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;
		select(0,NULL,NULL,NULL,&timeout);

		if (0 != second)
		{
			num++;
			if (num >= second)
				break;
		}
	}//while loop

	return SUCCESS;
}
#endif
int timedown_data_to_embed(sockfd_t *sockfd,timedown_t *timedown)
{
	if ((NULL == sockfd) || (NULL == timedown))
	{
	#ifdef COMMON_DEBUG
		printf("pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned int			temp = 0;
	unsigned int			num = 0;
	unsigned char			i = 0;
	unsigned char			buf[15] = {0};
	

	buf[0] = 'C';
	buf[1] = 'Y';
	buf[2] = 'T';
	buf[3] = '5';
	buf[4] = timedown->mode;
	buf[5] = timedown->stageline;
	buf[6] = timedown->lampcolor;
	buf[7] = timedown->lamptime;
	if (0 == timedown->phaseid)
	{
		for (i = 8; i < 12; i++)
		{
			buf[i] = 0;
		}
	}
	else
	{
		num = 0x01;
		num <<= ((timedown->phaseid) - 1);
		temp = num;
		buf[8] = temp & 0x000000ff;	//0~7bit
		temp = num;
		temp &= 0x0000ff00;
		temp >>= 8;
		buf[9] = temp & 0x000000ff;//8~15bit
		temp = num;
		temp &= 0x00ff0000;
		temp >>= 16;
		buf[10] = temp & 0x000000ff;//16~23bit
		temp = num;
		temp &= 0xff000000;
		temp >>= 24;
		buf[11] = temp & 0x000000ff;//24~31bit
	}
	buf[12] = 'E';
	buf[13] = 'N';
	buf[14] = 'D';

	if (write(*(sockfd->lhsockfd),buf,sizeof(buf)) < 0)
    {
        printf("write error,File: %s,Line: %d\n",__FILE__,__LINE__);
        return FAIL;
    }

	return SUCCESS;
}

int timedown_data_to_conftool(sockfd_t *sockfd,timedown_t *timedown)
{
	if ((NULL == sockfd) || (NULL == timedown))
	{
	#ifdef COMMON_DEBUG
		printf("pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned int			temp = 0;
	unsigned int			num = 0;
	unsigned char			i = 0;
	unsigned char			buf[15] = {0};
	

	buf[0] = 'C';
	buf[1] = 'Y';
	buf[2] = 'T';
	buf[3] = '5';
	buf[4] = timedown->mode;
	buf[5] = timedown->stageline;
	buf[6] = timedown->lampcolor;
	buf[7] = timedown->lamptime;
	if (0 == timedown->phaseid)
	{
		for (i = 8; i < 12; i++)
		{
			buf[i] = 0;
		}
	}
	else
	{
		num = 0x01;
		num <<= ((timedown->phaseid) - 1);
		temp = num;
		buf[8] = temp & 0x000000ff;	//0~7bit
		temp = num;
		temp &= 0x0000ff00;
		temp >>= 8;
		buf[9] = temp & 0x000000ff;//8~15bit
		temp = num;
		temp &= 0x00ff0000;
		temp >>= 16;
		buf[10] = temp & 0x000000ff;//16~23bit
		temp = num;
		temp &= 0xff000000;
		temp >>= 24;
		buf[11] = temp & 0x000000ff;//24~31bit
	}
	buf[12] = 'E';
	buf[13] = 'N';
	buf[14] = 'D';
	if (write(*(sockfd->ssockfd),buf,sizeof(buf)) < 0)
	{
	#ifdef COMMON_DEBUG
		printf("write error,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return FAIL;
	}

	#ifdef EMBED_CONFIGURE_TOOL
	if (write(*(sockfd->lhsockfd),buf,sizeof(buf)) < 0)
    {
        printf("write error,File: %s,Line: %d\n",__FILE__,__LINE__);
        return FAIL;
    }
	#endif

	return SUCCESS;
}

/*Since ip address is less than 255,so 'unsigned char' type is enough*/
int digit_to_string(unsigned char digit,unsigned char *str)
{
	if (NULL == str)
    {
    #ifdef COMMON_DEBUG
        printf("pointer address is NULL,File:%s,Line:%d\n",__FILE__,__LINE__);
    #endif
        return MEMERR;
    }
	unsigned char       tem = 0;
    unsigned char       i = 0;
    unsigned char       *pstr = str;

	tem = digit;
    while (1)
    {
        tem = tem/10;
        i++;
        if (0 == tem)
            break;
    }
	tem = digit;
    while (1)
    {
        pstr[i-1] = (unsigned char)(tem%10 + 48);
        i -= 1;
        if (0 == i)
            break;
        tem = tem/10;
    }
	
	return SUCCESS;
}

int short_to_string(unsigned short digit,unsigned char *str)
{
	if (NULL == str)
    {
    #ifdef COMMON_DEBUG
        printf("pointer address is NULL,File:%s,Line:%d\n",__FILE__,__LINE__);
    #endif
        return MEMERR;
    }

    unsigned short       tem = 0;
    unsigned char       i = 0;
    unsigned char       *pstr = str;

    tem = digit;
    while (1)
    {
        tem = tem/10;
        i++;
        if (0 == tem)
            break;
    }
    tem = digit;
    while (1)
    {
        pstr[i-1] = (unsigned char)(tem%10 + 48);
        i -= 1;
        if (0 == i)
            break;
        tem = tem/10;
    }

    return SUCCESS;
}

/*switch digit string to digit*/
int string_to_digit(unsigned char *str,int *digit)
{
	if ((NULL == str) || (NULL == digit))
	{
	#ifdef COMMON_DEBUG
		printf("pointer address is NULL,File:%s,Line:%d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	int				len = 0;
	char			*temp = NULL;

	*digit = 0;
    len = strlen(str) - 1;
    for (temp=str; *temp != '\0'; temp++)
    {
        *digit += ((int)(*temp) - 48)*(pow(10,len));
        len -= 1;
    }
	return SUCCESS;
}

int get_network_section(unsigned char *str,unsigned char *st1,unsigned char *st2,unsigned char *st3,unsigned char *st4)
{
	if ((NULL == str) || (NULL == st1) || (NULL == st2) || (NULL == st3) || (NULL == st4))
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char				z = 0;
	unsigned char				*tem2 = str;
	unsigned char				comma = 0;
	for (z = 0; z < 4; z++)
	{
		if ('\0' == *tem2)
			break;
		if ('.' == *tem2)
		{
			comma += 1;
			z = 0;
			tem2++;
		}
		if (0 == comma)
		{
			st1[z] = *tem2;
		}
		else if (1 == comma)
		{
			st2[z] = *tem2;
		}
		else if (2 == comma)
		{
			st3[z] = *tem2;
		}
		else if (3 == comma)
		{
			st4[z] = *tem2;
		}
		tem2++;
	}

	return SUCCESS;
}

int day_to_week(int year,int mon,int day,int *w)
{
	if (NULL == w)
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	int 	e = 0;
	int		t = 0;

	switch(mon)   
	{   
		case 1:
			e = day;
			break;   
		case 2:
			e=31+day;
			break;   
		case 3:
			e=59+day;
			break;   
		case 4:
			e=90+day;
			break;   
		case 5:
			e=120+day;
			break;   
		case 6:
			e=151+day;
			break;   
		case 7:
			e=181+day;
			break;   
		case 8:
			e=212+day;
			break;   
		case 9:
			e=243+day;
			break;   
		case 10:
			e=273+day;
			break;   
		case 11:
			e=304+day;
			break;   
		case 12:
			e=334+day;
			break;   
		default:
			break;   
	}
	
	if (1 == mon)
		mon = 13;
	if (2 == mon)
		mon = 14;

	if(((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0)) 
	{
		if (mon > 2)
			++e;
	}
	--year;
	t = year+year/4-year/100+year/400+e;
	*w = t%7;

	return SUCCESS;
}

int gps_parse(int gpsn,unsigned char *gps,int *year,int *month,int *day,int *hour,int *min,int *sec,int *week,JingWeiDu_t *jwd) 
{
	if ((NULL == gps) || (NULL == year) || (NULL == month) || (NULL == day))
	{
	#ifdef COMMON_DEBUG
		printf("pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	if ((NULL == day) || (NULL == min) || (NULL == sec) || (NULL == week))
	{
	#ifdef COMMON_DEBUG
		printf("pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	unsigned char			*tgps = gps;
	unsigned char			hms[7] = {0};
	unsigned char			*phms = hms;
	unsigned char			ymd[7] = {0};
	unsigned char			*pymd = ymd;

	//for parse jingweidu
	unsigned char           wdz[10] = {0};
    unsigned char           wdx[10] = {0};
    unsigned char           *pwdz = wdz;
    unsigned char           *pwdx = wdx;
    unsigned char           jdz[10] = {0};
    unsigned char           jdx[10] = {0};
    unsigned char           *pjdz = jdz;
    unsigned char           *pjdx = jdx;
	unsigned char			z = 0,k = 0;
	//for parse jingweidu

	unsigned char			i = 0,j = 0;
	int						n = 0;
	unsigned char			dotn = 0;
	unsigned char			vad = 0;

	*year = 0;
	*month = 0;
	*day = 0;
	*hour = 0;
	*min = 0;
	*sec = 0;
	*week = 0;	
	for (n = 0; n < gpsn; n++)
	{//for loop
		if ('\0' == *tgps)
			break;
		if(('$'==*tgps)&&('G'==*(tgps+1))&&('P'==*(tgps+2))&&('R'==*(tgps+3)) \
			&&('M'==*(tgps+4))&&('C'==*(tgps+5)))
		{
			while (1)
			{//find out data
				if ('\0' == *tgps)
					break;
				tgps++;
				if(','== *tgps)
				{
					dotn += 1;
					tgps++;
				}
				if (1 == dotn)
				{//find out hour/min/sec
					if (i == 6)
						continue;
					*phms = *tgps;
					phms++;
					i++;
					continue;
				}//find out hour/min/sec			
				if (2 == dotn)
				{//find out invalud/valid
					*phms = '\0';
					if ('A' == *tgps)
						vad = 1;
					else
						vad = 0;
					continue;
				}//find out invalud/valid

				if (3 == dotn)
                {//find out weidu
                    if (9 == z)
                        continue;
                    if (z < 4)
                    {
                        *pwdz = *tgps;
                        pwdz++;
                        z++;
                    }
                    if (4 == z)
                    {
                        z++;
						tgps++;
						continue;
                    }
                    if (z > 4)
                    {
                        *pwdx = *tgps;
                        pwdx++;
                        z++;
                    }
                    continue;
                }//find out weidu
				if (4 == dotn)
				{
					*pwdz = '\0';
                    *pwdx = '\0';
					continue;
				}
				if (5 == dotn)
                {//find out jingdu
                    if (10 == k)
                        continue;
                    if (k < 5)
                    {
                        *pjdz = *tgps;
                        pjdz++;
                        k++;
                    }
                    if (5 == k)
                    {
                        k++;
						tgps++;
						continue;
                    }
                    if (k > 5)
                    {
                        *pjdx = *tgps;
                        pjdx++;
                        k++;
                    }
                    continue;
                }//find out jingdu
				if (6 == dotn)
				{
					*pjdz = '\0';
                    *pjdx = '\0';
					continue;
				}

				if (9 == dotn)
				{//find out year/month/day
					if (j == 6)
						continue;
					*pymd = *tgps;
					pymd++;
					j++;
					continue;
				}//find out year/month/day
				if (10 == dotn)
				{
					*pymd = '\0';
					break;
				}
			}//find out data
			if (1 == vad)
			{
				if (6 == strlen(ymd))
				{
					*day = (ymd[0]-48)*10 + (ymd[1]-48);
					*month = (ymd[2]-48)*10 + (ymd[3]-48);
					*year = 2000 + (ymd[4]-48)*10 + (ymd[5]-48);
				}
				if (6 == strlen(hms))
				{
					*hour = (hms[0]-48)*10 + (hms[1]-48) + 8;
					*min = (hms[2]-48)*10 + (hms[3]-48);
					*sec = (hms[4]-48)*10 + (hms[5]-48);
				}
				if ((0 < *month) && (0 < *day) && (2000 < *year))
				{
					day_to_week(*year,*month,*day,week);		
				}

				#ifdef COMMON_DEBUG
                printf("****************wdz: %s,wdx: %s,jdz: %s,jdx: %s,File: %s,Line: %d\n",wdz,wdx,jdz,jdx,__FILE__,__LINE__);
                #endif
				string_to_digit(wdz,&(jwd->wdz));
				string_to_digit(wdx,&(jwd->wdx));
				string_to_digit(jdz,&(jwd->jdz));
				string_to_digit(jdx,&(jwd->jdx));
				#ifdef COMMON_DEBUG
				printf("*****************wdz: %d,wdx: %d,jdz: %d,jdx: %d,File: %s,Line: %d\n", \
						jwd->wdz,jwd->wdx,jwd->jdz,jwd->jdx,__FILE__,__LINE__);
				#endif

				return SUCCESS;
			}
			else
			{
			#ifdef COMMON_DEBUG
				printf("Invalid gps data,loop %d times,File: %s,Line: %d\n",n,__FILE__,__LINE__);
			#endif
				return FAIL;
			}
		}
		else
		{
			if (1 == vad)
				break;
			tgps++;
			continue;
		}
	}//for loop

	return FAIL;
}

int set_object_value(tscdata_t *tscdata, infotype_t *itype,objectinfo_t *objecti)
{
	if ((NULL == tscdata) || (NULL == objecti) || (NULL == itype))
	{
	#ifdef COMMON_DEBUG
		printf("Poninter address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	unsigned char				i = 0,j = 0,z = 0,s = 0,t = 0;
	unsigned char				*pobjectv = objecti[i].objectv;
	unsigned short				stemp = 0;
	unsigned int				itemp = 0;
	unsigned int				tempi = 0;
	unsigned char				row1 = 0, row2 = 0;
	unsigned char				totaltr= 0;
	unsigned char				mapped = 0;

	for (i = 0; i < (itype->objectn + 1); i++)
	{//for (i = 0; i < (itype->objectn + 1); i++)
		if (0x8D == objecti[i].objectid)
		{//if (0x8D == objecti[i].objectid)
			if ((0 != objecti[i].indexn) && (0 == objecti[i].cobject))
			{//someone table
				for(j = 0; j < objecti[i].indexn; j++)
				{//for (j = 0; j < objecti[i].indexn; j++)
					z = objecti[i].index[j];
					if (z > (tscdata->schedule->FactScheduleNum))
                	{
                	#ifdef COMMON_DEBUG
                    	printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                    	return FAIL;
                	}
					tscdata->schedule->ScheduleList[z-1].ScheduleId = *pobjectv;
					pobjectv++;

					stemp = 0;
					stemp |= *pobjectv;
					pobjectv++;
					stemp <<= 8;
					stemp &= 0xff00;
					stemp |= *pobjectv;
					pobjectv++;
					tscdata->schedule->ScheduleList[z-1].ScheduleMonth = stemp;

					tscdata->schedule->ScheduleList[z-1].ScheduleWeek = *pobjectv;
					pobjectv++;

					itemp = 0;
					tempi = 0;
					tempi |= *pobjectv;
					pobjectv++;
					tempi <<= 24;
					tempi &= 0xff000000;
					itemp |= tempi;
					tempi = 0;
					tempi |= *pobjectv;
					pobjectv++;
					tempi <<= 16;
					tempi &= 0x00ff0000;
					itemp |= tempi;
					tempi = 0;
					tempi |= *pobjectv;
					pobjectv++;
					tempi <<= 8;
					tempi &= 0x0000ff00;
					itemp |= tempi;
					tempi = 0;
					tempi |= *pobjectv;
					pobjectv++;
					tempi &= 0x000000ff;
					itemp |= tempi;
					tscdata->schedule->ScheduleList[z-1].ScheduleDay = itemp;

					tscdata->schedule->ScheduleList[z-1].TimeSectionId = *pobjectv;	
					pobjectv++;					
				}//for (j = 0; j < objecti[i].indexn; j++)
			}//someone table
			else if ((0 != objecti[i].indexn) && (0 != objecti[i].cobject))	
			{//someone field
				for (j = 0; j < objecti[i].indexn; j++)
				{//for (j = 0; j < objecti[i].indexn; j++)
					z = objecti[i].index[j];
                    if (z > (tscdata->schedule->FactScheduleNum))
                    {
                    #ifdef COMMON_DEBUG
                        printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                        return FAIL;
                    }
					if (1 == objecti[i].cobject)
					{
						tscdata->schedule->ScheduleList[z-1].ScheduleId = *pobjectv;
						pobjectv++;	
					}
					else if (2 == objecti[i].cobject)
					{
						stemp = 0;
						stemp |= *pobjectv;
						pobjectv++;
						stemp <<= 8;
						stemp &= 0xff00;
						stemp |= *pobjectv;
                    	pobjectv++;
                    	tscdata->schedule->ScheduleList[z-1].ScheduleMonth = stemp;						
					}
					else if (3 == objecti[i].cobject)
					{
						tscdata->schedule->ScheduleList[z-1].ScheduleWeek = *pobjectv;
						pobjectv++;
					}
					else if (4 == objecti[i].cobject)
					{
						itemp = 0;
                    	tempi = 0;
                    	tempi |= *pobjectv;
                    	pobjectv++;
                    	tempi <<= 24;
                    	tempi &= 0xff000000;
                    	itemp |= tempi;
                    	tempi = 0;
                    	tempi |= *pobjectv;
                    	pobjectv++;
                    	tempi <<= 16;
                    	tempi &= 0x00ff0000;
                    	itemp |= tempi;
                    	tempi = 0;
                    	tempi |= *pobjectv;
                    	pobjectv++;
                    	tempi <<= 8;
                    	tempi &= 0x0000ff00;
                    	itemp |= tempi;
                    	tempi = 0;
                    	tempi |= *pobjectv;
                    	pobjectv++;
                    	tempi &= 0x000000ff;
                    	itemp |= tempi;
                    	tscdata->schedule->ScheduleList[z-1].ScheduleDay = itemp;	
					}
					else if (5 == objecti[i].cobject)
					{
						tscdata->schedule->ScheduleList[z-1].TimeSectionId = *pobjectv;
                    	pobjectv++;
					}
				}//for (j = 0; j < objecti[i].indexn; j++)
			}//someone field
			else if ((0 == objecti[i].indexn) && (0 == objecti[i].cobject))	
			{//whole table or single object
				//0x8D is one dimension table
				memset(tscdata->schedule,0,sizeof(Schedule_t));
				row1 = *pobjectv;
				pobjectv++;
				tscdata->schedule->FactScheduleNum = row1;
				for (j = 0; j < row1; j++)
				{//for (j = 0; j < row1; j++)
					tscdata->schedule->ScheduleList[j].ScheduleId = *pobjectv;
					pobjectv++;
					stemp = 0;
					stemp |= *pobjectv;
					pobjectv++;
					stemp <<= 8;
					stemp &= 0xff00;
					stemp |= *pobjectv;
					pobjectv++;
					tscdata->schedule->ScheduleList[j].ScheduleMonth = stemp;
					tscdata->schedule->ScheduleList[j].ScheduleWeek = *pobjectv;
					pobjectv++;
					itemp = 0;
					tempi = 0;
					tempi |= *pobjectv;
					pobjectv++;
					tempi <<= 24;
					tempi &= 0xff000000;
					itemp |= tempi;
					tempi = 0;
					tempi |= *pobjectv;
					pobjectv++;
					tempi <<= 16;
					tempi &= 0x00ff0000;
					itemp |= tempi;
					tempi = 0;
					tempi |= *pobjectv;
					pobjectv++;
					tempi <<= 8;
					tempi &= 0x0000ff00;
					itemp |= tempi;
					tempi = 0;
					tempi |= *pobjectv;
					pobjectv++;
					tempi &= 0x000000ff;
					itemp |= tempi;
					tscdata->schedule->ScheduleList[j].ScheduleDay = itemp;
					tscdata->schedule->ScheduleList[j].TimeSectionId = *pobjectv;
					pobjectv++;
				}//for (j = 0; j < row1; j++)
			}//whole table or single object
		}//if (0x8D == objecti[i].objectid)
		else if (0x8E == objecti[i].objectid)
		{//else if (0x8E == objecti[i].objectid)
			if ((0 != objecti[i].indexn) && (0 == objecti[i].cobject))
            {//someone table
				for (j = 0; j < objecti[i].indexn; j++)
				{//for (j = 0; j < objecti[i].indexn; j++)
					z = objecti[i].index[j];
					totaltr = 0;
					mapped = 0;
					for (s = 0; ;s++)
					{
						if (0 == (tscdata->timesection->TimeSectionList[s][0].TimeSectionId))
							break;
						for (t = 0; ;t++)
						{
							if (0 == (tscdata->timesection->TimeSectionList[s][t].TimeSectionId))
								break;
							totaltr += 1;
							if (z == totaltr)
							{
								mapped = 1;
								break;
							}
						}
						if (1 == mapped)
							break;
					}
					if (0 == mapped)
					{
					#ifdef COMMON_DEBUG
						printf("Not fit row,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						return FAIL;
					}
					tscdata->timesection->TimeSectionList[s][t].TimeSectionId = *pobjectv;
					pobjectv++;
					tscdata->timesection->TimeSectionList[s][t].EventId = *pobjectv;
					pobjectv++;
					tscdata->timesection->TimeSectionList[s][t].StartHour = *pobjectv;
					pobjectv++;
					tscdata->timesection->TimeSectionList[s][t].StartMinute = *pobjectv;
					pobjectv++;
					tscdata->timesection->TimeSectionList[s][t].ControlMode = *pobjectv;
					pobjectv++;
					tscdata->timesection->TimeSectionList[s][t].PatternId = *pobjectv;
					pobjectv++;
					tscdata->timesection->TimeSectionList[s][t].AuxFunc = *pobjectv;
					pobjectv++;
					tscdata->timesection->TimeSectionList[s][t].SpecFunc = *pobjectv;
					pobjectv++;	
				}//for (j = 0; j < objecti[i].indexn; j++)
            }//someone table
            else if ((0 != objecti[i].indexn) && (0 != objecti[i].cobject))
            {//someone field
				for (j = 0; j < objecti[i].indexn; j++)
				{//for (j = 0; j < objecti[i].indexn; j++)
					z = objecti[i].index[j];
					totaltr = 0;
					mapped = 0;
					for (s = 0; ;s++)
					{
						if (0 == (tscdata->timesection->TimeSectionList[s][0].TimeSectionId))
							break;
						for (t = 0; ;t++)
						{
							if (0 == (tscdata->timesection->TimeSectionList[s][t].TimeSectionId))
								break;
							totaltr += 1;
							if (z == totaltr)
							{
								mapped = 1;
								break;
							}
						}
						if (1 == mapped)
							break;
					}
					if (0 == mapped)
					{
					#ifdef COMMON_DEBUG
						printf("Not fit row,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						return FAIL;
					}
					if (1 == objecti[i].cobject)
					{
						tscdata->timesection->TimeSectionList[s][t].TimeSectionId = *pobjectv;
						pobjectv++;
					}
					else if (2 == objecti[i].cobject)
					{
                    	tscdata->timesection->TimeSectionList[s][t].EventId = *pobjectv;
                    	pobjectv++;
					}
					else if (3 == objecti[i].cobject)
					{
                    	tscdata->timesection->TimeSectionList[s][t].StartHour = *pobjectv;
                    	pobjectv++;
					}
					else if (4 == objecti[i].cobject)
					{
                    	tscdata->timesection->TimeSectionList[s][t].StartMinute = *pobjectv;
                    	pobjectv++;
					}
					else if (5 == objecti[i].cobject)
					{
                    	tscdata->timesection->TimeSectionList[s][t].ControlMode = *pobjectv;
                    	pobjectv++;
					}
					else if (6 == objecti[i].cobject)
					{
                    	tscdata->timesection->TimeSectionList[s][t].PatternId = *pobjectv;
                    	pobjectv++;
					}
					else if (7 == objecti[i].cobject)
					{
                    	tscdata->timesection->TimeSectionList[s][t].AuxFunc = *pobjectv;
                    	pobjectv++;
					}
					else if (8 == objecti[i].cobject)
					{
                    	tscdata->timesection->TimeSectionList[s][t].SpecFunc = *pobjectv;
                    	pobjectv++;
					}
				}//for (j = 0; j < objecti[i].indexn; j++)
            }//someone field
            else if ((0 == objecti[i].indexn) && (0 == objecti[i].cobject))
            {//whole table or single object
				//0x8E is 2 dimension table
				memset(tscdata->timesection,0,sizeof(TimeSection_t));
				row1 = *pobjectv;
				tscdata->timesection->FactTimeSectionNum = row1;
				pobjectv++;
				row2 = *pobjectv;
				tscdata->timesection->FactEventNum = row2;
				pobjectv++;
				for (j = 0; j < row1; j++)
				{
					for (z = 0; z < row2; z++)
					{
						if (0 == *pobjectv)
						{
							pobjectv += 8;//8 invalid data
							continue;
						}
						tscdata->timesection->TimeSectionList[j][z].TimeSectionId = *pobjectv;
						pobjectv++;
						tscdata->timesection->TimeSectionList[j][z].EventId = *pobjectv;
						pobjectv++;
						tscdata->timesection->TimeSectionList[j][z].StartHour = *pobjectv;
						pobjectv++;
						tscdata->timesection->TimeSectionList[j][z].StartMinute = *pobjectv;
						pobjectv++;
						tscdata->timesection->TimeSectionList[j][z].ControlMode = *pobjectv;
						pobjectv++;
						tscdata->timesection->TimeSectionList[j][z].PatternId = *pobjectv;
						pobjectv++;
						tscdata->timesection->TimeSectionList[j][z].AuxFunc = *pobjectv;
						pobjectv++;
						tscdata->timesection->TimeSectionList[j][z].SpecFunc = *pobjectv;
						pobjectv++;
					}
				}	
            }//whole table or single object
		}//else if (0x8E == objecti[i].objectid)
		else if (0x95 == objecti[i].objectid)
		{//else if (0x95 == objecti[i].objectid)
			if ((0 != objecti[i].indexn) && (0 == objecti[i].cobject))
            {//someone table
				for(j = 0; j < objecti[i].indexn; j++)
				{//for (j = 0; j < objecti[i].indexn; j++)
					z = objecti[i].index[j];

					if (z > (tscdata->phase->FactPhaseNum))
                	{
                	#ifdef COMMON_DEBUG
                    	printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                    	return FAIL;
                	}
					tscdata->phase->PhaseList[z-1].PhaseId = *pobjectv;
					pobjectv++;
					tscdata->phase->PhaseList[z-1].PhaseWalkGreen = *pobjectv;
					pobjectv++;
					tscdata->phase->PhaseList[z-1].PhaseWalkClear = *pobjectv;
					pobjectv++;
					tscdata->phase->PhaseList[z-1].PhaseMinGreen = *pobjectv;
					pobjectv++;
					tscdata->phase->PhaseList[z-1].PhaseGreenDelay = *pobjectv;
					pobjectv++;
					tscdata->phase->PhaseList[z-1].PhaseMaxGreen1 = *pobjectv;
					pobjectv++;
					tscdata->phase->PhaseList[z-1].PhaseMaxGreen2 = *pobjectv;
					pobjectv++;
					tscdata->phase->PhaseList[z-1].PhaseFixGreen = *pobjectv;
					pobjectv++;
					tscdata->phase->PhaseList[z-1].PhaseGreenFlash = *pobjectv;
					pobjectv++;
					tscdata->phase->PhaseList[z-1].PhaseType = *pobjectv;
					pobjectv++;
					tscdata->phase->PhaseList[z-1].PhaseSpecFunc = *pobjectv;
					pobjectv++;
					tscdata->phase->PhaseList[z-1].PhaseReserved = *pobjectv;
					pobjectv++;
				}//for (j = 0; j < objecti[i].indexn; j++)
            }//someone table
            else if ((0 != objecti[i].indexn) && (0 != objecti[i].cobject))
            {//someone field
				for (j = 0; j < objecti[i].indexn; j++)
				{//for (j = 0; j < objecti[i].indexn; j++)
					z = objecti[i].index[j];
                    if (z > (tscdata->phase->FactPhaseNum))
                    {
                    #ifdef COMMON_DEBUG
                        printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                        return FAIL;
                    }
					if (1 == objecti[i].cobject)
					{
						tscdata->phase->PhaseList[z-1].PhaseId = *pobjectv;
						pobjectv++;	
					}
					else if (2 == objecti[i].cobject)
					{
						tscdata->phase->PhaseList[z-1].PhaseWalkGreen = *pobjectv;
						pobjectv++;
					}
					else if (3 == objecti[i].cobject)
					{
						tscdata->phase->PhaseList[z-1].PhaseWalkClear = *pobjectv;
						pobjectv++;
					}
					else if (4 == objecti[i].cobject)
					{
						tscdata->phase->PhaseList[z-1].PhaseMinGreen = *pobjectv;
						pobjectv++;
					}
					else if (5 == objecti[i].cobject)
					{
						tscdata->phase->PhaseList[z-1].PhaseGreenDelay = *pobjectv;
						pobjectv++;
					}
					else if (6 == objecti[i].cobject)
					{
						tscdata->phase->PhaseList[z-1].PhaseMaxGreen1 = *pobjectv;
						pobjectv++;
					}
					else if (7 == objecti[i].cobject)
					{
						tscdata->phase->PhaseList[z-1].PhaseMaxGreen2 = *pobjectv;
						pobjectv++;
					}
					else if (8 == objecti[i].cobject)
					{
						tscdata->phase->PhaseList[z-1].PhaseFixGreen = *pobjectv;
						pobjectv++;
					}
					else if (9 == objecti[i].cobject)
                    {
						tscdata->phase->PhaseList[z-1].PhaseGreenFlash = *pobjectv;
						pobjectv++;
                    }
					else if (10 == objecti[i].cobject)
                    {
						tscdata->phase->PhaseList[z-1].PhaseType = *pobjectv;
						pobjectv++;
                    }
					else if (11 == objecti[i].cobject)
                    {
						tscdata->phase->PhaseList[z-1].PhaseSpecFunc = *pobjectv;
						pobjectv++;
                    }
					else if (12 == objecti[i].cobject)
                    {
						tscdata->phase->PhaseList[z-1].PhaseReserved = *pobjectv;
						pobjectv++;
                    }
				}//for (j = 0; j < objecti[i].indexn; j++)
            }//someone field
            else if ((0 == objecti[i].indexn) && (0 == objecti[i].cobject))
            {//whole table or single object
				memset(tscdata->phase,0,sizeof(Phase_t));
				row1 = *pobjectv;
				pobjectv++;
				tscdata->phase->FactPhaseNum = row1;
				for (j = 0; j < row1; j++)
				{//for (j = 0; j < row1; j++)
					tscdata->phase->PhaseList[j].PhaseId = *pobjectv;
					pobjectv++;
					tscdata->phase->PhaseList[j].PhaseWalkGreen = *pobjectv;
					pobjectv++;
					tscdata->phase->PhaseList[j].PhaseWalkClear = *pobjectv;
					pobjectv++;
					tscdata->phase->PhaseList[j].PhaseMinGreen = *pobjectv;
					pobjectv++;
					tscdata->phase->PhaseList[j].PhaseGreenDelay = *pobjectv;
					pobjectv++;
					tscdata->phase->PhaseList[j].PhaseMaxGreen1 = *pobjectv;
					pobjectv++;
					tscdata->phase->PhaseList[j].PhaseMaxGreen2 = *pobjectv;
                    pobjectv++;
					tscdata->phase->PhaseList[j].PhaseFixGreen = *pobjectv;
					pobjectv++;
					tscdata->phase->PhaseList[j].PhaseGreenFlash = *pobjectv;
					pobjectv++;
					tscdata->phase->PhaseList[j].PhaseType = *pobjectv;
					pobjectv++;
					tscdata->phase->PhaseList[j].PhaseSpecFunc = *pobjectv;
					pobjectv++;
					tscdata->phase->PhaseList[j].PhaseReserved = *pobjectv;
					pobjectv++;
				}//for (j = 0; j < row1; j++)
            }//whole table or single object
		}//else if (0x95 == objecti[i].objectid)
		else if (0x97 == objecti[i].objectid)
		{//else if (0x97 == objecti[i].objectid)
			if ((0 != objecti[i].indexn) && (0 == objecti[i].cobject))
            {//someone table
				for(j = 0; j < objecti[i].indexn; j++)
				{//for (j = 0; j < objecti[i].indexn; j++)
					z = objecti[i].index[j];
					if (z > (tscdata->phaseerror->FactPhaseErrorNum))
                	{
                	#ifdef COMMON_DEBUG
                    	printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                    	return FAIL;
                	}
					tscdata->phaseerror->PhaseErrorList[z-1].PhaseId = *pobjectv;
					pobjectv++;
					itemp = 0;

					tempi = 0;
					tempi |= *pobjectv;
					pobjectv++;
					tempi <<= 24;
					tempi &= 0xff000000;
					itemp |= tempi;

					tempi = 0;
					tempi |= *pobjectv;
					pobjectv++;
					tempi <<= 16;
					tempi &= 0x00ff0000;
					itemp |= tempi;

					tempi = 0;
					tempi |= *pobjectv;
					pobjectv++;
					tempi <<= 8;
					tempi &= 0x0000ff00;
					itemp |= tempi;

					tempi = 0;
					tempi |= *pobjectv;
					pobjectv++;
					tempi &= 0x000000ff;
					itemp |= tempi;
					tscdata->phaseerror->PhaseErrorList[z-1].PhaseConflict = itemp;						
				}//for (j = 0; j < objecti[i].indexn; j++)
            }//someone table
            else if ((0 != objecti[i].indexn) && (0 != objecti[i].cobject))
            {//someone field
				for (j = 0; j < objecti[i].indexn; j++)
				{//for (j = 0; j < objecti[i].indexn; j++)
					z = objecti[i].index[j];
                    if (z > (tscdata->phaseerror->FactPhaseErrorNum))
                    {
                    #ifdef COMMON_DEBUG
                        printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                        return FAIL;
                    }
					if (1 == objecti[i].cobject)
					{
						tscdata->phaseerror->PhaseErrorList[z-1].PhaseId = *pobjectv;
						pobjectv++;	
					}
					else if (2 == objecti[i].cobject)	
					{
						itemp = 0;
						tempi = 0;
						tempi |= *pobjectv;
						pobjectv++;
						tempi <<= 24;
						tempi &= 0xff000000;
						itemp |= tempi;

						tempi = 0;
						tempi |= *pobjectv;
						pobjectv++;
						tempi <<= 16;
						tempi &= 0x00ff0000;
						itemp |= tempi;

						tempi = 0;
						tempi |= *pobjectv;
						pobjectv++;
						tempi <<= 8;
						tempi &= 0x0000ff00;
						itemp |= tempi;

						tempi = 0;
						tempi |= *pobjectv;
						pobjectv++;
						tempi &= 0x000000ff;
						itemp |= tempi;
						tscdata->phaseerror->PhaseErrorList[z-1].PhaseConflict = itemp;
					}
				}//for (j = 0; j < objecti[i].indexn; j++)
            }//someone field
            else if ((0 == objecti[i].indexn) && (0 == objecti[i].cobject))
            {//whole table or single object
				memset(tscdata->phaseerror,0,sizeof(PhaseError_t));
				row1 = *pobjectv;
				pobjectv++;
				tscdata->phaseerror->FactPhaseErrorNum = row1;
				for (j = 0; j < row1; j++)
				{//for (j = 0; j < row1; j++)
					tscdata->phaseerror->PhaseErrorList[j].PhaseId = *pobjectv;
					pobjectv++;
					itemp = 0;
					tempi = 0;
					tempi |= *pobjectv;
					pobjectv++;
					tempi <<= 24;
					tempi &= 0xff000000;
					itemp |= tempi;

					tempi = 0;
					tempi |= *pobjectv;
					pobjectv++;
					tempi <<= 16;
					tempi &= 0x00ff0000;
					itemp |= tempi;

					tempi = 0;
					tempi |= *pobjectv;
					pobjectv++;
					tempi <<= 8;
					tempi &= 0x0000ff00;
					itemp |= tempi;

					tempi = 0;
					tempi |= *pobjectv;
					pobjectv++;
					tempi &= 0x000000ff;
					itemp |= tempi;
					
					tscdata->phaseerror->PhaseErrorList[z-1].PhaseConflict = itemp;	
				}//for (j = 0; j < row1; j++)
            }//whole table or single object
		}//else if (0x97 == objecti[i].objectid)
		else if (0x9F == objecti[i].objectid)
		{//else if (0x9F == objecti[i].objectid)
			if ((0 != objecti[i].indexn) && (0 == objecti[i].cobject))
            {//someone table
				for(j = 0; j < objecti[i].indexn; j++)
				{//for (j = 0; j < objecti[i].indexn; j++)
					z = objecti[i].index[j];
					if (z > (tscdata->detector->FactDetectorNum))
                	{
                	#ifdef COMMON_DEBUG
                    	printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                    	return FAIL;
                	}
					tscdata->detector->DetectorList[z-1].DetectorId = *pobjectv;
					pobjectv++;
					tscdata->detector->DetectorList[z-1].DetectorPhase = *pobjectv;
					pobjectv++;
					tscdata->detector->DetectorList[z-1].DetectorType = *pobjectv;
					pobjectv++;
					tscdata->detector->DetectorList[z-1].DetectorDirect = *pobjectv;
					pobjectv++;
					tscdata->detector->DetectorList[z-1].DetectorDelay = *pobjectv;
					pobjectv++;
					tscdata->detector->DetectorList[z-1].DetectorSpecFunc = *pobjectv;
					pobjectv++;
					stemp = 0;
                    stemp |= *pobjectv;
                    pobjectv++;
                    stemp <<= 8;
                    stemp &= 0xff00;
                    stemp |= *pobjectv;
                    pobjectv++;	
					tscdata->detector->DetectorList[z-1].DetectorFlow = stemp;
					tscdata->detector->DetectorList[z-1].DetectorOccupy = *pobjectv;
					pobjectv++;
				}//for (j = 0; j < objecti[i].indexn; j++)
            }//someone table
            else if ((0 != objecti[i].indexn) && (0 != objecti[i].cobject))
            {//someone field
				for (j = 0; j < objecti[i].indexn; j++)
				{//for (j = 0; j < objecti[i].indexn; j++)
					z = objecti[i].index[j];
                    if (z > (tscdata->detector->FactDetectorNum))
                    {
                    #ifdef COMMON_DEBUG
                        printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                        return FAIL;
                    }
					if (1 == objecti[i].cobject)
					{
						tscdata->detector->DetectorList[z-1].DetectorId = *pobjectv;
						pobjectv++;	
					}
					else if (2 == objecti[i].cobject)	
					{
						tscdata->detector->DetectorList[z-1].DetectorPhase = *pobjectv;
						pobjectv++;	
					}
					else if (3 == objecti[i].cobject)
					{
						tscdata->detector->DetectorList[z-1].DetectorType = *pobjectv;
                    	pobjectv++;
					}
					else if (4 == objecti[i].cobject)
					{
						tscdata->detector->DetectorList[z-1].DetectorDirect = *pobjectv;
                    	pobjectv++;
					}
					else if (5 == objecti[i].cobject)
					{
						tscdata->detector->DetectorList[z-1].DetectorDelay = *pobjectv;
                    	pobjectv++;
					}
					else if (6 == objecti[i].cobject)
					{
						tscdata->detector->DetectorList[z-1].DetectorSpecFunc = *pobjectv;
                    	pobjectv++;
					}
					else if (7 == objecti[i].cobject)
					{
						stemp = 0;
						stemp |= *pobjectv;
						pobjectv++;
						stemp <<= 8;
						stemp &= 0xff00;
						stemp |= *pobjectv;
						pobjectv++;
						tscdata->detector->DetectorList[z-1].DetectorFlow = stemp;
					}
					else if (8 == objecti[i].cobject)
					{
						tscdata->detector->DetectorList[z-1].DetectorOccupy = *pobjectv;
						pobjectv++;
					}	
				}//for (j = 0; j < objecti[i].indexn; j++)
            }//someone field
            else if ((0 == objecti[i].indexn) && (0 == objecti[i].cobject))
            {//whole table or single object
				memset(tscdata->detector,0,sizeof(Detector_t));
				row1 = *pobjectv;
				pobjectv++;
				tscdata->detector->FactDetectorNum = row1;
				for (j = 0; j < row1; j++)
				{//for (j = 0; j < row1; j++)
					tscdata->detector->DetectorList[j].DetectorId = *pobjectv;
					pobjectv++;
					tscdata->detector->DetectorList[j].DetectorPhase = *pobjectv;
					pobjectv++;
					tscdata->detector->DetectorList[j].DetectorType = *pobjectv;
					pobjectv++;
					tscdata->detector->DetectorList[j].DetectorDirect = *pobjectv;
					pobjectv++;
					tscdata->detector->DetectorList[j].DetectorDelay = *pobjectv;
					pobjectv++;
					tscdata->detector->DetectorList[j].DetectorSpecFunc = *pobjectv;
					pobjectv++;
					stemp = 0;
					stemp |= *pobjectv;
					pobjectv++;
					stemp <<= 8;
					stemp &= 0xff00;
					stemp |= *pobjectv;
					pobjectv++;
					tscdata->detector->DetectorList[j].DetectorFlow = stemp;
					tscdata->detector->DetectorList[j].DetectorOccupy = *pobjectv;
					pobjectv++;	
				}//for (j = 0; j < row1; j++)
            }//whole table or single object
		}//else if (0x9F == objecti[i].objectid)
		else if (0xB0 == objecti[i].objectid)
		{//else if (0xB0 == objecti[i].objectid)
			if ((0 != objecti[i].indexn) && (0 == objecti[i].cobject))
            {//someone table
				for(j = 0; j < objecti[i].indexn; j++)
				{//for (j = 0; j < objecti[i].indexn; j++)
					z = objecti[i].index[j];
					if (z > (tscdata->channel->FactChannelNum))
                	{
                	#ifdef COMMON_DEBUG
                    	printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
                	#endif
                    	return FAIL;
                	}
					tscdata->channel->ChannelList[z-1].ChannelId = *pobjectv;
					pobjectv++;
					tscdata->channel->ChannelList[z-1].ChannelCtrlSrc = *pobjectv;
					pobjectv++;
					tscdata->channel->ChannelList[z-1].ChannelFlash = *pobjectv;
					pobjectv++;
					tscdata->channel->ChannelList[z-1].ChannelType = *pobjectv;
					pobjectv++;
				}//for (j = 0; j < objecti[i].indexn; j++)
            }//someone table
            else if ((0 != objecti[i].indexn) && (0 != objecti[i].cobject))
            {//someone field
				for (j = 0; j < objecti[i].indexn; j++)
				{//for (j = 0; j < objecti[i].indexn; j++)
					z = objecti[i].index[j];
                    if (z > (tscdata->channel->FactChannelNum))
                    {
                    #ifdef COMMON_DEBUG
                        printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                        return FAIL;
                    }
					if (1 == objecti[i].cobject)
					{
						tscdata->channel->ChannelList[z-1].ChannelId = *pobjectv;
						pobjectv++;	
					}
					else if (2 == objecti[i].cobject)	
					{
						tscdata->channel->ChannelList[z-1].ChannelCtrlSrc = *pobjectv;
						pobjectv++;	
					}
					else if (3 == objecti[i].cobject)
					{
						tscdata->channel->ChannelList[z-1].ChannelFlash = *pobjectv;
                    	pobjectv++;
					}
					else if (4 == objecti[i].cobject)
					{
						tscdata->channel->ChannelList[z-1].ChannelType = *pobjectv;
                    	pobjectv++;
					}
				}//for (j = 0; j < objecti[i].indexn; j++)
            }//someone field
            else if ((0 == objecti[i].indexn) && (0 == objecti[i].cobject))
            {//whole table or single object
				memset(tscdata->channel,0,sizeof(Channel_t));
				row1 = *pobjectv;
				pobjectv++;
				tscdata->channel->FactChannelNum = row1;
				for (j = 0; j < row1; j++)
				{//for (j = 0; j < row1; j++)
					tscdata->channel->ChannelList[j].ChannelId = *pobjectv;
					pobjectv++;
					tscdata->channel->ChannelList[j].ChannelCtrlSrc = *pobjectv;
					pobjectv++;
					tscdata->channel->ChannelList[j].ChannelFlash = *pobjectv;
					pobjectv++;
					tscdata->channel->ChannelList[j].ChannelType = *pobjectv;
					pobjectv++;
				}//for (j = 0; j < row1; j++)
            }//whole table or single object
		}//else if (0xB0 == objecti[i].objectid)
		else if (0xC0 == objecti[i].objectid)
		{//else if (0xC0 == objecti[i].objectid)
			if ((0 != objecti[i].indexn) && (0 == objecti[i].cobject))
            {//someone table
				for(j = 0; j < objecti[i].indexn; j++)
                {//for (j = 0; j < objecti[i].indexn; j++)
                    z = objecti[i].index[j];
                    if (z > (tscdata->pattern->FactPatternNum))
                    {
                    #ifdef COMMON_DEBUG
                        printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                        return FAIL;
                    }
                    tscdata->pattern->PatternList[z-1].PatternId = *pobjectv;
					if (0 == tscdata->pattern->PatternList[z-1].PatternId)
					{
					#ifdef COMMON_DEBUG
						printf("Not fit pattern id,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						return FAIL;
					}
                    pobjectv++;
					stemp = 0;
					stemp |= *pobjectv;
					pobjectv++;
					stemp <<= 8;
					stemp &= 0xff00;
					stemp |= *pobjectv;
					pobjectv++;
					tscdata->pattern->PatternList[z-1].CycleTime = stemp;
//					tscdata->pattern->PatternList[z-1].PhaseOffset = *pobjectv;
//					pobjectv++;
					//modified on 20150414
					stemp = 0;
                    stemp |= *pobjectv;
                    pobjectv++;
                    stemp <<= 8;
                    stemp &= 0xff00;
                    stemp |= *pobjectv;
                    pobjectv++;
					tscdata->pattern->PatternList[z-1].PhaseOffset = stemp;
					//end modify

					tscdata->pattern->PatternList[z-1].CoordPhase = *pobjectv;
					pobjectv++;
                    tscdata->pattern->PatternList[z-1].TimeConfigId = *pobjectv;
					pobjectv++;
					if (0 == tscdata->pattern->PatternList[z-1].TimeConfigId)
                    {
                    #ifdef COMMON_DEBUG
                        printf("Not fit time config id,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                        return FAIL;
                    }
                }//for (j = 0; j < objecti[i].indexn; j++)	
            }//someone table
            else if ((0 != objecti[i].indexn) && (0 != objecti[i].cobject))
            {//someone field
				for (j = 0; j < objecti[i].indexn; j++)
				{//for (j = 0; j < objecti[i].indexn; j++)
					z = objecti[i].index[j];
                    if (z > (tscdata->pattern->FactPatternNum))
                    {
                    #ifdef COMMON_DEBUG
                        printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                        return FAIL;
                    }
					if (1 == objecti[i].cobject)
					{
						tscdata->pattern->PatternList[z-1].PatternId = *pobjectv;
						pobjectv++;
						if (0 == tscdata->pattern->PatternList[z-1].PatternId)
                    	{
                    	#ifdef COMMON_DEBUG
                        	printf("Not fit pattern id,File: %s,Line: %d\n",__FILE__,__LINE__);
                    	#endif
                        	return FAIL;
                    	}	
					}
					else if (2 == objecti[i].cobject)	
					{
						stemp = 0;
						stemp |= *pobjectv;
						pobjectv++;
						stemp <<= 8;
						stemp &= 0xff00;
						stemp |= *pobjectv;
						pobjectv++;
						tscdata->pattern->PatternList[z-1].CycleTime = stemp;	
					}
					else if (3 == objecti[i].cobject)
					{
//						tscdata->pattern->PatternList[z-1].PhaseOffset = *pobjectv;
//						pobjectv++;
						//modified on 20150414
						stemp = 0;
                        stemp |= *pobjectv;
                        pobjectv++;
                        stemp <<= 8;
                        stemp &= 0xff00;
                        stemp |= *pobjectv;
                        pobjectv++;
                        tscdata->pattern->PatternList[z-1].PhaseOffset = stemp;
					}
					else if (4 == objecti[i].cobject)
					{
						tscdata->pattern->PatternList[z-1].CoordPhase = *pobjectv;
						pobjectv++;
					}
					else if (5 == objecti[i].cobject)
                    {
						tscdata->pattern->PatternList[z-1].TimeConfigId = *pobjectv;
                        pobjectv++;
						if (0 == tscdata->pattern->PatternList[z-1].TimeConfigId)
                    	{
                    	#ifdef COMMON_DEBUG
                        	printf("Not fit time config id,File: %s,Line: %d\n",__FILE__,__LINE__);
                    	#endif
                        	return FAIL;
                    	}
                    }
				}//for (j = 0; j < objecti[i].indexn; j++)
            }//someone field
            else if ((0 == objecti[i].indexn) && (0 == objecti[i].cobject))
            {//whole table or single object
				memset(tscdata->pattern,0,sizeof(Pattern_t));
                row1 = *pobjectv;
                pobjectv++;
                tscdata->pattern->FactPatternNum = row1;
                for (j = 0; j < row1; j++)
                {//for (j = 0; j < row1; j++)
                    tscdata->pattern->PatternList[j].PatternId = *pobjectv;
                    pobjectv++;
					if (0 == tscdata->pattern->PatternList[j].PatternId)
                    {
                    #ifdef COMMON_DEBUG
                        printf("Not fit pattern id,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                        return FAIL;
                    }
					stemp = 0;
					stemp |= *pobjectv;
					pobjectv++;
					stemp <<= 8;
					stemp &= 0xff00;
					stemp |= *pobjectv;
					pobjectv++;
					tscdata->pattern->PatternList[j].CycleTime = stemp;
//					tscdata->pattern->PatternList[j].PhaseOffset = *pobjectv;
//					pobjectv++;
					//modified on 20150414
					stemp = 0;
                    stemp |= *pobjectv;
                    pobjectv++;
                    stemp <<= 8;
                    stemp &= 0xff00;
                    stemp |= *pobjectv;
                    pobjectv++;
                    tscdata->pattern->PatternList[j].PhaseOffset = stemp;
					//end modify

					tscdata->pattern->PatternList[j].CoordPhase = *pobjectv;
                    pobjectv++;
					tscdata->pattern->PatternList[j].TimeConfigId = *pobjectv;
                    pobjectv++;
					if (0 == tscdata->pattern->PatternList[j].TimeConfigId)
                    {
                    #ifdef COMMON_DEBUG
                        printf("Not fit time config id,File: %s,Line: %d\n",__FILE__,__LINE__);
                    #endif
                        return FAIL;
                    }
                }//for (j = 0; j < row1; j++)
            }//whole table or single object
		}//else if (0xC0 == objecti[i].objectid)
		else if (0xC1 == objecti[i].objectid)
		{//else if (0xC1 == objecti[i].objectid)
			if ((0 != objecti[i].indexn) && (0 == objecti[i].cobject))
            {//someone table
				for (j = 0; j < objecti[i].indexn; j++)
				{//for (j = 0; j < objecti[i].indexn; j++)
					z = objecti[i].index[j];
					totaltr = 0;
					mapped = 0;
					for (s = 0; ;s++)
					{
						if (0 == (tscdata->timeconfig->TimeConfigList[s][0].TimeConfigId))
							break;
						for (t = 0; ;t++)
						{
							if (0 == (tscdata->timeconfig->TimeConfigList[s][t].TimeConfigId))
								break;
							totaltr += 1;
							if (z == totaltr)
							{
								mapped = 1;
								break;
							}
						}
						if (1 == mapped)
							break;
					}
					if (0 == mapped)
					{
					#ifdef COMMON_DEBUG
						printf("Not fit row,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						return FAIL;
					}
					tscdata->timeconfig->TimeConfigList[s][t].TimeConfigId = *pobjectv;
					pobjectv++;
					tscdata->timeconfig->TimeConfigList[s][t].StageId = *pobjectv;
					pobjectv++;
					itemp = 0;
					tempi = 0;
					tempi |= *pobjectv;
					pobjectv++;
					tempi <<= 24;
					tempi &= 0xff000000;
					itemp |= tempi;
					tempi = 0;
					tempi |= *pobjectv;
					pobjectv++;
					tempi <<= 16;
					tempi &= 0x00ff0000;
					itemp |= tempi;
					tempi = 0;
					tempi |= *pobjectv;
					pobjectv++;
					tempi <<= 8;
					tempi &= 0x0000ff00;
					itemp |= tempi;
					tempi = 0;
					tempi |= *pobjectv;
					pobjectv++;
					tempi &= 0x000000ff;
					itemp |= tempi;
					tscdata->timeconfig->TimeConfigList[s][t].PhaseId = itemp;
					tscdata->timeconfig->TimeConfigList[s][t].GreenTime = *pobjectv;
					pobjectv++;
					tscdata->timeconfig->TimeConfigList[s][t].YellowTime = *pobjectv;
					pobjectv++;
					tscdata->timeconfig->TimeConfigList[s][t].RedTime = *pobjectv;
					pobjectv++;
					tscdata->timeconfig->TimeConfigList[s][t].SpecFunc = *pobjectv;
					pobjectv++;
				}//for (j = 0; j < objecti[i].indexn; j++)
            }//someone table
            else if ((0 != objecti[i].indexn) && (0 != objecti[i].cobject))
            {//someone field
				for (j = 0; j < objecti[i].indexn; j++)
				{//for (j = 0; j < objecti[i].indexn; j++)
					z = objecti[i].index[j];
					totaltr = 0;
					mapped = 0;
					for (s = 0; ;s++)
					{
						if (0 == (tscdata->timeconfig->TimeConfigList[s][0].TimeConfigId))
							break;
						for (t = 0; ;t++)
						{
							if (0 == (tscdata->timeconfig->TimeConfigList[s][t].TimeConfigId))
								break;
							totaltr += 1;
							if (z == totaltr)
							{
								mapped = 1;
								break;
							}
						}
						if (1 == mapped)
							break;
					}
					if (0 == mapped)
					{
					#ifdef COMMON_DEBUG
						printf("Not fit row,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						return FAIL;
					}
					if (1 == objecti[i].cobject)
					{
						tscdata->timeconfig->TimeConfigList[s][t].TimeConfigId = *pobjectv;
						pobjectv++;
					}
					else if (2 == objecti[i].cobject)
					{
                    	tscdata->timeconfig->TimeConfigList[s][t].StageId = *pobjectv;
                    	pobjectv++;
					}
					else if (3 == objecti[i].cobject)
					{
						itemp = 0;
						tempi = 0;
						tempi |= *pobjectv;
						pobjectv++;
						tempi <<= 24;
						tempi &= 0xff000000;
						itemp |= tempi;
						tempi = 0;
						tempi |= *pobjectv;
						pobjectv++;
						tempi <<= 16;
						tempi &= 0x00ff0000;
						itemp |= tempi;
						tempi = 0;
						tempi |= *pobjectv;
						pobjectv++;
						tempi <<= 8;
						tempi &= 0x0000ff00;
						itemp |= tempi;
						tempi = 0;
						tempi |= *pobjectv;
						pobjectv++;
						tempi &= 0x000000ff;
						itemp |= tempi;
						tscdata->timeconfig->TimeConfigList[s][t].PhaseId = itemp;
					}
					else if (4 == objecti[i].cobject)
					{
                    	tscdata->timeconfig->TimeConfigList[s][t].GreenTime = *pobjectv;
                    	pobjectv++;
					}
					else if (5 == objecti[i].cobject)
					{
                    	tscdata->timeconfig->TimeConfigList[s][t].YellowTime = *pobjectv;
                    	pobjectv++;
					}
					else if (6 == objecti[i].cobject)
					{
                    	tscdata->timeconfig->TimeConfigList[s][t].RedTime = *pobjectv;
                    	pobjectv++;
					}
					else if (7 == objecti[i].cobject)
					{
                    	tscdata->timeconfig->TimeConfigList[s][t].SpecFunc = *pobjectv;
                    	pobjectv++;
					}
				}//for (j = 0; j < objecti[i].indexn; j++)
            }//someone field
            else if ((0 == objecti[i].indexn) && (0 == objecti[i].cobject))
            {//whole table or single object
				memset(tscdata->timeconfig,0,sizeof(TimeConfig_t));
				row1 = *pobjectv;
				tscdata->timeconfig->FactTimeConfigNum = row1;
				pobjectv++;
				row2 = *pobjectv;
				tscdata->timeconfig->FactStageNum = row2;
				pobjectv++;
				for (j = 0; j < row1; j++)
				{
					for (z = 0; z < row2; z++)
					{
						if (0 == *pobjectv)
						{
							pobjectv += 10;//10 invalid data
							continue;
						}
						tscdata->timeconfig->TimeConfigList[j][z].TimeConfigId = *pobjectv;
						pobjectv++;
						tscdata->timeconfig->TimeConfigList[j][z].StageId = *pobjectv;
						pobjectv++;
						itemp = 0;
						tempi = 0;
						tempi |= *pobjectv;
						pobjectv++;
						tempi <<= 24;
						tempi &= 0xff000000;
						itemp |= tempi;
						tempi = 0;
						tempi |= *pobjectv;
						pobjectv++;
						tempi <<= 16;
						tempi &= 0x00ff0000;
						itemp |= tempi;
						tempi = 0;
						tempi |= *pobjectv;
						pobjectv++;
						tempi <<= 8;
						tempi &= 0x0000ff00;
						itemp |= tempi;
						tempi = 0;
						tempi |= *pobjectv;
						pobjectv++;
						tempi &= 0x000000ff;
						itemp |= tempi;
						tscdata->timeconfig->TimeConfigList[j][z].PhaseId = itemp;
						tscdata->timeconfig->TimeConfigList[j][z].GreenTime = *pobjectv;
						pobjectv++;
						tscdata->timeconfig->TimeConfigList[j][z].YellowTime = *pobjectv;
						pobjectv++;
						tscdata->timeconfig->TimeConfigList[j][z].RedTime = *pobjectv;
						pobjectv++;
						tscdata->timeconfig->TimeConfigList[j][z].SpecFunc = *pobjectv;
						pobjectv++;
					}
				}
            }//whole table or single object
		}//else if (0xC1 == objecti[i].objectid)
	}//for (i = 0; i < (itype->objectn + 1); i++)
	
	return SUCCESS;
}

int get_object_value(tscdata_t *tscdata,unsigned char objectid,unsigned char indexn,unsigned char *index, \
						unsigned char cobject,unsigned short *objectvs,unsigned char *objectv)
{
	if ((NULL == objectv) || (NULL == index) || (NULL == tscdata) || (NULL == objectvs))
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char				i = 0,j = 0,z = 0,s = 0;	
	unsigned char				*pobjectv = objectv;
	unsigned short				stemp = 0;
	unsigned int				itemp = 0;
	unsigned char           	totaltr = 0; //total table row
	unsigned char           	mapped = 0;

	*objectvs = 0;
	if (0x8D == objectid)
    {//if (0x8D == objectid)
		if ((0 != indexn) && (0 == cobject))
		{//someone row table
			for (i = 0; i < indexn; i++)
			{//for (i = 0; i < indexn; i++)
				j = index[i]; //get row number of table
				if (j > (tscdata->schedule->FactScheduleNum))
				{
				#ifdef COMMON_DEBUG
					printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					return FAIL;
				}
				*objectvs += 9;
				*pobjectv = tscdata->schedule->ScheduleList[j-1].ScheduleId;
				pobjectv++;
				stemp = tscdata->schedule->ScheduleList[j-1].ScheduleMonth;
				*pobjectv |= (((stemp & 0xff00) >> 8) & 0x00ff);
				pobjectv++;
				*pobjectv |= (stemp & 0x00ff);
				pobjectv++;
				*pobjectv = tscdata->schedule->ScheduleList[j-1].ScheduleWeek;
				pobjectv++;
				itemp = tscdata->schedule->ScheduleList[j-1].ScheduleDay;
				*pobjectv |= (((itemp & 0xff000000) >> 24) & 0x000000ff);
				pobjectv++;
				*pobjectv |= (((itemp & 0x00ff0000) >> 16) & 0x000000ff);
                pobjectv++;
				*pobjectv |= (((itemp & 0x0000ff00) >> 8) & 0x000000ff);
                pobjectv++;
				*pobjectv |= (itemp & 0x000000ff);
                pobjectv++;
				*pobjectv = tscdata->schedule->ScheduleList[j-1].TimeSectionId;
				pobjectv++;
			}//for (i = 0; i < indexn; i++)
		}//someone row table
		else if ((0 != indexn) && (0 != cobject))
		{//someone field
			for (i = 0; i < indexn; i++)
			{//for (i = 0; i < indexn; i++)
				j = index[i]; //get row number of table
				if (j > (tscdata->schedule->FactScheduleNum))
                {
                #ifdef COMMON_DEBUG
                    printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                    return FAIL;
                }
				if (1 == cobject)
				{
					*objectvs += 1;
					*pobjectv = tscdata->schedule->ScheduleList[j-1].ScheduleId;
					pobjectv++;
				}
				else if (2 == cobject)
				{
					*objectvs += 2;
					stemp = tscdata->schedule->ScheduleList[j-1].ScheduleMonth;
					*pobjectv |= (((stemp & 0xff00) >> 8) & 0x00ff);
					pobjectv++;
					*pobjectv |= (stemp & 0x00ff);
					pobjectv++;
				}
				else if (3 == cobject)
				{
                    *objectvs += 1;
                    *pobjectv = tscdata->schedule->ScheduleList[j-1].ScheduleWeek;
                    pobjectv++;
				}
				else if (4 == cobject)
				{
                    *objectvs += 4;
					itemp = tscdata->schedule->ScheduleList[j-1].ScheduleDay;
					*pobjectv |= (((itemp & 0xff000000) >> 24) & 0x000000ff);
                    pobjectv++;
					*pobjectv |= (((itemp & 0x00ff0000) >> 16) & 0x000000ff);
                    pobjectv++;
					*pobjectv |= (((itemp & 0x0000ff00) >> 8) & 0x000000ff);
                    pobjectv++;
					*pobjectv |= (itemp & 0x000000ff);
                    pobjectv++;
				}
				else if (5 == cobject)
				{
                    *objectvs += 1;
                    *pobjectv = tscdata->schedule->ScheduleList[j-1].TimeSectionId;
                    pobjectv++;
				}
			}//for (i = 0; i < indexn; i++)
		}//someone field
		else if ((0 == indexn) && (0 == cobject))
		{//whole table or single object
			*pobjectv = tscdata->schedule->FactScheduleNum;//the row number of 1th dimension 
			pobjectv++;
			*objectvs += 1;
			for (i = 0; i < (tscdata->schedule->FactScheduleNum); i++)
			{//for (i = 0; i < (tscdata->schedule->FactScheduleNum); i++)
				*objectvs += 9;
				*pobjectv = tscdata->schedule->ScheduleList[i].ScheduleId;
                pobjectv++;
                stemp = tscdata->schedule->ScheduleList[i].ScheduleMonth;
                *pobjectv |= (((stemp & 0xff00) >> 8) & 0x00ff);
                pobjectv++;
				*pobjectv |= (stemp & 0x00ff);
                pobjectv++;
                *pobjectv = tscdata->schedule->ScheduleList[i].ScheduleWeek;
                pobjectv++;
                itemp = tscdata->schedule->ScheduleList[i].ScheduleDay;
                *pobjectv |= (((itemp & 0xff000000) >> 24) & 0x000000ff);
                pobjectv++;
				*pobjectv |= (((itemp & 0x00ff0000) >> 16) & 0x000000ff);
                pobjectv++;
				*pobjectv |= (((itemp & 0x0000ff00) >> 8) & 0x000000ff);
                pobjectv++;
				*pobjectv |= (itemp & 0x000000ff);
                pobjectv++;	
                *pobjectv = tscdata->schedule->ScheduleList[i].TimeSectionId;
                pobjectv++;
			}//for (i = 0; i < (tscdata->schedule->FactScheduleNum); i++)	
		}//whole table or single object
    }//if (0x8D == objectid)
    else if (0x8E == objectid)
    {//else if (0x8E == objectid)
		if ((0 != indexn) && (0 == cobject))
		{//someone row table
			for (i = 0; i < indexn; i++)
			{//for (i = 0; i < indexn; i++)
				j = index[i];//get row number of table
				totaltr = 0;
				mapped = 0;
				for (z = 0; ;z++)
				{
					if (0 == (tscdata->timesection->TimeSectionList[z][0].TimeSectionId))
						break;
					for (s = 0; ;s++)
					{
						if (0 == (tscdata->timesection->TimeSectionList[z][s].TimeSectionId))
							break;
						totaltr += 1;
						if (totaltr == j)
						{
							mapped = 1;
							break;
						}
					}//for (s = 0; ;s++)
					if (1 == mapped)
						break;
				}//for (z = 0; ;z++)
				if (0 == mapped)
                {
                #ifdef COMMON_DEBUG
                    printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                    return FAIL;
                }
				*objectvs += 8;
				*pobjectv = tscdata->timesection->TimeSectionList[z][s].TimeSectionId;
				pobjectv++;			
				*pobjectv = tscdata->timesection->TimeSectionList[z][s].EventId;
				pobjectv++;
				*pobjectv = tscdata->timesection->TimeSectionList[z][s].StartHour;
				pobjectv++;
				*pobjectv = tscdata->timesection->TimeSectionList[z][s].StartMinute;
				pobjectv++;
				*pobjectv = tscdata->timesection->TimeSectionList[z][s].ControlMode;
				pobjectv++;
				*pobjectv = tscdata->timesection->TimeSectionList[z][s].PatternId;
				pobjectv++;
				*pobjectv = tscdata->timesection->TimeSectionList[z][s].AuxFunc;
				pobjectv++;
				*pobjectv = tscdata->timesection->TimeSectionList[z][s].SpecFunc;
				pobjectv++;
			}//for (i = 0; i < indexn; i++)
		}//someone row table
		else if ((0 != indexn) && (0 != cobject))
		{//someone field
			for (i = 0; i < indexn; i++)
			{//for (i = 0; i < indexn; i++)
				j = index[i];//get row number of table
                totaltr = 0;
                mapped = 0;
                for (z = 0; ;z++)
                {
					if (0 == (tscdata->timesection->TimeSectionList[z][0].TimeSectionId))
                        break;
                    for (s = 0; ;s++)
                    {
                        if (0 == (tscdata->timesection->TimeSectionList[z][s].TimeSectionId))
                            break;
                        totaltr += 1;
                        if (totaltr == j)
                        {
                            mapped = 1;
                            break;
                        }
                    }//for (s = 0; ;s++)
                    if (1 == mapped)
                        break;
                }//for (z = 0; ;z++)
				if (0 == mapped)
                {
                #ifdef COMMON_DEBUG
                    printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                    return FAIL;
                }
				if (1 == cobject)
				{
					*objectvs += 1;
					*pobjectv = tscdata->timesection->TimeSectionList[z][s].TimeSectionId;
					pobjectv++;
				}
				else if (2 == cobject)
				{
					*objectvs += 1;
                    *pobjectv = tscdata->timesection->TimeSectionList[z][s].EventId;
                    pobjectv++;
				}
				else if (3 == cobject)
				{
					*objectvs += 1;
                    *pobjectv = tscdata->timesection->TimeSectionList[z][s].StartHour;
                    pobjectv++;
				}
				else if (4 == cobject)
				{
					*objectvs += 1;
                    *pobjectv = tscdata->timesection->TimeSectionList[z][s].StartMinute;
                    pobjectv++;
				}
				else if (5 == cobject)
				{
					*objectvs += 1;
                    *pobjectv = tscdata->timesection->TimeSectionList[z][s].ControlMode;
                    pobjectv++;
				}
				else if (6 == cobject)
				{
					*objectvs += 1;
                    *pobjectv = tscdata->timesection->TimeSectionList[z][s].PatternId;
                    pobjectv++;
				}
				else if (7 == cobject)
				{
					*objectvs += 1;
                    *pobjectv = tscdata->timesection->TimeSectionList[z][s].AuxFunc;
                    pobjectv++;
				}
				else if (8 == cobject)
				{
					*objectvs += 1;
                    *pobjectv = tscdata->timesection->TimeSectionList[z][s].SpecFunc;
                    pobjectv++;
				}
			}//for (i = 0; i < indexn; i++)
		}//someone field
		else if ((0 == indexn) && (0 == cobject))
		{//single object or whole table
			*pobjectv = tscdata->timesection->FactTimeSectionNum; //the row num of 1th dimension
			pobjectv++;
			*pobjectv = tscdata->timesection->FactEventNum;
			pobjectv++;
			*objectvs += 2;
			for (z = 0; z < (tscdata->timesection->FactTimeSectionNum); z++)
			{
				for (s = 0; s < (tscdata->timesection->FactEventNum); s++)
                {
					*objectvs += 8;
					*pobjectv = tscdata->timesection->TimeSectionList[z][s].TimeSectionId;
					pobjectv++;
					*pobjectv = tscdata->timesection->TimeSectionList[z][s].EventId;
					pobjectv++;
					*pobjectv = tscdata->timesection->TimeSectionList[z][s].StartHour;
					pobjectv++;
					*pobjectv = tscdata->timesection->TimeSectionList[z][s].StartMinute;
                    pobjectv++;
					*pobjectv = tscdata->timesection->TimeSectionList[z][s].ControlMode;
                    pobjectv++;
					*pobjectv = tscdata->timesection->TimeSectionList[z][s].PatternId;
                    pobjectv++;
					*pobjectv = tscdata->timesection->TimeSectionList[z][s].AuxFunc;
                    pobjectv++;
					*pobjectv = tscdata->timesection->TimeSectionList[z][s].SpecFunc;
                    pobjectv++;
                }
			}//for (z = 0; z < (tscdata->timesection->FactTimeSectionNum); z++)
		}//single object or whole table
    }//else if (0x8E == objectid)
    else if (0x95 == objectid)
    {//else if (0x95 == objectid)
		if ((0 != indexn) && (0 == cobject))
        {//someone row table
			for (i = 0; i < indexn; i++)
			{//for (i = 0; i < indexn; i++)
				j = index[i];
				if (j > (tscdata->phase->FactPhaseNum))	
				{
				#ifdef COMMON_DEBUG
					printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					return FAIL;
				}
				*objectvs += 12;
				*pobjectv = tscdata->phase->PhaseList[j-1].PhaseId;
				pobjectv++;
				*pobjectv = tscdata->phase->PhaseList[j-1].PhaseWalkGreen;
				pobjectv++;
				*pobjectv = tscdata->phase->PhaseList[j-1].PhaseWalkClear;
				pobjectv++;
				*pobjectv = tscdata->phase->PhaseList[j-1].PhaseMinGreen;
				pobjectv++;
				*pobjectv = tscdata->phase->PhaseList[j-1].PhaseGreenDelay;
				pobjectv++;
				*pobjectv = tscdata->phase->PhaseList[j-1].PhaseMaxGreen1;
				pobjectv++;
				*pobjectv = tscdata->phase->PhaseList[j-1].PhaseMaxGreen2;
				pobjectv++;
				*pobjectv = tscdata->phase->PhaseList[j-1].PhaseFixGreen;
				pobjectv++;
				*pobjectv = tscdata->phase->PhaseList[j-1].PhaseGreenFlash;
				pobjectv++;
				*pobjectv = tscdata->phase->PhaseList[j-1].PhaseType;
				pobjectv++;
				*pobjectv = tscdata->phase->PhaseList[j-1].PhaseSpecFunc;
				pobjectv++;
				*pobjectv = tscdata->phase->PhaseList[j-1].PhaseReserved;
				pobjectv++;
			}//for (i = 0; i < indexn; i++)
        }//someone row table
        else if ((0 != indexn) && (0 != cobject))
        {//someone field
			for (i = 0; i < indexn; i++)
			{//for (i = 0; i < indexn; i++)
				j = index[i]; //get row number of table
				if (j > (tscdata->phase->FactPhaseNum))
                {
                #ifdef COMMON_DEBUG
                    printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                    return FAIL;
                }
				if (1 == cobject)
				{
					*objectvs += 1;
					*pobjectv = tscdata->phase->PhaseList[j-1].PhaseId;
					pobjectv++;
				}
				else if (2 == cobject)
				{
                    *objectvs += 1;
                    *pobjectv = tscdata->phase->PhaseList[j-1].PhaseWalkGreen;
                    pobjectv++;	
				}
				else if (3 == cobject)
				{
                    *objectvs += 1;
                    *pobjectv = tscdata->phase->PhaseList[j-1].PhaseWalkClear;
                    pobjectv++;
				}
				else if (4 == cobject)
				{
                    *objectvs += 1;
                    *pobjectv = tscdata->phase->PhaseList[j-1].PhaseMinGreen;
                    pobjectv++;	
				}
				else if (5 == cobject)
				{
                    *objectvs += 1;
                    *pobjectv = tscdata->phase->PhaseList[j-1].PhaseGreenDelay;
                    pobjectv++;
				}
				else if (6 == cobject)
				{
                    *objectvs += 1;
                    *pobjectv = tscdata->phase->PhaseList[j-1].PhaseMaxGreen1;
                    pobjectv++;
				}
				else if (7 == cobject)
                {
                    *objectvs += 1;
                    *pobjectv = tscdata->phase->PhaseList[j-1].PhaseMaxGreen2;
                    pobjectv++;
                }
				else if (8 == cobject)
                {
                    *objectvs += 1;
                    *pobjectv = tscdata->phase->PhaseList[j-1].PhaseFixGreen;
                    pobjectv++;
                }
				else if (9 == cobject)
                {
                    *objectvs += 1;
                    *pobjectv = tscdata->phase->PhaseList[j-1].PhaseGreenFlash;
                    pobjectv++;
                }
				else if (10 == cobject)
                {
                    *objectvs += 1;
                    *pobjectv = tscdata->phase->PhaseList[j-1].PhaseType;
                    pobjectv++;
                }
				else if (11 == cobject)
                {
                    *objectvs += 1;
                    *pobjectv = tscdata->phase->PhaseList[j-1].PhaseSpecFunc;
                    pobjectv++;
                }
				else if (12 == cobject)
                {
                    *objectvs += 1;
                    *pobjectv = tscdata->phase->PhaseList[j-1].PhaseReserved;
                    pobjectv++;
                }
			}//for (i = 0; i < indexn; i++)
        }//someone field
        else if ((0 == indexn) && (0 == cobject))
        {//single object or whole table
			*pobjectv = tscdata->phase->FactPhaseNum;//the row number of 1th dimension 
			pobjectv++;
			*objectvs += 1;
			for (i = 0; i < (tscdata->phase->FactPhaseNum); i++)
			{//for (i = 0; i < (tscdata->phase->FactPhaseNum); i++)
				*objectvs += 12;
				*pobjectv = tscdata->phase->PhaseList[i].PhaseId;
                pobjectv++;
            	*pobjectv = tscdata->phase->PhaseList[i].PhaseWalkGreen;
                pobjectv++;
                *pobjectv = tscdata->phase->PhaseList[i].PhaseWalkClear;
                pobjectv++;
                *pobjectv = tscdata->phase->PhaseList[i].PhaseMinGreen;
                pobjectv++;
                *pobjectv = tscdata->phase->PhaseList[i].PhaseGreenDelay;
                pobjectv++;
                *pobjectv = tscdata->phase->PhaseList[i].PhaseMaxGreen1;
                pobjectv++;
                *pobjectv = tscdata->phase->PhaseList[i].PhaseMaxGreen2;
                pobjectv++;
                *pobjectv = tscdata->phase->PhaseList[i].PhaseFixGreen;
                pobjectv++;
                *pobjectv = tscdata->phase->PhaseList[i].PhaseGreenFlash;
                pobjectv++;
                *pobjectv = tscdata->phase->PhaseList[i].PhaseType;
                pobjectv++;
                *pobjectv = tscdata->phase->PhaseList[i].PhaseSpecFunc;
                pobjectv++;
                *pobjectv = tscdata->phase->PhaseList[i].PhaseReserved;
                pobjectv++;    
			}//for (i = 0; i < (tscdata->phase->FactPhaseNum); i++)
        }//single object or whole table
    }//else if (0x95 == objectid)
    else if (0x97 == objectid)
    {//else if (0x97 == objectid)
		if ((0 != indexn) && (0 == cobject))
        {//someone row table
			for (i = 0; i < indexn; i++)
			{//for (i = 0; i < indexn; i++)
				j = index[i];
				if (j > (tscdata->phaseerror->FactPhaseErrorNum))	
				{
				#ifdef COMMON_DEBUG
					printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					return FAIL;
				}
				*objectvs += 5;
				*pobjectv = tscdata->phaseerror->PhaseErrorList[j-1].PhaseId;
				pobjectv++;
				itemp = tscdata->phaseerror->PhaseErrorList[j-1].PhaseConflict;
                *pobjectv |= (((itemp & 0xff000000) >> 24) & 0x000000ff);
                pobjectv++;
				*pobjectv |= (((itemp & 0x00ff0000) >> 16) & 0x000000ff);
                pobjectv++;
				*pobjectv |= (((itemp & 0x0000ff00) >> 8) & 0x000000ff);
                pobjectv++;
				*pobjectv |= (itemp & 0x000000ff);
                pobjectv++;	
			}//for (i = 0; i < indexn; i++)
        }//someone row table
        else if ((0 != indexn) && (0 != cobject))
        {//someone field
			for (i = 0; i < indexn; i++)
			{//for (i = 0; i < indexn; i++)
				j = index[i]; //get row number of table
				if (j > (tscdata->phaseerror->FactPhaseErrorNum))
                {
                #ifdef COMMON_DEBUG
                    printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                    return FAIL;
                }
				if (1 == cobject)
				{
					*objectvs += 1;
					*pobjectv = tscdata->phaseerror->PhaseErrorList[j-1].PhaseId;
					pobjectv++;
				}
				else if (2 == cobject)
				{
					*objectvs += 4;
					itemp = tscdata->phaseerror->PhaseErrorList[j-1].PhaseConflict;
                	*pobjectv |= (((itemp & 0xff000000) >> 24) & 0x000000ff);
                	pobjectv++;
					*pobjectv |= (((itemp & 0x00ff0000) >> 16) & 0x000000ff);
                    pobjectv++;
					*pobjectv |= (((itemp & 0x0000ff00) >> 8) & 0x000000ff);
                    pobjectv++;
					*pobjectv |= (itemp & 0x000000ff);
                    pobjectv++;
				}
			}
        }//someone field
        else if ((0 == indexn) && (0 == cobject))
        {//single object or whole table
			*pobjectv = tscdata->phaseerror->FactPhaseErrorNum;//the row number of 1th dimension 
			pobjectv++;
			*objectvs += 1;
			for (i = 0; i < (tscdata->phaseerror->FactPhaseErrorNum); i++)
			{//for (i = 0; i < (tscdata->phaseerror->FactPhaseErrorNum); i++)
				*objectvs += 5;
				*pobjectv = tscdata->phaseerror->PhaseErrorList[i].PhaseId;
                pobjectv++;
				itemp = tscdata->phaseerror->PhaseErrorList[i].PhaseConflict;
				*pobjectv |= (((itemp & 0xff000000) >> 24) & 0x000000ff);
				pobjectv++;
				*pobjectv |= (((itemp & 0x00ff0000) >> 16) & 0x000000ff);
                pobjectv++;
				*pobjectv |= (((itemp & 0x0000ff00) >> 8) & 0x000000ff);
                pobjectv++;
				*pobjectv |= (itemp & 0x000000ff);
                pobjectv++;
			}//for (i = 0; i < (tscdata->phaseerror->FactPhaseErrorNum); i++)
        }//single object or whole table
    }//else if (0x97 == objectid)
    else if (0x9F == objectid)
    {//else if (0x9F == objectid)
		if ((0 != indexn) && (0 == cobject))
        {//someone row table
			for (i = 0; i < indexn; i++)
			{
				j = index[i];
				if (j > (tscdata->detector->FactDetectorNum))	
				{
				#ifdef COMMON_DEBUG
					printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					return FAIL;
				}
				*objectvs += 9;
				*pobjectv = tscdata->detector->DetectorList[j-1].DetectorId;
				pobjectv++;
				*pobjectv = tscdata->detector->DetectorList[j-1].DetectorPhase;	
				pobjectv++;
				*pobjectv = tscdata->detector->DetectorList[j-1].DetectorType;
				pobjectv++;
				*pobjectv = tscdata->detector->DetectorList[j-1].DetectorDirect;
				pobjectv++;
				*pobjectv = tscdata->detector->DetectorList[j-1].DetectorDelay;
				pobjectv++;
				*pobjectv = tscdata->detector->DetectorList[j-1].DetectorSpecFunc;
				pobjectv++;
				stemp = tscdata->detector->DetectorList[j-1].DetectorFlow;
				*pobjectv |= (((stemp & 0xff00) >> 8) & 0x00ff);
				pobjectv++;
				*pobjectv |= (stemp & 0x00ff);
                pobjectv++;
				*pobjectv = tscdata->detector->DetectorList[j-1].DetectorOccupy;
				pobjectv++;
			}
        }//someone row table
        else if ((0 != indexn) && (0 != cobject))
        {//someone field
			for (i = 0; i < indexn; i++)
			{//for (i = 0; i < indexn; i++)
				j = index[i]; //get row number of table
				if (j > (tscdata->detector->FactDetectorNum))
                {
                #ifdef COMMON_DEBUG
                    printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                    return FAIL;
                }
				if (1 == cobject)
				{
					*objectvs += 1;
					*pobjectv = tscdata->detector->DetectorList[j-1].DetectorId;
					pobjectv++;
				}
				else if (2 == cobject)
				{
					*objectvs += 1;
                    *pobjectv = tscdata->detector->DetectorList[j-1].DetectorPhase;
                    pobjectv++;
				}
				else if (3 == cobject)
                {
                    *objectvs += 1;
                    *pobjectv = tscdata->detector->DetectorList[j-1].DetectorType;
                    pobjectv++;
                }
				else if (4 == cobject)
                {   
                    *objectvs += 1;
                    *pobjectv = tscdata->detector->DetectorList[j-1].DetectorDirect;
                    pobjectv++;
                }
				else if (5 == cobject)
                {   
                    *objectvs += 1;
                    *pobjectv = tscdata->detector->DetectorList[j-1].DetectorDelay;
                    pobjectv++;
                }
				else if (6 == cobject)
                {   
                    *objectvs += 1;
                    *pobjectv = tscdata->detector->DetectorList[j-1].DetectorSpecFunc;
                    pobjectv++;
                }
				else if (7 == cobject)
                {   
                    *objectvs += 2;
                    stemp = tscdata->detector->DetectorList[j-1].DetectorFlow;
					*pobjectv |= (((stemp & 0xff00) >> 8) & 0x00ff);
					pobjectv++;
					*pobjectv |= (stemp & 0x00ff);
                    pobjectv++;
                }
				else if (8 == cobject)
				{
					*objectvs += 1;
					*pobjectv = tscdata->detector->DetectorList[j-1].DetectorOccupy;
					pobjectv++;
				}
			}//for (i = 0; i < indexn; i++)
        }//someone field
        else if ((0 == indexn) && (0 == cobject))
        {//single object or whole table
			*pobjectv = tscdata->detector->FactDetectorNum;//the row number of 1th dimension 
			pobjectv++;
			*objectvs += 1;
			for (i = 0; i < (tscdata->detector->FactDetectorNum); i++)
			{//for (i = 0; i < (tscdata->detector->FactDetectorNum); i++)
				*objectvs += 9;
				*pobjectv = tscdata->detector->DetectorList[i].DetectorId;
                pobjectv++;
				*pobjectv = tscdata->detector->DetectorList[i].DetectorPhase;
				pobjectv++;
				*pobjectv = tscdata->detector->DetectorList[i].DetectorType;
				pobjectv++;
				*pobjectv = tscdata->detector->DetectorList[i].DetectorDirect;
                pobjectv++;
				*pobjectv = tscdata->detector->DetectorList[i].DetectorDelay;
                pobjectv++;
				*pobjectv = tscdata->detector->DetectorList[i].DetectorSpecFunc;
                pobjectv++;
				stemp = tscdata->detector->DetectorList[i].DetectorFlow;
				*pobjectv |= (((stemp & 0xff00) >> 8) & 0x00ff);
				pobjectv++;
				*pobjectv |= (stemp & 0x00ff);
                pobjectv++;
				*pobjectv = tscdata->detector->DetectorList[i].DetectorOccupy;
				pobjectv++;
			}//for (i = 0; i < (tscdata->detector->FactDetectorNum); i++)
        }//single object or whole table
    }//else if (0x9F == objectid)
    else if (0xB0 == objectid)
    {//else if (0xB0 == objectid)
		if ((0 != indexn) && (0 == cobject))
        {//someone row table
			for (i = 0; i < indexn; i++)
			{
				j = index[i];
				if (j > (tscdata->channel->FactChannelNum))	
				{
				#ifdef COMMON_DEBUG
					printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					return FAIL;
				}
				*objectvs += 4;
				*pobjectv = tscdata->channel->ChannelList[j-1].ChannelId;
				pobjectv++;
				*pobjectv = tscdata->channel->ChannelList[j-1].ChannelCtrlSrc;
				pobjectv++;
				*pobjectv = tscdata->channel->ChannelList[j-1].ChannelFlash;
				pobjectv++;
				*pobjectv = tscdata->channel->ChannelList[j-1].ChannelType;
				pobjectv++;
			}
        }//someone row table
        else if ((0 != indexn) && (0 != cobject))
        {//someone field
			for (i = 0; i < indexn; i++)
			{
				j = index[i];
                if (j > (tscdata->channel->FactChannelNum))
                {
                #ifdef COMMON_DEBUG
                    printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                    return FAIL;
                }
				if (1 == cobject)
				{
					*objectvs += 1;
					*pobjectv = tscdata->channel->ChannelList[j-1].ChannelId;
					pobjectv++;
				}
				else if (2 == cobject)
				{
					*objectvs += 1;
                    *pobjectv = tscdata->channel->ChannelList[j-1].ChannelCtrlSrc;
                    pobjectv++;
				}
				else if (3 == cobject)
                {
                    *objectvs += 1;
                    *pobjectv = tscdata->channel->ChannelList[j-1].ChannelFlash;
                    pobjectv++;
                }
				else if (4 == cobject)
                {   
                    *objectvs += 1;
                    *pobjectv = tscdata->channel->ChannelList[j-1].ChannelType;
                    pobjectv++;
                }
			}
        }//someone field
        else if ((0 == indexn) && (0 == cobject))
        {//single object or whole table
			*pobjectv = tscdata->channel->FactChannelNum;//the row number of 1th dimension 
			pobjectv++;
			*objectvs += 1;
			for (i = 0; i < (tscdata->channel->FactChannelNum); i++)
			{//for (i = 0; i < (tscdata->channel->FactChannelNum); i++)
				*objectvs += 4;
				*pobjectv = tscdata->channel->ChannelList[i].ChannelId;
                pobjectv++;
				*pobjectv = tscdata->channel->ChannelList[i].ChannelCtrlSrc;
				pobjectv++;
				*pobjectv = tscdata->channel->ChannelList[i].ChannelFlash;
				pobjectv++;
				*pobjectv = tscdata->channel->ChannelList[i].ChannelType;
                pobjectv++;
			}//for (i = 0; i < (tscdata->channel->FactChannelNum); i++)
        }//single object or whole table
    }//else if (0xB0 == objectid)
    else if (0xC0 == objectid)
    {//else if (0xC0 == objectid)
		if ((0 != indexn) && (0 == cobject))
        {//someone row table
			for (i = 0; i < indexn; i++)
			{
				j = index[i];
				if (j > (tscdata->pattern->FactPatternNum))	
				{
				#ifdef COMMON_DEBUG
					printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					return FAIL;
				}
				*objectvs += 7; //modified on 20150414
				*pobjectv = tscdata->pattern->PatternList[j-1].PatternId;
				pobjectv++;
				stemp = tscdata->pattern->PatternList[j-1].CycleTime;
				*pobjectv |= ((stemp & 0xff00) >> 8) & 0x00ff;
				pobjectv++;
				*pobjectv |= (stemp & 0x00ff);
                pobjectv++;
//				*pobjectv = tscdata->pattern->PatternList[j-1].PhaseOffset;
//				pobjectv++;
			//modified on 20150414
				stemp = tscdata->pattern->PatternList[j-1].PhaseOffset;
				*pobjectv |= ((stemp & 0xff00) >> 8) & 0x00ff;
                pobjectv++;
                *pobjectv |= (stemp & 0x00ff);
                pobjectv++;
			//end modify
				*pobjectv = tscdata->pattern->PatternList[j-1].CoordPhase;
				pobjectv++;
				*pobjectv = tscdata->pattern->PatternList[j-1].TimeConfigId;
                pobjectv++;
			}
        }//someone row table
        else if ((0 != indexn) && (0 != cobject))
        {//someone field
			for (i = 0; i < indexn; i++)
			{
				j = index[i];
                if (j > (tscdata->pattern->FactPatternNum))
                {
                #ifdef COMMON_DEBUG
                    printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                    return FAIL;
                }
				if (1 == cobject)
				{
					*objectvs += 1;
					*pobjectv = tscdata->pattern->PatternList[j-1].PatternId;
					pobjectv++;
				}
				else if (2 == cobject)
				{
					*objectvs += 2;
					stemp = tscdata->pattern->PatternList[j-1].CycleTime;
                    *pobjectv = ((stemp & 0xff00) >> 8) & 0x00ff;
                    pobjectv++;
					*pobjectv = (stemp & 0x00ff);
                    pobjectv++;
				}
				else if (3 == cobject)
                {
				#if 0
                    *objectvs += 1;
                    *pobjectv = tscdata->pattern->PatternList[j-1].PhaseOffset;
                    pobjectv++;
				#endif
				//modified on 20150414
					*objectvs += 2;
                    stemp = tscdata->pattern->PatternList[j-1].PhaseOffset;
                    *pobjectv = ((stemp & 0xff00) >> 8) & 0x00ff;
                    pobjectv++;
                    *pobjectv = (stemp & 0x00ff);
                    pobjectv++;
				//end modify
                }
				else if (4 == cobject)
                {   
                    *objectvs += 1;
                    *pobjectv = tscdata->pattern->PatternList[j-1].CoordPhase;
                    pobjectv++;
                }
				else if (5 == cobject)
				{
					*objectvs += 1;
					*pobjectv = tscdata->pattern->PatternList[j-1].TimeConfigId;
					pobjectv++;
				}
			}
        }//someone field
        else if ((0 == indexn) && (0 == cobject))
        {//single object or whole table
			*pobjectv = tscdata->pattern->FactPatternNum;//the row number of 1th dimension 
			pobjectv++;
			*objectvs += 1;
			for (i = 0; i < (tscdata->pattern->FactPatternNum); i++)
			{//for (i = 0; i < (tscdata->pattern->FactPatternNum); i++)
				*objectvs += 7; //modified on 20150414
				*pobjectv = tscdata->pattern->PatternList[i].PatternId;
                pobjectv++;
                stemp = tscdata->pattern->PatternList[i].CycleTime;
                *pobjectv |= ((stemp & 0xff00) >> 8) & 0x00ff;
                pobjectv++;
				*pobjectv |= (stemp & 0x00ff);
                pobjectv++;
//                *pobjectv = tscdata->pattern->PatternList[i].PhaseOffset;
//                pobjectv++;
				//modified on 20150414
				stemp = tscdata->pattern->PatternList[i].PhaseOffset;
                *pobjectv |= ((stemp & 0xff00) >> 8) & 0x00ff;
                pobjectv++;
                *pobjectv |= (stemp & 0x00ff);
                pobjectv++;
				//end modify
                *pobjectv = tscdata->pattern->PatternList[i].CoordPhase;
                pobjectv++;
                *pobjectv = tscdata->pattern->PatternList[i].TimeConfigId;
                pobjectv++;
			}//for (i = 0; i < (tscdata->pattern->FactPatternNum); i++)
        }//single object or whole table
    }//else if (0xC0 == objectid)
    else if (0xC1 == objectid)
    {//else if (0xC1 == objectid)
		if ((0 != indexn) && (0 == cobject))
        {//someone row table
			for (i = 0; i < indexn; i++)
			{//for (i = 0; i < indexn; i++)
				j = index[i];//get row number of table
				totaltr = 0;
				mapped = 0;
				for (z = 0; ;z++)
				{
					if (0 == (tscdata->timeconfig->TimeConfigList[z][0].TimeConfigId))
						break;
					for (s = 0; ;s++)
					{
						if (0 == (tscdata->timeconfig->TimeConfigList[z][s].TimeConfigId))
							break;
						totaltr += 1;
						if (totaltr == j)
						{
							mapped = 1;
							break;
						}
					}//for (s = 0; ;s++)
					if (1 == mapped)
						break;
				}//for (z = 0; ;z++)
				if (0 == mapped)
                {
                #ifdef COMMON_DEBUG
                    printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                    return FAIL;
                }
				*objectvs += 10;
				*pobjectv = tscdata->timeconfig->TimeConfigList[z][s].TimeConfigId;
				pobjectv++;			
				*pobjectv = tscdata->timeconfig->TimeConfigList[z][s].StageId;
				pobjectv++;
				itemp = tscdata->timeconfig->TimeConfigList[z][s].PhaseId;
				*pobjectv |= ((itemp & 0xff000000) >> 24) & 0x000000ff;
                pobjectv++;
				*pobjectv |= ((itemp & 0x00ff0000) >> 16) & 0x000000ff;
                pobjectv++;
				*pobjectv |= ((itemp & 0x0000ff00) >> 8) & 0x000000ff;
                pobjectv++;
				*pobjectv |= (itemp & 0x000000ff);
                pobjectv++;
				*pobjectv = tscdata->timeconfig->TimeConfigList[z][s].GreenTime;
				pobjectv++;
				*pobjectv = tscdata->timeconfig->TimeConfigList[z][s].YellowTime;
				pobjectv++;
				*pobjectv = tscdata->timeconfig->TimeConfigList[z][s].RedTime;
				pobjectv++;
				*pobjectv = tscdata->timeconfig->TimeConfigList[z][s].SpecFunc;
				pobjectv++;
			}//for (i = 0; i < indexn; i++)
        }//someone row table
        else if ((0 != indexn) && (0 != cobject))
        {//someone field
			for (i = 0; i < indexn; i++)
			{//for (i = 0; i < indexn; i++)
				j = index[i];//get row number of table
                totaltr = 0;
                mapped = 0;
                for (z = 0; ;z++)
                {
					if (0 == (tscdata->timeconfig->TimeConfigList[z][0].TimeConfigId))
                        break;
                    for (s = 0; ;s++)
                    {
                        if (0 == (tscdata->timeconfig->TimeConfigList[z][s].TimeConfigId))
                            break;
                        totaltr += 1;
                        if (totaltr == j)
                        {
                            mapped = 1;
                            break;
                        }
                    }//for (s = 0; ;s++)
                    if (1 == mapped)
                        break;
                }//for (z = 0; ;z++)
				if (0 == mapped)
                {
                #ifdef COMMON_DEBUG
                    printf("Not fit row number,File: %s,Line: %d\n",__FILE__,__LINE__);
                #endif
                    return FAIL;
                }
				if (1 == cobject)
				{
					*objectvs += 1;
					*pobjectv = tscdata->timeconfig->TimeConfigList[z][s].TimeConfigId;
					pobjectv++;
				}
				else if (2 == cobject)
				{
					*objectvs += 1;
                    *pobjectv = tscdata->timeconfig->TimeConfigList[z][s].StageId;
                    pobjectv++;
				}
				else if (3 == cobject)
				{
					*objectvs += 4;
					itemp = tscdata->timeconfig->TimeConfigList[z][s].PhaseId;
                	*pobjectv |= ((itemp & 0xff000000) >> 24) & 0x000000ff;
                	pobjectv++;
					*pobjectv |= ((itemp & 0x00ff0000) >> 16) & 0x000000ff;
                    pobjectv++;
					*pobjectv |= ((itemp & 0x0000ff00) >> 8) & 0x000000ff;
                    pobjectv++;
					*pobjectv |= (itemp & 0x000000ff);
                    pobjectv++;
				}
				else if (4 == cobject)
				{
					*objectvs += 1;
                    *pobjectv = tscdata->timeconfig->TimeConfigList[z][s].GreenTime;
                    pobjectv++;
				}
				else if (5 == cobject)
				{
					*objectvs += 1;
                    *pobjectv = tscdata->timeconfig->TimeConfigList[z][s].YellowTime;
                    pobjectv++;
				}
				else if (6 == cobject)
				{
					*objectvs += 1;
                    *pobjectv = tscdata->timeconfig->TimeConfigList[z][s].RedTime;
                    pobjectv++;
				}
				else if (7 == cobject)
				{
					*objectvs += 1;
                    *pobjectv = tscdata->timeconfig->TimeConfigList[z][s].SpecFunc;
                    pobjectv++;
				}
			}//for (i = 0; i < indexn; i++)
        }//someone field
        else if ((0 == indexn) && (0 == cobject))
        {//single object or whole table
			*pobjectv = tscdata->timeconfig->FactTimeConfigNum; //the row num of 1th dimension
			pobjectv++;
			*pobjectv = tscdata->timeconfig->FactStageNum;
			pobjectv++;
			*objectvs += 2;
			for (z = 0; z < (tscdata->timeconfig->FactTimeConfigNum); z++)
			{
				for (s = 0; s < (tscdata->timeconfig->FactStageNum); s++)
                {
					*objectvs += 10;
					*pobjectv = tscdata->timeconfig->TimeConfigList[z][s].TimeConfigId;
					pobjectv++;
					*pobjectv = tscdata->timeconfig->TimeConfigList[z][s].StageId;
					pobjectv++;
					itemp = tscdata->timeconfig->TimeConfigList[z][s].PhaseId;
					*pobjectv |= ((itemp & 0xff000000) >> 24) & 0x000000ff;
                    pobjectv++;
					*pobjectv |= ((itemp & 0x00ff0000) >> 16) & 0x000000ff;
                    pobjectv++;
					*pobjectv |= ((itemp & 0x0000ff00) >> 8) & 0x000000ff;
                    pobjectv++;
					*pobjectv |= (itemp & 0x000000ff);
                    pobjectv++;
					*pobjectv = tscdata->timeconfig->TimeConfigList[z][s].GreenTime;
                    pobjectv++;
					*pobjectv = tscdata->timeconfig->TimeConfigList[z][s].YellowTime;
                    pobjectv++;
					*pobjectv = tscdata->timeconfig->TimeConfigList[z][s].RedTime;
                    pobjectv++;
					*pobjectv = tscdata->timeconfig->TimeConfigList[z][s].SpecFunc;
                    pobjectv++;
                }
			}//for (z = 0; z < (tscdata->timesection->FactTimeSectionNum); z++)
        }//single object or whole table
    }//else if (0xC1 == objectid)
    else
    {
    #ifdef COMMON_DEBUG
        printf("Not fit object ID,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
        return FAIL;
    }

	
	return SUCCESS;
}

int err_data_pack(unsigned char *sbuf,infotype_t *itype,unsigned char errs,unsigned char errindex)
{
	if ((NULL == sbuf) || (NULL == itype))
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char			*psbuf = sbuf;
	unsigned char			temp = 0;

	//protol header
	*psbuf = 0x21;
	psbuf++;
	//end protol header pack

	//info type field
	temp = 0;
	temp |= itype->highbit;
	temp <<= 7;
	temp &= 0x80;
	*psbuf |= temp;
	temp = 0;
	temp |= itype->objectn;
	temp <<= 4;
	temp &= 0x70;
	*psbuf |= temp;
	temp = 0;
	temp |= itype->opertype;
	temp &= 0x0f;
	*psbuf |= temp;
	psbuf++;	
	//end info type field pack

	//err status
	*psbuf = errs;
	psbuf++;
	//end err status
	
	//err index
	*psbuf = errindex;
	psbuf++;
	//end err index

	return SUCCESS;
}

int control_data_pack(unsigned char *sbuf,infotype_t *itype,objectinfo_t *objecti,unsigned short *factovs,unsigned char mark)
{
	if ((NULL == sbuf) || (NULL == itype) || (NULL == objecti) || (NULL == factovs))
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,FILE: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	unsigned char				*psbuf = sbuf;
	unsigned char				temp = 0;
	unsigned short				i = 0,j = 0;

	*factovs = 0;
	//protol header
	*psbuf = 0x21;
	psbuf++;
	*factovs += 1;
	//end protol header pack

	//info type field
	temp = 0;
	temp |= itype->highbit;
	temp <<= 7;
	temp &= 0x80;
	*psbuf |= temp;
	temp = 0;
	temp |= itype->objectn;
	temp <<= 4;
	temp &= 0x70;
	*psbuf |= temp;
	temp = 0;
	temp |= itype->opertype;
	temp &= 0x0f;
	*psbuf |= temp;
	psbuf++;
	*factovs += 1;	
	//end info type field pack

	//info field
	for (i = 0; i < ((itype->objectn) + 1); i++)
	{
		*psbuf = objecti[i].objectid;//object id
		//index number and child object
		psbuf++;
		*factovs += 1;
		temp = 0;
		temp |= objecti[i].indexn;
		temp <<= 6;
		temp &= 0xc0;
		*psbuf |= temp;
		temp = 0;
		temp |= objecti[i].cobject;
		temp &= 0x3f;
		*psbuf |= temp;

		psbuf++;
		*factovs += 1;
		//index number and child object;

		for (j = 0; j < objecti[i].indexn; j++)
		{
			*psbuf = objecti[i].index[j];
			psbuf++;
			*factovs += 1;
		}
		//index value

		//object value
		for (j = 0; j < objecti[i].objectvs; j++)
		{
			*psbuf = objecti[i].objectv[j];
			psbuf++; 
			*factovs += 1;
		}
		//object value
	}
	//end info field pack
	*psbuf = mark;
	psbuf++;
	*factovs += 1;

	return SUCCESS;	
}

int fastforward_data_pack(unsigned char *sbuf,infotype_t *itype,objectinfo_t *objecti,unsigned short *factovs,unsigned char mark)
{
	if ((NULL == sbuf) || (NULL == itype) || (NULL == objecti) || (NULL == factovs))
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,FILE: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	unsigned char				*psbuf = sbuf;
	unsigned char				temp = 0;
	unsigned short				i = 0,j = 0;

	*factovs = 0;
	//protol header
	*psbuf = 0x21;
	psbuf++;
	*factovs += 1;
	//end protol header pack

	//info type field
	temp = 0;
	temp |= itype->highbit;
	temp <<= 7;
	temp &= 0x80;
	*psbuf |= temp;
	temp = 0;
	temp |= itype->objectn;
	temp <<= 4;
	temp &= 0x70;
	*psbuf |= temp;
	temp = 0;
	temp |= itype->opertype;
	temp &= 0x0f;
	*psbuf |= temp;
	psbuf++;
	*factovs += 1;	
	//end info type field pack

	//info field
	for (i = 0; i < ((itype->objectn) + 1); i++)
	{
		*psbuf = objecti[i].objectid;//object id
		//index number and child object
		psbuf++;
		*factovs += 1;
		temp = 0;
		temp |= objecti[i].indexn;
		temp <<= 6;
		temp &= 0xc0;
		*psbuf |= temp;
		temp = 0;
		temp |= objecti[i].cobject;
		temp &= 0x3f;
		*psbuf |= temp;

		psbuf++;
		*factovs += 1;
		//index number and child object;

		for (j = 0; j < objecti[i].indexn; j++)
		{
			*psbuf = objecti[i].index[j];
			psbuf++;
			*factovs += 1;
		}
		//index value

		//object value
		for (j = 0; j < objecti[i].objectvs; j++)
		{
			*psbuf = objecti[i].objectv[j];
			psbuf++; 
			*factovs += 1;
		}
		//object value
	}
	//end info field pack

	*psbuf = mark;
	psbuf++;
	*factovs += 1;

	return SUCCESS;
}

int server_data_pack(unsigned char *sbuf,infotype_t *itype,objectinfo_t *objecti,unsigned short *factovs)
{
	if ((NULL == sbuf) || (NULL == itype) || (NULL == objecti) || (NULL == factovs))
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,FILE: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	unsigned char				*psbuf = sbuf;
	unsigned char				temp = 0;
	unsigned short				i = 0,j = 0;

	*factovs = 0;
	//protol header
	*psbuf = 0x21;
	psbuf++;
	*factovs += 1;
	//end protol header pack

	//info type field
	temp = 0;
	temp |= itype->highbit;
	temp <<= 7;
	temp &= 0x80;
	*psbuf |= temp;
	temp = 0;
	temp |= itype->objectn;
	temp <<= 4;
	temp &= 0x70;
	*psbuf |= temp;
	temp = 0;
	temp |= itype->opertype;
	temp &= 0x0f;
	*psbuf |= temp;
	psbuf++;
	*factovs += 1;	
	//end info type field pack

	//info field
	for (i = 0; i < ((itype->objectn) + 1); i++)
	{
		*psbuf = objecti[i].objectid;//object id
		//index number and child object
		psbuf++;
		*factovs += 1;
		temp = 0;
		temp |= objecti[i].indexn;
		temp <<= 6;
		temp &= 0xc0;
		*psbuf |= temp;
		temp = 0;
		temp |= objecti[i].cobject;
		temp &= 0x3f;
		*psbuf |= temp;

		psbuf++;
		*factovs += 1;
		//index number and child object;

		for (j = 0; j < objecti[i].indexn; j++)
		{
			*psbuf = objecti[i].index[j];
			psbuf++;
			*factovs += 1;
		}
		//index value

		//object value
		for (j = 0; j < objecti[i].objectvs; j++)
		{
			*psbuf = objecti[i].objectv[j];
			psbuf++; 
			*factovs += 1;
		}
		//object value
	}
	//end info field pack

	return SUCCESS;
}

int server_data_parse(infotype_t *itype,objectinfo_t *objecti,unsigned char *extend,unsigned int *ct,unsigned char *sbuf)
{
	if ((NULL == sbuf) || (NULL == objecti) || (NULL == itype) || (NULL == extend) || (NULL == ct))
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR; 
	}
	unsigned char		*psbuf = sbuf;
	unsigned char		temp = 0;
	unsigned char		i = 0,j = 0,z = 0,s = 0;
	unsigned char		row1 = 0;
	unsigned char		row2 = 0;
	unsigned char		*objectv = NULL;
	unsigned int		tct = 0;
	unsigned char		cn = 0;

	if (0x21 != *psbuf)
	{
	#ifdef COMMON_DEBUG
		printf("Not fit protol,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return FAIL;
	}
	psbuf++; 
	temp = *psbuf;
//	if(temp == 0x70)仿真测试
//	{
//		unsigned char detect_array[64] = {0};
//		unsigned char senddata[] = {0x7E,0x05,0x10,0x82,0x08,0x01,0x01,0x01,0xAF,0x7E};
//		detect_array[0] = 0x21;
//		detect_array[1] = 0x70;
//		psbuf++;
//		detect_array[2] = *psbuf;
//		psbuf++;
//		detect_array[3] = *psbuf;
//		psbuf++;
//		detect_array[4] = *psbuf;
//		for(i = 0; i < detect_array[4]; i++)
//		{
//			psbuf++;
//			//detect_array[5+i] = *psbuf;
//			senddata[6] = *psbuf;
//			psbuf++;
//			if(*psbuf == 1)
//			{
//				senddata[7] = 1;
//			}
//			else
//			{
//				senddata[7] = 0;
//			}
//			
//			ParserCarPassData(senddata);
//		}
//		return SUCCESS;
//	}

	
    itype->highbit = ((temp & 0x80) >> 7) & 0x01;
    itype->objectn = ((temp & 0x70) >> 4) & 0x07;
    itype->opertype = temp & 0x0f;
	psbuf++;

	//get object info;
	for (i = 0; i < ((itype->objectn) + 1); i++)
	{
		objecti[i].objectid = *psbuf;
		if ((0xD3 == objecti[i].objectid) && (2 == itype->opertype))
		{//the object belongs to extend range
			itype->objectn = 0;
        	psbuf++;
        	psbuf++;
        	*extend = *psbuf;
			break; 
		}//the object belongs to extend range
		if ((0xD4 == objecti[i].objectid) && (1 == itype->opertype))
		{//traffic control
			itype->objectn = 0;
            psbuf++;
            psbuf++;
            *extend = *psbuf;
            break;
		}//traffic control
		if ((0xEF == objecti[i].objectid) && (1 == itype->opertype))
		{//channel traffic control
			itype->objectn = 0;
			psbuf++;
            psbuf++;
			cn = *psbuf;
			psbuf++;
			for (j = 0; j < cn; j++)
			{
				*ct |= (0x00000001 << (*psbuf-1));
				psbuf++;
			}
			break;	
		}//channel traffic control
		if ((0xF4 == objecti[i].objectid) && (1 == itype->opertype))
		{//use one key to achieve green wave
			itype->objectn = 0;
			psbuf++;
			psbuf++;
			*extend = *psbuf;
			break;
		}//use one key to achieve green wave
		if ((0xF5 == objecti[i].objectid) && (2 == itype->opertype))
        {//use one key to restore
            itype->objectn = 0;
            psbuf++;
            psbuf++;
            *extend = *psbuf;
            break;
        }//use one key to restore
		if ((0xF8 == objecti[i].objectid) && (1 == itype->opertype))
		{//system set delay time of green lamp
			itype->objectn = 0;
			psbuf++;
			psbuf++;
			*extend = *psbuf;//num of detector;
			psbuf++;
			*ct = *psbuf; //delay time of system send;
			break;
		}//system set delay time of green lamp
		if ((0xE1 == objecti[i].objectid) && (1 == itype->opertype))
        {//feedback whether or not  wireless terminal is lawful
            itype->objectn = 0;
            psbuf++;
            psbuf++;
            *extend = *psbuf;
            break;
        }//feedback whether or not  wireless terminal is lawful
		if ((0xDB == objecti[i].objectid) && (2 == itype->opertype))
        {//reboot machine
			itype->objectn = 0;
			psbuf++;
			psbuf++;
			*extend = *psbuf;
			break;
        }//reboot machine
		if ((0xDE == objecti[i].objectid) && (1 == itype->opertype))
        {//fast forward
            itype->objectn = 0;
            psbuf++;
            psbuf++;
            *extend = *psbuf;
            break;
        }//fast forward
		if ((0xDC == objecti[i].objectid) && (1 == itype->opertype))
		{//start order of all pattern set
			itype->objectn = 0;
			psbuf++;
			psbuf++;
			*extend = *psbuf;
			break;	
		}//start order of all pattern set
		if ((0xDD == objecti[i].objectid) && (1 == itype->opertype))
        {//end order of all pattern set
            itype->objectn = 0;
            psbuf++;
            psbuf++;
            *extend = *psbuf;
            break;
        }//end order of all pattern set
		if ((0xFA == objecti[i].objectid) && (1 == itype->opertype))
		{//system send roadinfo
			itype->objectn = 0;
			*ct = 0;
			psbuf++;

			psbuf++;
			tct = 0;
			tct |= *psbuf;
			tct <<= 24;
			tct &= 0xff000000;
			*ct |= tct;

			psbuf++;
			tct = 0;
			tct |= *psbuf;
			tct <<= 16;
			tct &= 0x00ff0000;
			*ct |= tct;

			psbuf++;
			tct = 0;
			tct |= *psbuf;
			tct <<= 8;
			tct &= 0x0000ff00;
			*ct |= tct;			

			psbuf++;
			tct = 0;
			tct |= *psbuf;
			tct &= 0x000000ff;
			*ct |= tct;

			break;
		}//system send roadinfo
		if ((0x88 == objecti[i].objectid) && ((1 == itype->opertype) || (2 == itype->opertype)))
		{//set time,0x88 for keli system timing
			itype->objectn = 0;
			*ct = 0;
			psbuf++;
			psbuf++;
			tct = 0;
			tct |= *psbuf;
			tct &= 0x000000ff;
			*ct |= tct;
			psbuf++;
			tct = 0;
			tct |= *psbuf;
			tct <<= 8;
			tct &= 0x0000ff00;
			*ct |= tct;
			psbuf++;
			tct = 0;
			tct |= *psbuf;
			tct <<= 16;
			tct &= 0x00ff0000;
			*ct |= tct;
			psbuf++;
			tct = 0;
			tct |= *psbuf;
			tct <<= 24;
			tct &= 0xff000000;
			*ct |= tct;

			break;
		}//set time,0x88 for keli system timing
		if ((0xDA == objecti[i].objectid) && ((1 == itype->opertype) || (2 == itype->opertype)))
		{//set time;
			itype->objectn = 0;
			*ct = 0;
			psbuf++;
			psbuf++;
			tct = 0;
			tct |= *psbuf;
			tct <<= 24;
			tct &= 0xff000000;
			*ct |= tct;
			psbuf++;
			tct = 0;
			tct |= *psbuf;
			tct <<= 16;
			tct &= 0x00ff0000;
			*ct |= tct;
			psbuf++;
			tct = 0;
			tct |= *psbuf;
			tct <<= 8;
			tct &= 0x0000ff00;
			*ct |= tct;
			psbuf++;
			tct = 0;
			tct |= *psbuf;
			tct &= 0x000000ff;
			*ct |= tct;

			break;	
		}//set time
		if ((0xD7 == objecti[i].objectid) && (0 == itype->opertype))
		{//heart beat info
			break;  
		}//heart beat info
		if ((0xD9 == objecti[i].objectid) && (0 == itype->opertype))
		{//inquiry version
			break;
		}//inquiry version
		if ((0xD8 == objecti[i].objectid) && ((0 == itype->opertype) || (1 == itype->opertype)))
        {//inquiry or set configure pattern
			itype->objectn = 0;
            break;
        }//inquiry or set configure pattern
		if (((0x88 == objecti[i].objectid) || (0xDA == objecti[i].objectid)) && (0 == itype->opertype))
        {//inquiry time,0x88 for keli system inquiry time;
            break;
        }//inquiry time
		if ((0xEB == objecti[i].objectid) && (0 == itype->opertype))
        {//inquiry ID Code and version
            break;
        }//inquiry ID Code and version

		objecti[i].objects = 0;
		for (j = 0; j < OBJECTNUM; j++)
		{
			if (objecti[i].objectid == object[j].obid)
			{
				objecti[i].objects = object[j].obsize;
				break;
			}
		}
		if (0 == objecti[i].objects)
		{
		#ifdef COMMON_DEBUG
			printf("Not have fit object id,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
		psbuf++;
		temp = 0;
		temp = *psbuf;
		objecti[i].indexn = ((temp & 0xc0) >> 6) & 0x03;
		objecti[i].cobject = temp & 0x3f;
		psbuf++;
		//get size of child object
		if (objecti[i].cobject > 0)
		{
			for (j = 0; j < OBJECTNUM; j++)
        	{
            	if (objecti[i].objectid == object[j].obid)
            	{
                	if (objecti[i].cobject > object[j].fieldn)
					{
					#ifdef COMMON_DEBUG
						printf("Not have fit child object,File: %s,Line: %d\n",__FILE__,__LINE__);
					#endif
						objecti[i].cobjects = 0;
                		return FAIL;
					}
					if (1 == objecti[i].cobject)
						objecti[i].cobjects = object[j].field1s;
					if (2 == objecti[i].cobject)
                        objecti[i].cobjects = object[j].field2s;
					if (3 == objecti[i].cobject)
                        objecti[i].cobjects = object[j].field3s;
					if (4 == objecti[i].cobject)
                        objecti[i].cobjects = object[j].field4s;
					if (5 == objecti[i].cobject)
                        objecti[i].cobjects = object[j].field5s;
					if (6 == objecti[i].cobject)
                        objecti[i].cobjects = object[j].field6s;
					if (7 == objecti[i].cobject)
                        objecti[i].cobjects = object[j].field7s;
					if (8 == objecti[i].cobject)
                        objecti[i].cobjects = object[j].field8s;
					if (9 == objecti[i].cobject)
                        objecti[i].cobjects = object[j].field9s;
					if (10 == objecti[i].cobject)
                        objecti[i].cobjects = object[j].field10s;
					if (11 == objecti[i].cobject)
                        objecti[i].cobjects = object[j].field11s;
					if (12 == objecti[i].cobject)
                        objecti[i].cobjects = object[j].field12s;
					break;
            	}
        	}
		}
		else
		{
			objecti[i].cobjects = 0;
		}
		//end get size of child object	

		//get number of index
		if (0 == objecti[i].indexn)
		{//single object or whole table 
			memset(objecti[i].index,0,sizeof(objecti[i].index));
		}//single object or whole table
		else if (1 == objecti[i].indexn)
		{
			objecti[i].index[0] = *psbuf;
			psbuf++;
			objecti[i].index[1] = 0;
			objecti[i].index[2] = 0;
		}
		else if (2 == objecti[i].indexn)	
		{
			objecti[i].index[0] = *psbuf;
			psbuf++;
			objecti[i].index[1] = *psbuf;
			psbuf++;
			objecti[i].index[2] = 0;
		}
		else if (3 == objecti[i].indexn)
		{
			objecti[i].index[0] = *psbuf;
			psbuf++;
			objecti[i].index[1] = *psbuf;
			psbuf++;
			objecti[i].index[2] = *psbuf;
			psbuf++;
		}	
		//end get number of index;
		if (0 == (itype->opertype))
		{//inquiry request,ignore object value
		//	memset(objecti[i].objectv,0,sizeof(objecti[i].objectv));
			objecti[i].objectvs = 0;
			continue;
		}//inquiry request,ignore object value
		else if ((1 == (itype->opertype)) || (2 == (itype->opertype)))
		{//set request or no response
			objectv = objecti[i].objectv;
			objecti[i].objectvs = 0;
			if ((0 != objecti[i].indexn) || (0 != objecti[i].cobject))
			{//someone row table or someone field
				if ((0 != objecti[i].indexn) && (0 != objecti[i].cobject))
				{//someone field
					for (j = 0; j < objecti[i].indexn; j++)
                	{
						objecti[i].objectvs += objecti[i].cobjects;
						for (z = 0; z < objecti[i].cobjects; z++)
						{
							*objectv = *psbuf;
							objectv++;
							psbuf++;
						}
                	}
				}//someone field
				else if ((0 != objecti[i].indexn) && (0 == objecti[i].cobject))
				{//someone row table
					for (j = 0; j < objecti[i].indexn; j++)
                    {	objecti[i].objectvs += objecti[i].objects;
                        for (z = 0; z < objecti[i].objects; z++)
                        {
                            *objectv = *psbuf;
                            objectv++;
                            psbuf++;
                        }
                    }
				}//someone row table
				else if ((0 == objecti[i].indexn) && (0 != objecti[i].cobject))
				{//not fit protol data
				#ifdef COMMON_DEBUG
					printf("Not fit protol data,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
					return FAIL;	
				}//not fit protol data
			}//someone row table or someone field
			else
			{//whole table or single object;
				if ((0x8E == objecti[i].objectid) || (0xC1 == objecti[i].objectid))
				{//two dimension table
					row1 = *psbuf;//fact row number of 1th dimension table
					*objectv = *psbuf;
					psbuf++;
					objectv++;
					row2 = *psbuf;//max row number of 2th dimension table
					*objectv = *psbuf;
					objectv++;
					psbuf++;
					objecti[i].objectvs += 2;
					for (j = 0; j < row1; j++)
					{
						for (z = 0; z < row2; z++)
						{
							objecti[i].objectvs += objecti[i].objects;
							for (s = 0; s < objecti[i].objects; s++)
							{
								*objectv = *psbuf;
								objectv++;
								psbuf++;
							}
						}
					}
				}//two dimension table
				else
				{//one dimension table
					row1 = *psbuf;
					*objectv = *psbuf;
					psbuf++;
					objectv++;
					objecti[i].objectvs += 1;
					for (j = 0; j < row1; j++)
					{
						objecti[i].objectvs += objecti[i].objects;
						for (z = 0; z < objecti[i].objects; z++)
						{
							*objectv = *psbuf;
                            objectv++;
                            psbuf++;
						}
					}
				}//one dimension table
			}//whole table or single object;
		}//set request or no response
	}//for (i = 0; i < ((itype->objectn) + 1); i++)

	return SUCCESS;
}

int wireless_info_report(unsigned char *buf,unsigned int commt,unsigned char *wlmark,unsigned char event)
{
	if ((NULL == buf) || (NULL == wlmark))
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char			*pbuf = buf;
	unsigned int			tint = 0;

	*pbuf = 0x21;
	pbuf++;
	*pbuf = 0x83;
	pbuf++;
	*pbuf = 0xDF;
	pbuf++;
	*pbuf = 0x00;
	pbuf++;

	tint = commt;
	tint &= 0xff000000;
	tint >>= 24;
	tint &= 0x000000ff;
	*pbuf |= tint;
	pbuf++;
	tint = commt;
	tint &= 0x00ff0000;
	tint >>= 16;
	tint &= 0x000000ff;
	*pbuf |= tint;
	pbuf++;
	tint = commt;
	tint &= 0x0000ff00;
	tint >>= 8;
	tint &= 0x000000ff;
	*pbuf |= tint;
	pbuf++;
	tint = commt;
	tint &= 0x000000ff;
	*pbuf |= tint;
	pbuf++;

	*pbuf = wlmark[0];
	pbuf++;
	*pbuf = wlmark[1];
	pbuf++;
	*pbuf = wlmark[2];
	pbuf++;
	*pbuf = wlmark[3];
	pbuf++;

	*pbuf = event;
	pbuf++;

	return SUCCESS;
}

int err_report(unsigned char *buf,unsigned int commt,unsigned char type,unsigned char event)
{
	if (NULL == buf)
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	unsigned char			*pbuf = buf;
	unsigned int			tint = 0;
	
	*pbuf = 0x21;
	pbuf++;
	*pbuf = 0x83;
	pbuf++;
	*pbuf = 0xD0;
	pbuf++;
	*pbuf = 0x00;
	pbuf++;

	tint = commt;
	tint &= 0xff000000;
	tint >>= 24;
	tint &= 0x000000ff;
	*pbuf |= tint;
	pbuf++;
	tint = commt;
	tint &= 0x00ff0000;
	tint >>= 16;
	tint &= 0x000000ff;
	*pbuf |= tint;
	pbuf++;
	tint = commt;
	tint &= 0x0000ff00;
	tint >>= 8;
	tint &= 0x000000ff;
	*pbuf |= tint;
	pbuf++;
	tint = commt;
	tint &= 0x000000ff;
	*pbuf |= tint;
	pbuf++;
	
	*pbuf = type;
	pbuf++;
	*pbuf = event;
	pbuf++;	

	return SUCCESS;
}

int detect_err_pack(unsigned char *buf,unsigned char type,unsigned char event)
{
	if (NULL == buf)
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}	

	struct tm			*cst = NULL;
	cst = (struct tm *)get_system_time();

	if (((0 <= cst->tm_hour) && (cst->tm_hour <= 6)) && (0x14 == type))
	{
	#ifdef COMMON_DEBUG
		printf("I will not report detect err,File:%s,Line:%d\n",__FILE__,__LINE__);
	#endif
		return FAIL;
	}
	else
	{
		if (SUCCESS != err_report(buf,(long)mktime(cst),type,event))
		{
		#ifdef COMMON_DEBUG
			printf("call err_report err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
	}

	return SUCCESS;
}

int status_info_report(unsigned char *buf,statusinfo_t	*statusi)
{
	if ((NULL == statusi) || (NULL == buf))
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char		*pbuf = buf;
	unsigned short		tcyct = 0;
	unsigned char		i = 0;
	unsigned int		tphase = 0;


	*pbuf = 0x21;
	pbuf++;
	*pbuf = 0x83;
	pbuf++;
	*pbuf = 0xD1;
	pbuf++;
	*pbuf = 0x00;
	pbuf++;

	*pbuf = statusi->conmode;
	pbuf++;
	*pbuf = statusi->pattern;
	pbuf++;

	tcyct = statusi->cyclet;
	tcyct &= 0xff00;
	tcyct >>= 8;
	tcyct &= 0x00ff;
	*pbuf |= tcyct;
	pbuf++;
	tcyct = statusi->cyclet;
	tcyct &= 0x00ff;
	*pbuf |= tcyct;
	pbuf++;	

	*pbuf = statusi->color;
	pbuf++;
	*pbuf = statusi->time;
	pbuf++;
	*pbuf = statusi->stage;
	pbuf++;

	tphase = statusi->phase;
	tphase &= 0xff000000;
	tphase >>= 24;
	tphase &= 0x000000ff;
	*pbuf |= tphase;
	pbuf++;
	tphase = statusi->phase;
	tphase &= 0x00ff0000;
	tphase >>= 16;
	tphase &= 0x000000ff;
	*pbuf |= tphase;
	pbuf++;
	tphase = statusi->phase;
	tphase &= 0x0000ff00;
	tphase >>= 8;
	tphase &= 0x000000ff;
	*pbuf |= tphase;
	pbuf++;
	tphase = statusi->phase;
	tphase &= 0x000000ff;
	*pbuf |= tphase;
	pbuf++;
	
	*pbuf = statusi->chans;
	pbuf++;	
	
	for (i = 0; i < (statusi->chans); i++)
	{
		if (0 == statusi->csta[i])
			break;
		*pbuf = statusi->csta[i];
		pbuf++;
	}	

	return SUCCESS;
}

int time_info_report_keli(unsigned char *buf,unsigned int time)
{
	if (NULL == buf)
	{
	#ifdef COMMON_DEBUG
        printf("pointer address err,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
        return MEMERR;
	}

	unsigned char		*pbuf = buf;
	unsigned int		tt = 0;

	*pbuf = 0x21;
	pbuf++;
	*pbuf = 0x84;
	pbuf++;
	*pbuf = 0x88;
	pbuf++;
	*pbuf = 0x00;
    pbuf++;

	tt = time;
	tt &= 0x000000ff;
	*pbuf |= tt;
	pbuf++;

	tt = time;
	tt &= 0x0000ff00;
	tt >>= 8;
	tt &= 0x000000ff;
	*pbuf |= tt;
	pbuf++;

	tt = time;
	tt &= 0x00ff0000;
	tt >>= 16;
	tt &= 0x000000ff;
	*pbuf |= tt;
	pbuf++;

	tt = time;
	tt &= 0xff000000;
	tt >>= 24;
	tt &= 0x000000ff;
	*pbuf |= tt;
	pbuf++;

	return SUCCESS;
}

int time_info_report(unsigned char *buf,unsigned int time)
{
	if (NULL == buf)
	{
	#ifdef COMMON_DEBUG
        printf("pointer address err,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
        return MEMERR;
	}

	unsigned char		*pbuf = buf;
	unsigned int		tt = 0;

	*pbuf = 0x21;
	pbuf++;
	*pbuf = 0x84;
	pbuf++;
	*pbuf = 0xDA;
	pbuf++;
	*pbuf = 0x00;
    pbuf++;

	tt = time;
	tt &= 0xff000000;
	tt >>= 24;
	tt &= 0x000000ff;
	*pbuf |= tt;
	pbuf++;

	tt = time;
	tt &= 0x00ff0000;
	tt >>= 16;
	tt &= 0x000000ff;
	*pbuf |= tt;
	pbuf++;

	tt = time;
	tt &= 0x0000ff00;
	tt >>= 8;
	tt &= 0x000000ff;
	*pbuf |= tt;
	pbuf++;

	tt = time;
	tt &= 0x000000ff;
	*pbuf |= tt;
	pbuf++;

	return SUCCESS;
}

int version_id_report(unsigned char *buf,versioninfo_t *vi)
{
	if ((NULL == buf) || (NULL == vi))
    {
    #ifdef COMMON_DEBUG
        printf("pointer address err,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
        return MEMERR;  
    }

	unsigned char			*pbuf = buf;

	*pbuf = 0x21;
	pbuf++;
	*pbuf = 0x84;
	pbuf++;
	*pbuf = 0xD9;
	pbuf++;
	*pbuf = 0x00;
	pbuf++;

	*pbuf = vi->majorid;
	pbuf++;
	*pbuf = vi->minorid1;
	pbuf++;
	*pbuf = vi->minorid2;
	pbuf++;
	*pbuf = vi->year;
	pbuf++;
	*pbuf = vi->month;
	pbuf++;
	*pbuf = vi->day;
	pbuf++;	

	return SUCCESS;
}

int detect_info_report(unsigned char *buf,detectinfo_t *di)
{
	if ((NULL == buf) || (NULL == di))
	{
	#ifdef COMMON_DEBUG
		printf("pointer address err,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	
	unsigned char		*pbuf = buf;
	unsigned int		tt = 0;
	unsigned int		tphase = 0;

	*pbuf = 0x21;
	pbuf++;
	*pbuf = 0x83;
	pbuf++;
	*pbuf = 0xD2;
	pbuf++;
	*pbuf = 0x00;
	pbuf++;
	
	*pbuf = *(di->pattern);
	pbuf++;
	*pbuf = *(di->stage);
	pbuf++;

	tt = di->time;
	tt &= 0xff000000;
	tt >>= 24;
	tt &= 0x000000ff;
	*pbuf |= tt;
	pbuf++;
	tt = di->time;
	tt &= 0x00ff0000;
	tt >>= 16;
	tt &= 0x000000ff;
	*pbuf |= tt;
	pbuf++;
	tt = di->time;
	tt &= 0x0000ff00;
	tt >>= 8;
	tt &= 0x000000ff;
	*pbuf |= tt;
	pbuf++;
	tt = di->time;
	tt &= 0x000000ff;
	*pbuf |= tt;
	pbuf++;

	tphase = *(di->phase);
	tphase &= 0xff000000;
	tphase >>= 24;
	tphase &= 0x000000ff;
	*pbuf |= tphase;
	pbuf++;
	tphase = *(di->phase);
	tphase &= 0x00ff0000;
	tphase >>= 16;
	tphase &= 0x000000ff;
	*pbuf |= tphase;
	pbuf++;
	tphase = *(di->phase);
	tphase &= 0x0000ff00;
	tphase >>= 8;
	tphase &= 0x000000ff;
	*pbuf |= tphase;
	pbuf++;
	tphase = *(di->phase);
	tphase &= 0x000000ff;
	*pbuf |= tphase;
	pbuf++;

	*pbuf = *(di->color);
	pbuf++;
	*pbuf = di->detectid;
	pbuf++;

	return SUCCESS;
}

int read_file(unsigned char *ip,unsigned int *port)
{
	if ((NULL == ip) || (NULL == port))
	{
	#ifdef COMMON_DEBUG
		printf("pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

    FILE                    *fp = NULL;
    unsigned char           ips[16] = {0};
    unsigned char           ports[10] = {0};
    unsigned char           i = 0;
    unsigned char           *temp = NULL;

    fp = fopen("./ipport.txt","r");
    if (NULL == fp)
    {
        printf("read file err!\n");
    }
    fgets(ips,15,fp);
    fgets(ports,10,fp);
    fclose(fp);

    temp = ip;
    for (i = 0; ;i++)
    {
        if (('\0' == ips[i]) || ('\n' == ips[i]))
            break;
        *temp = ips[i];
        temp++;
    }

    return SUCCESS;
}

/*
 *Return -2: pointer address err; 
 *Return -1: green conflict exist;
 *Return 0: green conflict not exist;
*/
int green_conflict_check(Phase_t *phaselist,Channel_t *channel)
{
	if ((NULL == phaselist) || (NULL == channel))
	{
	#ifdef COMMON_DEBUG
		printf("pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	unsigned char		i = 0;
	unsigned char		j = 0;
	unsigned char		z = 0;
	unsigned char		chan[MAX_CHANNEL] = {0};
	unsigned char		*pchan = NULL;
	unsigned char		cn = 0;//channel number;

	for (i = 0; i < (phaselist->FactPhaseNum); i++)
	{//for (i = 0; i < (phaselist->FactPhaseNum); i++)
		if (0 == phaselist->PhaseList[i].PhaseId)
			break;
		memset(chan,0,sizeof(chan));
		pchan = chan;
		cn = 0;
		for (j = 0; j < (channel->FactChannelNum); j++)
		{//for (j = 0; j < (channel->FactChannelNum); j++)
			if (0 == (channel->ChannelList[j].ChannelId))
				break;
			if ((phaselist->PhaseList[i].PhaseId) == (channel->ChannelList[j].ChannelCtrlSrc))
			{
				*pchan = channel->ChannelList[j].ChannelId;
				pchan++;
				cn += 1;	
			}
		}//for (j = 0; j < (channel->FactChannelNum); j++)
		#ifdef COMMON_DEBUG
		printf("the phase map channel num: %d,File: %s,Line: %d\n",cn,__FILE__,__LINE__);
		#endif
		for (j = 0; j < cn; j++)
		{//for (j = 0; j < cn; j++)
			if (1 == chan[j])
			{
				for (z = 0; z < cn; z++)
				{
					if (2 == chan[z])
						return FAIL;
					if (4 == chan[z])
						return FAIL;
					if (8 == chan[z])
						return FAIL;
					if (14 == chan[z])
						return FAIL;
					if (16 == chan[z])
						return FAIL;
				}//for (z = 0; z < cn; z++)
			}//if (1 == chan[j])
			else if (2 == chan[j])
			{
				for (z = 0; z < cn; z++)
				{
					if (1 == chan[z])
						return FAIL;
					if (3 == chan[z])
						return FAIL;
					if (5 == chan[z])
						return FAIL;
					if (13 == chan[z])
						return FAIL;
					if (15 == chan[z])
						return FAIL;
				}//for (z = 0; z < cn; z++)
			}//else if (2 == chan[j])
			else if (3 == chan[j])
			{
				for (z = 0; z < cn; z++)
				{
					if (2 == chan[z])
                        return FAIL;
                    if (4 == chan[z])
                        return FAIL;
                    if (6 == chan[z])
                        return FAIL;
                    if (14 == chan[z])
                        return FAIL;
                    if (16 == chan[z])
                        return FAIL;
				}//for (z = 0; z < cn; z++)
			}//else if (3 == chan[j])
			else if (4 == chan[j])
			{
				for (z = 0; z < cn; z++)
                {
                    if (1 == chan[z])
                        return FAIL;
                    if (3 == chan[z])
                        return FAIL;
                    if (7 == chan[z])
                        return FAIL;
                    if (13 == chan[z])
                        return FAIL;
                    if (15 == chan[z])
                        return FAIL;
                }//for (z = 0; z < cn; z++)	
			}//else if (4 == chan[j])
			else if (5 == chan[j])
			{
				for (z = 0; z < cn; z++)
                {
                    if (2 == chan[z])
                        return FAIL;
                    if (6 == chan[z])
                        return FAIL;
                    if (8 == chan[z])
                        return FAIL;
                    if (14 == chan[z])
                        return FAIL;
                }//for (z = 0; z < cn; z++)
			}//else if (5 == chan[j])
			else if (6 == chan[j])
			{
				for (z = 0; z < cn; z++)
                {
                    if (3 == chan[z])
                        return FAIL;
                    if (5 == chan[z])
                        return FAIL;
                    if (7 == chan[z])
                        return FAIL;
                    if (15 == chan[z])
                        return FAIL;
                }//for (z = 0; z < cn; z++)
			}//else if (6 == chan[j])
			else if (7 == chan[j])
			{
				for (z = 0; z < cn; z++)
                {
                    if (4 == chan[z])
                        return FAIL;
                    if (6 == chan[z])
                        return FAIL;
                    if (8 == chan[z])
                        return FAIL;
                    if (16 == chan[z])
                        return FAIL;
                }//for (z = 0; z < cn; z++)
			}//else if (7 == chan[j])
			else if (8 == chan[j])
			{
				for (z = 0; z < cn; z++)
                {
                    if (1 == chan[z])
                        return FAIL;
                    if (5 == chan[z])
                        return FAIL;
                    if (7 == chan[z])
                        return FAIL;
                    if (13 == chan[z])
                        return FAIL;
                }//for (z = 0; z < cn; z++)
			}//else if (8 == chan[j])
		}//for (j = 0; j < cn; j++)		
	}//for (i = 0; i < (phaselist->FactPhaseNum); i++)
	

	return SUCCESS;
}

int terminal_protol_pack(unsigned char *buf,terminalpp_t *tpp)
{
	if ((NULL == buf) || (NULL == tpp))
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address err,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char			*pbuf = buf;

	*pbuf = tpp->head;
	pbuf++;
	*pbuf = tpp->size;
	pbuf++;
	*pbuf = tpp->id;
	pbuf++;
	*pbuf = tpp->type;
	pbuf++;
	if (0x01 == tpp->type)
	{
		*pbuf = tpp->sta;
		*pbuf++;
		*pbuf = tpp->func[0];
		pbuf++;
		*pbuf = tpp->func[1];
        pbuf++;
		*pbuf = tpp->func[2];
        pbuf++;
		*pbuf = tpp->func[3];
        pbuf++;
	}
	else if (0x04 == tpp->type)
	{
		*pbuf = tpp->func[0];
		pbuf++;
	}	
	*pbuf = tpp->end;
	pbuf++;

	return SUCCESS;
}

int wireless_terminal_mark(unsigned char *buf,unsigned char *mark)
{
	if ((NULL == buf) || (NULL == mark))
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address error,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char			*pbuf = buf;
	unsigned char			*pmark = mark;
	unsigned char			i = 0;

	*pbuf = 0x21;
	pbuf++;
	*pbuf = 0x83;
	pbuf++;
	*pbuf = 0xE1;
	pbuf++;
	*pbuf = 0x00;
	pbuf++;

	for (i = 0; i < 4; i++)
	{
		*pbuf = *pmark;
		pbuf++;
		pmark++;
	}	

	return SUCCESS;
}

#if 0
int terminal_protol_parse(unsigned char *buf,terminalpp_t *tpp)
{
	if ((NULL == buf) || (NULL == tpp))
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address err,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char			*pbuf = buf;

	tpp->head = *pbuf;
	pbuf++;
	if (0xB9 == tpp->head)
	{
		tpp->size = *pbuf;
		pbuf++;
		tpp->id = *pbuf;
		pbuf++;		
		tpp->type = *pbuf;
		pbuf++;
		if (0x01 == (tpp->type))
		{
			tpp->func[0] = *pbuf;
			pbuf++;
			tpp->func[1] = *pbuf;
			pbuf++;
			tpp->func[2] = *pbuf;
			pbuf++;
			tpp->func[3] = *pbuf;
			pbuf++;
		}
		else
		{
			tpp->func[0] = *pbuf;
			pbuf++;
		}
		tpp->end = *pbuf;
		pbuf++;	
	}//if (0xB9 == tpp->head)
	else
	{
	#ifdef COMMON_DEBUG
		printf("Not our data,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return FAIL;
	}

	return SUCCESS;
}
#endif
int environment_pack(unsigned char *buf,unsigned char object,unsigned char par1,unsigned char par2)
{
	if (NULL == buf)
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char       *pbuf = buf;

	*pbuf = 0x21;
    pbuf++;
    *pbuf = 0x83;
    pbuf++;
	*pbuf = object;
	pbuf++;
    *pbuf = 0x00;
    pbuf++;

	*pbuf = par1;
	pbuf++;
	*pbuf = par2;
	pbuf++;

	return SUCCESS;
} 

int voltage_pack(unsigned char *buf,unsigned char volh,unsigned char voll)
{
	if (NULL == buf)
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char		*pbuf = buf;

	*pbuf = 0x21;
	pbuf++;
	*pbuf = 0x83;
	pbuf++;
	*pbuf = 0xE4;
	pbuf++;
	*pbuf = 0x00;
	pbuf++;

	*pbuf = volh;
	pbuf++;
	*pbuf = voll;
	pbuf++;	

	return SUCCESS;
}

int wendu_pack(unsigned char *buf,unsigned char wenduh,unsigned char wendul)
{
	if (NULL == buf)
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char		*pbuf = buf;

	*pbuf = 0x21;
	pbuf++;
	*pbuf = 0x83;
	pbuf++;
	*pbuf = 0xE5;
	pbuf++;
	*pbuf = 0x00;
	pbuf++;

	*pbuf = wenduh;
	pbuf++;
	*pbuf = wendul;
	pbuf++;	

	return SUCCESS;
}

int shidu_pack(unsigned char *buf,unsigned char shiduh,unsigned char shidul)
{
	if (NULL == buf)
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char		*pbuf = buf;

	*pbuf = 0x21;
	pbuf++;
	*pbuf = 0x83;
	pbuf++;
	*pbuf = 0xE6;
	pbuf++;
	*pbuf = 0x00;
	pbuf++;

	*pbuf = shiduh;
	pbuf++;
	*pbuf = shidul;
	pbuf++;	

	return SUCCESS;
}

int zdong_pack(unsigned char *buf,unsigned char zdongh,unsigned char zdongl)
{
	if (NULL == buf)
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char		*pbuf = buf;

	*pbuf = 0x21;
	pbuf++;
	*pbuf = 0x83;
	pbuf++;
	*pbuf = 0xE7;
	pbuf++;
	*pbuf = 0x00;
	pbuf++;

	*pbuf = zdongh;
	pbuf++;
	*pbuf = zdongl;
	pbuf++;	

	return SUCCESS;
}

int yanwu_pack(unsigned char *buf,unsigned char yw1,unsigned char yw2)
{
	if (NULL == buf)
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}	

	unsigned char		*pbuf = buf;

	*pbuf = 0x21;
	pbuf++;
	*pbuf = 0x83;
	pbuf++;
	*pbuf = 0xF1;
	pbuf++;
	*pbuf = 0x00;
	pbuf++;

	*pbuf = yw1;
	pbuf++;
	*pbuf = yw2;
	pbuf++;	
	
	return SUCCESS;
}

int shuiwei_pack(unsigned char *buf,unsigned char sw1,unsigned char sw2)
{
	if (NULL == buf)
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}	

	unsigned char		*pbuf = buf;

	*pbuf = 0x21;
	pbuf++;
	*pbuf = 0x83;
	pbuf++;
	*pbuf = 0xF2;
	pbuf++;
	*pbuf = 0x00;
	pbuf++;

	*pbuf = sw1;
	pbuf++;
	*pbuf = sw2;
	pbuf++;	
	
	return SUCCESS;
}

int menkaiguan_pack(unsigned char *buf,unsigned char dw1,unsigned char dw2)
{
	if (NULL == buf)
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}	

	unsigned char		*pbuf = buf;

	*pbuf = 0x21;
	pbuf++;
	*pbuf = 0x83;
	pbuf++;
	*pbuf = 0xF3;
	pbuf++;
	*pbuf = 0x00;
	pbuf++;

	*pbuf = dw1;
	pbuf++;
	*pbuf = dw2;
	pbuf++;	
	
	return SUCCESS;
}

int green_conflict_pack(unsigned char *buf,unsigned int commt,unsigned int gcbit)
{
	if (NULL == buf)
	{
	#ifdef COMMON_DEBUG
        printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
        return MEMERR;
	}
	unsigned char       *pbuf = buf;
	unsigned int		temp = 0;

	*pbuf = 0x21;
	pbuf++;
	*pbuf = 0x83;
	pbuf++;
	*pbuf = 0xE8;
	pbuf++;
	*pbuf = 0x00;
    pbuf++;

	temp = commt;
	temp &= 0xff000000;
	temp >>= 24;
	temp &= 0x000000ff;
	*pbuf |= temp;
	pbuf++;

	temp = commt;
	temp &= 0x00ff0000;
	temp >>= 16;
	temp &= 0x000000ff;
	*pbuf |= temp;
	pbuf++;

	temp = commt;
	temp &= 0x0000ff00;
	temp >>= 8;
	temp &= 0x000000ff;
	*pbuf |= temp;
	pbuf++;

	temp = commt;
	temp &= 0x000000ff;
	*pbuf |= temp;
	pbuf++;

	temp = gcbit;
	temp &= 0xff000000;
	temp >>= 24;
	temp &= 0x000000ff;
	*pbuf |= temp;
	pbuf++;

	temp = gcbit;
	temp &= 0x00ff0000;
	temp >>= 16;
	temp &= 0x000000ff;
	*pbuf |= temp;
	pbuf++;

	temp = gcbit;
	temp &= 0x0000ff00;
	temp >>= 8;
	temp &= 0x000000ff;
	*pbuf |= temp;
	pbuf++;

	temp = gcbit;
	temp &= 0x000000ff;
	*pbuf |= temp;
	pbuf++;

	return SUCCESS;
}

//read bus and reader/writer info of one crossing road from ./data/busc.txt
//busc.txt文本内容如下：
//公交车ID 读写器ID 检测器ID
//1        1        56
//1        3        57
//...................
//...................
int read_bus_rw_info(bus_pri_t *bpt)
{
	if (NULL == bpt)
	{
	#ifdef COMMOM_DEBUG
		printf("Pointer is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	FILE				*fp = NULL;
	unsigned char		bus_pri[16] = {0};
	unsigned char		i = 0;

	fp = fopen(BUS_CONFIG,"r");
	if (NULL == fp)
	{
	#ifdef COMMON_DEBUG
		printf("open file err,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return FAIL;
	}

	while (NULL != fgets(bus_pri,sizeof(bus_pri),fp))
	{
		if (0 == i)
        {
            i++;
            continue;
        }

		sscanf(bus_pri,"%hd %hd %hd",&(bpt->busid),&(bpt->rwid),&(bpt->dectid));
		#ifdef COMMON_DEBUG
		printf("%d,%d,%d\n",bpt->busid,bpt->rwid,bpt->dectid);
		#endif
		bpt++;
	}
	fclose(fp);

	return SUCCESS;	
}

int channel_close_end_report(int *csockfd,unsigned char *ccchan)
{
	if ((NULL == csockfd) || (NULL == ccchan))
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	unsigned char		buf[9] = {0};
	unsigned int		cc = 0;
	unsigned char		i = 0;

	buf[0] = 0x21;
	buf[1] = 0x83;
	buf[2] = 0xF7;
	buf[3] = 0x00;
	buf[4] = 0x01;
	for (i = 0; i < MAX_CHANNEL; i++)
	{
		if (0 == ccchan[i])
			break;
		cc |= (0x00000001 << (ccchan[i]-1)); 
	}
	buf[5] |= (cc & 0xff000000)	>> 24;
	buf[6] |= (cc & 0x00ff0000) >> 16;
	buf[7] |= (cc & 0x0000ff00) >> 8;
	buf[8] |= (cc & 0x000000ff);
	
	write(*csockfd,buf,sizeof(buf));
	
	return SUCCESS;
}

int channel_close_begin_report(int *csockfd,unsigned char *ccchan)
{
	if ((NULL == csockfd) || (NULL == ccchan))
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	unsigned char		buf[9] = {0};
	unsigned int		cc = 0;
	unsigned char		i = 0;

	buf[0] = 0x21;
	buf[1] = 0x83;
	buf[2] = 0xF6;
	buf[3] = 0x00;
	buf[4] = 0x01;
	for (i = 0; i < MAX_CHANNEL; i++)
	{
		if (0 == ccchan[i])
			break;
		cc |= (0x00000001 << (ccchan[i]-1)); 
	}
	buf[5] |= (cc & 0xff000000)	>> 24;
	buf[6] |= (cc & 0x00ff0000) >> 16;
	buf[7] |= (cc & 0x0000ff00) >> 8;
	buf[8] |= (cc & 0x000000ff);
	
	write(*csockfd,buf,sizeof(buf));
	
	return SUCCESS;
}

int red_flash_begin_report(int *csockfd,unsigned char *rfchan)
{
	if ((NULL == csockfd) || (NULL == rfchan))
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	unsigned char		buf[9] = {0};
	unsigned int		cc = 0;
	unsigned char		i = 0;

	buf[0] = 0x21;
	buf[1] = 0x83;
	buf[2] = 0xE9;
	buf[3] = 0x00;
	buf[4] = 0x01;
	for (i = 0; i < MAX_CHANNEL; i++)
	{
		if (0 == rfchan[i])
			break;
		cc |= (0x00000001 << (rfchan[i]-1)); 
	}
	buf[5] |= (cc & 0xff000000)	>> 24;
	buf[6] |= (cc & 0x00ff0000) >> 16;
	buf[7] |= (cc & 0x0000ff00) >> 8;
	buf[8] |= (cc & 0x000000ff);
	
	write(*csockfd,buf,sizeof(buf));
	
	return SUCCESS;
}

int red_flash_end_report(int *csockfd)
{
	if (NULL == csockfd)
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char			buf[4] = {0};

	buf[0] = 0x21;
	buf[1] = 0x83;
	buf[2] = 0xEA;
	buf[3] = 0x00;
	
	write(*csockfd,buf,sizeof(buf));

	return SUCCESS;
}

int dunhua_close_lamp(fcdata_t *fd,ChannelStatus_t *cs)
{
	if ((NULL == fd) || (NULL == cs))
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	unsigned char		chan[8] = {0};
	unsigned char		sdata[4] = {0};
	sdata[0] = 0xAA;
	sdata[2] = 0x03;
	sdata[3] = 0xED;
	unsigned char		i = 0,j = 0;

#ifdef CLOSE_LAMP
	for (i = 0; i < 8; i++)
	{
		if (*(fd->specfunc) & (0x01 << i))
		{
			chan[j] = i + 5;
			j++;
		}
	}
#else
	if ((*(fd->specfunc) & 0x10) && (*(fd->specfunc) & 0x20))
	{
		chan[0] = 5;
		chan[1] = 6;
		chan[2] = 7;
        chan[3] = 8;
		chan[4] = 9;
        chan[5] = 10;
        chan[6] = 11;
        chan[7] = 12;
	}
	else if ((*(fd->specfunc) & 0x10) && (!(*(fd->specfunc) & 0x20)))
	{
		chan[0] = 5;
		chan[1] = 6;
		chan[2] = 7;
		chan[3] = 8;
	}
	else if ((*(fd->specfunc) & 0x20) && (!(*(fd->specfunc) & 0x10)))
	{
		chan[0] = 9;
		chan[1] = 10;
		chan[2] = 11;
		chan[3] = 12;
	}
	else
	{
	#ifdef COMMON_DEBUG
		printf("Not need to close lamp,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return SUCCESS;
	}
#endif

	for (i = 0; i < 8; i++)
	{
		if (0 == chan[i])
			break;
		sdata[1] = chan[i];
		if (!wait_write_serial(*(fd->bbserial)))
		{
			if (write(*(fd->bbserial),sdata,sizeof(sdata)) < 0)
			{
			#ifdef COMMON_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				return FAIL;
			}
		}
		else
		{
		#ifdef COMMON_DEBUG
			printf("serial port cannot be written,File:%s,Line:%d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
	}
	//send end mark to serial port
	sdata[1] = 0;
	if (!wait_write_serial(*(fd->bbserial)))
	{
		if (write(*(fd->bbserial),sdata,sizeof(sdata)) < 0)
		{
		#ifdef COMMON_DEBUG
			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
	}
	else
	{
	#ifdef COMMON_DEBUG
		printf("serial port cannot be written,File:%s,Line:%d\n",__FILE__,__LINE__);
	#endif
		return FAIL;
	}
	if (SUCCESS != update_channel_status(fd->sockfd,cs,chan,0x03,fd->markbit))
	{
	#ifdef COMMON_DEBUG
		printf("update channel err,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return FAIL;
	}

	return SUCCESS;
}

//保留位增加五个字节的口令，口令admin，使用其ascii码，分别为0x61,0x64,0x6D,0x69,0x6E
int	standard_protocol_encode(standard_data_t *sdt,unsigned char	*buf,unsigned short *typen)
{
	if ((NULL == sdt) || (NULL == buf))
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	unsigned char			*pbuf = buf;
	unsigned short			i = 0;
	unsigned short			temp1 = 0;
	unsigned short			temp2 = 0;
	unsigned char			tep = 0;
	unsigned char			chc = 0;

	*pbuf = sdt->Head;
	pbuf++;
	*typen = 1;

	if (0xC0 == sdt->VerId)
	{
		*pbuf = 0xDB;
		pbuf++;
		*pbuf = 0xDC;
		pbuf++;
		*typen += 2;
	}
	else if (0xDB == sdt->VerId)
	{
		*pbuf = 0xDB;
		pbuf++;
		*pbuf = 0xDD;
		pbuf++;
		*typen += 2;
	}
	else
	{
		*pbuf = sdt->VerId;
		pbuf++;
		*typen += 1;
	}
	chc = sdt->VerId;

	*pbuf = sdt->SendMark;
	pbuf++;
	*typen += 1;
	chc &= sdt->SendMark;

	*pbuf = sdt->RecMark;
	pbuf++;
	*typen += 1;
	chc &= sdt->RecMark;

	*pbuf = sdt->DataLink;
	pbuf++;
	*typen += 1;
	chc &= sdt->DataLink;

	if (0xC0 == sdt->AreaId)
	{
		*pbuf = 0xDB;
        pbuf++;
        *pbuf = 0xDC;
        pbuf++;
		*typen += 2;
	}
	else if (0xDB == sdt->AreaId)
	{
		*pbuf = 0xDB;
        pbuf++;
        *pbuf = 0xDD;
        pbuf++;
		*typen += 2;
	}
	else
	{
		*pbuf = sdt->AreaId;
		pbuf++;
		*typen += 1;
	}
	chc &= sdt->AreaId;

	tep = 0;
	tep = sdt->CrossId & 0x00ff;
	if (0xC0 == tep)
	{
		*pbuf = 0xDB;
        pbuf++;
        *pbuf = 0xDC;
        pbuf++;
		*typen += 2;
	}
	else if (0xDB == tep)
	{
		*pbuf = 0xDB;
        pbuf++;
        *pbuf = 0xDD;
        pbuf++;
		*typen += 2;
	}
	else
	{
		*pbuf = tep;
		pbuf++;
		*typen += 1;
	}
	chc &= tep;
	
	tep = 0;
	tep = ((sdt->CrossId & 0xff00) >> 8) & 0x00ff;
	if (0xC0 == tep)
    {
        *pbuf = 0xDB;
        pbuf++;
        *pbuf = 0xDC;
        pbuf++;
		*typen += 2;
    }
    else if (0xDB == tep)
    {
        *pbuf = 0xDB;
        pbuf++;
        *pbuf = 0xDD;
        pbuf++;
		*typen += 2;
    }
    else
    {
        *pbuf = tep;
        pbuf++;
		*typen += 1;
    }
	chc &= tep;

	*pbuf = sdt->OperType;
	pbuf++;
	*typen += 1;
	chc &= sdt->OperType;

	*pbuf = sdt->ObjectId;
	pbuf++;
	*typen += 1;
	chc &= sdt->ObjectId;

	for (i = 0; i < 5; i++)
	{
		*pbuf = sdt->Reserve[i];
		pbuf++;
		chc &= sdt->Reserve[i];
	}
	*typen += 5;
	
	//0x01:对象标识为0x01时没有数据内容；
	if (0x02 == sdt->ObjectId)
	{//信号机主动上传交通流信息；
		temp1 = sdt->Data[0];
		temp2 = 1 + temp1*6;
//		*typen += temp2;
		for (i = 0; i < temp2; i++)
		{
			chc &= sdt->Data[i];
			if (0xC0 == sdt->Data[i])
			{
				*pbuf = 0xDB;
				pbuf++;
				*pbuf = 0xDC;
				pbuf++;
				*typen += 2;
			}
			else if (0xDB == sdt->Data[i])
			{
				*pbuf = 0xDB;
                pbuf++;
                *pbuf = 0xDD;
                pbuf++;
				*typen += 2;
			}
			else
			{
				*pbuf = sdt->Data[i];
				pbuf++;
				*typen += 1;
			}
		}	
	}//信号机主动上传交通流信息；
	else if (0x03 == sdt->ObjectId)
    {
		//信号机工作状态的查询应答和主动上报的数据内容是一样的
		for (i = 0; i < 6; i++)
		{
			chc &= sdt->Data[i];
			if (0xC0 == sdt->Data[i])
            {
                *pbuf = 0xDB;
                pbuf++;
                *pbuf = 0xDC;
                pbuf++;
				*typen += 2;
            }
            else if (0xDB == sdt->Data[i])
            {
                *pbuf = 0xDB;
                pbuf++;
                *pbuf = 0xDD;
                pbuf++;
				*typen += 2;
            }
            else
            {
                *pbuf = sdt->Data[i];
                pbuf++;
				*typen += 1;
            }
		}
//		*typen += 6;
    }
	else if (0x04 == sdt->ObjectId)
    {
		//灯色状态查询应答和主动上报的数据内容格式是一样的
		for (i = 0; i < 12; i++)
		{
			chc &= sdt->Data[i];
			if (0xC0 == sdt->Data[i])
            {
                *pbuf = 0xDB;
                pbuf++;
                *pbuf = 0xDC;
                pbuf++;
				*typen += 2;
            }
            else if (0xDB == sdt->Data[i])
            {
                *pbuf = 0xDB;
                pbuf++;
                *pbuf = 0xDD;
                pbuf++;
				*typen += 2;
            }
            else
            {
                *pbuf = sdt->Data[i];
                pbuf++;
				*typen += 1;
            }
		}
//		*typen += 12;
    }
	else if (0x05 == sdt->ObjectId)
    {
		if (0x83 == sdt->OperType)
		{//查询应答
			for (i = 0; i < 4; i++)
			{
				chc &= sdt->Data[i];
				if (0xC0 == sdt->Data[i])
				{
					*pbuf = 0xDB;
					pbuf++;
					*pbuf = 0xDC;
					pbuf++;
					*typen += 2;
				}
				else if (0xDB == sdt->Data[i])
				{
					*pbuf = 0xDB;
					pbuf++;
					*pbuf = 0xDD;
					pbuf++;
					*typen += 2;
				}
				else
				{
					*pbuf = sdt->Data[i];
					pbuf++;
					*typen += 1;
				}
			}
		}//查询应答
		//上位机时间设置应答数据内容是空的；
    }
	else if (0x06 == sdt->ObjectId)
    {
		if (0x83 == sdt->OperType)
		{//查询应答
			temp1 = sdt->Data[0];
			temp2 = 1 + temp1*12;
			for (i = 0; i < temp2; i++)
			{
				chc &= sdt->Data[i];
				if (0xC0 == sdt->Data[i])
                {
                    *pbuf = 0xDB;
                    pbuf++;
                    *pbuf = 0xDC;
                    pbuf++;
					*typen += 2;
                }
                else if (0xDB == sdt->Data[i])
                {
                    *pbuf = 0xDB;
                    pbuf++;
                    *pbuf = 0xDD;
                    pbuf++;
					*typen += 2;
                }
                else
                {
                    *pbuf = sdt->Data[i];
                    pbuf++;
					*typen += 1;
                }	
			}
//			*typen += temp2;
		}//查询应答
		//信号灯组设置应答数据内容是空的；
    }
	else if (0x07 == sdt->ObjectId)
    {
		if (0x83 == sdt->OperType)
		{//查询应答
			temp1 = sdt->Data[0];
			temp2 = 1 + temp1*24;
			for (i = 0; i < temp2; i++)
			{
				chc &= sdt->Data[i];
				if (0xC0 == sdt->Data[i])
                {
                    *pbuf = 0xDB;
                    pbuf++;
                    *pbuf = 0xDC;
                    pbuf++;
					*typen += 2;
                }
                else if (0xDB == sdt->Data[i])
                {
                    *pbuf = 0xDB;
                    pbuf++;
                    *pbuf = 0xDD;
                    pbuf++;
					*typen += 2;
                }
                else
                {
                    *pbuf = sdt->Data[i];
                    pbuf++;
					*typen += 1;
                }
			}
//			*typen += temp2;
		}//查询应答
		//相位设置应答数据内容是空的；
    }
	else if (0x08 == sdt->ObjectId)
    {
		if (0x83 == sdt->OperType)
		{//查询应答
			temp1 = sdt->Data[0];
			temp2 = 1 + temp1*24;
			for (i = 0; i < temp2; i++)
			{
				chc &= sdt->Data[i];
				if (0xC0 == sdt->Data[i])
                {
                    *pbuf = 0xDB;
                    pbuf++;
                    *pbuf = 0xDC;
                    pbuf++;
					*typen += 2;
                }
                else if (0xDB == sdt->Data[i])
                {
                    *pbuf = 0xDB;
                    pbuf++;
                    *pbuf = 0xDD;
                    pbuf++;
					*typen += 2;
                }
                else
                {
                    *pbuf = sdt->Data[i];
                    pbuf++;
					*typen += 1;
                }
			}
//			*typen += temp2;
		}//查询应答
		//信号配时方案设置应答数据内容是空的；
    }
	else if (0x09 == sdt->ObjectId)
    {
		if (0x83 == sdt->OperType)
        {//查询应答
            temp1 = sdt->Data[0];
            temp2 = 1 + temp1*12;
            for (i = 0; i < temp2; i++)
            {
				chc &= sdt->Data[i];
                if (0xC0 == sdt->Data[i])
                {
                    *pbuf = 0xDB;
                    pbuf++;
                    *pbuf = 0xDC;
                    pbuf++;
					*typen += 2;
                }
                else if (0xDB == sdt->Data[i])
                {
                    *pbuf = 0xDB;
                    pbuf++;
                    *pbuf = 0xDD;
                    pbuf++;
					*typen += 2;
                }
                else
                {
                    *pbuf = sdt->Data[i];
                    pbuf++;
					*typen += 1;
                }
            }
//			*typen += temp2;
        }//查询应答
        //方案调度计划设置应答数据内容是空的；
    }
	else if (0x0a == sdt->ObjectId)
    {
		if (0x83 == sdt->OperType)
		{//查询应答
			chc &= sdt->Data[0];
			if (0xC0 == sdt->Data[0])
			{
				*pbuf = 0xDB;
				pbuf++;
				*pbuf = 0xDC;
				pbuf++;
				*typen += 2;
			}
			else if (0xDB == sdt->Data[0])
			{
				*pbuf = 0xDB;
				pbuf++;
				*pbuf = 0xDD;
				pbuf++;
				*typen += 2;
			}
			else
			{
				*pbuf = sdt->Data[0];
				pbuf++;
				*typen += 1;
			}
//			*typen += 1;
		}//查询应答
		//工作方式设置应答数据内容是空的；
    }
	else if (0x0b == sdt->ObjectId)
    {
		//信号机故障查询应答和主动上传数据内容是一样的；
		temp1 = sdt->Data[0];
		temp2 = 1 + temp1*12;
		for (i = 0; i < temp2; i++)
		{
			chc &= sdt->Data[i];
			if (0xC0 == sdt->Data[i])
			{
				*pbuf = 0xDB;
				pbuf++;
				*pbuf = 0xDC;
				pbuf++;
				*typen += 2;
			}
			else if (0xDB == sdt->Data[i])
			{
				*pbuf = 0xDB;
				pbuf++;
				*pbuf = 0xDD;
				pbuf++;
				*typen += 2;
			}
			else
			{
				*pbuf = sdt->Data[i];
				pbuf++;
				*typen += 1;
			}
		}
//		*typen += temp2;	
    }
	else if (0x0c == sdt->ObjectId)
    {
		//信号机版本查询应答和主动上传数据内容是一样的；
		for (i = 0; i < 20; i++)
        {
			chc &= sdt->Data[i];
            if (0xC0 == sdt->Data[i])
            {
                *pbuf = 0xDB;
                pbuf++;
                *pbuf = 0xDC;
                pbuf++;
				*typen += 2;
            }
            else if (0xDB == sdt->Data[i])
            {
                *pbuf = 0xDB;
                pbuf++;
                *pbuf = 0xDD;
                pbuf++;
				*typen += 2;
            }
            else
            {
                *pbuf = sdt->Data[i];
                pbuf++;
				*typen += 1;
            }
        }
//		*typen += 20;
    }
	else if (0x0d == sdt->ObjectId)
    {
		if (0x83 == sdt->OperType)
		{//查询应答
			chc &= sdt->Data[0];
			if (0xC0 == sdt->Data[0])
            {
                *pbuf = 0xDB;
                pbuf++;
                *pbuf = 0xDC;
                pbuf++;
				*typen += 2;
            }
            else if (0xDB == sdt->Data[0])
            {
                *pbuf = 0xDB;
                pbuf++;
                *pbuf = 0xDD;
                pbuf++;
				*typen += 2;
            }
            else
            {
                *pbuf = sdt->Data[0];
                pbuf++;
				*typen += 1;
            }
//			*typen += 1;
		}//查询应答
		//特征参数版本设置应答数据内容是空的
    }
	else if (0x0e == sdt->ObjectId)
    {
		if (0x83 == sdt->OperType)
		{//仅有查询应答
			for (i = 0; i < 14; i++)
			{
				chc &= sdt->Data[i];
				if (0xC0 == sdt->Data[i])
				{
					*pbuf = 0xDB;
					pbuf++;
					*pbuf = 0xDC;
					pbuf++;
					*typen += 2;
				}
				else if (0xDB == sdt->Data[i])
				{
					*pbuf = 0xDB;
					pbuf++;
					*pbuf = 0xDD;
					pbuf++;
					*typen += 2;
				}
				else
				{
					*pbuf = sdt->Data[i];
					pbuf++;
					*typen += 1;
				}
			}
//			*typen += 14;
		}//仅有查询应答
    }
	//对象标识0x0f，远程控制仅仅有设置应答，数据内容是空的；
	else if (0x10 == sdt->ObjectId)
    {
		if (0x83 == sdt->OperType)
		{//查询应答
			temp1 = sdt->Data[0];
			temp2 = 1 + temp1*15;
			for (i = 0; i < temp2; i++)
			{
				chc &= sdt->Data[i];
				if (0xC0 == sdt->Data[i])
				{
					*pbuf = 0xDB;
					pbuf++;
					*pbuf = 0xDC;
					pbuf++;
					*typen += 2;
				}
				else if (0xDB == sdt->Data[i])
				{
					*pbuf = 0xDB;
					pbuf++;
					*pbuf = 0xDD;
					pbuf++;
					*typen += 2;
				}
				else
				{
					*pbuf = sdt->Data[i];
					pbuf++;
					*typen += 1;
				}
			}
//			*typen += temp2;
		}//查询应答
		//检测器设置应答数据内容是空的；	
    }

	if (0xC0 == chc)
	{
		*pbuf = 0xDB;
		pbuf++;
		*pbuf = 0xDC;
		pbuf++;
		*typen += 2;
	}
	else if (0xDB == chc)
	{
		*pbuf = 0xDB;
		pbuf++;
		*pbuf = 0xDD;
		pbuf++;
		*typen += 2;
	}
	else
	{
		*pbuf = chc;
		pbuf++;
		*typen += 1;
	}	
	*pbuf = 0xC0;
	pbuf++;
	*typen += 1;

	return SUCCESS;
}

int standard_protocol_discode(standard_data_t *sdt,unsigned char *buf,int *sock)
{
	if ((NULL == sdt) || (NULL == buf))
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	unsigned char		*pbuf = buf;
	unsigned short		i = 0;
	unsigned short		j = 0;
	unsigned short		temp1 = 0;
	unsigned short		temp2 = 0;
	unsigned short		tep = 0;
	unsigned char		chc = 0;
	unsigned char		ochc = 0;
	unsigned char		errn = 0;
	unsigned short		n_type = 0;
	unsigned char		n_buf[1038] = {0};
	standard_data_t		n_sdt;
	memset(&n_sdt,0,sizeof(standard_data_t));

	//Head
	sdt->Head = *pbuf;
	pbuf = pbuf+1;

	//data table
	if ((0xDB == *pbuf) && (0xDC == *(pbuf+1)))
	{
		sdt->VerId = 0xC0;
		pbuf = pbuf+2;
	}
	else if ((0xDB == *pbuf) && (0xDD == *(pbuf+1)))
	{
		sdt->VerId = 0xDB;
		pbuf = pbuf+2;
	}
	else
	{
		sdt->VerId = *pbuf;
		pbuf = pbuf+1;
	}
	chc = sdt->VerId;

	sdt->SendMark = *pbuf;
    pbuf = pbuf+1;
	chc &= sdt->SendMark;
	
    sdt->RecMark = *pbuf;
    pbuf = pbuf+1;
	chc &= sdt->RecMark;

	sdt->DataLink = *pbuf;
	pbuf = pbuf+1;
	chc &= sdt->DataLink;
	
	if ((0xDB == *pbuf) && (0xDC == *(pbuf+1)))
	{
		sdt->AreaId = 0xC0;
		pbuf = pbuf+2;
	}
	else if ((0xDB == *pbuf) && (0xDD == *(pbuf+1)))
	{
		sdt->AreaId = 0xDB;
		pbuf = pbuf+2;
	}
	else
	{
		sdt->AreaId = *pbuf;
		pbuf = pbuf+1;
	}

	chc &= sdt->AreaId;

	sdt->CrossId = 0;	
	if ((0xDB == *pbuf) && (0xDC == *(pbuf+1)))
    {
        sdt->CrossId |= 0xC0;
        pbuf = pbuf+2;
		chc &= 0xC0;
    }
    else if ((0xDB == *pbuf) && (0xDD == *(pbuf+1)))
    {
        sdt->CrossId |= 0xDB;
        pbuf = pbuf+2;
		chc &= 0xDB;
    }
    else
    {
        sdt->CrossId |= *pbuf;
		chc &= *pbuf;
        pbuf = pbuf+1;
    }

	if ((0xDB == *pbuf) && (0xDC == *(pbuf+1)))
    {
		tep = 0;
		tep |= 0xC0;	
		tep = (tep << 8) & 0xff00;
        sdt->CrossId |= tep;
		chc &= 0xC0;
        pbuf = pbuf+2;
    }
    else if ((0xDB == *pbuf) && (0xDD == *(pbuf+1)))
    {
		tep = 0;
		tep |= 0xDB;
		tep = (tep << 8) & 0xff00;
        sdt->CrossId |= tep;
		chc &= 0xDB;
        pbuf = pbuf+2;
    }
    else
    {
		tep = 0;
		tep |= *pbuf;
		tep = (tep << 8) & 0xff00;
        sdt->CrossId |= tep;
		chc &= *pbuf;
        pbuf = pbuf+1;
    }

	if ((0x0002 != sdt->CrossId) || (0x01 != sdt->AreaId))
	{
	#ifdef COMMON_DEBUG
        printf("Cross id is not corrent,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
		memset(&n_sdt,0,sizeof(standard_data_t));
		n_sdt.Head = 0xC0;
		n_sdt.VerId = 0x10;
		n_sdt.SendMark = 0x10;
		n_sdt.RecMark = 0x20;
		n_sdt.DataLink = sdt->DataLink;
		n_sdt.AreaId = 0x01;
		n_sdt.CrossId = 0x0002;
		n_sdt.OperType = 0x85;
		n_sdt.ObjectId = 0x01;
		//保留位用作口令admin，ascii为0x61,0x64,0x6D,0x69,0x6E
		n_sdt.Reserve[0] = 0x61;
		n_sdt.Reserve[1] = 0x64;
		n_sdt.Reserve[2] = 0x6D;
		n_sdt.Reserve[3] = 0x69;
		n_sdt.Reserve[4] = 0x6E;
		memset(n_sdt.Data,0,sizeof(n_sdt.Data));
		n_sdt.End = 0xC0;
		memset(n_buf,0,sizeof(n_buf));
		standard_protocol_encode(&n_sdt,n_buf,&n_type);
		write(*sock,n_buf,n_type);
		return FAIL2;
	}
	
	sdt->OperType = *pbuf;
	pbuf = pbuf+1;
	chc &= sdt->OperType;

	sdt->ObjectId = *pbuf;
	pbuf = pbuf+1;
	chc &= sdt->ObjectId;

	//Reserve bit
	sdt->Reserve[0] = *pbuf;
	pbuf = pbuf+1;
	chc &= sdt->Reserve[0];
	errn = 0;
	if ((0x61 != sdt->Reserve[0]) && (0x81 == sdt->OperType))
	{
	#ifdef COMMOM_DEBUG
		printf("Password is not right,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		errn = 1;
//        return FAIL;
	}

	sdt->Reserve[1] = *pbuf;
    pbuf = pbuf+1;
	chc &= sdt->Reserve[1];
	if ((0x64 != sdt->Reserve[1]) && (0x81 == sdt->OperType))
	{
	#ifdef COMMOM_DEBUG
        printf("Password is not right,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
		errn = 1;
//        return FAIL;
	}

	sdt->Reserve[2] = *pbuf;
    pbuf = pbuf+1;
	chc &= sdt->Reserve[2];
	if ((0x6D != sdt->Reserve[2]) && (0x81 == sdt->OperType))
	{
	#ifdef COMMOM_DEBUG
        printf("Password is not right,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
		errn = 1;
//        return FAIL;
	}

	sdt->Reserve[3] = *pbuf;
    pbuf = pbuf+1;
	chc &= sdt->Reserve[3];
	if ((0x69 != sdt->Reserve[3]) && (0x81 == sdt->OperType))
	{
	#ifdef COMMOM_DEBUG
        printf("Password is not right,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
		errn = 1;
//        return FAIL;
	}

	sdt->Reserve[4] = *pbuf;
    pbuf = pbuf+1;
	chc &= sdt->Reserve[4];
	if ((0x6E != sdt->Reserve[4]) && (0x81 == sdt->OperType))
	{
	#ifdef COMMOM_DEBUG
        printf("Password is not right,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
		errn = 1;
//        return FAIL;
	}
	if (1 == errn)
	{
	#ifdef COMMON_DEBUG
		printf("Password is not right,File:%s,Line:%d\n",__FILE__,__LINE__);
	#endif
		errn = 0;
		memset(&n_sdt,0,sizeof(standard_data_t));
        n_sdt.Head = 0xC0;
        n_sdt.VerId = 0x10;
        n_sdt.SendMark = 0x10;
        n_sdt.RecMark = 0x20;
        n_sdt.DataLink = sdt->DataLink;
        n_sdt.AreaId = 0x01;
        n_sdt.CrossId = 0x0002;
        n_sdt.OperType = 0x85;
        n_sdt.ObjectId = sdt->ObjectId;
        //保留位用作口令admin，ascii为0x61,0x64,0x6D,0x69,0x6E
        n_sdt.Reserve[0] = 0x61;
        n_sdt.Reserve[1] = 0x64;
        n_sdt.Reserve[2] = 0x6D;
        n_sdt.Reserve[3] = 0x69;
        n_sdt.Reserve[4] = 0x6E;
        memset(n_sdt.Data,0,sizeof(n_sdt.Data));
        n_sdt.End = 0xC0;
        memset(n_buf,0,sizeof(n_buf));
        standard_protocol_encode(&n_sdt,n_buf,&n_type);
        write(*sock,n_buf,n_type);

		return FAIL;
	}

	errn = 0;
	if (0x01 == sdt->ObjectId)
	{//联机request or 查询应答，数据是空的；
		if (0x84 == sdt->OperType)
		{
			memset(sdt->Data,0,sizeof(sdt->Data));
			sdt->Data[0] = *pbuf;
			pbuf = pbuf + 1;
			if (0x61 != sdt->Data[0])
			{
				errn = 1;
//				return FAIL2;
			}
			sdt->Data[1] = *pbuf;
            pbuf = pbuf + 1;
            if (0x64 != sdt->Data[1])
            {
				 errn = 1;
          //      return FAIL2;
            }
			sdt->Data[2] = *pbuf;
            pbuf = pbuf + 1;
            if (0x6D != sdt->Data[2])
            {
				 errn = 1;
           //     return FAIL2;
            }
			sdt->Data[3] = *pbuf;
            pbuf = pbuf + 1;
            if (0x69 != sdt->Data[3])
            {
				 errn = 1;
          //      return FAIL2;
            }
			sdt->Data[4] = *pbuf;
            pbuf = pbuf + 1;
            if (0x6E != sdt->Data[4])
            {
				errn = 1;
      //          return FAIL2;
            }
			if (1 == errn)
			{
				errn = 0;
			
				memset(&n_sdt,0,sizeof(standard_data_t));
				n_sdt.Head = 0xC0;
				n_sdt.VerId = 0x10;
				n_sdt.SendMark = 0x10;
				n_sdt.RecMark = 0x20;
				n_sdt.DataLink = sdt->DataLink;
				n_sdt.AreaId = 0x01;
				n_sdt.CrossId = 0x0002;
				n_sdt.OperType = 0x85;
				n_sdt.ObjectId = sdt->ObjectId;
				//保留位用作口令admin，ascii为0x61,0x64,0x6D,0x69,0x6E
				n_sdt.Reserve[0] = 0x61;
				n_sdt.Reserve[1] = 0x64;
				n_sdt.Reserve[2] = 0x6D;
				n_sdt.Reserve[3] = 0x69;
				n_sdt.Reserve[4] = 0x6E;
				memset(n_sdt.Data,0,sizeof(n_sdt.Data));
				n_sdt.End = 0xC0;
				memset(n_buf,0,sizeof(n_buf));
				standard_protocol_encode(&n_sdt,n_buf,&n_type);
				write(*sock,n_buf,n_type);	
				return FAIL2;
			}	
		}//if (0x84 == sdt->OperType)
		else
		{
			memset(sdt->Data,0,sizeof(sdt->Data));
		}
	}//联机request or 查询应答，数据是空的；
	else if (0x02 == sdt->ObjectId)
	{//交通流主动上报，上位机解析，信号机这边不用解析；
		memset(sdt->Data,0,sizeof(sdt->Data));
	}//交通流主动上报，上位机解析，信号机这边不用解析；
	else if (0x03 == sdt->ObjectId)
    {//仅有上位机对信号机的工作状态查询；
		memset(sdt->Data,0,sizeof(sdt->Data));
    }//仅有上位机对信号机的工作状态查询；
	else if (0x04 == sdt->ObjectId)
    {//灯色状态查询指令；
		memset(sdt->Data,0,sizeof(sdt->Data));
    }//灯色状态查询指令；
	else if (0x05 == sdt->ObjectId)
    {//时间查询和设置操作
		if (0x80 == (sdt->OperType))
		{//上位机发出时间查询指令；
			memset(sdt->Data,0,sizeof(sdt->Data));
		}//上位机发出时间查询指令；	
		if (0x81 == (sdt->OperType))
		{//上位机发出时间设置指令；
			memset(sdt->Data,0,sizeof(sdt->Data));
			for (i = 0; i < 4; i++)
			{
				if ((0xDB == *pbuf) && (0xDC == *(pbuf+1)))
				{
					sdt->Data[i] = 0xC0;
					pbuf = pbuf + 2;
				}
				else if ((0xDB == *pbuf) && (0xDD == *(pbuf+1)))
				{
					sdt->Data[i] = 0xDB;
					pbuf = pbuf + 2;
				}
				else
				{
					sdt->Data[i] = *pbuf;
					pbuf++;
				}
				chc &= sdt->Data[i];
			}
		}//上位机发出时间设置指令；
    }//时间查询和设置操作
	else if (0x06 == sdt->ObjectId)
    {//灯组查询及设置
		if (0x80 == (sdt->OperType))
		{
			memset(sdt->Data,0,sizeof(sdt->Data));
		}
		if (0x81 == (sdt->OperType))
		{
			memset(sdt->Data,0,sizeof(sdt->Data));
			if ((0xDB == *pbuf) && (0xDC == *(pbuf+1)))
			{
				temp1 = 0xC0;
				pbuf = pbuf + 2;
			}
			else if ((0xDB == *pbuf) && (0xDD == *(pbuf+1)))
			{
				temp1 = 0xDB;
				pbuf = pbuf + 2;
			}
			else
			{
				temp1 = *pbuf;
				pbuf = pbuf + 1;
			}
			sdt->Data[0] = temp1;
			chc &= sdt->Data[0];
			temp2 = temp1*12;
			for (i = 1; i < (temp2+1); i++)
			{
				if ((0xDB == *pbuf) && (0xDC == *(pbuf+1)))
				{
					sdt->Data[i] = 0xC0;
					pbuf = pbuf + 2;
				}
				else if ((0xDB == *pbuf) && (0xDD == *(pbuf+1)))
				{
					sdt->Data[i] = 0xDB;
					pbuf = pbuf + 2;
				}
				else
				{
					sdt->Data[i] = *pbuf;
					pbuf = pbuf + 1;
				}
				chc &= sdt->Data[i];				
			}				
		}
    }//灯组查询及设置
	else if (0x07 == sdt->ObjectId)
    {//相位查询指令及设置
		if (0x80 == (sdt->OperType))
		{
			memset(sdt->Data,0,sizeof(sdt->Data));
		}
		if (0x81 == (sdt->OperType))
		{
			memset(sdt->Data,0,sizeof(sdt->Data));
			if ((0xDB == *pbuf) && (0xDC == *(pbuf+1)))
            {
                temp1 = 0xC0;
                pbuf = pbuf + 2;
            }
            else if ((0xDB == *pbuf) && (0xDD == *(pbuf+1)))
            {
                temp1 = 0xDB;
                pbuf = pbuf + 2;
            }
            else
            {
                temp1 = *pbuf;
                pbuf = pbuf + 1;
            }
            sdt->Data[0] = temp1;
			chc &= sdt->Data[0];
			temp2 = temp1*24;
			for (i = 1; i < (temp2+1); i++)
            {	
				if ((0xDB == *pbuf) && (0xDC == *(pbuf+1)))
				{
					sdt->Data[i] = 0xC0;
					pbuf = pbuf + 2;
				}
				else if ((0xDB == *pbuf) && (0xDD == *(pbuf+1)))
				{
					sdt->Data[i] = 0xDB;
					pbuf = pbuf + 2;
				}
				else
				{
					sdt->Data[i] = *pbuf;
					pbuf = pbuf + 1;
				}
				chc &= sdt->Data[i];
            }
		}
    }//相位查询指令及设置
	else if (0x08 == sdt->ObjectId)
    {//信号配时方案查询及设置
		if (0x80 == (sdt->OperType))
		{
			memset(sdt->Data,0,sizeof(sdt->Data));
		}
		if (0x81 == (sdt->OperType))
		{
			memset(sdt->Data,0,sizeof(sdt->Data));
			if ((0xDB == *pbuf) && (0xDC == *(pbuf+1)))
            {
                temp1 = 0xC0;
                pbuf = pbuf + 2;
            }
            else if ((0xDB == *pbuf) && (0xDD == *(pbuf+1)))
            {
                temp1 = 0xDB;
                pbuf = pbuf + 2;
            }
            else
            {
                temp1 = *pbuf;
                pbuf = pbuf + 1;
            }
            sdt->Data[0] = temp1;
			chc &= sdt->Data[0];
			temp2 = temp1*24;
			for (i = 1; i < (temp2+1); i++)
            {
				if ((0xDB == *pbuf) && (0xDC == *(pbuf+1)))
				{
					sdt->Data[i] = 0xC0;
					pbuf = pbuf + 2;
				}
				else if ((0xDB == *pbuf) && (0xDD == *(pbuf+1)))
				{
					sdt->Data[i] = 0xDB;
					pbuf = pbuf + 2;
				}
				else
				{
					sdt->Data[i] = *pbuf;
					pbuf = pbuf + 1;
				}
				chc &= sdt->Data[i];
            }
		}
    }//信号配时方案查询及设置
	else if (0x09 == sdt->ObjectId)
    {//方案调度计划查询和设置
		if (0x80 == (sdt->OperType))
		{
			memset(sdt->Data,0,sizeof(sdt->Data));
		}
		if (0x81 == (sdt->OperType))
		{
			memset(sdt->Data,0,sizeof(sdt->Data));
			if ((0xDB == *pbuf) && (0xDC == *(pbuf+1)))
            {
                temp1 = 0xC0;
                pbuf = pbuf + 2;
            }
            else if ((0xDB == *pbuf) && (0xDD == *(pbuf+1)))
            {
                temp1 = 0xDB;
                pbuf = pbuf + 2;
            }
            else
            {
                temp1 = *pbuf;
                pbuf = pbuf + 1;
            }
            sdt->Data[0] = temp1;
			chc &= sdt->Data[0];
            temp2 = temp1*12;
            for (i = 1; i < (temp2+1); i++)
            {
				if ((0xDB == *pbuf) && (0xDC == *(pbuf+1)))
				{
					sdt->Data[i] = 0xC0;
					pbuf = pbuf + 2;
				}
				else if ((0xDB == *pbuf) && (0xDD == *(pbuf+1)))
				{
					sdt->Data[i] = 0xDB;
					pbuf = pbuf + 2;
				}
				else
				{
					sdt->Data[i] = *pbuf;
					pbuf = pbuf + 1;
				}
				chc &= sdt->Data[i];
            }
		}
    }//方案调度计划查询和设置
	else if (0x0a == sdt->ObjectId)
    {//工作方式查询和设置
		if (0x80 == (sdt->OperType))
        {
            memset(sdt->Data,0,sizeof(sdt->Data));
        }
		if (0x81 == (sdt->OperType))
		{
			memset(sdt->Data,0,sizeof(sdt->Data));
			if ((0xDB == *pbuf) && (0xDC == *(pbuf+1)))
			{
				sdt->Data[0] = 0xC0;
				pbuf = pbuf + 2;
			}
			else if ((0xDB == *pbuf) && (0xDD == *(pbuf+1)))
			{
				sdt->Data[0] = 0xDB;
				pbuf = pbuf + 2;
			}
			else
			{
				sdt->Data[0] = *pbuf;
				pbuf = pbuf + 1;
			}
			chc &= sdt->Data[0];			
		}
    }//工作方式查询和设置
	else if (0x0b == sdt->ObjectId)
    {//信号机故障查询
		if (0x80 == (sdt->OperType))
		{
			memset(sdt->Data,0,sizeof(sdt->Data));
		}
    }//信号机故障查询
	else if (0x0c == sdt->ObjectId)
    {//信号机版本查询
		if (0x80 == (sdt->OperType))
        {
            memset(sdt->Data,0,sizeof(sdt->Data));
        }	
    }//信号机版本查询
	else if (0x0d == sdt->ObjectId)
    {//特征参数版本查询和设置
		if (0x80 == (sdt->OperType))
		{
			memset(sdt->Data,0,sizeof(sdt->Data));
		}
		if (0x81 == (sdt->OperType))
		{
			memset(sdt->Data,0,sizeof(sdt->Data));
			if ((0xDB == *pbuf) && (0xDC == *(pbuf+1)))
			{
				sdt->Data[0] = 0xC0;
				pbuf = pbuf + 2;
			}
			else if ((0xDB == *pbuf) && (0xDD == *(pbuf+1)))
			{
				sdt->Data[0] = 0xDB;
				pbuf = pbuf + 2;
			}
			else
			{
				sdt->Data[0] = *pbuf;
				pbuf = pbuf + 1;
			}
			chc &= sdt->Data[0];
		}
    }//特征参数版本查询和设置
	else if (0x0e == sdt->ObjectId)
    {//信号机识别码查询
		if (0x80 == (sdt->OperType))
        {
            memset(sdt->Data,0,sizeof(sdt->Data));
        }	
    }//信号机识别码查询
	else if (0x0f == sdt->ObjectId)
    {//远程控制设置
		if (0x81 == (sdt->OperType))
		{
			memset(sdt->Data,0,sizeof(sdt->Data));
			if ((0xDB == *pbuf) && (0xDC == *(pbuf+1)))
            {
                sdt->Data[0] = 0xC0;
                pbuf = pbuf + 2;
            }
            else if ((0xDB == *pbuf) && (0xDD == *(pbuf+1)))
            {
                sdt->Data[0] = 0xDB;
                pbuf = pbuf + 2;
            }
            else
            {
                sdt->Data[0] = *pbuf;
                pbuf = pbuf + 1;
            }
			chc &= sdt->Data[0];
		}
    }//远程控制设置
	else if (0x10 == sdt->ObjectId)
    {//检测器查询和设置
		if (0x80 == (sdt->OperType))
        {
            memset(sdt->Data,0,sizeof(sdt->Data));
        }
		if (0x81 == (sdt->OperType))
		{
			memset(sdt->Data,0,sizeof(sdt->Data));
			if ((0xDB == *pbuf) && (0xDC == *(pbuf+1)))
            {
                temp1 = 0xC0;
                pbuf = pbuf + 2;
            }
            else if ((0xDB == *pbuf) && (0xDD == *(pbuf+1)))
            {
                temp1 = 0xDB;
                pbuf = pbuf + 2;
            }
            else
            {
                temp1 = *pbuf;
                pbuf = pbuf + 1;
            }
            sdt->Data[0] = temp1;
			chc &= sdt->Data[0];
            temp2 = temp1*15;
            for (i = 1; i < (temp2+1); i++)
            {
				if ((0xDB == *pbuf) && (0xDC == *(pbuf+1)))
				{
					sdt->Data[i] = 0xC0;
					pbuf = pbuf + 2;
				}
				else if ((0xDB == *pbuf) && (0xDD == *(pbuf+1)))
				{
					sdt->Data[i] = 0xDB;
					pbuf = pbuf + 2;
				}
				else
				{
					sdt->Data[i] = *pbuf;
					pbuf = pbuf + 1;
				}
				chc &= sdt->Data[i];
            }
		}	
    }//检测器查询和设置

	//check code
	if ((0xDB == *pbuf) && (0xDC == *(pbuf+1)))
	{
		ochc = 0xC0;
		pbuf = pbuf + 2;
	}
	else if ((0xDB == *pbuf) && (0xDD == *(pbuf+1)))
	{
		ochc = 0xDB;
        pbuf = pbuf + 2;
	}
	else
	{
		ochc = *pbuf;
		pbuf = pbuf+1;
	}

	if (chc == ochc)
	{
	#ifdef COMMON_DEBUG
		printf("This is our data,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
	}
	else
	{
	#ifdef COMMON_DEBUG
        printf("This is not our data,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
	}

	//End
	sdt->End = *pbuf;
	if (0x01 != sdt->ObjectId)
	{
		if (0xc0 != sdt->End)
		{
			memset(&n_sdt,0,sizeof(standard_data_t));
			n_sdt.Head = 0xC0;
			n_sdt.VerId = 0x10;
			n_sdt.SendMark = 0x10;
			n_sdt.RecMark = 0x20;
			n_sdt.DataLink = sdt->DataLink;
			n_sdt.AreaId = 0x01;
			n_sdt.CrossId = 0x0002;
			n_sdt.OperType = 0x85;
			n_sdt.ObjectId = sdt->ObjectId;
			//保留位用作口令admin，ascii为0x61,0x64,0x6D,0x69,0x6E
			n_sdt.Reserve[0] = 0x61;
			n_sdt.Reserve[1] = 0x64;
			n_sdt.Reserve[2] = 0x6D;
			n_sdt.Reserve[3] = 0x69;
			n_sdt.Reserve[4] = 0x6E;
			memset(n_sdt.Data,0,sizeof(n_sdt.Data));
			n_sdt.End = 0xC0;
			memset(n_buf,0,sizeof(n_buf));
			standard_protocol_encode(&n_sdt,n_buf,&n_type);
			write(*sock,n_buf,n_type);
			return FAIL;
		}
	}
	pbuf = pbuf+1;

	return SUCCESS;
}

int send_close_lamp_status(int *sock,unsigned char pattern,unsigned char *chan)
{
	if ((NULL == sock) || (NULL == chan))
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	unsigned char		buf[15] = {0};
	unsigned char		*pbuf = buf;
	unsigned char		i = 0;
	
	*pbuf = 0x21;
	pbuf++;
	*pbuf = 0x83;
	pbuf++;
	*pbuf = 0xEC;
	pbuf++;
	*pbuf = 0x00;
	pbuf++;

	*pbuf = pattern;
	pbuf++;

	for (i = 0; i < 16; i++)
	{
		if (0 == chan[i])
			break;
	}
	*pbuf = i;
	pbuf++;

	for (i = 0; i < 16; i++)
    {
        if (0 == chan[i])
            break;
		*pbuf = chan[i];
		pbuf++;
    }	

	write(*sock,buf,sizeof(buf));
	
	return SUCCESS;
}


int RecSen_Module_DataDecode(unsigned char *buf,RecSenData_t *rbd)
{
	if ((NULL == buf) || (NULL == rbd))
	{
	#ifdef COMMON_DEBUG
		printf("Poniter address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char			*pbuf = buf;
	unsigned char			i = 0,j = 0;
	unsigned char			len = 0;
	unsigned short			tbus = 0;
	for (i = 0; i < 64; i ++)
	{//for (i = 0; i < 64; i ++)
		if (0xF2 == *pbuf)
		{
			len = *(pbuf+1);
			if (0xED == *(pbuf+len+1))
			{
				rbd->Head = *pbuf;
				pbuf++;
				rbd->Len = *pbuf;
				pbuf++;
				rbd->Func = *pbuf;
				pbuf++;
				if (0x04 == rbd->Func)
				{
					rbd->Err = *pbuf;
					break;
				}

				tbus = 0;
				tbus = *pbuf;
				pbuf++;
				rbd->Busn |= tbus;
				tbus = 0;
				tbus = *pbuf;
				pbuf++;
				tbus <<= 8;
				tbus &= 0xff00;
				rbd->Busn |= tbus; 			
	
				rbd->Udlink = *pbuf;
				pbuf++;
				for (j = 0; j < 13; j++)
				{
					rbd->ChePaiN[j] = *pbuf;
					pbuf++;
				}
				rbd->End = *pbuf;
				pbuf++;

				break;
			}
			else
			{
				pbuf++;
				continue;
			}
		}
		else
		{
			pbuf++;
			continue;
		}	
	}//for (i = 0; i < 64; i ++)

	return SUCCESS;
}

int SenData1_Encode(unsigned char *buf,SenData1_t *sd1)
{
	if ((NULL == buf) || (NULL == sd1))
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	unsigned char		*pbuf = buf;
	unsigned char		i = 0;	

	*pbuf = sd1->Head;
	pbuf++;
	*pbuf = sd1->Len;
	pbuf++;
	*pbuf = sd1->Func;
	pbuf++;
	
	for (i = 0; i < 13; i++)
	{
		*pbuf = sd1->ChePaiN[i];
		pbuf++;
	}
	*pbuf = sd1->PhaseId;
	pbuf++;
	*pbuf = sd1->End;
	pbuf++;

	return SUCCESS;
}

int SenData2_Encode(unsigned char *buf,SenData2_t *sd2)
{
	if ((NULL == buf) || (NULL == sd2))
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char		*pbuf = buf;
	unsigned char		i = 0;

	*pbuf = sd2->Head;
	pbuf++;
	*pbuf = sd2->Len;
	pbuf++;
	*pbuf = sd2->Func;
	pbuf++;
	*pbuf = sd2->PhaseN;
	pbuf++;

	for (i = 0; i < (sd2->PhaseN); i++)
	{
		*pbuf = sd2->PI[i].PhaseId;
		pbuf++;
		*pbuf = sd2->PI[i].LampColor;
		pbuf++;
		*pbuf = sd2->PI[i].PhaseT;
		pbuf++;
	}

	*pbuf = sd2->End;
	pbuf++;

	return SUCCESS;
}

int channel_control_encode(unsigned char *buf,unsigned char mark)
{
	if (NULL == buf)
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char			*pbuf = buf;
		
	*pbuf = 0x21;
	pbuf++;
	*pbuf = 0x85;
	pbuf++;
	*pbuf = 0xEF;
	pbuf++;
	*pbuf = 0x00;
	pbuf++;
	*pbuf = mark;
	pbuf++;

	return SUCCESS;
}

int delay_time_report(sockfd_t *sockfd, unsigned char delaytime)
{
	if (NULL == sockfd)
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char		buf[6] = {0};
	unsigned char		*pbuf = buf;
	*pbuf = 0x21;
	pbuf++;
	*pbuf = 0x85;
	pbuf++;
	*pbuf = 0xF9;
	pbuf++;
	*pbuf = 0x00;
	pbuf++;
	*pbuf = delaytime;
	pbuf++;

	write(*(sockfd->csockfd),buf,sizeof(buf));

	return SUCCESS;
}

int SenServer_Data_Encode(unsigned char *buf,SenServer_t *ss)
{
	if ((NULL == buf) || (NULL == ss))
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}	
	unsigned char			*pbuf = buf;
	unsigned char			i = 0;
	unsigned short			temp = 0;

	*pbuf = ss->Head;
	pbuf++;
	*pbuf = ss->Operate;
	pbuf++;
	*pbuf = ss->ObjectId;
	pbuf++;
	*pbuf = ss->Rec;
	pbuf++;

	for (i = 0; i < 13; i++)
	{
		*pbuf = ss->ChePaiN[i];
		pbuf++;
	}
	*pbuf = ss->udlink;
	pbuf++;
	*pbuf = ss->PhaseId;
	pbuf++;
	*pbuf = ss->Mark;
	pbuf++;

	temp = ss->TimeT;
	temp &= 0x00ff;
	*pbuf = temp;
	pbuf++;
	temp = ss->TimeT;
	temp &= 0xff00;
	temp >>= 8;
	temp &= 0x00ff;
	*pbuf = temp;
	pbuf++;

	return SUCCESS;
}

//According to channel id to match fit phase id;
int match_phase(tscdata_t *td,unsigned char tcline,unsigned char slnum,unsigned char chid,unsigned char *phid)
{
	if ((NULL == td) || (NULL == phid))
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	unsigned char			i = 0,j = 0;
	unsigned char			tcid = 0;
	unsigned char			pid = 0;
	unsigned char			find = 0;

	//compute match phase id of chanid
	i = slnum;
	while (1)
	{
		get_phase_id(td->timeconfig->TimeConfigList[tcline][i].PhaseId,&pid);
	
		for (j = 0; j < (td->channel->FactChannelNum); j++)
		{
			if (0 == (td->channel->ChannelList[j].ChannelId))
				break;
			if (pid == (td->channel->ChannelList[j].ChannelCtrlSrc))
			{
				if (chid == (td->channel->ChannelList[j].ChannelId))
				{
					*phid = pid;
					find = 1;
					break;
				}	
			}	
		}	
		if (1 == find)
		{
			break;
		}

		i = i + 1;
		if (0 == (td->timeconfig->TimeConfigList[tcline][i].StageId))
		{
			i = 0;
		}
		if (i == slnum)
			break;			
	}//while (1)
	if (0 == find)
		return FAIL;//not find map phase

	return SUCCESS;
}

int HaiKang_Video_DisCode(unsigned char *buf,unsigned short bufn,RadarErrList_t *rel,StatisDataList_t *sdl,QueueDataList_t *qdl,TrafficDataList_t *tdl,EvaDataList_t *edl)
{
	if ((NULL == buf)||(NULL == rel)||(NULL == sdl)||(NULL == qdl)||(NULL == tdl)||(NULL == edl))
	{
	#ifdef COMMON_DEBUG
        printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
    #endif
        return MEMERR;
	}

	unsigned short					i = 0,j = 0;
	unsigned char					*pbuf = buf;
	unsigned char					check = 0;
	unsigned short					len = 0;
	unsigned short					temp = 0;
	unsigned char					roadid = 0;
	unsigned char					cmd = 0;

	while (1)
	{//while loop
		if (j >= bufn)
			break;
		check = 0;
		len = 0;
		if (0xFE == *pbuf)
		{//if (0xFE == *pbuf)
			check ^= *pbuf;
			pbuf += 1;//cmd
			j += 1;
			if (0x03 == *pbuf)
			{//if (0x03 == *pbuf))
				cmd = *pbuf;
				check ^= *pbuf;

				pbuf += 1;//Res
				j += 1;
				check ^= *pbuf;
				pbuf += 1;//Res
				j += 1;
				check ^= *pbuf;

				pbuf += 1;//ID
				j += 1;
				check ^= *pbuf;
				pbuf += 1;//ID
				j += 1;
				check ^= *pbuf;

				pbuf += 1;//Len
				j += 1;
				check ^= *pbuf;
				len |= *pbuf;
				pbuf += 1;//Len
				j += 1;
				check ^= *pbuf;
				temp = 0;
				temp |= *pbuf;
				temp <<= 8;
				temp &= 0xff00;
				len |= temp;
				#ifdef COMMON_DEBUG
				printf("Data length: %d,File: %s, Line: %d\n",len,__FILE__,__LINE__);
				#endif
				pbuf += 1;//lane
				j += 1;
				check ^= *pbuf;
				roadid = *pbuf;

				pbuf += 1;//speed
				j += 1;
                check ^= *pbuf;

				pbuf += 1;//status
				j += 1;
                check ^= *pbuf;

				pbuf += 1;//queue
				j += 1;
                check ^= *pbuf;
				qdl->QueueData[roadid-1].RoadId = roadid;
				qdl->QueueData[roadid-1].QueueLen = *pbuf + BLINDLEN;//估计有30米盲区	
				qdl->QueueData[roadid-1].QueueNum = (*pbuf+BLINDLEN)/CARLEN;				
				qdl->QueueData[roadid-1].valid = 1;
				gettimeofday(&(qdl->tl),NULL);
				qdl->RoadNum += 1;
				
				for (i = 0; i < 24; i++)
				{
					pbuf += 1;
					j += 1;
					check ^= *pbuf;
				}
						 
				pbuf += 1;//check
				j += 1;
				if (check != *pbuf)
				{
				#ifdef COMMON_DEBUG
					printf("***********************Not fit data,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
				}
			}//if (0x03 == *pbuf)
			else if ((0x08 == *pbuf) || (0x01 == *pbuf) || (0x02 == *pbuf))
			{//else if (0x08 == *pbuf)
			#ifdef COMMON_DEBUG
				printf("Not consider other data temply,CMD:%d,File:%s,Line:%d\n",*pbuf,__FILE__,__LINE__);
			#endif
			}//else if (0x08 == *pbuf)
		}//if (0xFE == *pbuf)
		else
		{
			j += 1;
            pbuf += 1;
            continue;
		}	
	}//while loop

	return SUCCESS;
}

int HuiEr_Radar_DisCode(unsigned char *buf,unsigned short bufn,RadarErrList_t *rel,StatisDataList_t *sdl,QueueDataList_t *qdl,TrafficDataList_t *tdl,EvaDataList_t *edl)
{
	if ((NULL == buf) || (NULL == rel) || (NULL == sdl) || (NULL == qdl) || (NULL == tdl) || (NULL == edl))
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	unsigned char			revbuf[512] = {0};
	unsigned char			*pbuf = buf;
	unsigned char			*bpbuf = NULL;
	unsigned short			i = 0, j = 0;
	unsigned char			link = 0;
	unsigned char			ver = 0;
	unsigned char			oper = 0;
	unsigned char			object = 0;
	unsigned char			check = 0;
	unsigned int			temp = 0;
	unsigned short			temp1 = 0;	
	unsigned short			mark = 0;
	unsigned char			roadid = 0;

	//Firstly,replace;
	while (1)
	{//1
		if (j >= bufn)
			break;
		if (0x7E == *pbuf)
		{//if (0x7E == *pbuf)
			memset(revbuf,0,sizeof(revbuf));
			revbuf[0] = *pbuf;
			pbuf += 1;
			i = 1;
			mark = 1;
			j += 1;
			for (i = 1; i < bufn; i++)
			{//2
				if (mark >= bufn)
					break;
				if ((0x7D == *pbuf) && (0x5E == *(pbuf+1)))
				{
					revbuf[i] = 0x7E;
					pbuf += 2;
					mark += 2;
					j += 2; 
				}
				else if ((0x7D == *pbuf) && (0x5D == *(pbuf+1)))	
				{
					revbuf[i] = 0x7D;
					pbuf += 2;
					mark += 2;
					j += 2;
				}
				else
				{
					revbuf[i] = *pbuf;
					if (0x7E == *pbuf)
					{
						pbuf += 1;
						mark += 1;
						j += 1;
						break;//last one byte,break;
					}
					else
					{
						pbuf += 1;
						mark += 1;
						j += 1;
					}
				}
			}//2
			//begin get to data	
			bpbuf = revbuf;
			bpbuf += 1;
			check = 0;
			link = *bpbuf;
			check ^= *bpbuf;
			bpbuf += 1;

			ver = *bpbuf;
			check ^= *bpbuf;
			bpbuf += 1;

			oper = *bpbuf;
			check ^= *bpbuf;
			bpbuf += 1;

			object = *bpbuf;
			check ^= *bpbuf;
			bpbuf += 1;

			if (0x09 == object)
			{//err report actively;
				gettimeofday(&(rel->tl),NULL);
				rel->RoadNum = *bpbuf;
				check ^= *bpbuf;
				bpbuf += 1;
				for (i = 0; i < rel->RoadNum; i++)
				{
					roadid = *bpbuf;//backup road id;
					rel->RadarErr[roadid-1].RoadId = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					rel->RadarErr[roadid-1].RoadStatus = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					rel->RadarErr[roadid-1].ErrType = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					rel->RadarErr[roadid-1].valid = 1;
				}
				#ifdef COMMON_DEBUG
				printf("check: 0x%02x, *bpbuf: 0x%02x,File: %s,Line: %d\n",check,*bpbuf,__FILE__,__LINE__);
				#endif
				if ((check != *bpbuf) || (0x7E != *(bpbuf+1)))
				{
				#ifdef COMMON_DEBUG
					printf("Not fit data,File: %s, Line: %d\n",__FILE__,__LINE__);
				#endif
					memset(rel,0,sizeof(RadarErrList_t));
				}
			}//err report actively;
		#if 0 
			else if (0x05 == object)
			{//statis data report actively
				gettimeofday(&(sdl->tl),NULL);
				temp = 0;
				temp |= *bpbuf;
				check ^= *bpbuf;
				sdl->FormTime |= temp;
				temp = 0;
				bpbuf += 1;

				temp |= *bpbuf;
				check ^= *bpbuf;	
				temp <<= 8;
				temp &= 0x0000ff00;
				sdl->FormTime |= temp;
				temp = 0;
				bpbuf += 1;

				temp |= *bpbuf;
				check ^= *bpbuf;
				temp <<= 16;
				temp &= 0x00ff0000;
				sdl->FormTime |= temp;
				temp = 0;
				bpbuf += 1;

				temp |= *bpbuf;
				check ^= *bpbuf;
				temp <<= 24;
				temp &= 0xff000000;
				sdl->FormTime |= temp;
				temp = 0;
				bpbuf += 1;

				temp1 = 0;
				temp1 |= *bpbuf;
				check ^= *bpbuf;
				sdl->StatisCycle |= temp1;
				temp1 = 0;
				bpbuf += 1;

				temp1 |= *bpbuf;
				check ^= *bpbuf;
				temp1 <<= 8;
				temp1 &= 0xff00;
				sdl->StatisCycle |= temp1;
				temp1 = 0;
				bpbuf += 1;

				sdl->TypeALen = *bpbuf;
				check ^= *bpbuf;
				bpbuf += 1;
				sdl->TypeBLen = *bpbuf;
				check ^= *bpbuf;
				bpbuf += 1;
				sdl->TypeCLen = *bpbuf;
				check ^= *bpbuf;
				bpbuf += 1;
				sdl->Reser1 = *bpbuf;
				check ^= *bpbuf;
				bpbuf += 1;
				sdl->Reser2 = *bpbuf;
				check ^= *bpbuf;
				bpbuf += 1;
				sdl->Reser3 = *bpbuf;
				check ^= *bpbuf;
				bpbuf += 1;
				sdl->Reser4 = *bpbuf;
				check ^= *bpbuf;
				bpbuf += 1;		
				sdl->RoadNum = *bpbuf;
				check ^= *bpbuf;
				bpbuf += 1;

				for (i = 0; i < sdl->RoadNum; i++)
				{
					roadid = *bpbuf;//backup road id;
					sdl->StatisData[roadid-1].RoadId = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					sdl->StatisData[roadid-1].TypeAFlow = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					sdl->StatisData[roadid-1].TypeBFlow = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					sdl->StatisData[roadid-1].TypeCFlow = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					sdl->StatisData[roadid-1].Occupancy = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					sdl->StatisData[roadid-1].AverSpeed = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					sdl->StatisData[roadid-1].AverCarLen = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					sdl->StatisData[roadid-1].AverHeadWay = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					sdl->StatisData[roadid-1].QueueLen = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					sdl->StatisData[roadid-1].TrafficInfo = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					sdl->StatisData[roadid-1].Reser1 = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					sdl->StatisData[roadid-1].Reser2 = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					sdl->StatisData[roadid-1].Reser3 = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					sdl->StatisData[roadid-1].valid = 1;
				}
				#ifdef COMMON_DEBUG
				printf("check: 0x%02x, *bpbuf: 0x%02x,File: %s,Line: %d\n",check,*bpbuf,__FILE__,__LINE__);
				#endif
				if ((check != *bpbuf) || (0x7E != *(bpbuf+1)))
				{
				#ifdef COMMON_DEBUG
					printf("Not fit data,File: %s, Line: %d\n",__FILE__,__LINE__);
				#endif
					memset(sdl,0,sizeof(StatisDataList_t));
				}
			}//statis data report actively
		#endif
			else if (0x51 == object)
			{//queue data report actively
				gettimeofday(&(qdl->tl),NULL);
				qdl->RoadNum = *bpbuf;
				check ^= *bpbuf;
				bpbuf += 1;
				for (i = 0; i < qdl->RoadNum; i++)
				{
					roadid = *bpbuf;//backup road id
					if ((roadid >= ROADN) || (0 == roadid))
					{
						bpbuf += 5;
						continue;
					}
					qdl->QueueData[roadid-1].RoadId = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					qdl->QueueData[roadid-1].QueueLen = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					qdl->QueueData[roadid-1].QueueHead = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					qdl->QueueData[roadid-1].QueueEnd = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					qdl->QueueData[roadid-1].QueueNum = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					qdl->QueueData[roadid-1].valid = 1;
				}
				#ifdef COMMON_DEBUG
				printf("check: 0x%02x, *bpbuf: 0x%02x,File: %s,Line: %d\n",check,*bpbuf,__FILE__,__LINE__);
				#endif
				if ((check != *bpbuf) || (0x7E != *(bpbuf+1)))
				{
				#ifdef COMMON_DEBUG 
					printf("Not fit data,File: %s, Line: %d\n",__FILE__,__LINE__);
				#endif
					memset(qdl,0,sizeof(QueueDataList_t));
				}
			}//queue data report actively
		#if 0
			else if (0x52 == object)
			{//road condition data report actively
				gettimeofday(&(tdl->tl),NULL);
				tdl->RoadNum = *bpbuf;
				check ^= *bpbuf;
				bpbuf += 1;
				for (i = 0; i < tdl->RoadNum; i++)
				{
					roadid = *bpbuf;//backup road id;
					tdl->TrafficData[roadid-1].RoadId = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					tdl->TrafficData[roadid-1].CarNum = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					tdl->TrafficData[roadid-1].SpaceOccu = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					tdl->TrafficData[roadid-1].AverSpeed = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					tdl->TrafficData[roadid-1].SpreadInfo = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					tdl->TrafficData[roadid-1].LastCarLoca = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					tdl->TrafficData[roadid-1].valid = 1;
				}
				#ifdef COMMON_DEBUG
				printf("check: 0x%02x, *bpbuf: 0x%02x,File: %s,Line: %d\n",check,*bpbuf,__FILE__,__LINE__);
				#endif
				if ((check != *bpbuf) || (0x7E != *(bpbuf+1)))
				{
				#ifdef COMMON_DEBUG 
					printf("Not fit data,File: %s, Line: %d\n",__FILE__,__LINE__);
				#endif
					memset(tdl,0,sizeof(TrafficDataList_t));
				}
			}//road condition data report actively
			else if (0x53 == object)
			{//evaluation data report actively
				gettimeofday(&(edl->tl),NULL);
				edl->RoadNum = *bpbuf;
				check ^= *bpbuf;
				bpbuf += 1;
				for (i = 0; i < edl->RoadNum; i++)
				{
					roadid = *bpbuf;
					edl->EvaData[roadid-1].RoadId = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					edl->EvaData[roadid-1].AverStop = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					edl->EvaData[roadid-1].AverDelay = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					edl->EvaData[roadid-1].FuelConsume = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					edl->EvaData[roadid-1].GasEmiss = *bpbuf;
					check ^= *bpbuf;
					bpbuf += 1;
					edl->EvaData[roadid-1].valid = 1;
				}
				#ifdef COMMON_DEBUG
				printf("check: 0x%02x, *bpbuf: 0x%02x,File: %s,Line: %d\n",check,*bpbuf,__FILE__,__LINE__);
				#endif
				if ((check != *bpbuf) || (0x7E != *(bpbuf+1)))
				{
				#ifdef COMMON_DEBUG 
					printf("Not fit data,File: %s, Line: %d\n",__FILE__,__LINE__);
				#endif
					memset(edl,0,sizeof(EvaDataList_t));
				}
			}//evaluation data report actively	
		#endif
			else
			{
			#ifdef COMMON_DEBUG
				printf("Not have fit data,object:0x%02x,File: %s,Line: %d\n",object,__FILE__,__LINE__);
			#endif
			}
			//get data			
		}//if (0x7E == *pbuf)
		else
		{
			j += 1;
			pbuf += 1;
			continue;
		}	
	}//1
	//replace end;
	
	return SUCCESS;
}

// 求弧度
double radian(double d)
{
    return d * PIE / 180.0;   //角度1˚ = π / 180
}

double get_distance(double lat1, double lng1, double lat2, double lng2)
{
    double radLat1 = radian(lat1);
    double radLat2 = radian(lat2);
    double a = radLat1 - radLat2;
    double b = radian(lng1) - radian(lng2);

    double dst = 2 * asin((sqrt(pow(sin(a / 2), 2) + cos(radLat1) * cos(radLat2) * pow(sin(b / 2), 2) )));

    dst = dst * EARTH_RADIUS;
    dst= round(dst * 10000) / 10000;
    return dst;
}

int bring_number_info(unsigned char *dest,unsigned char *source)
{
    if ((NULL == dest) || (NULL == source))
    {
	#ifdef COMMON_DEBUG
        printf("pointer addree is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
        return MEMERR;
    }
    unsigned char       *sour = source;

    while (1)
    {
        if (('\0' == *sour) || ('\n' == *sour))
            break;
        if ((('0' <= *sour) && (*sour <= '9')) || ('.' == *sour))
        {
            *dest = *sour;
            dest++;
        }
        sour++;
    }

    return SUCCESS;
}

int get_net_address(unsigned char *ip)
{
	if (NULL == ip)
	{
	#ifdef COMMON_DEBUG
		printf("Pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

    FILE                    *ipfp = NULL;
    unsigned char           msg[64] = {0};
	int						pos = 0;
    unsigned char           ipmark = 0;

	ipfp = fopen(IPFILE,"rb");
	if (NULL == ipfp)
	{
		printf("open file err,File: %s,Line: %d\n",__FILE__,__LINE__);
		return FAIL;
	}

	while (1)
	{
    	memset(msg,0,sizeof(msg));
    	if (NULL == fgets(msg,sizeof(msg),ipfp))
        	break;
		/*
        if (0 == find_out_child_str(msg,"DefaultGateway",&pos))
        {
			bring_number_info(gw,msg);
            continue;
        }*/
        if (0 == ipmark)
        {//have two "IPAddress" in userinfo.txt,we need first "IPAddress"; 
        	if (0 == find_out_child_str(msg,"IPAddress",&pos))
            {
				bring_number_info(ip,msg);
                ipmark = 1;
				break;
              //  continue;
            }
        }
		/*
        if (0 == find_out_child_str(msg,"SubnetMask",&pos))
        {
			bring_number_info(netmask,msg);
            continue;
        }*/
    }//while loop

	fclose(ipfp);

	return SUCCESS;
}


int send_ip_address(int *sock)
{
	if (NULL == sock)
	{
	#ifdef COMMON_DEBUG
		printf("sock address is null,File:%s,Line:%d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char       ip[16] = {0};
	unsigned char       str1[4] = {0};
    unsigned char       str2[4] = {0};
    unsigned char       str3[4] = {0};
    unsigned char       str4[4] = {0};
	int					num1 = 0;
	int                 num2 = 0;
	int                 num3 = 0;
	int                 num4 = 0;
	unsigned char		buf[10] = {0};
	unsigned char		*pbuf = buf;
	unsigned char		temp = 0;
	
	if (SUCCESS != get_net_address(ip))
	{
	#ifdef COMMON_DEBUG
		printf("get net address err,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return FAIL;
	}
	if (SUCCESS != get_network_section(ip,str1,str2,str3,str4))
	{
	#ifdef COMMON_DEBUG
		printf("get network section err,File: %s,LIne:%d\n",__FILE__,__LINE__);
	#endif
		return FAIL;
	}
	string_to_digit(str1,&num1);
	string_to_digit(str2,&num2);
	string_to_digit(str3,&num3);
	string_to_digit(str4,&num4);
	#ifdef COMMON_DEBUG
	printf("*************ip address,num1: %d,num2: %d,num3: %d,num4: %d\n",num1,num2,num3,num4);
	#endif
	*pbuf = 0x21;
	pbuf++;
	*pbuf = 0x83;
	pbuf++;
	*pbuf = 0xF0;
	pbuf++;
	*pbuf = 0x00;
	pbuf++;
	
	temp = 0;
	temp = (unsigned char)num1;
	*pbuf = temp;
	pbuf++;
	temp = 0;
	temp = (unsigned char)num2;
	*pbuf = temp;
	pbuf++;
	temp = 0;
	temp = (unsigned char)num3;
	*pbuf = temp;
	pbuf++;
	temp = 0;
	temp = (unsigned char)num4;
	*pbuf = temp;
	pbuf++;	

	write(*sock,buf,sizeof(buf));

	return SUCCESS;
}

int gaode_v2x_data_encode(unsigned char	*buf,SignalLightGroup_t *slg,tscdata_t *td,unsigned short *totaln,unsigned short *cyct,unsigned char *contmode)
{
	if ((NULL == buf) || (NULL == slg) || (NULL == td) || (NULL == totaln) || (NULL == cyct) || (NULL == contmode))
	{
	#ifdef COMMON_DEBUG
		printf("********************Memory address err,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	unsigned char					*pbuf = buf;
	unsigned short					num = 0;
	unsigned char					i = 0,j = 0;

	*pbuf = 0x21;
	pbuf++;
	*pbuf = 0x83;
	pbuf++;
	*pbuf = 0xF5;
	pbuf++;

	for (i = 0; i < MAX_CHANNEL; i++)
	{
		if (slg[i].slgid > 0)
            j += 1;
	}
	num = j * 7 + 6;
	*totaln = num + 5;

	*pbuf = num & 0x00ff;
	pbuf++;
	*pbuf = ((num & 0xff00) >> 8) & 0x00ff;
	pbuf++;

	*pbuf = (((td->unit->BrightStart) & 0x00ff0000) >> 16) & 0x000000ff;
    pbuf++;
    *pbuf = (((td->unit->BrightStart) & 0xff000000) >> 24) & 0x000000ff;
    pbuf++;	

	*pbuf = *contmode;
	pbuf++;

	*pbuf = *cyct & 0x00ff;
	pbuf++;
	*pbuf = ((*cyct & 0xff00) >> 8) & 0x00ff;
	pbuf++;

	*pbuf = j;
	pbuf++;

	for (i = 0; i < MAX_CHANNEL; i++)
    {
        if ((slg[i].slgid) > 0)
        {
            *pbuf = slg[i].slgid;
            pbuf++;
            *pbuf = slg[i].slgstatus;
            pbuf++;

			if (0xff == slg[i].countdown)
			{
				*pbuf = 0;//slg[i].countdown;
            	pbuf++;
			}
			else
			{
				*pbuf = slg[i].countdown;
				pbuf++;
			}

            *pbuf = slg[i].greent;
            pbuf++;
			*pbuf = slg[i].yellowt;
			pbuf++;
			*pbuf = slg[i].redt;
			pbuf++;
			*pbuf = slg[i].allrt;
			pbuf++;
        }
    }	
	
	return SUCCESS;
}

int v2x_data_encode(unsigned char *buf,SignalLightGroup_t *slg,tscdata_t *td,struct timeval *nowt,unsigned short *totaln)
{
	if ((NULL == buf) || (NULL == slg) || (NULL == td) || (NULL == nowt) || (NULL == totaln)) 
	{
	#ifdef COMMON_DEBUG
		printf("**************Memory address err,File:%s,Line:%d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}

	unsigned char				*pbuf = buf;
	unsigned short				num = 0;
	unsigned char				i = 0,j = 0;

	*pbuf = 0x21;
	pbuf++;
	*pbuf = 0x83;
	pbuf++;
	*pbuf = 0xF5;
	pbuf++;

	for (i = 0; i < MAX_CHANNEL; i++)
	{
		if (slg[i].slgid > 0)
			j += 1;
	}
	num = j * 4 + 16;
	*totaln = num + 5;

	*pbuf = num & 0x00ff;
	pbuf++;
	*pbuf = ((num & 0xff00) >> 8) & 0x00ff;
	pbuf++;

	//temp
	*pbuf = (td->unit->BrightStart) & 0x000000ff;
	pbuf++;
	*pbuf = (((td->unit->BrightStart) & 0x0000ff00) >> 8) & 0x000000ff;
	pbuf++;

	*pbuf = (td->unit->BrightStop) & 0x000000ff;
	pbuf++;
	*pbuf = (((td->unit->BrightStop) & 0x0000ff00) >> 8) & 0x000000ff;
	pbuf++;
	*pbuf = (((td->unit->BrightStop) & 0x00ff0000) >> 16) & 0x000000ff;
	pbuf++;
	*pbuf = (((td->unit->BrightStop) & 0xff000000) >> 24) & 0x000000ff;
	pbuf++;
	
	*pbuf = (((td->unit->BrightStart) & 0x00ff0000) >> 16) & 0x000000ff;
	pbuf++;
	*pbuf = (((td->unit->BrightStart) & 0xff000000) >> 24) & 0x000000ff;
	pbuf++;
	//temp

	*pbuf = 0x00;
	pbuf++;

//	nowt->tv_sec = nowt->tv_sec - 28800;//8*3600 = 28800S
	*pbuf = nowt->tv_sec & 0x000000ff;
	pbuf++;
	*pbuf = ((nowt->tv_sec & 0x0000ff00) >> 8) & 0x000000ff;
	pbuf++;
	*pbuf = ((nowt->tv_sec & 0x00ff0000) >> 16) & 0x000000ff;
	pbuf++;
	*pbuf = ((nowt->tv_sec & 0xff000000) >> 24) & 0x000000ff;
	pbuf++;

	*pbuf = ((nowt->tv_usec)/1000) & 0x00ff;
	pbuf++;
	*pbuf = ((((nowt->tv_usec)/1000) & 0xff00) >> 8) & 0x00ff;
	pbuf++;

 	*pbuf = j;
	pbuf++;	

	for (i = 0; i < MAX_CHANNEL; i++)
	{
		if ((slg[i].slgid) > 0)
		{
			*pbuf = slg[i].slgid;
			pbuf++;
			*pbuf = slg[i].slgstatus;
			pbuf++;
			*pbuf = slg[i].countdown;
			pbuf++;
			*pbuf = slg[i].nslgstatus;
			pbuf++; 
		}	
	}	

	return SUCCESS;
}

int adapt_data_report(unsigned char	*pbuf,AdaptData_t *adaptd)
{
	if ((NULL == pbuf) || (NULL == adaptd))
	{
	#ifdef COMMON_DEBUG
		printf("pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
	#endif
		return MEMERR;
	}
	unsigned char		temp = 0;
	unsigned char		i = 0;

	*pbuf = 0x21;	
	pbuf++;
	*pbuf = 0x83;
	pbuf++;
	*pbuf = 0xFC;
	pbuf++;
	*pbuf = 0x00;
	pbuf++;

	*pbuf = adaptd->pattid;
	pbuf++;
	
	*pbuf = (adaptd->cycle) & 0x00ff;
	pbuf++;

	*pbuf = (((adaptd->cycle) & 0xff00) >> 8) & 0x00ff;
	pbuf++;

	*pbuf = adaptd->contype;
	pbuf++;

	*pbuf = adaptd->stageid;
	pbuf++;

	*pbuf = (adaptd->time) & 0x000000ff;
	pbuf++;

	*pbuf = (((adaptd->time) & 0x0000ff00) >> 8) & 0x000000ff;
	pbuf++;

	*pbuf = (((adaptd->time) & 0x00ff0000) >> 16) & 0x000000ff;
	pbuf++;

	*pbuf = (((adaptd->time) & 0xff000000) >> 24) & 0x000000ff;
	pbuf++;

	*pbuf = adaptd->phasenum;
	pbuf++;

	for (i = 0; i < (adaptd->phasenum); i++)
	{



		*pbuf = ( adaptd->phi[i].phasestarttime) & 0x000000ff;
		pbuf++;

		*pbuf = ((( adaptd->phi[i].phasestarttime) & 0x0000ff00) >> 8) & 0x000000ff;
		pbuf++;

		*pbuf = ((( adaptd->phi[i].phasestarttime) & 0x00ff0000) >> 16) & 0x000000ff;
		pbuf++;

		*pbuf = ((( adaptd->phi[i].phasestarttime) & 0xff000000) >> 24) & 0x000000ff;
		pbuf++;


		
		*pbuf = ( adaptd->phi[i].phaseendtime) & 0x000000ff;
		pbuf++;

		*pbuf = (((adaptd->phi[i].phaseendtime) & 0x0000ff00) >> 8) & 0x000000ff;
		pbuf++;

		*pbuf = (((adaptd->phi[i].phaseendtime) & 0x00ff0000) >> 16) & 0x000000ff;
		pbuf++;

		*pbuf = (((adaptd->phi[i].phaseendtime) & 0xff000000) >> 24) & 0x000000ff;
		pbuf++;
			
	
	
		*pbuf = adaptd->phi[i].pid;
		pbuf++;
		*pbuf = adaptd->phi[i].pming;
		pbuf++;
		*pbuf = adaptd->phi[i].pmaxg;
		pbuf++;
		*pbuf = adaptd->phi[i].pfactt;
		pbuf++;
		*pbuf = adaptd->phi[i].paddt;
		pbuf++;
	}

	return SUCCESS;
}


int check_single_fangan(tscdata_t *tscdata)
	{
	//配置方案规范检查
	if(NULL == tscdata)
	{
		return MEMERR;
	}
	unsigned char z=0,i1=0,j1=0;					

	if(tscdata->unit->StartUpFlash < 10)
	{
		return 1;//启动闪光时间至少为10秒；							
	}
	if(tscdata->unit->StartUpRed < 5)
	{
		return 2;//全红状态时间至少为5秒

	}
	//unsigned char data[100] = {0};
	
	for(j1 =0 ;j1 <  tscdata->timeconfig->FactTimeConfigNum;j1++)
	{
		for(z = 0; z <tscdata->timeconfig->FactStageNum;z++)
		{
			unsigned char phaseid = 0;
			get_phase_id(tscdata->timeconfig->TimeConfigList[j1][z].PhaseId,&phaseid);
			//sprintf(data,"StageId:%02x PhaseId:%02x TimeConfigId:%02x",tscdata->timeconfig->TimeConfigList[j1][z].StageId,tscdata->timeconfig->TimeConfigList[j1][z].PhaseId,tscdata->timeconfig->TimeConfigList[j1][z].TimeConfigId);
			//output_log(data);
			if(tscdata->timeconfig->TimeConfigList[j1][z].StageId == 0)
			{
				break;
			}
			if(tscdata->timeconfig->TimeConfigList[j1][z].YellowTime < 3)
			{
				return 3;//阶段黄灯时间至少为3秒														
			}
			if(tscdata->timeconfig->TimeConfigList[j1][z].YellowTime > tscdata->timeconfig->TimeConfigList[j1][z].GreenTime)
			{
				return 4;//阶段黄灯时间不能大于阶段绿灯时间
				
			}
			for(i1 = 0;i1 < tscdata->phase->FactPhaseNum;i1++)
			{
				if(0 == tscdata->phase->PhaseList[i1].PhaseId)
				{
					break;
				}
				if(phaseid == tscdata->phase->PhaseList[i1].PhaseId)
				{
					if(tscdata->timeconfig->TimeConfigList[j1][z].GreenTime < tscdata->phase->PhaseList[i1].PhaseMinGreen )
					{
						return 5;//最小绿灯时间不能大于阶段绿灯时间
					}
					if((tscdata->phase->PhaseList[i1].PhaseWalkGreen + tscdata->phase->PhaseList[i1].PhaseWalkClear) > tscdata->timeconfig->TimeConfigList[j1][z].GreenTime)
					{
						return 6;//行人绿灯时间+行人清空时间不能大于阶段绿灯时间
					}
				}
			}
		}
	}

	for(i1 = 0;i1<tscdata->phase->FactPhaseNum;i1++)
	{
		//memset(data,0,sizeof(data));
		//sprintf(data,"PhaseId:%02x PhaseType:%02x TimeCoPhaseMinGreennfigId:%d",tscdata->phase->PhaseList[i1].PhaseId,tscdata->phase->PhaseList[i1].PhaseType,tscdata->phase->PhaseList[i1].PhaseMinGreen);
		//output_log(data);
		if(tscdata->phase->PhaseList[i1].PhaseId == 0)
		{
			break;
		}
		if(tscdata->phase->PhaseList[i1].PhaseMinGreen > tscdata->phase->PhaseList[i1].PhaseMaxGreen1)
		{
			return 7;// 最小绿灯时间不能大于最大绿灯时间1
			
			
		}
		if(tscdata->phase->PhaseList[i1].PhaseGreenDelay > tscdata->phase->PhaseList[i1].PhaseMinGreen)
		{
			return 8;//绿灯延迟时间小于最小绿灯时间
			
		}
		
		if((tscdata->phase->PhaseList[i1].PhaseType == 0X80) && (tscdata->phase->PhaseList[i1].PhaseFixGreen <= 0))
		{
			return 9;//如果相位类型为固定相位，固定相位时间必须大于0; 
				//如果固定相位中包含行人通道，行人清空   时间必须大于0，且小于固定相位时间
			
		}
		if(tscdata->phase->PhaseList[i1].PhaseType == 0X04)
		{
			if(!((tscdata->phase->PhaseList[i1].PhaseWalkClear) > 0 && (tscdata->phase->PhaseList[i1].PhaseWalkClear < tscdata->phase->PhaseList[i1].PhaseWalkGreen)))
			{
				return 10;//如果相位类型为行人相位，行人清空时间必须大于0且小于行人绿灯时间
			
			}
		}
		unsigned char phaseid = 0;
		phaseid = tscdata->phase->PhaseList[i1].PhaseId;
		for(j1 = 0; j1 < tscdata->channel->FactChannelNum; j1++)
		{
			if(tscdata->channel->ChannelList[j1].ChannelId == 0)
			{
				break;
			}
			if(tscdata->channel->ChannelList[j1].ChannelCtrlSrc == phaseid)
			{
				if((tscdata->phase->PhaseList[i1].PhaseType == 0X80) && ((tscdata->channel->ChannelList[j1].ChannelId == 0x0d) || \
					(tscdata->channel->ChannelList[j1].ChannelId == 0x0e) || ((tscdata->channel->ChannelList[j1].ChannelId == 0x0f) || ((tscdata->channel->ChannelList[j1].ChannelId == 0x10)))
					))
				{
					if(!((tscdata->phase->PhaseList[i1].PhaseWalkClear > 0) && (tscdata->phase->PhaseList[i1].PhaseWalkClear < tscdata->phase->PhaseList[i1].PhaseFixGreen)))
					{
						return 9;//如果相位类型为固定相位，固定相位时间必须大于0; 
							//如果固定相位中包含行人通道，行人清空   时间必须大于0，且小于固定相位时间		
					
					}
				}
				if((tscdata->channel->ChannelList[j1].ChannelId == 0x0d) || (tscdata->channel->ChannelList[j1].ChannelId == 0x0e) || (tscdata->channel->ChannelList[j1].ChannelId == 0x0f) || (tscdata->channel->ChannelList[j1].ChannelId == 0x10))
				{
					if(!((tscdata->phase->PhaseList[i1].PhaseWalkClear > 0) && (tscdata->phase->PhaseList[i1].PhaseWalkClear < tscdata->phase->PhaseList[i1].PhaseMinGreen)))
					{
						return 11;// 如果相位中包含行人通道，行人清空时间必须大于0,且行人清空时间必须小于最小绿时间
						
					}
				}
			}
		}
		if(tscdata->phase->PhaseList[i1].PhaseType == 0X40)
		{
			if(i1 == 0)
			{
				continue;
			}
			else
			{
				unsigned char lastphaseid = 0;
				lastphaseid = tscdata->phase->PhaseList[i1-1].PhaseId;
				unsigned char currentchannel[MAX_CHANNEL_LINE] = {0};
				unsigned char lastchannel[MAX_CHANNEL_LINE] = {0};
				unsigned char k=0,q=0,w=0;
				z=0;
				for(j1 = 0; j1 < tscdata->channel->FactChannelNum; j1++)
				{
					if(phaseid == tscdata->channel->ChannelList[j1].ChannelCtrlSrc)
					{
						currentchannel[z] = tscdata->channel->ChannelList[j1].ChannelId;
						z++;
					}
					if(lastphaseid == tscdata->channel->ChannelList[j1].ChannelCtrlSrc)
					{
						lastchannel[k] = tscdata->channel->ChannelList[j1].ChannelId;
						k++;
					}						
				}
				for(q = 0; q < z;q++)
				{
					for(w =0;w<k;w++)
					{
						if(currentchannel[q] == lastchannel[w])
						{
							return 12;//待定相位不能与上一个相位共用通道
					
						}
					}										
				}
			}
		}
	}
	return SUCCESS;							
}

int sendfaceInfoToBoard(fcdata_t *fd,unsigned char *fbdata)
{
	if((NULL == fd) || (NULL==fbdata))
	{
		#ifdef COMMON_DEBUG
		printf("pointer address is null,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		return MEMERR;
	}
	struct timeval				tel;
	//20230412 面板倒计时发送给底板
	if (!wait_write_serial(*(fd->bbserial)))
	{
		if (write(*(fd->bbserial),fbdata,6) < 0)
		{
			#ifdef TIMING_DEBUG
			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
			*(fd->markbit) |= 0x0800;
			gettimeofday(&tel,NULL);
        	update_event_list(fd->ec,fd->el,1,16,tel.tv_sec,fd->markbit);
        	if (SUCCESS != generate_event_file(fd->ec,fd->el, \
												fd->softevent,fd->markbit))
        	{
				#ifdef TIMING_DEBUG
            	printf("gen file err,File: %s,Line: %d\n",__FILE__,__LINE__);
				#endif
        	}
		}
	}
	else
	{
		#ifdef TIMING_DEBUG
		printf("face board serial cannot write,File:%s,Line:%d\n",__FILE__,__LINE__);
		#endif
	}
	return SUCCESS;
}


int phase_err_report(unsigned char *buf,unsigned int commt,unsigned char phaseid,unsigned char *adetectid)
{
	if ((NULL == buf) || (NULL == adetectid))
	{
		#ifdef COMMON_DEBUG
		printf("Pointer address is NULL,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
		return MEMERR;
	}

	unsigned char			*pbuf = buf,i = 0;
	unsigned int			tint = 0;
	
	*pbuf = 0x21;
	pbuf++;
	*pbuf = 0x83;
	pbuf++;
	*pbuf = 0xF4;
	pbuf++;
	*pbuf = 0x00;
	pbuf++;

	tint = commt;
	tint &= 0xff000000;
	tint >>= 24;
	tint &= 0x000000ff;
	*pbuf |= tint;
	pbuf++;
	tint = commt;
	tint &= 0x00ff0000;
	tint >>= 16;
	tint &= 0x000000ff;
	*pbuf |= tint;
	pbuf++;
	tint = commt;
	tint &= 0x0000ff00;
	tint >>= 8;
	tint &= 0x000000ff;
	*pbuf |= tint;
	pbuf++;
	tint = commt;
	tint &= 0x000000ff;
	*pbuf |= tint;
	pbuf++;

	*pbuf = 0x20;
	pbuf++;
	
	*pbuf = phaseid;
	pbuf++;

	for(i = 0;i<7;i++)
	{
		*pbuf = adetectid[i];
		pbuf++;
	}		
	return SUCCESS;
}

