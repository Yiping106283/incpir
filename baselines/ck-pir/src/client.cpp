#include "client.hpp"
#include "puncprf.hpp"
#include "pir.hpp"
#include <math.h>
#include <cmath>
#include <random>
#include <openssl/rand.h>
#include <iostream>
using namespace std;

PIRClient::PIRClient() {
    std::cout << "client is created." << std::endl;
}

void PIRClient::set_parms(uint32_t dbrange_, uint32_t setsize_, uint32_t nbrsets_) {
    dbrange = dbrange_;
    lgn = int(log2(dbrange)-1)+1;

    setsize = setsize_;
    nbrsets = nbrsets_;

    punc_x = 0;
    /*if (setsize != (1<<(lgn/2))) {
        cout << setsize << " " << (1<<(lgn/2))<< endl;
        throw invalid_argument("set size not square root n!");
    }*/

}

void find_valid_key(uint8_t *key, int lgn, uint32_t dbrange) {

    // find valid key without collisions
    set<uint32_t> tmp;

    // loop at most 128
    int cnt;
    for(cnt = 0; cnt < 128; cnt++) {
        RAND_bytes(key, KeyLen);

        tmp.clear();

        for (int i = 0; i < (1<<(lgn/2)); i++) {
            uint32_t y = Eval(key, i, lgn, dbrange);
            if (tmp.find(y) != tmp.end()) {
                break; // found collision
            } else {
                tmp.insert(y);
            }
        }

        if (tmp.size() == (1<<(lgn/2))) {
            //cout << "find key success" << endl;
            break;
        }
    }

    if (cnt == 128) {
        throw invalid_argument("can find valid key");
    }
}

void PIRClient::generate_setkeys() {

    // re-gen keys until valid (no collision)

    for(int i = 0; i < nbrsets; i++) {

        SetDesc tmp;

        tmp.prf_key = static_cast<uint8_t *>(malloc(KeyLen));
        memset(tmp.prf_key, 0, KeyLen);

        // re-gen key until valid
        find_valid_key(tmp.prf_key, lgn, dbrange);

        tmp.shift = rand() % dbrange;

        sets.push_back(tmp);
    }

}

void CopyKey(Key &key, uint8_t *prf_key) {

    for (int i = 0; i < KeyLen; i++) {
        key[i] = prf_key[i];
    }
}

OfflineQuery PIRClient::generate_offline_query() {
    OfflineQuery tmp;
    tmp.nbrsets = nbrsets;
    tmp.setsize = setsize;

    for (int i = 0; i < nbrsets; i++) {
        Key key;
        CopyKey(key, sets[i].prf_key);
        tmp.offline_keys.push_back(key);

        tmp.shifts.push_back(sets[i].shift);

    }

    return tmp;
}

void PIRClient::update_local_hints(OfflineReply offline_reply) {
    for (int i = 0; i < offline_reply.nbrsets; i++) {
        sets[i].hint = sets[i].hint ^ offline_reply.hints[i];
    }
}

// refresh query
OnlineQuery PIRClient::generate_refresh_query(uint32_t desired_idx) {

    uint32_t setno = cur_qry_setno;
    OnlineQuery refresh_query;

    // sample a new key
    if (sets[setno].prf_key != NULL)
        free(sets[setno].prf_key);

    // generate new key
    sets[setno].prf_key = static_cast<uint8_t *>(malloc(KeyLen));
    RAND_bytes(sets[setno].prf_key, KeyLen);

    // add shift
    uint32_t r = rand() % setsize;
    uint32_t y = Eval(sets[setno].prf_key, r, lgn, dbrange);
    sets[setno].shift = (desired_idx + dbrange - y) % dbrange; // shift is positive number

    // TODO probabilistic punc

    PuncKeys punckeys = Punc(sets[setno].prf_key, punc_x, lgn);

    refresh_query.height = punckeys.height;
    refresh_query.bitvec = punckeys.bitvec;
    refresh_query.keys = punckeys.keys;
    refresh_query.shift = sets[setno].shift;

    return refresh_query;

}

OnlineQuery PIRClient::generate_online_query(uint32_t desired_idx) {

    OnlineQuery online_query;

    // (use PRF to find one by one)

    // find a set
    int setno = -1;
    //uint32_t punc_x = 0;

    for (int s = 0; s < nbrsets; s++) {

        for (int i = 0; i < (1<<(lgn/2)); i++) {

            uint32_t y = Eval(sets[s].prf_key, i, lgn, dbrange);

            if ((y+sets[s].shift)%dbrange == desired_idx) {
                punc_x = i;
                setno = s;
                goto Found;
            }
        }
    }

    Found:
    if (setno == -1) {
        throw std::invalid_argument("cannot find desired index!");
    }

    cur_qry_setno = setno;


    // TODO: probabilistic

    // punc the set key

    PuncKeys punckeys = Punc(sets[setno].prf_key, punc_x, lgn);

    online_query.height = punckeys.height;
    online_query.bitvec = punckeys.bitvec;
    online_query.keys = punckeys.keys;
    online_query.shift = sets[setno].shift;

    return online_query;
}

Block PIRClient::recover_block(OnlineReply online_reply) {
    int setno = cur_qry_setno;
    Block blk = online_reply.parity ^ sets[setno].hint;
    return blk;
}