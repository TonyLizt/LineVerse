#ifndef IDIOM_CHAIN_GAME_H
#define IDIOM_CHAIN_GAME_H

#include "app/IdiomChainController.h"
#include "core/ChainRule.h"
#include "core/HintEngine.h"
#include "core/IdiomGraph.h"
#include "core/PathSolver.h"
#include "data/IIdiomRepository.h"
#include "data/RecordRepository.h"
#include "ui/IdiomChainScene.h"

#include <memory>

/**
 * @brief Facade class for the IdiomChainGame module.
 *
 * The module coordinates data loading, graph building, controller creation,
 * scene execution, result saving and cleanup.
 */
class IdiomChainGame {
public:
    /**
     * @brief Run the module lifecycle.
     * @return 0 to exit the program, 1 to return to menu, -1 on failure.
     */
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
    std::unique_ptr<IdiomChainController> controller_;
    std::unique_ptr<IdiomChainScene> scene_;
};

#endif
