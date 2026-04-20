#pragma once

#include <SDL_ttf.h>

#include <map>
#include <string>
#include <vector>

namespace VerseUnfold {

class VerseUnfoldAssets {
public:
    enum class FontSize { Small, Medium, Large };

    VerseUnfoldAssets() = default;
    ~VerseUnfoldAssets();

    bool load();
    TTF_Font* getFont(FontSize size) const;
    const std::string& getResolvedFontPath() const { return resolvedFontPath; }

private:
    bool loadFontsFromPath(const std::string& path);

private:
    std::map<FontSize, TTF_Font*> fonts;
    std::vector<std::string> fontCandidates = {
    // Windows 系统字体，优先使用
    "C:/Windows/Fonts/msyh.ttc",
    "C:/Windows/Fonts/msyhbd.ttc",
    "C:/Windows/Fonts/simhei.ttf",
    "C:/Windows/Fonts/simsun.ttc",

    // 从 build/bin 启动时可用
    "assets/VerseUnfoldGame/font/simsun.ttc",
    "../assets/VerseUnfoldGame/font/simsun.ttc",
    "../../assets/VerseUnfoldGame/font/simsun.ttc",
    "../../../assets/VerseUnfoldGame/font/simsun.ttc",

    // 从项目根目录启动 .\\build\\bin\\LineVerse.exe 时可用
    "build/bin/assets/VerseUnfoldGame/font/simsun.ttc"
    };
    std::string resolvedFontPath;
};

} // namespace VerseUnfold
