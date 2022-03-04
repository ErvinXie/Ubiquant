#include "network.h"

#include <sys/socket.h>
#include <unistd.h>

#include <cassert>
#include <memory>
#include <thread>

using std::make_shared;
using std::shared_ptr;
using std::thread;

const size_t CHUNK_SIZE = 10 * 4096;

class Fd {
    int fd;

   public:
    Fd(int fd) : fd(fd) {}
    ~Fd() { ::close(fd); }

    operator int() { return fd; }
};

class Server {
    shared_ptr<Fd> sockfd;
    shared_ptr<DataQueue> queue;

   public:
    Server(shared_ptr<Fd> sockfd, shared_ptr<DataQueue> queue) : sockfd(sockfd), queue(queue) {}

    void run() {
        while (true) {
            auto buffer = vector<uint8_t>(CHUNK_SIZE);
            ssize_t bytes_read = ::read(*sockfd, buffer.data(), buffer.size());
            if (bytes_read < 0) {
                assert(!"failed to read");
            }
            buffer.resize(bytes_read);
            queue->insert(0, buffer);
        }
    }
};

class Client {
    shared_ptr<Fd> sockfd;
    shared_ptr<DataQueue> queue;

   public:
    Client(shared_ptr<Fd> sockfd, shared_ptr<DataQueue> queue) : sockfd(sockfd), queue(queue) {}

    void run() {
        while (true) {
            auto data = queue->pop();
            size_t nbyte = data.size();
            size_t bytes_written = 0;
            while (bytes_written < nbyte) {
                ssize_t ret = ::write(*sockfd, (const void*)(data.data() + bytes_written), nbyte - bytes_written);
                if (ret < 0) {
                    assert(!"failed to write");
                }
                bytes_written += ret;
            }
        }
    }
};

class Listener {
    int sockfd = 0;
    shared_ptr<DataQueue> queue;

   public:
    Listener() {
        sockfd = ::socket(PF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            assert(!"failed to create socket");
        }
    }

    ~Listener() { ::close(sockfd); }

    void listen(struct sockaddr address) {
        if (::bind(sockfd, &address, sizeof(struct sockaddr)) < 0) {
            assert(!"failed to bind to specific port");
        }
        if (::listen(sockfd, 16) < 0) {
            assert(!"failed to listen");
        }
        struct sockaddr client_address;
        socklen_t address_len;
        int streamfd = -1;
        while (true) {
            streamfd = ::accept(sockfd, &client_address, &address_len);
            if (streamfd < 0) {
                assert(!"failed to accept");
            }
            shared_ptr<Fd> fd = make_shared<Fd>(streamfd);

            // spawn server thread
            thread([=] {
                Server server(fd, queue);
                server.run();
            });

            // spawn client thread
            thread([=] {
                Client client(fd, queue);
                client.run();
            });
        }
    }
};
