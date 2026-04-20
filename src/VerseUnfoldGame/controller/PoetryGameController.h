#pragma once

#include <string>
#include <vector>

#include "../data/PoetryIndex.h"
#include "../data/PoetryRepository.h"
#include "../data/model/GameState.h"
#include "../service/CandidateFilter.h"
#include "../service/PoetryDescriptionParser.h"
#include "../service/PoetryHintEngine.h"
#include "../service/PoetryJudge.h"
#include "../service/PoetryQuestionManager.h"
#include "../service/PoetryRetrievalEngine.h"

struct Mode2TurnFeedback {
    int confidencePercent = 0;
    bool guessed = false;
    std::string guessTitle;
    std::string nextQuestion;
    std::string feedbackText;
};

class PoetryGameController {
public:
    bool initialize(const std::string& dbPath);

    void startNewGame(int mode, int difficulty);

    JudgeResult submitGuess(const std::string& input);
    Mode2TurnFeedback submitDescription(const std::string& input);
    bool revealNextHint();

    bool hasPendingMode2Guess() const;
    const Poem* getPendingMode2GuessPoem() const;
    std::string confirmMode2Guess(bool correct);
    const Poem* revealMode2AnswerByTitle(const std::string& input);

    const GameState& getState() const;
    const std::vector<Poem>& getAllPoems() const;
    const Poem* getCurrentPoem() const;
    const Poem* getBestGuessPoem() const;

    int getCandidateCount() const;
    bool isInitialized() const;

private:
    void resetState();
    void buildInitialCandidateSet(int difficulty);
    void finishGame(bool isWin);
    int calculateScore() const;
    int findPoemIndexByTitleOrAlias(const std::string& input) const;
    DescriptionQuery buildMergedMode2Query() const;

private:
    PoetryRepository repository;
    PoetryIndex index;
    PoetryQuestionManager questionManager;
    PoetryHintEngine hintEngine;
    PoetryJudge judge;
    CandidateFilter candidateFilter;
    PoetryDescriptionParser descriptionParser;
    PoetryRetrievalEngine retrievalEngine;

    GameState state;
    bool initialized = false;
};
