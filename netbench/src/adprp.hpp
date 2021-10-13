#ifndef _ADPRP
#define _ADPRP

#include <bitset>
#include <stdint.h>
#include <cstring>

#define ROUNDS 7

uint32_t adprf(uint32_t range, uint8_t *key, uint32_t x);

// small range PRF, compute PRF(key, x)
uint32_t small_prf(uint32_t in_range, uint32_t out_range,
        uint8_t *key, uint32_t x);

uint32_t round_function(uint16_t block, uint8_t *key, uint16_t tweak,
        uint32_t input_length, uint32_t output_length);

uint32_t feistel_prp(uint32_t block, uint32_t block_length,
        uint32_t rounds, uint8_t *keys);

uint32_t feistel_inv_prp(uint32_t block, uint32_t block_length,
        uint32_t rounds, uint8_t *keys);

uint32_t cycle_walk(uint32_t num, uint32_t range, uint8_t* key);
uint32_t inv_cycle_walk(uint32_t num, uint32_t range, uint8_t *key);

#endif
