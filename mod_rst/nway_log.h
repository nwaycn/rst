#ifndef NWAY_FS_LOG_H
#define NWAY_FS_LOG_H
#include <switch.h>
#define log_err(fmt, args...) switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, fmt,##args)
#define log_dbg(fmt, args...) switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, fmt,##args)
#define log_ntc(fmt, args...) switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, fmt,##args)

#endif