#ifndef ncr18650_h
#define ncr18650_h

#include <stdint.h>

float get_battery_voltage(uint16_t* raw_measurement);

#endif