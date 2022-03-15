#include "config.h"

#include "network.h"

std::vector<std::string> split(std::string s, char c) {
    std::stringstream ss(s);
    std::vector<std::string> re;
    std::string x;
    while (std::getline(ss, x, c)) {
        re.push_back(x);
    }
    return re;
}

void usage() {
    printf(
        "Usage:\n"
        "       q or quit or exit; to exit\n"
        "       (exchanger)set portA portB; set listening port from portA to portB \n"
        "       (  trader )set portA portB [portC portD]; set portC to portD, reconnet to exchanger with B\n"
        "       p; show current progress\n");
};

void set_port(std::vector<std::string> xs, Config &conf, std::function<void(int, int, int)> change) {
    if (conf.id == 0 || conf.id == 1) {
        if (xs.size() != 5 || xs.size() != 3) {
            usage();
        }
        auto A = std::stoi(xs[1]);
        auto B = std::stoi(xs[2]);
        int C = -1;
        int D = -1;
        if (xs.size() == 5) {
            C = std::stoi(xs[3]);
            D = std::stoi(xs[4]);
        }

        for (size_t i = 0; i < 4; i++) {
            if (conf.has_connection[i][conf.id]) {
                for (size_t k = 0; k < 3; k++) {
                    if (conf.open_ports[i][conf.id][k] == A) {
                        conf.open_ports[i][conf.id][k] = B;
                        printf("change %s to %zu, from %d to %d\n", conf.identity_string().c_str(), i, A, B);
                        int me_port = conf.open_ports[conf.id][i][k];
                        if (xs.size() == 5) {
                            conf.open_ports[conf.id][i][k] = D;
                        }

                        change(i, B, D);
                    }
                }
            }
        }
    }

    else if (conf.id == 2 || conf.id == 3) {
        if (xs.size() != 3) {
            usage();
            return;
        }
        auto A = std::stoi(xs[1]);
        auto B = std::stoi(xs[2]);
        auto id = conf.id;

        for (size_t i = 0; i < 4; i++) {
            if (conf.has_connection[id][i]) {
                for (size_t k = 0; k < 3; k++) {
                    if (conf.open_ports[id][i][k] == A) {
                        conf.open_ports[id][i][k] = B;
                        INFO("change %s to %zu, from %d to %d\n", conf.identity_string().c_str(), i, A, B);
                        change(-1, B, -1);
                    }
                }
            }
        }
    }
}

Metrics metrics(NR_STOCKS);
const std::time_t start_time = std::time(nullptr);

void cui(Config &conf, std::function<void(int, int, int)> change_port_with) {
    std::string x;
    std::cout << ":";
    while (std::getline(std::cin, x)) {
        if (x == "q" || x == "quit" || x == "exit") {
            std::cout << "Bye" << std::endl;
            break;
        }
        auto xs = split(x, ' ');
        if (!xs.empty()) {
            if (xs[0] == "set") {
                set_port(xs, conf, change_port_with);
            } else if (xs[0] == "info") {
                conf.info();
            } else if (xs[0] == "progress" || xs[0] == "p") {
                for (auto &metric : metrics) {
                    metric.info();
                }
                printf("network: bytes tx = %zu, bytes rx = %zu\n", network_bytes_tx.load(), network_bytes_rx.load());
            } else {
                for (auto i : xs) {
                    std::cout << i << std::endl;
                }

                usage();
            }
        }
        std::cout << ":";
    }
}
