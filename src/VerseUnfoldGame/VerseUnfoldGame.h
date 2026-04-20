#pragma once

#include <memory>
#include <string>

#include "controller/PoetryGameController.h"

class VerseUnfoldGame {
public:
    VerseUnfoldGame();
    ~VerseUnfoldGame();

    int start();

private:
    std::shared_ptr<PoetryGameController> controller;
    std::string dbPath = "data/raw/verse_unfold_poetry_db.json";
};
