#include <algorithm>
#include <bitset>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <type_traits>

#include "H5Cpp.h"
#include "trade.h"

namespace fs = std::filesystem;

using namespace H5;

struct hook
{
    int self_order_id;
    int target_stk_code;
    int target_trade_idx;
    int arg;
};

// raw order struct when reading and sorting
struct raw_order
{
    int16_t volume__type__direction;
    int32_t price=-1;

    void set_price(double price) { this->price = static_cast<int32_t>(price * 100); }
    void set_volume(int volume)
    {
        volume__type__direction = volume__type__direction & (0b1111000000000000);
        volume__type__direction = volume__type__direction | static_cast<int16_t>(volume);
    }
    int get_volume() { return volume__type__direction & 0b0000111111111111; }

    void set_type(int type)
    {
        volume__type__direction = volume__type__direction & (0b1000111111111111);
        volume__type__direction = volume__type__direction | (static_cast<int16_t>(type) << 12);
    }

    int get_type() { return (volume__type__direction & 0b0111000000000000) >> 12; }

    void set_direction(int direction)
    {
        if (direction == 1)
            volume__type__direction = volume__type__direction & (0b0111111111111111);
        else if (direction == -1)
            volume__type__direction = volume__type__direction | (0b1000000000000000);
        else
            assert(false);
    };

    int get_direction()
    {
        if (volume__type__direction >= 0)
        {
            return 1;
        }
        else
        {
            return -1;
        }
    }

    void debug()
    {
        if (get_direction() == 1)
        {
            printf(" buy %d with $ %d type %d\n", get_volume(), this->price, get_type());
        }

        else if (get_direction() == -1)
        {
            printf(" sell %d with $ %d type %d\n", get_volume(), this->price, get_type());
        }
        else
        {
            assert(false);
        }
    }

};

struct OrderIterator
{
    raw_order *start;
    size_t now_cnt=1;

    OrderIterator() = delete;
    OrderIterator(raw_order *start) : start(start) {}
    ~OrderIterator() { delete[] start; }

    Order next();
    size_t next_order_id();
};

template <typename T>
void read_one_data(const char *path_100x1000x1000, const char *trader, const char *what, raw_order *raw_orders[], int max_order,
                   int *order_id_p);

int *read_order_id(const char *path_100x1000x1000, const char *trader, int max_order);

int *read_prev_close(const char *path_100x1000x1000, const char *trader);

int *read_hook(const char *path);

void read_all(const char *path_100x1000x1000, const char *trader, int single_stk_order_size, raw_order *raw_orders[]);