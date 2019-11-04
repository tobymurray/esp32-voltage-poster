#include <string.h>
#include <sys/time.h>

#include "esp_attr.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_sntp.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "esp_pm.h"

#include "ads1115.h"
#include "mqtt_helper.h"
#include "battery32700.h"
#include "sntp_helper.h"
#include "wifi_helper.h"
#include "mcp9808.h"

#define DEEP_SLEEP_PERIOD_SECONDS CONFIG_DEEP_SLEEP_PERIOD_SECONDS

static const char *TAG = "app_main";

/* Variable holding number of times ESP32 restarted since first boot.
 * It is placed into RTC memory using RTC_DATA_ATTR and
 * maintains its value when ESP32 wakes from deep sleep.
 */
RTC_DATA_ATTR static int boot_count = 0;

static void initialize(void) {
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Configure dynamic frequency scaling:
  // maximum and minimum frequencies are set in sdkconfig,
  // automatic light sleep is enabled if tickless idle support is enabled.
  esp_pm_config_esp32_t pm_config = {
          .max_freq_mhz = 80,
          .min_freq_mhz = 13,
          .light_sleep_enable = true
  };
  ESP_ERROR_CHECK( esp_pm_configure(&pm_config) );
}

void app_main(void) {
  ++boot_count;
  ESP_LOGI(TAG, "Boot count: %d", boot_count);
  initialize();

  ESP_ERROR_CHECK(i2c_master_init());

  uint16_t raw_measurement;

  ESP_ERROR_CHECK(read_ads1115(&raw_measurement));
  float voltage;
  float battery_percentage;
  get_battery_voltage(&raw_measurement, &battery_percentage, &voltage);

  float temperature_0 = -999.9;
  float temperature_1 = -999.9;
  read_temperature(&temperature_0, &temperature_1);

  initialize_wifi_in_station_mode();
  time_t now;
  set_current_time(&now);

  if (!time_is_set(now) || time_is_stale(now)) {
    ESP_LOGI(TAG, "Time has either not been set or become stale. Connecting to WiFi and syncing time over NTP.");
    wait_for_ip();
    obtain_time(&now);
  }

  wait_for_ip();
  initialize_mqtt();
  wait_for_mqtt_to_connect();

  // Publish battery percentage
  char strftime_buf[64];
  get_time_string(strftime_buf);

  char battery_percentage_as_string[6];
  snprintf(battery_percentage_as_string, 50, "%.1f%%", battery_percentage);

  char voltage_as_string[7];
  snprintf(voltage_as_string, 50, "%.2fV", voltage);

  publish_message(strftime_buf, "esp32/battery", "state_of_charge", battery_percentage_as_string, "voltage", voltage_as_string);

  // Publish temperature
  ESP_LOGI(TAG, "Temperature 0 is %.2f", temperature_0);
  ESP_LOGI(TAG, "Temperature 1 is %.2f", temperature_1);

  char temperature_0_as_string[6];
  char temperature_1_as_string[6];
  snprintf(temperature_0_as_string, 7, "%.2f", temperature_0);
  snprintf(temperature_1_as_string, 7, "%.2f", temperature_1);

  publish_message(strftime_buf, "esp32/temperature", "temperature_0", temperature_0_as_string, "temperature_1", temperature_1_as_string);

  wait_for_all_messages_to_be_published();

  ESP_LOGI(TAG, "Entering deep sleep for %d seconds", DEEP_SLEEP_PERIOD_SECONDS);
  esp_deep_sleep(1000000LL * DEEP_SLEEP_PERIOD_SECONDS);
}
