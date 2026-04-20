#include "VerseUnfoldRenderer.h"

#include "widgets/UIWidgets.h"

namespace VerseUnfold {

void VerseUnfoldRenderer::drawRect(int x, int y, int w, int h, SDL_Color color, bool filled) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect rect{x, y, w, h};
    if (filled) {
        SDL_RenderFillRect(renderer, &rect);
    } else {
        SDL_RenderDrawRect(renderer, &rect);
    }
}

void VerseUnfoldRenderer::drawText(const std::string& text, int x, int y, SDL_Color color,
                                   VerseUnfoldAssets::FontSize size) {
    if (assets == nullptr) {
        return;
    }
    UIRenderUtils::renderText(renderer, assets->getFont(size), text, x, y, color);
}

void VerseUnfoldRenderer::drawTextCentered(const std::string& text, int x, int y, int w, int h,
                                           SDL_Color color, VerseUnfoldAssets::FontSize size) {
    if (assets == nullptr) {
        return;
    }
    UIRenderUtils::renderCenteredText(renderer, assets->getFont(size), text, SDL_Rect{x, y, w, h}, color);
}

} // namespace VerseUnfold
