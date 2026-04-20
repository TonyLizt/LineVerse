#include "PoetryIndex.h"

#include "../utils/StringNormalizer.h"

namespace {
const std::vector<int>* findOrNull(const std::unordered_map<std::string, std::vector<int>>& mp,
                                   const std::string& key) {
    auto it = mp.find(key);
    return (it == mp.end()) ? nullptr : &it->second;
}
}

void PoetryIndex::build(const std::vector<Poem>& poems) {
    dynastyIndex.clear();
    authorIndex.clear();
    typeIndex.clear();
    emotionIndex.clear();
    backgroundIndex.clear();
    featureIndex.clear();
    imageryIndex.clear();
    prefixIndex.clear();
    titleIndex.clear();
    structureIndex.clear();
    difficultyIndex.clear();

    for (int i = 0; i < static_cast<int>(poems.size()); ++i) {
        const Poem& p = poems[i];

        if (!p.dynastyStd.empty()) dynastyIndex[p.dynastyStd].push_back(i);
        if (!p.authorStd.empty()) authorIndex[p.authorStd].push_back(i);
        if (!p.typeStd.empty()) typeIndex[p.typeStd].push_back(i);
        if (!p.emotionCoreStd.empty()) emotionIndex[p.emotionCoreStd].push_back(i);
        if (!p.bgCoreStd.empty()) backgroundIndex[p.bgCoreStd].push_back(i);
        if (!p.featureCoreStd.empty()) featureIndex[p.featureCoreStd].push_back(i);
        if (!p.firstlinePrefixStd.empty()) prefixIndex[p.firstlinePrefixStd].push_back(i);
        if (!p.titleStd.empty()) titleIndex[p.titleStd].push_back(i);

        std::string structureKey = std::to_string(p.sentenceCount) + "|" + std::to_string(p.charCount);
        structureIndex[structureKey].push_back(i);
        difficultyIndex[p.difficulty].push_back(i);

        if (!p.mainImageryOneStd.empty()) {
            imageryIndex[p.mainImageryOneStd].push_back(i);
        }
        for (const auto& img : p.imageryGameAllStd) {
            if (!img.empty()) imageryIndex[img].push_back(i);
        }
        for (const auto& img : p.imageryStd) {
            if (!img.empty()) imageryIndex[img].push_back(i);
        }
    }
}

const std::vector<int>* PoetryIndex::queryByDynasty(const std::string& dynasty) const {
    return findOrNull(dynastyIndex, StringNormalizer::normalizeTag(dynasty));
}

const std::vector<int>* PoetryIndex::queryByAuthor(const std::string& author) const {
    return findOrNull(authorIndex, StringNormalizer::normalizeTag(author));
}

const std::vector<int>* PoetryIndex::queryByType(const std::string& type) const {
    return findOrNull(typeIndex, StringNormalizer::normalizeTag(type));
}

const std::vector<int>* PoetryIndex::queryByEmotion(const std::string& emotionCore) const {
    return findOrNull(emotionIndex, StringNormalizer::normalizeTag(emotionCore));
}

const std::vector<int>* PoetryIndex::queryByBackground(const std::string& bgCore) const {
    return findOrNull(backgroundIndex, StringNormalizer::normalizeTag(bgCore));
}

const std::vector<int>* PoetryIndex::queryByFeature(const std::string& featureCore) const {
    return findOrNull(featureIndex, StringNormalizer::normalizeTag(featureCore));
}

const std::vector<int>* PoetryIndex::queryByImagery(const std::string& imagery) const {
    return findOrNull(imageryIndex, StringNormalizer::normalizeTag(imagery));
}

const std::vector<int>* PoetryIndex::queryByPrefix(const std::string& prefix) const {
    return findOrNull(prefixIndex, StringNormalizer::normalizeContent(prefix));
}

const std::vector<int>* PoetryIndex::queryByTitle(const std::string& title) const {
    return findOrNull(titleIndex, StringNormalizer::normalizeTitle(title));
}

const std::vector<int>* PoetryIndex::queryByStructure(const std::string& structure) const {
    return findOrNull(structureIndex, structure);
}

const std::vector<int>* PoetryIndex::queryByDifficulty(int difficulty) const {
    auto it = difficultyIndex.find(difficulty);
    return (it == difficultyIndex.end()) ? nullptr : &it->second;
}
