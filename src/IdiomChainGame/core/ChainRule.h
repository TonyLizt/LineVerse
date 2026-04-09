#ifndef IDIOM_CHAIN_GAME_CHAIN_RULE_H
#define IDIOM_CHAIN_GAME_CHAIN_RULE_H

#include "IdiomEntry.h"

#include <string>
#include <vector>

/**
 * @brief Interface for idiom chain connection rules.
 */
class IChainRule {
public:
    virtual ~IChainRule() = default;

    /**
     * @brief Determine whether two idioms can connect.
     */
    virtual bool canConnect(const IdiomEntry& from, const IdiomEntry& to) const = 0;

    /**
     * @brief Extract the first matching key from a pinyin string.
     */
    virtual std::string extractFirstKey(const std::string& pinyinWithTone) const = 0;

    /**
     * @brief Extract the last matching key from a pinyin string.
     */
    virtual std::string extractLastKey(const std::string& pinyinWithTone) const = 0;
};

/**
 * @brief Chain rule based on full pinyin syllable equality with tone marks.
 *
 * Example:
 *   "ā bí dì yù" -> firstKey = "ā", lastKey = "yù"
 * Two idioms can connect only when lastKey(from) == firstKey(to).
 */
class PinyinToneChainRule : public IChainRule {
public:
    bool canConnect(const IdiomEntry& from, const IdiomEntry& to) const override;
    std::string extractFirstKey(const std::string& pinyinWithTone) const override;
    std::string extractLastKey(const std::string& pinyinWithTone) const override;

private:
    static std::vector<std::string> splitSyllables(const std::string& pinyinWithTone);
};

#endif
