#pragma once

#include <string>
#include <vector>

class StringNormalizer {
public:
    static std::string normalizeTitle(const std::string& s);
    static std::string normalizeContent(const std::string& s);
    static std::string normalizeTag(const std::string& s);

    static std::string normalizeForJudge(const std::string& s);
    static bool equalsForJudge(const std::string& a, const std::string& b);
    static bool containsNormalized(const std::string& whole, const std::string& part);
    static std::string joinNormalized(const std::vector<std::string>& parts);

private:
    static std::string removeSpaces(const std::string& s);
    static std::string removeCommonPunctuation(const std::string& s);
    static void replaceAll(std::string& s, const std::string& from, const std::string& to);
};
