/* 
 * File:   igc.h
 * Author: root
 *
 * Created on 2022年7月9日, 上午1:25
 */

#ifndef IGC_H
#define	IGC_H

#ifdef	__cplusplus
extern "C" {
#endif

int Inter_Grate_Control(fcdata_t *fcdata, tscdata_t *tscdata,ChannelStatus_t *chanstatus);
void Inter_Control_End_Child_Thread();




#ifdef	__cplusplus
}
#endif

#endif	/* IGC_H */

