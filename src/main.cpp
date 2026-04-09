#include "main.h"
#include "IdiomChainGame/IdiomChainGame.h"

#include <exception>
#include <iostream>

int LineVerseApp::run() {
    IdiomChainGame game;
    return game.start();
}

int main() {
    try {
        LineVerseApp app;
        return app.run();
    } catch (const std::exception& ex) {
        std::cerr << "[Fatal] " << ex.what() << '\n';
        return -1;
    } catch (...) {
        std::cerr << "[Fatal] Unknown exception.\n";
        return -1;
    }
}
