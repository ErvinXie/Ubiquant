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
#define DEBUG_LEVEL 3
#endif

#ifndef TERMINAL_COLOR
#define TERMINAL_COLOR 1
#endif

#if TERMINAL_COLOR
#define TERMINAL_SET_RED "\u001b[31m"
#define TERMINAL_SET_YELLOW "\u001b[33m"
#define TERMINAL_SET_GREEN "\u001b[32m"
#define TERMINAL_SET_BLUE "\u001b[34m"
#define TERMINAL_SET_MAGENTA "\u001b[35m"
#define TERMINAL_RESET "\u001b[0m"
#else
#define TERMINAL_SET_RED
#define TERMINAL_SET_YELLOW
#define TERMINAL_SET_GREEN
#define TERMINAL_SET_BLUE
#define TERMINAL_SET_MAGENTA
#define TERMINAL_RESET
#endif

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

#if DEBUG_LEVEL > 4
#define TRACE(fmt, ...)                                                                                              \
    std::fprintf(stderr, "[" TERMINAL_SET_GREEN "TRACE" TERMINAL_RESET "] %s:%d\t%s: " fmt "\n", __FILE__, __LINE__, \
                 __func__, ##__VA_ARGS__)
#else
#define TRACE(...)
#endif

struct nocopy {
    nocopy() = default;
    nocopy(const nocopy &) = delete;
    nocopy &operator=(const nocopy &) = delete;
};

template <class Function, class... Args>
inline void thread_guard(std::string name, Function &&f, Args &&...args) {
    try {
        f(std::forward<Args>(args)...);
        INFO("thread '%s' exited normally", name.c_str());
    } catch (const char *str) {
        ERROR("thread '%s' error caught: %s, exited", name.c_str(), str);
    } catch (std::exception &e) {
        ERROR("thread '%s' exception caught: %s, exited", name.c_str(), e.what());
    } catch (...) {
        ERROR("thread '%s' unidentified exception, exited", name.c_str());
    }
}

template <class Function, class... Args>
[[nodiscard]] inline std::thread create_thread(std::string name, Function &&f, Args &&...args) {
    return std::thread(thread_guard<std::decay_t<Function>, std::decay_t<Args>...>, std::move(name),
                       std::forward<Function>(f), std::forward<Args>(args)...);
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

template <typename T>
struct ScopeGuard {
   private:
    T func;
    bool valid = true;

   public:
    ScopeGuard(T &&t) : func(std::forward<T>(t)) {}

    void reset() { valid = false; }

    ~ScopeGuard() {
        if (valid) {
            func();
        }
    }
};

constexpr size_t NR_STOCKS = 10;

constexpr size_t ORDER_DX = 500;
constexpr size_t ORDER_DY = 1000;
constexpr size_t ORDER_DZ = 1000;
constexpr size_t NR_ORDERS_SINGLE_STK_HALF = ORDER_DX * ORDER_DY * ORDER_DZ / NR_STOCKS;

constexpr size_t READ_SLICE_SIZE = 100;

constexpr size_t HOOK_DX = 10;
constexpr size_t HOOK_DY = 100;
constexpr size_t HOOK_DZ = 4;

#endif