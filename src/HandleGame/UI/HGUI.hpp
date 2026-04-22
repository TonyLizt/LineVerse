#pragma once

#include "core/HGCore.hpp"
#include "core/HGEngine.hpp"
#include "core/HGTone.hpp"

#include <SDL.h>
#include <SDL_ttf.h>

#include <array>
#include <random>
#include <string>
#include <vector>

namespace HandleGame {

struct HGUIConfig {
    std::string windowTitle = "HandleGame";
    int width = 1200;
    int height = 800;
    std::vector<std::string> fontCandidates;
    int fontSize = 40;
    int pinyinFontSize = 22;
    ToneDisplayMode toneDisplay = ToneDisplayMode::Number;
};

class HGUI {
public:
    HGUI();
    ~HGUI();

    int run(HGCore& core,
            const std::vector<std::string>& allInitials,
            const std::vector<std::string>& allFinals,
            const HGUIConfig& cfg);

    int run(SDL_Window* externalWindow,
            SDL_Renderer* externalRenderer,
            HGCore& core,
            const std::vector<std::string>& allInitials,
            const std::vector<std::string>& allFinals,
            const HGUIConfig& cfg);

private:
    enum class Screen {
        DifficultySelect,
        Playing
    };

    struct RGB {
        uint8_t r, g, b, a;
    };

    // 提示弹窗状态（保留显示）
    struct HintModalState {
        bool open = false;
        int step = 0; // 已推进次数（用于控制“进一步提示”次数）
        std::array<bool, 4> showPinyin{false, false, false, false};
        std::array<bool, 4> showChar{false, false, false, false};
        std::array<int, 4> order{-1, -1, -1, -1}; // 提示顺序：第 k 次揭示的是哪个格
        int maxSteps = 0;                          // 最大推进次数
    };

private:
    bool initSDL_(const HGUIConfig& cfg, std::string& err,
                  SDL_Window* externalWindow = nullptr,
                  SDL_Renderer* externalRenderer = nullptr);
    void shutdown_();

    void computeLayout_(int w, int h);
    static bool pointInRect_(int x, int y, const SDL_Rect& r);
    static void popUtf8Last_(std::string& s);

    void handleEvent_(const SDL_Event& e, HGCore& core, int& outExitCode);
    void render_(HGCore& core,
                 const std::vector<std::string>& allInitials,
                 const std::vector<std::string>& allFinals,
                 const HGUIConfig& cfg);

    void submit_(HGCore& core);
    void chooseDifficulty_(HGCore& core, Difficulty diff);

    // ====== UI drawing ======
    RGB bgForCell_();
    RGB fontForMark_(Mark m);
    RGB fontForChartState_(int st);

    void clear_(RGB c);
    void fillRect_(const SDL_Rect& r, RGB c);
    void drawRect_(const SDL_Rect& r, RGB c);

    int measureTextW_(TTF_Font* f, const std::string& s) const;
    void drawText_(TTF_Font* f, const std::string& s, int x, int y, RGB c);
    void drawTextCentered_(TTF_Font* f, const std::string& s, const SDL_Rect& r, RGB c);

    void drawTopBar_(const CoreSnapshot& snap, const HGUIConfig& cfg);
    void drawDifficultyScreen_();
    void drawPlayingScreen_(HGCore& core,
                            const std::vector<std::string>& allInitials,
                            const std::vector<std::string>& allFinals);
    void drawSelectedBg_(const SDL_Rect& r);
    void drawTextureOrText_(SDL_Texture* texture, const SDL_Rect& r, const std::string& fallbackText);
    void updatePlayingHover_(int mx, int my);
    void resetPlayingHover_();

    void updateDifficultyLayout_(int w, int h);
    void loadDifficultySelectionResources_(const HGUIConfig& cfg, const std::string& chosenFontPath);
    void destroyDifficultySelectionResources_();

    // ====== tone toggle switch ======
    void toggleToneMode_();

    // ====== hint modal ======
    void openHintModal_(HGCore& core);
    void advanceHint_(HGCore& core);
    void drawHintModal_(HGCore& core);

private:
    // SDL
    SDL_Window* win_ = nullptr;
    SDL_Renderer* ren_ = nullptr;
    bool ownsSDL_ = false;
    bool ownsTTF_ = false;
    bool ownsIMG_ = false;
    bool ownsWindowRenderer_ = true;

    TTF_Font* fontHanzi_ = nullptr;
    TTF_Font* fontPinyin_ = nullptr;
    TTF_Font* fontSubmit_ = nullptr;
    TTF_Font* fontCellHanzi_ = nullptr;
    TTF_Font* fontDifficultyCaption_ = nullptr;
    TTF_Font* fontDifficultyButton_ = nullptr;

    SDL_Texture* difficultySelectionBgTexture_ = nullptr;
    SDL_Texture* difficultySelectedBgTexture_ = nullptr;
    SDL_Texture* difficultyBackIconTexture_ = nullptr;
    SDL_Texture* gameQuitIconTexture_ = nullptr;
    SDL_Texture* gameBackDifficultyIconTexture_ = nullptr;
    SDL_Texture* gameTipsIconTexture_ = nullptr;
    SDL_Texture* hintCloseIconTexture_ = nullptr;

    // 状态
    Screen screen_ = Screen::DifficultySelect;

    std::string input_;
    std::string composing_;
    int guessScroll_ = 0;
    int chartScroll_ = 0;
    int difficultySelectedIndex_ = 0;
    int difficultyHoveredIndex_ = -1;
    bool difficultyBackHovered_ = false;

    bool playingQuitHovered_ = false;
    bool playingBackDifficultyHovered_ = false;
    bool playingTipsHovered_ = false;
    bool submitHovered_ = false;
    bool toneNumberHovered_ = false;
    bool toneMarkHovered_ = false;
    bool hintCloseHovered_ = false;
    bool hintMoreHovered_ = false;

    ToneDisplayMode toneMode_ = ToneDisplayMode::Number;
    bool toneMarkAvailable_ = false;

    // Layout
    SDL_Rect msgBar_{};
    SDL_Rect panelLeft_{};
    SDL_Rect panelRight_{};
    SDL_Rect inputBox_{};
    SDL_Rect btnSubmit_{};

    SDL_Rect btnBackMenu_{};
    SDL_Rect btnExitGame_{};
    SDL_Rect btnHint_{};

    SDL_Rect gameQuitIconRect_{};
    SDL_Rect gameBackDifficultyIconRect_{};
    SDL_Rect gameTipsIconRect_{};
    SDL_Rect gameQuitHighlightRect_{};
    SDL_Rect gameBackDifficultyHighlightRect_{};
    SDL_Rect gameTipsHighlightRect_{};
    SDL_Rect toneNumberTextRect_{};
    SDL_Rect toneMarkTextRect_{};
    SDL_Rect toneNumberHighlightRect_{};
    SDL_Rect toneMarkHighlightRect_{};
    SDL_Rect submitTextRect_{};
    SDL_Rect submitHitRect_{};
    SDL_Rect submitHighlightRect_{};

    SDL_Rect btnEasy_{};
    SDL_Rect btnNormal_{};
    SDL_Rect btnHard_{};
    SDL_Rect difficultySelectionBgRect_{};
    SDL_Rect difficultyBackIconRect_{};
    SDL_Rect difficultyBackHitRect_{};
    SDL_Rect difficultyBackHighlightRect_{};
    std::array<SDL_Rect, 3> difficultyTextRects_{};
    std::array<SDL_Rect, 3> difficultyHitRects_{};

    // 顶部音调切换开关
    SDL_Rect toneToggle_{};

    // 提示弹窗 UI
    HintModalState hint_;
    SDL_Rect hintBox_{};
    SDL_Rect hintClose_{};
    SDL_Rect hintMore_{};
    SDL_Rect hintCloseHighlightRect_{};
    SDL_Rect hintMoreHighlightRect_{};

    // Random
    std::mt19937 rng_{};
};

} // namespace HandleGame
