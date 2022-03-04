#include <network.h>

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

using std::shared_ptr;
using std::uint8_t;
using std::vector;

struct OBitStream {
    vector<uint8_t> buf;
    int cursor;
    uint8_t current;
    shared_ptr<DataQueue> queue;

    OBitStream& put_bit(bool bit) {
        current |= (1 << cursor);
        if (++cursor == 8) {
            buf.push_back(current);
            current = 0;
            cursor = 0;
        }
        return *this;
    }

    vector<uint8_t>&& take() { return std::move(buf); }
};

struct IBitStream {
    vector<uint8_t> buf;
    int ptr;
    int cursor;
    uint8_t current;
    shared_ptr<DataQueue> queue;
};