#include "persister.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>

Persister::Persister(const char *path, int stk_id, int max_trade_count)
    : stk_id(stk_id), max_trade_cnt(max_trade_count), now_cnt(0) {
    size_t max_size = max_trade_count * sizeof(PersistTrade);
    int fd = ::open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    this->fd = fd;
    assert(fd != -1);
    int re = ::ftruncate(fd, max_size);
    assert(re != -1);

    tp = (PersistTrade *)::mmap(NULL, max_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert(tp != MAP_FAILED);

    ::madvise((void *)tp, max_size, MADV_SEQUENTIAL);
}

void Persister::send(Trade trade) {
    tp[now_cnt++] = PersistTrade{
        .stk_code = stk_id,
        .bid_id = (int)trade.bid_id,
        .ask_id = (int)trade.ask_id,
        .price = (double)trade.price / 100.0,
        .volume = (int)trade.volume,
    };
}

Persister::~Persister() {
    ::munmap((void *)tp, max_trade_cnt * sizeof(PersistTrade));
    ::fsync(fd);
    ::ftruncate(fd, now_cnt * sizeof(PersistTrade));
    ::close(fd);
}
