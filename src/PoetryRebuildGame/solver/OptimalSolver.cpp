#include "OptimalSolver.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace lineverse::poetryrebuild {

OptimalSolver::OptimalSolver(const IndexStore& indexStore)
    : indexStore_(indexStore) {
}

SolveResult OptimalSolver::searchOptimalMax(const CandidateIdList& candidates, const CharFreq& pool) const {
    SolveResult result;
    const CandidateIdList validCandidates = sanitizeCandidates(candidates, pool);
    result.candidateCount = static_cast<int>(validCandidates.size());

    if (validCandidates.empty()) {
        lastResult_ = result;
        return result;
    }

    if (validCandidates.size() <= kExactSolveThreshold) {
        const ConflictGraph localConflictGraph = buildConflictGraph(validCandidates, pool);
        const CandidateIdList ordered = rankCandidates(validCandidates, pool, &localConflictGraph);

        std::vector<std::string> currentSet;
        SolveResult best;
        best.usedExact = true;
        best.strategy = "exact_dfs_branch_and_bound";
        best.candidateCount = static_cast<int>(validCandidates.size());

        dfsPick(0, ordered, pool, currentSet, best);

        lastResult_ = best;
        return best;
    }

    const CandidateIdList ordered = rankCandidates(validCandidates, pool, nullptr);
    SolveResult greedy = greedyFallback(ordered, pool);
    greedy = improveGreedySolution(greedy, ordered, pool);
    greedy.usedExact = false;
    greedy.candidateCount = static_cast<int>(validCandidates.size());
    greedy.strategy = "greedy_local_improvement";

    lastResult_ = greedy;
    return greedy;
}

SolveResult OptimalSolver::searchOptimalMax(
    const CharFreq& pool,
    const CandidateEngine& candidateEngine,
    const CandidateQueryOptions& options
) const {
    const CandidateIdList candidates = candidateEngine.getCandidates(pool, options);
    return searchOptimalMax(candidates, pool);
}

int OptimalSolver::upperBound(
    std::size_t idx,
    const CandidateIdList& ordered,
    const CharFreq& remainFreq,
    int pickedCount
) const {
    int bound = pickedCount;

    for (std::size_t i = idx; i < ordered.size(); ++i) {
        const PhraseInfo* phrase = indexStore_.getPhraseById(ordered[i]);
        if (phrase == nullptr) {
            continue;
        }
        CandidateEngine checker(indexStore_);
        if (checker.canCompose(phrase->charFreq, remainFreq)) {
            ++bound;
        }
    }

    return bound;
}

bool OptimalSolver::isConflict(const PhraseInfo& phraseA, const PhraseInfo& phraseB, const CharFreq& pool) const {
    CharFreq merged = addFreq(phraseA.charFreq, phraseB.charFreq);
    CandidateEngine checker(indexStore_);
    return !checker.canCompose(merged, pool);
}

OptimalSolver::ConflictGraph OptimalSolver::buildConflictGraph(
    const CandidateIdList& candidates,
    const CharFreq& pool
) const {
    ConflictGraph graph;
    for (const auto& id : candidates) {
        graph[id];
    }

    for (std::size_t i = 0; i < candidates.size(); ++i) {
        const PhraseInfo* a = indexStore_.getPhraseById(candidates[i]);
        if (a == nullptr) {
            continue;
        }

        for (std::size_t j = i + 1; j < candidates.size(); ++j) {
            const PhraseInfo* b = indexStore_.getPhraseById(candidates[j]);
            if (b == nullptr) {
                continue;
            }

            if (isConflict(*a, *b, pool)) {
                graph[candidates[i]].push_back(candidates[j]);
                graph[candidates[j]].push_back(candidates[i]);
            }
        }
    }

    for (auto& [_, ids] : graph) {
        std::sort(ids.begin(), ids.end());
        ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    }

    return graph;
}

std::vector<std::string> OptimalSolver::getBestSolutionSet() const {
    return lastResult_.bestSet;
}

void OptimalSolver::dfsPick(
    std::size_t idx,
    const CandidateIdList& ordered,
    const CharFreq& remainFreq,
    std::vector<std::string>& currentSet,
    SolveResult& bestResult
) const {
    ++bestResult.exploredNodes;

    if (idx >= ordered.size()) {
        const int currentCount = static_cast<int>(currentSet.size());
        if (currentCount > bestResult.optimalMax ||
            (currentCount == bestResult.optimalMax &&
             lexicographicallyBetter(currentSet, bestResult.bestSet))) {
            bestResult.optimalMax = currentCount;
            bestResult.bestSet = currentSet;
        }
        return;
    }

    const int bound = upperBound(idx, ordered, remainFreq, static_cast<int>(currentSet.size()));
    if (bound < bestResult.optimalMax) {
        return;
    }
    if (bound == bestResult.optimalMax &&
        !bestResult.bestSet.empty() &&
        !lexicographicallyBetter(currentSet, bestResult.bestSet)) {
        return;
    }

    const std::string& id = ordered[idx];
    const PhraseInfo* phrase = indexStore_.getPhraseById(id);

    if (phrase != nullptr) {
        CandidateEngine checker(indexStore_);
        if (checker.canCompose(phrase->charFreq, remainFreq)) {
            currentSet.push_back(id);
            const CharFreq nextRemain = subtractFreq(remainFreq, phrase->charFreq);
            dfsPick(idx + 1, ordered, nextRemain, currentSet, bestResult);
            currentSet.pop_back();
        }
    }

    dfsPick(idx + 1, ordered, remainFreq, currentSet, bestResult);
}

OptimalSolver::CandidateIdList OptimalSolver::sanitizeCandidates(
    const CandidateIdList& candidates,
    const CharFreq& pool
) const {
    CandidateIdList valid;
    std::unordered_set<std::string> seen;
    CandidateEngine checker(indexStore_);

    for (const auto& id : candidates) {
        if (!seen.insert(id).second) {
            continue;
        }

        const PhraseInfo* phrase = indexStore_.getPhraseById(id);
        if (phrase == nullptr) {
            continue;
        }

        if (!checker.canCompose(phrase->charFreq, pool)) {
            continue;
        }

        valid.push_back(id);
    }

    return valid;
}

OptimalSolver::CandidateIdList OptimalSolver::rankCandidates(
    const CandidateIdList& candidates,
    const CharFreq& pool,
    const ConflictGraph* conflictGraph
) const {
    struct ScoreRow {
        std::string id;
        int length = 0;
        double rarityScore = 0.0;
        int conflictDegree = 0;
        PhraseType type = PhraseType::Poem;
        std::string text;
    };

    std::vector<ScoreRow> rows;
    rows.reserve(candidates.size());

    for (const auto& id : candidates) {
        const PhraseInfo* phrase = indexStore_.getPhraseById(id);
        if (phrase == nullptr) {
            continue;
        }

        double rarityScore = 0.0;
        for (const auto& [cp, need] : phrase->charFreq) {
            const auto it = pool.find(cp);
            if (it == pool.end() || it->second <= 0) {
                continue;
            }
            rarityScore += static_cast<double>(need) / static_cast<double>(it->second);
        }

        int conflictDegree = 0;
        if (conflictGraph != nullptr) {
            const auto it = conflictGraph->find(id);
            if (it != conflictGraph->end()) {
                conflictDegree = static_cast<int>(it->second.size());
            }
        }

        rows.push_back({
            id,
            phrase->length,
            rarityScore,
            conflictDegree,
            phrase->type,
            phrase->text
        });
    }

    std::sort(rows.begin(), rows.end(), [](const ScoreRow& a, const ScoreRow& b) {
        if (a.length != b.length) {
            return a.length < b.length;                     // 短句优先
        }
        if (std::fabs(a.rarityScore - b.rarityScore) > 1e-9) {
            return a.rarityScore > b.rarityScore;          // 稀有字优先
        }
        if (a.conflictDegree != b.conflictDegree) {
            return a.conflictDegree < b.conflictDegree;    // 冲突度低优先
        }
        if (a.type != b.type) {
            return static_cast<int>(a.type) < static_cast<int>(b.type);
        }
        if (a.text != b.text) {
            return a.text < b.text;
        }
        return a.id < b.id;
    });

    CandidateIdList ordered;
    ordered.reserve(rows.size());
    for (const auto& row : rows) {
        ordered.push_back(row.id);
    }
    return ordered;
}

SolveResult OptimalSolver::greedyFallback(const CandidateIdList& ordered, const CharFreq& pool) const {
    SolveResult result;
    result.usedExact = false;
    result.strategy = "greedy";

    CandidateEngine checker(indexStore_);
    CharFreq remain = pool;

    for (const auto& id : ordered) {
        const PhraseInfo* phrase = indexStore_.getPhraseById(id);
        if (phrase == nullptr) {
            continue;
        }

        if (checker.canCompose(phrase->charFreq, remain)) {
            result.bestSet.push_back(id);
            remain = subtractFreq(remain, phrase->charFreq);
        }
    }

    result.optimalMax = static_cast<int>(result.bestSet.size());
    return result;
}

SolveResult OptimalSolver::improveGreedySolution(
    const SolveResult& base,
    const CandidateIdList& ordered,
    const CharFreq& pool
) const {
    SolveResult best = base;
    best.usedExact = false;
    best.strategy = "greedy_local_improvement";

    if (base.bestSet.empty()) {
        return best;
    }

    const std::size_t dropLimit = std::min<std::size_t>(kLocalRepairDropLimit, base.bestSet.size());

    for (std::size_t dropIdx = 0; dropIdx < dropLimit; ++dropIdx) {
        const std::string& droppedId = base.bestSet[dropIdx];
        const PhraseInfo* droppedPhrase = indexStore_.getPhraseById(droppedId);
        if (droppedPhrase == nullptr) {
            continue;
        }

        std::unordered_set<std::string> fixedChosen;
        CharFreq remain = pool;

        for (std::size_t i = 0; i < base.bestSet.size(); ++i) {
            if (i == dropIdx) {
                continue;
            }

            const PhraseInfo* phrase = indexStore_.getPhraseById(base.bestSet[i]);
            if (phrase == nullptr) {
                continue;
            }

            remain = subtractFreq(remain, phrase->charFreq);
            fixedChosen.insert(base.bestSet[i]);
        }

        SolveResult candidate;
        candidate.usedExact = false;
        candidate.strategy = "greedy_local_improvement";

        for (const auto& id : base.bestSet) {
            if (fixedChosen.find(id) != fixedChosen.end()) {
                candidate.bestSet.push_back(id);
            }
        }

        CandidateEngine checker(indexStore_);
        for (const auto& id : ordered) {
            if (fixedChosen.find(id) != fixedChosen.end()) {
                continue;
            }

            const PhraseInfo* phrase = indexStore_.getPhraseById(id);
            if (phrase == nullptr) {
                continue;
            }

            if (checker.canCompose(phrase->charFreq, remain)) {
                candidate.bestSet.push_back(id);
                fixedChosen.insert(id);
                remain = subtractFreq(remain, phrase->charFreq);
            }
        }

        candidate.optimalMax = static_cast<int>(candidate.bestSet.size());
        std::sort(candidate.bestSet.begin(), candidate.bestSet.end());

        if (candidate.optimalMax > best.optimalMax ||
            (candidate.optimalMax == best.optimalMax &&
             lexicographicallyBetter(candidate.bestSet, best.bestSet))) {
            best = candidate;
        }
    }

    return best;
}

CharFreq OptimalSolver::subtractFreq(const CharFreq& lhs, const CharFreq& rhs) {
    CharFreq result = lhs;

    for (const auto& [cp, cnt] : rhs) {
        auto it = result.find(cp);
        if (it == result.end()) {
            continue;
        }

        it->second -= cnt;
        if (it->second <= 0) {
            result.erase(it);
        }
    }

    return result;
}

CharFreq OptimalSolver::addFreq(const CharFreq& lhs, const CharFreq& rhs) {
    CharFreq result = lhs;
    for (const auto& [cp, cnt] : rhs) {
        result[cp] += cnt;
    }
    return result;
}

bool OptimalSolver::lexicographicallyBetter(
    const std::vector<std::string>& lhs,
    const std::vector<std::string>& rhs
) {
    if (rhs.empty()) {
        return true;
    }

    std::vector<std::string> a = lhs;
    std::vector<std::string> b = rhs;
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());

    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
}

} // namespace lineverse::poetryrebuild