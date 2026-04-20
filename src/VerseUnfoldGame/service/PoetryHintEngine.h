#pragma once

#include <queue>
#include <vector>

#include "../data/model/HintItem.h"
#include "../data/model/Poem.h"

class PoetryHintEngine {
public:
    void init(const Poem& poem);

    bool hasNextHint() const;
    HintItem revealNextHint();
    const std::vector<HintItem>& getShownHints() const;

    void reset();

private:
    std::queue<HintItem> remainingHints;
    std::vector<HintItem> shownHints;

    std::vector<HintItem> buildHintChain(const Poem& poem) const;
};