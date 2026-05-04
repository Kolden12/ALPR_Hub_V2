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

#define BATT_STABILIZE_VOLTAGE 12.6
#define BATT_CRITICAL_VOLTAGE  11.8
#define SHUTDOWN_DELAY_MS      (5 * 60 * 1000)

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

    // UART
    uart_config_t uart_cfg = { .baud_rate = 115200, .data_bits = UART_DATA_8_BITS, .parity = UART_PARITY_DISABLE, .stop_bits = UART_STOP_BITS_1 };
    uart_param_config(UART_NUM_0, &uart_cfg);
    uart_driver_install(UART_NUM_0, 1024, 0, 0, NULL, 0);

    // GPIO
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << IMU_INT_GPIO) | (1ULL << IGNITION_SENSE_GPIO),
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add((gpio_num_t)IMU_INT_GPIO, imu_isr, NULL);

    gpio_set_direction((gpio_num_t)JETSON_POWER_EN_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)CRITICAL_FLUSH_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)SYS_RESET_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)SYS_RESET_GPIO, 1);
}

void process_packet(uint8_t *data, size_t len) {
    sabre_packet_header_t *h = (sabre_packet_header_t*)data;
    if (h->header != SABRE_HEADER) return;

    if (h->command_id == CMD_HEARTBEAT) {
        last_heartbeat = esp_timer_get_time() / 1000;
    } else if (h->command_id == CMD_CLEAR_INTERRUPT) {
        gpio_set_level((gpio_num_t)CRITICAL_FLUSH_GPIO, 0);
    }
}

extern "C" void app_main() {
    init_guardian();
    last_heartbeat = esp_timer_get_time() / 1000;

    uint8_t rx_byte;
    uint8_t packet_buffer[256];
    int state = 0, idx = 0, payload_len = 0;

    while(1) {
        // Watchdog & Power Management
        int raw = adc1_get_raw(ADC1_CHANNEL_0);
        float v = (raw / 4095.0) * 15.0;
        bool ign = gpio_get_level((gpio_num_t)IGNITION_SENSE_GPIO);

        if (!jetson_on && v >= BATT_STABILIZE_VOLTAGE && ign) {
            gpio_set_level((gpio_num_t)JETSON_POWER_EN_GPIO, 1);
            jetson_on = true;
        }

        if (uart_read_bytes(UART_NUM_0, &rx_byte, 1, 5 / portTICK_PERIOD_MS) > 0) {
            if (state == 0 && rx_byte == 0x53) { packet_buffer[idx++] = rx_byte; state = 1; }
            else if (state == 1 && rx_byte == 0x42) { packet_buffer[idx++] = rx_byte; state = 2; }
            else if (state == 2) { payload_len = rx_byte; packet_buffer[idx++] = rx_byte; state = 3; }
            else if (state == 3) {
                packet_buffer[idx++] = rx_byte;
                if (idx >= (4 + payload_len + 2)) {
                    process_packet(packet_buffer, idx);
                    state = 0; idx = 0;
                }
            } else { state = 0; idx = 0; }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
