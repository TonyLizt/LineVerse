#include "CorpusBuilder.h"

#include <fstream>
#include <iostream>
#include <sstream>

namespace lineverse::poetryrebuild {

namespace {

void appendJsonStringLike(const Json& value, std::vector<std::string>& out) {
    if (value.is_string()) {
        out.push_back(value.get<std::string>());
        return;
    }

    if (value.is_array()) {
        std::string merged;
        for (const auto& item : value) {
            if (item.is_string()) {
                merged += item.get<std::string>();
            }
        }
        if (!merged.empty()) {
            out.push_back(merged);
        }
        return;
    }

    if (value.is_object()) {
        if (value.contains("content") && value["content"].is_string()) {
            out.push_back(value["content"].get<std::string>());
            return;
        }
        if (value.contains("paragraph") && value["paragraph"].is_string()) {
            out.push_back(value["paragraph"].get<std::string>());
            return;
        }
        if (value.contains("sentence") && value["sentence"].is_string()) {
            out.push_back(value["sentence"].get<std::string>());
            return;
        }
    }
}

} // namespace

CorpusBuilder::CorpusBuilder(TextNormalizer normalizer)
    : normalizer_(std::move(normalizer)) {
}

std::vector<PhraseInfo> CorpusBuilder::buildPoemCorpus(const std::vector<Json>& rawEntries, BuildStats& stats) const {
    stats.rawPoemEntries = static_cast<int>(rawEntries.size());

    std::vector<PhraseInfo> poems;
    poems.reserve(rawEntries.size() * 2);
    std::unordered_set<std::string> seenText;

    for (const auto& raw : rawEntries) {
        const std::string author = raw.value("author", "");
        const std::string title = raw.value("title", "");
        const auto paragraphs = readParagraphs(raw);

        if (paragraphs.empty()) {
            ++stats.skippedPoemEntries;
            ++stats.poemEntriesWithoutParagraphs;

            if (stats.poemDebugSampleCount < 10) {
                std::ostringstream oss;
                oss << "author=" << author
                    << " | title=" << title
                    << " | keys=" << raw.dump();
                stats.poemNoParagraphSamples.push_back(oss.str());
                ++stats.poemDebugSampleCount;
            }
            continue;
        }

        auto lines = normalizer_.splitPoemToLines(paragraphs);

        // 回退策略：如果按句切分后为空，尝试把整个 paragraph 直接规范化
        if (lines.empty()) {
            for (const auto& paragraph : paragraphs) {
                const std::string normalizedWhole = normalizer_.normalizeText(paragraph);
                if (!normalizedWhole.empty() && normalizer_.countChineseChars(normalizedWhole) >= 2) {
                    lines.push_back(normalizedWhole);
                }
            }
            if (!lines.empty()) {
                ++stats.poemEntriesRecoveredByFallback;
            }
        }

        if (lines.empty()) {
            ++stats.skippedPoemEntries;
            ++stats.poemEntriesWithoutUsableLines;

            if (static_cast<int>(stats.poemNoLineSamples.size()) < 10) {
                std::ostringstream oss;
                oss << "author=" << author
                    << " | title=" << title
                    << " | firstParagraphRaw=" << paragraphs.front()
                    << " | firstParagraphNormalized=" << normalizer_.normalizeText(paragraphs.front());
                stats.poemNoLineSamples.push_back(oss.str());
            }
            continue;
        }

        for (const auto& line : lines) {
            ++stats.poemLinesBeforeDedupe;
            const std::string dedupeKey = "poem|" + line;
            if (!seenText.insert(dedupeKey).second) {
                ++stats.duplicatePoemLines;
                continue;
            }

            PhraseInfo info;
            info.id = normalizer_.generateInternalId("poem", line, author, title);
            info.text = line;
            info.type = PhraseType::Poem;
            info.length = normalizer_.countChineseChars(line);
            info.charFreq = normalizer_.buildCharFreq(line);
            info.author = author;
            info.title = title;
            info.rarity = info.length >= 7 ? 3 : (info.length >= 5 ? 2 : 1);
            info.scoreWeight = info.length;
            info.sourceFile = raw.value("_source_file", "");
            poems.push_back(std::move(info));
        }
    }

    stats.finalPoemCount = static_cast<int>(poems.size());

    std::cout << "[PoemDebug] rawPoemEntries=" << stats.rawPoemEntries
              << ", poemEntriesWithoutParagraphs=" << stats.poemEntriesWithoutParagraphs
              << ", poemEntriesWithoutUsableLines=" << stats.poemEntriesWithoutUsableLines
              << ", poemEntriesRecoveredByFallback=" << stats.poemEntriesRecoveredByFallback
              << ", poemLinesBeforeDedupe=" << stats.poemLinesBeforeDedupe
              << ", duplicatePoemLines=" << stats.duplicatePoemLines
              << ", finalPoemCount=" << stats.finalPoemCount
              << std::endl;

    if (!stats.poemNoParagraphSamples.empty()) {
        std::cout << "[PoemDebug] samples of entries without paragraphs:\n";
        for (const auto& sample : stats.poemNoParagraphSamples) {
            std::cout << "  - " << sample << '\n';
        }
    }

    if (!stats.poemNoLineSamples.empty()) {
        std::cout << "[PoemDebug] samples of entries that produced no usable lines:\n";
        for (const auto& sample : stats.poemNoLineSamples) {
            std::cout << "  - " << sample << '\n';
        }
    }

    return poems;
}

std::vector<PhraseInfo> CorpusBuilder::buildIdiomCorpus(const std::vector<Json>& rawEntries, BuildStats& stats) const {
    stats.rawIdiomEntries = static_cast<int>(rawEntries.size());

    std::vector<PhraseInfo> idioms;
    idioms.reserve(rawEntries.size());
    std::unordered_set<std::string> seenText;

    for (const auto& raw : rawEntries) {
        const std::string word = normalizer_.normalizeText(raw.value("word", ""));
        if (word.empty()) {
            ++stats.skippedIdiomEntries;
            continue;
        }

        const std::string dedupeKey = "idiom|" + word;
        if (!seenText.insert(dedupeKey).second) {
            ++stats.duplicateIdioms;
            continue;
        }

        PhraseInfo info;
        info.id = normalizer_.generateInternalId("idiom", word, "", "");
        info.text = word;
        info.type = PhraseType::Idiom;
        info.length = normalizer_.countChineseChars(word);
        info.charFreq = normalizer_.buildCharFreq(word);
        info.pinyin = raw.value("pinyin", "");
        info.explanation = raw.value("explanation", "");
        info.derivation = raw.value("derivation", "");
        info.example = raw.value("example", "");
        info.rarity = info.length >= 4 ? 2 : 1;
        info.scoreWeight = info.length;
        info.sourceFile = raw.value("_source_file", "");
        idioms.push_back(std::move(info));
    }

    stats.finalIdiomCount = static_cast<int>(idioms.size());
    return idioms;
}

void CorpusBuilder::writePhraseList(const std::vector<PhraseInfo>& phrases, const std::filesystem::path& outFile) const {
    Json root = Json::object();
    root["count"] = static_cast<int>(phrases.size());
    root["items"] = Json::array();

    for (const auto& phrase : phrases) {
        root["items"].push_back(phraseInfoToJson(phrase));
    }

    std::ofstream out(outFile, std::ios::binary);
    out << root.dump(2, ' ', true);
}

void CorpusBuilder::writeBuildReport(const BuildStats& stats, const std::filesystem::path& outFile) const {
    Json poemNoParagraphSamples = Json::array();
    for (const auto& s : stats.poemNoParagraphSamples) {
        poemNoParagraphSamples.push_back(s);
    }

    Json poemNoLineSamples = Json::array();
    for (const auto& s : stats.poemNoLineSamples) {
        poemNoLineSamples.push_back(s);
    }

    const Json report = {
        {"rawPoemEntries", stats.rawPoemEntries},
        {"poemLinesBeforeDedupe", stats.poemLinesBeforeDedupe},
        {"finalPoemCount", stats.finalPoemCount},
        {"duplicatePoemLines", stats.duplicatePoemLines},
        {"skippedPoemEntries", stats.skippedPoemEntries},
        {"poemEntriesWithoutParagraphs", stats.poemEntriesWithoutParagraphs},
        {"poemEntriesWithoutUsableLines", stats.poemEntriesWithoutUsableLines},
        {"poemEntriesRecoveredByFallback", stats.poemEntriesRecoveredByFallback},
        {"poemNoParagraphSamples", poemNoParagraphSamples},
        {"poemNoLineSamples", poemNoLineSamples},
        {"rawIdiomEntries", stats.rawIdiomEntries},
        {"finalIdiomCount", stats.finalIdiomCount},
        {"duplicateIdioms", stats.duplicateIdioms},
        {"skippedIdiomEntries", stats.skippedIdiomEntries}
    };

    std::ofstream out(outFile, std::ios::binary);
    out << report.dump(2, ' ', true);
}

std::vector<std::string> CorpusBuilder::readParagraphs(const Json& raw) {
    std::vector<std::string> paragraphs;

    if (raw.contains("paragraphs")) {
        const auto& value = raw["paragraphs"];
        if (value.is_array()) {
            for (const auto& item : value) {
                appendJsonStringLike(item, paragraphs);
            }
        } else {
            appendJsonStringLike(value, paragraphs);
        }
    }

    if (paragraphs.empty() && raw.contains("content")) {
        appendJsonStringLike(raw["content"], paragraphs);
    }

    if (paragraphs.empty() && raw.contains("lines")) {
        appendJsonStringLike(raw["lines"], paragraphs);
    }

    if (paragraphs.empty() && raw.contains("paragraph")) {
        appendJsonStringLike(raw["paragraph"], paragraphs);
    }

    return paragraphs;
}

} // namespace lineverse::poetryrebuild