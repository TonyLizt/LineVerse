#include "ResultScreen.h"

#include <memory>
#include <string>
#include <vector>

#include "../VerseUnfoldSDLApp.h"
#include "MenuScreen.h"

namespace VerseUnfold {

ResultScreen::ResultScreen(VerseUnfoldSDLApp* app)
    : BaseScreen(app),
      scrollOffset(0),
      contentHeight(0),
      detailPanel({120, 180, 960, 480}),
      detailViewport({170, 310, 860, 300}) {
    // 修改按钮位置与尺寸
    restartBtn = std::make_unique<Button>(
        SDL_Rect{420, 700, 160, 56},
        "再来一局",
        [this]() {
            this->app->changeScreen(std::make_unique<MenuScreen>(this->app));
        }
    );

    exitBtn = std::make_unique<Button>(
        SDL_Rect{620, 700, 160, 56},
        "退出模块",
        [this]() {
            this->app->quit();
        }
    );
}

std::string ResultScreen::joinPoemContent(const Poem* poem) {
    if (poem == nullptr) {
        return "";
    }

    std::string result;
    for (size_t i = 0; i < poem->content.size(); ++i) {
        result += poem->content[i];
        if (i + 1 < poem->content.size()) {
            result += "\n";
        }
    }
    return result;
}

// ===================== 新增辅助函数 1：joinVector =====================
std::string ResultScreen::joinVector(const std::vector<std::string>& items, const std::string& sep) {
    std::string result;
    for (size_t i = 0; i < items.size(); ++i) {
        if (items[i].empty()) continue;
        if (!result.empty()) result += sep;
        result += items[i];
    }
    return result;
}

// ===================== 新增辅助函数 2：buildDetailText =====================
std::string ResultScreen::buildDetailText(const Poem* poem) {
    if (poem == nullptr) return "";

    std::string text;

    text += "【原文】\n";
    text += joinPoemContent(poem);
    text += "\n\n";

    if (!poem->emotionCore.empty()) {
        text += "【核心情感】\n";
        text += poem->emotionCore;
        text += "\n\n";
    }

    if (!poem->emotion.empty()) {
        text += "【情感解析】\n";
        text += poem->emotion;
        text += "\n\n";
    }

    if (!poem->bgCore.empty() || !poem->creationBg.empty()) {
        text += "【创作背景】\n";
        if (!poem->bgCore.empty()) {
            text += poem->bgCore;
            if (!poem->creationBg.empty()) text += "；";
        }
        if (!poem->creationBg.empty()) {
            text += poem->creationBg;
        }
        text += "\n\n";
    }

    std::string imageryText;
    if (!poem->mainImageryOne.empty()) imageryText += poem->mainImageryOne;
    if (!poem->imageryGameAll.empty()) {
        if (!imageryText.empty()) imageryText += "、";
        imageryText += joinVector(poem->imageryGameAll, "、");
    } else if (!poem->imagery.empty()) {
        if (!imageryText.empty()) imageryText += "、";
        imageryText += joinVector(poem->imagery, "、");
    }
    if (!imageryText.empty()) {
        text += "【主要意象】\n";
        text += imageryText;
        text += "\n\n";
    }

    if (!poem->featureCore.empty() || !poem->distinctFeature.empty()) {
        text += "【艺术特色】\n";
        if (!poem->featureCore.empty()) {
            text += poem->featureCore;
            if (!poem->distinctFeature.empty()) text += "；";
        }
        if (!poem->distinctFeature.empty()) {
            text += poem->distinctFeature;
        }
        text += "\n\n";
    }

    if (!poem->authorTag.empty()) {
        text += "【作者标签】\n";
        text += poem->authorTag;
        text += "\n\n";
    }

    if (!poem->rhythm.empty()) {
        text += "【体裁与音律】\n";
        text += poem->rhythm;
        text += "\n\n";
    }

    text += "【基本信息】\n";
    text += "句数：" + std::to_string(poem->sentenceCount) + "，";
    text += "字数：" + std::to_string(poem->charCount);

    return text;
}

// ===================== 新增辅助函数 3：clampScroll =====================
void ResultScreen::clampScroll() {
    int maxScroll = contentHeight - detailViewport.h;
    if (maxScroll < 0) maxScroll = 0;

    if (scrollOffset < 0) scrollOffset = 0;
    if (scrollOffset > maxScroll) scrollOffset = maxScroll;
}

// ===================== 修改 handleEvent：支持鼠标滚轮 =====================
void ResultScreen::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_MOUSEWHEEL) {
        int mx = 0, my = 0;
        SDL_GetMouseState(&mx, &my);

        SDL_Point p{mx, my};
        if (SDL_PointInRect(&p, &detailViewport)) {
            scrollOffset -= event.wheel.y * 36;   // 一格滚 36 像素
            clampScroll();
            return;
        }
    }

    restartBtn->handleEvent(event);
    exitBtn->handleEvent(event);
}

void ResultScreen::update() {}

// ===================== 重写 render：滚动文本框 + 完整布局 =====================
void ResultScreen::render(SDL_Renderer* renderer) {
    auto* assets = app->getAssets();
    const auto& state = app->getController()->getState();
    const Poem* poem = app->getController()->getCurrentPoem();

    // 顶部标题（挑战成功/结束）
    const SDL_Color titleColor = state.isWin
        ? SDL_Color{72, 116, 61, 255}
        : SDL_Color{148, 61, 52, 255};

    UIRenderUtils::renderText(
        renderer,
        assets->getFont(VerseUnfoldAssets::FontSize::Large),
        state.isWin ? "挑战成功" : "挑战结束",
        500, 30,
        titleColor
    );

    // 统计信息
    std::string summary = "最终得分：" + std::to_string(state.score)
                        + "   已用提示：" + std::to_string(state.usedHintCount)
                        + "   错误次数：" + std::to_string(state.wrongGuessCount);

    UIRenderUtils::renderText(
        renderer,
        assets->getFont(VerseUnfoldAssets::FontSize::Small),
        summary,
        340, 92,
        {98, 80, 58, 255}
    );

    // 绘制详情外框（固定布局）
    UIRenderUtils::renderBox(
        renderer,
        detailPanel,
        {255, 251, 243, 255},
        {196, 176, 138, 255},
        3
    );

    if (poem != nullptr) {
        // ===================== 诗题（居中，无重复书名号） =====================
        SDL_Rect titleArea{0, 205, 1200, 50};
        UIRenderUtils::renderCenteredText(
            renderer,
            assets->getFont(VerseUnfoldAssets::FontSize::Large),
            poem->title,
            titleArea,
            {60, 48, 33, 255}
        );

        // ===================== 作者信息 =====================
        SDL_Rect metaArea{0, 255, 1200, 40};
        UIRenderUtils::renderCenteredText(
            renderer,
            assets->getFont(VerseUnfoldAssets::FontSize::Medium),
            poem->author + " · " + poem->dynasty + " · " + poem->type,
            metaArea,
            {100, 84, 62, 255}
        );

        // ===================== 核心：滚动详情文本 =====================
        std::string detailText = buildDetailText(poem);
        TTF_Font* detailFont = assets->getFont(VerseUnfoldAssets::FontSize::Small);
        
        // 自动换行计算行
        std::vector<std::string> lines =
            UIRenderUtils::wrapText(detailFont, detailText, detailViewport.w - 24);

        int lineHeight = TTF_FontHeight(detailFont);
        int lineSpacing = 8;
        contentHeight = static_cast<int>(lines.size()) * lineHeight
                      + static_cast<int>(lines.size() - 1) * lineSpacing
                      + 20;
        clampScroll();

        // 设置裁剪区域，只显示视口内内容
        SDL_RenderSetClipRect(renderer, &detailViewport);
        
        // 绘制文本内容
        int currentY = detailViewport.y + 10 - scrollOffset;
        for (const auto& line : lines) {
            if (currentY + lineHeight >= detailViewport.y &&
                currentY <= detailViewport.y + detailViewport.h) {
                UIRenderUtils::renderText(
                    renderer,
                    detailFont,
                    line,
                    detailViewport.x + 8,
                    currentY,
                    {55, 47, 38, 255}
                );
            }
            currentY += lineHeight + lineSpacing;
        }
        
        // 取消裁剪
        SDL_RenderSetClipRect(renderer, nullptr);

        // 滚动提示
        UIRenderUtils::renderText(
            renderer,
            assets->getFont(VerseUnfoldAssets::FontSize::Small),
            "鼠标滚轮可上下查看详细赏析",
            detailPanel.x + 24,
            detailPanel.y + detailPanel.h - 32,
            {120, 104, 82, 255}
        );

        // ===================== 可选：右侧滚动条 =====================
        if (contentHeight > detailViewport.h) {
            SDL_Rect track{
                detailViewport.x + detailViewport.w + 8,
                detailViewport.y,
                8,
                detailViewport.h
            };

            SDL_SetRenderDrawColor(renderer, 220, 210, 190, 255);
            SDL_RenderFillRect(renderer, &track);

            int thumbH = detailViewport.h * detailViewport.h / contentHeight;
            if (thumbH < 30) thumbH = 30;

            int maxScroll = contentHeight - detailViewport.h;
            int thumbY = detailViewport.y;
            if (maxScroll > 0) {
                thumbY += (detailViewport.h - thumbH) * scrollOffset / maxScroll;
            }

            SDL_Rect thumb{track.x, thumbY, track.w, thumbH};
            SDL_SetRenderDrawColor(renderer, 150, 120, 80, 255);
            SDL_RenderFillRect(renderer, &thumb);
        }
    } else {
        // 无诗词数据提示
        SDL_Rect emptyArea{250, 350, 700, 60};
        UIRenderUtils::renderCenteredText(
            renderer,
            assets->getFont(VerseUnfoldAssets::FontSize::Medium),
            "未能匹配到正确诗词信息。",
            emptyArea,
            {120, 72, 52, 255}
        );
    }

    // 绘制按钮
    restartBtn->render(renderer, assets);
    exitBtn->render(renderer, assets);
}

} // namespace VerseUnfold