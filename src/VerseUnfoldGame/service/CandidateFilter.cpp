#include "CandidateFilter.h"

#include <sstream>
#include <unordered_set>

#include "../utils/StringNormalizer.h"

std::vector<int> CandidateFilter::filter(
    const std::vector<int>& currentCandidates,
    const HintItem& hint,
    const PoetryIndex& index,
    const std::vector<Poem>& poems
) const {
    if (hint.type == "dynasty") {
        const std::vector<int>* indexed = index.queryByDynasty(hint.value);
        return indexed ? intersect(currentCandidates, *indexed) : std::vector<int>{};
    }

    if (hint.type == "type") {
        const std::vector<int>* indexed = index.queryByType(hint.value);
        return indexed ? intersect(currentCandidates, *indexed) : std::vector<int>{};
    }

    if (hint.type == "emotion") {
        const std::vector<int>* indexed = index.queryByEmotion(hint.value);
        return indexed ? intersect(currentCandidates, *indexed) : std::vector<int>{};
    }

    if (hint.type == "main_imagery") {
        const std::vector<int>* indexed = index.queryByImagery(hint.value);
        return indexed ? intersect(currentCandidates, *indexed) : std::vector<int>{};
    }

    if (hint.type == "firstline_prefix") {
        const std::vector<int>* indexed = index.queryByPrefix(hint.value);
        return indexed ? intersect(currentCandidates, *indexed) : std::vector<int>{};
    }

    return filterByTraversal(currentCandidates, hint, poems);
}

std::vector<int> CandidateFilter::intersect(
    const std::vector<int>& a,
    const std::vector<int>& b
) const {
    std::unordered_set<int> setB(b.begin(), b.end());
    std::vector<int> result;

    for (int x : a) {
        if (setB.find(x) != setB.end()) {
            result.push_back(x);
        }
    }

    return result;
}

std::vector<int> CandidateFilter::filterByTraversal(
    const std::vector<int>& currentCandidates,
    const HintItem& hint,
    const std::vector<Poem>& poems
) const {
    std::vector<int> result;

    for (int idx : currentCandidates) {
        if (idx < 0 || idx >= static_cast<int>(poems.size())) {
            continue;
        }

        const Poem& poem = poems[idx];
        bool matched = false;

        if (hint.type == "background") {
            matched = StringNormalizer::equalsForJudge(poem.bgCore, hint.value);
        }
        else if (hint.type == "feature") {
            matched = StringNormalizer::equalsForJudge(poem.featureCore, hint.value);
        }
        else if (hint.type == "all_imagery") {
            std::ostringstream oss;
            for (size_t i = 0; i < poem.imageryGameAll.size(); ++i) {
                oss << poem.imageryGameAll[i];
                if (i + 1 < poem.imageryGameAll.size()) {
                    oss << "、";
                }
            }
            matched = StringNormalizer::equalsForJudge(oss.str(), hint.value);
        }
        else if (hint.type == "structure") {
            std::ostringstream oss;
            oss << poem.sentenceCount << "|" << poem.charCount;
            matched = (oss.str() == hint.value);
        }
        else if (hint.type == "answer") {
            matched = true;
        }

        if (matched) {
            result.push_back(idx);
        }
    }

    return result;
}
