#pragma once

#include <SDL.h>

#include <memory>

#include "../controller/PoetryGameController.h"
#include "VerseUnfoldAssets.h"
#include "screens/BaseScreen.h"

namespace VerseUnfold {

class VerseUnfoldSDLApp {
public:
    static constexpr int WINDOW_WIDTH = 1200;
    static constexpr int WINDOW_HEIGHT = 800;

    explicit VerseUnfoldSDLApp(std::shared_ptr<PoetryGameController> ctrl);
    ~VerseUnfoldSDLApp();

    bool init();
    int run();
    void cleanup();

    // 注意：现在 changeScreen 不再立即切换，而是请求切换
    void changeScreen(std::unique_ptr<BaseScreen> newScreen);
    void quit();

    std::shared_ptr<PoetryGameController> getController() const { return controller; }
    VerseUnfoldAssets* getAssets() const { return assets.get(); }
    SDL_Renderer* getRenderer() const { return renderer; }

private:
    void applyPendingTransitions();

private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool isRunning = false;

    std::shared_ptr<PoetryGameController> controller;
    std::unique_ptr<VerseUnfoldAssets> assets;

    std::unique_ptr<BaseScreen> currentScreen;
    std::unique_ptr<BaseScreen> pendingScreen;
    bool pendingQuit = false;
};

} // namespace VerseUnfold