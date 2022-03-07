#include "hook.h"

std::vector<std::pair<HookChecker, HookNotifier>> prepare_hooks(std::vector<Hook> hooks) {
    std::vector<std::vector<Future>> futures(NR_STOCKS);
    std::vector<std::vector<Promise>> promises(NR_STOCKS);
    for (auto hook : hooks) {
        std::promise<bool> promise;
        std::future<bool> future = promise.get_future();
        futures[hook.src_stk_code].emplace_back(hook.self_order_id, std::move(future));
        promises[hook.target_stk_code].emplace_back(hook.target_trade_id, hook.threshold, std::move(promise));
    };
    std::vector<std::pair<HookChecker, HookNotifier>> ret;
    for (size_t i = 0; i < NR_STOCKS; i++) {
        ret.emplace_back(HookChecker(std::move(futures[i])), HookNotifier(std::move(promises[i])));
    }
    return ret;
}
