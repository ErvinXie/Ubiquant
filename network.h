#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using std::move;
using std::shared_ptr;
using std::string;
using std::uint32_t;
using std::uint8_t;
using std::vector;

constexpr size_t CHUNK_SIZE = 64 * 1024;

struct Packet {
    uint32_t shard;
    uint32_t seq;
    uint32_t num_bits;
    vector<uint8_t> data;

    void check_well_formedness() {
        size_t num_bytes = (num_bits + 7) / 8;
        assert(num_bytes == data.size());
    }
};

class PacketStream {
   public:
    virtual Packet next() = 0;
};

class PacketSink {
   public:
    virtual void send(Packet packet) = 0;
};

void network_listen(string address, shared_ptr<PacketStream> stream, shared_ptr<PacketSink> sink,
                    shared_ptr<PacketSink> requeue);

void network_connect(string address, shared_ptr<PacketStream> stream, shared_ptr<PacketSink> sink,
                     shared_ptr<PacketSink> requeue);
