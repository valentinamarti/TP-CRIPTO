#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/err.h>
#include <string.h>
#include <stdio.h>
#include "crypto.h"

const EVP_CIPHER *get_evp_cipher(const char *algo, const char *mode) {
    // defaults if password passed but not a specific mode or algorithm
    const char *a = (algo == NULL) ? "aes128" : algo;
    const char *m = (mode == NULL) ? "cbc" : mode;

    if (strcmp(a, "aes128") == 0) {
        if (strcmp(m, "ecb") == 0) return EVP_aes_128_ecb();
        if (strcmp(m, "cbc") == 0) return EVP_aes_128_cbc();
        if (strcmp(m, "cfb") == 0) return EVP_aes_128_cfb8(); // 8 bits
        if (strcmp(m, "ofb") == 0) return EVP_aes_128_ofb(); // 128 bits
    } else if (strcmp(a, "aes192") == 0) {
        if (strcmp(m, "ecb") == 0) return EVP_aes_192_ecb();
        if (strcmp(m, "cbc") == 0) return EVP_aes_192_cbc();
        if (strcmp(m, "cfb") == 0) return EVP_aes_192_cfb8();
        if (strcmp(m, "ofb") == 0) return EVP_aes_192_ofb();
    } else if (strcmp(a, "aes256") == 0) {
        if (strcmp(m, "ecb") == 0) return EVP_aes_256_ecb();
        if (strcmp(m, "cbc") == 0) return EVP_aes_256_cbc();
        if (strcmp(m, "cfb") == 0) return EVP_aes_256_cfb8();
        if (strcmp(m, "ofb") == 0) return EVP_aes_256_ofb();
    } else if (strcmp(a, "3des") == 0) {
        // des ede3 = 3DES con 3 claves
        if (strcmp(m, "ecb") == 0) return EVP_des_ede3_ecb();
        if (strcmp(m, "cbc") == 0) return EVP_des_ede3_cbc();
        if (strcmp(m, "cfb") == 0) return EVP_des_ede3_cfb8();
        if (strcmp(m, "ofb") == 0) return EVP_des_ede3_ofb();
    }

    fprintf(stderr, "Error: AAlgorithm/mode combination not supported ('%s'/'%s').\n", a, m);
    return NULL;
}

int derive_key_iv_pbkdf2(const char *password, const EVP_CIPHER *cipher, unsigned char *key_iv_buffer) {
    int key_len = EVP_CIPHER_key_length(cipher); // library functions
    int iv_len = EVP_CIPHER_iv_length(cipher);
    int total_len = key_len + iv_len;

    if (PKCS5_PBKDF2_HMAC(password, strlen(password),
                          FIXED_SALT, FIXED_SALT_LEN,
                          10000, // iterations
                          EVP_sha256(), // hash function (SHA-256) - la que se usó en los archivos de la cátedra
                          total_len, key_iv_buffer) == 0) {
        fprintf(stderr, "Error: Falló la derivación de clave PBKDF2.\n");
        return -1;
    }
    return 0;
}

unsigned char *encrypt_data(const unsigned char *plaintext, int plaintext_len, const EVP_CIPHER *cipher, const unsigned char *key, const unsigned char *iv, int *ciphertext_len) {
    EVP_CIPHER_CTX *ctx; // encryption operation current state
    int len;
    int ct_len;

    // output buffer size: block size for padding
    unsigned char *ciphertext = malloc(plaintext_len + EVP_CIPHER_block_size(cipher));
    if (!ciphertext)
        return NULL;

    if (!((ctx = EVP_CIPHER_CTX_new()))) {
        ERR_print_errors_fp(stderr);
        free(ciphertext);
        return NULL;
    }

    // initialize encryption operation via context
    if (1 != EVP_EncryptInit_ex(ctx, cipher, NULL, key, iv)) {
        ERR_print_errors_fp(stderr);
        EVP_CIPHER_CTX_free(ctx);
        free(ciphertext);
        return NULL;
    }

    // encrypt
    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len)) {
        ERR_print_errors_fp(stderr);
        EVP_CIPHER_CTX_free(ctx);
        free(ciphertext);
        return NULL;
    }
    ct_len = len;

    // finalize encryption
    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) {
        ERR_print_errors_fp(stderr);
        EVP_CIPHER_CTX_free(ctx);
        free(ciphertext);
        return NULL;
    }
    ct_len += len;

    *ciphertext_len = ct_len;
    EVP_CIPHER_CTX_free(ctx);
    return ciphertext;
}

unsigned char *decrypt_data(const unsigned char *ciphertext, int ciphertext_len, const EVP_CIPHER *cipher, const unsigned char *key, const unsigned char *iv, int *plaintext_len) {
    EVP_CIPHER_CTX *ctx;
    int len;

    // output buffer size: ciphertext_len (max) since padding is removed
    unsigned char *plaintext = malloc(ciphertext_len);
    if (!plaintext) return NULL;

    if (!((ctx = EVP_CIPHER_CTX_new()))) {
        ERR_print_errors_fp(stderr);
        free(plaintext);
        return NULL;
    }

    // initialize decryption operation
    if (1 != EVP_DecryptInit_ex(ctx, cipher, NULL, key, iv)) {
        ERR_print_errors_fp(stderr);
        EVP_CIPHER_CTX_free(ctx);
        free(plaintext);
        return NULL;
    }

    // decrypt
    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) {
        ERR_print_errors_fp(stderr);
        EVP_CIPHER_CTX_free(ctx);
        free(plaintext);
        return NULL;
    }
    int pt_len = len;

    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) {
        fprintf(stderr, "Error: Decryption failed. Possible wrong password or corrupted data.\n");
        EVP_CIPHER_CTX_free(ctx);
        free(plaintext);
        return NULL;
    }
    pt_len += len;

    *plaintext_len = pt_len;
    EVP_CIPHER_CTX_free(ctx);
    return plaintext;
}