#ifndef CODEC_H
#define CODEC_H

#include <cassert>
#include <utility>

#include "bitstream.h"
#include "trade.h"

using std::pair;

constexpr int SHORTINT_LENGTH = 12;

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

    void encode_uint(uint32_t value) {
        int length = 31 - __builtin_clz(value);
        os.put_bits(5, length);
        os.put_bits(length, value);
    }

   public:
    OrderEncoder(uint32_t stk_id, std::shared_ptr<Sink<Packet>> sink) : os(stk_id, sink) {}

    virtual void send(Order order) override {
        encode_direction(order.dir);
        os.put_bits(10, order.volume);
        switch (order.type) {
            case Order::Limit: {
                int32_t diff = ((int32_t)order.price - (int32_t)last_price);
                int32_t sig = diff >> (SHORTINT_LENGTH - 1);
                if (sig == 0 || sig == -1) {
                    os.put_bits(3, 0b000);
                    os.put_bits(SHORTINT_LENGTH, diff);
                } else if (diff > 0) {
                    os.put_bits(3, 0b001);
                    encode_uint(diff);
                } else if (diff < 0) {
                    os.put_bits(3, 0b010);
                    encode_uint(-diff);
                } else {
                    assert(!"unreachable");
                }
                last_price = order.price;
                break;
            }
            case Order::CounterBest:
                os.put_bits(3, 0b011);
                break;
            case Order::ClientBest:
                os.put_bits(3, 0b100);
                break;
            case Order::BestFive:
                os.put_bits(3, 0b101);
                break;
            case Order::FAK:
                os.put_bits(3, 0b110);
                break;
            case Order::FOK:
                os.put_bits(3, 0b111);
                break;
            default:
                assert(!"unrecognized order type!");
        }
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

    uint32_t decode_uint() {
        int length = is.get_bits(5);
        uint32_t value = is.get_bits(length);
        return value | ((uint32_t)1) << length;
    }

   public:
    OrderDecoder(uint32_t stk_id, std::shared_ptr<Stream<Packet>> stream) : is(stk_id, stream) {}

    virtual std::optional<Order> next() override {
        auto dir = decode_direction();
        uint32_t volume = is.get_bits(10);
        Order::OrderType type;
        uint32_t price = 0;
        switch (is.get_bits(3)) {
            case 0b000:
                type = Order::Limit;
                price = (int32_t)(is.get_bits(SHORTINT_LENGTH) << (32 - SHORTINT_LENGTH)) >> (32 - SHORTINT_LENGTH);
                price += last_price;
                break;
            case 0b001:
                type = Order::Limit;
                price = last_price + decode_uint();
                break;
            case 0b010:
                type = Order::Limit;
                price = last_price - decode_uint();
                break;
            case 0b011:
                type = Order::CounterBest;
                break;
            case 0b100:
                type = Order::ClientBest;
                break;
            case 0b101:
                type = Order::BestFive;
                break;
            case 0b110:
                type = Order::FAK;
                break;
            case 0b111:
                type = Order::FOK;
                break;
            default:
                assert(!"unreachable");
        }
        if (type == Order::Limit) {
            last_price = price;
        }
        return Order{
            .dir = dir,
            .type = type,
            .price = price,
            .volume = volume,
        };
    }
};

#endif
