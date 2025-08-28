/* 
 * File:   scs.c
 * Author: sk
 *
 * Created on 201311月11日, 下午3:29
 */

#include "scs.h"

int sc_set_lamp_color(int serial,unsigned char *chan,unsigned char color)
{
	if (NULL == chan)
	{
	#ifdef SAME_DEBUG
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
			#ifdef SAME_DEBUG
				printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
			#endif
				return FAIL;
			}
		}
		else
		{
		#ifdef SAME_DEBUG
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
		#ifdef SAME_DEBUG
			printf("write err,File: %s,Line: %d\n",__FILE__,__LINE__);
		#endif
			return FAIL;
		}
	}
	else
	{
	#ifdef SAME_DEBUG
		printf("serial port cannot be written,File:%s,Line:%d\n",__FILE__,__LINE__);
	#endif
		return FAIL;
	}

	return SUCCESS;
}

/*
 * 
 */

