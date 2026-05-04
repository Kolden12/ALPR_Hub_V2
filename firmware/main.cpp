#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "../../shared/sabre_protocol.h"

#define JETSON_POWER_EN_GPIO 12
#define JUMPER_CLEAR_GPIO    21
#define UART_PORT_NUM        UART_NUM_0

static bool is_bricked = false;

void check_brick_status() {
    nvs_handle_t nvs;
    if (nvs_open("storage", NVS_READONLY, &nvs) == ESP_OK) {
        uint8_t brick_flag = 0;
        nvs_get_u8(nvs, "bricked", &brick_flag);
        is_bricked = (brick_flag == 1);
        nvs_close(nvs);
    }

    // Hardware Jumper Override (Active Low)
    if (gpio_get_level((gpio_num_t)JUMPER_CLEAR_GPIO) == 0) {
        is_bricked = false;
        nvs_open("storage", NVS_READWRITE, &nvs);
        nvs_set_u8(nvs, "bricked", 0);
        nvs_commit(nvs);
        nvs_close(nvs);
        printf("JUMPER DETECTED: Unit UN-BRICKED.\n");
    }
}

void process_packet(uint8_t *data, size_t len) {
    if (len < 4) return;
    sabre_packet_header_t *h = (sabre_packet_header_t*)data;
    if (h->header != SABRE_HEADER) return;

    if (h->command_id == 0xFE) { // WIPE COMMAND
        uint16_t wipe_code;
        memcpy(&wipe_code, data + 4, 2);
        if (wipe_code == 0xDEAD) {
            is_bricked = true;
            nvs_handle_t nvs;
            nvs_open("storage", NVS_READWRITE, &nvs);
            nvs_set_u8(nvs, "bricked", 1);
            nvs_commit(nvs);
            nvs_close(nvs);
            gpio_set_level((gpio_num_t)JETSON_POWER_EN_GPIO, 0);
            printf("NUCLEAR WIPE: Unit BRICKED.\n");
        }
    }
}

extern "C" void app_main() {
    nvs_flash_init();

    // GPIO Config
    gpio_set_direction((gpio_num_t)JUMPER_CLEAR_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode((gpio_num_t)JUMPER_CLEAR_GPIO, GPIO_PULLUP_ONLY);
    gpio_set_direction((gpio_num_t)JETSON_POWER_EN_GPIO, GPIO_MODE_OUTPUT);

    // UART Init
    uart_config_t uart_cfg = { .baud_rate = 115200, .data_bits = UART_DATA_8_BITS, .parity = UART_PARITY_DISABLE, .stop_bits = UART_STOP_BITS_1 };
    uart_param_config(UART_PORT_NUM, &uart_cfg);
    uart_driver_install(UART_PORT_NUM, 1024, 0, 0, NULL, 0);

    check_brick_status();

    uint8_t rx_byte;
    uint8_t packet_buffer[256];
    int state = 0, idx = 0, payload_len = 0;

    while(1) {
        if (is_bricked) {
            gpio_set_level((gpio_num_t)JETSON_POWER_EN_GPIO, 0);
        } else {
            gpio_set_level((gpio_num_t)JETSON_POWER_EN_GPIO, 1);
        }

        // UART Framing state machine
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
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
