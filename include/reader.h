#ifndef READER_H
#define READER_H

#include <vector>

#include "common.h"
#include "hook.h"

struct OrderList {
    double last_close;
    size_t length;
    std::vector<int> order_id;
    std::vector<double> price;
    std::vector<int> volume;
    std::vector<int> type;
    std::vector<int> direction;
};

std::vector<Hook> read_hooks(const char* path);

OrderList read_orders(const char* path, uint32_t stk_id);

#endif