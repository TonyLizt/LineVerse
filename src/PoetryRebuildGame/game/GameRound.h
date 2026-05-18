#pragma once

#include <limits>
#include <string>
#include <unordered_set>
#include <vector>

#include "../candidate/CandidateEngine.h"
#include "../common/Types.h"
#include "../index/IndexStore.h"
#include "../preprocess/TextNormalizer.h"
#include "../solver/OptimalSolver.h"

namespace lineverse::poetryrebuild {

struct GamePool {
    CharFreq originFreq;
    CharFreq remainFreq;
    CharFreq disturbFreq;
    int totalChars = 0;
};

struct GameState {
    std::unordered_set<std::string> foundSet;
    int score = 0;
    int combo = 0;
    int maxCombo = 0;
    double comboMultiplier = 1.0;
    long long lastSuccessAtMs = -1;
    int fastAnswerCount = 0;

    int timeLeft = 60;
    int candidateCount = 0;   // A
    int optimalMax = 0;       // B
    std::vector<std::string> submitHistory;
    GamePool pool;
};

enum class SubmitStatus {
    Success,
    EmptyAnswer,
    NotInLexicon,
    DuplicateAnswer,
    NotEnoughChars,
    RoundNotStarted
};

struct SubmitResult {
    SubmitStatus status = SubmitStatus::RoundNotStarted;
    bool accepted = false;
    std::string normalizedText;
    std::string answerId;
    int gainedScore = 0;
    int scoreAfterSubmit = 0;
    int comboAfterSubmit = 0;
    int candidateCountAfterSubmit = 0;
    int optimalMaxAfterSubmit = 0;
    std::string message;
};

struct GameRoundConfig {
    int initialTime = 60;
    bool recomputeCandidatesOnSubmit = true;
    bool recomputeOptimalOnSubmit = true;
    bool resetComboOnFailedSubmit = true;

    int poemBonus = 5;
    int rarityBonusUnit = 5;

    int comboWindowSeconds = 8;
    int speedBonusFast = 15;   // <= 2s
    int speedBonusMid = 8;     // <= 4s
    int speedBonusSlow = 3;    // <= comboWindow
};

class GameRound {
public:
    using CandidateIdList = std::vector<std::string>;

    GameRound(
        const IndexStore& indexStore,
        const CandidateEngine& candidateEngine,
        const OptimalSolver& optimalSolver,
        GameRoundConfig config = {}
    );

    void createGameRound(const GamePool& pool, const CandidateQueryOptions& options = {});
    SubmitResult submitAnswer(const std::string& rawInput);

    bool consumeChars(const CharFreq& need);
    int updateScore(const PhraseInfo& phrase, long long nowMs);

    bool isGameOver() const;
    int calcGapToOptimal() const;

    void setTimeLeft(int seconds);

    const GameState& state() const;
    const std::vector<std::string>& getBestSolutionSet() const;

    CandidateIdList getRemainingCandidates() const;
    std::vector<std::string> getRemainingCandidateTexts(std::size_t limit = 50) const;

    // ===== hint first 状态 =====
    bool hasActiveFirstHintTarget() const;
    const std::string& activeFirstHintTargetId() const;
    int activeFirstHintRevealCount() const;
    void setActiveFirstHintProgress(const std::string& id, int revealCount);
    void clearActiveFirstHint();
    bool canStillComposeAnswerId(const std::string& id) const;

    double currentComboMultiplier() const;

private:
    const IndexStore& indexStore_;
    const CandidateEngine& candidateEngine_;
    const OptimalSolver& optimalSolver_;
    GameRoundConfig config_;
    TextNormalizer normalizer_;

    GameState state_;
    CandidateQueryOptions candidateOptions_;
    std::vector<std::string> bestSolutionSet_;
    bool roundStarted_ = false;

    std::string activeFirstHintTargetId_;
    int activeFirstHintRevealCount_ = 0;

    void recomputeRoundMetrics();
    static int calcTotalChars(const CharFreq& freq);
    static int calcTotalChars(const GamePool& pool);

    double calcComboMultiplier(int combo) const;
    static long long currentTimeMs();
};

} // namespace lineverse::poetryrebuild