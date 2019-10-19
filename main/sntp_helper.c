#include "sntp_helper.h"
#include <sys/time.h>
#include <stdbool.h>
#include "esp_log.h"
#include "esp_sntp.h"


#define SNTP_HOST CONFIG_SNTP_HOST
#define NTP_SYNC_PERIOD_SECONDS CONFIG_NTP_SYNC_PERIOD_SECONDS

static const char *TAG = "sntp_helper";
RTC_DATA_ATTR static time_t last_sntp_sync;

void obtain_time(time_t* current_time) {
  initialize_sntp();

  int retry = 0;
  const int retry_count = 10;
  while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
    ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }

  if (retry == retry_count) {
    ESP_LOGE(TAG, "Failed to sync with NTP server, not updating time");
  } else {
    time(current_time);
    time(&last_sntp_sync);
  }
}

void initialize_sntp(void) {
  int sntp_sync_status = sntp_get_sync_status();
  if (sntp_sync_status == SNTP_SYNC_STATUS_RESET) {
    ESP_LOGI(TAG, "    SNTP_SYNC_STATUS_RESET");
  } else if (sntp_sync_status == SNTP_SYNC_STATUS_COMPLETED) {
    ESP_LOGI(TAG, "    SNTP_SYNC_STATUS_COMPLETED");
  } else if (sntp_sync_status == SNTP_SYNC_STATUS_IN_PROGRESS) {
    ESP_LOGI(TAG, "    SNTP_SYNC_STATUS_IN_PROGRESS");
  }
  ESP_LOGI(TAG, "Initializing SNTP");
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, SNTP_HOST);
  sntp_set_time_sync_notification_cb(time_sync_notification_cb);
  sntp_init();
}

void time_sync_notification_cb(struct timeval *tv) {
  ESP_LOGI(TAG, "Time has been synchronized with NTP!");
}

void set_current_time(time_t* now) {
    // Set timezone to Eastern Standard Time
    setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
    tzset();
    time(now);
}

// Is time set? If not, tm_year will be (1970 - 1900).
bool time_is_set(time_t now) {
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  return timeinfo.tm_year >= (2019 - 1900);
}

bool time_is_stale(time_t now) {
  struct tm timeinfo;
  char strftime_buf[64];
  
  localtime_r(&now, &timeinfo);
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  ESP_LOGI(TAG, "The current date/time is: %s", strftime_buf);

  localtime_r(&last_sntp_sync, &timeinfo);
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);

  double time_since_last_sync = difftime(now, last_sntp_sync);
  ESP_LOGI(TAG, "The last sync with network time was: %s, which was %.2f seconds ago", strftime_buf, time_since_last_sync);

  if (time_since_last_sync < 0) {
    ESP_LOGW(TAG, "Last NTP sync time was in the future, something is wrong so forcing re-sync");
    return true;
  } else if (time_since_last_sync > NTP_SYNC_PERIOD_SECONDS) {
    ESP_LOGI(TAG, "It has been more than %d seconds, time is stale", NTP_SYNC_PERIOD_SECONDS);
    return true;
  }

  return false;
}

void get_time_string(char timestring[]) {
  struct tm timeinfo;
  time_t now;
  time(&now);
  localtime_r(&now, &timeinfo);
  strftime(timestring, 64 * sizeof(timestring[0]), "%c", &timeinfo);
}