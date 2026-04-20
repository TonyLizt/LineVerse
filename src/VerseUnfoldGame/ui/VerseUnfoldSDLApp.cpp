#include "VerseUnfoldSDLApp.h"

#include <iostream>
#include <utility>

#include "screens/MenuScreen.h"

namespace VerseUnfold {

VerseUnfoldSDLApp::VerseUnfoldSDLApp(std::shared_ptr<PoetryGameController> ctrl)
    : controller(std::move(ctrl)) {}

VerseUnfoldSDLApp::~VerseUnfoldSDLApp() {
    cleanup();
}

bool VerseUnfoldSDLApp::init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "[VerseUnfoldSDLApp] SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

    window = SDL_CreateWindow(
        "句读之间 - VerseUnfoldGame",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (window == nullptr) {
        std::cerr << "[VerseUnfoldSDLApp] SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        cleanup();
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        std::cerr << "[VerseUnfoldSDLApp] SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        cleanup();
        return false;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    assets = std::make_unique<VerseUnfoldAssets>();
    if (!assets->load()) {
        std::cerr << "[VerseUnfoldSDLApp] Failed to load assets." << std::endl;
        cleanup();
        return false;
    }

    // 初始页面直接放到 pending 里，然后统一应用
    changeScreen(std::make_unique<MenuScreen>(this));
    applyPendingTransitions();

    isRunning = true;
    return true;
}

int VerseUnfoldSDLApp::run() {
    SDL_Event event;
    while (isRunning) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                pendingQuit = true;
            } else if (currentScreen) {
                currentScreen->handleEvent(event);
            }

            applyPendingTransitions();
            if (!isRunning) {
                break;
            }
        }

        if (!isRunning) {
            break;
        }

        if (currentScreen) {
            currentScreen->update();
        }

        applyPendingTransitions();
        if (!isRunning) {
            break;
        }

        SDL_SetRenderDrawColor(renderer, 247, 242, 225, 255);
        SDL_RenderClear(renderer);

        if (currentScreen) {
            currentScreen->render(renderer);
        }

        SDL_RenderPresent(renderer);
    }

    return 1;
}

void VerseUnfoldSDLApp::applyPendingTransitions() {
    if (pendingScreen) {
        currentScreen = std::move(pendingScreen);
    }

    if (pendingQuit) {
        isRunning = false;
        pendingQuit = false;
    }
}

void VerseUnfoldSDLApp::cleanup() {
    pendingScreen.reset();
    currentScreen.reset();
    assets.reset();

    if (renderer != nullptr) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window != nullptr) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    if (SDL_WasInit(SDL_INIT_VIDEO)) {
        SDL_Quit();
    }
}

void VerseUnfoldSDLApp::changeScreen(std::unique_ptr<BaseScreen> newScreen) {
    pendingScreen = std::move(newScreen);
}

void VerseUnfoldSDLApp::quit() {
    pendingQuit = true;
}

} // namespace VerseUnfold