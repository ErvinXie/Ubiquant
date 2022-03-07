#include "reader.h"

#include <cassert>

#include "H5Cpp.h"

std::vector<Hook> read_hooks(const char *path) {
    using namespace H5;

    const H5std_string file_name(path);
    const H5std_string dataset_name("hook");

    H5File file(file_name, H5F_ACC_RDONLY);
    DataSet dataset = file.openDataSet(dataset_name);

    DataSpace dataspace = dataset.getSpace();
    int rank = dataspace.getSimpleExtentNdims();

    hsize_t dims_out[3];
    dataspace.getSimpleExtentDims(dims_out, NULL);

    assert(dims_out[0] == HOOK_DX);
    assert(dims_out[1] == HOOK_DY);
    assert(dims_out[2] == HOOK_DZ);

    int data_read[HOOK_DX][HOOK_DY][HOOK_DZ];

    hsize_t offset[] = {0, 0, 0};
    hsize_t count[] = {HOOK_DX, HOOK_DY, HOOK_DZ};
    dataspace.selectHyperslab(H5S_SELECT_SET, count, offset);

    DataSpace memspace(rank, count);
    memspace.selectHyperslab(H5S_SELECT_SET, count, offset);

    dataset.read(data_read, PredType::NATIVE_INT, memspace, dataspace);

    std::vector<Hook> ret;
    for (size_t stk_code = 0; stk_code < HOOK_DX; stk_code++) {
        for (size_t i = 0; i < HOOK_DY; i++) {
            ret.push_back(Hook{
                .src_stk_code = (uint32_t)stk_code,
                .self_order_id = (uint32_t)data_read[stk_code][i][0],
                .target_stk_code = (uint32_t)data_read[stk_code][i][1],
                .target_trade_id = (uint32_t)data_read[stk_code][i][2],
                .threshold = (uint32_t)data_read[stk_code][i][3],
            });
        }
    }
    return ret;
}

template <typename T>
static std::vector<T> read_column(const char *path, const char *what, uint32_t stk_code) {
    using namespace H5;

    const H5std_string file_name(path);

    H5File file(file_name, H5F_ACC_RDONLY);
    DataSet dataset = file.openDataSet(H5std_string(what));

    DataSpace dataspace = dataset.getSpace();
    int rank = dataspace.getSimpleExtentNdims();
    assert(rank == 3);

    hsize_t dims_out[3];
    dataspace.getSimpleExtentDims(dims_out, NULL);

    assert(dims_out[0] == ORDER_DX);
    assert(dims_out[1] == ORDER_DY);
    assert(dims_out[2] == ORDER_DZ);

    size_t total_size = ORDER_DX / NR_STOCKS * ORDER_DY * ORDER_DZ;
    std::vector<T> data_read(total_size);

    for (size_t x = stk_code; x < ORDER_DX; x += NR_STOCKS) {
        hsize_t offset[] = {x, 0, 0};
        hsize_t count[] = {1, ORDER_DY, ORDER_DZ};
        dataspace.selectHyperslab(H5S_SELECT_SET, count, offset);

        hsize_t out_offset[] = {x / NR_STOCKS, 0, 0};
        hsize_t out_count[] = {1, ORDER_DY, ORDER_DZ};
        DataSpace memspace(3, count);
        memspace.selectHyperslab(H5S_SELECT_SET, out_count, out_offset);

        if constexpr (std::is_same_v<T, int>) {
            dataset.read(data_read.data(), PredType::NATIVE_INT, memspace, dataspace);
        } else if constexpr (std::is_same_v<T, double>) {
            dataset.read(data_read.data(), PredType::NATIVE_DOUBLE, memspace, dataspace);
        } else {
            static_assert(std::is_same_v<T, int> || std::is_same_v<T, double>, "invalid data type");
        }
    }
    return data_read;
}

OrderList read_orders(const char *path, uint32_t stk_code) {
    return OrderList{
        .length = ORDER_DX / NR_STOCKS * ORDER_DY * ORDER_DZ,
        .order_id = read_column<int>(path, "order_id", stk_code),
        .price = read_column<double>(path, "price", stk_code),
        .volume = read_column<int>(path, "volume", stk_code),
        .type = read_column<int>(path, "type", stk_code),
        .direction = read_column<int>(path, "direction", stk_code),
    };
}
