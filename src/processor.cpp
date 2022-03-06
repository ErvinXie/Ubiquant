#include "processor.h"

void Processor::process(Order order) {
    if (!checker.check(order.order_id)) {
        // filtered by hook
        return;
    }
    switch (order.type) {
        case Order::Limit:
            process_limit(order.dir, order.order_id, order.price, order.volume);
            break;
        case Order::CounterBest:
            process_counter_best(order.dir, order.order_id, order.volume);
            break;
        case Order::ClientBest:
            process_client_best(order.dir, order.order_id, order.volume);
            break;
        case Order::BestFive:
            process_best_five(order.dir, order.order_id, order.volume);
            break;
        case Order::FAK:
            process_fak(order.dir, order.order_id, order.volume);
            break;
        case Order::FOK:
            process_fok(order.dir, order.order_id, order.volume);
            break;
        default:
            assert(!"unrecognized order type");
    }
}

void Processor::process_limit(Order::Direction dir, uint32_t order_id, uint32_t price, uint32_t volume) {}

void Processor::process_counter_best(Order::Direction dir, uint32_t order_id, uint32_t volume) {}

void Processor::process_client_best(Order::Direction dir, uint32_t order_id, uint32_t volume) {}

void Processor::process_best_five(Order::Direction dir, uint32_t order_id, uint32_t volume) {}

void Processor::process_fak(Order::Direction dir, uint32_t order_id, uint32_t volume) {}

void Processor::process_fok(Order::Direction dir, uint32_t order_id, uint32_t volume) {}

void Processor::commit(uint32_t bid_id, uint32_t ask_id, uint32_t price, uint32_t volume) {
    notifier.notify(volume);
    persister.persist(bid_id, ask_id, price, volume);
}
