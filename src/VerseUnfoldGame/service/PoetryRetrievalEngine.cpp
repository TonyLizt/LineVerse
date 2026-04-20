#include "PoetryRetrievalEngine.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>

#include "../utils/StringNormalizer.h"

namespace {
const std::vector<int>* queryFieldSet(const PoetryIndex& index, QueryField field, const std::string& value) {
    switch (field) {
    case QueryField::Dynasty:
        return index.queryByDynasty(value);
    case QueryField::Author:
        return index.queryByAuthor(value);
    case QueryField::Type:
        return index.queryByType(value);
    case QueryField::Imagery:
        return index.queryByImagery(value);
    case QueryField::Prefix:
        return index.queryByPrefix(value);
    case QueryField::Title:
        return index.queryByTitle(value);
    case QueryField::Structure:
        return index.queryByStructure(value);
    case QueryField::Emotion:
    case QueryField::Background:
    case QueryField::Feature:
    case QueryField::Content:
        return nullptr;
    }
    return nullptr;
}

// ===================== 替换1：新增强弱字段判断函数 =====================
bool isWeakGuessField(QueryField field) {
    return field == QueryField::Dynasty ||
           field == QueryField::Type ||
           field == QueryField::Structure;
}

bool isGuessStrongField(QueryField field) {
    return field == QueryField::Author ||
           field == QueryField::Imagery ||
           field == QueryField::Prefix ||
           field == QueryField::Title ||
           field == QueryField::Content ||
           field == QueryField::Emotion ||
           field == QueryField::Background ||
           field == QueryField::Feature;
}
// ====================================================================

std::string normalizedTextJoin(std::initializer_list<std::string> parts) {
    std::string joined;
    for (const auto& part : parts) {
        joined += StringNormalizer::normalizeContent(part);
    }
    return joined;
}

double tokenSignalWeight(const std::string& tokenStd) {
    if (tokenStd.size() >= 8) return 1.35;
    if (tokenStd.size() >= 5) return 1.00;
    if (tokenStd.size() >= 3) return 0.70;
    return 0.30;
}

std::vector<std::string> extractMeaningfulFragments(const std::string& tokenStd) {
    static const std::unordered_set<std::string> stop = {
        "作者", "朝代", "体裁", "作品", "诗词", "这首", "表达", "表现", "描写", "抒发",
        "创作", "背景", "而作", "所作", "时候", "当时", "之一", "一种", "这个", "那个",
        "核心", "情感", "意象", "特色", "语言", "风格", "非常", "比较", "有点", "的", "了"
    };

    std::vector<std::string> result;
    std::unordered_set<std::string> seen;
    const size_t n = tokenStd.size();
    for (size_t len = 4; len >= 2; --len) {
        if (n < len) continue;
        for (size_t i = 0; i + len <= n; ++i) {
            std::string frag = tokenStd.substr(i, len);
            if (stop.find(frag) != stop.end()) {
                continue;
            }
            bool allDigit = true;
            for (char ch : frag) {
                if (ch < '0' || ch > '9') {
                    allDigit = false;
                    break;
                }
            }
            if (allDigit) {
                continue;
            }
            if (seen.insert(frag).second) {
                result.push_back(frag);
            }
        }
        if (len == 2) break;
    }
    return result;
}

double partialFragmentMatchScore(const std::string& tokenStd, const std::string& searchableStd) {
    if (tokenStd.empty() || searchableStd.empty()) {
        return 0.0;
    }
    if (searchableStd.find(tokenStd) != std::string::npos) {
        return 2.3;
    }

    double score = 0.0;
    for (const auto& frag : extractMeaningfulFragments(tokenStd)) {
        if (searchableStd.find(frag) == std::string::npos) {
            continue;
        }
        if (frag.size() >= 4) score += 0.95;
        else if (frag.size() == 3) score += 0.62;
        else score += 0.30;
    }
    if (score > 2.6) score = 2.6;
    return score;
}
}

// ===================== 替换2：完整替换 retrieve 函数 =====================
RetrievalResult PoetryRetrievalEngine::retrieve(
    const DescriptionQuery& query,
    const std::vector<int>& currentCandidates,
    const PoetryIndex& index,
    const std::vector<Poem>& poems,
    const std::vector<std::string>& historyDescriptions,
    bool allowQuestion
) const {
    RetrievalResult result;

    const int beforeCount = static_cast<int>(currentCandidates.size());
    std::vector<int> narrowed = applyStructuredFilter(query, currentCandidates, index);
    if (narrowed.empty()) {
        narrowed = currentCandidates;
    }

    result.ranked = rankCandidates(query, narrowed, poems);

    result.nextCandidates.clear();
    for (const auto& item : result.ranked) {
        result.nextCandidates.push_back(item.poemIndex);
    }

    if (!result.ranked.empty()) {
        result.bestGuessIndex = result.ranked.front().poemIndex;
    }

    result.confidencePercent = computeConfidencePercent(
        query,
        result.ranked,
        static_cast<int>(result.nextCandidates.size())
    );

    result.feedback = buildFeedback(
        beforeCount,
        static_cast<int>(result.nextCandidates.size()),
        result.confidencePercent
    );

    const int candidateCount = static_cast<int>(result.nextCandidates.size());
    if (result.bestGuessIndex >= 0 && !result.ranked.empty()) {
        const double top1 = result.ranked[0].score;
        const double top2 = (result.ranked.size() >= 2) ? result.ranked[1].score : 0.0;
        const double gap = top1 - top2;

        int strongHitCount = 0;
        for (const auto& hit : query.hits) {
            if (isGuessStrongField(hit.field)) {
                ++strongHitCount;
            }
        }

        int strongTokenCount = 0;
        for (const auto& token : query.freeTokens) {
            if (token.size() >= 3) {
                ++strongTokenCount;
            }
        }

        if (candidateCount == 1 &&
            result.confidencePercent >= 90 &&
            (strongHitCount >= 1 || strongTokenCount >= 1) &&
            top1 >= 3.4) {
            result.shouldGuess = true;
        }
        else if (candidateCount <= 2 &&
                 result.confidencePercent >= 94 &&
                 strongHitCount >= 2 &&
                 gap >= 2.8 &&
                 top1 >= 4.8) {
            result.shouldGuess = true;
        }
        else if (candidateCount <= 3 &&
                 result.confidencePercent >= 97 &&
                 strongHitCount >= 2 &&
                 (strongHitCount + strongTokenCount) >= 3 &&
                 gap >= 3.4 &&
                 top1 >= 6.0) {
            result.shouldGuess = true;
        }
    }

    if (!result.shouldGuess && allowQuestion) {
        result.nextQuestion = buildNextQuestion(result.nextCandidates, poems, historyDescriptions);
    }

    return result;
}
// ====================================================================

std::string PoetryRetrievalEngine::suggestNextQuestion(
    const std::vector<int>& candidates,
    const std::vector<Poem>& poems,
    const std::vector<std::string>& historyDescriptions
) const {
    return buildNextQuestion(candidates, poems, historyDescriptions);
}

std::vector<int> PoetryRetrievalEngine::intersect(const std::vector<int>& a, const std::vector<int>& b) const {
    std::unordered_set<int> setB(b.begin(), b.end());
    std::vector<int> result;
    for (int x : a) {
        if (setB.find(x) != setB.end()) {
            result.push_back(x);
        }
    }
    return result;
}

std::vector<int> PoetryRetrievalEngine::unionAll(const std::vector<std::vector<int>>& groups) const {
    std::vector<int> result;
    std::unordered_set<int> seen;
    for (const auto& g : groups) {
        for (int x : g) {
            if (seen.insert(x).second) {
                result.push_back(x);
            }
        }
    }
    return result;
}

std::vector<int> PoetryRetrievalEngine::applyStructuredFilter(
    const DescriptionQuery& query,
    const std::vector<int>& currentCandidates,
    const PoetryIndex& index
) const {
    std::vector<int> result = currentCandidates;
    bool usedAnyIndex = false;

    for (int fieldId = static_cast<int>(QueryField::Dynasty);
         fieldId <= static_cast<int>(QueryField::Structure);
         ++fieldId) {
        QueryField field = static_cast<QueryField>(fieldId);
        std::vector<std::vector<int>> sameFieldGroups;

        for (const auto& hit : query.hits) {
            if (hit.field != field) {
                continue;
            }

            const std::vector<int>* indexed = queryFieldSet(index, field, hit.value);
            if (indexed != nullptr) {
                sameFieldGroups.push_back(*indexed);
            }
        }

        if (!sameFieldGroups.empty()) {
            usedAnyIndex = true;
            std::vector<int> fieldUnion = unionAll(sameFieldGroups);
            result = intersect(result, fieldUnion);
        }
    }

    if (!usedAnyIndex) {
        return currentCandidates;
    }

    return result;
}

std::vector<RetrievalCandidate> PoetryRetrievalEngine::rankCandidates(
    const DescriptionQuery& query,
    const std::vector<int>& candidates,
    const std::vector<Poem>& poems
) const {
    std::vector<RetrievalCandidate> ranked;
    for (int idx : candidates) {
        if (idx < 0 || idx >= static_cast<int>(poems.size())) {
            continue;
        }
        double score = scorePoem(query, poems[idx]);
        ranked.push_back({idx, score});
    }

    std::sort(ranked.begin(), ranked.end(), [](const RetrievalCandidate& a, const RetrievalCandidate& b) {
        if (a.score != b.score) {
            return a.score > b.score;
        }
        return a.poemIndex < b.poemIndex;
    });

    return ranked;
}

double PoetryRetrievalEngine::scorePoem(const DescriptionQuery& query, const Poem& poem) const {
    double score = 0.0;
    const std::string bgText = normalizedTextJoin({poem.bgCore, poem.creationBg, poem.searchTextStd});
    const std::string emotionText = normalizedTextJoin({poem.emotionCore, poem.emotion, poem.searchTextStd});
    const std::string featureText = normalizedTextJoin({poem.featureCore, poem.distinctFeature, poem.searchTextStd});

    for (const auto& hit : query.hits) {
        const std::string hitStd = StringNormalizer::normalizeContent(hit.value);
        switch (hit.field) {
        case QueryField::Dynasty:
            if (StringNormalizer::equalsForJudge(hit.value, poem.dynasty)) score += hit.weight;
            break;
        case QueryField::Author:
            if (StringNormalizer::equalsForJudge(hit.value, poem.author)) score += hit.weight;
            break;
        case QueryField::Type:
            if (StringNormalizer::equalsForJudge(hit.value, poem.type)) score += hit.weight;
            break;
        case QueryField::Emotion: {
            const double partial = partialFragmentMatchScore(hitStd, emotionText);
            if (partial > 0.0) score += hit.weight + partial;
            else if (!hitStd.empty()) score -= 0.75;
            break;
        }
        case QueryField::Background: {
            const double partial = partialFragmentMatchScore(hitStd, bgText);
            if (partial > 0.0) score += hit.weight + partial + 0.2;
            else if (!hitStd.empty()) score -= 0.95;
            break;
        }
        case QueryField::Feature: {
            const double partial = partialFragmentMatchScore(hitStd, featureText);
            if (partial > 0.0) score += hit.weight + partial * 0.85;
            else if (!hitStd.empty()) score -= 0.55;
            break;
        }
        case QueryField::Imagery:
            if (StringNormalizer::containsNormalized(poem.mainImageryOne, hit.value)) {
                score += hit.weight + 0.9;
                break;
            }
            for (const auto& img : poem.imageryGameAll) {
                if (StringNormalizer::containsNormalized(img, hit.value) || StringNormalizer::containsNormalized(hit.value, img)) {
                    score += hit.weight + 0.45;
                    goto imagery_done;
                }
            }
            for (const auto& img : poem.imagery) {
                if (StringNormalizer::containsNormalized(img, hit.value) || StringNormalizer::containsNormalized(hit.value, img)) {
                    score += hit.weight;
                    goto imagery_done;
                }
            }
            score -= 0.40;
imagery_done:
            break;
        case QueryField::Prefix:
            if (StringNormalizer::containsNormalized(poem.firstlinePrefix, hit.value)) score += hit.weight;
            break;
        case QueryField::Title:
            if (StringNormalizer::equalsForJudge(hit.value, poem.title)) score += hit.weight + 1.0;
            break;
        case QueryField::Content:
            for (const auto& line : poem.lines) {
                if (StringNormalizer::containsNormalized(line, hit.value) || StringNormalizer::containsNormalized(hit.value, line)) {
                    score += hit.weight;
                    break;
                }
            }
            break;
        case QueryField::Structure:
            if (hit.value == buildSearchableStructureText(poem)) score += hit.weight + 0.5;
            break;
        }
    }

    for (const auto& token : query.freeTokens) {
        if (token.empty()) continue;
        const double weight = tokenSignalWeight(token);
        if (poem.searchTextStd.find(token) != std::string::npos) {
            score += weight + 0.35;
        }
        else {
            const double partial = partialFragmentMatchScore(token, poem.searchTextStd);
            if (partial > 0.0) score += partial * 0.9;
            else if (token.size() >= 4) score -= std::min(0.55, weight * 0.40);
        }
    }

    return score;
}

std::string PoetryRetrievalEngine::buildSearchableStructureText(const Poem& poem) const {
    return std::to_string(poem.sentenceCount) + "|" + std::to_string(poem.charCount);
}

// ===================== 替换3：完整替换 computeConfidencePercent 函数 =====================
int PoetryRetrievalEngine::computeConfidencePercent(
    const DescriptionQuery& query,
    const std::vector<RetrievalCandidate>& ranked,
    int candidateCount
) const {
    if (ranked.empty()) {
        return 0;
    }

    const double top1 = ranked[0].score;
    const double top2 = (ranked.size() >= 2) ? ranked[1].score : 0.0;
    const double gap = std::max(0.0, top1 - top2);

    int strongHitCount = 0;
    for (const auto& hit : query.hits) {
        if (isGuessStrongField(hit.field)) {
            ++strongHitCount;
        }
    }

    int strongTokenCount = 0;
    for (const auto& token : query.freeTokens) {
        if (token.size() >= 3) {
            ++strongTokenCount;
        }
    }

    int confidence = 4;
    confidence += std::min(24, static_cast<int>(std::round(top1 * 3.2)));
    confidence += std::min(18, static_cast<int>(std::round(gap * 5.5)));
    confidence += std::min(12, static_cast<int>(query.hits.size()) * 2);
    confidence += std::min(16, strongHitCount * 4);
    confidence += std::min(8, strongTokenCount * 3);

    if (candidateCount <= 10) confidence += 3;
    if (candidateCount <= 5) confidence += 5;
    if (candidateCount <= 3) confidence += 7;
    if (candidateCount == 1) confidence += 10;

    if (strongHitCount == 0 && strongTokenCount == 0) {
        confidence = std::min(confidence, 52);
    }
    else if (strongHitCount == 0 && strongTokenCount == 1) {
        confidence = std::min(confidence, 66);
    }
    else if (strongHitCount == 1 && candidateCount > 1) {
        confidence = std::min(confidence, 80);
    }

    if (candidateCount > 12) confidence = std::min(confidence, 48);
    else if (candidateCount > 6) confidence = std::min(confidence, 60);
    else if (candidateCount > 3) confidence = std::min(confidence, 72);
    else if (candidateCount > 1) confidence = std::min(confidence, 86);

    if (top1 < 3.0) {
        confidence = std::min(confidence, 60);
    }
    if (gap < 1.0 && candidateCount > 1) {
        confidence = std::min(confidence, 62);
    }

    if (confidence < 0) confidence = 0;
    if (confidence > 98) confidence = 98;
    return confidence;
}
// ====================================================================

std::string PoetryRetrievalEngine::buildNextQuestion(
    const std::vector<int>& candidates,
    const std::vector<Poem>& poems,
    const std::vector<std::string>& historyDescriptions
) const {
    if (candidates.empty()) {
        return "你刚才给的信息太模糊了，请直接补充朝代、作者、体裁、意象或结构信息。";
    }
    if (candidates.size() == 1) {
        return "我已经缩小到很小范围了，请再补充一条最关键的信息。";
    }

    std::string historyStd;
    for (const auto& s : historyDescriptions) {
        historyStd += StringNormalizer::normalizeContent(s);
    }

    struct QuestionOption {
        int maxBucket = 0;
        int distinctCount = 0;
        std::string label;
    };

    std::vector<QuestionOption> options;
    const std::vector<std::pair<QueryField, std::string>> fields = {
        {QueryField::Author, "作者"},
        {QueryField::Type, "体裁"},
        {QueryField::Imagery, "核心意象"},
        {QueryField::Emotion, "核心情感"},
        {QueryField::Background, "创作背景"},
        {QueryField::Feature, "艺术特色"},
        {QueryField::Dynasty, "朝代"},
        {QueryField::Prefix, "首句前两个字"},
        {QueryField::Structure, "句数和总字数"}
    };

    for (const auto& fieldInfo : fields) {
        std::unordered_map<std::string, int> bucket;
        bool alreadyMentioned = false;

        for (int idx : candidates) {
            if (idx < 0 || idx >= static_cast<int>(poems.size())) {
                continue;
            }

            const Poem& poem = poems[idx];
            std::string value;
            switch (fieldInfo.first) {
            case QueryField::Author: value = poem.author; break;
            case QueryField::Type: value = poem.type; break;
            case QueryField::Imagery: value = poem.mainImageryOne; break;
            case QueryField::Emotion: value = poem.emotionCore; break;
            case QueryField::Background: value = poem.bgCore; break;
            case QueryField::Feature: value = poem.featureCore; break;
            case QueryField::Dynasty: value = poem.dynasty; break;
            case QueryField::Prefix: value = poem.firstlinePrefix; break;
            case QueryField::Structure:
                value = std::to_string(poem.sentenceCount) + "句" + std::to_string(poem.charCount) + "字";
                break;
            default:
                break;
            }

            if (value.empty()) {
                continue;
            }

            bucket[value]++;
            if (!alreadyMentioned) {
                const std::string valueStd = StringNormalizer::normalizeContent(value);
                if (!valueStd.empty() && historyStd.find(valueStd) != std::string::npos) {
                    alreadyMentioned = true;
                }
            }
        }

        if (alreadyMentioned || bucket.size() <= 1) {
            continue;
        }

        int maxBucket = 0;
        for (const auto& kv : bucket) {
            if (kv.second > maxBucket) {
                maxBucket = kv.second;
            }
        }

        options.push_back({maxBucket, static_cast<int>(bucket.size()), fieldInfo.second});
    }

    if (options.empty()) {
        return "请再补充一条更具体的信息，例如作者、意象、体裁或首句前两个字。";
    }

    std::sort(options.begin(), options.end(), [](const QuestionOption& a, const QuestionOption& b) {
        if (a.maxBucket != b.maxBucket) {
            return a.maxBucket < b.maxBucket;
        }
        return a.distinctCount > b.distinctCount;
    });

    return "请你再告诉我这首诗的" + options.front().label + "。";
}

std::string PoetryRetrievalEngine::buildFeedback(int beforeCount, int afterCount, int confidencePercent) const {
    if (afterCount <= 1) {
        return "这个提示非常关键，我已经非常接近答案了。";
    }
    if (afterCount < beforeCount / 2) {
        return "这个信息很关键，范围明显缩小了。";
    }
    if (confidencePercent >= 82) {
        return "我的判断已经比较集中，再多一条关键信息就更稳了。";
    }
    if (confidencePercent >= 58) {
        return "这个提示有帮助，但我还不够稳，最好再补充作者、体裁或核心意象。";
    }
    return "这个提示有点宽泛，可以继续补充作者、体裁、意象或结构信息。";
}