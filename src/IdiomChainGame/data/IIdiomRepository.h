#ifndef IDIOM_CHAIN_GAME_I_IDIOM_REPOSITORY_H
#define IDIOM_CHAIN_GAME_I_IDIOM_REPOSITORY_H

#include "../core/IdiomEntry.h"

#include <string>
#include <vector>

/**
 * @brief Repository interface for idiom data access.
 */
class IIdiomRepository {
public:
    virtual ~IIdiomRepository() = default;

    virtual bool load() = 0;
    virtual const IdiomEntry* findByWord(const std::string& wordOrAbbreviation) const = 0;
    virtual const std::vector<IdiomEntry>& getAll() const = 0;
};

#endif
