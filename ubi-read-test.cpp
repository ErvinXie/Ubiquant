#include "ubi-read-write.h"
#include <fstream>
int main()
{
    int sqsize = 1000;
    std::string datapath = "/data/100x" + std::to_string(sqsize) + "x" + std::to_string(sqsize);
    raw_order *raw_orders[11] = {};

    int single_order_max = 100*sqsize*sqsize;

    int* hook = read_hook(datapath.data());
  
    read_all(datapath.data(), "1", single_order_max, raw_orders);
    read_all(datapath.data(), "2", single_order_max, raw_orders);

    // read_all("/data/100x10x10", "1", 100 * 10 * 10);

    for (size_t i = 1; i <= 10; i++)
    {
        std::string path = "/data/team-4/" + std::to_string(sqsize)+"-"+std::to_string(i)+".csv";
        std::ofstream out(path, std::ofstream::out);
        out<<"direction,price(cent),volume,type"<<std::endl;
        for (size_t j = 0; j < single_order_max; j++)
        {
            out << raw_orders[i][j].get_direction() << ","
                << raw_orders[i][j].price << ","
                << raw_orders[i][j].get_volume() << ","
                << raw_orders[i][j].get_type() << std::endl;
        }
        
    }
    
   
    


    return 0;
}