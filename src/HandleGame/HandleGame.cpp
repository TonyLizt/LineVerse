#include "HandleGame.hpp"

#include "core/HGCore.hpp"
#include "core/HGEngine.hpp"
#include "core/HGPinyinTables.hpp"
#include "UI/HGUI.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <vector>

namespace HandleGame {

static bool fileExists(const std::string& p) {
    std::error_code ec;
    return std::filesystem::exists(std::filesystem::path(p), ec);
}

static std::string pickFirstExisting(const std::vector<std::string>& candidates) {
    for (const auto& p : candidates) {
        if (fileExists(p)) return p;
    }
    return "";
}

int start() {
    return start(nullptr, nullptr);
}

int start(SDL_Window* externalWindow, SDL_Renderer* externalRenderer) {
    HGEngine engine;

    // data: 你指定的目录（按难度分别加载不同 tsv）
    const std::vector<std::string> easyCandidates = {
        "../../data/raw/HandleGame/idioms_easy.tsv"
    };
    const std::vector<std::string> normalCandidates = {
        "../../data/raw/HandleGame/idioms_normal.tsv"
    };
    const std::vector<std::string> hardCandidates = {
        "../../data/raw/HandleGame/idioms_hard.tsv"
    };

    const std::string easyPath = pickFirstExisting(easyCandidates);
    const std::string normalPath = pickFirstExisting(normalCandidates);
    const std::string hardPath = pickFirstExisting(hardCandidates);

    if (easyPath.empty() || normalPath.empty() || hardPath.empty()) {
        std::cerr << "[HandleGame] TSV not found. Expected under data/raw/HandleGame:\n"
                  << "  idioms_easy.tsv (or idioms_easy.tsv)\n"
                  << "  idioms_normal.tsv\n"
                  << "  idioms_hard.tsv\n";
        return -1;
    }

    HGCore core(engine);
    core.setLexiconPaths(easyPath, normalPath, hardPath);

    // 固定表（速查表用）
    const auto& initials = kStdInitials;
    const auto& finals   = kStdFinals;

    const std::vector<std::string> fontCandidates = {
        "assets/HandleGame/font/font3.ttf",
        "assets/HandleGame/font/font2.ttf",
        "assets/HandleGame/font/font1.ttf",
        "C:/Windows/Fonts/simkai.ttf",
        "C:/Windows/Fonts/msyh.ttc",
        "C:/Windows/Fonts/msyhbd.ttc",
        "C:/Windows/Fonts/simhei.ttf",
        "C:/Windows/Fonts/simsun.ttc"
    };

    HGUI gui;
    HGUIConfig cfg;
    cfg.windowTitle = "汉兜";
    cfg.width = 1200;
    cfg.height = 800;
    cfg.fontCandidates = fontCandidates;
    cfg.fontSize = 24;
    cfg.pinyinFontSize = 26;

    return gui.run(externalWindow, externalRenderer, core, initials, finals, cfg);
}

} // namespace HandleGame