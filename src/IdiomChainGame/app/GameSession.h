#ifndef IDIOM_CHAIN_GAME_GAME_SESSION_H
#define IDIOM_CHAIN_GAME_GAME_SESSION_H

#include <stack>
#include <string>
#include <vector>

/**
 * @brief Supported game modes.
 */
enum class GameMode {
    SingleEasy,
    SingleMedium,
    SingleHard,
    BattleEasy,
    BattleMedium,
    BattleHard
};

/**
 * @brief Runtime state for a single game session.
 */
class GameSession {
public:
    std::string playerName;
    GameMode mode { GameMode::SingleEasy };

    int startId { -1 };
    int targetId { -1 };

    std::vector<int> playerPath;
    std::stack<int> rollbackStack;

    int hintCount { 0 };
    int maxHints { 0 };
    int rollbackCount { 0 };

    int stepCount { 0 };
    int bestStepCount { -1 };
    double elapsedSeconds { 0.0 };
    int timeLimitSeconds { 0 };

    bool finished { false };
    bool success { false };

    int score { 0 };

    std::vector<int> bestPath;
    std::vector<int> easyPool;
    std::vector<int> distanceToTarget;
    std::vector<int> mediumOptions;
};

#endif
