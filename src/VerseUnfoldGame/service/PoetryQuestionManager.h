#pragma once

#include <vector>
#include "../data/model/Poem.h"

class PoetryQuestionManager {
public:
    int selectQuestion(int difficulty, const std::vector<Poem>& poems) const;
};