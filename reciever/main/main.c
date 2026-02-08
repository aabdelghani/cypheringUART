#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "aes_wrapper.h"

static const char *TAG = "RECEIVER";

// UART Configuration
#define UART_NUM UART_NUM_2
#define RXD_PIN (GPIO_NUM_16)
#define TXD_PIN (GPIO_NUM_17)
#define UART_BAUD_RATE 115200
#define BUF_SIZE 1024

// AES-128 Pre-shared Key (must match sender)
static const uint8_t AES_SHARED_KEY[AES_KEY_SIZE] = {
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};

// Packet structure: [NONCE(16 bytes)][ENCRYPTED_DATA]
typedef struct {
    uint8_t nonce[AES_BLOCK_SIZE];
    uint8_t encrypted_data[BUF_SIZE];
    uint8_t decrypted_data[BUF_SIZE];
    size_t data_len;
} packet_t;

/**
 * @brief Initialize UART for communication
 */
static void uart_init(void) {
    const uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Install UART driver
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "UART initialized on RX: GPIO%d, TX: GPIO%d", RXD_PIN, TXD_PIN);
}

/**
 * @brief Receive and decrypt data from UART
 */
static bool receive_and_decrypt(packet_t *packet) {
    // Read nonce first (16 bytes)
    int nonce_len = uart_read_bytes(UART_NUM, packet->nonce, AES_BLOCK_SIZE, pdMS_TO_TICKS(1000));

    if (nonce_len != AES_BLOCK_SIZE) {
        if (nonce_len > 0) {
            ESP_LOGW(TAG, "Incomplete nonce received: %d bytes", nonce_len);
        }
        return false;
    }

    ESP_LOGI(TAG, "Received nonce:");
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, packet->nonce, AES_BLOCK_SIZE, ESP_LOG_INFO);

    // Wait a bit for encrypted data to arrive
    vTaskDelay(pdMS_TO_TICKS(10));

    // Read encrypted data
    int data_len = uart_read_bytes(UART_NUM, packet->encrypted_data, BUF_SIZE, pdMS_TO_TICKS(500));

    if (data_len <= 0) {
        ESP_LOGE(TAG, "No encrypted data received");
        return false;
    }

    packet->data_len = data_len;

    ESP_LOGI(TAG, "Received encrypted data (%d bytes):", data_len);
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, packet->encrypted_data, data_len, ESP_LOG_INFO);

    // Decrypt the data
    aes_decrypt_ctr(packet->encrypted_data, packet->decrypted_data, data_len, packet->nonce);

    ESP_LOGI(TAG, "Decrypted data:");
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, packet->decrypted_data, data_len, ESP_LOG_INFO);

    return true;
}

/**
 * @brief Main receiver task
 */
static void receiver_task(void *arg) {
    packet_t packet;
    int message_count = 0;

    ESP_LOGI(TAG, "Receiver task started, waiting for encrypted messages...");

    while (1) {
        // Clear packet buffer
        memset(&packet, 0, sizeof(packet));

        // Receive and decrypt packet
        if (receive_and_decrypt(&packet)) {
            message_count++;

            ESP_LOGI(TAG, "\n========================================");
            ESP_LOGI(TAG, "Message #%d successfully decrypted!", message_count);
            ESP_LOGI(TAG, "========================================");

            // Try to display as string
            packet.decrypted_data[packet.data_len] = '\0';  // Null terminate
            ESP_LOGI(TAG, "Plaintext message: \"%s\"", (char *)packet.decrypted_data);

            ESP_LOGI(TAG, "Total packet size: %d bytes (nonce: %d + data: %d)",
                     AES_BLOCK_SIZE + packet.data_len, AES_BLOCK_SIZE, packet.data_len);
            ESP_LOGI(TAG, "========================================\n");
        }

        // Small delay to prevent tight loop
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "=== ESP32-S3 Encrypted UART Receiver ===");
    ESP_LOGI(TAG, "Initializing AES-128 CTR decryption...");

    // Initialize AES with pre-shared key
    aes_init(AES_SHARED_KEY);
    ESP_LOGI(TAG, "AES initialized with shared key");

    // Initialize UART
    uart_init();

    // Create receiver task
    xTaskCreate(receiver_task, "receiver_task", 8192, NULL, 5, NULL);

    ESP_LOGI(TAG, "Receiver ready, waiting for encrypted data...");
}
