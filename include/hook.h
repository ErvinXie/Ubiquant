#ifndef HOOK_H
#define HOOK_H

#include <algorithm>
#include <future>
#include <vector>

#include "common.h"

struct Hook {
    uint32_t src_stk_code;
    uint32_t self_order_id;
    uint32_t target_stk_code;
    uint32_t target_trade_id;
    uint32_t threshold;
};

class Future {
    uint32_t _M_order_id;
    std::future<bool> future;

   public:
    Future(uint32_t order_id, std::future<bool>&& future) : _M_order_id(order_id), future(std::move(future)) {}

    Future(Future&&) = default;

    Future& operator=(Future&&) = default;

    uint32_t order_id() const { return _M_order_id; }

    bool get() { return future.get(); }
};

class Promise {
    uint32_t _M_trade_idx;
    uint32_t threshold;
    std::promise<bool> promise;

   public:
    Promise(uint32_t trade_idx, uint32_t threshold, std::promise<bool>&& promise)
        : _M_trade_idx(trade_idx), threshold(threshold), promise(std::move(promise)) {}

    Promise(Promise&&) = default;

    Promise& operator=(Promise&&) = default;

    uint32_t trade_idx() const { return _M_trade_idx; }

    void resolve(uint32_t volume) { promise.set_value(volume <= threshold); }
};

class HookChecker {
    std::vector<Future> futures;

   public:
    HookChecker(std::vector<Future>&& _futures) : futures(std::move(_futures)) {
        std::sort(futures.begin(), futures.end(),
                  [](const Future& first, const Future& second) { return first.order_id() > second.order_id(); });
    }

    bool check(uint32_t order_id) {
        if (!futures.empty() && futures.back().order_id() == order_id) {
            bool result = futures.back().get();
            futures.pop_back();
            return result;
        } else {
            return true;
        }
    }
};

class HookNotifier {
    uint32_t idx = 0;
    std::vector<Promise> promises;

   public:
    HookNotifier(std::vector<Promise>&& _promises) : promises(std::move(_promises)) {
        std::sort(promises.begin(), promises.end(),
                  [](const Promise& first, const Promise& second) { return first.trade_idx() > second.trade_idx(); });
    }

    HookNotifier(HookNotifier&&) = default;

    HookNotifier& operator=(HookNotifier&&) = default;

    void notify(uint32_t volume) {
        idx++;
        while (!promises.empty() && promises.back().trade_idx() == idx) {
            promises.back().resolve(volume);
            promises.pop_back();
        }
    }

    ~HookNotifier() {
        if (!promises.empty()) {
            ERROR("promises still remain: %zu", promises.size());
            ERROR("final trade_idx: %u, next hook expects: %u", idx, promises.back().trade_idx());
        }
    }
};

std::vector<std::pair<HookChecker, HookNotifier>> prepare_hooks(std::vector<Hook> hooks);

#endif