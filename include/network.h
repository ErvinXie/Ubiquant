#ifndef NETWORK_H
#define NETWORK_H

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <arpa/inet.h>
#include <sys/socket.h>

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "common.h"

using std::size_t;
using std::uint16_t;
using std::uint32_t;
using std::uint8_t;

const std::uint64_t NETWORK_KEY = 0x6b6da5cff59af717;
constexpr size_t CHUNK_SIZE = 255 * 1024;

extern std::atomic<size_t> network_bytes_tx, network_bytes_rx;

struct Packet {
    uint32_t shard;
    uint32_t seq;
    uint32_t num_bits;
    std::vector<uint8_t> data;

    void check_well_formedness() {
        size_t num_bytes = (num_bits + 7) / 8;
        assert(num_bytes == data.size());
    }

    bool operator<(const Packet& other) const { return seq > other.seq; }
};

class PacketQueue final : public Sink<Packet>, public Stream<Packet> {
    std::condition_variable cv{};
    std::mutex mtx{};
    std::priority_queue<Packet> queue{};

   public:
    virtual void send(Packet packet) override {
        std::unique_lock lk(mtx);
        queue.push(std::move(packet));
        lk.unlock();
        cv.notify_one();
    }

    virtual std::optional<Packet> next() override {
        std::unique_lock lk(mtx);
        while (queue.empty()) {
            cv.wait(lk);
        }
        Packet packet = std::move(queue.top());
        queue.pop();
        return packet;
    }

    size_t size() {
        std::unique_lock lk(mtx);
        return queue.size();
    }
};

std::optional<struct in_addr> parse_address(std::string address);

void network_listen(struct in_addr addr, uint16_t port, std::shared_ptr<PacketQueue> stream,
                    std::shared_ptr<Sink<Packet>> sink);

void network_connect(struct in_addr addr, uint16_t port, struct in_addr self_addr, uint16_t self_port,
                     std::shared_ptr<PacketQueue> stream, std::shared_ptr<Sink<Packet>> sink);

#endif
