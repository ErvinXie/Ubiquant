#ifndef PERSISTER_H
#define PERSISTER_H

#include <string>

#include "common.h"
#include "trade.h"

class Persister : public Sink<Trade> {
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

    virtual void send(Trade trade) override;

    ~Persister();
};

#endif