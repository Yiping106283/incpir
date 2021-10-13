#include "pir.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <random>
#include <iostream>
using namespace std;

void print_key(uint8_t *key) {




    if (key == NULL) return;
    for (int i = 0; i < KeyLen; i++) {
        printf("%x ", key[i]);
    }
    printf("\n");
}

uint8_t *derive_key(uint8_t *master_key, uint8_t *cset_key, int batch_no) {

    if (master_key == NULL)
        throw invalid_argument("master key is null");

    if (cset_key == NULL)
        throw invalid_argument("set key is null");

    uint8_t *plaintext;
    plaintext = static_cast<uint8_t *>(malloc(16));
    memset(plaintext, 0, 16);

    memcpy(plaintext, cset_key, KeyLen);
    plaintext[0] ^= (batch_no & 0xFF);

    int outlen;
    static uint8_t outbuf[16];

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX_init(ctx);
    EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, master_key, NULL);

    EVP_EncryptUpdate(ctx, outbuf, &outlen, plaintext, 16);

    /*for (int i = 0; i < 16; i++) {
        printf("%x ", outbuf[i]);
    }*/

    if (plaintext) free(plaintext);
    if (ctx) EVP_CIPHER_CTX_free(ctx);
    return outbuf;

}


uint8_t *test_derive_key(uint8_t *master_key, uint8_t *cset_key, int batch_no) {

    uint8_t *plaintext;
    plaintext = static_cast<uint8_t *>(malloc(16));
    memset(plaintext, 0, 16);


    int outlen;
    static uint8_t outbuf[16];

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX_init(ctx);
    EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, master_key, NULL);

    EVP_EncryptUpdate(ctx, outbuf, &outlen, plaintext, 16);

    for (int i = 0; i < 16; i++) {
        printf("%x ", outbuf[i]);
    }

    return outbuf; //plaintext;

}
