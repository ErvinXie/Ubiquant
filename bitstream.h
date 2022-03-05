#include <cassert>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "network.h"

using std::shared_ptr;
using std::uint32_t;
using std::uint8_t;
using std::vector;

class OBitStream {
    vector<uint8_t> buf;
    int cursor;
    uint8_t current;
    shared_ptr<DataQueue> queue;

   public:
    void put_bit(bool bit) {
        current |= (1 << cursor);
        if (++cursor == 8) {
            buf.push_back(current);
            current = 0;
            cursor = 0;
        }
    }

    void put_varuint(uint32_t value) { assert(!"unimplemented"); }

    void put_varint(int32_t value) { assert(!"unimplemented"); }
};

class IBitStream {
    vector<uint8_t> buf;
    int ptr;
    int cursor;
    uint8_t current;
    shared_ptr<DataQueue> queue;

   public:
    bool get_bit() { assert(!"unimplemented"); }

    uint32_t get_varuint() { assert(!"unimplemented"); }

    uint32_t get_varint() { assert(!"unimplemented"); }
};