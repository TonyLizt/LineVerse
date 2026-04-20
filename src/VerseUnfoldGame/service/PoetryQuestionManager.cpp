#include "PoetryQuestionManager.h"

#include <cstdlib>
#include <ctime>
#include <vector>

int PoetryQuestionManager::selectQuestion(int difficulty, const std::vector<Poem>& poems) const {
    std::vector<int> candidates;
    for (int i = 0; i < static_cast<int>(poems.size()); ++i) {
        if (poems[i].difficulty == difficulty) {
            candidates.push_back(i);
        }
    }

    if (candidates.empty()) {
        return -1;
    }

    static bool seeded = false;
    if (!seeded) {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        seeded = true;
    }

    int pos = std::rand() % static_cast<int>(candidates.size());
    return candidates[pos];
}