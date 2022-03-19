#include "reader.h"

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <string>
#include <type_traits>
#include <vector>

#include "H5Cpp.h"

using std::int8_t;

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
                .target_stk_code = (uint32_t)data_read[stk_code][i][1] - 1,
                .target_trade_id = (uint32_t)data_read[stk_code][i][2],
                .threshold = (uint32_t)data_read[stk_code][i][3],
            });
        }
    }
    INFO("hook read (%lld, %lld, %lld)", dims_out[0], dims_out[1], dims_out[2]);
    return ret;
}

template <typename T>
static std::vector<T> read_one(const char *path_100x1000x1000, const char *trader, const char *what) {
    using namespace H5;

    std::string swhat(what), spath(path_100x1000x1000), strader(trader);
    std::filesystem::path dir(spath);
    std::filesystem::path fname(swhat + strader + ".h5");
    std::filesystem::path full_path = dir / fname;

    const H5std_string FILE_NAME(full_path.c_str());
    const H5std_string DATASET_NAME(swhat.c_str());

    std::vector<T> data_read(ORDER_DX * ORDER_DY * ORDER_DZ);
    INFO("reading %s, data size: %lf GB", what, sizeof(T) * data_read.size() / 1e9);

    H5File file(FILE_NAME, H5F_ACC_RDONLY);
    DataSet dataset = file.openDataSet(DATASET_NAME);

    DataSpace dataspace = dataset.getSpace();
    int rank = dataspace.getSimpleExtentNdims();

    hsize_t dims_out[3];
    dataspace.getSimpleExtentDims(dims_out, NULL);

    INFO("rank %d, shape (%llu, %llu, %llu)", rank, dims_out[0], dims_out[1], dims_out[2]);

    const int RANK_OUT = 3;

    size_t NX_SUB = dims_out[0];
    size_t NY_SUB = dims_out[1];
    size_t NZ_SUB = dims_out[2];

    assert(NX_SUB == ORDER_DX);
    assert(NY_SUB == ORDER_DY);
    assert(NZ_SUB == ORDER_DZ);

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

    if constexpr (std::is_same_v<T, int>) {
        dataset.read(data_read.data(), PredType::NATIVE_INT, memspace, dataspace);
    } else if constexpr (std::is_same_v<T, double>) {
        dataset.read(data_read.data(), PredType::NATIVE_DOUBLE, memspace, dataspace);
    } else if constexpr (std::is_same_v<T, int8_t>) {
        dataset.read(data_read.data(), PredType::NATIVE_INT8, memspace, dataspace);
    } else {
        assert(!"unreachable");
    }
    file.close();
    return std::move(data_read);  // prevent copy on compilers without NRVO
}

static std::vector<uint32_t> read_prev_close(const char *path_100x1000x1000, const char *trader) {
    using namespace H5;

    std::string swhat("price"), spath(path_100x1000x1000), strader(trader);
    std::filesystem::path dir(spath);
    std::filesystem::path fname(swhat + strader + ".h5");
    std::filesystem::path full_path = dir / fname;
    // std::cout << full_path << std::endl;

    const H5std_string FILE_NAME(full_path.c_str());
    const H5std_string DATASET_NAME("prev_close");

    H5File file(FILE_NAME, H5F_ACC_RDONLY);
    DataSet dataset = file.openDataSet(DATASET_NAME);

    DataSpace dataspace = dataset.getSpace();
    int rank = dataspace.getSimpleExtentNdims();
    assert(rank == 1);

    hsize_t dims_out[3];
    dataspace.getSimpleExtentDims(dims_out, NULL);

    // printf("rank %d, shape (%llu)\n", rank, dims_out[0]);

    size_t NX_SUB;

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
    file.close();
    return ret;
}

std::shared_ptr<RawData> read_all(const char *path_100x1000x1000, const char *trader) {
    INFO("reading orders\n");
    auto fut_order_id = std::async(std::launch::async, read_one<int>, path_100x1000x1000, trader, "order_id");
    auto fut_direction = std::async(std::launch::async, read_one<int8_t>, path_100x1000x1000, trader, "direction");
    auto fut_type = std::async(std::launch::async, read_one<int8_t>, path_100x1000x1000, trader, "type");
    auto fut_price = std::async(std::launch::async, read_one<double>, path_100x1000x1000, trader, "price");
    auto fut_volume = std::async(std::launch::async, read_one<int>, path_100x1000x1000, trader, "volume");

    std::vector<std::vector<int>> order_id_pos;
    for (size_t i = 0; i < NR_STOCKS; i++) {
        order_id_pos.emplace_back(NR_ORDERS_SINGLE_STK_HALF * 2, -1);
    }

    auto order_id = fut_order_id.get();

    for (size_t x = 0; x < ORDER_DX; x++) {
        for (size_t y = 0; y < ORDER_DY; y++) {
            for (size_t z = 0; z < ORDER_DZ; z++) {
                int idx = (x * ORDER_DY + y) * ORDER_DZ + z;
                int id = order_id[idx] - 1;
                order_id_pos[x % 10][id] = idx;
            }
        }
    }

    return std::make_shared<RawData>(RawData{
        .prev_close = read_prev_close(path_100x1000x1000, trader),
        .order_id_pos = std::move(order_id_pos),
        .direction = std::move(fut_direction.get()),
        .type = std::move(fut_type.get()),
        .price = std::move(fut_price.get()),
        .volume = std::move(fut_volume.get()),
    });
}
