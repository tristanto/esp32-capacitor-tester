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
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_adc/adc_continuous.h"
#include "driver/mcpwm_cap.h"
#include "driver/mcpwm_sync.h"
#include "driver/uart.h"

static const char *TAG = "CAP_METER_V1.0";
// ADC configuration parameters
#define ADC_UNIT ADC_UNIT_1
#define ADC_CHANNEL ADC_CHANNEL_3 // ADC1_CH3 is physically GPIO 4 in ESP32
#define ADC_ATTEN ADC_ATTEN_DB_12 // max voltage range  ~3.3V
#define READ_LEN 256

#define PIN_CHARGE GPIO_NUM_23
#define PIN_COMP_OUT GPIO_NUM_19 // Terhubung ke Output LM393
#define RESISTOR_VAL 10000.0     // Resistor pengisi 10k Ohm

volatile uint32_t timestamp_start = 0;
volatile uint32_t timestamp_end = 0;
volatile bool capture_done = false;
uint32_t timer_resolution = 0;
mcpwm_cap_timer_handle_t cap_timer = NULL;
mcpwm_sync_handle_t soft_sync_src = NULL; // to reset the timer counter to 0 at the moment we start charging the capacitor

void uart_setup()
{
    // Setup UART buffered IO with event queue
    const int uart_buffer_size = (1024 * 2);
    QueueHandle_t uart_queue;
    // Install UART driver using an event queue here
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0));

    const uart_port_t uart_num = UART_NUM_2;
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
        .rx_flow_ctrl_thresh = 122,
    };
    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
}
static bool IRAM_ATTR on_capture_reached(mcpwm_cap_channel_handle_t cap_chan, const mcpwm_capture_event_data_t *edata, void *user_data)
{

    timestamp_end = edata->cap_value;
    capture_done = true;
    // Kembalikan false karena kita tidak membangunkan task prioritas tinggi di dalam ISR ini
    return false;
}

static void GPIO_charge_init()
{
    gpio_reset_pin(PIN_CHARGE);
    gpio_set_direction(PIN_CHARGE, GPIO_MODE_OUTPUT);
}
static void start_capacitor_measurement()
{
    // Reset flag dan timestamp
    capture_done = false;
    gpio_set_level(PIN_CHARGE, 1); // Mulai mengisi kapasitor dengan memberikan level HIGH ke pin charge
    timestamp_start = 0;
    
}

static adc_channel_t channel[2] = {ADC_CHANNEL_6, ADC_CHANNEL_7};
/*TODO: Implement ADC continuous initialization*/
static void adc_continuous_init(adc_continuous_handle_t *handle)
{
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = READ_LEN,
        .conv_frame_size = READ_LEN,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, handle));

    adc_continuous_config_t adc_continuous_config = {
        .sample_freq_hz = 1000,                 // Sample frequency in Hz
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,    // Use ADC unit 1
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
        }};

    adc_continuous_config.pattern_num = 2;
    adc_continuous_config.adc_pattern = adc_pattern;
}

void task_read_adc(void *pvParameters)
{
    while (1)
    {
        ESP_LOGI("ADC", "Reading ADC value... ");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
void measurement_setup()
{
    // GPIO init for charging the capacitor
    gpio_reset_pin(PIN_CHARGE);
    gpio_set_direction(PIN_CHARGE, GPIO_MODE_OUTPUT);


    mcpwm_soft_sync_config_t soft_sync_config = {};
    ESP_ERROR_CHECK(mcpwm_new_soft_sync_src(&soft_sync_config, &soft_sync_src)); //

    // 3. Alokasikan Modul MCPWM Capture Timer

    mcpwm_capture_timer_config_t timer_config = {
        .group_id = 0,
        .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT,

    };
    ESP_ERROR_CHECK(mcpwm_new_capture_timer(&timer_config, &cap_timer));

    // interupt init for capture channel
    // config to receive signal from comparator output, which is connected to GPIO 19 (PIN_COMP_OUT)
    mcpwm_cap_channel_handle_t cap_chan = NULL;
    mcpwm_capture_channel_config_t chan_config = {
        .gpio_num = PIN_COMP_OUT,
        .prescale = 1,
        .flags.pos_edge = true, // Triggered on rising edge, which indicates the capacitor voltage has reached the comparator threshold

    };
    ESP_ERROR_CHECK(mcpwm_new_capture_channel(cap_timer, &chan_config, &cap_chan));
    // Register Callback
    mcpwm_capture_event_callbacks_t cbs = {
        .on_cap = on_capture_reached,

        

    };
    ESP_ERROR_CHECK(mcpwm_capture_channel_register_event_callbacks(cap_chan, &cbs, NULL));

    // Activate Hardware Timer & Channel
    ESP_ERROR_CHECK(mcpwm_capture_timer_enable(cap_timer));
    ESP_ERROR_CHECK(mcpwm_capture_channel_enable(cap_chan));
    ESP_ERROR_CHECK(mcpwm_capture_timer_start(cap_timer));
    mcpwm_capture_timer_get_resolution(cap_timer, &timer_resolution);
}
void app_main(void)
{
    GPIO_charge_init();
    uart_setup();
    uint32_t count = 0;
    adc_continuous_handle_t adc_handle;
    adc_continuous_init(&adc_handle);
    measurement_setup();
    xTaskCreate(task_read_adc, "ADC Reader", 2048, NULL, 10, NULL);
    while (1)
    {
        if (capture_done)
        {

            // calculat the time to charge the capacitor using the captured timestamps
            uint32_t cap_charge_time_us = timestamp_end - timestamp_start; // Karena timer MCPWM sudah diatur dengan resolusi 1us
            ESP_LOGI(TAG, "Measurement %d: Time to charge capacitor = %u microseconds", count++, cap_charge_time_us);
            double duration_sec = (double)cap_charge_time_us / (double)timer_resolution; // Konversi ke detik
            // basic formula: C = t / R
            double capacitance_uf = (duration_sec / RESISTOR_VAL) * 1000000.0;
            ESP_LOGI(TAG, "Measurement %d: Capacitance = %.2f microfarads", count++, capacitance_uf);
            start_capacitor_measurement(); // Reset untuk pengukuran berikutnya
        }

        taskYIELD(); // Yield to other tasks (like ADC reading task)
    }
}
