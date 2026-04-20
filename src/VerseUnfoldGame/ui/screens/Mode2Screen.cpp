#include "Mode2Screen.h"

#include <algorithm>
#include <memory>
#include <string>

#include "../VerseUnfoldSDLApp.h"
#include "MenuScreen.h"
#include "ResultScreen.h"
#include <SDL_ttf.h>

namespace VerseUnfold {

std::string Mode2Screen::buildGuessPreviewText(const Poem* poem) {
    if (!poem) {
        return "";
    }

    std::string preview = poem->title + "\n\n";
    for (const auto& line : poem->lines) {
        preview += line;
        preview += "\n";
    }
    return preview;
}

void Mode2Screen::clampGuessPreviewScroll() {
    int maxScroll = guessPreviewContentHeight - guessPreviewViewport.h;
    if (maxScroll < 0) {
        maxScroll = 0;
    }
    if (guessPreviewScrollOffset < 0) {
        guessPreviewScrollOffset = 0;
    }
    if (guessPreviewScrollOffset > maxScroll) {
        guessPreviewScrollOffset = maxScroll;
    }
}

Mode2Screen::Mode2Screen(VerseUnfoldSDLApp* appPtr)
    : BaseScreen(appPtr) {
    descriptionInput = std::make_unique<InputBox>(
        SDL_Rect{120, 688, 640, 54},
        "输入关于这首诗的描述、关键词或线索..."
    );
    descriptionInput->setFocused(true);

    sendBtn = std::make_unique<Button>(
        SDL_Rect{790, 688, 120, 54},
        "发送",
        [this]() { submitDescription(); }
    );

    revealBtn = std::make_unique<Button>(
        SDL_Rect{940, 688, 140, 54},
        "公布答案",
        [this]() {
            revealDialogVisible = true;
            descriptionInput->setFocused(false);
            revealInput->clear();
            revealInput->setFocused(true);
            statusMessage = "请输入你心里那首诗的标题，用于揭晓结果。";
        }
    );

    menuBtn = std::make_unique<Button>(
        SDL_Rect{1030, 28, 130, 46},
        "返回菜单",
        [this]() {
            app->changeScreen(std::make_unique<MenuScreen>(app));
        }
    );

    // ===================== 1. 修改按钮坐标/尺寸/逻辑 =====================
    yesBtn = std::make_unique<Button>(SDL_Rect{420, 545, 150, 52}, "猜对了", [this]() {
        app->getController()->confirmMode2Guess(true);
        statusMessage = "电脑猜对了。";
    });

    noBtn = std::make_unique<Button>(SDL_Rect{630, 545, 150, 52}, "猜错了", [this]() {
        const std::string next = app->getController()->confirmMode2Guess(false);
        statusMessage = next.empty() ? "这次没猜中。" : next;
        if (!app->getController()->hasPendingMode2Guess() &&
            !app->getController()->getState().isFinished) {
            descriptionInput->setFocused(true);
        }
    });
    // ====================================================================

    revealInput = std::make_unique<InputBox>(
        SDL_Rect{360, 392, 480, 54},
        "输入正确诗题，例如：静夜思"
    );

    revealSubmitBtn = std::make_unique<Button>(
        SDL_Rect{430, 472, 140, 50},
        "确认揭晓",
        [this]() { submitRevealTitle(); }
    );

    revealCancelBtn = std::make_unique<Button>(
        SDL_Rect{630, 472, 140, 50},
        "取消",
        [this]() {
            revealDialogVisible = false;
            revealInput->clear();
            revealInput->setFocused(false);
            if (!app->getController()->getState().isFinished) {
                descriptionInput->setFocused(true);
            }
        }
    );
}

void Mode2Screen::submitDescription() {
    const std::string input = descriptionInput->getText();
    if (input.empty()) {
        statusMessage = "请输入至少一条提示后再发送。";
        return;
    }

    Mode2TurnFeedback feedback = app->getController()->submitDescription(input);
    descriptionInput->clear();
    descriptionInput->setFocused(true);

    if (!feedback.feedbackText.empty()) {
        statusMessage = feedback.feedbackText;
    }
    else if (!feedback.nextQuestion.empty()) {
        statusMessage = feedback.nextQuestion;
    }
    else {
        statusMessage = "线索已提交。";
    }
}

void Mode2Screen::submitRevealTitle() {
    const std::string title = revealInput->getText();
    if (title.empty()) {
        statusMessage = "请输入诗题后再确认。";
        return;
    }

    const Poem* poem = app->getController()->revealMode2AnswerByTitle(title);
    if (poem == nullptr) {
        statusMessage = "数据库中没有匹配到这个诗题，请检查标题或别名。";
        return;
    }

    revealDialogVisible = false;
    revealInput->setFocused(false);
    goToResultAfterReveal = true;
    statusMessage = "答案已揭晓。";
}

void Mode2Screen::handleEvent(const SDL_Event& event) {
    if (revealDialogVisible) {
        revealInput->handleEvent(event);

        if (event.type == SDL_KEYDOWN &&
            !event.key.repeat &&
            (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER)) {
            if (revealInput->isFocused() && !revealInput->isComposing()) {
                submitRevealTitle();
                return;
            }
        }

        revealSubmitBtn->handleEvent(event);
        revealCancelBtn->handleEvent(event);
        return;
    }

    if (app->getController()->hasPendingMode2Guess()) {
        if (event.type == SDL_MOUSEWHEEL) {
            int mx = 0;
            int my = 0;
            SDL_GetMouseState(&mx, &my);
            SDL_Point p{mx, my};
            if (SDL_PointInRect(&p, &guessPreviewViewport)) {
                guessPreviewScrollOffset -= event.wheel.y * 32;
                clampGuessPreviewScroll();
                return;
            }
        }

        yesBtn->handleEvent(event);
        noBtn->handleEvent(event);
        return;
    }

    descriptionInput->handleEvent(event);

    if (event.type == SDL_KEYDOWN &&
        !event.key.repeat &&
        (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER)) {
        if (descriptionInput->isFocused() && !descriptionInput->isComposing()) {
            submitDescription();
            return;
        }
    }

    sendBtn->handleEvent(event);
    revealBtn->handleEvent(event);
    menuBtn->handleEvent(event);
}

void Mode2Screen::update() {
    const auto& state = app->getController()->getState();

    if (goToResultAfterReveal) {
        goToResultAfterReveal = false;
        app->changeScreen(std::make_unique<ResultScreen>(app));
        return;
    }

    if (state.isFinished && state.targetPoemIndex >= 0) {
        app->changeScreen(std::make_unique<ResultScreen>(app));
        return;
    }

    if (state.isFinished && state.targetPoemIndex < 0) {
        revealDialogVisible = true;
        descriptionInput->setFocused(false);
        revealInput->clear();
        revealInput->setFocused(true);

        if (statusMessage.empty()) {
            statusMessage = "本局已结束，请输入正确诗题以揭晓答案。";
        }
    }
}

void Mode2Screen::render(SDL_Renderer* renderer) {
    auto* assets = app->getAssets();
    const auto& state = app->getController()->getState();

    UIRenderUtils::renderText(
        renderer,
        assets->getFont(VerseUnfoldAssets::FontSize::Large),
        "模式二：你划我猜",
        120, 36,
        {65, 48, 28, 255}
    );
    menuBtn->render(renderer, assets);

    UIRenderUtils::renderText(
        renderer,
        assets->getFont(VerseUnfoldAssets::FontSize::Small),
        "你先在心里想好一首数据库中的诗，逐步描述朝代、作者、体裁、情感、意象、背景等信息。",
        124, 96,
        {110, 92, 66, 255}
    );

    UIRenderUtils::renderText(
        renderer,
        assets->getFont(VerseUnfoldAssets::FontSize::Small),
        "电脑把握度",
        120, 146,
        {90, 76, 58, 255}
    );

    SDL_Rect barBg{245, 144, 560, 26};
    UIRenderUtils::renderBox(renderer, barBg, {235, 229, 216, 255}, {170, 150, 118, 255}, 2);

    int clampedConfidence = std::max(0, std::min(100, state.confidencePercent));
    int fillWidth = (barBg.w - 4) * clampedConfidence / 100;
    SDL_Rect barFill{barBg.x + 2, barBg.y + 2, fillWidth, barBg.h - 4};
    SDL_SetRenderDrawColor(renderer, 142, 112, 74, 255);
    SDL_RenderFillRect(renderer, &barFill);

    UIRenderUtils::renderText(
        renderer,
        assets->getFont(VerseUnfoldAssets::FontSize::Small),
        std::to_string(clampedConfidence) + "%",
        825, 146,
        {90, 76, 58, 255}
    );

    SDL_Rect historyPanel{110, 196, 980, 270};
    UIRenderUtils::renderBox(renderer, historyPanel, {255, 250, 241, 255}, {196, 176, 138, 255}, 2);

    UIRenderUtils::renderText(
        renderer,
        assets->getFont(VerseUnfoldAssets::FontSize::Medium),
        "提示记录",
        140, 220,
        {78, 59, 39, 255}
    );

    int currentY = 264;
    const size_t maxHistoryToShow = 5;
    const size_t startIndex =
        state.descriptionHistory.size() > maxHistoryToShow
            ? state.descriptionHistory.size() - maxHistoryToShow
            : 0;

    for (size_t i = startIndex; i < state.descriptionHistory.size(); ++i) {
        const std::string line = "我：" + state.descriptionHistory[i];
        UIRenderUtils::renderWrappedText(
            renderer,
            assets->getFont(VerseUnfoldAssets::FontSize::Small),
            line,
            145, currentY,
            900,
            {60, 53, 44, 255},
            4
        );
        currentY += UIRenderUtils::measureWrappedTextHeight(
                        assets->getFont(VerseUnfoldAssets::FontSize::Small),
                        line,
                        900,
                        4
                    ) + 12;
    }

    if (!state.mode2GuessHistory.empty()) {
        UIRenderUtils::renderText(
            renderer,
            assets->getFont(VerseUnfoldAssets::FontSize::Small),
            "电脑已猜过：",
            120, 492,
            {90, 76, 58, 255}
        );

        const size_t guessStart =
            state.mode2GuessHistory.size() > 3
                ? state.mode2GuessHistory.size() - 3
                : 0;

        int guessY = 520;
        for (size_t i = guessStart; i < state.mode2GuessHistory.size(); ++i) {
            UIRenderUtils::renderText(
                renderer,
                assets->getFont(VerseUnfoldAssets::FontSize::Small),
                "- " + state.mode2GuessHistory[i],
                145, guessY,
                {80, 70, 58, 255}
            );
            guessY += 28;
        }
    }

    if (!state.lastSystemQuestion.empty()) {
        UIRenderUtils::renderWrappedText(
            renderer,
            assets->getFont(VerseUnfoldAssets::FontSize::Small),
            "电脑追问：" + state.lastSystemQuestion,
            120, 584,
            960,
            {120, 68, 46, 255},
            4
        );
    }

    if (!statusMessage.empty()) {
        UIRenderUtils::renderWrappedText(
            renderer,
            assets->getFont(VerseUnfoldAssets::FontSize::Small),
            "系统反馈：" + statusMessage,
            120, 620,
            960,
            {135, 54, 54, 255},
            4
        );
    }

    descriptionInput->render(renderer, assets);
    sendBtn->render(renderer, assets);
    revealBtn->render(renderer, assets);

    if (app->getController()->hasPendingMode2Guess()) {
        const Poem* guessPoem = app->getController()->getPendingMode2GuessPoem();

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 110);
        SDL_Rect fullScreen{0, 0, VerseUnfoldSDLApp::WINDOW_WIDTH, VerseUnfoldSDLApp::WINDOW_HEIGHT};
        SDL_RenderFillRect(renderer, &fullScreen);

        // 弹窗尺寸（符合要求）
        SDL_Rect dialog{280, 170, 640, 460};
        UIRenderUtils::renderBox(renderer, dialog, {255, 250, 245, 255}, {150, 105, 55, 255}, 3);

        const std::string guessHeader =
            "电脑给出了第 " + std::to_string(state.mode2GuessAttemptCount + 1) + " 次猜测";

        // 标题文字
        UIRenderUtils::renderText(
            renderer,
            assets->getFont(VerseUnfoldAssets::FontSize::Medium),
            guessHeader,
            430, 205,
            {60, 50, 40, 255}
        );

        // ===================== 核心修改：新增滚轮提示（标题下方） =====================
        UIRenderUtils::renderText(
            renderer,
            assets->getFont(VerseUnfoldAssets::FontSize::Small),
            "鼠标滚轮可查看完整诗句",
            470, 245,
            {120, 100, 80, 255}
        );
        // ==========================================================================

        if (guessPoem != nullptr) {
            UIRenderUtils::renderBox(
                renderer,
                guessPreviewPanel,
                {245, 239, 226, 255},
                {170, 150, 120, 255},
                2
            );

            std::string previewText = buildGuessPreviewText(guessPoem);
            TTF_Font* font = assets->getFont(VerseUnfoldAssets::FontSize::Small);

            guessPreviewContentHeight =
                UIRenderUtils::measureWrappedTextHeight(
                    font,
                    previewText,
                    guessPreviewViewport.w - 24,
                    6
                );

            SDL_RenderSetClipRect(renderer, &guessPreviewViewport);
            int renderY = guessPreviewViewport.y + 12 - guessPreviewScrollOffset;
            UIRenderUtils::renderWrappedText(
                renderer,
                font,
                previewText,
                guessPreviewViewport.x + 12,
                renderY,
                guessPreviewViewport.w - 24,
                {50, 40, 30, 255},
                6
            );
            SDL_RenderSetClipRect(renderer, nullptr);

            // 已删除原底部的滚轮提示文字
        }

        // 按钮坐标符合要求（545），无挤压
        yesBtn->render(renderer, assets);
        noBtn->render(renderer, assets);
    }

    if (revealDialogVisible) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 110);
        SDL_Rect fullScreen{0, 0, VerseUnfoldSDLApp::WINDOW_WIDTH, VerseUnfoldSDLApp::WINDOW_HEIGHT};
        SDL_RenderFillRect(renderer, &fullScreen);

        SDL_Rect dialog{330, 290, 540, 240};
        UIRenderUtils::renderBox(renderer, dialog, {255, 250, 245, 255}, {150, 105, 55, 255}, 3);

        UIRenderUtils::renderText(
            renderer,
            assets->getFont(VerseUnfoldAssets::FontSize::Medium),
            "请输入正确诗题以揭晓结果",
            420, 330,
            {60, 50, 40, 255}
        );

        revealInput->render(renderer, assets);
        revealSubmitBtn->render(renderer, assets);
        revealCancelBtn->render(renderer, assets);
    }
}

} // namespace VerseUnfold