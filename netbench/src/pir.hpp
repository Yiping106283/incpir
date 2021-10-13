#ifndef _PIRBASIC
#define _PIRBASIC

#include <vector>
#include <bitset>
#include <set>
#include <array>
#include <tuple>
#include <cstring>

#define KeyLen 16

typedef unsigned long long ulonglong;

typedef std::array<uint8_t, KeyLen> Key;
typedef std::bitset<16000> Block;
typedef std::vector<Block> Database;


typedef struct {
    uint8_t *prf_key;
    uint32_t shift = 0;

    // if prf_key for whole range, then linfo and sinfo size is zero

    std::vector< std::tuple<uint32_t, uint32_t> > linfo; // store indices in the clear (addin, kickout)
    std::vector< std::tuple<uint32_t, uint32_t> > sinfo; // store keys and tuple=(range, points)

    Block hint;

} SetDesc;

typedef struct {
    uint32_t nbrsets;
    uint32_t setsize;
    uint32_t keylen;
    std::vector<Key> offline_keys;
    std::vector<uint32_t> shifts;

    // side info?
} OfflineQuery;

typedef struct {
    std::vector<uint32_t> indices;
    //std::set<uint32_t> indices;
} OnlineQuery;

typedef struct {
    std::set<uint32_t> left_indices;
    uint32_t left_word;
    std::set<uint32_t> right_indices;
    uint32_t right_word;
} NewOnlineQuery;

typedef struct {
    std::set<uint32_t> indices;
} RefreshQuery;

typedef struct {
    uint32_t nbrsets;
    std::vector<Block> hints;
} OfflineReply;

typedef struct {
    Block parity;
} OnlineReply;

typedef struct {
    int setno;
    uint32_t shift;
    Key key;
    std::vector<std::tuple<uint32_t, uint32_t> > prev_side;
    std::vector<std::tuple<uint32_t, uint32_t> > cur_side;

    // cur_side should be one length longer than prev_side
    // for each tuple in prev_side, Eval on points in (cur_side, prev_side]
} DiffSideInfo;

typedef struct {
    uint32_t nbrsets;
    uint32_t setsize;

    uint8_t *master_key;

    std::vector<DiffSideInfo> req;

} OfflineAddQueryShort;

typedef struct {
    std::vector<std::tuple<uint32_t, std::vector<std::tuple<uint32_t, uint32_t> > > > qry;
    // setno, (3, n+1), (7, n+2), ...
} OfflineAddQueryLong;

uint8_t *derive_key(uint8_t *master_key, uint8_t *cset_key, int batch_no);
uint8_t *test_derive_key(uint8_t *master_key, uint8_t *cset_key, int batch_no);
void print_key(uint8_t *key);


#endif
