#include "MenuScreen.h"

#include <memory>

#include "../VerseUnfoldSDLApp.h"
#include "Mode1Screen.h"
#include "Mode2Screen.h"

namespace VerseUnfold {

MenuScreen::MenuScreen(VerseUnfoldSDLApp* app)
    : BaseScreen(app) {
    easyBtn = std::make_unique<Button>(SDL_Rect{180, 260, 180, 54}, "初窥门径", [this]() {
        selectedDifficulty = 1;
        refreshDifficultyButtons();
    });
    normalBtn = std::make_unique<Button>(SDL_Rect{420, 260, 180, 54}, "渐入佳境", [this]() {
        selectedDifficulty = 2;
        refreshDifficultyButtons();
    });
    hardBtn = std::make_unique<Button>(SDL_Rect{660, 260, 180, 54}, "登堂入室", [this]() {
        selectedDifficulty = 3;
        refreshDifficultyButtons();
    });

    mode1Btn = std::make_unique<Button>(SDL_Rect{220, 390, 260, 72}, "开始模式一", [this]() {
        startMode1();
    });
    mode2Btn = std::make_unique<Button>(SDL_Rect{540, 390, 260, 72}, "开始模式二", [this]() {
        startMode2();
    });
    exitBtn = std::make_unique<Button>(SDL_Rect{422, 620, 180, 54}, "退出", [this]() {
        this->app->quit();
    });

    refreshDifficultyButtons();
}

void MenuScreen::refreshDifficultyButtons() {
    easyBtn->setSelected(selectedDifficulty == 1);
    normalBtn->setSelected(selectedDifficulty == 2);
    hardBtn->setSelected(selectedDifficulty == 3);
}

void MenuScreen::startMode1() {
    app->getController()->startNewGame(1, selectedDifficulty);
    app->changeScreen(std::make_unique<Mode1Screen>(app));
}

void MenuScreen::startMode2() {
    app->getController()->startNewGame(2, 0);
    app->changeScreen(std::make_unique<Mode2Screen>(app));
}

void MenuScreen::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_1) {
            selectedDifficulty = 1;
            refreshDifficultyButtons();
        } else if (event.key.keysym.sym == SDLK_2) {
            selectedDifficulty = 2;
            refreshDifficultyButtons();
        } else if (event.key.keysym.sym == SDLK_3) {
            selectedDifficulty = 3;
            refreshDifficultyButtons();
        } else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
            startMode1();
            return;
        }
    }

    easyBtn->handleEvent(event);
    normalBtn->handleEvent(event);
    hardBtn->handleEvent(event);
    mode1Btn->handleEvent(event);
    mode2Btn->handleEvent(event);
    exitBtn->handleEvent(event);
}

void MenuScreen::update() {}

void MenuScreen::render(SDL_Renderer* renderer) {
    auto* assets = app->getAssets();

    UIRenderUtils::renderText(renderer, assets->getFont(VerseUnfoldAssets::FontSize::Large),
                              "句读之间 · VerseUnfoldGame", 210, 84, {65, 48, 28, 255});
    UIRenderUtils::renderText(renderer, assets->getFont(VerseUnfoldAssets::FontSize::Medium),
                              "模式一：电脑出题，你来猜诗", 320, 180, {90, 70, 48, 255});
    UIRenderUtils::renderText(renderer, assets->getFont(VerseUnfoldAssets::FontSize::Small),
                              "先选择模式一难度；模式二不区分难度。键盘 1/2/3 也可切换难度，回车可直接开始模式一。",
                              150, 220, {110, 100, 88, 255});

    easyBtn->render(renderer, assets);
    normalBtn->render(renderer, assets);
    hardBtn->render(renderer, assets);
    mode1Btn->render(renderer, assets);
    mode2Btn->render(renderer, assets);
    exitBtn->render(renderer, assets);

    SDL_Rect noteRect{120, 500, 784, 84};
    UIRenderUtils::renderBox(renderer, noteRect, {255, 250, 240, 255}, {198, 176, 138, 255}, 2);
    UIRenderUtils::renderWrappedText(renderer, assets->getFont(VerseUnfoldAssets::FontSize::Small),
                                     "模式二玩法说明：你先在心里想好数据库中的一首诗，逐步描述朝代、作者、体裁、情感、意象、背景等信息；电脑在把握足够高时会主动猜测。",
                                     145, 522, 734, {75, 66, 56, 255}, 4);
}

} // namespace VerseUnfold
