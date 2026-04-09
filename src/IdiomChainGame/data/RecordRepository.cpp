#include "RecordRepository.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace {
std::string chooseRecordPath() {
    const std::vector<std::string> baseCandidates = {
        "data/prebuild/IdiomChainGame",
        "../data/prebuild/IdiomChainGame",
        "../../data/prebuild/IdiomChainGame"
    };

    for (const std::string& base : baseCandidates) {
        std::error_code ec;
        std::filesystem::create_directories(base, ec);
        if (!ec) {
            return base + "/idiom_chain_records.csv";
        }
    }

    return "idiom_chain_records.csv";
}
} // namespace

RecordRepository::RecordRepository()
    : recordPath_(chooseRecordPath()) {
}

void RecordRepository::saveRecord(const GameRecord& record) {
    std::ofstream output(recordPath_, std::ios::app);
    if (!output.is_open()) {
        std::cerr << "[Warn] Could not open record file for writing: " << recordPath_ << '\n';
        return;
    }

    output << '"' << record.playerName << "\"," 
           << '"' << record.startWord << "\"," 
           << '"' << record.targetWord << "\"," 
           << '"' << record.difficulty << "\"," 
           << std::fixed << std::setprecision(2) << record.elapsedSeconds << ','
           << record.pathLength << ','
           << record.score << '\n';
}

std::vector<GameRecord> RecordRepository::loadRecords() const {
    std::vector<GameRecord> records;
    std::ifstream input(recordPath_);
    if (!input.is_open()) {
        return records;
    }

    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        std::vector<std::string> fields;
        std::string field;
        bool inQuotes = false;
        for (char ch : line) {
            if (ch == '"') {
                inQuotes = !inQuotes;
            } else if (ch == ',' && !inQuotes) {
                fields.push_back(field);
                field.clear();
            } else {
                field.push_back(ch);
            }
        }
        fields.push_back(field);

        if (fields.size() < 7U) {
            continue;
        }

        GameRecord record;
        record.playerName = fields[0];
        record.startWord = fields[1];
        record.targetWord = fields[2];
        record.difficulty = fields[3];
        record.elapsedSeconds = std::stod(fields[4]);
        record.pathLength = std::stoi(fields[5]);
        record.score = std::stoi(fields[6]);
        records.push_back(record);
    }

    return records;
}

std::priority_queue<RankItem> RecordRepository::buildLeaderboard() const {
    std::unordered_map<std::string, int> totalScores;
    for (const GameRecord& record : loadRecords()) {
        totalScores[record.playerName] += record.score;
    }

    std::priority_queue<RankItem> ranking;
    for (const auto& entry : totalScores) {
        ranking.push(RankItem{entry.first, entry.second});
    }
    return ranking;
}
