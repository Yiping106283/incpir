#ifndef _SERVER
#define _SERVER

#include "pir.hpp"

#include <openssl/rand.h>
#include <set>
#include <bitset>

class PIRServer {
public:
    uint32_t db_size;
    Database *db = nullptr;

    PIRServer();


    void set_database(uint32_t db_size_, std::vector<Block> *db_);

    OfflineReply generate_offline_reply(OfflineQuery offline_qry, uint32_t client_id);
    OnlineReply generate_online_reply(OnlineQuery online_qry, uint32_t client_id);

    void add_elements(uint32_t nbr_add, std::vector<Block> v);


    OfflineReply incremental_addition_reply(OfflineAddQueryLong offline_add_qry);
    OfflineReply batched_addition_reply(OfflineAddQueryShort offline_add_qry);

    // sends back xor individually
    Block delete_element(uint32_t idx);
    Block edit_element(uint32_t idx, Block new_element);

};


#endif
