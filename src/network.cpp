#include "network.h"

#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#include <cassert>
#include <chrono>
#include <cstdint>
#include <type_traits>

#include "common.h"

const int BACKLOG_SIZE = 16;

std::atomic<size_t> network_bytes_tx{}, network_bytes_rx{};

class Fd {
    int fd;

   public:
    explicit Fd(int fd) : fd(fd) {}
    ~Fd() { ::close(fd); }

    operator int() { return fd; }
};

struct PacketHeader {
    uint32_t shard;
    uint32_t seq;
    uint32_t num_bits;
};

void read_exact(int fd, char *buf, size_t nbytes) {
    size_t bytes_read = 0;
    while (bytes_read < nbytes) {
        ssize_t ret = ::read(fd, buf + bytes_read, nbytes - bytes_read);
        if (ret < 0) {
            ERROR("read: %s", strerror(errno));
            throw "read failed";
        }
        bytes_read += ret;
    }
    network_bytes_rx += nbytes;
}

void write_exact(int fd, const char *buf, size_t nbytes) {
    size_t bytes_written = 0;
    while (bytes_written < nbytes) {
        ssize_t ret = ::write(fd, buf + bytes_written, nbytes - bytes_written);
        if (ret < 0) {
            ERROR("write: %s", strerror(errno));
            throw "write failed";
        }
        bytes_written += ret;
    }
    network_bytes_tx += nbytes;
}

class Receiver {
    std::shared_ptr<Fd> sockfd;
    std::shared_ptr<Sink<Packet>> sink;

   public:
    Receiver(std::shared_ptr<Fd> sockfd, std::shared_ptr<Sink<Packet>> sink) : sockfd(sockfd), sink(sink) {}

    void run() {
        while (true) {
            PacketHeader header;
            read_exact(*sockfd, (char *)&header, sizeof header);

            std::vector<uint8_t> buffer((header.num_bits + 7) / 8);
            read_exact(*sockfd, (char *)buffer.data(), buffer.size());

            Packet packet{.shard = header.shard, .seq = header.seq, .num_bits = header.num_bits, .data = buffer};
            packet.check_well_formedness();
            sink->send(std::move(packet));
        }
    }
};

class Sender {
    std::shared_ptr<Fd> sockfd;
    std::shared_ptr<PacketQueue> stream;

   public:
    Sender(std::shared_ptr<Fd> sockfd, std::shared_ptr<PacketQueue> stream) : sockfd(sockfd), stream(stream) {}

    void run() {
        while (std::optional<Packet> optional_packet = stream->next()) {
            Packet packet = std::move(optional_packet.value());
            packet.check_well_formedness();

            PacketHeader header{
                .shard = packet.shard,
                .seq = packet.seq,
                .num_bits = packet.num_bits,
            };

            write_exact(*sockfd, (const char *)&header, sizeof header);
            write_exact(*sockfd, (const char *)packet.data.data(), packet.data.size());
        }
    }
};

std::optional<struct in_addr> parse_address(std::string address) {
    struct in_addr addr;
    if (::inet_pton(AF_INET, address.c_str(), &addr) > 0) {
        return addr;
    } else {
        return {};
    }
}

void network_listen(struct in_addr addr, uint16_t port, std::shared_ptr<PacketQueue> stream,
                    std::shared_ptr<Sink<Packet>> sink) {
    struct sockaddr_in sa {
        .sin_family = AF_INET, .sin_port = htons(port), .sin_addr = {}
    };
    sa.sin_addr.s_addr = INADDR_ANY;

    char buf[128];
    ::inet_ntop(AF_INET, (void *)&addr, buf, sizeof buf);
    auto str_addr = std::string(buf) + ":" + std::to_string(port);

    create_thread("network listener " + str_addr, [=] {
        int fd = ::socket(AF_INET, SOCK_STREAM, 0);

        if (fd < 0) {
            ERROR("socket: %s", strerror(errno));
            throw "failed to create socket";
        }

        // Fd sockfd(fd);
        int opt = 1;
        if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
            ERROR("setsockopt: %s", strerror(errno));
            throw "failed to reuse socket address";
        }

        if (::bind(fd, reinterpret_cast<const struct sockaddr *>(&sa), sizeof(sa)) < 0) {
            ERROR("bind: %s", strerror(errno));
            throw "failed to bind address to socket";
        }

        if (::listen(fd, BACKLOG_SIZE) < 0) {
            ERROR("listen: %s", strerror(errno));
            throw "failed to listen";
        }
        INFO("listening on %d", port);

        struct sockaddr_in client_address;
        socklen_t address_len = sizeof(client_address);
        while (true) {
            int new_socket = ::accept(fd, (struct sockaddr *)&client_address, &address_len);
            if (new_socket < 0) {
                ERROR("accept: %s", strerror(errno));
                throw "failed to accept connection";
            }
            char buf[INET_ADDRSTRLEN];
            ::inet_ntop(AF_INET, &(client_address.sin_addr), buf, sizeof(buf));
            std::shared_ptr<Fd> shared_new_socket = std::make_shared<Fd>(new_socket);

            std::uint64_t key;
            read_exact(*shared_new_socket, (char *)&key, sizeof NETWORK_KEY);

            if (key != NETWORK_KEY) {
                WARN("connection from %s:%d key verification failed", buf, ntohs(client_address.sin_port));
                continue;
            }

            INFO("accepted connection from %s:%d", buf, ntohs(client_address.sin_port));

            // spawn receiver thread
            create_thread(std::string("network receiver"), [=] {
                Receiver rx(shared_new_socket, sink);
                rx.run();
            }).detach();

            // spawn sender thread
            create_thread(std::string("network sender"), [=] {
                Sender tx(shared_new_socket, stream);
                tx.run();
            }).detach();
        }
    }).detach();
}

void network_connect(struct in_addr addr, uint16_t port, struct in_addr self_addr, uint16_t self_port,
                     std::shared_ptr<PacketQueue> stream, std::shared_ptr<Sink<Packet>> sink) {
    struct sockaddr_in sa {
        .sin_family = AF_INET, .sin_port = htons(port), .sin_addr = addr
    };
    struct sockaddr_in self_sa {
        .sin_family = AF_INET, .sin_port = htons(self_port), .sin_addr = self_addr
    };
    char buf[128];
    ::inet_ntop(AF_INET, (void *)&addr, buf, sizeof buf);
    auto str_addr = std::string(buf) + ":" + std::to_string(port);
    create_thread("network connector " + str_addr, [=] {
        int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            ERROR("socket: %s", strerror(errno));
            throw "failed to create socket";
        }

        int opt = 1;
        if (::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
            ERROR("setsockopt: %s", strerror(errno));
            throw "failed to reuse socket address";
        }

        if (::bind(sockfd, reinterpret_cast<const struct sockaddr *>(&self_sa), sizeof(self_sa)) == 0) {
            INFO("bind success to port %d", self_port);
        } else {
            ERROR("bind failure: %s", strerror(errno));
            throw "failed to bind to the address";
        }

        std::shared_ptr<Fd> fd = std::make_shared<Fd>(sockfd);

        while (::connect(sockfd, reinterpret_cast<const struct sockaddr *>(&sa), sizeof sa) < 0) {
            using namespace std::chrono_literals;
            WARN("connect: %s", strerror(errno));
            INFO("attempt to retry connection");
            std::this_thread::sleep_for(1000ms);
        }

        write_exact(*fd, (const char *)&NETWORK_KEY, sizeof NETWORK_KEY);

        INFO("connected to %s", str_addr.c_str());
        // spawn receiver thread
        create_thread(std::string("network receiver"), [=] {
            Receiver rx(fd, sink);
            rx.run();
        }).detach();

        // spawn sender thread
        create_thread(std::string("network sender"), [=] {
            Sender tx(fd, stream);
            tx.run();
        }).detach();
    }).detach();
}
