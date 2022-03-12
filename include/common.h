#ifndef COMMON_H
#define COMMON_H

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <string.h>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <optional>
#include <thread>
#include <utility>

using std::size_t;
using std::uint32_t;

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 4
#endif

#define TERMINAL_SET_RED "\u001b[31m"
#define TERMINAL_SET_YELLOW "\u001b[33m"
#define TERMINAL_SET_GREEN "\u001b[32m"
#define TERMINAL_SET_BLUE "\u001b[34m"
#define TERMINAL_SET_MAGENTA "\u001b[35m"
#define TERMINAL_RESET "\u001b[0m"

#if DEBUG_LEVEL > 0
#define ERROR(fmt, ...)                                                                                            \
    std::fprintf(stderr, "[" TERMINAL_SET_RED "ERROR" TERMINAL_RESET "] %s:%d\t%s: " fmt "\n", __FILE__, __LINE__, \
                 __func__, ##__VA_ARGS__)
#else
#define ERROR(...)
#endif

#if DEBUG_LEVEL > 1
#define WARN(fmt, ...)                                                                                                \
    std::fprintf(stderr, "[" TERMINAL_SET_YELLOW "WARN" TERMINAL_RESET "]  %s:%d\t%s: " fmt "\n", __FILE__, __LINE__, \
                 __func__, ##__VA_ARGS__)
#else
#define WARN(...)
#endif

#if DEBUG_LEVEL > 2
#define INFO(fmt, ...)                                                                                              \
    std::fprintf(stderr, "[" TERMINAL_SET_BLUE "INFO" TERMINAL_RESET "]  %s:%d\t%s: " fmt "\n", __FILE__, __LINE__, \
                 __func__, ##__VA_ARGS__)
#else
#define INFO(...)
#endif

#if DEBUG_LEVEL > 3
#define DEBUG(fmt, ...)                                                                                                \
    std::fprintf(stderr, "[" TERMINAL_SET_MAGENTA "DEBUG" TERMINAL_RESET "] %s:%d\t%s: " fmt "\n", __FILE__, __LINE__, \
                 __func__, ##__VA_ARGS__)
#else
#define DEBUG(...)
#endif

struct nocopy {
    nocopy() = default;
    nocopy(const nocopy &) = delete;
    nocopy &operator=(const nocopy &) = delete;
};

template <class Function>
void spawn_thread(std::string name, Function f) {
    std::thread([f = std::move(f), name = std::move(name)]() mutable {
        try {
            f();
            INFO("thread '%s' exited normally", name.c_str());
        } catch (const char *str) {
            ERROR("thread '%s' error caught: %s, exited", name.c_str(), str);
        } catch (std::exception &e) {
            ERROR("thread '%s' exception caught: %s, exited", name.c_str(), e.what());
        } catch (...) {
            ERROR("thread '%s' unidentified exception, exited", name.c_str());
        }
    }).detach();
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

template <typename T>
struct Tee final : Sink<T> {
   private:
    std::shared_ptr<Sink<T>> left, right;

   public:
    Tee(std::shared_ptr<Sink<T>> left, std::shared_ptr<Sink<T>> right) : left(left), right(right) {}

    virtual void send(T x) override {
        left->send(x);
        right->send(std::move(x));
    }
};

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