#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "model/Poem.h"

class PoetryIndex {
public:
    void build(const std::vector<Poem>& poems);

    const std::vector<int>* queryByDynasty(const std::string& dynasty) const;
    const std::vector<int>* queryByAuthor(const std::string& author) const;
    const std::vector<int>* queryByType(const std::string& type) const;
    const std::vector<int>* queryByEmotion(const std::string& emotionCore) const;
    const std::vector<int>* queryByBackground(const std::string& bgCore) const;
    const std::vector<int>* queryByFeature(const std::string& featureCore) const;
    const std::vector<int>* queryByImagery(const std::string& imagery) const;
    const std::vector<int>* queryByPrefix(const std::string& prefix) const;
    const std::vector<int>* queryByTitle(const std::string& title) const;
    const std::vector<int>* queryByStructure(const std::string& structure) const;
    const std::vector<int>* queryByDifficulty(int difficulty) const;

private:
    std::unordered_map<std::string, std::vector<int>> dynastyIndex;
    std::unordered_map<std::string, std::vector<int>> authorIndex;
    std::unordered_map<std::string, std::vector<int>> typeIndex;
    std::unordered_map<std::string, std::vector<int>> emotionIndex;
    std::unordered_map<std::string, std::vector<int>> backgroundIndex;
    std::unordered_map<std::string, std::vector<int>> featureIndex;
    std::unordered_map<std::string, std::vector<int>> imageryIndex;
    std::unordered_map<std::string, std::vector<int>> prefixIndex;
    std::unordered_map<std::string, std::vector<int>> titleIndex;
    std::unordered_map<std::string, std::vector<int>> structureIndex;
    std::unordered_map<int, std::vector<int>> difficultyIndex;
};
