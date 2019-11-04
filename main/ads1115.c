#include "esp_log.h"
#include "driver/i2c.h"

#define _I2C_NUMBER(num) I2C_NUM_##num
#define I2C_NUMBER(num) _I2C_NUMBER(num)

#define I2C_MASTER_NUM I2C_NUMBER(CONFIG_I2C_MASTER_PORT_NUM) /*!< I2C port number for master dev */
#define I2C_MASTER_SDA_IO CONFIG_I2C_MASTER_SDA               /*!< gpio number for I2C master data  */
#define I2C_MASTER_SCL_IO CONFIG_I2C_MASTER_SCL               /*!< gpio number for I2C master clock */
#define I2C_MASTER_FREQ_HZ CONFIG_I2C_MASTER_FREQUENCY        /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */

#define ADS1X15_REG_CONFIG_CQUE_MASK (0x0003)
#define ADS1X15_REG_CONFIG_CQUE_1CONV (0x0000)  // Assert ALERT/RDY after one conversions
#define ADS1X15_REG_CONFIG_CQUE_2CONV (0x0001)  // Assert ALERT/RDY after two conversions
#define ADS1X15_REG_CONFIG_CQUE_4CONV (0x0002)  // Assert ALERT/RDY after four conversions
#define ADS1X15_REG_CONFIG_CQUE_NONE (0x0003)   // Disable the comparator and put ALERT/RDY in high state (default)

#define ADS1X15_REG_CONFIG_CLAT_MASK (0x0004)    // Determines if ALERT/RDY pin latches once asserted
#define ADS1X15_REG_CONFIG_CLAT_NONLAT (0x0000)  // Non-latching comparator (default)
#define ADS1X15_REG_CONFIG_CLAT_LATCH (0x0004)   // Latching comparator

#define ADS1X15_REG_CONFIG_CPOL_MASK (0x0008)
#define ADS1X15_REG_CONFIG_CPOL_ACTVLOW (0x0000)  // ALERT/RDY pin is low when active (default)
#define ADS1X15_REG_CONFIG_CPOL_ACTVHI (0x0008)   // ALERT/RDY pin is high when active

#define ADS1X15_REG_CONFIG_CMODE_MASK (0x0010)
#define ADS1X15_REG_CONFIG_CMODE_TRAD (0x0000)    // Traditional comparator with hysteresis (default)
#define ADS1X15_REG_CONFIG_CMODE_WINDOW (0x0010)  // Window comparator

#define ADS1X15_REG_CONFIG_MODE_MASK (0x0100)
#define ADS1X15_REG_CONFIG_MODE_CONTIN (0x0000)  // Continuous conversion mode
#define ADS1X15_REG_CONFIG_MODE_SINGLE (0x0100)  // Power-down single-shot mode (default)

#define ADS1X15_REG_CONFIG_PGA_MASK (0x0E00)
#define ADS1X15_REG_CONFIG_PGA_6_144V (0x0000)  // +/-6.144V range = Gain 2/3
#define ADS1X15_REG_CONFIG_PGA_4_096V (0x0200)  // +/-4.096V range = Gain 1
#define ADS1X15_REG_CONFIG_PGA_2_048V (0x0400)  // +/-2.048V range = Gain 2 (default)
#define ADS1X15_REG_CONFIG_PGA_1_024V (0x0600)  // +/-1.024V range = Gain 4
#define ADS1X15_REG_CONFIG_PGA_0_512V (0x0800)  // +/-0.512V range = Gain 8
#define ADS1X15_REG_CONFIG_PGA_0_256V (0x0A00)  // +/-0.256V range = Gain 16

#define ADS1X15_REG_CONFIG_MUX_MASK (0x7000)
#define ADS1X15_REG_CONFIG_MUX_DIFF_0_1 (0x0000)  // Differential P = AIN0, N = AIN1 (default)
#define ADS1X15_REG_CONFIG_MUX_DIFF_0_3 (0x1000)  // Differential P = AIN0, N = AIN3
#define ADS1X15_REG_CONFIG_MUX_DIFF_1_3 (0x2000)  // Differential P = AIN1, N = AIN3
#define ADS1X15_REG_CONFIG_MUX_DIFF_2_3 (0x3000)  // Differential P = AIN2, N = AIN3
#define ADS1X15_REG_CONFIG_MUX_SINGLE_0 (0x4000)  // Single-ended AIN0
#define ADS1X15_REG_CONFIG_MUX_SINGLE_1 (0x5000)  // Single-ended AIN1
#define ADS1X15_REG_CONFIG_MUX_SINGLE_2 (0x6000)  // Single-ended AIN2
#define ADS1X15_REG_CONFIG_MUX_SINGLE_3 (0x7000)  // Single-ended AIN3

#define ADS1X15_REG_CONFIG_OS_MASK (0x8000)
#define ADS1X15_REG_CONFIG_OS_SINGLE (0x8000)   // Write: Set to start a single-conversion
#define ADS1X15_REG_CONFIG_OS_BUSY (0x0000)     // Read: Bit = 0 when conversion is in progress
#define ADS1X15_REG_CONFIG_OS_NOTBUSY (0x8000)  // Read: Bit = 1 when device is not performing a conversion

#define ADS1X15_REG_POINTER_MASK (0x03)
#define ADS1X15_REG_POINTER_CONVERT (0x00)
#define ADS1X15_REG_POINTER_CONFIG (0x01)
#define ADS1X15_REG_POINTER_LOWTHRESH (0x02)
#define ADS1X15_REG_POINTER_HITHRESH (0x03)

#define WRITE_BIT I2C_MASTER_WRITE              /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ                /*!< I2C master read */
#define ACK_CHECK_EN 0x1                        /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0                       /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                             /*!< I2C ack value */
#define NACK_VAL 0x1                            /*!< I2C nack value */

#define ADS1115_ADDRESS CONFIG_ADS1115_ADDRESS  /*!< slave address for BH1750 sensor */

#define GPIO_OUTPUT_IO_0 13
#define GPIO_OUTPUT_PIN_SEL (1ULL << GPIO_OUTPUT_IO_0)

static const char *TAG = "ads1115";

esp_err_t i2c_master_init(void) {
    ESP_LOGI(TAG, "Initializing I2C master with SDA on %d and SCL on %d", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

static void enable_gpio(void) {
    gpio_config_t io_conf;
    // disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    // set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    // bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    // disable pull-down mode
    io_conf.pull_down_en = 0;
    // disable pull-up mode
    io_conf.pull_up_en = 0;
    // configure GPIO with the given settings
    gpio_config(&io_conf);

    gpio_set_level(GPIO_OUTPUT_IO_0, 1);
}

static void disable_gpio(void) {
    gpio_set_level(GPIO_OUTPUT_IO_0, 0);
}

esp_err_t read_ads1115(u_int16_t* raw_measurement) {
    enable_gpio();

    // Start with default values
    uint16_t config = ADS1X15_REG_CONFIG_CQUE_NONE |     // Disable the comparator (default val)
                    ADS1X15_REG_CONFIG_CLAT_NONLAT |   // Non-latching (default val)
                    ADS1X15_REG_CONFIG_CPOL_ACTVLOW |  // Alert/Rdy active low   (default val)
                    ADS1X15_REG_CONFIG_CMODE_TRAD |    // Traditional comparator (default val)
                    ADS1X15_REG_CONFIG_MODE_SINGLE;    // Single-shot mode (default)

    // Set PGA/voltage range
    config |= ADS1X15_REG_CONFIG_PGA_6_144V;

    // Set Samples per Second
    config |= 0x0080;  // 128

    // Set channels
    config |= ADS1X15_REG_CONFIG_MUX_DIFF_0_1;  // set P and N inputs for differential

    // Set 'start single-conversion' bit
    config |= ADS1X15_REG_CONFIG_OS_SINGLE;

    ESP_LOGI(TAG, "Read is %d and write is %d", READ_BIT, WRITE_BIT);

    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ADS1115_ADDRESS << 1 | WRITE_BIT, ACK_VAL);
    i2c_master_write_byte(cmd, ADS1X15_REG_POINTER_CONFIG, ACK_VAL);
    i2c_master_write_byte(cmd, (uint8_t)(config >> 8), ACK_VAL);
    i2c_master_write_byte(cmd, (uint8_t)config, ACK_VAL);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    ESP_LOGI(TAG, "Return value from pointing to the config register: %d", ret);

    if (ret != ESP_OK) {
    return ret;
    }

    uint8_t data_h;
    uint8_t data_l;
    vTaskDelay(30 / portTICK_RATE_MS);
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ADS1115_ADDRESS << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, &data_h, ACK_VAL);
    i2c_master_read_byte(cmd, &data_l, ACK_VAL);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    ESP_LOGI(TAG, "Return value from trying to read from the config register: %d", ret);

    if (ret != ESP_OK) {
    return ret;
        }

    vTaskDelay(30 / portTICK_RATE_MS);
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ADS1115_ADDRESS << 1 | WRITE_BIT, ACK_VAL);
    i2c_master_write_byte(cmd, ADS1X15_REG_POINTER_CONVERT, ACK_VAL);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    ESP_LOGI(TAG, "Return value from pointing to the conversion register: %d", ret);

    if (ret != ESP_OK) {
        return ret;
    }

    vTaskDelay(30 / portTICK_RATE_MS);
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ADS1115_ADDRESS << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, &data_h, ACK_VAL);
    i2c_master_read_byte(cmd, &data_l, NACK_VAL);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    ESP_LOGI(TAG, "Return value from trying to read from the conversion register: %d", ret);

    // Convert the two u_int8_t sensor reading into a single u_int16_t
    *raw_measurement = (data_h << 8 | data_l);

    disable_gpio();

    return ret;
}