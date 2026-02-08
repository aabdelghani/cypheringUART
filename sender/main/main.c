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

// Packet structure: [NONCE(16 bytes)][ENCRYPTED_DATA]
typedef struct {
    uint8_t nonce[AES_BLOCK_SIZE];
    uint8_t data[BUF_SIZE];
    size_t data_len;
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
 * @brief Encrypt and send data over UART
 */
static void send_encrypted_data(const uint8_t *plaintext, size_t length) {
    encrypted_packet_t packet;

    // Generate random nonce
    aes_generate_nonce(packet.nonce);

    // Encrypt the data
    aes_encrypt_ctr(plaintext, packet.data, length, packet.nonce);
    packet.data_len = length;

    // Log the operation
    ESP_LOGI(TAG, "Encrypting %d bytes", length);
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, plaintext, length, ESP_LOG_INFO);
    ESP_LOGI(TAG, "Nonce:");
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, packet.nonce, AES_BLOCK_SIZE, ESP_LOG_INFO);
    ESP_LOGI(TAG, "Encrypted data:");
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, packet.data, length, ESP_LOG_INFO);

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

    ESP_LOGI(TAG, "Sent %d bytes (nonce + encrypted data)", nonce_sent + data_sent);
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
