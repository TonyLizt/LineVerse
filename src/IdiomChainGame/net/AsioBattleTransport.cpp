#include "AsioBattleTransport.h"

#include <cstring>
#include <string>

#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {
constexpr int kBufferSize = 4096;
}

AsioBattleTransport::AsioBattleTransport() = default;

AsioBattleTransport::~AsioBattleTransport() {
    stop();
}

bool AsioBattleTransport::ensureSocketRuntime() {
#ifdef _WIN32
    if (wsaReady_) {
        return true;
    }
    WSADATA wsaData{};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }
    wsaReady_ = true;
#endif
    return true;
}

void AsioBattleTransport::closeSocket(SocketHandle& sock) {
    if (sock == kInvalidSocket) {
        return;
    }
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    sock = kInvalidSocket;
}

void AsioBattleTransport::shutdownSocket(SocketHandle sock) {
    if (sock == kInvalidSocket) {
        return;
    }
#ifdef _WIN32
    shutdown(sock, SD_BOTH);
#else
    shutdown(sock, SHUT_RDWR);
#endif
}

bool AsioBattleTransport::host(unsigned short port) {
    stop();
    if (!ensureSocketRuntime()) {
        return false;
    }

    listenSocket_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket_ == kInvalidSocket) {
        return false;
    }

    int opt = 1;
#ifdef _WIN32
    setsockopt(listenSocket_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
#else
    setsockopt(listenSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (::bind(listenSocket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        closeSocket(listenSocket_);
        return false;
    }
    if (::listen(listenSocket_, 1) != 0) {
        closeSocket(listenSocket_);
        return false;
    }

    running_ = true;
    connected_ = false;
    acceptThread_ = std::thread(&AsioBattleTransport::acceptLoop, this);
    return true;
}

bool AsioBattleTransport::connectTo(const std::string& ip, unsigned short port) {
    stop();
    if (!ensureSocketRuntime()) {
        return false;
    }

    SocketHandle client = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client == kInvalidSocket) {
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
        closeSocket(client);
        return false;
    }

    if (::connect(client, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        closeSocket(client);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(socketMutex_);
        socket_ = client;
    }
    running_ = true;
    connected_ = true;
    return startReadLoop();
}

bool AsioBattleTransport::startReadLoop() {
    if (readThread_.joinable()) {
        readThread_.join();
    }
    readThread_ = std::thread(&AsioBattleTransport::readLoop, this);
    return true;
}

void AsioBattleTransport::acceptLoop() {
    sockaddr_in peer{};
#ifdef _WIN32
    int len = sizeof(peer);
#else
    socklen_t len = sizeof(peer);
#endif

    SocketHandle accepted = ::accept(listenSocket_, reinterpret_cast<sockaddr*>(&peer), &len);
    if (!running_) {
        if (accepted != kInvalidSocket) {
            closeSocket(accepted);
        }
        return;
    }

    if (accepted == kInvalidSocket) {
        running_ = false;
        return;
    }

    {
        std::lock_guard<std::mutex> lock(socketMutex_);
        socket_ = accepted;
    }
    connected_ = true;
    closeSocket(listenSocket_);
    startReadLoop();
}

void AsioBattleTransport::readLoop() {
    std::string pending;
    char buffer[kBufferSize];

    while (running_) {
        SocketHandle sock = kInvalidSocket;
        {
            std::lock_guard<std::mutex> lock(socketMutex_);
            sock = socket_;
        }
        if (sock == kInvalidSocket) {
            break;
        }

#ifdef _WIN32
        const int n = ::recv(sock, buffer, kBufferSize, 0);
#else
        const ssize_t n = ::recv(sock, buffer, kBufferSize, 0);
#endif
        if (n <= 0) {
            connected_ = false;
            running_ = false;
            break;
        }

        pending.append(buffer, buffer + n);
        std::size_t pos = std::string::npos;
        while ((pos = pending.find('\n')) != std::string::npos) {
            std::string line = pending.substr(0, pos);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (!line.empty()) {
                pushMessage(line);
            }
            pending.erase(0, pos + 1);
        }
    }
}

void AsioBattleTransport::pushMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    inbox_.push_back(message);
}

bool AsioBattleTransport::send(const std::string& message) {
    if (!connected_) {
        return false;
    }

    SocketHandle sock = kInvalidSocket;
    {
        std::lock_guard<std::mutex> lock(socketMutex_);
        sock = socket_;
    }
    if (sock == kInvalidSocket) {
        return false;
    }

    const std::string packet = message + "\n";
    const char* data = packet.data();
    std::size_t total = 0;

    std::lock_guard<std::mutex> sendLock(sendMutex_);
    while (total < packet.size()) {
#ifdef _WIN32
        const int sent = ::send(sock, data + total, static_cast<int>(packet.size() - total), 0);
#else
        const ssize_t sent = ::send(sock, data + total, packet.size() - total, 0);
#endif
        if (sent <= 0) {
            connected_ = false;
            return false;
        }
        total += static_cast<std::size_t>(sent);
    }
    return true;
}

bool AsioBattleTransport::poll(std::string& message) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    if (inbox_.empty()) {
        return false;
    }
    message = inbox_.front();
    inbox_.pop_front();
    return true;
}

void AsioBattleTransport::stop() {
    running_ = false;
    connected_ = false;

    {
        std::lock_guard<std::mutex> lock(socketMutex_);
        shutdownSocket(socket_);
        shutdownSocket(listenSocket_);
    }

    if (acceptThread_.joinable()) {
        acceptThread_.join();
    }
    if (readThread_.joinable()) {
        readThread_.join();
    }

    {
        std::lock_guard<std::mutex> lock(socketMutex_);
        closeSocket(socket_);
        closeSocket(listenSocket_);
    }

    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        inbox_.clear();
    }

#ifdef _WIN32
    if (wsaReady_) {
        WSACleanup();
        wsaReady_ = false;
    }
#endif
}
