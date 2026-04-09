#ifndef IDIOM_CHAIN_GAME_HINT_ENGINE_H
#define IDIOM_CHAIN_GAME_HINT_ENGINE_H

#include <optional>
#include <vector>

class IdiomGraph;
class PathSolver;

/**
 * @brief Hint service built on top of shortest-path queries.
 */
class HintEngine {
public:
    HintEngine(const IdiomGraph& graph, const PathSolver& solver);

    std::optional<int> suggestNextStep(int currentId, int targetId) const;
    std::vector<int> suggestPreferredPath(int startId, int targetId) const;
    std::vector<std::vector<int>> suggestMultiplePreferredPaths(int startId, int targetId, int limit) const;

private:
    const IdiomGraph& graph_;
    const PathSolver& solver_;
};

#endif
