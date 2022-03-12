#include "processor.h"

#include <cassert>

void Processor::process(Order order) {
    if (!checker.check(++order_id)) {
        // filtered by hook
        return;
    }
    switch (order.type) {
        case Order::Limit:
            process_limit(order.dir, order_id, order.price, order.volume);
            break;
        case Order::CounterBest:
            process_counter_best(order.dir, order_id, order.volume);
            break;
        case Order::ClientBest:
            process_client_best(order.dir, order_id, order.volume);
            break;
        case Order::BestFive:
            process_best_five(order.dir, order_id, order.volume);
            break;
        case Order::FAK:
            process_fak(order.dir, order_id, order.volume);
            break;
        case Order::FOK:
            process_fok(order.dir, order_id, order.volume);
            break;
        default:
            assert(!"unrecognized order type");
    }
}

void Processor::process_limit(Order::Direction dir, uint32_t order_id, uint32_t price, uint32_t volume) {
    if (price < min_price || price > max_price) {
        return;
    }
    ComOrder tmp;
    if (dir == Order::Direction::Bid) {
        while (!sell.empty() && volume && sell.top().price <= price) {
            tmp = sell.top();
            sell.pop();
            if (tmp.volume < volume) {
                commit(order_id, tmp.order_id, tmp.price, tmp.volume);
                sell_total -= tmp.volume;
                volume -= tmp.volume;
            } else {
                commit(order_id, tmp.order_id, tmp.price, volume);
                sell_total -= volume;
                if (tmp.volume > volume) {
                    tmp.volume -= volume;
                    sell.push(tmp);
                }
                volume = 0;
                break;
            }
        }
        if (volume) {
            buy.push(ComOrder(order_id, price, volume));
            buy_total += volume;
        }
    } else {
        while (!buy.empty() && volume && buy.top().price >= price) {
            tmp = buy.top();
            buy.pop();
            if (tmp.volume < volume) {
                commit(tmp.order_id, order_id, tmp.price, tmp.volume);
                buy_total -= tmp.volume;
                volume -= tmp.volume;
            } else {
                commit(tmp.order_id, order_id, tmp.price, volume);
                if (tmp.volume > volume) {
                    tmp.volume -= volume;
                    buy_total -= volume;
                    buy.push(tmp);
                }
                volume = 0;
                break;
            }
        }
        if (volume) {
            sell.push(ComOrder(order_id, price, volume));
            sell_total += volume;
        }
    }
}

void Processor::process_counter_best(Order::Direction dir, uint32_t order_id, uint32_t volume) {
    ComOrder tmp;
    if (dir == Order::Direction::Bid) {
        if (sell.empty()) {
            // DEBUG("invalid order: counter empty");
        } else {
            process_limit(dir, order_id, sell.top().price, volume);
        }
    } else if (dir == Order::Direction::Ask) {
        if (buy.empty()) {
            // DEBUG("invalid order: counter empty");
        } else {
            process_limit(dir, order_id, buy.top().price, volume);
        }
    } else {
        assert(!"unreachable");
    }
}

void Processor::process_client_best(Order::Direction dir, uint32_t order_id, uint32_t volume) {
    if (dir == Order::Direction::Bid) {
        if (buy.empty()) return;
        buy.push(ComOrder(order_id, buy.top().price, volume));
        buy_total += volume;
    } else if (dir == Order::Direction::Ask) {
        if (sell.empty()) return;
        sell.push(ComOrder(order_id, sell.top().price, volume));
        sell_total += volume;
    }
}

void Processor::process_best_five(Order::Direction dir, uint32_t order_id, uint32_t volume) {
    ComOrder tmp;
    uint32_t last_price = 0, price_count = 0;
    if (dir == Order::Direction::Bid) {
        while (!sell.empty() && volume) {
            if (sell.top().price != last_price) {
                price_count++;
                if (price_count > 5) break;
                last_price = sell.top().price;
            }
            tmp = sell.top();
            sell.pop();
            if (tmp.volume < volume) {
                commit(order_id, tmp.order_id, tmp.price, tmp.volume);
                sell_total -= tmp.volume;
                volume -= tmp.volume;
            } else {
                commit(order_id, tmp.order_id, tmp.price, volume);
                sell_total -= volume;
                if (tmp.volume > volume) {
                    tmp.volume -= volume;
                    sell.push(tmp);
                }
                volume = 0;
                break;
            }
        }
    } else {
        while (!buy.empty() && volume) {
            if (buy.top().price != last_price) {
                price_count++;
                if (price_count > 5) break;
                last_price = buy.top().price;
            }
            tmp = buy.top();
            buy.pop();
            if (tmp.volume < volume) {
                commit(tmp.order_id, order_id, tmp.price, tmp.volume);
                buy_total -= tmp.volume;
                volume -= tmp.volume;
            } else {
                commit(tmp.order_id, order_id, tmp.price, volume);
                buy_total -= volume;
                if (tmp.volume > volume) {
                    tmp.volume -= volume;
                    buy.push(tmp);
                }
                volume = 0;
                break;
            }
        }
    }
}

void Processor::process_fak(Order::Direction dir, uint32_t order_id, uint32_t volume) {
    ComOrder tmp;
    if (dir == Order::Direction::Bid) {
        while (!sell.empty() && volume) {
            tmp = sell.top();
            sell.pop();
            if (tmp.volume < volume) {
                commit(order_id, tmp.order_id, tmp.price, tmp.volume);
                sell_total -= tmp.volume;
                volume -= tmp.volume;
            } else {
                commit(order_id, tmp.order_id, tmp.price, volume);
                sell_total -= volume;
                if (tmp.volume > volume) {
                    tmp.volume -= volume;
                    sell.push(tmp);
                }
                volume = 0;
                break;
            }
        }
    } else {
        while (!buy.empty() && volume) {
            tmp = buy.top();
            buy.pop();
            if (tmp.volume < volume) {
                commit(tmp.order_id, order_id, tmp.price, tmp.volume);
                buy_total -= tmp.volume;
                volume -= tmp.volume;
            } else {
                commit(tmp.order_id, order_id, tmp.price, volume);
                buy_total -= volume;
                if (tmp.volume > volume) {
                    tmp.volume -= volume;
                    buy.push(tmp);
                }
                volume = 0;
                break;
            }
        }
    }
}

void Processor::process_fok(Order::Direction dir, uint32_t order_id, uint32_t volume) {
    ComOrder tmp;
    if (dir == Order::Direction::Bid) {
        if (sell_total < volume) return;
        while (!sell.empty() && volume) {
            tmp = sell.top();
            sell.pop();
            if (tmp.volume < volume) {
                commit(order_id, tmp.order_id, tmp.price, tmp.volume);
                sell_total -= tmp.volume;
                volume -= tmp.volume;
            } else {
                commit(order_id, tmp.order_id, tmp.price, volume);
                sell_total -= volume;
                if (tmp.volume > volume) {
                    tmp.volume -= volume;
                    sell.push(tmp);
                }
                volume = 0;
                break;
            }
        }
    } else {
        if (buy_total < volume) return;
        while (!buy.empty() && volume) {
            tmp = buy.top();
            buy.pop();
            if (tmp.volume < volume) {
                commit(tmp.order_id, order_id, tmp.price, tmp.volume);
                buy_total -= tmp.volume;
                volume -= tmp.volume;
            } else {
                commit(tmp.order_id, order_id, tmp.price, volume);
                buy_total -= volume;
                if (tmp.volume > volume) {
                    tmp.volume -= volume;
                    buy.push(tmp);
                }
                volume = 0;
                break;
            }
        }
    }
}

void Processor::commit(uint32_t bid_id, uint32_t ask_id, uint32_t price, uint32_t volume) {
    notifier.notify(volume);
    persister.persist(bid_id, ask_id, price, volume);
}
