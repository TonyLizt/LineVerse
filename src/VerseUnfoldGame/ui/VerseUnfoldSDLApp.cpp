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
    return init(nullptr, nullptr);
}

bool VerseUnfoldSDLApp::init(SDL_Window* externalWindow, SDL_Renderer* externalRenderer) {
    if ((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) == 0) {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "[VerseUnfoldSDLApp] SDL_Init failed: " << SDL_GetError() << std::endl;
            return false;
        }
        ownsSDL = true;
    }

    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

    if (externalWindow != nullptr && externalRenderer != nullptr) {
        window = externalWindow;
        renderer = externalRenderer;
        ownsWindowRenderer = false;
        SDL_SetWindowTitle(window, "句读之间 - VerseUnfoldGame");
        SDL_ShowWindow(window);
        SDL_RaiseWindow(window);
    } else {
        ownsWindowRenderer = true;
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

    if (ownsWindowRenderer) {
        if (renderer != nullptr) {
            SDL_DestroyRenderer(renderer);
        }
        if (window != nullptr) {
            SDL_DestroyWindow(window);
        }
    }
    renderer = nullptr;
    window = nullptr;
    ownsWindowRenderer = true;

    if (ownsSDL) {
        SDL_Quit();
        ownsSDL = false;
    }
}

void VerseUnfoldSDLApp::changeScreen(std::unique_ptr<BaseScreen> newScreen) {
    pendingScreen = std::move(newScreen);
}

void VerseUnfoldSDLApp::quit() {
    pendingQuit = true;
}

} // namespace VerseUnfold