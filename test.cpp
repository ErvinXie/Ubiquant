#include "ubi-read-write.h"

#include "common.h"

int tar[10] = {5755, 5655, 5822, 5639, 5598, 5493, 5146, 5032, 5077,5178};
int main()
{

    int *hook = read_hook("/data/100x10x10/hook.h5");

    for (size_t i = 0; i < 10; i++)
    {
        for (size_t j = 0; j < 100; j++)
        {
            int *pos = &hook[i * 100 * 4 + j * 4];
            if ((pos[0] < tar[i]+10&&pos[0]>tar[i]-10))
                printf("%d hook:%d self-oid:%d target-stk:%d trade-idx:%d arg:%d\n", i + 1, j, pos[0], pos[1], pos[2], pos[3]);
        }
    }

    delete[] hook;

    return 0;
}