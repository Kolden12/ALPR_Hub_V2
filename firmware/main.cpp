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

// Pin Definitions
#define JETSON_POWER_EN_GPIO 12
#define CRITICAL_FLUSH_GPIO  14
#define FAN_PWM_GPIO         15
#define IGNITION_SENSE_GPIO  16
#define IMU_INT_GPIO         17
#define SYS_RESET_GPIO       18

#define UART_PORT_NUM      UART_NUM_0
#define UART_BAUD_RATE     115200

// PSRAM Buffer for 10s H.264
#define PSRAM_BUFFER_SIZE (16 * 1024 * 1024)
static uint8_t *video_buffer = NULL;
static size_t write_ptr = 0;

static bool jetson_powered = false;
static uint64_t ignition_lost_time = 0;
static uint64_t last_heartbeat_time = 0;

void IRAM_ATTR imu_interrupt_handler(void* arg) {
    gpio_set_level((gpio_num_t)CRITICAL_FLUSH_GPIO, 1);
}

void init_hardware() {
    // PSRAM Allocation
    video_buffer = (uint8_t*)heap_caps_malloc(PSRAM_BUFFER_SIZE, MALLOC_CAP_SPIRAM);

    // GPIO Config
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.pin_bit_mask = (1ULL << IMU_INT_GPIO) | (1ULL << IGNITION_SENSE_GPIO);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add((gpio_num_t)IMU_INT_GPIO, imu_interrupt_handler, NULL);

    gpio_set_direction((gpio_num_t)JETSON_POWER_EN_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)CRITICAL_FLUSH_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)SYS_RESET_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)SYS_RESET_GPIO, 1); // Active Low

    // PWM Fan Config
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 25000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = FAN_PWM_GPIO,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&ledc_channel);

    // UART Config
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_driver_install(UART_PORT_NUM, 1024, 0, 0, NULL, 0);

    // ADC Config
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);

    last_heartbeat_time = esp_timer_get_time() / 1000;
}

void process_packet(uint8_t *data, size_t len) {
    if (len < sizeof(sabre_packet_header_t) + 2) return;
    sabre_packet_header_t *header = (sabre_packet_header_t*)data;
    if (header->header != SABRE_HEADER) return;

    uint16_t received_crc;
    memcpy(&received_crc, data + sizeof(sabre_packet_header_t) + header->length, 2);

    if (sabre_crc16(data, sizeof(sabre_packet_header_t) + header->length) == received_crc) {
        switch(header->command_id) {
            case CMD_HEARTBEAT:
                last_heartbeat_time = esp_timer_get_time() / 1000;
                break;
            case CMD_FAN_CONTROL: {
                sabre_fan_payload_t *payload = (sabre_fan_payload_t*)(data + sizeof(sabre_packet_header_t));
                ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, payload->fan_pwm_percent);
                ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
                break;
            }
            case CMD_CLEAR_INTERRUPT:
                gpio_set_level((gpio_num_t)CRITICAL_FLUSH_GPIO, 0);
                break;
        }
    }
}

void power_management_task(void *pvParameters) {
    while(1) {
        int raw = adc1_get_raw(ADC1_CHANNEL_0);
        float voltage = (raw / 4095.0) * 15.0;
        bool ignition = gpio_get_level((gpio_num_t)IGNITION_SENSE_GPIO);

        if (!jetson_powered) {
            if (voltage >= BATT_STABILIZE_VOLTAGE && ignition) {
                gpio_set_level((gpio_num_t)JETSON_POWER_EN_GPIO, 1);
                jetson_powered = true;
            }
        } else {
            // Watchdog Check
            uint64_t now = esp_timer_get_time() / 1000;
            if (now - last_heartbeat_time > 60000) {
                gpio_set_level((gpio_num_t)SYS_RESET_GPIO, 0);
                vTaskDelay(pdMS_TO_TICKS(500));
                gpio_set_level((gpio_num_t)SYS_RESET_GPIO, 1);
                last_heartbeat_time = now;
            }
            // Shutdown Logic
            if (voltage < BATT_CRITICAL_VOLTAGE || (!ignition && (esp_timer_get_time()/1000 - ignition_lost_time >= SHUTDOWN_DELAY_MS && ignition_lost_time != 0))) {
                gpio_set_level((gpio_num_t)JETSON_POWER_EN_GPIO, 0);
                jetson_powered = false;
            }
            if (!ignition && ignition_lost_time == 0) ignition_lost_time = esp_timer_get_time()/1000;
            if (ignition) ignition_lost_time = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

extern "C" void app_main() {
    init_hardware();
    xTaskCreate(power_management_task, "power_task", 4096, NULL, 5, NULL);

    uint8_t rx_byte;
    uint8_t packet_buffer[256];
    int state = 0, idx = 0, payload_len = 0;

    while(1) {
        if (uart_read_bytes(UART_PORT_NUM, &rx_byte, 1, 10 / portTICK_PERIOD_MS) > 0) {
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
    }
}
