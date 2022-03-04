#include <condition_variable>
#include <cstdint>
#include <deque>
#include <map>
#include <mutex>
#include <utility>
#include <vector>

using std::condition_variable;
using std::deque;
using std::map;
using std::mutex;
using std::uint32_t;
using std::uint8_t;
using std::unique_lock;
using std::vector;

class DataQueue {
    mutex mtx;
    condition_variable cv;
    map<uint32_t, vector<uint8_t>> chunks;
    uint32_t counter = 0;

   public:
    DataQueue() = default;

    void insert(uint32_t id, vector<uint8_t> bytes) {
        unique_lock<mutex> lk(mtx);
        chunks[id] = bytes;
        lk.unlock();
        cv.notify_one();
    }

    vector<uint8_t> pop() {
        unique_lock<mutex> lk(mtx);
        cv.wait(lk, [&] { return chunks.count(counter) > 0; });
        auto ret = std::move(chunks[counter]);
        chunks.erase(counter);
        counter++;
        return ret;
    }
};
