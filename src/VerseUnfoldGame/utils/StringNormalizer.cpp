#include "StringNormalizer.h"

void StringNormalizer::replaceAll(std::string& s, const std::string& from, const std::string& to) {
    if (from.empty()) return;

    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.length(), to);
        pos += to.length();
    }
}

std::string StringNormalizer::removeSpaces(const std::string& s) {
    std::string result = s;

    replaceAll(result, " ", "");
    replaceAll(result, "\t", "");
    replaceAll(result, "\n", "");
    replaceAll(result, "\r", "");
    replaceAll(result, "　", "");

    return result;
}

std::string StringNormalizer::removeCommonPunctuation(const std::string& s) {
    std::string result = s;

    replaceAll(result, "《", "");
    replaceAll(result, "》", "");
    replaceAll(result, "“", "");
    replaceAll(result, "”", "");
    replaceAll(result, "‘", "");
    replaceAll(result, "’", "");
    replaceAll(result, "\"", "");
    replaceAll(result, "'", "");

    replaceAll(result, "，", "");
    replaceAll(result, "。", "");
    replaceAll(result, "、", "");
    replaceAll(result, "！", "");
    replaceAll(result, "？", "");
    replaceAll(result, "；", "");
    replaceAll(result, "：", "");
    replaceAll(result, "（", "");
    replaceAll(result, "）", "");
    replaceAll(result, "【", "");
    replaceAll(result, "】", "");
    replaceAll(result, "—", "");
    replaceAll(result, "-", "");
    replaceAll(result, "_", "");
    replaceAll(result, "/", "");
    replaceAll(result, "\\", "");
    replaceAll(result, "·", "");

    replaceAll(result, ",", "");
    replaceAll(result, ".", "");
    replaceAll(result, "!", "");
    replaceAll(result, "?", "");
    replaceAll(result, ";", "");
    replaceAll(result, ":", "");
    replaceAll(result, "(", "");
    replaceAll(result, ")", "");
    replaceAll(result, "[", "");
    replaceAll(result, "]", "");
    replaceAll(result, "{", "");
    replaceAll(result, "}", "");
    replaceAll(result, "<", "");
    replaceAll(result, ">", "");
    replaceAll(result, "`", "");

    return result;
}

std::string StringNormalizer::normalizeTitle(const std::string& s) {
    return removeCommonPunctuation(removeSpaces(s));
}

std::string StringNormalizer::normalizeContent(const std::string& s) {
    return removeCommonPunctuation(removeSpaces(s));
}

std::string StringNormalizer::normalizeTag(const std::string& s) {
    return normalizeContent(s);
}

std::string StringNormalizer::normalizeForJudge(const std::string& s) {
    return normalizeContent(s);
}

bool StringNormalizer::equalsForJudge(const std::string& a, const std::string& b) {
    return normalizeForJudge(a) == normalizeForJudge(b);
}

bool StringNormalizer::containsNormalized(const std::string& whole, const std::string& part) {
    std::string wholeStd = normalizeContent(whole);
    std::string partStd = normalizeContent(part);
    if (wholeStd.empty() || partStd.empty()) {
        return false;
    }
    return wholeStd.find(partStd) != std::string::npos;
}

std::string StringNormalizer::joinNormalized(const std::vector<std::string>& parts) {
    std::string joined;
    for (const auto& part : parts) {
        joined += normalizeContent(part);
    }
    return joined;
}
