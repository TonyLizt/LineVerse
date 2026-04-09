// #include "IdiomChainScene.h"

// #include "../app/GameSession.h"
// #include "../app/IdiomChainController.h"
// #include "../data/RecordRepository.h"

// #include <SDL.h>
// #include <SDL_ttf.h>

// #include <algorithm>
// #include <cstdio>
// #include <sstream>
// #include <string>
// #include <vector>

// namespace {
// constexpr int kWindowWidth = 1200;
// constexpr int kWindowHeight = 800;
// constexpr int kOuterMargin = 36;
// constexpr int kInnerGap = 20;
// constexpr int kButtonHeight = 58;
// constexpr int kTileWidth = 110;
// constexpr int kTileHeight = 58;
// constexpr int kTileGap = 10;

// SDL_Rect makeRect(int x, int y, int w, int h) {
//     return SDL_Rect{x, y, w, h};
// }

// bool pointInRect(int x, int y, const SDL_Rect& rect) {
//     return x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h;
// }

// SDL_Color rgb(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255) {
//     return SDL_Color{r, g, b, a};
// }

// void fillRect(SDL_Renderer* renderer, const SDL_Rect& rect, SDL_Color color) {
//     SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
//     SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
//     SDL_RenderFillRect(renderer, &rect);
// }

// void strokeRect(SDL_Renderer* renderer, const SDL_Rect& rect, SDL_Color color, int thickness = 1) {
//     SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
//     for (int i = 0; i < thickness; ++i) {
//         SDL_Rect r{rect.x - i, rect.y - i, rect.w + i * 2, rect.h + i * 2};
//         SDL_RenderDrawRect(renderer, &r);
//     }
// }

// void drawLine(SDL_Renderer* renderer, int x1, int y1, int x2, int y2, SDL_Color color) {
//     SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
//     SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
// }

// std::string formatSeconds(double seconds) {
//     if (seconds < 0.0) {
//         seconds = 0.0;
//     }
//     const int total = static_cast<int>(seconds);
//     const int mm = total / 60;
//     const int ss = total % 60;
//     char buf[32];
//     std::snprintf(buf, sizeof(buf), "%02d:%02d", mm, ss);
//     return buf;
// }

// std::string joinWords(const std::vector<std::string>& words, const std::string& sep) {
//     std::ostringstream oss;
//     for (std::size_t i = 0; i < words.size(); ++i) {
//         if (i > 0U) {
//             oss << sep;
//         }
//         oss << words[i];
//     }
//     return oss.str();
// }

// int clampInt(int value, int low, int high) {
//     return std::max(low, std::min(value, high));
// }

// } // namespace

// IdiomChainScene::IdiomChainScene(IdiomChainController& controller)
//     : controller_(controller)
//     , window_(nullptr)
//     , renderer_(nullptr)
//     , titleFont_(nullptr)
//     , subtitleFont_(nullptr)
//     , bodyFont_(nullptr)
//     , smallFont_(nullptr)
//     , viewMode_(ViewMode::MainMenu)
//     , activeModeIndex_(2)
//     , lastMediumPathSize_(-1)
//     , lastHardPathSize_(-1)
//     , playerName_("Player") {
// }

// IdiomChainScene::~IdiomChainScene() {
//     shutdown();
// }

// int IdiomChainScene::run() {
//     if (!initialize()) {
//         return -1;
//     }

//     openMainMenu();
//     bool running = true;
//     int resultCode = 1;

//     while (running) {
//         SDL_Event event{};
//         while (SDL_PollEvent(&event) != 0) {
//             handleEvent(event, running, resultCode);
//         }

//         if (viewMode_ == ViewMode::Game) {
//             controller_.tick();
//             const GameSession& session = controller_.getSession();
//             if (session.finished) {
//                 SDL_StopTextInput();
//                 viewMode_ = ViewMode::Result;
//             }
//         }

//         render();
//         SDL_Delay(16);
//     }

//     shutdown();
//     return resultCode;
// }

// bool IdiomChainScene::initialize() {
//     if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
//         return false;
//     }
//     if (TTF_Init() != 0) {
//         SDL_Quit();
//         return false;
//     }
//     SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

//     window_ = SDL_CreateWindow(
//         "LineVerse - IdiomChainGame",
//         SDL_WINDOWPOS_CENTERED,
//         SDL_WINDOWPOS_CENTERED,
//         kWindowWidth,
//         kWindowHeight,
//         SDL_WINDOW_SHOWN
//     );
//     if (window_ == nullptr) {
//         return false;
//     }

//     renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
//     if (renderer_ == nullptr) {
//         renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
//     }
//     if (renderer_ == nullptr) {
//         return false;
//     }

//     return loadFonts();
// }

// void IdiomChainScene::shutdown() {
//     SDL_StopTextInput();
//     if (smallFont_ != nullptr) {
//         TTF_CloseFont(smallFont_);
//         smallFont_ = nullptr;
//     }
//     if (bodyFont_ != nullptr) {
//         TTF_CloseFont(bodyFont_);
//         bodyFont_ = nullptr;
//     }
//     if (subtitleFont_ != nullptr) {
//         TTF_CloseFont(subtitleFont_);
//         subtitleFont_ = nullptr;
//     }
//     if (titleFont_ != nullptr) {
//         TTF_CloseFont(titleFont_);
//         titleFont_ = nullptr;
//     }
//     if (renderer_ != nullptr) {
//         SDL_DestroyRenderer(renderer_);
//         renderer_ = nullptr;
//     }
//     if (window_ != nullptr) {
//         SDL_DestroyWindow(window_);
//         window_ = nullptr;
//     }
//     if (TTF_WasInit() != 0) {
//         TTF_Quit();
//     }
//     if (SDL_WasInit(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0U) {
//         SDL_Quit();
//     }
// }

// bool IdiomChainScene::loadFonts() {
//     for (const std::string& path : candidateFontPaths()) {
//         titleFont_ = TTF_OpenFont(path.c_str(), 42);
//         subtitleFont_ = TTF_OpenFont(path.c_str(), 26);
//         bodyFont_ = TTF_OpenFont(path.c_str(), 20);
//         smallFont_ = TTF_OpenFont(path.c_str(), 15);
//         if (titleFont_ != nullptr && subtitleFont_ != nullptr && bodyFont_ != nullptr && smallFont_ != nullptr) {
//             return true;
//         }
//         if (titleFont_ != nullptr) {
//             TTF_CloseFont(titleFont_);
//             titleFont_ = nullptr;
//         }
//         if (subtitleFont_ != nullptr) {
//             TTF_CloseFont(subtitleFont_);
//             subtitleFont_ = nullptr;
//         }
//         if (bodyFont_ != nullptr) {
//             TTF_CloseFont(bodyFont_);
//             bodyFont_ = nullptr;
//         }
//         if (smallFont_ != nullptr) {
//             TTF_CloseFont(smallFont_);
//             smallFont_ = nullptr;
//         }
//     }
//     return false;
// }

// std::vector<std::string> IdiomChainScene::candidateFontPaths() {
//     return {
//         "assets/IdiomChainGame/fonts/NotoSansSC-Regular.otf",
//         "assets/IdiomChainGame/fonts/NotoSansCJKsc-Regular.otf",
//         "assets/fonts/NotoSansSC-Regular.otf",
//         "assets/fonts/msyh.ttc",
//         "C:/Windows/Fonts/msyh.ttc",
//         "C:/Windows/Fonts/msyhbd.ttc",
//         "C:/Windows/Fonts/simhei.ttf",
//         "C:/Windows/Fonts/simsun.ttc"
//     };
// }

// void IdiomChainScene::handleEvent(const SDL_Event& event, bool& running, int& resultCode) {
//     if (event.type == SDL_QUIT) {
//         running = false;
//         resultCode = 0;
//         return;
//     }

//     if (event.type == SDL_MOUSEMOTION) {
//         handleMouseMotion(event.motion);
//     } else if (event.type == SDL_MOUSEWHEEL) {
//         handleMouseWheel(event.wheel);
//         return;
//     } else if (event.type == SDL_TEXTEDITING) {
//         handleTextEditing(event.edit);
//         return;
//     }

//     switch (viewMode_) {
//     case ViewMode::MainMenu:
//         handleMainMenuEvent(event, running, resultCode);
//         break;
//     case ViewMode::DifficultyMenu:
//         handleDifficultyMenuEvent(event);
//         break;
//     case ViewMode::Game:
//         handleGameEvent(event);
//         break;
//     case ViewMode::Result:
//         handleResultEvent(event);
//         break;
//     case ViewMode::Leaderboard:
//         handleLeaderboardEvent(event);
//         break;
//     }
// }

// void IdiomChainScene::handleMainMenuEvent(const SDL_Event& event, bool& running, int& resultCode) {
//     if (event.type == SDL_KEYDOWN) {
//         if (event.key.keysym.sym == SDLK_ESCAPE) {
//             running = false;
//             resultCode = 1;
//             return;
//         }
//     }

//     if (event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT) {
//         return;
//     }

//     for (const auto& button : mainMenuButtons_) {
//         if (!button.enabled || !pointInRect(event.button.x, event.button.y, button.rect)) {
//             continue;
//         }
//         switch (button.actionId) {
//         case 1:
//             openDifficultyMenu();
//             return;
//         case 2:
//             statusMessage_ = "当前代码中尚未接入多人对战逻辑。";
//             return;
//         case 3:
//             openLeaderboard();
//             return;
//         case 4:
//             running = false;
//             resultCode = 1;
//             return;
//         default:
//             break;
//         }
//     }
// }

// void IdiomChainScene::handleDifficultyMenuEvent(const SDL_Event& event) {
//     if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
//         openMainMenu();
//         return;
//     }
//     if (event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT) {
//         return;
//     }

//     for (const auto& button : difficultyButtons_) {
//         if (!button.enabled || !pointInRect(event.button.x, event.button.y, button.rect)) {
//             continue;
//         }
//         if (button.actionId >= 1 && button.actionId <= 3) {
//             startGame(button.actionId);
//         } else {
//             openMainMenu();
//         }
//         return;
//     }
// }

// void IdiomChainScene::handleGameEvent(const SDL_Event& event) {
//     if (event.type == SDL_TEXTINPUT && activeModeIndex_ == 3 && hardInputFocused_) {
//         inputBuffer_ += event.text.text;
//         imeComposition_.clear();
//         imeCursor_ = 0;
//         refreshHardCandidates();
//         return;
//     }

//     if (event.type == SDL_KEYDOWN) {
//         const SDL_Keycode key = event.key.keysym.sym;
//         if (key == SDLK_ESCAPE) {
//             openMainMenu();
//             return;
//         }
//         if (activeModeIndex_ == 3 && hardInputFocused_) {
//             if (key == SDLK_BACKSPACE) {
//                 if (!inputBuffer_.empty()) {
//                     inputBuffer_.pop_back();
//                 }
//                 refreshHardCandidates();
//                 return;
//             }
//             if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
//                 submitHardInputBuffer();
//                 return;
//             }
//         }
//     }

//     if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
//         const int mx = event.button.x;
//         const int my = event.button.y;

//         if (pointInRect(mx, my, gameBackButton_.rect)) {
//             openMainMenu();
//             return;
//         }
//         if (pointInRect(mx, my, gameRevealButton_.rect)) {
//             const PathResult answer = controller_.revealAnswer();

//             std::vector<std::string> words;
//             for (int idiomId : answer.path) {
//                 words.push_back(controller_.wordOf(idiomId));
//             }
//             revealedAnswerText_ = joinWords(words, " -> ");
//             statusMessage_ = controller_.getLastMessage();
//             return;
//         }
//         if (activeModeIndex_ != 1 && pointInRect(mx, my, gameHintButton_.rect)) {
//             const auto hint = controller_.requestHint();
//             hintMessage_ = hint.has_value() ? ("提示：下一步可考虑 “" + *hint + "”。") : controller_.getLastMessage();
//             statusMessage_ = controller_.getLastMessage();
//             return;
//         }
//         if (activeModeIndex_ != 1 && pointInRect(mx, my, gameUndoButton_.rect)) {
//             controller_.rollbackOneStep();
//             statusMessage_ = controller_.getLastMessage();
//             refreshGameCaches(true);
//             return;
//         }

//         if (activeModeIndex_ == 1) {
//             if (pointInRect(mx, my, easySubmitButton_.rect)) {
//                 const bool ok = controller_.submitEasyOrder(easySelectedOrder_);
//                 statusMessage_ = ok ? "排序已提交。" : controller_.getLastMessage();
//                 return;
//             }
//             if (pointInRect(mx, my, easyResetButton_.rect)) {
//                 resetEasyTilesFromPool();
//                 statusMessage_ = "已恢复到初始候选池。";
//                 return;
//             }
//             beginEasyDrag(mx, my);
//             return;
//         }

//         if (activeModeIndex_ == 2) {
//             for (const auto& button : mediumButtons_) {
//                 if (pointInRect(mx, my, button.rect)) {
//                     const bool ok = controller_.submitMediumChoiceById(button.actionId);
//                     statusMessage_ = ok ? "已提交该步选择。" : controller_.getLastMessage();
//                     refreshGameCaches(true);
//                     return;
//                 }
//             }
//             return;
//         }

//         if (activeModeIndex_ == 3) {
//             // const SDL_Rect inputBox = makeRect(320 + 24, 136 + 300, 560 - 48, 54);
//             // if (pointInRect(mx, my, inputBox)) {
//             //     hardInputFocused_ = true;
//             //     SDL_StartTextInput();
//             //     SDL_SetTextInputRect(&inputBox);
//             //     return;
//             // }
//             const SDL_Rect inputBox = makeRect(320 + 24, 136 + 300, 560 - 48, 54);
//             const SDL_Rect composeRect = makeRect(inputBox.x + 10, inputBox.y + 28, inputBox.w - 20, 22);

//             if (pointInRect(mx, my, inputBox)) {
//                 hardInputFocused_ = true;
//                 SDL_StartTextInput();
//                 SDL_SetTextInputRect(&composeRect);
//                 return;
//             }
//             if (pointInRect(mx, my, hardSubmitButton_.rect)) {
//                 hardInputFocused_ = true;
//                 submitHardInputBuffer();
//                 return;
//             }
//             for (const auto& button : hardCandidateButtons_) {
//                 if (pointInRect(mx, my, button.rect)) {
//                     hardInputFocused_ = true;
//                     inputBuffer_ = button.label;
//                     imeComposition_.clear();
//                     imeCursor_ = 0;
//                     submitHardInputBuffer();
//                     return;
//                 }
//             }
//             hardInputFocused_ = false;
//             return;
//         }
//     }

//     if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
//         if (activeModeIndex_ == 1 && draggingEasyTile_) {
//             endEasyDrag();
//             return;
//         }
//     }

//     if (event.type == SDL_MOUSEMOTION && activeModeIndex_ == 1 && draggingEasyTile_) {
//         updateEasyDrag(event.motion.x, event.motion.y);
//         return;
//     }
// }

// void IdiomChainScene::handleResultEvent(const SDL_Event& event) {
//     if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
//         openMainMenu();
//         return;
//     }
//     if (event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT) {
//         return;
//     }

//     const int mx = event.button.x;
//     const int my = event.button.y;
//     if (pointInRect(mx, my, resultAgainButton_.rect)) {
//         openDifficultyMenu();
//     } else if (pointInRect(mx, my, resultRankButton_.rect)) {
//         openLeaderboard();
//     } else if (pointInRect(mx, my, resultMenuButton_.rect)) {
//         openMainMenu();
//     }
// }

// void IdiomChainScene::handleLeaderboardEvent(const SDL_Event& event) {
//     if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
//         openMainMenu();
//         return;
//     }
//     if (event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT) {
//         return;
//     }
//     if (pointInRect(event.button.x, event.button.y, leaderboardBackButton_.rect)) {
//         openMainMenu();
//     }
// }

// void IdiomChainScene::handleMouseMotion(const SDL_MouseMotionEvent& event) {
//     mouseX_ = event.x;
//     mouseY_ = event.y;
// }

// void IdiomChainScene::handleTextEditing(const SDL_TextEditingEvent& event) {
//     if (viewMode_ != ViewMode::Game || activeModeIndex_ != 3 || !hardInputFocused_) {
//         return;
//     }
//     imeComposition_ = event.text;
//     imeCursor_ = event.start;
// }

// void IdiomChainScene::handleMouseWheel(const SDL_MouseWheelEvent& event) {
//     int mx = 0;
//     int my = 0;
//     SDL_GetMouseState(&mx, &my);

//     const int delta = -event.y * 24;
//     auto applyScroll = [&](const SDL_Rect& rect, int& offset, int contentHeight) -> bool {
//         if (!pointInRect(mx, my, rect)) {
//             return false;
//         }
//         clampScrollOffset(offset, contentHeight, rect.h);
//         offset += delta;
//         clampScrollOffset(offset, contentHeight, rect.h);
//         return true;
//     };

//     switch (viewMode_) {
//     case ViewMode::MainMenu:
//         applyScroll(makeRect(120 + 18, 250 + 58, 250 - 36, 280 - 74), mainMenuLeftScroll_, mainMenuLeftContentHeight_) ||
//         applyScroll(makeRect(830 + 18, 250 + 58, 250 - 36, 280 - 74), mainMenuRightScroll_, mainMenuRightContentHeight_);
//         break;
//     case ViewMode::DifficultyMenu:
//         applyScroll(makeRect(200 + 28, 220 + 62, 800 - 56, 240 - 86), difficultyInfoScroll_, difficultyInfoContentHeight_);
//         break;
//     case ViewMode::Game:   
//         applyScroll(makeRect(320 + 24 + 12, 136 + 122 + 34, (560 - 48) - 24, 72 - 40),
//                     gamePathScroll_,
//                     gamePathContentHeight_) ||
//         applyScroll(makeRect(900 + 16, 136 + 52, 240 - 32, 620 - 68),
//                     gameExplanationScroll_,
//                     gameExplanationContentHeight_);
//         break;
//     case ViewMode::Result:
//         applyScroll(makeRect(330 + 30, 180 + 88, 390 - 60, 128 - 46), resultMyRouteScroll_, resultMyRouteContentHeight_) ||
//         applyScroll(makeRect(330 + 30, 180 + 232, 390 - 60, 170 - 46), resultBestRouteScroll_, resultBestRouteContentHeight_) ||
//         applyScroll(makeRect(740 + 16, 180 + 54, 400 - 32, 430 - 70), resultExplanationScroll_, resultExplanationContentHeight_);
//         break;
//     case ViewMode::Leaderboard:
//         applyScroll(makeRect(50 + 20, 180 + 112, 760 - 40, 500 - 132), leaderboardRankScroll_, leaderboardRankContentHeight_) ||
//         applyScroll(makeRect(830 + 16, 180 + 56, 320 - 32, 500 - 72), leaderboardHistoryScroll_, leaderboardHistoryContentHeight_);
//         break;
//     }
// }

// void IdiomChainScene::openMainMenu() {
//     SDL_StopTextInput();
//     viewMode_ = ViewMode::MainMenu;
//     inputBuffer_.clear();
//     imeComposition_.clear();
//     imeCursor_ = 0;
//     hardInputFocused_ = false;
//     resetScrollOffsets();
//     hardCandidates_.clear();
//     mediumButtons_.clear();
//     hardCandidateButtons_.clear();
//     refreshMainMenuButtons();
// }

// void IdiomChainScene::openDifficultyMenu() {
//     SDL_StopTextInput();
//     viewMode_ = ViewMode::DifficultyMenu;
//     imeComposition_.clear();
//     imeCursor_ = 0;
//     hardInputFocused_ = false;
//     resetScrollOffsets();
//     refreshDifficultyButtons();
// }

// void IdiomChainScene::openLeaderboard() {
//     SDL_StopTextInput();
//     viewMode_ = ViewMode::Leaderboard;
//     imeComposition_.clear();
//     imeCursor_ = 0;
//     hardInputFocused_ = false;
//     resetScrollOffsets();
//     refreshLeaderboardCaches();
// }


// void IdiomChainScene::resetScrollOffsets() {
//     mainMenuLeftScroll_ = 0;
//     mainMenuRightScroll_ = 0;
//     difficultyInfoScroll_ = 0;
//     gamePathScroll_ = 0;
//     gameExplanationScroll_ = 0;
//     resultMyRouteScroll_ = 0;
//     resultBestRouteScroll_ = 0;
//     resultExplanationScroll_ = 0;
//     leaderboardHistoryScroll_ = 0;
//     leaderboardRankScroll_ = 0;
// }

// void IdiomChainScene::clampScrollOffset(int& offset, int contentHeight, int viewHeight) const {
//     const int maxOffset = std::max(0, contentHeight - viewHeight);
//     offset = clampInt(offset, 0, maxOffset);
// }

// void IdiomChainScene::startGame(int modeIndex) {
//     activeModeIndex_ = modeIndex;
//     inputBuffer_.clear();
//     imeComposition_.clear();
//     imeCursor_ = 0;
//     hardInputFocused_ = (modeIndex == 3);
//     resetScrollOffsets();
//     statusMessage_.clear();
//     hintMessage_.clear();
//     revealedAnswerText_.clear();
//     lastMediumPathSize_ = -1;
//     lastHardPathSize_ = -1;
//     lastHardQuery_.clear();

//     if (modeIndex == 1) {
//         controller_.startSingleGame(GameMode::SingleEasy, playerName_);
//         SDL_StopTextInput();
//     } else if (modeIndex == 2) {
//         controller_.startSingleGame(GameMode::SingleMedium, playerName_);
//         SDL_StopTextInput();
//     } else {
//         controller_.startSingleGame(GameMode::SingleHard, playerName_);
//         SDL_StartTextInput();

//         SDL_Rect inputRect   = makeRect(320 + 24, 136 + 300, 560 - 48, 54);
//         SDL_Rect composeRect = makeRect(inputRect.x + 10, inputRect.y + 28, inputRect.w - 20, 22);
//         SDL_SetTextInputRect(&composeRect);
//         // SDL_Rect inputRect = makeRect(320 + 24, 136 + 300, 560 - 48, 54);
//         // SDL_SetTextInputRect(&inputRect);
//     }

//     resetEasyTilesFromPool();
//     refreshGameCaches(true);
//     viewMode_ = ViewMode::Game;
// }

// void IdiomChainScene::refreshMainMenuButtons() {
//     mainMenuButtons_.clear();
//     const int w = 240;
//     const int h = 64;
//     const int x = kWindowWidth / 2 - w / 2;
//     int y = 250;
//     mainMenuButtons_.push_back(ButtonSpec{makeRect(x, y, w, h), "单人游戏", 1, true});
//     y += 88;
//     mainMenuButtons_.push_back(ButtonSpec{makeRect(x, y, w, h), "多人游戏（待接入）", 2, false});
//     y += 88;
//     mainMenuButtons_.push_back(ButtonSpec{makeRect(x, y, w, h), "排行榜", 3, true});
//     y += 88;
//     mainMenuButtons_.push_back(ButtonSpec{makeRect(x, y, w, h), "返回", 4, true});
// }

// void IdiomChainScene::refreshDifficultyButtons() {
//     difficultyButtons_.clear();
//     const int w = 240;
//     const int h = 50;
//     const int x = kWindowWidth / 2 - w / 2;
//     int y = 500;
//     difficultyButtons_.push_back(ButtonSpec{makeRect(x, y, w, h), "简单模式", 1, true});
//     y += 62;
//     difficultyButtons_.push_back(ButtonSpec{makeRect(x, y, w, h), "中等模式", 2, true});
//     y += 62;
//     difficultyButtons_.push_back(ButtonSpec{makeRect(x, y, w, h), "困难模式", 3, true});
//     y += 62;
//     difficultyButtons_.push_back(ButtonSpec{makeRect(x, y, w, 50), "返回上一级", 4, true});
// }

// void IdiomChainScene::refreshGameCaches(bool force) {
//     if (activeModeIndex_ == 2) {
//         refreshMediumOptions(force);
//     }
//     if (activeModeIndex_ == 3) {
//         refreshHardCandidates(force);
//     }
// }

// void IdiomChainScene::refreshMediumOptions(bool force) {
//     const GameSession& session = controller_.getSession();
//     const int pathSize = static_cast<int>(session.playerPath.size());
//     if (!force && pathSize == lastMediumPathSize_) {
//         return;
//     }
//     lastMediumPathSize_ = pathSize;
//     mediumOptionIds_ = controller_.getMediumOptions();
//     mediumOptionDistances_ = controller_.getMediumOptionDistances();

//     mediumButtons_.clear();
//     const int startX = 360;
//     const int startY = 405;
//     const int w = 220;
//     const int h = 60;
//     for (std::size_t i = 0; i < mediumOptionIds_.size() && i < 4; ++i) {
//         const int row = static_cast<int>(i) / 2;
//         const int col = static_cast<int>(i) % 2;
//         const std::string word = controller_.wordOf(mediumOptionIds_[i]);
//         mediumButtons_.push_back(ButtonSpec{
//             makeRect(startX + col * (w + 20), startY + row * (h + 36), w, h),
//             word,
//             mediumOptionIds_[i],
//             true
//         });
//     }
// }

// void IdiomChainScene::refreshHardCandidates(bool force) {
//     const GameSession& session = controller_.getSession();
//     const int pathSize = static_cast<int>(session.playerPath.size());
//     if (!force && pathSize == lastHardPathSize_ && inputBuffer_ == lastHardQuery_) {
//         return;
//     }
//     lastHardPathSize_ = pathSize;
//     lastHardQuery_ = inputBuffer_;
//     hardCandidates_ = controller_.getHardCandidateWords(inputBuffer_);

//     hardCandidateButtons_.clear();
//     const int startX = 350;
//     const int startY = 560;
//     const int w = 112;
//     const int h = 42;
//     for (std::size_t i = 0; i < hardCandidates_.size() && i < 8; ++i) {
//         const int row = static_cast<int>(i) / 4;
//         const int col = static_cast<int>(i) % 4;
//         hardCandidateButtons_.push_back(ButtonSpec{
//             makeRect(startX + col * (w + 12), startY + row * (h + 10), w, h),
//             hardCandidates_[i],
//             static_cast<int>(i),
//             true
//         });
//     }
// }

// void IdiomChainScene::refreshLeaderboardCaches() {
//     leaderboardCache_.clear();
//     auto ranking = controller_.buildLeaderboard();
//     while (!ranking.empty()) {
//         leaderboardCache_.push_back(ranking.top());
//         ranking.pop();
//     }

//     historyCache_ = controller_.loadAllRecords();
//     std::reverse(historyCache_.begin(), historyCache_.end());

//     leaderboardBackButton_ = ButtonSpec{makeRect(kWindowWidth / 2 - 80, 710, 160, 52), "返回主菜单", 1, true};
// }

// void IdiomChainScene::resetEasyTilesFromPool() {
//     easySelectedOrder_.clear();
//     easyDiscardOrder_.clear();
//     const auto pool = controller_.getEasyPool();
//     for (std::size_t i = 0; i < pool.size(); ++i) {
//         easyDiscardOrder_.push_back(static_cast<int>(i));
//     }
//     relayoutEasyTiles();
// }

// void IdiomChainScene::relayoutEasyTiles() {
//     easyTiles_.clear();
//     easySelectedZoneRect_ = makeRect(344, 346, 512, 168);
//     easyDiscardZoneRect_ = makeRect(344, 522, 512, 168);

//     auto appendTiles = [&](const std::vector<int>& order, bool inSelected, const SDL_Rect& zone) {
//         for (std::size_t i = 0; i < order.size(); ++i) {
//             const int row = static_cast<int>(i) / 4;
//             const int col = static_cast<int>(i) % 4;
//             EasyTile tile;
//             tile.poolIndex = order[i];
//             tile.inSelectedZone = inSelected;
//             tile.rect = makeRect(zone.x + 18 + col * (kTileWidth + kTileGap),
//                                  zone.y + 36 + row * (kTileHeight + kTileGap),
//                                  kTileWidth,
//                                  kTileHeight);
//             easyTiles_.push_back(tile);
//         }
//     };

//     appendTiles(easySelectedOrder_, true, easySelectedZoneRect_);
//     appendTiles(easyDiscardOrder_, false, easyDiscardZoneRect_);
// }

// void IdiomChainScene::drawText(TTF_Font* font,
//                                const std::string& text,
//                                int x,
//                                int y,
//                                SDL_Color color,
//                                bool centered) const {
//     if (font == nullptr || text.empty()) {
//         return;
//     }
//     SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
//     if (surface == nullptr) {
//         return;
//     }
//     SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
//     if (texture == nullptr) {
//         SDL_FreeSurface(surface);
//         return;
//     }
//     SDL_Rect dst{x, y, surface->w, surface->h};
//     if (centered) {
//         dst.x -= dst.w / 2;
//         dst.y -= dst.h / 2;
//     }
//     SDL_FreeSurface(surface);
//     SDL_RenderCopy(renderer_, texture, nullptr, &dst);
//     SDL_DestroyTexture(texture);
// }

// void IdiomChainScene::drawWrappedText(TTF_Font* font,
//                                       const std::string& text,
//                                       const SDL_Rect& rect,
//                                       SDL_Color color) const {
//     if (font == nullptr || text.empty()) {
//         return;
//     }
//     SDL_Surface* surface = TTF_RenderUTF8_Blended_Wrapped(font, text.c_str(), color, rect.w);
//     if (surface == nullptr) {
//         return;
//     }
//     SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
//     if (texture == nullptr) {
//         SDL_FreeSurface(surface);
//         return;
//     }
//     SDL_Rect dst{rect.x, rect.y, surface->w, surface->h};
//     SDL_FreeSurface(surface);
//     SDL_RenderSetClipRect(renderer_, &rect);
//     SDL_RenderCopy(renderer_, texture, nullptr, &dst);
//     SDL_RenderSetClipRect(renderer_, nullptr);
//     SDL_DestroyTexture(texture);
// }

// int IdiomChainScene::wrappedTextHeight(TTF_Font* font,
//                                        const std::string& text,
//                                        int wrapWidth) const {
//     if (font == nullptr || text.empty() || wrapWidth <= 0) {
//         return 0;
//     }
//     SDL_Surface* surface = TTF_RenderUTF8_Blended_Wrapped(font, text.c_str(), rgb(0, 0, 0), wrapWidth);
//     if (surface == nullptr) {
//         return 0;
//     }
//     const int h = surface->h;
//     SDL_FreeSurface(surface);
//     return h;
// }

// int IdiomChainScene::drawWrappedTextScrollable(TTF_Font* font,
//                                                const std::string& text,
//                                                const SDL_Rect& rect,
//                                                SDL_Color color,
//                                                int& scrollOffset) const {
//     if (font == nullptr || rect.w <= 0 || rect.h <= 0) {
//         return 0;
//     }
//     const std::string safeText = text.empty() ? " " : text;
//     SDL_Surface* surface = TTF_RenderUTF8_Blended_Wrapped(font, safeText.c_str(), color, rect.w - 10);
//     if (surface == nullptr) {
//         return 0;
//     }
//     SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
//     const int contentHeight = surface->h;
//     if (texture == nullptr) {
//         SDL_FreeSurface(surface);
//         return contentHeight;
//     }

//     const int maxOffset = std::max(0, contentHeight - rect.h);
//     scrollOffset = clampInt(scrollOffset, 0, maxOffset);

//     SDL_Rect dst{rect.x, rect.y - scrollOffset, surface->w, surface->h};
//     SDL_FreeSurface(surface);

//     SDL_RenderSetClipRect(renderer_, &rect);
//     SDL_RenderCopy(renderer_, texture, nullptr, &dst);
//     SDL_RenderSetClipRect(renderer_, nullptr);
//     SDL_DestroyTexture(texture);

//     if (contentHeight > rect.h) {
//         SDL_Rect track{rect.x + rect.w - 6, rect.y, 4, rect.h};
//         fillRect(renderer_, track, rgb(232, 232, 232));
//         const int thumbH = std::max(24, rect.h * rect.h / std::max(contentHeight, rect.h));
//         const int thumbY = rect.y + (rect.h - thumbH) * scrollOffset / std::max(1, maxOffset);
//         SDL_Rect thumb{track.x, thumbY, track.w, thumbH};
//         fillRect(renderer_, thumb, rgb(120, 120, 120));
//     }
//     return contentHeight;
// }

// void IdiomChainScene::drawFittedTextInRect(TTF_Font* preferredFont,
//                                            const std::string& text,
//                                            const SDL_Rect& rect,
//                                            SDL_Color color,
//                                            bool centered,
//                                            bool allowWrap) const {
//     if (text.empty() || rect.w <= 0 || rect.h <= 0) {
//         return;
//     }
//     std::vector<TTF_Font*> fonts{preferredFont, bodyFont_, smallFont_};
//     fonts.erase(std::remove(fonts.begin(), fonts.end(), nullptr), fonts.end());
//     fonts.erase(std::unique(fonts.begin(), fonts.end()), fonts.end());

//     for (TTF_Font* font : fonts) {
//         int w = 0;
//         int h = 0;
//         if (TTF_SizeUTF8(font, text.c_str(), &w, &h) == 0 && w <= rect.w - 8 && h <= rect.h - 4) {
//             const int drawX = centered ? rect.x + rect.w / 2 : rect.x + 4;
//             const int drawY = centered ? rect.y + rect.h / 2 : rect.y + (rect.h - h) / 2;
//             drawText(font, text, drawX, drawY, color, centered);
//             return;
//         }
//     }

//     TTF_Font* wrapFont = smallFont_ != nullptr ? smallFont_ : preferredFont;
//     if (wrapFont == nullptr) {
//         return;
//     }
//     if (!allowWrap) {
//         drawText(wrapFont, text, centered ? rect.x + rect.w / 2 : rect.x + 4,
//                  centered ? rect.y + rect.h / 2 : rect.y + 2, color, centered);
//         return;
//     }

//     SDL_Surface* surface = TTF_RenderUTF8_Blended_Wrapped(wrapFont, text.c_str(), color, rect.w - 8);
//     if (surface == nullptr) {
//         return;
//     }
//     SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
//     if (texture == nullptr) {
//         SDL_FreeSurface(surface);
//         return;
//     }
//     SDL_Rect dst{rect.x + 4, rect.y + 2, surface->w, surface->h};
//     if (centered) {
//         dst.x = rect.x + (rect.w - surface->w) / 2;
//         dst.y = rect.y + (rect.h - std::min(surface->h, rect.h - 4)) / 2;
//     }
//     SDL_FreeSurface(surface);
//     SDL_RenderSetClipRect(renderer_, &rect);
//     SDL_RenderCopy(renderer_, texture, nullptr, &dst);
//     SDL_RenderSetClipRect(renderer_, nullptr);
//     SDL_DestroyTexture(texture);
// }

// void IdiomChainScene::drawPanel(const SDL_Rect& rect, const std::string& title) const {
//     fillRect(renderer_, rect, rgb(250, 248, 243));
//     strokeRect(renderer_, rect, rgb(28, 28, 28), 2);
//     drawText(subtitleFont_, title, rect.x + 18, rect.y + 14, rgb(20, 20, 20), false);
// }

// void IdiomChainScene::drawButton(const ButtonSpec& button, bool primary) const {
//     const bool hovered = button.enabled && pointInRect(mouseX_, mouseY_, button.rect);
//     SDL_Color fill = primary ? rgb(220, 233, 224) : rgb(246, 244, 239);
//     SDL_Color text = primary ? rgb(40, 90, 65) : rgb(20, 20, 20);
//     if (!button.enabled) {
//         fill = rgb(225, 225, 225);
//         text = rgb(120, 120, 120);
//     } else if (hovered) {
//         fill = primary ? rgb(198, 223, 208) : rgb(232, 228, 220);
//     }
//     fillRect(renderer_, button.rect, fill);
//     strokeRect(renderer_, button.rect, hovered ? rgb(58, 112, 82) : rgb(30, 30, 30), hovered ? 3 : 2);
//     drawFittedTextInRect(subtitleFont_, button.label,
//                          makeRect(button.rect.x + 6, button.rect.y + 4, button.rect.w - 12, button.rect.h - 8),
//                          text, true, true);
// }

// void IdiomChainScene::drawBadge(const SDL_Rect& rect, const std::string& text) const {
//     fillRect(renderer_, rect, rgb(224, 236, 228));
//     strokeRect(renderer_, rect, rgb(70, 96, 76), 1);
//     drawText(smallFont_, text, rect.x + rect.w / 2, rect.y + rect.h / 2, rgb(40, 80, 56), true);
// }

// void IdiomChainScene::drawCard(const SDL_Rect& rect,
//                                const std::string& title,
//                                const std::string& content,
//                                bool emphasized) const {
//     fillRect(renderer_, rect, emphasized ? rgb(244, 233, 205) : rgb(255, 255, 255));
//     strokeRect(renderer_, rect, rgb(36, 36, 36), 1);
//     if (rect.h <= 44) {
//         drawFittedTextInRect(smallFont_, title + "：" + content,
//                              makeRect(rect.x + 8, rect.y + 4, rect.w - 16, rect.h - 8),
//                              rgb(22, 22, 22), false, true);
//         return;
//     }
//     SDL_Rect titleRect = makeRect(rect.x + 10, rect.y + 8, rect.w - 20, 22);
//     SDL_Rect contentRect = makeRect(rect.x + 12, rect.y + 34, rect.w - 24, std::max(8, rect.h - 40));
//     drawFittedTextInRect(bodyFont_, title, titleRect, rgb(22, 22, 22), false, true);
//     drawWrappedText(smallFont_, content, contentRect, rgb(92, 92, 92));
// }

// void IdiomChainScene::drawSimpleTableRow(const SDL_Rect& rect,
//                                          const std::vector<std::string>& columns,
//                                          const std::vector<int>& widths,
//                                          bool header) const {
//     fillRect(renderer_, rect, header ? rgb(236, 236, 232) : rgb(255, 255, 255));
//     strokeRect(renderer_, rect, rgb(36, 36, 36), 1);
//     int x = rect.x;
//     for (std::size_t i = 0; i < widths.size(); ++i) {
//         if (i > 0) {
//             drawLine(renderer_, x, rect.y, x, rect.y + rect.h, rgb(36, 36, 36));
//         }
//         if (i < columns.size()) {
//             SDL_Rect cell = makeRect(x + 3, rect.y + 2, widths[i] - 6, rect.h - 4);
//             drawFittedTextInRect(header ? bodyFont_ : smallFont_, columns[i], cell, rgb(20, 20, 20), true, true);
//         }
//         x += widths[i];
//     }
// }

// void IdiomChainScene::render() {
//     SDL_SetRenderDrawColor(renderer_, 240, 237, 231, 255);
//     SDL_RenderClear(renderer_);

//     SDL_Rect frame = makeRect(kOuterMargin, kOuterMargin, kWindowWidth - kOuterMargin * 2, kWindowHeight - kOuterMargin * 2);
//     strokeRect(renderer_, frame, rgb(18, 18, 18), 3);

//     switch (viewMode_) {
//     case ViewMode::MainMenu:
//         renderMainMenu();
//         break;
//     case ViewMode::DifficultyMenu:
//         renderDifficultyMenu();
//         break;
//     case ViewMode::Game:
//         renderGame();
//         break;
//     case ViewMode::Result:
//         renderResult();
//         break;
//     case ViewMode::Leaderboard:
//         renderLeaderboard();
//         break;
//     }

//     SDL_RenderPresent(renderer_);
// }

// void IdiomChainScene::renderMainMenu() {
//     drawText(titleFont_, "成语接龙", kWindowWidth / 2, 130, rgb(16, 16, 16), true);
//     drawText(subtitleFont_, "最短路径模块", kWindowWidth / 2, 178, rgb(88, 88, 88), true);

// //     SDL_Rect leftInfo = makeRect(120, 250, 250, 280);
// //     SDL_Rect rightInfo = makeRect(830, 250, 250, 280);
// //     drawPanel(leftInfo, "模块说明");
// //     mainMenuLeftContentHeight_ = drawWrappedTextScrollable(
// //         bodyFont_,
// //         R"(以精确带调拼音接龙为核心。

// // - 简单：拖拽排序
// // - 中等：候选抉择
// // - 困难：自由输入

// // 记录用时、步数、最优解差距与得分。)",
// //         makeRect(leftInfo.x + 18, leftInfo.y + 58, leftInfo.w - 36, leftInfo.h - 74),
// //         rgb(78, 78, 78),
// //         mainMenuLeftScroll_);

// //     drawPanel(rightInfo, "当前状态");
// //     mainMenuRightContentHeight_ = drawWrappedTextScrollable(
// //         bodyFont_,
// //         statusMessage_.empty()
// //             ? R"(多人模式尚未在当前逻辑中实现。

// // 当前前端已完整接入单人三种模式、结算页与排行榜。)"
// //             : statusMessage_,
// //         makeRect(rightInfo.x + 18, rightInfo.y + 58, rightInfo.w - 36, rightInfo.h - 74),
// //         rgb(78, 78, 78),
// //         mainMenuRightScroll_);

//     for (std::size_t i = 0; i < mainMenuButtons_.size(); ++i) {
//         drawButton(mainMenuButtons_[i], i == 0);
//     }
// }

// void IdiomChainScene::renderDifficultyMenu() {
//     drawText(titleFont_, "成语接龙", kWindowWidth / 2, 130, rgb(16, 16, 16), true);
//     drawText(subtitleFont_, "单人游戏 · 选择难度", kWindowWidth / 2, 178, rgb(88, 88, 88), true);

//     SDL_Rect info = makeRect(200, 220, 800, 240);
//     drawPanel(info, "模式说明");
//     difficultyInfoContentHeight_ = drawWrappedTextScrollable(
//         bodyFont_,
//         R"(简单模式：给出最优路径和少量干扰成语，玩家通过拖拽完成排序。

// 中等模式：每一步显示 4 个可达候选，均能到达终点，但步数优劣不同。

// 困难模式：玩家自由输入成语，系统按熟悉度与最短路接近程度提供候选和提示。)",
//         makeRect(info.x + 28, info.y + 62, info.w - 56, info.h - 86),
//         rgb(72, 72, 72),
//         difficultyInfoScroll_);

//     for (std::size_t i = 0; i < difficultyButtons_.size(); ++i) {
//         drawButton(difficultyButtons_[i], i == 0);
//     }
// }

// void IdiomChainScene::renderGame() {
//     const GameSession& session = controller_.getSession();

//     drawText(titleFont_, modeText(activeModeIndex_), kWindowWidth / 2, 82, rgb(20, 20, 20), true);
//     drawText(smallFont_, "成语接龙最短路径", kWindowWidth / 2, 116, rgb(88, 88, 88), true);

//     SDL_Rect leftPanel = makeRect(60, 136, 240, 620);
//     SDL_Rect centerPanel = makeRect(320, 136, 560, 620);
//     SDL_Rect rightPanel = makeRect(900, 136, 240, 620);
//     drawPanel(leftPanel, "局内状态");
//     drawPanel(centerPanel, "答题区");
//     drawPanel(rightPanel, "释义 / 提示");

//     SDL_Rect timeBox = makeRect(leftPanel.x + 18, leftPanel.y + 48, leftPanel.w - 36, 52);
//     SDL_Rect stepBox = makeRect(leftPanel.x + 18, leftPanel.y + 116, 96, 84);
//     SDL_Rect bestBox = makeRect(leftPanel.x + 126, leftPanel.y + 116, 96, 84);
//     fillRect(renderer_, timeBox, rgb(255, 255, 255));
//     fillRect(renderer_, stepBox, rgb(255, 255, 255));
//     fillRect(renderer_, bestBox, rgb(255, 255, 255));
//     strokeRect(renderer_, timeBox, rgb(36, 36, 36), 1);
//     strokeRect(renderer_, stepBox, rgb(36, 36, 36), 1);
//     strokeRect(renderer_, bestBox, rgb(36, 36, 36), 1);

//     drawFittedTextInRect(bodyFont_, "当前用时：" + formatSeconds(session.elapsedSeconds),
//                          makeRect(timeBox.x + 8, timeBox.y + 6, timeBox.w - 16, timeBox.h - 12),
//                          rgb(20, 20, 20), false, true);
//     drawText(smallFont_, "步数", stepBox.x + stepBox.w / 2, stepBox.y + 22, rgb(92, 92, 92), true);
//     drawText(subtitleFont_, std::to_string(session.stepCount), stepBox.x + stepBox.w / 2, stepBox.y + 54, rgb(20, 20, 20), true);
//     drawText(smallFont_, "最优步数", bestBox.x + bestBox.w / 2, bestBox.y + 22, rgb(92, 92, 92), true);
//     drawText(subtitleFont_, std::to_string(session.bestStepCount), bestBox.x + bestBox.w / 2, bestBox.y + 54, rgb(20, 20, 20), true);

//     drawCard(makeRect(leftPanel.x + 18, leftPanel.y + 214, leftPanel.w - 36, 78),
//              "题目",
//              "起点：" + controller_.wordOf(session.startId) + "\n终点：" + controller_.wordOf(session.targetId));

//     std::string statusText = statusMessage_.empty() ? controller_.getLastMessage() : statusMessage_;
//     drawCard(makeRect(leftPanel.x + 18, leftPanel.y + 304, leftPanel.w - 36, 80),
//              "当前状态",
//              statusText.empty() ? "等待操作……" : statusText,
//              !statusText.empty());

//     std::ostringstream helper;
//     if (activeModeIndex_ == 3) {
//         helper << "剩余提示：" << std::max(0, session.maxHints - session.hintCount) << "\n";
//     }
//     helper << "玩家：" << playerName_;
//     drawCard(makeRect(leftPanel.x + 18, leftPanel.y + 400, leftPanel.w - 36, 80),
//              "补充信息",
//              helper.str());

//     gameBackButton_ = ButtonSpec{makeRect(leftPanel.x + 18, leftPanel.y + 504, 78, 46), "主菜单", 1, true};
//     gameHintButton_ = ButtonSpec{makeRect(leftPanel.x + 103, leftPanel.y + 504, 56, 46), "提示", 2, activeModeIndex_ != 1};
//     gameUndoButton_ = ButtonSpec{makeRect(leftPanel.x + 166, leftPanel.y + 504, 56, 46), "撤回", 3, activeModeIndex_ != 1};
//     gameRevealButton_ = ButtonSpec{makeRect(leftPanel.x + 18, leftPanel.y + 560, leftPanel.w - 36, 46), "查看最优解", 4, true};
//     drawButton(gameBackButton_);
//     drawButton(gameHintButton_);
//     drawButton(gameUndoButton_);
//     drawButton(gameRevealButton_);

//     SDL_Rect startBox = makeRect(centerPanel.x + 24, centerPanel.y + 48, 150, 60);
//     SDL_Rect targetBox = makeRect(centerPanel.x + centerPanel.w - 174, centerPanel.y + 48, 150, 60);
//     fillRect(renderer_, startBox, rgb(255, 255, 255));
//     fillRect(renderer_, targetBox, rgb(255, 255, 255));
//     strokeRect(renderer_, startBox, rgb(36, 36, 36), 1);
//     strokeRect(renderer_, targetBox, rgb(36, 36, 36), 1);
//     drawFittedTextInRect(bodyFont_, "起点：" + controller_.wordOf(session.startId),
//                          makeRect(startBox.x + 8, startBox.y + 6, startBox.w - 16, startBox.h - 12),
//                          rgb(20, 20, 20), false, true);
//     drawFittedTextInRect(bodyFont_, "终点：" + controller_.wordOf(session.targetId),
//                          makeRect(targetBox.x + 8, targetBox.y + 6, targetBox.w - 16, targetBox.h - 12),
//                          rgb(20, 20, 20), false, true);
//     drawLine(renderer_, startBox.x + startBox.w + 16, startBox.y + 30, targetBox.x - 16, targetBox.y + 30, rgb(36, 36, 36));
//     drawLine(renderer_, targetBox.x - 26, targetBox.y + 22, targetBox.x - 16, targetBox.y + 30, rgb(36, 36, 36));
//     drawLine(renderer_, targetBox.x - 26, targetBox.y + 38, targetBox.x - 16, targetBox.y + 30, rgb(36, 36, 36));

//     {
//         SDL_Rect pathCard = makeRect(centerPanel.x + 24, centerPanel.y + 122, centerPanel.w - 48, 100);
//         fillRect(renderer_, pathCard, rgb(255, 255, 255));
//         strokeRect(renderer_, pathCard, rgb(36, 36, 36), 1);
//         drawFittedTextInRect(bodyFont_, "当前路径链",
//                              makeRect(pathCard.x + 10, pathCard.y + 8, pathCard.w - 20, 25),
//                              rgb(22, 22, 22), false, true);

//         SDL_Rect pathViewport = makeRect(pathCard.x + 12, pathCard.y + 34, pathCard.w - 24, pathCard.h - 40);
//         const std::string pathText = currentPathText().empty() ? "暂无路径。" : currentPathText();

//         if (activeModeIndex_ == 2 || activeModeIndex_ == 3) {
//             gamePathContentHeight_ = drawWrappedTextScrollable(
//                 smallFont_,
//                 pathText,
//                 pathViewport,
//                 rgb(92, 92, 92),
//                 gamePathScroll_);
//         } else {
//             drawWrappedText(
//                 smallFont_,
//                 pathText,
//                 pathViewport,
//                 rgb(92, 92, 92));
//             gamePathContentHeight_ = wrappedTextHeight(smallFont_, pathText, pathViewport.w - 10);
//             clampScrollOffset(gamePathScroll_, gamePathContentHeight_, pathViewport.h);
//         }
//     }


//     if (activeModeIndex_ == 1) {
//         fillRect(renderer_, easySelectedZoneRect_, rgb(236, 245, 237));
//         fillRect(renderer_, easyDiscardZoneRect_, rgb(248, 240, 235));
//         strokeRect(renderer_, easySelectedZoneRect_, rgb(36, 100, 54), 1);
//         strokeRect(renderer_, easyDiscardZoneRect_, rgb(120, 86, 56), 1);
//         drawFittedTextInRect(bodyFont_, "路径区：把正确路线拖到这里",
//                              makeRect(easySelectedZoneRect_.x + 12, easySelectedZoneRect_.y + 6, easySelectedZoneRect_.w - 24, 28),
//                              rgb(40, 96, 54), false, true);
//         drawFittedTextInRect(bodyFont_, "候选池 / 干扰区",
//                              makeRect(easyDiscardZoneRect_.x + 12, easyDiscardZoneRect_.y + 6, easyDiscardZoneRect_.w - 24, 28),
//                              rgb(112, 80, 54), false, true);

//         const auto poolWords = controller_.getEasyPoolWords();
//         for (const auto& tile : easyTiles_) {
//             if (draggingEasyTile_ && tile.poolIndex == draggedEasyPoolIndex_) {
//                 continue;
//             }
//             const std::string& word = (tile.poolIndex >= 0 && static_cast<std::size_t>(tile.poolIndex) < poolWords.size())
//                                           ? poolWords[static_cast<std::size_t>(tile.poolIndex)]
//                                           : std::string("?");
//             drawCard(tile.rect, word, tile.inSelectedZone ? "已放入路径区" : "候选成语", false);
//         }
//         if (draggingEasyTile_ && draggedEasyPoolIndex_ >= 0 && static_cast<std::size_t>(draggedEasyPoolIndex_) < poolWords.size()) {
//             drawCard(makeRect(dragMouseX_ - kTileWidth / 2,
//                               dragMouseY_ - kTileHeight / 2,
//                               kTileWidth,
//                               kTileHeight),
//                      poolWords[static_cast<std::size_t>(draggedEasyPoolIndex_)],
//                      "拖动中",
//                      true);
//         }

//         easySubmitButton_ = ButtonSpec{makeRect(centerPanel.x + 86, centerPanel.y + 570, 140, 46), "提交排序", 1, true};
//         easyResetButton_ = ButtonSpec{makeRect(centerPanel.x + 246, centerPanel.y + 570, 140, 46), "全部归位", 2, true};
//         drawButton(easySubmitButton_, true);
//         drawButton(easyResetButton_);
//     } else if (activeModeIndex_ == 2) {
//         drawText(bodyFont_, "本步候选（点击提交下一步）", centerPanel.x + 24, centerPanel.y + 240, rgb(70, 70, 70), false);
//         for (std::size_t i = 0; i < mediumButtons_.size(); ++i) {
//             drawButton(mediumButtons_[i], i == 0);
//             // if (i < mediumOptionDistances_.size()) {
//             //     drawText(smallFont_,
//             //              mediumOptionDistances_[i] >= 0
//             //                  ? ("距终点约 " + std::to_string(mediumOptionDistances_[i]) + " 步")
//             //                  : "不可达",
//             //              mediumButtons_[i].rect.x + mediumButtons_[i].rect.w / 2,
//             //              mediumButtons_[i].rect.y + mediumButtons_[i].rect.h + 12,
//             //              rgb(92, 92, 92), true);
//             // }
//         }
//     } else {
//          SDL_Rect inputBox = makeRect(centerPanel.x + 24, centerPanel.y + 300, centerPanel.w - 48, 54);
//     fillRect(renderer_, inputBox, hardInputFocused_ ? rgb(250, 248, 243) : rgb(255, 255, 255));
//     strokeRect(renderer_, inputBox,
//                hardInputFocused_ ? rgb(58, 112, 82) : rgb(36, 36, 36),
//                hardInputFocused_ ? 2 : 1);

//     SDL_Rect committedRect = makeRect(inputBox.x + 10, inputBox.y + 6,  inputBox.w - 20, 22);
//     SDL_Rect composeRect   = makeRect(inputBox.x + 10, inputBox.y + 28, inputBox.w - 20, 22);

//     // 让系统原生输入法候选条显示，并尽量贴在“拼音/组合”这一行附近
//     if (hardInputFocused_) {
//         // SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
//         SDL_SetTextInputRect(&composeRect);
//     }

//     const std::string committedText =
//         inputBuffer_.empty() ? "中文结果：" : ("中文结果：" + inputBuffer_);
//     const std::string composeText =
//         imeComposition_.empty() ? "拼音/组合：等待输入法上屏…" : ("拼音/组合：" + imeComposition_);

//     drawFittedTextInRect(
//         smallFont_,
//         committedText,
//         committedRect,
//         inputBuffer_.empty() ? rgb(130, 130, 130) : rgb(20, 20, 20),
//         false,
//         true
//     );

//     drawFittedTextInRect(
//         smallFont_,
//         composeText,
//         composeRect,
//         imeComposition_.empty() ? rgb(130, 130, 130) : rgb(48, 96, 160),
//         false,
//         true
//     );

//     if (hardInputFocused_) {
//         drawLine(renderer_,
//                  inputBox.x + 8, inputBox.y + inputBox.h - 6,
//                  inputBox.x + inputBox.w - 8, inputBox.y + inputBox.h - 6,
//                  rgb(58, 112, 82));
//     }

//     hardSubmitButton_ = ButtonSpec{
//         makeRect(centerPanel.x + centerPanel.w - 164, centerPanel.y + 368, 140, 44),
//         "提交输入", 1, true
//     };
//     drawButton(hardSubmitButton_, true);

//     for (std::size_t i = 0; i < hardCandidateButtons_.size(); ++i) {
//         drawButton(hardCandidateButtons_[i], i == 0);
//     }
//     }

//     std::string explanationText;
//     const auto explanations = controller_.currentPathExplanations();
//     if (!explanations.empty()) {
//         explanationText += explanations.back();
//     } else {
//         explanationText += "当前路径还没有可显示的释义。";
//     }
//     if (!hintMessage_.empty()) {
//         explanationText += "\n\n" + hintMessage_;
//     }
//     if (!revealedAnswerText_.empty()) {
//         explanationText += "\n\n最优解：\n" + revealedAnswerText_;
//     }
//     gameExplanationContentHeight_ = drawWrappedTextScrollable(bodyFont_, explanationText,
//                     makeRect(rightPanel.x + 16, rightPanel.y + 52, rightPanel.w - 32, rightPanel.h - 68),
//                     rgb(72, 72, 72),
//                     gameExplanationScroll_);
// }

// void IdiomChainScene::renderResult() {
//     const GameSession& session = controller_.getSession();
//     const int diffSteps = session.bestStepCount >= 0 ? std::max(0, session.stepCount - session.bestStepCount) : 0;

//     drawText(titleFont_, "本局结算", kWindowWidth / 2, 88, rgb(20, 20, 20), true);
//     drawText(subtitleFont_, session.success ? "挑战成功" : "挑战失败", kWindowWidth / 2, 128, session.success ? rgb(34, 108, 62) : rgb(156, 44, 44), true);

//     SDL_Rect leftPanel = makeRect(60, 180, 250, 430);
//     SDL_Rect centerPanel = makeRect(330, 180, 390, 430);
//     SDL_Rect rightPanel = makeRect(740, 180, 400, 430);
//     drawPanel(leftPanel, "成绩总览");
//     drawPanel(centerPanel, "路径对照");
//     drawPanel(rightPanel, "成语释义");

//     drawCard(makeRect(leftPanel.x + 18, leftPanel.y + 54, leftPanel.w - 36, 70), "你的用时", formatSeconds(session.elapsedSeconds));
//     drawCard(makeRect(leftPanel.x + 18, leftPanel.y + 136, leftPanel.w - 36, 70), "路径长度", std::to_string(session.stepCount));
//     drawCard(makeRect(leftPanel.x + 18, leftPanel.y + 218, leftPanel.w - 36, 70), "最优解步数", std::to_string(session.bestStepCount));
//     drawCard(makeRect(leftPanel.x + 18, leftPanel.y + 300, leftPanel.w - 36, 70), "得分", std::to_string(session.score), true);
//     drawCard(makeRect(leftPanel.x + 18, leftPanel.y + 382, leftPanel.w - 36, 34), "与最优解差距", std::to_string(diffSteps) + " 步");

//     {
//         SDL_Rect myRouteCard = makeRect(centerPanel.x + 18, centerPanel.y + 54, centerPanel.w - 36, 128);
//         fillRect(renderer_, myRouteCard, rgb(255, 255, 255));
//         strokeRect(renderer_, myRouteCard, rgb(36, 36, 36), 1);
//         drawFittedTextInRect(bodyFont_, "你的路线",
//                              makeRect(myRouteCard.x + 10, myRouteCard.y + 8, myRouteCard.w - 20, 22),
//                              rgb(22, 22, 22), false, true);
//         resultMyRouteContentHeight_ = drawWrappedTextScrollable(
//             smallFont_,
//             currentPathText().empty() ? "暂无记录。" : currentPathText(),
//             makeRect(myRouteCard.x + 12, myRouteCard.y + 34, myRouteCard.w - 24, myRouteCard.h - 42),
//             rgb(92, 92, 92),
//             resultMyRouteScroll_);
//     }
//     {
//         SDL_Rect bestRouteCard = makeRect(centerPanel.x + 18, centerPanel.y + 198, centerPanel.w - 36, 170);
//         fillRect(renderer_, bestRouteCard, rgb(255, 255, 255));
//         strokeRect(renderer_, bestRouteCard, rgb(36, 36, 36), 1);
//         drawFittedTextInRect(bodyFont_, "最优路线",
//                              makeRect(bestRouteCard.x + 10, bestRouteCard.y + 8, bestRouteCard.w - 20, 22),
//                              rgb(22, 22, 22), false, true);
//         resultBestRouteContentHeight_ = drawWrappedTextScrollable(
//             smallFont_,
//             joinWords(answerPathWords(), " -> "),
//             makeRect(bestRouteCard.x + 12, bestRouteCard.y + 34, bestRouteCard.w - 24, bestRouteCard.h - 42),
//             rgb(92, 92, 92),
//             resultBestRouteScroll_);
//     }

//     std::string explanationText;
//     const auto explanations = controller_.currentPathExplanations();
//     if (explanations.empty()) {
//         explanationText = "暂无释义。";
//     } else {
//         const std::size_t limit = std::min<std::size_t>(5, explanations.size());
//         for (std::size_t i = 0; i < limit; ++i) {
//             if (i > 0U) {
//                 explanationText += "\n\n";
//             }
//             explanationText += explanations[i];
//         }
//     }
//     resultExplanationContentHeight_ = drawWrappedTextScrollable(bodyFont_, explanationText,
//                     makeRect(rightPanel.x + 16, rightPanel.y + 54, rightPanel.w - 32, rightPanel.h - 70),
//                     rgb(72, 72, 72),
//                     resultExplanationScroll_);

//     resultAgainButton_ = ButtonSpec{makeRect(kWindowWidth / 2 - 220, 660, 140, 50), "再来一局", 1, true};
//     resultRankButton_ = ButtonSpec{makeRect(kWindowWidth / 2 - 70, 660, 140, 50), "排行榜", 2, true};
//     resultMenuButton_ = ButtonSpec{makeRect(kWindowWidth / 2 + 80, 660, 140, 50), "主菜单", 3, true};
//     drawButton(resultAgainButton_, true);
//     drawButton(resultRankButton_);
//     drawButton(resultMenuButton_);
// }

// void IdiomChainScene::renderLeaderboard() {
//     drawText(titleFont_, "排行榜", kWindowWidth / 2, 88, rgb(20, 20, 20), true);
//     drawText(subtitleFont_, "总积分排行 + 最近对局记录", kWindowWidth / 2, 128, rgb(88, 88, 88), true);

//     SDL_Rect rankPanel = makeRect(50, 180, 760, 500);
//     SDL_Rect historyPanel = makeRect(830, 180, 320, 500);
//     drawPanel(rankPanel, "总积分排行榜");
//     drawPanel(historyPanel, "最近记录");

//     const std::vector<int> widths{70, 160, 120, 80, 290};
//     SDL_Rect header = makeRect(rankPanel.x + 20, rankPanel.y + 54, rankPanel.w - 40, 50);
//     drawSimpleTableRow(header, {"排名", "玩家", "总得分", "胜场", "称号"}, widths, true);

//     const SDL_Rect rankViewport = makeRect(header.x, header.y + 58, header.w, rankPanel.h - 132);
//     leaderboardRankContentHeight_ = static_cast<int>(leaderboardCache_.size()) * 50;
//     clampScrollOffset(leaderboardRankScroll_, leaderboardRankContentHeight_, rankViewport.h);
//     SDL_RenderSetClipRect(renderer_, &rankViewport);
//     int rowY = header.y + 58 - leaderboardRankScroll_;
//     for (std::size_t i = 0; i < leaderboardCache_.size(); ++i) {
//         SDL_Rect row = makeRect(header.x, rowY, header.w, 44);
//         drawSimpleTableRow(row,
//                            {
//                                std::to_string(static_cast<int>(i) + 1),
//                                leaderboardCache_[i].playerName,
//                                std::to_string(leaderboardCache_[i].totalScore),
//                                "-",
//                                (i == 0 ? "榜首" : i == 1 ? "强者" : i == 2 ? "高手" : "玩家")
//                            },
//                            widths,
//                            false);
//         rowY += 50;
//     }
//     SDL_RenderSetClipRect(renderer_, nullptr);
//     if (leaderboardRankContentHeight_ > rankViewport.h) {
//         SDL_Rect track{rankViewport.x + rankViewport.w - 6, rankViewport.y, 4, rankViewport.h};
//         fillRect(renderer_, track, rgb(232, 232, 232));
//         const int maxOffset = std::max(1, leaderboardRankContentHeight_ - rankViewport.h);
//         const int thumbH = std::max(24, rankViewport.h * rankViewport.h / leaderboardRankContentHeight_);
//         const int thumbY = rankViewport.y + (rankViewport.h - thumbH) * leaderboardRankScroll_ / maxOffset;
//         fillRect(renderer_, makeRect(track.x, thumbY, track.w, thumbH), rgb(120, 120, 120));
//     }

//     const SDL_Rect historyViewport = makeRect(historyPanel.x + 16, historyPanel.y + 56, historyPanel.w - 32, historyPanel.h - 72);
//     if (historyCache_.empty()) {
//         leaderboardHistoryContentHeight_ = drawWrappedTextScrollable(bodyFont_, "暂无记录。完成至少一局后这里会显示最近的对局信息。",
//                         makeRect(historyPanel.x + 16, historyPanel.y + 56, historyPanel.w - 32, 120), rgb(82, 82, 82), leaderboardHistoryScroll_);
//     } else {
//         leaderboardHistoryContentHeight_ = static_cast<int>(historyCache_.size()) * 74;
//         clampScrollOffset(leaderboardHistoryScroll_, leaderboardHistoryContentHeight_, historyViewport.h);
//         SDL_RenderSetClipRect(renderer_, &historyViewport);
//         int hy = historyPanel.y + 56 - leaderboardHistoryScroll_;
//         for (std::size_t i = 0; i < historyCache_.size(); ++i) {
//             SDL_Rect card = makeRect(historyPanel.x + 16, hy, historyPanel.w - 32, 66);
//             const GameRecord& rec = historyCache_[i];
//             std::ostringstream oss;
//             oss << rec.playerName << "｜" << rec.difficulty << "\n"
//                 << rec.startWord << " -> " << rec.targetWord << "\n"
//                 << "用时：" << formatSeconds(rec.elapsedSeconds) << "    得分：" << rec.score;
//             drawCard(card, "对局记录", oss.str(), false);
//             hy += 74;
//         }
//         SDL_RenderSetClipRect(renderer_, nullptr);
//         if (leaderboardHistoryContentHeight_ > historyViewport.h) {
//             SDL_Rect track{historyViewport.x + historyViewport.w - 6, historyViewport.y, 4, historyViewport.h};
//             fillRect(renderer_, track, rgb(232, 232, 232));
//             const int maxOffset = std::max(1, leaderboardHistoryContentHeight_ - historyViewport.h);
//             const int thumbH = std::max(24, historyViewport.h * historyViewport.h / leaderboardHistoryContentHeight_);
//             const int thumbY = historyViewport.y + (historyViewport.h - thumbH) * leaderboardHistoryScroll_ / maxOffset;
//             fillRect(renderer_, makeRect(track.x, thumbY, track.w, thumbH), rgb(120, 120, 120));
//         }
//     }

//     drawButton(leaderboardBackButton_);
// }

// void IdiomChainScene::beginEasyDrag(int mouseX, int mouseY) {
//     draggingEasyTile_ = false;
//     draggedEasyPoolIndex_ = -1;
//     for (auto it = easyTiles_.rbegin(); it != easyTiles_.rend(); ++it) {
//         if (pointInRect(mouseX, mouseY, it->rect)) {
//             draggingEasyTile_ = true;
//             draggedEasyPoolIndex_ = it->poolIndex;
//             draggedFromSelectedZone_ = it->inSelectedZone;
//             dragMouseX_ = mouseX;
//             dragMouseY_ = mouseY;
//             return;
//         }
//     }
// }

// void IdiomChainScene::updateEasyDrag(int mouseX, int mouseY) {
//     dragMouseX_ = mouseX;
//     dragMouseY_ = mouseY;
// }

// void IdiomChainScene::endEasyDrag() {
//     if (!draggingEasyTile_ || draggedEasyPoolIndex_ < 0) {
//         draggingEasyTile_ = false;
//         return;
//     }

//     auto eraseValue = [](std::vector<int>& values, int target) {
//         values.erase(std::remove(values.begin(), values.end(), target), values.end());
//     };
//     eraseValue(easySelectedOrder_, draggedEasyPoolIndex_);
//     eraseValue(easyDiscardOrder_, draggedEasyPoolIndex_);

//     auto insertIntoOrder = [&](std::vector<int>& order, const SDL_Rect& zone, int cols) {
//         const int localX = dragMouseX_ - (zone.x + 18);
//         const int localY = dragMouseY_ - (zone.y + 44);
//         const int col = std::max(0, std::min(cols - 1, localX / (kTileWidth + kTileGap)));
//         const int row = std::max(0, localY / (kTileHeight + kTileGap));
//         int pos = row * cols + col;
//         pos = std::max(0, std::min(pos, static_cast<int>(order.size())));
//         order.insert(order.begin() + pos, draggedEasyPoolIndex_);
//     };

//     if (pointInRect(dragMouseX_, dragMouseY_, easySelectedZoneRect_)) {
//         insertIntoOrder(easySelectedOrder_, easySelectedZoneRect_, 4);
//     } else if (pointInRect(dragMouseX_, dragMouseY_, easyDiscardZoneRect_)) {
//         insertIntoOrder(easyDiscardOrder_, easyDiscardZoneRect_, 4);
//     } else {
//         if (draggedFromSelectedZone_) {
//             easySelectedOrder_.push_back(draggedEasyPoolIndex_);
//         } else {
//             easyDiscardOrder_.push_back(draggedEasyPoolIndex_);
//         }
//     }

//     relayoutEasyTiles();
//     draggingEasyTile_ = false;
//     draggedEasyPoolIndex_ = -1;
// }

// bool IdiomChainScene::submitHardInputBuffer() {
//     if (inputBuffer_.empty()) {
//         statusMessage_ = "请输入成语后再提交。";
//         return false;
//     }
//     const bool ok = controller_.submitHardInput(inputBuffer_);
//     statusMessage_ = ok ? "已提交一步。" : controller_.getLastMessage();
//     if (ok) {
//         inputBuffer_.clear();
//         imeComposition_.clear();
//         imeCursor_ = 0;
//         revealedAnswerText_.clear();
//     }
//     refreshHardCandidates(true);
//     return ok;
// }

// std::string IdiomChainScene::modeText(int modeIndex) const {
//     switch (modeIndex) {
//     case 1: return "简单模式";
//     case 2: return "中等模式";
//     case 3: return "困难模式";
//     default: return "成语接龙";
//     }
// }

// std::string IdiomChainScene::currentPathText() const {
//     return joinWords(currentPathWords(), " -> ");
// }

// std::vector<std::string> IdiomChainScene::currentPathWords() const {
//     std::vector<std::string> words;
//     const GameSession& session = controller_.getSession();
//     for (int idiomId : session.playerPath) {
//         words.push_back(controller_.wordOf(idiomId));
//     }
//     return words;
// }

// std::vector<std::string> IdiomChainScene::answerPathWords() const {
//     return controller_.bestPathWords();
// }
#include "IdiomChainScene.h"

#include "../app/GameSession.h"
#include "../app/IdiomChainController.h"
#include "../data/RecordRepository.h"

#include <SDL.h>
#include <SDL_ttf.h>

#include <algorithm>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

namespace {
constexpr int kWindowWidth = 1200;
constexpr int kWindowHeight = 800;
constexpr int kOuterMargin = 36;
constexpr int kInnerGap = 20;
constexpr int kButtonHeight = 58;
constexpr int kTileWidth = 110;
constexpr int kTileHeight = 58;
constexpr int kTileGap = 10;

SDL_Rect makeRect(int x, int y, int w, int h) {
    return SDL_Rect{x, y, w, h};
}

bool pointInRect(int x, int y, const SDL_Rect& rect) {
    return x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h;
}

SDL_Color rgb(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255) {
    return SDL_Color{r, g, b, a};
}

void fillRect(SDL_Renderer* renderer, const SDL_Rect& rect, SDL_Color color) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

void strokeRect(SDL_Renderer* renderer, const SDL_Rect& rect, SDL_Color color, int thickness = 1) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (int i = 0; i < thickness; ++i) {
        SDL_Rect r{rect.x - i, rect.y - i, rect.w + i * 2, rect.h + i * 2};
        SDL_RenderDrawRect(renderer, &r);
    }
}

void drawLine(SDL_Renderer* renderer, int x1, int y1, int x2, int y2, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
}

std::string formatSeconds(double seconds) {
    if (seconds < 0.0) {
        seconds = 0.0;
    }
    const int total = static_cast<int>(seconds);
    const int mm = total / 60;
    const int ss = total % 60;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", mm, ss);
    return buf;
}

std::string joinWords(const std::vector<std::string>& words, const std::string& sep) {
    std::ostringstream oss;
    for (std::size_t i = 0; i < words.size(); ++i) {
        if (i > 0U) {
            oss << sep;
        }
        oss << words[i];
    }
    return oss.str();
}

int clampInt(int value, int low, int high) {
    return std::max(low, std::min(value, high));
}

bool isBattleMode(GameMode mode) {
    return mode == GameMode::BattleEasy
        || mode == GameMode::BattleMedium
        || mode == GameMode::BattleHard;
}

} // namespace

IdiomChainScene::IdiomChainScene(IdiomChainController& controller)
    : controller_(controller)
    , window_(nullptr)
    , renderer_(nullptr)
    , titleFont_(nullptr)
    , subtitleFont_(nullptr)
    , bodyFont_(nullptr)
    , smallFont_(nullptr)
    , viewMode_(ViewMode::MainMenu)
    , activeModeIndex_(2)
    , lastMediumPathSize_(-1)
    , lastHardPathSize_(-1)
    , playerName_("Player") {
}

IdiomChainScene::~IdiomChainScene() {
    shutdown();
}

int IdiomChainScene::run() {
    if (!initialize()) {
        return -1;
    }

    openMainMenu();
    bool running = true;
    int resultCode = 1;

    while (running) {
        SDL_Event event{};
        while (SDL_PollEvent(&event) != 0) {
            handleEvent(event, running, resultCode);
        }

        if (viewMode_ == ViewMode::BattleLobby || isBattleMode(controller_.getSession().mode)) {
            controller_.pollBattle();
        }

        if (viewMode_ == ViewMode::BattleLobby && controller_.isBattleRoundStarted()) {
            const GameSession& session = controller_.getSession();
            activeModeIndex_ = battleModeIndexToLocal(session.mode);
            playerName_ = session.playerName;
            inputBuffer_.clear();
            imeComposition_.clear();
            imeCursor_ = 0;
            hardInputFocused_ = (activeModeIndex_ == 3);
            resetEasyTilesFromPool();
            refreshGameCaches(true);
            statusMessage_.clear();
            hintMessage_.clear();
            revealedAnswerText_.clear();
            if (activeModeIndex_ == 3) {
                SDL_StartTextInput();
                SDL_Rect inputRect = makeRect(320 + 24, 136 + 300, 560 - 48, 54);
                SDL_Rect composeRect = makeRect(inputRect.x + 10, inputRect.y + 28, inputRect.w - 20, 22);
                SDL_SetTextInputRect(&composeRect);
            } else {
                SDL_StopTextInput();
            }
            viewMode_ = ViewMode::Game;
        }

        if (viewMode_ == ViewMode::Game) {
            controller_.tick();
            const GameSession& session = controller_.getSession();
            if (session.finished) {
                if (!isBattleMode(session.mode) || session.remoteFinished) {
                    SDL_StopTextInput();
                    viewMode_ = ViewMode::Result;
                } else {
                    statusMessage_ = "你已完成，等待对手结束后结算。";
                }
            }
        }

        render();
        SDL_Delay(16);
    }

    shutdown();
    return resultCode;
}

bool IdiomChainScene::initialize() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        return false;
    }
    if (TTF_Init() != 0) {
        SDL_Quit();
        return false;
    }
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

    window_ = SDL_CreateWindow(
        "LineVerse - IdiomChainGame",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        kWindowWidth,
        kWindowHeight,
        SDL_WINDOW_SHOWN
    );
    if (window_ == nullptr) {
        return false;
    }

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer_ == nullptr) {
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    }
    if (renderer_ == nullptr) {
        return false;
    }

    return loadFonts();
}

void IdiomChainScene::shutdown() {
    SDL_StopTextInput();
    if (smallFont_ != nullptr) {
        TTF_CloseFont(smallFont_);
        smallFont_ = nullptr;
    }
    if (bodyFont_ != nullptr) {
        TTF_CloseFont(bodyFont_);
        bodyFont_ = nullptr;
    }
    if (subtitleFont_ != nullptr) {
        TTF_CloseFont(subtitleFont_);
        subtitleFont_ = nullptr;
    }
    if (titleFont_ != nullptr) {
        TTF_CloseFont(titleFont_);
        titleFont_ = nullptr;
    }
    if (renderer_ != nullptr) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    if (TTF_WasInit() != 0) {
        TTF_Quit();
    }
    if (SDL_WasInit(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0U) {
        SDL_Quit();
    }
}

bool IdiomChainScene::loadFonts() {
    for (const std::string& path : candidateFontPaths()) {
        titleFont_ = TTF_OpenFont(path.c_str(), 42);
        subtitleFont_ = TTF_OpenFont(path.c_str(), 26);
        bodyFont_ = TTF_OpenFont(path.c_str(), 20);
        smallFont_ = TTF_OpenFont(path.c_str(), 15);
        if (titleFont_ != nullptr && subtitleFont_ != nullptr && bodyFont_ != nullptr && smallFont_ != nullptr) {
            return true;
        }
        if (titleFont_ != nullptr) {
            TTF_CloseFont(titleFont_);
            titleFont_ = nullptr;
        }
        if (subtitleFont_ != nullptr) {
            TTF_CloseFont(subtitleFont_);
            subtitleFont_ = nullptr;
        }
        if (bodyFont_ != nullptr) {
            TTF_CloseFont(bodyFont_);
            bodyFont_ = nullptr;
        }
        if (smallFont_ != nullptr) {
            TTF_CloseFont(smallFont_);
            smallFont_ = nullptr;
        }
    }
    return false;
}

std::vector<std::string> IdiomChainScene::candidateFontPaths() {
    return {
        "assets/IdiomChainGame/fonts/NotoSansSC-Regular.otf",
        "assets/IdiomChainGame/fonts/NotoSansCJKsc-Regular.otf",
        "assets/fonts/NotoSansSC-Regular.otf",
        "assets/fonts/msyh.ttc",
        "C:/Windows/Fonts/msyh.ttc",
        "C:/Windows/Fonts/msyhbd.ttc",
        "C:/Windows/Fonts/simhei.ttf",
        "C:/Windows/Fonts/simsun.ttc"
    };
}

void IdiomChainScene::handleEvent(const SDL_Event& event, bool& running, int& resultCode) {
    if (event.type == SDL_QUIT) {
        running = false;
        resultCode = 0;
        return;
    }

    if (viewMode_ == ViewMode::BattleSetup && event.type == SDL_TEXTINPUT) {
        std::string* target = nullptr;
        if (activeTextField_ == TextField::PlayerName) {
            target = &battlePlayerInput_;
        } else if (activeTextField_ == TextField::HostIp) {
            target = &battleHostIpInput_;
        } else if (activeTextField_ == TextField::Port) {
            target = &battlePortInput_;
        }
        if (target != nullptr) {
            *target += event.text.text;
            return;
        }
    }

    if (event.type == SDL_MOUSEMOTION) {
        handleMouseMotion(event.motion);
    } else if (event.type == SDL_MOUSEWHEEL) {
        handleMouseWheel(event.wheel);
        return;
    } else if (event.type == SDL_TEXTEDITING) {
        handleTextEditing(event.edit);
        return;
    }

    switch (viewMode_) {
    case ViewMode::MainMenu:
        handleMainMenuEvent(event, running, resultCode);
        break;
    case ViewMode::DifficultyMenu:
        handleDifficultyMenuEvent(event);
        break;
    case ViewMode::BattleSetup:
        handleBattleSetupEvent(event);
        break;
    case ViewMode::BattleLobby:
        handleBattleLobbyEvent(event);
        break;
    case ViewMode::Game:
        handleGameEvent(event);
        break;
    case ViewMode::Result:
        handleResultEvent(event);
        break;
    case ViewMode::Leaderboard:
        handleLeaderboardEvent(event);
        break;
    }
}

void IdiomChainScene::handleMainMenuEvent(const SDL_Event& event, bool& running, int& resultCode) {
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_ESCAPE) {
            running = false;
            resultCode = 1;
            return;
        }
    }

    if (event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT) {
        return;
    }

    for (const auto& button : mainMenuButtons_) {
        if (!button.enabled || !pointInRect(event.button.x, event.button.y, button.rect)) {
            continue;
        }
        switch (button.actionId) {
        case 1:
            openDifficultyMenu();
            return;
        case 2:
            openBattleSetup();
            return;
        case 3:
            openLeaderboard();
            return;
        case 4:
            running = false;
            resultCode = 1;
            return;
        default:
            break;
        }
    }
}

void IdiomChainScene::handleDifficultyMenuEvent(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
        openMainMenu();
        return;
    }
    if (event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT) {
        return;
    }

    for (const auto& button : difficultyButtons_) {
        if (!button.enabled || !pointInRect(event.button.x, event.button.y, button.rect)) {
            continue;
        }
        if (button.actionId >= 1 && button.actionId <= 3) {
            startGame(button.actionId);
        } else {
            openMainMenu();
        }
        return;
    }
}

void IdiomChainScene::handleBattleSetupEvent(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_ESCAPE) {
            openMainMenu();
            return;
        }
        if (event.key.keysym.sym == SDLK_BACKSPACE) {
            std::string* target = nullptr;
            if (activeTextField_ == TextField::PlayerName) {
                target = &battlePlayerInput_;
            } else if (activeTextField_ == TextField::HostIp) {
                target = &battleHostIpInput_;
            } else if (activeTextField_ == TextField::Port) {
                target = &battlePortInput_;
            }
            if (target != nullptr && !target->empty()) {
                target->pop_back();
            }
            return;
        }
        if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
            const unsigned short port = static_cast<unsigned short>(std::max(1, std::atoi(battlePortInput_.c_str())));
            const std::string player = battlePlayerInput_.empty() ? "Player" : battlePlayerInput_;
            playerName_ = player;
            bool ok = false;
            if (battleHostSelected_) {
                ok = controller_.hostBattle(selectedBattleMode(), player, port);
            } else {
                ok = controller_.joinBattle(selectedBattleMode(), player,
                                            battleHostIpInput_.empty() ? "127.0.0.1" : battleHostIpInput_, port);
            }
            statusMessage_ = controller_.getLastMessage();
            if (ok) {
                openBattleLobby();
            }
            return;
        }
    }

    if (event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT) {
        return;
    }

    const int mx = event.button.x;
    const int my = event.button.y;

    if (pointInRect(mx, my, battlePlayerField_)) {
        activeTextField_ = TextField::PlayerName;
        return;
    }
    if (pointInRect(mx, my, battleIpField_)) {
        activeTextField_ = TextField::HostIp;
        return;
    }
    if (pointInRect(mx, my, battlePortField_)) {
        activeTextField_ = TextField::Port;
        return;
    }

    if (pointInRect(mx, my, battleHostButton_.rect)) {
        battleHostSelected_ = true;
        return;
    }
    if (pointInRect(mx, my, battleJoinButton_.rect)) {
        battleHostSelected_ = false;
        return;
    }

    if (pointInRect(mx, my, battleModeEasyButton_.rect)) {
        battleModeIndex_ = 1;
        return;
    }
    if (pointInRect(mx, my, battleModeMediumButton_.rect)) {
        battleModeIndex_ = 2;
        return;
    }
    if (pointInRect(mx, my, battleModeHardButton_.rect)) {
        battleModeIndex_ = 3;
        return;
    }

    if (pointInRect(mx, my, battleConfirmButton_.rect)) {
        const unsigned short port = static_cast<unsigned short>(std::max(1, std::atoi(battlePortInput_.c_str())));
        const std::string player = battlePlayerInput_.empty() ? "Player" : battlePlayerInput_;
        playerName_ = player;
        bool ok = false;
        if (battleHostSelected_) {
            ok = controller_.hostBattle(selectedBattleMode(), player, port);
        } else {
            ok = controller_.joinBattle(selectedBattleMode(), player,
                                        battleHostIpInput_.empty() ? "127.0.0.1" : battleHostIpInput_, port);
        }
        statusMessage_ = controller_.getLastMessage();
        if (ok) {
            openBattleLobby();
        }
        return;
    }

    if (pointInRect(mx, my, battleSetupBackButton_.rect)) {
        openMainMenu();
        return;
    }

    activeTextField_ = TextField::None;
}

void IdiomChainScene::handleBattleLobbyEvent(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
        controller_.leaveBattle();
        openMainMenu();
        return;
    }
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        if (pointInRect(event.button.x, event.button.y, battleLobbyBackButton_.rect)) {
            controller_.leaveBattle();
            openMainMenu();
        }
    }
}

void IdiomChainScene::handleGameEvent(const SDL_Event& event) {
    if (event.type == SDL_TEXTINPUT && activeModeIndex_ == 3 && hardInputFocused_) {
        inputBuffer_ += event.text.text;
        imeComposition_.clear();
        imeCursor_ = 0;
        refreshHardCandidates();
        return;
    }

    if (event.type == SDL_KEYDOWN) {
        const SDL_Keycode key = event.key.keysym.sym;
        if (key == SDLK_ESCAPE) {
            if (isBattleMode(controller_.getSession().mode)) {
                controller_.leaveBattle();
            }
            openMainMenu();
            return;
        }
        if (activeModeIndex_ == 3 && hardInputFocused_) {
            if (key == SDLK_BACKSPACE) {
                if (!inputBuffer_.empty()) {
                    inputBuffer_.pop_back();
                }
                refreshHardCandidates();
                return;
            }
            if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                submitHardInputBuffer();
                return;
            }
        }
    }

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        const int mx = event.button.x;
        const int my = event.button.y;

        if (pointInRect(mx, my, gameBackButton_.rect)) {
            if (isBattleMode(controller_.getSession().mode)) {
                controller_.leaveBattle();
            }
            openMainMenu();
            return;
        }
        if (pointInRect(mx, my, gameRevealButton_.rect)) {
            const PathResult answer = controller_.revealAnswer();

            std::vector<std::string> words;
            for (int idiomId : answer.path) {
                words.push_back(controller_.wordOf(idiomId));
            }
            revealedAnswerText_ = joinWords(words, " -> ");
            statusMessage_ = controller_.getLastMessage();
            return;
        }
        if (activeModeIndex_ != 1 && pointInRect(mx, my, gameHintButton_.rect)) {
            const auto hint = controller_.requestHint();
            hintMessage_ = hint.has_value() ? ("提示：下一步可考虑 “" + *hint + "”。") : controller_.getLastMessage();
            statusMessage_ = controller_.getLastMessage();
            return;
        }
        if (activeModeIndex_ != 1 && pointInRect(mx, my, gameUndoButton_.rect)) {
            controller_.rollbackOneStep();
            statusMessage_ = controller_.getLastMessage();
            refreshGameCaches(true);
            return;
        }

        if (activeModeIndex_ == 1) {
            if (pointInRect(mx, my, easySubmitButton_.rect)) {
                const bool ok = controller_.submitEasyOrder(easySelectedOrder_);
                statusMessage_ = ok ? "排序已提交。" : controller_.getLastMessage();
                return;
            }
            if (pointInRect(mx, my, easyResetButton_.rect)) {
                resetEasyTilesFromPool();
                statusMessage_ = "已恢复到初始候选池。";
                return;
            }
            beginEasyDrag(mx, my);
            return;
        }

        if (activeModeIndex_ == 2) {
            for (const auto& button : mediumButtons_) {
                if (pointInRect(mx, my, button.rect)) {
                    const bool ok = controller_.submitMediumChoiceById(button.actionId);
                    statusMessage_ = ok ? "已提交该步选择。" : controller_.getLastMessage();
                    refreshGameCaches(true);
                    return;
                }
            }
            return;
        }

        if (activeModeIndex_ == 3) {
            const SDL_Rect inputBox = makeRect(320 + 24, 136 + 300, 560 - 48, 54);
            const SDL_Rect composeRect = makeRect(inputBox.x + 10, inputBox.y + 28, inputBox.w - 20, 22);

            if (pointInRect(mx, my, inputBox)) {
                hardInputFocused_ = true;
                SDL_StartTextInput();
                SDL_SetTextInputRect(&composeRect);
                return;
            }
            if (pointInRect(mx, my, hardSubmitButton_.rect)) {
                hardInputFocused_ = true;
                submitHardInputBuffer();
                return;
            }
            for (const auto& button : hardCandidateButtons_) {
                if (pointInRect(mx, my, button.rect)) {
                    hardInputFocused_ = true;
                    inputBuffer_ = button.label;
                    imeComposition_.clear();
                    imeCursor_ = 0;
                    submitHardInputBuffer();
                    return;
                }
            }
            hardInputFocused_ = false;
            return;
        }
    }

    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        if (activeModeIndex_ == 1 && draggingEasyTile_) {
            endEasyDrag();
            return;
        }
    }

    if (event.type == SDL_MOUSEMOTION && activeModeIndex_ == 1 && draggingEasyTile_) {
        updateEasyDrag(event.motion.x, event.motion.y);
        return;
    }
}

void IdiomChainScene::handleResultEvent(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
        if (isBattleMode(controller_.getSession().mode)) {
            controller_.leaveBattle();
        }
        openMainMenu();
        return;
    }
    if (event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT) {
        return;
    }

    const int mx = event.button.x;
    const int my = event.button.y;
    if (pointInRect(mx, my, resultAgainButton_.rect)) {
        if (isBattleMode(controller_.getSession().mode)) {
            controller_.leaveBattle();
            openBattleSetup();
        } else {
            openDifficultyMenu();
        }
    } else if (pointInRect(mx, my, resultRankButton_.rect)) {
        openLeaderboard();
    } else if (pointInRect(mx, my, resultMenuButton_.rect)) {
        if (isBattleMode(controller_.getSession().mode)) {
            controller_.leaveBattle();
        }
        openMainMenu();
    }
}

void IdiomChainScene::handleLeaderboardEvent(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
        openMainMenu();
        return;
    }
    if (event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT) {
        return;
    }
    if (pointInRect(event.button.x, event.button.y, leaderboardBackButton_.rect)) {
        openMainMenu();
    }
}

void IdiomChainScene::handleMouseMotion(const SDL_MouseMotionEvent& event) {
    mouseX_ = event.x;
    mouseY_ = event.y;
}

void IdiomChainScene::handleTextEditing(const SDL_TextEditingEvent& event) {
    if (viewMode_ != ViewMode::Game || activeModeIndex_ != 3 || !hardInputFocused_) {
        return;
    }
    imeComposition_ = event.text;
    imeCursor_ = event.start;
}

void IdiomChainScene::handleMouseWheel(const SDL_MouseWheelEvent& event) {
    int mx = 0;
    int my = 0;
    SDL_GetMouseState(&mx, &my);

    const int delta = -event.y * 24;
    auto applyScroll = [&](const SDL_Rect& rect, int& offset, int contentHeight) -> bool {
        if (!pointInRect(mx, my, rect)) {
            return false;
        }
        clampScrollOffset(offset, contentHeight, rect.h);
        offset += delta;
        clampScrollOffset(offset, contentHeight, rect.h);
        return true;
    };

    switch (viewMode_) {
    case ViewMode::MainMenu:
        applyScroll(makeRect(120 + 18, 250 + 58, 250 - 36, 280 - 74), mainMenuLeftScroll_, mainMenuLeftContentHeight_) ||
        applyScroll(makeRect(830 + 18, 250 + 58, 250 - 36, 280 - 74), mainMenuRightScroll_, mainMenuRightContentHeight_);
        break;
    case ViewMode::DifficultyMenu:
        applyScroll(makeRect(200 + 28, 220 + 62, 800 - 56, 240 - 86), difficultyInfoScroll_, difficultyInfoContentHeight_);
        break;
    case ViewMode::Game:
        applyScroll(makeRect(320 + 24 + 12, 136 + 122 + 34, (560 - 48) - 24, 100 - 40),
                    gamePathScroll_,
                    gamePathContentHeight_) ||
        applyScroll(makeRect(900 + 16, 136 + 52, 240 - 32, 620 - 68),
                    gameExplanationScroll_,
                    gameExplanationContentHeight_);
        break;
    case ViewMode::Result:
        applyScroll(makeRect(330 + 30, 180 + 88, 390 - 60, 128 - 46), resultMyRouteScroll_, resultMyRouteContentHeight_) ||
        applyScroll(makeRect(330 + 30, 180 + 232, 390 - 60, 170 - 46), resultBestRouteScroll_, resultBestRouteContentHeight_) ||
        applyScroll(makeRect(740 + 16, 180 + 54, 400 - 32, 430 - 70), resultExplanationScroll_, resultExplanationContentHeight_);
        break;
    case ViewMode::Leaderboard:
        applyScroll(makeRect(50 + 20, 180 + 112, 760 - 40, 500 - 132), leaderboardRankScroll_, leaderboardRankContentHeight_) ||
        applyScroll(makeRect(830 + 16, 180 + 56, 320 - 32, 500 - 72), leaderboardHistoryScroll_, leaderboardHistoryContentHeight_);
        break;
    case ViewMode::BattleSetup:
    case ViewMode::BattleLobby:
        break;
    }
}

void IdiomChainScene::openMainMenu() {
    SDL_StopTextInput();
    viewMode_ = ViewMode::MainMenu;
    inputBuffer_.clear();
    imeComposition_.clear();
    imeCursor_ = 0;
    hardInputFocused_ = false;
    resetScrollOffsets();
    hardCandidates_.clear();
    mediumButtons_.clear();
    hardCandidateButtons_.clear();
    refreshMainMenuButtons();
}

void IdiomChainScene::openDifficultyMenu() {
    SDL_StopTextInput();
    viewMode_ = ViewMode::DifficultyMenu;
    imeComposition_.clear();
    imeCursor_ = 0;
    hardInputFocused_ = false;
    resetScrollOffsets();
    refreshDifficultyButtons();
}

void IdiomChainScene::openBattleSetup() {
    SDL_StartTextInput();
    viewMode_ = ViewMode::BattleSetup;
    activeTextField_ = TextField::PlayerName;
    statusMessage_.clear();
    refreshBattleSetupButtons();
}

void IdiomChainScene::openBattleLobby() {
    SDL_StopTextInput();
    viewMode_ = ViewMode::BattleLobby;
    battleLobbyBackButton_ = ButtonSpec{makeRect(kWindowWidth / 2 - 120, 602, 240, 64), "取消并返回", 1, true};
}

void IdiomChainScene::openLeaderboard() {
    SDL_StopTextInput();
    viewMode_ = ViewMode::Leaderboard;
    imeComposition_.clear();
    imeCursor_ = 0;
    hardInputFocused_ = false;
    resetScrollOffsets();
    refreshLeaderboardCaches();
}

void IdiomChainScene::resetScrollOffsets() {
    mainMenuLeftScroll_ = 0;
    mainMenuRightScroll_ = 0;
    difficultyInfoScroll_ = 0;
    gamePathScroll_ = 0;
    gameExplanationScroll_ = 0;
    resultMyRouteScroll_ = 0;
    resultBestRouteScroll_ = 0;
    resultExplanationScroll_ = 0;
    leaderboardHistoryScroll_ = 0;
    leaderboardRankScroll_ = 0;
}

void IdiomChainScene::clampScrollOffset(int& offset, int contentHeight, int viewHeight) const {
    const int maxOffset = std::max(0, contentHeight - viewHeight);
    offset = clampInt(offset, 0, maxOffset);
}

void IdiomChainScene::startGame(int modeIndex) {
    activeModeIndex_ = modeIndex;
    inputBuffer_.clear();
    imeComposition_.clear();
    imeCursor_ = 0;
    hardInputFocused_ = (modeIndex == 3);
    resetScrollOffsets();
    statusMessage_.clear();
    hintMessage_.clear();
    revealedAnswerText_.clear();
    lastMediumPathSize_ = -1;
    lastHardPathSize_ = -1;
    lastHardQuery_.clear();

    if (modeIndex == 1) {
        controller_.startSingleGame(GameMode::SingleEasy, playerName_);
        SDL_StopTextInput();
    } else if (modeIndex == 2) {
        controller_.startSingleGame(GameMode::SingleMedium, playerName_);
        SDL_StopTextInput();
    } else {
        controller_.startSingleGame(GameMode::SingleHard, playerName_);
        SDL_StartTextInput();

        SDL_Rect inputRect   = makeRect(320 + 24, 136 + 300, 560 - 48, 54);
        SDL_Rect composeRect = makeRect(inputRect.x + 10, inputRect.y + 28, inputRect.w - 20, 22);
        SDL_SetTextInputRect(&composeRect);
    }

    resetEasyTilesFromPool();
    refreshGameCaches(true);
    viewMode_ = ViewMode::Game;
}

void IdiomChainScene::refreshMainMenuButtons() {
    mainMenuButtons_.clear();
    const int w = 240;
    const int h = 64;
    const int x = kWindowWidth / 2 - w / 2;
    int y = 250;
    mainMenuButtons_.push_back(ButtonSpec{makeRect(x, y, w, h), "单人游戏", 1, true});
    y += 88;
    mainMenuButtons_.push_back(ButtonSpec{makeRect(x, y, w, h), "多人游戏", 2, true});
    y += 88;
    mainMenuButtons_.push_back(ButtonSpec{makeRect(x, y, w, h), "排行榜", 3, true});
    y += 88;
    mainMenuButtons_.push_back(ButtonSpec{makeRect(x, y, w, h), "返回", 4, true});
}

void IdiomChainScene::refreshDifficultyButtons() {
    difficultyButtons_.clear();
    const int w = 240;
    const int h = 50;
    const int x = kWindowWidth / 2 - w / 2;
    int y = 500;
    difficultyButtons_.push_back(ButtonSpec{makeRect(x, y, w, h), "简单模式", 1, true});
    y += 62;
    difficultyButtons_.push_back(ButtonSpec{makeRect(x, y, w, h), "中等模式", 2, true});
    y += 62;
    difficultyButtons_.push_back(ButtonSpec{makeRect(x, y, w, h), "困难模式", 3, true});
    y += 62;
    difficultyButtons_.push_back(ButtonSpec{makeRect(x, y, w, 50), "返回上一级", 4, true});
}

void IdiomChainScene::refreshBattleSetupButtons() {
    const int centerX = kWindowWidth / 2 - 120;
    battleHostButton_ = ButtonSpec{makeRect(centerX, 250, 240, 64), "创建房间", 1, true};
    battleJoinButton_ = ButtonSpec{makeRect(centerX, 338, 240, 64), "加入房间", 2, true};

    battleModeEasyButton_ = ButtonSpec{makeRect(120 + 20, 250 + 64, 210, 50), "对战简单模式", 3, true};
    battleModeMediumButton_ = ButtonSpec{makeRect(120 + 20, 250 + 126, 210, 50), "对战中等模式", 4, true};
    battleModeHardButton_ = ButtonSpec{makeRect(120 + 20, 250 + 188, 210, 50), "对战困难模式", 5, true};

    battlePlayerField_ = makeRect(centerX, 438, 240, 48);
    battleIpField_ = makeRect(centerX, 500, 240, 48);
    battlePortField_ = makeRect(centerX, 562, 240, 48);

    battleConfirmButton_ = ButtonSpec{makeRect(centerX, 624, 240, 50),
                                      battleHostSelected_ ? "创建并等待" : "连接房主",
                                      6, true};
    battleSetupBackButton_ = ButtonSpec{makeRect(centerX, 686, 240, 50), "返回主菜单", 7, true};
}

void IdiomChainScene::refreshGameCaches(bool force) {
    if (activeModeIndex_ == 2) {
        refreshMediumOptions(force);
    }
    if (activeModeIndex_ == 3) {
        refreshHardCandidates(force);
    }
}

void IdiomChainScene::refreshMediumOptions(bool force) {
    const GameSession& session = controller_.getSession();
    const int pathSize = static_cast<int>(session.playerPath.size());
    if (!force && pathSize == lastMediumPathSize_) {
        return;
    }
    lastMediumPathSize_ = pathSize;
    mediumOptionIds_ = controller_.getMediumOptions();
    mediumOptionDistances_ = controller_.getMediumOptionDistances();

    mediumButtons_.clear();
    const int startX = 360;
    const int startY = 405;
    const int w = 220;
    const int h = 60;
    for (std::size_t i = 0; i < mediumOptionIds_.size() && i < 4; ++i) {
        const int row = static_cast<int>(i) / 2;
        const int col = static_cast<int>(i) % 2;
        const std::string word = controller_.wordOf(mediumOptionIds_[i]);
        mediumButtons_.push_back(ButtonSpec{
            makeRect(startX + col * (w + 20), startY + row * (h + 36), w, h),
            word,
            mediumOptionIds_[i],
            true
        });
    }
}

void IdiomChainScene::refreshHardCandidates(bool force) {
    const GameSession& session = controller_.getSession();
    const int pathSize = static_cast<int>(session.playerPath.size());
    if (!force && pathSize == lastHardPathSize_ && inputBuffer_ == lastHardQuery_) {
        return;
    }
    lastHardPathSize_ = pathSize;
    lastHardQuery_ = inputBuffer_;
    hardCandidates_ = controller_.getHardCandidateWords(inputBuffer_);

    hardCandidateButtons_.clear();
    const int startX = 350;
    const int startY = 560;
    const int w = 112;
    const int h = 42;
    for (std::size_t i = 0; i < hardCandidates_.size() && i < 8; ++i) {
        const int row = static_cast<int>(i) / 4;
        const int col = static_cast<int>(i) % 4;
        hardCandidateButtons_.push_back(ButtonSpec{
            makeRect(startX + col * (w + 12), startY + row * (h + 10), w, h),
            hardCandidates_[i],
            static_cast<int>(i),
            true
        });
    }
}

void IdiomChainScene::refreshLeaderboardCaches() {
    leaderboardCache_.clear();
    auto ranking = controller_.buildLeaderboard();
    while (!ranking.empty()) {
        leaderboardCache_.push_back(ranking.top());
        ranking.pop();
    }

    historyCache_ = controller_.loadAllRecords();
    std::reverse(historyCache_.begin(), historyCache_.end());

    leaderboardBackButton_ = ButtonSpec{makeRect(kWindowWidth / 2 - 80, 710, 160, 52), "返回主菜单", 1, true};
}

void IdiomChainScene::resetEasyTilesFromPool() {
    easySelectedOrder_.clear();
    easyDiscardOrder_.clear();
    const auto pool = controller_.getEasyPool();
    for (std::size_t i = 0; i < pool.size(); ++i) {
        easyDiscardOrder_.push_back(static_cast<int>(i));
    }
    relayoutEasyTiles();
}

void IdiomChainScene::relayoutEasyTiles() {
    easyTiles_.clear();
    easySelectedZoneRect_ = makeRect(344, 346, 512, 168);
    easyDiscardZoneRect_ = makeRect(344, 522, 512, 168);

    auto appendTiles = [&](const std::vector<int>& order, bool inSelected, const SDL_Rect& zone) {
        for (std::size_t i = 0; i < order.size(); ++i) {
            const int row = static_cast<int>(i) / 4;
            const int col = static_cast<int>(i) % 4;
            EasyTile tile;
            tile.poolIndex = order[i];
            tile.inSelectedZone = inSelected;
            tile.rect = makeRect(zone.x + 18 + col * (kTileWidth + kTileGap),
                                 zone.y + 36 + row * (kTileHeight + kTileGap),
                                 kTileWidth,
                                 kTileHeight);
            easyTiles_.push_back(tile);
        }
    };

    appendTiles(easySelectedOrder_, true, easySelectedZoneRect_);
    appendTiles(easyDiscardOrder_, false, easyDiscardZoneRect_);
}

void IdiomChainScene::drawText(TTF_Font* font,
                               const std::string& text,
                               int x,
                               int y,
                               SDL_Color color,
                               bool centered) const {
    if (font == nullptr || text.empty()) {
        return;
    }
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (surface == nullptr) {
        return;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    if (texture == nullptr) {
        SDL_FreeSurface(surface);
        return;
    }
    SDL_Rect dst{x, y, surface->w, surface->h};
    if (centered) {
        dst.x -= dst.w / 2;
        dst.y -= dst.h / 2;
    }
    SDL_FreeSurface(surface);
    SDL_RenderCopy(renderer_, texture, nullptr, &dst);
    SDL_DestroyTexture(texture);
}

void IdiomChainScene::drawWrappedText(TTF_Font* font,
                                      const std::string& text,
                                      const SDL_Rect& rect,
                                      SDL_Color color) const {
    if (font == nullptr || text.empty()) {
        return;
    }
    SDL_Surface* surface = TTF_RenderUTF8_Blended_Wrapped(font, text.c_str(), color, rect.w);
    if (surface == nullptr) {
        return;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    if (texture == nullptr) {
        SDL_FreeSurface(surface);
        return;
    }
    SDL_Rect dst{rect.x, rect.y, surface->w, surface->h};
    SDL_FreeSurface(surface);
    SDL_RenderSetClipRect(renderer_, &rect);
    SDL_RenderCopy(renderer_, texture, nullptr, &dst);
    SDL_RenderSetClipRect(renderer_, nullptr);
    SDL_DestroyTexture(texture);
}

int IdiomChainScene::wrappedTextHeight(TTF_Font* font,
                                       const std::string& text,
                                       int wrapWidth) const {
    if (font == nullptr || text.empty() || wrapWidth <= 0) {
        return 0;
    }
    SDL_Surface* surface = TTF_RenderUTF8_Blended_Wrapped(font, text.c_str(), rgb(0, 0, 0), wrapWidth);
    if (surface == nullptr) {
        return 0;
    }
    const int h = surface->h;
    SDL_FreeSurface(surface);
    return h;
}

int IdiomChainScene::drawWrappedTextScrollable(TTF_Font* font,
                                               const std::string& text,
                                               const SDL_Rect& rect,
                                               SDL_Color color,
                                               int& scrollOffset) const {
    if (font == nullptr || rect.w <= 0 || rect.h <= 0) {
        return 0;
    }
    const std::string safeText = text.empty() ? " " : text;
    SDL_Surface* surface = TTF_RenderUTF8_Blended_Wrapped(font, safeText.c_str(), color, rect.w - 10);
    if (surface == nullptr) {
        return 0;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    const int contentHeight = surface->h;
    if (texture == nullptr) {
        SDL_FreeSurface(surface);
        return contentHeight;
    }

    const int maxOffset = std::max(0, contentHeight - rect.h);
    scrollOffset = clampInt(scrollOffset, 0, maxOffset);

    SDL_Rect dst{rect.x, rect.y - scrollOffset, surface->w, surface->h};
    SDL_FreeSurface(surface);

    SDL_RenderSetClipRect(renderer_, &rect);
    SDL_RenderCopy(renderer_, texture, nullptr, &dst);
    SDL_RenderSetClipRect(renderer_, nullptr);
    SDL_DestroyTexture(texture);

    if (contentHeight > rect.h) {
        SDL_Rect track{rect.x + rect.w - 6, rect.y, 4, rect.h};
        fillRect(renderer_, track, rgb(232, 232, 232));
        const int thumbH = std::max(24, rect.h * rect.h / std::max(contentHeight, rect.h));
        const int thumbY = rect.y + (rect.h - thumbH) * scrollOffset / std::max(1, maxOffset);
        SDL_Rect thumb{track.x, thumbY, track.w, thumbH};
        fillRect(renderer_, thumb, rgb(120, 120, 120));
    }
    return contentHeight;
}

void IdiomChainScene::drawFittedTextInRect(TTF_Font* preferredFont,
                                           const std::string& text,
                                           const SDL_Rect& rect,
                                           SDL_Color color,
                                           bool centered,
                                           bool allowWrap) const {
    if (text.empty() || rect.w <= 0 || rect.h <= 0) {
        return;
    }
    std::vector<TTF_Font*> fonts{preferredFont, bodyFont_, smallFont_};
    fonts.erase(std::remove(fonts.begin(), fonts.end(), nullptr), fonts.end());
    fonts.erase(std::unique(fonts.begin(), fonts.end()), fonts.end());

    for (TTF_Font* font : fonts) {
        int w = 0;
        int h = 0;
        if (TTF_SizeUTF8(font, text.c_str(), &w, &h) == 0 && w <= rect.w - 8 && h <= rect.h - 4) {
            const int drawX = centered ? rect.x + rect.w / 2 : rect.x + 4;
            const int drawY = centered ? rect.y + rect.h / 2 : rect.y + (rect.h - h) / 2;
            drawText(font, text, drawX, drawY, color, centered);
            return;
        }
    }

    TTF_Font* wrapFont = smallFont_ != nullptr ? smallFont_ : preferredFont;
    if (wrapFont == nullptr) {
        return;
    }
    if (!allowWrap) {
        drawText(wrapFont, text, centered ? rect.x + rect.w / 2 : rect.x + 4,
                 centered ? rect.y + rect.h / 2 : rect.y + 2, color, centered);
        return;
    }

    SDL_Surface* surface = TTF_RenderUTF8_Blended_Wrapped(wrapFont, text.c_str(), color, rect.w - 8);
    if (surface == nullptr) {
        return;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    if (texture == nullptr) {
        SDL_FreeSurface(surface);
        return;
    }
    SDL_Rect dst{rect.x + 4, rect.y + 2, surface->w, surface->h};
    if (centered) {
        dst.x = rect.x + (rect.w - surface->w) / 2;
        dst.y = rect.y + (rect.h - std::min(surface->h, rect.h - 4)) / 2;
    }
    SDL_FreeSurface(surface);
    SDL_RenderSetClipRect(renderer_, &rect);
    SDL_RenderCopy(renderer_, texture, nullptr, &dst);
    SDL_RenderSetClipRect(renderer_, nullptr);
    SDL_DestroyTexture(texture);
}

void IdiomChainScene::drawPanel(const SDL_Rect& rect, const std::string& title) const {
    fillRect(renderer_, rect, rgb(250, 248, 243));
    strokeRect(renderer_, rect, rgb(28, 28, 28), 2);
    drawText(subtitleFont_, title, rect.x + 18, rect.y + 14, rgb(20, 20, 20), false);
}

void IdiomChainScene::drawButton(const ButtonSpec& button, bool primary) const {
    const bool hovered = button.enabled && pointInRect(mouseX_, mouseY_, button.rect);
    SDL_Color fill = primary ? rgb(220, 233, 224) : rgb(246, 244, 239);
    SDL_Color text = primary ? rgb(40, 90, 65) : rgb(20, 20, 20);
    if (!button.enabled) {
        fill = rgb(225, 225, 225);
        text = rgb(120, 120, 120);
    } else if (hovered) {
        fill = primary ? rgb(198, 223, 208) : rgb(232, 228, 220);
    }
    fillRect(renderer_, button.rect, fill);
    strokeRect(renderer_, button.rect, hovered ? rgb(58, 112, 82) : rgb(30, 30, 30), hovered ? 3 : 2);
    drawFittedTextInRect(subtitleFont_, button.label,
                         makeRect(button.rect.x + 6, button.rect.y + 4, button.rect.w - 12, button.rect.h - 8),
                         text, true, true);
}

void IdiomChainScene::drawBadge(const SDL_Rect& rect, const std::string& text) const {
    fillRect(renderer_, rect, rgb(224, 236, 228));
    strokeRect(renderer_, rect, rgb(70, 96, 76), 1);
    drawText(smallFont_, text, rect.x + rect.w / 2, rect.y + rect.h / 2, rgb(40, 80, 56), true);
}

void IdiomChainScene::drawCard(const SDL_Rect& rect,
                               const std::string& title,
                               const std::string& content,
                               bool emphasized) const {
    fillRect(renderer_, rect, emphasized ? rgb(244, 233, 205) : rgb(255, 255, 255));
    strokeRect(renderer_, rect, rgb(36, 36, 36), 1);
    if (rect.h <= 44) {
        drawFittedTextInRect(smallFont_, title + "：" + content,
                             makeRect(rect.x + 8, rect.y + 4, rect.w - 16, rect.h - 8),
                             rgb(22, 22, 22), false, true);
        return;
    }
    SDL_Rect titleRect = makeRect(rect.x + 10, rect.y + 8, rect.w - 20, 22);
    SDL_Rect contentRect = makeRect(rect.x + 12, rect.y + 34, rect.w - 24, std::max(8, rect.h - 40));
    drawFittedTextInRect(bodyFont_, title, titleRect, rgb(22, 22, 22), false, true);
    drawWrappedText(smallFont_, content, contentRect, rgb(92, 92, 92));
}

void IdiomChainScene::drawSimpleTableRow(const SDL_Rect& rect,
                                         const std::vector<std::string>& columns,
                                         const std::vector<int>& widths,
                                         bool header) const {
    fillRect(renderer_, rect, header ? rgb(236, 236, 232) : rgb(255, 255, 255));
    strokeRect(renderer_, rect, rgb(36, 36, 36), 1);
    int x = rect.x;
    for (std::size_t i = 0; i < widths.size(); ++i) {
        if (i > 0) {
            drawLine(renderer_, x, rect.y, x, rect.y + rect.h, rgb(36, 36, 36));
        }
        if (i < columns.size()) {
            SDL_Rect cell = makeRect(x + 3, rect.y + 2, widths[i] - 6, rect.h - 4);
            drawFittedTextInRect(header ? bodyFont_ : smallFont_, columns[i], cell, rgb(20, 20, 20), true, true);
        }
        x += widths[i];
    }
}

void IdiomChainScene::drawInputField(const SDL_Rect& rect,
                                     const std::string& label,
                                     const std::string& value,
                                     bool active) const {
    drawText(bodyFont_, label, rect.x, rect.y - 24, rgb(70, 70, 70), false);
    fillRect(renderer_, rect, rgb(255, 255, 255));
    strokeRect(renderer_, rect, active ? rgb(58,112,82) : rgb(36,36,36), active ? 2 : 1);
    drawFittedTextInRect(bodyFont_, value.empty() ? " " : value,
                         makeRect(rect.x + 8, rect.y + 4, rect.w - 16, rect.h - 8),
                         value.empty() ? rgb(130,130,130) : rgb(20,20,20), false, true);
}

void IdiomChainScene::render() {
    SDL_SetRenderDrawColor(renderer_, 240, 237, 231, 255);
    SDL_RenderClear(renderer_);

    SDL_Rect frame = makeRect(kOuterMargin, kOuterMargin, kWindowWidth - kOuterMargin * 2, kWindowHeight - kOuterMargin * 2);
    strokeRect(renderer_, frame, rgb(18, 18, 18), 3);

    switch (viewMode_) {
    case ViewMode::MainMenu:
        renderMainMenu();
        break;
    case ViewMode::DifficultyMenu:
        renderDifficultyMenu();
        break;
    case ViewMode::BattleSetup:
        renderBattleSetup();
        break;
    case ViewMode::BattleLobby:
        renderBattleLobby();
        break;
    case ViewMode::Game:
        renderGame();
        break;
    case ViewMode::Result:
        renderResult();
        break;
    case ViewMode::Leaderboard:
        renderLeaderboard();
        break;
    }

    SDL_RenderPresent(renderer_);
}

void IdiomChainScene::renderMainMenu() {
    drawText(titleFont_, "成语接龙", kWindowWidth / 2, 130, rgb(16, 16, 16), true);
    drawText(subtitleFont_, "最短路径模块", kWindowWidth / 2, 178, rgb(88, 88, 88), true);

    for (std::size_t i = 0; i < mainMenuButtons_.size(); ++i) {
        drawButton(mainMenuButtons_[i], i == 0);
    }
}

void IdiomChainScene::renderDifficultyMenu() {
    drawText(titleFont_, "成语接龙", kWindowWidth / 2, 130, rgb(16, 16, 16), true);
    drawText(subtitleFont_, "单人游戏 · 选择难度", kWindowWidth / 2, 178, rgb(88, 88, 88), true);

    SDL_Rect info = makeRect(200, 220, 800, 240);
    drawPanel(info, "模式说明");
    difficultyInfoContentHeight_ = drawWrappedTextScrollable(
        bodyFont_,
        R"(简单模式：给出最优路径和少量干扰成语，玩家通过拖拽完成排序。

中等模式：每一步显示 4 个可达候选，均能到达终点，但步数优劣不同。

困难模式：玩家自由输入成语，系统按熟悉度与最短路接近程度提供候选和提示。)",
        makeRect(info.x + 28, info.y + 62, info.w - 56, info.h - 86),
        rgb(72, 72, 72),
        difficultyInfoScroll_);

    for (std::size_t i = 0; i < difficultyButtons_.size(); ++i) {
        drawButton(difficultyButtons_[i], i == 0);
    }
}

void IdiomChainScene::renderBattleSetup() {
    drawText(titleFont_, "成语接龙", kWindowWidth / 2, 130, rgb(16, 16, 16), true);
    drawText(subtitleFont_, "多人游戏 · 创建/加入房间", kWindowWidth / 2, 178, rgb(88, 88, 88), true);

    SDL_Rect leftInfo = makeRect(120, 250, 250, 280);
    SDL_Rect rightInfo = makeRect(830, 250, 250, 280);
    drawPanel(leftInfo, "对战模式");
    drawPanel(rightInfo, "连接说明");

    drawButton(battleModeEasyButton_, battleModeIndex_ == 1);
    drawButton(battleModeMediumButton_, battleModeIndex_ == 2);
    drawButton(battleModeHardButton_, battleModeIndex_ == 3);

    mainMenuRightContentHeight_ = drawWrappedTextScrollable(
        bodyFont_,
        battleHostSelected_
            ? "创建房间后停留在等待页。\n\n客户端加入成功、房主同步题目后，会自动进入对战。"
            : "加入房间时请填写房主 IP 和端口。\n\n连接成功后等待房主同步题目。",
        makeRect(rightInfo.x + 18, rightInfo.y + 58, rightInfo.w - 36, rightInfo.h - 74),
        rgb(78, 78, 78),
        mainMenuRightScroll_);

    drawButton(battleHostButton_, battleHostSelected_);
    drawButton(battleJoinButton_, !battleHostSelected_);
    drawInputField(battlePlayerField_, "玩家名称", battlePlayerInput_, activeTextField_ == TextField::PlayerName);
    drawInputField(battleIpField_, "房主 IP", battleHostIpInput_, activeTextField_ == TextField::HostIp);
    drawInputField(battlePortField_, "端口", battlePortInput_, activeTextField_ == TextField::Port);
    drawButton(battleConfirmButton_, true);
    drawButton(battleSetupBackButton_);

    if (!statusMessage_.empty()) {
        drawWrappedText(bodyFont_,
                        statusMessage_,
                        makeRect(830 + 18, 250 + 58, 250 - 36, 280 - 74),
                        rgb(140, 60, 60));
    }
}

void IdiomChainScene::renderBattleLobby() {
    const GameSession& session = controller_.getSession();
    drawText(titleFont_, "成语接龙", kWindowWidth / 2, 130, rgb(16, 16, 16), true);
    drawText(subtitleFont_, "多人游戏 · 等待房间开始", kWindowWidth / 2, 178, rgb(88, 88, 88), true);

    SDL_Rect leftInfo = makeRect(120, 250, 250, 280);
    SDL_Rect rightInfo = makeRect(830, 250, 250, 280);
    drawPanel(leftInfo, "本方状态");
    drawPanel(rightInfo, "连接状态");

    mainMenuLeftContentHeight_ = drawWrappedTextScrollable(
        bodyFont_,
        std::string("玩家：") + session.playerName + "\n"
        + "身份：" + (session.battleIsHost ? std::string("房主") : std::string("客户端")) + "\n"
        + "模式：" + modeText(battleModeIndex_),
        makeRect(leftInfo.x + 18, leftInfo.y + 58, leftInfo.w - 36, leftInfo.h - 74),
        rgb(78, 78, 78),
        mainMenuLeftScroll_);

    std::ostringstream oss;
    oss << "已连接：" << (controller_.isBattleConnected() ? "是" : "否") << "\n";
    oss << "对手：" << (session.remotePlayerName.empty() ? "等待中" : session.remotePlayerName) << "\n";
    oss << "状态：" << controller_.getLastMessage() << "\n";
    oss << "当双方握手完成且题目同步后，将自动进入对战。";
    mainMenuRightContentHeight_ = drawWrappedTextScrollable(
        bodyFont_,
        oss.str(),
        makeRect(rightInfo.x + 18, rightInfo.y + 58, rightInfo.w - 36, rightInfo.h - 74),
        rgb(78, 78, 78),
        mainMenuRightScroll_);

    drawButton(battleLobbyBackButton_);
}

void IdiomChainScene::renderGame() {
    const GameSession& session = controller_.getSession();

    drawText(titleFont_, modeText(activeModeIndex_), kWindowWidth / 2, 82, rgb(20, 20, 20), true);
    drawText(smallFont_, isBattleMode(session.mode) ? "成语接龙最短路径 · 双人对战" : "成语接龙最短路径", kWindowWidth / 2, 116, rgb(88, 88, 88), true);

    SDL_Rect leftPanel = makeRect(60, 136, 240, 620);
    SDL_Rect centerPanel = makeRect(320, 136, 560, 620);
    SDL_Rect rightPanel = makeRect(900, 136, 240, 620);
    drawPanel(leftPanel, "局内状态");
    drawPanel(centerPanel, "答题区");
    drawPanel(rightPanel, isBattleMode(session.mode) ? "对手状态 / 释义" : "释义 / 提示");

    SDL_Rect timeBox = makeRect(leftPanel.x + 18, leftPanel.y + 48, leftPanel.w - 36, 52);
    SDL_Rect stepBox = makeRect(leftPanel.x + 18, leftPanel.y + 116, 96, 84);
    SDL_Rect bestBox = makeRect(leftPanel.x + 126, leftPanel.y + 116, 96, 84);
    fillRect(renderer_, timeBox, rgb(255, 255, 255));
    fillRect(renderer_, stepBox, rgb(255, 255, 255));
    fillRect(renderer_, bestBox, rgb(255, 255, 255));
    strokeRect(renderer_, timeBox, rgb(36, 36, 36), 1);
    strokeRect(renderer_, stepBox, rgb(36, 36, 36), 1);
    strokeRect(renderer_, bestBox, rgb(36, 36, 36), 1);

    drawFittedTextInRect(bodyFont_, "当前用时：" + formatSeconds(session.elapsedSeconds),
                         makeRect(timeBox.x + 8, timeBox.y + 6, timeBox.w - 16, timeBox.h - 12),
                         rgb(20, 20, 20), false, true);
    drawText(smallFont_, "步数", stepBox.x + stepBox.w / 2, stepBox.y + 22, rgb(92, 92, 92), true);
    drawText(subtitleFont_, std::to_string(session.stepCount), stepBox.x + stepBox.w / 2, stepBox.y + 54, rgb(20, 20, 20), true);
    drawText(smallFont_, "最优步数", bestBox.x + bestBox.w / 2, bestBox.y + 22, rgb(92, 92, 92), true);
    drawText(subtitleFont_, std::to_string(session.bestStepCount), bestBox.x + bestBox.w / 2, bestBox.y + 54, rgb(20, 20, 20), true);

    drawCard(makeRect(leftPanel.x + 18, leftPanel.y + 214, leftPanel.w - 36, 78),
             "题目",
             "起点：" + controller_.wordOf(session.startId) + "\n终点：" + controller_.wordOf(session.targetId));

    std::string statusText = statusMessage_.empty() ? controller_.getLastMessage() : statusMessage_;
    drawCard(makeRect(leftPanel.x + 18, leftPanel.y + 304, leftPanel.w - 36, 80),
             "当前状态",
             statusText.empty() ? "等待操作……" : statusText,
             !statusText.empty());

    std::ostringstream helper;
    if (activeModeIndex_ == 3) {
        helper << "剩余提示：" << std::max(0, session.maxHints - session.hintCount) << "\n";
    }
    helper << "玩家：" << playerName_;
    if (isBattleMode(session.mode)) {
        helper << "\n对手：" << (session.remotePlayerName.empty() ? "等待中" : session.remotePlayerName);
    }
    drawCard(makeRect(leftPanel.x + 18, leftPanel.y + 400, leftPanel.w - 36, 80),
             "补充信息",
             helper.str());

    gameBackButton_ = ButtonSpec{makeRect(leftPanel.x + 18, leftPanel.y + 504, 78, 46), "主菜单", 1, true};
    gameHintButton_ = ButtonSpec{makeRect(leftPanel.x + 103, leftPanel.y + 504, 56, 46), "提示", 2, activeModeIndex_ != 1};
    gameUndoButton_ = ButtonSpec{makeRect(leftPanel.x + 166, leftPanel.y + 504, 56, 46), "撤回", 3, activeModeIndex_ != 1};
    gameRevealButton_ = ButtonSpec{makeRect(leftPanel.x + 18, leftPanel.y + 560, leftPanel.w - 36, 46), "查看最优解", 4, true};
    drawButton(gameBackButton_);
    drawButton(gameHintButton_);
    drawButton(gameUndoButton_);
    drawButton(gameRevealButton_);

    SDL_Rect startBox = makeRect(centerPanel.x + 24, centerPanel.y + 48, 150, 60);
    SDL_Rect targetBox = makeRect(centerPanel.x + centerPanel.w - 174, centerPanel.y + 48, 150, 60);
    fillRect(renderer_, startBox, rgb(255, 255, 255));
    fillRect(renderer_, targetBox, rgb(255, 255, 255));
    strokeRect(renderer_, startBox, rgb(36, 36, 36), 1);
    strokeRect(renderer_, targetBox, rgb(36, 36, 36), 1);
    drawFittedTextInRect(bodyFont_, "起点：" + controller_.wordOf(session.startId),
                         makeRect(startBox.x + 8, startBox.y + 6, startBox.w - 16, startBox.h - 12),
                         rgb(20, 20, 20), false, true);
    drawFittedTextInRect(bodyFont_, "终点：" + controller_.wordOf(session.targetId),
                         makeRect(targetBox.x + 8, targetBox.y + 6, targetBox.w - 16, targetBox.h - 12),
                         rgb(20, 20, 20), false, true);
    drawLine(renderer_, startBox.x + startBox.w + 16, startBox.y + 30, targetBox.x - 16, targetBox.y + 30, rgb(36, 36, 36));
    drawLine(renderer_, targetBox.x - 26, targetBox.y + 22, targetBox.x - 16, targetBox.y + 30, rgb(36, 36, 36));
    drawLine(renderer_, targetBox.x - 26, targetBox.y + 38, targetBox.x - 16, targetBox.y + 30, rgb(36, 36, 36));

    {
        SDL_Rect pathCard = makeRect(centerPanel.x + 24, centerPanel.y + 122, centerPanel.w - 48, 100);
        fillRect(renderer_, pathCard, rgb(255, 255, 255));
        strokeRect(renderer_, pathCard, rgb(36, 36, 36), 1);
        drawFittedTextInRect(bodyFont_, "当前路径链",
                             makeRect(pathCard.x + 10, pathCard.y + 8, pathCard.w - 20, 25),
                             rgb(22, 22, 22), false, true);

        SDL_Rect pathViewport = makeRect(pathCard.x + 12, pathCard.y + 34, pathCard.w - 24, pathCard.h - 40);
        const std::string pathText = currentPathText().empty() ? "暂无路径。" : currentPathText();

        if (activeModeIndex_ == 2 || activeModeIndex_ == 3) {
            gamePathContentHeight_ = drawWrappedTextScrollable(
                smallFont_,
                pathText,
                pathViewport,
                rgb(92, 92, 92),
                gamePathScroll_);
        } else {
            drawWrappedText(
                smallFont_,
                pathText,
                pathViewport,
                rgb(92, 92, 92));
            gamePathContentHeight_ = wrappedTextHeight(smallFont_, pathText, pathViewport.w - 10);
            clampScrollOffset(gamePathScroll_, gamePathContentHeight_, pathViewport.h);
        }
    }

    if (activeModeIndex_ == 1) {
        fillRect(renderer_, easySelectedZoneRect_, rgb(236, 245, 237));
        fillRect(renderer_, easyDiscardZoneRect_, rgb(248, 240, 235));
        strokeRect(renderer_, easySelectedZoneRect_, rgb(36, 100, 54), 1);
        strokeRect(renderer_, easyDiscardZoneRect_, rgb(120, 86, 56), 1);
        drawFittedTextInRect(bodyFont_, "路径区：把正确路线拖到这里",
                             makeRect(easySelectedZoneRect_.x + 12, easySelectedZoneRect_.y + 6, easySelectedZoneRect_.w - 24, 28),
                             rgb(40, 96, 54), false, true);
        drawFittedTextInRect(bodyFont_, "候选池 / 干扰区",
                             makeRect(easyDiscardZoneRect_.x + 12, easyDiscardZoneRect_.y + 6, easyDiscardZoneRect_.w - 24, 28),
                             rgb(112, 80, 54), false, true);

        const auto poolWords = controller_.getEasyPoolWords();
        for (const auto& tile : easyTiles_) {
            if (draggingEasyTile_ && tile.poolIndex == draggedEasyPoolIndex_) {
                continue;
            }
            const std::string& word = (tile.poolIndex >= 0 && static_cast<std::size_t>(tile.poolIndex) < poolWords.size())
                                          ? poolWords[static_cast<std::size_t>(tile.poolIndex)]
                                          : std::string("?");
            drawCard(tile.rect, word, tile.inSelectedZone ? "已放入路径区" : "候选成语", false);
        }
        if (draggingEasyTile_ && draggedEasyPoolIndex_ >= 0 && static_cast<std::size_t>(draggedEasyPoolIndex_) < poolWords.size()) {
            drawCard(makeRect(dragMouseX_ - kTileWidth / 2,
                              dragMouseY_ - kTileHeight / 2,
                              kTileWidth,
                              kTileHeight),
                     poolWords[static_cast<std::size_t>(draggedEasyPoolIndex_)],
                     "拖动中",
                     true);
        }

        easySubmitButton_ = ButtonSpec{makeRect(centerPanel.x + 86, centerPanel.y + 570, 140, 46), "提交排序", 1, true};
        easyResetButton_ = ButtonSpec{makeRect(centerPanel.x + 246, centerPanel.y + 570, 140, 46), "全部归位", 2, true};
        drawButton(easySubmitButton_, true);
        drawButton(easyResetButton_);
    } else if (activeModeIndex_ == 2) {
        drawText(bodyFont_, "本步候选（点击提交下一步）", centerPanel.x + 24, centerPanel.y + 240, rgb(70, 70, 70), false);
        for (std::size_t i = 0; i < mediumButtons_.size(); ++i) {
            drawButton(mediumButtons_[i], i == 0);
        }
    } else {
        SDL_Rect inputBox = makeRect(centerPanel.x + 24, centerPanel.y + 300, centerPanel.w - 48, 54);
        fillRect(renderer_, inputBox, hardInputFocused_ ? rgb(250, 248, 243) : rgb(255, 255, 255));
        strokeRect(renderer_, inputBox,
                   hardInputFocused_ ? rgb(58, 112, 82) : rgb(36, 36, 36),
                   hardInputFocused_ ? 2 : 1);

        SDL_Rect committedRect = makeRect(inputBox.x + 10, inputBox.y + 6,  inputBox.w - 20, 22);
        SDL_Rect composeRect   = makeRect(inputBox.x + 10, inputBox.y + 28, inputBox.w - 20, 22);

        if (hardInputFocused_) {
            SDL_SetTextInputRect(&composeRect);
        }

        const std::string committedText =
            inputBuffer_.empty() ? "中文结果：" : ("中文结果：" + inputBuffer_);
        const std::string composeText =
            imeComposition_.empty() ? "拼音/组合：等待输入法上屏…" : ("拼音/组合：" + imeComposition_);

        drawFittedTextInRect(
            smallFont_,
            committedText,
            committedRect,
            inputBuffer_.empty() ? rgb(130, 130, 130) : rgb(20, 20, 20),
            false,
            true
        );

        drawFittedTextInRect(
            smallFont_,
            composeText,
            composeRect,
            imeComposition_.empty() ? rgb(130, 130, 130) : rgb(48, 96, 160),
            false,
            true
        );

        if (hardInputFocused_) {
            drawLine(renderer_,
                     inputBox.x + 8, inputBox.y + inputBox.h - 6,
                     inputBox.x + inputBox.w - 8, inputBox.y + inputBox.h - 6,
                     rgb(58, 112, 82));
        }

        hardSubmitButton_ = ButtonSpec{
            makeRect(centerPanel.x + centerPanel.w - 164, centerPanel.y + 368, 140, 44),
            "提交输入", 1, true
        };
        drawButton(hardSubmitButton_, true);

        for (std::size_t i = 0; i < hardCandidateButtons_.size(); ++i) {
            drawButton(hardCandidateButtons_[i], i == 0);
        }
    }

    std::string explanationText;
    if (isBattleMode(session.mode)) {
        explanationText += "对手：";
        explanationText += (session.remotePlayerName.empty() ? "等待中" : session.remotePlayerName);
        explanationText += "\n对手步数：" + std::to_string(session.remoteStepCount);
        explanationText += "\n对手用时：" + formatSeconds(session.remoteElapsedSeconds);
        explanationText += "\n对手得分：" + std::to_string(session.remoteScore);
        explanationText += "\n对手状态：";
        explanationText += session.remoteFinished ? "已完成" : "进行中";
        explanationText += "\n\n";
    }

    const auto explanations = controller_.currentPathExplanations();
    if (!explanations.empty()) {
        explanationText += explanations.back();
    } else {
        explanationText += "当前路径还没有可显示的释义。";
    }
    if (!hintMessage_.empty()) {
        explanationText += "\n\n" + hintMessage_;
    }
    if (!revealedAnswerText_.empty()) {
        explanationText += "\n\n最优解：\n" + revealedAnswerText_;
    }
    gameExplanationContentHeight_ = drawWrappedTextScrollable(bodyFont_, explanationText,
                    makeRect(rightPanel.x + 16, rightPanel.y + 52, rightPanel.w - 32, rightPanel.h - 68),
                    rgb(72, 72, 72),
                    gameExplanationScroll_);
}

void IdiomChainScene::renderResult() {
    const GameSession& session = controller_.getSession();
    const int diffSteps = session.bestStepCount >= 0 ? std::max(0, session.stepCount - session.bestStepCount) : 0;

    drawText(titleFont_, "本局结算", kWindowWidth / 2, 88, rgb(20, 20, 20), true);
    drawText(subtitleFont_,
             isBattleMode(session.mode)
                 ? (session.battleWinnerText.empty() ? "双人对战结算" : session.battleWinnerText)
                 : (session.success ? "挑战成功" : "挑战失败"),
             kWindowWidth / 2, 128,
             session.success ? rgb(34, 108, 62) : rgb(156, 44, 44), true);

    SDL_Rect leftPanel = makeRect(60, 180, 250, 430);
    SDL_Rect centerPanel = makeRect(330, 180, 390, 430);
    SDL_Rect rightPanel = makeRect(740, 180, 400, 430);
    drawPanel(leftPanel, "成绩总览");
    drawPanel(centerPanel, isBattleMode(session.mode) ? "对手结果 / 路径对照" : "路径对照");
    drawPanel(rightPanel, "成语释义");

    drawCard(makeRect(leftPanel.x + 18, leftPanel.y + 54, leftPanel.w - 36, 70), "你的用时", formatSeconds(session.elapsedSeconds));
    drawCard(makeRect(leftPanel.x + 18, leftPanel.y + 136, leftPanel.w - 36, 70), "路径长度", std::to_string(session.stepCount));
    drawCard(makeRect(leftPanel.x + 18, leftPanel.y + 218, leftPanel.w - 36, 70), "最优解步数", std::to_string(session.bestStepCount));
    drawCard(makeRect(leftPanel.x + 18, leftPanel.y + 300, leftPanel.w - 36, 70), "得分", std::to_string(session.score), true);
    drawCard(makeRect(leftPanel.x + 18, leftPanel.y + 382, leftPanel.w - 36, 34), "与最优解差距", std::to_string(diffSteps) + " 步");

    if (isBattleMode(session.mode)) {
        SDL_Rect rivalCard = makeRect(centerPanel.x + 18, centerPanel.y + 54, centerPanel.w - 36, 128);
        fillRect(renderer_, rivalCard, rgb(255, 255, 255));
        strokeRect(renderer_, rivalCard, rgb(36, 36, 36), 1);
        drawFittedTextInRect(bodyFont_, "对手成绩",
                             makeRect(rivalCard.x + 10, rivalCard.y + 8, rivalCard.w - 20, 22),
                             rgb(22, 22, 22), false, true);
        std::ostringstream rival;
        rival << "玩家：" << (session.remotePlayerName.empty() ? "对手" : session.remotePlayerName) << "\n";
        rival << "用时：" << formatSeconds(session.remoteElapsedSeconds) << "\n";
        rival << "步数：" << session.remoteStepCount << "\n";
        rival << "得分：" << session.remoteScore << "\n";
        rival << "状态：" << (session.remoteFinished ? "已完成" : "未完成");
        resultMyRouteContentHeight_ = drawWrappedTextScrollable(
            smallFont_, rival.str(),
            makeRect(rivalCard.x + 12, rivalCard.y + 34, rivalCard.w - 24, rivalCard.h - 42),
            rgb(92, 92, 92),
            resultMyRouteScroll_);

        SDL_Rect pathCard = makeRect(centerPanel.x + 18, centerPanel.y + 198, centerPanel.w - 36, 170);
        fillRect(renderer_, pathCard, rgb(255, 255, 255));
        strokeRect(renderer_, pathCard, rgb(36, 36, 36), 1);
        drawFittedTextInRect(bodyFont_, "对手路径 / 最优路线",
                             makeRect(pathCard.x + 10, pathCard.y + 8, pathCard.w - 20, 22),
                             rgb(22, 22, 22), false, true);
        std::string centerText = "对手路径：\n" + (remotePathText().empty() ? "暂无同步路径。" : remotePathText())
                               + "\n\n最优路线：\n" + joinWords(answerPathWords(), " -> ");
        resultBestRouteContentHeight_ = drawWrappedTextScrollable(
            smallFont_, centerText,
            makeRect(pathCard.x + 12, pathCard.y + 34, pathCard.w - 24, pathCard.h - 42),
            rgb(92, 92, 92),
            resultBestRouteScroll_);
    } else {
        SDL_Rect myRouteCard = makeRect(centerPanel.x + 18, centerPanel.y + 54, centerPanel.w - 36, 128);
        fillRect(renderer_, myRouteCard, rgb(255, 255, 255));
        strokeRect(renderer_, myRouteCard, rgb(36, 36, 36), 1);
        drawFittedTextInRect(bodyFont_, "你的路线",
                             makeRect(myRouteCard.x + 10, myRouteCard.y + 8, myRouteCard.w - 20, 22),
                             rgb(22, 22, 22), false, true);
        resultMyRouteContentHeight_ = drawWrappedTextScrollable(
            smallFont_,
            currentPathText().empty() ? "暂无记录。" : currentPathText(),
            makeRect(myRouteCard.x + 12, myRouteCard.y + 34, myRouteCard.w - 24, myRouteCard.h - 42),
            rgb(92, 92, 92),
            resultMyRouteScroll_);

        SDL_Rect bestRouteCard = makeRect(centerPanel.x + 18, centerPanel.y + 198, centerPanel.w - 36, 170);
        fillRect(renderer_, bestRouteCard, rgb(255, 255, 255));
        strokeRect(renderer_, bestRouteCard, rgb(36, 36, 36), 1);
        drawFittedTextInRect(bodyFont_, "最优路线",
                             makeRect(bestRouteCard.x + 10, bestRouteCard.y + 8, bestRouteCard.w - 20, 22),
                             rgb(22, 22, 22), false, true);
        resultBestRouteContentHeight_ = drawWrappedTextScrollable(
            smallFont_,
            joinWords(answerPathWords(), " -> "),
            makeRect(bestRouteCard.x + 12, bestRouteCard.y + 34, bestRouteCard.w - 24, bestRouteCard.h - 42),
            rgb(92, 92, 92),
            resultBestRouteScroll_);
    }

    std::string explanationText;
    const auto explanations = controller_.currentPathExplanations();
    if (isBattleMode(session.mode) && !session.battleWinnerText.empty()) {
        explanationText += "胜负结果：" + session.battleWinnerText + "\n\n";
    }
    if (explanations.empty()) {
        explanationText += "暂无释义。";
    } else {
        const std::size_t limit = std::min<std::size_t>(5, explanations.size());
        for (std::size_t i = 0; i < limit; ++i) {
            if (i > 0U) {
                explanationText += "\n\n";
            }
            explanationText += explanations[i];
        }
    }
    resultExplanationContentHeight_ = drawWrappedTextScrollable(bodyFont_, explanationText,
                    makeRect(rightPanel.x + 16, rightPanel.y + 54, rightPanel.w - 32, rightPanel.h - 70),
                    rgb(72, 72, 72),
                    resultExplanationScroll_);

    resultAgainButton_ = ButtonSpec{makeRect(kWindowWidth / 2 - 220, 660, 140, 50), isBattleMode(session.mode) ? "再开房间" : "再来一局", 1, true};
    resultRankButton_ = ButtonSpec{makeRect(kWindowWidth / 2 - 70, 660, 140, 50), "排行榜", 2, true};
    resultMenuButton_ = ButtonSpec{makeRect(kWindowWidth / 2 + 80, 660, 140, 50), "主菜单", 3, true};
    drawButton(resultAgainButton_, true);
    drawButton(resultRankButton_);
    drawButton(resultMenuButton_);
}

void IdiomChainScene::renderLeaderboard() {
    drawText(titleFont_, "排行榜", kWindowWidth / 2, 88, rgb(20, 20, 20), true);
    drawText(subtitleFont_, "总积分排行 + 最近对局记录", kWindowWidth / 2, 128, rgb(88, 88, 88), true);

    SDL_Rect rankPanel = makeRect(50, 180, 760, 500);
    SDL_Rect historyPanel = makeRect(830, 180, 320, 500);
    drawPanel(rankPanel, "总积分排行榜");
    drawPanel(historyPanel, "最近记录");

    const std::vector<int> widths{70, 160, 120, 80, 290};
    SDL_Rect header = makeRect(rankPanel.x + 20, rankPanel.y + 54, rankPanel.w - 40, 50);
    drawSimpleTableRow(header, {"排名", "玩家", "总得分", "胜场", "称号"}, widths, true);

    const SDL_Rect rankViewport = makeRect(header.x, header.y + 58, header.w, rankPanel.h - 132);
    leaderboardRankContentHeight_ = static_cast<int>(leaderboardCache_.size()) * 50;
    clampScrollOffset(leaderboardRankScroll_, leaderboardRankContentHeight_, rankViewport.h);
    SDL_RenderSetClipRect(renderer_, &rankViewport);
    int rowY = header.y + 58 - leaderboardRankScroll_;
    for (std::size_t i = 0; i < leaderboardCache_.size(); ++i) {
        SDL_Rect row = makeRect(header.x, rowY, header.w, 44);
        drawSimpleTableRow(row,
                           {
                               std::to_string(static_cast<int>(i) + 1),
                               leaderboardCache_[i].playerName,
                               std::to_string(leaderboardCache_[i].totalScore),
                               "-",
                               (i == 0 ? "榜首" : i == 1 ? "强者" : i == 2 ? "高手" : "玩家")
                           },
                           widths,
                           false);
        rowY += 50;
    }
    SDL_RenderSetClipRect(renderer_, nullptr);
    if (leaderboardRankContentHeight_ > rankViewport.h) {
        SDL_Rect track{rankViewport.x + rankViewport.w - 6, rankViewport.y, 4, rankViewport.h};
        fillRect(renderer_, track, rgb(232, 232, 232));
        const int maxOffset = std::max(1, leaderboardRankContentHeight_ - rankViewport.h);
        const int thumbH = std::max(24, rankViewport.h * rankViewport.h / leaderboardRankContentHeight_);
        const int thumbY = rankViewport.y + (rankViewport.h - thumbH) * leaderboardRankScroll_ / maxOffset;
        fillRect(renderer_, makeRect(track.x, thumbY, track.w, thumbH), rgb(120, 120, 120));
    }

    const SDL_Rect historyViewport = makeRect(historyPanel.x + 16, historyPanel.y + 56, historyPanel.w - 32, historyPanel.h - 72);
    if (historyCache_.empty()) {
        leaderboardHistoryContentHeight_ = drawWrappedTextScrollable(bodyFont_, "暂无记录。完成至少一局后这里会显示最近的对局信息。",
                        makeRect(historyPanel.x + 16, historyPanel.y + 56, historyPanel.w - 32, 120), rgb(82, 82, 82), leaderboardHistoryScroll_);
    } else {
        leaderboardHistoryContentHeight_ = static_cast<int>(historyCache_.size()) * 74;
        clampScrollOffset(leaderboardHistoryScroll_, leaderboardHistoryContentHeight_, historyViewport.h);
        SDL_RenderSetClipRect(renderer_, &historyViewport);
        int hy = historyPanel.y + 56 - leaderboardHistoryScroll_;
        for (std::size_t i = 0; i < historyCache_.size(); ++i) {
            SDL_Rect card = makeRect(historyPanel.x + 16, hy, historyPanel.w - 32, 66);
            const GameRecord& rec = historyCache_[i];
            std::ostringstream oss;
            oss << rec.playerName << "｜" << rec.difficulty << "\n"
                << rec.startWord << " -> " << rec.targetWord << "\n"
                << "用时：" << formatSeconds(rec.elapsedSeconds) << "    得分：" << rec.score;
            drawCard(card, "对局记录", oss.str(), false);
            hy += 74;
        }
        SDL_RenderSetClipRect(renderer_, nullptr);
        if (leaderboardHistoryContentHeight_ > historyViewport.h) {
            SDL_Rect track{historyViewport.x + historyViewport.w - 6, historyViewport.y, 4, historyViewport.h};
            fillRect(renderer_, track, rgb(232, 232, 232));
            const int maxOffset = std::max(1, leaderboardHistoryContentHeight_ - historyViewport.h);
            const int thumbH = std::max(24, historyViewport.h * historyViewport.h / leaderboardHistoryContentHeight_);
            const int thumbY = historyViewport.y + (historyViewport.h - thumbH) * leaderboardHistoryScroll_ / maxOffset;
            fillRect(renderer_, makeRect(track.x, thumbY, track.w, thumbH), rgb(120, 120, 120));
        }
    }

    drawButton(leaderboardBackButton_);
}

void IdiomChainScene::beginEasyDrag(int mouseX, int mouseY) {
    draggingEasyTile_ = false;
    draggedEasyPoolIndex_ = -1;
    for (auto it = easyTiles_.rbegin(); it != easyTiles_.rend(); ++it) {
        if (pointInRect(mouseX, mouseY, it->rect)) {
            draggingEasyTile_ = true;
            draggedEasyPoolIndex_ = it->poolIndex;
            draggedFromSelectedZone_ = it->inSelectedZone;
            dragMouseX_ = mouseX;
            dragMouseY_ = mouseY;
            return;
        }
    }
}

void IdiomChainScene::updateEasyDrag(int mouseX, int mouseY) {
    dragMouseX_ = mouseX;
    dragMouseY_ = mouseY;
}

void IdiomChainScene::endEasyDrag() {
    if (!draggingEasyTile_ || draggedEasyPoolIndex_ < 0) {
        draggingEasyTile_ = false;
        return;
    }

    auto eraseValue = [](std::vector<int>& values, int target) {
        values.erase(std::remove(values.begin(), values.end(), target), values.end());
    };
    eraseValue(easySelectedOrder_, draggedEasyPoolIndex_);
    eraseValue(easyDiscardOrder_, draggedEasyPoolIndex_);

    auto insertIntoOrder = [&](std::vector<int>& order, const SDL_Rect& zone, int cols) {
        const int localX = dragMouseX_ - (zone.x + 18);
        const int localY = dragMouseY_ - (zone.y + 44);
        const int col = std::max(0, std::min(cols - 1, localX / (kTileWidth + kTileGap)));
        const int row = std::max(0, localY / (kTileHeight + kTileGap));
        int pos = row * cols + col;
        pos = std::max(0, std::min(pos, static_cast<int>(order.size())));
        order.insert(order.begin() + pos, draggedEasyPoolIndex_);
    };

    if (pointInRect(dragMouseX_, dragMouseY_, easySelectedZoneRect_)) {
        insertIntoOrder(easySelectedOrder_, easySelectedZoneRect_, 4);
    } else if (pointInRect(dragMouseX_, dragMouseY_, easyDiscardZoneRect_)) {
        insertIntoOrder(easyDiscardOrder_, easyDiscardZoneRect_, 4);
    } else {
        if (draggedFromSelectedZone_) {
            easySelectedOrder_.push_back(draggedEasyPoolIndex_);
        } else {
            easyDiscardOrder_.push_back(draggedEasyPoolIndex_);
        }
    }

    relayoutEasyTiles();
    draggingEasyTile_ = false;
    draggedEasyPoolIndex_ = -1;
}

bool IdiomChainScene::submitHardInputBuffer() {
    if (inputBuffer_.empty()) {
        statusMessage_ = "请输入成语后再提交。";
        return false;
    }
    const bool ok = controller_.submitHardInput(inputBuffer_);
    statusMessage_ = ok ? "已提交一步。" : controller_.getLastMessage();
    if (ok) {
        inputBuffer_.clear();
        imeComposition_.clear();
        imeCursor_ = 0;
        revealedAnswerText_.clear();
    }
    refreshHardCandidates(true);
    return ok;
}

std::string IdiomChainScene::modeText(int modeIndex) const {
    switch (modeIndex) {
    case 1: return "简单模式";
    case 2: return "中等模式";
    case 3: return "困难模式";
    default: return "成语接龙";
    }
}

std::string IdiomChainScene::currentPathText() const {
    return joinWords(currentPathWords(), " -> ");
}

std::vector<std::string> IdiomChainScene::currentPathWords() const {
    std::vector<std::string> words;
    const GameSession& session = controller_.getSession();
    for (int idiomId : session.playerPath) {
        words.push_back(controller_.wordOf(idiomId));
    }
    return words;
}

std::vector<std::string> IdiomChainScene::answerPathWords() const {
    return controller_.bestPathWords();
}

std::string IdiomChainScene::remotePathText() const {
    std::vector<std::string> words;
    const GameSession& session = controller_.getSession();
    for (int idiomId : session.remotePath) {
        words.push_back(controller_.wordOf(idiomId));
    }
    return joinWords(words, " -> ");
}

int IdiomChainScene::battleModeIndexToLocal(GameMode mode) const {
    switch (mode) {
    case GameMode::BattleEasy: return 1;
    case GameMode::BattleMedium: return 2;
    case GameMode::BattleHard: return 3;
    default: return 2;
    }
}

GameMode IdiomChainScene::selectedBattleMode() const {
    switch (battleModeIndex_) {
    case 1: return GameMode::BattleEasy;
    case 2: return GameMode::BattleMedium;
    case 3: return GameMode::BattleHard;
    default: return GameMode::BattleMedium;
    }
}
