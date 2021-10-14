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

#include "src/pir.hpp"
#include "src/adprp.hpp"
#include "src/client.hpp"
#include "src/server.hpp"

#include <boost/math/distributions/hypergeometric.hpp>
#include <random>
#include "src/serial.hpp"

#define ONESEC 1000000

using namespace std;

int main(int argc, char *argv[]) {

    /* Make test database and set related parms */

    uint32_t db_size = 7000;
    uint32_t set_size = 80;
    uint32_t nbr_sets = 90*12;
    int opt;

    while ((opt = getopt(argc, argv, "d:s:n:")) != -1) {
        if (opt == 'd') {
            db_size = atoi(optarg);
        }
        else if (opt == 's') {
            set_size = atoi(optarg);
        }
        else if (opt == 'n') {
            nbr_sets = atoi(optarg);
        }
    }

    std::random_device rd;
    Database my_database(db_size);
    for (int i = 0; i < db_size; i++) {
        auto val = generateRandBlock();
        my_database[i] = val;
    }

// Offline phase begin

    /* Setup client */
    PIRClient client;

    /* Client set params */
    client.set_parms(db_size, set_size, nbr_sets);

    /* Client generate set keys (locally stored) */
    client.generate_setkeys();

    /* Client generate offline query for fetching hints (preprocessed info) */
    OfflineQuery offline_qry = client.generate_offline_query();
    assert(equal_offline_query(offline_qry, deserialize_offline_query(serialize_offline_query(offline_qry))));

    cout << "offline query size: " 
      << double(serialize_offline_query(offline_qry).size()) / double(1000) << " KB" << endl;

    /* Setup server */
    PIRServer server;

    /* Server setup database */
    server.set_database(db_size, &my_database);
    //std::cout << "server database set." << std::endl;


    /* Offline reply: output hints (preprocessed info) */
    auto s_prep_st = chrono::high_resolution_clock::now();
    OfflineReply offline_reply = server.generate_offline_reply(offline_qry, 1);
    auto s_prep_ed = chrono::high_resolution_clock::now();
    auto s_prep_time = std::chrono::duration_cast<std::chrono::microseconds>
            (s_prep_ed - s_prep_st).count();
    cout << "Prep: server generates offline reply in "
    << double(s_prep_time)/double(ONESEC) << " sec" << endl;

    OfflineReply a = deserialize_offline_reply(serialize_offline_reply(offline_reply));
    assert(equal_offline_reply(a, offline_reply));

    cout << "offline reply size: " 
      << double(serialize_offline_reply(offline_reply).size()) / double(1000) << " KB" << endl;

    /* Offline client hint: client locally stores hints */
    client.update_local_hints(offline_reply);

// Offline phase end


// Online phase begin

    /* Online query */
    OnlineQuery online_qry;
    uint32_t qry_idx = rand()%db_size;
    online_qry = client.generate_online_query(qry_idx);

    assert(equal_online_query(online_qry, deserialize_online_query(serialize_online_query(online_qry))));

    cout << "online query size: " 
      << double(serialize_online_query(online_qry).size()) / double(1000) << " KB" << endl;

    /* Online reply */
    OnlineReply online_reply = server.generate_online_reply(online_qry, 1);
    assert(equal_online_reply(online_reply, deserialize_online_reply(serialize_online_reply(online_reply))));
    cout << "online reply size: " 
      << double(serialize_online_reply(online_reply).size()) / double(1000) << " KB" << endl;

    OnlineReply online_reply1 = deserialize_online_reply(serialize_online_reply(online_reply));

    /* Client recover data block */
    Block blk = client.recover_block(online_reply1);

    if (blk == my_database[qry_idx]) {
        std::cout << "recover success!" << std::endl;
    } else {
        std::cout << "recover fail!" << std::endl;
    }

    /* Client refresh (probabilistic correctness version) */
    OnlineQuery refresh_query = client.generate_refresh_query(qry_idx);
    
    cout << "refresh query size: " 
      << double(serialize_online_query(refresh_query).size()) / double(1000) << " KB" << endl;

    OnlineReply refresh_reply = server.generate_online_reply(refresh_query, 1);

    cout << "refresh reply size: " 
      << double(serialize_online_reply(refresh_reply).size()) / double(1000) << " KB" << endl;

    client.sets[client.cur_qry_setno].hint = blk ^ refresh_reply.parity;

    // incorporate Checklist refresh if possible later

// Online phase end


// Batched addition begin (adding a single batch)

    /* Set up data objects to be added */
    int nbr_add = 0.05 * db_size;   // number of additions (i.e., batch size) = 1% of database size

    // assign random value to items to be added
    std::vector<Block> v(nbr_add);
    for (int i = 0; i < nbr_add; i++) {
        v[i] = rand()%100000;
    }

    /* Server adds new data objects */
    server.add_elements(nbr_add, v);

    /* Server notifies client about 'nbr_add' */
    // TODO: we probably can omit this in testing?

    /* Client generates batched addition query */
    OfflineAddQueryShort offline_add_qry = client.batched_addition_query(nbr_add);
    cout << "client add query generated." << endl;

    assert(equal_offline_add_query(offline_add_qry, deserialize_offline_add_query(serialize_offline_add_query(offline_add_qry))));
    cout << "batched addition query size: " 
      << double(serialize_offline_add_query(offline_add_qry).size()) / double(1000) << " KB" << endl;

    /* Server generates batched addition reply */
    OfflineAddQueryShort offline_add_qry1 = deserialize_offline_add_query(serialize_offline_add_query(offline_add_qry));
    OfflineReply offline_add_reply = server.batched_addition_reply(offline_add_qry1);

    OfflineReply offline_add_reply1 = deserialize_offline_reply(serialize_offline_reply(offline_add_reply));

    cout << "batched addition reply size: " 
      << double(serialize_offline_reply(offline_add_reply).size()) / double(1000) << " KB" << endl;

    /* Client update local hints after one batched addition */
    client.update_local_hints(offline_add_reply1);

// Batched addition end


// Test begin
// In order to see if hints are correctly updated, let client issue online query

    /* Test client online query after one batched addition */
    qry_idx = 7010;
    online_qry = client.generate_online_query(qry_idx);

    /* Server generates online reply */
    online_reply = server.generate_online_reply(online_qry, 1);

    /* Client recovers block */
    blk = client.recover_block(online_reply);
    if (blk == my_database[qry_idx])
        std::cout << "success!" << std::endl;
    else std::cout << "fail!" << std::endl;


    /* Client refresh */
    refresh_query = client.generate_refresh_query(qry_idx);
    refresh_reply = server.generate_online_reply(refresh_query, 1);
    client.sets[client.cur_qry_setno].hint = blk ^ refresh_reply.parity;

// Test end

    return 0;
}
