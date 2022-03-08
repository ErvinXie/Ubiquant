#include "persister.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>

#include "common.h"

Persister::Persister(const char *path, int stk_id, int max_trade_count)
    : stk_id(stk_id), max_trade_cnt(max_trade_count), now_cnt(0) {
    size_t max_size = max_trade_count * sizeof(Trade);
    fd = ::open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        ERROR("open: %s", strerror(errno));
        throw "failed to open the file";
    }
    if (::ftruncate(fd, max_size) < 0) {
        ERROR("truncate: %s", strerror(errno));
        throw "failed to truncate";
    }

    tp = (Trade *)::mmap(NULL, max_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (tp == MAP_FAILED) {
        ERROR("mmap: %s", strerror(errno));
        throw "failed to mmap";
    }

    ::madvise((void *)tp, max_size, MADV_SEQUENTIAL);
}

void Persister::persist(uint32_t bid_id, uint32_t ask_id, uint32_t price, uint32_t volume) {
    tp[now_cnt++] = Trade{
        .stk_code = stk_id,
        .bid_id = (int)bid_id,
        .ask_id = (int)ask_id,
        .price = (double)price / 100.0,
        .volume = (int)volume,
    };
}

Persister::~Persister() {
    ::munmap((void *)tp, max_trade_cnt * sizeof(Trade));
    if (::ftruncate(fd, now_cnt * sizeof(Trade)) < 0) {
        ERROR("ftruncate: %s", strerror(errno));
    }
    if (::fsync(fd) < 0) {
        ERROR("fsync: %s", strerror(errno));
    }
    ::close(fd);
}
