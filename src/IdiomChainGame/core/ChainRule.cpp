#include "ChainRule.h"

#include <sstream>

bool PinyinToneChainRule::canConnect(const IdiomEntry& from, const IdiomEntry& to) const {
    return !from.lastKey.empty() && from.lastKey == to.firstKey;
}

std::string PinyinToneChainRule::extractFirstKey(const std::string& pinyinWithTone) const {
    const std::vector<std::string> syllables = splitSyllables(pinyinWithTone);
    if (syllables.empty()) {
        return {};
    }
    return syllables.front();
}

std::string PinyinToneChainRule::extractLastKey(const std::string& pinyinWithTone) const {
    const std::vector<std::string> syllables = splitSyllables(pinyinWithTone);
    if (syllables.empty()) {
        return {};
    }
    return syllables.back();
}

std::vector<std::string> PinyinToneChainRule::splitSyllables(const std::string& pinyinWithTone) {
    std::vector<std::string> syllables;
    std::istringstream input(pinyinWithTone);
    std::string token;
    while (input >> token) {
        syllables.push_back(token);
    }
    return syllables;
}
