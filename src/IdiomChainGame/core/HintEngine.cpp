#include "HintEngine.h"

#include "PathSolver.h"

HintEngine::HintEngine(const IdiomGraph& graph, const PathSolver& solver)
    : graph_(graph), solver_(solver) {
    (void)graph_;
}

std::optional<int> HintEngine::suggestNextStep(int currentId, int targetId) const {
    const PathResult result = solver_.solveShortestPath(currentId, targetId);
    if (!result.reachable || result.path.size() < 2U) {
        return std::nullopt;
    }
    return result.path[1];
}

std::vector<int> HintEngine::suggestPreferredPath(int startId, int targetId) const {
    const PathResult result = solver_.solveShortestPath(startId, targetId);
    return result.path;
}

std::vector<std::vector<int>> HintEngine::suggestMultiplePreferredPaths(int startId, int targetId, int limit) const {
    std::vector<std::vector<int>> paths;
    for (const PathResult& result : solver_.solveMultipleShortestPaths(startId, targetId, limit)) {
        paths.push_back(result.path);
    }
    return paths;
}
