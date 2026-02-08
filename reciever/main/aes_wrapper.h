#ifndef AES_WRAPPER_H
#define AES_WRAPPER_H

#include <stdint.h>
#include <stddef.h>

// AES-128 key size in bytes
#define AES_KEY_SIZE 16

// AES block size in bytes
#define AES_BLOCK_SIZE 16

/**
 * @brief Initialize AES encryption with a 128-bit key
 *
 * @param key Pointer to 16-byte encryption key
 */
void aes_init(const uint8_t *key);

/**
 * @brief Encrypt data using AES-128 CTR mode
 *
 * @param input Pointer to plaintext data
 * @param output Pointer to buffer for encrypted data (must be same size as input)
 * @param length Length of data to encrypt
 * @param nonce Pointer to 16-byte nonce/IV (counter initialization vector)
 */
void aes_encrypt_ctr(const uint8_t *input, uint8_t *output, size_t length, const uint8_t *nonce);

/**
 * @brief Decrypt data using AES-128 CTR mode
 *
 * @param input Pointer to encrypted data
 * @param output Pointer to buffer for decrypted data (must be same size as input)
 * @param length Length of data to decrypt
 * @param nonce Pointer to 16-byte nonce/IV (must be same as used for encryption)
 */
void aes_decrypt_ctr(const uint8_t *input, uint8_t *output, size_t length, const uint8_t *nonce);

/**
 * @brief Generate a random nonce for CTR mode
 *
 * @param nonce Pointer to 16-byte buffer to store generated nonce
 */
void aes_generate_nonce(uint8_t *nonce);

#endif // AES_WRAPPER_H
