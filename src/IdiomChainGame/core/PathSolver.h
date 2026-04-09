#ifndef IDIOM_CHAIN_GAME_PATH_SOLVER_H
#define IDIOM_CHAIN_GAME_PATH_SOLVER_H

#include <vector>

class IdiomGraph;

/**
 * @brief Result for a path search.
 */
struct PathResult {
    bool reachable { false };
    int stepCount { 0 };
    std::vector<int> path;
};

/**
 * @brief BFS-based shortest path solver for the idiom graph.
 */
class PathSolver {
public:
    explicit PathSolver(const IdiomGraph& graph);

    PathResult solveShortestPath(int startId, int targetId) const;
    std::vector<PathResult> solveMultipleShortestPaths(int startId, int targetId, int limit) const;
    std::vector<int> computeDistanceToTarget(int targetId) const;

private:
    const IdiomGraph& graph_;
};

#endif
