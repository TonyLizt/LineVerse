#pragma once

#include <memory>
#include <string>

#include "BaseScreen.h"
#include "../widgets/UIWidgets.h"

namespace VerseUnfold {

class Mode1Screen : public BaseScreen {
public:
    explicit Mode1Screen(VerseUnfoldSDLApp* app);

    void handleEvent(const SDL_Event& event) override;
    void update() override;
    void render(SDL_Renderer* renderer) override;

private:
    void submitAnswer();

private:
    std::unique_ptr<InputBox> answerInput;
    std::unique_ptr<Button> submitBtn;
    std::unique_ptr<Button> nextHintBtn;
    std::unique_ptr<Button> menuBtn;
    std::string statusMessage;
};

} // namespace VerseUnfold
