#ifndef IDIOM_CHAIN_GAME_ASIO_BATTLE_TRANSPORT_H
#define IDIOM_CHAIN_GAME_ASIO_BATTLE_TRANSPORT_H

#include "IBattleTransport.h"

/**
 * @brief Placeholder transport.
 *
 * This class is intentionally left as a stub in the packaged baseline code.
 * You can later replace it with a real Asio TCP implementation for LAN battle.
 */
class AsioBattleTransport : public IBattleTransport {
public:
    bool host(unsigned short port) override;
    bool connectTo(const std::string& ip, unsigned short port) override;
    bool send(const std::string& message) override;
    bool poll(std::string& message) override;
    void stop() override;
};

#endif
