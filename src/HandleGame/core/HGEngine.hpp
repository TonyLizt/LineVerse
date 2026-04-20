#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

namespace HandleGame {

enum class Mark { Red, Yellow, Green };

struct IdiomFeatures {
    std::array<std::string, 4> chars{};     // UTF-8 单字切片
    std::array<std::string, 4> initials{};  // 声母（"" 表示零声母）
    std::array<std::string, 4> finals{};    // 韵母
    std::array<int, 4> tones{};             // 0/1/2/3/4
};

struct EvalResult {
    std::array<Mark, 4> mChar{};
    std::array<Mark, 4> mIni{};
    std::array<Mark, 4> mFin{};
    std::array<Mark, 4> mTone{};
};

class HGEngine {
public:
    // 从 TSV 加载词库：每行 "成语\t拼音"；拼音空格分 4 个音节。
    // 成功返回 true；失败返回 false，并可在 err 中得到原因。
    bool loadTSV(const std::string& tsvPath, std::string* err = nullptr);

    bool contains(const std::string& idiom) const;
    const IdiomFeatures& featuresOf(const std::string& idiom) const;
    const std::vector<std::string>& allIdioms() const { return all_; }

    EvalResult evaluate(const IdiomFeatures& ans, const IdiomFeatures& guess) const;

    // 工具函数
    static std::vector<std::string> splitUtf8Chars(const std::string& s);
    static std::string idiomToString(const IdiomFeatures& f);
    static std::string buildPinyin(const IdiomFeatures& ans, int idx); // initial+final+tone

private:
    std::unordered_map<std::string, IdiomFeatures> dict_;
    std::vector<std::string> all_;
};

} // namespace HandleGame