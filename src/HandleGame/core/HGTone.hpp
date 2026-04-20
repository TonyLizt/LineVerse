#pragma once
#include <string>

namespace HandleGame {

// 两种显示模式
enum class ToneDisplayMode {
    Number,   // 1 2 3 4
    Mark      // ˉ ˊ ˇ ˋ
};

// 数字显示：0/5/无 -> ""
inline std::string toneToNumber(int tone) {
    if (tone >= 1 && tone <= 4) return std::to_string(tone);
    return "";
}

// 符号显示：1->ˉ 2->ˊ 3->ˇ 4->ˋ；0/5/无 -> ""
inline std::string toneToMark(int tone) {
    switch (tone) {
    case 1: return u8"ˉ";
    case 2: return u8"ˊ";
    case 3: return u8"ˇ";
    case 4: return u8"ˋ";
    default: return "";
    }
}

// 统一入口
inline std::string toneToDisplay(int tone, ToneDisplayMode mode) {
    return (mode == ToneDisplayMode::Mark) ? toneToMark(tone) : toneToNumber(tone);
}

inline const char* toneModeName(ToneDisplayMode mode) {
    return (mode == ToneDisplayMode::Mark) ? "符号" : "数字";
}

} // namespace HandleGame