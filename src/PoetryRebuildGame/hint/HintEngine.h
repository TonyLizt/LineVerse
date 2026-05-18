#pragma once

#include <string>

#include "../game/GameRound.h"
#include "../index/IndexStore.h"

namespace lineverse::poetryrebuild {

struct HintResult {
    std::string text;
    bool consumeCount = true;
};

class HintEngine {
public:
    explicit HintEngine(const IndexStore& indexStore);

    HintResult getHintByFirstChar(GameRound& round) const;
    HintResult getHintByLength(const GameRound& round) const;
    HintResult getHintByType(const GameRound& round) const;
    HintResult getMostValuableRemainingAnswer(const GameRound& round) const;
    HintResult suggestNextBestMove(const GameRound& round) const;
    HintResult getAllRemainingAnswers(const GameRound& round) const;

private:
    const IndexStore& indexStore_;

    const PhraseInfo* pickBestRemainingFromOptimal(const GameRound& round) const;
    const PhraseInfo* pickMostValuableRemaining(const GameRound& round) const;
};

} // namespace lineverse::poetryrebuild