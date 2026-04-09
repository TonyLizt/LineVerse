#ifndef IDIOM_CHAIN_GAME_IDIOM_GRAPH_H
#define IDIOM_CHAIN_GAME_IDIOM_GRAPH_H

#include "IdiomEntry.h"

#include <string>
#include <unordered_map>
#include <vector>

class IIdiomRepository;
class IChainRule;

/**
 * @brief Directed idiom graph used by the chain game.
 */
class IdiomGraph {
public:
    bool build(const IIdiomRepository& repository, const IChainRule& rule);

    const IdiomEntry* getEntry(int id) const;
    int getIdByWord(const std::string& wordOrAbbreviation) const;
    const std::vector<int>& getNextIds(int id) const;
    const std::vector<int>& getPrevIds(int id) const;
    const std::vector<IdiomEntry>& getAllEntries() const;
    std::size_t size() const;

private:
    std::vector<IdiomEntry> entries_;
    std::unordered_map<std::string, int> wordToId_;
    std::unordered_map<std::string, int> abbreviationToId_;
    std::vector<std::vector<int>> adjacency_;
    std::vector<std::vector<int>> reverseAdjacency_;
};

#endif
