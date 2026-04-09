#ifndef IDIOM_CHAIN_GAME_I_BATTLE_TRANSPORT_H
#define IDIOM_CHAIN_GAME_I_BATTLE_TRANSPORT_H

#include <string>

/**
 * @brief Network transport abstraction for LAN battle mode.
 *
 * The current package only provides a stub implementation, but the interface
 * is already in place so that the module can later switch to a real Asio-based
 * LAN transport without changing controller or UI code.
 */
class IBattleTransport {
public:
    virtual ~IBattleTransport() = default;

    virtual bool host(unsigned short port) = 0;
    virtual bool connectTo(const std::string& ip, unsigned short port) = 0;
    virtual bool send(const std::string& message) = 0;
    virtual bool poll(std::string& message) = 0;
    virtual void stop() = 0;
};

#endif
