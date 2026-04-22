#include "HGCore.hpp"

#include <algorithm>

namespace HandleGame {

HGCore::HGCore(HGEngine& engine) : engine_(engine) {
    std::random_device rd;
    rng_.seed(rd());
}

void HGCore::setLexiconPaths(std::string easyTsv, std::string normalTsv, std::string hardTsv) {
    tsvEasy_ = std::move(easyTsv);
    tsvNormal_ = std::move(normalTsv);
    tsvHard_ = std::move(hardTsv);
}

bool HGCore::ensureLexiconLoaded_(Difficulty diff) {
    const std::string* target = nullptr;
    switch (diff) {
    case Difficulty::Easy:   target = &tsvEasy_; break;
    case Difficulty::Normal: target = &tsvNormal_; break;
    case Difficulty::Hard:   target = &tsvHard_; break;
    }

    if (!target || target->empty()) {
        snap_.message = "词库路径未配置：请检查 data/prebuild/HandleGame 下的 tsv 文件是否存在。";
        return false;
    }

    // 已加载同一份词库：不重复 IO
    if (currentTsv_ == *target && !engine_.allIdioms().empty()) {
        return true;
    }

    std::string err;
    if (!engine_.loadTSV(*target, &err)) {
        snap_.message = "加载词库失败: " + *target + " (" + err + ")";
        return false;
    }

    currentTsv_ = *target;
    return true;
}

void HGCore::resetForNewGame_() {
    snap_.history.clear();
    snap_.chart.iniState.clear();
    snap_.chart.finState.clear();
    snap_.message.clear();
    snap_.finished = false;

    snap_.revealedChar = {{false,false,false,false}};
    snap_.revealedPinyin = {{false,false,false,false}};

    hintStage_ = 0;
    hintIndex_ = -1;

    ansIniSet_.clear();
    ansFinSet_.clear();
}

void HGCore::newGame(Difficulty diff, uint32_t seed) {
    // 先清空上一局状态
    resetForNewGame_();
    snap_.difficulty = diff;

    rng_.seed(seed);

    // ✅ 根据难度加载对应词库
    if (!ensureLexiconLoaded_(diff)) {
        snap_.started = false;
        snap_.finished = true;
        return;
    }

    const auto& all = engine_.allIdioms();
    if (all.empty()) {
        snap_.message = "词库为空，无法开始游戏。";
        snap_.started = false;
        snap_.finished = true;
        return;
    }

    snap_.started = true;
    snap_.finished = false;

    std::uniform_int_distribution<size_t> dist(0, all.size() - 1);
    std::string ansIdiom = all[dist(rng_)];
    answer_ = engine_.featuresOf(ansIdiom);

    for (int i = 0; i < 4; i++) {
        ansIniSet_.insert(answer_.initials[i].empty() ? "∅" : answer_.initials[i]);
        ansFinSet_.insert(answer_.finals[i].empty() ? "∅" : answer_.finals[i]);
    }

    switch (diff) {
    case Difficulty::Easy:   snap_.message = ""; break;
    case Difficulty::Normal: snap_.message = ""; break;
    case Difficulty::Hard:   snap_.message = ""; break;
    }
}

int HGCore::promoteState_(int cur, int next) {
    auto pri = [](int s) {
        if (s == 2) return 3;
        if (s == 1) return 2;
        if (s == 0) return 1;
        return 0;
    };
    return (pri(next) > pri(cur)) ? next : cur;
}

void HGCore::updateChart_(const IdiomFeatures& guess, const EvalResult& res) {
    for (int i = 0; i < 4; i++) {
        std::string ini = guess.initials[i].empty() ? "∅" : guess.initials[i];
        std::string fin = guess.finals[i].empty() ? "∅" : guess.finals[i];

        int nextIni = ansIniSet_.count(ini) ? 1 : 0;
        if (res.mIni[i] == Mark::Green) nextIni = 2;
        int curIni = snap_.chart.iniState.count(ini) ? snap_.chart.iniState[ini] : -1;
        snap_.chart.iniState[ini] = promoteState_(curIni, nextIni);

        int nextFin = ansFinSet_.count(fin) ? 1 : 0;
        if (res.mFin[i] == Mark::Green) nextFin = 2;
        int curFin = snap_.chart.finState.count(fin) ? snap_.chart.finState[fin] : -1;
        snap_.chart.finState[fin] = promoteState_(curFin, nextFin);
    }
}

int HGCore::chooseRandomIndex_(const std::array<bool,4>& revealed) {
    std::vector<int> cand;
    for (int i = 0; i < 4; i++) if (!revealed[i]) cand.push_back(i);
    if (cand.empty()) return -1;
    std::uniform_int_distribution<int> dist(0, (int)cand.size() - 1);
    return cand[dist(rng_)];
}

void HGCore::autoHintOnWrong_() {
    // 这里保留你原来的自动提示逻辑（若你不想显示文字，可只更新 revealed 标记、不写 message）
    switch (snap_.difficulty) {
    case Difficulty::Easy:
        // revealRandomChar_();  // 你若已删掉文字提示，这里仍可保留揭示行为
        break;
    case Difficulty::Normal:
        // revealRandomPinyin_();
        break;
    case Difficulty::Hard:
        break;
    }
}

bool HGCore::submitGuess(const std::string& idiom) {
    if (!snap_.started) {
        snap_.message = "请先选择难度再开始。";
        return false;
    }
    if (snap_.finished) {
        snap_.message = "本局已结束。";
        return false;
    }

    if (!engine_.contains(idiom)) {
        snap_.message = "该成语不在词库中，请重新输入。";
        return false;
    }

    const auto& gF = engine_.featuresOf(idiom);
    EvalResult res = engine_.evaluate(answer_, gF);

    GuessRow row;
    for (int i = 0; i < 4; i++) {
        row.chars[i]    = gF.chars[i];
        row.initials[i] = gF.initials[i];
        row.finals[i]   = gF.finals[i];
        row.tones[i]    = gF.tones[i];

        row.mChar[i] = res.mChar[i];
        row.mIni[i]  = res.mIni[i];
        row.mFin[i]  = res.mFin[i];
        row.mTone[i] = res.mTone[i];
    }
    snap_.history.push_back(row);

    updateChart_(gF, res);

    bool win = true;
    for (int i = 0; i < 4; i++) {
        if (res.mChar[i] != Mark::Green) { win = false; break; }
    }
    if (win) {
        snap_.finished = true;
        snap_.message = " 恭喜猜中！答案就是：" + idiom;
        return true;
    }

    autoHintOnWrong_();
    return true;
}

void HGCore::manualHint() {
    if (!snap_.started) {
        snap_.message = "请先选择难度再开始。";
        return;
    }
    // 你的原手动提示逻辑保持不动（略）
}

std::string HGCore::answerIdiom() const {
    return HGEngine::idiomToString(answer_);
}

} // namespace HandleGame