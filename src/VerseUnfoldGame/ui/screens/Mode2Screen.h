#pragma once

#include <memory>
#include <string>
#include <SDL.h>

#include "BaseScreen.h"
#include "../widgets/UIWidgets.h"

class Poem;

namespace VerseUnfold {

class Mode2Screen : public BaseScreen {
public:
    explicit Mode2Screen(VerseUnfoldSDLApp* app);

    void handleEvent(const SDL_Event& event) override;
    void update() override;
    void render(SDL_Renderer* renderer) override;

private:
    void submitDescription();
    void submitRevealTitle();

    static std::string buildGuessPreviewText(const Poem* poem);
    void clampGuessPreviewScroll();

private:
    SDL_Rect guessPreviewPanel{350, 320, 500, 200};
    SDL_Rect guessPreviewViewport{370, 340, 460, 150};

    int guessPreviewScrollOffset = 0;
    int guessPreviewContentHeight = 0;

    std::unique_ptr<InputBox> descriptionInput;
    std::unique_ptr<Button> sendBtn;
    std::unique_ptr<Button> revealBtn;
    std::unique_ptr<Button> menuBtn;

    std::unique_ptr<Button> yesBtn;
    std::unique_ptr<Button> noBtn;

    std::unique_ptr<InputBox> revealInput;
    std::unique_ptr<Button> revealSubmitBtn;
    std::unique_ptr<Button> revealCancelBtn;

    bool revealDialogVisible = false;
    bool goToResultAfterReveal = false;
    std::string statusMessage;
};

} // namespace VerseUnfold