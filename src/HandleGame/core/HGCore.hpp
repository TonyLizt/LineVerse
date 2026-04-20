#pragma once

#include "HGEngine.hpp"

#include <array>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace HandleGame {

enum class Difficulty {
    Easy,   // 错一次自动揭示答案中的一个字
    Normal, // 错一次自动揭示答案中的一个拼音
    Hard    // 不自动揭示
};

// 一行历史：每格保存 “汉字/声母/韵母/声调” 及其对应 Mark（红黄绿）
struct GuessRow {
    std::array<std::string, 4> chars;
    std::array<std::string, 4> initials; // 可能为空(零声母)
    std::array<std::string, 4> finals;
    std::array<int, 4> tones;            // 0/1/2/3/4

    std::array<Mark, 4> mChar;
    std::array<Mark, 4> mIni;
    std::array<Mark, 4> mFin;
    std::array<Mark, 4> mTone;
};

struct ChartSnapshot {
    // -1: 未使用（白），0: 红，1: 黄，2: 绿
    std::unordered_map<std::string, int> iniState;
    std::unordered_map<std::string, int> finState;
};

struct CoreSnapshot {
    bool started = false;
    bool finished = false;
    Difficulty difficulty = Difficulty::Hard;

    std::vector<GuessRow> history;
    ChartSnapshot chart;

    std::string message;

    std::array<bool, 4> revealedChar{{false,false,false,false}};
    std::array<bool, 4> revealedPinyin{{false,false,false,false}};
};

class HGCore {
public:
    explicit HGCore(HGEngine& engine);

    // ✅ 配置不同难度对应的词库 TSV 路径（在 HandleGame::start() 中设置）
    void setLexiconPaths(std::string easyTsv, std::string normalTsv, std::string hardTsv);

    void newGame(Difficulty diff, uint32_t seed);

    // 词库不存在 -> false；存在 -> true（会新增一行历史并更新状态）
    bool submitGuess(const std::string& idiom);

    // 手动提示：两段式（先拼音后汉字）
    void manualHint();

    const CoreSnapshot& snapshot() const { return snap_; }

    std::string answerIdiom() const;
    const IdiomFeatures& answerFeatures() const { return answer_; }

private:
    HGEngine& engine_;

    // ✅ 不同难度的词库路径（由 setLexiconPaths() 注入）
    std::string tsvEasy_;
    std::string tsvNormal_;
    std::string tsvHard_;
    std::string currentTsv_; // 当前已加载的 TSV（避免重复 IO）

    std::mt19937 rng_;

    IdiomFeatures answer_{};

    std::unordered_set<std::string> ansIniSet_;
    std::unordered_set<std::string> ansFinSet_;

    int hintStage_ = 0; // 0 -> show pinyin, 1 -> show char
    int hintIndex_ = -1;

    CoreSnapshot snap_;

private:
    void resetForNewGame_();

    // 确保当前 engine_ 已加载对应难度的词库；失败则设置 snap_.message 并返回 false
    bool ensureLexiconLoaded_(Difficulty diff);

    static int promoteState_(int cur, int next);

    void updateChart_(const IdiomFeatures& guess, const EvalResult& res);

    void autoHintOnWrong_();

    int chooseRandomIndex_(const std::array<bool,4>& revealed);
};

} // namespace HandleGame