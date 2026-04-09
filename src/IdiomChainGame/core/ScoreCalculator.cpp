#include "ScoreCalculator.h"

#include "../app/GameSession.h"

#include <algorithm>

int ScoreCalculator::calculate(const GameSession& session) const {
    if (!session.finished || !session.success) {
        return 0;
    }

    int score = 100;

    const int extraSteps = std::max(0, session.stepCount - session.bestStepCount);
    score -= extraSteps * 10;
    score -= session.hintCount * 8;
    score -= session.rollbackCount * 3;
    score -= static_cast<int>(session.elapsedSeconds / 5);

    switch (session.mode) {
    case GameMode::SingleEasy:
        score += 10;
        break;
    case GameMode::SingleMedium:
        score += 20;
        break;
    case GameMode::SingleHard:
        score += 30;
        break;
    case GameMode::BattleEasy:
    case GameMode::BattleMedium:
    case GameMode::BattleHard:
        score += 40;
        break;
    }

    return std::max(score, 1);
}
