#include "RuntimeBootstrap.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include "../common/Json.h"
#include "../preprocess/CorpusBuilder.h"
#include "../preprocess/TextNormalizer.h"
#include "../resource/ResourceLoader.h"
#include "../util/Utf8.h"

namespace fs = std::filesystem;

namespace lineverse::poetryrebuild {

namespace {

constexpr std::uint32_t kRuntimeCacheVersion = 1;

fs::path runtimeDir(const ProjectPaths& paths) {
    return paths.moduleCacheRoot / "runtime";
}

fs::path runtimeBinPath(const ProjectPaths& paths) {
    return runtimeDir(paths) / "runtime_cache.bin";
}

fs::path runtimeManifestPath(const ProjectPaths& paths) {
    return runtimeDir(paths) / "runtime_manifest.json";
}

void writeString(std::ostream& out, const std::string& s) {
    const std::uint64_t n = static_cast<std::uint64_t>(s.size());
    out.write(reinterpret_cast<const char*>(&n), sizeof(n));
    out.write(s.data(), static_cast<std::streamsize>(n));
}

std::string readString(std::istream& in) {
    std::uint64_t n = 0;
    in.read(reinterpret_cast<char*>(&n), sizeof(n));
    std::string s(n, '\0');
    if (n > 0) {
        in.read(s.data(), static_cast<std::streamsize>(n));
    }
    return s;
}

void writeCharFreq(std::ostream& out, const CharFreq& freq) {
    const std::uint64_t n = static_cast<std::uint64_t>(freq.size());
    out.write(reinterpret_cast<const char*>(&n), sizeof(n));
    for (const auto& [cp, count] : freq) {
        const std::int32_t codepoint = static_cast<std::int32_t>(cp);
        const std::int32_t c = static_cast<std::int32_t>(count);
        out.write(reinterpret_cast<const char*>(&codepoint), sizeof(codepoint));
        out.write(reinterpret_cast<const char*>(&c), sizeof(c));
    }
}

CharFreq readCharFreq(std::istream& in) {
    std::uint64_t n = 0;
    in.read(reinterpret_cast<char*>(&n), sizeof(n));
    CharFreq freq;
    for (std::uint64_t i = 0; i < n; ++i) {
        std::int32_t cp = 0;
        std::int32_t count = 0;
        in.read(reinterpret_cast<char*>(&cp), sizeof(cp));
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        freq[static_cast<CodePoint>(cp)] = count;
    }
    return freq;
}

void writePhraseInfo(std::ostream& out, const PhraseInfo& info) {
    writeString(out, info.id);
    writeString(out, info.text);
    writeString(out, info.author);
    writeString(out, info.title);
    writeString(out, info.pinyin);
    writeString(out, info.explanation);
    writeString(out, info.derivation);
    writeString(out, info.example);
    writeString(out, info.sourceFile);

    const std::int32_t type = static_cast<std::int32_t>(info.type);
    const std::int32_t length = info.length;
    const std::int32_t rarity = info.rarity;
    const std::int32_t scoreWeight = info.scoreWeight;

    out.write(reinterpret_cast<const char*>(&type), sizeof(type));
    out.write(reinterpret_cast<const char*>(&length), sizeof(length));
    out.write(reinterpret_cast<const char*>(&rarity), sizeof(rarity));
    out.write(reinterpret_cast<const char*>(&scoreWeight), sizeof(scoreWeight));

    writeCharFreq(out, info.charFreq);
}

PhraseInfo readPhraseInfo(std::istream& in) {
    PhraseInfo info;

    info.id = readString(in);
    info.text = readString(in);
    info.author = readString(in);
    info.title = readString(in);
    info.pinyin = readString(in);
    info.explanation = readString(in);
    info.derivation = readString(in);
    info.example = readString(in);
    info.sourceFile = readString(in);

    std::int32_t type = 0;
    std::int32_t length = 0;
    std::int32_t rarity = 0;
    std::int32_t scoreWeight = 0;

    in.read(reinterpret_cast<char*>(&type), sizeof(type));
    in.read(reinterpret_cast<char*>(&length), sizeof(length));
    in.read(reinterpret_cast<char*>(&rarity), sizeof(rarity));
    in.read(reinterpret_cast<char*>(&scoreWeight), sizeof(scoreWeight));

    info.type = static_cast<PhraseType>(type);
    info.length = length;
    info.rarity = rarity;
    info.scoreWeight = scoreWeight;
    info.charFreq = readCharFreq(in);

    return info;
}

void writePhraseList(std::ostream& out, const std::vector<PhraseInfo>& phrases) {
    const std::uint64_t n = static_cast<std::uint64_t>(phrases.size());
    out.write(reinterpret_cast<const char*>(&n), sizeof(n));
    for (const auto& p : phrases) {
        writePhraseInfo(out, p);
    }
}

std::vector<PhraseInfo> readPhraseList(std::istream& in) {
    std::uint64_t n = 0;
    in.read(reinterpret_cast<char*>(&n), sizeof(n));
    std::vector<PhraseInfo> phrases;
    phrases.reserve(static_cast<std::size_t>(n));
    for (std::uint64_t i = 0; i < n; ++i) {
        phrases.push_back(readPhraseInfo(in));
    }
    return phrases;
}

bool isRuntimeCacheFresh(const ProjectPaths& paths) {
    const fs::path bin = runtimeBinPath(paths);
    const fs::path manifest = runtimeManifestPath(paths);
    const fs::path poemsFile = paths.modulePrebuildRoot / "poems.json";
    const fs::path idiomsFile = paths.modulePrebuildRoot / "idioms.json";

    if (!fs::exists(bin) || !fs::exists(manifest) ||
        !fs::exists(poemsFile) || !fs::exists(idiomsFile)) {
        return false;
    }

    Json meta;
    {
        std::ifstream in(manifest, std::ios::binary);
        if (!in) {
            return false;
        }
        in >> meta;
    }

    if (!meta.is_object()) {
        return false;
    }

    if (meta.value("version", 0) != static_cast<int>(kRuntimeCacheVersion)) {
        return false;
    }

    const auto binTime = fs::last_write_time(bin);
    if (binTime < fs::last_write_time(poemsFile) ||
        binTime < fs::last_write_time(idiomsFile)) {
        return false;
    }

    return true;
}

std::vector<PhraseInfo> loadPhraseListFromPrebuild(
    const fs::path& filePath,
    PhraseType explicitType,
    TextNormalizer& normalizer
) {
    std::ifstream input(filePath, std::ios::binary);
    if (!input) {
        throw std::runtime_error("无法打开 JSON 文件: " + filePath.string());
    }

    Json root;
    input >> root;

    Json items = Json::array();
    if (root.is_object() && root.contains("items") && root["items"].is_array()) {
        items = root["items"];
    } else if (root.is_array()) {
        items = root;
    } else {
        throw std::runtime_error("预构建文件格式不正确: " + filePath.string());
    }

    std::vector<PhraseInfo> phrases;
    phrases.reserve(items.size());

    for (const auto& item : items) {
        if (!item.is_object()) {
            continue;
        }

        PhraseInfo info;
        info.text = item.value("text", "");
        if (info.text.empty()) {
            continue;
        }

        info.type = explicitType;
        info.id = item.value(
            "id",
            normalizer.generateInternalId(
                (info.type == PhraseType::Poem ? "poem" : "idiom"),
                info.text,
                item.value("author", ""),
                item.value("title", "")
            )
        );

        info.author = item.value("author", "");
        info.title = item.value("title", "");
        info.pinyin = item.value("pinyin", "");
        info.explanation = item.value("explanation", "");
        info.derivation = item.value("derivation", "");
        info.example = item.value("example", "");
        info.sourceFile = item.value("sourceFile", item.value("_source_file", ""));
        info.length = item.value("length", normalizer.countChineseChars(info.text));
        info.charFreq = normalizer.buildCharFreq(info.text);
        info.rarity = item.value("rarity", 1);
        info.scoreWeight = item.value("scoreWeight", info.length);

        phrases.push_back(std::move(info));
    }

    return phrases;
}

void buildRuntimeCache(const ProjectPaths& paths) {
    fs::create_directories(runtimeDir(paths));

    TextNormalizer normalizer;
    std::vector<PhraseInfo> poems;
    std::vector<PhraseInfo> idioms;

    const fs::path poemsFile = paths.modulePrebuildRoot / "poems.json";
    const fs::path idiomsFile = paths.modulePrebuildRoot / "idioms.json";

    if (fs::exists(poemsFile) && fs::exists(idiomsFile)) {
        poems = loadPhraseListFromPrebuild(poemsFile, PhraseType::Poem, normalizer);
        idioms = loadPhraseListFromPrebuild(idiomsFile, PhraseType::Idiom, normalizer);
    } else {
        ResourceLoader loader(paths);
        CorpusBuilder builder(normalizer);
        BuildStats stats{};

        auto rawPoems = loader.loadTangPoemEntries();
        auto rawIdioms = loader.loadIdiomEntries();

        poems = builder.buildPoemCorpus(rawPoems, stats);
        idioms = builder.buildIdiomCorpus(rawIdioms, stats);

        builder.writePhraseList(poems, poemsFile);
        builder.writePhraseList(idioms, idiomsFile);
        builder.writeBuildReport(stats, paths.moduleCacheRoot / "build_report.json");
    }

    IndexStore indexStore;
    indexStore.build(poems, idioms);

    {
        std::ofstream out(runtimeBinPath(paths), std::ios::binary);
        if (!out) {
            throw std::runtime_error("无法写入 runtime cache bin");
        }

        out.write(reinterpret_cast<const char*>(&kRuntimeCacheVersion), sizeof(kRuntimeCacheVersion));
        writePhraseList(out, poems);
        writePhraseList(out, idioms);

        // 这里要求你在 IndexStore 里补 saveSnapshot(out)
        indexStore.saveSnapshot(out);
    }

    {
        Json meta;
        meta["version"] = kRuntimeCacheVersion;
        meta["poems"] = static_cast<int>(poems.size());
        meta["idioms"] = static_cast<int>(idioms.size());

        std::ofstream out(runtimeManifestPath(paths), std::ios::binary);
        if (!out) {
            throw std::runtime_error("无法写入 runtime manifest");
        }
        out << meta.dump(2);
    }
}

} // namespace

void RuntimeBootstrap::prepare() {
    ProjectPaths paths = ProjectPaths::detect();
    paths.ensureOutputDirectories();

    if (isRuntimeCacheFresh(paths)) {
        return;
    }

    const fs::path poemsFile = paths.modulePrebuildRoot / "poems.json";
    const fs::path idiomsFile = paths.modulePrebuildRoot / "idioms.json";
    const bool hasPrebuiltCorpus = fs::exists(poemsFile) && fs::exists(idiomsFile);

    if (!hasPrebuiltCorpus) {
        ResourceLoader loader(paths);
        loader.validateRawDataRoots();
        loader.writeRawManifest();
    }

    buildRuntimeCache(paths);
}

bool RuntimeBootstrap::loadRuntimeCache(
    const ProjectPaths& paths,
    std::vector<PhraseInfo>& poems,
    std::vector<PhraseInfo>& idioms,
    IndexStore& indexStore
) {
    if (!isRuntimeCacheFresh(paths)) {
        return false;
    }

    std::ifstream in(runtimeBinPath(paths), std::ios::binary);
    if (!in) {
        return false;
    }

    std::uint32_t version = 0;
    in.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != kRuntimeCacheVersion) {
        return false;
    }

    poems = readPhraseList(in);
    idioms = readPhraseList(in);

    // 这里要求你在 IndexStore 里补 loadSnapshot(in)
    indexStore.loadSnapshot(in);

    return true;
}

} // namespace lineverse::poetryrebuild