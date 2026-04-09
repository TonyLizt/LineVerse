#include "AsioBattleTransport.h"

bool AsioBattleTransport::host(unsigned short port) {
    (void)port;
    return false;
}

bool AsioBattleTransport::connectTo(const std::string& ip, unsigned short port) {
    (void)ip;
    (void)port;
    return false;
}

bool AsioBattleTransport::send(const std::string& message) {
    (void)message;
    return false;
}

bool AsioBattleTransport::poll(std::string& message) {
    (void)message;
    return false;
}

void AsioBattleTransport::stop() {
}
