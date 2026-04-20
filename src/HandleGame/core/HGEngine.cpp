#include "HGEngine.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <unordered_map>

namespace HandleGame {

// 声母表（最长匹配优先）
static const std::vector<std::string> kInitials = {
    "zh","ch","sh",
    "b","p","m","f","d","t","n","l","g","k","h",
    "j","q","x","r","z","c","s","y","w"
    //"" // 零声母
};

std::vector<std::string> HGEngine::splitUtf8Chars(const std::string& s) {
    std::vector<std::string> out;
    out.reserve(s.size());

    for (size_t i = 0; i < s.size();) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        size_t len = 1;
        if ((c & 0x80) == 0x00) len = 1;
        else if ((c & 0xE0) == 0xC0) len = 2;
        else if ((c & 0xF0) == 0xE0) len = 3;
        else if ((c & 0xF8) == 0xF0) len = 4;
        else throw std::runtime_error("Invalid UTF-8");

        out.push_back(s.substr(i, len));
        i += len;
    }
    return out;
}

static std::pair<std::string,int> normalizeToneMarks(const std::string& syll) {
    // ü 系列统一转 v，避免多字节 char 常量。
    static const std::unordered_map<std::string, std::pair<char,int>> mp = {
        {"ā",{'a',1}}, {"á",{'a',2}}, {"ǎ",{'a',3}}, {"à",{'a',4}},
        {"ō",{'o',1}}, {"ó",{'o',2}}, {"ǒ",{'o',3}}, {"ò",{'o',4}},
        {"ē",{'e',1}}, {"é",{'e',2}}, {"ě",{'e',3}}, {"è",{'e',4}},
        {"ī",{'i',1}}, {"í",{'i',2}}, {"ǐ",{'i',3}}, {"ì",{'i',4}},
        {"ū",{'u',1}}, {"ú",{'u',2}}, {"ǔ",{'u',3}}, {"ù",{'u',4}},
        {"ǖ",{'v',1}}, {"ǘ",{'v',2}}, {"ǚ",{'v',3}}, {"ǜ",{'v',4}},
        {"ü", {'v',0}}
    };

    auto chars = HGEngine::splitUtf8Chars(syll);
    std::string base;
    int tone = 0;

    for (auto& ch : chars) {
        if (ch.size() == 1 && std::isdigit(static_cast<unsigned char>(ch[0]))) {
            base.push_back(ch[0]);
            continue;
        }
        auto it = mp.find(ch);
        if (it != mp.end()) {
            base.push_back(it->second.first);
            if (it->second.second != 0) tone = it->second.second;
        } else {
            base += ch;
        }
    }
    return {base, tone};
}

static std::tuple<std::string,std::string,int> parseSyllable(std::string syll) {
    int tone = 0;

    // 数字声调：shi4
    if (!syll.empty() && std::isdigit(static_cast<unsigned char>(syll.back()))) {
        tone = syll.back() - '0';
        syll.pop_back();
        if (tone == 5) tone = 0;
    } else {
        // 声调符号：shì
        auto norm = normalizeToneMarks(syll);
        syll = norm.first;
        if (norm.second != 0) tone = norm.second;

        // 兜底：极少情况残留数字
        if (!syll.empty() && std::isdigit(static_cast<unsigned char>(syll.back()))) {
            tone = syll.back() - '0';
            syll.pop_back();
            if (tone == 5) tone = 0;
        }
    }

    // 声母最长匹配
    std::string initial;
    for (const auto& ini : kInitials) {
        if (ini.empty()) continue;
        if (syll.rfind(ini, 0) == 0) { // starts_with
            initial = ini;
            break;
        }
    }
    std::string final_ = syll.substr(initial.size());
    return {initial, final_, tone};
}

static bool buildFeatures4(const std::string& idiom, const std::string& pinyinLine, IdiomFeatures& out) {
    auto chars = HGEngine::splitUtf8Chars(idiom);
    if (chars.size() != 4) return false;

    std::vector<std::string> pys;
    {
        std::istringstream iss(pinyinLine);
        std::string token;
        while (iss >> token) pys.push_back(token);
    }
    if (pys.size() != 4) return false;

    for (int i = 0; i < 4; i++) {
        out.chars[i] = chars[i];
        auto [ini, fin, tone] = parseSyllable(pys[i]);
        out.initials[i] = ini;
        out.finals[i] = fin;
        out.tones[i] = tone;
    }
    return true;
}

bool HGEngine::loadTSV(const std::string& tsvPath, std::string* err) {
    std::ifstream fin(tsvPath);
    if (!fin) {
        if (err) *err = "cannot open: " + tsvPath;
        return false;
    }

    dict_.clear();
    all_.clear();
    dict_.reserve(40000);
    all_.reserve(40000);

    std::string line;
    while (std::getline(fin, line)) {
        if (line.empty()) continue;
        if (!line.empty() && line.back() == '\r') line.pop_back();

        auto pos = line.find('\t');
        if (pos == std::string::npos) continue;

        std::string idiom = line.substr(0, pos);
        std::string pinyin = line.substr(pos + 1);

        IdiomFeatures f;
        if (!buildFeatures4(idiom, pinyin, f)) continue;

        dict_.emplace(idiom, f);
        all_.push_back(idiom);
    }

    if (all_.empty()) {
        if (err) *err = "no valid 4-char idioms loaded";
        return false;
    }
    return true;
}

bool HGEngine::contains(const std::string& idiom) const {
    return dict_.find(idiom) != dict_.end();
}

const IdiomFeatures& HGEngine::featuresOf(const std::string& idiom) const {
    return dict_.at(idiom);
}

// 两趟扫描 + Counter（处理重复元素）
template <class T>
static std::array<Mark,4> score4(const std::array<T,4>& ans, const std::array<T,4>& g) {
    std::array<Mark,4> st{Mark::Red, Mark::Red, Mark::Red, Mark::Red};

    std::unordered_map<T,int> counter;
    counter.reserve(8);
    for (int i = 0; i < 4; i++) counter[ans[i]]++;

    // GREEN
    for (int i = 0; i < 4; i++) {
        if (g[i] == ans[i]) {
            st[i] = Mark::Green;
            counter[g[i]]--;
        }
    }

    // YELLOW / RED
    for (int i = 0; i < 4; i++) {
        if (st[i] == Mark::Green) continue;
        auto it = counter.find(g[i]);
        if (it != counter.end() && it->second > 0) {
            st[i] = Mark::Yellow;
            it->second--;
        } else {
            st[i] = Mark::Red;
        }
    }

    return st;
}

EvalResult HGEngine::evaluate(const IdiomFeatures& ans, const IdiomFeatures& guess) const {
    EvalResult r;
    r.mChar = score4(ans.chars, guess.chars);
    r.mIni  = score4(ans.initials, guess.initials);
    r.mFin  = score4(ans.finals, guess.finals);
    r.mTone = score4(ans.tones, guess.tones);
    return r;
}

std::string HGEngine::idiomToString(const IdiomFeatures& f) {
    return f.chars[0] + f.chars[1] + f.chars[2] + f.chars[3];
}

std::string HGEngine::buildPinyin(const IdiomFeatures& ans, int idx) {
    if (idx < 0 || idx >= 4) return "";
    std::string base = ans.initials[idx] + ans.finals[idx];
    if (base.empty()) base = ans.finals[idx];
    if (ans.tones[idx] > 0) base += std::to_string(ans.tones[idx]);
    return base;
}

} // namespace HandleGame