#include "PoetryDescriptionParser.h"

#include <cctype>
#include <unordered_map>
#include <vector>

#include "../utils/StringNormalizer.h"

namespace {
void replaceAllLocal(std::string& s, const std::string& from, const std::string& to) {
    if (from.empty()) return;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
}

bool containsAny(const std::string& text, const std::vector<std::string>& patterns) {
    const std::string textStd = StringNormalizer::normalizeContent(text);
    for (const auto& pattern : patterns) {
        const std::string patternStd = StringNormalizer::normalizeContent(pattern);
        if (!patternStd.empty() && textStd.find(patternStd) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool looksLikeEmotionHint(const std::string& token) {
    static const std::vector<std::string> patterns = {
        "喜爱", "热爱", "思乡", "怀乡", "乡愁", "忧国", "伤时", "孤独", "孤寂",
        "离别", "惜别", "豪情", "悲凉", "哀愁", "高洁", "坚贞", "豁达", "闲适", "赞美"
    };
    return containsAny(token, patterns);
}

bool looksLikeBackgroundHint(const std::string& token) {
    static const std::vector<std::string> patterns = {
        "而作", "所作", "写下", "创作", "背景", "被贬", "旅居", "闲居", "登楼", "夜泊",
        "西湖", "杭州", "扬州", "边塞", "战乱", "秋夜", "春游", "早春", "晚年", "任", "刺史"
    };
    return containsAny(token, patterns);
}

bool looksLikeFeatureHint(const std::string& token) {
    static const std::vector<std::string> patterns = {
        "白描", "托物言志", "借景抒情", "移步换景", "情景交融", "对仗", "工整",
        "朴素", "自然", "豪放", "婉约", "含蓄", "比喻", "夸张", "衬托"
    };
    return containsAny(token, patterns);
}

bool isSpecificToken(const std::string& token) {
    return StringNormalizer::normalizeContent(token).size() >= 3;
}

bool flexibleFieldMatch(const std::string& token, const std::string& value, size_t minLenForContain) {
    const std::string tokenStd = StringNormalizer::normalizeContent(token);
    const std::string valueStd = StringNormalizer::normalizeContent(value);
    if (tokenStd.empty() || valueStd.empty()) {
        return false;
    }
    if (tokenStd == valueStd) {
        return true;
    }
    if (tokenStd.size() < minLenForContain || valueStd.size() < minLenForContain) {
        return false;
    }
    return tokenStd.find(valueStd) != std::string::npos || valueStd.find(tokenStd) != std::string::npos;
}
}

DescriptionQuery PoetryDescriptionParser::parse(const std::string& input, const std::vector<Poem>& poems) const {
    DescriptionQuery query;
    query.rawInput = input;
    query.normalizedInput = StringNormalizer::normalizeContent(input);

    std::vector<std::string> tokens = splitTokens(input);
    if (tokens.empty()) {
        tokens.push_back(input);
    }

    const std::string structureValue = tryParseStructure(input);
    if (!structureValue.empty()) {
        addUniqueHit(query.hits, QueryField::Structure, structureValue, 3.0);
    }

    for (std::string token : tokens) {
        token = cleanupSemanticToken(token);
        token = normalizeSynonym(token);
        const std::string tokenStd = StringNormalizer::normalizeContent(token);
        if (tokenStd.empty()) {
            continue;
        }

        bool matchedExactField = false;
        for (const auto& poem : poems) {
            if (flexibleFieldMatch(token, poem.dynasty, 1)) {
                addUniqueHit(query.hits, QueryField::Dynasty, poem.dynasty, 3.8);
                matchedExactField = true;
            }
            if (flexibleFieldMatch(token, poem.author, 2)) {
                addUniqueHit(query.hits, QueryField::Author, poem.author, 4.8);
                matchedExactField = true;
            }
            if (flexibleFieldMatch(token, poem.type, 2)) {
                addUniqueHit(query.hits, QueryField::Type, poem.type, 3.8);
                matchedExactField = true;
            }
            if (flexibleFieldMatch(token, poem.title, 2) || flexibleFieldMatch(token, poem.titleStd, 2)) {
                addUniqueHit(query.hits, QueryField::Title, poem.title, 5.6);
                matchedExactField = true;
            }
            if (flexibleFieldMatch(token, poem.firstlinePrefix, 2)) {
                addUniqueHit(query.hits, QueryField::Prefix, poem.firstlinePrefix, 3.5);
                matchedExactField = true;
            }

            for (const auto& img : poem.imageryGameAll) {
                if (flexibleFieldMatch(token, img, 2)) {
                    addUniqueHit(query.hits, QueryField::Imagery, img, 3.3);
                    matchedExactField = true;
                }
            }
            for (const auto& img : poem.imagery) {
                if (flexibleFieldMatch(token, img, 2)) {
                    addUniqueHit(query.hits, QueryField::Imagery, img, 2.9);
                    matchedExactField = true;
                }
            }
            for (const auto& alias : poem.answerAlias) {
                if (flexibleFieldMatch(token, alias, 2)) {
                    addUniqueHit(query.hits, QueryField::Title, poem.title, 5.0);
                    matchedExactField = true;
                }
            }
            for (const auto& line : poem.lines) {
                if (flexibleFieldMatch(token, line, 2)) {
                    addUniqueHit(query.hits, QueryField::Content, line, 4.0);
                    matchedExactField = true;
                }
            }
        }

        if (looksLikeEmotionHint(token) && isSpecificToken(token)) {
            addUniqueHit(query.hits, QueryField::Emotion, token, 3.1);
        }
        if (looksLikeBackgroundHint(token) && isSpecificToken(token)) {
            addUniqueHit(query.hits, QueryField::Background, token, 3.4);
        }
        if (looksLikeFeatureHint(token) && isSpecificToken(token)) {
            addUniqueHit(query.hits, QueryField::Feature, token, 2.8);
        }

        if (!matchedExactField || isSpecificToken(token)) {
            bool exists = false;
            for (const auto& existing : query.freeTokens) {
                if (existing == tokenStd) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                query.freeTokens.push_back(tokenStd);
            }
        }
    }

    return query;
}

std::vector<std::string> PoetryDescriptionParser::splitTokens(const std::string& input) {
    std::string temp = input;
    replaceAllLocal(temp, "，", "|");
    replaceAllLocal(temp, "。", "|");
    replaceAllLocal(temp, "、", "|");
    replaceAllLocal(temp, "；", "|");
    replaceAllLocal(temp, "：", "|");
    replaceAllLocal(temp, ",", "|");
    replaceAllLocal(temp, ".", "|");
    replaceAllLocal(temp, ";", "|");
    replaceAllLocal(temp, ":", "|");
    replaceAllLocal(temp, " ", "|");

    std::vector<std::string> tokens;
    std::string current;
    for (char ch : temp) {
        if (ch == '|') {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        }
        else {
            current.push_back(ch);
        }
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

std::string PoetryDescriptionParser::cleanupSemanticToken(const std::string& token) {
    std::string s = token;
    replaceAllLocal(s, "这首诗", "");
    replaceAllLocal(s, "这首词", "");
    replaceAllLocal(s, "这首作品", "");
    replaceAllLocal(s, "这篇作品", "");
    replaceAllLocal(s, "表达了", "");
    replaceAllLocal(s, "表达", "");
    replaceAllLocal(s, "体现了", "");
    replaceAllLocal(s, "体现", "");
    replaceAllLocal(s, "描写了", "");
    replaceAllLocal(s, "描写", "");
    replaceAllLocal(s, "表现了", "");
    replaceAllLocal(s, "表现", "");
    replaceAllLocal(s, "抒发了", "");
    replaceAllLocal(s, "抒发", "");
    replaceAllLocal(s, "创作背景", "");
    replaceAllLocal(s, "背景", "");
    replaceAllLocal(s, "作者", "");
    replaceAllLocal(s, "朝代", "");
    replaceAllLocal(s, "体裁", "");
    replaceAllLocal(s, "意象", "");
    replaceAllLocal(s, "核心意象", "");
    replaceAllLocal(s, "语言风格", "");
    replaceAllLocal(s, "艺术特色", "");
    replaceAllLocal(s, "首句", "");
    replaceAllLocal(s, "前两个字", "");
    replaceAllLocal(s, "前缀", "");
    replaceAllLocal(s, "是", "");
    replaceAllLocal(s, "有", "");
    return s;
}

std::string PoetryDescriptionParser::normalizeSynonym(const std::string& token) {
    static const std::unordered_map<std::string, std::string> synonyms = {
        {StringNormalizer::normalizeContent("唐朝"), "唐"},
        {StringNormalizer::normalizeContent("宋朝"), "宋"},
        {StringNormalizer::normalizeContent("元朝"), "元"},
        {StringNormalizer::normalizeContent("明朝"), "明"},
        {StringNormalizer::normalizeContent("清朝"), "清"},
        {StringNormalizer::normalizeContent("五绝"), "五言绝句"},
        {StringNormalizer::normalizeContent("七绝"), "七言绝句"},
        {StringNormalizer::normalizeContent("五律"), "五言律诗"},
        {StringNormalizer::normalizeContent("七律"), "七言律诗"},
        {StringNormalizer::normalizeContent("月亮"), "明月"},
        {StringNormalizer::normalizeContent("月光"), "明月"},
        {StringNormalizer::normalizeContent("想家"), "思乡"},
        {StringNormalizer::normalizeContent("怀乡"), "思乡"},
        {StringNormalizer::normalizeContent("乡愁"), "思乡"},
        {StringNormalizer::normalizeContent("送别"), "离别"},
        {StringNormalizer::normalizeContent("惜别"), "离别"},
        {StringNormalizer::normalizeContent("边疆"), "边塞"},
        {StringNormalizer::normalizeContent("战争"), "战争"},
        {StringNormalizer::normalizeContent("爱国"), "忧国"},
        {StringNormalizer::normalizeContent("春天"), "春"},
        {StringNormalizer::normalizeContent("冬天"), "冬"},
        {StringNormalizer::normalizeContent("四句二十字"), "4句20字"}
    };

    const std::string tokenStd = StringNormalizer::normalizeContent(token);
    auto it = synonyms.find(tokenStd);
    if (it != synonyms.end()) {
        return it->second;
    }
    return token;
}

bool PoetryDescriptionParser::containsEitherNormalized(const std::string& a, const std::string& b) {
    const std::string aStd = StringNormalizer::normalizeContent(a);
    const std::string bStd = StringNormalizer::normalizeContent(b);
    if (aStd.empty() || bStd.empty()) {
        return false;
    }
    return aStd.find(bStd) != std::string::npos || bStd.find(aStd) != std::string::npos;
}

std::string PoetryDescriptionParser::tryParseStructure(const std::string& input) {
    int sentenceCount = -1;
    int charCount = -1;

    std::string s = input;
    for (size_t i = 0; i < s.size(); ++i) {
        if (std::isdigit(static_cast<unsigned char>(s[i]))) {
            int value = 0;
            size_t j = i;
            while (j < s.size() && std::isdigit(static_cast<unsigned char>(s[j]))) {
                value = value * 10 + (s[j] - '0');
                ++j;
            }
            if (j < s.size()) {
                if (s.compare(j, std::string(u8"句").size(), u8"句") == 0) {
                    sentenceCount = value;
                }
                else if (s.compare(j, std::string(u8"字").size(), u8"字") == 0) {
                    charCount = value;
                }
            }
            i = j;
        }
    }

    if (sentenceCount > 0 && charCount > 0) {
        return std::to_string(sentenceCount) + "|" + std::to_string(charCount);
    }
    return "";
}

void PoetryDescriptionParser::addUniqueHit(std::vector<QueryTokenHit>& hits, QueryField field, const std::string& value, double weight) {
    const std::string valueStd = StringNormalizer::normalizeContent(value);
    for (const auto& hit : hits) {
        if (hit.field == field && StringNormalizer::normalizeContent(hit.value) == valueStd) {
            return;
        }
    }
    hits.push_back({field, value, weight});
}
