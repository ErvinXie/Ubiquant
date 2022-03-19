#ifndef CONFIG_H
#define CONFIG_H
#include <atomic>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "common.h"

struct Measure {
    size_t stk_id;
    std::atomic<size_t> order_processed{};
    std::atomic<size_t> trade_committed{};
    std::atomic<size_t> order_in_book{};
    std::atomic<size_t> seq_no{};

    void info() {
        printf("stock %zu: %zu orders_processed, %zu orders in book, %zu trades committed, seq_no = %zu\n", stk_id + 1,
               order_processed.load(), order_in_book.load(), trade_committed.load(), seq_no.load());
    }

    void increment_order_processed() { order_processed.store(order_processed + 1, std::memory_order_release); }

    void increment_trade_commited() { trade_committed.store(trade_committed + 1, std::memory_order_release); }

    void set_order_in_book(size_t num) { order_in_book.store(num, std::memory_order_release); }

    void set_seq_no(size_t num) { seq_no.store(num, std::memory_order_release); }
};

struct Metrics : std::vector<Measure> {
    explicit Metrics(size_t nr_stocks) : std::vector<Measure>(nr_stocks) {
        for (size_t i = 0; i < nr_stocks; i++) {
            at(i).stk_id = i;
        }
    }
};

struct Config {
    std::string data_path;
    std::string out_path;
    int id;  // 0:Trader1, 1:Trader2, 2:Exchanger1, 3:Exchanger2
    std::string ip[4];
    int open_ports[4][4][3] = {};
    bool has_connection[4][4] = {{0, 0, 1, 1}, {0, 0, 1, 1}, {1, 1, 0, 0}, {1, 1, 0, 0}};

    inline std::string identity_string() {
        switch (id) {
            case 0:
                return std::string("Trader1");
            case 1:
                return std::string("Trader2");
            case 2:
                return std::string("Exchanger1");
            case 3:
                return std::string("Exchanger2");
            default:
                exit(1);
                break;
        }
    }

    inline static Config parse_config(int argc, char *argv[]) {
        if (argc != 3) {
            printf("Usage: config_path self_id(0:Trader1, 1:Trader2, 2:Exchanger1, 3:Exchanger2)\n");
            exit(0);
        }
        Config config;

        config.id = atoi(argv[2]);
        if (config.id < 0 || config.id >= 4) {
            printf("Not a valid id\n");
            exit(1);
        }

        auto inconf = std::ifstream(argv[1]);
        inconf >> config.data_path;
        inconf >> config.out_path;
        for (size_t i = 0; i < 4; i++) {
            inconf >> config.ip[i];
        }
        for (size_t i = 0; i < 4; i++) {
            for (size_t j = 0; j < 4; j++) {
                if (config.has_connection[i][j]) {
                    for (size_t k = 0; k < 3; k++) {
                        inconf >> config.open_ports[i][j][k];
                    }
                }
            }
        }
        return config;
    };

    inline void info() {
        printf("id : %d\n", id);
        for (size_t i = 0; i < 4; i++) {
            printf("server %zu ip %s\n", i, ip[i].c_str());
        }
        for (size_t i = 0; i < 4; i++) {
            for (size_t j = 0; j < 4; j++) {
                if (has_connection[i][j]) {
                    printf("server %zu to sever %zu at port: ", i, j);
                    for (size_t k = 0; k < 3; k++) {
                        printf("%d ", open_ports[i][j][k]);
                    }
                    printf("\n");
                }
            }
        }
    }
    inline std::filesystem::path hook_path() {
        return std::filesystem::path(data_path) / std::filesystem::path("hook.h5");
    }
    inline std::filesystem::path what_path(const char *what) {
        std::filesystem::path filename = std::string(what) + std::to_string(id + 1) + ".h5";
        return std::filesystem::path(data_path) / filename;
    }
};

std::vector<std::string> split(std::string s, char c);
void cui(Config &conf, std::function<void(int, int, int)> change_port_with);
void maintain(int argc, char *argv[]);

extern Metrics metrics;
extern const std::time_t start_time;

#endif