#ifndef ads1115_h
#define ads1115_h

esp_err_t i2c_master_init(void);
esp_err_t read_ads1115(u_int16_t* raw_measurement);

#endif