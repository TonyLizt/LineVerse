#include "VerseUnfoldAssets.h"

#include <iostream>

namespace VerseUnfold {

VerseUnfoldAssets::~VerseUnfoldAssets() {
    for (auto& [_, font] : fonts) {
        if (font != nullptr) {
            TTF_CloseFont(font);
        }
    }
    fonts.clear();
    if (TTF_WasInit()) {
        TTF_Quit();
    }
}

bool VerseUnfoldAssets::loadFontsFromPath(const std::string& path) {
    fonts[FontSize::Small] = TTF_OpenFont(path.c_str(), 20);
    fonts[FontSize::Medium] = TTF_OpenFont(path.c_str(), 28);
    fonts[FontSize::Large] = TTF_OpenFont(path.c_str(), 44);

    for (const auto& [_, font] : fonts) {
        if (font == nullptr) {
            for (auto& [__, loadedFont] : fonts) {
                if (loadedFont != nullptr) {
                    TTF_CloseFont(loadedFont);
                    loadedFont = nullptr;
                }
            }
            return false;
        }
    }
    resolvedFontPath = path;
    return true;
}

bool VerseUnfoldAssets::load() {
    if (TTF_Init() == -1) {
        std::cerr << "[VerseUnfoldAssets] TTF_Init failed: " << TTF_GetError() << std::endl;
        return false;
    }

    for (const auto& candidate : fontCandidates) {
        if (loadFontsFromPath(candidate)) {
            return true;
        }
    }

    std::cerr << "[VerseUnfoldAssets] Failed to load font. Tried:" << std::endl;
    for (const auto& candidate : fontCandidates) {
        std::cerr << "  - " << candidate << std::endl;
    }
    return false;
}

TTF_Font* VerseUnfoldAssets::getFont(FontSize size) const {
    auto it = fonts.find(size);
    return it == fonts.end() ? nullptr : it->second;
}

} // namespace VerseUnfold
