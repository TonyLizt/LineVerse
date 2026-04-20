#pragma once

#include <vector>

#include "../data/PoetryIndex.h"
#include "../data/model/HintItem.h"
#include "../data/model/Poem.h"

class CandidateFilter {
public:
    std::vector<int> filter(
        const std::vector<int>& currentCandidates,
        const HintItem& hint,
        const PoetryIndex& index,
        const std::vector<Poem>& poems
    ) const;

private:
    std::vector<int> intersect(
        const std::vector<int>& a,
        const std::vector<int>& b
    ) const;

    std::vector<int> filterByTraversal(
        const std::vector<int>& currentCandidates,
        const HintItem& hint,
        const std::vector<Poem>& poems
    ) const;
};