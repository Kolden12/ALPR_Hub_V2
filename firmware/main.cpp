#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/adc.h"
#include "driver/ledc.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "../../shared/sabre_protocol.h"

// Hardware Mapping
#define JETSON_POWER_EN_GPIO 12
#define JUMPER_CLEAR_GPIO    21
#define FAN_PWM_GPIO         15
#define IGNITION_SENSE_GPIO  16
#define IMU_INT_GPIO         17
#define SYS_RESET_GPIO       18

#define UART_PORT_NUM        UART_NUM_0
#define UART_BAUD_RATE       115200

static bool is_bricked = false;
static uint8_t last_reset_reason = 0x00;
static uint64_t last_heartbeat = 0;

void set_nvs_u8(const char* key, uint8_t val) {
    nvs_handle_t nvs;
    if (nvs_open("storage", NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_set_u8(nvs, key, val);
        nvs_commit(nvs);
        nvs_close(nvs);
    }
}

void check_brick_status() {
    nvs_handle_t nvs;
    if (nvs_open("storage", NVS_READWRITE, &nvs) == ESP_OK) {
        uint8_t flag = 0;
        nvs_get_u8(nvs, "bricked", &flag);
        is_bricked = (flag == 1);
        nvs_get_u8(nvs, "reset_reason", &last_reset_reason);
        nvs_close(nvs);
    }

    // Hardware Jumper Override (Active Low)
    if (gpio_get_level((gpio_num_t)JUMPER_CLEAR_GPIO) == 0) {
        is_bricked = false;
        set_nvs_u8("bricked", 0);
        printf("JUMPER DETECTED: Unit UN-BRICKED.\n");
    }
}

void process_packet(uint8_t *data, size_t len) {
    if (len < 4) return;
    sabre_packet_header_t *h = (sabre_packet_header_t*)data;
    if (h->header != SABRE_HEADER) return;

    // CRC Validation
    uint16_t calc_crc = sabre_crc16(data, len - 2);
    uint16_t recv_crc;
    memcpy(&recv_crc, data + len - 2, 2);
    if (calc_crc != recv_crc) return;

    switch(h->command_id) {
        case CMD_HEARTBEAT:
            last_heartbeat = esp_timer_get_time() / 1000;
            break;
        case 0xFE: { // WIPE_CMD
            uint16_t code;
            memcpy(&code, data + 4, 2);
            if (code == 0xDEAD) {
                is_bricked = true;
                set_nvs_u8("bricked", 1);
                gpio_set_level((gpio_num_t)JETSON_POWER_EN_GPIO, 0);
            }
            break;
        }
        case 0x08: { // CMD_QUERY_DIAGNOSTICS
            uint8_t resp[] = {0x53, 0x42, 0x01, 0x08, last_reset_reason, 0x00, 0x00};
            uint16_t crc = sabre_crc16(resp, 5);
            memcpy(resp + 5, &crc, 2);
            uart_write_bytes(UART_PORT_NUM, (const char*)resp, sizeof(resp));
            break;
        }
    }
}

extern "C" void app_main() {
    nvs_flash_init();

    // GPIO
    gpio_set_direction((gpio_num_t)JUMPER_CLEAR_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode((gpio_num_t)JUMPER_CLEAR_GPIO, GPIO_PULLUP_ONLY);
    gpio_set_direction((gpio_num_t)JETSON_POWER_EN_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)SYS_RESET_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)SYS_RESET_GPIO, 1);

    // UART
    uart_config_t u_cfg = { .baud_rate = 115200, .data_bits = UART_DATA_8_BITS, .parity = UART_PARITY_DISABLE, .stop_bits = UART_STOP_BITS_1 };
    uart_param_config(UART_PORT_NUM, &u_cfg);
    uart_driver_install(UART_PORT_NUM, 1024, 0, 0, NULL, 0);

    check_brick_status();
    last_heartbeat = esp_timer_get_time() / 1000;

    uint8_t rx_byte;
    uint8_t p_buf[256];
    int state = 0, idx = 0, p_len = 0;

    while(1) {
        gpio_set_level((gpio_num_t)JETSON_POWER_EN_GPIO, is_bricked ? 0 : 1);

        if (uart_read_bytes(UART_PORT_NUM, &rx_byte, 1, 10 / portTICK_PERIOD_MS) > 0) {
            if (state == 0 && rx_byte == 0x53) { p_buf[idx++] = rx_byte; state = 1; }
            else if (state == 1 && rx_byte == 0x42) { p_buf[idx++] = rx_byte; state = 2; }
            else if (state == 2) { p_len = rx_byte; p_buf[idx++] = rx_byte; state = 3; }
            else if (state == 3) {
                p_buf[idx++] = rx_byte;
                if (idx >= (4 + p_len + 2)) {
                    process_packet(p_buf, idx);
                    state = 0; idx = 0;
                }
            } else { state = 0; idx = 0; }
        }

        // Watchdog (60s)
        if (!is_bricked && (esp_timer_get_time()/1000 - last_heartbeat > 60000)) {
            set_nvs_u8("reset_reason", 0x01); // 0x01 = Watchdog
            gpio_set_level((gpio_num_t)SYS_RESET_GPIO, 0);
            vTaskDelay(pdMS_TO_TICKS(500));
            gpio_set_level((gpio_num_t)SYS_RESET_GPIO, 1);
            last_heartbeat = esp_timer_get_time() / 1000;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
