#include "HomepageScreen.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

namespace fs = std::filesystem;

namespace lineverse::ui {

namespace {

constexpr int kWindowWidth = 1200;
constexpr int kWindowHeight = 800;

constexpr SDL_Color kBgColor{255, 255, 255, 255};
constexpr SDL_Color kTextColor{0, 0, 0, 255};

constexpr int kTitleTop = 80;
constexpr int kTitleRight = 160;

constexpr int kHomeButtonStartY = 460;
constexpr int kHomeButtonGap = 20;
constexpr int kHomeSelectedBgHeight = 120;
constexpr int kHomeSelectedBgWidth = 360;

constexpr int kSelectionCaptionTop = 170;
constexpr int kSelectionButtonStartY = 300;
constexpr int kSelectionButtonGap = 45;
constexpr int kSelectionSelectedBgHeight = 150;
constexpr int kSelectionSelectedBgWidth = 460;
constexpr int kSelectionBgTargetHeight = 1000;
constexpr int kSelectionBgOffsetX = 200;

constexpr double kBreathSpeed = 0.0045;
constexpr Uint8 kBreathAlphaMin = 255;
constexpr Uint8 kBreathAlphaMax = 255;

bool pointInRect(int px, int py, const SDL_Rect& rect) {
    return px >= rect.x && px < rect.x + rect.w &&
           py >= rect.y && py < rect.y + rect.h;
}

struct SdlWindowGuard {
    SDL_Window* ptr = nullptr;

    SdlWindowGuard() = default;
    SdlWindowGuard(const SdlWindowGuard&) = delete;
    SdlWindowGuard& operator=(const SdlWindowGuard&) = delete;

    SdlWindowGuard(SdlWindowGuard&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }

    SdlWindowGuard& operator=(SdlWindowGuard&& other) noexcept {
        if (this != &other) {
            if (ptr != nullptr) {
                SDL_DestroyWindow(ptr);
            }
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    ~SdlWindowGuard() {
        if (ptr != nullptr) {
            SDL_DestroyWindow(ptr);
        }
    }
};

struct SdlRendererGuard {
    SDL_Renderer* ptr = nullptr;

    SdlRendererGuard() = default;
    SdlRendererGuard(const SdlRendererGuard&) = delete;
    SdlRendererGuard& operator=(const SdlRendererGuard&) = delete;

    SdlRendererGuard(SdlRendererGuard&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }

    SdlRendererGuard& operator=(SdlRendererGuard&& other) noexcept {
        if (this != &other) {
            if (ptr != nullptr) {
                SDL_DestroyRenderer(ptr);
            }
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    ~SdlRendererGuard() {
        if (ptr != nullptr) {
            SDL_DestroyRenderer(ptr);
        }
    }
};

struct SdlTextureGuard {
    SDL_Texture* ptr = nullptr;

    SdlTextureGuard() = default;
    SdlTextureGuard(const SdlTextureGuard&) = delete;
    SdlTextureGuard& operator=(const SdlTextureGuard&) = delete;

    SdlTextureGuard(SdlTextureGuard&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }

    SdlTextureGuard& operator=(SdlTextureGuard&& other) noexcept {
        if (this != &other) {
            if (ptr != nullptr) {
                SDL_DestroyTexture(ptr);
            }
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    ~SdlTextureGuard() {
        if (ptr != nullptr) {
            SDL_DestroyTexture(ptr);
        }
    }
};

struct SdlSurfaceGuard {
    SDL_Surface* ptr = nullptr;

    SdlSurfaceGuard() = default;
    SdlSurfaceGuard(const SdlSurfaceGuard&) = delete;
    SdlSurfaceGuard& operator=(const SdlSurfaceGuard&) = delete;

    SdlSurfaceGuard(SdlSurfaceGuard&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }

    SdlSurfaceGuard& operator=(SdlSurfaceGuard&& other) noexcept {
        if (this != &other) {
            if (ptr != nullptr) {
                SDL_FreeSurface(ptr);
            }
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    ~SdlSurfaceGuard() {
        if (ptr != nullptr) {
            SDL_FreeSurface(ptr);
        }
    }
};

struct TtfFontGuard {
    TTF_Font* ptr = nullptr;

    TtfFontGuard() = default;
    TtfFontGuard(const TtfFontGuard&) = delete;
    TtfFontGuard& operator=(const TtfFontGuard&) = delete;

    TtfFontGuard(TtfFontGuard&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }

    TtfFontGuard& operator=(TtfFontGuard&& other) noexcept {
        if (this != &other) {
            if (ptr != nullptr) {
                TTF_CloseFont(ptr);
            }
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    ~TtfFontGuard() {
        if (ptr != nullptr) {
            TTF_CloseFont(ptr);
        }
    }
};

struct TextTexture {
    SdlTextureGuard texture;
    int width = 0;
    int height = 0;

    TextTexture() = default;
    TextTexture(const TextTexture&) = delete;
    TextTexture& operator=(const TextTexture&) = delete;
    TextTexture(TextTexture&&) noexcept = default;
    TextTexture& operator=(TextTexture&&) noexcept = default;
};

struct PageLayout {
    TextTexture captionText;
    SDL_Rect captionRect{0, 0, 0, 0};
    std::vector<TextTexture> buttonTexts;
    std::vector<SDL_Rect> buttonBaseRects;
    std::vector<SDL_Rect> buttonHitRects;
    int selectedBgWidth = 0;
    int selectedBgHeight = 0;
};

struct PageLayoutSpec {
    int captionTop = 0;
    int buttonStartY = 0;
    int buttonGap = 0;
    int selectedBgWidth = 0;
    int selectedBgHeight = 0;
};

enum class PageState {
    Home,
    ModeSelect,
    DifficultySelect
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

SDL_Rect buildHomeBgRect(int texW, int texH) {
    if (texW <= 0 || texH <= 0) {
        return SDL_Rect{0, 100, 900, 650};
    }

    const int dstH = 650;
    const int dstW = static_cast<int>(std::lround(
        static_cast<double>(texW) * static_cast<double>(dstH) / static_cast<double>(texH)
    ));

    return SDL_Rect{
        0,
        (kWindowHeight - dstH) / 2,
        dstW,
        dstH
    };
}

SDL_Rect buildTopLeftBgRect(int texW, int texH, int targetHeight) {
    if (texW <= 0 || texH <= 0) {
        return SDL_Rect{kSelectionBgOffsetX, 0, 1000, targetHeight};
    }

    const int dstW = static_cast<int>(std::lround(
        static_cast<double>(texW) * static_cast<double>(targetHeight) / static_cast<double>(texH)
    ));

    return SDL_Rect{kSelectionBgOffsetX, 0, dstW, targetHeight};
}

TextTexture createTextTexture(
    SDL_Renderer* renderer,
    TTF_Font* font,
    const std::string& text,
    SDL_Color color
) {
    if (renderer == nullptr || font == nullptr) {
        throw std::runtime_error("createTextTexture: renderer or font is null");
    }

    SdlSurfaceGuard surface;
    surface.ptr = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (surface.ptr == nullptr) {
        throw std::runtime_error(std::string("TTF_RenderUTF8_Blended failed: ") + TTF_GetError());
    }

    TextTexture result;
    result.width = surface.ptr->w;
    result.height = surface.ptr->h;
    result.texture.ptr = SDL_CreateTextureFromSurface(renderer, surface.ptr);
    if (result.texture.ptr == nullptr) {
        throw std::runtime_error(std::string("SDL_CreateTextureFromSurface failed: ") + SDL_GetError());
    }

    SDL_SetTextureBlendMode(result.texture.ptr, SDL_BLENDMODE_BLEND);
    return result;
}

PageLayout buildPageLayout(
    SDL_Renderer* renderer,
    TTF_Font* captionFont,
    TTF_Font* buttonFont,
    int columnCenterX,
    const std::string& caption,
    const std::vector<std::string>& labels,
    const PageLayoutSpec& spec
) {
    PageLayout layout;
    layout.selectedBgWidth = spec.selectedBgWidth;
    layout.selectedBgHeight = spec.selectedBgHeight;

    if (!caption.empty()) {
        layout.captionText = createTextTexture(renderer, captionFont, caption, kTextColor);
        layout.captionRect = SDL_Rect{
            columnCenterX - layout.captionText.width / 2,
            spec.captionTop,
            layout.captionText.width,
            layout.captionText.height
        };
    }

    layout.buttonTexts.reserve(labels.size());
    layout.buttonBaseRects.resize(labels.size());
    layout.buttonHitRects.resize(labels.size());

    int currentY = spec.buttonStartY;
    for (const auto& label : labels) {
        layout.buttonTexts.push_back(createTextTexture(renderer, buttonFont, label, kTextColor));
    }

    for (std::size_t i = 0; i < layout.buttonTexts.size(); ++i) {
        const auto& text = layout.buttonTexts[i];
        layout.buttonBaseRects[i] = SDL_Rect{
            columnCenterX - text.width / 2,
            currentY,
            text.width,
            text.height
        };

        layout.buttonHitRects[i] = SDL_Rect{
            columnCenterX - spec.selectedBgWidth / 2,
            currentY + (text.height - spec.selectedBgHeight) / 2,
            spec.selectedBgWidth,
            spec.selectedBgHeight
        };

        currentY += text.height + spec.buttonGap;
    }

    return layout;
}

// std::string modeChoiceToText(HomepageModeChoice mode) {
//     switch (mode) {
//     case HomepageModeChoice::Idiom:
//         return u8"成語";
//     case HomepageModeChoice::Mixed:
//         return u8"混合";
//     case HomepageModeChoice::None:
//     default:
//         return u8"未選擇";
//     }
// }

// std::string difficultyChoiceToText(HomepageDifficultyChoice difficulty) {
//     switch (difficulty) {
//     case HomepageDifficultyChoice::Easy:
//         return u8"簡單";
//     case HomepageDifficultyChoice::Medium:
//         return u8"中等";
//     case HomepageDifficultyChoice::Hard:
//         return u8"困難";
//     case HomepageDifficultyChoice::None:
//     default:
//         return u8"未選擇";
//     }
// }

} // namespace

HomepageScreen::HomepageScreen(const fs::path& gifPath)
    : gifPath_(gifPath) {
}

int HomepageScreen::show(HomepageLaunchSelection& selection) {
    return show(nullptr, nullptr, selection);
}

int HomepageScreen::show(SDL_Window* externalWindow,
                         SDL_Renderer* externalRenderer,
                         HomepageLaunchSelection& selection) {
    selection = HomepageLaunchSelection{};

    if (!fs::exists(gifPath_)) {
        throw std::runtime_error("Homepage bg not found: " + gifPath_.string());
    }

    const fs::path homepageRoot = gifPath_.parent_path().parent_path();
    const fs::path homeFontPath = homepageRoot / "font" / "font2.ttf";
    if (!fs::exists(homeFontPath)) {
        throw std::runtime_error("Homepage home font not found: " + homeFontPath.string());
    }

    const fs::path selectionFontPath = homepageRoot / "font" / "font2.ttf";
    if (!fs::exists(selectionFontPath)) {
        throw std::runtime_error("Homepage selection font not found: " + selectionFontPath.string());
    }

    const fs::path selectedBgPath = homepageRoot / "image" / "bg2.png";
    if (!fs::exists(selectedBgPath)) {
        throw std::runtime_error("Homepage selected bg not found: " + selectedBgPath.string());
    }

    const fs::path selectionBgPath = homepageRoot / "image" / "bg3.jpg";
    if (!fs::exists(selectionBgPath)) {
        throw std::runtime_error("Homepage selection bg not found: " + selectionBgPath.string());
    }

    bool sdlInited = false;
    bool imageInited = false;
    bool ttfInited = false;
    const bool externalContext = (externalWindow != nullptr && externalRenderer != nullptr);

    try {
        if (!externalContext && SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
            throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
        }
        sdlInited = !externalContext;

        if (TTF_WasInit() == 0) {
            if (TTF_Init() != 0) {
                throw std::runtime_error(std::string("TTF_Init failed: ") + TTF_GetError());
            }
            ttfInited = true;
        }

        int imgFlags = 0;
        std::string ext = gifPath_.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });

        if (ext == ".png") {
            imgFlags |= IMG_INIT_PNG;
        } else if (ext == ".jpg" || ext == ".jpeg") {
            imgFlags |= IMG_INIT_JPG;
        } else if (ext == ".webp") {
            imgFlags |= IMG_INIT_WEBP;
        }

        if (imgFlags != 0) {
            const int alreadyImgFlags = IMG_Init(0);
            if ((alreadyImgFlags & imgFlags) != imgFlags) {
                if ((IMG_Init(imgFlags) & imgFlags) != imgFlags) {
                    throw std::runtime_error(std::string("IMG_Init failed: ") + IMG_GetError());
                }
                imageInited = true;
            }
        }

        int result = kResultExit;

        {
            SdlWindowGuard ownedWindow;
            SdlRendererGuard ownedRenderer;
            SDL_Window* window = externalWindow;
            SDL_Renderer* renderer = externalRenderer;
            SdlTextureGuard homeBgTexture;
            SdlTextureGuard selectionBgTexture;
            SdlTextureGuard selectedBgTexture;

            if (!externalContext) {
                ownedWindow.ptr = SDL_CreateWindow(
                    "LineVerse",
                    SDL_WINDOWPOS_CENTERED,
                    SDL_WINDOWPOS_CENTERED,
                    kWindowWidth,
                    kWindowHeight,
                    SDL_WINDOW_SHOWN
                );
                if (ownedWindow.ptr == nullptr) {
                    throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
                }

                ownedRenderer.ptr = createRendererWithFallback(ownedWindow.ptr);
                if (ownedRenderer.ptr == nullptr) {
                    throw std::runtime_error(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());
                }

                window = ownedWindow.ptr;
                renderer = ownedRenderer.ptr;
            } else {
                SDL_SetWindowTitle(window, "LineVerse");
                SDL_ShowWindow(window);
                SDL_RaiseWindow(window);
            }

            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

            homeBgTexture.ptr = IMG_LoadTexture(renderer, gifPath_.string().c_str());
            if (homeBgTexture.ptr == nullptr) {
                throw std::runtime_error(
                    std::string("IMG_LoadTexture failed: ") + IMG_GetError() +
                    " | file=" + gifPath_.string()
                );
            }

            selectedBgTexture.ptr = IMG_LoadTexture(renderer, selectedBgPath.string().c_str());
            if (selectedBgTexture.ptr == nullptr) {
                throw std::runtime_error(
                    std::string("IMG_LoadTexture failed: ") + IMG_GetError() +
                    " | file=" + selectedBgPath.string()
                );
            }
            SDL_SetTextureBlendMode(selectedBgTexture.ptr, SDL_BLENDMODE_BLEND);

            int bgTexW = 0;
            int bgTexH = 0;
            SDL_QueryTexture(homeBgTexture.ptr, nullptr, nullptr, &bgTexW, &bgTexH);
            const SDL_Rect homeBgRect = buildHomeBgRect(bgTexW, bgTexH);

            selectionBgTexture.ptr = IMG_LoadTexture(renderer, selectionBgPath.string().c_str());
            if (selectionBgTexture.ptr == nullptr) {
                throw std::runtime_error(
                    std::string("IMG_LoadTexture failed: ") + IMG_GetError() +
                    " | file=" + selectionBgPath.string()
                );
            }

            int selectionBgTexW = 0;
            int selectionBgTexH = 0;
            SDL_QueryTexture(selectionBgTexture.ptr, nullptr, nullptr, &selectionBgTexW, &selectionBgTexH);
            const SDL_Rect selectionBgRect = buildTopLeftBgRect(
                selectionBgTexW,
                selectionBgTexH,
                kSelectionBgTargetHeight
            );

            TtfFontGuard titleFont;
            TtfFontGuard captionFont;
            TtfFontGuard buttonFont;
            TtfFontGuard selectionButtonFont;

            titleFont.ptr = TTF_OpenFont(homeFontPath.string().c_str(), 100);
            if (titleFont.ptr == nullptr) {
                throw std::runtime_error(std::string("TTF_OpenFont titleFont failed: ") + TTF_GetError());
            }

            buttonFont.ptr = TTF_OpenFont(homeFontPath.string().c_str(), 60);
            if (buttonFont.ptr == nullptr) {
                throw std::runtime_error(std::string("TTF_OpenFont buttonFont failed: ") + TTF_GetError());
            }

            captionFont.ptr = TTF_OpenFont(selectionFontPath.string().c_str(), 40);
            if (captionFont.ptr == nullptr) {
                throw std::runtime_error(std::string("TTF_OpenFont captionFont failed: ") + TTF_GetError());
            }

            selectionButtonFont.ptr = TTF_OpenFont(selectionFontPath.string().c_str(), 80);
            if (selectionButtonFont.ptr == nullptr) {
                throw std::runtime_error(std::string("TTF_OpenFont selectionButtonFont failed: ") + TTF_GetError());
            }

            TextTexture titleText = createTextTexture(
                renderer,
                titleFont.ptr,
                u8"句读之间",
                kTextColor
            );

            const SDL_Rect titleRect{
                kWindowWidth - kTitleRight - titleText.width,
                kTitleTop,
                titleText.width,
                titleText.height
            };
            const int columnCenterX = titleRect.x + titleRect.w / 2;

            const std::vector<std::string> homeLabels = {
                u8"汉兜",
                u8"成语接龙",
                u8"诗成语现",
                u8"线索诗词"
            };
            const std::vector<std::string> modeLabels = {
                u8"成语",
                u8"混合"
            };
            const std::vector<std::string> difficultyLabels = {
                u8"简单",
                u8"中等",
                u8"困难"
            };

            const PageLayoutSpec homeLayoutSpec{
                0,
                kHomeButtonStartY,
                kHomeButtonGap,
                kHomeSelectedBgWidth,
                kHomeSelectedBgHeight
            };
            const PageLayoutSpec selectionLayoutSpec{
                kSelectionCaptionTop,
                kSelectionButtonStartY,
                kSelectionButtonGap,
                kSelectionSelectedBgWidth,
                kSelectionSelectedBgHeight
            };

            PageLayout homeLayout = buildPageLayout(
                renderer,
                captionFont.ptr,
                buttonFont.ptr,
                columnCenterX,
                "",
                homeLabels,
                homeLayoutSpec
            );
            PageLayout modeLayout = buildPageLayout(
                renderer,
                captionFont.ptr,
                selectionButtonFont.ptr,
                kWindowWidth / 2,
                u8"请选择模式",
                modeLabels,
                selectionLayoutSpec
            );
            PageLayout difficultyLayout = buildPageLayout(
                renderer,
                captionFont.ptr,
                selectionButtonFont.ptr,
                kWindowWidth / 2,
                u8"请选择难度",
                difficultyLabels,
                selectionLayoutSpec
            );

            PageState pageState = PageState::Home;
            HomepageModeChoice selectedMode = HomepageModeChoice::None;

            int homeSelectedIndex = 0;
            int modeSelectedIndex = 0;
            int difficultySelectedIndex = 0;

            auto currentLayout = [&]() -> PageLayout& {
                switch (pageState) {
                case PageState::ModeSelect:
                    return modeLayout;
                case PageState::DifficultySelect:
                    return difficultyLayout;
                case PageState::Home:
                default:
                    return homeLayout;
                }
            };

            auto currentSelectedIndex = [&]() -> int& {
                switch (pageState) {
                case PageState::ModeSelect:
                    return modeSelectedIndex;
                case PageState::DifficultySelect:
                    return difficultySelectedIndex;
                case PageState::Home:
                default:
                    return homeSelectedIndex;
                }
            };

            auto moveToHome = [&]() {
                pageState = PageState::Home;
            };

            auto moveToModeSelect = [&]() {
                pageState = PageState::ModeSelect;
                modeSelectedIndex = 0;
            };
            auto moveToDifficultySelect = [&]() {
                pageState = PageState::DifficultySelect;
                difficultySelectedIndex = 0;
            };

            auto confirmDifficulty = [&](HomepageDifficultyChoice difficultyChoice) {
                selection.targetGame = HomepageTargetGame::PoetryRebuildGame;
                selection.mode = selectedMode;
                selection.difficulty = difficultyChoice;
                result = kResultLaunch;
            };

            auto activateCurrentSelection = [&]() -> bool {
                switch (pageState) {
                case PageState::Home:
                    switch (homeSelectedIndex) {
                    case 0:
                        /*
                        * 汉兜
                        */
                        selection.targetGame = HomepageTargetGame::HandleGame;
                        selection.mode = HomepageModeChoice::None;
                        selection.difficulty = HomepageDifficultyChoice::None;
                        result = kResultLaunch;
                        return false;

                    case 1:
                        /*
                        * 成语接龙
                        */
                        selection.targetGame = HomepageTargetGame::IdiomChainGame;
                        selection.mode = HomepageModeChoice::None;
                        selection.difficulty = HomepageDifficultyChoice::None;
                        result = kResultLaunch;
                        return false;

                    case 2:
                        /*
                        * 诗成语现：需要继续选择模式和难度
                        */
                        moveToModeSelect();
                        return true;

                    case 3:
                        /*
                        * 线索诗词
                        */
                        selection.targetGame = HomepageTargetGame::VerseUnfoldGame;
                        selection.mode = HomepageModeChoice::None;
                        selection.difficulty = HomepageDifficultyChoice::None;
                        result = kResultLaunch;
                        return false;

                    default:
                        std::cout << "[Homepage] 未知入口。\n";
                        return true;
                    }

                case PageState::ModeSelect:
                    selectedMode = (modeSelectedIndex == 0)
                        ? HomepageModeChoice::Idiom
                        : HomepageModeChoice::Mixed;
                    moveToDifficultySelect();
                    return true;

                case PageState::DifficultySelect:
                    if (difficultySelectedIndex == 0) {
                        confirmDifficulty(HomepageDifficultyChoice::Easy);
                    } else if (difficultySelectedIndex == 1) {
                        confirmDifficulty(HomepageDifficultyChoice::Medium);
                    } else {
                        confirmDifficulty(HomepageDifficultyChoice::Hard);
                    }
                    return false;

                default:
                    return true;
                }
            };

            bool running = true;
            while (running) {

                SDL_Event event;
                while (running && SDL_PollEvent(&event)) {
                    if (event.type == SDL_QUIT) {
                        result = kResultExit;
                        running = false;
                        break;
                    }

                    PageLayout& layout = currentLayout();
                    int& selectedIndex = currentSelectedIndex();

                    if (event.type == SDL_MOUSEMOTION) {
                        const int mx = event.motion.x;
                        const int my = event.motion.y;
                        for (std::size_t i = 0; i < layout.buttonHitRects.size(); ++i) {
                            if (pointInRect(mx, my, layout.buttonHitRects[i])) {
                                selectedIndex = static_cast<int>(i);
                                break;
                            }
                        }
                    }

                    if (event.type == SDL_MOUSEBUTTONDOWN &&
                        event.button.button == SDL_BUTTON_LEFT) {
                        const int mx = event.button.x;
                        const int my = event.button.y;
                        for (std::size_t i = 0; i < layout.buttonHitRects.size(); ++i) {
                            if (pointInRect(mx, my, layout.buttonHitRects[i])) {
                                selectedIndex = static_cast<int>(i);
                                running = activateCurrentSelection();
                                break;
                            }
                        }
                    }

                    if (event.type == SDL_KEYDOWN) {
                        if (event.key.keysym.sym == SDLK_UP) {
                            selectedIndex =
                                (selectedIndex - 1 + static_cast<int>(layout.buttonTexts.size())) %
                                static_cast<int>(layout.buttonTexts.size());
                        } else if (event.key.keysym.sym == SDLK_DOWN) {
                            selectedIndex =
                                (selectedIndex + 1) % static_cast<int>(layout.buttonTexts.size());
                        } else if (event.key.keysym.sym == SDLK_RETURN ||
                                   event.key.keysym.sym == SDLK_KP_ENTER) {
                            running = activateCurrentSelection();
                            break;
                        } else if (event.key.keysym.sym == SDLK_ESCAPE) {
                            if (pageState == PageState::DifficultySelect) {
                                moveToModeSelect();
                            } else if (pageState == PageState::ModeSelect) {
                                moveToHome();
                            } else {
                                result = kResultExit;
                                running = false;
                            }
                            break;
                        }
                    }
                }

                if (!running) {
                    break;
                }

                SDL_SetRenderDrawColor(renderer, kBgColor.r, kBgColor.g, kBgColor.b, kBgColor.a);
                SDL_RenderClear(renderer);

                const bool isSelectionPage =
                    (pageState == PageState::ModeSelect || pageState == PageState::DifficultySelect);
                if (isSelectionPage) {
                    SDL_RenderCopy(renderer, selectionBgTexture.ptr, nullptr, &selectionBgRect);
                } else {
                    SDL_RenderCopy(renderer, homeBgTexture.ptr, nullptr, &homeBgRect);
                    SDL_RenderCopy(renderer, titleText.texture.ptr, nullptr, &titleRect);
                }

                const double t = static_cast<double>(SDL_GetTicks());
                const double wave = (std::sin(t * kBreathSpeed) + 1.0) * 0.5;
                const Uint8 breathAlpha = static_cast<Uint8>(
                    std::lround(kBreathAlphaMin +
                    (kBreathAlphaMax - kBreathAlphaMin) * wave)
                );

                PageLayout& layout = currentLayout();
                const int selectedIndex = currentSelectedIndex();

                if (layout.captionText.texture.ptr != nullptr) {
                    SDL_RenderCopy(renderer, layout.captionText.texture.ptr, nullptr, &layout.captionRect);
                }

                for (std::size_t i = 0; i < layout.buttonTexts.size(); ++i) {
                    SDL_Rect textRect = layout.buttonBaseRects[i];

                    if (static_cast<int>(i) == selectedIndex) {
                        SDL_Rect selectedBgRect{
                            layout.buttonHitRects[i].x,
                            layout.buttonHitRects[i].y,
                            layout.selectedBgWidth,
                            layout.selectedBgHeight
                        };

                        SDL_SetTextureAlphaMod(selectedBgTexture.ptr, breathAlpha);
                        SDL_RenderCopy(renderer, selectedBgTexture.ptr, nullptr, &selectedBgRect);
                    }

                    SDL_RenderCopy(renderer, layout.buttonTexts[i].texture.ptr, nullptr, &textRect);
                }

                SDL_RenderPresent(renderer);
            }
        }

        if (ttfInited) {
            TTF_Quit();
        }
        if (imageInited) {
            IMG_Quit();
        }
        if (sdlInited) {
            SDL_Quit();
        }

        return result;
    } catch (...) {
        if (ttfInited) {
            TTF_Quit();
        }
        if (imageInited) {
            IMG_Quit();
        }
        if (sdlInited) {
            SDL_Quit();
        }
        throw;
    }
}

} // namespace lineverse::ui
