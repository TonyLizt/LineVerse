#include "LevelGenerator.h"

#include <algorithm>
#include <limits>
#include <sstream>
#include <unordered_set>

#include "../util/Utf8.h"

namespace lineverse::poetryrebuild {

LevelGenerator::LevelGenerator(
    const IndexStore& indexStore,
    const std::vector<PhraseInfo>& poems,
    const std::vector<PhraseInfo>& idioms
)
    : indexStore_(indexStore),
      poems_(poems),
      idioms_(idioms),
      rng_(std::random_device{}()) {
    idiomPool_.reserve(idioms_.size());
    for (const auto& item : idioms_) {
        if (item.length == 4) {
            idiomPool_.push_back(&item);
        }
    }

    poemPool_.reserve(poems_.size());
    for (const auto& item : poems_) {
        if (item.length == 5 || item.length == 7) {
            poemPool_.push_back(&item);
        }
    }

    std::unordered_set<CodePoint> uniqueChars;
    for (const auto& item : idioms_) {
        const auto cps = Utf8::toCodePoints(item.text);
        uniqueChars.insert(cps.begin(), cps.end());
    }
    for (const auto& item : poems_) {
        const auto cps = Utf8::toCodePoints(item.text);
        uniqueChars.insert(cps.begin(), cps.end());
    }

    disturbCandidateChars_.assign(uniqueChars.begin(), uniqueChars.end());
    std::sort(disturbCandidateChars_.begin(), disturbCandidateChars_.end());
}

std::string LevelGenerator::modeToString(LevelMode mode) {
    switch (mode) {
    case LevelMode::Idiom:
        return "成语";
    case LevelMode::Mixed:
        return "混合";
    }
    return "未知模式";
}

std::string LevelGenerator::difficultyToString(LevelDifficulty difficulty) {
    switch (difficulty) {
    case LevelDifficulty::Easy:
        return "简单";
    case LevelDifficulty::Medium:
        return "中等";
    case LevelDifficulty::Hard:
        return "困难";
    }
    return "未知难度";
}

LevelGenerator::DifficultyProfile LevelGenerator::getProfile(
    LevelMode mode,
    LevelDifficulty difficulty
) const {
    DifficultyProfile p;

    if (mode == LevelMode::Idiom) {
        switch (difficulty) {
        case LevelDifficulty::Easy:
            p.idiomSeedCount = 2;
            p.poemSeedCount = 0;
            p.minA = 2;  p.maxA = 24;
            p.minB = 1;  p.maxB = 4;
            p.disturbMin = 0; p.disturbMax = 2;
            p.maxTotalChars = 22;
            p.maxAttempts = 500;
            p.baseHintLimit = 2;
            return p;
        case LevelDifficulty::Medium:
            p.idiomSeedCount = 3;
            p.poemSeedCount = 0;
            p.minA = 6;  p.maxA = 72;
            p.minB = 2;  p.maxB = 6;
            p.disturbMin = 1; p.disturbMax = 3;
            p.maxTotalChars = 22;
            p.maxAttempts = 600;
            p.baseHintLimit = 3;
            return p;
        case LevelDifficulty::Hard:
            p.idiomSeedCount = 4;
            p.poemSeedCount = 0;
            p.minA = 10; p.maxA = 160;
            p.minB = 3;  p.maxB = 10;
            p.disturbMin = 2; p.disturbMax = 4;
            p.maxTotalChars = 22;
            p.maxAttempts = 800;
            p.baseHintLimit = 4;
            return p;
        }
    }

    switch (difficulty) {
    case LevelDifficulty::Easy:
        p.idiomSeedCount = 1;
        p.poemSeedCount = 1;
        p.minA = 2;  p.maxA = 36;
        p.minB = 1;  p.maxB = 4;
        p.disturbMin = 0; p.disturbMax = 2;
        p.maxTotalChars = 22;
        p.maxAttempts = 500;
        p.baseHintLimit = 2;
        return p;
    case LevelDifficulty::Medium:
        p.idiomSeedCount = 2;
        p.poemSeedCount = 1;
        p.minA = 5;  p.maxA = 96;
        p.minB = 2;  p.maxB = 6;
        p.disturbMin = 1; p.disturbMax = 3;
        p.maxTotalChars = 22;
        p.maxAttempts = 700;
        p.baseHintLimit = 3;
        return p;
    case LevelDifficulty::Hard:
        p.idiomSeedCount = 2;
        p.poemSeedCount = 2;
        p.minA = 10; p.maxA = 180;
        p.minB = 3;  p.maxB = 8;
        p.disturbMin = 0; p.disturbMax = 2;
        p.maxTotalChars = 22;
        p.maxAttempts = 900;
        p.baseHintLimit = 4;
        return p;
    }

    return p;
}

int LevelGenerator::totalFreqCount(const CharFreq& freq) {
    int total = 0;
    for (const auto& [_, cnt] : freq) {
        total += cnt;
    }
    return total;
}

CharFreq LevelGenerator::mergeFreq(const CharFreq& a, const CharFreq& b) {
    CharFreq merged = a;
    for (const auto& [cp, cnt] : b) {
        merged[cp] += cnt;
    }
    return merged;
}

std::vector<const PhraseInfo*> LevelGenerator::sampleDistinct(
    const std::vector<const PhraseInfo*>& source,
    std::size_t count
) {
    if (count == 0) {
        return {};
    }

    if (source.size() < count) {
        return {};
    }

    std::vector<const PhraseInfo*> shuffled = source;
    std::shuffle(shuffled.begin(), shuffled.end(), rng_);

    std::vector<const PhraseInfo*> picked;
    std::unordered_set<std::string> usedText;

    for (const auto* item : shuffled) {
        if (item == nullptr) {
            continue;
        }
        if (!usedText.insert(item->text).second) {
            continue;
        }

        picked.push_back(item);
        if (picked.size() >= count) {
            break;
        }
    }

    if (picked.size() < count) {
        return {};
    }

    return picked;
}

std::vector<std::string> LevelGenerator::buildDisturbChars(
    const CharFreq& baseFreq,
    int wantCount,
    int maxTotalChars
) const {
    std::vector<std::string> disturbChars;
    if (wantCount <= 0) {
        return disturbChars;
    }

    const int baseCount = totalFreqCount(baseFreq);
    const int room = std::max(0, maxTotalChars - baseCount);
    const int finalWant = std::min(wantCount, room);
    if (finalWant <= 0) {
        return disturbChars;
    }

    std::unordered_set<CodePoint> usedChars;
    for (const auto& [cp, _] : baseFreq) {
        usedChars.insert(cp);
    }

    std::vector<CodePoint> shuffled = disturbCandidateChars_;
    static thread_local std::mt19937 localRng(std::random_device{}());
    std::shuffle(shuffled.begin(), shuffled.end(), localRng);

    for (const auto cp : shuffled) {
        if (usedChars.find(cp) != usedChars.end()) {
            continue;
        }

        disturbChars.push_back(Utf8::fromCodePoint(cp));
        usedChars.insert(cp);

        if (static_cast<int>(disturbChars.size()) >= finalWant) {
            break;
        }
    }

    return disturbChars;
}

int LevelGenerator::calcPenalty(int value, int minValue, int maxValue) {
    if (value < minValue) {
        return minValue - value;
    }
    if (value > maxValue) {
        return value - maxValue;
    }
    return 0;
}

bool LevelGenerator::build(
    LevelMode mode,
    LevelDifficulty difficulty,
    GameRound& round,
    LevelBuildResult& outResult
) {
    const DifficultyProfile profile = getProfile(mode, difficulty);

    CandidateQueryOptions options;
    if (mode == LevelMode::Idiom) {
        options.minLen = 4;
        options.maxLen = 4;
        options.allowPoem = false;
        options.allowIdiom = true;
    } else {
        options.minLen = 4;
        options.maxLen = 7;
        options.allowPoem = true;
        options.allowIdiom = true;
    }

    int bestPenalty = std::numeric_limits<int>::max();
    LevelBuildResult bestFallback;
    bool hasFallback = false;

    for (int attempt = 0; attempt < profile.maxAttempts; ++attempt) {
        std::vector<const PhraseInfo*> pickedIdioms = sampleDistinct(idiomPool_, profile.idiomSeedCount);
        std::vector<const PhraseInfo*> pickedPoems = sampleDistinct(poemPool_, profile.poemSeedCount);

        if (pickedIdioms.size() != profile.idiomSeedCount ||
            pickedPoems.size() != profile.poemSeedCount) {
            return false;
        }

        GamePool pool;
        std::vector<std::string> seedIds;

        for (const auto* item : pickedIdioms) {
            pool.originFreq = mergeFreq(pool.originFreq, item->charFreq);
            seedIds.push_back(item->id);
        }
        for (const auto* item : pickedPoems) {
            pool.originFreq = mergeFreq(pool.originFreq, item->charFreq);
            seedIds.push_back(item->id);
        }

        if (totalFreqCount(pool.originFreq) > profile.maxTotalChars) {
            continue;
        }

        std::uniform_int_distribution<int> disturbDist(profile.disturbMin, profile.disturbMax);
        const int wantDisturbCount = disturbDist(rng_);
        const std::vector<std::string> disturbChars =
            buildDisturbChars(pool.originFreq, wantDisturbCount, profile.maxTotalChars);

        CharFreq disturbFreq;
        for (const auto& ch : disturbChars) {
            const auto cps = Utf8::toCodePoints(ch);
            if (!cps.empty()) {
                disturbFreq[cps.front()] += 1;
            }
        }

        pool.disturbFreq = disturbFreq;
        pool.originFreq = mergeFreq(pool.originFreq, disturbFreq);
        pool.remainFreq = pool.originFreq;
        pool.totalChars = totalFreqCount(pool.originFreq);

        if (pool.totalChars > profile.maxTotalChars) {
            continue;
        }

        round.createGameRound(pool, options);

        const int A = round.state().candidateCount;
        const int B = round.state().optimalMax;

        const int penalty =
            calcPenalty(A, profile.minA, profile.maxA) * 3 +
            calcPenalty(B, profile.minB, profile.maxB) * 5 +
            std::max(0, pool.totalChars - profile.maxTotalChars) * 1000;

        LevelBuildResult current;
        current.pool = pool;
        current.options = options;
        current.mode = mode;
        current.difficulty = difficulty;
        current.seedIds = std::move(seedIds);
        current.disturbChars = disturbChars;
        current.candidateCount = A;
        current.optimalMax = B;
        current.exactMatch = (penalty == 0);
        current.hintLimit = std::min(5, profile.baseHintLimit + (B >= 5 ? 1 : 0));

        std::ostringstream oss;
        oss << modeToString(mode) << " / " << difficultyToString(difficulty)
            << " | 字数=" << pool.totalChars
            << " | A=" << A
            << " | B=" << B
            << " | 干扰字=" << current.disturbChars.size()
            << " | 提示=" << current.hintLimit;
        current.summary = oss.str();

        if (current.exactMatch) {
            outResult = std::move(current);
            return true;
        }

        if (!hasFallback || penalty < bestPenalty) {
            bestPenalty = penalty;
            bestFallback = std::move(current);
            hasFallback = true;
        }
    }

    if (hasFallback) {
        round.createGameRound(bestFallback.pool, bestFallback.options);
        bestFallback.candidateCount = round.state().candidateCount;
        bestFallback.optimalMax = round.state().optimalMax;
        bestFallback.exactMatch = false;
        bestFallback.hintLimit = std::min(5, profile.baseHintLimit + (bestFallback.optimalMax >= 5 ? 1 : 0));
        bestFallback.summary += " | 使用最接近目标的关卡";
        outResult = std::move(bestFallback);
        return true;
    }

    return false;
}

} // namespace lineverse::poetryrebuild