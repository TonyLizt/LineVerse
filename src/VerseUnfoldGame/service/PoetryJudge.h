#pragma once

#include <string>

#include "../data/model/Poem.h"

enum class JudgeResult {
    Correct,
    Near,
    Wrong
};

class PoetryJudge {
public:
    JudgeResult judge(const std::string& input, const Poem& poem) const;

private:
    bool matchTitle(const std::string& normalizedInput, const Poem& poem) const;
    bool matchAlias(const std::string& normalizedInput, const Poem& poem) const;
    bool matchContent(const std::string& normalizedInput, const Poem& poem) const;
    bool matchAnyLine(const std::string& normalizedInput, const Poem& poem) const;
    bool isNearMatch(const std::string& normalizedInput, const Poem& poem) const;
};