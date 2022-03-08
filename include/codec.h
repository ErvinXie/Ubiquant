#ifndef CODEC_H
#define CODEC_H

#include <cassert>
#include <utility>

#include "bitstream.h"
#include "trade.h"

using std::pair;

class OrderEncoder final : public Sink<Order> {
    uint32_t last_price = 0;
    OBitStream os;

    void encode_direction(Order::Direction dir) {
        switch (dir) {
            case Order::Bid:
                os.put_bit(0);
                break;
            case Order::Ask:
                os.put_bit(1);
                break;
            default:
                assert(!"invalid direction");
        }
    }

    void encode_type_and_price(Order::OrderType type, uint32_t price) {
        switch (type) {
            case Order::Limit:
                os.put_bit(0);
                encode_int32((int32_t)price - (int32_t)last_price);
                last_price = price;
                break;
            case Order::CounterBest:
                os.put_bit(1);
                os.put_bit(0);
                os.put_bit(0);
                os.put_bit(0);
                break;
            case Order::ClientBest:
                os.put_bit(1);
                os.put_bit(0);
                os.put_bit(0);
                os.put_bit(1);
                break;
            case Order::BestFive:
                os.put_bit(1);
                os.put_bit(0);
                os.put_bit(1);
                os.put_bit(0);
                break;
            case Order::FAK:
                os.put_bit(1);
                os.put_bit(0);
                os.put_bit(1);
                os.put_bit(1);
                break;
            case Order::FOK:
                os.put_bit(1);
                os.put_bit(1);
                os.put_bit(0);
                os.put_bit(0);
                break;
            default:
                assert(!"unrecognized order type!");
        }
    }

    void encode_int32(int32_t value) { os.put_bits(32, value); }

   public:
    OrderEncoder(uint32_t stk_id, std::shared_ptr<Sink<Packet>> sink) : os(stk_id, sink) {}

    virtual void send(Order order) override {
        encode_direction(order.dir);
        encode_type_and_price(order.type, order.price);
        os.put_bits(10, order.volume);
    }

    void flush() { os.flush(); }
};

class OrderDecoder final : public Stream<Order> {
    uint32_t last_price = 0;
    IBitStream is;

    Order::Direction decode_direction() {
        if (is.get_bit() == 0) {
            return Order::Bid;
        } else {
            return Order::Ask;
        }
    }

    pair<Order::OrderType, uint32_t> decode_type_and_price() {
        if (is.get_bit() == 0) {
            // Limit
            last_price += decode_int32();
            return {Order::Limit, last_price};
        } else {
            if (is.get_bit() == 0) {
                if (is.get_bit() == 0) {
                    if (is.get_bit() == 0) {
                        return {Order::CounterBest, 0};
                    } else {
                        return {Order::ClientBest, 0};
                    }
                } else {
                    if (is.get_bit() == 0) {
                        return {Order::BestFive, 0};
                    } else {
                        return {Order::FAK, 0};
                    }
                }
            } else {
                is.get_bit();
                is.get_bit();
                return {Order::FOK, 0};
            }
        }
    }

    int32_t decode_int32() { return is.get_bits(32); }

   public:
    OrderDecoder(uint32_t stk_id, std::shared_ptr<Stream<Packet>> stream) : is(stk_id, stream) {}

    virtual std::optional<Order> next() override {
        auto dir = decode_direction();
        auto [type, price] = decode_type_and_price();
        uint32_t volume = is.get_bits(10);
        return Order{
            .dir = dir,
            .type = type,
            .price = price,
            .volume = volume,
        };
    }
};

#endif
