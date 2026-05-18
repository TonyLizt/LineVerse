#include "TerminalGameRunner.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <unordered_set>

#include "../util/Utf8.h"

namespace lineverse::poetryrebuild {

namespace {

CharFreq mergeFreq(const CharFreq& a, const CharFreq& b) {
    CharFreq merged = a;
    for (const auto& [cp, cnt] : b) {
        merged[cp] += cnt;
    }
    return merged;
}

int totalFreqCount(const CharFreq& freq) {
    int total = 0;
    for (const auto& [_, cnt] : freq) {
        total += cnt;
    }
    return total;
}

} // namespace

TerminalGameRunner::TerminalGameRunner(const IndexStore& indexStore)
    : indexStore_(indexStore),
      hintEngine_(indexStore),
      rng_(std::random_device{}()) {
}

void TerminalGameRunner::run(const std::vector<PhraseInfo>& poems, const std::vector<PhraseInfo>& idioms) {
    CandidateEngine engine(indexStore_);
    OptimalSolver solver(indexStore_);

    while (true) {
        printMainMenu();

        std::string input;
        std::getline(std::cin, input);
        if (!std::cin) {
            return;
        }

        if (input == "0" || input == "q" || input == "quit") {
            std::cout << "[Terminal] 已退出终端试玩。\n";
            return;
        }

        if (input == "1") {
            DifficultyLevel difficulty{};
            if (!chooseDifficulty("成语练习", difficulty)) {
                continue;
            }

            GameRoundConfig config;
            switch (difficulty) {
            case DifficultyLevel::Easy:
                config.initialTime = 150;
                config.comboWindowSeconds = 12;
                break;
            case DifficultyLevel::Medium:
                config.initialTime = 120;
                config.comboWindowSeconds = 8;
                break;
            case DifficultyLevel::Hard:
                config.initialTime = 90;
                config.comboWindowSeconds = 5;
                break;
            }

            config.recomputeCandidatesOnSubmit = true;
            config.recomputeOptimalOnSubmit = true;

            GameRound round(indexStore_, engine, solver, config);
            std::vector<std::string> seedIds;
            int hintCount = 0;

            if (!prepareIdiomRound(idioms, difficulty, engine, solver, round, seedIds, hintCount)) {
                std::cout << "[Terminal] 无法生成成语练习局，请重试。\n";
                continue;
            }

            playRound(round, "成语练习", difficulty, seedIds, hintCount);
            continue;
        }

        if (input == "2") {
            DifficultyLevel difficulty{};
            if (!chooseDifficulty("混合练习", difficulty)) {
                continue;
            }

            GameRoundConfig config;
            switch (difficulty) {
            case DifficultyLevel::Easy:
                config.initialTime = 180;
                config.comboWindowSeconds = 12;
                break;
            case DifficultyLevel::Medium:
                config.initialTime = 150;
                config.comboWindowSeconds = 8;
                break;
            case DifficultyLevel::Hard:
                config.initialTime = 120;
                config.comboWindowSeconds = 5;
                break;
            }

            config.recomputeCandidatesOnSubmit = true;
            config.recomputeOptimalOnSubmit = true;

            GameRound round(indexStore_, engine, solver, config);
            std::vector<std::string> seedIds;
            int hintCount = 0;

            if (!prepareMixedRound(poems, idioms, difficulty, engine, solver, round, seedIds, hintCount)) {
                std::cout << "[Terminal] 无法生成混合练习局，请重试。\n";
                continue;
            }

            playRound(round, "混合练习", difficulty, seedIds, hintCount);
            continue;
        }

        std::cout << "[Terminal] 无效选项，请输入 1 / 2 / 0。\n";
    }
}

void TerminalGameRunner::printMainMenu() const {
    std::cout << "\n========== PoetryRebuildGame Terminal ==========\n";
    std::cout << "1. 成语练习\n";
    std::cout << "2. 混合练习（诗句 + 成语）\n";
    std::cout << "0. 退出程序\n";
    std::cout << "请选择模式: ";
}

void TerminalGameRunner::printDifficultyMenu(const std::string& modeName) const {
    std::cout << "\n========== " << modeName << " / 选择难度 ==========\n";
    std::cout << "1. 简单\n";
    std::cout << "2. 中等\n";
    std::cout << "3. 困难\n";
    std::cout << "b. 返回上一页\n";
    std::cout << "请选择难度: ";
}

void TerminalGameRunner::printCommands() const {
    std::cout << "\n可用命令：\n";
    std::cout << "  直接输入答案         -> 提交答案\n";
    std::cout << "  /help               -> 查看帮助\n";
    std::cout << "  /pool               -> 查看剩余字池\n";
    std::cout << "  /status             -> 查看当前状态\n";
    std::cout << "  /best               -> 查看当前最优解集合（调试用）\n";
    std::cout << "  /hint first         -> 渐进式首字提示（同一答案逐字揭示）\n";
    std::cout << "  /hint len           -> 长度提示\n";
    std::cout << "  /hint type          -> 类型提示\n";
    std::cout << "  /hint best          -> 高价值答案提示\n";
    std::cout << "  /hint next          -> 推荐下一步\n";
    std::cout << "  /hint all           -> 显示当前所有可选正确答案\n";
    std::cout << "  /giveup             -> 放弃本局并揭示答案\n";
    std::cout << "  /quit               -> 退出本局返回主菜单\n";
}

std::string TerminalGameRunner::difficultyToString(DifficultyLevel difficulty) const {
    switch (difficulty) {
    case DifficultyLevel::Easy:
        return "简单";
    case DifficultyLevel::Medium:
        return "中等";
    case DifficultyLevel::Hard:
        return "困难";
    }
    return "未知";
}

std::string TerminalGameRunner::formatPool(const CharFreq& freq) const {
    std::vector<std::pair<CodePoint, int>> rows(freq.begin(), freq.end());
    std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });

    std::string out;
    for (std::size_t i = 0; i < rows.size(); ++i) {
        if (i > 0) {
            out += " ";
        }
        out += Utf8::fromCodePoint(rows[i].first);
        out += "x";
        out += std::to_string(rows[i].second);
    }
    return out;
}

std::vector<std::string> TerminalGameRunner::idsToTexts(const std::vector<std::string>& ids) const {
    std::vector<std::string> texts;
    texts.reserve(ids.size());

    for (const auto& id : ids) {
        const PhraseInfo* phrase = indexStore_.getPhraseById(id);
        if (phrase != nullptr) {
            texts.push_back(phrase->text);
        }
    }
    return texts;
}

std::vector<const PhraseInfo*> TerminalGameRunner::buildIdiomPool(const std::vector<PhraseInfo>& idioms) const {
    std::vector<const PhraseInfo*> pool;
    pool.reserve(idioms.size());

    for (const auto& item : idioms) {
        if (item.length == 4) {
            pool.push_back(&item);
        }
    }
    return pool;
}

std::vector<const PhraseInfo*> TerminalGameRunner::buildPoemPool(const std::vector<PhraseInfo>& poems) const {
    std::vector<const PhraseInfo*> pool;
    pool.reserve(poems.size());

    for (const auto& item : poems) {
        if (item.length == 5 || item.length == 7) {
            pool.push_back(&item);
        }
    }
    return pool;
}

std::vector<const PhraseInfo*> TerminalGameRunner::sampleDistinct(
    const std::vector<const PhraseInfo*>& source,
    std::size_t count
) {
    if (source.size() < count) {
        return {};
    }

    std::vector<const PhraseInfo*> shuffled = source;
    std::shuffle(shuffled.begin(), shuffled.end(), rng_);

    std::vector<const PhraseInfo*> picked;
    std::unordered_set<std::string> usedText;

    for (const auto* item : shuffled) {
        if (item == nullptr) {
            continue;
        }
        if (!usedText.insert(item->text).second) {
            continue;
        }
        picked.push_back(item);
        if (picked.size() >= count) {
            break;
        }
    }

    if (picked.size() < count) {
        return {};
    }
    return picked;
}

bool TerminalGameRunner::chooseDifficulty(const std::string& modeName, DifficultyLevel& difficulty) const {
    while (true) {
        printDifficultyMenu(modeName);

        std::string input;
        std::getline(std::cin, input);
        if (!std::cin) {
            return false;
        }

        if (input == "b" || input == "back") {
            return false;
        }

        if (input == "1") {
            difficulty = DifficultyLevel::Easy;
            return true;
        }
        if (input == "2") {
            difficulty = DifficultyLevel::Medium;
            return true;
        }
        if (input == "3") {
            difficulty = DifficultyLevel::Hard;
            return true;
        }

        std::cout << "[Terminal] 这里只能选择 1 / 2 / 3，或输入 b 返回上一页。\n";
    }
}

bool TerminalGameRunner::prepareIdiomRound(
    const std::vector<PhraseInfo>& idioms,
    DifficultyLevel difficulty,
    CandidateEngine& engine,
    OptimalSolver& solver,
    GameRound& round,
    std::vector<std::string>& seedIds,
    int& hintCount
) {
    (void)engine;
    (void)solver;

    const auto idiomPool = buildIdiomPool(idioms);

    CandidateQueryOptions options;
    options.minLen = 4;
    options.maxLen = 4;
    options.allowPoem = false;
    options.allowIdiom = true;

    std::size_t seedCount = 3;
    int minA = 3, maxA = 80, minB = 2, maxB = 8;
    switch (difficulty) {
    case DifficultyLevel::Easy:
        seedCount = 2;
        minA = 2; maxA = 20; minB = 1; maxB = 4;
        hintCount = 5;
        break;
    case DifficultyLevel::Medium:
        seedCount = 3;
        minA = 6; maxA = 60; minB = 2; maxB = 6;
        hintCount = 3;
        break;
    case DifficultyLevel::Hard:
        seedCount = 4;
        minA = 8; maxA = 180; minB = 2; maxB = 10;
        hintCount = 2;
        break;
    }

    for (int attempt = 0; attempt < 500; ++attempt) {
        seedIds.clear();

        const auto picked = sampleDistinct(idiomPool, seedCount);
        if (picked.size() != seedCount) {
            return false;
        }

        GamePool pool;
        for (const auto* item : picked) {
            pool.originFreq = mergeFreq(pool.originFreq, item->charFreq);
            seedIds.push_back(item->id);
        }
        pool.remainFreq = pool.originFreq;
        pool.totalChars = totalFreqCount(pool.originFreq);

        round.createGameRound(pool, options);

        const GameState& s = round.state();
        if (s.candidateCount >= minA && s.candidateCount <= maxA &&
            s.optimalMax >= minB && s.optimalMax <= maxB) {
            return true;
        }
    }

    seedIds.clear();
    return false;
}

bool TerminalGameRunner::prepareMixedRound(
    const std::vector<PhraseInfo>& poems,
    const std::vector<PhraseInfo>& idioms,
    DifficultyLevel difficulty,
    CandidateEngine& engine,
    OptimalSolver& solver,
    GameRound& round,
    std::vector<std::string>& seedIds,
    int& hintCount
) {
    (void)engine;
    (void)solver;

    const auto idiomPool = buildIdiomPool(idioms);
    const auto poemPool = buildPoemPool(poems);

    CandidateQueryOptions options;
    options.minLen = 4;
    options.maxLen = 7;
    options.allowPoem = true;
    options.allowIdiom = true;

    std::size_t idiomCount = 2;
    std::size_t poemCount = 1;
    int minA = 3, maxA = 120, minB = 2, maxB = 8;
    switch (difficulty) {
    case DifficultyLevel::Easy:
        idiomCount = 1;
        poemCount = 1;
        minA = 2; maxA = 30; minB = 1; maxB = 4;
        hintCount = 5;
        break;
    case DifficultyLevel::Medium:
        idiomCount = 2;
        poemCount = 1;
        minA = 5; maxA = 80; minB = 2; maxB = 6;
        hintCount = 3;
        break;
    case DifficultyLevel::Hard:
        idiomCount = 2;
        poemCount = 2;
        minA = 10; maxA = 160; minB = 3; maxB = 8;
        hintCount = 2;
        break;
    }

    for (int attempt = 0; attempt < 220; ++attempt) {
        seedIds.clear();

        const auto pickedIdioms = sampleDistinct(idiomPool, idiomCount);
        const auto pickedPoems = sampleDistinct(poemPool, poemCount);

        if (pickedIdioms.size() != idiomCount || pickedPoems.size() != poemCount) {
            return false;
        }

        GamePool pool;

        for (const auto* item : pickedIdioms) {
            pool.originFreq = mergeFreq(pool.originFreq, item->charFreq);
            seedIds.push_back(item->id);
        }
        for (const auto* item : pickedPoems) {
            pool.originFreq = mergeFreq(pool.originFreq, item->charFreq);
            seedIds.push_back(item->id);
        }

        pool.remainFreq = pool.originFreq;
        pool.totalChars = totalFreqCount(pool.originFreq);

        round.createGameRound(pool, options);

        const GameState& s = round.state();
        if (s.candidateCount >= minA && s.candidateCount <= maxA &&
            s.optimalMax >= minB && s.optimalMax <= maxB) {
            return true;
        }
    }

    seedIds.clear();
    return false;
}

void TerminalGameRunner::playRound(
    GameRound& round,
    const std::string& modeName,
    DifficultyLevel difficulty,
    const std::vector<std::string>& seedIds,
    int hintCount
) {
    printCommands();

    std::cout << "\n[Terminal] 已进入 " << modeName
              << " / 难度：" << difficultyToString(difficulty) << "\n";
    printStatus(round, hintCount);
    std::cout << "[Terminal] 初始字池: " << formatPool(round.state().pool.remainFreq) << "\n";

    const int totalTime = round.state().timeLeft;
    const auto start = std::chrono::steady_clock::now();
    bool gaveUp = false;

    while (true) {
        const auto now = std::chrono::steady_clock::now();
        const int elapsed = static_cast<int>(
            std::chrono::duration_cast<std::chrono::seconds>(now - start).count()
        );
        const int remain = std::max(0, totalTime - elapsed);
        round.setTimeLeft(remain);

        if (round.isGameOver()) {
            break;
        }

        std::cout << "\n[" << modeName
                  << " / " << difficultyToString(difficulty)
                  << " | 剩余时间 " << round.state().timeLeft
                  << "s | 分数 " << round.state().score
                  << " | Combo " << round.state().combo
                  << " x" << round.currentComboMultiplier()
                  << " | 已找到 " << round.state().foundSet.size()
                  << " | 提示 " << hintCount
                  << "] > ";

        std::string input;
        std::getline(std::cin, input);
        if (!std::cin) {
            std::cout << "\n[Terminal] 输入流结束，返回主菜单。\n";
            return;
        }

        const auto afterInput = std::chrono::steady_clock::now();
        const int elapsedAfterInput = static_cast<int>(
            std::chrono::duration_cast<std::chrono::seconds>(afterInput - start).count()
        );
        round.setTimeLeft(std::max(0, totalTime - elapsedAfterInput));

        if (round.state().timeLeft <= 0) {
            break;
        }

        if (input == "/help") {
            printCommands();
            continue;
        }

        if (input == "/pool") {
            std::cout << "[Pool] " << formatPool(round.state().pool.remainFreq) << "\n";
            continue;
        }

        if (input == "/status") {
            printStatus(round, hintCount);
            continue;
        }

        if (input == "/best") {
            printBestSet(round);
            continue;
        }

        if (input == "/giveup") {
            std::cout << "[Terminal] 你已放弃本局。\n";
            gaveUp = true;
            break;
        }

        if (input == "/quit") {
            std::cout << "[Terminal] 返回主菜单。\n";
            return;
        }

        if (input == "/hint first" || input == "/hint len" ||
            input == "/hint type" || input == "/hint best" ||
            input == "/hint next" || input == "/hint all") {

            HintResult hintResult;

            if (input == "/hint first") {
                hintResult = hintEngine_.getHintByFirstChar(round);
            } else if (input == "/hint len") {
                hintResult = hintEngine_.getHintByLength(round);
            } else if (input == "/hint type") {
                hintResult = hintEngine_.getHintByType(round);
            } else if (input == "/hint best") {
                hintResult = hintEngine_.getMostValuableRemainingAnswer(round);
            } else if (input == "/hint next") {
                hintResult = hintEngine_.suggestNextBestMove(round);
            } else {
                hintResult = hintEngine_.getAllRemainingAnswers(round);
            }

            if (hintResult.consumeCount && hintCount <= 0) {
                std::cout << "[Hint] 提示次数已用尽。\n";
                continue;
            }

            if (hintResult.consumeCount) {
                --hintCount;
            }

            std::cout << "[Hint] " << hintResult.text << "\n";
            std::cout << "[Hint] 剩余提示次数：" << hintCount << "\n";
            continue;
        }

        const SubmitResult submit = round.submitAnswer(input);
        std::cout << "[Submit] " << submit.message;

        if (submit.accepted) {
            std::cout << " | +" << submit.gainedScore
                      << " 分 | 当前分数 " << submit.scoreAfterSubmit
                      << " | Combo " << submit.comboAfterSubmit
                      << " x" << round.currentComboMultiplier();
        }
        std::cout << "\n";

        if (round.isGameOver()) {
            break;
        }
    }

    std::cout << "\n========== 本局结束 ==========\n";
    printStatus(round, hintCount);

    const auto found = round.state().submitHistory;
    if (!found.empty()) {
        std::cout << "你找到的答案：\n";
        for (const auto& x : found) {
            std::cout << "  - " << x << "\n";
        }
    } else {
        std::cout << "你本局没有提交成功任何答案。\n";
    }

    std::cout << "与最优解差距: " << round.calcGapToOptimal() << "\n";

    std::cout << "最优解集合：\n";
    const auto bestTexts = idsToTexts(round.getBestSolutionSet());
    for (const auto& x : bestTexts) {
        std::cout << "  - " << x << "\n";
    }

    if (gaveUp) {
        std::cout << "放弃时当前所有可选正确答案：\n";
        const HintResult allAnswers = hintEngine_.getAllRemainingAnswers(round);
        std::cout << allAnswers.text << "\n";
    }

    if (!seedIds.empty()) {
        std::cout << "出题种子答案（调试信息）：\n";
        const auto seedTexts = idsToTexts(seedIds);
        for (const auto& x : seedTexts) {
            std::cout << "  - " << x << "\n";
        }
    }

    std::cout << "==============================\n";
}

void TerminalGameRunner::printStatus(const GameRound& round, int hintCount) const {
    const GameState& s = round.state();
    std::cout << "[Status] 分数=" << s.score
              << ", Combo=" << s.combo
              << ", Multiplier=x" << round.currentComboMultiplier()
              << ", MaxCombo=" << s.maxCombo
              << ", TimeLeft=" << s.timeLeft
              << ", 已找到=" << s.foundSet.size()
              << ", A=" << s.candidateCount
              << ", B=" << s.optimalMax
              << ", Gap=" << round.calcGapToOptimal()
              << ", Hint=" << hintCount
              << "\n";
}

void TerminalGameRunner::printBestSet(const GameRound& round) const {
    const auto texts = idsToTexts(round.getBestSolutionSet());
    if (texts.empty()) {
        std::cout << "[Best] 当前最优解集合为空。\n";
        return;
    }

    std::cout << "[Best] 当前最优解集合：\n";
    for (const auto& x : texts) {
        std::cout << "  - " << x << "\n";
    }
}

} // namespace lineverse::poetryrebuild