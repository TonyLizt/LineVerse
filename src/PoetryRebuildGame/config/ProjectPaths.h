#pragma once

#include <filesystem>

namespace lineverse::poetryrebuild {

struct ProjectPaths {
    std::filesystem::path projectRoot;
    std::filesystem::path moduleRoot;
    std::filesystem::path rawRoot;
    std::filesystem::path cacheRoot;
    std::filesystem::path prebuildRoot;
    std::filesystem::path tangPoetryRoot;
    std::filesystem::path idiomRoot;
    std::filesystem::path moduleCacheRoot;
    std::filesystem::path modulePrebuildRoot;

    static ProjectPaths detect();
    void ensureOutputDirectories() const;
};

} // namespace lineverse::poetryrebuild
