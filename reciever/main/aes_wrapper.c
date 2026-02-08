#include "aes_wrapper.h"
#include "aes.h"
#include <string.h>
#include "esp_system.h"
#include "esp_random.h"
#include "mbedtls/md.h"

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

void compute_hmac_sha256(const uint8_t *data, size_t data_len,
                         const uint8_t *key, size_t key_len,
                         uint8_t *hmac) {
    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t *info;

    // Initialize mbedtls MD context
    mbedtls_md_init(&ctx);
    info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);

    // Setup HMAC with SHA256
    mbedtls_md_setup(&ctx, info, 1); // 1 = HMAC mode
    mbedtls_md_hmac_starts(&ctx, key, key_len);
    mbedtls_md_hmac_update(&ctx, data, data_len);
    mbedtls_md_hmac_finish(&ctx, hmac);

    // Free context
    mbedtls_md_free(&ctx);
}

bool verify_hmac_sha256(const uint8_t *data, size_t data_len,
                        const uint8_t *key, size_t key_len,
                        const uint8_t *received_hmac) {
    uint8_t computed_hmac[HMAC_SIZE];

    // Compute HMAC for the received data
    compute_hmac_sha256(data, data_len, key, key_len, computed_hmac);

    // Constant-time comparison to prevent timing attacks
    int result = 0;
    for (int i = 0; i < HMAC_SIZE; i++) {
        result |= computed_hmac[i] ^ received_hmac[i];
    }

    return (result == 0);
}
