// src/HandleGame/UI/HGUI.cpp
#include "UI/HGUI.hpp"

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

// -------------------- 圆形组合滑块：胶囊轨道 + 圆形按钮 --------------------
static void fillCircle(SDL_Renderer* ren, int cx, int cy, int r, SDL_Color c) {
    SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
    for (int dy = -r; dy <= r; ++dy) {
        int dx = (int)std::floor(std::sqrt((double)r * r - (double)dy * dy));
        SDL_RenderDrawLine(ren, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

static void fillCapsule(SDL_Renderer* ren, const SDL_Rect& r, SDL_Color c) {
    int radius = r.h / 2;
    SDL_Rect mid{r.x + radius, r.y, r.w - radius * 2, r.h};
    SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(ren, &mid);
    fillCircle(ren, r.x + radius, r.y + radius, radius, c);
    fillCircle(ren, r.x + r.w - radius, r.y + radius, radius, c);
}

static void drawToggleSwitch(SDL_Renderer* ren, const SDL_Rect& r, bool on) {
    SDL_Color track{0xd1, 0xd5, 0xdb, 0xff}; // 灰色轨道
    SDL_Color knob = on ? SDL_Color{0x22, 0xc5, 0x5e, 0xff} : SDL_Color{0xef, 0x44, 0x44, 0xff};

    fillCapsule(ren, r, track);

    int pad = 4;
    int radius = r.h / 2 - pad;
    int cy = r.y + r.h / 2;
    int cx = on ? (r.x + r.w - r.h / 2) : (r.x + r.h / 2);
    fillCircle(ren, cx, cy, radius, knob);
}

static bool fontSupportsCJK_(TTF_Font* font) {
    if (!font) return false;
    return TTF_GlyphIsProvided32(font, 0x6C49) != 0 &&
           TTF_GlyphIsProvided32(font, 0x5B57) != 0;
}

bool HGUI::initSDL_(const HGUIConfig& cfg, std::string& err) {
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

    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

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
    if (fontPinyin_ && fontPinyin_ != fontHanzi_) {
        TTF_CloseFont(fontPinyin_);
    }
    fontPinyin_ = nullptr;

    if (fontHanzi_) {
        TTF_CloseFont(fontHanzi_);
        fontHanzi_ = nullptr;
    }

    if (ren_)  { SDL_DestroyRenderer(ren_); ren_ = nullptr; }
    if (win_)  { SDL_DestroyWindow(win_); win_ = nullptr; }

    if (ownsTTF_) { TTF_Quit(); ownsTTF_ = false; }
    if (ownsSDL_) { SDL_Quit(); ownsSDL_ = false; }
}

int HGUI::run(HGCore& core,
              const std::vector<std::string>& allInitials,
              const std::vector<std::string>& allFinals,
              const HGUIConfig& cfg) {
    std::string err;
    if (!initSDL_(cfg, err)) {
        SDL_Log("HandleGame GUI init failed: %s", err.c_str());
        return -1;
    }

    screen_ = Screen::DifficultySelect;
    input_.clear();
    composing_.clear();
    guessScroll_ = 0;
    chartScroll_ = 0;

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

    msgBar_ = SDL_Rect{pad, pad, w - pad * 2, 46};

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

    btnBackMenu_ = SDL_Rect{panelRight_.x + 16, panelRight_.y + 12, 120, 34};
    btnExitGame_ = SDL_Rect{panelRight_.x + 16 + 120 + 10, panelRight_.y + 12, 120, 34};
    btnHint_ = SDL_Rect{panelRight_.x + 16 + 2 * (120 + 10), panelRight_.y + 12, 120, 34};

    // 音调开关位置仍保留（但只在 Playing 绘制）
    toneToggle_ = SDL_Rect{msgBar_.x + msgBar_.w - 790, msgBar_.y + 10, 48, 26};

    const int cx = msgBar_.x + msgBar_.w / 2;
    const int bw = 260, bh = 56;
    const int by = msgBar_.y + msgBar_.h + 90;

    btnEasy_   = SDL_Rect{cx - bw / 2, by, bw, bh};
    btnNormal_ = SDL_Rect{cx - bw / 2, by + (bh + 18), bw, bh};
    btnHard_   = SDL_Rect{cx - bw / 2, by + 2 * (bh + 18), bw, bh};

    // 提示弹窗布局
    hintBox_ = SDL_Rect{ (w - 520) / 2, (h - 360) / 2, 520, 360 };
    hintClose_ = SDL_Rect{ hintBox_.x + hintBox_.w - 34, hintBox_.y + 10, 24, 24 };
    hintMore_  = SDL_Rect{ hintBox_.x + (hintBox_.w - 220) / 2, hintBox_.y + hintBox_.h - 56, 220, 40 };
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

HGUI::RGB HGUI::bgForCell_() { return RGB{244, 233, 205, 0xff}; }

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

static void drawButton(SDL_Renderer* ren, TTF_Font* font, const SDL_Rect& r,
                       const std::string& text, SDL_Color bg, SDL_Color fg) {
    SDL_SetRenderDrawColor(ren, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderFillRect(ren, &r);
    SDL_SetRenderDrawColor(ren, 0x37, 0x41, 0x51, 0xff);
    SDL_RenderDrawRect(ren, &r);

    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, text.c_str(), fg);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
    if (!tex) { SDL_FreeSurface(surf); return; }

    SDL_Rect dst{r.x + (r.w - surf->w) / 2, r.y + (r.h - surf->h) / 2, surf->w, surf->h};
    SDL_RenderCopy(ren, tex, nullptr, &dst);

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
    fillRect_(msgBar_, RGB{244, 233, 205, 0xff});
    drawRect_(msgBar_, RGB{244, 233, 205, 0xff});

    std::string title = cfg.windowTitle;

    //  只在游戏界面绘制滑块开关
    if (screen_ == Screen::Playing) {
        bool on = (toneMode_ == ToneDisplayMode::Mark) && toneMarkAvailable_;
        drawToggleSwitch(ren_, toneToggle_, on);

        
        title += "  ";
        title += toneModeName(toneMode_);
        title += "音调";

    }

    drawText_(fontHanzi_, title, msgBar_.x + 12, msgBar_.y + 10, RGB{0x14, 0x14, 0x14, 0xff});

    if (screen_ == Screen::Playing && !snap.message.empty()) {
        drawText_(fontPinyin_, snap.message, msgBar_.x + 12, msgBar_.y + 44, RGB{0x14, 0x14, 0x14, 0xff});
    }
}

void HGUI::drawDifficultyScreen_() {
    drawText_(fontHanzi_, "请选择难度",
              msgBar_.x + msgBar_.w / 2 - 70,
              msgBar_.y + msgBar_.h + 30,
              RGB{0x14, 0x14, 0x14, 0xff});

    drawButton(ren_, fontHanzi_, btnBackMenu_, "返回主菜单",
               SDL_Color{88, 88, 88, 0xff}, SDL_Color{245, 245, 245, 255});
    drawButton(ren_, fontHanzi_, btnExitGame_, "退出游戏",
               SDL_Color{156, 44, 44, 0xff}, SDL_Color{245, 245, 245, 255});

    drawButton(ren_, fontHanzi_, btnEasy_,   "简单模式",
               SDL_Color{0x25, 0x63, 0xeb, 0xff}, SDL_Color{245, 245, 245, 255});
    drawButton(ren_, fontHanzi_, btnNormal_, "普通模式",
               SDL_Color{0x25, 0x63, 0xeb, 0xff}, SDL_Color{245, 245, 245, 255});
    drawButton(ren_, fontHanzi_, btnHard_,   "困难模式",
               SDL_Color{0x25, 0x63, 0xeb, 0xff}, SDL_Color{245, 245, 245, 255});
}

void HGUI::drawPlayingScreen_(HGCore& core,
                              const std::vector<std::string>& allInitials,
                              const std::vector<std::string>& allFinals) {
    const CoreSnapshot& snap = core.snapshot();

    drawButton(ren_, fontHanzi_, btnBackMenu_, "返回主菜单",
               SDL_Color{88, 88, 88, 0xff}, SDL_Color{245, 245, 245, 255});
    drawButton(ren_, fontHanzi_, btnExitGame_, "重选难度",
               SDL_Color{156, 44, 44, 0xff}, SDL_Color{245, 245, 245, 255});
    drawButton(ren_, fontHanzi_, btnHint_, "提示",
               SDL_Color{0x25, 0x63, 0xeb, 0xff}, SDL_Color{245, 245, 245, 255});

    // 输入框
    fillRect_(inputBox_, RGB{244, 233, 205, 0xff});
    drawRect_(inputBox_, RGB{0x37, 0x41, 0x51, 0xff});
    drawText_(fontHanzi_, "输入：", inputBox_.x + 10, inputBox_.y + 10, RGB{0x14, 0x14, 0x14, 0xff});
    drawText_(fontHanzi_, input_, inputBox_.x + 70, inputBox_.y + 10, RGB{0x14, 0x14, 0x14, 0xff});
    if (!composing_.empty()) {
        int x = inputBox_.x + 70 + measureTextW_(fontHanzi_, input_);
        drawText_(fontHanzi_, composing_, x, inputBox_.y + 10, RGB{0x14, 0x14, 0x14, 0xff});
    }
    drawButton(ren_, fontHanzi_, btnSubmit_, "提交",
               SDL_Color{0x10, 0xb9, 0x81, 0xff}, SDL_Color{245, 245, 245, 255});

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

    SDL_Color bgCell{244, 233, 205, 0xff};

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
            int pyY = cellR.y + 10;

            drawPinyinParts(ren_, fontPinyin_, cellR, pyStartX, pyY,
                           ini, fin, row.tones[c],
                           toneMode_, toneMarkAvailable_,
                           iniCol, finCol, toneCol,
                           bgCell,
                           /*toneYOffsetPx*/ 12,
                           /*toneMaxRows*/ 5);

            // 汉字行
            SDL_Rect hzR = cellR;
            hzR.y += 22;
            hzR.h -= 22;
            drawTextCentered_(fontHanzi_, row.chars[c], hzR, fontForMark_(row.mChar[c]));
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
    int cy = panelRight_.y + 60 - chartScroll_;

    drawText_(fontHanzi_, "拼音速查表", cx, cy, RGB{0x14, 0x14, 0x14, 0xff});
    cy += 28;
    drawText_(fontPinyin_, "红=不在答案 黄=在答案 绿=位置正确", cx, cy, RGB{0x14, 0x14, 0x14, 0xff});
    cy += 30;

    int tokenW = 58, tokenH = 32, tokenGap = 8;
    int cols = (panelRight_.w - 32) / (tokenW + tokenGap);
    if (cols < 4) cols = 4;

    drawText_(fontHanzi_, "声母", cx, cy, RGB{0x14, 0x14, 0x14, 0xff});
    cy += 24;

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
    cy += 24;

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

    fillRect_(hintBox_, RGB{244, 233, 205, 0xff});
    drawRect_(hintBox_, RGB{0x37, 0x41, 0x51, 0xff});

    drawButton(ren_, fontHanzi_, hintClose_, "×",
               SDL_Color{156, 44, 44, 0xff}, SDL_Color{245, 245, 245, 255});

    drawText_(fontHanzi_, "正确答案", hintBox_.x + 210, hintBox_.y + 36, RGB{0x14, 0x14, 0x14, 0xff});

     const auto& ans = core.answerFeatures();

    const int cell = 86;
    const int gap = 10;
    int x0 = hintBox_.x + (hintBox_.w - (4 * cell + 3 * gap)) / 2;
    int y0 = hintBox_.y + 90;

    SDL_Color bgCell{244, 233, 205, 0xff};
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
            int pyY = cellR.y + 10;

            // 提示窗可调：toneYOffsetPx / toneMaxRows
            drawPinyinParts(ren_, fontPinyin_, cellR, pyStartX, pyY,
                           ini, fin, tone,
                           toneMode_, toneMarkAvailable_,
                           black, black, black, bgCell,
                           /*toneYOffsetPx*/ 12,
                           /*toneMaxRows*/ 5);
        }

        if (hint_.showChar[(size_t)c]) {
            SDL_Rect hzR = cellR;
            hzR.y += 22;
            hzR.h -= 22;
            drawTextCentered_(fontHanzi_, ans.chars[c], hzR, RGB{0x14, 0x14, 0x14, 0xff});
        }
    }

    bool hasMore = (hint_.step < hint_.maxSteps);
    if (hasMore) {
        drawButton(ren_, fontHanzi_, hintMore_, "进一步提示",
                   SDL_Color{0x25, 0x63, 0xeb, 0xff}, SDL_Color{245, 245, 245, 255});
    }
}

void HGUI::handleEvent_(const SDL_Event& e, HGCore& core, int& outExitCode) {
    if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        computeLayout_(e.window.data1, e.window.data2);
        return;
    }

    if (e.type == SDL_MOUSEWHEEL) {
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

    // Tab：数字 <-> 符号（只在 Playing 生效）
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_TAB) {
        if (screen_ == Screen::Playing) {
            toggleToneMode_();
            return;
        }
    }

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x, my = e.button.y;

        //  滑块点击：只在 Playing 生效
        if (screen_ == Screen::Playing && pointInRect_(mx, my, toneToggle_)) {
            toggleToneMode_();
            return;
        }

        if (hint_.open) {
            if (pointInRect_(mx, my, hintClose_)) {
                hint_.open = false;
                return;
            }
            if (hint_.step < hint_.maxSteps && pointInRect_(mx, my, hintMore_)) {
                advanceHint_(core);
                return;
            }
            return;
        }

        if (screen_ == Screen::DifficultySelect) {
            if (pointInRect_(mx, my, btnEasy_))   { chooseDifficulty_(core, Difficulty::Easy); return; }
            if (pointInRect_(mx, my, btnNormal_)) { chooseDifficulty_(core, Difficulty::Normal); return; }
            if (pointInRect_(mx, my, btnHard_))   { chooseDifficulty_(core, Difficulty::Hard); return; }
            if (pointInRect_(mx, my, btnBackMenu_)) { outExitCode = 1; return; }
            if (pointInRect_(mx, my, btnExitGame_)) { outExitCode = 0; return; }
            return;
        }

        if (pointInRect_(mx, my, btnBackMenu_)) { outExitCode = 1; return; }

        if (pointInRect_(mx, my, btnExitGame_)) {
            screen_ = Screen::DifficultySelect;
            input_.clear();
            composing_.clear();
            guessScroll_ = 0;
            chartScroll_ = 0;
            return;
        }

        if (pointInRect_(mx, my, btnHint_)) {
            openHintModal_(core);
            return;
        }
        if (pointInRect_(mx, my, btnSubmit_)) { submit_(core); return; }
        return;
    }

    if (hint_.open) {
        return;
    }

    if (screen_ == Screen::Playing && core.snapshot().finished) {
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) outExitCode = 1;
        return;
    }

    if (e.type == SDL_TEXTEDITING && screen_ == Screen::Playing) {
        composing_ = e.edit.text;
        return;
    }

    if (e.type == SDL_TEXTINPUT && screen_ == Screen::Playing) {
        if (e.text.text[0] == '\t' && e.text.text[1] == '\0') return;
        input_ += e.text.text;
        composing_.clear();
        truncateUtf8InPlace(input_, 4);
        return;
    }

    if (e.type == SDL_KEYDOWN && screen_ == Screen::Playing) {
        SDL_Keycode key = e.key.keysym.sym;

        if (key == SDLK_ESCAPE) { outExitCode = 1; return; }
        if (key == SDLK_BACKSPACE) { popUtf8Last_(input_); return; }
        if (key == SDLK_RETURN || key == SDLK_KP_ENTER) { submit_(core); return; }
    }
}

void HGUI::render_(HGCore& core,
                   const std::vector<std::string>& allInitials,
                   const std::vector<std::string>& allFinals,
                   const HGUIConfig& cfg) {
    clear_(RGB{244, 233, 205, 0xff});
    fillRect_(panelLeft_, RGB{244, 233, 205, 0xff});
    fillRect_(panelRight_, RGB{244, 233, 205, 0xff});

    SDL_SetTextInputRect(&inputBox_);

    drawTopBar_(core.snapshot(), cfg);

    if (screen_ == Screen::DifficultySelect) {
        drawDifficultyScreen_();
    } else {
        drawPlayingScreen_(core, allInitials, allFinals);
    }

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

    hint_ = HintModalState{};
    hint_.open = false;

    screen_ = Screen::Playing;
}

} // namespace HandleGame