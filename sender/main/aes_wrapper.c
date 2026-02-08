#include "aes_wrapper.h"
#include "aes.h"
#include <string.h>
#include "esp_system.h"
#include "esp_random.h"

// Global AES key storage
static uint8_t aes_key[AES_KEY_SIZE];
static struct AES_ctx ctx;

void aes_init(const uint8_t *key) {
    memcpy(aes_key, key, AES_KEY_SIZE);
}

void aes_encrypt_ctr(const uint8_t *input, uint8_t *output, size_t length, const uint8_t *nonce) {
    // Initialize AES context with key and nonce
    AES_init_ctx_iv(&ctx, aes_key, nonce);

    // Copy input to output buffer
    memcpy(output, input, length);

    // Encrypt in CTR mode (in-place encryption)
    AES_CTR_xcrypt_buffer(&ctx, output, length);
}

void aes_decrypt_ctr(const uint8_t *input, uint8_t *output, size_t length, const uint8_t *nonce) {
    // CTR mode decryption is the same as encryption
    // Initialize AES context with key and nonce
    AES_init_ctx_iv(&ctx, aes_key, nonce);

    // Copy input to output buffer
    memcpy(output, input, length);

    // Decrypt in CTR mode (in-place decryption)
    AES_CTR_xcrypt_buffer(&ctx, output, length);
}

void aes_generate_nonce(uint8_t *nonce) {
    // Generate random nonce using ESP32 hardware RNG
    esp_fill_random(nonce, AES_BLOCK_SIZE);
}
