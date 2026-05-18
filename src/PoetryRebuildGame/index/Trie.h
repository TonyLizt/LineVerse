#pragma once

#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../common/Types.h"

namespace lineverse::poetryrebuild {

class Trie {
public:
    void insert(const std::string& text);
    bool contains(const std::string& text) const;
    bool isValidPrefix(const std::string& prefix) const;
    std::vector<std::string> complete(const std::string& prefix, std::size_t limit = 10) const;
    std::size_t size() const;
    void clear();

    void saveSnapshot(std::ostream& out) const;
    void loadSnapshot(std::istream& in);

private:
    struct Node {
        bool isTerminal = false;
        std::unordered_map<CodePoint, std::unique_ptr<Node>> children;
    };

    Node root_;
    std::size_t size_ = 0;

    const Node* findNode(const std::string& text) const;
    static void collect(const Node* node, CodePointList& path, std::vector<std::string>& out, std::size_t limit);

    static void saveNode(std::ostream& out, const Node& node);
    static void loadNode(std::istream& in, Node& node);
};

} // namespace lineverse::poetryrebuild