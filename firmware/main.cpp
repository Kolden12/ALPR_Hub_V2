#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "esp_heap_caps.h"
#include "../../shared/sabre_protocol.h"

#define PSRAM_BUFFER_SIZE (16 * 1024 * 1024)
static uint8_t *video_buffer = NULL;
static size_t write_idx = 0;

// Scan for NAL Start Code + IDR Type (0x05)
void handle_incoming_video(uint8_t *data, size_t len) {
    for (size_t i = 0; i < len - 5; i++) {
        if (data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x00 && data[i+3] == 0x01) {
            uint8_t nal_type = data[i+4] & 0x1F;
            if (nal_type == 5) { // IDR Frame Found
                write_idx = 0; // Reset pointer to ensure IDR start
            }
        }
    }
    // Copy data to PSRAM with wrap...
    if (write_idx + len < PSRAM_BUFFER_SIZE) {
        memcpy(video_buffer + write_idx, data, len);
        write_idx += len;
    } else {
        write_idx = 0; // Simple wrap for GM
    }
}

extern "C" void app_main() {
    video_buffer = (uint8_t*)heap_caps_malloc(PSRAM_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
    // Initialize UART and GPIOs ...
}
