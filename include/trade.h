#ifndef TRADE_H
#define TRADE_H

#include <cstdint>
#include <vector>

#include "common.h"

using std::uint32_t;

enum Direction {
    Bid = 1,   // 买入
    Ask = -1,  // 卖出
};

enum Type {
    Limit = 0,        // 限价申报
    CounterBest = 1,  // 对手方最优
    ClientBest = 2,   // 本方最优
    BestFive = 3,     // 最优五档剩余撤销
    FAK = 4,          // 即时成交剩余撤销
    FOK = 5,          // 全额成交或撤销
};

// 订单
struct Order {
    uint32_t order_id;
    Direction dir;
    Type type;
    uint32_t price;  // 仅限价单有意义
    uint32_t volume;
};

// 成交记录
struct Trade {
    uint32_t bid_id;  // 买方id
    uint32_t ask_id;  // 卖方id
    uint32_t price;
    uint32_t volume;
};

// 合并器
class OrderMerger : Stream<Order> {
    std::shared_ptr<Stream<Order>> left, right;
    std::optional<Order> left_buf, right_buf;
    uint32_t current;

   public:
    OrderMerger(std::shared_ptr<Stream<Order>> left, std::shared_ptr<Stream<Order>> right, uint32_t init)
        : left(left), right(right), current(init) {}

    virtual std::optional<Order> next() override {
        if (!left_buf) {
            left_buf = left->next();
        }
        if (left_buf && left_buf->order_id == current) {
            auto order = std::move(left_buf.value());
            left_buf.reset();
            current++;
            return order;
        }
        if (!right_buf) {
            right_buf = right->next();
        }
        if (right_buf && right_buf->order_id == current) {
            auto order = std::move(right_buf.value());
            right_buf.reset();
            current++;
            return order;
        }
        return {};
    }
};

#endif
