#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "esp_adc_cal.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "../../shared/sabre_protocol.h"

// Thresholds
#define BATT_STABILIZE_VOLTAGE 12.6
#define BATT_CRITICAL_VOLTAGE  11.8
#define SHUTDOWN_DELAY_MS      (5 * 60 * 1000)

// Pin Definitions
#define JETSON_POWER_EN_GPIO 12
#define CRITICAL_FLUSH_GPIO  14
#define FAN_PWM_GPIO         15
#define IGNITION_SENSE_GPIO  16
#define IMU_INT_GPIO         17
#define SYS_RESET_GPIO       18

#define PSRAM_BUFFER_SIZE (16 * 1024 * 1024)
static uint8_t *video_buffer = NULL;
static size_t write_idx = 0;

static uint64_t last_heartbeat = 0;
static bool jetson_on = false;

void IRAM_ATTR imu_isr(void* arg) {
    gpio_set_level((gpio_num_t)CRITICAL_FLUSH_GPIO, 1);
}

void init_guardian() {
    video_buffer = (uint8_t*)heap_caps_malloc(PSRAM_BUFFER_SIZE, MALLOC_CAP_SPIRAM);

    // UART Setup
    uart_config_t uart_cfg = { .baud_rate = 115200, .data_bits = UART_DATA_8_BITS, .parity = UART_PARITY_DISABLE, .stop_bits = UART_STOP_BITS_1 };
    uart_param_config(UART_NUM_0, &uart_cfg);
    uart_driver_install(UART_NUM_0, 1024, 0, 0, NULL, 0);

    // GPIO Setup
    gpio_set_direction((gpio_num_t)JETSON_POWER_EN_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)CRITICAL_FLUSH_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)SYS_RESET_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)SYS_RESET_GPIO, 1);

    gpio_set_direction((gpio_num_t)IGNITION_SENSE_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode((gpio_num_t)IGNITION_SENSE_GPIO, GPIO_PULLUP_ONLY);

    // ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
}

// H.264 PSRAM Circular Buffer with IDR alignment
void store_video_packet(uint8_t *data, size_t len) {
    if (len > PSRAM_BUFFER_SIZE) return;

    // Scan for IDR (NAL 0x05)
    for (size_t i = 0; i < len - 4; i++) {
        if (data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x00 && data[i+3] == 0x01) {
            if ((data[i+4] & 0x1F) == 5) {
                write_idx = 0; // Snap to Start
            }
        }
    }

    if (write_idx + len > PSRAM_BUFFER_SIZE) write_idx = 0;
    memcpy(video_buffer + write_idx, data, len);
    write_idx += len;
}

extern "C" void app_main() {
    init_guardian();
    last_heartbeat = esp_timer_get_time() / 1000;

    uint8_t rx_buf[256];
    while(1) {
        // Power Logic
        int raw = adc1_get_raw(ADC1_CHANNEL_0);
        float v = (raw / 4095.0) * 15.0;
        bool ign = gpio_get_level((gpio_num_t)IGNITION_SENSE_GPIO);

        if (!jetson_on && v >= BATT_STABILIZE_VOLTAGE && ign) {
            gpio_set_level((gpio_num_t)JETSON_POWER_EN_GPIO, 1);
            jetson_on = true;
        }

        // UART / Heartbeat
        int len = uart_read_bytes(UART_NUM_0, rx_buf, sizeof(rx_buf), 10 / portTICK_PERIOD_MS);
        if (len > 0) {
            sabre_packet_header_t *h = (sabre_packet_header_t*)rx_buf;
            if (h->header == SABRE_HEADER && h->command_id == CMD_HEARTBEAT) {
                last_heartbeat = esp_timer_get_time() / 1000;
            }
        }

        // Watchdog
        if (jetson_on && (esp_timer_get_time() / 1000 - last_heartbeat > 60000)) {
            gpio_set_level((gpio_num_t)SYS_RESET_GPIO, 0);
            vTaskDelay(pdMS_TO_TICKS(500));
            gpio_set_level((gpio_num_t)SYS_RESET_GPIO, 1);
            last_heartbeat = esp_timer_get_time() / 1000;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
