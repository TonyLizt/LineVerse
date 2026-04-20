#include "ModeStrategy.h"

#include "IdiomGraph.h"
#include "PathSolver.h"

#include <algorithm>
#include <random>

namespace {
int pickReachablePair(const IdiomGraph& graph, const PathSolver& solver, int minSteps, int maxSteps, int& outTargetId) {
    std::mt19937 randomEngine(static_cast<unsigned int>(std::random_device{}()));
    std::uniform_int_distribution<int> distribution(0, static_cast<int>(graph.size()) - 1);

    for (int attempt = 0; attempt < 600; ++attempt) {
        const int startId = distribution(randomEngine);
        const int targetId = distribution(randomEngine);
        if (startId == targetId) {
            continue;
        }

        const PathResult result = solver.solveShortestPath(startId, targetId);
        if (result.reachable && result.stepCount >= minSteps && result.stepCount <= maxSteps) {
            outTargetId = targetId;
            return startId;
        }
    }

    outTargetId = -1;
    return -1;
}
} // namespace

std::string EasyModeStrategy::name() const {
    return "SingleEasy";
}

void EasyModeStrategy::prepareQuestion(GameSession& session, const IdiomGraph& graph, const PathSolver& solver) {
    session.timeLimitSeconds = 180;
    int targetId = -1;
    const int startId = pickReachablePair(graph, solver, 3, 5, targetId);
    session.startId = startId;
    session.targetId = targetId;
}

std::string MediumModeStrategy::name() const {
    return "SingleMedium";
}

void MediumModeStrategy::prepareQuestion(GameSession& session, const IdiomGraph& graph, const PathSolver& solver) {
    session.timeLimitSeconds = 240;
    int targetId = -1;
    const int startId = pickReachablePair(graph, solver, 4, 6, targetId);
    session.startId = startId;
    session.targetId = targetId;
}

std::string HardModeStrategy::name() const {
    return "SingleHard";
}

void HardModeStrategy::prepareQuestion(GameSession& session, const IdiomGraph& graph, const PathSolver& solver) {
    session.timeLimitSeconds = 300;
    int targetId = -1;
    const int startId = pickReachablePair(graph, solver, 3, 4, targetId);
    session.startId = startId;
    session.targetId = targetId;
    session.maxHints = 3;
}
