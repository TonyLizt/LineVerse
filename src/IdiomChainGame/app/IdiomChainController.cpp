#include "IdiomChainController.h"

#include "../core/HintEngine.h"
#include "../core/IdiomGraph.h"
#include "../core/ModeStrategy.h"
#include "../core/ScoreCalculator.h"
#include "../data/IIdiomRepository.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <cctype>
#include <random>
#include <stdexcept>
#include <unordered_set>

namespace {

/**
 * @brief Convert game mode to persistent text.
 * @param mode Game mode.
 * @return Mode text.
 */
std::string modeToText(GameMode mode) {
    switch (mode) {
    case GameMode::SingleEasy:
        return "Easy";
    case GameMode::SingleMedium:
        return "Medium";
    case GameMode::SingleHard:
        return "Hard";
    case GameMode::BattleEasy:
        return "BattleEasy";
    case GameMode::BattleMedium:
        return "BattleMedium";
    case GameMode::BattleHard:
        return "BattleHard";
    }
    return "Unknown";
}

/**
 * @brief Convert ASCII letters to lowercase.
 * @param text Input text.
 * @return Lowercased text.
 */
std::string toLowerAscii(std::string text) {
    for (char& ch : text) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return text;
}

/**
 * @brief Check whether haystack contains needle using ASCII-lower comparison.
 * @param haystack Full text.
 * @param needle Query text.
 * @return True when contained.
 */
bool containsInsensitive(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) {
        return true;
    }
    const std::string loweredHaystack = toLowerAscii(haystack);
    const std::string loweredNeedle = toLowerAscii(needle);
    return loweredHaystack.find(loweredNeedle) != std::string::npos;
}

/**
 * @brief Lightweight timer holder for one active round.
 */
class TimerHolder {
public:
    /**
     * @brief Reset start point to current time.
     */
    void reset() {
        start_ = std::chrono::steady_clock::now();
    }

    /**
     * @brief Get elapsed seconds since the last reset.
     * @return Elapsed time in seconds.
     */
    double elapsedSeconds() const {
        const auto diff = std::chrono::steady_clock::now() - start_;
        return std::chrono::duration_cast<std::chrono::duration<double>>(diff).count();
    }

private:
    std::chrono::steady_clock::time_point start_ { std::chrono::steady_clock::now() };
};

TimerHolder g_timer;

} // namespace

IdiomChainController::IdiomChainController(
    const IIdiomRepository& repository,
    const IdiomGraph& graph,
    const PathSolver& solver,
    const HintEngine& hintEngine,
    RecordRepository& recordRepository)
    : repository_(repository)
    , graph_(graph)
    , solver_(solver)
    , hintEngine_(hintEngine)
    , recordRepository_(recordRepository) {
}

void IdiomChainController::startSingleGame(GameMode mode, const std::string& playerName) {
    session_ = GameSession{};
    session_.playerName = playerName;
    session_.mode = mode;

    std::unique_ptr<IModeStrategy> strategy;
    if (mode == GameMode::SingleEasy) {
        strategy = std::make_unique<EasyModeStrategy>();
    } else if (mode == GameMode::SingleMedium) {
        strategy = std::make_unique<MediumModeStrategy>();
    } else {
        strategy = std::make_unique<HardModeStrategy>();
    }

    prepareSession(*strategy);
    refreshMediumOptions();
    g_timer.reset();
    pendingRecordFlush_ = false;
}

void IdiomChainController::tick() {
    if (!session_.finished) {
        updateElapsedSeconds();
    }
}

const GameSession& IdiomChainController::getSession() const {
    return session_;
}

std::vector<int> IdiomChainController::getEasyPool() const {
    return session_.easyPool;
}

std::vector<std::string> IdiomChainController::getEasyPoolWords() const {
    std::vector<std::string> words;
    words.reserve(session_.easyPool.size());
    for (int idiomId : session_.easyPool) {
        words.push_back(wordOf(idiomId));
    }
    return words;
}

bool IdiomChainController::submitEasyOrder(const std::vector<int>& orderedPoolIndexes) {
    if (session_.mode != GameMode::SingleEasy) {
        return false;
    }

    updateElapsedSeconds();
    if (session_.finished) {
        return false;
    }

    std::vector<int> candidatePath;
    std::unordered_set<int> used;
    for (int poolIndex : orderedPoolIndexes) {
        if (poolIndex < 0 || static_cast<std::size_t>(poolIndex) >= session_.easyPool.size()) {
            lastMessage_ = "Index out of range.";
            return false;
        }
        const int idiomId = session_.easyPool[static_cast<std::size_t>(poolIndex)];
        if (!used.insert(idiomId).second) {
            lastMessage_ = "Duplicate indices are not allowed.";
            return false;
        }
        candidatePath.push_back(idiomId);
    }

    if (candidatePath.empty()
        || candidatePath.front() != session_.startId
        || candidatePath.back() != session_.targetId) {
        lastMessage_ = "Path must start with the start idiom and end with the target idiom.";
        return false;
    }

    bool valid = true;
    for (std::size_t index = 1; index < candidatePath.size(); ++index) {
        const std::vector<int>& nextIds = graph_.getNextIds(candidatePath[index - 1]);
        if (std::find(nextIds.begin(), nextIds.end(), candidatePath[index]) == nextIds.end()) {
            valid = false;
            break;
        }
    }

    if (!valid) {
        lastMessage_ = "Selected order does not form a valid chain.";
        return false;
    }

    session_.playerPath = candidatePath;
    session_.stepCount = static_cast<int>(candidatePath.size()) - 1;
    session_.finished = true;
    session_.success = true;

    const ScoreCalculator calculator;
    session_.score = calculator.calculate(session_);
    saveRecordIfNeeded();
    lastMessage_ = "Easy mode solved.";
    return true;
}

std::vector<int> IdiomChainController::getMediumOptions() const {
    return computeMediumOptionsInternal();
}

std::vector<std::string> IdiomChainController::getMediumOptionWords() const {
    std::vector<std::string> words;
    const std::vector<int>& optionIds = session_.mediumOptions;
    words.reserve(optionIds.size());
    for (int idiomId : optionIds) {
        words.push_back(wordOf(idiomId));
    }
    return words;
}

std::vector<int> IdiomChainController::getMediumOptionDistances() const {
    std::vector<int> distances;
    const std::vector<int>& optionIds = session_.mediumOptions;
    distances.reserve(optionIds.size());
    for (int idiomId : optionIds) {
        if (idiomId >= 0 && static_cast<std::size_t>(idiomId) < session_.distanceToTarget.size()) {
            distances.push_back(session_.distanceToTarget[static_cast<std::size_t>(idiomId)]);
        } else {
            distances.push_back(-1);
        }
    }
    return distances;
}

bool IdiomChainController::submitMediumChoice(int optionIndex) {
    if (session_.mode != GameMode::SingleMedium) {
        return false;
    }

    updateElapsedSeconds();
    if (session_.finished) {
        return false;
    }

    const std::vector<int>& options = session_.mediumOptions;
    if (options.empty()) {
        session_.finished = true;
        session_.success = false;
        lastMessage_ = "No available reachable options. This round is lost.";
        saveRecordIfNeeded();
        return false;
    }

    if (optionIndex < 0 || static_cast<std::size_t>(optionIndex) >= options.size()) {
        lastMessage_ = "Invalid option index.";
        return false;
    }

    const int selectedId = options[static_cast<std::size_t>(optionIndex)];
    if (!isInputValidNext(selectedId)) {
        return false;
    }

    session_.rollbackStack.push(session_.playerPath.back());
    session_.playerPath.push_back(selectedId);
    session_.stepCount += 1;

    finalizeIfTargetReached();
    if (!session_.finished) {
        refreshMediumOptions();
        lastMessage_ = "Choice accepted.";
    }
    return true;
}


bool IdiomChainController::submitMediumChoiceById(int idiomId) {
    if (session_.mode != GameMode::SingleMedium) {
        return false;
    }

    const std::vector<int>& options = session_.mediumOptions;
    auto it = std::find(options.begin(), options.end(), idiomId);
    if (it == options.end()) {
        lastMessage_ = "Selected idiom is not in current options.";
        return false;
    }

    return submitMediumChoice(static_cast<int>(std::distance(options.begin(), it)));
}

bool IdiomChainController::submitHardInput(const std::string& inputWordOrAbbreviation) {
    if (session_.mode != GameMode::SingleHard) {
        return false;
    }

    updateElapsedSeconds();
    if (session_.finished) {
        return false;
    }

    const int nextId = graph_.getIdByWord(inputWordOrAbbreviation);
    if (nextId < 0) {
        lastMessage_ = "Idiom not found (you may also type abbreviation).";
        return false;
    }
    if (!isInputValidNext(nextId)) {
        return false;
    }

    session_.rollbackStack.push(session_.playerPath.back());
    session_.playerPath.push_back(nextId);
    session_.stepCount += 1;

    finalizeIfTargetReached();
    if (!session_.finished) {
        lastMessage_ = "Step accepted.";
    }
    return true;
}

std::vector<std::string> IdiomChainController::getHardCandidateWords(const std::string& query) const {
    std::vector<std::string> words;
    if (session_.mode != GameMode::SingleHard || session_.playerPath.empty()) {
        return words;
    }

    const int currentId = session_.playerPath.back();
    const std::vector<int>& nextIds = graph_.getNextIds(currentId);
    std::unordered_set<int> used(session_.playerPath.begin(), session_.playerPath.end());

    struct CandidateInfo {
        int idiomId { -1 };
        int familiarityScore { 0 };
        int distanceToTarget { -1 };
        int queryScore { 0 };
    };

    const std::string loweredQuery = toLowerAscii(query);
    std::vector<CandidateInfo> candidates;

    for (int nextId : nextIds) {
        if (used.find(nextId) != used.end()) {
            continue;
        }

        const IdiomEntry* entry = graph_.getEntry(nextId);
        if (entry == nullptr) {
            continue;
        }

        if (nextId >= 0 && static_cast<std::size_t>(nextId) < session_.distanceToTarget.size()) {
            if (session_.distanceToTarget[static_cast<std::size_t>(nextId)] < 0) {
                continue;
            }
        }

        int queryScore = 0;
        if (!loweredQuery.empty()) {
            if (containsInsensitive(entry->word, loweredQuery)) {
                queryScore = 4;
            } else if (!entry->abbreviation.empty() && toLowerAscii(entry->abbreviation).rfind(loweredQuery, 0) == 0U) {
                queryScore = 3;
            } else if (!entry->pinyinRaw.empty() && containsInsensitive(entry->pinyinRaw, loweredQuery)) {
                queryScore = 2;
            } else if (!entry->pinyin.empty() && containsInsensitive(entry->pinyin, loweredQuery)) {
                queryScore = 1;
            } else {
                continue;
            }
        }

        const int distance = (nextId >= 0 && static_cast<std::size_t>(nextId) < session_.distanceToTarget.size())
                                 ? session_.distanceToTarget[static_cast<std::size_t>(nextId)]
                                 : -1;

        candidates.push_back(CandidateInfo{nextId, entry->familiarityScore, distance, queryScore});
    }

    std::sort(candidates.begin(), candidates.end(), [&](const CandidateInfo& lhs, const CandidateInfo& rhs) {
        if (lhs.queryScore != rhs.queryScore) {
            return lhs.queryScore > rhs.queryScore;
        }
        if (lhs.distanceToTarget != rhs.distanceToTarget) {
            return lhs.distanceToTarget < rhs.distanceToTarget;
        }
        if (lhs.familiarityScore != rhs.familiarityScore) {
            return lhs.familiarityScore > rhs.familiarityScore;
        }
        return wordOf(lhs.idiomId) < wordOf(rhs.idiomId);
    });

    const std::size_t limit = std::min<std::size_t>(12U, candidates.size());
    words.reserve(limit);
    for (std::size_t index = 0; index < limit; ++index) {
        words.push_back(wordOf(candidates[index].idiomId));
    }

    std::mt19937 randomEngine(static_cast<unsigned int>(std::random_device{}()));
    std::shuffle(words.begin(), words.end(), randomEngine);
    return words;
}

bool IdiomChainController::rollbackOneStep() {
    if (session_.mode == GameMode::SingleEasy) {
        lastMessage_ = "Easy mode does not support rollback.";
        return false;
    }
    if (session_.playerPath.size() <= 1U) {
        lastMessage_ = "Nothing to rollback.";
        return false;
    }

    session_.playerPath.pop_back();
    if (!session_.rollbackStack.empty()) {
        session_.rollbackStack.pop();
    }
    session_.rollbackCount += 1;
    session_.stepCount += 1;
    if (session_.mode == GameMode::SingleMedium) {
        refreshMediumOptions();
    }
    lastMessage_ = "Rolled back one step.";
    return true;
}

std::optional<std::string> IdiomChainController::requestHint() {
    if (session_.mode == GameMode::SingleEasy) {
        lastMessage_ = "Easy mode does not use hints.";
        return std::nullopt;
    }
    if (session_.mode == GameMode::SingleHard && session_.hintCount >= session_.maxHints) {
        lastMessage_ = "No hints remaining.";
        return std::nullopt;
    }

    const int currentId = session_.playerPath.back();
    const std::optional<int> nextId = hintEngine_.suggestNextStep(currentId, session_.targetId);
    if (!nextId.has_value()) {
        lastMessage_ = "No hint available.";
        return std::nullopt;
    }

    session_.hintCount += 1;
    if (session_.mode == GameMode::SingleHard) {
        session_.stepCount += 1;
    }

    lastMessage_ = "Hint provided.";
    return wordOf(*nextId);
}

PathResult IdiomChainController::revealAnswer() const {
    return solver_.solveShortestPath(session_.startId, session_.targetId);
}

const std::string& IdiomChainController::getLastMessage() const {
    return lastMessage_;
}

void IdiomChainController::flushPendingRecord() {
    if (pendingRecordFlush_) {
        pendingRecordFlush_ = false;
    }
}

std::string IdiomChainController::wordOf(int idiomId) const {
    const IdiomEntry* entry = graph_.getEntry(idiomId);
    return entry != nullptr ? entry->word : std::string();
}

std::string IdiomChainController::explanationOf(int idiomId) const {
    const IdiomEntry* entry = graph_.getEntry(idiomId);
    return entry != nullptr ? entry->explanation : std::string();
}

std::vector<std::string> IdiomChainController::bestPathWords() const {
    std::vector<std::string> words;
    words.reserve(session_.bestPath.size());
    for (int idiomId : session_.bestPath) {
        words.push_back(wordOf(idiomId));
    }
    return words;
}

std::vector<std::string> IdiomChainController::currentPathExplanations() const {
    std::vector<std::string> explanations;
    explanations.reserve(session_.playerPath.size());
    for (int idiomId : session_.playerPath) {
        const IdiomEntry* entry = graph_.getEntry(idiomId);
        if (entry == nullptr) {
            continue;
        }
        std::string line = entry->word + "：";
        if (!entry->explanation.empty()) {
            line += entry->explanation;
        } else if (!entry->derivation.empty()) {
            line += entry->derivation;
        } else {
            line += "（暂无释义）";
        }
        explanations.push_back(line);
    }
    return explanations;
}

std::priority_queue<RankItem> IdiomChainController::buildLeaderboard() const {
    return recordRepository_.buildLeaderboard();
}

std::vector<GameRecord> IdiomChainController::loadAllRecords() const {
    return recordRepository_.loadRecords();
}

void IdiomChainController::prepareSession(IModeStrategy& strategy) {
    strategy.prepareQuestion(session_, graph_, solver_);

    if (session_.startId < 0 || session_.targetId < 0) {
        throw std::runtime_error("Failed to create a reachable question.");
    }

    const PathResult best = solver_.solveShortestPath(session_.startId, session_.targetId);
    if (!best.reachable) {
        throw std::runtime_error("Internal error: generated question is unreachable.");
    }

    session_.bestPath = best.path;
    session_.bestStepCount = best.stepCount;
    session_.distanceToTarget = solver_.computeDistanceToTarget(session_.targetId);
    session_.playerPath = { session_.startId };
    session_.rollbackStack = std::stack<int>{};
    session_.stepCount = 0;
    session_.finished = false;
    session_.success = false;
    session_.score = 0;
    lastMessage_ = "Round ready. Reach the target with exact tone-matching pinyin chaining.";

    if (session_.mode == GameMode::SingleEasy) {
        session_.easyPool = best.path;

        std::vector<int> distractors;
        const std::vector<int>& startNeighbors = graph_.getNextIds(session_.startId);
        for (int neighborId : startNeighbors) {
            if (std::find(best.path.begin(), best.path.end(), neighborId) == best.path.end()) {
                distractors.push_back(neighborId);
            }
            if (distractors.size() >= 2U) {
                break;
            }
        }
        session_.easyPool.insert(session_.easyPool.end(), distractors.begin(), distractors.end());

        std::mt19937 randomEngine(static_cast<unsigned int>(std::random_device{}()));
        std::shuffle(session_.easyPool.begin(), session_.easyPool.end(), randomEngine);
    }
}

void IdiomChainController::updateElapsedSeconds() {
    session_.elapsedSeconds = g_timer.elapsedSeconds();
    if (session_.timeLimitSeconds > 0
        && session_.elapsedSeconds > static_cast<double>(session_.timeLimitSeconds)) {
        session_.finished = true;
        session_.success = false;
        lastMessage_ = "Time limit exceeded.";
        saveRecordIfNeeded();
    }
}

void IdiomChainController::finalizeIfTargetReached() {
    if (!session_.playerPath.empty() && session_.playerPath.back() == session_.targetId) {
        session_.finished = true;
        session_.success = true;
        const ScoreCalculator calculator;
        session_.score = calculator.calculate(session_);
        lastMessage_ = "Target reached.";
        saveRecordIfNeeded();
        return;
    }

    if (session_.mode == GameMode::SingleMedium) {
        const std::vector<int>& options = session_.mediumOptions;
        if (options.empty()) {
            session_.finished = true;
            session_.success = false;
            lastMessage_ = "No remaining reachable options. This round is lost.";
            saveRecordIfNeeded();
        }
    }
}

void IdiomChainController::saveRecordIfNeeded() {
    if (!session_.finished || pendingRecordFlush_) {
        return;
    }

    const IdiomEntry* startEntry = graph_.getEntry(session_.startId);
    const IdiomEntry* targetEntry = graph_.getEntry(session_.targetId);
    if (startEntry == nullptr || targetEntry == nullptr) {
        return;
    }

    GameRecord record;
    record.playerName = session_.playerName;
    record.startWord = startEntry->word;
    record.targetWord = targetEntry->word;
    record.difficulty = modeToText(session_.mode);
    record.elapsedSeconds = session_.elapsedSeconds;
    record.pathLength = session_.stepCount;
    record.score = session_.score;
    recordRepository_.saveRecord(record);

    pendingRecordFlush_ = true;
}


void IdiomChainController::refreshMediumOptions() {
    if (session_.mode != GameMode::SingleMedium) {
        session_.mediumOptions.clear();
        return;
    }
    session_.mediumOptions = computeMediumOptionsInternal();
}

std::vector<int> IdiomChainController::computeMediumOptionsInternal() const {
    std::vector<int> options;
    if (session_.playerPath.empty()) {
        return options;
    }

    const int currentId = session_.playerPath.back();
    if (currentId < 0 || static_cast<std::size_t>(currentId) >= session_.distanceToTarget.size()) {
        return options;
    }

    std::unordered_set<int> used(session_.playerPath.begin(), session_.playerPath.end());

    struct OptionInfo {
        int idiomId { -1 };
        int distance { -1 };
        int familiarity { 0 };
    };

    std::vector<OptionInfo> candidates;
    for (int nextId : graph_.getNextIds(currentId)) {
        if (used.find(nextId) != used.end()) {
            continue;
        }
        if (nextId < 0 || static_cast<std::size_t>(nextId) >= session_.distanceToTarget.size()) {
            continue;
        }
        const int distance = session_.distanceToTarget[static_cast<std::size_t>(nextId)];
        if (distance < 0) {
            continue;
        }
        const IdiomEntry* entry = graph_.getEntry(nextId);
        candidates.push_back(OptionInfo{nextId, distance, entry != nullptr ? entry->familiarityScore : 0});
    }

    std::sort(candidates.begin(), candidates.end(), [&](const OptionInfo& lhs, const OptionInfo& rhs) {
        if (lhs.distance != rhs.distance) {
            return lhs.distance < rhs.distance;
        }
        if (lhs.familiarity != rhs.familiarity) {
            return lhs.familiarity > rhs.familiarity;
        }
        return wordOf(lhs.idiomId) < wordOf(rhs.idiomId);
    });

    const std::size_t limit = std::min<std::size_t>(4U, candidates.size());
    for (std::size_t index = 0; index < limit; ++index) {
        options.push_back(candidates[index].idiomId);
    }

    std::mt19937 randomEngine(static_cast<unsigned int>(std::random_device{}()));
    std::shuffle(options.begin(), options.end(), randomEngine);
    return options;
}

bool IdiomChainController::isInputValidNext(int nextId) {
    if (session_.playerPath.empty()) {
        return false;
    }

    const int currentId = session_.playerPath.back();
    const std::vector<int>& nextIds = graph_.getNextIds(currentId);
    if (std::find(nextIds.begin(), nextIds.end(), nextId) == nextIds.end()) {
        lastMessage_ = "The selected idiom does not satisfy exact pinyin-with-tone chaining.";
        return false;
    }

    if (std::find(session_.playerPath.begin(), session_.playerPath.end(), nextId) != session_.playerPath.end()) {
        lastMessage_ = "This idiom is already used in the current path.";
        return false;
    }

    return true;
}
