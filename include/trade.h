#ifndef TRADE_H
#define TRADE_H

#include <cmath>
#include <cstdint>
#include <vector>

#include "common.h"

using std::uint32_t;

// 订单
struct Order {
    enum Direction {
        Bid = 0,  // 买入
        Ask = 1,  // 卖出
    };

    enum OrderType {
        Limit = 0,        // 限价申报
        CounterBest = 1,  // 对手方最优
        ClientBest = 2,   // 本方最优
        BestFive = 3,     // 最优五档剩余撤销
        FAK = 4,          // 即时成交剩余撤销
        FOK = 5,          // 全额成交或撤销
    };

    Direction dir;
    OrderType type;
    uint32_t price;  // 仅限价单有意义
    uint32_t volume;

    static Order from_raw(uint32_t price, int volume, int type, int direction) {
        Order order;
        assert(0 <= type && type < 6);
        order.type = (OrderType)type;
        if (direction == 1) {
            order.dir = Bid;
        } else {
            order.dir = Ask;
        }
        order.volume = volume;
        order.price = price;
        return order;
    }

    bool operator==(const Order& another) const {
        return dir == another.dir && type == another.type && (type == Limit ? price == another.price : true) &&
               volume == another.volume;
    }

    bool operator!=(const Order& another) const { return !(*this == another); }
};

// 成交记录
struct Trade {
    uint32_t bid_id;  // 买方id
    uint32_t ask_id;  // 卖方id
    uint32_t price;
    uint32_t volume;
};

#endif
