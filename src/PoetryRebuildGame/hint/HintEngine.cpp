#include "HintEngine.h"

#include <algorithm>
#include <sstream>

#include "../util/Utf8.h"

namespace lineverse::poetryrebuild {

HintEngine::HintEngine(const IndexStore& indexStore)
    : indexStore_(indexStore) {
}

const PhraseInfo* HintEngine::pickBestRemainingFromOptimal(const GameRound& round) const {
    const auto& bestSet = round.getBestSolutionSet();
    for (const auto& id : bestSet) {
        const PhraseInfo* phrase = indexStore_.getPhraseById(id);
        if (phrase == nullptr) {
            continue;
        }
        if (round.state().foundSet.find(phrase->text) == round.state().foundSet.end()) {
            return phrase;
        }
    }
    return nullptr;
}

const PhraseInfo* HintEngine::pickMostValuableRemaining(const GameRound& round) const {
    const auto remaining = round.getRemainingCandidates();

    const PhraseInfo* best = nullptr;
    for (const auto& id : remaining) {
        const PhraseInfo* phrase = indexStore_.getPhraseById(id);
        if (phrase == nullptr) {
            continue;
        }

        if (best == nullptr) {
            best = phrase;
            continue;
        }

        if (phrase->scoreWeight > best->scoreWeight) {
            best = phrase;
            continue;
        }
        if (phrase->scoreWeight == best->scoreWeight && phrase->length > best->length) {
            best = phrase;
            continue;
        }
        if (phrase->scoreWeight == best->scoreWeight &&
            phrase->length == best->length &&
            phrase->text < best->text) {
            best = phrase;
        }
    }

    return best;
}

HintResult HintEngine::getHintByFirstChar(GameRound& round) const {
    const PhraseInfo* target = nullptr;
    int nextRevealCount = 1;

    if (round.hasActiveFirstHintTarget()) {
        const std::string& activeId = round.activeFirstHintTargetId();
        const PhraseInfo* activePhrase = indexStore_.getPhraseById(activeId);

        if (activePhrase != nullptr &&
            round.state().foundSet.find(activePhrase->text) == round.state().foundSet.end() &&
            round.canStillComposeAnswerId(activeId)) {

            target = activePhrase;
            nextRevealCount = round.activeFirstHintRevealCount() + 1;
        } else {
            round.clearActiveFirstHint();
        }
    }

    if (target == nullptr) {
        target = pickBestRemainingFromOptimal(round);
        if (target == nullptr) {
            target = pickMostValuableRemaining(round);
        }

        if (target == nullptr) {
            return {"当前没有可提示的剩余答案。", false};
        }

        nextRevealCount = 1;
    }

    const auto cps = Utf8::toCodePoints(target->text);
    if (cps.empty()) {
        return {"当前没有可提示的剩余答案。", false};
    }

    if (round.hasActiveFirstHintTarget() &&
        round.activeFirstHintTargetId() == target->id &&
        round.activeFirstHintRevealCount() >= static_cast<int>(cps.size())) {
        return {"这条提示答案已经完整揭示为「" + target->text + "」，本次不扣提示次数。", false};
    }

    nextRevealCount = std::min<int>(nextRevealCount, static_cast<int>(cps.size()));

    CodePointList reveal;
    reveal.reserve(nextRevealCount);
    for (int i = 0; i < nextRevealCount; ++i) {
        reveal.push_back(cps[static_cast<std::size_t>(i)]);
    }

    round.setActiveFirstHintProgress(target->id, nextRevealCount);

    std::string revealedText = Utf8::fromCodePoints(reveal);
    std::ostringstream oss;
    oss << "渐进首字提示：当前已揭示「" << revealedText << "」"
        << "（" << nextRevealCount << "/" << cps.size() << "）";

    if (nextRevealCount == static_cast<int>(cps.size())) {
        oss << "，答案已完整揭示。";
    }

    return {oss.str(), true};
}

HintResult HintEngine::getHintByLength(const GameRound& round) const {
    const PhraseInfo* target = pickBestRemainingFromOptimal(round);
    if (target == nullptr) {
        target = pickMostValuableRemaining(round);
    }
    if (target == nullptr) {
        return {"当前没有可提示的剩余答案。", false};
    }

    return {
        "长度提示：还有一条值得优先考虑的答案，长度为 " + std::to_string(target->length) + " 字。",
        true
    };
}

HintResult HintEngine::getHintByType(const GameRound& round) const {
    const PhraseInfo* target = pickBestRemainingFromOptimal(round);
    if (target == nullptr) {
        target = pickMostValuableRemaining(round);
    }
    if (target == nullptr) {
        return {"当前没有可提示的剩余答案。", false};
    }

    const std::string typeText = (target->type == PhraseType::Poem) ? "诗句" : "成语";
    return {
        "类型提示：还有一条高价值剩余答案属于「" + typeText + "」。",
        true
    };
}

HintResult HintEngine::getMostValuableRemainingAnswer(const GameRound& round) const {
    const PhraseInfo* target = pickMostValuableRemaining(round);
    if (target == nullptr) {
        return {"当前没有剩余高价值答案。", false};
    }

    return {
        "高价值答案提示：你可以考虑「" + target->text + "」。",
        true
    };
}

HintResult HintEngine::suggestNextBestMove(const GameRound& round) const {
    const PhraseInfo* target = pickBestRemainingFromOptimal(round);
    if (target == nullptr) {
        target = pickMostValuableRemaining(round);
    }
    if (target == nullptr) {
        return {"当前没有可推荐的下一步。", false};
    }

    const std::string typeText = (target->type == PhraseType::Poem) ? "诗句" : "成语";
    return {
        "推荐下一步：优先尝试「" + target->text + "」"
        + "（类型：" + typeText
        + "，长度：" + std::to_string(target->length) + "）。",
        true
    };
}

HintResult HintEngine::getAllRemainingAnswers(const GameRound& round) const {
    const auto remaining = round.getRemainingCandidates();
    if (remaining.empty()) {
        return {"当前没有剩余可选正确答案。", false};
    }

    std::vector<std::string> texts;
    texts.reserve(remaining.size());

    for (const auto& id : remaining) {
        const PhraseInfo* phrase = indexStore_.getPhraseById(id);
        if (phrase != nullptr) {
            texts.push_back(phrase->text);
        }
    }

    std::sort(texts.begin(), texts.end());
    texts.erase(std::unique(texts.begin(), texts.end()), texts.end());

    std::ostringstream oss;
    oss << "当前所有可选正确答案（共 " << texts.size() << " 条）：";

    for (std::size_t i = 0; i < texts.size(); ++i) {
        oss << "\n  - " << texts[i];
    }

    return {oss.str(), true};
}

} // namespace lineverse::poetryrebuild