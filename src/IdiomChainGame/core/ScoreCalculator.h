#ifndef IDIOM_CHAIN_GAME_SCORE_CALCULATOR_H
#define IDIOM_CHAIN_GAME_SCORE_CALCULATOR_H

class GameSession;

/**
 * @brief Scoring helper for all modes.
 */
class ScoreCalculator {
public:
    int calculate(const GameSession& session) const;
};

#endif
