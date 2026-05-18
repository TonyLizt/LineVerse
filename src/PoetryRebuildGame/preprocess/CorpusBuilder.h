#pragma once

#include <filesystem>
#include <string>
#include <unordered_set>
#include <vector>

#include "../common/Json.h"
#include "../model/PhraseInfo.h"
#include "TextNormalizer.h"

namespace lineverse::poetryrebuild {

struct BuildStats {
    int rawPoemEntries = 0;
    int poemLinesBeforeDedupe = 0;
    int finalPoemCount = 0;

    int rawIdiomEntries = 0;
    int finalIdiomCount = 0;

    int duplicatePoemLines = 0;
    int duplicateIdioms = 0;

    int skippedPoemEntries = 0;
    int skippedIdiomEntries = 0;

    // ===== 诗句调试统计 =====
    int poemEntriesWithoutParagraphs = 0;
    int poemEntriesWithoutUsableLines = 0;
    int poemEntriesRecoveredByFallback = 0;
    int poemDebugSampleCount = 0;
    std::vector<std::string> poemNoParagraphSamples;
    std::vector<std::string> poemNoLineSamples;
};

class CorpusBuilder {
public:
    explicit CorpusBuilder(TextNormalizer normalizer = TextNormalizer{});

    std::vector<PhraseInfo> buildPoemCorpus(const std::vector<Json>& rawEntries, BuildStats& stats) const;
    std::vector<PhraseInfo> buildIdiomCorpus(const std::vector<Json>& rawEntries, BuildStats& stats) const;

    void writePhraseList(const std::vector<PhraseInfo>& phrases, const std::filesystem::path& outFile) const;
    void writeBuildReport(const BuildStats& stats, const std::filesystem::path& outFile) const;

private:
    TextNormalizer normalizer_;

    static std::vector<std::string> readParagraphs(const Json& raw);
};

} // namespace lineverse::poetryrebuild