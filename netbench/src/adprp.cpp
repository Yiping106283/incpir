#include "adprp.hpp"

#include <openssl/evp.h>

#include <cmath>
#include <bitset>
#include <iostream>
#include <string>

uint32_t small_prf(uint32_t in_bits, uint32_t out_bits,
                uint8_t  *key, uint32_t x) {

    if (in_bits != out_bits || x >= (1<<in_bits)) {
        throw std::invalid_argument("small prf range error");
    }

    uint8_t *plaintext;
    plaintext = static_cast<uint8_t *>(malloc(16));
    memset(plaintext, 0, 16);
    for (int i = 0; i < 4; i++) {
        plaintext[i] = uint8_t((0xFF<<(8*i) & x)>>(8*i));
    }

    int outlen;
    uint8_t outbuf[16];

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX_init(ctx);
    EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, NULL);

    EVP_EncryptUpdate(ctx, outbuf, &outlen, plaintext, 16);


    // turn outbuf to uint32_t
    uint32_t res = 0;
    for (int i = 0; i < 4; i++) {
       res <<= 8;
       res ^= outbuf[i];
    }

    res = res & ((1<<out_bits) - 1);

    return res;
}



uint32_t round_function(uint16_t block, uint8_t *key, uint16_t tweak,
                        uint32_t input_length, uint32_t output_length) {

    // TODO: after change to uint32_t, related things should be changed

    block = block ^ tweak;

    uint8_t *plaintext;
    plaintext = static_cast<uint8_t *>(malloc(16));
    memset(plaintext, 0, 16);
    plaintext[14] = (uint8_t) ((block>>8) & 0xFF);
    plaintext[15] = (uint8_t) (block & 0xFF);

    int outlen;
    uint8_t outbuf[16];

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX_init(ctx);
    EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, NULL);

    EVP_EncryptUpdate(ctx, outbuf, &outlen, plaintext, 16);

    uint32_t res = outbuf[14];
    res = res << 8;
    res = res | outbuf[15];
    res = res & ((1<<output_length)-1);

    free(plaintext);
    if (ctx != NULL) EVP_CIPHER_CTX_free(ctx);

    return res;

}


/**
 * @param block: input of PRP, of length at most 32 bits
 * @param block_length: input block length n, where n is the smallest number s.t. 2^n > PRP range
 * @param rounds: feistel network rounds
 * @param keys: subkeys array
 * @return
 */
uint32_t feistel_prp(uint32_t block, uint32_t block_length,
        uint32_t rounds, uint8_t *keys) {

    uint32_t left_length = block_length/2;
    uint32_t right_length =  block_length - left_length;

    // split left and right

    uint32_t left = (block>>right_length) & ((1<<left_length)-1);
    uint32_t right = ((1<<right_length)-1) & block;


    uint32_t left1, right1;
    uint32_t perm_block;

    for (int i = 0; i < rounds; i++) {

        left1 = right;
        right1 = left ^ round_function(right, keys, i+1, right_length, left_length);

        // concat left and right

        // re-assign left and right

        if (i == rounds - 1) {

            perm_block = (left1<<left_length) | right1;

        } else {

            perm_block = (left1<<left_length) | right1;

            left = perm_block>>right_length & ((1<<left_length)-1);
            right = perm_block & ((1<<right_length )- 1);

        }
    }


    return perm_block;
}


uint32_t feistel_inv_prp(uint32_t perm_block, uint32_t block_length,
        uint32_t rounds, uint8_t *keys) {

    uint32_t right_length = block_length/2;
    uint32_t left_length = block_length - right_length;

    uint32_t right = perm_block & ((1<<right_length)-1);
    uint32_t left = (perm_block>>right_length) & ((1<<left_length)-1);

    uint32_t left1, right1;
    uint32_t block;

    for (int i = 0; i < rounds; i++) {

        right1 = left;
        left1 = right ^ round_function(left, keys, rounds-i, left_length, right_length);

        if (i == rounds - 1) {

            block = (left1<<left_length) | right1;

        } else {
            block = (left1<<left_length) | right1;

            left = (block>>right_length) & ((1<<left_length)-1);
            right = block & ((1<<right_length)-1);

        }
    }

    return block;
}

uint32_t cycle_walk(uint32_t num, uint32_t range, uint8_t *key) {

    if (num >= range) {
        std::cout << "error:" << num << " , " << range << std::endl;
        throw std::invalid_argument("PRP input is invalid");
    }
    // compute the smallest n s.t. 2^n>range

    uint32_t cnt = log2(range) + 1;

    uint32_t tmp = feistel_prp(num, cnt, ROUNDS, key);

    while(tmp >= range) {
        tmp = feistel_prp(tmp, cnt, ROUNDS, key);
    }

    return tmp;
}

uint32_t inv_cycle_walk(uint32_t num, uint32_t range, uint8_t *key) {

    if (num >= range) {
        std::cout << "error:" << num << " , " << range << std::endl;
        throw std::invalid_argument("PRP input is invalid");
    }

    uint32_t cnt = log2(range) + 1;

    uint32_t tmp = feistel_inv_prp(num, cnt, ROUNDS, key);

    while(tmp >= range) {
        tmp = feistel_inv_prp(tmp, cnt, ROUNDS, key);
    }

    return tmp;
}
