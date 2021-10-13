#include "puncprf.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <random>
#include <iostream>
using namespace std;

unsigned char iv[16] = {0};

void print_key(uint8_t *key) {
    if (key == NULL)
        return;
    for (int i = 0; i < KeyLen; i++) {
        printf("%x ", key[i]);
    }
    printf("\n");
}

// Single-AES call PRG

void NewCopyKey(Key &key, uint8_t *ptr) {

    if (ptr == NULL) throw invalid_argument("CopyKey: key is null");

    for (int i = 0; i < KeyLen; i++) {
        key[i] = ptr[i];
    }
}

// breadth-first Eval (obtain PRF(k, 1), ... , PRF(k, s) in one-shot)
vector<uint32_t> BreadthEval(uint8_t *rootkey, int low, int high,
        uint32_t lgn, uint32_t range) {
    // low should be 0
    // high should be s (i.e., (1,,(lgn/2)))
    vector<uint32_t> vec(high-low);

    /*cout << "root key = ";
    print_key(rootkey);*/

    // all leaves of a proper left tree (need to find that)
    // traverse level by level
    // release the previous level after getting the current level node label

    uint32_t subheight = lgn/2;

    // eval from root to subheight, get that node label (go straight left)
    //uint8_t *subroot = static_cast<uint8_t *>(malloc(KeyLen));
    //memcpy(subroot, rootkey, KeyLen);

    Key subroot;
    for (int i = 0; i < KeyLen; i++) {
        subroot[i] = rootkey[i];
    }

    for (int i = 0; i < subheight; i++) {
        subroot = get<0>(PRG(subroot));
        //memcpy(subroot, get<0>(PRG(subroot)), KeyLen);
    }
    // now subroot is the node label

    /*cout << "subroot = ";
    print_key(subroot);*/

    vector<Key> prev_nodes; // node labels at current level
    prev_nodes.push_back(subroot);

    vector<Key> cur_nodes;

    //cout << "prev_node size = " << prev_nodes.size() << endl;

    Key keyb;

    for (int i = subheight; i < lgn; i++) {

        if (prev_nodes.size() != (1<<(i-subheight))) {
            throw invalid_argument("vec size error");
        }

        for (int b = 0; b < (1<<(i-subheight)); b++) {
            // fetch one key from vec,
            // expand to two keys,
            // put the two keys into the current vec pool

            //memcpy(keyb, prev_nodes[b], KeyLen);
            keyb = prev_nodes[b];

            tuple<Key, Key> derived_keys = PRG(keyb);

            cur_nodes.push_back(get<0>(derived_keys));
            cur_nodes.push_back(get<1>(derived_keys));

        }

        prev_nodes = cur_nodes;
        cur_nodes.clear();

    }

    /*if (prev_nodes.size() != (1<<(lgn/2))) {
        cout << (1<<(lgn/2)) << endl;
        cout << prev_nodes.size() << endl;
        throw invalid_argument("fast eval results err");
    }*/

    // output eval results in prev_nodes;

    for (int i = 0; i < vec.size(); i++) {
        uint32_t res = 0;

        /*for (int pos = 0; pos < 4; pos++) {
            res <<= 8;

            res |= prev_nodes[i][pos];
        }
        res &= ((1<<lgn)-1);*/

        unsigned long long v = 0;

        for ( unsigned ui = 0 ; ui < 8 ; ++ui ) {
            v <<= 8;
            v |=  prev_nodes[i][ui];
        }

        for (uint32_t j = 0; j < 64; j++) {
            unsigned long long slide = (((1<<lgn)-1) << j);
            if (((v & slide) >> j) < range) {
                res = uint32_t(((v & slide) >> j));
                break;
            }
        }

        vec[i] = res;
    }


    //free(subroot);

    return vec;
}


// TODO optimize Eval
tuple<Key, Key> PRG (Key stkey) {

    uint8_t *plaintext;
    plaintext = static_cast<uint8_t *>(malloc(KeyLen));
    for (int i = 0; i < KeyLen; i++) {
        plaintext[i] = stkey[i];
    }
    //memcpy(plaintext, key, KeyLen);

    int outlen;
    uint8_t outbuf[KeyLen];

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX_init(ctx);
    EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, plaintext, iv);

    Key key_left, key_right;

    EVP_EncryptUpdate(ctx, outbuf, &outlen, plaintext, KeyLen);
    NewCopyKey(key_left, outbuf);

    EVP_EncryptUpdate(ctx, outbuf, &outlen, plaintext, KeyLen);
    NewCopyKey(key_right, outbuf);

    free(plaintext);
    if (ctx != NULL) EVP_CIPHER_CTX_free(ctx);

    return make_tuple(key_left, key_right);

}

// range must <= lgn
uint32_t Eval (uint8_t *key, uint32_t x, uint32_t lgn, uint32_t range) {

    if (x >= (1<<lgn))
        throw invalid_argument("Eval input invalid");

    // extract bits in x, and decide whether go left or right

    //uint8_t *node_key = static_cast<uint8_t *>(malloc(KeyLen));
    //memset(node_key, 0, KeyLen);

    Key node_key;
    for (int i = 0; i < KeyLen; i++) {
        node_key[i] = key[i];
    }

    //memcpy(node_key, key, KeyLen);

   /* cout << "check memcpy:" << endl;
    print_key(key);
    print_key(node_key);*/

    for (int i = 1; i <= lgn; i++) {
        int cur_bit = (x>>(lgn-i) & 1);
        //cout << cur_bit ;

        if (cur_bit != 0 && cur_bit != 1)
            throw invalid_argument("error");

        tuple<Key, Key> derived_keys = PRG(node_key);

        if (cur_bit == 0) {
            // go left, get left pseudorandom label
            //memcpy(node_key, get<0>(derived_keys), KeyLen);
            node_key = get<0>(derived_keys);

        } else {
            // go right, get right pseudorandom label
            //memcpy(node_key, get<1>(derived_keys), KeyLen);
            node_key = get<1>(derived_keys);
        }
    }

    // use node_key to find lgn bits values in range
    unsigned long long v = 0;

    for ( unsigned ui = 0 ; ui < 8 ; ++ui ) {
        v <<= 8;
        auto tmp = (unsigned long) node_key[ui];
        //td::bitset<64> bTmp { (unsigned long) outbuf[ui] };

        v |= tmp;
    }

    uint32_t res = 0;

    for (uint32_t i = 0; i < 64; i++) {
        unsigned long long slide = (((1<<lgn)-1) << i);
        if (((v & slide) >> i) < range) {
            res = uint32_t(((v & slide) >> i));
            break;
        }
    }


    // truncate node_key to lgn bits
    /*uint32_t res = 0;
    for (int i = 0; i < 4; i++) {
        res <<= 8;
        //printf("node_key: %x,", node_key[i]);
        res |= node_key[i];
        //printf("res: %d\n", res);
    }
    res &= ((1<<lgn)-1);
     */

    return res;
}


int EvalPunc (PuncKeys punc_keys, uint32_t x, uint32_t lgn, uint32_t range) {

    // we know what point is punctured given punc_keys
    uint32_t punc_point = punc_keys.bitvec;
    //cout << "punc point = " << punc_point << endl;

    if (x == punc_keys.bitvec) {
        return -1; // input point being punctured, can't be evaluated
    }

    // parse x from lgn-1 bits, to 0 bits
    // determine the path

    uint8_t *cur_key = static_cast<uint8_t *>(malloc(KeyLen));
    memset(cur_key, 0, KeyLen);

    int cur_pos = 1;

    for (int i = 1; i <= lgn; i++) {

        // get x's current bit
        int bitvec_cur = (punc_keys.bitvec & (1<<(lgn-i)));
        int x_cur = (x & (1<<(lgn-i)));

        if (x_cur != bitvec_cur) {
            // if x's current bit differs from bitvec current bit
            // then we take the current key
            // and eval (traverse down) the rest of the bits of x

            memcpy(cur_key, punc_keys.keys[i-1], KeyLen);

            /*cout << "EvalPunc: level " << i << ": ";
            print_key(cur_key);*/

            break;
        }

        cur_pos++;
    }


    // let the cur_key be the "root" key and eval the rest bits of x

    //uint8_t *node_key = static_cast<uint8_t *>(malloc(KeyLen));
    //memset(node_key, 0, KeyLen);

    Key node_key;
    for (int i = 0; i < KeyLen; i++){
        node_key[i] = cur_key[i];
    }

    //memcpy(node_key, cur_key, KeyLen);


    for (int i = cur_pos+1; i <= lgn; i++) {

        int cur_bit = ((x>>(lgn-i)) & 1);

        if (cur_bit != 0 && cur_bit != 1)
            throw invalid_argument("error");

        tuple<Key, Key> derived_keys = PRG(node_key);

        /*cout << "check derived keys: " ;
        print_key(get<0>(derived_keys));
        cout << "cur_bit = " << cur_bit << endl;*/

        if (cur_bit == 0) {
            // go left, get left pseudorandom label
            //memcpy(node_key, get<0>(derived_keys), KeyLen);

            node_key = get<0>(derived_keys);

        } else {
            // go right, get right pseudorandom label
            //memcpy(node_key, get<1>(derived_keys), KeyLen);
            node_key = get<1>(derived_keys);
        }
    }
    free(cur_key);
    // here we should get the pseudorandom label at leaf

    // use node_key to find lgn bits values in range
    unsigned long long v = 0;

    for ( unsigned ui = 0 ; ui < 8 ; ++ui ) {
        v <<= 8;
        auto tmp = (unsigned long) node_key[ui];
        //td::bitset<64> bTmp { (unsigned long) outbuf[ui] };

        v |= tmp;
    }

    uint32_t res = 0;

    for (uint32_t i = 0; i < 64; i++) {
        unsigned long long slide = (((1<<lgn)-1) << i);
        if (((v & slide) >> i) < range) {
            res = uint32_t(((v & slide) >> i));
            break;
        }
    }


    // truncate node_key to lgn bits
    /*uint32_t res = 0;
    for (int i = 0; i < 4; i++) {
        res <<= 8;
        //printf("node_key: %x,", node_key[i]);
        res |= node_key[i];
        //printf("res: %d\n", res);
    }

    res &= ((1<<lgn)-1);*/

    return res;

}


// generate punturable keys
PuncKeys Punc(uint8_t *key, uint32_t punc_x, uint32_t lgn) {
    uint32_t punc_number = punc_x;
    PuncKeys punckeys;

    punckeys.height = lgn;

    //uint8_t *node_key = static_cast<uint8_t *>(malloc(KeyLen));
    //memset(node_key, 0, KeyLen);
    Key node_key;
    for (int i = 0; i < KeyLen; i++) {
        node_key[i] = key[i];
    }

    //memcpy(node_key, key, KeyLen);

    for (int i = 1; i <= lgn; i++) {
        // for each level, derive punc keys
        // each punc key includes a uint8_t* key and a level number

        tuple<Key, Key> derived_keys = PRG(node_key);

        if ((1<<lgn)/(1<<i) <= punc_number) {
            punc_number -= (1<<lgn)/(1<<i);

            // extract left node
            uint8_t *dleft = static_cast<uint8_t *>(malloc(KeyLen));
            for (int kidx = 0; kidx < KeyLen; kidx++) {
                dleft[kidx] = get<0>(derived_keys)[kidx];
            }
            punckeys.keys.push_back(dleft);

            // go to right tree
            // i.e., set node_key to right_key
            //memcpy(node_key, get<1>(derived_keys), KeyLen);
            node_key = get<1>(derived_keys);

            // going left is puncturing the right point
            // set bit to 1
            punckeys.bitvec |= (1<<(lgn-i));

        } else {

            // punc_number remains the same

            // extract right node
            uint8_t *dright = static_cast<uint8_t *>(malloc(KeyLen));
            for (int kidx = 0; kidx < KeyLen; kidx++) {
                dright[kidx] = get<1>(derived_keys)[kidx];
            }
            punckeys.keys.push_back(dright);

            // go to left tree, set node_key to left_key
            //memcpy(node_key, get<0>(derived_keys), KeyLen);
            node_key = get<0>(derived_keys);

        }

    }

//    free(node_key);

    // y size = " << punckeys.keys.size() << endl;

    return punckeys;
}


