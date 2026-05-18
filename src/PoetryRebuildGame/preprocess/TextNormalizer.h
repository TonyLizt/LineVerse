#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "../common/Types.h"

namespace lineverse::poetryrebuild {

class TextNormalizer {
public:
    TextNormalizer();

    std::string normalizeText(const std::string& text) const;
    std::vector<std::string> splitPoemToLines(const std::vector<std::string>& paragraphs) const;
    CharFreq buildCharFreq(const std::string& text) const;
    int countChineseChars(const std::string& text) const;
    std::string generateInternalId(
        const std::string& sourceType,
        const std::string& text,
        const std::string& author,
        const std::string& title) const;

private:
    std::unordered_map<CodePoint, CodePoint> traditionalToSimplified_;

    static bool isChineseChar(CodePoint cp);
    static bool isWhitespace(CodePoint cp);
    static bool isSentenceDelimiter(CodePoint cp);
    static std::uint64_t fnv1a64(const std::string& value);
    CodePoint normalizeCodePoint(CodePoint cp) const;
};

} // namespace lineverse::poetryrebuild
