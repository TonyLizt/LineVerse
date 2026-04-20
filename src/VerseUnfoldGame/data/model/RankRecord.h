#pragma once

#include <string>

struct RankRecord {
    std::string userName;
    int totalScore = 0;
    int totalGames = 0;
    int winGames = 0;
    double accuracy = 0.0;
    double avgHintCount = 0.0;
    int bestTimeSeconds = 0;
    std::string lastDate;
    int highestDifficulty = 1;
};