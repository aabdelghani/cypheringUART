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

// HMAC Key (must match sender)
static const uint8_t HMAC_KEY[32] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
    0x0f, 0x1e, 0x2d, 0x3c, 0x4b, 0x5a, 0x69, 0x78,
    0x87, 0x96, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0
};

// Packet structure: [NONCE(16 bytes)][LENGTH(2 bytes)][ENCRYPTED_DATA][HMAC(32 bytes)]
typedef struct {
    uint8_t nonce[AES_BLOCK_SIZE];
    uint16_t data_len;
    uint8_t encrypted_data[BUF_SIZE];
    uint8_t decrypted_data[BUF_SIZE];
    uint8_t received_hmac[HMAC_SIZE];
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

    // Install UART driver (RX buffer, TX buffer, queue size, queue handle, interrupt flags)
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "UART initialized on RX: GPIO%d, TX: GPIO%d", RXD_PIN, TXD_PIN);
}

/**
 * @brief Receive and decrypt data from UART with HMAC verification
 */
static bool receive_and_decrypt(packet_t *packet) {
    uint8_t hmac_input[AES_BLOCK_SIZE + 2 + BUF_SIZE];
    size_t hmac_input_len;
    uint8_t length_bytes[2];

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

    // Wait a bit for length to arrive
    vTaskDelay(pdMS_TO_TICKS(10));

    // Read length (2 bytes, big-endian)
    int length_len = uart_read_bytes(UART_NUM, length_bytes, 2, pdMS_TO_TICKS(500));

    if (length_len != 2) {
        ESP_LOGE(TAG, "Incomplete or no length received: %d bytes", length_len);
        return false;
    }

    // Parse length from big-endian
    packet->data_len = ((uint16_t)length_bytes[0] << 8) | length_bytes[1];

    ESP_LOGI(TAG, "Received length: %d bytes", packet->data_len);

    // Validate length
    if (packet->data_len == 0 || packet->data_len > BUF_SIZE) {
        ESP_LOGE(TAG, "Invalid data length: %d bytes", packet->data_len);
        return false;
    }

    // Wait a bit for encrypted data to arrive
    vTaskDelay(pdMS_TO_TICKS(10));

    // Read exactly data_len bytes of encrypted data
    int data_len = uart_read_bytes(UART_NUM, packet->encrypted_data, packet->data_len, pdMS_TO_TICKS(500));

    if (data_len != packet->data_len) {
        ESP_LOGE(TAG, "Incomplete encrypted data received: %d of %d bytes", data_len, packet->data_len);
        return false;
    }

    ESP_LOGI(TAG, "Received encrypted data (%d bytes):", data_len);
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, packet->encrypted_data, data_len, ESP_LOG_INFO);

    // Wait for HMAC to arrive
    vTaskDelay(pdMS_TO_TICKS(10));

    // Read HMAC (32 bytes)
    int hmac_len = uart_read_bytes(UART_NUM, packet->received_hmac, HMAC_SIZE, pdMS_TO_TICKS(500));

    if (hmac_len != HMAC_SIZE) {
        ESP_LOGE(TAG, "Incomplete or no HMAC received: %d bytes", hmac_len);
        return false;
    }

    ESP_LOGI(TAG, "Received HMAC:");
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, packet->received_hmac, HMAC_SIZE, ESP_LOG_INFO);

    // Verify HMAC before decryption (authenticate then decrypt)
    // HMAC is computed over [NONCE || LENGTH || ENCRYPTED_DATA]
    memcpy(hmac_input, packet->nonce, AES_BLOCK_SIZE);
    memcpy(hmac_input + AES_BLOCK_SIZE, length_bytes, 2);
    memcpy(hmac_input + AES_BLOCK_SIZE + 2, packet->encrypted_data, packet->data_len);
    hmac_input_len = AES_BLOCK_SIZE + 2 + packet->data_len;

    if (!verify_hmac_sha256(hmac_input, hmac_input_len, HMAC_KEY, sizeof(HMAC_KEY), packet->received_hmac)) {
        ESP_LOGE(TAG, "HMAC verification FAILED! Message may be corrupted or tampered!");
        return false;
    }

    ESP_LOGI(TAG, "✓ HMAC verification PASSED - Message authentic");

    // Decrypt the data (only after successful authentication)
    aes_decrypt_ctr(packet->encrypted_data, packet->decrypted_data, packet->data_len, packet->nonce);

    ESP_LOGI(TAG, "Decrypted data:");
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, packet->decrypted_data, packet->data_len, ESP_LOG_INFO);

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

            ESP_LOGI(TAG, "Total packet size: %d bytes (nonce: %d + length: 2 + data: %d + hmac: %d)",
                     AES_BLOCK_SIZE + 2 + packet.data_len + HMAC_SIZE, AES_BLOCK_SIZE, packet.data_len, HMAC_SIZE);
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
