#pragma once

#include <filesystem>
#include <iosfwd>
#include <string>
#include <unordered_map>
#include <vector>

#include "../common/Types.h"
#include "../model/PhraseInfo.h"
#include "Trie.h"

namespace lineverse::poetryrebuild {

class IndexStore {
public:
    using PhraseIdList = std::vector<std::string>;
    using LengthBuckets = std::unordered_map<int, PhraseIdList>;
    using TypeBuckets = std::unordered_map<std::string, PhraseIdList>;
    using CharToPhraseIDs = std::unordered_map<CodePoint, PhraseIdList>;
    using ConflictGraph = std::unordered_map<std::string, PhraseIdList>;

    void build(const std::vector<PhraseInfo>& poems, const std::vector<PhraseInfo>& idioms);
    void clear();

    bool empty() const;
    std::size_t phraseCount() const;

    bool hasText(const std::string& text) const;
    const std::string* getIdByText(const std::string& text) const;
    const PhraseInfo* getPhraseById(const std::string& id) const;

    PhraseIdList getPhraseIdsByChar(CodePoint cp) const;
    PhraseIdList getPhraseIdsByLength(int length) const;
    PhraseIdList getPhraseIdsByType(PhraseType type) const;
    PhraseIdList getConflictIds(const std::string& id) const;

    bool containsPhrase(const std::string& text) const;
    bool isValidPrefix(const std::string& prefix) const;
    std::vector<std::string> completePrefix(const std::string& prefix, std::size_t limit = 10) const;

    const Trie& trie() const;
    const LengthBuckets& lengthBuckets() const;
    const TypeBuckets& typeBuckets() const;
    const CharToPhraseIDs& charToPhraseIDs() const;
    const ConflictGraph& conflictGraph() const;

    std::size_t skippedDuplicateTextCount() const;
    const std::vector<std::string>& duplicateTextSamples() const;
    bool conflictGraphBuilt() const;

    void writeIndexReport(const std::filesystem::path& outFile) const;

    // runtime cache snapshot
    void saveSnapshot(std::ostream& out) const;
    void loadSnapshot(std::istream& in);

private:
    std::unordered_map<std::string, PhraseInfo> phraseMap_;
    std::unordered_map<std::string, std::string> textToId_;
    CharToPhraseIDs charToPhraseIDs_;
    LengthBuckets lengthBuckets_;
    TypeBuckets typeBuckets_;
    ConflictGraph conflictGraph_;
    Trie trie_;

    std::size_t skippedDuplicateTextCount_ = 0;
    std::vector<std::string> duplicateTextSamples_;
    bool conflictGraphBuilt_ = false;

    void addPhrase(const PhraseInfo& phrase);
    void buildConflictGraph();
    static bool sharesAnyChar(const PhraseInfo& a, const PhraseInfo& b);
    static void dedupeAndSort(PhraseIdList& ids);
};

} // namespace lineverse::poetryrebuild