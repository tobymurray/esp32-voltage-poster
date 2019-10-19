#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "esp_sntp.h"

#include "sntp_helper.h"
#include "wifi_helper.h"
#include "ads1115.h"
#include "ncr18650.h"

#define DEEP_SLEEP_PERIOD_SECONDS CONFIG_DEEP_SLEEP_PERIOD_SECONDS

static const char *TAG = "example";

/* Variable holding number of times ESP32 restarted since first boot.
 * It is placed into RTC memory using RTC_DATA_ATTR and
 * maintains its value when ESP32 wakes from deep sleep.
 */
RTC_DATA_ATTR static int boot_count = 0;

static void initialize(void) {
  //Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
}

void app_main(void) {
  ++boot_count;
  ESP_LOGI(TAG, "Boot count: %d", boot_count);
  initialize();

  ESP_ERROR_CHECK(i2c_master_init());

  uint16_t raw_measurement;

  ESP_ERROR_CHECK(read_ads1115(&raw_measurement));
  get_battery_voltage(&raw_measurement);

  time_t now;
  set_current_time(&now);

  if (!time_is_set(now) || time_is_stale(now)) {
      ESP_LOGI(TAG, "Time has either not been set or become stale. Connecting to WiFi and syncing time over NTP.");
      initialize_wifi_in_station_mode();
      wait_for_ip();
      obtain_time(&now);
  }
  
  ESP_LOGI(TAG, "Entering deep sleep for %d seconds", DEEP_SLEEP_PERIOD_SECONDS);
  esp_deep_sleep(1000000LL * DEEP_SLEEP_PERIOD_SECONDS);
}
