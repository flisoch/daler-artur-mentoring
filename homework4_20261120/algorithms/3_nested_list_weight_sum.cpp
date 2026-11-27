/*
339. Nested List Weight Sum
Difficulty: Medium

You are given a nested list of integers nestedList.
Each element is either an integer or a list whose
elements may also be integers or other lists.

The depth of an integer is the number of lists that
it is inside of. For example, the nested list
[1,[2,2],[[3],2],1] has each integer's value set
to its depth.

Return the sum of each integer in nestedList
multiplied by its depth.


Example 1:

Input: nestedList = [[1,1],2,[1,1]]
Output: 10
Explanation: Four 1's at depth 2, one 2 at depth 1.
1*2 + 1*2 + 2*1 + 1*2 + 1*2 = 10.


Example 2:

Input: nestedList = [1,[4,[6]]]
Output: 27
Explanation: One 1 at depth 1, one 4 at depth 2,
and one 6 at depth 3.
1*1 + 4*2 + 6*3 = 27.


Example 3:

Input: nestedList = [0]
Output: 0


Constraints:

- 1 <= nestedList.length <= 50
- The values of the integers in the nested list
  is in the range [-100, 100].
- The maximum depth of any integer is less than
  or equal to 50.
*/


/*
339. Nested List Weight Sum
Difficulty: Medium

You are given a nested list of integers nestedList.
Each element is either an integer or a list whose
elements may also be integers or other lists.

The depth of an integer is the number of lists that
it is inside of. For example, the nested list
[1,[2,2],[[3],2],1] has each integer's value set
to its depth.

Return the sum of each integer in nestedList
multiplied by its depth.

Example 1:
Input: nestedList = [[1,1],2,[1,1]]
Output: 10

Example 2:
Input: nestedList = [1,[4,[6]]]
Output: 27

Example 3:
Input: nestedList = [0]
Output: 0

Constraints:
- 1 <= nestedList.length <= 50
- -100 <= integer value <= 100
- The maximum depth of any integer is <= 50.

Compilation:
% clang++ 3_nested_list_weight_sum.cpp -O3 -std=c++23
% ./a.out
*/

#include <cstddef>
#include <functional>
#include <iostream>
#include <vector>
#include <variant>
#include <initializer_list>
#include <queue>
#include <cstdint>

class NestedInteger
{
public:
    // Constructs an empty nested list.
    NestedInteger()
        : data_(std::vector<NestedInteger>{})
    {}

    // Constructs a single integer.
    NestedInteger(int value)
        : data_(value)
    {}

    // Additional constructor for convenient test creation.
    NestedInteger(std::initializer_list<NestedInteger> list)
        : data_(std::vector<NestedInteger>(list))
    {}

    // Returns true if this object holds a single integer.
    bool isInteger() const
    {
        return std::holds_alternative<int>(data_);
    }

    // Returns the integer held by this object.
    int getInteger() const
    {
        return std::get<int>(data_);
    }

    // Sets this object to hold a single integer.
    void setInteger(int value)
    {
        data_ = value;
    }

    // Sets this object to hold a nested list and adds
    // a nested integer to it.
    void add(const NestedInteger& ni)
    {
        if (isInteger())
        {
            data_ = std::vector<NestedInteger>{};
        }

        std::get<std::vector<NestedInteger>>(data_)
            .push_back(ni);
    }

    // Returns the nested list held by this object.
    const std::vector<NestedInteger>& getList() const
    {
        return std::get<std::vector<NestedInteger>>(data_);
    }

private:
    std::variant<int, std::vector<NestedInteger>> data_;
};

class Solution
{
public:
    int depthSum(std::vector<NestedInteger>& nestedList)
    {
        int answer = 0;

        if (nestedList.empty())
        {
            return answer;
        }

        using TLevel = int;
        std::queue<std::pair<std::reference_wrapper<const NestedInteger>, TLevel>> q;

        size_t initial_level = 1;

        for (size_t i = 0; i < nestedList.size(); ++i)
        {
            q.emplace(nestedList[i], initial_level);
        }

        while (!q.empty())
        {
            auto& [nestedInteger, level] = q.front();
            
            if (nestedInteger.get().isInteger())
            {
                answer += nestedInteger.get().getInteger() * level;
            }
            else 
            {
                const auto& list = nestedInteger.get().getList();

                for (size_t i = 0; i < list.size(); ++i)
                {
                    q.emplace(list[i], level + 1);
                }
            }

            q.pop();
        }

        return answer;
    }
};

void runTest(
    Solution& solution,
    std::vector<NestedInteger>& nestedList,
    int expected,
    int testNumber)
{
    const auto result = solution.depthSum(nestedList);

    std::cout << "Example " << testNumber << ":\n";
    std::cout << "Result:   " << result << "\n";
    std::cout << "Expected: " << expected << "\n";
    std::cout << (result == expected ? "PASSED" : "FAILED")
              << "\n\n";
}

int main()
{
    Solution solution;

    // Example 1
    // [[1,1],2,[1,1]]
    // 1*2 + 1*2 + 2*1 + 1*2 + 1*2 = 10

    std::vector<NestedInteger> list1 = {
        {1, 1},
        2,
        {1, 1}
    };

    // Example 2
    // [1,[4,[6]]]
    // 1*1 + 4*2 + 6*3 = 27

    std::vector<NestedInteger> list2 = {
        1,
        {4, {6}}
    };

    // Example 3
    // [0]
    // 0*1 = 0

    std::vector<NestedInteger> list3 = {
        0
    };

    // Example 4: Flat list
    // [1,2,3,4,5]
    // 1 + 2 + 3 + 4 + 5 = 15

    std::vector<NestedInteger> list4 = {
        1, 2, 3, 4, 5
    };

    // Example 5: Negative integers
    // [-1,[2,[-3]],4]
    // -1*1 + 2*2 + (-3)*3 + 4*1 = -2

    std::vector<NestedInteger> list5 = {
        -1,
        {2, {-3}},
        4
    };

    // Example 6: Deeply nested list
    // [[[[5]]]]
    // 5*4 = 20

    NestedInteger inner;
    inner.add(5);

    NestedInteger middle;
    middle.add(inner);

    NestedInteger outer;
    outer.add(middle);

    std::vector<NestedInteger> list6 = {outer};

    // Example 7: Empty nested lists
    // [[],1,[[],2]]
    // 1*1 + 2*2 = 5

    std::vector<NestedInteger> list7 = {
        NestedInteger(),
        1,
        {NestedInteger(), 2}
    };

    // Example 8: Multiple integers at different depths
    // [1,[2,3],[4,[5,6]]]
    // 1*1 + 2*2 + 3*2 + 4*2 + 5*3 + 6*3 = 52

    std::vector<NestedInteger> list8 = {
        1,
        {2, 3},
        {4, {5, 6}}
    };

    runTest(solution, list1, 10, 1);
    runTest(solution, list2, 27, 2);
    runTest(solution, list3, 0, 3);
    runTest(solution, list4, 15, 4);
    runTest(solution, list5, -2, 5);
    runTest(solution, list6, 20, 6);
    runTest(solution, list7, 5, 7);
    runTest(solution, list8, 52, 8);

    return 0;
}
