#include "H5Cpp.h"
#include <string>
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <type_traits>
#include <cstring>
#include <cassert>

namespace fs = std::filesystem;

using namespace H5;

// the order struct given by the pdf
struct given_order
{
    int stk_code;
    int order_id;
    int direction; // 1 buy; -1 sell;
    int type;
    double price;
    int volume;
};

// raw order struct when reading and sorting
struct raw_order
{
    int16_t volume__type__direction;
    int32_t price;

    void set_price(double price)
    {
        this->price = static_cast<int32_t>(price * 100);
    }
    void set_volume(int volume)
    {
        volume__type__direction = volume__type__direction & (0b1111000000000000);
        volume__type__direction = volume__type__direction | static_cast<int16_t>(volume);
    }
    int get_volume()
    {
        return volume__type__direction & 0b0000111111111111;
    }

    void set_type(int type)
    {
        volume__type__direction = volume__type__direction & (0b1000111111111111);
        volume__type__direction = volume__type__direction | (static_cast<int16_t>(type) << 12);
    }

    int get_type()
    {
        return volume__type__direction & 0b0111000000000000;
    }

    void set_direction(int direction)
    {
        if (direction == 1)
            volume__type__direction = volume__type__direction & (0b0111111111111111);
        else if (direction == -1)
            volume__type__direction = volume__type__direction | (0b1000000000000000);
        else
            assert(false);
    };

    int get_direction()
    {
        if (volume__type__direction >= 0)
        {
            return 1;
        }
        else
        {
            return -1;
        }
    }

    void debug()
    {
        if (get_direction() == 1)
        {
            printf(" buy %d with $ %d type %d\n", get_volume(), this->price, get_type());
        }

        else if (get_direction() == -1)
        {
            printf(" sell %d with $ %d type %d\n", get_volume(), this->price, get_type());
        }
        else
        {
            assert(false);
        }
    }

    raw_order order_compress(given_order x)
    {
        raw_order r;
        r.set_price(x.price);
        r.set_volume(x.volume);
        r.set_type(x.type);
        r.set_direction(x.direction);
        return r;
    }
};

template <typename T>
T *read_one_data(char *path_100x1000x1000, char *trader, char *what, raw_order *raw_orders[], int max_order, int *order_id_p)
{
    std::string swhat(what), spath(path_100x1000x1000), strader(trader);
    fs::path dir(spath);
    fs::path fname(swhat + strader + ".h5");
    fs::path full_path = dir / fname;
    std::cout << full_path << std::endl;

    const H5std_string FILE_NAME(full_path.c_str());
    const H5std_string DATASET_NAME(swhat.c_str());

    T *data_read = new T[10 * max_order];
    memset(data_read, 0, sizeof(T) * 10 * max_order);
    printf("Data Read Size: %lld B %lf GB\n", sizeof(T) * 10 * max_order, sizeof(T) * 10 * max_order / 1e9);

    H5File file(FILE_NAME, H5F_ACC_RDONLY);
    DataSet dataset = file.openDataSet(DATASET_NAME);

    DataSpace dataspace = dataset.getSpace();
    int rank = dataspace.getSimpleExtentNdims();

    hsize_t dims_out[3];
    dataspace.getSimpleExtentDims(dims_out, NULL);

    printf("rank %d, shape (%llu, %llu, %llu)\n", rank, dims_out[0], dims_out[1], dims_out[2]);

    int NX_SUB;
    int NY_SUB;
    int NZ_SUB;

    const int RANK_OUT = 3;

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

    auto predtype = PredType(PredType::NATIVE_INT);
    if (std::is_same<T, int>())
    {
    }
    else if (std::is_same<T, double>())
    {
        predtype = PredType::NATIVE_DOUBLE;
    }
    else
    {
        assert(false);
    }
    dataset.read(data_read, predtype, memspace, dataspace);

    if (swhat == "order_id")
    {
        return data_read;
    }
    for (size_t x = 0; x < NX_SUB; x++)
    {
        for (size_t y = 0; y < NY_SUB; y++)
        {
            for (size_t z = 0; z < NZ_SUB; z++)
            {
                int pos = x * (1000 * 1000) + y * 1000 + z;
                T d = data_read[pos];

                int order_id = order_id_p[pos];
                if (order_id > max_order)
                {
                    assert(false);
                }

                int stk_id = x % 10;
                if (swhat == "direction")
                {
                    raw_orders[stk_id][order_id].set_direction(d);
                }
                else if (swhat == "type")
                {
                    raw_orders[stk_id][order_id].set_type(d);
                }
                else if (swhat == "price")
                {
                    raw_orders[stk_id][order_id].set_price(d);
                }
                else if (swhat == "volume")
                {
                    raw_orders[stk_id][order_id].set_volume(d);
                }
                else
                {
                    assert(false);
                }
            }
        }
    }
    delete[] data_read;
    return nullptr;
}

void read_all(char *path_100x1000x1000, char *trader, int single_stk_order_size)
{
    char *whats[] = {
        "direction",
        "type",
        "price",
        "volume",
    };
    int64_t total_size = single_stk_order_size * sizeof(raw_order) * 10;
    printf("Raw Order Total Size: %lld B, %lf GB\n", total_size, total_size / 1e9);
    raw_order *raw_orders[11];
    for (size_t i = 1; i <= 10; i++)
    {
        raw_orders[i] = new raw_order[single_stk_order_size+1];
    }

    auto oid = read_one_data<int>(path_100x1000x1000, trader, "order_id", raw_orders, single_stk_order_size, nullptr);

    for (auto what : whats)
    {
        if (what == "price")
        {
            read_one_data<double>(path_100x1000x1000, trader, what, raw_orders, single_stk_order_size, oid);
        }
        else
        {
            read_one_data<int>(path_100x1000x1000, trader, what, raw_orders, single_stk_order_size, oid);
        }
    }
    delete[] oid;
    for (size_t i = 1; i <= 10; i++)
    {
        for (size_t j = 1; j <= 10; j++)
        {
            raw_orders[i][j].debug();
        }
        for (size_t j = single_stk_order_size - 10; j < single_stk_order_size; j++)
        {
            raw_orders[i][j].debug();
        }
    }
}
