#include "battery32700.h"
#include <stdint.h>
#include <math.h>

#include "esp_log.h"

static const char *TAG = "32700";

/**
 * This is a terrible fit, likely won't be able to tell battery life without actually figuring something out
 */
static float convert_voltage_to_32700_capacity(float voltage) { 
  if (voltage > 3.65) { return 100.0; }
  else if (voltage > 3.35) { return 97.0; }
  else if (voltage > 3.3) { return 68.0; }
  else if (voltage > 3.29) { return 64.0; }
  else if (voltage > 3.27) { return 60.0; }
  else if (voltage > 3.26) { return 45.0; }
  else if (voltage > 3.25) { return 30.0; }
  else if (voltage > 3.17) { return 8.0; }
  else if (voltage > 3) { return 4.0; }
  else {
    return 0.0;
  }
}


void get_battery_voltage(uint16_t* raw_measurement, float *battery_percentage, float *voltage) {
    // Convert the unsigned u_int16_t into a signed integer
    int raw_measurement_as_signed_int =
        (0x8000 & *raw_measurement ? (int)(0x7FFF & *raw_measurement) - 0x8000 : *raw_measurement);

    // Undo the effect of the gain
    float millivolts_before_gain = raw_measurement_as_signed_int * 0.1875;

    // Convert to volts
    float voltage_before_gain = millivolts_before_gain / 1000;

    ESP_LOGI(TAG, "*******************");
    ESP_LOGI(TAG, "Raw voltage is %.02f", voltage_before_gain);

    float absolute_voltage = fabsf(voltage_before_gain);

    ESP_LOGI(TAG, "Absolute voltage is %.02f", absolute_voltage);

    if (absolute_voltage - 0.01 > 0) {
        *voltage = absolute_voltage;
        *battery_percentage = convert_voltage_to_32700_capacity(*voltage);
    } else {
        *voltage = 0;
        *battery_percentage = 0;
    }
    ESP_LOGI(TAG, "Actual voltage is %.02f", *voltage);
    ESP_LOGI(TAG, "Battery is currently at %.02f%% charge", *battery_percentage);
    ESP_LOGI(TAG, "*******************");
}
