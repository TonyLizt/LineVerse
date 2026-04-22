#ifndef IDIOM_CHAIN_GAME_SCENE_H
#define IDIOM_CHAIN_GAME_SCENE_H

#include <SDL.h>
#include "../app/GameSession.h"
#include "../data/RecordRepository.h"

#include <string>
#include <vector>

struct SDL_Renderer;
struct SDL_Window;
struct _TTF_Font;
typedef struct _TTF_Font TTF_Font;
union SDL_Event;

class IdiomChainController;

class IdiomChainScene {
public:
    explicit IdiomChainScene(IdiomChainController& controller);
    ~IdiomChainScene();

    int run();
    int run(SDL_Window* externalWindow, SDL_Renderer* externalRenderer);

private:
    enum class ViewMode {
        MainMenu,
        DifficultyMenu,
        BattleSetup,
        BattleLobby,
        Game,
        Result,
        Leaderboard
    };

    enum class TextField {
        None,
        PlayerName,
        HostIp,
        Port
    };

    struct ButtonSpec {
        SDL_Rect rect{};
        std::string label;
        int actionId{-1};
        bool enabled{true};
    };

    struct EasyTile {
        int poolIndex{-1};
        SDL_Rect rect{};
        bool inSelectedZone{false};
    };

    bool initialize(SDL_Window* externalWindow = nullptr, SDL_Renderer* externalRenderer = nullptr);
    void shutdown();
    bool loadFonts();
    static std::vector<std::string> candidateFontPaths();

    void handleEvent(const SDL_Event& event, bool& running, int& resultCode);
    void handleMainMenuEvent(const SDL_Event& event, bool& running, int& resultCode);
    void handleDifficultyMenuEvent(const SDL_Event& event);
    void handleBattleSetupEvent(const SDL_Event& event);
    void handleBattleLobbyEvent(const SDL_Event& event);
    void handleGameEvent(const SDL_Event& event);
    void handleResultEvent(const SDL_Event& event);
    void handleLeaderboardEvent(const SDL_Event& event);
    void handleMouseMotion(const SDL_MouseMotionEvent& event);
    void handleMouseWheel(const SDL_MouseWheelEvent& event);
    void handleTextEditing(const SDL_TextEditingEvent& event);

    void openMainMenu();
    void openDifficultyMenu();
    void openBattleSetup();
    void openBattleLobby();
    void openLeaderboard();
    void startGame(int modeIndex);

    void refreshMainMenuButtons();
    void refreshDifficultyButtons();
    void refreshBattleSetupButtons();
    void refreshGameCaches(bool force = false);
    void refreshMediumOptions(bool force = false);
    void refreshHardCandidates(bool force = false);
    void refreshLeaderboardCaches();
    void resetEasyTilesFromPool();
    void relayoutEasyTiles();
    void resetScrollOffsets();
    void clampScrollOffset(int& offset, int contentHeight, int viewHeight) const;

    void drawText(TTF_Font* font,
                  const std::string& text,
                  int x,
                  int y,
                  SDL_Color color,
                  bool centered = false) const;
    void drawWrappedText(TTF_Font* font,
                         const std::string& text,
                         const SDL_Rect& rect,
                         SDL_Color color) const;
    int drawWrappedTextScrollable(TTF_Font* font,
                                  const std::string& text,
                                  const SDL_Rect& rect,
                                  SDL_Color color,
                                  int& scrollOffset) const;
    int wrappedTextHeight(TTF_Font* font,
                          const std::string& text,
                          int wrapWidth) const;
    void drawFittedTextInRect(TTF_Font* preferredFont,
                              const std::string& text,
                              const SDL_Rect& rect,
                              SDL_Color color,
                              bool centered = false,
                              bool allowWrap = true) const;
    void drawPanel(const SDL_Rect& rect, const std::string& title) const;
    void drawButton(const ButtonSpec& button, bool primary = false) const;
    void drawBadge(const SDL_Rect& rect, const std::string& text) const;
    void drawCard(const SDL_Rect& rect,
                  const std::string& title,
                  const std::string& content,
                  bool emphasized = false) const;
    void drawSimpleTableRow(const SDL_Rect& rect,
                            const std::vector<std::string>& columns,
                            const std::vector<int>& widths,
                            bool header) const;
    void drawInputField(const SDL_Rect& rect,
                        const std::string& label,
                        const std::string& value,
                        bool active) const;

    void render();
    void renderMainMenu();
    void renderDifficultyMenu();
    void renderBattleSetup();
    void renderBattleLobby();
    void renderGame();
    void renderResult();
    void renderLeaderboard();

    void beginEasyDrag(int mouseX, int mouseY);
    void updateEasyDrag(int mouseX, int mouseY);
    void endEasyDrag();
    bool submitHardInputBuffer();

    std::string modeText(int modeIndex) const;
    std::string currentPathText() const;
    std::vector<std::string> currentPathWords() const;
    std::vector<std::string> answerPathWords() const;
    std::string remotePathText() const;
    std::string battleRemoteSummary() const;
    std::string battleResultSummary() const;
    int battleModeIndexToLocal(GameMode mode) const;
    GameMode selectedBattleMode() const;

    IdiomChainController& controller_;

    SDL_Window* window_;
    SDL_Renderer* renderer_;
    bool ownsWindowRenderer_{true};
    bool ownsSDL_{false};
    bool ownsTTF_{false};
    TTF_Font* titleFont_;
    TTF_Font* subtitleFont_;
    TTF_Font* bodyFont_;
    TTF_Font* smallFont_;

    ViewMode viewMode_;
    int activeModeIndex_;
    int lastMediumPathSize_;
    int lastHardPathSize_;
    std::string lastHardQuery_;

    std::string playerName_;
    std::string statusMessage_;
    std::string hintMessage_;
    std::string revealedAnswerText_;
    std::string inputBuffer_;
    std::string imeComposition_;
    int imeCursor_{0};
    bool hardInputFocused_{false};

    int mouseX_{0};
    int mouseY_{0};

    int mainMenuLeftScroll_{0};
    int mainMenuRightScroll_{0};
    int difficultyInfoScroll_{0};
    int gamePathScroll_{0};
    int gameExplanationScroll_{0};
    int resultMyRouteScroll_{0};
    int resultBestRouteScroll_{0};
    int resultExplanationScroll_{0};
    int leaderboardHistoryScroll_{0};
    int leaderboardRankScroll_{0};

    int mainMenuLeftContentHeight_{0};
    int mainMenuRightContentHeight_{0};
    int difficultyInfoContentHeight_{0};
    int gamePathContentHeight_{0};
    int gameExplanationContentHeight_{0};
    int resultMyRouteContentHeight_{0};
    int resultBestRouteContentHeight_{0};
    int resultExplanationContentHeight_{0};
    int leaderboardHistoryContentHeight_{0};
    int leaderboardRankContentHeight_{0};

    std::vector<ButtonSpec> mainMenuButtons_;
    std::vector<ButtonSpec> difficultyButtons_;
    std::vector<ButtonSpec> mediumButtons_;
    std::vector<ButtonSpec> hardCandidateButtons_;

    ButtonSpec gameBackButton_;
    ButtonSpec gameHintButton_;
    ButtonSpec gameUndoButton_;
    ButtonSpec gameRevealButton_;
    ButtonSpec easySubmitButton_;
    ButtonSpec easyResetButton_;
    ButtonSpec hardSubmitButton_;
    ButtonSpec resultAgainButton_;
    ButtonSpec resultRankButton_;
    ButtonSpec resultMenuButton_;
    ButtonSpec leaderboardBackButton_;

    ButtonSpec battleHostButton_;
    ButtonSpec battleJoinButton_;
    ButtonSpec battleModeEasyButton_;
    ButtonSpec battleModeMediumButton_;
    ButtonSpec battleModeHardButton_;
    ButtonSpec battleConfirmButton_;
    ButtonSpec battleSetupBackButton_;
    ButtonSpec battleLobbyBackButton_;

    TextField activeTextField_{TextField::None};
    bool battleHostSelected_{true};
    int battleModeIndex_{2};
    std::string battlePlayerInput_{"Player"};
    std::string battleHostIpInput_{"127.0.0.1"};
    std::string battlePortInput_{"24567"};
    SDL_Rect battlePlayerField_{};
    SDL_Rect battleIpField_{};
    SDL_Rect battlePortField_{};

    std::vector<int> mediumOptionIds_;
    std::vector<int> mediumOptionDistances_;
    std::vector<std::string> hardCandidates_;

    std::vector<int> easySelectedOrder_;
    std::vector<int> easyDiscardOrder_;
    std::vector<EasyTile> easyTiles_;
    SDL_Rect easySelectedZoneRect_{};
    SDL_Rect easyDiscardZoneRect_{};
    bool draggingEasyTile_{false};
    int draggedEasyPoolIndex_{-1};
    bool draggedFromSelectedZone_{false};
    int dragMouseX_{0};
    int dragMouseY_{0};

    std::vector<RankItem> leaderboardCache_;
    std::vector<GameRecord> historyCache_;
};

#endif
