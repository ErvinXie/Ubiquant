#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using std::move;
using std::optional;
using std::shared_ptr;
using std::string;
using std::uint32_t;
using std::uint8_t;
using std::vector;

struct Packet {
    uint32_t stock_id;
    uint32_t seq_id;
    vector<uint8_t> data;
};

class PacketStream {
   public:
    virtual optional<Packet> next() = 0;
};

class PacketSink {
   public:
    virtual void send(Packet packet) = 0;
};

void network_listen(string address, shared_ptr<PacketStream> stream, shared_ptr<PacketSink> sink);

void network_connect(string address, shared_ptr<PacketStream> stream, shared_ptr<PacketSink> sink);
