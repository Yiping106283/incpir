#include "server.hpp"
#include "adprp.hpp"
#include "pir.hpp"
#include <iostream>
// #include <mach/port.h>

using namespace std;

PIRServer::PIRServer() {
    std::cout << "server is created." << std::endl;
}

void PIRServer::set_database(uint32_t db_size_, Database *db_) {

    if (db_ == nullptr) {
        throw std::invalid_argument("database cannot be null");
    }

    db_size = db_size_;
    db = db_;

}

OfflineReply PIRServer::generate_offline_reply(OfflineQuery offline_qry, uint32_t client_id) {

    OfflineReply tmp;
    tmp.nbrsets = offline_qry.nbrsets;

    // init parity vector 'hints'
    tmp.hints.resize(tmp.nbrsets);

    std::set<uint32_t> ts;

    // compute parity[i] for set i
    for (int i = 0; i < offline_qry.nbrsets; i++) {

        ts.clear();

        // TODO this should be optimized
        uint8_t *key;
        key = static_cast<uint8_t *>(malloc(KeyLen));
        for (int k = 0; k < KeyLen; k++) {
            key[k] = offline_qry.offline_keys[i][k];
        }

        // eval PRF/PRP until reaches set size
        for (uint32_t j = 0; j < offline_qry.setsize; j++) {


            // TODO shift??
            long long y = cycle_walk(j, db_size, key);


            //if (y + offline_qry.shifts[i] >= db_size) y = y + offline_qry.shifts[i] - db_size;
            //else if (y + offline_qry.shifts[i] < 0) y = y + offline_qry.shifts[i] + db_size;
            //else y += offline_qry.shifts[i];

            y = (y + offline_qry.shifts[i]) % db_size;


            tmp.hints[i] = tmp.hints[i] ^ ((*db)[y]);
        }

	free(key);
    }
    return tmp;
}

OnlineReply PIRServer::generate_online_reply(OnlineQuery online_qry, uint32_t client_id) {

    OnlineReply tmp;

    for (auto it = online_qry.indices.begin(); it != online_qry.indices.end(); it++) {

        tmp.parity = tmp.parity ^ ((*db)[(*it)]);
    }

    return tmp;
}

void PIRServer::add_elements(uint32_t nbr_add, std::vector<Block> v) {

    if (nbr_add != v.size()) {
        throw std::invalid_argument("server additions error");
    }

    for (int i = 0; i < nbr_add; i++) {
        (*db).push_back(v[i]);
    }

    db_size += nbr_add;

    std::cout << "server adds " << nbr_add << " elements,"
      << " now database has " << (*db).size() << " elements in total" << std::endl;
}

Block PIRServer::delete_element(uint32_t idx) {
    Block blk = rand()%10000;
    return blk ^ (*db)[idx];
}

Block PIRServer::edit_element(uint32_t idx, Block new_element) {
    return new_element ^ (*db)[idx];
}

OfflineReply PIRServer::incremental_addition_reply(OfflineAddQueryLong offline_add_qry) {

    OfflineReply tmp;
    uint32_t nbrsets = offline_add_qry.qry.size();
    tmp.nbrsets = nbrsets;
    tmp.hints.resize(nbrsets);

    for (int s = 0; s < nbrsets; s++) {

        // for each set
//        tmp.hints[s] = zero_block();

        std::vector<std::tuple<uint32_t, uint32_t> > indices_per_set = std::get<1>(offline_add_qry.qry[s]);

        for (int i = 0; i < indices_per_set.size(); i++) {

            int idx1 = std::get<0>(indices_per_set[i]);
            int idx2 = std::get<1>(indices_per_set[i]);

            //std::cout << idx1 << " , " << idx2 << std::endl;

            tmp.hints[s] = tmp.hints[s] ^ (*db)[idx1]; // this should be the new db!!
            tmp.hints[s] = tmp.hints[s] ^ (*db)[idx2];
        }
    }

    return tmp;
}

OfflineReply PIRServer::batched_addition_reply(OfflineAddQueryShort offline_add_qry) {

    // TODO consider shift

    if (offline_add_qry.req.size() != offline_add_qry.nbrsets) {
        throw std::invalid_argument("hint update request is invalid");
    }

    OfflineReply tmp;

    uint32_t nbrsets = offline_add_qry.nbrsets;
    uint32_t setsize = offline_add_qry.setsize;

    // initialize OfflineReply
    tmp.nbrsets = nbrsets;
    tmp.hints.resize(nbrsets);


    for (int s = 0; s < nbrsets; s++) {

        // for each set

        // eval PRF to obtain the difference set

        Key tmpkey = offline_add_qry.req[s].key;
        // convert offline keys to uint8_t *
        uint8_t *key;
        key = static_cast<uint8_t *>(malloc(KeyLen));
        for (int k = 0; k < KeyLen; k++) {
            key[k] = tmpkey[k];
        }

        std::vector<std::tuple<uint32_t, uint32_t> > prev = offline_add_qry.req[s].prev_side;
        std::vector<std::tuple<uint32_t, uint32_t> > cur = offline_add_qry.req[s].cur_side;


        // compute the difference of all slots except the last

        uint32_t kick_slot = prev.size();
        uint32_t accum_range = 0;

        for (int i = 0; i < kick_slot; i++) {

            // for each slot
            uint32_t range = std::get<0>(prev[i]);

            // derive key for each slot
            uint8_t *ki = static_cast<uint8_t *>(malloc(KeyLen));

            if (i == 0) {
                memcpy(ki, key, KeyLen);

            } else {

                ki = derive_key(offline_add_qry.master_key, key, i);
            }

            //cout << "flag" << endl;

            for (uint32_t ell = std::get<1>(cur[i]); ell < std::get<1>(prev[i]); ell++) {

                // DEBUG
                if (ell == range) {
                    throw std::invalid_argument("error");
                }

                //uint32_t idx = cycle_walk(ell, range, key);
                uint32_t idx = cycle_walk(ell, range, ki);


                if (offline_add_qry.req[s].shift != 0 && i == 0) {
                    idx = (idx + offline_add_qry.req[s].shift ) % range;
                }

                tmp.hints[s] = tmp.hints[s] ^ (*db)[accum_range + idx];

            }

            accum_range += range;

            //if (ki != NULL) free(ki);
        }


        // compute the last slot

        uint32_t new_slot = cur.size() - 1;
        uint32_t last_range = std::get<0>(cur[new_slot]);

        uint8_t *ki = derive_key(offline_add_qry.master_key, key, new_slot);
        //if (s==0) print_key(ki);
        // checked: ki is consistent with client online query

        for (uint32_t ell = 0; ell < std::get<1>(cur[new_slot]); ell++) {

            //uint32_t idx = cycle_walk(ell, last_range, key);
            uint32_t idx = cycle_walk(ell, last_range, ki);


            tmp.hints[s] = tmp.hints[s] ^ (*db)[accum_range + idx];
        }

        //if (ki != NULL) free(ki);

        //std::cout << "prev check ts size: "<< ts.size() << std::endl;
        // end
	free(key);
    }

    return tmp;
}
