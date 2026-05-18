#include "PoetryRebuildGame.h"
#include "bootstrap/RuntimeBootstrap.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "candidate/CandidateEngine.h"
#include "common/Json.h"
#include "game/GameRound.h"
#include "level/LevelGenerator.h"
#include "resource/ResourceLoader.h"
#include "solver/OptimalSolver.h"
#include "ui/SdlCardWindow.h"
#include "util/Utf8.h"

namespace fs = std::filesystem;

namespace lineverse::poetryrebuild {

namespace {

void assertTrue(bool cond, const std::string& msg) {
    if (!cond) {
        throw std::runtime_error(msg);
    }
}

CharFreq mergeFreq(const CharFreq& a, const CharFreq& b) {
    CharFreq merged = a;
    for (const auto& [cp, count] : b) {
        merged[cp] += count;
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

Json readJsonFileLocal(const fs::path& filePath) {
    std::ifstream input(filePath, std::ios::binary);
    if (!input) {
        throw std::runtime_error("无法打开 JSON 文件: " + filePath.string());
    }

    Json data;
    input >> data;
    return data;
}

PhraseType parsePhraseType(const std::string& typeText, PhraseType fallback) {
    if (typeText == "idiom") {
        return PhraseType::Idiom;
    }
    if (typeText == "poem") {
        return PhraseType::Poem;
    }
    return fallback;
}

} // namespace

void PoetryRebuildGame::prepareForPlay() {
    if (prepared_) {
        return;
    }

    cleanup();

    loadAssets();
    RuntimeBootstrap::prepare();

    if (!RuntimeBootstrap::loadRuntimeCache(paths_, poems_, idioms_, indexStore_)) {
        throw std::runtime_error("runtime cache missing or invalid");
    }

#ifndef NDEBUG
    selfCheckIndex();
    selfCheckCandidateEngine();
    selfCheckOptimalSolver();
    selfCheckGameRound();
#endif

    std::cout << "[Preview] 清洗结果统计：\n"
              << "  最终诗句数: " << poems_.size() << '\n'
              << "  最终成语数: " << idioms_.size() << '\n';

    prepared_ = true;
}

int PoetryRebuildGame::start(LevelMode mode, LevelDifficulty difficulty) {
    return start(nullptr, nullptr, mode, difficulty);
}

int PoetryRebuildGame::start(SDL_Window* externalWindow, SDL_Renderer* externalRenderer,
                             LevelMode mode, LevelDifficulty difficulty) {
    try {
        if (!prepared_) {
            prepareForPlay();
        }

        gameLoop(externalWindow, externalRenderer, mode, difficulty);
        saveResult();
        cleanup();
        prepared_ = false;
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "[PoetryRebuildGame] 启动失败: " << ex.what() << '\n';
        cleanup();
        prepared_ = false;
        return -1;
    }
}

void PoetryRebuildGame::loadAssets() {
    paths_ = ProjectPaths::detect();
    paths_.ensureOutputDirectories();

    std::cout << "[Step1] 已锁定项目目录：\n"
              << "  projectRoot: " << paths_.projectRoot << '\n'
              << "  prebuildRoot: " << paths_.modulePrebuildRoot << '\n'
              << "  cacheRoot: " << paths_.moduleCacheRoot << '\n';

    if (fs::exists(paths_.tangPoetryRoot) && fs::exists(paths_.idiomRoot)) {
        std::cout << "[Step1] 检测到原始数据目录：\n"
                  << "  全唐诗: " << paths_.tangPoetryRoot << '\n'
                  << "  成语库: " << paths_.idiomRoot << '\n';
    } else {
        std::cout << "[Step1] 未检测到完整原始数据目录，运行时将优先使用 prebuild/runtime cache。\n";
    }
}

bool PoetryRebuildGame::canLoadPrebuiltCorpus() const {
    const fs::path poemsFile = paths_.modulePrebuildRoot / "poems.json";
    const fs::path idiomsFile = paths_.modulePrebuildRoot / "idioms.json";
    return fs::exists(poemsFile) && fs::exists(idiomsFile);
}

std::vector<PhraseInfo> PoetryRebuildGame::loadPhraseListFromPrebuild(
    const fs::path& filePath,
    PhraseType explicitType
) const {
    Json root = readJsonFileLocal(filePath);
    Json items = Json::array();

    if (root.is_object() && root.contains("items") && root["items"].is_array()) {
        items = root["items"];
    } else if (root.is_array()) {
        items = root;
    } else {
        throw std::runtime_error("预构建文件格式不正确: " + filePath.string());
    }

    std::vector<PhraseInfo> phrases;
    phrases.reserve(items.size());

    for (const auto& item : items) {
        if (!item.is_object()) {
            continue;
        }

        PhraseInfo info;
        info.text = item.value("text", "");
        if (info.text.empty()) {
            continue;
        }

        info.type = parsePhraseType(item.value("type", ""), explicitType);
        info.id = item.value(
            "id",
            normalizer_.generateInternalId(
                (info.type == PhraseType::Poem ? "poem" : "idiom"),
                info.text,
                item.value("author", ""),
                item.value("title", "")
            )
        );

        info.author = item.value("author", "");
        info.title = item.value("title", "");
        info.pinyin = item.value("pinyin", "");
        info.explanation = item.value("explanation", "");
        info.derivation = item.value("derivation", "");
        info.example = item.value("example", "");
        info.sourceFile = item.value("sourceFile", item.value("_source_file", ""));

        info.length = item.value("length", normalizer_.countChineseChars(info.text));
        info.charFreq = normalizer_.buildCharFreq(info.text);
        info.rarity = item.value("rarity", 1);
        info.scoreWeight = item.value("scoreWeight", info.length);

        phrases.push_back(std::move(info));
    }

    return phrases;
}

void PoetryRebuildGame::loadData() {
    stats_ = BuildStats{};

    const fs::path poemsFile = paths_.modulePrebuildRoot / "poems.json";
    const fs::path idiomsFile = paths_.modulePrebuildRoot / "idioms.json";

    if (canLoadPrebuiltCorpus()) {
        std::cout << "[Step2] 检测到预构建语料，跳过原始清洗，直接从 prebuild 载入...\n";

        poems_ = loadPhraseListFromPrebuild(poemsFile, PhraseType::Poem);
        idioms_ = loadPhraseListFromPrebuild(idiomsFile, PhraseType::Idiom);

        stats_.finalPoemCount = static_cast<int>(poems_.size());
        stats_.finalIdiomCount = static_cast<int>(idioms_.size());

        std::cout << "[Step2] 预构建载入完成：\n"
                  << "  poems.json  -> " << poemsFile << "，结果数: " << poems_.size() << "\n"
                  << "  idioms.json -> " << idiomsFile << "，结果数: " << idioms_.size() << "\n";
        return;
    }

    ResourceLoader loader(paths_);
    CorpusBuilder builder(normalizer_);

    std::cout << "[Step2] 开始读取全唐诗原始数据..." << std::endl;
    auto rawPoems = loader.loadTangPoemEntries();
    std::cout << "[Step2] 全唐诗原始条目数: " << rawPoems.size() << std::endl;

    std::cout << "[Step2] 开始读取成语库原始数据..." << std::endl;
    auto rawIdioms = loader.loadIdiomEntries();
    std::cout << "[Step2] 成语原始条目数: " << rawIdioms.size() << std::endl;

    std::cout << "[Step2] 开始清洗诗句..." << std::endl;
    poems_ = builder.buildPoemCorpus(rawPoems, stats_);
    std::cout << "[Step2] 诗句清洗完成，结果数: " << poems_.size() << std::endl;

    std::cout << "[Step2] 开始清洗成语..." << std::endl;
    idioms_ = builder.buildIdiomCorpus(rawIdioms, stats_);
    std::cout << "[Step2] 成语清洗完成，结果数: " << idioms_.size() << std::endl;

    std::cout << "[Step2] 开始写出清洗结果..." << std::endl;
    builder.writePhraseList(poems_, poemsFile);
    builder.writePhraseList(idioms_, idiomsFile);
    builder.writeBuildReport(stats_, paths_.moduleCacheRoot / "build_report.json");

    std::cout << "[Step2] 数据清洗完成：\n"
              << "  poems.json  -> " << poemsFile << '\n'
              << "  idioms.json -> " << idiomsFile << '\n';
}

void PoetryRebuildGame::initGame() {
    indexStore_.build(poems_, idioms_);
    indexStore_.writeIndexReport(paths_.moduleCacheRoot / "index_report.json");

    std::cout << "[Step3] IndexStore 建立完成：\n"
              << "  phraseMap/textToId 总数: " << indexStore_.phraseCount() << "\n"
              << "  Trie 词条数: " << indexStore_.trie().size() << "\n"
              << "  跳过重复 text 数: " << indexStore_.skippedDuplicateTextCount() << "\n"
              << "  conflictGraph 是否建立: "
              << (indexStore_.conflictGraphBuilt() ? "yes" : "no") << "\n";

    const auto& samples = indexStore_.duplicateTextSamples();
    if (!samples.empty()) {
        std::cout << "  重复 text 样例: ";
        for (std::size_t i = 0; i < samples.size(); ++i) {
            if (i > 0) {
                std::cout << ", ";
            }
            std::cout << samples[i];
        }
        std::cout << "\n";
    }

    std::cout << "  index_report.json -> "
              << (paths_.moduleCacheRoot / "index_report.json") << "\n";

    selfCheckIndex();
    selfCheckCandidateEngine();
    selfCheckOptimalSolver();
    selfCheckGameRound();
}

void PoetryRebuildGame::selfCheckIndex() {
    assertTrue(!idioms_.empty(), "[Step3 self-check] idioms_ is empty");
    assertTrue(indexStore_.phraseCount() > 0, "[Step3 self-check] index is empty");

    const PhraseInfo& sample = idioms_.front();

    assertTrue(indexStore_.hasText(sample.text), "[Step3 self-check] hasText failed: " + sample.text);
    assertTrue(indexStore_.containsPhrase(sample.text), "[Step3 self-check] containsPhrase failed: " + sample.text);

    const std::string* foundId = indexStore_.getIdByText(sample.text);
    assertTrue(foundId != nullptr, "[Step3 self-check] getIdByText returned null: " + sample.text);
    assertTrue(*foundId == sample.id, "[Step3 self-check] getIdByText returned wrong id");

    const PhraseInfo* phrase = indexStore_.getPhraseById(sample.id);
    assertTrue(phrase != nullptr, "[Step3 self-check] getPhraseById returned null");
    assertTrue(phrase->text == sample.text, "[Step3 self-check] getPhraseById returned wrong phrase");

    const auto cps = Utf8::toCodePoints(sample.text);
    assertTrue(!cps.empty(), "[Step3 self-check] sample text code points empty");

    CodePointList prefixCps;
    prefixCps.push_back(cps.front());
    const std::string prefix1 = Utf8::fromCodePoints(prefixCps);
    assertTrue(indexStore_.isValidPrefix(prefix1), "[Step3 self-check] isValidPrefix failed: " + prefix1);

    if (cps.size() >= 2) {
        prefixCps.push_back(cps[1]);
        const std::string prefix2 = Utf8::fromCodePoints(prefixCps);
        assertTrue(indexStore_.isValidPrefix(prefix2), "[Step3 self-check] isValidPrefix failed: " + prefix2);

        const auto completed = indexStore_.completePrefix(prefix2, 20);
        assertTrue(
            std::find(completed.begin(), completed.end(), sample.text) != completed.end(),
            "[Step3 self-check] completePrefix failed to include sample text"
        );
    }

    auto idsByChar = indexStore_.getPhraseIdsByChar(cps.front());
    assertTrue(
        std::find(idsByChar.begin(), idsByChar.end(), sample.id) != idsByChar.end(),
        "[Step3 self-check] getPhraseIdsByChar failed"
    );

    auto idsByLength = indexStore_.getPhraseIdsByLength(sample.length);
    assertTrue(
        std::find(idsByLength.begin(), idsByLength.end(), sample.id) != idsByLength.end(),
        "[Step3 self-check] getPhraseIdsByLength failed"
    );

    auto idsByType = indexStore_.getPhraseIdsByType(PhraseType::Idiom);
    assertTrue(
        std::find(idsByType.begin(), idsByType.end(), sample.id) != idsByType.end(),
        "[Step3 self-check] getPhraseIdsByType failed"
    );

    std::cout << "[Step3 self-check] PASS\n";
}

void PoetryRebuildGame::selfCheckCandidateEngine() {
    assertTrue(!poems_.empty(), "[Step4 self-check] poems_ is empty");
    assertTrue(!idioms_.empty(), "[Step4 self-check] idioms_ is empty");

    CandidateEngine engine(indexStore_);

    const PhraseInfo& poemSample = poems_.front();
    const PhraseInfo& idiomSample = idioms_.front();

    {
        CandidateQueryOptions options;
        options.minLen = idiomSample.length;
        options.maxLen = idiomSample.length;
        options.allowPoem = false;
        options.allowIdiom = true;

        const auto ids = engine.getCandidates(idiomSample.charFreq, options);
        assertTrue(
            std::find(ids.begin(), ids.end(), idiomSample.id) != ids.end(),
            "[Step4 self-check] idiom candidate missing"
        );

        const auto texts = engine.getCandidateTexts(ids);
        assertTrue(
            std::find(texts.begin(), texts.end(), idiomSample.text) != texts.end(),
            "[Step4 self-check] idiom candidate text missing"
        );

        assertTrue(engine.countCandidates(idiomSample.charFreq) >= 1,
                   "[Step4 self-check] idiom candidate count invalid");
    }

    {
        CandidateQueryOptions options;
        options.minLen = poemSample.length;
        options.maxLen = poemSample.length;
        options.allowPoem = true;
        options.allowIdiom = false;

        const auto ids = engine.getCandidates(poemSample.charFreq, options);
        assertTrue(
            std::find(ids.begin(), ids.end(), poemSample.id) != ids.end(),
            "[Step4 self-check] poem candidate missing"
        );

        const auto texts = engine.getCandidateTexts(ids);
        assertTrue(
            std::find(texts.begin(), texts.end(), poemSample.text) != texts.end(),
            "[Step4 self-check] poem candidate text missing"
        );
    }

    {
        const CharFreq mixedPool = mergeFreq(poemSample.charFreq, idiomSample.charFreq);
        const auto initial = engine.getInitialCandidates(mixedPool);
        assertTrue(
            std::find(initial.begin(), initial.end(), poemSample.id) != initial.end(),
            "[Step4 self-check] mixed pool missing poem"
        );
        assertTrue(
            std::find(initial.begin(), initial.end(), idiomSample.id) != initial.end(),
            "[Step4 self-check] mixed pool missing idiom"
        );

        const auto onlyIdioms = engine.filterByType(initial, false, true);
        assertTrue(
            std::find(onlyIdioms.begin(), onlyIdioms.end(), idiomSample.id) != onlyIdioms.end(),
            "[Step4 self-check] filterByType idiom failed"
        );

        const auto onlyPoems = engine.filterByType(initial, true, false);
        assertTrue(
            std::find(onlyPoems.begin(), onlyPoems.end(), poemSample.id) != onlyPoems.end(),
            "[Step4 self-check] filterByType poem failed"
        );
    }

    std::cout << "[Step4 self-check] PASS\n";
}

void PoetryRebuildGame::selfCheckOptimalSolver() {
    assertTrue(!poems_.empty(), "[Step5 self-check] poems_ is empty");
    assertTrue(!idioms_.empty(), "[Step5 self-check] idioms_ is empty");

    const PhraseInfo& poemSample = poems_.front();
    const PhraseInfo& idiomSample = idioms_.front();

    const CharFreq tinyPool = mergeFreq(poemSample.charFreq, idiomSample.charFreq);
    std::vector<std::string> tinyCandidates = {poemSample.id, idiomSample.id};

    OptimalSolver solver(indexStore_);
    const SolveResult result = solver.searchOptimalMax(tinyCandidates, tinyPool);

    assertTrue(result.usedExact, "[Step5 self-check] expected exact mode");
    assertTrue(result.optimalMax == 2, "[Step5 self-check] optimalMax should be 2");
    assertTrue(
        std::find(result.bestSet.begin(), result.bestSet.end(), poemSample.id) != result.bestSet.end(),
        "[Step5 self-check] bestSet missing poem"
    );
    assertTrue(
        std::find(result.bestSet.begin(), result.bestSet.end(), idiomSample.id) != result.bestSet.end(),
        "[Step5 self-check] bestSet missing idiom"
    );

    const auto bestSet = solver.getBestSolutionSet();
    assertTrue(bestSet.size() == 2, "[Step5 self-check] getBestSolutionSet size invalid");

    std::cout << "[Step5 self-check] PASS\n";
}

void PoetryRebuildGame::selfCheckGameRound() {
    assertTrue(!poems_.empty(), "[Step6 self-check] poems_ is empty");
    assertTrue(!idioms_.empty(), "[Step6 self-check] idioms_ is empty");

    const PhraseInfo& poemSample = poems_.front();
    const PhraseInfo& idiomSample = idioms_.front();

    CandidateEngine engine(indexStore_);
    OptimalSolver solver(indexStore_);

    {
        GameRoundConfig config;
        config.initialTime = 90;
        config.recomputeCandidatesOnSubmit = true;
        config.recomputeOptimalOnSubmit = true;

        GameRound round(indexStore_, engine, solver, config);

        GamePool pool;
        pool.originFreq = mergeFreq(poemSample.charFreq, idiomSample.charFreq);
        pool.remainFreq = pool.originFreq;
        pool.totalChars = totalFreqCount(pool.originFreq);

        round.createGameRound(pool);

        const GameState& s0 = round.state();
        assertTrue(s0.candidateCount >= 2, "[Step6 self-check] initial candidateCount invalid");
        assertTrue(s0.optimalMax >= 2, "[Step6 self-check] initial optimalMax invalid");

        const SubmitResult r1 = round.submitAnswer(poemSample.text);
        assertTrue(r1.accepted, "[Step6 self-check] valid poem submit should pass");
        assertTrue(round.state().foundSet.count(poemSample.text) == 1,
                   "[Step6 self-check] foundSet missing poem");
        assertTrue(!round.state().submitHistory.empty(),
                   "[Step6 self-check] submitHistory should update");
        assertTrue(round.state().score > 0,
                   "[Step6 self-check] score should increase");
        assertTrue(round.state().combo == 1,
                   "[Step6 self-check] combo should become 1");

        const SubmitResult dup = round.submitAnswer(poemSample.text);
        assertTrue(dup.status == SubmitStatus::DuplicateAnswer,
                   "[Step6 self-check] duplicate submit should be blocked");

        const SubmitResult r2 = round.submitAnswer(idiomSample.text);
        assertTrue(r2.accepted, "[Step6 self-check] valid idiom submit should pass");
        assertTrue(round.calcGapToOptimal() == 0,
                   "[Step6 self-check] gap to optimal should be 0");
        assertTrue(round.isGameOver(),
                   "[Step6 self-check] tiny pool should be exhausted");
    }

    {
        GameRoundConfig config;
        GameRound round(indexStore_, engine, solver, config);

        GamePool pool;
        pool.originFreq = poemSample.charFreq;
        pool.remainFreq = pool.originFreq;
        pool.totalChars = totalFreqCount(pool.originFreq);

        round.createGameRound(pool);

        const SubmitResult notEnough = round.submitAnswer(idiomSample.text);
        assertTrue(notEnough.status == SubmitStatus::NotEnoughChars,
                   "[Step6 self-check] should block answer when remainFreq is insufficient");

        const SubmitResult invalid = round.submitAnswer("这不是合法答案");
        assertTrue(invalid.status == SubmitStatus::NotInLexicon,
                   "[Step6 self-check] invalid text should be blocked");
    }

    std::cout << "[Step6 self-check] PASS\n";
}

void PoetryRebuildGame::gameLoop(LevelMode mode, LevelDifficulty difficulty) {
    gameLoop(nullptr, nullptr, mode, difficulty);
}

void PoetryRebuildGame::gameLoop(SDL_Window* externalWindow, SDL_Renderer* externalRenderer,
                                 LevelMode mode, LevelDifficulty difficulty) {
    GameRoundConfig config;
    switch (difficulty) {
    case LevelDifficulty::Easy:
        config.initialTime = 60;
        config.comboWindowSeconds = 12;
        break;
    case LevelDifficulty::Medium:
        config.initialTime = 60;
        config.comboWindowSeconds = 8;
        break;
    case LevelDifficulty::Hard:
        config.initialTime = 60;
        config.comboWindowSeconds = 5;
        break;
    }

    config.recomputeCandidatesOnSubmit = true;
    config.recomputeOptimalOnSubmit = true;

    while (true) {
        SdlCardWindow window(paths_);
        const int result = window.runWithLoading(
            externalWindow,
            externalRenderer,
            "PoetryRebuildGame - " +
            LevelGenerator::modeToString(mode) + " - " +
            LevelGenerator::difficultyToString(difficulty),
            [&, mode, difficulty, config]() -> SdlCardWindow::PreparedRoundData {
                CandidateEngine localEngine(indexStore_);
                OptimalSolver localSolver(indexStore_);
                LevelGenerator localLevelGenerator(indexStore_, poems_, idioms_);
                GameRound round(indexStore_, localEngine, localSolver, config);
                LevelBuildResult level;

                if (!localLevelGenerator.build(mode, difficulty, round, level)) {
                    throw std::runtime_error("无法生成该关卡");
                }

                auto seedIdsToTextsLocal = [this](const std::vector<std::string>& seedIds) {
                    std::vector<std::string> texts;
                    texts.reserve(seedIds.size());

                    for (const auto& id : seedIds) {
                        const PhraseInfo* phrase = indexStore_.getPhraseById(id);
                        if (phrase != nullptr) {
                            texts.push_back(phrase->text);
                        }
                    }
                    return texts;
                };

                const auto seedTexts = seedIdsToTextsLocal(level.seedIds);

                const auto validIds =
                    localEngine.getCandidates(level.pool.originFreq, level.options);

                auto validAnswerTexts = localEngine.getCandidateTexts(validIds);
                std::sort(validAnswerTexts.begin(), validAnswerTexts.end());

                const SolveResult optimalResult =
                    localSolver.searchOptimalMax(validIds, level.pool.originFreq);

                std::vector<std::string> optimalAnswerTexts;
                optimalAnswerTexts.reserve(optimalResult.bestSet.size());

                for (const auto& id : optimalResult.bestSet) {
                    const PhraseInfo* phrase = indexStore_.getPhraseById(id);
                    if (phrase != nullptr) {
                        optimalAnswerTexts.push_back(phrase->text);
                    }
                }
                std::sort(optimalAnswerTexts.begin(), optimalAnswerTexts.end());

                std::cout << "[Level] " << level.summary << "\n";
                std::cout << "  模式: " << LevelGenerator::modeToString(level.mode) << "\n";
                std::cout << "  难度: " << LevelGenerator::difficultyToString(level.difficulty) << "\n";
                std::cout << "  总字数: " << level.pool.totalChars << " / 22\n";
                std::cout << "  A = " << round.state().candidateCount
                          << ", B = " << round.state().optimalMax << "\n";

                if (!seedTexts.empty()) {
                    std::cout << "  种子答案: ";
                    for (std::size_t i = 0; i < seedTexts.size(); ++i) {
                        if (i > 0) {
                            std::cout << " / ";
                        }
                        std::cout << seedTexts[i];
                    }
                    std::cout << "\n";
                }

                if (!level.disturbChars.empty()) {
                    std::cout << "  干扰字: ";
                    for (std::size_t i = 0; i < level.disturbChars.size(); ++i) {
                        if (i > 0) {
                            std::cout << " ";
                        }
                        std::cout << level.disturbChars[i];
                    }
                    std::cout << "\n";
                }

                std::cout << "  提示次数: " << level.hintLimit << "\n";

                std::cout << "  所有可能答案(" << validAnswerTexts.size() << "):\n";
                for (const auto& text : validAnswerTexts) {
                    std::cout << "    - " << text << "\n";
                }

                std::cout << "  最优答案(" << optimalAnswerTexts.size()
                          << " / B=" << optimalResult.optimalMax << "):\n";
                for (const auto& text : optimalAnswerTexts) {
                    std::cout << "    * " << text << "\n";
                }
                std::cout.flush();

                SdlCardWindow::PreparedRoundData out;
                out.poolFreq = round.state().pool.remainFreq;
                out.validAnswers = std::move(validAnswerTexts);
                out.optimalAnswers = std::move(optimalAnswerTexts);
                out.hintLimit = level.hintLimit;
                out.initialTimeSeconds = std::min(config.initialTime, 260);
                out.bgR = 249;
                out.bgG = 254;
                out.bgB = 255;
                return out;
            }
        );

        if (result == SdlCardWindow::kResultReplaySameDifficulty) {
            std::cout << "[SDL Preview] 再玩一回：重新生成同难度关卡。\n";
            continue;
        }

        std::cout << "[SDL Preview] 已结束本次关卡。\n";
        break;
    }
}

void PoetryRebuildGame::saveResult() {
    const auto summaryFile = paths_.moduleCacheRoot / "summary.txt";
    std::ofstream out(summaryFile, std::ios::binary);
    out << "PoetryRebuildGame Summary\n";
    out << "poems=" << poems_.size() << '\n';
    out << "idioms=" << idioms_.size() << '\n';
    out << "poemsOutput=" << (paths_.modulePrebuildRoot / "poems.json").string() << '\n';
    out << "idiomsOutput=" << (paths_.modulePrebuildRoot / "idioms.json").string() << '\n';
    out << "indexCount=" << indexStore_.phraseCount() << '\n';
    out << "skippedDuplicateTextCount=" << indexStore_.skippedDuplicateTextCount() << '\n';
    out << "conflictGraphBuilt=" << (indexStore_.conflictGraphBuilt() ? "true" : "false") << '\n';
    out << "indexReport=" << (paths_.moduleCacheRoot / "index_report.json").string() << '\n';
}

void PoetryRebuildGame::cleanup() {
    poems_.shrink_to_fit();
    idioms_.shrink_to_fit();
    indexStore_.clear();
}

} // namespace lineverse::poetryrebuild