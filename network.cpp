#include "network.h"

#include <sys/socket.h>
#include <unistd.h>

#include <cassert>
#include <thread>

using std::make_shared;
using std::thread;

const size_t CHUNK_SIZE = 10 * 4096;

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
            auto buffer = vector<uint8_t>(CHUNK_SIZE);
            ssize_t bytes_read = ::recv(*sockfd, buffer.data(), buffer.size(), 0);
            if (bytes_read < 0) {
                assert(!"failed to read");
            }
            buffer.resize(bytes_read);
            // sink->send(move(buffer));
            assert(!"todo: serdes");
        }
    }
};

class Sender {
    shared_ptr<Fd> sockfd;
    shared_ptr<PacketStream> stream;

   public:
    Sender(shared_ptr<Fd> sockfd, shared_ptr<PacketStream> stream) : sockfd(sockfd), stream(stream) {}

    void run() {
        optional<Packet> packet;
        while (packet = stream->next()) {
            assert(!"todo: serdes");
            // auto data = move(packet.value());
            // size_t nbyte = data.size();
            // size_t bytes_written = 0;
            // while (bytes_written < nbyte) {
            //     ssize_t ret = ::write(*sockfd, (const void*)(data.data() + bytes_written), nbyte - bytes_written);
            //     if (ret < 0) {
            //         assert(!"failed to write");
            //     }
            //     bytes_written += ret;
            // }
        }
    }
};

class Listener {
    int sockfd;
    struct sockaddr address;
    shared_ptr<PacketStream> stream;
    shared_ptr<PacketSink> sink;

   public:
    Listener(struct sockaddr address, shared_ptr<PacketStream> stream, shared_ptr<PacketSink> sink)
        : address(address), stream(stream), sink(sink) {
        sockfd = ::socket(PF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            assert(!"failed to create socket");
        }
        if (::bind(sockfd, &address, sizeof(struct sockaddr)) < 0) {
            assert(!"failed to bind to specific port");
        }
    }

    ~Listener() { ::close(sockfd); }

    void listen() {
        if (::listen(sockfd, 16) < 0) {
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

            // spawn server thread
            thread([=] {
                Receiver rx(fd, sink);
                rx.run();
            });

            // spawn client thread
            thread([=] {
                Sender tx(fd, stream);
                tx.run();
            });
        }
    }
};

void network_listen(string address, shared_ptr<PacketStream> stream, shared_ptr<PacketSink> sink);

void network_connect(string address, shared_ptr<PacketStream> stream, shared_ptr<PacketSink> sink);
