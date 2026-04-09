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

/**
 * @brief Controller that bridges UI interactions and core game logic.
 *
 * The controller owns no SDL-specific type. The UI layer communicates with
 * this class using plain data objects and simple method calls.
 */
class IdiomChainController {
public:
    /**
     * @brief Construct the controller with injected services.
     * @param repository Idiom data repository.
     * @param graph Built idiom graph.
     * @param solver Shortest path solver.
     * @param hintEngine Hint service.
     * @param recordRepository Persistent record storage.
     */
    IdiomChainController(
        const IIdiomRepository& repository,
        const IdiomGraph& graph,
        const PathSolver& solver,
        const HintEngine& hintEngine,
        RecordRepository& recordRepository);

    /**
     * @brief Start a single-player round.
     * @param mode Requested game mode.
     * @param playerName Player display name.
     */
    void startSingleGame(GameMode mode, const std::string& playerName);

    /**
     * @brief Update elapsed time and timeout status.
     */
    void tick();

    /**
     * @brief Get immutable view of the current session state.
     * @return Current session.
     */
    const GameSession& getSession() const;

    /**
     * @brief Get easy-mode pool item identifiers.
     * @return Pool identifiers.
     */
    std::vector<int> getEasyPool() const;

    /**
     * @brief Get easy-mode pool words.
     * @return Word list.
     */
    std::vector<std::string> getEasyPoolWords() const;

    /**
     * @brief Submit the reordered pool indices for easy mode.
     * @param orderedPoolIndexes Indices into the easy pool in desired order.
     * @return True when accepted.
     */
    bool submitEasyOrder(const std::vector<int>& orderedPoolIndexes);

    /**
     * @brief Get current medium-mode candidate identifiers.
     * @return Candidate identifiers.
     */
    std::vector<int> getMediumOptions() const;

    /**
     * @brief Get current medium-mode candidate words.
     * @return Candidate words.
     */
    std::vector<std::string> getMediumOptionWords() const;

    /**
     * @brief Get remaining shortest-path distance for each current medium option.
     * @return Distance list aligned with getMediumOptionWords().
     */
    std::vector<int> getMediumOptionDistances() const;

    /**
     * @brief Submit one medium-mode choice.
     * @param optionIndex Zero-based option index.
     * @return True when accepted.
     */
    bool submitMediumChoice(int optionIndex);

    /**
     * @brief Submit a concrete idiom identifier for medium mode.
     * @param idiomId Selected idiom identifier from the current option set.
     * @return True when accepted.
     */
    bool submitMediumChoiceById(int idiomId);

    /**
     * @brief Submit one hard-mode idiom input.
     * @param inputWordOrAbbreviation Full idiom or abbreviation.
     * @return True when accepted.
     */
    bool submitHardInput(const std::string& inputWordOrAbbreviation);

    /**
     * @brief Query hard-mode candidates by pinyin / abbreviation / idiom text.
     * @param query User query text.
     * @return Candidate idiom words ordered by relevance.
     */
    std::vector<std::string> getHardCandidateWords(const std::string& query) const;

    /**
     * @brief Roll back one step in the current session.
     * @return True when rollback succeeds.
     */
    bool rollbackOneStep();

    /**
     * @brief Request a hint for the current session.
     * @return Suggested next idiom, if available.
     */
    std::optional<std::string> requestHint();

    /**
     * @brief Reveal one optimal answer path.
     * @return Shortest-path result.
     */
    PathResult revealAnswer() const;

    /**
     * @brief Get the latest human-readable status message.
     * @return Status message.
     */
    const std::string& getLastMessage() const;

    /**
     * @brief Flush pending record flags after save.
     */
    void flushPendingRecord();

    /**
     * @brief Convert idiom identifier to display word.
     * @param idiomId Idiom identifier.
     * @return Idiom word, or empty string.
     */
    std::string wordOf(int idiomId) const;

    /**
     * @brief Get explanation text of an idiom.
     * @param idiomId Idiom identifier.
     * @return Explanation string.
     */
    std::string explanationOf(int idiomId) const;

    /**
     * @brief Get best-path words for the current round.
     * @return Word sequence.
     */
    std::vector<std::string> bestPathWords() const;

    /**
     * @brief Get explanation lines for each idiom already used in current path.
     * @return Explanation lines.
     */
    std::vector<std::string> currentPathExplanations() const;

    /**
     * @brief Build the total-score leaderboard.
     * @return Priority queue ordered by total score.
     */
    std::priority_queue<RankItem> buildLeaderboard() const;

    /**
     * @brief Load persisted raw records for history display.
     * @return Saved game records.
     */
    std::vector<GameRecord> loadAllRecords() const;

private:
    void prepareSession(IModeStrategy& strategy);
    void updateElapsedSeconds();
    void finalizeIfTargetReached();
    void saveRecordIfNeeded();
    std::vector<int> computeMediumOptionsInternal() const;
    void refreshMediumOptions();
    bool isInputValidNext(int nextId);

    const IIdiomRepository& repository_;
    const IdiomGraph& graph_;
    const PathSolver& solver_;
    const HintEngine& hintEngine_;
    RecordRepository& recordRepository_;

    GameSession session_;
    std::string lastMessage_;
    bool pendingRecordFlush_ { false };
};

#endif
