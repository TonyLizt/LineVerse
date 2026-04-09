#ifndef IDIOM_CHAIN_GAME_CONTROLLER_H
#define IDIOM_CHAIN_GAME_CONTROLLER_H

#include "GameSession.h"
#include "../core/PathSolver.h"
#include "../data/RecordRepository.h"

#include <optional>
#include <queue>
#include <string>
#include <vector>

class IIdiomRepository;
class IdiomGraph;
class HintEngine;
class IModeStrategy;
class IBattleTransport;

/**
 * @brief Controller that bridges UI interactions and core game logic.
 *
 * The controller owns no SDL-specific type. The UI layer communicates with
 * this class using plain data objects and simple method calls.
 */
class IdiomChainController {
public:
    IdiomChainController(
        const IIdiomRepository& repository,
        const IdiomGraph& graph,
        const PathSolver& solver,
        const HintEngine& hintEngine,
        RecordRepository& recordRepository,
        IBattleTransport* battleTransport = nullptr);

    void startSingleGame(GameMode mode, const std::string& playerName);

    // ===== Step 1: LAN battle backend =====
    bool hostBattle(GameMode mode, const std::string& playerName, unsigned short port);
    bool joinBattle(GameMode mode, const std::string& playerName, const std::string& ip, unsigned short port);
    void leaveBattle();
    bool isBattleConnected() const;
    bool isBattleRoundStarted() const;
    void pollBattle();

    void tick();
    const GameSession& getSession() const;
    std::vector<int> getEasyPool() const;
    std::vector<std::string> getEasyPoolWords() const;
    bool submitEasyOrder(const std::vector<int>& orderedPoolIndexes);
    std::vector<int> getMediumOptions() const;
    std::vector<std::string> getMediumOptionWords() const;
    std::vector<int> getMediumOptionDistances() const;
    bool submitMediumChoice(int optionIndex);
    bool submitMediumChoiceById(int idiomId);
    bool submitHardInput(const std::string& inputWordOrAbbreviation);
    std::vector<std::string> getHardCandidateWords(const std::string& query) const;
    bool rollbackOneStep();
    std::optional<std::string> requestHint();
    PathResult revealAnswer() const;
    const std::string& getLastMessage() const;
    void flushPendingRecord();
    std::string wordOf(int idiomId) const;
    std::string explanationOf(int idiomId) const;
    std::vector<std::string> bestPathWords() const;
    std::vector<std::string> currentPathExplanations() const;
    std::priority_queue<RankItem> buildLeaderboard() const;
    std::vector<GameRecord> loadAllRecords() const;

private:
    void prepareSession(IModeStrategy& strategy);
    void prepareFixedBattleSession(GameMode mode, int startId, int targetId);
    void initializeSessionForQuestion(GameMode mode, int startId, int targetId, bool generateEasyPool);
    void updateElapsedSeconds();
    void finalizeIfTargetReached();
    void saveRecordIfNeeded();
    std::vector<int> computeMediumOptionsInternal() const;
    void refreshMediumOptions();
    bool isInputValidNext(int nextId);

    // battle helpers
    void sendBattleState();
    void handleBattleMessage(const std::string& message);
    void startHostBattleRound();
    void updateBattleWinnerText();
    std::string serializePath(const std::vector<int>& path) const;
    std::vector<int> deserializePath(const std::string& text) const;

    const IIdiomRepository& repository_;
    const IdiomGraph& graph_;
    const PathSolver& solver_;
    const HintEngine& hintEngine_;
    RecordRepository& recordRepository_;
    IBattleTransport* battleTransport_;

    GameSession session_;
    std::string lastMessage_;
    bool pendingRecordFlush_ { false };
    GameMode pendingBattleMode_ { GameMode::BattleEasy };
};

#endif
