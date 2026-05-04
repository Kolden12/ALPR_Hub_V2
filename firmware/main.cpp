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

// Hardware Mapping
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

void IRAM_ATTR imu_isr(void* arg) {
    gpio_set_level((gpio_num_t)CRITICAL_FLUSH_GPIO, 1);
}

void init_guardian() {
    video_buffer = (uint8_t*)heap_caps_malloc(PSRAM_BUFFER_SIZE, MALLOC_CAP_SPIRAM);

    // UART 115200
    uart_config_t u_cfg = { .baud_rate = 115200, .data_bits = UART_DATA_8_BITS, .parity = UART_PARITY_DISABLE, .stop_bits = UART_STOP_BITS_1 };
    uart_param_config(UART_NUM_0, &u_cfg);
    uart_driver_install(UART_NUM_0, 1024, 0, 0, NULL, 0);

    // GPIO & ISR
    gpio_config_t io = { .intr_type = GPIO_INTR_POSEDGE, .mode = GPIO_MODE_INPUT, .pin_bit_mask = (1ULL << IMU_INT_GPIO), .pull_up_en = 1 };
    gpio_config(&io);
    gpio_install_isr_service(0);
    gpio_isr_handler_add((gpio_num_t)IMU_INT_GPIO, imu_isr, NULL);

    gpio_set_direction((gpio_num_t)JETSON_POWER_EN_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)SYS_RESET_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)SYS_RESET_GPIO, 1);

    // PWM Fan (25kHz)
    ledc_timer_config_t lt = { .speed_mode = LEDC_LOW_SPEED_MODE, .duty_resolution = LEDC_TIMER_8_BIT, .timer_num = LEDC_TIMER_0, .freq_hz = 25000, .clk_cfg = LEDC_AUTO_CLK };
    ledc_timer_config(&lt);
    ledc_channel_config_t lc = { .speed_mode = LEDC_LOW_SPEED_MODE, .channel = LEDC_CHANNEL_0, .timer_sel = LEDC_TIMER_0, .gpio_num = FAN_PWM_GPIO, .duty = 0 };
    ledc_channel_config(&lc);

    // ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
}

// NAL unit scanning and IDR snapping
void process_video(uint8_t *data, size_t len) {
    for (size_t i = 0; i < len - 4; i++) {
        if (data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x00 && data[i+3] == 0x01) {
            if ((data[i+4] & 0x1F) == 5) write_idx = 0; // IDR Frame
        }
    }
    if (write_idx + len < PSRAM_BUFFER_SIZE) {
        memcpy(video_buffer + write_idx, data, len);
        write_idx += len;
    }
}

extern "C" void app_main() {
    init_guardian();
    last_heartbeat = esp_timer_get_time() / 1000;
    uint8_t buf[256];
    while(1) {
        // Watchdog & Heartbeat
        int len = uart_read_bytes(UART_NUM_0, buf, sizeof(buf), 10 / portTICK_PERIOD_MS);
        if (len > 0) {
            sabre_packet_header_t *h = (sabre_packet_header_t*)buf;
            if (h->header == SABRE_HEADER && h->command_id == CMD_HEARTBEAT) last_heartbeat = esp_timer_get_time() / 1000;
        }
        if (esp_timer_get_time()/1000 - last_heartbeat > 60000) {
            gpio_set_level((gpio_num_t)SYS_RESET_GPIO, 0);
            vTaskDelay(pdMS_TO_TICKS(500));
            gpio_set_level((gpio_num_t)SYS_RESET_GPIO, 1);
            last_heartbeat = esp_timer_get_time() / 1000;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
