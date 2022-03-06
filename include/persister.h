#ifndef PERSISTER_H
#define PERSISTER_H

#include <string>

#include "trade.h"

class Persister {
    struct PersistTrade {
        int stk_code;
        int bid_id;
        int ask_id;
        double price;
        int volume;
    } __attribute__((packed));

    int stk_id;
    int max_trade_cnt;
    size_t now_cnt;

    PersistTrade* tp;
    int fd;

   public:
    Persister(const char* path, int stk_id, int max_trade_count);

    void persist(uint32_t bid_id, uint32_t ask_id, uint32_t price, uint32_t volume);

    ~Persister();
};

#endif