#pragma once

#include <string>

struct HintItem {
    int level = 0;
    std::string type;   // dynasty / type / emotion / ...
    std::string text;   // 给玩家看的提示文本
    std::string value;  // 给程序做筛选的标准值
};