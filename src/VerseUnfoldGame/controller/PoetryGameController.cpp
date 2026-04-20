#include "PoetryGameController.h"

#include <algorithm>
#include <iostream>
#include <unordered_set>
#include <vector>

#include "../utils/StringNormalizer.h"

namespace {

std::string hitKey(const QueryTokenHit& hit) {
    return std::to_string(static_cast<int>(hit.field)) + "#" +
           StringNormalizer::normalizeContent(hit.value);
}

bool isWeakGuessField(QueryField field) {
    return field == QueryField::Dynasty ||
           field == QueryField::Type ||
           field == QueryField::Structure;
}

bool hasStrongHintSignal(const DescriptionQuery& query) {
    for (const auto& hit : query.hits) {
        if (!isWeakGuessField(hit.field)) {
            return true;
        }
    }
    for (const auto& token : query.freeTokens) {
        if (token.size() >= 3) {
            return true;
        }
    }
    return false;
}

void excludeAlreadyGuessedPoems(std::vector<int>& candidates,
                                const std::vector<Poem>& poems,
                                const std::vector<std::string>& guessedTitles) {
    if (candidates.empty() || guessedTitles.empty()) {
        return;
    }

    std::unordered_set<std::string> guessedStd;
    for (const auto& title : guessedTitles) {
        guessedStd.insert(StringNormalizer::normalizeTitle(title));
    }

    std::vector<int> filtered;
    filtered.reserve(candidates.size());
    for (int idx : candidates) {
        if (idx < 0 || idx >= static_cast<int>(poems.size())) {
            continue;
        }
        const std::string titleStd = StringNormalizer::normalizeTitle(poems[idx].title);
        if (guessedStd.find(titleStd) == guessedStd.end()) {
            filtered.push_back(idx);
        }
    }
    candidates.swap(filtered);
}

int findBestAvailableGuessIndex(const RetrievalResult& result,
                                const std::vector<int>& availableCandidates) {
    std::unordered_set<int> allowed(availableCandidates.begin(), availableCandidates.end());
    for (const auto& item : result.ranked) {
        if (allowed.find(item.poemIndex) != allowed.end()) {
            return item.poemIndex;
        }
    }
    return -1;
}

} // namespace

bool PoetryGameController::initialize(const std::string& dbPath) {
    if (!repository.loadFromJson(dbPath)) {
        std::cerr << "PoetryGameController initialize failed: load database error.\n";
        initialized = false;
        return false;
    }

    index.build(repository.getAllPoems());
    initialized = true;
    return true;
}

void PoetryGameController::resetState() {
    state = GameState{};
    hintEngine.reset();
}

void PoetryGameController::buildInitialCandidateSet(int difficulty) {
    state.candidateSet.clear();

    const auto& poems = repository.getAllPoems();
    for (int i = 0; i < static_cast<int>(poems.size()); ++i) {
        if (difficulty <= 0 || poems[i].difficulty == difficulty) {
            state.candidateSet.push_back(i);
        }
    }
}

void PoetryGameController::startNewGame(int mode, int difficulty) {
    resetState();

    state.mode = mode;
    state.difficulty = (mode == 2 ? 0 : difficulty);
    buildInitialCandidateSet(state.difficulty);

    const auto& poems = repository.getAllPoems();

    if (mode == 1) {
        int selectedIndex = questionManager.selectQuestion(difficulty, poems);
        state.targetPoemIndex = selectedIndex;

        if (selectedIndex < 0 || selectedIndex >= static_cast<int>(poems.size())) {
            state.isFinished = true;
            state.isWin = false;
            return;
        }

        hintEngine.init(poems[selectedIndex]);
        revealNextHint();
    }
    else if (mode == 2) {
        state.targetPoemIndex = -1;
        state.bestGuessPoemIndex = -1;
        state.pendingGuessPoemIndex = -1;
        state.mode2GuessAttemptCount = 0;
        state.lastSystemQuestion = "请先给我第一条提示。至少两条提示之后，我才会开始正式猜。";
        state.lastFeedback = "我准备好了，你先给我第一条提示。";
    }
}

JudgeResult PoetryGameController::submitGuess(const std::string& input) {
    if (state.isFinished || state.targetPoemIndex < 0) {
        return JudgeResult::Wrong;
    }

    const auto& poems = repository.getAllPoems();
    const Poem& target = poems[state.targetPoemIndex];

    JudgeResult result = judge.judge(input, target);

    GuessRecord record;
    record.rawInput = input;
    record.normalizedInput = StringNormalizer::normalizeTitle(input);
    record.isCorrect = (result == JudgeResult::Correct);
    record.isNear = (result == JudgeResult::Near);
    state.guessHistory.push_back(record);

    if (result == JudgeResult::Correct) {
        finishGame(true);
        return result;
    }

    if (result == JudgeResult::Wrong) {
        state.wrongGuessCount++;
    }

    return result;
}

DescriptionQuery PoetryGameController::buildMergedMode2Query() const {
    DescriptionQuery merged;
    std::unordered_set<std::string> hitSeen;
    std::unordered_set<std::string> tokenSeen;

    const auto& poems = repository.getAllPoems();
    for (const auto& desc : state.descriptionHistory) {
        DescriptionQuery part = descriptionParser.parse(desc, poems);
        if (!merged.rawInput.empty()) {
            merged.rawInput += "，";
            merged.normalizedInput += "，";
        }
        merged.rawInput += part.rawInput;
        merged.normalizedInput += part.normalizedInput;

        for (const auto& hit : part.hits) {
            const std::string key = hitKey(hit);
            if (hitSeen.insert(key).second) {
                merged.hits.push_back(hit);
            }
        }
        for (const auto& token : part.freeTokens) {
            if (!token.empty() && tokenSeen.insert(token).second) {
                merged.freeTokens.push_back(token);
            }
        }
    }

    return merged;
}

Mode2TurnFeedback PoetryGameController::submitDescription(const std::string& input) {
    Mode2TurnFeedback feedback;
    if (state.isFinished || state.mode != 2 || state.pendingGuessPoemIndex >= 0) {
        return feedback;
    }

    state.descriptionHistory.push_back(input);
    state.usedHintCount = static_cast<int>(state.descriptionHistory.size());
    state.lastSystemQuestion.clear();

    const auto previousCandidates = state.candidateSet;
    const auto& poems = repository.getAllPoems();
    DescriptionQuery query = buildMergedMode2Query();

    RetrievalResult result = retrievalEngine.retrieve(
        query,
        state.candidateSet,
        index,
        poems,
        state.descriptionHistory,
        true
    );

    std::vector<int> nextCandidates = result.nextCandidates;
    excludeAlreadyGuessedPoems(nextCandidates, poems, state.mode2GuessHistory);

    if (nextCandidates.empty()) {
        nextCandidates = previousCandidates;
        excludeAlreadyGuessedPoems(nextCandidates, poems, state.mode2GuessHistory);
    }

    if (nextCandidates.empty()) {
        buildInitialCandidateSet(0);
        nextCandidates = state.candidateSet;
        excludeAlreadyGuessedPoems(nextCandidates, poems, state.mode2GuessHistory);
    }

    state.candidateSet = nextCandidates;
    result.bestGuessIndex = findBestAvailableGuessIndex(result, state.candidateSet);
    state.bestGuessPoemIndex = result.bestGuessIndex;
    state.confidencePercent = result.confidencePercent;

    feedback.confidencePercent = result.confidencePercent;

    const bool enoughHintsToStartGuessing = state.descriptionHistory.size() >= 2;
    const bool strongEnoughSignal = hasStrongHintSignal(query);

    if (!enoughHintsToStartGuessing) {
        state.pendingGuessPoemIndex = -1;
        state.lastFeedback = "我先记下这条提示，再给我一条，我才会开始正式猜。";
        state.lastSystemQuestion = "请继续给我第二条提示，最好补充作者、名句、意象或创作背景。";

        feedback.feedbackText = state.lastFeedback;
        feedback.nextQuestion = state.lastSystemQuestion;
        return feedback;
    }

    if (!strongEnoughSignal) {
        state.pendingGuessPoemIndex = -1;
        state.lastFeedback = "现在的信息还偏宽泛，我不想乱猜。";
        state.lastSystemQuestion = result.nextQuestion.empty()
            ? "请再补充作者、名句、核心意象、创作背景或艺术特色。"
            : result.nextQuestion;

        feedback.feedbackText = state.lastFeedback;
        feedback.nextQuestion = state.lastSystemQuestion;
        return feedback;
    }

    if (result.shouldGuess &&
        result.bestGuessIndex >= 0 &&
        result.bestGuessIndex < static_cast<int>(poems.size()) &&
        state.mode2GuessAttemptCount < 3) {
        state.pendingGuessQueue.clear();
        state.pendingGuessPoemIndex = result.bestGuessIndex;
        state.lastFeedback = "我有一个比较有把握的猜测。";
        state.lastSystemQuestion.clear();

        feedback.guessed = true;
        feedback.guessTitle = poems[state.pendingGuessPoemIndex].title;
        feedback.feedbackText = state.lastFeedback;
        feedback.nextQuestion.clear();

        state.mode2GuessHistory.push_back(poems[state.pendingGuessPoemIndex].title);
        return feedback;
    }

    state.pendingGuessPoemIndex = -1;
    state.lastFeedback = result.feedback.empty()
        ? "我还不够稳，请再给我一条提示。"
        : result.feedback;
    state.lastSystemQuestion = result.nextQuestion.empty()
        ? retrievalEngine.suggestNextQuestion(state.candidateSet, poems, state.descriptionHistory)
        : result.nextQuestion;

    if (state.lastSystemQuestion.empty()) {
        state.lastSystemQuestion = "请再补充一条更具体的信息，例如作者、名句、意象或创作背景。";
    }

    feedback.feedbackText = state.lastFeedback;
    feedback.nextQuestion = state.lastSystemQuestion;
    return feedback;
}

bool PoetryGameController::revealNextHint() {
    if (state.isFinished) {
        return false;
    }

    if (!hintEngine.hasNextHint()) {
        finishGame(false);
        return false;
    }

    HintItem hint = hintEngine.revealNextHint();
    state.shownHints = hintEngine.getShownHints();
    state.usedHintCount = static_cast<int>(state.shownHints.size());

    const auto& poems = repository.getAllPoems();
    state.candidateSet = candidateFilter.filter(
        state.candidateSet,
        hint,
        index,
        poems
    );

    if (hint.type == "answer") {
        finishGame(false);
    }

    return true;
}

bool PoetryGameController::hasPendingMode2Guess() const {
    return state.pendingGuessPoemIndex >= 0;
}

const Poem* PoetryGameController::getPendingMode2GuessPoem() const {
    const auto& poems = repository.getAllPoems();
    if (state.pendingGuessPoemIndex < 0 ||
        state.pendingGuessPoemIndex >= static_cast<int>(poems.size())) {
        return nullptr;
    }
    return &poems[state.pendingGuessPoemIndex];
}

std::string PoetryGameController::confirmMode2Guess(bool correct) {
    if (state.mode != 2 || state.pendingGuessPoemIndex < 0) {
        return "";
    }

    const auto& poems = repository.getAllPoems();
    const int guessedIndex = state.pendingGuessPoemIndex;

    if (correct) {
        state.targetPoemIndex = guessedIndex;
        state.bestGuessPoemIndex = guessedIndex;
        state.pendingGuessPoemIndex = -1;
        state.pendingGuessQueue.clear();
        finishGame(true);
        return "";
    }

    state.wrongGuessCount++;
    state.mode2GuessAttemptCount++;

    std::vector<int> remained;
    remained.reserve(state.candidateSet.size());
    for (int idx : state.candidateSet) {
        if (idx != guessedIndex) {
            remained.push_back(idx);
        }
    }
    state.candidateSet.swap(remained);

    if (state.candidateSet.empty()) {
        buildInitialCandidateSet(0);
        excludeAlreadyGuessedPoems(state.candidateSet, poems, state.mode2GuessHistory);
    }

    state.pendingGuessPoemIndex = -1;
    state.pendingGuessQueue.clear();
    state.bestGuessPoemIndex = -1;
    state.confidencePercent = std::max(0, state.confidencePercent - 20);

    if (state.mode2GuessAttemptCount >= 3) {
        state.lastFeedback = "三次猜测机会已经用完。";
        state.lastSystemQuestion = "我已经用完三次猜测机会了，请你揭晓答案。";
        finishGame(false);
        return state.lastFeedback;
    }

    state.lastFeedback = "这次没猜中，你再给我一条更具体的提示吧。";
    state.lastSystemQuestion = retrievalEngine.suggestNextQuestion(
        state.candidateSet,
        poems,
        state.descriptionHistory
    );

    if (state.lastSystemQuestion.empty()) {
        state.lastSystemQuestion = "请再告诉我作者、名句、核心意象、创作背景或艺术特色。";
    }

    return state.lastFeedback;
}

int PoetryGameController::findPoemIndexByTitleOrAlias(const std::string& input) const {
    std::string inputStd = StringNormalizer::normalizeTitle(input);
    if (inputStd.empty()) {
        return -1;
    }

    const auto& poems = repository.getAllPoems();
    for (int i = 0; i < static_cast<int>(poems.size()); ++i) {
        const Poem& poem = poems[i];
        if (poem.titleStd == inputStd) {
            return i;
        }
        for (const auto& aliasStd : poem.answerAliasStd) {
            if (aliasStd == inputStd) {
                return i;
            }
        }
    }
    return -1;
}

const Poem* PoetryGameController::revealMode2AnswerByTitle(const std::string& input) {
    int idx = findPoemIndexByTitleOrAlias(input);
    if (idx < 0) {
        return nullptr;
    }
    state.targetPoemIndex = idx;
    const auto& poems = repository.getAllPoems();
    return &poems[idx];
}

const GameState& PoetryGameController::getState() const {
    return state;
}

const std::vector<Poem>& PoetryGameController::getAllPoems() const {
    return repository.getAllPoems();
}

const Poem* PoetryGameController::getCurrentPoem() const {
    const auto& poems = repository.getAllPoems();
    if (state.targetPoemIndex < 0 || state.targetPoemIndex >= static_cast<int>(poems.size())) {
        return nullptr;
    }
    return &poems[state.targetPoemIndex];
}

const Poem* PoetryGameController::getBestGuessPoem() const {
    const auto& poems = repository.getAllPoems();
    if (state.bestGuessPoemIndex < 0 || state.bestGuessPoemIndex >= static_cast<int>(poems.size())) {
        return nullptr;
    }
    return &poems[state.bestGuessPoemIndex];
}

int PoetryGameController::getCandidateCount() const {
    return static_cast<int>(state.candidateSet.size());
}

bool PoetryGameController::isInitialized() const {
    return initialized;
}

void PoetryGameController::finishGame(bool isWin) {
    state.isFinished = true;
    state.isWin = isWin;
    state.score = calculateScore();
}

int PoetryGameController::calculateScore() const {
    if (!state.isWin) {
        return 0;
    }

    int difficultyBonus = 0;
    if (state.mode == 1) {
        if (state.difficulty == 2) {
            difficultyBonus = 20;
        }
        else if (state.difficulty == 3) {
            difficultyBonus = 40;
        }
    }

    int score = 50 + difficultyBonus;
    score += (10 - state.usedHintCount) * 5;
    score -= state.wrongGuessCount * 3;

    if (score < 0) {
        score = 0;
    }
    return score;
}