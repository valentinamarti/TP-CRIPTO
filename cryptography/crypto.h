#ifndef CRYPTO_H
#define CRYPTO_H

#include <openssl/evp.h>
#include <stddef.h>

// fixed salt as instructed
#define FIXED_SALT "0000000000000000"
#define KEY_IV_LEN 64 // Key + IV

/**
 * @brief Encryption algorithm and mode mapping.
 * @param mode (ecb, cbc, cfb, ofb) or NULL as default.
 * @return const EVP_CIPHER* or NULL if not found.
 */
const EVP_CIPHER *get_evp_cipher(const char *algo, const char *mode);

/**
 * @brief Derives the Key and Initialization Vector (IV) using PBKDF2.
 * *@param password
 * @param key_iv_buffer Output buffer stores key and IV.
 * @param cipher tpe.
 * @return 0 on success, -1 on error.
 */
int derive_key_iv_pbkdf2(const char *password, const EVP_CIPHER *cipher, unsigned char *key_iv_buffer);

/**
 * @brief Encrypts a data buffer.
 * * @param plaintext Buffer with data to be encrypted.
 * @param plaintext_len Length of the data.
 * @param cipher EVP cipher type.
 * @param key Pointer to the key.
 * @param iv Pointer to the IV.
 * @param ciphertext_len Pointer to store the length of the encrypted text (includes padding).
 * @return Pointer to the buffer with the encrypted data (must be freed by the caller) or NULL on error.
 */
unsigned char *encrypt_data(const unsigned char *plaintext, int plaintext_len, const EVP_CIPHER *cipher, const unsigned char *key, const unsigned char *iv, int *ciphertext_len);

/**
 * @brief Decrypts a data buffer.
 * @param ciphertext Buffer with data to be decrypted.
 * @param ciphertext_len Length of the data to be decrypted.
 * @param cipher EVP cipher type.
 * @param key Pointer to the key.
 * @param iv Pointer to the IV.
 * @param plaintext_len Pointer to store the length of the decrypted text.
 * @return Pointer to the buffer with the decrypted data (must be freed by the caller) or NULL on error.
 */
unsigned char *decrypt_data(const unsigned char *ciphertext, int ciphertext_len, const EVP_CIPHER *cipher, const unsigned char *key, const unsigned char *iv, int *plaintext_len);

#endif // CRYPTO_H