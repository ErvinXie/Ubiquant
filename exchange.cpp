#include <condition_variable>
#include <deque>
#include <mutex>

#include "network.h"

using std::condition_variable;
using std::deque;
using std::make_shared;
using std::mutex;
using std::unique_lock;

class MPMC final : PacketSink, PacketStream {
    condition_variable cv;
    mutex mtx;
    deque<Packet> queue;

    virtual Packet next() override {
        unique_lock lk(mtx);
        while (queue.empty()) {
            cv.wait(lk);
        }
        Packet packet = std::move(queue.front());
        queue.pop_front();
        return packet;
    }

    virtual void send(Packet packet) override {
        unique_lock lk(mtx);
        queue.push_back(std::move(packet));
    }

    void requeue(Packet packet) {
        unique_lock lk(mtx);
        queue.push_front(std::move(packet));
    }

    friend class MPMCRequeue;
};

class MPMCRequeue : PacketSink {
    shared_ptr<MPMC> queue;

    virtual void send(Packet packet) override { queue->requeue(std::move(packet)); }

   public:
    MPMCRequeue(shared_ptr<MPMC> queue) : queue(move(queue)) {}
};

shared_ptr<MPMC> mpmc = make_shared<MPMC>();

int main() {}
