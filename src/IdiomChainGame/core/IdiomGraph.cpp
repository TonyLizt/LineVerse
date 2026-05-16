#include "IdiomGraph.h"

#include "../data/IIdiomRepository.h"
#include "ChainRule.h"

#include <algorithm>
#include <unordered_map>

bool IdiomGraph::build(const IIdiomRepository& repository, const IChainRule& rule) {
    // 从词库里把所有成语读出来
    entries_ = repository.getAll();
    wordToId_.clear();
    abbreviationToId_.clear();
    adjacency_.clear();
    reverseAdjacency_.clear();

    if (entries_.empty()) {
        return false;
    }
    // 给每个成语分配一个 编号 id
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        entries_[i].id = static_cast<int>(i);
        entries_[i].firstKey = rule.extractFirstKey(entries_[i].pinyin);
        entries_[i].lastKey = rule.extractLastKey(entries_[i].pinyin);
        wordToId_[entries_[i].word] = static_cast<int>(i);
        if (!entries_[i].abbreviation.empty()) {
            abbreviationToId_[entries_[i].abbreviation] = static_cast<int>(i);
        }
    }
    // 建立两个邻接表
    adjacency_.assign(entries_.size(), {});
    reverseAdjacency_.assign(entries_.size(), {});
    // 把所有成语按 首拼音 key 分类
    // 对于某个成语 entry
    // 只需要看 bucketByFirstKey[entry.lastKey]
    // 就能直接找到所有可能接在它后面的成语
    std::unordered_map<std::string, std::vector<int>> bucketByFirstKey;
    for (const IdiomEntry& entry : entries_) {
        bucketByFirstKey[entry.firstKey].push_back(entry.id);
    }

    for (const IdiomEntry& entry : entries_) {
        // 找当前成语能接到哪些成语
        auto it = bucketByFirstKey.find(entry.lastKey);
        if (it == bucketByFirstKey.end()) {
            continue;
        }
        // 建边
        std::vector<int>& nextIds = adjacency_[static_cast<std::size_t>(entry.id)];
        for (int nextId : it->second) {
            if (nextId != entry.id) {
                nextIds.push_back(nextId);
            }
        }
        // 去重
        std::sort(nextIds.begin(), nextIds.end());
        nextIds.erase(std::unique(nextIds.begin(), nextIds.end()), nextIds.end());
        // 建立反向边
        for (int nextId : nextIds) {
            reverseAdjacency_[static_cast<std::size_t>(nextId)].push_back(entry.id);
        }
    }
    // 对反向邻接表也去重
    for (std::vector<int>& prevIds : reverseAdjacency_) {
        std::sort(prevIds.begin(), prevIds.end());
        prevIds.erase(std::unique(prevIds.begin(), prevIds.end()), prevIds.end());
    }

    return true;
}

const IdiomEntry* IdiomGraph::getEntry(int id) const {
    if (id < 0 || static_cast<std::size_t>(id) >= entries_.size()) {
        return nullptr;
    }
    return &entries_[static_cast<std::size_t>(id)];
}

int IdiomGraph::getIdByWord(const std::string& wordOrAbbreviation) const {
    auto it = wordToId_.find(wordOrAbbreviation);
    if (it != wordToId_.end()) {
        return it->second;
    }
    it = abbreviationToId_.find(wordOrAbbreviation);
    if (it != abbreviationToId_.end()) {
        return it->second;
    }
    return -1;
}

const std::vector<int>& IdiomGraph::getNextIds(int id) const {
    static const std::vector<int> kEmpty;
    if (id < 0 || static_cast<std::size_t>(id) >= adjacency_.size()) {
        return kEmpty;
    }
    return adjacency_[static_cast<std::size_t>(id)];
}

const std::vector<int>& IdiomGraph::getPrevIds(int id) const {
    static const std::vector<int> kEmpty;
    if (id < 0 || static_cast<std::size_t>(id) >= reverseAdjacency_.size()) {
        return kEmpty;
    }
    return reverseAdjacency_[static_cast<std::size_t>(id)];
}

const std::vector<IdiomEntry>& IdiomGraph::getAllEntries() const {
    return entries_;
}

std::size_t IdiomGraph::size() const {
    return entries_.size();
}
