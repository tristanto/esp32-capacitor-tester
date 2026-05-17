/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "sdkconfig.h"

void task_read_adc(void *pvParameters) {
    while(1) {
        ESP_LOGI("ADC", "Reading ADC value... ");
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}

void app_main(void)
{
    xTaskCreate(task_read_adc, "ADC Reader", 2048, NULL, 10, NULL);
    
}
