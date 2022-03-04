#include <bitstream.h>
#include <trade.h>

#include <cassert>

class OrderEncoder {
    uint32_t last_order_id;
    uint32_t last_price;
    OBitStream os;

   public:
    void encode(Order order) {
        switch (order.type) {
            case Limit:

                break;
            case CounterBest:

                break;
            case ClientBest:

                break;
            case BestFive:

                break;
            case FAK:

                break;
            case FOK:

                break;
            default:
                assert(!"unrecognized order type!");
        }
    }
};

struct OrderDecoder {
    uint32_t last_order_id;
    uint32_t last_price;
    IBitStream is;
};