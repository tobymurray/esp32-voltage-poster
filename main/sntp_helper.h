#ifndef sntp_helper_h
#define sntp_helper_h

#include <sys/time.h>
#include <stdbool.h>

void obtain_time(time_t* now);
void initialize_sntp(void);
void time_sync_notification_cb(struct timeval *tv);
void set_current_time(time_t* now);
bool time_is_set(time_t now);
bool time_is_stale(time_t now);
void get_time_string(char time[]);

#endif
