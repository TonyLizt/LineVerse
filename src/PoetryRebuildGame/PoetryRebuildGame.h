#pragma once

#include <filesystem>
#include <vector>

struct SDL_Window;
struct SDL_Renderer;

#include "config/ProjectPaths.h"
#include "index/IndexStore.h"
#include "level/LevelGenerator.h"
#include "model/PhraseInfo.h"
#include "preprocess/CorpusBuilder.h"
#include "preprocess/TextNormalizer.h"

namespace lineverse::poetryrebuild {

class PoetryRebuildGame {
public:
    void prepareForPlay();
    int start(LevelMode mode, LevelDifficulty difficulty);
    int start(SDL_Window* externalWindow, SDL_Renderer* externalRenderer,
              LevelMode mode, LevelDifficulty difficulty);

private:
    void loadAssets();

    bool canLoadPrebuiltCorpus() const;
    std::vector<PhraseInfo> loadPhraseListFromPrebuild(
        const std::filesystem::path& filePath,
        PhraseType explicitType
    ) const;

    void loadData();
    void initGame();

    void selfCheckIndex();
    void selfCheckCandidateEngine();
    void selfCheckOptimalSolver();
    void selfCheckGameRound();

    void gameLoop(LevelMode mode, LevelDifficulty difficulty);
    void gameLoop(SDL_Window* externalWindow, SDL_Renderer* externalRenderer,
                  LevelMode mode, LevelDifficulty difficulty);
    void saveResult();
    void cleanup();

private:
    ProjectPaths paths_;
    TextNormalizer normalizer_;
    BuildStats stats_{};

    std::vector<PhraseInfo> poems_;
    std::vector<PhraseInfo> idioms_;

    IndexStore indexStore_;
    bool prepared_ = false;
};

} // namespace lineverse::poetryrebuild
