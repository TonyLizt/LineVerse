#pragma once

#include <filesystem>
#include <vector>

#include "../common/Json.h"
#include "../config/ProjectPaths.h"

namespace lineverse::poetryrebuild {

class ResourceLoader {
public:
    explicit ResourceLoader(ProjectPaths paths);

    const ProjectPaths& paths() const;
    void validateRawDataRoots() const;
    void writeRawManifest() const;

    std::vector<std::filesystem::path> collectTangPoetryFiles() const;
    std::filesystem::path findIdiomJsonFile() const;

    std::vector<Json> loadTangPoemEntries() const;
    std::vector<Json> loadIdiomEntries() const;

private:
    ProjectPaths paths_;

    static Json readJsonFile(const std::filesystem::path& filePath);

    static bool isTangPoetryDataFile(const std::filesystem::path& filePath);
    static bool looksLikePoemEntry(const Json& item);
};

} // namespace lineverse::poetryrebuild