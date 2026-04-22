#pragma once

// HandleGame 模块对外唯一入口。
// 返回值约定：
//   0  : 直接退出整个游戏（主程序应关闭）
//   1  : 返回主菜单（主程序继续运行并回到菜单）
//  -1  : 模块异常退出（主程序可提示错误并回主菜单或退出）

struct SDL_Window;
struct SDL_Renderer;

namespace HandleGame {

int start();
int start(SDL_Window* externalWindow, SDL_Renderer* externalRenderer);

} // namespace HandleGame