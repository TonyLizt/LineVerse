#pragma once

#include <string>
#include <vector>
#include "model/Poem.h"

class PoetryRepository {
public:
    bool loadFromJson(const std::string& filePath);

    const std::vector<Poem>& getAllPoems() const;
    const Poem* getPoemById(int id) const;

private:
    std::vector<Poem> poems;

    static std::vector<std::string> splitPoemIntoLines(const std::string& content);
    static std::string trim(const std::string& s);
    static void replaceAll(std::string& s, const std::string& from, const std::string& to);
};