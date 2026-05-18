#include "GameRound.h"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace lineverse::poetryrebuild {

GameRound::GameRound(
    const IndexStore& indexStore,
    const CandidateEngine& candidateEngine,
    const OptimalSolver& optimalSolver,
    GameRoundConfig config
)
    : indexStore_(indexStore),
      candidateEngine_(candidateEngine),
      optimalSolver_(optimalSolver),
      config_(std::move(config)) {
}

void GameRound::createGameRound(const GamePool& pool, const CandidateQueryOptions& options) {
    state_ = GameState{};
    state_.timeLeft = config_.initialTime;
    state_.pool = pool;
    candidateOptions_ = options;
    bestSolutionSet_.clear();

    activeFirstHintTargetId_.clear();
    activeFirstHintRevealCount_ = 0;

    if (state_.pool.remainFreq.empty()) {
        state_.pool.remainFreq = state_.pool.originFreq;
    }

    if (state_.pool.totalChars <= 0) {
        state_.pool.totalChars = calcTotalChars(state_.pool);
    }

    roundStarted_ = true;
    recomputeRoundMetrics();
}

SubmitResult GameRound::submitAnswer(const std::string& rawInput) {
    SubmitResult result;
    result.normalizedText = normalizer_.normalizeText(rawInput);

    if (!roundStarted_) {
        result.status = SubmitStatus::RoundNotStarted;
        result.message = "当前没有进行中的对局";
        return result;
    }

    if (result.normalizedText.empty()) {
        result.status = SubmitStatus::EmptyAnswer;
        result.message = "提交为空答案";
        if (config_.resetComboOnFailedSubmit) {
            state_.combo = std::max(0, state_.combo - 1);
            state_.comboMultiplier = calcComboMultiplier(state_.combo);
        }
        result.comboAfterSubmit = state_.combo;
        result.scoreAfterSubmit = state_.score;
        result.candidateCountAfterSubmit = state_.candidateCount;
        result.optimalMaxAfterSubmit = state_.optimalMax;
        return result;
    }

    const std::string* idPtr = indexStore_.getIdByText(result.normalizedText);
    if (idPtr == nullptr) {
        result.status = SubmitStatus::NotInLexicon;
        result.message = "答案不在词库中";
        if (config_.resetComboOnFailedSubmit) {
            state_.combo = std::max(0, state_.combo - 1);
            state_.comboMultiplier = calcComboMultiplier(state_.combo);
        }
        result.comboAfterSubmit = state_.combo;
        result.scoreAfterSubmit = state_.score;
        result.candidateCountAfterSubmit = state_.candidateCount;
        result.optimalMaxAfterSubmit = state_.optimalMax;
        return result;
    }

    result.answerId = *idPtr;

    if (state_.foundSet.find(result.normalizedText) != state_.foundSet.end()) {
        result.status = SubmitStatus::DuplicateAnswer;
        result.message = "答案已重复提交";
        result.comboAfterSubmit = state_.combo;
        result.scoreAfterSubmit = state_.score;
        result.candidateCountAfterSubmit = state_.candidateCount;
        result.optimalMaxAfterSubmit = state_.optimalMax;
        return result;
    }

    const PhraseInfo* phrase = indexStore_.getPhraseById(result.answerId);
    if (phrase == nullptr) {
        result.status = SubmitStatus::NotInLexicon;
        result.message = "答案存在于 textToId，但找不到对应条目";
        if (config_.resetComboOnFailedSubmit) {
            state_.combo = std::max(0, state_.combo - 1);
            state_.comboMultiplier = calcComboMultiplier(state_.combo);
        }
        result.comboAfterSubmit = state_.combo;
        result.scoreAfterSubmit = state_.score;
        result.candidateCountAfterSubmit = state_.candidateCount;
        result.optimalMaxAfterSubmit = state_.optimalMax;
        return result;
    }

    if (!consumeChars(phrase->charFreq)) {
        result.status = SubmitStatus::NotEnoughChars;
        result.message = "剩余字池不足";
        if (config_.resetComboOnFailedSubmit) {
            state_.combo = 0;
            state_.comboMultiplier = 1.0;
        }
        result.comboAfterSubmit = state_.combo;
        result.scoreAfterSubmit = state_.score;
        result.candidateCountAfterSubmit = state_.candidateCount;
        result.optimalMaxAfterSubmit = state_.optimalMax;
        return result;
    }

    const long long submitMs = currentTimeMs();
    result.gainedScore = updateScore(*phrase, submitMs);

    state_.foundSet.insert(result.normalizedText);
    state_.submitHistory.push_back(result.normalizedText);

    if (hasActiveFirstHintTarget()) {
        if (activeFirstHintTargetId_ == result.answerId || !canStillComposeAnswerId(activeFirstHintTargetId_)) {
            clearActiveFirstHint();
        }
    }

    if (config_.recomputeCandidatesOnSubmit || config_.recomputeOptimalOnSubmit) {
        recomputeRoundMetrics();
    }

    if (hasActiveFirstHintTarget() && !canStillComposeAnswerId(activeFirstHintTargetId_)) {
        clearActiveFirstHint();
    }

    result.status = SubmitStatus::Success;
    result.accepted = true;
    result.message = "提交成功";
    result.scoreAfterSubmit = state_.score;
    result.comboAfterSubmit = state_.combo;
    result.candidateCountAfterSubmit = state_.candidateCount;
    result.optimalMaxAfterSubmit = state_.optimalMax;
    return result;
}

bool GameRound::consumeChars(const CharFreq& need) {
    CandidateEngine checker(indexStore_);
    if (!checker.canCompose(need, state_.pool.remainFreq)) {
        return false;
    }

    for (const auto& [cp, cnt] : need) {
        auto it = state_.pool.remainFreq.find(cp);
        if (it == state_.pool.remainFreq.end()) {
            return false;
        }

        it->second -= cnt;
        if (it->second <= 0) {
            state_.pool.remainFreq.erase(it);
        }
    }

    return true;
}

int GameRound::updateScore(const PhraseInfo& phrase, long long nowMs) {
    const long long comboWindowMs = static_cast<long long>(config_.comboWindowSeconds) * 1000LL;
    const long long deltaMs =
        (state_.lastSuccessAtMs >= 0) ? (nowMs - state_.lastSuccessAtMs) : std::numeric_limits<long long>::max();

    if (state_.lastSuccessAtMs >= 0 && deltaMs <= comboWindowMs) {
        state_.combo += 1;
    } else {
        state_.combo = 1;
    }

    state_.comboMultiplier = calcComboMultiplier(state_.combo);
    state_.maxCombo = std::max(state_.maxCombo, state_.combo);

    int baseScore = phrase.length * 10;

    if (phrase.type == PhraseType::Poem) {
        baseScore += config_.poemBonus;
    }

    baseScore += std::max(0, phrase.rarity - 1) * config_.rarityBonusUnit;

    int speedBonus = 0;
    if (state_.lastSuccessAtMs >= 0) {
        if (deltaMs <= 2000) {
            speedBonus = config_.speedBonusFast;
            state_.fastAnswerCount += 1;
        } else if (deltaMs <= 4000) {
            speedBonus = config_.speedBonusMid;
        } else if (deltaMs <= comboWindowMs) {
            speedBonus = config_.speedBonusSlow;
        }
    }

    const int gained = static_cast<int>(std::round(baseScore * state_.comboMultiplier)) + speedBonus;
    state_.score += gained;
    state_.lastSuccessAtMs = nowMs;
    return gained;
}

bool GameRound::isGameOver() const {
    if (!roundStarted_) {
        return true;
    }

    if (state_.timeLeft <= 0) {
        return true;
    }

    if (state_.candidateCount <= 0) {
        return true;
    }

    return false;
}

int GameRound::calcGapToOptimal() const {
    const int foundCount = static_cast<int>(state_.foundSet.size());
    return std::max(0, state_.optimalMax - foundCount);
}

void GameRound::setTimeLeft(int seconds) {
    state_.timeLeft = std::max(0, seconds);
}

const GameState& GameRound::state() const {
    return state_;
}

const std::vector<std::string>& GameRound::getBestSolutionSet() const {
    return bestSolutionSet_;
}

GameRound::CandidateIdList GameRound::getRemainingCandidates() const {
    if (!roundStarted_) {
        return {};
    }
    return candidateEngine_.getCandidates(state_.pool.remainFreq, candidateOptions_);
}

std::vector<std::string> GameRound::getRemainingCandidateTexts(std::size_t limit) const {
    const CandidateIdList ids = getRemainingCandidates();
    std::vector<std::string> texts;
    texts.reserve(std::min(limit, ids.size()));

    for (std::size_t i = 0; i < ids.size() && i < limit; ++i) {
        const PhraseInfo* phrase = indexStore_.getPhraseById(ids[i]);
        if (phrase != nullptr) {
            texts.push_back(phrase->text);
        }
    }
    return texts;
}

bool GameRound::hasActiveFirstHintTarget() const {
    return !activeFirstHintTargetId_.empty();
}

const std::string& GameRound::activeFirstHintTargetId() const {
    return activeFirstHintTargetId_;
}

int GameRound::activeFirstHintRevealCount() const {
    return activeFirstHintRevealCount_;
}

void GameRound::setActiveFirstHintProgress(const std::string& id, int revealCount) {
    activeFirstHintTargetId_ = id;
    activeFirstHintRevealCount_ = std::max(0, revealCount);
}

void GameRound::clearActiveFirstHint() {
    activeFirstHintTargetId_.clear();
    activeFirstHintRevealCount_ = 0;
}

bool GameRound::canStillComposeAnswerId(const std::string& id) const {
    const PhraseInfo* phrase = indexStore_.getPhraseById(id);
    if (phrase == nullptr) {
        return false;
    }

    if (state_.foundSet.find(phrase->text) != state_.foundSet.end()) {
        return false;
    }

    CandidateEngine checker(indexStore_);
    return checker.canCompose(phrase->charFreq, state_.pool.remainFreq);
}

double GameRound::currentComboMultiplier() const {
    return state_.comboMultiplier;
}

void GameRound::recomputeRoundMetrics() {
    const CandidateIdList remainingCandidates = candidateEngine_.getCandidates(
        state_.pool.remainFreq,
        candidateOptions_
    );
    state_.candidateCount = static_cast<int>(remainingCandidates.size());

    if (config_.recomputeOptimalOnSubmit) {
        const SolveResult solve = optimalSolver_.searchOptimalMax(remainingCandidates, state_.pool.remainFreq);
        state_.optimalMax = solve.optimalMax;
        bestSolutionSet_ = solve.bestSet;
    }
}

int GameRound::calcTotalChars(const CharFreq& freq) {
    int total = 0;
    for (const auto& [_, cnt] : freq) {
        total += cnt;
    }
    return total;
}

int GameRound::calcTotalChars(const GamePool& pool) {
    return calcTotalChars(pool.originFreq) + calcTotalChars(pool.disturbFreq);
}

double GameRound::calcComboMultiplier(int combo) const {
    if (combo <= 1) {
        return 1.0;
    }
    if (combo <= 3) {
        return 1.10;
    }
    if (combo <= 5) {
        return 1.25;
    }
    if (combo <= 8) {
        return 1.40;
    }
    return 1.55;
}

long long GameRound::currentTimeMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

} // namespace lineverse::poetryrebuild