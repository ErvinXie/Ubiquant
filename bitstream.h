#include <algorithm>
#include <cassert>
#include <cstdint>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "network.h"

using std::map;
using std::min;
using std::shared_ptr;
using std::uint32_t;
using std::uint8_t;
using std::vector;

class OBitStream {
    vector<uint8_t> buf(CHUNK_SIZE);
    uint32_t shard_id;
    uint32_t seq;
    uint32_t max_num_bits = CHUNK_SIZE * 8;
    uint32_t cursor;
    shared_ptr<PacketSink> sink;

   public:
    ~OBitStream() { flush(); }

    void put_bit(bool bit) {
        buf[cursor >> 3] |= ((uint8_t)bit << (cursor & 7));
        cursor++;
        while (cursor >= max_num_bits) {
            flush();
        }
    }

    void flush() {
        if (buf.empty() && cursor == 0) {
            return;
        }
        buf.resize((cursor + 7) / 8);
        sink->send(Packet{.shard_id = shard_id, .num_bits = cursor, .seq = seq++, .data = move(buf)});
        cursor = 0;
        buf = vector<uint8_t>(CHUNK_SIZE);
    }
};

class IBitStream {
    vector<uint8_t> buf;
    uint32_t shard_id;
    uint32_t seq;
    uint32_t num_bits;
    uint32_t cursor;
    map<uint32_t, Packet> packets;
    shared_ptr<PacketStream> stream;

    void fetch_packet() {
        auto it = packets.find(seq);
        while (it == packets.end()) {
            auto packet = stream->next();
            if (packet.seq >= seq) {
                packets[packet.seq] = packet;
            } else {
                // discard duplicated packets
            }
            it = packets.find(seq);
        }
        Packet packet = move(it->second);
        packets.erase(it);
        seq++;
        buf = move(packet.data);
        num_bits = packet.num_bits;
        cursor = 0;
    }

   public:
    bool get_bit() {
        while (cursor >= num_bits) {
            fetch_packet();
        }
        bool result = buf[cursor >> 3] & (1 << (cursor & 7));
        cursor++;
        return result;
    }
};
