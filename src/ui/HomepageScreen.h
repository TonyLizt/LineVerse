#pragma once

#include <filesystem>

namespace lineverse::ui {

enum class HomepageTargetGame {
    None,
    HandleGame,
    IdiomChainGame,
    PoetryRebuildGame,
    VerseUnfoldGame
};

enum class HomepageModeChoice {
    None,
    Idiom,
    Mixed
};

enum class HomepageDifficultyChoice {
    None,
    Easy,
    Medium,
    Hard
};

struct HomepageLaunchSelection {
    HomepageTargetGame targetGame = HomepageTargetGame::None;
    HomepageModeChoice mode = HomepageModeChoice::None;
    HomepageDifficultyChoice difficulty = HomepageDifficultyChoice::None;
};

class HomepageScreen {
public:
    static constexpr int kResultExit = 0;
    static constexpr int kResultLaunch = 1;


    explicit HomepageScreen(const std::filesystem::path& gifPath);
    int show(HomepageLaunchSelection& selection);

private:
    std::filesystem::path gifPath_;
};

} // namespace lineverse::ui
