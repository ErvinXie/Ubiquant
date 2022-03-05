#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "network.h"

using std::min;
using std::shared_ptr;
using std::uint32_t;
using std::uint8_t;
using std::vector;

class OBitStream {
    vector<uint8_t> buf;
    int cursor;
    uint8_t current;
    shared_ptr<PacketSink> sink;

    void push() {
        if (cursor == 8) {
            buf.push_back(current);
            current = 0;
            cursor = 0;
        }
    }

   public:
    ~OBitStream() { assert(!"unimplemented, should flush the data"); }

    void put_bit(bool bit) {
        current |= ((uint8_t)bit << cursor);
        cursor++;
        push();
    }
};

class IBitStream {
    vector<uint8_t> buf;
    int ptr;
    int cursor;
    uint8_t current;
    shared_ptr<PacketStream> stream;

   public:
    ~IBitStream() { assert(!"unimplemented, should flush the data"); }

    bool get_bit() { assert(!"unimplemented"); }
};
