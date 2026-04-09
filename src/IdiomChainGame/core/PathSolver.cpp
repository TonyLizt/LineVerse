#include "PathSolver.h"

#include "IdiomGraph.h"

#include <algorithm>
#include <functional>
#include <queue>

PathSolver::PathSolver(const IdiomGraph& graph)
    : graph_(graph) {
}

PathResult PathSolver::solveShortestPath(int startId, int targetId) const {
    PathResult result;
    if (startId < 0 || targetId < 0) {
        return result;
    }

    const std::size_t n = graph_.size();
    std::vector<int> dist(n, -1);
    std::vector<int> prev(n, -1);
    std::queue<int> pending;

    dist[static_cast<std::size_t>(startId)] = 0;
    pending.push(startId);

    while (!pending.empty()) {
        const int current = pending.front();
        pending.pop();

        if (current == targetId) {
            break;
        }

        for (int nextId : graph_.getNextIds(current)) {
            if (dist[static_cast<std::size_t>(nextId)] != -1) {
                continue;
            }
            dist[static_cast<std::size_t>(nextId)] = dist[static_cast<std::size_t>(current)] + 1;
            prev[static_cast<std::size_t>(nextId)] = current;
            pending.push(nextId);
        }
    }

    if (dist[static_cast<std::size_t>(targetId)] == -1) {
        return result;
    }

    result.reachable = true;
    result.stepCount = dist[static_cast<std::size_t>(targetId)];

    for (int at = targetId; at != -1; at = prev[static_cast<std::size_t>(at)]) {
        result.path.push_back(at);
    }
    std::reverse(result.path.begin(), result.path.end());
    return result;
}

std::vector<PathResult> PathSolver::solveMultipleShortestPaths(int startId, int targetId, int limit) const {
    std::vector<PathResult> results;
    if (startId < 0 || targetId < 0 || limit <= 0) {
        return results;
    }

    const std::size_t n = graph_.size();
    std::vector<int> dist(n, -1);
    std::vector<std::vector<int>> parents(n);
    std::queue<int> pending;

    dist[static_cast<std::size_t>(startId)] = 0;
    pending.push(startId);

    while (!pending.empty()) {
        const int current = pending.front();
        pending.pop();

        for (int nextId : graph_.getNextIds(current)) {
            const int candidateDist = dist[static_cast<std::size_t>(current)] + 1;
            if (dist[static_cast<std::size_t>(nextId)] == -1) {
                dist[static_cast<std::size_t>(nextId)] = candidateDist;
                parents[static_cast<std::size_t>(nextId)].push_back(current);
                pending.push(nextId);
            } else if (dist[static_cast<std::size_t>(nextId)] == candidateDist) {
                parents[static_cast<std::size_t>(nextId)].push_back(current);
            }
        }
    }

    if (dist[static_cast<std::size_t>(targetId)] == -1) {
        return results;
    }

    std::vector<int> currentPath;

    std::function<void(int)> backtrack = [&](int nodeId) {
        if (static_cast<int>(results.size()) >= limit) {
            return;
        }

        currentPath.push_back(nodeId);

        if (nodeId == startId) {
            PathResult result;
            result.reachable = true;
            result.stepCount = dist[static_cast<std::size_t>(targetId)];
            result.path.assign(currentPath.rbegin(), currentPath.rend());
            results.push_back(result);
            currentPath.pop_back();
            return;
        }

        for (int parentId : parents[static_cast<std::size_t>(nodeId)]) {
            backtrack(parentId);
            if (static_cast<int>(results.size()) >= limit) {
                break;
            }
        }

        currentPath.pop_back();
    };

    backtrack(targetId);
    return results;
}

std::vector<int> PathSolver::computeDistanceToTarget(int targetId) const {
    const std::size_t n = graph_.size();
    std::vector<int> dist(n, -1);
    if (targetId < 0 || static_cast<std::size_t>(targetId) >= n) {
        return dist;
    }

    std::queue<int> pending;
    dist[static_cast<std::size_t>(targetId)] = 0;
    pending.push(targetId);

    while (!pending.empty()) {
        const int current = pending.front();
        pending.pop();

        for (int prevId : graph_.getPrevIds(current)) {
            if (dist[static_cast<std::size_t>(prevId)] != -1) {
                continue;
            }
            dist[static_cast<std::size_t>(prevId)] = dist[static_cast<std::size_t>(current)] + 1;
            pending.push(prevId);
        }
    }

    return dist;
}
