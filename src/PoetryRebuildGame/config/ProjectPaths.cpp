#include "ProjectPaths.h"

#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace lineverse::poetryrebuild {

namespace {

bool looksLikeProjectRoot(const fs::path& path) {
    return fs::exists(path / "src") && fs::exists(path / "data");
}

fs::path getExecutableDir() {
#ifdef _WIN32
    std::wstring buffer(MAX_PATH, L'\0');
    DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    while (length == buffer.size()) {
        buffer.resize(buffer.size() * 2);
        length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    }
    if (length == 0) {
        return {};
    }
    buffer.resize(length);
    return fs::path(buffer).parent_path();
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    if (size == 0) {
        return {};
    }
    std::string buffer(size, '\0');
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
        return {};
    }
    return fs::path(buffer.c_str()).parent_path();
#elif defined(__linux__)
    std::vector<char> buffer(4096, '\0');
    const ssize_t length = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (length <= 0) {
        return {};
    }
    buffer[static_cast<std::size_t>(length)] = '\0';
    return fs::path(buffer.data()).parent_path();
#else
    return {};
#endif
}

void pushIfUnique(std::vector<fs::path>& roots, const fs::path& value) {
    if (value.empty()) {
        return;
    }

    std::error_code ec;
    fs::path normalized = fs::weakly_canonical(value, ec);
    if (ec) {
        normalized = value;
    }

    for (const auto& item : roots) {
        if (item == normalized) {
            return;
        }
    }
    roots.push_back(normalized);
}

std::vector<fs::path> buildSearchRoots() {
    std::vector<fs::path> roots;
    pushIfUnique(roots, fs::current_path());
    pushIfUnique(roots, getExecutableDir());
    return roots;
}

} // namespace

ProjectPaths ProjectPaths::detect() {
    const auto searchRoots = buildSearchRoots();

    for (auto current : searchRoots) {
        while (!current.empty()) {
            if (looksLikeProjectRoot(current)) {
                ProjectPaths paths;
                paths.projectRoot = current;
                paths.moduleRoot = current / "src" / "PoetryRebuildGame";
                paths.rawRoot = current / "data" / "raw";
                paths.cacheRoot = current / "data" / "cache";
                paths.prebuildRoot = current / "data" / "prebuild";
                paths.tangPoetryRoot = paths.rawRoot / "chinese-poetry-master" / "chinese-poetry-master" / "全唐诗";
                paths.idiomRoot = paths.rawRoot / "idiom-database-master" / "idiom-database-master" / "data";
                paths.moduleCacheRoot = paths.cacheRoot / "PoetryRebuildGame";
                paths.modulePrebuildRoot = paths.prebuildRoot / "PoetryRebuildGame";
                return paths;
            }

            const fs::path parent = current.parent_path();
            if (parent == current) {
                break;
            }
            current = parent;
        }
    }

    std::string rootsText;
    for (std::size_t i = 0; i < searchRoots.size(); ++i) {
        if (i > 0) {
            rootsText += " ; ";
        }
        rootsText += searchRoots[i].string();
    }

    throw std::runtime_error(
        "无法自动定位 LineVerse 项目根目录，请从项目根目录或其子目录启动程序。"
        " 已尝试的起点: " + rootsText
    );
}

void ProjectPaths::ensureOutputDirectories() const {
    fs::create_directories(moduleCacheRoot);
    fs::create_directories(modulePrebuildRoot);
}

} // namespace lineverse::poetryrebuild
