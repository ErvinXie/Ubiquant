#include "ubi-read-write.h"
int main()
{
    read_all("/data/100x1000x1000","1",100*1000*1000);
    return 0;
}