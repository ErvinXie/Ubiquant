#include <algorithm>
#include <condition_variable>
#include <csignal>
#include <ctime>
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

class Demux final : public Sink<Packet>
{
    std::vector<std::shared_ptr<PacketQueue>> queues;

public:
    explicit Demux(size_t size)
    {
        for (size_t i = 0; i < size; i++)
        {
            queues.push_back(std::make_shared<PacketQueue>());
        }
    }

    virtual void send(Packet packet) override { queues.at(packet.shard)->send(packet); }

    std::shared_ptr<PacketQueue> get_queue(uint32_t shard) { return queues.at(shard); }
};

void process_stock(uint32_t stk_id, std::shared_ptr<PacketQueue> rx_from_remote,
                   std::shared_ptr<PacketQueue> tx_to_remote, HookChecker checker, HookNotifier notifier,
                   std::shared_ptr<RawData> raw_data, std::filesystem::path out_path)
{
    auto prev_close = raw_data->prev_close[stk_id];

    std::vector<int> pos = std::move(raw_data->order_id_pos[stk_id]);

    auto self_queue = std::make_shared<PacketQueue>();

    OrderEncoder encoder(stk_id, std::make_shared<Tee<Packet>>(tx_to_remote, self_queue));
    INFO("encoding stock %u", stk_id);

    for (auto idx : pos)
    {
        if (idx == -1)
            continue;
        encoder.send(Order{
            .dir = (Order::Direction)(raw_data->direction[idx]),
            .type = (Order::OrderType)(raw_data->type[idx]),
            .price = (uint32_t)(lround(raw_data->price[idx] * 100)),
            .volume = (uint32_t)(raw_data->volume[idx]),
        });
    }
    encoder.flush();
    raw_data.reset();

    INFO("start merging stream stock %u", stk_id);

    OrderMerger order_stream(OrderDecoder(stk_id, self_queue), OrderDecoder(stk_id, rx_from_remote), std::move(pos));
    std::filesystem::path output_file = out_path / (std::string("trade") + std::to_string(stk_id + 1));

    Processor processor(stk_id, std::move(checker), std::move(notifier), Persister(output_file.c_str(), stk_id + 1),
                        prev_close);
    INFO("Processing Stock %u", stk_id);
    while (auto order = order_stream.next())
    {
        processor.process(order.value());
    }
    INFO("stock %u finished", stk_id);
}

void daemon(std::vector<std::thread> threads)
{
    for (auto &th : threads)
    {
        th.join();
    }
    INFO("all stocks are completed, time = %lfs", std::difftime(std::time(nullptr), start_time));
}

int main(int argc, char *argv[])
{
    std::signal(SIGPIPE, SIG_IGN);
    Config conf = Config::parse_config(argc, argv);

    std::shared_ptr<Demux> demux = std::make_shared<Demux>(NR_STOCKS);
    std::shared_ptr<PacketQueue> tx_to_remote = std::make_shared<PacketQueue>();

    // todo: spawn networking threads
    auto selfaddr = parse_address(conf.ip[conf.id]).value();
    auto e1addr = parse_address(conf.ip[2]).value();
    auto e2addr = parse_address(conf.ip[3]).value();

    for (size_t i = 0; i < 3; i++)
    {
        network_connect(e1addr, conf.open_ports[2][conf.id][i], selfaddr, conf.open_ports[conf.id][2][i], conf.id,
                        tx_to_remote, demux);

        network_connect(e2addr, conf.open_ports[3][conf.id][i], selfaddr, conf.open_ports[conf.id][3][i], conf.id,
                        tx_to_remote, demux);
    }

    std::shared_ptr<RawData> raw_data = read_all(conf.data_path.c_str(), std::to_string(conf.id + 1).c_str());
    auto hooks = prepare_hooks(read_hooks(conf.hook_path().c_str()));

    std::vector<std::thread> threads;
    for (uint32_t i = 0; i < NR_STOCKS; i++)
    {
        threads.push_back(create_thread("exchanger " + std::to_string(i), process_stock, i, demux->get_queue(i),
                                        tx_to_remote, std::move(hooks[i].first), std::move(hooks[i].second), raw_data, conf.out_path));
    }

    raw_data.reset();

    INFO("start exchanging, time = %lfs", std::difftime(std::time(nullptr), start_time));

    create_thread(std::string("daemon"), daemon, std::move(threads)).detach();

    cui(conf, [=](int eid, int eport, int meport)
        {
        auto eaddr = parse_address(conf.ip[eid]).value();
        network_connect(eaddr, eport, selfaddr, meport, conf.id, tx_to_remote, demux); });
}
