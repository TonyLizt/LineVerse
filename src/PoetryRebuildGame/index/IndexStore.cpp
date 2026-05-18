#include "IndexStore.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>

#include <cstdint>
#include <istream>
#include <ostream>
#include <stdexcept>

#include "../common/Json.h"
#include "../util/Utf8.h"

namespace lineverse::poetryrebuild {

    namespace {

void writeString(std::ostream& out, const std::string& s) {
    const std::uint64_t n = static_cast<std::uint64_t>(s.size());
    out.write(reinterpret_cast<const char*>(&n), sizeof(n));
    if (n > 0) {
        out.write(s.data(), static_cast<std::streamsize>(n));
    }
}

std::string readString(std::istream& in) {
    std::uint64_t n = 0;
    in.read(reinterpret_cast<char*>(&n), sizeof(n));

    std::string s(n, '\0');
    if (n > 0) {
        in.read(s.data(), static_cast<std::streamsize>(n));
    }
    return s;
}

void writePhraseIdList(std::ostream& out, const IndexStore::PhraseIdList& ids) {
    const std::uint64_t n = static_cast<std::uint64_t>(ids.size());
    out.write(reinterpret_cast<const char*>(&n), sizeof(n));
    for (const auto& id : ids) {
        writeString(out, id);
    }
}

IndexStore::PhraseIdList readPhraseIdList(std::istream& in) {
    std::uint64_t n = 0;
    in.read(reinterpret_cast<char*>(&n), sizeof(n));

    IndexStore::PhraseIdList ids;
    ids.reserve(static_cast<std::size_t>(n));
    for (std::uint64_t i = 0; i < n; ++i) {
        ids.push_back(readString(in));
    }
    return ids;
}

void writeCharFreq(std::ostream& out, const CharFreq& freq) {
    const std::uint64_t n = static_cast<std::uint64_t>(freq.size());
    out.write(reinterpret_cast<const char*>(&n), sizeof(n));

    for (const auto& [cp, count] : freq) {
        const std::int32_t codepoint = static_cast<std::int32_t>(cp);
        const std::int32_t c = static_cast<std::int32_t>(count);
        out.write(reinterpret_cast<const char*>(&codepoint), sizeof(codepoint));
        out.write(reinterpret_cast<const char*>(&c), sizeof(c));
    }
}

CharFreq readCharFreq(std::istream& in) {
    std::uint64_t n = 0;
    in.read(reinterpret_cast<char*>(&n), sizeof(n));

    CharFreq freq;
    for (std::uint64_t i = 0; i < n; ++i) {
        std::int32_t codepoint = 0;
        std::int32_t count = 0;
        in.read(reinterpret_cast<char*>(&codepoint), sizeof(codepoint));
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        freq[static_cast<CodePoint>(codepoint)] = count;
    }
    return freq;
}

void writePhraseInfo(std::ostream& out, const PhraseInfo& info) {
    writeString(out, info.id);
    writeString(out, info.text);
    writeString(out, info.author);
    writeString(out, info.title);
    writeString(out, info.pinyin);
    writeString(out, info.explanation);
    writeString(out, info.derivation);
    writeString(out, info.example);
    writeString(out, info.sourceFile);

    const std::int32_t type = static_cast<std::int32_t>(info.type);
    const std::int32_t length = static_cast<std::int32_t>(info.length);
    const std::int32_t rarity = static_cast<std::int32_t>(info.rarity);
    const std::int32_t scoreWeight = static_cast<std::int32_t>(info.scoreWeight);

    out.write(reinterpret_cast<const char*>(&type), sizeof(type));
    out.write(reinterpret_cast<const char*>(&length), sizeof(length));
    out.write(reinterpret_cast<const char*>(&rarity), sizeof(rarity));
    out.write(reinterpret_cast<const char*>(&scoreWeight), sizeof(scoreWeight));

    writeCharFreq(out, info.charFreq);
}

PhraseInfo readPhraseInfo(std::istream& in) {
    PhraseInfo info;

    info.id = readString(in);
    info.text = readString(in);
    info.author = readString(in);
    info.title = readString(in);
    info.pinyin = readString(in);
    info.explanation = readString(in);
    info.derivation = readString(in);
    info.example = readString(in);
    info.sourceFile = readString(in);

    std::int32_t type = 0;
    std::int32_t length = 0;
    std::int32_t rarity = 0;
    std::int32_t scoreWeight = 0;

    in.read(reinterpret_cast<char*>(&type), sizeof(type));
    in.read(reinterpret_cast<char*>(&length), sizeof(length));
    in.read(reinterpret_cast<char*>(&rarity), sizeof(rarity));
    in.read(reinterpret_cast<char*>(&scoreWeight), sizeof(scoreWeight));

    info.type = static_cast<PhraseType>(type);
    info.length = length;
    info.rarity = rarity;
    info.scoreWeight = scoreWeight;
    info.charFreq = readCharFreq(in);

    return info;
}

void writePhraseMap(std::ostream& out, const std::unordered_map<std::string, PhraseInfo>& phraseMap) {
    const std::uint64_t n = static_cast<std::uint64_t>(phraseMap.size());
    out.write(reinterpret_cast<const char*>(&n), sizeof(n));

    for (const auto& [id, info] : phraseMap) {
        writeString(out, id);
        writePhraseInfo(out, info);
    }
}

std::unordered_map<std::string, PhraseInfo> readPhraseMap(std::istream& in) {
    std::uint64_t n = 0;
    in.read(reinterpret_cast<char*>(&n), sizeof(n));

    std::unordered_map<std::string, PhraseInfo> phraseMap;
    phraseMap.reserve(static_cast<std::size_t>(n));

    for (std::uint64_t i = 0; i < n; ++i) {
        const std::string id = readString(in);
        PhraseInfo info = readPhraseInfo(in);
        phraseMap.emplace(id, std::move(info));
    }

    return phraseMap;
}

void writeTextToId(std::ostream& out, const std::unordered_map<std::string, std::string>& textToId) {
    const std::uint64_t n = static_cast<std::uint64_t>(textToId.size());
    out.write(reinterpret_cast<const char*>(&n), sizeof(n));

    for (const auto& [text, id] : textToId) {
        writeString(out, text);
        writeString(out, id);
    }
}

std::unordered_map<std::string, std::string> readTextToId(std::istream& in) {
    std::uint64_t n = 0;
    in.read(reinterpret_cast<char*>(&n), sizeof(n));

    std::unordered_map<std::string, std::string> textToId;
    textToId.reserve(static_cast<std::size_t>(n));

    for (std::uint64_t i = 0; i < n; ++i) {
        const std::string text = readString(in);
        const std::string id = readString(in);
        textToId.emplace(text, id);
    }

    return textToId;
}

void writeCharToPhraseIDs(std::ostream& out, const IndexStore::CharToPhraseIDs& buckets) {
    const std::uint64_t n = static_cast<std::uint64_t>(buckets.size());
    out.write(reinterpret_cast<const char*>(&n), sizeof(n));

    for (const auto& [cp, ids] : buckets) {
        const std::int32_t codepoint = static_cast<std::int32_t>(cp);
        out.write(reinterpret_cast<const char*>(&codepoint), sizeof(codepoint));
        writePhraseIdList(out, ids);
    }
}

IndexStore::CharToPhraseIDs readCharToPhraseIDs(std::istream& in) {
    std::uint64_t n = 0;
    in.read(reinterpret_cast<char*>(&n), sizeof(n));

    IndexStore::CharToPhraseIDs buckets;
    buckets.reserve(static_cast<std::size_t>(n));

    for (std::uint64_t i = 0; i < n; ++i) {
        std::int32_t codepoint = 0;
        in.read(reinterpret_cast<char*>(&codepoint), sizeof(codepoint));
        buckets.emplace(static_cast<CodePoint>(codepoint), readPhraseIdList(in));
    }

    return buckets;
}

void writeLengthBuckets(std::ostream& out, const IndexStore::LengthBuckets& buckets) {
    const std::uint64_t n = static_cast<std::uint64_t>(buckets.size());
    out.write(reinterpret_cast<const char*>(&n), sizeof(n));

    for (const auto& [length, ids] : buckets) {
        const std::int32_t len = static_cast<std::int32_t>(length);
        out.write(reinterpret_cast<const char*>(&len), sizeof(len));
        writePhraseIdList(out, ids);
    }
}

IndexStore::LengthBuckets readLengthBuckets(std::istream& in) {
    std::uint64_t n = 0;
    in.read(reinterpret_cast<char*>(&n), sizeof(n));

    IndexStore::LengthBuckets buckets;
    buckets.reserve(static_cast<std::size_t>(n));

    for (std::uint64_t i = 0; i < n; ++i) {
        std::int32_t len = 0;
        in.read(reinterpret_cast<char*>(&len), sizeof(len));
        buckets.emplace(static_cast<int>(len), readPhraseIdList(in));
    }

    return buckets;
}

void writeTypeBuckets(std::ostream& out, const IndexStore::TypeBuckets& buckets) {
    const std::uint64_t n = static_cast<std::uint64_t>(buckets.size());
    out.write(reinterpret_cast<const char*>(&n), sizeof(n));

    for (const auto& [typeKey, ids] : buckets) {
        writeString(out, typeKey);
        writePhraseIdList(out, ids);
    }
}

IndexStore::TypeBuckets readTypeBuckets(std::istream& in) {
    std::uint64_t n = 0;
    in.read(reinterpret_cast<char*>(&n), sizeof(n));

    IndexStore::TypeBuckets buckets;
    buckets.reserve(static_cast<std::size_t>(n));

    for (std::uint64_t i = 0; i < n; ++i) {
        const std::string key = readString(in);
        buckets.emplace(key, readPhraseIdList(in));
    }

    return buckets;
}

void writeConflictGraph(std::ostream& out, const IndexStore::ConflictGraph& graph) {
    const std::uint64_t n = static_cast<std::uint64_t>(graph.size());
    out.write(reinterpret_cast<const char*>(&n), sizeof(n));

    for (const auto& [id, ids] : graph) {
        writeString(out, id);
        writePhraseIdList(out, ids);
    }
}

IndexStore::ConflictGraph readConflictGraph(std::istream& in) {
    std::uint64_t n = 0;
    in.read(reinterpret_cast<char*>(&n), sizeof(n));

    IndexStore::ConflictGraph graph;
    graph.reserve(static_cast<std::size_t>(n));

    for (std::uint64_t i = 0; i < n; ++i) {
        const std::string id = readString(in);
        graph.emplace(id, readPhraseIdList(in));
    }

    return graph;
}

void writeStringVector(std::ostream& out, const std::vector<std::string>& values) {
    const std::uint64_t n = static_cast<std::uint64_t>(values.size());
    out.write(reinterpret_cast<const char*>(&n), sizeof(n));

    for (const auto& value : values) {
        writeString(out, value);
    }
}

std::vector<std::string> readStringVector(std::istream& in) {
    std::uint64_t n = 0;
    in.read(reinterpret_cast<char*>(&n), sizeof(n));

    std::vector<std::string> values;
    values.reserve(static_cast<std::size_t>(n));

    for (std::uint64_t i = 0; i < n; ++i) {
        values.push_back(readString(in));
    }

    return values;
}

} // namespace

void IndexStore::build(const std::vector<PhraseInfo>& poems, const std::vector<PhraseInfo>& idioms) {
    clear();

    for (const auto& phrase : poems) {
        addPhrase(phrase);
    }
    for (const auto& phrase : idioms) {
        addPhrase(phrase);
    }

    for (auto& [_, ids] : charToPhraseIDs_) {
        dedupeAndSort(ids);
    }
    for (auto& [_, ids] : lengthBuckets_) {
        dedupeAndSort(ids);
    }
    for (auto& [_, ids] : typeBuckets_) {
        dedupeAndSort(ids);
    }

    constexpr std::size_t kMaxPhraseCountForConflictGraph = 20000;
    if (phraseMap_.size() <= kMaxPhraseCountForConflictGraph) {
        buildConflictGraph();
        conflictGraphBuilt_ = true;
    } else {
        conflictGraphBuilt_ = false;
    }
}

void IndexStore::clear() {
    phraseMap_.clear();
    textToId_.clear();
    charToPhraseIDs_.clear();
    lengthBuckets_.clear();
    typeBuckets_.clear();
    conflictGraph_.clear();
    trie_.clear();

    skippedDuplicateTextCount_ = 0;
    duplicateTextSamples_.clear();
    conflictGraphBuilt_ = false;
}

bool IndexStore::empty() const {
    return phraseMap_.empty();
}

std::size_t IndexStore::phraseCount() const {
    return phraseMap_.size();
}

bool IndexStore::hasText(const std::string& text) const {
    return textToId_.find(text) != textToId_.end();
}

const std::string* IndexStore::getIdByText(const std::string& text) const {
    const auto it = textToId_.find(text);
    if (it == textToId_.end()) {
        return nullptr;
    }
    return &it->second;
}

const PhraseInfo* IndexStore::getPhraseById(const std::string& id) const {
    const auto it = phraseMap_.find(id);
    if (it == phraseMap_.end()) {
        return nullptr;
    }
    return &it->second;
}

IndexStore::PhraseIdList IndexStore::getPhraseIdsByChar(CodePoint cp) const {
    const auto it = charToPhraseIDs_.find(cp);
    return it == charToPhraseIDs_.end() ? PhraseIdList{} : it->second;
}

IndexStore::PhraseIdList IndexStore::getPhraseIdsByLength(int length) const {
    const auto it = lengthBuckets_.find(length);
    return it == lengthBuckets_.end() ? PhraseIdList{} : it->second;
}

IndexStore::PhraseIdList IndexStore::getPhraseIdsByType(PhraseType type) const {
    const auto it = typeBuckets_.find(toString(type));
    return it == typeBuckets_.end() ? PhraseIdList{} : it->second;
}

IndexStore::PhraseIdList IndexStore::getConflictIds(const std::string& id) const {
    const auto it = conflictGraph_.find(id);
    return it == conflictGraph_.end() ? PhraseIdList{} : it->second;
}

bool IndexStore::containsPhrase(const std::string& text) const {
    return trie_.contains(text);
}

bool IndexStore::isValidPrefix(const std::string& prefix) const {
    return trie_.isValidPrefix(prefix);
}

std::vector<std::string> IndexStore::completePrefix(const std::string& prefix, std::size_t limit) const {
    return trie_.complete(prefix, limit);
}

const Trie& IndexStore::trie() const {
    return trie_;
}

const IndexStore::LengthBuckets& IndexStore::lengthBuckets() const {
    return lengthBuckets_;
}

const IndexStore::TypeBuckets& IndexStore::typeBuckets() const {
    return typeBuckets_;
}

const IndexStore::CharToPhraseIDs& IndexStore::charToPhraseIDs() const {
    return charToPhraseIDs_;
}

const IndexStore::ConflictGraph& IndexStore::conflictGraph() const {
    return conflictGraph_;
}

std::size_t IndexStore::skippedDuplicateTextCount() const {
    return skippedDuplicateTextCount_;
}

const std::vector<std::string>& IndexStore::duplicateTextSamples() const {
    return duplicateTextSamples_;
}

bool IndexStore::conflictGraphBuilt() const {
    return conflictGraphBuilt_;
}

void IndexStore::writeIndexReport(const std::filesystem::path& outFile) const {
    Json report = Json::object();
    report["phraseCount"] = static_cast<int>(phraseMap_.size());
    report["textToIdCount"] = static_cast<int>(textToId_.size());
    report["charKeyCount"] = static_cast<int>(charToPhraseIDs_.size());
    report["lengthBucketCount"] = static_cast<int>(lengthBuckets_.size());
    report["typeBucketCount"] = static_cast<int>(typeBuckets_.size());
    report["trieWordCount"] = static_cast<int>(trie_.size());
    report["skippedDuplicateTextCount"] = static_cast<int>(skippedDuplicateTextCount_);
    report["conflictGraphBuilt"] = conflictGraphBuilt_;

    Json duplicateSamples = Json::array();
    for (const auto& text : duplicateTextSamples_) {
        duplicateSamples.push_back(text);
    }
    report["duplicateTextSamples"] = duplicateSamples;

    Json lengths = Json::object();
    for (const auto& [length, ids] : lengthBuckets_) {
        lengths[std::to_string(length)] = static_cast<int>(ids.size());
    }
    report["lengthBuckets"] = lengths;

    Json types = Json::object();
    for (const auto& [type, ids] : typeBuckets_) {
        types[type] = static_cast<int>(ids.size());
    }
    report["typeBuckets"] = types;

    Json topChars = Json::array();
    std::vector<std::pair<CodePoint, int>> charStats;
    charStats.reserve(charToPhraseIDs_.size());
    for (const auto& [cp, ids] : charToPhraseIDs_) {
        charStats.emplace_back(cp, static_cast<int>(ids.size()));
    }
    std::sort(charStats.begin(), charStats.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.second != rhs.second) {
            return lhs.second > rhs.second;
        }
        return lhs.first < rhs.first;
    });
    const std::size_t charLimit = std::min<std::size_t>(20, charStats.size());
    for (std::size_t i = 0; i < charLimit; ++i) {
        topChars.push_back({
            {"char", Utf8::fromCodePoint(charStats[i].first)},
            {"phraseCount", charStats[i].second}
        });
    }
    report["topChars"] = topChars;

    Json topConflicts = Json::array();
    if (conflictGraphBuilt_) {
        std::vector<std::pair<std::string, int>> conflictStats;
        conflictStats.reserve(conflictGraph_.size());
        for (const auto& [id, ids] : conflictGraph_) {
            conflictStats.emplace_back(id, static_cast<int>(ids.size()));
        }
        std::sort(conflictStats.begin(), conflictStats.end(), [](const auto& lhs, const auto& rhs) {
            if (lhs.second != rhs.second) {
                return lhs.second > rhs.second;
            }
            return lhs.first < rhs.first;
        });
        const std::size_t conflictLimit = std::min<std::size_t>(20, conflictStats.size());
        for (std::size_t i = 0; i < conflictLimit; ++i) {
            const auto* phrase = getPhraseById(conflictStats[i].first);
            topConflicts.push_back({
                {"id", conflictStats[i].first},
                {"text", phrase == nullptr ? "" : phrase->text},
                {"degree", conflictStats[i].second}
            });
        }
    }
    report["topConflictPhrases"] = topConflicts;

    std::ofstream out(outFile, std::ios::binary);
    out << report.dump(2, ' ', true);
}

void IndexStore::saveSnapshot(std::ostream& out) const {
    if (!out) {
        throw std::runtime_error("IndexStore::saveSnapshot output stream is not writable");
    }

    writePhraseMap(out, phraseMap_);
    writeTextToId(out, textToId_);
    writeCharToPhraseIDs(out, charToPhraseIDs_);
    writeLengthBuckets(out, lengthBuckets_);
    writeTypeBuckets(out, typeBuckets_);
    writeConflictGraph(out, conflictGraph_);
    trie_.saveSnapshot(out);

    const std::uint64_t skipped = static_cast<std::uint64_t>(skippedDuplicateTextCount_);
    out.write(reinterpret_cast<const char*>(&skipped), sizeof(skipped));

    writeStringVector(out, duplicateTextSamples_);

    const std::uint8_t conflictBuilt = conflictGraphBuilt_ ? 1 : 0;
    out.write(reinterpret_cast<const char*>(&conflictBuilt), sizeof(conflictBuilt));

    if (!out) {
        throw std::runtime_error("IndexStore::saveSnapshot failed while writing snapshot");
    }
}

void IndexStore::loadSnapshot(std::istream& in) {
    if (!in) {
        throw std::runtime_error("IndexStore::loadSnapshot input stream is not readable");
    }

    clear();

    phraseMap_ = readPhraseMap(in);
    textToId_ = readTextToId(in);
    charToPhraseIDs_ = readCharToPhraseIDs(in);
    lengthBuckets_ = readLengthBuckets(in);
    typeBuckets_ = readTypeBuckets(in);
    conflictGraph_ = readConflictGraph(in);
    trie_.loadSnapshot(in);

    std::uint64_t skipped = 0;
    in.read(reinterpret_cast<char*>(&skipped), sizeof(skipped));
    skippedDuplicateTextCount_ = static_cast<std::size_t>(skipped);

    duplicateTextSamples_ = readStringVector(in);

    std::uint8_t conflictBuilt = 0;
    in.read(reinterpret_cast<char*>(&conflictBuilt), sizeof(conflictBuilt));
    conflictGraphBuilt_ = (conflictBuilt != 0);

    if (!in) {
        clear();
        throw std::runtime_error("IndexStore::loadSnapshot failed while reading snapshot");
    }
}

void IndexStore::addPhrase(const PhraseInfo& phrase) {
    if (phrase.id.empty() || phrase.text.empty()) {
        return;
    }

    if (phraseMap_.find(phrase.id) != phraseMap_.end()) {
        throw std::runtime_error("IndexStore build failed: duplicated phrase id: " + phrase.id);
    }

    auto textIt = textToId_.find(phrase.text);
    if (textIt != textToId_.end()) {
        ++skippedDuplicateTextCount_;

        if (duplicateTextSamples_.size() < 20) {
            const bool alreadyRecorded = std::find(
                duplicateTextSamples_.begin(),
                duplicateTextSamples_.end(),
                phrase.text
            ) != duplicateTextSamples_.end();

            if (!alreadyRecorded) {
                duplicateTextSamples_.push_back(phrase.text);
            }
        }
        return;
    }

    phraseMap_.emplace(phrase.id, phrase);
    textToId_.emplace(phrase.text, phrase.id);
    lengthBuckets_[phrase.length].push_back(phrase.id);
    typeBuckets_[toString(phrase.type)].push_back(phrase.id);
    trie_.insert(phrase.text);

    for (const auto& [cp, _] : phrase.charFreq) {
        charToPhraseIDs_[cp].push_back(phrase.id);
    }
}

void IndexStore::buildConflictGraph() {
    for (const auto& [id, _] : phraseMap_) {
        conflictGraph_[id];
    }

    std::vector<const PhraseInfo*> phrases;
    phrases.reserve(phraseMap_.size());
    for (const auto& [id, phrase] : phraseMap_) {
        (void)id;
        phrases.push_back(&phrase);
    }

    for (std::size_t i = 0; i < phrases.size(); ++i) {
        for (std::size_t j = i + 1; j < phrases.size(); ++j) {
            if (!sharesAnyChar(*phrases[i], *phrases[j])) {
                continue;
            }
            conflictGraph_[phrases[i]->id].push_back(phrases[j]->id);
            conflictGraph_[phrases[j]->id].push_back(phrases[i]->id);
        }
    }

    for (auto& [_, ids] : conflictGraph_) {
        dedupeAndSort(ids);
    }
}

bool IndexStore::sharesAnyChar(const PhraseInfo& a, const PhraseInfo& b) {
    const PhraseInfo* smaller = &a;
    const PhraseInfo* bigger = &b;
    if (a.charFreq.size() > b.charFreq.size()) {
        smaller = &b;
        bigger = &a;
    }

    for (const auto& [cp, _] : smaller->charFreq) {
        if (bigger->charFreq.find(cp) != bigger->charFreq.end()) {
            return true;
        }
    }
    return false;
}

void IndexStore::dedupeAndSort(PhraseIdList& ids) {
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
}

} // namespace lineverse::poetryrebuild