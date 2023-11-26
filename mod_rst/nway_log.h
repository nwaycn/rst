
/*
auth:上海宁卫信息技术有限公司
License: GPL
Description: this is a module of FreeSWITCH，and it send any udp stream to other udp server
*/

#ifndef NWAY_FS_LOG_H
#define NWAY_FS_LOG_H
#include <switch.h>
#define log_err(fmt, args...) switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, fmt,##args)
#define log_dbg(fmt, args...) switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, fmt,##args)
#define log_ntc(fmt, args...) switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, fmt,##args)

#endif