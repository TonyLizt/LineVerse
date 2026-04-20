#pragma once

#include <string>

class EditDistance {
public:
    static int levenshtein(const std::string& a, const std::string& b);
    static double similarity(const std::string& a, const std::string& b);
};