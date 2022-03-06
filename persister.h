#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <filesystem>
#include <cstring>
#include <cstdio>
#include <cassert>

struct trade
{
    int stk_code;
    int bid_id;
    int ask_id;
    double price;
    int volume;
} __attribute__((packed));

namespace fs = std::filesystem;

struct Persister
{
    int stk_id;
    int max_trade_cnt;
    size_t now_cnt;

    fs::path result_path;
    trade *tp;
    int fd;

    Persister(std::string result_folder, int stk_id, int max_trade_count)
    {
        this->stk_id = stk_id;
        this->max_trade_cnt = max_trade_count;
        now_cnt = 0;


        result_path =
            fs::path(result_folder) /
            fs::path(std::string("trade-") + std::to_string(stk_id) + std::string("-") + std::to_string(max_trade_count));

        size_t max_size = max_trade_count * sizeof(trade);
        int fd = open(result_path.c_str(), O_RDWR | O_CREAT, (mode_t)0600);
        this->fd = fd;
        assert(fd != -1);
        int re = ftruncate(fd, max_size);
        assert(re != -1);

        tp = (trade *)mmap(NULL, max_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        assert(tp != MAP_FAILED);

        madvise((void*)tp,max_size,MADV_SEQUENTIAL);
    }

    Persister() = delete;

    // write num trades from start;
    void write_trades(trade *start, size_t num)
    {
        memcpy((void *)(tp + now_cnt), start, num);
        now_cnt += num;
    }

    void append_trade(trade &trade)
    {
        tp[now_cnt] = trade;
        now_cnt++;
    }

    ~Persister(){
        munmap((void *)tp, max_trade_cnt * sizeof(trade));
        fsync(fd);
        ftruncate(fd, now_cnt*sizeof(trade));
        close(fd);
    }
};
