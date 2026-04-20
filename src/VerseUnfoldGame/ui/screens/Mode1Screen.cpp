#include "Mode1Screen.h"

#include <algorithm>
#include <memory>

#include "../VerseUnfoldSDLApp.h"
#include "MenuScreen.h"
#include "ResultScreen.h"

namespace VerseUnfold {

Mode1Screen::Mode1Screen(VerseUnfoldSDLApp* app)
    : BaseScreen(app) {
    answerInput = std::make_unique<InputBox>(SDL_Rect{120, 650, 520, 52}, "请输入诗题、别名或任意一句诗句...");
    answerInput->setFocused(true);

    submitBtn = std::make_unique<Button>(SDL_Rect{670, 650, 110, 52}, "提交", [this]() {
        submitAnswer();
    });
    nextHintBtn = std::make_unique<Button>(SDL_Rect{800, 650, 110, 52}, "下一条", [this]() {
        if (!this->app->getController()->revealNextHint()) {
            statusMessage = "已经没有更多线索，答案即将揭晓。";
        } else {
            statusMessage = "已揭晓新的线索。";
        }
    });
    menuBtn = std::make_unique<Button>(SDL_Rect{860, 28, 120, 42}, "返回菜单", [this]() {
        this->app->changeScreen(std::make_unique<MenuScreen>(this->app));
    });
}

void Mode1Screen::submitAnswer() {
    const std::string input = answerInput->getText();
    if (input.empty()) {
        statusMessage = "请输入内容后再提交。";
        return;
    }

    JudgeResult result = app->getController()->submitGuess(input);
    answerInput->clear();
    answerInput->setFocused(true);

    if (result == JudgeResult::Correct) {
        statusMessage = "回答正确。";
    } else if (result == JudgeResult::Near) {
        statusMessage = "很接近了，再想一想。";
    } else {
        statusMessage = "回答错误，已自动揭晓下一条线索。";
        app->getController()->revealNextHint();
    }
}


void Mode1Screen::handleEvent(const SDL_Event& event) {
    answerInput->handleEvent(event);

    if (event.type == SDL_KEYDOWN &&
        !event.key.repeat &&
        (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER)) {
        if (answerInput->isFocused() && !answerInput->isComposing()) {
            submitAnswer();
            return;
        }
    }

    submitBtn->handleEvent(event);
    nextHintBtn->handleEvent(event);
    menuBtn->handleEvent(event);
}

void Mode1Screen::update() {
    if (app->getController()->getState().isFinished) {
        app->changeScreen(std::make_unique<ResultScreen>(app));
    }
}

void Mode1Screen::render(SDL_Renderer* renderer) {
    auto* assets = app->getAssets();
    const auto& state = app->getController()->getState();

    UIRenderUtils::renderText(renderer, assets->getFont(VerseUnfoldAssets::FontSize::Large),
                              "模式一：电脑出题", 90, 38, {65, 48, 28, 255});
    menuBtn->render(renderer, assets);

    std::string summary = "已用线索：" + std::to_string(state.usedHintCount)
                        + "   错误次数：" + std::to_string(state.wrongGuessCount)
                        + "   当前得分：" + std::to_string(state.score);
    UIRenderUtils::renderText(renderer, assets->getFont(VerseUnfoldAssets::FontSize::Small),
                              summary, 92, 98, {110, 92, 66, 255});

    SDL_Rect hintsPanel{80, 148, 864, 260};
    UIRenderUtils::renderBox(renderer, hintsPanel, {255, 250, 241, 255}, {196, 176, 138, 255}, 2);
    UIRenderUtils::renderText(renderer, assets->getFont(VerseUnfoldAssets::FontSize::Medium),
                              "线索卡", 108, 170, {78, 59, 39, 255});

    int currentY = 216;
    const size_t maxHintsToShow = 5;
    const size_t startIndex = state.shownHints.size() > maxHintsToShow ? state.shownHints.size() - maxHintsToShow : 0;
    for (size_t i = startIndex; i < state.shownHints.size(); ++i) {
        const auto& hint = state.shownHints[i];
        const std::string line = "[" + std::to_string(hint.level) + "] " + hint.text;
        UIRenderUtils::renderWrappedText(renderer, assets->getFont(VerseUnfoldAssets::FontSize::Small),
                                         line, 110, currentY, 800, {55, 48, 40, 255}, 4);
        currentY += UIRenderUtils::measureWrappedTextHeight(assets->getFont(VerseUnfoldAssets::FontSize::Small), line, 800, 4) + 10;
    }

    SDL_Rect historyPanel{80, 435, 864, 170};
    UIRenderUtils::renderBox(renderer, historyPanel, {255, 250, 241, 255}, {196, 176, 138, 255}, 2);
    UIRenderUtils::renderText(renderer, assets->getFont(VerseUnfoldAssets::FontSize::Medium),
                              "作答记录", 108, 456, {78, 59, 39, 255});

    int historyY = 500;
    const size_t maxHistoryToShow = 4;
    const size_t historyStart = state.guessHistory.size() > maxHistoryToShow ? state.guessHistory.size() - maxHistoryToShow : 0;
    for (size_t i = historyStart; i < state.guessHistory.size(); ++i) {
        const auto& guess = state.guessHistory[i];
        std::string suffix = guess.isCorrect ? "（正确）" : (guess.isNear ? "（接近）" : "（未命中）");
        UIRenderUtils::renderText(renderer, assets->getFont(VerseUnfoldAssets::FontSize::Small),
                                  "- " + guess.rawInput + suffix, 110, historyY, {75, 68, 58, 255});
        historyY += 30;
    }

    if (!statusMessage.empty()) {
        UIRenderUtils::renderText(renderer, assets->getFont(VerseUnfoldAssets::FontSize::Small),
                                  statusMessage, 122, 618, {135, 54, 54, 255});
    }

    answerInput->render(renderer, assets);
    submitBtn->render(renderer, assets);
    nextHintBtn->render(renderer, assets);
}

} // namespace VerseUnfold
