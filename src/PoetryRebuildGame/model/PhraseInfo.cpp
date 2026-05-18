#include "PhraseInfo.h"

#include "../util/Utf8.h"

namespace lineverse::poetryrebuild {

std::string toString(PhraseType type) {
    return type == PhraseType::Poem ? "poem" : "idiom";
}

PhraseType phraseTypeFromString(const std::string& value) {
    return value == "idiom" ? PhraseType::Idiom : PhraseType::Poem;
}

Json phraseInfoToJson(const PhraseInfo& info) {
    Json freq = Json::object();
    for (const auto& [ch, count] : info.charFreq) {
        freq[Utf8::fromCodePoint(ch)] = count;
    }

    return Json{
        {"id", info.id},
        {"text", info.text},
        {"type", toString(info.type)},
        {"length", info.length},
        {"charFreq", freq},
        {"author", info.author},
        {"title", info.title},
        {"pinyin", info.pinyin},
        {"explanation", info.explanation},
        {"derivation", info.derivation},
        {"example", info.example},
        {"rarity", info.rarity},
        {"scoreWeight", info.scoreWeight},
        {"sourceFile", info.sourceFile}
    };
}

} // namespace lineverse::poetryrebuild
