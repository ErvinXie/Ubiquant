#include <algorithm>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <numeric>

#include "codec.h"
#include "common.h"
#include "network.h"
#include "persister.h"
#include "processor.h"
#include "reader.h"

class Demux final : public Sink<Packet> {
    std::vector<std::shared_ptr<PacketQueue>> queues;

   public:
    explicit Demux(size_t size) {
        for (size_t i = 0; i < size; i++) {
            queues.push_back(std::make_shared<PacketQueue>());
        }
    }

    virtual void send(Packet packet) override { queues.at(packet.shard)->send(packet); }

    std::shared_ptr<PacketQueue> get_queue(uint32_t shard) { return queues.at(shard); }
};

void process_stock(uint32_t stk_id, std::shared_ptr<PacketQueue> rx_from_remote,
                   std::shared_ptr<PacketQueue> tx_to_remote, HookChecker checker, HookNotifier notifier) {
    OrderList orders = read_orders("/data/100x1000x1000/order_id1.h5", stk_id);
    assert(orders.length == NR_ORDERS_SINGLE);
    std::vector<bool> bitmask(NR_ORDERS_SINGLE * 2);
    std::vector<int> idx(NR_ORDERS_SINGLE);
    std::iota(idx.begin(), idx.end(), 0);
    std::sort(idx.begin(), idx.end(), [&](int x, int y) { return orders.order_id[x] < orders.order_id[y]; });
    for (size_t id : orders.order_id) {
        bitmask[id] = true;
    }
    auto self_queue = std::make_shared<PacketQueue>();
    OrderEncoder encoder(stk_id, std::make_shared<Tee<Packet>>(tx_to_remote, self_queue));
    for (int id : idx) {
        encoder.send(Order::from_raw(orders.price[id], orders.volume[id], orders.type[id], orders.direction[id]));
    }
    OrderMerger order_stream(OrderDecoder(stk_id, self_queue), OrderDecoder(stk_id, rx_from_remote),
                             std::move(bitmask));
    Processor processor(std::move(checker), std::move(notifier), Persister("todo", stk_id),
                        std::lround(orders.last_close / 100.0));
    while (auto order = order_stream.next()) {
        processor.process(order.value());
    }
}

int main() {
    std::shared_ptr<Demux> demux = std::make_shared<Demux>(NR_STOCKS);
    std::shared_ptr<PacketQueue> tx_to_remote = std::make_shared<PacketQueue>();

    // todo: spawn networking threads

    auto hooks = prepare_hooks(read_hooks("/data/100x1000x1000/hook.h5"));
    for (int i = 0; i < NR_STOCKS; i++) {
        spawn_thread(process_stock, i, demux->get_queue(i), tx_to_remote, std::move(hooks[i].first),
                     std::move(hooks[i].second));
    }

    // event loop
}
