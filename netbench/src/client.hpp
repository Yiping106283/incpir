#ifndef _CLIENT
#define _CLIENT

#include "pir.hpp"

#include <stdint.h>
#include <set>

class PIRClient {
public:
    uint32_t dbrange;
    uint32_t setsize;
    uint32_t nbrsets;

    uint8_t *master_key;

    int cur_qry_setno;

    std::vector<SetDesc> sets;

    PIRClient();

    void set_parms(uint32_t dbrange_, uint32_t setsize_, uint32_t nbrsets_);
    void generate_setkeys();

    OfflineQuery generate_offline_query();
    void update_local_hints(OfflineReply offline_reply);

    OnlineQuery generate_online_query(uint32_t desired_idx);


    //NewOnlineQuery query_easy(uint32_t desired_idx);
    //NewOnlineQuery query_hard(uint32_t desired_idx);

    //NewOnlineQuery checklist_generate_online_query(uint32_t desired_idx);

    Block recover_block(OnlineReply online_reply);

    // additions
    OfflineAddQueryLong incremental_addition_query(uint32_t prev_range, uint32_t nbr_add); // original dbsize, new elements
    OfflineAddQueryShort batched_addition_query(uint32_t nbr_add);


    // need to consider before and after add both
    OnlineQuery generate_refresh_query(uint32_t desired_idx);


};

#endif
