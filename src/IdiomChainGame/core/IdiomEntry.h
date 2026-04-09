#ifndef IDIOM_CHAIN_GAME_IDIOM_ENTRY_H
#define IDIOM_CHAIN_GAME_IDIOM_ENTRY_H

#include <string>

/**
 * @brief Basic idiom data record loaded from idiom.csv.
 */
struct IdiomEntry {
    int id { -1 };
    std::string word;
    std::string explanation;
    std::string derivation;
    std::string example;
    std::string pinyin;
    std::string abbreviation;
    std::string pinyinRaw;
    std::string firstKey;
    std::string lastKey;
    int familiarityScore { 50 };
};

#endif
