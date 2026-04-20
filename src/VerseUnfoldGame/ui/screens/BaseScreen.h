#pragma once

#include <SDL.h>

namespace VerseUnfold {

class VerseUnfoldSDLApp;

class BaseScreen {
public:
    explicit BaseScreen(VerseUnfoldSDLApp* app) : app(app) {}
    virtual ~BaseScreen() = default;

    virtual void handleEvent(const SDL_Event& event) = 0;
    virtual void update() = 0;
    virtual void render(SDL_Renderer* renderer) = 0;

protected:
    VerseUnfoldSDLApp* app;
};

} // namespace VerseUnfold
