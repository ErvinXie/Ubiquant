#include <cstdint>
#include <vector>

using std::uint32_t;
using std::vector;

enum Direction {
    Bid = 1,   // 买入
    Ask = -1,  // 卖出
};

enum Type {
    Limit = 0,        // 限价申报
    CounterBest = 1,  // 对手方最优
    ClientBest = 2,   // 本方最优
    BestFive = 3,     // 最优五档剩余撤销
    FAK = 4,          // 即时成交剩余撤销
    FOK = 5,          // 全额成交或撤销
};

// 订单
struct Order {
    uint32_t order_id;
    Direction dir;
    Type type;
    uint32_t price;  // 仅限价单有意义
    uint32_t volume;
};

// 成交记录
struct Trade {
    uint32_t bid_id;    // 买方id
    uint32_t trade_id;  // 卖方id
    uint32_t price;
    uint32_t volume;
};

// 单支股票的撮合器
struct Exchanger {
    // add exchanger definition

    // Insert an order to the exchanger, return the result trades.
    vector<Trade> insert(Order order);
};
