#ifndef ncr18650_h
#define ncr18650_h

#include <stdint.h>

void get_battery_voltage(uint16_t* raw_measurement, float *battery_percentage, float *voltage);

#endif