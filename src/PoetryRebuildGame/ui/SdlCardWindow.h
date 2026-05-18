#pragma once

#include <functional>
#include <string>
#include <vector>

struct SDL_Window;
struct SDL_Renderer;

#include "../common/Types.h"
#include "../config/ProjectPaths.h"

namespace lineverse::poetryrebuild {

class SdlCardWindow {
public:
    static constexpr int kResultBackToHome = 1;
    static constexpr int kResultReplaySameDifficulty = 2;

    struct PreparedRoundData {
        CharFreq poolFreq;
        std::vector<std::string> validAnswers;
        std::vector<std::string> optimalAnswers;
        int hintLimit = 0;
        int initialTimeSeconds = 60;

        int bgR = 249;
        int bgG = 254;
        int bgB = 255;
    };

    explicit SdlCardWindow(const ProjectPaths& paths);

    int run(
        const CharFreq& poolFreq,
        const std::string& windowTitle,
        const std::vector<std::string>& validAnswers,
        const std::vector<std::string>& optimalAnswers,
        int hintLimit,
        int initialTimeSeconds
    );

    int run(
        SDL_Window* externalWindow,
        SDL_Renderer* externalRenderer,
        const CharFreq& poolFreq,
        const std::string& windowTitle,
        const std::vector<std::string>& validAnswers,
        const std::vector<std::string>& optimalAnswers,
        int hintLimit,
        int initialTimeSeconds
    );

    int runWithLoading(
        const std::string& windowTitle,
        const std::function<PreparedRoundData()>& builder
    );

    int runWithLoading(
        SDL_Window* externalWindow,
        SDL_Renderer* externalRenderer,
        const std::string& windowTitle,
        const std::function<PreparedRoundData()>& builder
    );

private:
    ProjectPaths paths_;

    std::vector<std::string> expandPoolChars(const CharFreq& poolFreq) const;
    void splitRows(int totalCount, int& firstRowCount, int& secondRowCount) const;
};

} // namespace lineverse::poetryrebuild