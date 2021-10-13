#include "server.hpp"
#include "puncprf.hpp"
#include "pir.hpp"
#include <iostream>
#include <cmath>

using namespace std;


PIRServer::PIRServer() {
    std::cout << "server is created." << std::endl;
}

void PIRServer::set_database(uint32_t db_size_, Database *db_) {

    if (db_ == nullptr) {
        throw std::invalid_argument("database cannot be null");
    }

    db_size = db_size_;
    lgn = (int)(log2(db_size)-1)+1;
    db = db_;

}

OfflineReply PIRServer::generate_offline_reply_fast(OfflineQuery offline_qry, uint32_t client_id) {

    // TODO: Now call BreadthEval instead of Eval
    OfflineReply tmp;
    tmp.nbrsets = offline_qry.nbrsets;

    // init parity vector 'hints'
    tmp.hints.resize(tmp.nbrsets);

    // compute parity[i] for set i
    for (int i = 0; i < offline_qry.nbrsets; i++) {

        uint8_t *key;
        key = static_cast<uint8_t *>(malloc(KeyLen));
        for (int k = 0; k < KeyLen; k++) {
            key[k] = offline_qry.offline_keys[i][k];
        }

        vector<uint32_t> raw_indices = BreadthEval(key, 0, (1<<(lgn/2)), lgn, db_size);

        for (uint32_t j = 0; j < raw_indices.size(); j++) {
            uint32_t y = (raw_indices[j] + offline_qry.shifts[i]) % db_size;
            tmp.hints[i] = tmp.hints[i] ^ ((*db)[y]);
        }

        free(key);
    }

    return tmp;

}

/*
OfflineReply PIRServer::generate_offline_reply(OfflineQuery offline_qry, uint32_t client_id) {



    OfflineReply tmp;
    tmp.nbrsets = offline_qry.nbrsets;

    // init parity vector 'hints'
    tmp.hints.resize(tmp.nbrsets);

    // compute parity[i] for set i
    for (int i = 0; i < offline_qry.nbrsets; i++) {

        uint8_t *key;
        key = static_cast<uint8_t *>(malloc(KeyLen));
        for (int k = 0; k < KeyLen; k++) {
            key[k] = offline_qry.offline_keys[i][k];
        }

        // Eval
        // TODO set size should be (1<<(lgn/2))
        for (uint32_t j = 0; j < offline_qry.setsize; j++) {

            uint32_t y = Eval(key, j, lgn);

            y = (y + offline_qry.shifts[i]) % db_size;

            tmp.hints[i] = tmp.hints[i] ^ ((*db)[y]);
        }

        free(key);
    }

    return tmp;
}
*/

OnlineReply PIRServer::generate_online_reply(OnlineQuery online_qry, uint32_t client_id) {

    OnlineReply tmp;

    // TODO
    // EvalPunc

    PuncKeys punckeys;
    punckeys.height = online_qry.height;
    punckeys.bitvec = online_qry.bitvec;
    punckeys.keys = online_qry.keys;


    int count = 0;
    // cout << "\tlgn: " << lgn << endl;
    for (int i = 0; i < (1<<(lgn/2)); i++) {
        int y = EvalPunc(punckeys, i, lgn, db_size);
        //cout << y << " ";
        if (y == -1) {
            count++;
            continue;
        }

        y = (y + online_qry.shift) % db_size;

        tmp.parity = tmp.parity ^ ((*db)[y]);
    }

    //cout << "\tcount: " << count << endl;
    return tmp;
}




