#include "config.h"
#include "network.h"
std::shared_ptr<PacketQueue> queues[2] = {std::make_shared<PacketQueue>(), std::make_shared<PacketQueue>()};

int main(int argc, char *argv[]) {
    auto conf = Config::parse_config(argc, argv);

    auto selfip = parse_address(conf.ip[conf.id]).value();

    for (size_t i = 0; i < 3; i++) {
        network_listen(selfip, conf.open_ports[conf.id][0][i], queues[0], queues[1]);
        network_listen(selfip, conf.open_ports[conf.id][1][i], queues[1], queues[0]);
    }

    cui(conf);

    return 0;
}
