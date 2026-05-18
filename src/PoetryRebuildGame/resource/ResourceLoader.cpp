#include "ResourceLoader.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

namespace lineverse::poetryrebuild {

ResourceLoader::ResourceLoader(ProjectPaths paths)
    : paths_(std::move(paths)) {
}

const ProjectPaths& ResourceLoader::paths() const {
    return paths_;
}

void ResourceLoader::validateRawDataRoots() const {
    if (!fs::exists(paths_.tangPoetryRoot) || !fs::is_directory(paths_.tangPoetryRoot)) {
        throw std::runtime_error("未找到全唐诗原始目录: " + paths_.tangPoetryRoot.string());
    }
    if (!fs::exists(paths_.idiomRoot) || !fs::is_directory(paths_.idiomRoot)) {
        throw std::runtime_error("未找到成语原始目录: " + paths_.idiomRoot.string());
    }
}

void ResourceLoader::writeRawManifest() const {
    Json manifest;
    manifest["selectedSources"] = {
        {"poetry", paths_.tangPoetryRoot.string()},
        {"idiom", paths_.idiomRoot.string()}
    };

    Json tangFiles = Json::array();
    for (const auto& file : collectTangPoetryFiles()) {
        tangFiles.push_back(file.string());
    }
    manifest["tangPoetryFiles"] = tangFiles;
    manifest["idiomFile"] = findIdiomJsonFile().string();

    std::ofstream out(paths_.moduleCacheRoot / "raw_scan_manifest.json", std::ios::binary);
    out << manifest.dump(2, ' ', true);
}

bool ResourceLoader::isTangPoetryDataFile(const fs::path& filePath) {
    if (!filePath.has_filename() || filePath.extension() != ".json") {
        return false;
    }

    const std::string filename = filePath.filename().string();

    // 排除作者信息、说明文件、非诗数据辅助文件
    if (filename.rfind("authors.", 0) == 0) {
        return false;
    }
    if (filename == "README.md") {
        return false;
    }
    if (filename == "表面结构字.json") {
        return false;
    }

    // 优先接受真正的唐诗数据文件
    if (filename.rfind("poet.tang.", 0) == 0) {
        return true;
    }

    // 额外补充常见唐诗数据文件
    if (filename == "唐诗三百首.json" || filename == "唐诗补录.json") {
        return true;
    }

    return false;
}

bool ResourceLoader::looksLikePoemEntry(const Json& item) {
    return item.is_object()
        && item.contains("author")
        && item["author"].is_string()
        && item.contains("title")
        && item["title"].is_string()
        && item.contains("paragraphs")
        && item["paragraphs"].is_array();
}

std::vector<fs::path> ResourceLoader::collectTangPoetryFiles() const {
    std::vector<fs::path> files;

    for (const auto& entry : fs::recursive_directory_iterator(paths_.tangPoetryRoot)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const auto& path = entry.path();
        if (!isTangPoetryDataFile(path)) {
            continue;
        }

        files.push_back(path);
    }

    std::sort(files.begin(), files.end());
    return files;
}

fs::path ResourceLoader::findIdiomJsonFile() const {
    fs::path fallback;

    for (const auto& entry : fs::recursive_directory_iterator(paths_.idiomRoot)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json") {
            continue;
        }

        const std::string filename = entry.path().filename().string();
        if (filename == "idiom.json") {
            return entry.path();
        }
        if (fallback.empty()) {
            fallback = entry.path();
        }
    }

    if (fallback.empty()) {
        throw std::runtime_error("未找到 idiom-database 的 JSON 文件。");
    }
    return fallback;
}

std::vector<Json> ResourceLoader::loadTangPoemEntries() const {
    std::vector<Json> entries;

    for (const auto& file : collectTangPoetryFiles()) {
        Json data = readJsonFile(file);
        if (!data.is_array()) {
            continue;
        }

        for (auto& item : data) {
            if (!looksLikePoemEntry(item)) {
                continue;
            }

            item["_source_file"] = fs::relative(file, paths_.projectRoot).generic_string();
            entries.push_back(std::move(item));
        }
    }

    return entries;
}

std::vector<Json> ResourceLoader::loadIdiomEntries() const {
    std::vector<Json> entries;
    const fs::path idiomFile = findIdiomJsonFile();
    Json data = readJsonFile(idiomFile);

    if (!data.is_array()) {
        return entries;
    }

    for (auto& item : data) {
        if (!item.is_object()) {
            continue;
        }
        item["_source_file"] = fs::relative(idiomFile, paths_.projectRoot).generic_string();
        entries.push_back(std::move(item));
    }

    return entries;
}

Json ResourceLoader::readJsonFile(const fs::path& filePath) {
    std::ifstream input(filePath, std::ios::binary);
    if (!input) {
        throw std::runtime_error("无法打开 JSON 文件: " + filePath.string());
    }

    Json data;
    input >> data;
    return data;
}

} // namespace lineverse::poetryrebuild