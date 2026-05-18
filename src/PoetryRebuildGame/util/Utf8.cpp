#include "Utf8.h"

#include <stdexcept>

namespace lineverse::poetryrebuild {

CodePointList Utf8::toCodePoints(const std::string& text) {
    CodePointList output;
    output.reserve(text.size());

    for (std::size_t i = 0; i < text.size();) {
        const unsigned char c = static_cast<unsigned char>(text[i]);

        if (c < 0x80) {
            output.push_back(static_cast<CodePoint>(c));
            ++i;
            continue;
        }

        if ((c >> 5) == 0x6) {
            if (i + 1 >= text.size()) {
                break;
            }
            CodePoint cp = ((c & 0x1F) << 6)
                | (static_cast<unsigned char>(text[i + 1]) & 0x3F);
            output.push_back(cp);
            i += 2;
            continue;
        }

        if ((c >> 4) == 0xE) {
            if (i + 2 >= text.size()) {
                break;
            }
            CodePoint cp = ((c & 0x0F) << 12)
                | ((static_cast<unsigned char>(text[i + 1]) & 0x3F) << 6)
                | (static_cast<unsigned char>(text[i + 2]) & 0x3F);
            output.push_back(cp);
            i += 3;
            continue;
        }

        if ((c >> 3) == 0x1E) {
            if (i + 3 >= text.size()) {
                break;
            }
            CodePoint cp = ((c & 0x07) << 18)
                | ((static_cast<unsigned char>(text[i + 1]) & 0x3F) << 12)
                | ((static_cast<unsigned char>(text[i + 2]) & 0x3F) << 6)
                | (static_cast<unsigned char>(text[i + 3]) & 0x3F);
            output.push_back(cp);
            i += 4;
            continue;
        }

        ++i;
    }

    return output;
}

std::string Utf8::fromCodePoint(CodePoint cp) {
    std::string output;

    if (cp <= 0x7F) {
        output.push_back(static_cast<char>(cp));
        return output;
    }

    if (cp <= 0x7FF) {
        output.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
        output.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        return output;
    }

    if (cp <= 0xFFFF) {
        output.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
        output.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        return output;
    }

    output.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
    output.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
    output.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    output.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    return output;
}

std::string Utf8::fromCodePoints(const CodePointList& codePoints) {
    std::string output;
    for (CodePoint cp : codePoints) {
        output += fromCodePoint(cp);
    }
    return output;
}

} // namespace lineverse::poetryrebuild
