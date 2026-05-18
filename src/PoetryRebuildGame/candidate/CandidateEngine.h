#pragma once

#include <limits>
#include <string>
#include <vector>

#include "../common/Types.h"
#include "../index/IndexStore.h"
#include "../model/PhraseInfo.h"

namespace lineverse::poetryrebuild {

struct CandidateQueryOptions {
    int minLen = 0;
    int maxLen = std::numeric_limits<int>::max();
    bool allowPoem = true;
    bool allowIdiom = true;
};

class CandidateEngine {
public:
    using CandidateIdList = std::vector<std::string>;

    explicit CandidateEngine(const IndexStore& indexStore);

    bool canCompose(const CharFreq& need, const CharFreq& pool) const;

    CandidateIdList getInitialCandidates(const CharFreq& pool) const;
    CandidateIdList filterByLength(const CandidateIdList& ids, int minLen, int maxLen) const;
    CandidateIdList filterByType(const CandidateIdList& ids, bool allowPoem, bool allowIdiom) const;

    int countCandidates(const CharFreq& pool) const;

    CandidateIdList getCandidates(const CharFreq& pool, const CandidateQueryOptions& options = {}) const;
    std::vector<std::string> getCandidateTexts(const CandidateIdList& ids) const;

private:
    const IndexStore& indexStore_;

    void sortCandidates(CandidateIdList& ids) const;
};

} // namespace lineverse::poetryrebuild