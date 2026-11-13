#ifndef TREE_UTILS_H
#define TREE_UTILS_H

#include <print>
#include <string>
#include <unordered_map>

namespace treeUtils
{
    template <typename Node>
    void destroy(Node* node) noexcept {
        if (!node)
            return;

        destroy(node->m_pChild[0]);
        destroy(node->m_pChild[1]);
        delete node;
    }

    template <typename Node, typename Labeler>
    void visualize(const Node* root, const Labeler& labeler) {
        if (!root)
            return;

        struct Entry {
            std::string label;
            size_t depth;
            int y;
        };

        std::unordered_map<const Node*, Entry> entries;
        std::vector<size_t> maxWidths;

        int nextY = 0;
        int minY = 0;
        int maxY = 0;
        size_t maxDepth = 0;

        auto layout = [&](auto&& self,
            const Node* node,
            size_t depth) -> int
            {
                if (!node)
                    return 0;

                maxDepth = std::max(maxDepth, depth);

                if (maxWidths.size() <= depth)
                    maxWidths.resize(depth + 1, 0);

                const auto right = node->m_pChild[1];
                const auto left = node->m_pChild[0];

                int rightY = 0;
                int leftY = 0;

                if (right)
                    rightY = self(self, right, depth + 1);

                if (left)
                    leftY = self(self, left, depth + 1);

                int y;

                if (right && left) {
                    y = (rightY + leftY) / 2;
                }
                else if (right) {
                    y = rightY + 1;
                }
                else if (left) {
                    y = leftY - 1;
                }
                else {
                    y = nextY;
                    nextY += 2;
                }

                std::string label = labeler(node);

                maxWidths[depth] = std::max(
                    maxWidths[depth],
                    label.size()
                );

                entries.emplace(
                    node,
                    Entry{ std::move(label), depth, y }
                );

                minY = std::min(minY, y);
                maxY = std::max(maxY, y);

                return y;
            };

        layout(layout, root, 0);

        std::vector<size_t> xPositions(maxDepth + 1);

        for (size_t i = 1; i <= maxDepth; ++i) {
            xPositions[i] =
                xPositions[i - 1] + maxWidths[i - 1] + 8;
        }

        const size_t columns =
            xPositions[maxDepth] + maxWidths[maxDepth] + 8;

        const size_t rows =
            static_cast<size_t>(maxY - minY + 1);

        // Each cell contains one terminal character.
        std::vector<std::vector<std::string>> canvas(
            rows,
            std::vector<std::string>(columns, " ")
        );

        auto put = [&](int y, size_t x, const std::string& value) {
            const size_t row = static_cast<size_t>(y - minY);

            if (row < rows && x < columns)
                canvas[row][x] = value;
            };

        auto horizontal = [&](int y, size_t begin, size_t end) {
            for (size_t x = begin; x < end; ++x)
                put(y, x, "─");
            };

        for (const auto& [node, entry] : entries) {
            const auto right = node->m_pChild[1];
            const auto left = node->m_pChild[0];

            if (!right && !left)
                continue;

            const size_t x = xPositions[entry.depth];

            const size_t junctionX =
                x + maxWidths[entry.depth] + 4;

            const int parentY = entry.y;

            const auto rightEntry = right ? &entries.at(right) : nullptr;
            const auto leftEntry = left ? &entries.at(left) : nullptr;

            const int rightY = rightEntry ? rightEntry->y : parentY;
            const int leftY = leftEntry ? leftEntry->y : parentY;

            // Vertical connection.
            if (right && left) {
                const int top = std::min(rightY, leftY);
                const int bottom = std::max(rightY, leftY);

                for (int y = top; y <= bottom; ++y) {
                    if (y == parentY)
                        put(y, junctionX, "┤");
                    else if (y == rightY)
                        put(y, junctionX, "┌");
                    else if (y == leftY)
                        put(y, junctionX, "└");
                    else
                        put(y, junctionX, "│");
                }
            }
            else {
                const int childY = right ? rightY : leftY;

                const int top = std::min(parentY, childY);
                const int bottom = std::max(parentY, childY);

                for (int y = top; y <= bottom; ++y) {
                    if (y == parentY)
                        put(y, junctionX, "┤");
                    else if (right)
                        put(y, junctionX, "┌");
                    else
                        put(y, junctionX, "└");
                }
            }

            // Parent horizontal connection.
            horizontal(
                parentY,
                x + entry.label.size(),
                junctionX
            );

            // Child horizontal connections.
            const size_t childX = xPositions[entry.depth + 1];

            auto drawChild = [&](const Entry* child, bool isRight) {
                if (!child)
                    return;

                const int y = child->y;

                horizontal(y, junctionX + 1, childX - 2);

                put(y, junctionX, isRight ? "┌" : "└");
                };

            drawChild(rightEntry, true);
            drawChild(leftEntry, false);
        }

        for (const auto& [node, entry] : entries) {
            const size_t x = xPositions[entry.depth];

            for (size_t i = 0; i < entry.label.size(); ++i) {
                put(
                    entry.y,
                    x + i,
                    std::string(1, entry.label[i])
                );
            }
        }

        for (const auto& row : canvas) {
            size_t last = row.size();

            while (last > 0 && row[last - 1] == " ")
                --last;

            for (size_t i = 0; i < last; ++i)
                std::print("{}", row[i]);

            std::println();
        }
    }
}
#endif // TREE_UTILS_H