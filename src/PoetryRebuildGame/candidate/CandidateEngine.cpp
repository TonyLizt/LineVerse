#include "CandidateEngine.h"

#include <algorithm>
#include <unordered_set>

namespace lineverse::poetryrebuild {

CandidateEngine::CandidateEngine(const IndexStore& indexStore)
    : indexStore_(indexStore) {
}

bool CandidateEngine::canCompose(const CharFreq& need, const CharFreq& pool) const {
    for (const auto& [cp, requiredCount] : need) {
        const auto it = pool.find(cp);
        const int availableCount = (it == pool.end()) ? 0 : it->second;
        if (availableCount < requiredCount) {
            return false;
        }
    }
    return true;
}

CandidateEngine::CandidateIdList CandidateEngine::getInitialCandidates(const CharFreq& pool) const {
    CandidateIdList recalled;
    std::unordered_set<std::string> seen;

    for (const auto& [cp, count] : pool) {
        if (count <= 0) {
            continue;
        }

        const auto ids = indexStore_.getPhraseIdsByChar(cp);
        for (const auto& id : ids) {
            if (seen.insert(id).second) {
                recalled.push_back(id);
            }
        }
    }

    CandidateIdList candidates;
    candidates.reserve(recalled.size());

    for (const auto& id : recalled) {
        const PhraseInfo* phrase = indexStore_.getPhraseById(id);
        if (phrase == nullptr) {
            continue;
        }
        if (canCompose(phrase->charFreq, pool)) {
            candidates.push_back(id);
        }
    }

    sortCandidates(candidates);
    return candidates;
}

CandidateEngine::CandidateIdList CandidateEngine::filterByLength(
    const CandidateIdList& ids,
    int minLen,
    int maxLen
) const {
    CandidateIdList filtered;

    for (const auto& id : ids) {
        const PhraseInfo* phrase = indexStore_.getPhraseById(id);
        if (phrase == nullptr) {
            continue;
        }
        if (phrase->length < minLen || phrase->length > maxLen) {
            continue;
        }
        filtered.push_back(id);
    }

    sortCandidates(filtered);
    return filtered;
}

CandidateEngine::CandidateIdList CandidateEngine::filterByType(
    const CandidateIdList& ids,
    bool allowPoem,
    bool allowIdiom
) const {
    if (!allowPoem && !allowIdiom) {
        return {};
    }

    CandidateIdList filtered;

    for (const auto& id : ids) {
        const PhraseInfo* phrase = indexStore_.getPhraseById(id);
        if (phrase == nullptr) {
            continue;
        }

        const bool keepPoem = (phrase->type == PhraseType::Poem && allowPoem);
        const bool keepIdiom = (phrase->type == PhraseType::Idiom && allowIdiom);
        if (keepPoem || keepIdiom) {
            filtered.push_back(id);
        }
    }

    sortCandidates(filtered);
    return filtered;
}

int CandidateEngine::countCandidates(const CharFreq& pool) const {
    return static_cast<int>(getInitialCandidates(pool).size());
}

CandidateEngine::CandidateIdList CandidateEngine::getCandidates(
    const CharFreq& pool,
    const CandidateQueryOptions& options
) const {
    CandidateIdList ids = getInitialCandidates(pool);
    ids = filterByType(ids, options.allowPoem, options.allowIdiom);
    ids = filterByLength(ids, options.minLen, options.maxLen);
    sortCandidates(ids);
    return ids;
}

std::vector<std::string> CandidateEngine::getCandidateTexts(const CandidateIdList& ids) const {
    std::vector<std::string> texts;
    texts.reserve(ids.size());

    for (const auto& id : ids) {
        const PhraseInfo* phrase = indexStore_.getPhraseById(id);
        if (phrase != nullptr) {
            texts.push_back(phrase->text);
        }
    }

    return texts;
}

void CandidateEngine::sortCandidates(CandidateIdList& ids) const {
    std::sort(ids.begin(), ids.end(), [this](const std::string& lhs, const std::string& rhs) {
        const PhraseInfo* a = indexStore_.getPhraseById(lhs);
        const PhraseInfo* b = indexStore_.getPhraseById(rhs);

        if (a == nullptr || b == nullptr) {
            return lhs < rhs;
        }

        if (a->length != b->length) {
            return a->length < b->length;
        }

        if (a->type != b->type) {
            return static_cast<int>(a->type) < static_cast<int>(b->type);
        }

        if (a->text != b->text) {
            return a->text < b->text;
        }

        return a->id < b->id;
    });

    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
}

} // namespace lineverse::poetryrebuild