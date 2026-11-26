/*
314. Binary Tree Vertical Order Traversal
Difficulty: Medium

Given the root of a binary tree, return the vertical order traversal of its nodes' values (i.e., from top to bottom, column by column).

If two nodes are in the same row and column, the order should be from left to right.

Input: root = [3,9,20,null,null,15,7]
Output: [[9],[3,15],[20],[7]]

Input: root = [3,9,8,4,0,1,7]
Output: [[4],[9],[3,0,1],[8],[7]]

Input: root = [1,2,3,4,10,9,11,null,5,null,null,null,null,null,null,null,6]
Output: [[4],[2,5],[1,10,9,6],[3],[11]]

% clang++ 2_vertical_traversal.cpp -O3 -std=c++23
% ./a.out                                        
Example 1:
[[9],[3,15],[20],[7]]
Expected: [[9],[3,15],[20],[7]]
Example 2:
[[4],[9],[3,0,1],[8],[7]]
Expected: [[4],[9],[3,0,1],[8],[7]]
Example 3:
[[4],[2,5],[1,10,9,6],[3],[11]]
Expected: [[4],[2,5],[1,10,9,6],[3],[11]]
*/

#include <cstdint>
#include <queue>
#include <unordered_map>
#include <vector>
#include <iostream>

struct TreeNode
{
    int val;
    TreeNode* left;
    TreeNode* right;

    TreeNode() : val(0), left(nullptr), right(nullptr) {}

    TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}

    TreeNode(int x, TreeNode* left, TreeNode* right)
        : val(x), left(left), right(right) {}
};

class Solution {
public:
    using TColumnIndex = int64_t;

    std::vector<std::vector<int>> verticalOrder(TreeNode* root) {
        std::queue<std::pair<TreeNode*, TColumnIndex>> q;
        std::unordered_map<TColumnIndex, std::vector<int>> m;
        std::vector<std::vector<int>> answer;

        if (!root)
        {
            return {};
        }

        q.emplace(root, 0);
        int64_t min_index = 0;
        int64_t max_index = 0;
        
        while(!q.empty())
        {
            auto [node, index] = q.front();
            
            m[index].emplace_back(node->val);

            if (node->left)
            {
                auto left_index = index - 1;
                min_index = std::min(min_index, left_index);
                q.emplace(node->left, left_index);
            }

            if (node->right)
            {
                auto right_index = index + 1;
                max_index = std::max(max_index, right_index);
                q.emplace(node->right, right_index);
            }

            q.pop();
        }

        for (int64_t i = min_index; i <= max_index; ++i)
        {
            answer.emplace_back(std::move(m[i]));
        }

        return answer;
    }
};

void print(const std::vector<std::vector<int>>& result)
{
    std::cout << "[";

    for (size_t i = 0; i < result.size(); ++i)
    {
        std::cout << "[";

        for (size_t j = 0; j < result[i].size(); ++j)
        {
            std::cout << result[i][j];

            if (j + 1 < result[i].size())
            {
                std::cout << ",";
            }
        }

        std::cout << "]";

        if (i + 1 < result.size())
        {
            std::cout << ",";
        }
    }

    std::cout << "]\n";
}

void deleteTree(TreeNode* root)
{
    if (!root)
    {
        return;
    }

    deleteTree(root->left);
    deleteTree(root->right);

    delete root;
}

int main()
{
    Solution solution;

    // Example 1
    //
    //       3
    //      / \
    //     9   20
    //        /  \
    //       15   7

    auto* root1 = new TreeNode(3,
        new TreeNode(9),
        new TreeNode(20,
            new TreeNode(15),
            new TreeNode(7)
        )
    );

    // Example 2
    //
    //        3
    //      /   \
    //     9     8
    //    / \   / \
    //   4   0 1   7

    auto* root2 = new TreeNode(3,
        new TreeNode(9,
            new TreeNode(4),
            new TreeNode(0)
        ),
        new TreeNode(8,
            new TreeNode(1),
            new TreeNode(7)
        )
    );

    // Example 3
    //
    //          1
    //        /   \
    //       2     3
    //      / \   / \
    //     4  10 9   11
    //      \
    //       5
    //        \
    //         6

    auto* root3 = new TreeNode(1,
        new TreeNode(2,
            new TreeNode(4,
                nullptr,
                new TreeNode(5,
                    nullptr,
                    new TreeNode(6)
                )
            ),
            new TreeNode(10)
        ),
        new TreeNode(3,
            new TreeNode(9),
            new TreeNode(11)
        )
    );

    std::cout << "Example 1:\n";
    print(solution.verticalOrder(root1));
    std::cout << "Expected: [[9],[3,15],[20],[7]]\n";

    std::cout << "Example 2:\n";
    print(solution.verticalOrder(root2));
    std::cout << "Expected: [[4],[9],[3,0,1],[8],[7]]\n";

    std::cout << "Example 3:\n";
    print(solution.verticalOrder(root3));
    std::cout << "Expected: [[4],[2,5],[1,10,9,6],[3],[11]]\n";

    deleteTree(root1);
    deleteTree(root2);
    deleteTree(root3);

    return 0;
}