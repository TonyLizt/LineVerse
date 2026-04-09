#include "IdiomChainController.h"

#include "../core/HintEngine.h"
#include "../core/IdiomGraph.h"
#include "../core/ModeStrategy.h"
#include "../core/ScoreCalculator.h"
#include "../data/IIdiomRepository.h"
#include "../net/IBattleTransport.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <limits>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace {

std::string modeToText(GameMode mode) {
    switch (mode) {
    case GameMode::SingleEasy:   return "简单模式";
    case GameMode::SingleMedium: return "中等模式";
    case GameMode::SingleHard:   return "困难模式";
    case GameMode::BattleEasy:   return "对战简单模式";
    case GameMode::BattleMedium: return "对战中等模式";
    case GameMode::BattleHard:   return "对战困难模式";
    }
    return "未知模式";
}

bool isEasyMode(GameMode mode) {
    return mode == GameMode::SingleEasy || mode == GameMode::BattleEasy;
}

bool isMediumMode(GameMode mode) {
    return mode == GameMode::SingleMedium || mode == GameMode::BattleMedium;
}

bool isHardMode(GameMode mode) {
    return mode == GameMode::SingleHard || mode == GameMode::BattleHard;
}

bool isBattleMode(GameMode mode) {
    return mode == GameMode::BattleEasy || mode == GameMode::BattleMedium || mode == GameMode::BattleHard;
}

std::string toLowerAscii(std::string text) {
    for (char& ch : text) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return text;
}

bool containsInsensitive(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) {
        return true;
    }
    const std::string loweredHaystack = toLowerAscii(haystack);
    const std::string loweredNeedle = toLowerAscii(needle);
    return loweredHaystack.find(loweredNeedle) != std::string::npos;
}

std::vector<std::string> split(const std::string& text, char delim) {
    std::vector<std::string> parts;
    std::stringstream ss(text);
    std::string item;
    while (std::getline(ss, item, delim)) {
        parts.push_back(item);
    }
    return parts;
}

class TimerHolder {
public:
    void reset() {
        start_ = std::chrono::steady_clock::now();
    }

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
    RecordRepository& recordRepository,
    IBattleTransport* battleTransport)
    : repository_(repository)
    , graph_(graph)
    , solver_(solver)
    , hintEngine_(hintEngine)
    , recordRepository_(recordRepository)
    , battleTransport_(battleTransport) {
}

void IdiomChainController::startSingleGame(GameMode mode, const std::string& playerName) {
    session_ = GameSession{};
    session_.playerName = playerName;
    session_.mode = mode;

    std::unique_ptr<IModeStrategy> strategy;
    if (isEasyMode(mode)) {
        strategy = std::make_unique<EasyModeStrategy>();
    } else if (isMediumMode(mode)) {
        strategy = std::make_unique<MediumModeStrategy>();
    } else {
        strategy = std::make_unique<HardModeStrategy>();
    }

    prepareSession(*strategy);
    refreshMediumOptions();
    g_timer.reset();
    pendingRecordFlush_ = false;
    lastHeartbeatSentSeconds_ = -1.0;
}

bool IdiomChainController::hostBattle(GameMode mode, const std::string& playerName, unsigned short port) {
    if (!battleTransport_) {
        lastMessage_ = "当前构建未注入联网传输层。";
        return false;
    }
    if (!isBattleMode(mode)) {
        lastMessage_ = "房主模式必须使用对战难度。";
        return false;
    }
    leaveBattle();

    session_ = GameSession{};
    session_.playerName = playerName;
    session_.mode = mode;
    session_.battleIsHost = true;
    session_.battleConnected = false;
    session_.battleRoundStarted = false;
    pendingBattleMode_ = mode;
    pendingRecordFlush_ = false;
    lastHeartbeatSentSeconds_ = -1.0;
    if (!battleTransport_->host(port)) {
        lastMessage_ = "创建房间失败。";
        return false;
    }
    lastMessage_ = "已创建房间，等待对手连接。";
    return true;
}

bool IdiomChainController::joinBattle(GameMode mode, const std::string& playerName, const std::string& ip, unsigned short port) {
    if (!battleTransport_) {
        lastMessage_ = "当前构建未注入联网传输层。";
        return false;
    }
    if (!isBattleMode(mode)) {
        lastMessage_ = "加入房间时必须选择对战难度。";
        return false;
    }
    leaveBattle();

    session_ = GameSession{};
    session_.playerName = playerName;
    session_.mode = mode;
    session_.battleIsHost = false;
    session_.battleConnected = false;
    session_.battleRoundStarted = false;
    pendingBattleMode_ = mode;
    pendingRecordFlush_ = false;
    lastHeartbeatSentSeconds_ = -1.0;

    if (!battleTransport_->connectTo(ip, port)) {
        lastMessage_ = "连接房主失败。";
        return false;
    }

    const std::string hello = "HELLO|" + playerName + "|" + std::to_string(static_cast<int>(mode));
    if (!battleTransport_->send(hello)) {
        battleTransport_->stop();
        lastMessage_ = "已连接网络，但发送握手失败。";
        return false;
    }

    lastMessage_ = "正在连接房主，等待题目同步。";
    return true;
}

void IdiomChainController::leaveBattle() {
    if (battleTransport_ && isBattleMode(session_.mode) && session_.battleConnected) {
        battleTransport_->send("LEAVE|" + session_.playerName);
    }

    if (battleTransport_) {
        battleTransport_->stop();
    }

    if (isBattleMode(session_.mode)) {
        session_.battleConnected = false;
        session_.battleRoundStarted = false;
        session_.remotePlayerName.clear();
        session_.remotePath.clear();
        session_.battleWinnerText.clear();
        session_.remoteFinished = false;
        session_.remoteSuccess = false;
        session_.remoteStepCount = 0;
        session_.remoteElapsedSeconds = 0.0;
        session_.remoteScore = 0;
        lastMessage_ = "已退出房间。";
    }
}

bool IdiomChainController::isBattleConnected() const {
    return session_.battleConnected;
}

bool IdiomChainController::isBattleRoundStarted() const {
    return session_.battleRoundStarted;
}

void IdiomChainController::pollBattle() {
    if (!battleTransport_) {
        return;
    }
    std::string message;
    while (battleTransport_->poll(message)) {
        handleBattleMessage(message);
    }
}

void IdiomChainController::tick() {
    if (isBattleMode(session_.mode)) {
        pollBattle();
    }

    if (!session_.finished && (!isBattleMode(session_.mode) || session_.battleRoundStarted)) {
        updateElapsedSeconds();
    }

    // battle 中对局开始后，定时同步状态，保证对手端时间实时刷新
    if (isBattleMode(session_.mode) && session_.battleConnected && session_.battleRoundStarted) {
        if (lastHeartbeatSentSeconds_ < 0.0
            || session_.elapsedSeconds - lastHeartbeatSentSeconds_ >= heartbeatIntervalSeconds_) {
            sendBattleState();
            lastHeartbeatSentSeconds_ = session_.elapsedSeconds;
        }
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
    if (!isEasyMode(session_.mode)) {
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
            lastMessage_ = "排序索引超出范围。";
            return false;
        }
        const int idiomId = session_.easyPool[static_cast<std::size_t>(poolIndex)];
        if (!used.insert(idiomId).second) {
            lastMessage_ = "排序结果中存在重复项。";
            return false;
        }
        candidatePath.push_back(idiomId);
    }

    if (candidatePath.empty()
        || candidatePath.front() != session_.startId
        || candidatePath.back() != session_.targetId) {
        lastMessage_ = "路径必须以起点成语开始，并以终点成语结束。";
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
        lastMessage_ = "当前排序无法构成合法接龙路径。";
        return false;
    }

    session_.playerPath = candidatePath;
    session_.stepCount = static_cast<int>(candidatePath.size()) - 1;
    session_.finished = true;
    session_.success = true;

    const ScoreCalculator calculator;
    session_.score = answerRevealedThisRound_ ? 0 : calculator.calculate(session_);
    saveRecordIfNeeded();
    lastMessage_ = isBattleMode(session_.mode) ? "本方已完成对局，等待对手。" : "简单模式已完成。";
    if (isBattleMode(session_.mode)) {
        sendBattleState();
        updateBattleWinnerText();
    }
    return true;
}

std::vector<int> IdiomChainController::getMediumOptions() const {
    return session_.mediumOptions;
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
    if (!isMediumMode(session_.mode)) {
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
        lastMessage_ = "当前没有可到达终点的可选项，本局失败。";
        saveRecordIfNeeded();
        if (isBattleMode(session_.mode)) {
            sendBattleState();
            updateBattleWinnerText();
        }
        return false;
    }

    if (optionIndex < 0 || static_cast<std::size_t>(optionIndex) >= options.size()) {
        lastMessage_ = "所选项索引无效。";
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
        lastMessage_ = "已提交该步选择。";
    }
    if (isBattleMode(session_.mode)) {
        sendBattleState();
        updateBattleWinnerText();
    }
    return true;
}

bool IdiomChainController::submitMediumChoiceById(int idiomId) {
    if (!isMediumMode(session_.mode)) {
        return false;
    }

    const std::vector<int>& options = session_.mediumOptions;
    auto it = std::find(options.begin(), options.end(), idiomId);
    if (it == options.end()) {
        lastMessage_ = "所选成语不在当前四个选项中。";
        return false;
    }

    return submitMediumChoice(static_cast<int>(std::distance(options.begin(), it)));
}

bool IdiomChainController::submitHardInput(const std::string& inputWordOrAbbreviation) {
    if (!isHardMode(session_.mode)) {
        return false;
    }

    updateElapsedSeconds();
    if (session_.finished) {
        return false;
    }

    const int nextId = graph_.getIdByWord(inputWordOrAbbreviation);
    if (nextId < 0) {
        lastMessage_ = "未找到对应成语，可尝试输入完整成语或缩写。";
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
        lastMessage_ = "已提交一步。";
    }
    if (isBattleMode(session_.mode)) {
        sendBattleState();
        updateBattleWinnerText();
    }
    return true;
}

std::vector<std::string> IdiomChainController::getHardCandidateWords(const std::string& query) const {
    std::vector<std::string> words;
    if (!isHardMode(session_.mode) || session_.playerPath.empty()) {
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
    if (isEasyMode(session_.mode)) {
        lastMessage_ = "简单模式不支持撤回。";
        return false;
    }
    if (session_.playerPath.size() <= 1U) {
        lastMessage_ = "当前没有可撤回的步骤。";
        return false;
    }

    session_.playerPath.pop_back();
    if (!session_.rollbackStack.empty()) {
        session_.rollbackStack.pop();
    }
    session_.rollbackCount += 1;
    session_.stepCount += 1;
    if (isMediumMode(session_.mode)) {
        refreshMediumOptions();
    }
    lastMessage_ = "已撤回一步。";
    if (isBattleMode(session_.mode)) {
        sendBattleState();
    }
    return true;
}

std::optional<std::string> IdiomChainController::requestHint() {
    if (isEasyMode(session_.mode)) {
        lastMessage_ = "简单模式不提供提示。";
        return std::nullopt;
    }
    if (isHardMode(session_.mode) && session_.hintCount >= session_.maxHints) {
        lastMessage_ = "提示次数已用完。";
        return std::nullopt;
    }

    if (session_.playerPath.empty()) {
        lastMessage_ = "当前没有可供提示的路径状态。";
        return std::nullopt;
    }

    int hintId = -1;

    if (isMediumMode(session_.mode)) {
        const std::vector<int>& options = session_.mediumOptions;
        if (options.empty()) {
            lastMessage_ = "当前四个选项为空，无法提供提示。";
            return std::nullopt;
        }

        int bestDistance = std::numeric_limits<int>::max();
        int bestFamiliarity = std::numeric_limits<int>::min();
        for (int optionId : options) {
            if (optionId < 0 || static_cast<std::size_t>(optionId) >= session_.distanceToTarget.size()) {
                continue;
            }
            const int distance = session_.distanceToTarget[static_cast<std::size_t>(optionId)];
            if (distance < 0) {
                continue;
            }
            const IdiomEntry* entry = graph_.getEntry(optionId);
            const int familiarity = entry != nullptr ? entry->familiarityScore : 0;
            if (distance < bestDistance || (distance == bestDistance && familiarity > bestFamiliarity)) {
                bestDistance = distance;
                bestFamiliarity = familiarity;
                hintId = optionId;
            }
        }

        if (hintId < 0) {
            lastMessage_ = "当前四个选项都无法有效到达终点。";
            return std::nullopt;
        }
    } else {
        const int currentId = session_.playerPath.back();
        const std::optional<int> nextId = hintEngine_.suggestNextStep(currentId, session_.targetId);
        if (!nextId.has_value()) {
            lastMessage_ = "当前无法提供有效提示。";
            return std::nullopt;
        }
        hintId = *nextId;
    }

    session_.hintCount += 1;
    if (isMediumMode(session_.mode) || isHardMode(session_.mode)) {
        session_.stepCount += 1;
    }

    lastMessage_ = "已提供提示。";
    if (isBattleMode(session_.mode)) {
        sendBattleState();
    }
    return wordOf(hintId);
}

PathResult IdiomChainController::revealAnswer() {
    answerRevealedThisRound_ = true;
    lastMessage_ = "已查看最优解，本局得分记为0。";
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
    initializeSessionForQuestion(session_.mode, session_.startId, session_.targetId, true);
    lastMessage_ = "本局已开始，请通过严格带调拼音接龙到达终点。";
}

void IdiomChainController::prepareFixedBattleSession(GameMode mode, int startId, int targetId) {
    initializeSessionForQuestion(mode, startId, targetId, true);
    refreshMediumOptions();
    session_.battleRoundStarted = true;
    lastMessage_ = "对战已开始，请尽快完成。";
    g_timer.reset();
    lastHeartbeatSentSeconds_ = -1.0;
}

void IdiomChainController::initializeSessionForQuestion(GameMode mode, int startId, int targetId, bool generateEasyPool) {
    session_.mode = mode;
    session_.startId = startId;
    session_.targetId = targetId;

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
    session_.elapsedSeconds = 0.0;
    session_.finished = false;
    session_.success = false;
    session_.score = 0;
    session_.hintCount = 0;
    session_.rollbackCount = 0;
    session_.mediumOptions.clear();
    session_.easyPool.clear();
    session_.remotePath.clear();
    session_.remoteStepCount = 0;
    session_.remoteElapsedSeconds = 0.0;
    session_.remoteScore = 0;
    session_.remoteFinished = false;
    session_.remoteSuccess = false;
    session_.battleWinnerText.clear();

    if (isEasyMode(mode)) {
        session_.timeLimitSeconds = 180;
    } else if (isMediumMode(mode)) {
        session_.timeLimitSeconds = 240;
    } else {
        session_.timeLimitSeconds = 300;
        session_.maxHints = 3;
    }

    if (generateEasyPool && isEasyMode(mode)) {
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
        lastMessage_ = "已超出时间限制。";
        saveRecordIfNeeded();
        if (isBattleMode(session_.mode)) {
            sendBattleState();
            updateBattleWinnerText();
        }
    }
}

void IdiomChainController::finalizeIfTargetReached() {
    if (!session_.playerPath.empty() && session_.playerPath.back() == session_.targetId) {
        session_.finished = true;
        session_.success = true;
        const ScoreCalculator calculator;
        session_.score = answerRevealedThisRound_ ? 0 : calculator.calculate(session_);
        lastMessage_ = isBattleMode(session_.mode) ? "已到达终点，等待对手完成。" : "已成功到达终点。";
        saveRecordIfNeeded();
        return;
    }

    if (isMediumMode(session_.mode)) {
        const std::vector<int>& options = session_.mediumOptions;
        if (options.empty()) {
            session_.finished = true;
            session_.success = false;
            lastMessage_ = "已没有可继续到达终点的选项，本局失败。";
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
    if (!isMediumMode(session_.mode)) {
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

    std::unordered_map<std::string, OptionInfo> bestByWord;

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
        const int familiarity = entry != nullptr ? entry->familiarityScore : 0;
        const std::string word = entry != nullptr ? entry->word : wordOf(nextId);
        if (word.empty()) {
            continue;
        }

        OptionInfo candidate{nextId, distance, familiarity};
        auto it = bestByWord.find(word);
        if (it == bestByWord.end()) {
            bestByWord.emplace(word, candidate);
            continue;
        }
        const OptionInfo& old = it->second;
        if (candidate.distance < old.distance ||
            (candidate.distance == old.distance && candidate.familiarity > old.familiarity) ||
            (candidate.distance == old.distance && candidate.familiarity == old.familiarity &&
             candidate.idiomId == session_.targetId && old.idiomId != session_.targetId)) {
            it->second = candidate;
        }
    }

    std::vector<OptionInfo> candidates;
    candidates.reserve(bestByWord.size());
    for (const auto& kv : bestByWord) {
        candidates.push_back(kv.second);
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
        lastMessage_ = "所选成语不满足严格带调拼音接龙规则。";
        return false;
    }

    if (std::find(session_.playerPath.begin(), session_.playerPath.end(), nextId) != session_.playerPath.end()) {
        lastMessage_ = "该成语已经在当前路径中使用过了。";
        return false;
    }

    return true;
}

void IdiomChainController::sendBattleState() {
    if (!battleTransport_ || !isBattleMode(session_.mode) || !session_.battleConnected || !session_.battleRoundStarted) {
        return;
    }

    std::ostringstream oss;
    oss << "STATE|"
        << session_.stepCount << "|"
        << session_.elapsedSeconds << "|"
        << (session_.finished ? 1 : 0) << "|"
        << (session_.success ? 1 : 0) << "|"
        << session_.score << "|"
        << serializePath(session_.playerPath);
    battleTransport_->send(oss.str());
}

void IdiomChainController::handleBattleMessage(const std::string& message) {
    const std::vector<std::string> parts = split(message, '|');
    if (parts.empty()) {
        return;
    }

    if (parts[0] == "HELLO") {
        if (!session_.battleIsHost || parts.size() < 2) {
            return;
        }
        session_.remotePlayerName = parts[1];
        session_.battleConnected = true;

        std::ostringstream ack;
        ack << "HELLO_ACK|" << session_.playerName << "|" << static_cast<int>(pendingBattleMode_);
        battleTransport_->send(ack.str());

        startHostBattleRound();
        return;
    }

    if (parts[0] == "HELLO_ACK") {
        if (session_.battleIsHost || parts.size() < 3) {
            return;
        }
        session_.remotePlayerName = parts[1];
        session_.battleConnected = true;
        pendingBattleMode_ = static_cast<GameMode>(std::stoi(parts[2]));
        lastMessage_ = "已连接到房主，等待题目同步。";
        return;
    }

    if (parts[0] == "START") {
        if (parts.size() < 4) {
            return;
        }
        const GameMode mode = static_cast<GameMode>(std::stoi(parts[1]));
        const int startId = std::stoi(parts[2]);
        const int targetId = std::stoi(parts[3]);

        prepareFixedBattleSession(mode, startId, targetId);
        session_.battleConnected = true;
        session_.battleRoundStarted = true;
        lastMessage_ = "已收到对局题目，对局开始。";
        sendBattleState();
        return;
    }

    if (parts[0] == "STATE") {
        if (parts.size() < 7) {
            return;
        }
        session_.remoteStepCount = std::stoi(parts[1]);
        session_.remoteElapsedSeconds = std::stod(parts[2]);
        session_.remoteFinished = (std::stoi(parts[3]) != 0);
        session_.remoteSuccess = (std::stoi(parts[4]) != 0);
        session_.remoteScore = std::stoi(parts[5]);
        session_.remotePath = deserializePath(parts[6]);
        updateBattleWinnerText();
        return;
    }

    if (parts[0] == "LEAVE") {
        session_.battleConnected = false;
        session_.battleRoundStarted = false;
        session_.remoteFinished = false;
        session_.remoteSuccess = false;
        session_.battleWinnerText.clear();

        if (parts.size() >= 2) {
            session_.remotePlayerName = parts[1];
            lastMessage_ = parts[1] + " 已离开房间。";
        } else {
            lastMessage_ = "对手已离开房间。";
        }
        return;
    }
}

void IdiomChainController::startHostBattleRound() {
    session_.mode = pendingBattleMode_;
    std::unique_ptr<IModeStrategy> strategy;
    if (isEasyMode(pendingBattleMode_)) {
        strategy = std::make_unique<EasyModeStrategy>();
    } else if (isMediumMode(pendingBattleMode_)) {
        strategy = std::make_unique<MediumModeStrategy>();
    } else {
        strategy = std::make_unique<HardModeStrategy>();
    }

    prepareSession(*strategy);
    refreshMediumOptions();
    session_.battleConnected = true;
    session_.battleRoundStarted = true;
    session_.battleIsHost = true;
    g_timer.reset();
    lastHeartbeatSentSeconds_ = -1.0;

    std::ostringstream startMsg;
    startMsg << "START|"
             << static_cast<int>(pendingBattleMode_) << "|"
             << session_.startId << "|"
             << session_.targetId;
    battleTransport_->send(startMsg.str());
    sendBattleState();
    lastMessage_ = "对手已连接，对局开始。";
}

void IdiomChainController::updateBattleWinnerText() {
    if (!isBattleMode(session_.mode) || !session_.battleRoundStarted) {
        return;
    }
    if (!session_.finished || !session_.remoteFinished) {
        session_.battleWinnerText.clear();
        return;
    }

    if (session_.score != session_.remoteScore) {
        session_.battleWinnerText = (session_.score > session_.remoteScore)
            ? (session_.playerName + " 获胜")
            : (session_.remotePlayerName + " 获胜");
        return;
    }

    if (session_.stepCount != session_.remoteStepCount) {
        session_.battleWinnerText = (session_.stepCount < session_.remoteStepCount)
            ? (session_.playerName + " 获胜")
            : (session_.remotePlayerName + " 获胜");
        return;
    }

    if (session_.elapsedSeconds != session_.remoteElapsedSeconds) {
        session_.battleWinnerText = (session_.elapsedSeconds < session_.remoteElapsedSeconds)
            ? (session_.playerName + " 获胜")
            : (session_.remotePlayerName + " 获胜");
        return;
    }

    session_.battleWinnerText = "平局";
}

std::string IdiomChainController::serializePath(const std::vector<int>& path) const {
    std::ostringstream oss;
    for (std::size_t i = 0; i < path.size(); ++i) {
        if (i > 0U) {
            oss << ",";
        }
        oss << path[i];
    }
    return oss.str();
}

std::vector<int> IdiomChainController::deserializePath(const std::string& text) const {
    std::vector<int> path;
    if (text.empty()) {
        return path;
    }
    for (const std::string& item : split(text, ',')) {
        if (!item.empty()) {
            path.push_back(std::stoi(item));
        }
    }
    return path;
}
