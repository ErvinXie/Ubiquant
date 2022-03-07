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

class Fd {
    int fd;

   public:
    explicit Fd(int fd) : fd(fd) {}
    ~Fd() { ::close(fd); }

    operator int() { return fd; }
};

class Receiver {
    std::shared_ptr<Fd> sockfd;
    std::shared_ptr<Sink<Packet>> sink;

   public:
    Receiver(std::shared_ptr<Fd> sockfd, std::shared_ptr<Sink<Packet>> sink) : sockfd(sockfd), sink(sink) {}

    void run() {
        while (true) {
            uint32_t shard;
            uint32_t seq;
            uint32_t num_bits;

            const struct iovec iov[] = {
                iovec{.iov_base = &shard, .iov_len = sizeof shard},
                iovec{.iov_base = &seq, .iov_len = sizeof seq},
                iovec{.iov_base = &num_bits, .iov_len = sizeof num_bits},
            };
            {
                ssize_t ret = ::readv(*sockfd, iov, std::extent_v<decltype(iov)>);
                if (ret < (ssize_t)(sizeof shard + sizeof seq + sizeof num_bits)) {
                    WARN("readv: %s", strerror(errno));
                    return;
                }
            }

            size_t bytes = (num_bits + 7) / 8;
            std::vector<uint8_t> buffer(bytes);
            {
                ssize_t ret = ::read(*sockfd, buffer.data(), bytes);
                if (ret < (ssize_t)bytes) {
                    WARN("read: %s", strerror(errno));
                    return;
                }
            }

            Packet packet{.shard = shard, .seq = seq, .num_bits = num_bits, .data = buffer};
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

            const struct iovec iov[] = {
                iovec{.iov_base = &packet.shard, .iov_len = sizeof packet.shard},
                iovec{.iov_base = &packet.seq, .iov_len = sizeof packet.seq},
                iovec{.iov_base = &packet.num_bits, .iov_len = sizeof packet.num_bits},
                iovec{.iov_base = packet.data.data(), .iov_len = packet.data.size()},
            };

            ssize_t ret = ::writev(*sockfd, iov, std::extent_v<decltype(iov)>);
            if (ret < 0) {
                stream->send(std::move(packet));
                WARN("writev: %s", strerror(errno));
                WARN("packet requeued");
                return;
            }
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
        .sin_family = AF_INET, .sin_port = port, .sin_addr = addr
    };
    spawn_thread([=] {
        int fd = ::socket(PF_INET, SOCK_STREAM, 0);

        if (fd < 0) {
            ERROR("socket: %s", strerror(errno));
            throw "failed to create socket";
        }

        Fd sockfd(fd);

        if (::bind(sockfd, reinterpret_cast<const struct sockaddr*>(&sa), sizeof sa) < 0) {
            ERROR("bind: %s", strerror(errno));
            throw "failed to bind address to socket";
        }

        if (::listen(sockfd, BACKLOG_SIZE) < 0) {
            ERROR("listen: %s", strerror(errno));
            throw "failed to listen";
        }

        struct sockaddr client_address;
        socklen_t address_len;
        while (true) {
            int streamfd = ::accept(sockfd, &client_address, &address_len);
            if (streamfd < 0) {
                ERROR("accept: %s", strerror(errno));
                throw "failed to accept connection";
            }

            std::shared_ptr<Fd> fd = std::make_shared<Fd>(streamfd);

            // spawn receiver thread
            spawn_thread([=] {
                Receiver rx(fd, sink);
                rx.run();
            });

            // spawn sender thread
            spawn_thread([=] {
                Sender tx(fd, stream);
                tx.run();
            });
        }
    });
}

void network_connect(struct in_addr addr, uint16_t port, std::shared_ptr<PacketQueue> stream,
                     std::shared_ptr<Sink<Packet>> sink) {
    struct sockaddr_in sa {
        .sin_family = AF_INET, .sin_port = port, .sin_addr = addr
    };
    spawn_thread([=] {
        int sockfd = ::socket(PF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            ERROR("socket: %s", strerror(errno));
            throw "failed to create socket";
        }

        std::shared_ptr<Fd> fd = std::make_shared<Fd>(sockfd);

        while (::connect(sockfd, reinterpret_cast<const struct sockaddr*>(&sa), sizeof sa) < 0) {
            using namespace std::chrono_literals;
            WARN("connect: %s", strerror(errno));
            INFO("attempt to retry connection");
            std::this_thread::sleep_for(1000ms);
        }

        // spawn receiver thread
        spawn_thread([=] {
            Receiver rx(fd, sink);
            rx.run();
        });

        // spawn sender thread
        spawn_thread([=] {
            Sender tx(fd, stream);
            tx.run();
        });
    });
}
