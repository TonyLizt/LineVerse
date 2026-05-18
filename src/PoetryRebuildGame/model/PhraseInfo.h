#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "../common/Json.h"
#include "../common/Types.h"

namespace lineverse::poetryrebuild {

enum class PhraseType {
    Poem,
    Idiom
};

struct PhraseInfo {
    std::string id;
    std::string text;
    PhraseType type = PhraseType::Poem;
    int length = 0;
    CharFreq charFreq;

    std::string author;
    std::string title;

    std::string pinyin;
    std::string explanation;
    std::string derivation;
    std::string example;

    int rarity = 1;
    int scoreWeight = 1;
    std::string sourceFile;
};

std::string toString(PhraseType type);
PhraseType phraseTypeFromString(const std::string& value);
Json phraseInfoToJson(const PhraseInfo& info);

} // namespace lineverse::poetryrebuild
