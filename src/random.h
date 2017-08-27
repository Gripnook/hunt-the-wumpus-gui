#pragma once

#include <random>
#include <algorithm>

namespace wumpus {

// Returns a random integer in the range [lower, upper).
int random(int lower, int upper)
{
    static std::default_random_engine rng_engine{std::random_device()()};
    std::uniform_int_distribution<int> rng_dist(lower, upper - 1);
    return rng_dist(rng_engine);
}

// Returns a random integer in the range [lower, upper) that satisfies the given
// predicate. UnaryPredicate must be a predicate acting on an integer.
template <typename UnaryPredicate>
int random_if(int lower, int upper, UnaryPredicate pred)
{
    int result = random(lower, upper);
    while (!pred(result))
        result = random(lower, upper);
    return result;
}

// Returns a random integer in the range [lower, upper) that is not in the
// excludes. Container must be a container of integers.
template <typename Container>
int random(int lower, int upper, const Container& excludes)
{
    auto begin = std::begin(excludes);
    auto end = std::end(excludes);
    return random_if(lower, upper, [begin, end](int x) {
        return std::find(begin, end, x) == end;
    });
}
}
