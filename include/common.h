#ifndef COMMON_H
#define COMMON_H

#include <string.h>

#include <cstdint>
#include <cstdio>
#include <memory>
#include <optional>
#include <thread>
#include <utility>

using std::size_t;
using std::uint32_t;

template <class Function, class... Args>
void spawn_thread(Function &&f, Args &&...args)
{
    std::thread(std::forward<Function>(f), std::forward<Args>(args)...).detach();
}

template <typename T>
struct Sink
{
protected:
    ~Sink() = default;

public:
    virtual void send(T x) = 0;
};

template <typename T>
struct Stream
{
protected:
    ~Stream() = default;

public:
    virtual std::optional<T> next() = 0;
};

template <typename T>
struct Tee final : Sink<T>
{
private:
    std::shared_ptr<Sink<T>> left, right;

public:
    Tee(std::shared_ptr<Sink<T>> left, std::shared_ptr<Sink<T>> right) : left(left), right(right) {}

    virtual void send(T x) override
    {
        left->send(x);
        right->send(std::move(x));
    }
};

#define ERROR(...) std::fprintf(stderr, "[ERROR] " __VA_ARGS__)
#define WARN(...) std::fprintf(stderr, "[WARN]  " __VA_ARGS__)
#define INFO(...) std::fprintf(stderr, "[INFO]  " __VA_ARGS__)
#define DEBUG(...) std::fprintf(stderr, "[DEBUG] " __VA_ARGS__)

constexpr size_t NR_STOCKS = 10;

constexpr size_t ORDER_DX = 500;
constexpr size_t ORDER_DY = 10;
constexpr size_t ORDER_DZ = 10;
constexpr size_t NR_ORDERS_SINGLE_STK_HALF = ORDER_DX * ORDER_DY * ORDER_DZ / NR_STOCKS;

constexpr size_t READ_SLICE_SIZE = 50;

constexpr size_t HOOK_DX = 10;
constexpr size_t HOOK_DY = 100;
constexpr size_t HOOK_DZ = 4;

#endif