#include "mcp9808.h"

#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "MCP9808";

#define _I2C_NUMBER(num) I2C_NUM_##num
#define I2C_NUMBER(num) _I2C_NUMBER(num)

#define I2C_MASTER_NUM I2C_NUMBER(CONFIG_I2C_MASTER_PORT_NUM) /*!< I2C port number for master dev */

#define READ_BIT I2C_MASTER_READ                /*!< I2C master read */
#define ACK_CHECK_ENABLE 0x1                        /*!< I2C master will check ack from slave*/
#define ACK_VALUE 0x0                             /*!< I2C ack value */
#define NACK_VALUE 0x1                            /*!< I2C nack value */

#define MCP9808_SENSOR_0_ADDRESS CONFIG_MCP9808_ADDRESS_1
#define MCP9808_SENSOR_1_ADDRESS CONFIG_MCP9808_ADDRESS_2

static float convert_temperature_reading_to_temperature(uint8_t *data_h, uint8_t *data_l) {
  //	First Check flag bits
  if ((*data_h & 0x80) == 0x80) {  // TA ³ TCRIT
  }
  if ((*data_h & 0x40) == 0x40) {  // TA > TUPPER
  }
  if ((*data_h & 0x20) == 0x20) {  // TA < TLOWER
  }
  *data_h = *data_h & 0x1F;        // Clear flag bits
  if ((*data_h & 0x10) == 0x10) {  // Is the temperature lower than 0°C
    *data_h = *data_h & 0x0F;      // Clear SIGN
    return 256 - (*data_h * 16 + *data_l / 16);
  } else {
    return *data_h * 16.0 + *data_l / 16.0;
  }
}

static esp_err_t read_temperature_sensor(int sensor_address, i2c_port_t i2c_num, float *temperature) {
  i2c_cmd_handle_t command = i2c_cmd_link_create();
  i2c_master_start(command);
  i2c_master_write_byte(command, sensor_address << 1 & 0xFE, ACK_CHECK_ENABLE);
  i2c_master_write_byte(command, 0x05, ACK_CHECK_ENABLE);
  i2c_master_stop(command);
  int return_value = i2c_master_cmd_begin(i2c_num, command, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(command);

  if (return_value != ESP_OK) {
    ESP_LOGE(TAG, "Return value was %i but expected %i", return_value, ESP_OK);
    return return_value;
  }

  vTaskDelay(30 / portTICK_RATE_MS);
  command = i2c_cmd_link_create();
  i2c_master_start(command);
  i2c_master_write_byte(command, sensor_address << 1 | READ_BIT, ACK_CHECK_ENABLE);

  uint8_t data_h;
  uint8_t data_l;
  i2c_master_read_byte(command, &data_h, ACK_VALUE);
  i2c_master_read_byte(command, &data_l, NACK_VALUE);
  i2c_master_stop(command);

  return_value = i2c_master_cmd_begin(i2c_num, command, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(command);

  *temperature = convert_temperature_reading_to_temperature(&data_h, &data_l);
  ESP_LOGI(TAG, "The converted temperature is: %.2f", *temperature);
  return return_value;
}

void read_temperature(float *temperature0, float *temperature1) {
    ESP_LOGI(TAG, "Reading temperature 0");
    ESP_ERROR_CHECK(read_temperature_sensor(MCP9808_SENSOR_0_ADDRESS, I2C_MASTER_NUM, temperature0));
    ESP_LOGI(TAG, "Reading temperature 1");
    ESP_ERROR_CHECK(read_temperature_sensor(MCP9808_SENSOR_1_ADDRESS, I2C_MASTER_NUM, temperature1));
}