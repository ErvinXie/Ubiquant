#ifndef PERSISTER_H
#define PERSISTER_H

#include <string>
#include <utility>

#include "common.h"
#include "trade.h"

class Persister : nocopy {
    struct Trade {
        int stk_code;
        int bid_id;
        int ask_id;
        double price;
        int volume;
    } __attribute__((packed));

    int stk_id;
    int max_trade_cnt;
    size_t now_cnt;

    Trade* tp;
    int fd;

   public:
    Persister(const char* path, int stk_id, int max_trade_count = 2 * NR_ORDERS_SINGLE_STK_HALF);

    Persister(Persister&& another)
        : stk_id(another.stk_id),
          max_trade_cnt(another.max_trade_cnt),
          now_cnt(another.now_cnt),
          tp(std::exchange(another.tp, nullptr)),
          fd(another.fd) {}

    void persist(uint32_t bid_id, uint32_t ask_id, uint32_t price, uint32_t volume);

    ~Persister();
};

#endif