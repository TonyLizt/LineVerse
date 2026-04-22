#pragma once

#include <memory>
#include <string>

struct SDL_Window;
struct SDL_Renderer;

#include "controller/PoetryGameController.h"

class VerseUnfoldGame {
public:
    VerseUnfoldGame();
    ~VerseUnfoldGame();

    int start();
    int start(SDL_Window* externalWindow, SDL_Renderer* externalRenderer);

private:
    std::shared_ptr<PoetryGameController> controller;
    std::string dbPath = "data/raw/verse_unfold_poetry_db.json";
};
