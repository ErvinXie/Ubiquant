#ifndef COMMON_H
#define COMMON_H

#include <cstdio>
#include <optional>
#include <thread>
#include <utility>

template <class Function, class... Args>
void spawn_thread(Function&& f, Args&&... args) {
    std::thread(std::forward<Function>(f), std::forward<Args>(args)...).detach();
}

template <typename T>
struct Sink {
   protected:
    ~Sink() = default;

   public:
    virtual void send(T x) = 0;
};

template <typename T>
struct Stream {
   protected:
    ~Stream() = default;

   public:
    virtual std::optional<T> next() = 0;
};

#define ERROR(...) std::fprintf(stderr, "[ERROR] " __VA_ARGS__)
#define WARN(...) std::fprintf(stderr, "[WARN]  " __VA_ARGS__)
#define INFO(...) std::fprintf(stderr, "[INFO]  " __VA_ARGS__)
#define DEBUG(...) std::fprintf(stderr, "[DEBUG] " __VA_ARGS__)

#endif