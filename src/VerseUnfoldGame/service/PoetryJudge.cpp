#include "PoetryJudge.h"

#include "../utils/EditDistance.h"
#include "../utils/StringNormalizer.h"

JudgeResult PoetryJudge::judge(const std::string& input, const Poem& poem) const {
    std::string normalizedTitleInput = StringNormalizer::normalizeTitle(input);
    std::string normalizedContentInput = StringNormalizer::normalizeContent(input);

    if (matchTitle(normalizedTitleInput, poem)) {
        return JudgeResult::Correct;
    }

    if (matchAlias(normalizedTitleInput, poem)) {
        return JudgeResult::Correct;
    }

    if (matchContent(normalizedContentInput, poem)) {
        return JudgeResult::Correct;
    }

    if (matchAnyLine(normalizedContentInput, poem)) {
        return JudgeResult::Correct;
    }

    if (isNearMatch(normalizedTitleInput, poem)) {
        return JudgeResult::Near;
    }

    return JudgeResult::Wrong;
}

bool PoetryJudge::matchTitle(const std::string& normalizedInput, const Poem& poem) const {
    return !normalizedInput.empty() && normalizedInput == poem.titleStd;
}

bool PoetryJudge::matchAlias(const std::string& normalizedInput, const Poem& poem) const {
    if (normalizedInput.empty()) {
        return false;
    }
    for (const auto& aliasStd : poem.answerAliasStd) {
        if (normalizedInput == aliasStd) {
            return true;
        }
    }
    return false;
}

bool PoetryJudge::matchContent(const std::string& normalizedInput, const Poem& poem) const {
    return !normalizedInput.empty() && normalizedInput == poem.contentStd;
}

bool PoetryJudge::matchAnyLine(const std::string& normalizedInput, const Poem& poem) const {
    if (normalizedInput.empty()) {
        return false;
    }

    for (const auto& lineStd : poem.linesStd) {
        if (normalizedInput == lineStd) {
            return true;
        }
    }
    return false;
}

bool PoetryJudge::isNearMatch(const std::string& normalizedInput, const Poem& poem) const {
    if (normalizedInput.empty()) {
        return false;
    }

    double titleSimilarity = EditDistance::similarity(normalizedInput, poem.titleStd);
    if (titleSimilarity >= 0.75) {
        return true;
    }

    for (const auto& aliasStd : poem.answerAliasStd) {
        double sim = EditDistance::similarity(normalizedInput, aliasStd);
        if (sim >= 0.75) {
            return true;
        }
    }

    return false;
}
