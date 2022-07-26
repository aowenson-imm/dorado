#include "utils/math_utils.h"

#include <catch2/catch.hpp>

#define CUT_TAG "[MathUtils]"

TEST_CASE(CUT_TAG ": test quantiles", CUT_TAG) {
    SECTION("Empty input") {
        auto quartiles = ::utils::quantiles(std::vector<double>(), {0.25, 0.5, 0.75});
        CHECK(quartiles.empty());
    }

    SECTION("Empty quantiles") {
        std::vector<double> in = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
        auto quartiles = ::utils::quantiles(in, {});
        CHECK(quartiles.empty());
    }

    SECTION("Test some simple quantiles") {
        std::vector<double> in = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
        auto quartiles = ::utils::quantiles(in, {0.25, 0.5, 0.75});

        std::vector<double> expected_values{3.5, 6.0, 8.5};
        REQUIRE(quartiles.size() == expected_values.size());
        for (size_t i = 0; i < quartiles.size(); ++i) {
            CHECK(quartiles[i] == Approx(expected_values[i]));
        }
    }
}

TEST_CASE(CUT_TAG ": test linear_regression", CUT_TAG) {
    std::vector<double> x = {1, 2, 4, 5, 10, 20};
    std::vector<double> y = {4, 6, 12, 15, 34, 68};

    auto [m, b, r] = ::utils::linear_regression(x, y);

    double expected_m = 3.43651, expected_b = -0.888889, expected_r = 0.999192;
    CHECK(m == Approx(expected_m));
    CHECK(b == Approx(expected_b));
    CHECK(r == Approx(expected_r));
}