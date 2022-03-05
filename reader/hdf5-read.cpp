#include <iostream>
#include <cstring>
#include <algorithm>

#include "H5Cpp.h"
using namespace H5;

const H5std_string FILE_NAME("/data/100x1000x1000/volume2.h5");
const H5std_string DATASET_NAME("volume");

int NX_SUB = 3;
int NY_SUB = 4;
int NZ_SUB = 2; // read a 3x4x2 matrix

const int RANK_OUT = 3;

using datatype = int;

int main()
{
    datatype *data_read = new datatype[500 * 1000 * 1000];
    memset(data_read, 0, sizeof(datatype) * 500 * 1000 * 1000);
    H5File file(FILE_NAME, H5F_ACC_RDONLY);
    DataSet dataset = file.openDataSet(DATASET_NAME);

    DataSpace dataspace = dataset.getSpace();
    int rank = dataspace.getSimpleExtentNdims();

    hsize_t dims_out[3];
    dataspace.getSimpleExtentDims(dims_out, NULL);

    printf("rank %d, shape (%llu, %llu, %llu)\n", rank, dims_out[0], dims_out[1], dims_out[2]);

    NX_SUB = dims_out[0];
    NY_SUB = dims_out[1];
    NZ_SUB = dims_out[2];

    hsize_t offset[3];
    hsize_t count[3];
    offset[0] = 0;
    offset[1] = 0;
    offset[2] = 0;
    count[0] = NX_SUB;
    count[1] = NY_SUB;
    count[2] = NZ_SUB;
    dataspace.selectHyperslab(H5S_SELECT_SET, count, offset); // select in file, this api can set api

    hsize_t dimsm[3];
    dimsm[0] = 500;
    dimsm[1] = 1000;
    dimsm[2] = 1000;
    DataSpace memspace(RANK_OUT, dimsm);

    hsize_t offset_out[3];
    hsize_t count_out[3];
    offset_out[0] = 0;
    offset_out[1] = 0;
    offset_out[2] = 0;
    count_out[0] = NX_SUB;
    count_out[1] = NY_SUB;
    count_out[2] = NZ_SUB;
    memspace.selectHyperslab(H5S_SELECT_SET, count_out, offset_out); // select in memory

    dataset.read(data_read, PredType::NATIVE_INT, memspace, dataspace);
    // read from file to memory, you can set offset in memory space

    // int cnt = NX_SUB * NY_SUB * NZ_SUB;
    // std::sort(data_read,data_read+cnt);
    // for (size_t i = 1; i < cnt; i++)
    // {
    //     if(data_read[i]==data_read[i-1]){
    //         std::cout<<i-1<<" and "<<i<<": "<<data_read[i]<<std::endl;
    //         break;
    //     }
    // }

    // std::cout<<data_read[0]<<" "<<data_read[cnt-1]<<std::endl;

    for (size_t i = 0; i < 5; i++)
    {
        for (size_t j = 0; j < 5; j++)
        {
            for (size_t k = 0; k < 5; k++)
            {
                int pos = i * (1000 * 1000) + j * 1000 + k;
                std::cout<<data_read[pos]<<std::endl;
            }
        }
    }

    int maxnum = data_read[0], maxpos = 0;
    int minnum = data_read[0], minpos = 0;
    for (size_t i = 0; i < NX_SUB; i++)
    {
        for (size_t j = 0; j < NY_SUB; j++)
        {
            for (size_t k = 0; k < NZ_SUB; k++)
            {
                int pos = i * (1000 * 1000) + j * 1000 + k;
                int x = data_read[pos];
                if (x > maxnum)
                {
                    maxnum = x;
                    maxpos = pos;
                }
                if (x < minnum)
                {
                    minnum = x;
                    minpos = pos;
                }
            }
        }
    }
    std::cout << "max: " << maxnum << " at: " << maxpos << " min: " << minnum << " at: " << minpos << std::endl;

    // for (int i = 0; i < 5; ++i) {
    //     for (int j = 0; j < 5; ++j) {
    //         std::cout << data_read[i * (1000 * 1000) + j * 1000 + 0] << " ";
    //     }
    //     std::cout << std::endl;
    // }

    // std::cout << std::endl;
    // for (int i = 0; i < 5; ++i) {
    //     for (int j = 0; j < 5; ++j) {
    //         std::cout << data_read[i * (1000 * 1000) + j * 1000 + 1] << " ";
    //     }
    //     std::cout << std::endl;
    // }

    // std::cout << std::endl;
    // for (int i = 0; i < 5; ++i) {
    //     for (int j = 0; j < 5; ++j) {
    //         std::cout << data_read[i * (1000 * 1000) + j * 1000 + 2] << " ";
    //     }
    //     std::cout << std::endl;
    // }
    delete[] data_read;

    return 0;
}
