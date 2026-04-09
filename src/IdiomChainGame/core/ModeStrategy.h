#ifndef IDIOM_CHAIN_GAME_MODE_STRATEGY_H
#define IDIOM_CHAIN_GAME_MODE_STRATEGY_H

#include "../app/GameSession.h"

#include <string>

class IdiomGraph;
class PathSolver;

/**
 * @brief Interface for per-mode question preparation and score policy.
 */
class IModeStrategy {
public:
    virtual ~IModeStrategy() = default;

    virtual std::string name() const = 0;
    virtual void prepareQuestion(GameSession& session, const IdiomGraph& graph, const PathSolver& solver) = 0;
};

/**
 * @brief Easy mode strategy.
 */
class EasyModeStrategy : public IModeStrategy {
public:
    std::string name() const override;
    void prepareQuestion(GameSession& session, const IdiomGraph& graph, const PathSolver& solver) override;
};

/**
 * @brief Medium mode strategy.
 */
class MediumModeStrategy : public IModeStrategy {
public:
    std::string name() const override;
    void prepareQuestion(GameSession& session, const IdiomGraph& graph, const PathSolver& solver) override;
};

/**
 * @brief Hard mode strategy.
 */
class HardModeStrategy : public IModeStrategy {
public:
    std::string name() const override;
    void prepareQuestion(GameSession& session, const IdiomGraph& graph, const PathSolver& solver) override;
};

#endif
