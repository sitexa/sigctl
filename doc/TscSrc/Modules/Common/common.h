/* 
 * File:   common.h
 * Author: sk
 *
 * Created on 2013年5月22日, 下午4:33
 */

#ifndef COMMON_H
#define	COMMON_H

#ifdef	__cplusplus
extern "C" {
#endif

void output_log(char *str);
void tsc_save_eventlog(char *pType, char *pInfo);
struct tm *get_system_time(void);
void modify_content_value(char *filename,char *str,int value);
int wait_write_serial(int serial);
int get_phase_ids(unsigned int  Iphaseid,unsigned char *phaseid);
int get_phase_id(unsigned int Iphaseid,unsigned char *phaseid);
int find_out_child_str(const char *source,const char *childstr,int *pos);

int update_lamp_status(lamperrtype_t *let,unsigned char chanid,unsigned char etype, unsigned emark,unsigned short *markbit);

/*ecid: event class id; elv: event log value; time: current time*/
int update_event_list(EventClass_t *ec,EventLog_t *el,unsigned char ecid,unsigned char elv,int time, \
	unsigned short *markbit);
/*chanid: channel id; mark: color of lamp, 3:close,0:red,1:yellow,2:green*/
int update_channel_status(sockfd_t *sockfd,ChannelStatus_t *cs,unsigned char *chanid,unsigned char mark,unsigned short *markbit);
//int all_yellow_flash(int serial,unsigned char second);
int new_yellow_flash(yfdata_t *yfdata);
//int all_red(int serial,unsigned char second);
int new_all_red(yfdata_t *yfdata);
int new_all_close(yfdata_t *acdata);
int timedown_data_to_conftool(sockfd_t *sockfd,timedown_t *timedown);
int timedown_data_to_embed(sockfd_t *sockfd,timedown_t *timedown);
int string_to_digit(unsigned char *str,int *digit);
int digit_to_string(unsigned char digit,unsigned char *str);
int short_to_string(unsigned short digit,unsigned char *str);
int get_network_section(unsigned char *str,unsigned char *st1,unsigned char *st2,unsigned char *st3,unsigned char *st4);
int generate_event_file(EventClass_t *ec,EventLog_t *el,unsigned char **softevent,unsigned short *markbit);
int generate_lamp_err_file(lamperrtype_t *let,unsigned short *markbit);

int get_object_value(tscdata_t *tscdata,unsigned char objectid,unsigned char indexn,unsigned char *index, \
                        unsigned char cobject,unsigned short *objectvs,unsigned char *objectv);
int set_object_value(tscdata_t *tscdata, infotype_t *itype,objectinfo_t *objecti);
int err_data_pack(unsigned char *sbuf,infotype_t *itype,unsigned char errs,unsigned char errindex);

int control_data_pack(unsigned char *sbuf,infotype_t *itype,objectinfo_t *objecti,unsigned short *factovs,unsigned char mark);
int server_data_pack(unsigned char *sbuf,infotype_t *itype,objectinfo_t *objecti,unsigned short *factovs);
//int server_data_parse(infotype_t *itype,objectinfo_t *objecti,unsigned char *extend,unsigned char *sbuf);
int server_data_parse(infotype_t *itype,objectinfo_t *objecti,unsigned char *extend,unsigned int *ct,unsigned char *sbuf);
int fastforward_data_pack(unsigned char *sbuf,infotype_t *itype,objectinfo_t *objecti,unsigned short *factovs,unsigned char mark);
int err_report(unsigned char *buf,unsigned int commt,unsigned char type,unsigned char event);
int status_info_report(unsigned char *buf,statusinfo_t  *statusi);
int detect_err_pack(unsigned char *buf,unsigned char type,unsigned char event);
int detect_info_report(unsigned char *buf,detectinfo_t *di);
int version_id_report(unsigned char *buf,versioninfo_t *vi);
int time_info_report(unsigned char *buf,unsigned int time);

int green_conflict_check(Phase_t *phaselist,Channel_t *channel);

int terminal_protol_pack(unsigned char *buf,terminalpp_t *tpp);
//int terminal_protol_parse(unsigned char *buf,terminalpp_t *tpp);

int wireless_info_report(unsigned char *buf,unsigned int commt,unsigned char *wlmark,unsigned char event);

int voltage_pack(unsigned char *buf,unsigned char volh,unsigned char voll);
int wendu_pack(unsigned char *buf,unsigned char wenduh,unsigned char wendul);
int shidu_pack(unsigned char *buf,unsigned char shiduh,unsigned char shidul);
int zdong_pack(unsigned char *buf,unsigned char zdongh,unsigned char zdongl);
int yanwu_pack(unsigned char *buf,unsigned char yw1,unsigned char yw2);
int shuiwei_pack(unsigned char *buf,unsigned char sw1,unsigned char sw2);
int menkaiguan_pack(unsigned char *buf,unsigned char dw1,unsigned char dw2);
int environment_pack(unsigned char *buf,unsigned char object,unsigned char par1,unsigned char par2);

int green_conflict_pack(unsigned char *buf,unsigned int commt,unsigned int gcbit);

int read_bus_rw_info(bus_pri_t *bpt);

int red_flash_begin_report(int *csockfd,unsigned char *rfchan);
int red_flash_end_report(int *csockfd);

int dunhua_close_lamp(fcdata_t *fd,ChannelStatus_t *cs);
int standard_protocol_encode(standard_data_t *sdt,unsigned char *buf,unsigned short *typen);
//int standard_protocol_discode(standard_data_t *sdt,unsigned char *buf);
int standard_protocol_discode(standard_data_t *sdt,unsigned char *buf,int *sock);
int send_close_lamp_status(int *sock,unsigned char pattern,unsigned char *chan);
int HuiEr_Radar_DisCode(unsigned char *buf,unsigned short bufn,RadarErrList_t *rel,StatisDataList_t *sdl,QueueDataList_t *qdl,TrafficDataList_t *tdl,EvaDataList_t *edl);
int HaiKang_Video_DisCode(unsigned char *buf,unsigned short bufn,RadarErrList_t *rel,StatisDataList_t *sdl,QueueDataList_t *qdl,TrafficDataList_t *tdl,EvaDataList_t *edl);
int channel_control_encode(unsigned char *buf,unsigned char mark);
int send_ip_address(int *sock);
int gps_parse_jingweidu(int gpsn,unsigned char *gps,JingWeiDu_t *jwd);
int v2x_data_encode(unsigned char *buf,SignalLightGroup_t *slg,tscdata_t *td,struct timeval *nowt,unsigned short *totaln);
int gaode_v2x_data_encode(unsigned char *buf,SignalLightGroup_t *slg,tscdata_t *td,unsigned short *totaln,unsigned short *cyct,unsigned char *contmode);
int channel_close_begin_report(int *csockfd,unsigned char *ccchan);
int channel_close_end_report(int *csockfd,unsigned char *ccchan);
int time_info_report_keli(unsigned char *buf,unsigned int time);
int delay_time_report(sockfd_t *sockfd, unsigned char delaytime);
int check_single_fangan(tscdata_t * tscdata);
int adapt_data_report(unsigned char *pbuf,AdaptData_t *adaptd);
int sendfaceInfoToBoard(fcdata_t *fd,unsigned char *fbdata);

#ifdef	__cplusplus
}
#endif

#endif	/* COMMON_H */

