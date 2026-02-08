#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "aes_wrapper.h"

static const char *TAG = "SENDER";

// UART Configuration
#define UART_NUM UART_NUM_2
#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)
#define UART_BAUD_RATE 115200
#define BUF_SIZE 1024

// AES-128 Pre-shared Key (16 bytes)
// In production, this should be securely stored and managed
static const uint8_t AES_SHARED_KEY[AES_KEY_SIZE] = {
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};

// HMAC Key (separate from encryption key for better security)
static const uint8_t HMAC_KEY[32] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
    0x0f, 0x1e, 0x2d, 0x3c, 0x4b, 0x5a, 0x69, 0x78,
    0x87, 0x96, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0
};

// Packet structure: [NONCE(16 bytes)][ENCRYPTED_DATA][HMAC(32 bytes)]
typedef struct {
    uint8_t nonce[AES_BLOCK_SIZE];
    uint8_t data[BUF_SIZE];
    size_t data_len;
    uint8_t hmac[HMAC_SIZE];
} encrypted_packet_t;

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

    ESP_LOGI(TAG, "UART initialized on TX: GPIO%d, RX: GPIO%d", TXD_PIN, RXD_PIN);
}

/**
 * @brief Encrypt and send data over UART with HMAC authentication
 */
static void send_encrypted_data(const uint8_t *plaintext, size_t length) {
    encrypted_packet_t packet;
    uint8_t hmac_input[AES_BLOCK_SIZE + BUF_SIZE];
    size_t hmac_input_len;

    // Generate random nonce
    aes_generate_nonce(packet.nonce);

    // Encrypt the data
    aes_encrypt_ctr(plaintext, packet.data, length, packet.nonce);
    packet.data_len = length;

    // Compute HMAC over [NONCE || ENCRYPTED_DATA]
    memcpy(hmac_input, packet.nonce, AES_BLOCK_SIZE);
    memcpy(hmac_input + AES_BLOCK_SIZE, packet.data, length);
    hmac_input_len = AES_BLOCK_SIZE + length;
    compute_hmac_sha256(hmac_input, hmac_input_len, HMAC_KEY, sizeof(HMAC_KEY), packet.hmac);

    // Log the operation
    ESP_LOGI(TAG, "Encrypting %d bytes", length);
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, plaintext, length, ESP_LOG_INFO);
    ESP_LOGI(TAG, "Nonce:");
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, packet.nonce, AES_BLOCK_SIZE, ESP_LOG_INFO);
    ESP_LOGI(TAG, "Encrypted data:");
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, packet.data, length, ESP_LOG_INFO);
    ESP_LOGI(TAG, "HMAC:");
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, packet.hmac, HMAC_SIZE, ESP_LOG_INFO);

    // Send nonce first (16 bytes)
    int nonce_sent = uart_write_bytes(UART_NUM, packet.nonce, AES_BLOCK_SIZE);
    if (nonce_sent != AES_BLOCK_SIZE) {
        ESP_LOGE(TAG, "Failed to send nonce");
        return;
    }

    // Send encrypted data
    int data_sent = uart_write_bytes(UART_NUM, packet.data, length);
    if (data_sent != length) {
        ESP_LOGE(TAG, "Failed to send encrypted data");
        return;
    }

    // Send HMAC (32 bytes)
    int hmac_sent = uart_write_bytes(UART_NUM, packet.hmac, HMAC_SIZE);
    if (hmac_sent != HMAC_SIZE) {
        ESP_LOGE(TAG, "Failed to send HMAC");
        return;
    }

    ESP_LOGI(TAG, "Sent %d bytes (nonce: %d + data: %d + hmac: %d)",
             nonce_sent + data_sent + hmac_sent, nonce_sent, data_sent, hmac_sent);
}

/**
 * @brief Main sender task
 */
static void sender_task(void *arg) {
    // Example messages to send
    const char *messages[] = {
        "Hello from ESP32 Sender!",
        "This is encrypted data",
        "AES-128 CTR mode active",
        "Secure communication test"
    };

    int msg_index = 0;
    int total_messages = sizeof(messages) / sizeof(messages[0]);

    while (1) {
        const char *message = messages[msg_index];
        size_t msg_len = strlen(message);

        ESP_LOGI(TAG, "\n=== Sending message %d ===", msg_index + 1);
        ESP_LOGI(TAG, "Plaintext: %s", message);

        // Encrypt and send the message
        send_encrypted_data((const uint8_t *)message, msg_len);

        // Move to next message
        msg_index = (msg_index + 1) % total_messages;

        // Wait before sending next message
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "=== ESP32 Encrypted UART Sender ===");
    ESP_LOGI(TAG, "Initializing AES-128 CTR encryption...");

    // Initialize AES with pre-shared key
    aes_init(AES_SHARED_KEY);
    ESP_LOGI(TAG, "AES initialized with shared key");

    // Initialize UART
    uart_init();

    // Create sender task
    xTaskCreate(sender_task, "sender_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Sender ready, starting transmission...");
}
