#ifndef TREE_UTILS_H
#define TREE_UTILS_H

#include <print>
#include <string>

template <typename Node>
void destroy(Node* node) noexcept {
    if (!node)
        return;

    destroy(node->m_pChild[0]);
    destroy(node->m_pChild[1]);
    delete node;
}

template <typename Node, typename Printer>
void visualize(const Node* node, const std::string& prefix, bool isLast, bool isRoot, const Printer& printer)
{
    if (!node)
        return;

    if (isRoot) {
        printer(node);
        std::println();
    }
    else {
        std::print("{}{}", prefix, isLast ? "└── " : "├── ");
        printer(node);
        std::println();
    }

    const auto nextPrefix =
        prefix + (isRoot ? "" : (isLast ? "    " : "│   "));

    if (node->m_pChild[1])
        visualize(node->m_pChild[1], nextPrefix, !node->m_pChild[0], false, printer);

    if (node->m_pChild[0])
        visualize(node->m_pChild[0], nextPrefix, true, false, printer);
}
#endif // TREE_UTILS_H