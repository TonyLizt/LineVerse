#include "CsvIdiomRepository.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

CsvIdiomRepository::CsvIdiomRepository() = default;

bool CsvIdiomRepository::load() {
    const std::vector<std::string> candidatePaths = {
        "data/prebuild/IdiomChainGame/idiom.csv",
        "data/raw/idiom.csv",
        "data/idiom.csv",
        "idiom.csv"
    };

    for (const std::string& path : candidatePaths) {
        if (loadFromPath(path)) {
            std::cout << "[Info] Loaded idiom CSV from: " << path << '\n';
            return true;
        }
    }

    std::cerr << "[Error] Could not find idiom.csv in LineVerse/data/prebuild/IdiomChainGame, data/raw, data, or current directory.\n";
    return false;
}

const IdiomEntry* CsvIdiomRepository::findByWord(const std::string& wordOrAbbreviation) const {
    auto it = wordToIndex_.find(wordOrAbbreviation);
    if (it != wordToIndex_.end()) {
        return &entries_[static_cast<std::size_t>(it->second)];
    }
    it = abbreviationToIndex_.find(wordOrAbbreviation);
    if (it != abbreviationToIndex_.end()) {
        return &entries_[static_cast<std::size_t>(it->second)];
    }
    return nullptr;
}

const std::vector<IdiomEntry>& CsvIdiomRepository::getAll() const {
    return entries_;
}

std::vector<std::string> CsvIdiomRepository::parseCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool inQuotes = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (ch == '"') {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                field.push_back('"');
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (ch == ',' && !inQuotes) {
            fields.push_back(field);
            field.clear();
        } else {
            field.push_back(ch);
        }
    }

    fields.push_back(field);
    return fields;
}

std::string CsvIdiomRepository::trim(const std::string& value) {
    std::size_t begin = 0;
    std::size_t end = value.size();

    while (begin < end && (value[begin] == ' ' || value[begin] == '\t' || value[begin] == '\r' || value[begin] == '\n')) {
        ++begin;
    }
    while (end > begin && (value[end - 1] == ' ' || value[end - 1] == '\t' || value[end - 1] == '\r' || value[end - 1] == '\n')) {
        --end;
    }

    return value.substr(begin, end - begin);
}

bool CsvIdiomRepository::loadFromPath(const std::string& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        return false;
    }

    entries_.clear();
    wordToIndex_.clear();
    abbreviationToIndex_.clear();

    std::string headerLine;
    if (!std::getline(input, headerLine)) {
        return false;
    }

    const std::vector<std::string> headers = parseCsvLine(headerLine);
    std::unordered_map<std::string, int> indexOf;
    for (std::size_t i = 0; i < headers.size(); ++i) {
        indexOf[trim(headers[i])] = static_cast<int>(i);
    }

    auto getField = [&](const std::vector<std::string>& row, const std::string& key) -> std::string {
        auto it = indexOf.find(key);
        if (it == indexOf.end()) {
            return {};
        }
        if (it->second < 0 || static_cast<std::size_t>(it->second) >= row.size()) {
            return {};
        }
        return trim(row[static_cast<std::size_t>(it->second)]);
    };

    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        const std::vector<std::string> row = parseCsvLine(line);
        IdiomEntry entry;
        entry.word = getField(row, "word");
        entry.explanation = getField(row, "explanation");
        entry.derivation = getField(row, "derivation");
        entry.example = getField(row, "example");
        entry.pinyin = getField(row, "pinyin");
        entry.abbreviation = getField(row, "abbreviation");
        entry.pinyinRaw = getField(row, "pinyin_r");

        if (entry.word.empty() || entry.pinyin.empty()) {
            continue;
        }

        entries_.push_back(entry);
        const int index = static_cast<int>(entries_.size() - 1U);
        wordToIndex_[entry.word] = index;
        if (!entry.abbreviation.empty()) {
            abbreviationToIndex_[entry.abbreviation] = index;
        }
    }

    return !entries_.empty();
}
