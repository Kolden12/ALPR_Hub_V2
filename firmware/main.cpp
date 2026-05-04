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
#define SYS_RESET_GPIO       18

static bool hardware_bricked = false;

void check_brick_status() {
    nvs_handle_t nvs;
    if (nvs_open("storage", NVS_READONLY, &nvs) == ESP_OK) {
        uint8_t bricked = 0;
        nvs_get_u8(nvs, "bricked", &bricked);
        hardware_bricked = (bricked == 1);
        nvs_close(nvs);
    }

    // Physical Jumper Override
    if (gpio_get_level((gpio_num_t)JUMPER_CLEAR_GPIO) == 0) {
        hardware_bricked = false;
        nvs_open("storage", NVS_READWRITE, &nvs);
        nvs_set_u8(nvs, "bricked", 0);
        nvs_commit(nvs);
        nvs_close(nvs);
    }
}

void process_packet(uint8_t *data, size_t len) {
    sabre_packet_header_t *h = (sabre_packet_header_t*)data;
    if (h->command_id == 0xFE) { // WIPE COMMAND
        uint16_t code;
        memcpy(&code, data + 4, 2);
        if (code == 0xDEAD) {
            hardware_bricked = true;
            nvs_handle_t nvs;
            nvs_open("storage", NVS_READWRITE, &nvs);
            nvs_set_u8(nvs, "bricked", 1);
            nvs_commit(nvs);
            nvs_close(nvs);
            gpio_set_level((gpio_num_t)JETSON_POWER_EN_GPIO, 0);
        }
    }
}

extern "C" void app_main() {
    nvs_flash_init();
    gpio_set_direction((gpio_num_t)JUMPER_CLEAR_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode((gpio_num_t)JUMPER_CLEAR_GPIO, GPIO_PULLUP_ONLY);

    check_brick_status();

    while(1) {
        if (hardware_bricked) {
            gpio_set_level((gpio_num_t)JETSON_POWER_EN_GPIO, 0);
        } else {
            // Normal operation logic ...
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
