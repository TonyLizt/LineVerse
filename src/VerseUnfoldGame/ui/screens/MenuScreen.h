#pragma once

#include <memory>

#include "BaseScreen.h"
#include "../widgets/UIWidgets.h"

namespace VerseUnfold {

class MenuScreen : public BaseScreen {
public:
    explicit MenuScreen(VerseUnfoldSDLApp* app);

    void handleEvent(const SDL_Event& event) override;
    void update() override;
    void render(SDL_Renderer* renderer) override;

private:
    void refreshDifficultyButtons();
    void startMode1();
    void startMode2();

private:
    int selectedDifficulty = 1;
    std::unique_ptr<Button> easyBtn;
    std::unique_ptr<Button> normalBtn;
    std::unique_ptr<Button> hardBtn;
    std::unique_ptr<Button> mode1Btn;
    std::unique_ptr<Button> mode2Btn;
    std::unique_ptr<Button> exitBtn;
};

} // namespace VerseUnfold
