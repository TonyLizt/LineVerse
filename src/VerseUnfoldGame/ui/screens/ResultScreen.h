#pragma once

#include <memory>
#include <string>
#include <vector>

#include "BaseScreen.h"
#include "../widgets/UIWidgets.h"

struct Poem;

namespace VerseUnfold {

class ResultScreen : public BaseScreen {
public:
    explicit ResultScreen(VerseUnfoldSDLApp* app);

    void handleEvent(const SDL_Event& event) override;
    void update() override;
    void render(SDL_Renderer* renderer) override;

private:
    static std::string joinPoemContent(const Poem* poem);
    static std::string buildDetailText(const Poem* poem);
    static std::string joinVector(const std::vector<std::string>& items, const std::string& sep = "、");
    void clampScroll();

private:
    std::unique_ptr<Button> restartBtn;
    std::unique_ptr<Button> exitBtn;

    SDL_Rect detailPanel{120, 190, 960, 470};
    SDL_Rect detailViewport{155, 300, 890, 330};

    int scrollOffset = 0;
    int contentHeight = 0;
};

} // namespace VerseUnfold