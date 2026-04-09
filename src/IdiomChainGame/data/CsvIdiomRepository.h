#ifndef IDIOM_CHAIN_GAME_CSV_IDIOM_REPOSITORY_H
#define IDIOM_CHAIN_GAME_CSV_IDIOM_REPOSITORY_H

#include "IIdiomRepository.h"

#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief CSV-based idiom repository.
 */
class CsvIdiomRepository : public IIdiomRepository {
public:
    CsvIdiomRepository();

    bool load() override;
    const IdiomEntry* findByWord(const std::string& wordOrAbbreviation) const override;
    const std::vector<IdiomEntry>& getAll() const override;

private:
    static std::vector<std::string> parseCsvLine(const std::string& line);
    static std::string trim(const std::string& value);
    bool loadFromPath(const std::string& path);

    std::vector<IdiomEntry> entries_;
    std::unordered_map<std::string, int> wordToIndex_;
    std::unordered_map<std::string, int> abbreviationToIndex_;
};

#endif
