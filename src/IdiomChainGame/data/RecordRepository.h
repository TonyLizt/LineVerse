#ifndef IDIOM_CHAIN_GAME_RECORD_REPOSITORY_H
#define IDIOM_CHAIN_GAME_RECORD_REPOSITORY_H

#include <queue>
#include <string>
#include <vector>

/**
 * @brief Persisted record for one finished game.
 */
struct GameRecord {
    std::string playerName;
    std::string startWord;
    std::string targetWord;
    std::string difficulty;
    double elapsedSeconds { 0.0 };
    int pathLength { 0 };
    int score { 0 };
};

/**
 * @brief Leaderboard item ordered by total score.
 */
struct RankItem {
    std::string playerName;
    int totalScore { 0 };

    bool operator<(const RankItem& rhs) const {
        return totalScore < rhs.totalScore;
    }
};

/**
 * @brief Simple CSV-based persistence for records and ranking.
 */
class RecordRepository {
public:
    RecordRepository();

    void saveRecord(const GameRecord& record);
    std::vector<GameRecord> loadRecords() const;
    std::priority_queue<RankItem> buildLeaderboard() const;

private:
    std::string recordPath_;
};

#endif
