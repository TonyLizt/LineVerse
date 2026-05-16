# 句读之间 LineVerse

## 项目简介

《句读之间》（LineVerse）是一款以中华优秀传统文化为主题的文字益智游戏系统，基于 C++ 与 SDL2 开发。项目围绕成语和古诗词内容设计多个小游戏模块，将传统文化学习、文字推理和数据结构算法实践结合起来。

本项目是北京邮电大学计算机学院《数据结构课程设计》课程项目。项目目标不仅是完成一个可以运行的小游戏合集，也希望通过不同玩法体现图、队列、栈、哈希表、集合、Trie、字符串处理、DFS、BFS 等数据结构与算法在实际软件中的应用。

---

## 项目成员

| 姓名   | 主要负责内容                                      |
| ------ | ------------------------------------------------- |
| 李泽同 | 组长；成语接龙模块；整体架构设计与项目整合        |
| 刘懋兴 | 汉兜模块；中文输入、拼音特征判定与界面交互        |
| 何明涛 | 诗词描述猜测模块；题库、提示、候选集过滤与判题    |
| 梁兆荣 | [VinsorLeongSioWeng](https://github.com/VinsorLeongSioWeng) | 首页 UI；诗成语现模块；字池、候选检索与最优解搜索 |

---

## 功能模块

### 1. 首页 UI

首页作为整个游戏平台的统一入口，负责展示各个小游戏模块，并支持鼠标和键盘操作。

源码位置：

```text
src/ui/HomepageScreen.h
src/ui/HomepageScreen.cpp
src/main.cpp
```

主要功能：

- 展示游戏主菜单；
- 支持鼠标悬浮、点击选择；
- 支持方向键、回车键、Esc 键操作；
- 支持首页、模式选择页、难度选择页的状态切换；
- 根据玩家选择启动对应游戏模块；
- 复用主窗口和渲染器，减少多窗口切换带来的割裂感。

首页目前包含以下入口：

- 汉兜；
- 成语接龙；
- 诗成语现；
- 线索诗词。

---

### 2. 成语接龙 IdiomChainGame

成语接龙模块会生成一组起点成语和终点成语，玩家按照成语接龙规则构造路径，最终到达终点。与普通成语接龙不同，本模块强调“如何接得更短、更快、更优”，因此引入了最短路径、路径评价、提示、排行榜和对战机制。

源码位置：

```text
src/IdiomChainGame/
├── app/
├── core/
├── data/
├── net/
├── ui/
├── IdiomChainGame.cpp
└── IdiomChainGame.h
```

主要功能：

- 支持简单、中等、困难三种难度；
- 支持完整成语和缩写输入；
- 支持 BFS 求最短路径；
- 支持反向 BFS 计算所有成语到终点的距离；
- 支持多条同长最优路径；
- 支持分层提示、撤回、路径评价；
- 支持 CSV 成绩记录和排行榜；
- 支持双人对战状态同步。

主要数据结构与算法：

- 有向图；
- 邻接表；
- 反向邻接表；
- BFS 最短路径；
- 反向 BFS；
- 栈；
- 哈希表；
- 哈希集合；
- 优先队列；
- TCP socket 消息通信。

---

### 3. 汉兜 HandleGame

汉兜模块是一个基于 Wordle 机制的四字成语猜测游戏。玩家输入四字成语后，系统不仅反馈汉字是否正确，还会从声母、韵母和声调三个维度给出判定结果。

源码位置：

```text
src/HandleGame/
├── core/
├── UI/
├── tools/
├── HandleGame.cpp
└── HandleGame.hpp
```

主要功能：

- 支持简单、中等、困难三种词库；
- 支持中文输入和 UTF-8 字符处理；
- 支持四字成语合法性判断；
- 支持汉字、声母、韵母、声调四类反馈；
- 支持数字音调和符号音调显示切换；
- 支持提示弹窗和界面交互；
- 支持返回主菜单。

主要数据结构与算法：

- UTF-8 中文字符切分；
- 固定长度数组建模；
- 哈希表词库索引；
- 哈希集合快速判断；
- 拼音拆分与声调归一化；
- 双阶段颜色判定算法。

---

### 4. 线索诗词 VerseUnfoldGame

线索诗词模块围绕一首目标诗词逐步释放提示，玩家根据朝代、体裁、情感、意象、创作背景、首句前缀等线索猜测目标诗词。

源码位置：

```text
src/VerseUnfoldGame/
├── controller/
├── data/
├── service/
├── ui/
├── utils/
├── VerseUnfoldGame.cpp
└── VerseUnfoldGame.h
```

主要功能：

- 从 JSON 题库读取结构化诗词数据；
- 支持按难度选题；
- 支持提示逐步释放；
- 支持标题、别名、正文等多种判题方式；
- 支持输入标准化，减少空格、标点、书名号等格式差异造成的误判；
- 支持候选集动态缩小；
- 支持积分结算和排行榜扩展。

主要数据结构与算法：

- 顺序表；
- 多维哈希索引；
- 提示队列；
- 集合交集；
- 字符串标准化；
- 编辑距离；
- 排序。

---

### 5. 诗成语现 PoetryRebuildGame

诗成语现模块是一个基于诗句和成语语料库的文字重组游戏。系统生成一个汉字字池，玩家从字池中选择汉字组成合法诗句或成语。每次成功提交后，系统会扣除对应字牌，并更新分数、连击和剩余字池状态。

源码位置：

```text
src/PoetryRebuildGame/
├── bootstrap/
├── candidate/
├── common/
├── config/
├── game/
├── hint/
├── index/
├── level/
├── model/
├── preprocess/
├── resource/
├── solver/
├── terminal/
├── ui/
├── util/
├── PoetryRebuildGame.cpp
└── PoetryRebuildGame.h
```

主要功能：

- 支持成语模式和混合模式；
- 支持简单、中等、困难三种难度；
- 支持诗句和成语语料处理；
- 支持字池生成；
- 支持候选答案检索；
- 支持理论最优完成数计算；
- 支持提示、撤销、重做、出牌和结算；
- 支持 SDL 字牌交互界面；
- 支持加载界面和异步关卡生成。

主要数据结构与算法：

- 字频表；
- 多重集合包含判定；
- 倒排索引；
- Trie 字典树；
- 冲突图；
- DFS / 回溯；
- 分支限界；
- 贪心降级策略；
- 随机抽样；
- 候选排序。

---

## 项目结构

```text
LineVerse/
├── assets/                         # 图片、字体等资源
├── data/                           # 成语、诗词、预处理数据
├── libs/                           # 第三方库
│   ├── asio-1.36.0
│   ├── nlohmann
│   └── SDL2-2.26.0-allinone
├── src/                            # 源代码
│   ├── HandleGame/                 # 汉兜模块
│   ├── IdiomChainGame/             # 成语接龙模块
│   ├── PoetryRebuildGame/          # 诗成语现模块
│   ├── VerseUnfoldGame/            # 线索诗词模块
│   ├── ui/                         # 首页 UI
│   ├── main.cpp                    # 主程序入口
│   └── main.h
├── CMakeLists.txt                  # CMake 构建文件
├── README.md                       # 项目说明
└── .gitignore
```

---

## 开发环境

本项目主要开发和测试环境如下：

| 项目     | 版本 / 说明             |
| -------- | ----------------------- |
| 操作系统 | Windows 10 / Windows 11 |
| 开发语言 | C++17                   |
| 构建工具 | CMake                   |
| 编译器   | MinGW-w64 / GCC 14.2.0  |
| 图形库   | SDL2                    |
| 图片库   | SDL2_image              |
| 字体库   | SDL2_ttf                |
| JSON 库  | nlohmann/json           |
| 网络通信 | TCP socket              |
| IDE      | Visual Studio Code      |

第三方依赖位于：

```text
libs/
├── asio-1.36.0
├── nlohmann
└── SDL2-2.26.0-allinone
```

说明：

- `SDL2-2.26.0-allinone` 用于窗口创建、事件处理、图片加载和字体显示；
- `nlohmann` 用于 JSON 数据解析；
- `asio-1.36.0` 用于网络模块，为多人游戏提供支持；
- 当前成语接龙对战模块的网络传输代码位于 `src/IdiomChainGame/net/`，实现方式为 TCP socket 通信。

---

## 数据与资源路径

程序运行时需要以下资源和数据目录存在：

```text
assets/
data/
libs/
```

部分模块会查找以下路径：

```text
assets/Homepage/image/bg.jpg
assets/Homepage/image/bg2.png
assets/Homepage/image/bg3.jpg
assets/Homepage/font/font2.ttf

assets/HandleGame/font/font1.ttf
assets/HandleGame/font/font2.ttf
assets/HandleGame/font/font3.ttf

assets/IdiomChainGame/fonts/
assets/VerseUnfoldGame/font/
assets/PoetryRebuildGame/image/
assets/PoetryRebuildGame/font/

data/prebuild/HandleGame/idioms_easy.tsv
data/prebuild/HandleGame/idioms_normal.tsv
data/prebuild/HandleGame/idioms_hard.tsv

data/prebuild/IdiomChainGame/idiom.csv
data/raw/verse_unfold_poetry_db.json
data/raw/
data/cache/
data/prebuild/
```

如果运行时报错提示找不到图片、字体或数据文件，请检查是否从项目根目录启动程序，或确认 `assets/` 和 `data/` 是否完整上传。

---

## 编译运行

### 方式一：使用 CMake

在项目根目录下执行：

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

编译完成后运行生成的可执行文件。

### 方式二：使用 IDE

可以使用 Visual Studio Code、CLion 或 Visual Studio 打开项目根目录，然后通过 CMake 配置并编译运行。

建议从项目根目录运行程序，便于程序自动定位 `assets/` 和 `data/` 目录。

---

## Windows 运行提示

如果运行时提示找不到 SDL2 相关动态库，请检查：

1. `libs/SDL2-2.26.0-allinone` 是否存在；
2. SDL2、SDL2_image、SDL2_ttf 的 `.dll` 是否能被程序找到；
3. 必要时可以将相关 `.dll` 复制到可执行文件所在目录；
4. 或者将 SDL2 的运行库目录加入系统 `PATH`。

如果终端中文输出乱码，可以使用支持 UTF-8 的终端运行。程序在 Windows 下会尝试设置控制台编码为 UTF-8。

---

## 第三方资源说明

本项目使用 SDL2、SDL2_image、SDL2_ttf、nlohmann/json 、asio等第三方开源库。以上第三方库版权归其原作者所有，本项目仅在课程设计中学习和使用。

本项目使用了以下开源数据集作为成语和诗词数据来源，并在课程设计中根据游戏需求进行了清洗、筛选、字段转换和预处理。

| 数据类型 | 数据来源 | 用途 | 说明 |
|---|---|---|---|
| 成语数据 | [crazywhalecc/idiom-database](https://github.com/crazywhalecc/idiom-database) | 成语接龙、汉兜、诗成语现中的成语数据 | 该项目提供 30000+ 个成语，并包含拼音、释义、缩写、首尾拼音等字段，适合用于成语接龙类游戏。 |
| 诗词数据 | [chinese-poetry/chinese-poetry](https://github.com/chinese-poetry/chinese-poetry) | 线索诗词、诗成语现中的诗词数据 | 该项目提供较完整的中华古诗词 JSON 数据，包括唐诗、宋诗、宋词等内容。 |


项目中的诗词、成语数据以及图片、字体等资源仅用于课程设计和学习展示。

---

## 学术诚信说明

本项目为北京邮电大学计算机学院《数据结构课程设计》课程项目，仅用于学习交流与项目展示。欢迎参考本项目的设计思路、数据结构应用和代码组织方式，但请勿直接复制、改名后作为个人或小组课程作业提交。

若在课程作业、实验报告或其他公开项目中参考了本项目内容，请注明来源。

Academic Integrity Notice: This repository is published for learning and demonstration purposes. You may refer to the ideas and implementation, but please do not copy or submit this project as your own coursework.

---

## 项目特色

1. 以中华传统文化为主题，融合成语和古诗词内容；
2. 采用多模块游戏平台结构，支持多个独立小游戏；
3. 使用 SDL2 实现图形界面、事件处理和中文文本显示；
4. 综合应用图、栈、队列、哈希表、集合、Trie、BFS、DFS、回溯等数据结构与算法；
5. 各模块尽量采用数据层、算法层、控制层、界面层分离的设计；
6. 支持提示、撤回、排行榜、对战、字池最优解等扩展功能。

---

## 说明

本项目为课程设计作品，主要用于学习、展示和交流。