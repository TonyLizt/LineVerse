#include "UIWidgets.h"

#include <algorithm>
#include <utility>

namespace VerseUnfold {

namespace {

std::vector<std::string> splitUtf8Codepoints(const std::string& s) {
    std::vector<std::string> result;
    for (size_t i = 0; i < s.size();) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        size_t len = 1;

        if ((c & 0x80) == 0x00) len = 1;
        else if ((c & 0xE0) == 0xC0) len = 2;
        else if ((c & 0xF0) == 0xE0) len = 3;
        else if ((c & 0xF8) == 0xF0) len = 4;

        if (i + len > s.size()) len = 1;
        result.push_back(s.substr(i, len));
        i += len;
    }
    return result;
}

} // anonymous namespace

// ==================== UIRenderUtils ====================

void UIRenderUtils::renderText(SDL_Renderer* renderer,
                               TTF_Font* font,
                               const std::string& text,
                               int x,
                               int y,
                               SDL_Color color) {
    if (!renderer || !font || text.empty()) {
        return;
    }

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface) {
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect dst{ x, y, surface->w, surface->h };
    SDL_RenderCopy(renderer, texture, nullptr, &dst);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void UIRenderUtils::renderCenteredText(SDL_Renderer* renderer,
                                       TTF_Font* font,
                                       const std::string& text,
                                       SDL_Rect area,
                                       SDL_Color color) {
    if (!renderer || !font || text.empty()) {
        return;
    }

    int textW = 0, textH = 0;
    if (TTF_SizeUTF8(font, text.c_str(), &textW, &textH) != 0) {
        return;
    }

    int x = area.x + (area.w - textW) / 2;
    int y = area.y + (area.h - textH) / 2;
    renderText(renderer, font, text, x, y, color);
}

void UIRenderUtils::renderBox(SDL_Renderer* renderer,
                              SDL_Rect rect,
                              SDL_Color bg,
                              SDL_Color border,
                              int borderSize) {
    if (!renderer) {
        return;
    }

    SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
    SDL_RenderFillRect(renderer, &rect);

    SDL_Rect inner{
        rect.x + borderSize,
        rect.y + borderSize,
        std::max(0, rect.w - borderSize * 2),
        std::max(0, rect.h - borderSize * 2)
    };

    SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderFillRect(renderer, &inner);
}

std::vector<std::string> UIRenderUtils::wrapText(TTF_Font* font,
                                                 const std::string& text,
                                                 int maxWidth) {
    std::vector<std::string> lines;
    if (!font || text.empty()) {
        return lines;
    }

    std::vector<std::string> cps = splitUtf8Codepoints(text);
    std::string current;

    for (const auto& cp : cps) {
        if (cp == "\n") {
            lines.push_back(current);
            current.clear();
            continue;
        }

        std::string candidate = current + cp;
        int w = 0, h = 0;
        TTF_SizeUTF8(font, candidate.c_str(), &w, &h);

        if (w <= maxWidth || current.empty()) {
            current = candidate;
        } else {
            lines.push_back(current);
            current = cp;
        }
    }

    if (!current.empty()) {
        lines.push_back(current);
    }

    return lines;
}

void UIRenderUtils::renderWrappedText(SDL_Renderer* renderer,
                                      TTF_Font* font,
                                      const std::string& text,
                                      int x,
                                      int y,
                                      int maxWidth,
                                      SDL_Color color,
                                      int lineSpacing) {
    if (!renderer || !font) {
        return;
    }

    std::vector<std::string> lines = wrapText(font, text, maxWidth);
    int lineH = TTF_FontHeight(font);
    int currentY = y;

    for (const auto& line : lines) {
        renderText(renderer, font, line, x, currentY, color);
        currentY += lineH + lineSpacing;
    }
}

int UIRenderUtils::measureWrappedTextHeight(TTF_Font* font,
                                            const std::string& text,
                                            int maxWidth,
                                            int lineSpacing) {
    if (!font) {
        return 0;
    }

    std::vector<std::string> lines = wrapText(font, text, maxWidth);
    if (lines.empty()) {
        return 0;
    }

    int lineH = TTF_FontHeight(font);
    return static_cast<int>(lines.size()) * lineH
         + static_cast<int>(lines.size() - 1) * lineSpacing;
}

// ==================== Button ====================

Button::Button(SDL_Rect rect, std::string label, std::function<void()> onClick)
    : rect(rect), label(std::move(label)), onClick(std::move(onClick)) {}

bool Button::contains(int x, int y) const {
    return x >= rect.x && x <= rect.x + rect.w &&
           y >= rect.y && y <= rect.y + rect.h;
}

void Button::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_MOUSEMOTION) {
        isHovered = contains(event.motion.x, event.motion.y);
        return;
    }

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        if (contains(event.button.x, event.button.y)) {
            if (onClick) {
                onClick();
            }
        }
    }
}

void Button::render(SDL_Renderer* renderer, VerseUnfoldAssets* assets) const {
    if (!renderer || !assets) {
        return;
    }

    SDL_Color bg;
    if (selected) {
        bg = SDL_Color{226, 210, 170, 255};
    } else if (isHovered) {
        bg = SDL_Color{235, 228, 210, 255};
    } else {
        bg = SDL_Color{245, 239, 222, 255};
    }

    SDL_Color border{155, 120, 72, 255};
    SDL_Color textColor{60, 50, 40, 255};

    UIRenderUtils::renderBox(renderer, rect, bg, border, 2);

    TTF_Font* font = assets->getFont(VerseUnfoldAssets::FontSize::Medium);
    if (!font) {
        return;
    }

    UIRenderUtils::renderCenteredText(renderer, font, label, rect, textColor);
}

// ==================== InputBox ====================

InputBox::InputBox(SDL_Rect rect, std::string placeholder)
    : rect(rect), placeholder(std::move(placeholder)) {}

void InputBox::eraseLastUtf8Codepoint(std::string& s) {
    if (s.empty()) {
        return;
    }

    s.pop_back();
    while (!s.empty() && (static_cast<unsigned char>(s.back()) & 0xC0) == 0x80) {
        s.pop_back();
    }
}

bool InputBox::contains(int x, int y) const {
    return x >= rect.x && x <= rect.x + rect.w &&
           y >= rect.y && y <= rect.y + rect.h;
}

// ———————————————— 你要求的 setFocused ————————————————
void InputBox::setFocused(bool value) {
    focused = value;
    if (focused) {
        SDL_StartTextInput();
        SDL_SetTextInputRect(&rect);
    } else {
        SDL_StopTextInput();
        editingText.clear();
        composing = false;
    }
}

// ———————————————— 你要求的 clear() ————————————————
void InputBox::clear() {
    text.clear();
    editingText.clear();
    composing = false;
}

// ———————————————— 你要求的 handleEvent ————————————————
void InputBox::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        setFocused(contains(event.button.x, event.button.y));
        return;
    }

    if (!focused) {
        return;
    }

    if (event.type == SDL_TEXTEDITING) {
        editingText = event.edit.text;
        composing = !editingText.empty();
        return;
    }

    if (event.type == SDL_TEXTINPUT) {
        text += event.text.text;
        editingText.clear();
        composing = false;
        return;
    }

    if (event.type == SDL_KEYDOWN &&
        event.key.keysym.sym == SDLK_BACKSPACE &&
        !text.empty()) {
        eraseLastUtf8Codepoint(text);
        return;
    }
}

// ———————————————— 你要求的 render ————————————————
void InputBox::render(SDL_Renderer* renderer, VerseUnfoldAssets* assets) const {
    SDL_Color bg{255, 252, 245, 255};
    SDL_Color border = focused ? SDL_Color{130, 84, 32, 255} : SDL_Color{180, 160, 120, 255};
    UIRenderUtils::renderBox(renderer, rect, bg, border, 2);

    TTF_Font* renderFont = assets->getFont(VerseUnfoldAssets::FontSize::Small);
    if (renderFont == nullptr) {
        return;
    }

    std::string visibleText;
    SDL_Color color;

    if (text.empty() && editingText.empty()) {
        visibleText = placeholder;
        color = SDL_Color{150, 150, 150, 255};
    } else {
        visibleText = text + editingText;
        color = SDL_Color{30, 30, 30, 255};
    }

    int width = 0;
    int height = 0;
    TTF_SizeUTF8(renderFont, visibleText.c_str(), &width, &height);

    while (width > rect.w - 20 && !visibleText.empty()) {
        eraseLastUtf8Codepoint(visibleText);
        TTF_SizeUTF8(renderFont, visibleText.c_str(), &width, &height);
    }

    UIRenderUtils::renderText(
        renderer,
        renderFont,
        visibleText,
        rect.x + 10,
        rect.y + (rect.h - height) / 2,
        color
    );
}

} // namespace VerseUnfold