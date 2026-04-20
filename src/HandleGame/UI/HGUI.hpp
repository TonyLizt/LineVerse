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
    bool initSDL_(const HGUIConfig& cfg, std::string& err);
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

    TTF_Font* fontHanzi_ = nullptr;
    TTF_Font* fontPinyin_ = nullptr;

    // 状态
    Screen screen_ = Screen::DifficultySelect;

    std::string input_;
    std::string composing_;
    int guessScroll_ = 0;
    int chartScroll_ = 0;

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

    SDL_Rect btnEasy_{};
    SDL_Rect btnNormal_{};
    SDL_Rect btnHard_{};

    // 顶部音调切换开关
    SDL_Rect toneToggle_{};

    // 提示弹窗 UI
    HintModalState hint_;
    SDL_Rect hintBox_{};
    SDL_Rect hintClose_{};
    SDL_Rect hintMore_{};

    // Random
    std::mt19937 rng_{};
};

} // namespace HandleGame