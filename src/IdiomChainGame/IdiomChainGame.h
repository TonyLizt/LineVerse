#ifndef IDIOM_CHAIN_GAME_H
#define IDIOM_CHAIN_GAME_H

#include "app/IdiomChainController.h"
#include "core/ChainRule.h"
#include "core/HintEngine.h"
#include "core/IdiomGraph.h"
#include "core/PathSolver.h"
#include "data/IIdiomRepository.h"
#include "data/RecordRepository.h"
#include "net/IBattleTransport.h"
#include "ui/IdiomChainScene.h"

#include <memory>

class IdiomChainGame {
public:
    int start();

private:
    int loadAssets();
    int loadData();
    int initGame();
    int gameLoop();
    int saveResult();
    void cleanup();

    std::unique_ptr<IIdiomRepository> repository_;
    std::unique_ptr<IChainRule> chainRule_;
    std::unique_ptr<IdiomGraph> graph_;
    std::unique_ptr<PathSolver> solver_;
    std::unique_ptr<HintEngine> hintEngine_;
    std::unique_ptr<RecordRepository> recordRepository_;
    std::unique_ptr<IBattleTransport> battleTransport_;
    std::unique_ptr<IdiomChainController> controller_;
    std::unique_ptr<IdiomChainScene> scene_;
};

#endif
