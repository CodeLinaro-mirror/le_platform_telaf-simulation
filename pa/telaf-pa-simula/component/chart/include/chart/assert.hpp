// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

// chart/assert.hpp — runtime invariant macros.
//
// Tiny ASSERT / REQUIRE / ENSURE family for runtime invariants.
// Disabled when NDEBUG is defined so release builds pay nothing.

#ifndef CHART_ASSERT_HPP
#define CHART_ASSERT_HPP

#include <cstdio>
#include <cstdlib>

#ifndef NDEBUG
#define CHART_ASSERT(cond)                                                  \
    do {                                                                    \
        if (!(cond)) {                                                      \
            std::fprintf(stderr, "[chart] assertion failed: %s @ %s:%d\n",  \
                         #cond, __FILE__, __LINE__);                        \
            std::abort();                                                   \
        }                                                                   \
    } while (0)
#else
#define CHART_ASSERT(cond) ((void)0)
#endif

// Aliases — same behavior, but communicate intent (preconditions vs.
// postconditions). Common Design-by-Contract idiom.
#define CHART_REQUIRE(cond) CHART_ASSERT(cond)
#define CHART_ENSURE(cond)  CHART_ASSERT(cond)

#endif  // CHART_ASSERT_HPP
