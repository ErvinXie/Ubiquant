#include "persister.h"
#include <iostream>
#include <cstring>

int main()
{
    int max_count = 100000000;
    Persister *persisters[11];

    for (size_t i = 1; i <= 10; i++)
        persisters[i] = new Persister("/data/team-4/", i, max_count);

// // append 1 trade 
//     for (size_t i = 0; i < max_count; i++)
//     {
//         for (size_t j = 1; j <= 10; j++)
//         {
//             trade t;
//             t.ask_id = i;
//             t.bid_id = i + 2;
//             t.price = i - 2;
//             t.stk_code = j;
//             t.volume = i + 10;
//             persisters[j]->append_trade(t);
//         }
//     }

// append trades
    size_t delta = max_count/10;
    for (size_t i = 0; i < max_count; i+=delta)
    {
        trade *tmp =new trade[delta];
        for (size_t k = 0; k < delta; k++)
        {
            trade t;
            t.ask_id = i;
            t.bid_id = i + 2;
            t.price = i + 10;
            t.stk_code = k;
            t.volume = i + 10;
            tmp[k] = t;
        }
        
        for (size_t j = 1; j <= 10; j++)
        {
            persisters[j]->write_trades(tmp,delta);
        }
    }

    return 0;
}
