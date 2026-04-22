// src/HandleGame/UI/HGUI.cpp
#include "UI/HGUI.hpp"

#include <SDL_image.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <string>

namespace HandleGame {

HGUI::HGUI() = default;
HGUI::~HGUI() { shutdown_(); }

static bool fileExists(const std::string& p) {
    std::error_code ec;
    return std::filesystem::exists(std::filesystem::path(p), ec);
}


static constexpr int kDesignWidth = 1200;
static constexpr int kDesignHeight = 800;
static constexpr int kDifficultyCaptionTop = 170;
static constexpr int kDifficultyButtonStartY = 300;
static constexpr int kDifficultyButtonGap = 45;
static constexpr int kDifficultySelectedBgWidth = 460;
static constexpr int kDifficultySelectedBgHeight = 150;
static constexpr int kDifficultySelectionBgTargetHeight = 1000;
static constexpr int kDifficultySelectionBgOffsetX = 200;
static constexpr int kDifficultyBackIconSize = 64;
static constexpr int kDifficultyBackMarginTop = 34;
static constexpr int kDifficultyBackMarginRight = 42;
static constexpr int kDifficultyBackHighlightWidth = 150;
static constexpr int kDifficultyBackHighlightHeight = 90;

static const std::array<std::string, 3> kDifficultyLabels = {
    u8"简单",
    u8"中等",
    u8"困难"
};

static constexpr int kPlayingTopIconSize = 56;
static constexpr int kPlayingTopIconGap = 18;
static constexpr int kPlayingTopIconMarginRight = 42;
static constexpr int kPlayingTopIconHighlightWidth = 136;
static constexpr int kPlayingTopIconHighlightHeight = 86;
static constexpr int kSubmitHighlightWidth = 170;
static constexpr int kSubmitHighlightHeight = 86;
static constexpr int kHintCloseIconSize = 38;

static SDL_Rect centeredRectAround(const SDL_Rect& inner, int w, int h) {
    return SDL_Rect{
        inner.x + inner.w / 2 - w / 2,
        inner.y + inner.h / 2 - h / 2,
        w,
        h
    };
}

static const char* difficultyModeName(Difficulty diff) {
    if (diff == Difficulty::Easy) return u8"简单模式";
    if (diff == Difficulty::Normal) return u8"中等模式";
    return u8"困难模式";
}

static bool fsExists(const std::filesystem::path& p) {
    std::error_code ec;
    return std::filesystem::exists(p, ec);
}

static SDL_Rect buildDifficultyBgRect(int texW, int texH, int windowW, int windowH) {
    const double scaleY = windowH > 0
        ? static_cast<double>(windowH) / static_cast<double>(kDesignHeight)
        : 1.0;
    const int targetH = std::max(1, static_cast<int>(std::lround(kDifficultySelectionBgTargetHeight * scaleY)));
    const int offsetX = static_cast<int>(std::lround(kDifficultySelectionBgOffsetX *
        (windowW > 0 ? static_cast<double>(windowW) / static_cast<double>(kDesignWidth) : 1.0)));

    if (texW <= 0 || texH <= 0) {
        return SDL_Rect{offsetX, 0, 1000, targetH};
    }

    const int targetW = static_cast<int>(std::lround(
        static_cast<double>(texW) * static_cast<double>(targetH) / static_cast<double>(texH)
    ));
    return SDL_Rect{offsetX, 0, targetW, targetH};
}

static std::filesystem::path findHomepageRootFrom(const std::filesystem::path& startPath) {
    std::error_code ec;
    if (startPath.empty()) return {};

    std::filesystem::path p = startPath;
    if (fsExists(p) && std::filesystem::is_regular_file(p, ec)) {
        p = p.parent_path();
    }

    for (int i = 0; i < 10 && !p.empty(); ++i) {
        const auto selectedBg = p / "image" / "bg2.png";
        const auto selectionBg = p / "image" / "bg3.jpg";
        const auto selectionFont = p / "font" / "font2.ttf";
        if (fsExists(selectedBg) && fsExists(selectionBg) && fsExists(selectionFont)) {
            return p;
        }
        if (p == p.parent_path()) break;
        p = p.parent_path();
    }
    return {};
}

static std::filesystem::path findHomepageRoot(const HGUIConfig& cfg, const std::string& chosenFontPath) {
    std::vector<std::filesystem::path> candidates;
    if (!chosenFontPath.empty()) candidates.emplace_back(chosenFontPath);
    for (const auto& fp : cfg.fontCandidates) {
        if (!fp.empty()) candidates.emplace_back(fp);
    }
    candidates.emplace_back(std::filesystem::current_path());

    for (const auto& candidate : candidates) {
        auto root = findHomepageRootFrom(candidate);
        if (!root.empty()) return root;
    }
    return {};
}

static std::filesystem::path findHandleGameImagePath(const HGUIConfig& cfg,
                                                     const std::string& chosenFontPath,
                                                     const std::string& filename) {
    std::vector<std::filesystem::path> candidates;
    if (!chosenFontPath.empty()) candidates.emplace_back(chosenFontPath);
    for (const auto& fp : cfg.fontCandidates) {
        if (!fp.empty()) candidates.emplace_back(fp);
    }
    candidates.emplace_back(std::filesystem::current_path());

    std::error_code ec;
    for (auto p : candidates) {
        if (p.empty()) continue;
        if (fsExists(p) && std::filesystem::is_regular_file(p, ec)) {
            p = p.parent_path();
        }

        for (int i = 0; i < 12 && !p.empty(); ++i) {
            const std::array<std::filesystem::path, 3> imageCandidates = {
                p / "assets" / "HandleGame" / "image" / filename,
                p / "HandleGame" / "image" / filename,
                p / "image" / filename
            };
            for (const auto& imagePath : imageCandidates) {
                if (fsExists(imagePath)) return imagePath;
            }
            if (p == p.parent_path()) break;
            p = p.parent_path();
        }
    }
    return {};
}

static void drawDifficultySelectedFallback(SDL_Renderer* ren, const SDL_Rect& r) {
    SDL_BlendMode oldMode = SDL_BLENDMODE_NONE;
    SDL_GetRenderDrawBlendMode(ren, &oldMode);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, 255, 255, 255, 210);
    SDL_RenderFillRect(ren, &r);
    SDL_SetRenderDrawColor(ren, 80, 80, 80, 110);
    SDL_RenderDrawRect(ren, &r);
    SDL_SetRenderDrawBlendMode(ren, oldMode);
}

static void trimInPlace(std::string& s) {
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) s.pop_back();
    size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) i++;
    if (i > 0) s.erase(0, i);
}

static void truncateUtf8InPlace(std::string& s, size_t maxChars) {
    auto chs = HGEngine::splitUtf8Chars(s);
    if (chs.size() <= maxChars) return;
    std::string out;
    for (size_t i = 0; i < maxChars; i++) out += chs[i];
    s.swap(out);
}

static std::vector<std::string> utf8CharsAtMost(const std::string& s, size_t maxChars) {
    auto chs = HGEngine::splitUtf8Chars(s);
    if (chs.size() <= maxChars) return chs;
    chs.resize(maxChars);
    return chs;
}

// -------------------- 标调定位核心：选择韵母中“承载声调”的元音索引 --------------------
static inline bool isVowelLetter(char c) {
    return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'v';
}

static int toneAnchorIndexInFinal(const std::string& fin) {
    if (fin.empty()) return -1;

    auto posA = fin.find('a');
    if (posA != std::string::npos) return (int)posA;

    auto posE = fin.find('e');
    if (posE != std::string::npos) return (int)posE;

    auto posOU = fin.find("ou");
    if (posOU != std::string::npos) return (int)posOU; // ou 标在 o 上

    for (int i = (int)fin.size() - 1; i >= 0; --i) {
        if (isVowelLetter(fin[(size_t)i])) return i;
    }
    return (int)fin.size() / 2;
}

// v -> ü（仅显示）
static std::string displayReplaceVWithUmlaut(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char ch : s) {
        if (ch == 'v') out += u8"ü";
        else out.push_back(ch);
    }
    return out;
}

// -------------------- 方案二：渲染不透明背景的“抗锯齿”字符贴片 --------------------
static SDL_Surface* renderOpaqueBgAA(TTF_Font* font, const std::string& text, SDL_Color fg, SDL_Color bg) {
    if (!font || text.empty()) return nullptr;

    SDL_Surface* aa = TTF_RenderUTF8_Blended(font, text.c_str(), fg);
    if (!aa) return nullptr;

    SDL_Surface* out = SDL_CreateRGBSurfaceWithFormat(0, aa->w, aa->h, 32, SDL_PIXELFORMAT_RGBA32);
    if (!out) {
        SDL_FreeSurface(aa);
        return nullptr;
    }
    SDL_FillRect(out, nullptr, SDL_MapRGBA(out->format, bg.r, bg.g, bg.b, bg.a));
    SDL_SetSurfaceBlendMode(aa, SDL_BLENDMODE_BLEND);
    SDL_BlitSurface(aa, nullptr, out, nullptr);
    SDL_FreeSurface(aa);
    return out;
}

//  只裁剪“Y方向”（保持X宽度不变），避免水平定位漂移
static bool findInkRowsRGBA32(SDL_Surface* surf, SDL_Color bg, int& outMinY, int& outMaxY) {
    if (!surf) return false;
    if (surf->format->format != SDL_PIXELFORMAT_RGBA32) return false;

    const uint32_t bgpx = SDL_MapRGBA(surf->format, bg.r, bg.g, bg.b, bg.a);

    if (SDL_LockSurface(surf) != 0) return false;

    auto* px = (uint32_t*)surf->pixels;
    const int pitch32 = surf->pitch / 4;

    int minY = surf->h, maxY = -1;
    for (int y = 0; y < surf->h; ++y) {
        bool ink = false;
        uint32_t* row = px + y * pitch32;
        for (int x = 0; x < surf->w; ++x) {
            if (row[x] != bgpx) { ink = true; break; }
        }
        if (ink) {
            minY = std::min(minY, y);
            maxY = std::max(maxY, y);
        }
    }

    SDL_UnlockSurface(surf);

    if (maxY < 0) return false;
    outMinY = minY;
    outMaxY = maxY;
    return true;
}

// -------------------- 音调符号：优先 ˉˊˇˋ；不支持则回退 ¯´ˇ` --------------------
struct ToneSym { uint32_t cp; const char* utf8; };

static ToneSym kTone1Pref[] = { {0x02C9, u8"ˉ"}, {0x00AF, u8"¯"} };
static ToneSym kTone2Pref[] = { {0x02CA, u8"ˊ"}, {0x00B4, u8"´"} };
static ToneSym kTone3Pref[] = { {0x02C7, u8"ˇ"} };
static ToneSym kTone4Pref[] = { {0x02CB, u8"ˋ"}, {0x0060, u8"`"} };

static bool fontHasGlyph(TTF_Font* f, uint32_t cp) {
    if (!f) return false;
    return TTF_GlyphIsProvided32(f, cp) != 0;
}

static const char* pickToneSymbolUTF8(TTF_Font* f, int tone) {
    if (tone < 1 || tone > 4) return "";

    auto pick = [&](ToneSym* arr, int n) -> const char* {
        for (int i = 0; i < n; ++i) {
            if (fontHasGlyph(f, arr[i].cp)) return arr[i].utf8;
        }
        return "";
    };

    if (tone == 1) return pick(kTone1Pref, (int)(sizeof(kTone1Pref) / sizeof(ToneSym)));
    if (tone == 2) return pick(kTone2Pref, (int)(sizeof(kTone2Pref) / sizeof(ToneSym)));
    if (tone == 3) return pick(kTone3Pref, (int)(sizeof(kTone3Pref) / sizeof(ToneSym)));
    if (tone == 4) return pick(kTone4Pref, (int)(sizeof(kTone4Pref) / sizeof(ToneSym)));
    return "";
}

static bool toneSymbolsAvailable(TTF_Font* f) {
    if (!f) return false;
    for (int t = 1; t <= 4; ++t) {
        const char* s = pickToneSymbolUTF8(f, t);
        if (!s || s[0] == '\0') return false;
    }
    return true;
}

//  方案二：音调贴片（不缩放；可裁剪前N行像素）
// 重点修复：
// - 水平位置用“centerX”传入，然后用实际贴片surf->w居中
// - 只裁剪Y，不裁剪X，避免漂移
static void drawToneStickerCropped(SDL_Renderer* ren,
                                  TTF_Font* font,
                                  const std::string& sym,
                                  const SDL_Rect& clip,
                                  int centerX,
                                  int baseY,
                                  SDL_Color fg,
                                  SDL_Color bg,
                                  int toneYOffsetPx,
                                  int toneMaxRows) {
    if (!ren || !font || sym.empty()) return;

    SDL_Surface* surf = renderOpaqueBgAA(font, sym, fg, bg);
    if (!surf) return;

    // 找到“有墨迹”的行，做Y裁剪（保留一点pad）
    int minY = 0, maxY = surf->h - 1;
    if (findInkRowsRGBA32(surf, bg, minY, maxY)) {
        const int padY = 1;
        minY = std::max(0, minY - padY);
        maxY = std::min(surf->h - 1, maxY + padY);
    } else {
        minY = 0; maxY = surf->h - 1;
    }

    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
    if (!tex) { SDL_FreeSurface(surf); return; }

    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_NONE);

    SDL_Rect src{0, minY, surf->w, maxY - minY + 1};

    // toneMaxRows：只绘制前N行像素（从src顶部开始算）
    if (toneMaxRows > 0 && toneMaxRows < src.h) {
        src.h = toneMaxRows;
    }

    //  用实际贴片宽度居中（修复水平定位偏差）
    SDL_Rect dst{centerX - (src.w / 2), baseY + minY + toneYOffsetPx, src.w, src.h};

    // clip push/pop
    const bool hadClip = SDL_RenderIsClipEnabled(ren) == SDL_TRUE;
    SDL_Rect oldClip{};
    if (hadClip) SDL_RenderGetClipRect(ren, &oldClip);

    SDL_RenderSetClipRect(ren, &clip);
    SDL_RenderCopy(ren, tex, &src, &dst);

    if (hadClip) SDL_RenderSetClipRect(ren, &oldClip);
    else SDL_RenderSetClipRect(ren, nullptr);

    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

// 拼音绘制：字母一层 + 音调贴片一层（方案二）
// toneYOffsetPx：符号层Y微调
// toneMaxRows：符号层“只画前N行像素”（0=全部）
static void drawPinyinParts(SDL_Renderer* ren,
                            TTF_Font* font,
                            const SDL_Rect& clip,
                            int pyStartX,
                            int pyY,
                            const std::string& ini,
                            const std::string& fin,
                            int tone,
                            ToneDisplayMode toneMode,
                            bool toneMarkAvailable,
                            SDL_Color iniColor,
                            SDL_Color finColor,
                            SDL_Color toneColor,
                            SDL_Color bgColor,
                            int toneYOffsetPx,
                            int toneMaxRows) {
    const std::string iniDisp = displayReplaceVWithUmlaut(ini);

    auto measureW = [&](const std::string& s) -> int {
        int w = 0, h = 0;
        if (!s.empty()) TTF_SizeUTF8(font, s.c_str(), &w, &h);
        return w;
    };

    auto drawText = [&](const std::string& s, int x, int y, SDL_Color c) {
        if (s.empty()) return;
        SDL_Surface* surf = TTF_RenderUTF8_Blended(font, s.c_str(), c);
        if (!surf) return;
        SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
        if (!tex) { SDL_FreeSurface(surf); return; }
        SDL_Rect dst{x, y, surf->w, surf->h};
        SDL_RenderCopy(ren, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    };

    auto supportsDotlessI = [&]() -> bool {
        return font && TTF_GlyphIsProvided32(font, 0x0131) != 0; // ı
    };

    // -------------------- 符号模式：字母层 + 音标贴片层 --------------------
    if (toneMode == ToneDisplayMode::Mark && toneMarkAvailable) {
        int anchor = toneAnchorIndexInFinal(fin);
        if (anchor < 0) anchor = 0;
        if (anchor >= (int)fin.size()) anchor = (int)fin.size() - 1;

        const bool hasTone = (tone >= 1 && tone <= 4);
        const bool anchorIsI = (!fin.empty() && fin[(size_t)anchor] == 'i');
        const bool useDotlessI = hasTone && anchorIsI && supportsDotlessI();

        // 生成显示韵母（v->ü；必要时anchor i->ı）
        std::string finDisp;
        finDisp.reserve(fin.size() + 8);
        for (int i = 0; i < (int)fin.size(); ++i) {
            char ch = fin[(size_t)i];
            if (ch == 'v') finDisp += u8"ü";
            else if (useDotlessI && i == anchor && ch == 'i') finDisp += u8"ı";
            else finDisp.push_back(ch);
        }

        // 先画字母
        int x = pyStartX;
        if (!iniDisp.empty()) {
            drawText(iniDisp, x, pyY, iniColor);
            x += measureW(iniDisp);
        }
        if (!finDisp.empty()) {
            drawText(finDisp, x, pyY, finColor);
        }

        const char* symC = pickToneSymbolUTF8(font, tone);
        if (symC && symC[0] != '\0' && !fin.empty()) {
            std::string sym(symC);

            //  用“承载元音中心”定位：避免贴片宽度/裁剪导致的偏移
            std::string preDisp = displayReplaceVWithUmlaut(fin.substr(0, (size_t)anchor));
            std::string vowelDisp;
            {
                char v = fin[(size_t)anchor];
                if (useDotlessI && v == 'i') vowelDisp = u8"ı";
                else if (v == 'v') vowelDisp = u8"ü";
                else vowelDisp = std::string(1, v);
            }

            int wIni   = measureW(iniDisp);
            int wPre   = measureW(preDisp);
            int wVowel = measureW(vowelDisp);

            int centerX = pyStartX + wIni + wPre + (wVowel / 2);

            // baseY 选在拼音行上方一点（你原来的视觉逻辑）
            int baseY = pyY - 12;

            drawToneStickerCropped(ren, font, sym, clip,
                                   centerX, baseY,
                                   toneColor, bgColor,
                                   toneYOffsetPx,
                                   toneMaxRows);
        }

        return;
    }

    // -------------------- 数字模式：ini + fin + digit --------------------
    const std::string finDisp = displayReplaceVWithUmlaut(fin);
    std::string toneDigit = (tone >= 1 && tone <= 4) ? std::to_string(tone) : "";

    int x = pyStartX;
    if (!iniDisp.empty()) {
        drawText(iniDisp, x, pyY, iniColor);
        x += measureW(iniDisp);
    }
    if (!finDisp.empty()) {
        drawText(finDisp, x, pyY, finColor);
        x += measureW(finDisp);
    }
    if (!toneDigit.empty()) {
        drawText(toneDigit, x, pyY, toneColor);
    }
}

static bool fontSupportsCJK_(TTF_Font* font) {
    if (!font) return false;
    return TTF_GlyphIsProvided32(font, 0x6C49) != 0 &&
           TTF_GlyphIsProvided32(font, 0x5B57) != 0;
}

bool HGUI::initSDL_(const HGUIConfig& cfg, std::string& err,
                    SDL_Window* externalWindow,
                    SDL_Renderer* externalRenderer) {
    if ((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) == 0) {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            err = std::string("SDL_Init failed: ") + SDL_GetError();
            return false;
        }
        ownsSDL_ = true;
    }

    if (TTF_WasInit() == 0) {
        if (TTF_Init() != 0) {
            err = std::string("TTF_Init failed: ") + TTF_GetError();
            return false;
        }
        ownsTTF_ = true;
    }

    const int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    const int alreadyImgFlags = IMG_Init(0);
    if ((alreadyImgFlags & imgFlags) != imgFlags) {
        const int nowImgFlags = IMG_Init(imgFlags);
        if ((nowImgFlags & imgFlags) != 0) {
            ownsIMG_ = true;
        }
    }

    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

    if (externalWindow != nullptr && externalRenderer != nullptr) {
        win_ = externalWindow;
        ren_ = externalRenderer;
        ownsWindowRenderer_ = false;
        SDL_SetWindowTitle(win_, cfg.windowTitle.c_str());
        SDL_ShowWindow(win_);
        SDL_RaiseWindow(win_);
    } else {
        ownsWindowRenderer_ = true;
        win_ = SDL_CreateWindow(
            cfg.windowTitle.c_str(),
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            cfg.width,
            cfg.height,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
        );
        if (!win_) {
            err = std::string("SDL_CreateWindow failed: ") + SDL_GetError();
            return false;
        }

        ren_ = SDL_CreateRenderer(win_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!ren_) {
            err = std::string("SDL_CreateRenderer failed: ") + SDL_GetError();
            return false;
        }
    }

    toneMode_ = cfg.toneDisplay;

    std::string chosenPath;
    for (const auto& fp : cfg.fontCandidates) {
        if (!fileExists(fp)) continue;

        TTF_Font* f = TTF_OpenFont(fp.c_str(), cfg.fontSize);
        if (!f) continue;

        if (!fontSupportsCJK_(f)) {
            TTF_CloseFont(f);
            continue;
        }

        fontHanzi_ = f;
        chosenPath = fp;
        break;
    }

    if (!fontHanzi_) {
        err = "Failed to load a CJK-capable font. Please provide msyh/simhei/NotoCJK etc.";
        return false;
    }

    fontPinyin_ = TTF_OpenFont(chosenPath.c_str(), cfg.pinyinFontSize);
    if (!fontPinyin_) {
        fontPinyin_ = fontHanzi_;
    }

    fontSubmit_ = TTF_OpenFont(chosenPath.c_str(), 52);
    if (!fontSubmit_) {
        fontSubmit_ = fontHanzi_;
    }

    fontCellHanzi_ = TTF_OpenFont(chosenPath.c_str(), 52);
    if (!fontCellHanzi_) {
        fontCellHanzi_ = fontHanzi_;
    }

    loadDifficultySelectionResources_(cfg, chosenPath);

    // 关键：符号可用性判断
    toneMarkAvailable_ = toneSymbolsAvailable(fontPinyin_);
    if (toneMode_ == ToneDisplayMode::Mark && !toneMarkAvailable_) {
        toneMode_ = ToneDisplayMode::Number;
    }

    SDL_StartTextInput();

    rng_.seed((unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count());

    int w = 0, h = 0;
    SDL_GetWindowSize(win_, &w, &h);
    computeLayout_(w, h);

    return true;
}

void HGUI::shutdown_() {
    destroyDifficultySelectionResources_();

    if (fontCellHanzi_ && fontCellHanzi_ != fontHanzi_ && fontCellHanzi_ != fontPinyin_) {
        TTF_CloseFont(fontCellHanzi_);
    }
    fontCellHanzi_ = nullptr;

    if (fontSubmit_ && fontSubmit_ != fontHanzi_ && fontSubmit_ != fontPinyin_) {
        TTF_CloseFont(fontSubmit_);
    }
    fontSubmit_ = nullptr;

    if (fontPinyin_ && fontPinyin_ != fontHanzi_) {
        TTF_CloseFont(fontPinyin_);
    }
    fontPinyin_ = nullptr;

    if (fontHanzi_) {
        TTF_CloseFont(fontHanzi_);
        fontHanzi_ = nullptr;
    }

    if (ownsWindowRenderer_) {
        if (ren_)  { SDL_DestroyRenderer(ren_); }
        if (win_)  { SDL_DestroyWindow(win_); }
    }
    ren_ = nullptr;
    win_ = nullptr;
    ownsWindowRenderer_ = true;

    if (ownsIMG_) { IMG_Quit(); ownsIMG_ = false; }
    if (ownsTTF_) { TTF_Quit(); ownsTTF_ = false; }
    if (ownsSDL_) { SDL_Quit(); ownsSDL_ = false; }
}

int HGUI::run(HGCore& core,
              const std::vector<std::string>& allInitials,
              const std::vector<std::string>& allFinals,
              const HGUIConfig& cfg) {
    return run(nullptr, nullptr, core, allInitials, allFinals, cfg);
}

int HGUI::run(SDL_Window* externalWindow,
              SDL_Renderer* externalRenderer,
              HGCore& core,
              const std::vector<std::string>& allInitials,
              const std::vector<std::string>& allFinals,
              const HGUIConfig& cfg) {
    std::string err;
    if (!initSDL_(cfg, err, externalWindow, externalRenderer)) {
        SDL_Log("HandleGame GUI init failed: %s", err.c_str());
        return -1;
    }

    screen_ = Screen::DifficultySelect;
    input_.clear();
    composing_.clear();
    guessScroll_ = 0;
    chartScroll_ = 0;
    difficultySelectedIndex_ = 0;
    difficultyHoveredIndex_ = -1;
    difficultyBackHovered_ = false;
    resetPlayingHover_();

    hint_ = HintModalState{};
    hint_.open = false;

    int exitCode = 999;

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                exitCode = 0;
                running = false;
                break;
            }
            handleEvent_(e, core, exitCode);
            if (exitCode != 999) {
                running = false;
                break;
            }
        }
        render_(core, allInitials, allFinals, cfg);
    }

    if (exitCode == 999) exitCode = 1;
    return exitCode;
}

void HGUI::computeLayout_(int w, int h) {
    const int pad = 16;
    const int bottomH = 78;
    const int rightW = 440;

    msgBar_ = SDL_Rect{pad, pad, w - pad * 2, 84};

    panelLeft_ = SDL_Rect{
        pad,
        msgBar_.y + msgBar_.h + pad,
        w - rightW - pad * 3,
        h - bottomH - pad * 3 - msgBar_.h
    };
    panelRight_ = SDL_Rect{
        panelLeft_.x + panelLeft_.w + pad,
        panelLeft_.y,
        rightW,
        panelLeft_.h
    };

    inputBox_ = SDL_Rect{pad, h - bottomH + 10, w - pad * 3 - 160, bottomH - 20};
    btnSubmit_ = SDL_Rect{inputBox_.x + inputBox_.w + pad, inputBox_.y, 160, inputBox_.h};

    int submitW = 0;
    int submitH = 0;
    if (fontSubmit_) {
        TTF_SizeUTF8(fontSubmit_, u8"提交", &submitW, &submitH);
    }
    submitTextRect_ = SDL_Rect{
        btnSubmit_.x + (btnSubmit_.w - submitW) / 2,
        btnSubmit_.y + (btnSubmit_.h - submitH) / 2,
        submitW,
        submitH
    };
    submitHitRect_ = SDL_Rect{
        submitTextRect_.x - 26,
        submitTextRect_.y - 14,
        submitTextRect_.w + 52,
        submitTextRect_.h + 28
    };
    submitHighlightRect_ = centeredRectAround(submitTextRect_, kSubmitHighlightWidth, kSubmitHighlightHeight);

    const double sx = w > 0 ? static_cast<double>(w) / static_cast<double>(kDesignWidth) : 1.0;
    const double sy = h > 0 ? static_cast<double>(h) / static_cast<double>(kDesignHeight) : 1.0;
    const double scale = std::max(0.5, std::min(sx, sy));

    const int iconSize = static_cast<int>(std::lround(kPlayingTopIconSize * scale));
    const int iconGap = static_cast<int>(std::lround(kPlayingTopIconGap * scale));
    const int iconMarginRight = static_cast<int>(std::lround(kPlayingTopIconMarginRight * sx));
    const int iconY = msgBar_.y + (msgBar_.h - iconSize) / 2;

    // 视觉顺序：tips、返回难度、退出；最右侧为退出主菜单。
    gameQuitIconRect_ = SDL_Rect{w - iconMarginRight - iconSize, iconY, iconSize, iconSize};
    gameBackDifficultyIconRect_ = SDL_Rect{gameQuitIconRect_.x - iconGap - iconSize, iconY, iconSize, iconSize};
    gameTipsIconRect_ = SDL_Rect{gameBackDifficultyIconRect_.x - iconGap - iconSize, iconY, iconSize, iconSize};

    const int hoverW = static_cast<int>(std::lround(kPlayingTopIconHighlightWidth * scale));
    const int hoverH = static_cast<int>(std::lround(kPlayingTopIconHighlightHeight * scale));
    gameQuitHighlightRect_ = centeredRectAround(gameQuitIconRect_, hoverW, hoverH);
    gameBackDifficultyHighlightRect_ = centeredRectAround(gameBackDifficultyIconRect_, hoverW, hoverH);
    gameTipsHighlightRect_ = centeredRectAround(gameTipsIconRect_, hoverW, hoverH);

    // 保留旧变量名称给事件代码兼容使用：quit=回主菜单，back=回难度，hint=提示。
    btnBackMenu_ = gameQuitIconRect_;
    btnExitGame_ = gameBackDifficultyIconRect_;
    btnHint_ = gameTipsIconRect_;

    // 旧的滑块不再绘制，但保留 Tab 键切换逻辑。
    toneToggle_ = SDL_Rect{};

    updateDifficultyLayout_(w, h);

    // 提示弹窗布局
    hintBox_ = SDL_Rect{ (w - 560) / 2, (h - 380) / 2, 560, 380 };
    hintClose_ = SDL_Rect{
        hintBox_.x + hintBox_.w - kHintCloseIconSize - 18,
        hintBox_.y + 18,
        kHintCloseIconSize,
        kHintCloseIconSize
    };
    hintCloseHighlightRect_ = centeredRectAround(hintClose_, 112, 76);

    int moreW = 0;
    int moreH = 0;
    if (fontHanzi_) {
        TTF_SizeUTF8(fontHanzi_, u8"进一步提示", &moreW, &moreH);
    }
    hintMore_ = SDL_Rect{
        hintBox_.x + (hintBox_.w - moreW) / 2,
        hintBox_.y + hintBox_.h - 66,
        moreW,
        moreH
    };
    hintMoreHighlightRect_ = centeredRectAround(hintMore_, 260, 82);
}

void HGUI::destroyDifficultySelectionResources_() {
    if (difficultySelectionBgTexture_) {
        SDL_DestroyTexture(difficultySelectionBgTexture_);
        difficultySelectionBgTexture_ = nullptr;
    }
    if (difficultySelectedBgTexture_) {
        SDL_DestroyTexture(difficultySelectedBgTexture_);
        difficultySelectedBgTexture_ = nullptr;
    }
    if (difficultyBackIconTexture_) {
        SDL_DestroyTexture(difficultyBackIconTexture_);
        difficultyBackIconTexture_ = nullptr;
    }
    if (gameQuitIconTexture_) {
        SDL_DestroyTexture(gameQuitIconTexture_);
        gameQuitIconTexture_ = nullptr;
    }
    if (gameBackDifficultyIconTexture_) {
        SDL_DestroyTexture(gameBackDifficultyIconTexture_);
        gameBackDifficultyIconTexture_ = nullptr;
    }
    if (gameTipsIconTexture_) {
        SDL_DestroyTexture(gameTipsIconTexture_);
        gameTipsIconTexture_ = nullptr;
    }
    if (hintCloseIconTexture_) {
        SDL_DestroyTexture(hintCloseIconTexture_);
        hintCloseIconTexture_ = nullptr;
    }

    auto closeOwnedFont = [&](TTF_Font*& font) {
        if (font && font != fontHanzi_ && font != fontPinyin_) {
            TTF_CloseFont(font);
        }
        font = nullptr;
    };

    closeOwnedFont(fontDifficultyButton_);
    closeOwnedFont(fontDifficultyCaption_);
}

void HGUI::loadDifficultySelectionResources_(const HGUIConfig& cfg, const std::string& chosenFontPath) {
    destroyDifficultySelectionResources_();

    fontDifficultyCaption_ = fontHanzi_;
    fontDifficultyButton_ = nullptr;

    if (!chosenFontPath.empty() && fileExists(chosenFontPath)) {
        fontDifficultyButton_ = TTF_OpenFont(chosenFontPath.c_str(), 80);
    }
    if (!fontDifficultyButton_) {
        fontDifficultyButton_ = fontHanzi_;
    }

    auto loadHandleIcon = [&](SDL_Texture*& dst, const std::string& filename) {
        const auto iconPath = findHandleGameImagePath(cfg, chosenFontPath, filename);
        if (iconPath.empty()) return;
        dst = IMG_LoadTexture(ren_, iconPath.string().c_str());
        if (dst) {
            SDL_SetTextureBlendMode(dst, SDL_BLENDMODE_BLEND);
        }
    };

    loadHandleIcon(difficultyBackIconTexture_, "go-back-arrow.png");
    loadHandleIcon(gameQuitIconTexture_, "quit.png");
    loadHandleIcon(gameBackDifficultyIconTexture_, "go-back-arrow-gray.png");
    loadHandleIcon(gameTipsIconTexture_, "tips.png");
    loadHandleIcon(hintCloseIconTexture_, "close.png");

    const auto selectedBgPath = findHandleGameImagePath(cfg, chosenFontPath, "bg2.png");
    if (!selectedBgPath.empty()) {
        difficultySelectedBgTexture_ = IMG_LoadTexture(ren_, selectedBgPath.string().c_str());
        if (difficultySelectedBgTexture_) {
            SDL_SetTextureBlendMode(difficultySelectedBgTexture_, SDL_BLENDMODE_BLEND);
        }
    }

    const auto selectionBgPath = findHandleGameImagePath(cfg, chosenFontPath, "bg3.jpg");
    if (!selectionBgPath.empty()) {
        difficultySelectionBgTexture_ = IMG_LoadTexture(ren_, selectionBgPath.string().c_str());
    }

    const auto homepageRoot = findHomepageRoot(cfg, chosenFontPath);
    if (homepageRoot.empty()) {
        return;
    }

    const auto selectionFontPath = homepageRoot / "font" / "font2.ttf";

    TTF_Font* captionFont = TTF_OpenFont(selectionFontPath.string().c_str(), 40);
    if (captionFont) {
        if (fontDifficultyCaption_ && fontDifficultyCaption_ != fontHanzi_ && fontDifficultyCaption_ != fontPinyin_) {
            TTF_CloseFont(fontDifficultyCaption_);
        }
        fontDifficultyCaption_ = captionFont;
    }

    TTF_Font* buttonFont = TTF_OpenFont(selectionFontPath.string().c_str(), 80);
    if (buttonFont) {
        if (fontDifficultyButton_ && fontDifficultyButton_ != fontHanzi_ && fontDifficultyButton_ != fontPinyin_) {
            TTF_CloseFont(fontDifficultyButton_);
        }
        fontDifficultyButton_ = buttonFont;
    }

    if (!difficultySelectedBgTexture_) {
        const auto fallbackSelectedBgPath = homepageRoot / "image" / "bg2.png";
        difficultySelectedBgTexture_ = IMG_LoadTexture(ren_, fallbackSelectedBgPath.string().c_str());
        if (difficultySelectedBgTexture_) {
            SDL_SetTextureBlendMode(difficultySelectedBgTexture_, SDL_BLENDMODE_BLEND);
        }
    }

    if (!difficultySelectionBgTexture_) {
        const auto fallbackSelectionBgPath = homepageRoot / "image" / "bg3.jpg";
        difficultySelectionBgTexture_ = IMG_LoadTexture(ren_, fallbackSelectionBgPath.string().c_str());
    }
}

void HGUI::updateDifficultyLayout_(int w, int h) {
    int bgTexW = 0;
    int bgTexH = 0;
    if (difficultySelectionBgTexture_) {
        SDL_QueryTexture(difficultySelectionBgTexture_, nullptr, nullptr, &bgTexW, &bgTexH);
    }
    difficultySelectionBgRect_ = buildDifficultyBgRect(bgTexW, bgTexH, w, h);

    const double sx = w > 0 ? static_cast<double>(w) / static_cast<double>(kDesignWidth) : 1.0;
    const double sy = h > 0 ? static_cast<double>(h) / static_cast<double>(kDesignHeight) : 1.0;
    const double scale = std::max(0.5, std::min(sx, sy));

    const int backIconSize = static_cast<int>(std::lround(kDifficultyBackIconSize * scale));
    const int backMarginTop = static_cast<int>(std::lround(kDifficultyBackMarginTop * sy));
    const int backMarginRight = static_cast<int>(std::lround(kDifficultyBackMarginRight * sx));
    const int backHighlightW = static_cast<int>(std::lround(kDifficultyBackHighlightWidth * scale));
    const int backHighlightH = static_cast<int>(std::lround(kDifficultyBackHighlightHeight * scale));

    difficultyBackIconRect_ = SDL_Rect{
        w - backMarginRight - backIconSize,
        backMarginTop,
        backIconSize,
        backIconSize
    };
    difficultyBackHitRect_ = SDL_Rect{
        difficultyBackIconRect_.x - std::max(8, backIconSize / 6),
        difficultyBackIconRect_.y - std::max(8, backIconSize / 6),
        difficultyBackIconRect_.w + std::max(16, backIconSize / 3),
        difficultyBackIconRect_.h + std::max(16, backIconSize / 3)
    };
    difficultyBackHighlightRect_ = SDL_Rect{
        difficultyBackIconRect_.x + difficultyBackIconRect_.w / 2 - backHighlightW / 2,
        difficultyBackIconRect_.y + difficultyBackIconRect_.h / 2 - backHighlightH / 2,
        backHighlightW,
        backHighlightH
    };

    const int centerX = w / 2;
    int currentY = static_cast<int>(std::lround(kDifficultyButtonStartY * sy));
    const int gap = static_cast<int>(std::lround(kDifficultyButtonGap * scale));
    const int selectedW = static_cast<int>(std::lround(kDifficultySelectedBgWidth * scale));
    const int selectedH = static_cast<int>(std::lround(kDifficultySelectedBgHeight * scale));

    TTF_Font* buttonFont = fontDifficultyButton_ ? fontDifficultyButton_ : fontHanzi_;
    for (std::size_t i = 0; i < kDifficultyLabels.size(); ++i) {
        int textW = 0;
        int textH = 0;
        if (buttonFont) {
            TTF_SizeUTF8(buttonFont, kDifficultyLabels[i].c_str(), &textW, &textH);
        }

        difficultyTextRects_[i] = SDL_Rect{
            centerX - textW / 2,
            currentY,
            textW,
            textH
        };
        difficultyHitRects_[i] = SDL_Rect{
            centerX - selectedW / 2,
            currentY + (textH - selectedH) / 2,
            selectedW,
            selectedH
        };

        currentY += textH + gap;
    }

    btnEasy_ = difficultyHitRects_[0];
    btnNormal_ = difficultyHitRects_[1];
    btnHard_ = difficultyHitRects_[2];

    if (difficultySelectedIndex_ < 0 || difficultySelectedIndex_ >= static_cast<int>(kDifficultyLabels.size())) {
        difficultySelectedIndex_ = 0;
    }
}

bool HGUI::pointInRect_(int x, int y, const SDL_Rect& r) {
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

void HGUI::popUtf8Last_(std::string& s) {
    if (s.empty()) return;
    size_t i = s.size() - 1;
    while (i > 0 && ((unsigned char)s[i] & 0xC0) == 0x80) i--;
    s.erase(i);
}

HGUI::RGB HGUI::bgForCell_() { return RGB{0xff, 0xff, 0xff, 0xff}; }

HGUI::RGB HGUI::fontForMark_(Mark m) {
    if (m == Mark::Green)  return RGB{0x22, 0xc5, 0x5e, 0xff};
    if (m == Mark::Yellow) return RGB{0xfa, 0xcc, 0x15, 0xff};
    return RGB{0xef, 0x44, 0x44, 0xff};
}

HGUI::RGB HGUI::fontForChartState_(int st) {
    if (st == 2) return RGB{0x22, 0xc5, 0x5e, 0xff};
    if (st == 1) return RGB{0xfa, 0xcc, 0x15, 0xff};
    if (st == 0) return RGB{0xef, 0x44, 0x44, 0xff};
    return RGB{0x14, 0x14, 0x14, 0xff};
}

void HGUI::clear_(RGB c) {
    SDL_SetRenderDrawColor(ren_, c.r, c.g, c.b, c.a);
    SDL_RenderClear(ren_);
}

void HGUI::fillRect_(const SDL_Rect& r, RGB c) {
    SDL_SetRenderDrawColor(ren_, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(ren_, &r);
}

void HGUI::drawRect_(const SDL_Rect& r, RGB c) {
    SDL_SetRenderDrawColor(ren_, c.r, c.g, c.b, c.a);
    SDL_RenderDrawRect(ren_, &r);
}

int HGUI::measureTextW_(TTF_Font* f, const std::string& s) const {
    if (!f || s.empty()) return 0;
    int w = 0, h = 0;
    if (TTF_SizeUTF8(f, s.c_str(), &w, &h) != 0) return 0;
    return w;
}

void HGUI::drawText_(TTF_Font* f, const std::string& s, int x, int y, RGB c) {
    if (!f || s.empty()) return;
    SDL_Color col{c.r, c.g, c.b, c.a};
    SDL_Surface* surf = TTF_RenderUTF8_Blended(f, s.c_str(), col);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren_, surf);
    if (!tex) { SDL_FreeSurface(surf); return; }
    SDL_Rect dst{x, y, surf->w, surf->h};
    SDL_RenderCopy(ren_, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

void HGUI::drawTextCentered_(TTF_Font* f, const std::string& s, const SDL_Rect& r, RGB c) {
    if (!f || s.empty()) return;
    SDL_Color col{c.r, c.g, c.b, c.a};
    SDL_Surface* surf = TTF_RenderUTF8_Blended(f, s.c_str(), col);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren_, surf);
    if (!tex) { SDL_FreeSurface(surf); return; }
    SDL_Rect dst{r.x + (r.w - surf->w) / 2, r.y + (r.h - surf->h) / 2, surf->w, surf->h};
    SDL_RenderCopy(ren_, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

void HGUI::toggleToneMode_() {
    if (toneMode_ == ToneDisplayMode::Number) {
        if (toneMarkAvailable_) toneMode_ = ToneDisplayMode::Mark;
    } else {
        toneMode_ = ToneDisplayMode::Number;
    }
}

void HGUI::drawTopBar_(const CoreSnapshot& snap, const HGUIConfig& cfg) {
    // 与选择难度界面保持统一：顶部区域不再使用米色底。
    fillRect_(msgBar_, RGB{0xff, 0xff, 0xff, 0xff});

    TTF_Font* topFont = fontDifficultyCaption_ ? fontDifficultyCaption_ : fontHanzi_;
    const int baseX = msgBar_.x + 12;
    const int baseY = msgBar_.y + 18;
    const int gapSmall = 28;
    const int gapLarge = 42;

    const std::string gameName = u8"汉兜";
    const std::string modeText = difficultyModeName(snap.difficulty);
    const std::string numberTone = u8"数字音调";
    const std::string markTone = u8"符号音调";

    int gameW = measureTextW_(topFont, gameName);
    int modeW = measureTextW_(topFont, modeText);
    int numberW = measureTextW_(topFont, numberTone);
    int markW = measureTextW_(topFont, markTone);
    int topH = 0;
    if (topFont) {
        int tmpW = 0;
        TTF_SizeUTF8(topFont, gameName.c_str(), &tmpW, &topH);
    }

    int x = baseX;
    const int y = baseY;

    SDL_Rect gameTextRect{x, y, gameW, topH};
    x += gameW + gapLarge;
    SDL_Rect modeTextRect{x, y, modeW, topH};
    x += modeW + gapLarge;
    toneNumberTextRect_ = SDL_Rect{x, y, numberW, topH};
    x += numberW + gapSmall;
    toneMarkTextRect_ = SDL_Rect{x, y, markW, topH};

    toneNumberHighlightRect_ = centeredRectAround(toneNumberTextRect_, numberW + 92, 78);
    toneMarkHighlightRect_ = centeredRectAround(toneMarkTextRect_, markW + 92, 78);

    // 音调模式高亮：同一时刻只绘制一个 bg2.png。
    // 鼠标悬浮在某个音调文字上时，高亮跟随鼠标，但不自动切换模式；
    // 鼠标不在两种音调文字上时，高亮当前正在使用的音调模式。
    if (toneNumberHovered_) {
        drawSelectedBg_(toneNumberHighlightRect_);
    } else if (toneMarkHovered_) {
        drawSelectedBg_(toneMarkHighlightRect_);
    } else if (toneMode_ == ToneDisplayMode::Number) {
        drawSelectedBg_(toneNumberHighlightRect_);
    } else {
        drawSelectedBg_(toneMarkHighlightRect_);
    }

    drawText_(topFont, gameName, gameTextRect.x, gameTextRect.y, RGB{0x00, 0x00, 0x00, 0xff});
    drawText_(topFont, modeText, modeTextRect.x, modeTextRect.y, RGB{0x00, 0x00, 0x00, 0xff});
    drawText_(topFont, numberTone, toneNumberTextRect_.x, toneNumberTextRect_.y, RGB{0x00, 0x00, 0x00, 0xff});
    drawText_(topFont,
              markTone,
              toneMarkTextRect_.x,
              toneMarkTextRect_.y,
              toneMarkAvailable_ ? RGB{0x00, 0x00, 0x00, 0xff} : RGB{0x94, 0x94, 0x94, 0xff});

    if (playingTipsHovered_) {
        drawSelectedBg_(gameTipsHighlightRect_);
    } else if (playingBackDifficultyHovered_) {
        drawSelectedBg_(gameBackDifficultyHighlightRect_);
    } else if (playingQuitHovered_) {
        drawSelectedBg_(gameQuitHighlightRect_);
    }

    drawTextureOrText_(gameTipsIconTexture_, gameTipsIconRect_, u8"?");
    drawTextureOrText_(gameBackDifficultyIconTexture_, gameBackDifficultyIconRect_, u8"←");
    drawTextureOrText_(gameQuitIconTexture_, gameQuitIconRect_, u8"×");

    if (!snap.message.empty()) {
        drawText_(fontPinyin_, snap.message, msgBar_.x + 12, msgBar_.y + 62, RGB{0x14, 0x14, 0x14, 0xff});
    }

    (void)cfg;
}

void HGUI::drawDifficultyScreen_() {
    if (difficultySelectionBgTexture_) {
        SDL_RenderCopy(ren_, difficultySelectionBgTexture_, nullptr, &difficultySelectionBgRect_);
    }

    // bg2.png 只作为“鼠标悬浮高亮”绘制：同一帧最多绘制一次。
    // 鼠标不在返回箭头或三个难度选项上时，不绘制 bg2.png。
    if (difficultyBackHovered_) {
        if (difficultySelectedBgTexture_) {
            SDL_SetTextureAlphaMod(difficultySelectedBgTexture_, 255);
            SDL_RenderCopy(ren_, difficultySelectedBgTexture_, nullptr, &difficultyBackHighlightRect_);
        } else {
            drawDifficultySelectedFallback(ren_, difficultyBackHighlightRect_);
        }
    } else if (difficultyHoveredIndex_ >= 0 &&
               difficultyHoveredIndex_ < static_cast<int>(difficultyHitRects_.size())) {
        const SDL_Rect& hoveredRect = difficultyHitRects_[static_cast<std::size_t>(difficultyHoveredIndex_)];
        if (difficultySelectedBgTexture_) {
            SDL_SetTextureAlphaMod(difficultySelectedBgTexture_, 255);
            SDL_RenderCopy(ren_, difficultySelectedBgTexture_, nullptr, &hoveredRect);
        } else {
            drawDifficultySelectedFallback(ren_, hoveredRect);
        }
    }

    if (difficultyBackIconTexture_) {
        SDL_RenderCopy(ren_, difficultyBackIconTexture_, nullptr, &difficultyBackIconRect_);
    } else {
        drawTextCentered_(fontDifficultyCaption_ ? fontDifficultyCaption_ : fontHanzi_,
                          u8"←",
                          difficultyBackIconRect_,
                          RGB{0x00, 0x00, 0x00, 0xff});
    }

    TTF_Font* captionFont = fontDifficultyCaption_ ? fontDifficultyCaption_ : fontHanzi_;
    int captionW = 0;
    int captionH = 0;
    if (captionFont) {
        TTF_SizeUTF8(captionFont, u8"请选择难度", &captionW, &captionH);
    }

    int windowW = 0;
    int windowH = 0;
    SDL_GetWindowSize(win_, &windowW, &windowH);
    const double sy = windowH > 0
        ? static_cast<double>(windowH) / static_cast<double>(kDesignHeight)
        : 1.0;
    const int captionY = static_cast<int>(std::lround(kDifficultyCaptionTop * sy));
    drawText_(captionFont,
              u8"请选择难度",
              windowW / 2 - captionW / 2,
              captionY,
              RGB{0x00, 0x00, 0x00, 0xff});

    TTF_Font* buttonFont = fontDifficultyButton_ ? fontDifficultyButton_ : fontHanzi_;
    for (std::size_t i = 0; i < kDifficultyLabels.size(); ++i) {
        drawText_(buttonFont,
                  kDifficultyLabels[i],
                  difficultyTextRects_[i].x,
                  difficultyTextRects_[i].y,
                  RGB{0x00, 0x00, 0x00, 0xff});
    }
}

void HGUI::drawSelectedBg_(const SDL_Rect& r) {
    if (difficultySelectedBgTexture_) {
        SDL_SetTextureAlphaMod(difficultySelectedBgTexture_, 255);
        SDL_RenderCopy(ren_, difficultySelectedBgTexture_, nullptr, &r);
    } else {
        drawDifficultySelectedFallback(ren_, r);
    }
}

void HGUI::drawTextureOrText_(SDL_Texture* texture, const SDL_Rect& r, const std::string& fallbackText) {
    if (texture) {
        SDL_RenderCopy(ren_, texture, nullptr, &r);
        return;
    }
    drawTextCentered_(fontHanzi_, fallbackText, r, RGB{0x14, 0x14, 0x14, 0xff});
}


void HGUI::drawPlayingScreen_(HGCore& core,
                              const std::vector<std::string>& allInitials,
                              const std::vector<std::string>& allFinals) {
    const CoreSnapshot& snap = core.snapshot();

    // 顶部的退出 / 返回难度 / 提示已经改为图片按钮，绘制在 drawTopBar_ 中。

    // 输入框
    fillRect_(inputBox_, RGB{0xff, 0xff, 0xff, 0xff});
    drawRect_(inputBox_, RGB{0x37, 0x41, 0x51, 0xff});
    drawText_(fontHanzi_, "输入：", inputBox_.x + 10, inputBox_.y + 10, RGB{0x14, 0x14, 0x14, 0xff});
    drawText_(fontHanzi_, input_, inputBox_.x + 70, inputBox_.y + 10, RGB{0x14, 0x14, 0x14, 0xff});
    if (!composing_.empty()) {
        int x = inputBox_.x + 70 + measureTextW_(fontHanzi_, input_);
        drawText_(fontHanzi_, composing_, x, inputBox_.y + 10, RGB{0x14, 0x14, 0x14, 0xff});
    }
    if (submitHovered_) {
        drawSelectedBg_(submitHighlightRect_);
    }
    drawText_(fontSubmit_, u8"提交", submitTextRect_.x, submitTextRect_.y, RGB{0x22, 0xc5, 0x5e, 0xff});

    // 左侧：历史 + 当前输入行
    drawText_(fontHanzi_, "历史尝试",
              panelLeft_.x + 16, panelLeft_.y + 14, RGB{0x14, 0x14, 0x14, 0xff});

    const int cell = 86;
    const int gap = 10;
    int x0 = panelLeft_.x + 16;
    int y0 = panelLeft_.y + 48;

    int maxRowsVis = (panelLeft_.h - 64) / (cell + gap);
    if (maxRowsVis < 1) maxRowsVis = 1;

    int historySlots = std::max(0, maxRowsVis - 1);
    int totalHistory = (int)snap.history.size();

    int startHistory = std::max(0, totalHistory - historySlots);
    startHistory = std::max(0, startHistory - guessScroll_);
    int endHistory = std::min(totalHistory, startHistory + historySlots);
    int visibleHistoryCount = endHistory - startHistory;

    SDL_Color bgCell{0xff, 0xff, 0xff, 0xff};

    for (int i = 0; i < visibleHistoryCount; i++) {
        const auto& row = snap.history[startHistory + i];

        for (int c = 0; c < 4; c++) {
            SDL_Rect cellR{x0 + c * (cell + gap), y0 + i * (cell + gap), cell, cell};
            fillRect_(cellR, bgForCell_());
            drawRect_(cellR, RGB{0x37, 0x41, 0x51, 0xff});

            const std::string ini = row.initials[c];
            const std::string fin = row.finals[c];

            SDL_Color iniCol{fontForMark_(row.mIni[c]).r, fontForMark_(row.mIni[c]).g, fontForMark_(row.mIni[c]).b, 0xff};
            SDL_Color finCol{fontForMark_(row.mFin[c]).r, fontForMark_(row.mFin[c]).g, fontForMark_(row.mFin[c]).b, 0xff};
            SDL_Color toneCol{fontForMark_(row.mTone[c]).r, fontForMark_(row.mTone[c]).g, fontForMark_(row.mTone[c]).b, 0xff};

            const std::string iniDisp = displayReplaceVWithUmlaut(ini);
            const std::string finDisp = displayReplaceVWithUmlaut(fin);
            const std::string toneDigit = (row.tones[c] >= 1 && row.tones[c] <= 4) ? std::to_string(row.tones[c]) : "";

            int wIni = measureTextW_(fontPinyin_, iniDisp);
            int wFin = measureTextW_(fontPinyin_, finDisp);
            int wTone = (toneMode_ == ToneDisplayMode::Number) ? measureTextW_(fontPinyin_, toneDigit) : 0;
            int totalW = wIni + wFin + wTone;

            int pyStartX = cellR.x + (cellR.w - totalW) / 2;
            int pyY = cellR.y + 0;

            drawPinyinParts(ren_, fontPinyin_, cellR, pyStartX+5, pyY,
                           ini, fin, row.tones[c],
                           toneMode_, toneMarkAvailable_,
                           iniCol, finCol, toneCol,
                           bgCell,
                           /*toneYOffsetPx*/ 10,
                           /*toneMaxRows*/ 6);

            // 汉字行
            SDL_Rect hzR = cellR;
            hzR.y += 22;
            hzR.h -= 22;
            drawTextCentered_(fontCellHanzi_, row.chars[c], hzR, fontForMark_(row.mChar[c]));
        }
    }

    // 当前输入行
    if (!snap.finished) {
        int activeRowY = y0 + visibleHistoryCount * (cell + gap);
        std::string preview = input_ + composing_;
        auto previewChars = utf8CharsAtMost(preview, 4);

        for (int c = 0; c < 4; c++) {
            SDL_Rect cellR{x0 + c * (cell + gap), activeRowY, cell, cell};
            fillRect_(cellR, bgForCell_());
            drawRect_(cellR, RGB{0x37, 0x41, 0x51, 0xff});

            if (c < (int)previewChars.size()) {
                SDL_Rect hzR = cellR;
                hzR.y += 22;
                hzR.h -= 22;
                drawTextCentered_(fontHanzi_, previewChars[c], hzR, RGB{0x14, 0x14, 0x14, 0xff});
            }
        }
    }

    // 右侧：速查表
    int cx = panelRight_.x + 16;
    int cy = panelRight_.y + 14 - chartScroll_;

    drawText_(fontHanzi_, "拼音速查表", cx, cy, RGB{0x14, 0x14, 0x14, 0xff});
    cy += 28;
    drawText_(fontPinyin_, "红=不在答案 黄=在答案 绿=位置正确", cx, cy, RGB{0x14, 0x14, 0x14, 0xff});
    cy += 30;

    int tokenW = 58, tokenH = 36, tokenGap = 8;
    int cols = (panelRight_.w - 32) / (tokenW + tokenGap);
    if (cols < 4) cols = 4;

    drawText_(fontHanzi_, "声母", cx, cy, RGB{0x14, 0x14, 0x14, 0xff});
    cy += 32;

    int idx = 0;
    for (const auto& tok : allInitials) {
        int rr = idx / cols, cc = idx % cols;
        SDL_Rect rct{cx + cc * (tokenW + tokenGap), cy + rr * (tokenH + tokenGap), tokenW, tokenH};
        drawRect_(rct, RGB{0x37, 0x41, 0x51, 0xff});

        int stv = -1;
        auto it = snap.chart.iniState.find(tok);
        if (it != snap.chart.iniState.end()) stv = it->second;

        drawTextCentered_(fontHanzi_, tok, rct, fontForChartState_(stv));
        idx++;
    }

    int iniRows = (idx + cols - 1) / cols;
    cy += iniRows * (tokenH + tokenGap) + 18;

    drawText_(fontHanzi_, "韵母", cx, cy, RGB{0x14, 0x14, 0x14, 0xff});
    cy += 32;

    idx = 0;
    for (const auto& tok : allFinals) {
        int rr = idx / cols, cc = idx % cols;
        SDL_Rect rct{cx + cc * (tokenW + tokenGap), cy + rr * (tokenH + tokenGap), tokenW, tokenH};
        drawRect_(rct, RGB{0x37, 0x41, 0x51, 0xff});

        int stv = -1;
        auto it = snap.chart.finState.find(tok);
        if (it != snap.chart.finState.end()) stv = it->second;

        drawTextCentered_(fontHanzi_, tok, rct, fontForChartState_(stv));
        idx++;
    }

    if (hint_.open) {
        drawHintModal_(core);
    }
}

void HGUI::openHintModal_(HGCore& core) {
    if (!hint_.open) hint_.open = true;

    bool hasOrder = false;
    for (int i = 0; i < 4; ++i) if (hint_.order[(size_t)i] != -1) hasOrder = true;

    if (!hasOrder) {
        std::array<int, 4> idx{0, 1, 2, 3};
        std::shuffle(idx.begin(), idx.end(), rng_);
        for (int i = 0; i < 4; ++i) hint_.order[(size_t)i] = idx[(size_t)i];
    }

    auto diff = core.snapshot().difficulty;
    if (diff == Difficulty::Easy) hint_.maxSteps = 4;
    else if (diff == Difficulty::Normal) hint_.maxSteps = 8;
    else hint_.maxSteps = 4;

    if (hint_.step == 0) {
        advanceHint_(core);
    }
}

void HGUI::advanceHint_(HGCore& core) {
    if (hint_.step >= hint_.maxSteps) return;

    auto diff = core.snapshot().difficulty;

    if (diff == Difficulty::Easy) {
        int k = hint_.step;
        if (k < 4) {
            int idx = hint_.order[(size_t)k];
            hint_.showChar[(size_t)idx] = true;
            hint_.showPinyin[(size_t)idx] = true;
        }
        hint_.step++;
        return;
    }

    if (diff == Difficulty::Normal) {
        int k = hint_.step;
        int pos = hint_.order[(size_t)(k / 2)];
        bool showChar = (k % 2 == 1);

        hint_.showPinyin[(size_t)pos] = true;
        if (showChar) hint_.showChar[(size_t)pos] = true;

        hint_.step++;
        return;
    }

    {
        int k = hint_.step;
        int idx = hint_.order[(size_t)std::min(k, 3)];
        hint_.showPinyin[(size_t)idx] = true;
        hint_.step++;
    }
}

void HGUI::drawHintModal_(HGCore& core) {
    SDL_SetRenderDrawBlendMode(ren_, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren_, 0, 0, 0, 120);
    SDL_Rect full{0, 0, 0, 0};
    SDL_GetWindowSize(win_, &full.w, &full.h);
    SDL_RenderFillRect(ren_, &full);
    SDL_SetRenderDrawBlendMode(ren_, SDL_BLENDMODE_NONE);

    fillRect_(hintBox_, RGB{0xff, 0xff, 0xff, 0xff});
    drawRect_(hintBox_, RGB{0x37, 0x41, 0x51, 0xff});

    if (hintCloseHovered_) {
        drawSelectedBg_(hintCloseHighlightRect_);
    }
    drawTextureOrText_(hintCloseIconTexture_, hintClose_, u8"×");

    drawText_(fontSubmit_, u8"正确答案", hintBox_.x + 175, hintBox_.y + 24, RGB{0x14, 0x14, 0x14, 0xff});

     const auto& ans = core.answerFeatures();

    const int cell = 86;
    const int gap = 10;
    int x0 = hintBox_.x + (hintBox_.w - (4 * cell + 3 * gap)) / 2;
    int y0 = hintBox_.y + 100;

    SDL_Color bgCell{0xff, 0xff, 0xff, 0xff};
    SDL_Color black{0x14, 0x14, 0x14, 0xff};

    for (int c = 0; c < 4; ++c) {
        SDL_Rect cellR{x0 + c * (cell + gap), y0, cell, cell};
        fillRect_(cellR, bgForCell_());
        drawRect_(cellR, RGB{0x37, 0x41, 0x51, 0xff});

        if (hint_.showPinyin[(size_t)c]) {
            const std::string ini = ans.initials[c];
            const std::string fin = ans.finals[c];
            const int tone = ans.tones[c];

            const std::string iniDisp = displayReplaceVWithUmlaut(ini);
            const std::string finDisp = displayReplaceVWithUmlaut(fin);
            const std::string toneDigit = (tone >= 1 && tone <= 4) ? std::to_string(tone) : "";
            int wIni = measureTextW_(fontPinyin_, iniDisp);
            int wFin = measureTextW_(fontPinyin_, finDisp);
            int wTone = (toneMode_ == ToneDisplayMode::Number) ? measureTextW_(fontPinyin_, toneDigit) : 0;
            int totalW = wIni + wFin + wTone;
            int pyStartX = cellR.x + (cellR.w - totalW) / 2;
            int pyY = cellR.y + 0;

            // 提示窗可调：toneYOffsetPx / toneMaxRows
            drawPinyinParts(ren_, fontPinyin_, cellR, pyStartX+5, pyY,
                           ini, fin, tone,
                           toneMode_, toneMarkAvailable_,
                           black, black, black, bgCell,
                           /*toneYOffsetPx*/ 10,
                           /*toneMaxRows*/ 6);
        }

        if (hint_.showChar[(size_t)c]) {
            SDL_Rect hzR = cellR;
            hzR.y += 22;
            hzR.h -= 22;
            drawTextCentered_(fontCellHanzi_, ans.chars[c], hzR, RGB{0x14, 0x14, 0x14, 0xff});
        }
    }

    bool hasMore = (hint_.step < hint_.maxSteps);
    if (hasMore) {
        if (hintMoreHovered_) {
            drawSelectedBg_(hintMoreHighlightRect_);
        }
        drawText_(fontSubmit_, u8"进一步提示", hintMore_.x-75, hintMore_.y-20, RGB{0x00, 0x00, 0x00, 0xff});
    } else {
        int doneW = measureTextW_(fontSubmit_, u8"提示已全部显示");
        drawText_(fontSubmit_,
                  u8"提示已全部显示",
                  hintBox_.x + (hintBox_.w - doneW) / 2,
                  hintMore_.y-20,
                  RGB{0x66, 0x66, 0x66, 0xff});
    }
}

void HGUI::resetPlayingHover_() {
    playingQuitHovered_ = false;
    playingBackDifficultyHovered_ = false;
    playingTipsHovered_ = false;
    submitHovered_ = false;
    toneNumberHovered_ = false;
    toneMarkHovered_ = false;
    hintCloseHovered_ = false;
    hintMoreHovered_ = false;
}

void HGUI::updatePlayingHover_(int mx, int my) {
    resetPlayingHover_();

    if (screen_ != Screen::Playing) {
        return;
    }

    if (hint_.open) {
        hintCloseHovered_ = pointInRect_(mx, my, hintClose_);
        hintMoreHovered_ = hint_.step < hint_.maxSteps && pointInRect_(mx, my, hintMoreHighlightRect_);
        return;
    }

    // 音调模式只在鼠标位于文字本身上方时判定为悬浮，避免高亮范围过大。
    toneNumberHovered_ = pointInRect_(mx, my, toneNumberTextRect_);
    toneMarkHovered_ = pointInRect_(mx, my, toneMarkTextRect_);
    playingTipsHovered_ = pointInRect_(mx, my, gameTipsIconRect_);
    playingBackDifficultyHovered_ = pointInRect_(mx, my, gameBackDifficultyIconRect_);
    playingQuitHovered_ = pointInRect_(mx, my, gameQuitIconRect_);
    submitHovered_ = pointInRect_(mx, my, submitHitRect_);
}


void HGUI::handleEvent_(const SDL_Event& e, HGCore& core, int& outExitCode) {
    auto resetToDifficulty = [&]() {
        screen_ = Screen::DifficultySelect;
        input_.clear();
        composing_.clear();
        guessScroll_ = 0;
        chartScroll_ = 0;
        hint_ = HintModalState{};
        difficultySelectedIndex_ = 0;
        difficultyHoveredIndex_ = -1;
        difficultyBackHovered_ = false;
        resetPlayingHover_();
    };

    auto chooseHoveredDifficulty = [&]() -> bool {
        if (difficultyHoveredIndex_ == 0) {
            chooseDifficulty_(core, Difficulty::Easy);
            return true;
        }
        if (difficultyHoveredIndex_ == 1) {
            chooseDifficulty_(core, Difficulty::Normal);
            return true;
        }
        if (difficultyHoveredIndex_ == 2) {
            chooseDifficulty_(core, Difficulty::Hard);
            return true;
        }
        return false;
    };

    auto refreshDifficultyHover = [&]() {
        int mx = 0;
        int my = 0;
        SDL_GetMouseState(&mx, &my);

        difficultyBackHovered_ = pointInRect_(mx, my, difficultyBackHitRect_);
        difficultyHoveredIndex_ = -1;
        if (!difficultyBackHovered_) {
            for (std::size_t i = 0; i < difficultyHitRects_.size(); ++i) {
                if (pointInRect_(mx, my, difficultyHitRects_[i])) {
                    difficultyHoveredIndex_ = static_cast<int>(i);
                    difficultySelectedIndex_ = static_cast<int>(i);
                    break;
                }
            }
        }
    };

    auto refreshPlayingHover = [&]() {
        int mx = 0;
        int my = 0;
        SDL_GetMouseState(&mx, &my);
        updatePlayingHover_(mx, my);
    };

    auto activateHoveredPlayingTarget = [&]() -> bool {
        if (hint_.open) {
            if (hintCloseHovered_) {
                hint_.open = false;
                resetPlayingHover_();
                return true;
            }
            if (hintMoreHovered_ && hint_.step < hint_.maxSteps) {
                advanceHint_(core);
                return true;
            }
            return false;
        }

        if (toneNumberHovered_) {
            toneMode_ = ToneDisplayMode::Number;
            return true;
        }
        if (toneMarkHovered_) {
            if (toneMarkAvailable_) {
                toneMode_ = ToneDisplayMode::Mark;
            }
            return true;
        }
        if (playingTipsHovered_) {
            openHintModal_(core);
            resetPlayingHover_();
            return true;
        }
        if (playingBackDifficultyHovered_) {
            resetToDifficulty();
            return true;
        }
        if (playingQuitHovered_) {
            outExitCode = 1;
            return true;
        }
        if (submitHovered_) {
            submit_(core);
            return true;
        }
        return false;
    };

    if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        computeLayout_(e.window.data1, e.window.data2);
        if (screen_ == Screen::DifficultySelect) {
            refreshDifficultyHover();
        } else {
            refreshPlayingHover();
        }
        return;
    }

    if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_LEAVE) {
        if (screen_ == Screen::DifficultySelect) {
            difficultyBackHovered_ = false;
            difficultyHoveredIndex_ = -1;
        }
        resetPlayingHover_();
        return;
    }

    if (screen_ == Screen::DifficultySelect && e.type == SDL_MOUSEMOTION) {
        const int mx = e.motion.x;
        const int my = e.motion.y;

        difficultyBackHovered_ = pointInRect_(mx, my, difficultyBackHitRect_);
        difficultyHoveredIndex_ = -1;

        // 难度页只有鼠标悬浮在返回箭头或难度文字上时，才显示 bg2.png。
        if (!difficultyBackHovered_) {
            for (std::size_t i = 0; i < difficultyHitRects_.size(); ++i) {
                if (pointInRect_(mx, my, difficultyHitRects_[i])) {
                    difficultyHoveredIndex_ = static_cast<int>(i);
                    difficultySelectedIndex_ = static_cast<int>(i);
                    break;
                }
            }
        }
        return;
    }

    if (screen_ == Screen::Playing && e.type == SDL_MOUSEMOTION) {
        updatePlayingHover_(e.motion.x, e.motion.y);
        return;
    }

    if (e.type == SDL_MOUSEWHEEL && screen_ == Screen::Playing && !hint_.open) {
        int mx = 0, my = 0;
        SDL_GetMouseState(&mx, &my);
        if (pointInRect_(mx, my, panelLeft_)) {
            if (e.wheel.y < 0) guessScroll_ = std::min(guessScroll_ + 1, 999);
            if (e.wheel.y > 0) guessScroll_ = std::max(guessScroll_ - 1, 0);
        } else if (pointInRect_(mx, my, panelRight_)) {
            if (e.wheel.y < 0) chartScroll_ = std::min(chartScroll_ + 30, 3000);
            if (e.wheel.y > 0) chartScroll_ = std::max(chartScroll_ - 30, 0);
        }
        return;
    }

    if (hint_.open) {
        if (e.type == SDL_KEYDOWN) {
            refreshPlayingHover();
            const SDL_Keycode key = e.key.keysym.sym;
            if (key == SDLK_ESCAPE) {
                hint_.open = false;
                resetPlayingHover_();
                return;
            }
            if (key == SDLK_TAB) {
                toggleToneMode_();
                return;
            }
            if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                activateHoveredPlayingTarget();
                return;
            }
        }

        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            updatePlayingHover_(e.button.x, e.button.y);
            activateHoveredPlayingTarget();
            return;
        }
        return;
    }

    // Tab：数字 <-> 符号。提示框打开时已在上面的 hint_.open 分支中处理。
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_TAB) {
        if (screen_ == Screen::Playing) {
            toggleToneMode_();
            return;
        }
    }

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        const int mx = e.button.x;
        const int my = e.button.y;

        if (screen_ == Screen::DifficultySelect) {
            difficultyBackHovered_ = pointInRect_(mx, my, difficultyBackHitRect_);
            difficultyHoveredIndex_ = -1;
            if (!difficultyBackHovered_) {
                for (std::size_t i = 0; i < difficultyHitRects_.size(); ++i) {
                    if (pointInRect_(mx, my, difficultyHitRects_[i])) {
                        difficultyHoveredIndex_ = static_cast<int>(i);
                        difficultySelectedIndex_ = static_cast<int>(i);
                        break;
                    }
                }
            }

            if (difficultyBackHovered_) {
                outExitCode = 1;
                return;
            }
            chooseHoveredDifficulty();
            return;
        }

        if (screen_ == Screen::Playing) {
            updatePlayingHover_(mx, my);
            activateHoveredPlayingTarget();
            return;
        }
    }

    if (screen_ == Screen::DifficultySelect && e.type == SDL_KEYDOWN) {
        refreshDifficultyHover();
        const SDL_Keycode key = e.key.keysym.sym;

        if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
            if (difficultyBackHovered_) {
                outExitCode = 1;
                return;
            }
            chooseHoveredDifficulty();
            return;
        }
        if (key == SDLK_ESCAPE) {
            outExitCode = 1;
            return;
        }
        return;
    }

    if (e.type == SDL_TEXTEDITING && screen_ == Screen::Playing && !core.snapshot().finished) {
        composing_ = e.edit.text;
        return;
    }

    if (e.type == SDL_TEXTINPUT && screen_ == Screen::Playing && !core.snapshot().finished) {
        if (e.text.text[0] == '\t' && e.text.text[1] == '\0') return;
        input_ += e.text.text;
        composing_.clear();
        truncateUtf8InPlace(input_, 4);
        return;
    }

    if (e.type == SDL_KEYDOWN && screen_ == Screen::Playing) {
        refreshPlayingHover();
        SDL_Keycode key = e.key.keysym.sym;

        if (key == SDLK_ESCAPE) {
            outExitCode = 1;
            return;
        }
        if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
            if (!activateHoveredPlayingTarget()) {
                submit_(core);
            }
            return;
        }
        if (!core.snapshot().finished && key == SDLK_BACKSPACE) {
            popUtf8Last_(input_);
            return;
        }
    }
}

void HGUI::render_(HGCore& core,
                   const std::vector<std::string>& allInitials,
                   const std::vector<std::string>& allFinals,
                   const HGUIConfig& cfg) {
    if (screen_ == Screen::DifficultySelect) {
        clear_(RGB{0xff, 0xff, 0xff, 0xff});
        drawDifficultyScreen_();
        SDL_RenderPresent(ren_);
        return;
    }

    // 运行界面的背景色与选择难度界面保持一致。
    clear_(RGB{0xff, 0xff, 0xff, 0xff});
    fillRect_(panelLeft_, RGB{0xff, 0xff, 0xff, 0xff});
    fillRect_(panelRight_, RGB{0xff, 0xff, 0xff, 0xff});

    SDL_SetTextInputRect(&inputBox_);

    drawTopBar_(core.snapshot(), cfg);
    drawPlayingScreen_(core, allInitials, allFinals);

    SDL_RenderPresent(ren_);
}

void HGUI::submit_(HGCore& core) {
    trimInPlace(input_);
    truncateUtf8InPlace(input_, 4);
    if (input_.empty()) return;

    bool accepted = core.submitGuess(input_);
    if (accepted) {
        input_.clear();
        composing_.clear();
    }
}

void HGUI::chooseDifficulty_(HGCore& core, Difficulty diff) {
    uint32_t seed = (uint32_t)std::chrono::high_resolution_clock::now().time_since_epoch().count();
    core.newGame(diff, seed);

    input_.clear();
    composing_.clear();
    guessScroll_ = 0;
    chartScroll_ = 0;
    toneMode_ = ToneDisplayMode::Number;

    hint_ = HintModalState{};
    hint_.open = false;
    resetPlayingHover_();

    screen_ = Screen::Playing;
}

} // namespace HandleGame