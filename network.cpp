#include "network.h"

#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#include <cassert>
#include <thread>
#include <type_traits>

using std::extent_v;
using std::make_shared;
using std::thread;

const int BACKLOG_SIZE = 16;

class Fd {
    int fd;

   public:
    explicit Fd(int fd) : fd(fd) {}
    ~Fd() { ::close(fd); }

    operator int() { return fd; }
};

class Receiver {
    shared_ptr<Fd> sockfd;
    shared_ptr<PacketSink> sink;

   public:
    Receiver(shared_ptr<Fd> sockfd, shared_ptr<PacketSink> sink) : sockfd(sockfd), sink(sink) {}

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

            ssize_t ret = ::readv(*sockfd, iov, extent_v<decltype(iov)>);
            if (ret < 0) {
                assert(!"failed to read");
            }

            size_t bytes = (num_bits + 7) / 8;
            vector<uint8_t> buffer(bytes);
            ssize_t bytes_read = ::read(*sockfd, buffer.data(), buffer.size());
            if (ret < 0) {
                assert(!"failed to read");
            }
            Packet packet{.shard = shard, .seq = seq, .num_bits = num_bits, .data = buffer};
            packet.check_well_formedness();
            sink->send(move(packet));
        }
    }
};

class Sender {
    shared_ptr<Fd> sockfd;
    shared_ptr<PacketStream> stream;
    shared_ptr<PacketSink> requeue;

   public:
    Sender(shared_ptr<Fd> sockfd, shared_ptr<PacketStream> stream, shared_ptr<PacketSink> requeue)
        : sockfd(sockfd), stream(stream), requeue(requeue) {}

    void run() {
        while (true) {
            Packet packet = stream->next();
            packet.check_well_formedness();

            const struct iovec iov[] = {
                iovec{.iov_base = &packet.shard, .iov_len = sizeof packet.shard},
                iovec{.iov_base = &packet.seq, .iov_len = sizeof packet.seq},
                iovec{.iov_base = &packet.num_bits, .iov_len = sizeof packet.num_bits},
                iovec{.iov_base = packet.data.data(), .iov_len = packet.data.size()},
            };

            ssize_t ret = ::writev(*sockfd, iov, extent_v<decltype(iov)>);
            if (ret < 0) {
                requeue->send(move(packet));
                assert(!"todo: handle transmission error");
            }
        }
    }
};

static struct sockaddr parse_address(string address) { assert(!"unimplemented"); }

void network_listen(string address, shared_ptr<PacketStream> stream, shared_ptr<PacketSink> sink,
                    shared_ptr<PacketSink> requeue) {
    thread([=] {
        int sockfd = ::socket(PF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            assert(!"failed to create socket");
        }

        struct sockaddr addr = parse_address(address);

        if (::bind(sockfd, &addr, sizeof addr) < 0) {
            assert(!"failed to bind to specific port");
        }

        if (::listen(sockfd, BACKLOG_SIZE) < 0) {
            assert(!"failed to listen");
        }

        struct sockaddr client_address;
        socklen_t address_len;
        while (true) {
            int streamfd = ::accept(sockfd, &client_address, &address_len);
            if (streamfd < 0) {
                assert(!"failed to accept");
            }

            shared_ptr<Fd> fd = make_shared<Fd>(streamfd);

            // spawn receiver thread
            thread([=] {
                Receiver rx(fd, sink);
                rx.run();
            });

            // spawn sender thread
            thread([=] {
                Sender tx(fd, stream, requeue);
                tx.run();
            });
        }
    });
}

void network_connect(string address, shared_ptr<PacketStream> stream, shared_ptr<PacketSink> sink,
                     shared_ptr<PacketSink> requeue) {
    thread([=] {
        int sockfd = ::socket(PF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            assert(!"failed to create socket");
        }

        struct sockaddr addr = parse_address(address);

        while (::connect(sockfd, &addr, sizeof addr) < 0) {
            assert(!"retry connection");
        }

        shared_ptr<Fd> fd = make_shared<Fd>(sockfd);

        // spawn receiver thread
        thread([=] {
            Receiver rx(fd, sink);
            rx.run();
        });

        // spawn sender thread
        thread([=] {
            Sender tx(fd, stream, requeue);
            tx.run();
        });
    });
}
