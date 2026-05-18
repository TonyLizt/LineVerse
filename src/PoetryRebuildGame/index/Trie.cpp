#include "Trie.h"

#include <cstdint>
#include <istream>
#include <ostream>

#include "../util/Utf8.h"

namespace lineverse::poetryrebuild {

void Trie::insert(const std::string& text) {
    Node* node = &root_;
    for (CodePoint cp : Utf8::toCodePoints(text)) {
        auto& next = node->children[cp];
        if (!next) {
            next = std::make_unique<Node>();
        }
        node = next.get();
    }

    if (!node->isTerminal) {
        node->isTerminal = true;
        ++size_;
    }
}

bool Trie::contains(const std::string& text) const {
    const Node* node = findNode(text);
    return node != nullptr && node->isTerminal;
}

bool Trie::isValidPrefix(const std::string& prefix) const {
    return findNode(prefix) != nullptr;
}

std::vector<std::string> Trie::complete(const std::string& prefix, std::size_t limit) const {
    std::vector<std::string> result;
    const Node* node = findNode(prefix);
    if (node == nullptr || limit == 0) {
        return result;
    }

    CodePointList path = Utf8::toCodePoints(prefix);
    if (node->isTerminal) {
        result.push_back(prefix);
        if (result.size() >= limit) {
            return result;
        }
    }

    collect(node, path, result, limit);
    return result;
}

std::size_t Trie::size() const {
    return size_;
}

void Trie::clear() {
    root_.children.clear();
    root_.isTerminal = false;
    size_ = 0;
}

void Trie::saveSnapshot(std::ostream& out) const {
    const std::uint64_t savedSize = static_cast<std::uint64_t>(size_);
    out.write(reinterpret_cast<const char*>(&savedSize), sizeof(savedSize));
    saveNode(out, root_);
}

void Trie::loadSnapshot(std::istream& in) {
    clear();

    std::uint64_t savedSize = 0;
    in.read(reinterpret_cast<char*>(&savedSize), sizeof(savedSize));

    loadNode(in, root_);
    size_ = static_cast<std::size_t>(savedSize);
}

void Trie::saveNode(std::ostream& out, const Node& node) {
    const std::uint8_t terminal = node.isTerminal ? 1 : 0;
    out.write(reinterpret_cast<const char*>(&terminal), sizeof(terminal));

    const std::uint64_t childCount = static_cast<std::uint64_t>(node.children.size());
    out.write(reinterpret_cast<const char*>(&childCount), sizeof(childCount));

    for (const auto& [cp, child] : node.children) {
        const std::int32_t codepoint = static_cast<std::int32_t>(cp);
        out.write(reinterpret_cast<const char*>(&codepoint), sizeof(codepoint));
        saveNode(out, *child);
    }
}

void Trie::loadNode(std::istream& in, Node& node) {
    node.children.clear();
    node.isTerminal = false;

    std::uint8_t terminal = 0;
    in.read(reinterpret_cast<char*>(&terminal), sizeof(terminal));
    node.isTerminal = (terminal != 0);

    std::uint64_t childCount = 0;
    in.read(reinterpret_cast<char*>(&childCount), sizeof(childCount));

    for (std::uint64_t i = 0; i < childCount; ++i) {
        std::int32_t codepoint = 0;
        in.read(reinterpret_cast<char*>(&codepoint), sizeof(codepoint));

        auto child = std::make_unique<Node>();
        loadNode(in, *child);
        node.children[static_cast<CodePoint>(codepoint)] = std::move(child);
    }
}

const Trie::Node* Trie::findNode(const std::string& text) const {
    const Node* node = &root_;
    for (CodePoint cp : Utf8::toCodePoints(text)) {
        const auto it = node->children.find(cp);
        if (it == node->children.end()) {
            return nullptr;
        }
        node = it->second.get();
    }
    return node;
}

void Trie::collect(const Node* node, CodePointList& path, std::vector<std::string>& out, std::size_t limit) {
    if (node == nullptr || out.size() >= limit) {
        return;
    }

    for (const auto& [cp, child] : node->children) {
        path.push_back(cp);
        if (child->isTerminal) {
            out.push_back(Utf8::fromCodePoints(path));
            if (out.size() >= limit) {
                path.pop_back();
                return;
            }
        }
        collect(child.get(), path, out, limit);
        path.pop_back();
        if (out.size() >= limit) {
            return;
        }
    }
}

} // namespace lineverse::poetryrebuild