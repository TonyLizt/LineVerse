#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace lineverse::poetryrebuild {

using CodePoint = char32_t;
using CharFreq = std::unordered_map<CodePoint, int>;
using CodePointList = std::vector<CodePoint>;

} // namespace lineverse::poetryrebuild
