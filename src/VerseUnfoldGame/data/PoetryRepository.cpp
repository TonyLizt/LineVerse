#include "PoetryRepository.h"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>

#include "../utils/StringNormalizer.h"

using json = nlohmann::json;

bool PoetryRepository::loadFromJson(const std::string& filePath) {
    poems.clear();

    std::ifstream fin(filePath);
    if (!fin.is_open()) {
        std::cerr << "Failed to open poetry database: " << filePath << std::endl;
        return false;
    }

    json root;
    fin >> root;

    if (!root.is_array()) {
        std::cerr << "Poetry database format error: root is not array." << std::endl;
        return false;
    }

    for (const auto& item : root) {
        Poem poem;

        poem.id = item.value("id", 0);
        poem.title = item.value("title", "");
        poem.author = item.value("author", "");
        poem.dynasty = item.value("dynasty", "");
        poem.type = item.value("type", "");
        poem.difficulty = item.value("difficulty", 1);

        poem.emotion = item.value("emotion", "");
        poem.creationBg = item.value("creation_bg", "");
        poem.distinctFeature = item.value("distinct_feature", "");
        poem.authorTag = item.value("author_tag", "");
        poem.rhythm = item.value("rhythm", "");

        poem.sentenceCount = item.value("sentence_count", 0);
        poem.charCount = item.value("char_count", 0);

        auto pushLineIfValid = [&](const std::string& rawText) {
            std::string cleaned = trim(rawText);
            if (cleaned.empty()) {
                return;
            }

            std::string cleanedStd = StringNormalizer::normalizeContent(cleaned);
            if (cleanedStd.empty()) {
                return;
            }

            for (const auto& existingStd : poem.linesStd) {
                if (existingStd == cleanedStd) {
                    return;
                }
            }

            poem.lines.push_back(cleaned);
            poem.linesStd.push_back(cleanedStd);
        };

        if (item.contains("content") && item["content"].is_array()) {
            for (const auto& lineNode : item["content"]) {
                std::string rawLine = lineNode.get<std::string>();
                poem.content.push_back(rawLine);
                pushLineIfValid(rawLine);

                std::vector<std::string> parts = splitPoemIntoLines(rawLine);
                for (const auto& part : parts) {
                    pushLineIfValid(part);
                }
            }

            for (size_t i = 0; i + 1 < poem.content.size(); ++i) {
                std::string merged = poem.content[i] + poem.content[i + 1];
                pushLineIfValid(merged);
            }
        }

        if (item.contains("imagery") && item["imagery"].is_array()) {
            for (const auto& x : item["imagery"]) {
                std::string value = x.get<std::string>();
                poem.imagery.push_back(value);
                poem.imageryStd.push_back(StringNormalizer::normalizeTag(value));
            }
        }

        poem.emotionCore = item.value("emotion_core", "");
        poem.bgCore = item.value("bg_core", "");
        poem.mainImageryOne = item.value("main_imagery_one", "");
        poem.featureCore = item.value("feature_core", "");
        poem.firstlinePrefix = item.value("firstline_prefix", "");

        if (item.contains("imagery_game_all") && item["imagery_game_all"].is_array()) {
            for (const auto& x : item["imagery_game_all"]) {
                std::string value = x.get<std::string>();
                poem.imageryGameAll.push_back(value);
                poem.imageryGameAllStd.push_back(StringNormalizer::normalizeTag(value));
            }
        }

        if (item.contains("answer_alias") && item["answer_alias"].is_array()) {
            for (const auto& x : item["answer_alias"]) {
                std::string value = x.get<std::string>();
                poem.answerAlias.push_back(value);
                poem.answerAliasStd.push_back(StringNormalizer::normalizeTitle(value));
            }
        }

        poem.titleStd = item.contains("title_std")
            ? StringNormalizer::normalizeTitle(item.value("title_std", ""))
            : StringNormalizer::normalizeTitle(poem.title);

        if (item.contains("content_std")) {
            poem.contentStd = StringNormalizer::normalizeContent(item.value("content_std", ""));
        }
        else {
            poem.contentStd = StringNormalizer::joinNormalized(poem.content);
        }

        poem.authorStd = StringNormalizer::normalizeTag(poem.author);
        poem.dynastyStd = StringNormalizer::normalizeTag(poem.dynasty);
        poem.typeStd = StringNormalizer::normalizeTag(poem.type);
        poem.emotionCoreStd = StringNormalizer::normalizeTag(poem.emotionCore);
        poem.bgCoreStd = StringNormalizer::normalizeTag(poem.bgCore);
        poem.featureCoreStd = StringNormalizer::normalizeTag(poem.featureCore);
        poem.mainImageryOneStd = StringNormalizer::normalizeTag(poem.mainImageryOne);
        poem.firstlinePrefixStd = StringNormalizer::normalizeContent(poem.firstlinePrefix);
        poem.rhythmStd = StringNormalizer::normalizeTag(poem.rhythm);

        std::vector<std::string> searchableParts;
        searchableParts.push_back(poem.title);
        searchableParts.push_back(poem.author);
        searchableParts.push_back(poem.dynasty);
        searchableParts.push_back(poem.type);
        searchableParts.push_back(poem.emotion);
        searchableParts.push_back(poem.creationBg);
        searchableParts.push_back(poem.distinctFeature);
        searchableParts.push_back(poem.authorTag);
        searchableParts.push_back(poem.rhythm);
        searchableParts.push_back(poem.emotionCore);
        searchableParts.push_back(poem.bgCore);
        searchableParts.push_back(poem.mainImageryOne);
        searchableParts.push_back(poem.featureCore);
        searchableParts.push_back(poem.firstlinePrefix);

        for (const auto& line : poem.content) searchableParts.push_back(line);
        for (const auto& x : poem.imagery) searchableParts.push_back(x);
        for (const auto& x : poem.imageryGameAll) searchableParts.push_back(x);
        for (const auto& x : poem.answerAlias) searchableParts.push_back(x);

        searchableParts.push_back(std::to_string(poem.sentenceCount) + "句" + std::to_string(poem.charCount) + "字");
        searchableParts.push_back(std::to_string(poem.sentenceCount) + "句");
        searchableParts.push_back(std::to_string(poem.charCount) + "字");

        poem.searchTextStd = StringNormalizer::joinNormalized(searchableParts);

        poems.push_back(poem);
    }

    return true;
}

const std::vector<Poem>& PoetryRepository::getAllPoems() const {
    return poems;
}

void PoetryRepository::replaceAll(std::string& s, const std::string& from, const std::string& to) {
    if (from.empty()) return;

    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.length(), to);
        pos += to.length();
    }
}

std::vector<std::string> PoetryRepository::splitPoemIntoLines(const std::string& content) {
    std::string temp = content;

    replaceAll(temp, "\r\n", "\n");
    replaceAll(temp, "\r", "\n");

    replaceAll(temp, "，", "\n");
    replaceAll(temp, "。", "\n");
    replaceAll(temp, "！", "\n");
    replaceAll(temp, "？", "\n");
    replaceAll(temp, "；", "\n");

    replaceAll(temp, ",", "\n");
    replaceAll(temp, ".", "\n");
    replaceAll(temp, "!", "\n");
    replaceAll(temp, "?", "\n");
    replaceAll(temp, ";", "\n");

    std::vector<std::string> result;
    std::stringstream ss(temp);
    std::string line;

    while (std::getline(ss, line, '\n')) {
        line = trim(line);
        if (!line.empty()) {
            result.push_back(line);
        }
    }

    return result;
}

std::string PoetryRepository::trim(const std::string& s) {
    size_t left = 0;
    while (left < s.size() &&
           (s[left] == ' ' || s[left] == '\t' || s[left] == '\n' || s[left] == '\r')) {
        ++left;
    }

    if (left == s.size()) return "";

    size_t right = s.size() - 1;
    while (right > left &&
           (s[right] == ' ' || s[right] == '\t' || s[right] == '\n' || s[right] == '\r')) {
        --right;
    }

    return s.substr(left, right - left + 1);
}

const Poem* PoetryRepository::getPoemById(int id) const {
    for (const auto& poem : poems) {
        if (poem.id == id) {
            return &poem;
        }
    }
    return nullptr;
}
