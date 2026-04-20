#pragma once

#include <string>
#include <vector>

struct Poem {
    int id = 0;

    std::string title;
    std::vector<std::string> content;
    std::string author;
    std::string dynasty;
    std::string type;
    std::vector<std::string> lines;
    int difficulty = 1; // 1: 初窥门径 2: 渐入佳境 3: 登堂入室

    std::string emotion;
    std::vector<std::string> imagery;
    std::string creationBg;
    std::string distinctFeature;
    std::string authorTag;
    std::string rhythm;

    int sentenceCount = 0;
    int charCount = 0;

    // 游戏提示相关字段
    std::string emotionCore;
    std::string bgCore;
    std::string mainImageryOne;
    std::string featureCore;
    std::vector<std::string> imageryGameAll;
    std::string firstlinePrefix;
    std::vector<std::string> answerAlias;

    // 标准化字段
    std::string titleStd;
    std::string contentStd;
    std::string authorStd;
    std::string dynastyStd;
    std::string typeStd;
    std::string emotionCoreStd;
    std::string bgCoreStd;
    std::string featureCoreStd;
    std::string mainImageryOneStd;
    std::string firstlinePrefixStd;
    std::string rhythmStd;
    std::vector<std::string> imageryStd;
    std::vector<std::string> imageryGameAllStd;
    std::vector<std::string> answerAliasStd;
    std::vector<std::string> linesStd;
    std::string searchTextStd;
};
