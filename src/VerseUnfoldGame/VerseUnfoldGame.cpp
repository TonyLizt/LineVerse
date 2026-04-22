#include "VerseUnfoldGame.h"

#include <fstream>
#include <iostream>
#include <vector>

#include "ui/VerseUnfoldSDLApp.h"

namespace {
std::string resolveExistingPath(const std::vector<std::string>& candidates) {
    for (const auto& path : candidates) {
        std::ifstream fin(path);
        if (fin.good()) {
            return path;
        }
    }
    return candidates.empty() ? std::string{} : candidates.front();
}
}

VerseUnfoldGame::VerseUnfoldGame()
    : controller(std::make_shared<PoetryGameController>()) {}

VerseUnfoldGame::~VerseUnfoldGame() = default;

int VerseUnfoldGame::start() {
    return start(nullptr, nullptr);
}

int VerseUnfoldGame::start(SDL_Window* externalWindow, SDL_Renderer* externalRenderer) {
    const std::string resolvedDbPath = resolveExistingPath({
        dbPath,
        "../" + dbPath,
        "../../" + dbPath,
        "../../../" + dbPath
    });

    if (!controller->initialize(resolvedDbPath)) {
        std::cerr << "[VerseUnfoldGame] Failed to initialize PoetryGameController with database: "
                  << resolvedDbPath << std::endl;
        return -1;
    }

    VerseUnfold::VerseUnfoldSDLApp app(controller);
    if (!app.init(externalWindow, externalRenderer)) {
        std::cerr << "[VerseUnfoldGame] Failed to initialize SDL App." << std::endl;
        return -1;
    }

    return app.run();
}
