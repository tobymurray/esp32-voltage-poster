#include <stdint.h>
#include <math.h>

#include "esp_log.h"

#include "ncr18650.h"

static const char *TAG = "ncr18650";

/**
 * This isn't a perfect fit, but hopefully it's good enough. This is a linear fit to measurements
 * taken from https://lygte-info.dk/info/BatteryChargePercent%20UK.html for a NCR18650 3400mAh
 */
static float convert_voltage_to_ncr18650_capacity(float voltage) { return 120.1212 * voltage - 399.2545; }


void get_battery_voltage(uint16_t* raw_measurement) {
    // Convert the unsigned u_int16_t into a signed integer
    int raw_measurement_as_signed_int =
        (0x8000 & *raw_measurement ? (int)(0x7FFF & *raw_measurement) - 0x8000 : *raw_measurement);

    // Undo the effect of the gain
    float millivolts_before_gain = raw_measurement_as_signed_int * 0.1875;

    // Convert to volts
    float voltage = millivolts_before_gain / 1000;

    float voltage_divider_fraction = (100000.0 + 68000.0) / 68000.0;

    ESP_LOGI(TAG, "*******************");
    ESP_LOGI(TAG, "The voltage divider fraction is %0.2f", voltage_divider_fraction);
    ESP_LOGI(TAG, "Raw voltage is %.02f", voltage);

    float absolute_voltage = fabsf(voltage);

    ESP_LOGI(TAG, "Absolute voltage is %.02f", absolute_voltage);

    float actual_voltage;
    float battery_charge_level;
    if (absolute_voltage - 0.01 > 0) {
        actual_voltage = absolute_voltage * voltage_divider_fraction;
        battery_charge_level = convert_voltage_to_ncr18650_capacity(actual_voltage);
    } else {
        actual_voltage = 0;
        battery_charge_level = 0;
    }
    ESP_LOGI(TAG, "Actual voltage is %.02f", actual_voltage);
    ESP_LOGI(TAG, "Battery is currently at %.02f%% charge", battery_charge_level);
    ESP_LOGI(TAG, "*******************");
}
