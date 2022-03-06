#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "network.h"

using std::shared_ptr;
using std::uint32_t;
using std::uint8_t;

class OBitStream {
    std::vector<uint8_t> buf;
    uint32_t shard;
    uint32_t seq;
    uint32_t max_num_bits;
    uint32_t cursor;
    std::shared_ptr<PacketQueue> sink;

   public:
    OBitStream(uint32_t shard, std::shared_ptr<PacketQueue> sink, uint32_t max_num_bits = CHUNK_SIZE * 8)
        : buf((max_num_bits + 7) / 8), shard(shard), seq(0), max_num_bits(max_num_bits), cursor(0), sink(sink) {}

    ~OBitStream() { flush(); }

    void put_bit(bool bit) {
        buf[cursor >> 3] |= ((uint8_t)bit << (cursor & 7));
        cursor++;
        while (cursor >= max_num_bits) {
            flush();
        }
    }

    void put_bits(int num, uint32_t bits) {
        for (int i = 0; i < num; i++) {
            put_bit(bits & (1 << num));
        }
    }

    void flush() {
        if (buf.empty() && cursor == 0) {
            return;
        }
        buf.resize((cursor + 7) / 8);
        sink->send(Packet{.shard = shard, .seq = seq++, .num_bits = cursor, .data = move(buf)});
        cursor = 0;
        buf = std::vector<uint8_t>((max_num_bits + 7) / 8);
    }
};

class IBitStream {
    std::vector<uint8_t> buf;
    uint32_t shard;
    uint32_t seq;
    uint32_t num_bits;
    uint32_t cursor;
    std::map<uint32_t, Packet> packets;
    std::shared_ptr<PacketQueue> stream;

    void fetch_packet() {
        auto it = packets.find(seq);
        while (it == packets.end()) {
            if (auto optional_packet = stream->next()) {
                auto packet = optional_packet.value();
                assert(packet.shard == shard);
                if (packet.seq >= seq) {
                    packets[packet.seq] = std::move(packet);
                } else {
                    // discard duplicated packets
                }
                it = packets.find(seq);
            } else {
                // eof
            }
        }
        Packet packet = std::move(it->second);
        packets.erase(it);
        seq++;
        buf = move(packet.data);
        num_bits = packet.num_bits;
        cursor = 0;
    }

   public:
    IBitStream(uint32_t shard, std::shared_ptr<PacketQueue> stream) : shard(shard), stream(stream) {}

    uint32_t get_bits(int num) {
        uint32_t ret = 0;
        for (int i = 0; i < num; i++) {
            if (get_bit()) {
                ret |= 1 << i;
            }
        }
        return ret;
    }

    bool get_bit() {
        while (cursor >= num_bits) {
            fetch_packet();
        }
        bool result = buf[cursor >> 3] & (1 << (cursor & 7));
        cursor++;
        return result;
    }
};

#endif
