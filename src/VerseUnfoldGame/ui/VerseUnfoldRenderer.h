#pragma once

#include <SDL.h>

#include <string>

#include "VerseUnfoldAssets.h"

namespace VerseUnfold {

class VerseUnfoldRenderer {
public:
    VerseUnfoldRenderer(SDL_Renderer* renderer, VerseUnfoldAssets* assets)
        : renderer(renderer), assets(assets) {}

    void drawRect(int x, int y, int w, int h, SDL_Color color, bool filled = true);
    void drawText(const std::string& text, int x, int y, SDL_Color color,
                  VerseUnfoldAssets::FontSize size = VerseUnfoldAssets::FontSize::Medium);
    void drawTextCentered(const std::string& text, int x, int y, int w, int h, SDL_Color color,
                          VerseUnfoldAssets::FontSize size = VerseUnfoldAssets::FontSize::Medium);

private:
    SDL_Renderer* renderer;
    VerseUnfoldAssets* assets;
};

} // namespace VerseUnfold
