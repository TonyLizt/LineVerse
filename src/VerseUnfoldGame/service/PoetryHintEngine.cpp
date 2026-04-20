#include "PoetryHintEngine.h"

#include <sstream>

namespace {
std::string formatTitle(const std::string& title) {
    if (title.empty()) {
        return title;
    }
    if (title.rfind("《", 0) == 0 && title.size() >= std::string("》").size() && title.substr(title.size() - std::string("》").size()) == "》") {
        return title;
    }
    return "《" + title + "》";
}
}

void PoetryHintEngine::reset() {
    while (!remainingHints.empty()) {
        remainingHints.pop();
    }
    shownHints.clear();
}

void PoetryHintEngine::init(const Poem& poem) {
    reset();

    std::vector<HintItem> hints = buildHintChain(poem);
    for (const auto& hint : hints) {
        remainingHints.push(hint);
    }
}

bool PoetryHintEngine::hasNextHint() const {
    return !remainingHints.empty();
}

HintItem PoetryHintEngine::revealNextHint() {
    if (remainingHints.empty()) {
        return HintItem{};
    }

    HintItem hint = remainingHints.front();
    remainingHints.pop();
    shownHints.push_back(hint);
    return hint;
}

const std::vector<HintItem>& PoetryHintEngine::getShownHints() const {
    return shownHints;
}

std::vector<HintItem> PoetryHintEngine::buildHintChain(const Poem& poem) const {
    std::vector<HintItem> hints;

    hints.push_back({
        1,
        "dynasty",
        "这首诗词所属朝代（或时代）是：" + poem.dynasty,
        poem.dynasty
    });

    hints.push_back({
        2,
        "type",
        "这首作品的体裁是：" + poem.type,
        poem.type
    });

    hints.push_back({
        3,
        "emotion",
        "这首作品的核心情感是：" + poem.emotionCore,
        poem.emotionCore
    });

    hints.push_back({
        4,
        "background",
        "这首作品的创作背景关键词是：" + poem.bgCore,
        poem.bgCore
    });

    hints.push_back({
        5,
        "main_imagery",
        "这首作品的核心意象之一是：" + poem.mainImageryOne,
        poem.mainImageryOne
    });

    hints.push_back({
        6,
        "feature",
        "这首作品的艺术特色关键词是：" + poem.featureCore,
        poem.featureCore
    });

    {
        std::ostringstream oss;
        for (size_t i = 0; i < poem.imageryGameAll.size(); ++i) {
            oss << poem.imageryGameAll[i];
            if (i + 1 < poem.imageryGameAll.size()) {
                oss << "、";
            }
        }
        hints.push_back({
            7,
            "all_imagery",
            "这首作品涉及的意象包括：" + oss.str(),
            oss.str()
        });
    }

    {
        std::ostringstream ossText;
        ossText << "这首作品共有 " << poem.sentenceCount
                << " 句，总字数约为 " << poem.charCount << " 字";

        std::ostringstream ossValue;
        ossValue << poem.sentenceCount << "|" << poem.charCount;

        hints.push_back({
            8,
            "structure",
            ossText.str(),
            ossValue.str()
        });
    }

    hints.push_back({
        9,
        "firstline_prefix",
        "这首作品首句的前缀提示是：" + poem.firstlinePrefix,
        poem.firstlinePrefix
    });

    hints.push_back({
        10,
        "answer",
        "答案揭晓：" + formatTitle(poem.title) + " 作者：" + poem.author,
        poem.title
    });

    return hints;
}
