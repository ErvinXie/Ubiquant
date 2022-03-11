#include <algorithm>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <numeric>

#include "codec.h"
#include "common.h"
#include "config.h"
#include "network.h"
#include "persister.h"
#include "processor.h"
#include "reader.h"
#include "ubi-read-write.h"

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
                   std::shared_ptr<PacketQueue> tx_to_remote, HookChecker checker, HookNotifier notifier, Config conf,
                   raw_order *raw_orders, double prev_close) {
    std::vector<bool> bitmask(NR_ORDERS_SINGLE_STK_HALF * 2);

    OrderIterator *iter = new OrderIterator(raw_orders);

    for (size_t i = 0; i < NR_ORDERS_SINGLE_STK_HALF; i++) {
        bitmask[iter->next_order_id()] = true;
    }
    iter->now_cnt = 1;

    auto self_queue = std::make_shared<PacketQueue>();
    OrderEncoder encoder(stk_id, std::make_shared<Tee<Packet>>(tx_to_remote, self_queue));
    DEBUG("Sending Stock %d\n", stk_id);

    for (size_t i = 0; i < NR_ORDERS_SINGLE_STK_HALF; i++) {
        encoder.send(iter->next());
    }
    DEBUG("Stock %d iter now cnt %d\n", stk_id, iter->now_cnt);
    delete iter;

    OrderMerger order_stream(OrderDecoder(stk_id, self_queue), OrderDecoder(stk_id, rx_from_remote),
                             std::move(bitmask));
    std::string output_file("/data/team-4/trade");
    output_file += std::to_string(stk_id + 1);
    Processor processor(std::move(checker), std::move(notifier), Persister(output_file.c_str(), stk_id),
                        std::lround(prev_close));
    DEBUG("Processing Stock %d\n", stk_id);
    while (auto order = order_stream.next()) {
        DEBUG("std %d, get %d\n",stk_id,order.value().volume);
        processor.process(order.value());
    }
}

int main(int argc, char *argv[]) {
    auto conf = Config::parse_config(argc, argv);

    std::shared_ptr<Demux> demux = std::make_shared<Demux>(NR_STOCKS);
    std::shared_ptr<PacketQueue> tx_to_remote = std::make_shared<PacketQueue>();

    // todo: spawn networking threads
    auto selfaddr = parse_address(conf.ip[conf.id]).value();
    auto e1addr = parse_address(conf.ip[2]).value();
    auto e2addr = parse_address(conf.ip[3]).value();

    for (size_t i = 0; i < 3; i++) {
        network_connect(e1addr, conf.open_ports[2][conf.id][i], selfaddr, conf.open_ports[conf.id][2][i], tx_to_remote,
                        demux);

        network_connect(e2addr, conf.open_ports[3][conf.id][i], selfaddr, conf.open_ports[conf.id][3][i], tx_to_remote,
                        demux);
    }

    raw_order *raw_orders[11] = {};
    read_all(conf.data_path.data(), std::to_string(conf.id + 1).data(), NR_ORDERS_SINGLE_STK_HALF * 2, raw_orders);

    int *prev_close = read_prev_close(conf.data_path.c_str(), std::to_string(conf.id + 1).c_str());
    auto hooks = prepare_hooks(read_hooks(conf.hook_path().c_str()));

    for (size_t i = 0; i < NR_STOCKS; i++) {
        spawn_thread(process_stock, i, demux->get_queue(i), tx_to_remote, std::move(hooks[i].first),
                     std::move(hooks[i].second), conf, raw_orders[i + 1], prev_close[i + 1] / 100.0);
    }
    delete prev_close;

    cui(conf);
    // event loop
}
