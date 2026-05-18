#pragma once

#include <vector>

#include "../common/Types.h"
#include "../config/ProjectPaths.h"
#include "../index/IndexStore.h"

namespace lineverse::poetryrebuild {

class RuntimeBootstrap {
public:
    static void prepare();

    static bool loadRuntimeCache(
        const ProjectPaths& paths,
        std::vector<PhraseInfo>& poems,
        std::vector<PhraseInfo>& idioms,
        IndexStore& indexStore
    );
};

} // namespace lineverse::poetryrebuild