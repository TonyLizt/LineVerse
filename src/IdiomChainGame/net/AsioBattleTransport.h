#ifndef IDIOM_CHAIN_GAME_ASIO_BATTLE_TRANSPORT_H
#define IDIOM_CHAIN_GAME_ASIO_BATTLE_TRANSPORT_H

#include "IBattleTransport.h"

#include <atomic>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

/**
 * @brief TCP LAN transport implementation.
 *
 * Despite the legacy class name, this implementation does not depend on Asio.
 * It uses a background accept/read thread plus a newline-delimited text protocol.
 */
class AsioBattleTransport : public IBattleTransport {
public:
    AsioBattleTransport();
    ~AsioBattleTransport() override;

    bool host(unsigned short port) override;
    bool connectTo(const std::string& ip, unsigned short port) override;
    bool send(const std::string& message) override;
    bool poll(std::string& message) override;
    void stop() override;

private:
#ifdef _WIN32
    using SocketHandle = SOCKET;
    static constexpr SocketHandle kInvalidSocket = INVALID_SOCKET;
#else
    using SocketHandle = int;
    static constexpr SocketHandle kInvalidSocket = -1;
#endif

    bool ensureSocketRuntime();
    void closeSocket(SocketHandle& sock);
    void shutdownSocket(SocketHandle sock);
    bool startReadLoop();
    void acceptLoop();
    void readLoop();
    void pushMessage(const std::string& message);

    SocketHandle listenSocket_ { kInvalidSocket };
    SocketHandle socket_ { kInvalidSocket };

    std::thread acceptThread_;
    std::thread readThread_;

    std::mutex socketMutex_;
    std::mutex queueMutex_;
    std::mutex sendMutex_;
    std::deque<std::string> inbox_;

    std::atomic<bool> running_ { false };
    std::atomic<bool> connected_ { false };
#ifdef _WIN32
    bool wsaReady_ { false };
#endif
};

#endif
