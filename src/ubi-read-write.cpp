#include "ubi-read-write.h"

#include <algorithm>
#include <bitset>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include "H5Cpp.h"

namespace fs = std::filesystem;

using namespace H5;

template <typename T>
void read_one_data(const char *path_100x1000x1000, const char *trader, const char *what, raw_order *raw_orders[],
                   int max_order, std::vector<int> &order_id_p) {
    std::string swhat(what), spath(path_100x1000x1000), strader(trader);
    fs::path dir(spath);
    fs::path fname(swhat + strader + ".h5");
    fs::path full_path = dir / fname;
    std::cout << full_path << std::endl;

    const H5std_string FILE_NAME(full_path.c_str());
    const H5std_string DATASET_NAME(swhat.c_str());

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

    const int DX = READ_SLICE_SIZE;
    int buffer_size = 10 * max_order / (NX_SUB / DX);
    assert(NX_SUB % DX == 0);
    printf("Data Read Size: %lf GB\n", sizeof(T) * buffer_size * 1.0);
    for (size_t xidx = 0; xidx < NX_SUB; xidx += DX) {
        printf("reading x %d\n", xidx);

        T *data_read = new T[buffer_size];
        memset(data_read, 0, sizeof(T) * buffer_size);

        hsize_t offset[3];
        hsize_t count[3];
        offset[0] = xidx;
        offset[1] = 0;
        offset[2] = 0;
        count[0] = DX;
        count[1] = NY_SUB;
        count[2] = NZ_SUB;
        dataspace.selectHyperslab(H5S_SELECT_SET, count, offset);  // select in file, this api can set api

        hsize_t dimsm[3];
        dimsm[0] = DX;
        dimsm[1] = NY_SUB;
        dimsm[2] = NZ_SUB;
        DataSpace memspace(RANK_OUT, dimsm);

        hsize_t offset_out[3];
        hsize_t count_out[3];
        offset_out[0] = 0;
        offset_out[1] = 0;
        offset_out[2] = 0;
        count_out[0] = DX;
        count_out[1] = NY_SUB;
        count_out[2] = NZ_SUB;
        memspace.selectHyperslab(H5S_SELECT_SET, count_out, offset_out);  // select in memory

        auto predtype = PredType(PredType::NATIVE_INT);
        if (std::is_same<T, int>()) {
        } else if (std::is_same<T, double>()) {
            predtype = PredType::NATIVE_DOUBLE;
        } else {
            assert(false);
        }

        dataset.read(data_read, predtype, memspace, dataspace);
        for (size_t x = xidx; x < xidx + DX; x++)
            for (size_t y = 0; y < NY_SUB; y++) {
                for (size_t z = 0; z < NZ_SUB; z++) {
                    int mempos = (x - xidx) * (NY_SUB * NZ_SUB) + y * NZ_SUB + z;
                    int pos = x * (NY_SUB * NZ_SUB) + y * NZ_SUB + z;
                    T d = data_read[mempos];

                    int order_id = order_id_p[pos];
                    if (order_id > max_order) {
                        assert(false);
                    }

                    int stk_id = x % 10 + 1;
                    if (swhat == "direction") {
                        raw_orders[stk_id][order_id].set_direction(d);
                    } else if (swhat == "type") {
                        raw_orders[stk_id][order_id].set_type(d);
                    } else if (swhat == "price") {
                        raw_orders[stk_id][order_id].set_price(d);
                    } else if (swhat == "volume") {
                        raw_orders[stk_id][order_id].set_volume(d);
                    } else {
                        assert(false);
                    }
                }
            }

        delete[] data_read;
    }
}

std::vector<int> read_order_id(const char *path_100x1000x1000, const char *trader, int max_order) {
    std::string swhat("order_id"), spath(path_100x1000x1000), strader(trader);
    fs::path dir(spath);
    fs::path fname(swhat + strader + ".h5");
    fs::path full_path = dir / fname;

    const H5std_string FILE_NAME(full_path.c_str());
    const H5std_string DATASET_NAME(swhat.c_str());

    std::vector<int> data_read(10 * max_order);
    printf("Order ID Size: %lf GB\n", sizeof(int) * 10 * max_order / 1e9);

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
    dataspace.selectHyperslab(H5S_SELECT_SET, count, offset);  // select in file, this api can set api

    hsize_t dimsm[3];
    dimsm[0] = NX_SUB;
    dimsm[1] = NY_SUB;
    dimsm[2] = NZ_SUB;
    DataSpace memspace(RANK_OUT, dimsm);

    hsize_t offset_out[3];
    hsize_t count_out[3];
    offset_out[0] = 0;
    offset_out[1] = 0;
    offset_out[2] = 0;
    count_out[0] = NX_SUB;
    count_out[1] = NY_SUB;
    count_out[2] = NZ_SUB;
    memspace.selectHyperslab(H5S_SELECT_SET, count_out, offset_out);  // select in memory

    dataset.read(data_read.data(), PredType::NATIVE_INT, memspace, dataspace);

    return data_read;
}

std::vector<uint32_t> read_prev_close(const char *path_100x1000x1000, const char *trader) {
    std::string swhat("price"), spath(path_100x1000x1000), strader(trader);
    fs::path dir(spath);
    fs::path fname(swhat + strader + ".h5");
    fs::path full_path = dir / fname;
    std::cout << full_path << std::endl;

    const H5std_string FILE_NAME(full_path.c_str());
    const H5std_string DATASET_NAME("prev_close");

    H5File file(FILE_NAME, H5F_ACC_RDONLY);
    DataSet dataset = file.openDataSet(DATASET_NAME);

    DataSpace dataspace = dataset.getSpace();
    int rank = dataspace.getSimpleExtentNdims();

    hsize_t dims_out[3];
    dataspace.getSimpleExtentDims(dims_out, NULL);

    printf("rank %d, shape (%llu)\n", rank, dims_out[0]);

    int NX_SUB;

    const int RANK_OUT = 1;

    NX_SUB = dims_out[0];

    double *data_read = new double[NX_SUB];

    hsize_t offset[1];
    hsize_t count[1];
    offset[0] = 0;
    count[0] = NX_SUB;

    dataspace.selectHyperslab(H5S_SELECT_SET, count, offset);  // select in file, this api can set api

    hsize_t dimsm[1];
    dimsm[0] = NX_SUB;
    DataSpace memspace(RANK_OUT, dimsm);

    hsize_t offset_out[1];
    hsize_t count_out[1];
    offset_out[0] = 0;

    count_out[0] = NX_SUB;
    memspace.selectHyperslab(H5S_SELECT_SET, count_out, offset_out);  // select in memory

    dataset.read(data_read, PredType::NATIVE_DOUBLE, memspace, dataspace);

    std::vector<uint32_t> ret;
    for (size_t i = 0; i < NX_SUB; i++) {
        ret.push_back(std::lround(data_read[i] * 100));
    }
    return ret;
}

int *read_hook(const char *path) {
    const H5std_string file_name(path);
    const H5std_string dataset_name("hook");

    H5File file(file_name, H5F_ACC_RDONLY);
    DataSet dataset = file.openDataSet(dataset_name);

    DataSpace dataspace = dataset.getSpace();
    int rank = dataspace.getSimpleExtentNdims();

    hsize_t dims_out[3];
    dataspace.getSimpleExtentDims(dims_out, NULL);

    printf("rank %d, shape (%llu, %llu, %llu)\n", rank, dims_out[0], dims_out[1], dims_out[2]);

    hsize_t NX_SUB;
    hsize_t NY_SUB;
    hsize_t NZ_SUB;

    const int RANK_OUT = 3;

    NX_SUB = dims_out[0];
    NY_SUB = dims_out[1];
    NZ_SUB = dims_out[2];

    int all_size = NX_SUB * NY_SUB * NZ_SUB;
    int *data_read = new int[all_size];

    hsize_t offset[] = {0, 0, 0};
    hsize_t count[] = {NX_SUB, NY_SUB, NZ_SUB};
    dataspace.selectHyperslab(H5S_SELECT_SET, count, offset);  // select in file, this api can set api

    hsize_t dimsm[] = {NX_SUB, NY_SUB, NZ_SUB};
    DataSpace memspace(RANK_OUT, dimsm);

    hsize_t offset_out[] = {0, 0, 0};
    hsize_t count_out[3];
    count_out[0] = NX_SUB;
    count_out[1] = NY_SUB;
    count_out[2] = NZ_SUB;
    memspace.selectHyperslab(H5S_SELECT_SET, count_out, offset_out);  // select in memory

    dataset.read(data_read, PredType::NATIVE_INT, memspace, dataspace);

    return data_read;
}

void read_all(const char *path_100x1000x1000, const char *trader, int single_stk_order_size, raw_order *raw_orders[]) {
    const char *whats[] = {
        "direction",
        "type",
        "price",
        "volume",
    };
    int64_t total_size = single_stk_order_size * sizeof(raw_order) * 10;
    printf("Raw Order Total Size: %lld B, %lf GB\n", total_size, total_size / 1e9);
    for (size_t i = 1; i <= 10; i++) {
        if (raw_orders[i] == nullptr) {
            raw_orders[i] = new raw_order[single_stk_order_size + 1];
        }
    }

    printf("reading order\n");
    auto oid = read_order_id(path_100x1000x1000, trader, single_stk_order_size);

    for (auto what : whats) {
        printf("\nreading %s\n", what);
        if (strcmp(what, "price") == 0) {
            read_one_data<double>(path_100x1000x1000, trader, what, raw_orders, single_stk_order_size, oid);
        } else {
            read_one_data<int>(path_100x1000x1000, trader, what, raw_orders, single_stk_order_size, oid);
        }
    }

    for (size_t i = 1; i <= 10; i++) {
        printf("stock:%d\n", i);
        for (size_t j = 1; j <= 10; j++) {
            printf("order_id:%d ", j);
            raw_orders[i][j].debug();
        }
    }
}

Order OrderIterator::next() {
    while (start[now_cnt].price == -1) {
        now_cnt++;
    }
    Order x = Order::from_raw(start[now_cnt].price / 100.0, start[now_cnt].get_volume(), start[now_cnt].get_type(),
                              start[now_cnt].get_direction());
    now_cnt++;
    return x;
}

size_t OrderIterator::next_order_id() {
    while (start[now_cnt].price == -1) {
        now_cnt++;
    }
    now_cnt++;
    return now_cnt - 2;
}