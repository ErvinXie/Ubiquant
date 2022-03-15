#ifndef READER_H
#define READER_H

#include <vector>

#include "common.h"
#include "hook.h"
#include "trade.h"

struct RawData {
    std::vector<uint32_t> prev_close;
    std::vector<std::vector<int>> order_id_pos;
    std::vector<int8_t> direction;
    std::vector<int8_t> type;
    std::vector<double> price;
    std::vector<int> volume;
};

std::shared_ptr<RawData> read_all(const char *path_100x1000x1000, const char *trader);

std::vector<Hook> read_hooks(const char *path);

#endif