#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Módulos del proyecto
#include "handlers.h"
#include "error.h"
#include "bmp_lib.h"
#include "steganography/embed_utils.h"
#include "steganography/steganography.h"
#include "cryptography/crypto.h"
#include "steganography/extract_utils.h"

int handle_embed_mode(const ProgramArgs *args) {
    BMPImage *image = NULL;
    unsigned char *secret_buffer = NULL; // (real size || data || ext)
    int result = NO_SUCCESS;

    size_t buffer_len_bytes = 0;
    size_t required_bits = 0;
    int bits_per_pixel = 0;

    image = open_bmp(args->bitmap_file);
    if (!image) {
        goto cleanup;
    }

    // Build non-encrypted secret buffer
    secret_buffer = build_secret_buffer(args->input_file, &buffer_len_bytes);
    if (!secret_buffer) {
        goto cleanup;
    }


    // encryption logic
    if (prepare_encryption(args, &secret_buffer, &buffer_len_bytes) != SUCCESS) {
        goto cleanup;
    }


    if (strcmp(args->steg_algorithm, "LSB1") == 0) {
        bits_per_pixel = LSB1_BITS_PER_PIXEL;
        required_bits = buffer_len_bytes * 8;
    } else if (strcmp(args->steg_algorithm, "LSB4") == 0) {
        bits_per_pixel = LSB4_BITS_PER_PIXEL;
        required_bits = buffer_len_bytes * 8;
    } else if (strcmp(args->steg_algorithm, "LSBI") == 0) {
        bits_per_pixel = LSB1_BITS_PER_PIXEL;
        required_bits = (buffer_len_bytes * 8) + LSBI_CONTROL_BITS;
    } else {
        fprintf(stderr, ERR_INVALID_STEG_ALGORITHM, args->steg_algorithm);
        goto cleanup;
    }

    if (check_bmp_capacity(image, required_bits, bits_per_pixel) != TRUE) {
        goto cleanup;
    }

    FILE *out_fp = fopen(args->output_file, "wb");
    if (!out_fp) {
        perror(args->output_file);
        goto cleanup;
    }
    image->out = out_fp;

    if (strcmp(args->steg_algorithm, "LSB1") == 0) {
        if (embed_lsb1(image, secret_buffer, buffer_len_bytes) == 0) {
            result = SUCCESS;
        }
    } else if(strcmp(args->steg_algorithm, "LSB4") == 0) {
        if (embed_lsb4(image, secret_buffer, buffer_len_bytes) == 0) {
            result = SUCCESS;
        }
    } else if (strcmp(args->steg_algorithm, "LSBI") == 0) {
        if (embed_lsbi(image, secret_buffer, buffer_len_bytes) == 0) {
            result = SUCCESS;
        }
    }


    cleanup:
    free_secret_buffer(secret_buffer);

    if (image) {
        free_bmp_image(image);
    }

    return result;
}


int prepare_encryption(const ProgramArgs *args, unsigned char **secret_buffer_ptr, size_t *buffer_len_bytes_ptr) {
    if (!args->password) {
        return SUCCESS; // No encryption needed
    }

    printf("Encrypting data...\n");

    unsigned char *encrypted_data = NULL;
    unsigned char *final_buffer = NULL;
    int result = NO_SUCCESS;

    // get cypher function from openssl library
    const EVP_CIPHER *cipher = get_evp_cipher(args->encryption_algo, args->mode);
    if (!cipher) {
        goto cleanup_enc;
    }

    // derive key and iv from password and retrieved cipher
    unsigned char key_iv_buffer[KEY_IV_LEN];
    if (derive_key_iv_pbkdf2(args->password, cipher, key_iv_buffer) != 0) {
        goto cleanup_enc;
    }
    const unsigned char *key = key_iv_buffer;
    const unsigned char *iv = key_iv_buffer + EVP_CIPHER_key_length(cipher);

    // encrypt
    int encrypted_len = 0;
    encrypted_data = encrypt_data(*secret_buffer_ptr, *buffer_len_bytes_ptr, cipher, key, iv, &encrypted_len);
    if (!encrypted_data) {
        fprintf(stderr, "Error: Encryption failed.\n");
        goto cleanup_enc;
    }

    // final buffer: (encrypted size || encrypted data)
    size_t final_buffer_len = sizeof(uint32_t) + encrypted_len;
    final_buffer = malloc(final_buffer_len);
    if (!final_buffer) {
        fprintf(stderr, "Error: Failed to allocate memory for final encrypted buffer.\n");
        goto cleanup_enc;
    }

    write_size_header(final_buffer, (long)encrypted_len); // size in big-endian !
    memcpy(final_buffer + sizeof(uint32_t), encrypted_data, encrypted_len);

    free_secret_buffer(*secret_buffer_ptr); // free original buffer
    *secret_buffer_ptr = final_buffer;      // point to final buffer w/ encrypted data ;)
    *buffer_len_bytes_ptr = final_buffer_len;

    final_buffer = NULL;
    result = SUCCESS;

cleanup_enc:
    if (encrypted_data) {
        free(encrypted_data);
    }
    return result;
}


int handle_extract_mode(const ProgramArgs *args) {
    BMPImage *image = NULL;
    unsigned char *extracted_buffer = NULL; // Buffer: (encrypted data) or (real data || ext)
    unsigned char *decrypted_buffer = NULL; // Buffer: (size || real data || ext)
    int result = NO_SUCCESS;
    size_t extracted_len = 0;

    image = open_bmp(args->bitmap_file);
    if (!image) {
        goto cleanup_ext;
    }

    if (strcmp(args->steg_algorithm, "LSB1") == 0) {
        extracted_buffer = lsb1_extract(image, &extracted_len);
    } else {
        fprintf(stderr, "Error: Steganography algorithm '%s' not supported for extraction.\n", args->steg_algorithm);
        goto cleanup_ext;
    }

    if (!extracted_buffer) {
        fprintf(stderr, "Error: Failed to extract data from BMP image.\n");
        goto cleanup_ext;
    }

    // decryption logic
    if (args->password) {
        printf("Decrypting data...\n");

        const EVP_CIPHER *cipher = get_evp_cipher(args->encryption_algo, args->mode);
        if (!cipher) goto cleanup_ext;

        // derive key and iv from password and retrieved cipher
        unsigned char key_iv_buffer[KEY_IV_LEN];
        if (derive_key_iv_pbkdf2(args->password, cipher, key_iv_buffer) != 0) {
            goto cleanup_ext;
        }
        const unsigned char *key = key_iv_buffer;
        const unsigned char *iv = key_iv_buffer + EVP_CIPHER_key_length(cipher);

        // decrypt
        int decrypted_len = 0;
        decrypted_buffer = decrypt_data(extracted_buffer, extracted_len, cipher, key, iv, &decrypted_len);
        if (!decrypted_buffer) {
            // El error ya se imprimió en decrypt_data
            goto cleanup_ext;
        }

        if (write_secret_from_buffer(args->output_file, decrypted_buffer, decrypted_len) == 0) {
            result = SUCCESS;
        }

    } else {
        // No encryption, write directly
        size_t total_buffer_len = sizeof(uint32_t) + extracted_len;
        unsigned char *full_buffer = malloc(total_buffer_len);
        if (!full_buffer) goto cleanup_ext;

        write_size_header(full_buffer, (long)extracted_len);
        memcpy(full_buffer + sizeof(uint32_t), extracted_buffer, extracted_len);

        if (write_secret_from_buffer(args->output_file, full_buffer, total_buffer_len) == 0) {
            result = SUCCESS;
        }
        free(full_buffer);
    }

cleanup_ext:
    if (extracted_buffer) free(extracted_buffer);
    if (decrypted_buffer) free(decrypted_buffer);
    if (image) {
        free_bmp_image(image);
    }
    return result;
}