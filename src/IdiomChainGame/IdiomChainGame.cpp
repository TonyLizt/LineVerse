#include "IdiomChainGame.h"

#include "app/IdiomChainController.h"
#include "core/ChainRule.h"
#include "core/HintEngine.h"
#include "core/IdiomGraph.h"
#include "core/PathSolver.h"
#include "data/CsvIdiomRepository.h"
#include "data/RecordRepository.h"
#include "net/AsioBattleTransport.h"
#include "ui/IdiomChainScene.h"

#include <iostream>
#include <memory>

int IdiomChainGame::start() {
    return start(nullptr, nullptr);
}

int IdiomChainGame::start(SDL_Window* externalWindow, SDL_Renderer* externalRenderer) {
    if (loadAssets() != 0) {
        return -1;
    }

    if (loadData() != 0) {
        return -1;
    }

    /*
     * initGame 只负责初始化控制器和场景对象，
     * 不需要使用 externalWindow / externalRenderer。
     *
     * externalWindow / externalRenderer 真正需要传入的是 gameLoop，
     * 因为窗口和渲染器是在 scene_->run(...) 中使用的。
     */
    if (initGame() != 0) {
        return -1;
    }

    const int loopResult = gameLoop(externalWindow, externalRenderer);

    saveResult();
    cleanup();

    return loopResult;
}

int IdiomChainGame::loadAssets() {
    return 0;
}

int IdiomChainGame::loadData() {
    repository_ = std::make_unique<CsvIdiomRepository>();

    if (!repository_->load()) {
        std::cerr << "[Error] Failed to load idiom data.\n";
        return -1;
    }

    chainRule_ = std::make_unique<PinyinToneChainRule>();

    graph_ = std::make_unique<IdiomGraph>();
    if (!graph_->build(*repository_, *chainRule_)) {
        std::cerr << "[Error] Failed to build idiom graph.\n";
        return -1;
    }

    solver_ = std::make_unique<PathSolver>(*graph_);
    hintEngine_ = std::make_unique<HintEngine>(*graph_, *solver_);
    recordRepository_ = std::make_unique<RecordRepository>();
    battleTransport_ = std::make_unique<AsioBattleTransport>();

    return 0;
}

int IdiomChainGame::initGame() {
    controller_ = std::make_unique<IdiomChainController>(
        *repository_,
        *graph_,
        *solver_,
        *hintEngine_,
        *recordRepository_,
        battleTransport_.get()
    );

    scene_ = std::make_unique<IdiomChainScene>(*controller_);

    return 0;
}

int IdiomChainGame::gameLoop() {
    return gameLoop(nullptr, nullptr);
}

int IdiomChainGame::gameLoop(SDL_Window* externalWindow, SDL_Renderer* externalRenderer) {
    return scene_->run(externalWindow, externalRenderer);
}

int IdiomChainGame::saveResult() {
    if (controller_) {
        controller_->flushPendingRecord();
    }

    return 0;
}

void IdiomChainGame::cleanup() {
    scene_.reset();
    controller_.reset();
    battleTransport_.reset();
    recordRepository_.reset();
    hintEngine_.reset();
    solver_.reset();
    graph_.reset();
    chainRule_.reset();
    repository_.reset();
}