#ifndef COMMON_H
#define COMMON_H

#include <optional>
#include <thread>
#include <utility>

template <class Function, class... Args>
void spawn_thread(Function&& f, Args&&... args) {
    std::thread(std::forward<Function>(f), std::forward<Args>(args)...).detach();
}

template <typename T>
struct Sink {
   public:
    virtual void send(T x) = 0;
    virtual void close() {}
};

template <typename T>
struct Stream {
   public:
    virtual std::optional<T> next() = 0;
};

#endif