#pragma once

#include <string>
#include <vector>

#include "../data/model/Poem.h"

enum class QueryField {
    Dynasty,
    Author,
    Type,
    Emotion,
    Background,
    Feature,
    Imagery,
    Prefix,
    Title,
    Content,
    Structure
};

struct QueryTokenHit {
    QueryField field;
    std::string value;
    double weight = 1.0;
};

struct DescriptionQuery {
    std::string rawInput;
    std::string normalizedInput;
    std::vector<QueryTokenHit> hits;
    std::vector<std::string> freeTokens;
};

class PoetryDescriptionParser {
public:
    DescriptionQuery parse(const std::string& input, const std::vector<Poem>& poems) const;

private:
    static std::vector<std::string> splitTokens(const std::string& input);
    static std::string cleanupSemanticToken(const std::string& token);
    static std::string normalizeSynonym(const std::string& token);
    static bool containsEitherNormalized(const std::string& a, const std::string& b);
    static std::string tryParseStructure(const std::string& input);
    static void addUniqueHit(std::vector<QueryTokenHit>& hits, QueryField field, const std::string& value, double weight);
};
