#pragma once

#include <string>
#include <vector>

#include "../common/Types.h"

namespace lineverse::poetryrebuild {

class Utf8 {
public:
    static CodePointList toCodePoints(const std::string& text);
    static std::string fromCodePoint(CodePoint cp);
    static std::string fromCodePoints(const CodePointList& codePoints);
};

} // namespace lineverse::poetryrebuild
