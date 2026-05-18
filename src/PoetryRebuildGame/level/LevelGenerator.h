#pragma once

#include <random>
#include <string>
#include <unordered_set>
#include <vector>

#include "../common/Types.h"
#include "../game/GameRound.h"
#include "../index/IndexStore.h"
#include "../model/PhraseInfo.h"

namespace lineverse::poetryrebuild {

enum class LevelMode {
    Idiom,
    Mixed
};

enum class LevelDifficulty {
    Easy,
    Medium,
    Hard
};

struct LevelBuildResult {
    GamePool pool;
    CandidateQueryOptions options;

    LevelMode mode = LevelMode::Idiom;
    LevelDifficulty difficulty = LevelDifficulty::Easy;

    std::vector<std::string> seedIds;
    std::vector<std::string> disturbChars;

    int candidateCount = 0;   // A
    int optimalMax = 0;       // B
    bool exactMatch = false;

    int hintLimit = 0;

    std::string summary;
};

class LevelGenerator {
public:
    LevelGenerator(
        const IndexStore& indexStore,
        const std::vector<PhraseInfo>& poems,
        const std::vector<PhraseInfo>& idioms
    );

    bool build(
        LevelMode mode,
        LevelDifficulty difficulty,
        GameRound& round,
        LevelBuildResult& outResult
    );

    static std::string modeToString(LevelMode mode);
    static std::string difficultyToString(LevelDifficulty difficulty);

private:
    struct DifficultyProfile {
        std::size_t idiomSeedCount = 0;
        std::size_t poemSeedCount = 0;

        int minA = 0;
        int maxA = 999999;
        int minB = 0;
        int maxB = 999999;

        int disturbMin = 0;
        int disturbMax = 0;

        int maxTotalChars = 22;
        int maxAttempts = 500;

        int baseHintLimit = 0;
    };

    const IndexStore& indexStore_;
    const std::vector<PhraseInfo>& poems_;
    const std::vector<PhraseInfo>& idioms_;
    std::vector<const PhraseInfo*> idiomPool_;
    std::vector<const PhraseInfo*> poemPool_;
    std::vector<CodePoint> disturbCandidateChars_;
    std::mt19937 rng_;

    DifficultyProfile getProfile(LevelMode mode, LevelDifficulty difficulty) const;

    static int totalFreqCount(const CharFreq& freq);
    static CharFreq mergeFreq(const CharFreq& a, const CharFreq& b);

    std::vector<const PhraseInfo*> sampleDistinct(
        const std::vector<const PhraseInfo*>& source,
        std::size_t count
    );

    std::vector<std::string> buildDisturbChars(
        const CharFreq& baseFreq,
        int wantCount,
        int maxTotalChars
    ) const;

    static int calcPenalty(int value, int minValue, int maxValue);
};

} // namespace lineverse::poetryrebuild