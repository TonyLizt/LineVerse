#pragma once

#include <string>
#include <vector>

#include "HintItem.h"

struct GuessRecord {
    std::string rawInput;
    std::string normalizedInput;
    bool isCorrect = false;
    bool isNear = false;
};

struct GameState {
    int mode = 0;          // 1: 电脑问玩家 2: 玩家问电脑
    int difficulty = 1;    // 模式1使用1/2/3，模式2固定为0表示全库
    int targetPoemIndex = -1;
    int bestGuessPoemIndex = -1;
    int pendingGuessPoemIndex = -1;

    std::vector<int> candidateSet;
    std::vector<int> pendingGuessQueue;
    std::vector<HintItem> shownHints;
    std::vector<GuessRecord> guessHistory;
    std::vector<std::string> descriptionHistory;
    std::vector<std::string> systemQuestionHistory;
    std::vector<std::string> mode2GuessHistory;

    std::string lastSystemQuestion;
    std::string lastFeedback;
    int confidencePercent = 0;
    int mode2GuessAttemptCount = 0;
    int usedHintCount = 0;
    int wrongGuessCount = 0;
    int score = 0;

    bool isFinished = false;
    bool isWin = false;
};
