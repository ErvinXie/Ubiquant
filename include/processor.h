#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "codec.h"
#include "hook.h"
#include "persister.h"
#include "trade.h"

// 合并器
class OrderMerger final : Stream<Order> {
    OrderDecoder local, remote;
    std::vector<bool> bitmask;
    uint32_t current = 0;

   public:
    OrderMerger(OrderDecoder local, OrderDecoder remote, std::vector<bool> bitmask)
        : local(local), remote(remote), bitmask(std::move(bitmask)) {}
    virtual std::optional<Order> next() override {
        if (current >= bitmask.size()) {
            return {};
        }
        if (bitmask[current++]) {
            return local.next();
        } else {
            return remote.next();
        }
    }
};

// 单支股票的撮合器
class Processor {
    uint32_t stk_id;
    uint32_t order_id = 0;
    HookChecker checker;
    HookNotifier notifier;
    Persister persister;

    // add exchanger definition
    struct ComOrder {
        uint32_t order_id : 30;
        uint32_t price : 22;
        uint32_t volume : 10;
        ComOrder(uint32_t order_id = 0, uint32_t price = 0, uint32_t volume = 0)
            : order_id(order_id), price(price), volume(volume) {}
        bool operator<(const ComOrder& y) const {
            if (price == y.price) return order_id > y.order_id;
            return price < y.price;
        }
        bool operator>(const ComOrder& y) const {
            if (price == y.price) return order_id > y.order_id;
            return price > y.price;
        }
    };

    std::priority_queue<ComOrder, std::vector<ComOrder>, std::less<ComOrder>> buy;
    uint64_t buy_total;
    std::priority_queue<ComOrder, std::vector<ComOrder>, std::greater<ComOrder>> sell;
    uint64_t sell_total;

    uint32_t max_price, min_price;

    void process_limit(Order::Direction dir, uint32_t order_id, uint32_t price, uint32_t volume);
    void process_counter_best(Order::Direction dir, uint32_t order_id, uint32_t volume);
    void process_client_best(Order::Direction dir, uint32_t order_id, uint32_t volume);
    void process_best_five(Order::Direction dir, uint32_t order_id, uint32_t volume);
    void process_fak(Order::Direction dir, uint32_t order_id, uint32_t volume);
    void process_fok(Order::Direction dir, uint32_t order_id, uint32_t volume);

    void commit(uint32_t bid_id, uint32_t ask_id, uint32_t price, uint32_t volume);

   public:
    Processor(uint32_t stk_id, HookChecker checker, HookNotifier notifier, Persister persister, uint32_t last_price)
        : stk_id(stk_id),
          checker(std::move(checker)),
          notifier(std::move(notifier)),
          persister(std::move(persister)),
          buy_total(0),
          sell_total(0) {
        max_price = (last_price * 11 + 5) / 10;
        if (max_price == last_price) max_price++;
        min_price = (last_price * 9 + 5) / 10;
        if (min_price == last_price) min_price--;
    }

    void process(Order order);
};
#endif