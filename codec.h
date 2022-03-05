#include <cassert>
#include <utility>

#include "bitstream.h"
#include "trade.h"

using std::pair;

class OrderEncoder {
    uint32_t last_price;
    OBitStream os;

    void encode_direction(Direction dir) {
        switch (dir) {
            case Bid:
                os.put_bit(0);
                break;
            case Ask:
                os.put_bit(1);
                break;
            default:
                assert(!"invalid direction");
        }
    }

    void encode_type_and_price(Type type, uint32_t price) {
        switch (type) {
            case Limit:
                os.put_bit(0);
                encode_int32((int32_t)price - (int32_t)last_price);
                last_price = price;
                break;
            case CounterBest:
                os.put_bit(1);
                os.put_bit(0);
                os.put_bit(0);
                os.put_bit(0);
                break;
            case ClientBest:
                os.put_bit(1);
                os.put_bit(0);
                os.put_bit(0);
                os.put_bit(1);
                break;
            case BestFive:
                os.put_bit(1);
                os.put_bit(0);
                os.put_bit(1);
                os.put_bit(0);
                break;
            case FAK:
                os.put_bit(1);
                os.put_bit(0);
                os.put_bit(1);
                os.put_bit(1);
                break;
            case FOK:
                os.put_bit(1);
                os.put_bit(1);
                os.put_bit(0);
                os.put_bit(0);
                break;
            default:
                assert(!"unrecognized order type!");
        }
    }

    void encode_int32(int32_t value) { assert(!"unimplemented!"); }

    void encode_uint32(uint32_t value) { assert(!"unimplemented!"); }

   public:
    void encode(Order order) {
        encode_direction(order.dir);
        encode_type_and_price(order.type, order.price);
        encode_uint32(order.volume);
    }
};

class OrderDecoder {
    uint32_t last_price;
    IBitStream is;

    Direction decode_direction() {
        if (is.get_bit() == 0) {
            return Bid;
        } else {
            return Ask;
        }
    }

    pair<Type, uint32_t> decode_type_and_price() {
        if (is.get_bit() == 0) {
            // Limit
            last_price += decode_int32();
            return {Limit, last_price};
        } else {
            if (is.get_bit() == 0) {
                if (is.get_bit() == 0) {
                    if (is.get_bit() == 0) {
                        return {CounterBest, 0};
                    } else {
                        return {ClientBest, 0};
                    }
                } else {
                    if (is.get_bit() == 0) {
                        return {BestFive, 0};
                    } else {
                        return {FAK, 0};
                    }
                }
            } else {
                is.get_bit();
                is.get_bit();
                return {FOK, 0};
            }
        }
    }

    int32_t decode_int32() { assert(!"unimplemeted"); }

    uint32_t decode_uint32() { assert(!"unimplemented!"); }

   public:
    Order decode() {
        Direction dir = decode_direction();
        auto [type, price] = decode_type_and_price();
        uint32_t volume = decode_uint32();
        return Order{
            .order_id = 0,
            .dir = dir,
            .type = type,
            .price = price,
            .volume = volume,
        };
    }
};
