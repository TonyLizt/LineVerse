#include "SdlCardWindow.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <future>
#include <chrono>

#include "../util/Utf8.h"

namespace fs = std::filesystem;

namespace lineverse::poetryrebuild {

namespace {

constexpr int kWindowWidth = 1200;
constexpr int kWindowHeight = 800;

constexpr int kCardHeight = 150;
constexpr int kSquareSize = 80;
constexpr int kSquareOffsetY = 25;
constexpr int kFontSize = 70;

constexpr int kOrderFontSize = 30;
constexpr int kOrderPaddingLeft = 10;
constexpr int kOrderPaddingTop = 0;

constexpr int kAlertFontSize = 20;
constexpr int kAlertTop = 20;
constexpr int kAlertHeight = 40;
constexpr int kAlertGap = 10;
constexpr int kAlertMaxCount = 3;
constexpr Uint32 kAlertLifeMs = 3000;
constexpr Uint32 kAlertFadeMs = 300;
constexpr int kAlertLeading = 20;
constexpr int kAlertTrailing = 20;
constexpr int kAlertCloseSize = 18;
constexpr int kAlertTextGap = 16;

constexpr int kFirstRowBottom = 40;
constexpr int kSecondRowBottom = 140;

constexpr int kRowMaxCount = 11;
constexpr int kLiftOffsetY = 50;

// 顶部 UI
constexpr int kTopMargin = 40;
constexpr int kTopCenterWidth = 408;
constexpr int kTopCenterHeight = 60;

constexpr int kTopSideWidth = 160;
constexpr int kTopSideHeight = 60;
constexpr int kTopSideGapToCenter = 20;

constexpr int kTopLeftSmallX = 50;
constexpr int kTopLeftSmallY = 40;
constexpr int kTopLeftSmallSize = 60;

// 右侧按钮列
constexpr int kRightColumnSize = 60;
constexpr int kRightColumnCount = 4;
constexpr int kRightColumnGap = 10;
constexpr int kRightColumnRightMargin = 20;
constexpr int kRightColumnTopMargin = 40;

constexpr int kIconPadding = 8;

// 已出牌区
constexpr int kPlayedAreaY = 140;
constexpr int kPlayedCardWidth = 50;
constexpr int kPlayedCardHeight = 75;
constexpr int kPlayedGroupGap = 20;
constexpr int kPlayedRowGap = 20;
constexpr int kPlayedMaxRows = 3;
constexpr int kPlayedMaxCardsPerRow = 9;

constexpr int kHintBadgeMinSize = 22;
constexpr int kHintBadgePaddingX = 8;
constexpr int kHintBadgeOffsetX = 6;
constexpr int kHintBadgeOffsetY = -6;
constexpr int kHintBadgeFontSize = 16;

constexpr int kTimerFontSize = 500;
constexpr Uint8 kTimerAlpha = 160;
constexpr int kTimerMaxSeconds = 500;
constexpr int kTimerOffsetY = -160;

constexpr int kScoreFontSize = 65;
constexpr int kScoreStatOffsetY = 42;
constexpr Uint32 kMediumComboWindowMs = 8000;
constexpr int kOptimalAnswerBonus = 20;

constexpr int kScoreStatFontSize = 20;

constexpr Uint32 kScoreFlyLifeMs = 900;
constexpr int kScoreFlyStartOffsetY = 40;
constexpr SDL_Color kScoreFlyColor{244, 200, 41, 255};
constexpr int kScoreFlyEndSize = 20;

constexpr int kTimerBgSize = 540;
constexpr Uint8 kTimerBgAlpha = 80;

constexpr int kEndPanelWidth = 500;
constexpr int kEndPanelHeight = 644;
constexpr Uint8 kEndOverlayAlpha = 128;   // 50%

constexpr int kEndTitleTop = 40;
constexpr int kEndTitleFontSize = 40;
constexpr int kEndScoreFontSize = 150;
constexpr int kEndScoreGap = 0;

constexpr int kEndAnswerCardWidth = 30;
constexpr int kEndAnswerGroupGap = 10;
constexpr int kEndAnswerRowGap = 10;
constexpr int kEndAnswerMaxRows = 3;
constexpr int kEndAnswerTopGap = 15;

constexpr int kEndMetaTopGap = 20;
constexpr int kEndMetaRowGap = 10;
constexpr int kEndMetaFontSize = 30;
constexpr int kEndMetaPaddingX = 40;

constexpr int kEndButtonWidth = 220;
constexpr int kEndButtonHeight = 64;
constexpr int kEndButtonGap = 24;
constexpr int kEndButtonBottom = 32;
constexpr int kEndButtonFontSize = 28;

constexpr int kEndPanelCornerRadius = 20;

constexpr int kLeaveConfirmWidth = 500;
constexpr int kLeaveConfirmHeight = 200;
constexpr int kLeaveConfirmCornerRadius = 20;
constexpr Uint8 kLeaveConfirmOverlayAlpha = 128;   // #000000 50%

constexpr int kLeaveConfirmTitleFontSize = 28;
constexpr int kLeaveConfirmButtonFontSize = 24;

constexpr int kLeaveConfirmTitleTop = 42;
constexpr int kLeaveConfirmButtonHeight = 58;
constexpr SDL_Color kLeaveConfirmPanelColor{255, 255, 255, 245};
constexpr SDL_Color kLeaveConfirmBorderColor{220, 220, 220, 255};
constexpr SDL_Color kLeaveConfirmSeparatorColor{225, 225, 225, 255};

constexpr SDL_Color kLoadingBgColor{255, 255, 255, 255};
constexpr int kLoadingTextFontSize = 34;
constexpr int kLoadingTextGap = 28;
constexpr Uint32 kLoadingFadeMs = 550;
constexpr Uint32 kStageFadeMs = 550;
constexpr int kLoadingGifMaxSize = 260;

SDL_Color gCurrentRoundBgColor{249, 254, 255, 255};
SDL_Window* gTransferredWindow = nullptr;
SDL_Renderer* gTransferredRenderer = nullptr;
bool gTransferredContextOwned = true;

struct LeaveConfirmButtons {
    SDL_Rect panel{};
    SDL_Rect leaveGame{};
    SDL_Rect resumeGame{};
};

struct SdlTextureGuard {
    SDL_Texture* texture = nullptr;
    ~SdlTextureGuard() {
        if (texture != nullptr) {
            SDL_DestroyTexture(texture);
        }
    }
};

struct SdlWindowGuard {
    SDL_Window* window = nullptr;
    bool owns = true;
    ~SdlWindowGuard() {
        if (owns && window != nullptr) {
            SDL_DestroyWindow(window);
        }
    }
};

struct SdlRendererGuard {
    SDL_Renderer* renderer = nullptr;
    bool owns = true;
    ~SdlRendererGuard() {
        if (owns && renderer != nullptr) {
            SDL_DestroyRenderer(renderer);
        }
    }
};

struct TtfFontGuard {
    TTF_Font* font = nullptr;
    ~TtfFontGuard() {
        if (font != nullptr) {
            TTF_CloseFont(font);
        }
    }
};

struct FontCandidate {
    std::string label;
    fs::path path;
    TTF_Font* font = nullptr;
    bool loaded = false;
};

struct GlyphRenderInfo {
    TTF_Font* font = nullptr;
    std::string fontLabel;
    bool available = false;
};

struct UiButtonLayout {
    SDL_Rect back{};
    SDL_Rect hint{};
    SDL_Rect undo{};
    SDL_Rect redo{};
    SDL_Rect play{};
};

struct HandCard {
    int originIndex = -1;
    std::string text;
};

struct PlayedGroupState {
    std::vector<HandCard> cards;
    std::string text;

    int gainedScore = 0;
    bool isOptimalAnswer = false;
    int comboAfterPlay = 0;
};

struct AlertItem {
    std::string text;
    Uint32 bornAt = 0;
    bool closing = false;
    Uint32 closingAt = 0;
};

struct AlertLayout {
    SDL_Rect bubble{};
    SDL_Rect closeRect{};
};

struct ScoreFlyAnim {
    std::string text;
    Uint32 bornAt = 0;
    int startX = 0;
    int startY = 0;
    int endX = 0;
    int endY = 0;
};

struct EndOverlayButtons {
    SDL_Rect backHome{};
    SDL_Rect replay{};
};

SDL_Renderer* createRendererWithFallback(SDL_Window* window) {
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (renderer != nullptr) {
        return renderer;
    }

    return SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
}

TTF_Font* openFontWithFallback(
    const std::vector<FontCandidate>& fonts,
    const std::vector<fs::path>& preferredPaths,
    int pointSize
) {
    for (const auto& path : preferredPaths) {
        if (!path.empty() && fs::exists(path)) {
            if (TTF_Font* font = TTF_OpenFont(path.string().c_str(), pointSize)) {
                return font;
            }
        }
    }

    for (const auto& candidate : fonts) {
        if (!candidate.loaded || candidate.path.empty()) {
            continue;
        }
        if (TTF_Font* font = TTF_OpenFont(candidate.path.string().c_str(), pointSize)) {
            return font;
        }
    }

    return nullptr;
}

bool pointInRect(int px, int py, const SDL_Rect& rect) {
    return px >= rect.x && px < (rect.x + rect.w) &&
           py >= rect.y && py < (rect.y + rect.h);
}

SDL_Rect liftedRect(const SDL_Rect& rect, bool selected) {
    SDL_Rect out = rect;
    if (selected) {
        out.y -= kLiftOffsetY;
    }
    return out;
}

UiButtonLayout buildUiButtonLayout() {
    UiButtonLayout layout{};

    layout.back = SDL_Rect{
        kTopLeftSmallX,
        kTopLeftSmallY,
        kTopLeftSmallSize,
        kTopLeftSmallSize
    };

    const int rightX = kWindowWidth - kRightColumnRightMargin - kRightColumnSize;

    layout.hint = SDL_Rect{
        rightX,
        kRightColumnTopMargin + 0 * (kRightColumnSize + kRightColumnGap),
        kRightColumnSize,
        kRightColumnSize
    };

    layout.undo = SDL_Rect{
        rightX,
        kRightColumnTopMargin + 1 * (kRightColumnSize + kRightColumnGap),
        kRightColumnSize,
        kRightColumnSize
    };

    layout.redo = SDL_Rect{
        rightX,
        kRightColumnTopMargin + 2 * (kRightColumnSize + kRightColumnGap),
        kRightColumnSize,
        kRightColumnSize
    };

    layout.play = SDL_Rect{
        rightX,
        kRightColumnTopMargin + 3 * (kRightColumnSize + kRightColumnGap),
        kRightColumnSize,
        kRightColumnSize
    };

    return layout;
}

std::vector<FontCandidate> buildFontCandidates(const fs::path& fontRoot) {
    std::vector<FontCandidate> fonts = {
        {"font1.ttf",  fontRoot / "font1.ttf",  nullptr, false},
        {"font4.woff", fontRoot / "font4.woff", nullptr, false},
        {"font2.ttf",  fontRoot / "font2.ttf",  nullptr, false},
        {"font3.ttf",  fontRoot / "font3.ttf",  nullptr, false},
    };

#ifdef _WIN32
    fonts.push_back({"system: msyh.ttc",    fs::path("C:/Windows/Fonts/msyh.ttc"),    nullptr, false});
    fonts.push_back({"system: simhei.ttf",  fs::path("C:/Windows/Fonts/simhei.ttf"),  nullptr, false});
    fonts.push_back({"system: simsun.ttc",  fs::path("C:/Windows/Fonts/simsun.ttc"),  nullptr, false});
    fonts.push_back({"system: kaiu.ttf",    fs::path("C:/Windows/Fonts/kaiu.ttf"),    nullptr, false});
    fonts.push_back({"system: mingliu.ttc", fs::path("C:/Windows/Fonts/mingliu.ttc"), nullptr, false});
#elif defined(__APPLE__)
    fonts.push_back({"system: PingFang.ttc",         fs::path("/System/Library/Fonts/PingFang.ttc"),         nullptr, false});
    fonts.push_back({"system: STHeiti Light.ttc",    fs::path("/System/Library/Fonts/STHeiti Light.ttc"),    nullptr, false});
    fonts.push_back({"system: Hiragino Sans GB.ttc", fs::path("/System/Library/Fonts/Hiragino Sans GB.ttc"), nullptr, false});
#else
    fonts.push_back({"system: NotoSansCJK-Regular.ttc", fs::path("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"), nullptr, false});
    fonts.push_back({"system: NotoSansCJK-Regular.ttc", fs::path("/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc"), nullptr, false});
    fonts.push_back({"system: WenQuanYi Zen Hei.ttc",   fs::path("/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc"),           nullptr, false});
#endif

    return fonts;
}

GlyphRenderInfo resolveGlyphRenderInfo(
    const std::vector<FontCandidate>& fonts,
    const std::string& utf8Char
) {
    GlyphRenderInfo info;

    const auto cps = Utf8::toCodePoints(utf8Char);
    if (cps.empty()) {
        return info;
    }

    const Uint32 codepoint = static_cast<Uint32>(cps.front());

    for (const auto& candidate : fonts) {
        if (!candidate.loaded || candidate.font == nullptr) {
            continue;
        }

        if (TTF_GlyphIsProvided32(candidate.font, codepoint)) {
            info.font = candidate.font;
            info.fontLabel = candidate.label;
            info.available = true;
            return info;
        }
    }

    return info;
}

void buildGlyphCacheAndLogOnce(
    const std::vector<FontCandidate>& fonts,
    const std::vector<std::string>& chars,
    std::unordered_map<std::string, GlyphRenderInfo>& glyphCache
) {
    glyphCache.clear();

    std::unordered_set<std::string> uniqueChars(chars.begin(), chars.end());
    std::vector<std::string> orderedUnique(uniqueChars.begin(), uniqueChars.end());
    std::sort(orderedUnique.begin(), orderedUnique.end());

    for (const auto& ch : orderedUnique) {
        GlyphRenderInfo info = resolveGlyphRenderInfo(fonts, ch);

        if (info.available) {
            std::cout << "[FontFallback] 字「" << ch << "」用了 " << info.fontLabel << "\n";
        } else {
            std::cout << "[FontFallback] 字「" << ch
                      << "」在自带字体与系统字体中都缺字\n";
        }

        glyphCache[ch] = std::move(info);
    }
}

void drawUtf8Text(
    SDL_Renderer* renderer,
    const std::unordered_map<std::string, GlyphRenderInfo>& glyphCache,
    const std::string& text,
    const SDL_Rect& boxRect
) {
    if (renderer == nullptr || text.empty()) {
        return;
    }

    const auto it = glyphCache.find(text);
    if (it == glyphCache.end() || !it->second.available || it->second.font == nullptr) {
        return;
    }

    TTF_Font* resolvedFont = it->second.font;

    SDL_Color textColor{20, 20, 20, 255};
    SDL_Surface* textSurface = TTF_RenderUTF8_Blended(resolvedFont, text.c_str(), textColor);
    if (textSurface == nullptr) {
        return;
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (textTexture == nullptr) {
        SDL_FreeSurface(textSurface);
        return;
    }

    const int srcW = textSurface->w;
    const int srcH = textSurface->h;
    SDL_FreeSurface(textSurface);

    if (srcW <= 0 || srcH <= 0) {
        SDL_DestroyTexture(textTexture);
        return;
    }

    const float scaleX = static_cast<float>(boxRect.w - 8) / static_cast<float>(srcW);
    const float scaleY = static_cast<float>(boxRect.h - 8) / static_cast<float>(srcH);
    const float scale = std::min(1.0f, std::min(scaleX, scaleY));

    const int dstW = static_cast<int>(std::round(srcW * scale));
    const int dstH = static_cast<int>(std::round(srcH * scale));

    SDL_Rect dstRect{
        boxRect.x + (boxRect.w - dstW) / 2,
        boxRect.y + (boxRect.h - dstH) / 2,
        dstW,
        dstH
    };

    SDL_RenderCopy(renderer, textTexture, nullptr, &dstRect);
    SDL_DestroyTexture(textTexture);
}

CharFreq buildFreqFromHandCards(const std::vector<HandCard>& handCards) {
    CharFreq freq;
    for (const auto& card : handCards) {
        const auto cps = Utf8::toCodePoints(card.text);
        if (!cps.empty()) {
            freq[cps.front()] += 1;
        }
    }
    return freq;
}

CharFreq buildFreqFromText(const std::string& text) {
    CharFreq freq;
    const auto cps = Utf8::toCodePoints(text);
    for (const auto cp : cps) {
        freq[cp] += 1;
    }
    return freq;
}

bool canComposeTextFromFreq(const std::string& text, const CharFreq& availableFreq) {
    const CharFreq need = buildFreqFromText(text);
    for (const auto& [cp, cnt] : need) {
        auto it = availableFreq.find(cp);
        if (it == availableFreq.end() || it->second < cnt) {
            return false;
        }
    }
    return true;
}

std::string buildSelectedText(
    const std::vector<HandCard>& handCards,
    const std::vector<int>& selectedSequence
) {
    std::string out;
    for (const int idx : selectedSequence) {
        if (idx >= 0 && idx < static_cast<int>(handCards.size())) {
            out += handCards[static_cast<std::size_t>(idx)].text;
        }
    }
    return out;
}

void rebuildSelectionOrder(
    const std::vector<int>& selectedSequence,
    std::vector<int>& selectedOrder
) {
    std::fill(selectedOrder.begin(), selectedOrder.end(), 0);
    for (std::size_t i = 0; i < selectedSequence.size(); ++i) {
        const int idx = selectedSequence[i];
        if (idx >= 0 && idx < static_cast<int>(selectedOrder.size())) {
            selectedOrder[static_cast<std::size_t>(idx)] = static_cast<int>(i) + 1;
        }
    }
}

void toggleSelectionOrder(
    int clickedIndex,
    std::vector<int>& selectedSequence,
    std::vector<int>& selectedOrder
) {
    auto it = std::find(selectedSequence.begin(), selectedSequence.end(), clickedIndex);
    if (it == selectedSequence.end()) {
        selectedSequence.push_back(clickedIndex);
    } else {
        selectedSequence.erase(it);
    }
    rebuildSelectionOrder(selectedSequence, selectedOrder);
}

void clearSelection(
    std::vector<int>& selectedSequence,
    std::vector<int>& selectedOrder,
    std::size_t handSize
) {
    selectedSequence.clear();
    selectedOrder.assign(handSize, 0);
}

bool selectFirstMatchingCard(
    const std::vector<HandCard>& handCards,
    const std::string& targetChar,
    std::vector<int>& selectedSequence,
    std::vector<int>& selectedOrder
) {
    for (std::size_t i = 0; i < handCards.size(); ++i) {
        if (handCards[i].text != targetChar) {
            continue;
        }

        const bool alreadySelected = std::find(
            selectedSequence.begin(),
            selectedSequence.end(),
            static_cast<int>(i)
        ) != selectedSequence.end();

        if (alreadySelected) {
            continue;
        }

        selectedSequence.push_back(static_cast<int>(i));
        rebuildSelectionOrder(selectedSequence, selectedOrder);
        return true;
    }

    return false;
}

void drawOrderNumber(
    SDL_Renderer* renderer,
    TTF_Font* orderFont,
    int orderNumber,
    const SDL_Rect& cardRect
) {
    if (renderer == nullptr || orderFont == nullptr || orderNumber <= 0) {
        return;
    }

    const std::string text = std::to_string(orderNumber);
    SDL_Color textColor{30, 30, 30, 255};

    SDL_Surface* textSurface = TTF_RenderUTF8_Blended(orderFont, text.c_str(), textColor);
    if (textSurface == nullptr) {
        return;
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (textTexture == nullptr) {
        SDL_FreeSurface(textSurface);
        return;
    }

    SDL_Rect dstRect{
        cardRect.x + kOrderPaddingLeft,
        cardRect.y + kOrderPaddingTop,
        textSurface->w,
        textSurface->h
    };

    SDL_FreeSurface(textSurface);
    SDL_RenderCopy(renderer, textTexture, nullptr, &dstRect);
    SDL_DestroyTexture(textTexture);
}

float computeHalfCardStaggerShiftInCardWidth(
    int thisRowCount,
    int otherRowCount,
    bool isLowerRow
) {
    if (thisRowCount <= 0 || otherRowCount <= 0) {
        return 0.0f;
    }

    if (thisRowCount == otherRowCount) {
        return isLowerRow ? -0.25f : 0.25f;
    }

    return 0.0f;
}

std::vector<SDL_Rect> buildRowRects(
    SDL_Texture* cardTexture,
    int count,
    int bottomMargin,
    float rowShiftInCardWidth
) {
    std::vector<SDL_Rect> rects;
    if (count <= 0 || cardTexture == nullptr) {
        return rects;
    }

    int texW = 0;
    int texH = 0;
    SDL_QueryTexture(cardTexture, nullptr, nullptr, &texW, &texH);
    if (texW <= 0 || texH <= 0) {
        return rects;
    }

    const float scale = static_cast<float>(kCardHeight) / static_cast<float>(texH);
    const float cardWf = static_cast<float>(texW) * scale;
    const float cardHf = static_cast<float>(kCardHeight);

    const float totalWidth = cardWf * static_cast<float>(count);
    const float shiftX = rowShiftInCardWidth * cardWf;
    const float startX = (static_cast<float>(kWindowWidth) - totalWidth) * 0.5f + shiftX;
    const float y = static_cast<float>(kWindowHeight - bottomMargin) - cardHf;

    rects.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        const float x = startX + static_cast<float>(i) * cardWf;
        rects.push_back(SDL_Rect{
            static_cast<int>(std::round(x)),
            static_cast<int>(std::round(y)),
            static_cast<int>(std::round(cardWf)),
            static_cast<int>(std::round(cardHf))
        });
    }

    return rects;
}

void renderCard(
    SDL_Renderer* renderer,
    SDL_Texture* cardTexture,
    const std::unordered_map<std::string, GlyphRenderInfo>& glyphCache,
    TTF_Font* orderFont,
    const std::string& ch,
    const SDL_Rect& cardRect,
    int orderNumber
) {
    SDL_RenderCopy(renderer, cardTexture, nullptr, &cardRect);

    SDL_Rect squareRect{
        cardRect.x + (cardRect.w - kSquareSize) / 2,
        cardRect.y + (cardRect.h - kSquareSize) / 2 - kSquareOffsetY,
        kSquareSize,
        kSquareSize
    };

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 220);
    SDL_RenderFillRect(renderer, &squareRect);

    SDL_SetRenderDrawColor(renderer, 110, 90, 70, 255);
    SDL_RenderDrawRect(renderer, &squareRect);

    drawUtf8Text(renderer, glyphCache, ch, squareRect);
    drawOrderNumber(renderer, orderFont, orderNumber, cardRect);
}

void renderPlayedCard(
    SDL_Renderer* renderer,
    SDL_Texture* cardTexture,
    const std::unordered_map<std::string, GlyphRenderInfo>& glyphCache,
    const std::string& ch,
    const SDL_Rect& cardRect
) {
    if (renderer == nullptr || cardTexture == nullptr) {
        return;
    }

    SDL_RenderCopy(renderer, cardTexture, nullptr, &cardRect);

    const int squareSize = static_cast<int>(std::round(
        static_cast<float>(kSquareSize) * static_cast<float>(cardRect.h) / static_cast<float>(kCardHeight)
    ));
    const int squareOffsetY = static_cast<int>(std::round(
        static_cast<float>(kSquareOffsetY) * static_cast<float>(cardRect.h) / static_cast<float>(kCardHeight)
    ));

    SDL_Rect squareRect{
        cardRect.x + (cardRect.w - squareSize) / 2,
        cardRect.y + (cardRect.h - squareSize) / 2 - squareOffsetY,
        squareSize,
        squareSize
    };

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 220);
    SDL_RenderFillRect(renderer, &squareRect);

    SDL_SetRenderDrawColor(renderer, 110, 90, 70, 255);
    SDL_RenderDrawRect(renderer, &squareRect);

    drawUtf8Text(renderer, glyphCache, ch, squareRect);
}

void renderIconButton(
    SDL_Renderer* renderer,
    const SDL_Rect& rect,
    SDL_Texture* iconTexture,
    float scale
) {
    if (renderer == nullptr || iconTexture == nullptr) {
        return;
    }

    int texW = 0;
    int texH = 0;
    SDL_QueryTexture(iconTexture, nullptr, nullptr, &texW, &texH);
    if (texW <= 0 || texH <= 0) {
        return;
    }

    const float fitScaleX =
        static_cast<float>(rect.w - 2 * kIconPadding) / static_cast<float>(texW);
    const float fitScaleY =
        static_cast<float>(rect.h - 2 * kIconPadding) / static_cast<float>(texH);

    const float fitScale = std::min(fitScaleX, fitScaleY);
    const float finalScale = fitScale * scale;

    const int dstW = static_cast<int>(std::round(texW * finalScale));
    const int dstH = static_cast<int>(std::round(texH * finalScale));

    SDL_Rect inner{
        rect.x + (rect.w - dstW) / 2,
        rect.y + (rect.h - dstH) / 2,
        dstW,
        dstH
    };
    SDL_RenderCopy(renderer, iconTexture, nullptr, &inner);
}

void renderUiPanels(
    SDL_Renderer* renderer,
    const UiButtonLayout& buttons,
    SDL_Texture* backIcon,
    SDL_Texture* hintIcon,
    SDL_Texture* undoIcon,
    SDL_Texture* redoIcon,
    SDL_Texture* playIcon
) {
    if (renderer == nullptr) {
        return;
    }

    renderIconButton(renderer, buttons.back, backIcon, 1.2f);
    renderIconButton(renderer, buttons.hint, hintIcon, 1.4f);
    renderIconButton(renderer, buttons.undo, undoIcon, 1.0f);
    renderIconButton(renderer, buttons.redo, redoIcon, 1.0f);
    renderIconButton(renderer, buttons.play, playIcon, 1.4f);
}

void fillCapsuleRect(SDL_Renderer* renderer, const SDL_Rect& rect, SDL_Color color);
void drawCapsuleOutline(SDL_Renderer* renderer, const SDL_Rect& rect, SDL_Color color);
void fillRoundedRect(SDL_Renderer* renderer, const SDL_Rect& rect, int radius, SDL_Color color);
void drawRoundedRectOutline(SDL_Renderer* renderer, const SDL_Rect& rect, int radius, SDL_Color color);

void renderHintCountBadge(
    SDL_Renderer* renderer,
    TTF_Font* badgeFont,
    const SDL_Rect& hintRect,
    int remainingHints
) {
    if (renderer == nullptr || badgeFont == nullptr) {
        return;
    }

    const std::string text = (remainingHints > 99) ? "99+" : std::to_string(std::max(0, remainingHints));

    int textW = 0;
    int textH = 0;
    TTF_SizeUTF8(badgeFont, text.c_str(), &textW, &textH);

    const int badgeW = std::max(kHintBadgeMinSize, textW + 2 * kHintBadgePaddingX);
    const int badgeH = kHintBadgeMinSize;

    SDL_Rect badgeRect{
        hintRect.x + hintRect.w - badgeW / 2 - kHintBadgeOffsetX,
        hintRect.y - badgeH / 2 - kHintBadgeOffsetY,
        badgeW,
        badgeH
    };

    fillCapsuleRect(renderer, badgeRect, SDL_Color{210, 50, 60, 230});
    drawCapsuleOutline(renderer, badgeRect, SDL_Color{255, 255, 255, 140});

    SDL_Color textColor{255, 255, 255, 255};
    SDL_Surface* textSurface = TTF_RenderUTF8_Blended(badgeFont, text.c_str(), textColor);
    if (textSurface == nullptr) {
        return;
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (textTexture != nullptr) {
        SDL_Rect textRect{
            badgeRect.x + (badgeRect.w - textSurface->w) / 2,
            badgeRect.y + (badgeRect.h - textSurface->h) / 2,
            textSurface->w,
            textSurface->h
        };
        SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
        SDL_DestroyTexture(textTexture);
    }

    SDL_FreeSurface(textSurface);
}

std::size_t countAnswerChars(const std::string& text) {
    return Utf8::toCodePoints(text).size();
}

float calcComboMultiplier(int combo) {
    if (combo <= 1) {
        return 1.00f;
    }
    if (combo <= 3) {
        return 1.10f;
    }
    if (combo <= 5) {
        return 1.25f;
    }
    if (combo <= 8) {
        return 1.40f;
    }
    return 1.55f;
}

int calcSpeedBonus(Uint32 deltaMs) {
    if (deltaMs <= 2000) {
        return 15;
    }
    if (deltaMs <= 4000) {
        return 8;
    }
    if (deltaMs <= kMediumComboWindowMs) {
        return 3;
    }
    return 0;
}

// 这份窗口代码目前只有答案文本，没有 PhraseInfo。
// 这里按你当前玩法最稳的规则近似：4字按成语，其余按诗句。
int calcBaseScoreHeuristic(const std::string& answerText) {
    const int length = static_cast<int>(countAnswerChars(answerText));
    const bool isIdiom = (length == 4);
    const bool isPoem = !isIdiom;

    int rarity = 1;
    if (isPoem) {
        if (length >= 7) {
            rarity = 3;
        } else if (length >= 5) {
            rarity = 2;
        }
    } else {
        if (length >= 4) {
            rarity = 2;
        }
    }

    return length * 10 + (isPoem ? 5 : 0) + (rarity - 1) * 5;
}

int calcAnswerGainScore(
    const std::string& answerText,
    int combo,
    Uint32 deltaMs,
    const std::unordered_set<std::string>& optimalAnswerSet
) {
    const int baseScore = calcBaseScoreHeuristic(answerText);
    int gain = static_cast<int>(std::lround(baseScore * calcComboMultiplier(combo)));

    if (combo > 1) {
        gain += calcSpeedBonus(deltaMs);
    }

    if (optimalAnswerSet.find(answerText) != optimalAnswerSet.end()) {
        gain += kOptimalAnswerBonus;
    }

    return gain;
}

void renderTopCenterScore(
    SDL_Renderer* renderer,
    TTF_Font* scoreFont,
    int currentScore
) {
    if (renderer == nullptr || scoreFont == nullptr) {
        return;
    }

    const SDL_Rect topCenter{
        (kWindowWidth - kTopCenterWidth) / 2,
        kTopMargin,
        kTopCenterWidth,
        kTopCenterHeight
    };

    const std::string text = std::to_string(std::max(0, currentScore));
    SDL_Color textColor{20, 20, 20, 255};

    SDL_Surface* textSurface = TTF_RenderUTF8_Blended(scoreFont, text.c_str(), textColor);
    if (textSurface == nullptr) {
        return;
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (textTexture == nullptr) {
        SDL_FreeSurface(textSurface);
        return;
    }

    SDL_Rect dstRect{
        topCenter.x + (topCenter.w - textSurface->w) / 2,
        topCenter.y + (topCenter.h - textSurface->h) / 2 - 10,
        textSurface->w,
        textSurface->h
    };

    SDL_RenderCopy(renderer, textTexture, nullptr, &dstRect);

    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);
}

void renderTopCenterScoreStats(
    SDL_Renderer* renderer,
    TTF_Font* statFont,
    int optimalCorrectCount,
    int correctCount,
    int totalOptimalCount
) {
    if (renderer == nullptr || statFont == nullptr) {
        return;
    }

    const SDL_Rect topCenter{
        (kWindowWidth - kTopCenterWidth) / 2,
        kTopMargin,
        kTopCenterWidth,
        kTopCenterHeight
    };

    const std::string text =
        std::to_string(std::max(0, optimalCorrectCount)) +
        "(" +
        std::to_string(std::max(0, correctCount)) +
        ") / " +
        std::to_string(std::max(0, totalOptimalCount));

    SDL_Color textColor{20, 20, 20, 255};

    SDL_Surface* textSurface = TTF_RenderUTF8_Blended(statFont, text.c_str(), textColor);
    if (textSurface == nullptr) {
        return;
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (textTexture == nullptr) {
        SDL_FreeSurface(textSurface);
        return;
    }

    SDL_Rect dstRect{
        topCenter.x + (topCenter.w - textSurface->w) / 2,
        topCenter.y + kScoreStatOffsetY,
        textSurface->w,
        textSurface->h
    };

    SDL_RenderCopy(renderer, textTexture, nullptr, &dstRect);

    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);
}

std::string formatCountdownText(int remainingSeconds) {
    remainingSeconds = std::clamp(remainingSeconds, 0, kTimerMaxSeconds);
    return std::to_string(remainingSeconds);
}

void renderTimerBackground(
    SDL_Renderer* renderer,
    SDL_Texture* timerBgTexture
) {
    if (renderer == nullptr || timerBgTexture == nullptr) {
        return;
    }

    SDL_SetTextureBlendMode(timerBgTexture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaMod(timerBgTexture, kTimerBgAlpha);

    const int centerX = kWindowWidth / 2;
    const int centerY = kWindowHeight / 2 + kTimerOffsetY + 80;

    SDL_Rect bgRect{
        centerX - kTimerBgSize / 2,
        centerY - kTimerBgSize / 2,
        kTimerBgSize,
        kTimerBgSize
    };

    SDL_RenderCopy(renderer, timerBgTexture, nullptr, &bgRect);
}

void renderCountdown(
    SDL_Renderer* renderer,
    TTF_Font* timerFont,
    int remainingSeconds
) {
    if (renderer == nullptr || timerFont == nullptr) {
        return;
    }

    remainingSeconds = std::clamp(remainingSeconds, 0, kTimerMaxSeconds);
    const std::string text = formatCountdownText(remainingSeconds);

    SDL_Color textColor =
        (remainingSeconds <= 10)
            ? SDL_Color{210, 50, 60, 255}
            : SDL_Color{20, 20, 20, 255};

    SDL_Surface* textSurface = TTF_RenderUTF8_Blended(timerFont, text.c_str(), textColor);
    if (textSurface == nullptr) {
        return;
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (textTexture == nullptr) {
        SDL_FreeSurface(textSurface);
        return;
    }

    SDL_SetTextureBlendMode(textTexture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaMod(textTexture, kTimerAlpha);

    const int textX = (kWindowWidth - textSurface->w) / 2;
    const int textY = (kWindowHeight - textSurface->h) / 2 + kTimerOffsetY;

    SDL_Rect dstRect{
        textX,
        textY,
        textSurface->w,
        textSurface->h
    };

    SDL_RenderCopy(renderer, textTexture, nullptr, &dstRect);

    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);
}

SDL_Texture* loadTextureRequired(
    SDL_Renderer* renderer,
    const fs::path& path,
    SdlTextureGuard& guard
) {
    guard.texture = IMG_LoadTexture(renderer, path.string().c_str());
    if (guard.texture == nullptr) {
        throw std::runtime_error("Failed to load texture: " + path.string());
    }
    return guard.texture;
}

void eraseSelectedFromHand(
    std::vector<HandCard>& handCards,
    const std::vector<int>& selectedSequence
) {
    std::vector<int> indices = selectedSequence;
    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());

    for (auto it = indices.rbegin(); it != indices.rend(); ++it) {
        const int idx = *it;
        if (idx >= 0 && idx < static_cast<int>(handCards.size())) {
            handCards.erase(handCards.begin() + idx);
        }
    }
}

void restorePlayedGroupToHand(
    std::vector<HandCard>& handCards,
    const PlayedGroupState& group
) {
    for (const auto& card : group.cards) {
        handCards.push_back(card);
    }
    std::stable_sort(handCards.begin(), handCards.end(), [](const HandCard& a, const HandCard& b) {
        return a.originIndex < b.originIndex;
    });
}

void removePlayedGroupFromHandForRedo(
    std::vector<HandCard>& handCards,
    const PlayedGroupState& group
) {
    std::unordered_multimap<int, std::string> targets;
    for (const auto& card : group.cards) {
        targets.emplace(card.originIndex, card.text);
    }

    handCards.erase(
        std::remove_if(
            handCards.begin(),
            handCards.end(),
            [&targets](const HandCard& card) {
                auto range = targets.equal_range(card.originIndex);
                for (auto it = range.first; it != range.second; ++it) {
                    if (it->second == card.text) {
                        targets.erase(it);
                        return true;
                    }
                }
                return false;
            }
        ),
        handCards.end()
    );
}

std::optional<char32_t> extractHintCodePoint(
    const std::vector<HandCard>& handCards,
    const std::vector<std::string>& validAnswers,
    const std::string& selectedText
) {
    const CharFreq handFreq = buildFreqFromHandCards(handCards);

    std::vector<std::string> availableAnswers;
    for (const auto& ans : validAnswers) {
        if (canComposeTextFromFreq(ans, handFreq)) {
            availableAnswers.push_back(ans);
        }
    }

    std::sort(availableAnswers.begin(), availableAnswers.end(), [](const std::string& a, const std::string& b) {
        const auto ac = Utf8::toCodePoints(a).size();
        const auto bc = Utf8::toCodePoints(b).size();
        if (ac != bc) {
            return ac < bc;
        }
        return a < b;
    });

    if (selectedText.empty()) {
        if (availableAnswers.empty()) {
            return std::nullopt;
        }
        const auto cps = Utf8::toCodePoints(availableAnswers.front());
        if (cps.empty()) {
            return std::nullopt;
        }
        return cps.front();
    }

    for (const auto& ans : availableAnswers) {
        if (ans.rfind(selectedText, 0) == 0) {
            const auto selectedCps = Utf8::toCodePoints(selectedText);
            const auto ansCps = Utf8::toCodePoints(ans);

            if (ansCps.size() > selectedCps.size()) {
                return ansCps[selectedCps.size()];
            }
        }
    }

    return std::nullopt;
}

void renderPlayedGroups(
    SDL_Renderer* renderer,
    SDL_Texture* cardTexture,
    const std::unordered_map<std::string, GlyphRenderInfo>& glyphCache,
    const std::vector<PlayedGroupState>& playedGroups
) {
    if (renderer == nullptr || cardTexture == nullptr || playedGroups.empty()) {
        return;
    }

    const float cardWf = static_cast<float>(kPlayedCardWidth);
    const float cardHf = static_cast<float>(kPlayedCardHeight);

    std::size_t nextGroup = 0;
    int row = 0;

    while (nextGroup < playedGroups.size() && row < kPlayedMaxRows) {
        std::vector<std::size_t> rowGroups;
        int rowCardCount = 0;

        while (nextGroup < playedGroups.size()) {
            const int groupCardCount = static_cast<int>(playedGroups[nextGroup].cards.size());
            if (groupCardCount <= 0) {
                ++nextGroup;
                continue;
            }

            if (groupCardCount > kPlayedMaxCardsPerRow) {
                ++nextGroup;
                continue;
            }

            if (rowGroups.empty()) {
                rowGroups.push_back(nextGroup);
                rowCardCount = groupCardCount;
                ++nextGroup;
            } else if (rowCardCount + groupCardCount <= kPlayedMaxCardsPerRow) {
                rowGroups.push_back(nextGroup);
                rowCardCount += groupCardCount;
                ++nextGroup;
            } else {
                break;
            }
        }

        if (rowGroups.empty()) {
            break;
        }

        float rowWidth = 0.0f;
        for (std::size_t i = 0; i < rowGroups.size(); ++i) {
            rowWidth += static_cast<float>(playedGroups[rowGroups[i]].cards.size()) * cardWf;
        }
        rowWidth += static_cast<float>(std::max<std::size_t>(0, rowGroups.size() - 1)) * kPlayedGroupGap;

        const float startX =
            std::max(0.0f, (static_cast<float>(kWindowWidth) - rowWidth) * 0.5f);

        const float y = static_cast<float>(kPlayedAreaY) +
                        static_cast<float>(row) * (cardHf + static_cast<float>(kPlayedRowGap));

        float currentX = startX;
        for (std::size_t gi = 0; gi < rowGroups.size(); ++gi) {
            const auto& group = playedGroups[rowGroups[gi]];

            for (std::size_t ci = 0; ci < group.cards.size(); ++ci) {
                SDL_Rect cardRect{
                    static_cast<int>(std::round(currentX + static_cast<float>(ci) * cardWf)),
                    static_cast<int>(std::round(y)),
                    kPlayedCardWidth,
                    kPlayedCardHeight
                };

                renderPlayedCard(
                    renderer,
                    cardTexture,
                    glyphCache,
                    group.cards[ci].text,
                    cardRect
                );
            }

            currentX += static_cast<float>(group.cards.size()) * cardWf;
            if (gi + 1 < rowGroups.size()) {
                currentX += static_cast<float>(kPlayedGroupGap);
            }
        }

        ++row;
    }
}

void fillCapsuleRect(SDL_Renderer* renderer, const SDL_Rect& rect, SDL_Color color) {
    if (renderer == nullptr || rect.w <= 0 || rect.h <= 0) {
        return;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    const int radius = rect.h / 2;
    const float centerY = rect.y + rect.h / 2.0f - 0.5f;

    for (int y = rect.y; y < rect.y + rect.h; ++y) {
        const float dy = static_cast<float>(y) - centerY;
        const int dx = static_cast<int>(std::sqrt(std::max(0.0f, static_cast<float>(radius * radius) - dy * dy)));

        const int left = rect.x + radius - dx;
        const int right = rect.x + rect.w - radius + dx - 1;
        SDL_RenderDrawLine(renderer, left, y, right, y);
    }
}

void drawCapsuleOutline(SDL_Renderer* renderer, const SDL_Rect& rect, SDL_Color color) {
    if (renderer == nullptr || rect.w <= 0 || rect.h <= 0) {
        return;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    const int radius = rect.h / 2;
    const float leftCenterX = rect.x + radius - 0.5f;
    const float rightCenterX = rect.x + rect.w - radius - 0.5f;
    const float centerY = rect.y + rect.h / 2.0f - 0.5f;

    for (int deg = 0; deg < 360; ++deg) {
        const float rad = static_cast<float>(deg) * 3.1415926f / 180.0f;
        const int lx = static_cast<int>(std::round(leftCenterX + std::cos(rad) * radius));
        const int ly = static_cast<int>(std::round(centerY + std::sin(rad) * radius));
        const int rx = static_cast<int>(std::round(rightCenterX + std::cos(rad) * radius));
        const int ry = static_cast<int>(std::round(centerY + std::sin(rad) * radius));

        if (deg >= 90 && deg <= 270) {
            SDL_RenderDrawPoint(renderer, lx, ly);
        }
        if (deg <= 90 || deg >= 270) {
            SDL_RenderDrawPoint(renderer, rx, ry);
        }
    }

    SDL_RenderDrawLine(renderer, rect.x + radius, rect.y, rect.x + rect.w - radius, rect.y);
    SDL_RenderDrawLine(renderer, rect.x + radius, rect.y + rect.h - 1, rect.x + rect.w - radius, rect.y + rect.h - 1);
}

void pushAlert(std::vector<AlertItem>& alerts, const std::string& text, Uint32 now) {
    if (text.empty()) {
        return;
    }

    if (alerts.size() >= kAlertMaxCount) {
        alerts.erase(alerts.begin());
    }

    alerts.push_back(AlertItem{text, now, false, 0});
}

void updateAlerts(std::vector<AlertItem>& alerts, Uint32 now) {
    for (auto& alert : alerts) {
        if (!alert.closing && now - alert.bornAt >= kAlertLifeMs) {
            alert.closing = true;
            alert.closingAt = now;
        }
    }

    alerts.erase(
        std::remove_if(
            alerts.begin(),
            alerts.end(),
            [now](const AlertItem& alert) {
                return alert.closing && (now - alert.closingAt >= kAlertFadeMs);
            }
        ),
        alerts.end()
    );
}

Uint8 getAlertAlpha(const AlertItem& alert, Uint32 now) {
    if (!alert.closing) {
        return 220;
    }

    const Uint32 elapsed = now - alert.closingAt;
    if (elapsed >= kAlertFadeMs) {
        return 0;
    }

    const float remain = 1.0f - static_cast<float>(elapsed) / static_cast<float>(kAlertFadeMs);
    return static_cast<Uint8>(std::round(220.0f * remain));
}

std::vector<AlertLayout> buildAlertLayouts(
    TTF_Font* alertFont,
    const std::vector<AlertItem>& alerts
) {
    std::vector<AlertLayout> layouts;
    layouts.reserve(alerts.size());

    if (alertFont == nullptr) {
        return layouts;
    }

    for (std::size_t i = 0; i < alerts.size(); ++i) {
        int textW = 0;
        int textH = 0;
        TTF_SizeUTF8(alertFont, alerts[i].text.c_str(), &textW, &textH);

        const int bubbleW =
            kAlertLeading + textW + kAlertTextGap + kAlertCloseSize + kAlertTrailing;
        const int bubbleX = (kWindowWidth - bubbleW) / 2;
        const int bubbleY = kAlertTop + static_cast<int>(i) * (kAlertHeight + kAlertGap);

        AlertLayout layout;
        layout.bubble = SDL_Rect{bubbleX, bubbleY, bubbleW, kAlertHeight};
        layout.closeRect = SDL_Rect{
            bubbleX + bubbleW - kAlertTrailing - kAlertCloseSize,
            bubbleY + (kAlertHeight - kAlertCloseSize) / 2,
            kAlertCloseSize,
            kAlertCloseSize
        };
        layouts.push_back(layout);
    }

    return layouts;
}

void renderAlerts(
    SDL_Renderer* renderer,
    TTF_Font* alertFont,
    SDL_Texture* closeIcon,
    const std::vector<AlertItem>& alerts
) {
    if (renderer == nullptr || alertFont == nullptr || alerts.empty()) {
        return;
    }

    const auto layouts = buildAlertLayouts(alertFont, alerts);
    const Uint32 now = SDL_GetTicks();

    for (std::size_t i = 0; i < alerts.size() && i < layouts.size(); ++i) {
        const Uint8 alpha = getAlertAlpha(alerts[i], now);
        if (alpha == 0) {
            continue;
        }

        fillCapsuleRect(renderer, layouts[i].bubble, SDL_Color{210, 50, 60, alpha});
        drawCapsuleOutline(renderer, layouts[i].bubble, SDL_Color{255, 255, 255, static_cast<Uint8>(alpha * 0.45f)});

        SDL_Color textColor{255, 255, 255, alpha};
        SDL_Surface* textSurface = TTF_RenderUTF8_Blended(alertFont, alerts[i].text.c_str(), textColor);
        if (textSurface != nullptr) {
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            if (textTexture != nullptr) {
                SDL_Rect textRect{
                    layouts[i].bubble.x + kAlertLeading,
                    layouts[i].bubble.y + (kAlertHeight - textSurface->h) / 2,
                    textSurface->w,
                    textSurface->h
                };
                SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
                SDL_DestroyTexture(textTexture);
            }
            SDL_FreeSurface(textSurface);
        }

        if (closeIcon != nullptr) {
            SDL_SetTextureAlphaMod(closeIcon, alpha);
            SDL_RenderCopy(renderer, closeIcon, nullptr, &layouts[i].closeRect);
            SDL_SetTextureAlphaMod(closeIcon, 255);
        }
    }
}

float easeOutCubic(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    const float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

void pushScoreFlyAnim(
    std::vector<ScoreFlyAnim>& anims,
    int gainScore,
    Uint32 now
) {
    const SDL_Rect topCenter{
        (kWindowWidth - kTopCenterWidth) / 2,
        kTopMargin,
        kTopCenterWidth,
        kTopCenterHeight
    };

    ScoreFlyAnim anim;
    anim.text = "+" + std::to_string(std::max(0, gainScore));
    anim.bornAt = now;
    anim.startX = kWindowWidth / 2;
    anim.startY = kWindowHeight / 2 + kScoreFlyStartOffsetY;
    anim.endX = topCenter.x + topCenter.w / 2;
    anim.endY = topCenter.y + topCenter.h / 2;

    anims.push_back(std::move(anim));
}

void updateScoreFlyAnims(
    std::vector<ScoreFlyAnim>& anims,
    Uint32 now
) {
    anims.erase(
        std::remove_if(
            anims.begin(),
            anims.end(),
            [now](const ScoreFlyAnim& anim) {
                return now - anim.bornAt >= kScoreFlyLifeMs;
            }
        ),
        anims.end()
    );
}

void renderScoreFlyAnims(
    SDL_Renderer* renderer,
    TTF_Font* font,
    const std::vector<ScoreFlyAnim>& anims
) {
    if (renderer == nullptr || font == nullptr || anims.empty()) {
        return;
    }

    const Uint32 now = SDL_GetTicks();

    for (const auto& anim : anims) {
        float t = static_cast<float>(now - anim.bornAt) / static_cast<float>(kScoreFlyLifeMs);
        t = std::clamp(t, 0.0f, 1.0f);

        const float eased = easeOutCubic(t);

        const float x = static_cast<float>(anim.startX) +
                        (static_cast<float>(anim.endX - anim.startX) * eased);
        const float y = static_cast<float>(anim.startY) +
                        (static_cast<float>(anim.endY - anim.startY) * eased);

        const Uint8 alpha = static_cast<Uint8>(std::round(255.0f * (1.0f - t)));

        SDL_Surface* textSurface = TTF_RenderUTF8_Blended(font, anim.text.c_str(), kScoreFlyColor);
        if (textSurface == nullptr) {
            continue;
        }

        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        if (textTexture == nullptr) {
            SDL_FreeSurface(textSurface);
            continue;
        }

        SDL_SetTextureBlendMode(textTexture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(textTexture, alpha);

        // 从当前字体大小逐渐缩小到 20
        const float endScale = static_cast<float>(kScoreFlyEndSize) / static_cast<float>(kScoreFontSize);
        const float scale = 1.0f + (endScale - 1.0f) * eased;

        const int dstW = std::max(1, static_cast<int>(std::round(textSurface->w * scale)));
        const int dstH = std::max(1, static_cast<int>(std::round(textSurface->h * scale)));

        SDL_Rect dstRect{
            static_cast<int>(std::round(x)) - dstW / 2,
            static_cast<int>(std::round(y)) - dstH / 2,
            dstW,
            dstH
        };

        SDL_RenderCopy(renderer, textTexture, nullptr, &dstRect);

        SDL_DestroyTexture(textTexture);
        SDL_FreeSurface(textSurface);
    }
}

bool hasAnyPossibleAnswer(
    const std::vector<HandCard>& handCards,
    const std::vector<std::string>& validAnswers
) {
    const CharFreq handFreq = buildFreqFromHandCards(handCards);

    for (const auto& ans : validAnswers) {
        if (canComposeTextFromFreq(ans, handFreq)) {
            return true;
        }
    }
    return false;
}

int calcMaxComboFromHistory(const std::vector<PlayedGroupState>& playedHistory) {
    int best = 0;
    for (const auto& group : playedHistory) {
        best = std::max(best, group.comboAfterPlay);
    }
    return best;
}

std::string formatHMS(int totalSeconds) {
    totalSeconds = std::max(0, totalSeconds);
    const int h = totalSeconds / 3600;
    const int m = (totalSeconds % 3600) / 60;
    const int s = totalSeconds % 60;

    auto two = [](int v) {
        return (v < 10 ? "0" : "") + std::to_string(v);
    };

    return two(h) + ":" + two(m) + ":" + two(s);
}

int renderEndAnsweredCards(
    SDL_Renderer* renderer,
    SDL_Texture* cardTexture,
    const std::unordered_map<std::string, GlyphRenderInfo>& glyphCache,
    const SDL_Rect& panelRect,
    int startY,
    const std::vector<PlayedGroupState>& playedHistory
) {
    if (renderer == nullptr || cardTexture == nullptr || playedHistory.empty()) {
        return startY;
    }

    int texW = 0;
    int texH = 0;
    SDL_QueryTexture(cardTexture, nullptr, nullptr, &texW, &texH);
    if (texW <= 0 || texH <= 0) {
        return startY;
    }

    const int cardW = kEndAnswerCardWidth;
    const int cardH = static_cast<int>(std::round(
        static_cast<float>(texH) * static_cast<float>(cardW) / static_cast<float>(texW)
    ));

    const int usableLeft = panelRect.x + kEndMetaPaddingX;
    const int usableRight = panelRect.x + panelRect.w - kEndMetaPaddingX;
    const int usableWidth = usableRight - usableLeft;

    std::vector<std::vector<const PlayedGroupState*>> rows;
    std::vector<const PlayedGroupState*> currentRow;
    int currentRowWidth = 0;

    for (const auto& group : playedHistory) {
        if (group.cards.empty()) {
            continue;
        }

        const int groupW = static_cast<int>(group.cards.size()) * cardW;
        const int extraGap = currentRow.empty() ? 0 : kEndAnswerGroupGap;

        if (!currentRow.empty() && currentRowWidth + extraGap + groupW > usableWidth) {
            rows.push_back(currentRow);
            if (static_cast<int>(rows.size()) >= kEndAnswerMaxRows) {
                break;
            }
            currentRow.clear();
            currentRowWidth = 0;
        }

        if (groupW > usableWidth) {
            continue;
        }

        if (!currentRow.empty()) {
            currentRowWidth += kEndAnswerGroupGap;
        }
        currentRow.push_back(&group);
        currentRowWidth += groupW;
    }

    if (!currentRow.empty() && static_cast<int>(rows.size()) < kEndAnswerMaxRows) {
        rows.push_back(currentRow);
    }

    if (rows.empty()) {
        return startY;
    }

    int y = startY;

    for (std::size_t rowIdx = 0; rowIdx < rows.size(); ++rowIdx) {
        int rowWidth = 0;
        for (std::size_t i = 0; i < rows[rowIdx].size(); ++i) {
            rowWidth += static_cast<int>(rows[rowIdx][i]->cards.size()) * cardW;
        }
        rowWidth += static_cast<int>(std::max<std::size_t>(0, rows[rowIdx].size() - 1)) * kEndAnswerGroupGap;

        int x = usableLeft + (usableWidth - rowWidth) / 2;

        for (std::size_t gi = 0; gi < rows[rowIdx].size(); ++gi) {
            const auto& group = *rows[rowIdx][gi];

            for (std::size_t ci = 0; ci < group.cards.size(); ++ci) {
                SDL_Rect cardRect{
                    x + static_cast<int>(ci) * cardW,
                    y,
                    cardW,
                    cardH
                };

                renderPlayedCard(
                    renderer,
                    cardTexture,
                    glyphCache,
                    group.cards[ci].text,
                    cardRect
                );
            }

            x += static_cast<int>(group.cards.size()) * cardW;
            if (gi + 1 < rows[rowIdx].size()) {
                x += kEndAnswerGroupGap;
            }
        }

        if (rowIdx + 1 < rows.size()) {
            y += cardH + kEndAnswerRowGap;
        }
    }

    return y + cardH;
}

void renderEndMetaRow(
    SDL_Renderer* renderer,
    TTF_Font* font,
    const SDL_Rect& panelRect,
    int y,
    const std::string& leftText,
    const std::string& rightText
) {
    if (renderer == nullptr || font == nullptr) {
        return;
    }

    SDL_Color textColor{20, 20, 20, 255};

    SDL_Surface* leftSurface = TTF_RenderUTF8_Blended(font, leftText.c_str(), textColor);
    SDL_Surface* rightSurface = TTF_RenderUTF8_Blended(font, rightText.c_str(), textColor);

    if (leftSurface != nullptr) {
        SDL_Texture* leftTexture = SDL_CreateTextureFromSurface(renderer, leftSurface);
        if (leftTexture != nullptr) {
            SDL_Rect leftRect{
                panelRect.x + kEndMetaPaddingX,
                y,
                leftSurface->w,
                leftSurface->h
            };
            SDL_RenderCopy(renderer, leftTexture, nullptr, &leftRect);
            SDL_DestroyTexture(leftTexture);
        }
        SDL_FreeSurface(leftSurface);
    }

    if (rightSurface != nullptr) {
        SDL_Texture* rightTexture = SDL_CreateTextureFromSurface(renderer, rightSurface);
        if (rightTexture != nullptr) {
            SDL_Rect rightRect{
                panelRect.x + panelRect.w - kEndMetaPaddingX - rightSurface->w,
                y,
                rightSurface->w,
                rightSurface->h
            };
            SDL_RenderCopy(renderer, rightTexture, nullptr, &rightRect);
            SDL_DestroyTexture(rightTexture);
        }
        SDL_FreeSurface(rightSurface);
    }
}

void renderEndOverlay(
    SDL_Renderer* renderer,
    SDL_Texture* cardTexture,
    const std::unordered_map<std::string, GlyphRenderInfo>& glyphCache,
    TTF_Font* titleFont,
    TTF_Font* scoreFont,
    TTF_Font* metaFont,
    const std::string& titleText,
    int score,
    const std::vector<PlayedGroupState>& playedHistory,
    int maxCombo,
    int usedSeconds
) {
    if (renderer == nullptr || titleFont == nullptr || scoreFont == nullptr || metaFont == nullptr) {
        return;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_Rect fullScreen{0, 0, kWindowWidth, kWindowHeight};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, kEndOverlayAlpha);
    SDL_RenderFillRect(renderer, &fullScreen);

    SDL_Rect panelRect{
        (kWindowWidth - kEndPanelWidth) / 2,
        (kWindowHeight - kEndPanelHeight) / 2,
        kEndPanelWidth,
        kEndPanelHeight
    };

    fillRoundedRect(renderer, panelRect, kEndPanelCornerRadius, SDL_Color{255, 255, 255, 255});
    drawRoundedRectOutline(renderer, panelRect, kEndPanelCornerRadius, SDL_Color{220, 220, 220, 255});

    SDL_Color textColor{20, 20, 20, 255};

    SDL_Surface* titleSurface = TTF_RenderUTF8_Blended(titleFont, titleText.c_str(), textColor);
    if (titleSurface == nullptr) {
        return;
    }

    SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
    if (titleTexture == nullptr) {
        SDL_FreeSurface(titleSurface);
        return;
    }

    SDL_Rect titleRect{
        panelRect.x + (panelRect.w - titleSurface->w) / 2,
        panelRect.y + kEndTitleTop,
        titleSurface->w,
        titleSurface->h
    };
    SDL_RenderCopy(renderer, titleTexture, nullptr, &titleRect);
    SDL_DestroyTexture(titleTexture);

    const std::string scoreText = std::to_string(std::max(0, score));
    SDL_Surface* scoreSurface = TTF_RenderUTF8_Blended(scoreFont, scoreText.c_str(), textColor);
    SDL_Rect scoreRect{panelRect.x, titleRect.y + titleRect.h + kEndScoreGap, 0, 0};

    if (scoreSurface != nullptr) {
        SDL_Texture* scoreTexture = SDL_CreateTextureFromSurface(renderer, scoreSurface);
        if (scoreTexture != nullptr) {
            scoreRect = SDL_Rect{
                panelRect.x + (panelRect.w - scoreSurface->w) / 2,
                titleRect.y + titleRect.h + kEndScoreGap,
                scoreSurface->w,
                scoreSurface->h
            };
            SDL_RenderCopy(renderer, scoreTexture, nullptr, &scoreRect);
            SDL_DestroyTexture(scoreTexture);
        }
        SDL_FreeSurface(scoreSurface);
    }

    const int answersStartY = scoreRect.y + scoreRect.h + kEndAnswerTopGap;
    const int answersBottomY = renderEndAnsweredCards(
        renderer,
        cardTexture,
        glyphCache,
        panelRect,
        answersStartY,
        playedHistory
    );

    int comboY = answersBottomY + kEndMetaTopGap;
    renderEndMetaRow(
        renderer,
        metaFont,
        panelRect,
        comboY,
        "Combo",
        std::to_string(std::max(0, maxCombo))
    );

    int metaFontH = 0;
    TTF_SizeUTF8(metaFont, "Combo", nullptr, &metaFontH);

    int timeY = comboY + metaFontH + kEndMetaRowGap;
    renderEndMetaRow(
        renderer,
        metaFont,
        panelRect,
        timeY,
        "Time",
        formatHMS(usedSeconds)
    );

    SDL_FreeSurface(titleSurface);
}

EndOverlayButtons buildEndOverlayButtons() {
    SDL_Rect panelRect{
        (kWindowWidth - kEndPanelWidth) / 2,
        (kWindowHeight - kEndPanelHeight) / 2,
        kEndPanelWidth,
        kEndPanelHeight
    };

    const int totalWidth = kEndButtonWidth * 2 + kEndButtonGap;
    const int startX = panelRect.x + (panelRect.w - totalWidth) / 2;
    const int y = panelRect.y + panelRect.h - kEndButtonBottom - kEndButtonHeight;

    EndOverlayButtons buttons;
    buttons.backHome = SDL_Rect{
        startX,
        y,
        kEndButtonWidth,
        kEndButtonHeight
    };
    buttons.replay = SDL_Rect{
        startX + kEndButtonWidth + kEndButtonGap,
        y,
        kEndButtonWidth,
        kEndButtonHeight
    };
    return buttons;
}

LeaveConfirmButtons buildLeaveConfirmButtons() {
    LeaveConfirmButtons layout{};

    layout.panel = SDL_Rect{
        (kWindowWidth - kLeaveConfirmWidth) / 2,
        (kWindowHeight - kLeaveConfirmHeight) / 2,
        kLeaveConfirmWidth,
        kLeaveConfirmHeight
    };

    const int buttonY = layout.panel.y + layout.panel.h - kLeaveConfirmButtonHeight;
    const int halfW = layout.panel.w / 2;

    layout.leaveGame = SDL_Rect{
        layout.panel.x,
        buttonY,
        halfW,
        kLeaveConfirmButtonHeight
    };

    layout.resumeGame = SDL_Rect{
        layout.panel.x + halfW,
        buttonY,
        layout.panel.w - halfW,
        kLeaveConfirmButtonHeight
    };

    return layout;
}

void renderCenteredText(
    SDL_Renderer* renderer,
    TTF_Font* font,
    const SDL_Rect& rect,
    const std::string& text,
    SDL_Color color
) {
    if (renderer == nullptr || font == nullptr || text.empty()) {
        return;
    }

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (surface == nullptr) {
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture != nullptr) {
        SDL_Rect dst{
            rect.x + (rect.w - surface->w) / 2,
            rect.y + (rect.h - surface->h) / 2,
            surface->w,
            surface->h
        };
        SDL_RenderCopy(renderer, texture, nullptr, &dst);
        SDL_DestroyTexture(texture);
    }

    SDL_FreeSurface(surface);
}

void renderLeaveConfirmOverlay(
    SDL_Renderer* renderer,
    TTF_Font* titleFont,
    TTF_Font* buttonFont
) {
    if (renderer == nullptr || titleFont == nullptr || buttonFont == nullptr) {
        return;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_Rect fullScreen{0, 0, kWindowWidth, kWindowHeight};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, kLeaveConfirmOverlayAlpha);
    SDL_RenderFillRect(renderer, &fullScreen);

    const LeaveConfirmButtons layout = buildLeaveConfirmButtons();

    fillRoundedRect(
        renderer,
        layout.panel,
        kLeaveConfirmCornerRadius,
        kLeaveConfirmPanelColor
    );
    drawRoundedRectOutline(
        renderer,
        layout.panel,
        kLeaveConfirmCornerRadius,
        kLeaveConfirmBorderColor
    );

    const SDL_Rect titleRect{
        layout.panel.x + 24,
        layout.panel.y + kLeaveConfirmTitleTop,
        layout.panel.w - 48,
        36
    };

    renderCenteredText(
        renderer,
        titleFont,
        titleRect,
        "是否离开本局游戏",
        SDL_Color{20, 20, 20, 255}
    );

    const int separatorY = layout.leaveGame.y;
    SDL_SetRenderDrawColor(
        renderer,
        kLeaveConfirmSeparatorColor.r,
        kLeaveConfirmSeparatorColor.g,
        kLeaveConfirmSeparatorColor.b,
        kLeaveConfirmSeparatorColor.a
    );
    SDL_RenderDrawLine(
        renderer,
        layout.panel.x,
        separatorY,
        layout.panel.x + layout.panel.w,
        separatorY
    );

    const int midX = layout.panel.x + layout.panel.w / 2;
    SDL_RenderDrawLine(
        renderer,
        midX,
        separatorY,
        midX,
        layout.panel.y + layout.panel.h
    );

    renderCenteredText(
        renderer,
        buttonFont,
        layout.leaveGame,
        "离开游戏",
        SDL_Color{220, 64, 64, 255}
    );

    renderCenteredText(
        renderer,
        buttonFont,
        layout.resumeGame,
        "回到游戏",
        SDL_Color{20, 20, 20, 255}
    );
}

void renderEndButtonText(
    SDL_Renderer* renderer,
    TTF_Font* font,
    const SDL_Rect& rect,
    const std::string& text,
    SDL_Color textColor
) {
    if (renderer == nullptr || font == nullptr) {
        return;
    }

    SDL_Surface* textSurface = TTF_RenderUTF8_Blended(font, text.c_str(), textColor);
    if (textSurface == nullptr) {
        return;
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (textTexture != nullptr) {
        SDL_Rect textRect{
            rect.x + (rect.w - textSurface->w) / 2,
            rect.y + (rect.h - textSurface->h) / 2,
            textSurface->w,
            textSurface->h
        };
        SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
        SDL_DestroyTexture(textTexture);
    }

    SDL_FreeSurface(textSurface);
}

void renderEndOverlayButtons(
    SDL_Renderer* renderer,
    TTF_Font* font
) {
    if (renderer == nullptr || font == nullptr) {
        return;
    }

    const EndOverlayButtons buttons = buildEndOverlayButtons();

    renderEndButtonText(
        renderer,
        font,
        buttons.backHome,
        "返回主页",
        SDL_Color{210, 50, 60, 255}   // 紅字
    );

    renderEndButtonText(
        renderer,
        font,
        buttons.replay,
        "再玩一局",
        SDL_Color{20, 20, 20, 255}    // 黑字
    );
}

void fillRoundedRect(SDL_Renderer* renderer, const SDL_Rect& rect, int radius, SDL_Color color) {
    if (renderer == nullptr || rect.w <= 0 || rect.h <= 0) {
        return;
    }

    radius = std::max(0, std::min(radius, std::min(rect.w, rect.h) / 2));

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    if (radius == 0) {
        SDL_RenderFillRect(renderer, &rect);
        return;
    }

    // 中間主體
    SDL_Rect centerRect{
        rect.x,
        rect.y + radius,
        rect.w,
        rect.h - 2 * radius
    };
    if (centerRect.h > 0) {
        SDL_RenderFillRect(renderer, &centerRect);
    }

    // 上下圓角區 + 中間橫向填充
    for (int dy = 0; dy < radius; ++dy) {
        const float fy = static_cast<float>(radius - dy - 0.5f);
        const int dx = static_cast<int>(std::floor(std::sqrt(
            std::max(0.0f, static_cast<float>(radius * radius) - fy * fy)
        )));

        const int leftX = rect.x + radius - dx;
        const int rightX = rect.x + rect.w - radius + dx - 1;

        const int topY = rect.y + dy;
        const int bottomY = rect.y + rect.h - 1 - dy;

        SDL_RenderDrawLine(renderer, leftX, topY, rightX, topY);
        SDL_RenderDrawLine(renderer, leftX, bottomY, rightX, bottomY);
    }
}

void drawRoundedRectOutline(SDL_Renderer* renderer, const SDL_Rect& rect, int radius, SDL_Color color) {
    if (renderer == nullptr || rect.w <= 0 || rect.h <= 0) {
        return;
    }

    radius = std::max(0, std::min(radius, std::min(rect.w, rect.h) / 2));

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    if (radius == 0) {
        SDL_RenderDrawRect(renderer, &rect);
        return;
    }

    const int left = rect.x;
    const int right = rect.x + rect.w - 1;
    const int top = rect.y;
    const int bottom = rect.y + rect.h - 1;

    // 四條直線
    SDL_RenderDrawLine(renderer, left + radius, top, right - radius, top);
    SDL_RenderDrawLine(renderer, left + radius, bottom, right - radius, bottom);
    SDL_RenderDrawLine(renderer, left, top + radius, left, bottom - radius);
    SDL_RenderDrawLine(renderer, right, top + radius, right, bottom - radius);

    // 四個角
    for (int dy = 0; dy < radius; ++dy) {
        const float fy = static_cast<float>(radius - dy - 0.5f);
        const int dx = static_cast<int>(std::floor(std::sqrt(
            std::max(0.0f, static_cast<float>(radius * radius) - fy * fy)
        )));

        const int x1 = left + radius - dx;
        const int x2 = right - radius + dx;
        const int y1 = top + dy;
        const int y2 = bottom - dy;

        SDL_RenderDrawPoint(renderer, x1, y1);
        SDL_RenderDrawPoint(renderer, x2, y1);
        SDL_RenderDrawPoint(renderer, x1, y2);
        SDL_RenderDrawPoint(renderer, x2, y2);
    }
}

struct LoadingAnimData {
    std::vector<SDL_Texture*> frames;
    std::vector<int> delaysMs;
    int totalDurationMs = 0;

    ~LoadingAnimData() {
        for (SDL_Texture* tex : frames) {
            if (tex != nullptr) {
                SDL_DestroyTexture(tex);
            }
        }
    }
};

float clamp01(float t) {
    return std::max(0.0f, std::min(1.0f, t));
}

int lerpInt(int a, int b, float t) {
    t = clamp01(t);
    return static_cast<int>(std::lround(static_cast<float>(a) + (static_cast<float>(b - a) * t)));
}

Uint8 lerpU8(Uint8 a, Uint8 b, float t) {
    return static_cast<Uint8>(lerpInt(static_cast<int>(a), static_cast<int>(b), t));
}

SDL_Color lerpColor(SDL_Color a, SDL_Color b, float t) {
    return SDL_Color{
        lerpU8(a.r, b.r, t),
        lerpU8(a.g, b.g, t),
        lerpU8(a.b, b.b, t),
        lerpU8(a.a, b.a, t)
    };
}

bool loadLoadingAnimation(
    SDL_Renderer* renderer,
    const fs::path& gifPath,
    LoadingAnimData& out
) {
    if (renderer == nullptr || !fs::exists(gifPath)) {
        return false;
    }

#if SDL_IMAGE_VERSION_ATLEAST(2, 6, 0)
    if (IMG_Animation* anim = IMG_LoadAnimation(gifPath.string().c_str())) {
        for (int i = 0; i < anim->count; ++i) {
            SDL_Surface* frameSurface = anim->frames[i];
            if (frameSurface == nullptr) {
                continue;
            }

            SDL_Texture* frameTexture = SDL_CreateTextureFromSurface(renderer, frameSurface);
            if (frameTexture == nullptr) {
                continue;
            }

            out.frames.push_back(frameTexture);

            int delay = 100;
            if (anim->delays != nullptr && anim->delays[i] > 0) {
                delay = anim->delays[i];
            }
            out.delaysMs.push_back(delay);
            out.totalDurationMs += delay;
        }

        IMG_FreeAnimation(anim);

        if (!out.frames.empty()) {
            if (out.totalDurationMs <= 0) {
                out.totalDurationMs = static_cast<int>(out.frames.size()) * 100;
            }
            return true;
        }
    }
#endif

    SDL_Texture* fallback = IMG_LoadTexture(renderer, gifPath.string().c_str());
    if (fallback == nullptr) {
        return false;
    }

    out.frames.push_back(fallback);
    out.delaysMs.push_back(100);
    out.totalDurationMs = 100;
    return true;
}

int getLoadingFrameIndex(const LoadingAnimData& anim, Uint32 elapsedMs) {
    if (anim.frames.empty()) {
        return -1;
    }
    if (anim.frames.size() == 1 || anim.totalDurationMs <= 0) {
        return 0;
    }

    const int t = static_cast<int>(elapsedMs % static_cast<Uint32>(anim.totalDurationMs));

    int acc = 0;
    for (std::size_t i = 0; i < anim.delaysMs.size(); ++i) {
        acc += std::max(1, anim.delaysMs[i]);
        if (t < acc) {
            return static_cast<int>(i);
        }
    }

    return static_cast<int>(anim.frames.size() - 1);
}

void renderLoadingScene(
    SDL_Renderer* renderer,
    TTF_Font* textFont,
    const LoadingAnimData& anim,
    const std::string& loadingText,
    SDL_Color bgColor,
    Uint8 contentAlpha,
    Uint32 elapsedMs
) {
    if (renderer == nullptr) {
        return;
    }

    SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, 255);
    SDL_RenderClear(renderer);

    int gifBottomY = kWindowHeight / 2;

    const int frameIndex = getLoadingFrameIndex(anim, elapsedMs);
    if (frameIndex >= 0) {
        SDL_Texture* frame = anim.frames[static_cast<std::size_t>(frameIndex)];
        if (frame != nullptr) {
            int texW = 0;
            int texH = 0;
            SDL_QueryTexture(frame, nullptr, nullptr, &texW, &texH);

            if (texW > 0 && texH > 0) {
                const float scale = std::min(
                    static_cast<float>(kLoadingGifMaxSize) / static_cast<float>(texW),
                    static_cast<float>(kLoadingGifMaxSize) / static_cast<float>(texH)
                );

                const int dstW = std::max(1, static_cast<int>(std::round(texW * scale)));
                const int dstH = std::max(1, static_cast<int>(std::round(texH * scale)));

                SDL_Rect dstRect{
                    (kWindowWidth - dstW) / 2,
                    kWindowHeight / 2 - dstH / 2 - 40,
                    dstW,
                    dstH
                };

                gifBottomY = dstRect.y + dstRect.h;

                SDL_SetTextureBlendMode(frame, SDL_BLENDMODE_BLEND);
                SDL_SetTextureAlphaMod(frame, contentAlpha);
                SDL_RenderCopy(renderer, frame, nullptr, &dstRect);
            }
        }
    }

    if (textFont != nullptr) {
        SDL_Color textColor{20, 20, 20, 255};
        SDL_Surface* textSurface =
            TTF_RenderUTF8_Blended(textFont, loadingText.c_str(), textColor);

        if (textSurface != nullptr) {
            SDL_Texture* textTexture =
                SDL_CreateTextureFromSurface(renderer, textSurface);

            if (textTexture != nullptr) {
                SDL_SetTextureBlendMode(textTexture, SDL_BLENDMODE_BLEND);
                SDL_SetTextureAlphaMod(textTexture, contentAlpha);

                SDL_Rect textRect{
                    (kWindowWidth - textSurface->w) / 2,
                    gifBottomY + kLoadingTextGap,
                    textSurface->w,
                    textSurface->h
                };
                SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
                SDL_DestroyTexture(textTexture);
            }

            SDL_FreeSurface(textSurface);
        }
    }

    SDL_RenderPresent(renderer);
}

} // namespace

SdlCardWindow::SdlCardWindow(const ProjectPaths& paths)
    : paths_(paths) {
}

std::vector<std::string> SdlCardWindow::expandPoolChars(const CharFreq& poolFreq) const {
    std::vector<std::pair<CodePoint, int>> rows(poolFreq.begin(), poolFreq.end());
    std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });

    std::vector<std::string> chars;
    for (const auto& [cp, count] : rows) {
        for (int i = 0; i < count; ++i) {
            chars.push_back(Utf8::fromCodePoint(cp));
        }
    }
    return chars;
}

void SdlCardWindow::splitRows(int totalCount, int& firstRowCount, int& secondRowCount) const {
    if (totalCount <= 0) {
        firstRowCount = 0;
        secondRowCount = 0;
        return;
    }

    if (totalCount <= kRowMaxCount) {
        firstRowCount = totalCount;
        secondRowCount = 0;
        return;
    }

    firstRowCount = (totalCount + 1) / 2;
    secondRowCount = totalCount / 2;

    if (firstRowCount > kRowMaxCount) {
        firstRowCount = kRowMaxCount;
        secondRowCount = totalCount - firstRowCount;
    }
}

int SdlCardWindow::run(
    const CharFreq& poolFreq,
    const std::string& windowTitle,
    const std::vector<std::string>& validAnswers,
    const std::vector<std::string>& optimalAnswers,
    int hintLimit,
    int initialTimeSeconds
) {
    return run(nullptr, nullptr, poolFreq, windowTitle, validAnswers, optimalAnswers,
               hintLimit, initialTimeSeconds);
}

int SdlCardWindow::run(
    SDL_Window* externalWindow,
    SDL_Renderer* externalRenderer,
    const CharFreq& poolFreq,
    const std::string& windowTitle,
    const std::vector<std::string>& validAnswers,
    const std::vector<std::string>& optimalAnswers,
    int hintLimit,
    int initialTimeSeconds
) {
    const std::vector<std::string> originalChars = expandPoolChars(poolFreq);

    std::vector<HandCard> handCards;
    handCards.reserve(originalChars.size());
    for (std::size_t i = 0; i < originalChars.size(); ++i) {
        handCards.push_back(HandCard{static_cast<int>(i), originalChars[i]});
    }

    std::vector<std::string> sortedValidAnswers = validAnswers;
    std::sort(sortedValidAnswers.begin(), sortedValidAnswers.end());
    sortedValidAnswers.erase(std::unique(sortedValidAnswers.begin(), sortedValidAnswers.end()), sortedValidAnswers.end());

    std::unordered_set<std::string> validAnswerSet(sortedValidAnswers.begin(), sortedValidAnswers.end());
    std::unordered_set<std::string> optimalAnswerSet(optimalAnswers.begin(), optimalAnswers.end());

    int currentScore = 0;
    int currentCombo = 0;
    Uint32 lastSuccessTicks = 0;

    int correctCount = 0;
    int optimalCorrectCount = 0;
    const int totalOptimalCount = static_cast<int>(optimalAnswerSet.size());

    std::vector<PlayedGroupState> playedHistory;
    std::vector<PlayedGroupState> redoPlayedHistory;
    std::vector<AlertItem> alerts;
    std::vector<ScoreFlyAnim> scoreFlyAnims;

        bool sdlInited = false;
    bool imgInited = false;
    bool ttfInited = false;
    bool ownsSdlSession = false;
    int runResult = kResultBackToHome;

    try {
        const bool reuseExistingWindow =
            (gTransferredWindow != nullptr && gTransferredRenderer != nullptr);
        const bool externalContext = (externalWindow != nullptr && externalRenderer != nullptr);

        if (!reuseExistingWindow && !externalContext) {
            if (SDL_Init(SDL_INIT_VIDEO) != 0) {
                throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
            }
            sdlInited = true;
            ownsSdlSession = true;

            const int imgFlags = IMG_INIT_PNG;
            if ((IMG_Init(imgFlags) & imgFlags) != imgFlags) {
                throw std::runtime_error(std::string("IMG_Init failed: ") + IMG_GetError());
            }
            imgInited = true;

            if (TTF_Init() != 0) {
                throw std::runtime_error(std::string("TTF_Init failed: ") + TTF_GetError());
            }
            ttfInited = true;
        }

        {
            SdlWindowGuard window;
            SdlRendererGuard renderer;
            SdlTextureGuard cardTexture;
            SdlTextureGuard timerBgTexture;
            SdlTextureGuard backIconTexture;
            SdlTextureGuard hintIconTexture;
            SdlTextureGuard undoIconTexture;
            SdlTextureGuard undoGrayIconTexture;
            SdlTextureGuard redoIconTexture;
            SdlTextureGuard redoGrayIconTexture;
            SdlTextureGuard playIconTexture;
            SdlTextureGuard closeIconTexture;

            if (reuseExistingWindow) {
                window.window = gTransferredWindow;
                renderer.renderer = gTransferredRenderer;
                window.owns = gTransferredContextOwned;
                renderer.owns = gTransferredContextOwned;
                gTransferredWindow = nullptr;
                gTransferredRenderer = nullptr;
                gTransferredContextOwned = true;

                SDL_SetWindowTitle(window.window, windowTitle.c_str());
                SDL_ShowWindow(window.window);
                SDL_RaiseWindow(window.window);
            } else if (externalContext) {
                window.window = externalWindow;
                renderer.renderer = externalRenderer;
                window.owns = false;
                renderer.owns = false;
                SDL_SetWindowTitle(window.window, windowTitle.c_str());
                SDL_ShowWindow(window.window);
                SDL_RaiseWindow(window.window);
            } else {
                SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

                window.window = SDL_CreateWindow(
                    windowTitle.c_str(),
                    SDL_WINDOWPOS_CENTERED,
                    SDL_WINDOWPOS_CENTERED,
                    kWindowWidth,
                    kWindowHeight,
                    SDL_WINDOW_SHOWN
                );
                if (window.window == nullptr) {
                    throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
                }

                renderer.renderer = createRendererWithFallback(window.window);
                if (renderer.renderer == nullptr) {
                    throw std::runtime_error(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());
                }
            }

            const fs::path cardPath = paths_.projectRoot / "assets" / "PoetryRebuildGame" / "image" / "card.png";
            const fs::path fontRoot = paths_.projectRoot / "assets" / "PoetryRebuildGame" / "font";
            const fs::path imageRoot = paths_.projectRoot / "assets" / "PoetryRebuildGame" / "image";

            cardTexture.texture = IMG_LoadTexture(renderer.renderer, cardPath.string().c_str());
            if (cardTexture.texture == nullptr) {
                throw std::runtime_error("Failed to load card texture: " + cardPath.string());
            }

            SDL_Texture* backIcon = loadTextureRequired(renderer.renderer, imageRoot / "open-door.png", backIconTexture);
            SDL_Texture* hintIcon = loadTextureRequired(renderer.renderer, imageRoot / "hints.png", hintIconTexture);
            SDL_Texture* undoIcon = loadTextureRequired(renderer.renderer, imageRoot / "go-back-arrow.png", undoIconTexture);
            SDL_Texture* undoGrayIcon = loadTextureRequired(renderer.renderer, imageRoot / "go-back-arrow-gray.png", undoGrayIconTexture);
            SDL_Texture* redoIcon = loadTextureRequired(renderer.renderer, imageRoot / "go-next-arrow.png", redoIconTexture);
            SDL_Texture* redoGrayIcon = loadTextureRequired(renderer.renderer, imageRoot / "go-next-arrow-gray.png", redoGrayIconTexture);
            SDL_Texture* playIcon = loadTextureRequired(renderer.renderer, imageRoot / "playing_card.png", playIconTexture);
            SDL_Texture* closeIcon = loadTextureRequired(renderer.renderer, imageRoot / "close.png", closeIconTexture);
            SDL_Texture* timerBg = loadTextureRequired(renderer.renderer, imageRoot / "bg3.png", timerBgTexture);

            std::vector<FontCandidate> fonts = buildFontCandidates(fontRoot);
            std::vector<TtfFontGuard> fontGuards(fonts.size());

            for (std::size_t i = 0; i < fonts.size(); ++i) {
                if (!fonts[i].path.empty() && !fs::exists(fonts[i].path)) {
                    fonts[i].loaded = false;
                    fonts[i].font = nullptr;
                    continue;
                }

                fontGuards[i].font = TTF_OpenFont(fonts[i].path.string().c_str(), kFontSize);
                fonts[i].font = fontGuards[i].font;
                fonts[i].loaded = (fontGuards[i].font != nullptr);

                if (fonts[i].loaded) {
                    std::cout << "[FontFallback] 已加载 " << fonts[i].label << "\n";
                }
            }

            bool anyFontLoaded = false;
            for (const auto& f : fonts) {
                if (f.loaded) {
                    anyFontLoaded = true;
                    break;
                }
            }

            if (!anyFontLoaded) {
                throw std::runtime_error("No usable font loaded from assets or system fonts.");
            }

            TtfFontGuard orderFontGuard;
            TtfFontGuard alertFontGuard;
            TtfFontGuard hintBadgeFontGuard;
            TtfFontGuard timerFontGuard;
            TtfFontGuard scoreFontGuard;
            TtfFontGuard scoreStatFontGuard;
            TtfFontGuard endTitleFontGuard;
            TtfFontGuard endScoreFontGuard;
            TtfFontGuard endMetaFontGuard;
            TtfFontGuard endButtonFontGuard;
            TtfFontGuard leaveConfirmTitleFontGuard;
            TtfFontGuard leaveConfirmButtonFontGuard;

            for (const auto& candidate : fonts) {
                if (!candidate.loaded) {
                    continue;
                }

                orderFontGuard.font = TTF_OpenFont(candidate.path.string().c_str(), kOrderFontSize);
                if (orderFontGuard.font != nullptr) {
                    std::cout << "[OrderFont] 已加载顺序编号字体: " << candidate.label << "\n";
                    break;
                }
            }

            if (orderFontGuard.font == nullptr) {
                throw std::runtime_error("No usable font for order labels.");
            }

            for (const auto& candidate : fonts) {
                if (!candidate.loaded) {
                    continue;
                }
                if (candidate.label.rfind("system:", 0) != 0) {
                    continue;
                }

                alertFontGuard.font = TTF_OpenFont(candidate.path.string().c_str(), kAlertFontSize);
                if (alertFontGuard.font != nullptr) {
                    TTF_SetFontStyle(alertFontGuard.font, TTF_STYLE_BOLD);
                    break;
                }
            }

            if (alertFontGuard.font == nullptr) {
                for (const auto& candidate : fonts) {
                    if (!candidate.loaded) {
                        continue;
                    }

                    alertFontGuard.font = TTF_OpenFont(candidate.path.string().c_str(), kAlertFontSize);
                    if (alertFontGuard.font != nullptr) {
                        TTF_SetFontStyle(alertFontGuard.font, TTF_STYLE_BOLD);
                        break;
                    }
                }
            }

            if (alertFontGuard.font == nullptr) {
                throw std::runtime_error("No usable font for alerts.");
            }

            for (const auto& candidate : fonts) {
                if (!candidate.loaded) {
                    continue;
                }

                hintBadgeFontGuard.font = TTF_OpenFont(candidate.path.string().c_str(), kHintBadgeFontSize);
                if (hintBadgeFontGuard.font != nullptr) {
                    TTF_SetFontStyle(hintBadgeFontGuard.font, TTF_STYLE_BOLD);
                    break;
                }
            }

            if (hintBadgeFontGuard.font == nullptr) {
                throw std::runtime_error("No usable font for hint badge.");
            }

            {
                const fs::path timerFontPath = fontRoot / "font1.ttf";
                if (fs::exists(timerFontPath)) {
                    timerFontGuard.font = TTF_OpenFont(timerFontPath.string().c_str(), kTimerFontSize);
                }

                if (timerFontGuard.font == nullptr) {
                    for (const auto& candidate : fonts) {
                        if (!candidate.loaded) {
                            continue;
                        }

                        timerFontGuard.font = TTF_OpenFont(candidate.path.string().c_str(), kTimerFontSize);
                        if (timerFontGuard.font != nullptr) {
                            break;
                        }
                    }
                }

                if (timerFontGuard.font == nullptr) {
                    throw std::runtime_error("No usable font for countdown timer.");
                }
            }

            {
                const fs::path scoreFontPath = fontRoot / "font3.ttf";
                if (!fs::exists(scoreFontPath)) {
                    throw std::runtime_error("Score font not found: " + scoreFontPath.string());
                }

                scoreFontGuard.font = TTF_OpenFont(scoreFontPath.string().c_str(), kScoreFontSize);
                if (scoreFontGuard.font == nullptr) {
                    throw std::runtime_error("Failed to load score font: " + scoreFontPath.string());
                }

                TTF_SetFontStyle(scoreFontGuard.font, TTF_STYLE_BOLD);
            }

            {
                const fs::path scoreStatFontPath = fontRoot / "font3.ttf";
                if (!fs::exists(scoreStatFontPath)) {
                    throw std::runtime_error("Score stat font not found: " + scoreStatFontPath.string());
                }

                scoreStatFontGuard.font = TTF_OpenFont(scoreStatFontPath.string().c_str(), kScoreStatFontSize);
                if (scoreStatFontGuard.font == nullptr) {
                    throw std::runtime_error("Failed to load score stat font: " + scoreStatFontPath.string());
                }

                TTF_SetFontStyle(scoreStatFontGuard.font, TTF_STYLE_BOLD);
            }

            {
                const fs::path endTitleFontPath = fontRoot / "font1.ttf";
                if (!fs::exists(endTitleFontPath)) {
                    throw std::runtime_error("End title font not found: " + endTitleFontPath.string());
                }

                endTitleFontGuard.font = TTF_OpenFont(endTitleFontPath.string().c_str(), kEndTitleFontSize);
                if (endTitleFontGuard.font == nullptr) {
                    throw std::runtime_error("Failed to load end title font: " + endTitleFontPath.string());
                }
                TTF_SetFontStyle(endTitleFontGuard.font, TTF_STYLE_BOLD);
            }

            {
                const fs::path endOtherFontPath = fontRoot / "font1.ttf";
                if (!fs::exists(endOtherFontPath)) {
                    throw std::runtime_error("End overlay font not found: " + endOtherFontPath.string());
                }

                endScoreFontGuard.font = TTF_OpenFont(endOtherFontPath.string().c_str(), kEndScoreFontSize);
                if (endScoreFontGuard.font == nullptr) {
                    throw std::runtime_error("Failed to load end score font: " + endOtherFontPath.string());
                }

                endButtonFontGuard.font = TTF_OpenFont(endOtherFontPath.string().c_str(), kEndButtonFontSize);
                if (endButtonFontGuard.font == nullptr) {
                    throw std::runtime_error("Failed to load end button font: " + endOtherFontPath.string());
                }

                TTF_SetFontStyle(endScoreFontGuard.font, TTF_STYLE_BOLD);
                TTF_SetFontStyle(endButtonFontGuard.font, TTF_STYLE_BOLD);
            }

            {
                const fs::path metaFontPath = fontRoot / "font2.ttf";
                if (!fs::exists(metaFontPath)) {
                    throw std::runtime_error("End meta font not found: " + metaFontPath.string());
                }

                endMetaFontGuard.font = TTF_OpenFont(metaFontPath.string().c_str(), kEndMetaFontSize);
                if (endMetaFontGuard.font == nullptr) {
                    throw std::runtime_error("Failed to load end meta font: " + metaFontPath.string());
                }

                TTF_SetFontStyle(endMetaFontGuard.font, TTF_STYLE_BOLD);
            }

            {
                const fs::path leaveConfirmFontPath = fontRoot / "font4.woff";
                leaveConfirmTitleFontGuard.font = openFontWithFallback(
                    fonts,
                    {leaveConfirmFontPath, fontRoot / "font1.ttf", fontRoot / "font2.ttf", fontRoot / "font3.ttf"},
                    kLeaveConfirmTitleFontSize
                );
                if (leaveConfirmTitleFontGuard.font == nullptr) {
                    throw std::runtime_error("Failed to load leave confirm title font.");
                }

                leaveConfirmButtonFontGuard.font = openFontWithFallback(
                    fonts,
                    {leaveConfirmFontPath, fontRoot / "font1.ttf", fontRoot / "font2.ttf", fontRoot / "font3.ttf"},
                    kLeaveConfirmButtonFontSize
                );
                if (leaveConfirmButtonFontGuard.font == nullptr) {
                    throw std::runtime_error("Failed to load leave confirm button font.");
                }
            }

            std::unordered_map<std::string, GlyphRenderInfo> glyphCache;
            buildGlyphCacheAndLogOnce(fonts, originalChars, glyphCache);

            std::vector<int> selectedOrder(handCards.size(), 0);
            std::vector<int> selectedSequence;
            int remainingHints = std::max(0, hintLimit);
            
            const int totalTimeSeconds = std::clamp(initialTimeSeconds, 0, kTimerMaxSeconds);
            const Uint32 roundStartTicks = SDL_GetTicks();
            bool timeUpAlertShown = false;

            const UiButtonLayout buttons = buildUiButtonLayout();

            bool showLeaveConfirm = false;
            bool leaveConfirmPaused = false;
            Uint32 pauseStartedTicks = 0;
            Uint32 pausedAccumulatedMs = 0;

            bool counterFrozen = false;
            int frozenRemainingTimeSeconds = 0;

            bool quit = false;
            while (!quit) {
                const Uint32 nowTicks = SDL_GetTicks();

                Uint32 effectivePausedMs = pausedAccumulatedMs;
                if (leaveConfirmPaused) {
                    effectivePausedMs += (nowTicks - pauseStartedTicks);
                }

                const Uint32 effectiveElapsedMs =
                    (nowTicks >= roundStartTicks + effectivePausedMs)
                        ? (nowTicks - roundStartTicks - effectivePausedMs)
                        : 0;

                const int elapsedSeconds = static_cast<int>(effectiveElapsedMs / 1000);
                const int remainingTimeSeconds = std::max(0, totalTimeSeconds - elapsedSeconds);

                const bool noPossibleAnswers = !hasAnyPossibleAnswer(handCards, sortedValidAnswers);
                const bool endedByTime = (remainingTimeSeconds <= 0);
                const bool showEndOverlay = endedByTime || noPossibleAnswers;

                if (showEndOverlay && !counterFrozen) {
                    counterFrozen = true;
                    frozenRemainingTimeSeconds = remainingTimeSeconds;
                }

                const int displayRemainingTimeSeconds =
                    counterFrozen ? frozenRemainingTimeSeconds : remainingTimeSeconds;

                if (remainingTimeSeconds <= 0 && !timeUpAlertShown) {
                    pushAlert(alerts, "时间到，本局已结束", nowTicks);
                    timeUpAlertShown = true;
                }

                updateAlerts(alerts, nowTicks);
                updateScoreFlyAnims(scoreFlyAnims, nowTicks);
                const auto alertLayouts = buildAlertLayouts(alertFontGuard.font, alerts);

                int firstRowCount = 0;
                int secondRowCount = 0;
                splitRows(static_cast<int>(handCards.size()), firstRowCount, secondRowCount);

                const float firstRowShift =
                    computeHalfCardStaggerShiftInCardWidth(firstRowCount, secondRowCount, true);
                const float secondRowShift =
                    computeHalfCardStaggerShiftInCardWidth(secondRowCount, firstRowCount, false);

                std::vector<SDL_Rect> firstRowRects =
                    buildRowRects(cardTexture.texture, firstRowCount, kFirstRowBottom, firstRowShift);
                std::vector<SDL_Rect> secondRowRects =
                    buildRowRects(cardTexture.texture, secondRowCount, kSecondRowBottom, secondRowShift);

                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_QUIT) {
                        quit = true;
                    } else if (event.type == SDL_KEYDOWN) {
                        if (event.key.keysym.sym == SDLK_ESCAPE) {
                            if (showEndOverlay) {
                                continue;
                            }

                            if (!showLeaveConfirm) {
                                showLeaveConfirm = true;
                                leaveConfirmPaused = true;
                                pauseStartedTicks = SDL_GetTicks();
                            } else {
                                showLeaveConfirm = false;
                                if (leaveConfirmPaused) {
                                    const Uint32 now = SDL_GetTicks();
                                    pausedAccumulatedMs += (now - pauseStartedTicks);
                                    leaveConfirmPaused = false;
                                    pauseStartedTicks = 0;
                                }
                            }
                        }
                    } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                        const int mx = event.button.x;
                        const int my = event.button.y;
                        const Uint32 now = SDL_GetTicks();

                        bool closedAlert = false;
                        for (std::size_t i = 0; i < alertLayouts.size() && i < alerts.size(); ++i) {
                            if (pointInRect(mx, my, alertLayouts[i].closeRect)) {
                                alerts[i].closing = true;
                                alerts[i].closingAt = now;
                                closedAlert = true;
                                break;
                            }
                        }
                        if (closedAlert) {
                            continue;
                        }

                        if (showLeaveConfirm) {
                            const LeaveConfirmButtons leaveButtons = buildLeaveConfirmButtons();

                            if (pointInRect(mx, my, leaveButtons.leaveGame)) {
                                runResult = kResultBackToHome;
                                quit = true;
                                continue;
                            }

                            if (pointInRect(mx, my, leaveButtons.resumeGame)) {
                                showLeaveConfirm = false;
                                if (leaveConfirmPaused) {
                                    pausedAccumulatedMs += (now - pauseStartedTicks);
                                    leaveConfirmPaused = false;
                                    pauseStartedTicks = 0;
                                }
                                continue;
                            }

                            // 確認框打開時，點其他地方不往下穿透
                            continue;
                        }

                        if (pointInRect(mx, my, buttons.back)) {
                            if (!showEndOverlay && !showLeaveConfirm) {
                                showLeaveConfirm = true;
                                leaveConfirmPaused = true;
                                pauseStartedTicks = now;
                            }
                            continue;
                        }

                        if (showEndOverlay) {
                            const EndOverlayButtons endButtons = buildEndOverlayButtons();

                            if (pointInRect(mx, my, endButtons.backHome)) {
                                runResult = kResultBackToHome;
                                quit = true;
                                continue;
                            }

                            if (pointInRect(mx, my, endButtons.replay)) {
                                runResult = kResultReplaySameDifficulty;
                                quit = true;
                                continue;
                            }

                            continue;
                        }

                        if (pointInRect(mx, my, buttons.hint)) {
                            if (remainingHints <= 0) {
                                pushAlert(alerts, "提示次数已超过", now);
                                continue;
                            }

                            const std::string selectedText = buildSelectedText(handCards, selectedSequence);
                            const auto hintCp = extractHintCodePoint(handCards, sortedValidAnswers, selectedText);

                            if (!hintCp.has_value()) {
                                if (selectedText.empty()) {
                                    pushAlert(alerts, "当前剩余手牌中没有可提示的答案", now);
                                } else if (validAnswerSet.find(selectedText) != validAnswerSet.end()) {
                                    pushAlert(alerts, "当前已构成完整合法答案，可直接出牌", now);
                                } else {
                                    pushAlert(alerts, "当前已选字串不是任何可用答案的前缀", now);
                                }
                            } else {
                                const std::string hintChar = Utf8::fromCodePoint(*hintCp);
                                const bool selectedByHint = selectFirstMatchingCard(
                                    handCards,
                                    hintChar,
                                    selectedSequence,
                                    selectedOrder
                                );

                                if (selectedByHint) {
                                    --remainingHints;
                                } else {
                                    pushAlert(alerts, "找到了提示字，但当前没有可升起的未选中对应卡", now);
                                }
                            }
                            continue;
                        }

                        if (pointInRect(mx, my, buttons.undo)) {
                            if (playedHistory.empty()) {
                                pushAlert(alerts, "目前没有上一步可撤回", now);
                            } else {
                                PlayedGroupState last = playedHistory.back();
                                playedHistory.pop_back();

                                currentScore = std::max(0, currentScore - last.gainedScore);

                                if (correctCount > 0) {
                                    --correctCount;
                                }
                                if (last.isOptimalAnswer && optimalCorrectCount > 0) {
                                    --optimalCorrectCount;
                                }

                                restorePlayedGroupToHand(handCards, last);
                                redoPlayedHistory.push_back(last);

                                clearSelection(selectedSequence, selectedOrder, handCards.size());
                            }
                            continue;
                        }

                        if (pointInRect(mx, my, buttons.redo)) {
                            if (redoPlayedHistory.empty()) {
                                pushAlert(alerts, "目前没有下一步可返回", now);
                            } else {
                                PlayedGroupState last = redoPlayedHistory.back();
                                redoPlayedHistory.pop_back();

                                currentScore += last.gainedScore;
                                ++correctCount;
                                if (last.isOptimalAnswer) {
                                    ++optimalCorrectCount;
                                }

                                pushScoreFlyAnim(scoreFlyAnims, last.gainedScore, now);

                                removePlayedGroupFromHandForRedo(handCards, last);
                                playedHistory.push_back(last);

                                clearSelection(selectedSequence, selectedOrder, handCards.size());
                            }
                            continue;
                        }

                        if (pointInRect(mx, my, buttons.play)) {
                            if (selectedSequence.empty()) {
                                pushAlert(alerts, "请先选择要出的牌", now);
                            } else {
                                const std::string selectedText = buildSelectedText(handCards, selectedSequence);
                                if (validAnswerSet.find(selectedText) == validAnswerSet.end()) {
                                    pushAlert(alerts, "当前选择不是合法的成语 / 诗句", now);
                                } else {
                                    PlayedGroupState group;
                                    group.text = selectedText;
                                    group.cards.reserve(selectedSequence.size());

                                    for (const int idx : selectedSequence) {
                                        if (idx >= 0 && idx < static_cast<int>(handCards.size())) {
                                            group.cards.push_back(handCards[static_cast<std::size_t>(idx)]);
                                        }
                                    }

                                    if (!group.cards.empty()) {
                                        Uint32 deltaMs = kMediumComboWindowMs + 1;
                                        if (lastSuccessTicks != 0) {
                                            deltaMs = now - lastSuccessTicks;
                                        }

                                        if (lastSuccessTicks != 0 && deltaMs <= kMediumComboWindowMs) {
                                            ++currentCombo;
                                        } else {
                                            currentCombo = 1;
                                        }

                                        const bool isOptimalAnswer = (optimalAnswerSet.find(selectedText) != optimalAnswerSet.end());

                                        const int gainScore = calcAnswerGainScore(
                                            selectedText,
                                            currentCombo,
                                            deltaMs,
                                            optimalAnswerSet
                                        );

                                        group.gainedScore = gainScore;
                                        group.isOptimalAnswer = isOptimalAnswer;
                                        group.comboAfterPlay = currentCombo;

                                        currentScore += gainScore;
                                        lastSuccessTicks = now;

                                        ++correctCount;
                                        if (isOptimalAnswer) {
                                            ++optimalCorrectCount;
                                        }

                                        pushScoreFlyAnim(scoreFlyAnims, gainScore, now);

                                        playedHistory.push_back(group);
                                        redoPlayedHistory.clear();

                                        eraseSelectedFromHand(handCards, selectedSequence);
                                        clearSelection(selectedSequence, selectedOrder, handCards.size());
                                    }
                                }
                            }
                            continue;
                        }

                        bool hit = false;

                        // 先檢查下排
                        for (int i = firstRowCount - 1; i >= 0; --i) {
                            const bool isSelected = (selectedOrder[static_cast<std::size_t>(i)] > 0);
                            SDL_Rect currentRect = liftedRect(
                                firstRowRects[static_cast<std::size_t>(i)],
                                isSelected
                            );
                            if (pointInRect(mx, my, currentRect)) {
                                toggleSelectionOrder(i, selectedSequence, selectedOrder);
                                hit = true;
                                break;
                            }
                        }

                        // 再檢查上排
                        if (!hit) {
                            for (int i = secondRowCount - 1; i >= 0; --i) {
                                const int idx = firstRowCount + i;
                                const bool isSelected = (selectedOrder[static_cast<std::size_t>(idx)] > 0);
                                SDL_Rect currentRect = liftedRect(
                                    secondRowRects[static_cast<std::size_t>(i)],
                                    isSelected
                                );
                                if (pointInRect(mx, my, currentRect)) {
                                    toggleSelectionOrder(idx, selectedSequence, selectedOrder);
                                    hit = true;
                                    break;
                                }
                            }
                        }

                        // 點到其他空白區域：取消所有已選但未出的卡
                        if (!hit && !selectedSequence.empty()) {
                            clearSelection(selectedSequence, selectedOrder, handCards.size());
                        }
                    }
                }
                
                SDL_SetRenderDrawColor(
                    renderer.renderer,
                    gCurrentRoundBgColor.r,
                    gCurrentRoundBgColor.g,
                    gCurrentRoundBgColor.b,
                    gCurrentRoundBgColor.a
                );

                SDL_RenderClear(renderer.renderer);
                
                renderTimerBackground(
                    renderer.renderer,
                    timerBg
                );

                renderCountdown(
                    renderer.renderer,
                    timerFontGuard.font,
                    displayRemainingTimeSeconds
                );

                renderUiPanels(
                    renderer.renderer,
                    buttons,
                    backIcon,
                    hintIcon,
                    playedHistory.empty() ? undoGrayIcon : undoIcon,
                    redoPlayedHistory.empty() ? redoGrayIcon : redoIcon,
                    playIcon
                );

                renderTopCenterScore(
                    renderer.renderer,
                    scoreFontGuard.font,
                    currentScore
                );

                renderTopCenterScoreStats(
                    renderer.renderer,
                    scoreStatFontGuard.font,
                    optimalCorrectCount,
                    correctCount,
                    totalOptimalCount
                );

                renderHintCountBadge(
                    renderer.renderer,
                    hintBadgeFontGuard.font,
                    buttons.hint,
                    remainingHints
                );

                renderPlayedGroups(
                    renderer.renderer,
                    cardTexture.texture,
                    glyphCache,
                    playedHistory
                );

                renderScoreFlyAnims(
                    renderer.renderer,
                    scoreFontGuard.font,
                    scoreFlyAnims
                );

                renderAlerts(
                    renderer.renderer,
                    alertFontGuard.font,
                    closeIcon,
                    alerts
                );

                for (int i = 0; i < secondRowCount; ++i) {
                    const int idx = firstRowCount + i;
                    const bool isSelected = (selectedOrder[static_cast<std::size_t>(idx)] > 0);

                    SDL_Rect cardRect = liftedRect(
                        secondRowRects[static_cast<std::size_t>(i)],
                        isSelected
                    );

                    renderCard(
                        renderer.renderer,
                        cardTexture.texture,
                        glyphCache,
                        orderFontGuard.font,
                        handCards[static_cast<std::size_t>(idx)].text,
                        cardRect,
                        selectedOrder[static_cast<std::size_t>(idx)]
                    );
                }

                for (int i = 0; i < firstRowCount; ++i) {
                    const bool isSelected = (selectedOrder[static_cast<std::size_t>(i)] > 0);

                    SDL_Rect cardRect = liftedRect(
                        firstRowRects[static_cast<std::size_t>(i)],
                        isSelected
                    );

                    renderCard(
                        renderer.renderer,
                        cardTexture.texture,
                        glyphCache,
                        orderFontGuard.font,
                        handCards[static_cast<std::size_t>(i)].text,
                        cardRect,
                        selectedOrder[static_cast<std::size_t>(i)]
                    );
                }

                if (showEndOverlay) {
                    renderEndOverlay(
                        renderer.renderer,
                        cardTexture.texture,
                        glyphCache,
                        endTitleFontGuard.font,
                        endScoreFontGuard.font,
                        endMetaFontGuard.font,
                        endedByTime ? "时间到" : "胜利",
                        currentScore,
                        playedHistory,
                        calcMaxComboFromHistory(playedHistory),
                        totalTimeSeconds - displayRemainingTimeSeconds
                    );

                    renderEndOverlayButtons(
                        renderer.renderer,
                        endButtonFontGuard.font
                    );
                } else if (showLeaveConfirm) {
                    renderLeaveConfirmOverlay(
                        renderer.renderer,
                        leaveConfirmTitleFontGuard.font,
                        leaveConfirmButtonFontGuard.font
                    );
                }

                SDL_RenderPresent(renderer.renderer);
            }
        }

                if (ownsSdlSession && ttfInited) {
            TTF_Quit();
            ttfInited = false;
        }
        if (ownsSdlSession && imgInited) {
            IMG_Quit();
            imgInited = false;
        }
        if (ownsSdlSession && sdlInited) {
            SDL_Quit();
            sdlInited = false;
        }

        return runResult;
    } catch (...) {
        if (ownsSdlSession && ttfInited) {
            TTF_Quit();
        }
        if (ownsSdlSession && imgInited) {
            IMG_Quit();
        }
        if (ownsSdlSession && sdlInited) {
            SDL_Quit();
        }
        throw;
    }
}

int SdlCardWindow::runWithLoading(
    const std::string& windowTitle,
    const std::function<PreparedRoundData()>& builder
) {
    return runWithLoading(nullptr, nullptr, windowTitle, builder);
}

int SdlCardWindow::runWithLoading(
    SDL_Window* externalWindow,
    SDL_Renderer* externalRenderer,
    const std::string& windowTitle,
    const std::function<PreparedRoundData()>& builder
) {
    using namespace std::chrono_literals;

    bool sdlInited = false;
    bool imgInited = false;
    bool ttfInited = false;
    int runResult = kResultBackToHome;

    try {
        const bool externalContext = (externalWindow != nullptr && externalRenderer != nullptr);

        if (!externalContext) {
            if (SDL_Init(SDL_INIT_VIDEO) != 0) {
                throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
            }
            sdlInited = true;

            const int imgFlags = IMG_INIT_PNG;
            if ((IMG_Init(imgFlags) & imgFlags) != imgFlags) {
                throw std::runtime_error(std::string("IMG_Init failed: ") + IMG_GetError());
            }
            imgInited = true;

            if (TTF_Init() != 0) {
                throw std::runtime_error(std::string("TTF_Init failed: ") + TTF_GetError());
            }
            ttfInited = true;
        }

        PreparedRoundData prepared;

        {
            SdlWindowGuard window;
            SdlRendererGuard renderer;
            TtfFontGuard loadingFontGuard;

            if (externalContext) {
                window.window = externalWindow;
                renderer.renderer = externalRenderer;
                window.owns = false;
                renderer.owns = false;
                SDL_SetWindowTitle(window.window, windowTitle.c_str());
                SDL_ShowWindow(window.window);
                SDL_RaiseWindow(window.window);
            } else {
                window.window = SDL_CreateWindow(
                    windowTitle.c_str(),
                    SDL_WINDOWPOS_CENTERED,
                    SDL_WINDOWPOS_CENTERED,
                    kWindowWidth,
                    kWindowHeight,
                    SDL_WINDOW_SHOWN
                );
                if (window.window == nullptr) {
                    throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
                }

                renderer.renderer = createRendererWithFallback(window.window);
                if (renderer.renderer == nullptr) {
                    throw std::runtime_error(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());
                }
            }

            const fs::path fontRoot = paths_.projectRoot / "assets" / "PoetryRebuildGame" / "font";
            const fs::path imageRoot = paths_.projectRoot / "assets" / "PoetryRebuildGame" / "image";

            {
                const fs::path loadingFontPath = fontRoot / "font4.woff";
                if (fs::exists(loadingFontPath)) {
                    loadingFontGuard.font =
                        TTF_OpenFont(loadingFontPath.string().c_str(), kLoadingTextFontSize);
                }

                if (loadingFontGuard.font == nullptr) {
                    auto fonts = buildFontCandidates(fontRoot);
                    for (const auto& candidate : fonts) {
                        if (!candidate.path.empty() && fs::exists(candidate.path)) {
                            loadingFontGuard.font =
                                TTF_OpenFont(candidate.path.string().c_str(), kLoadingTextFontSize);
                            if (loadingFontGuard.font != nullptr) {
                                break;
                            }
                        }
                    }
                }

                if (loadingFontGuard.font == nullptr) {
                    throw std::runtime_error("No usable font for loading scene.");
                }
            }

            LoadingAnimData loadingAnim;
            loadLoadingAnimation(renderer.renderer, imageRoot / "giphy.gif", loadingAnim);

            auto future = std::async(std::launch::async, builder);

            const Uint32 loadingStartTicks = SDL_GetTicks();
            bool quitRequested = false;

            while (true) {
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_QUIT) {
                        quitRequested = true;
                    } else if (event.type == SDL_KEYDOWN &&
                               event.key.keysym.sym == SDLK_ESCAPE) {
                        quitRequested = true;
                    }
                }

                if (quitRequested) {
                    return kResultBackToHome;
                }

                if (future.wait_for(0ms) == std::future_status::ready) {
                    prepared = future.get();
                    break;
                }

                renderLoadingScene(
                    renderer.renderer,
                    loadingFontGuard.font,
                    loadingAnim,
                    "正在生成关卡中",
                    kLoadingBgColor,
                    255,
                    SDL_GetTicks() - loadingStartTicks
                );

                SDL_Delay(16);
            }

            gCurrentRoundBgColor = SDL_Color{
                static_cast<Uint8>(std::clamp(prepared.bgR, 0, 255)),
                static_cast<Uint8>(std::clamp(prepared.bgG, 0, 255)),
                static_cast<Uint8>(std::clamp(prepared.bgB, 0, 255)),
                255
            };

            const Uint32 fadeStartTicks = SDL_GetTicks();

            while (true) {
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_QUIT) {
                        return kResultBackToHome;
                    } else if (event.type == SDL_KEYDOWN &&
                               event.key.keysym.sym == SDLK_ESCAPE) {
                        return kResultBackToHome;
                    }
                }

                const Uint32 now = SDL_GetTicks();
                const float t = clamp01(
                    static_cast<float>(now - fadeStartTicks) /
                    static_cast<float>(kStageFadeMs)
                );

                const SDL_Color bg = lerpColor(kLoadingBgColor, gCurrentRoundBgColor, t);
                const Uint8 contentAlpha = static_cast<Uint8>(
                    std::lround(255.0f * (1.0f - t))
                );

                renderLoadingScene(
                    renderer.renderer,
                    loadingFontGuard.font,
                    loadingAnim,
                    "正在生成关卡中",
                    bg,
                    contentAlpha,
                    now - loadingStartTicks
                );

                if (t >= 1.0f) {
                    break;
                }

                SDL_Delay(16);
                        }

            // 把 loading 场景当前正在使用的 window / renderer 直接交给正式关卡继续使用
            gTransferredWindow = window.window;
            gTransferredRenderer = renderer.renderer;
            gTransferredContextOwned = !externalContext;
            window.window = nullptr;
            renderer.renderer = nullptr;
        }

        runResult = run(
            prepared.poolFreq,
            windowTitle,
            prepared.validAnswers,
            prepared.optimalAnswers,
            prepared.hintLimit,
            prepared.initialTimeSeconds
        );

        if (ttfInited) {
            TTF_Quit();
            ttfInited = false;
        }
        if (imgInited) {
            IMG_Quit();
            imgInited = false;
        }
        if (sdlInited) {
            SDL_Quit();
            sdlInited = false;
        }

        return runResult;
    } catch (...) {
        if (ttfInited) {
            TTF_Quit();
        }
        if (imgInited) {
            IMG_Quit();
        }
        if (sdlInited) {
            SDL_Quit();
        }
        throw;
    }
}

} // namespace lineverse::poetryrebuild