#ifndef EXCHANGER_H
#define EXCHANGER_H

#include "persister.h"
#include "trade.h"

// 单支股票的撮合器
struct Exchanger : Sink<Order> {
    Persister persister;
    // add exchanger definition

    // Insert an order to the exchanger, return the result trades.
    virtual void send(Order order) override;
};

#endif