#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "hook.h"
#include "persister.h"
#include "trade.h"

// 合并器
class OrderMerger final : Stream<Order> {
    std::shared_ptr<Stream<Order>> local, remote;
    std::optional<Order> buf;
    uint32_t current;

   public:
    OrderMerger(std::shared_ptr<Stream<Order>> local, std::shared_ptr<Stream<Order>> remote, uint32_t init)
        : local(local), remote(remote), current(init) {}

    virtual std::optional<Order> next() override {
        if (!buf) {
            buf = local->next();
        }
        if (buf && buf->order_id == current) {
            auto order = std::move(buf.value());
            buf.reset();
            current++;
            return order;
        }
        auto order = remote->next();
        if (order) {
            order->order_id = current++;
        }
        return order;
    }
};

// 单支股票的撮合器
class Processor {
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
    // Insert an order to the exchanger, return the result trades.
    void process(Order order);
};

#endif