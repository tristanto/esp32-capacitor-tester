/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_adc/adc_continuous.h"
#include "driver/mcpwm_cap.h" 

//ADC configuration parameters
#define ADC_UNIT            ADC_UNIT_1
#define ADC_CHANNEL         ADC_CHANNEL_3     // ADC1_CH3 is physically GPIO 4 in ESP32
#define ADC_ATTEN           ADC_ATTEN_DB_12   // max voltage range  ~3.3V
#define READ_LEN            256


static adc_channel_t channel[2] = {ADC_CHANNEL_6, ADC_CHANNEL_7};
/*TODO: Implement ADC continuous initialization*/
static void adc_continuous_init(adc_continuous_handle_t *handle) {
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = READ_LEN,
        .conv_frame_size = READ_LEN,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, handle));

    adc_continuous_config_t adc_continuous_config = {
        .sample_freq_hz = 1000, // Sample frequency in Hz
        .conv_mode = ADC_CONV_SINGLE_UNIT_1, // Use ADC unit 1
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1, // Output format
    };

    adc_digi_pattern_config_t adc_pattern[2] = {
        {
            .atten = ADC_ATTEN_DB_12,
            .channel = channel[0],
            .unit = ADC_UNIT_1,
            .bit_width = ADC_BITWIDTH_12,
        },
        {
            .atten = ADC_ATTEN_DB_12,
            .channel = channel[1],
            .unit = ADC_UNIT_1,
            .bit_width = ADC_BITWIDTH_12,
        }
    };

    adc_continuous_config.pattern_num = 2;
    adc_continuous_config.adc_pattern = adc_pattern;

}

void task_read_adc(void *pvParameters) {
    while(1) {
        ESP_LOGI("ADC", "Reading ADC value... ");
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}

void app_main(void)
{
    adc_continuous_handle_t adc_handle;
    adc_continuous_init(&adc_handle);
    xTaskCreate(task_read_adc, "ADC Reader", 2048, NULL, 10, NULL);
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Main task can perform other operations or sleep
    }

    
}
