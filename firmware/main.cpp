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

#define JETSON_POWER_EN_GPIO 12
#define CRITICAL_FLUSH_GPIO  14
#define FAN_PWM_GPIO         15
#define IGNITION_SENSE_GPIO  16
#define IMU_INT_GPIO         17
#define SYS_RESET_GPIO       18

#define UART_PORT_NUM      UART_NUM_0
#define UART_BAUD_RATE     115200

#define PSRAM_BUFFER_SIZE (16 * 1024 * 1024)
static uint8_t *video_buffer = NULL;
static size_t write_idx = 0;

static uint64_t last_heartbeat_time = 0;
static bool watchdog_active = true;

void IRAM_ATTR imu_interrupt_handler(void* arg) {
    gpio_set_level((gpio_num_t)CRITICAL_FLUSH_GPIO, 1);
}

void init_hardware() {
    video_buffer = (uint8_t*)heap_caps_malloc(PSRAM_BUFFER_SIZE, MALLOC_CAP_SPIRAM);

    gpio_set_direction((gpio_num_t)JETSON_POWER_EN_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)CRITICAL_FLUSH_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)SYS_RESET_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)SYS_RESET_GPIO, 1); // Active Low Reset

    // UART, ADC, PWM ... (Same as previous)
    last_heartbeat_time = esp_timer_get_time() / 1000;
}

// H.264 I-Frame Snapping Logic
void ingest_video_data(uint8_t *data, size_t len) {
    // Scan for 00 00 00 01
    for (size_t i = 0; i < len - 4; i++) {
        if (data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x00 && data[i+3] == 0x01) {
            uint8_t nal_type = data[i+4] & 0x1F;
            if (nal_type == 0x05) { // IDR / I-Frame
                write_idx = 0; // Snap pointer to start on IDR
            }
        }
    }
    // Copy data to video_buffer with wraparound logic...
}

void watchdog_task(void *pvParameters) {
    while(1) {
        uint64_t now = esp_timer_get_time() / 1000;
        if (now - last_heartbeat_time > 60000 && watchdog_active) {
            printf("WATCHDOG: Heartbeat lost > 60s. TRIGGERING HARD RESET.\n");
            gpio_set_level((gpio_num_t)SYS_RESET_GPIO, 0);
            vTaskDelay(pdMS_TO_TICKS(500));
            gpio_set_level((gpio_num_t)SYS_RESET_GPIO, 1);
            last_heartbeat_time = now; // Reset timer to allow reboot
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void process_packet(uint8_t *data, size_t len) {
    sabre_packet_header_t *header = (sabre_packet_header_t*)data;
    if (header->command_id == CMD_HEARTBEAT) {
        last_heartbeat_time = esp_timer_get_time() / 1000;
    } else if (header->command_id == CMD_CLEAR_INTERRUPT) {
        gpio_set_level((gpio_num_t)CRITICAL_FLUSH_GPIO, 0);
    }
}

extern "C" void app_main() {
    init_hardware();
    xTaskCreate(watchdog_task, "watchdog_task", 2048, NULL, 10, NULL);

    uint8_t rx_byte;
    // Main loop with framing state machine (implemented previously)
}
