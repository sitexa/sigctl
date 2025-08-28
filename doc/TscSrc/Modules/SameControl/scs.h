/* 
 * File:   scs.h
 * Author: sk
 *
 * Created on 20131111
 */

#ifndef SC_H
#define	SC_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../../Inc/mydef.h"

typedef struct scdata_t
{
	fcdata_t				*fd;
	tscdata_t				*td;
	ChannelStatus_t			*cs;
}scdata_t;

typedef struct scyfdata_t
{
	fcdata_t				*fd;
	ChannelStatus_t			*cs;
}scyfdata_t;

#ifdef	__cplusplus
}
#endif

#endif	/* SC_H */

