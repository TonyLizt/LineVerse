#include "TextNormalizer.h"

#include <iomanip>
#include <sstream>

#include "../util/Utf8.h"

namespace lineverse::poetryrebuild {

namespace {

void pushNormalizedLine(
    std::vector<std::string>& lines,
    const TextNormalizer& normalizer,
    const CodePointList& current
) {
    if (current.empty()) {
        return;
    }

    const std::string line = normalizer.normalizeText(Utf8::fromCodePoints(current));
    if (!line.empty() && normalizer.countChineseChars(line) >= 2) {
        lines.push_back(line);
    }
}

} // namespace

TextNormalizer::TextNormalizer()
    : traditionalToSimplified_{
        {U'國', U'国'}, {U'風', U'风'}, {U'詩', U'诗'}, {U'詞', U'词'}, {U'萬', U'万'},
        {U'與', U'与'}, {U'為', U'为'}, {U'後', U'后'}, {U'無', U'无'}, {U'長', U'长'},
        {U'樂', U'乐'}, {U'門', U'门'}, {U'時', U'时'}, {U'東', U'东'}, {U'西', U'西'},
        {U'雲', U'云'}, {U'龍', U'龙'}, {U'來', U'来'}, {U'歸', U'归'}, {U'開', U'开'},
        {U'見', U'见'}, {U'聞', U'闻'}, {U'陰', U'阴'}, {U'陽', U'阳'}, {U'對', U'对'},
        {U'興', U'兴'}, {U'臺', U'台'}, {U'樓', U'楼'}, {U'畫', U'画'}, {U'馬', U'马'},
        {U'鳥', U'鸟'}, {U'魚', U'鱼'}, {U'書', U'书'}, {U'車', U'车'}, {U'紅', U'红'},
        {U'綠', U'绿'}, {U'黃', U'黄'}, {U'愛', U'爱'}, {U'劍', U'剑'}, {U'邊', U'边'},
        {U'淚', U'泪'}, {U'濤', U'涛'}, {U'關', U'关'}, {U'聲', U'声'}, {U'處', U'处'},
        {U'葉', U'叶'}, {U'飛', U'飞'}, {U'過', U'过'}, {U'還', U'还'}, {U'鄉', U'乡'},
        {U'實', U'实'}, {U'虛', U'虚'}, {U'氣', U'气'}, {U'寶', U'宝'}, {U'劉', U'刘'},
        {U'陳', U'陈'}, {U'張', U'张'}, {U'簡', U'简'}, {U'體', U'体'}, {U'鐘', U'钟'}
    } {
}

std::string TextNormalizer::normalizeText(const std::string& text) const {
    const CodePointList cps = Utf8::toCodePoints(text);
    CodePointList cleaned;
    cleaned.reserve(cps.size());

    for (CodePoint cp : cps) {
        cp = normalizeCodePoint(cp);

        if (isWhitespace(cp) || isSentenceDelimiter(cp)) {
            continue;
        }

        if (isChineseChar(cp)) {
            cleaned.push_back(cp);
        }
    }

    return Utf8::fromCodePoints(cleaned);
}

std::vector<std::string> TextNormalizer::splitPoemToLines(const std::vector<std::string>& paragraphs) const {
    std::vector<std::string> lines;

    for (const auto& paragraph : paragraphs) {
        if (paragraph.empty()) {
            continue;
        }

        const CodePointList cps = Utf8::toCodePoints(paragraph);
        CodePointList current;

        for (CodePoint rawCp : cps) {
            CodePoint cp = normalizeCodePoint(rawCp);

            if (cp == U'\n' || cp == U'\r') {
                pushNormalizedLine(lines, *this, current);
                current.clear();
                continue;
            }

            if (isSentenceDelimiter(cp)) {
                pushNormalizedLine(lines, *this, current);
                current.clear();
                continue;
            }

            current.push_back(cp);
        }

        pushNormalizedLine(lines, *this, current);
    }

    return lines;
}

CharFreq TextNormalizer::buildCharFreq(const std::string& text) const {
    CharFreq freq;
    for (CodePoint cp : Utf8::toCodePoints(text)) {
        ++freq[cp];
    }
    return freq;
}

int TextNormalizer::countChineseChars(const std::string& text) const {
    int count = 0;
    for (CodePoint cp : Utf8::toCodePoints(text)) {
        if (isChineseChar(cp)) {
            ++count;
        }
    }
    return count;
}

std::string TextNormalizer::generateInternalId(
    const std::string& sourceType,
    const std::string& text,
    const std::string& author,
    const std::string& title) const {

    const std::string key = sourceType + "|" + text + "|" + author + "|" + title;
    const std::uint64_t hash = fnv1a64(key);

    std::ostringstream oss;
    oss << sourceType << "_" << std::hex << std::setw(16) << std::setfill('0') << hash;
    return oss.str();
}

bool TextNormalizer::isChineseChar(CodePoint cp) {
    return (cp >= 0x4E00 && cp <= 0x9FFF)
        || (cp >= 0x3400 && cp <= 0x4DBF)
        || (cp >= 0x20000 && cp <= 0x2A6DF)
        || (cp >= 0x2A700 && cp <= 0x2B73F)
        || (cp >= 0x2B740 && cp <= 0x2B81F)
        || (cp >= 0x2B820 && cp <= 0x2CEAF)
        || (cp >= 0xF900 && cp <= 0xFAFF);
}

bool TextNormalizer::isWhitespace(CodePoint cp) {
    return cp == U' ' || cp == U'\t' || cp == U'\n' || cp == U'\r' || cp == 0x3000;
}

bool TextNormalizer::isSentenceDelimiter(CodePoint cp) {
    switch (cp) {
    case U'，':
    case U'。':
    case U'！':
    case U'？':
    case U'；':
    case U'：':
    case U'、':
    case U',':
    case U'.':
    case U'!':
    case U'?':
    case U';':
    case U':':
    case U'（':
    case U'）':
    case U'(':
    case U')':
    case U'[':
    case U']':
    case U'【':
    case U'】':
    case U'《':
    case U'》':
    case U'〈':
    case U'〉':
    case U'「':
    case U'」':
    case U'『':
    case U'』':
    case U'"':
    case U'“':
    case U'”':
    case U'‘':
    case U'’':
    case U'—':
    case U'-':
    case U'_':
    case U'·':
    case U'…':
        return true;
    default:
        return false;
    }
}

std::uint64_t TextNormalizer::fnv1a64(const std::string& value) {
    constexpr std::uint64_t offset = 14695981039346656037ull;
    constexpr std::uint64_t prime = 1099511628211ull;

    std::uint64_t hash = offset;
    for (unsigned char c : value) {
        hash ^= static_cast<std::uint64_t>(c);
        hash *= prime;
    }
    return hash;
}

CodePoint TextNormalizer::normalizeCodePoint(CodePoint cp) const {
    if (cp == 0x3000) {
        return U' ';
    }

    if (cp >= 0xFF01 && cp <= 0xFF5E) {
        cp -= 0xFEE0;
    }

    const auto it = traditionalToSimplified_.find(cp);
    if (it != traditionalToSimplified_.end()) {
        return it->second;
    }

    return cp;
}

} // namespace lineverse::poetryrebuild