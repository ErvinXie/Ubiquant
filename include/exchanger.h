#ifndef EXCHANGER_H
#define EXCHANGER_H

#include "persister.h"
#include "trade.h"

// 合并器
class OrderMerger : Stream<Order> {
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
struct Exchanger : Sink<Order> {
    Persister persister;
    // add exchanger definition

    // Insert an order to the exchanger, return the result trades.
    virtual void send(Order order) override;
};

#endif