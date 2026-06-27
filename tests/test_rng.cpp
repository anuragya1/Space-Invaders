#include "test_common.h"
#include "../src/core/rng.h"

#include <vector>

int main() {
    using si::RNG;

    RNG a(42), b(42);
    for (int i = 0; i < 1000; ++i)
        CHECK_EQ(a.next(), b.next());

    RNG c(1), d(2);
    CHECK(c.next() != d.next());

    RNG e(7);
    std::vector<std::uint32_t> first;
    for (int i = 0; i < 100; ++i) first.push_back(e.next());
    e.reseed(7);
    for (int i = 0; i < 100; ++i) CHECK_EQ(e.next(), first[i]);

    RNG f(99);
    for (int i = 0; i < 10000; ++i) {
        int v = f.range(-5, 5);
        CHECK(v >= -5 && v <= 5);
    }

    return test_summary("test_rng");
}
