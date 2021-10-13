#include "client.hpp"
#include "adprp.hpp"
#include "pir.hpp"
#include <math.h>
#include <cmath>
#include <random>
#include <openssl/rand.h>
#include <iostream>
using namespace std;
/*
 * Given a set s, probabilistically remove the desired index or other index
 * */
void probabilistic_remove(std::set<uint32_t> &s, const double pr, uint32_t idx) {

    std::random_device rd;
    std::mt19937 bgen(rd());
    std::bernoulli_distribution d(pr);

    if (d(bgen) == 0) {
        // remove desired index
        s.erase(idx);

    } else {

        s.erase(idx);

        // remove a random number in the set other than idx
        auto it = s.begin();

        int r = rand()%s.size();  // sample a random idx in the set

        for (int i = 0; i < r; i++, it++);  // increase iterator to idx
        s.erase(*it);             // remove the element at idx

        s.insert(idx);
    }
}

PIRClient::PIRClient() {
    std::cout << "client is created." << std::endl;
}


void PIRClient::set_parms(uint32_t dbrange_, uint32_t setsize_, uint32_t nbrsets_) {
    dbrange = dbrange_;
    setsize = setsize_;
    nbrsets = nbrsets_;

    master_key = static_cast<uint8_t *>(malloc(KeyLen));
    RAND_bytes(master_key, KeyLen);
}

void PIRClient::generate_setkeys() {

    master_key = new uint8_t[KeyLen];
    RAND_bytes(master_key, KeyLen);

    /* for each set: generate key, shift, short side info */

    for(int i = 0; i < nbrsets; i++) {
        SetDesc tmp;

        tmp.prf_key = static_cast<uint8_t *>(malloc(KeyLen));
        RAND_bytes(tmp.prf_key, KeyLen);

        // TODO think about this shift again
        // tmp.shift = 0;
        tmp.shift = rand() % dbrange;

        tmp.sinfo.emplace_back(std::make_tuple(dbrange, setsize));

        sets.push_back(tmp);
    }
}

void CopyKey(Key &key, uint8_t *prf_key) {

    for (int i = 0; i < KeyLen; i++) {
        key[i] = prf_key[i];
    }
}

/*
 * Client sends keys to offline server
 * */
OfflineQuery PIRClient::generate_offline_query() {
    OfflineQuery tmp;
    tmp.nbrsets = nbrsets;
    tmp.setsize = setsize;
    tmp.keylen = KeyLen;

    for (int i = 0; i < nbrsets; i++) {
        Key key;
        CopyKey(key, sets[i].prf_key);
        tmp.offline_keys.push_back(key);

        tmp.shifts.push_back(sets[i].shift);

    }

    return tmp;
}

/*
 * update_local_hints can also be used with incremental updates offline reply
 * */
void PIRClient::update_local_hints(OfflineReply offline_reply) {

    for (int i = 0; i < offline_reply.nbrsets; i++) {
        sets[i].hint = sets[i].hint ^ offline_reply.hints[i];
    }
}

/*
 * Find a set containing desired index
 * Expand this set to clear indices
 * Probabilistically remove the desired index/other index from the set
 * */
OnlineQuery PIRClient::generate_online_query(uint32_t desired_idx) {

    //std::cout << "client generating online query..." << std::endl;

    OnlineQuery online_query;
    std::set<uint32_t> tmp;

    int setno = -1; // indicating no set is found to contain desired_idx

    // find the set that the desired index is in
    // TODO: consider shift in some prp key!

    bool flag = false;
    for (int i = 0; i < nbrsets; i++) {

        // expand a set: eval on each range

        uint32_t accum_range = 0;

        for (int slot = 0; slot < sets[i].sinfo.size(); slot++) {

            uint32_t range = std::get<0>(sets[i].sinfo[slot]);

            if (desired_idx >= accum_range && desired_idx < accum_range+range) {

                uint32_t inv;

                if (sets[i].shift != 0 && slot == 0) {
                    /* actually happen only in the first slot */
                    uint32_t shift_idx = (desired_idx + range - sets[i].shift) % range;

                    inv = inv_cycle_walk(shift_idx, range, sets[i].prf_key);

                } else {

                    uint8_t *ki = derive_key(master_key, sets[i].prf_key, slot);

                    //inv = inv_cycle_walk(desired_idx - accum_range, range, sets[i].prf_key);
                    inv = inv_cycle_walk(desired_idx - accum_range, range, ki);


                }

                if (inv < std::get<1>(sets[i].sinfo[slot])) {
                    setno = i;
                    goto Found;
                }
            } //else continue;

            accum_range += range;

        }
    }

    Found:
    if (setno == -1) {
        // will happen with low probability
        throw std::invalid_argument("cannot find desired index!");
    }

    cur_qry_setno = setno;

    uint32_t accum_range = 0;
    for (int slot = 0; slot < sets[setno].sinfo.size(); slot++) {

        uint32_t range = std::get<0>(sets[setno].sinfo[slot]);

        uint8_t *ki = static_cast<uint8_t *>(malloc(KeyLen));

        if (slot == 0) {
            memcpy(ki, sets[setno].prf_key, KeyLen);

        } else {
            ki = derive_key(master_key, sets[setno].prf_key, slot);
        }


        for (uint32_t x = 0; x < std::get<1>(sets[setno].sinfo[slot]); x++) {

            //uint32_t y = cycle_walk(x, range, sets[setno].prf_key);
            uint32_t y = cycle_walk(x, range, ki);

            if (sets[cur_qry_setno].shift != 0 && slot == 0)
                y = (y + sets[cur_qry_setno].shift) % range;

            tmp.insert(y + accum_range);

        }
        accum_range += range;
    }

    double pr = double(setsize-1) / double(dbrange);

    probabilistic_remove(tmp, pr, desired_idx);


    for (auto it = tmp.begin(); it != tmp.end(); it++) {
        online_query.indices.push_back(*it);
    }
    return online_query;
}




int is_in_tuple (uint32_t r, std::vector< std::tuple<uint32_t, uint32_t> > v) {

    int flag = -1;

    for (int i = 0; i < v.size(); i++) {
        if (r == std::get<0>(v[i])) {

            flag = i;
            break;
        }
    }

    return flag;
}

Block PIRClient::recover_block(OnlineReply online_reply) {
    int setno = cur_qry_setno;
    Block blk = online_reply.parity ^ sets[setno].hint;
    return blk;
}

OfflineAddQueryLong PIRClient::incremental_addition_query(uint32_t prev_range, uint32_t nbr_add) {

    dbrange = prev_range + nbr_add;

    OfflineAddQueryLong tmp;

    //std::cout << "nbrsets = " << nbrsets << std::endl;

    for (int i = 0; i < nbr_add; i++) {

        // sample db_size+i into every set

        uint32_t cur_idx = prev_range + i;

        double pr = double(setsize) / double(cur_idx);
        std::random_device rd;
        std::mt19937 bgen(rd());
        std::bernoulli_distribution d(pr);

        for (int s = 0; s < nbrsets; s++) {

            // choose a random number e.g. 3 in [setsize]
            uint32_t r = rand() % setsize;

            // push_back tuple e.g. (3, n+i) based on sampling
            if (d(bgen) == 1) {

                // if r == previous r in some tuple, replace n+prev with n+cur
                // otherwise push back

                int flag = is_in_tuple(r, sets[s].linfo);
                if (flag != -1) {
                    std::get<1>(sets[s].linfo[flag]) = cur_idx;
                } else {
                    sets[s].linfo.emplace_back(std::make_tuple(r, cur_idx)); // update client set
                }
            }
        }
    }

    tmp.qry.resize(nbrsets);
    for (int s = 0; s < nbrsets; s++) {

        std::get<0>(tmp.qry[s]) = s;
        std::get<1>(tmp.qry[s]) = sets[s].linfo;  // TODO should apply PRP to the first elem!!!

        for (int i = 0; i < sets[s].linfo.size(); i++) {

            //uint32_t y = cycle_walk(std::get<0>(sets[s].linfo[i]), prev_range, key);
            // apply PRP on the first elem in linfo, so that the server can compute xor for updating hints

            uint32_t y = small_prf(14, 14, sets[s].prf_key, std::get<0>(sets[s].linfo[i]));

            std::get<0>((std::get<1>(tmp.qry[s]))[i]) = y;
        }
    }

    return tmp;
}

/*
 * Send to the offline server both prev and cur side info
 * */
OfflineAddQueryShort PIRClient::batched_addition_query(uint32_t nbr_add) {

    uint32_t prev_range = dbrange;
    dbrange = prev_range + nbr_add;

    OfflineAddQueryShort tmp;
    tmp.master_key = master_key;


    tmp.setsize = setsize;
    tmp.nbrsets = nbrsets;


    // most inefficient part: HG
    //auto bnl_st = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < nbr_add; i++) {

        // sample db_size+i into every set

        uint32_t cur_idx = prev_range + i;

        double pr = double(setsize) / double(cur_idx);
        std::random_device rd;
        std::mt19937 bgen(rd());
        std::bernoulli_distribution d(pr);

        for (int s = 0; s < nbrsets; s++) {

            // choose a random number e.g. 3 in [setsize]
            uint32_t r = rand() % setsize;

            // push_back tuple e.g. (3, n+i) based on sampling
            if (d(bgen) == 1) {

                // if r == previous r in some tuple, replace n+prev with n+cur
                // otherwise push back

                int flag = is_in_tuple(r, sets[s].linfo);
                if (flag != -1) {
                    std::get<1>(sets[s].linfo[flag]) = cur_idx;
                } else {
                    sets[s].linfo.emplace_back(std::make_tuple(r, cur_idx)); // update client set
                }
            }
        }
    }

    //auto bnl_ed = std::chrono::high_resolution_clock::now();
    //auto bnl_time = std::chrono::duration_cast<std::chrono::microseconds>(bnl_ed - bnl_st).count();
    //std::cout << "bnl time for one batch: " << bnl_time << " microsec." << std::endl;


    tmp.req.resize(nbrsets);

    for (int s = 0; s < nbrsets; s++) {

        tmp.req[s].prev_side = sets[s].sinfo; // keep prev sideinfo

        uint32_t ell = uint32_t(sets[s].linfo.size()); // get number ell
        sets[s].linfo.clear(); // clear long info

        // update side info of previous ranges

        double lsum = 0;

        std::vector<uint32_t> cntr;
        cntr.resize(sets[s].sinfo.size());

        for (int i = 0; i < sets[s].sinfo.size(); i++) {
            lsum += std::get<1>(sets[s].sinfo[i]);
            cntr[i] = uint32_t(lsum);
        }

        // kick out ell random indices
        for (int i = 0; i < ell; i++) {
            uint32_t fall = rand() % uint32_t(lsum);

            if (fall < cntr[0]) {
                std::get<1>(sets[s].sinfo[0])--;
            } else {

                for (int cnt = 0; cnt < cntr.size() - 1; cnt++) {

                    if (fall >= cntr[cnt] && fall < cntr[cnt+1]) {
                        std::get<1>(sets[s].sinfo[cnt+1])--;
                        break;
                    }
                }
            }
        }

        sets[s].sinfo.emplace_back(std::make_tuple(nbr_add, ell));

        tmp.req[s].setno = s;
        tmp.req[s].cur_side = sets[s].sinfo;
        tmp.req[s].shift = sets[s].shift;

        CopyKey(tmp.req[s].key, sets[s].prf_key);
    }

    return tmp;
}


// TODO: make sure in CK paper how they do refresh, if it is probabilistic
OnlineQuery PIRClient::generate_refresh_query(uint32_t desired_idx) {

    uint32_t setno = cur_qry_setno;
    // identify which set
    // gen new prf_key
    // set new aux
    // shift to get i
    // eval prp
    // remove i with high probability

    OnlineQuery online_query;
    std::set<uint32_t> tmp;

    if (sets[setno].prf_key != NULL)
        free(sets[setno].prf_key);

    // generate new key
    sets[setno].prf_key = static_cast<uint8_t *>(malloc(KeyLen));
    RAND_bytes(sets[setno].prf_key, KeyLen);


    uint32_t r = rand() % setsize;

    uint32_t y = cycle_walk(r, dbrange, sets[setno].prf_key);
            //adprf(dbrange, sets[setno].prf_key, r);

    sets[setno].shift = (desired_idx + dbrange - y) % dbrange; // shift is positive number
    //std::cout << "shift = " << sets[setno].shift << std::endl;

    sets[setno].sinfo.clear();
    sets[setno].sinfo.emplace_back(std::make_tuple(dbrange, setsize));

    for (uint32_t i = 0; i < setsize; i++) {

        uint32_t v = cycle_walk(i, dbrange, sets[setno].prf_key);

        v = (v + sets[setno].shift) % dbrange;
        //if (v >= dbrange) v = v - dbrange;
        //else if (v < 0) v = v + dbrange;

        tmp.insert(v);
    }


    tmp.erase(desired_idx);



    //double pr = double(setsize-1) / double(dbrange);


    for (auto it = tmp.begin(); it != tmp.end(); it++) {
        online_query.indices.push_back(*it);
    }
    return online_query;
}
