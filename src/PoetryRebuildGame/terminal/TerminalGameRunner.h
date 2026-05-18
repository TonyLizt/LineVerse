#pragma once

#include <random>
#include <string>
#include <vector>

#include "../candidate/CandidateEngine.h"
#include "../game/GameRound.h"
#include "../hint/HintEngine.h"
#include "../index/IndexStore.h"
#include "../model/PhraseInfo.h"
#include "../preprocess/TextNormalizer.h"
#include "../solver/OptimalSolver.h"

namespace lineverse::poetryrebuild {

class TerminalGameRunner {
public:
    explicit TerminalGameRunner(const IndexStore& indexStore);

    void run(const std::vector<PhraseInfo>& poems, const std::vector<PhraseInfo>& idioms);

private:
    enum class DifficultyLevel {
        Easy,
        Medium,
        Hard
    };

    const IndexStore& indexStore_;
    TextNormalizer normalizer_;
    HintEngine hintEngine_;
    std::mt19937 rng_;

    void printMainMenu() const;
    void printDifficultyMenu(const std::string& modeName) const;
    void printCommands() const;

    std::string difficultyToString(DifficultyLevel difficulty) const;
    std::string formatPool(const CharFreq& freq) const;
    std::vector<std::string> idsToTexts(const std::vector<std::string>& ids) const;

    std::vector<const PhraseInfo*> buildIdiomPool(const std::vector<PhraseInfo>& idioms) const;
    std::vector<const PhraseInfo*> buildPoemPool(const std::vector<PhraseInfo>& poems) const;

    std::vector<const PhraseInfo*> sampleDistinct(
        const std::vector<const PhraseInfo*>& source,
        std::size_t count
    );

    bool chooseDifficulty(const std::string& modeName, DifficultyLevel& difficulty) const;

    bool prepareIdiomRound(
        const std::vector<PhraseInfo>& idioms,
        DifficultyLevel difficulty,
        CandidateEngine& engine,
        OptimalSolver& solver,
        GameRound& round,
        std::vector<std::string>& seedIds,
        int& hintCount
    );

    bool prepareMixedRound(
        const std::vector<PhraseInfo>& poems,
        const std::vector<PhraseInfo>& idioms,
        DifficultyLevel difficulty,
        CandidateEngine& engine,
        OptimalSolver& solver,
        GameRound& round,
        std::vector<std::string>& seedIds,
        int& hintCount
    );

    void playRound(
        GameRound& round,
        const std::string& modeName,
        DifficultyLevel difficulty,
        const std::vector<std::string>& seedIds,
        int hintCount
    );

    void printStatus(const GameRound& round, int hintCount) const;
    void printBestSet(const GameRound& round) const;
};

} // namespace lineverse::poetryrebuild