#pragma once

#include <string>
#include <vector>

#include "PoetryDescriptionParser.h"
#include "../data/PoetryIndex.h"
#include "../data/model/Poem.h"

struct RetrievalCandidate {
    int poemIndex = -1;
    double score = 0.0;
};

struct RetrievalResult {
    std::vector<int> nextCandidates;
    std::vector<RetrievalCandidate> ranked;
    bool shouldGuess = false;
    int bestGuessIndex = -1;
    int confidencePercent = 0;
    std::string nextQuestion;
    std::string feedback;
};

class PoetryRetrievalEngine {
public:
    RetrievalResult retrieve(
        const DescriptionQuery& query,
        const std::vector<int>& currentCandidates,
        const PoetryIndex& index,
        const std::vector<Poem>& poems,
        const std::vector<std::string>& historyDescriptions,
        bool allowQuestion
    ) const;

    std::string suggestNextQuestion(
        const std::vector<int>& candidates,
        const std::vector<Poem>& poems,
        const std::vector<std::string>& historyDescriptions
    ) const;

private:
    std::vector<int> intersect(const std::vector<int>& a, const std::vector<int>& b) const;
    std::vector<int> unionAll(const std::vector<std::vector<int>>& groups) const;
    std::vector<int> applyStructuredFilter(
        const DescriptionQuery& query,
        const std::vector<int>& currentCandidates,
        const PoetryIndex& index
    ) const;
    std::vector<RetrievalCandidate> rankCandidates(
        const DescriptionQuery& query,
        const std::vector<int>& candidates,
        const std::vector<Poem>& poems
    ) const;
    double scorePoem(const DescriptionQuery& query, const Poem& poem) const;
    std::string buildSearchableStructureText(const Poem& poem) const;
    std::string buildNextQuestion(
        const std::vector<int>& candidates,
        const std::vector<Poem>& poems,
        const std::vector<std::string>& historyDescriptions
    ) const;
    std::string buildFeedback(int beforeCount, int afterCount, int confidencePercent) const;
    int computeConfidencePercent(
        const DescriptionQuery& query,
        const std::vector<RetrievalCandidate>& ranked,
        int candidateCount
    ) const;
};
