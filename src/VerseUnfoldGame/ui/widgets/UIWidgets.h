#pragma once

#include <SDL.h>
#include <SDL_ttf.h>

#include <functional>
#include <string>
#include <vector>

#include "../VerseUnfoldAssets.h"

namespace VerseUnfold {

class UIRenderUtils {
public:
    static void renderText(SDL_Renderer* renderer,
                           TTF_Font* font,
                           const std::string& text,
                           int x,
                           int y,
                           SDL_Color color);

    static void renderCenteredText(SDL_Renderer* renderer,
                                   TTF_Font* font,
                                   const std::string& text,
                                   SDL_Rect area,
                                   SDL_Color color);

    static void renderBox(SDL_Renderer* renderer,
                          SDL_Rect rect,
                          SDL_Color bg,
                          SDL_Color border,
                          int borderSize = 2);

    static std::vector<std::string> wrapText(TTF_Font* font,
                                             const std::string& text,
                                             int maxWidth);

    static void renderWrappedText(SDL_Renderer* renderer,
                                  TTF_Font* font,
                                  const std::string& text,
                                  int x,
                                  int y,
                                  int maxWidth,
                                  SDL_Color color,
                                  int lineSpacing = 6);

    static int measureWrappedTextHeight(TTF_Font* font,
                                        const std::string& text,
                                        int maxWidth,
                                        int lineSpacing = 6);
};

class Button {
public:
    Button(SDL_Rect rect, std::string label, std::function<void()> onClick);

    void handleEvent(const SDL_Event& event);
    void render(SDL_Renderer* renderer, VerseUnfoldAssets* assets) const;

    void setSelected(bool value) { selected = value; }
    bool isSelected() const { return selected; }

private:
    bool contains(int x, int y) const;

private:
    SDL_Rect rect;
    std::string label;
    std::function<void()> onClick;
    bool isHovered = false;
    bool selected = false;
};
class InputBox {
public:
    InputBox(SDL_Rect rect, std::string placeholder);

    void handleEvent(const SDL_Event& event);
    void render(SDL_Renderer* renderer, VerseUnfoldAssets* assets) const;

    void setFocused(bool value);
    bool isFocused() const { return focused; }
    bool isComposing() const { return composing; }
    bool contains(int x, int y) const;

    const std::string& getText() const { return text; }
    void setText(const std::string& value) { text = value; }
    void clear();

private:
    static void eraseLastUtf8Codepoint(std::string& s);

private:
    SDL_Rect rect;
    std::string text;
    std::string placeholder;

    std::string editingText;   // 正在组合输入的拼音/候选前文字
    bool composing = false;    // 是否处于 IME 组合输入阶段
    bool focused = false;
};
} // namespace VerseUnfold