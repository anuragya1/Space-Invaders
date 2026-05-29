// test_common.h - tiny assertion helpers for the tests.
//
// Not pulling in gtest/Catch2 for a college submission keeps the build
// dependency-free and the test artifacts inspectable.
#pragma once

#include <cstdio>
#include <cstdlib>
#include <string>

inline int& fail_count() { static int n = 0; return n; }
inline int& pass_count() { static int n = 0; return n; }

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (cond) { ++pass_count(); }                                     \
        else {                                                            \
            ++fail_count();                                               \
            std::fprintf(stderr, "  FAIL %s:%d  %s\n",                    \
                         __FILE__, __LINE__, #cond);                      \
        }                                                                 \
    } while (0)

#define CHECK_EQ(a, b)                                                    \
    do {                                                                  \
        auto _av = (a); auto _bv = (b);                                   \
        if (_av == _bv) { ++pass_count(); }                               \
        else {                                                            \
            ++fail_count();                                               \
            std::fprintf(stderr, "  FAIL %s:%d  " #a " == " #b "\n",      \
                         __FILE__, __LINE__);                             \
        }                                                                 \
    } while (0)

inline int test_summary(const char* name) {
    std::fprintf(stdout, "%s: %d passed, %d failed\n",
                 name, pass_count(), fail_count());
    return fail_count() == 0 ? 0 : 1;
}
