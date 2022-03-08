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
        : local(local), remote(remote), bitmask(bitmask) {}

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
    uint32_t order_id = 0;
    HookChecker checker;
    HookNotifier notifier;
    Persister persister;
    // add exchanger definition

    void process_limit(Order::Direction dir, uint32_t order_id, uint32_t price, uint32_t volume);
    void process_counter_best(Order::Direction dir, uint32_t order_id, uint32_t volume);
    void process_client_best(Order::Direction dir, uint32_t order_id, uint32_t volume);
    void process_best_five(Order::Direction dir, uint32_t order_id, uint32_t volume);
    void process_fak(Order::Direction dir, uint32_t order_id, uint32_t volume);
    void process_fok(Order::Direction dir, uint32_t order_id, uint32_t volume);

    void commit(uint32_t bid_id, uint32_t ask_id, uint32_t price, uint32_t volume);

   public:
    Processor(HookChecker checker, HookNotifier notifier, Persister persister)
        : checker(std::move(checker)), notifier(std::move(notifier)), persister(std::move(persister)) {}

    void process(Order order);
};

#endif