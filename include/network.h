#ifndef NETWORK_H
#define NETWORK_H

#include <cassert>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "common.h"

using std::size_t;
using std::uint16_t;
using std::uint32_t;
using std::uint8_t;

constexpr size_t CHUNK_SIZE = 64 * 1024;

struct Packet {
    uint32_t shard;
    uint32_t seq;
    uint32_t num_bits;
    std::vector<uint8_t> data;

    void check_well_formedness() {
        size_t num_bytes = (num_bits + 7) / 8;
        assert(num_bytes == data.size());
    }
};

class PacketQueue final : public Sink<Packet>, public Stream<Packet> {
    std::condition_variable cv;
    std::mutex mtx;
    std::deque<Packet> queue;

   public:
    virtual void send(Packet packet) override {
        std::unique_lock lk(mtx);
        queue.push_back(std::move(packet));
        lk.unlock();
        cv.notify_one();
    }

    virtual std::optional<Packet> next() override {
        std::unique_lock lk(mtx);
        while (queue.empty()) {
            cv.wait(lk);
        }
        Packet packet = std::move(queue.front());
        queue.pop_front();
        return packet;
    }

    void requeue(Packet packet) {
        std::unique_lock lk(mtx);
        queue.push_front(std::move(packet));
        lk.unlock();
        cv.notify_one();
    }
};

std::optional<struct in_addr> parse_address(std::string address);

void network_listen(struct in_addr addr, uint16_t port, std::shared_ptr<PacketQueue> stream,
                    std::shared_ptr<Sink<Packet>> sink);

void network_connect(struct in_addr addr, uint16_t port, std::shared_ptr<PacketQueue> stream,
                     std::shared_ptr<Sink<Packet>> sink);

#endif
