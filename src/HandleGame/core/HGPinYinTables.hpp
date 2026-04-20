#pragma once
#include <vector>
#include <string>

namespace HandleGame {

// 你要求的“标准声母表”
inline const std::vector<std::string> kStdInitials = {
    "b","p","m","f",
    "d","t","n","l",
    "g","k","h",
    "j","q","r","x",
    "w","y",
    "zh","ch","sh",
    "z","c","s"
};

// 你要求的“标准韵母表”
inline const std::vector<std::string> kStdFinals = {
    "a","ai","an","ang","ao",
    "e","ei","en","eng","er",
    "i","ia","ian","iang","iao","ie","in","ing","io","iong","iu",
    "o","ong","ou",
    "u","ua","uai","uan","uang","ui","un","uo",
    "ü","üan","üe","ün"//ü = alt + 129
};

//这是我的四个声调符号ˉ ˊ ˇ ˋ

} // namespace HandleGame