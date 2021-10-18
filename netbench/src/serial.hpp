#include "interface.pb.h"
#include <iostream>
#include <math.h>
#include <ctime>
#include <string>
#include <sstream>
#include <cstring>
#include <array>
#include <chrono>

#include <openssl/evp.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

using namespace std;
using namespace chrono;

string serialize_online_reply(OnlineReply reply) {
    string res;
    interface::OnlineReply r;
    for (int j = 0; j < 16000/64; j++) {
        uint64_t result = 0;
        uint64_t mask = 1;
        for (int k = j*64; k < (j+1)*64; k++) {
            if (reply.parity[k] == 1)
                result |= mask;
            mask <<= 1;
        }
        r.add_reply(result);
    }
    r.SerializeToString(&res);
    return res;
}

OnlineReply deserialize_online_reply(string msg) {
    interface::OnlineReply r;
    if (!r.ParseFromString(msg)) {
        cout << "deserialize online reply failed\n";
        assert(0);
    }
    OnlineReply reply;
    bitset<16000> bitsets(0);
    for (int j = 0; j < 16000/64; j++) {
        uint64_t val = r.reply()[j];
        uint64_t mask = 1;
        for (int k = j*64; k < (j+1)*64; k++) {
            if ((val&mask) != 0)
                bitsets[k] = 1;
            else
                bitsets[k] = 0;
            mask <<= 1;
        }
    }
    reply.parity = bitsets;
    return reply;
}

bool equal_online_reply(OnlineReply a, OnlineReply b) {
    return (a.parity == b.parity);
}

string serialize_offline_reply(OfflineReply reply) {
    string res;
    interface::OfflineReply r;
    r.set_nbrsets(reply.nbrsets);
    chrono::system_clock::time_point start, end;
    for (int i = 0; i < reply.hints.size(); i++) {
        Block blk = reply.hints[i];
        for (int j = 0; j < 16000/64; j++) {
            uint64_t result = 0;
            uint64_t mask = 1;
            for (int k = j*64; k < (j+1)*64; k++) {
                if (blk[k] == 1)
                    result |= mask;
                mask <<= 1;
            }
            r.add_hints(result);
        }
    }
    r.SerializeToString(&res);
    return res;
}

OfflineReply deserialize_offline_reply(string msg) {
    interface::OfflineReply r;
    if (!r.ParseFromString(msg)) {
        cout << "deserialize offline reply failed\n";
        assert(0);
    }
    OfflineReply reply;
    reply.nbrsets = r.nbrsets();
    for (int i = 0; i < r.hints_size()/(16000/64); i++) {
        bitset<16000> bitsets(0);
        for (int j = 0; j < 16000/64; j++) {
            uint64_t val = r.hints()[i*(16000/64)+j];
            uint64_t mask = 1;
            for (int k = j*64; k < (j+1)*64; k++) {
                if ((val&mask) != 0)
                    bitsets[k] = 1;
                else
                    bitsets[k] = 0;
                mask <<= 1;
            }
        }
        reply.hints.push_back(bitsets);
    }
    return reply;
}

bool equal_offline_reply(OfflineReply a, OfflineReply b) {
    if ((a.nbrsets != b.nbrsets) || (a.hints.size() != b.hints.size()))
        return false;
    for (int i = 0; i < a.hints.size(); i++) {
        if (a.hints[i] != b.hints[i]) {
            // cout << a.hints[i] << endl;
            // cout << b.hints[i] << endl;
            return false;
        }
    }
    return true;
}

string serialize_online_query(OnlineQuery q) {
    interface::OnlineQuery query;
    for (int i = 0; i < q.indices.size(); i++) {
        query.add_indices(q.indices[i]);
    }
    string res;
    query.SerializeToString(&res);
    return res;
}

OnlineQuery deserialize_online_query(string msg) {
    interface::OnlineQuery query;
    if (!query.ParseFromString(msg)) {
        cout << "deserialize online query failed\n";
        assert(0);
    }
    OnlineQuery q;
    for (int i = 0; i < query.indices_size(); i++) {
        q.indices.push_back(query.indices()[i]);
    }
    return q;
}

bool equal_online_query(OnlineQuery a, OnlineQuery b) {
    if (a.indices.size() != b.indices.size())
        return false;
    for (int i = 0; i < a.indices.size(); i++) {
        if (a.indices[i] != b.indices[i])
            return false;
    }
    return true;
}

string serialize_offline_query(OfflineQuery q) {
    interface::OfflineQuery query;
    query.set_nbrsets(q.nbrsets);
    query.set_setsize(q.setsize);
    query.set_keylen(q.keylen);

    // check 
    // serialize offline keys, do not combine every 4 uint8 into uint32 for now
    assert(KeyLen % 4 == 0);
    for (int i = 0; i < q.offline_keys.size(); i++) {
        // combine 4 keys (uint8_t each) to uint32_t

        // for each key
        uint32_t tmp;
	for (int j = 0; j < KeyLen/4; j++) {
            tmp = 0;

            for (int b = 0; b < 4; b++) {
                tmp <<= 8;
                tmp |= q.offline_keys[i][4*j+b];
            }

            //query.add_offline_keys((uint32_t)q.offline_keys[i][j]);
            query.add_offline_keys(tmp);

        }

    }

    for (int i = 0; i < q.shifts.size(); i++) {
        query.add_shifts(q.shifts[i]);
    }
    string res;
    query.SerializeToString(&res);
    return res;
}

OfflineQuery deserialize_offline_query(string msg) {
    interface::OfflineQuery query;
    if (!query.ParseFromString(msg)) {
        cout << "deserialize offline query failed\n";
        assert(0);
    }
    OfflineQuery q;
    q.nbrsets = query.nbrsets();
    q.setsize = query.setsize();
    q.keylen = query.keylen();

    // check 
    for (int i = 0; i < (query.offline_keys_size()*4/KeyLen); i++) {
        Key key;
	uint32_t tmp;
        for (int j = 0; j < KeyLen/4; j++) {
            // for each j, uint32_t
            // parse to key

            uint32_t tmp = query.offline_keys()[i*(KeyLen/4)+j];
            key[4*j+0] = uint8_t((tmp>>24) & 0xFF);
            key[4*j+1] = uint8_t((tmp>>16) & 0xFF);
            key[4*j+2] = uint8_t((tmp>>8) & 0xFF);
            key[4*j+3] = uint8_t(tmp & 0xFF);

        }
 
        q.offline_keys.push_back(key);

    }
    // cout << q.offline_keys.size() << endl;

    for (int i = 0; i < query.shifts_size(); i++) {
        q.shifts.push_back(query.shifts()[i]);
    }
    return q;
}

bool equal_offline_query(OfflineQuery a, OfflineQuery b) {
    if (a.nbrsets != b.nbrsets || a.setsize != b.setsize || a.keylen != b.keylen || a.offline_keys.size() != b.offline_keys.size() || a.shifts.size() != b.shifts.size()) {
        return false;
    }

    for (int i = 0; i < a.offline_keys.size(); i++) {
        for (int j = 0; j < KeyLen; j++) {
            if (a.offline_keys[i][j] != b.offline_keys[i][j])
                return false;
        }
    }

    for (int i = 0; i < a.shifts.size(); i++) {
        if (a.shifts[i] != b.shifts[i])
            return false;
    }
    return true;
}

string serialize_offline_add_query(OfflineAddQueryShort q) {
    interface::OfflineAddQueryShort query;
    query.set_nbrsets(q.nbrsets);
    query.set_setsize(q.setsize);
    for (int i = 0; i < q.req.size(); i++) {
        interface::DiffSizeInfo* info = query.add_req();
        info->set_setno(q.req[i].setno);
        info->set_shift(q.req[i].shift);

        // TODO check change here
        for (int j = 0; j < q.req[i].key.size() / 4; j++) {
            uint32_t tmp = 0;
            for (int b = 0; b < 4; b++) {
                tmp <<= 8;
                tmp |= q.req[i].key[4*j+b];
            }
            info->add_key(tmp);
            //info->add_key(q.req[i].key[j]);
        }

        for (int j = 0; j < q.req[i].prev_side.size(); j++) {
            info->add_prev_side(get<0>(q.req[i].prev_side[j]));
            info->add_prev_side(get<1>(q.req[i].prev_side[j]));
        }
        for (int j = 0; j < q.req[i].cur_side.size(); j++) {
            info->add_cur_side(get<0>(q.req[i].cur_side[j]));
            info->add_cur_side(get<1>(q.req[i].cur_side[j]));
        }
    }

    // check 

    uint8_t* ptr = q.master_key;
    for (int i = 0; i < 16; i++) {
        uint8_t master_val = *(ptr);
        query.add_master_key(master_val);
        ptr++;
    }

    string res;
    query.SerializeToString(&res);
    return res;
}

OfflineAddQueryShort deserialize_offline_add_query(string msg) {
    interface::OfflineAddQueryShort query;
    if (!query.ParseFromString(msg)) {
        cout << "deserialize offline add query failed\n";
        assert(0);
    }
    OfflineAddQueryShort q;
    q.nbrsets = query.nbrsets();
    q.setsize = query.setsize();

    for (int i = 0; i < query.req_size(); i++) {
        DiffSideInfo info;
        info.setno = query.req()[i].setno();
        info.shift = query.req()[i].shift();

        // check 
        for (int j = 0; j < KeyLen/4; j++) {
            uint32_t tmp = query.req()[i].key()[j];

            info.key[4*j] = uint8_t((tmp>>24) & 0xFF);
            info.key[4*j+1] = uint8_t((tmp>>16) & 0xFF);
            info.key[4*j+2] = uint8_t((tmp>>8) & 0xFF);
            info.key[4*j+3] = uint8_t(tmp & 0xFF);
            //info.key[j] = query.req()[i].key()[j];
        }

        for (int j = 0; j < query.req()[i].prev_side_size() / 2; j++) {
            std::tuple<uint32_t, uint32_t> t(query.req()[i].prev_side()[j*2], query.req()[i].prev_side()[j*2+1]);
            info.prev_side.push_back(t);
        }
        for (int j = 0; j < query.req()[i].cur_side_size() / 2; j++) {
            std::tuple<uint32_t, uint32_t> t(query.req()[i].cur_side()[j*2], query.req()[i].cur_side()[j*2+1]);
            info.cur_side.push_back(t);
        }
        q.req.push_back(info);
    }

    // check
    assert(query.master_key_size() == 16);
    q.master_key = (uint8_t*)malloc(16*sizeof(uint8_t));
    uint8_t* ptr = q.master_key;
    for (int i = 0; i < query.master_key_size(); i++) {
        *(q.master_key+i) = (uint8_t)(query.master_key()[i]);
    }
    return q;
}

bool equal_offline_add_query(OfflineAddQueryShort a, OfflineAddQueryShort b) {
    if (a.nbrsets != b.nbrsets || a.setsize != b.setsize || a.req.size() != b.req.size()) {
        return false;
    }
    for (int i = 0; i < 16; i++) {
        if (*(a.master_key+i) != *(b.master_key+i))
            return false;
    }
    for (int i = 0; i < a.req.size(); i++) {
        if (a.req[i].setno != b.req[i].setno || a.req[i].shift != b.req[i].shift || a.req[i].prev_side.size() != b.req[i].prev_side.size() || a.req[i].cur_side.size() != b.req[i].cur_side.size()) {
            return false;
        }
        for (int j = 0; j < KeyLen; j++) {
            if (a.req[i].key[j] != b.req[i].key[j]) {
                return false;
            }
        }
        for (int j = 0; j < a.req[i].prev_side.size(); j++) {
            if (a.req[i].prev_side[j] != b.req[i].prev_side[j]) {
                return false;
            }
        }
        for (int j = 0; j < a.req[i].cur_side.size(); j++) {
            if (a.req[i].cur_side[j] != b.req[i].cur_side[j]) {
                return false;
            }
        }
    }

    return true;
}

string serializeQuery(string& query, interface::QueryType type) {
    interface::Query q;
    q.set_type(type);
    q.set_msg(query);
    string res;
    q.SerializeToString(&res);
    return res;
}

Block generateRandBlock() {
    std::bitset<16000> blk; 
    for (int i = 0; i < 16000; i++) {
       blk[i] = rand() % 2; 
    }
    return blk;
}
