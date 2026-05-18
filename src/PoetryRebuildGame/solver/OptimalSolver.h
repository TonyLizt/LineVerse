#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "../candidate/CandidateEngine.h"
#include "../common/Types.h"
#include "../index/IndexStore.h"
#include "../model/PhraseInfo.h"

namespace lineverse::poetryrebuild {

struct SolveResult {
    int optimalMax = 0;
    std::vector<std::string> bestSet;

    bool usedExact = true;
    int exploredNodes = 0;
    int candidateCount = 0;
    std::string strategy = "exact_dfs";
};

class OptimalSolver {
public:
    using CandidateIdList = std::vector<std::string>;
    using ConflictGraph = std::unordered_map<std::string, CandidateIdList>;

    explicit OptimalSolver(const IndexStore& indexStore);

    SolveResult searchOptimalMax(const CandidateIdList& candidates, const CharFreq& pool) const;
    SolveResult searchOptimalMax(
        const CharFreq& pool,
        const CandidateEngine& candidateEngine,
        const CandidateQueryOptions& options = {}
    ) const;

    int upperBound(
        std::size_t idx,
        const CandidateIdList& ordered,
        const CharFreq& remainFreq,
        int pickedCount
    ) const;

    bool isConflict(const PhraseInfo& phraseA, const PhraseInfo& phraseB, const CharFreq& pool) const;
    ConflictGraph buildConflictGraph(const CandidateIdList& candidates, const CharFreq& pool) const;

    std::vector<std::string> getBestSolutionSet() const;

private:
    const IndexStore& indexStore_;
    mutable SolveResult lastResult_;

    static constexpr std::size_t kExactSolveThreshold = 220;
    static constexpr std::size_t kLocalRepairDropLimit = 24;

    void dfsPick(
        std::size_t idx,
        const CandidateIdList& ordered,
        const CharFreq& remainFreq,
        std::vector<std::string>& currentSet,
        SolveResult& bestResult
    ) const;

    CandidateIdList sanitizeCandidates(const CandidateIdList& candidates, const CharFreq& pool) const;

    CandidateIdList rankCandidates(
        const CandidateIdList& candidates,
        const CharFreq& pool,
        const ConflictGraph* conflictGraph
    ) const;

    SolveResult greedyFallback(const CandidateIdList& ordered, const CharFreq& pool) const;
    SolveResult improveGreedySolution(
        const SolveResult& base,
        const CandidateIdList& ordered,
        const CharFreq& pool
    ) const;

    static CharFreq subtractFreq(const CharFreq& lhs, const CharFreq& rhs);
    static CharFreq addFreq(const CharFreq& lhs, const CharFreq& rhs);
    static bool lexicographicallyBetter(
        const std::vector<std::string>& lhs,
        const std::vector<std::string>& rhs
    );
};

} // namespace lineverse::poetryrebuild