#include <exception>
#include <future>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#endif

#include "PoetryRebuildGame/PoetryRebuildGame.h"
#include "IdiomChainGame/IdiomChainGame.h"
#include "VerseUnfoldGame/VerseUnfoldGame.h"
#include "HandleGame/HandleGame.hpp"

#include "ui/HomepageScreen.h"

namespace {

void showErrorDialog(const std::string& title, const std::string& message) {
#ifdef _WIN32
    MessageBoxA(nullptr, message.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
#else
    (void)title;
    (void)message;
#endif
}

lineverse::poetryrebuild::LevelMode toPoetryMode(
    lineverse::ui::HomepageModeChoice mode
) {
    switch (mode) {
    case lineverse::ui::HomepageModeChoice::Mixed:
        return lineverse::poetryrebuild::LevelMode::Mixed;

    case lineverse::ui::HomepageModeChoice::Idiom:
    case lineverse::ui::HomepageModeChoice::None:
    default:
        return lineverse::poetryrebuild::LevelMode::Idiom;
    }
}

lineverse::poetryrebuild::LevelDifficulty toPoetryDifficulty(
    lineverse::ui::HomepageDifficultyChoice difficulty
) {
    switch (difficulty) {
    case lineverse::ui::HomepageDifficultyChoice::Medium:
        return lineverse::poetryrebuild::LevelDifficulty::Medium;

    case lineverse::ui::HomepageDifficultyChoice::Hard:
        return lineverse::poetryrebuild::LevelDifficulty::Hard;

    case lineverse::ui::HomepageDifficultyChoice::Easy:
    case lineverse::ui::HomepageDifficultyChoice::None:
    default:
        return lineverse::poetryrebuild::LevelDifficulty::Easy;
    }
}

int runHandleGame() {
    return HandleGame::start();
}

int runPoetryRebuildGame(
    lineverse::poetryrebuild::PoetryRebuildGame& game,
    const lineverse::ui::HomepageLaunchSelection& selection
) {
    return game.start(
        toPoetryMode(selection.mode),
        toPoetryDifficulty(selection.difficulty)
    );
}

int runIdiomChainGame() {
    IdiomChainGame game;
    return game.start();
}

int runVerseUnfoldGame() {
    VerseUnfoldGame game;
    return game.start();
}

bool handleGameResult(
    const std::string& moduleName,
    int gameResult
) {
    if (gameResult == 0) {
        std::cout << "[" << moduleName << "] 退出程序。\n";
        return false;
    }

    if (gameResult == 1) {
        std::cout << "[" << moduleName << "] 返回主页面。\n";
        return true;
    }

    if (gameResult == -1) {
        const std::string message = "[" + moduleName + "] module failed.";
        std::cerr << message << '\n';
        showErrorDialog("LineVerse 运行失败", message);
        return false;
    }

    std::cout << "[" << moduleName << "] module return code: "
              << gameResult << '\n';

    return false;
}

} // namespace

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    try {
        const auto paths = lineverse::poetryrebuild::ProjectPaths::detect();

        lineverse::poetryrebuild::PoetryRebuildGame poetryGame;

        lineverse::ui::HomepageScreen homepage(
            paths.projectRoot / "assets" / "Homepage" / "image" / "bg.jpg"
        );

        while (true) {
            /*
             * 保留第一个 main.cpp 的逻辑：
             * 主页显示期间，后台预加载 PoetryRebuildGame。
             */
            auto preloadPromise = std::make_shared<std::promise<void>>();
            std::future<void> preloadFuture = preloadPromise->get_future();

            std::thread preloadThread(
                [&poetryGame, preloadPromise]() {
                    try {
                        poetryGame.prepareForPlay();
                        preloadPromise->set_value();
                    } catch (...) {
                        preloadPromise->set_exception(std::current_exception());
                    }
                }
            );

            lineverse::ui::HomepageLaunchSelection selection;
            const int homepageResult = homepage.show(selection);

            if (homepageResult != lineverse::ui::HomepageScreen::kResultLaunch) {
                if (preloadThread.joinable()) {
                    preloadThread.join();
                }
                return 0;
            }

            int gameResult = 0;

            switch (selection.targetGame) {
            case lineverse::ui::HomepageTargetGame::HandleGame:
                if (preloadThread.joinable()) {
                    preloadThread.join();
                }

                gameResult = runHandleGame();

                if (!handleGameResult("HandleGame", gameResult)) {
                    return gameResult == -1 ? 1 : 0;
                }

                break;

            case lineverse::ui::HomepageTargetGame::PoetryRebuildGame:
                /*
                 * 只有真正进入 PoetryRebuildGame 时，才读取预加载结果。
                 * 如果 prepareForPlay() 抛异常，这里会进入外层 catch。
                 */
                preloadFuture.get();

                if (preloadThread.joinable()) {
                    preloadThread.join();
                }

                gameResult = runPoetryRebuildGame(poetryGame, selection);

                if (!handleGameResult("PoetryRebuildGame", gameResult)) {
                    return gameResult == -1 ? 1 : 0;
                }

                break;

            case lineverse::ui::HomepageTargetGame::IdiomChainGame:
                if (preloadThread.joinable()) {
                    preloadThread.join();
                }

                gameResult = runIdiomChainGame();

                if (!handleGameResult("IdiomChainGame", gameResult)) {
                    return gameResult == -1 ? 1 : 0;
                }

                break;

            case lineverse::ui::HomepageTargetGame::VerseUnfoldGame:
                if (preloadThread.joinable()) {
                    preloadThread.join();
                }

                gameResult = runVerseUnfoldGame();

                if (!handleGameResult("VerseUnfoldGame", gameResult)) {
                    return gameResult == -1 ? 1 : 0;
                }

                break;

            case lineverse::ui::HomepageTargetGame::None:
            default:
                if (preloadThread.joinable()) {
                    preloadThread.join();
                }

                std::cout << "[LineVerse] 未选择有效游戏，返回主页面。\n";
                break;
            }
        }
    } catch (const std::exception& ex) {
        const std::string message =
            std::string("[LineVerse] startup failed: ") + ex.what();

        std::cerr << message << '\n';
        showErrorDialog("LineVerse 启动失败", message);

        return 1;
    } catch (...) {
        const std::string message = "[LineVerse] startup failed: unknown exception.";

        std::cerr << message << '\n';
        showErrorDialog("LineVerse 启动失败", message);

        return 1;
    }
}