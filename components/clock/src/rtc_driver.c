#include "rtc_driver.h"
#include "clock_service.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "RTC_DRIVER";
static i2c_port_t i2c_port = I2C_NUM_0;
static bool rtc_initialized = false;

// Helper functions for BCD conversion
uint8_t bcd_to_dec(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

uint8_t dec_to_bcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

// I2C helper functions
static esp_err_t rtc_write_reg(uint8_t reg, uint8_t data) {
    uint8_t write_buf[2] = {reg, data};
    return i2c_master_write_to_device(i2c_port, DS3231_I2C_ADDR, write_buf, 2, pdMS_TO_TICKS(100));
}

static esp_err_t rtc_read_reg(uint8_t reg, uint8_t* data) {
    return i2c_master_write_read_device(i2c_port, DS3231_I2C_ADDR, &reg, 1, data, 1, pdMS_TO_TICKS(100));
}

static esp_err_t rtc_read_regs(uint8_t reg, uint8_t* data, size_t len) {
    return i2c_master_write_read_device(i2c_port, DS3231_I2C_ADDR, &reg, 1, data, len, pdMS_TO_TICKS(100));
}

esp_err_t rtc_driver_init(void) {
    ESP_LOGI(TAG, "Initializing DS3231 RTC driver");
    
    // Configure I2C master
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = CLOCK_SERVICE_SDA_GPIO,
        .scl_io_num = CLOCK_SERVICE_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,  // 100kHz
    };
    
    esp_err_t ret = i2c_param_config(i2c_port, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = i2c_driver_install(i2c_port, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Test DS3231 communication
    uint8_t test_data;
    ret = rtc_read_reg(DS3231_REG_CONTROL, &test_data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "DS3231 communication test failed: %s", esp_err_to_name(ret));
        i2c_driver_delete(i2c_port);
        return ret;
    }
    
    // Clear oscillator stop flag if set
    uint8_t status_reg;
    ret = rtc_read_reg(DS3231_REG_STATUS, &status_reg);
    if (ret == ESP_OK && (status_reg & DS3231_STAT_OSF)) {
        ESP_LOGW(TAG, "Oscillator stop flag detected, clearing...");
        status_reg &= ~DS3231_STAT_OSF;
        rtc_write_reg(DS3231_REG_STATUS, status_reg);
    }
    
    // Configure control register for alarm interrupt operation
    uint8_t control_reg = DS3231_CTRL_INTCN;  // Enable interrupt output (active low)
    ret = rtc_write_reg(DS3231_REG_CONTROL, control_reg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure control register: %s", esp_err_to_name(ret));
        i2c_driver_delete(i2c_port);
        return ret;
    }
    
    // Clear any existing alarm flags
    ret = rtc_read_reg(DS3231_REG_STATUS, &status_reg);
    if (ret == ESP_OK) {
        status_reg &= ~(DS3231_STAT_A1F | DS3231_STAT_A2F);  // Clear alarm flags
        rtc_write_reg(DS3231_REG_STATUS, status_reg);
    }
    
    rtc_initialized = true;
    ESP_LOGI(TAG, "DS3231 RTC driver initialized successfully");
    return ESP_OK;
}

void rtc_driver_deinit(void) {
    if (rtc_initialized) {
        i2c_driver_delete(i2c_port);
        rtc_initialized = false;
        ESP_LOGI(TAG, "RTC driver deinitialized");
    }
}

esp_err_t rtc_driver_read_time(struct tm* time_info) {
    if (!rtc_initialized || !time_info) {
        return ESP_ERR_INVALID_STATE;
    }
    
    uint8_t time_regs[7];
    esp_err_t ret = rtc_read_regs(DS3231_REG_SECONDS, time_regs, 7);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read time registers: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Convert BCD to decimal and fill tm structure
    memset(time_info, 0, sizeof(struct tm));
    time_info->tm_sec = bcd_to_dec(time_regs[0] & 0x7F);        // Seconds (0-59)
    time_info->tm_min = bcd_to_dec(time_regs[1] & 0x7F);        // Minutes (0-59)
    time_info->tm_hour = bcd_to_dec(time_regs[2] & 0x3F);       // Hours (0-23, assuming 24-hour format)
    time_info->tm_wday = bcd_to_dec(time_regs[3] & 0x07) - 1;   // Day of week (0-6, Sunday=0)
    time_info->tm_mday = bcd_to_dec(time_regs[4] & 0x3F);       // Day of month (1-31)
    time_info->tm_mon = bcd_to_dec(time_regs[5] & 0x1F) - 1;    // Month (0-11)
    time_info->tm_year = bcd_to_dec(time_regs[6]) + 100;        // Year since 1900 (DS3231 starts from 2000)
    
    // Validate ranges
    if (time_info->tm_sec > 59 || time_info->tm_min > 59 || time_info->tm_hour > 23 ||
        time_info->tm_wday > 6 || time_info->tm_mday > 31 || time_info->tm_mon > 11) {
        ESP_LOGE(TAG, "Invalid time data read from DS3231");
        return ESP_ERR_INVALID_RESPONSE;
    }
    
    return ESP_OK;
}

esp_err_t rtc_driver_write_time(struct tm* time_info) {
    if (!rtc_initialized || !time_info) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Validate input ranges
    if (time_info->tm_sec > 59 || time_info->tm_min > 59 || time_info->tm_hour > 23 ||
        time_info->tm_wday > 6 || time_info->tm_mday > 31 || time_info->tm_mon > 11 ||
        time_info->tm_year < 100) {
        ESP_LOGE(TAG, "Invalid time data provided");
        return ESP_ERR_INVALID_ARG;
    }
    
    uint8_t time_regs[8];
    time_regs[0] = DS3231_REG_SECONDS;
    time_regs[1] = dec_to_bcd(time_info->tm_sec);                    // Seconds
    time_regs[2] = dec_to_bcd(time_info->tm_min);                    // Minutes
    time_regs[3] = dec_to_bcd(time_info->tm_hour);                   // Hours (24-hour format)
    time_regs[4] = dec_to_bcd(time_info->tm_wday + 1);               // Day of week (1-7)
    time_regs[5] = dec_to_bcd(time_info->tm_mday);                   // Day of month
    time_regs[6] = dec_to_bcd(time_info->tm_mon + 1);                // Month (1-12)
    time_regs[7] = dec_to_bcd(time_info->tm_year - 100);             // Year (0-99 for 2000-2099)
    
    esp_err_t ret = i2c_master_write_to_device(i2c_port, DS3231_I2C_ADDR, time_regs, 8, pdMS_TO_TICKS(100));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write time: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Time set: %04d-%02d-%02d %02d:%02d:%02d", 
             time_info->tm_year + 1900, time_info->tm_mon + 1, time_info->tm_mday,
             time_info->tm_hour, time_info->tm_min, time_info->tm_sec);
    
    return ESP_OK;
}

bool rtc_driver_is_running(void) {
    if (!rtc_initialized) {
        return false;
    }
    
    uint8_t status_reg;
    esp_err_t ret = rtc_read_reg(DS3231_REG_STATUS, &status_reg);
    if (ret != ESP_OK) {
        return false;
    }
    
    // Check if oscillator stop flag is clear
    return !(status_reg & DS3231_STAT_OSF);
}

esp_err_t rtc_driver_set_alarm(alarm_config_t* alarm) {
    if (!rtc_initialized || !alarm || alarm->alarm_num < 1 || alarm->alarm_num > 2) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (alarm->alarm_num == 1) {
        // Alarm 1: seconds, minutes, hours, day/date
        uint8_t alarm_regs[4];
        alarm_regs[0] = dec_to_bcd(alarm->second);
        alarm_regs[1] = dec_to_bcd(alarm->minute);
        alarm_regs[2] = dec_to_bcd(alarm->hour);
        alarm_regs[3] = 0x80;  // Disable day/date match
        
        for (int i = 0; i < 4; i++) {
            esp_err_t ret = rtc_write_reg(DS3231_REG_ALARM1_SEC + i, alarm_regs[i]);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to set alarm 1 register %d: %s", i, esp_err_to_name(ret));
                return ret;
            }
        }
    } else {
        // Alarm 2: minutes, hours, day/date (no seconds)
        uint8_t alarm_regs[3];
        alarm_regs[0] = dec_to_bcd(alarm->minute);
        alarm_regs[1] = dec_to_bcd(alarm->hour);
        alarm_regs[2] = 0x80;  // Disable day/date match
        
        for (int i = 0; i < 3; i++) {
            esp_err_t ret = rtc_write_reg(DS3231_REG_ALARM2_MIN + i, alarm_regs[i]);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to set alarm 2 register %d: %s", i, esp_err_to_name(ret));
                return ret;
            }
        }
    }
    
    ESP_LOGI(TAG, "Alarm %d set for %02d:%02d:%02d", alarm->alarm_num, 
             alarm->hour, alarm->minute, alarm->second);
    return ESP_OK;
}

esp_err_t rtc_driver_enable_alarm(uint8_t alarm_num, bool enable) {
    if (!rtc_initialized || alarm_num < 1 || alarm_num > 2) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint8_t control_reg;
    esp_err_t ret = rtc_read_reg(DS3231_REG_CONTROL, &control_reg);
    if (ret != ESP_OK) {
        return ret;
    }
    
    uint8_t alarm_bit = (alarm_num == 1) ? DS3231_CTRL_A1IE : DS3231_CTRL_A2IE;
    
    if (enable) {
        control_reg |= alarm_bit;
    } else {
        control_reg &= ~alarm_bit;
    }
    
    ret = rtc_write_reg(DS3231_REG_CONTROL, control_reg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to %s alarm %d: %s", enable ? "enable" : "disable", 
                 alarm_num, esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Alarm %d %s", alarm_num, enable ? "enabled" : "disabled");
    return ESP_OK;
}

bool rtc_driver_is_alarm_triggered(uint8_t alarm_num) {
    if (!rtc_initialized || alarm_num < 1 || alarm_num > 2) {
        return false;
    }
    
    uint8_t status_reg;
    esp_err_t ret = rtc_read_reg(DS3231_REG_STATUS, &status_reg);
    if (ret != ESP_OK) {
        return false;
    }
    
    uint8_t alarm_flag = (alarm_num == 1) ? DS3231_STAT_A1F : DS3231_STAT_A2F;
    return (status_reg & alarm_flag) != 0;
}

esp_err_t rtc_driver_clear_alarm(uint8_t alarm_num) {
    if (!rtc_initialized || alarm_num < 1 || alarm_num > 2) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint8_t status_reg;
    esp_err_t ret = rtc_read_reg(DS3231_REG_STATUS, &status_reg);
    if (ret != ESP_OK) {
        return ret;
    }
    
    uint8_t alarm_flag = (alarm_num == 1) ? DS3231_STAT_A1F : DS3231_STAT_A2F;
    status_reg &= ~alarm_flag;
    
    ret = rtc_write_reg(DS3231_REG_STATUS, status_reg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to clear alarm %d flag: %s", alarm_num, esp_err_to_name(ret));
        return ret;
    }
    
    return ESP_OK;
}

esp_err_t rtc_driver_read_temperature(float* temperature) {
    if (!rtc_initialized || !temperature) {
        return ESP_ERR_INVALID_STATE;
    }
    
    uint8_t temp_regs[2];
    esp_err_t ret = rtc_read_regs(DS3231_REG_TEMP_MSB, temp_regs, 2);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read temperature: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Convert temperature reading
    int16_t temp_raw = (int16_t)((temp_regs[0] << 8) | (temp_regs[1] & 0xC0));
    *temperature = (float)temp_raw / 256.0f;
    
    return ESP_OK;
}