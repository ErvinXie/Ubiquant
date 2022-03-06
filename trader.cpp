#include <condition_variable>
#include <deque>
#include <mutex>

#include "codec.h"
#include "common.h"
#include "network.h"
#include "persister.h"

class Demux final : public Sink<Packet> {
    std::vector<std::shared_ptr<PacketQueue>> queues;

   public:
    explicit Demux(size_t size) {
        for (size_t i = 0; i < size; i++) {
            queues.push_back(std::make_shared<PacketQueue>());
        }
    }

    virtual void send(Packet packet) override { queues.at(packet.shard)->send(packet); }

    std::shared_ptr<Stream<Packet>> get_stream_of(uint32_t shard) { return queues.at(shard); }
};

std::shared_ptr<PacketQueue> tx = std::make_shared<PacketQueue>();
std::shared_ptr<Demux> rx = std::make_shared<Demux>(10);

void transmit_orders(OrderEncoder encoder) {}

void process_exchange() {}

int main() {}
