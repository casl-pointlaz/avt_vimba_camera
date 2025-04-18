#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "railcam/imgproc/pyramids.h"

using namespace railcam::imgproc;


struct ClampBoundaryPolicy {
    int operator()(int index, int size) const {
        return std::clamp(index, 0, size - 1);
    }
};

// --------------------------------------------------------------------------------------
// MirrorBoundaryPolicy
// --------------------------------------------------------------------------------------
TEST_CASE("MirrorBoundaryPolicy with size=4") {
    MirrorBoundaryPolicy mirror;
    const int sz = 4;
    struct { int idx, exp; } data[] = {
        {-4, 3}, {-3, 2}, {-2, 1}, {-1, 0}, // negative
         { 0, 0}, { 1, 1}, { 2, 2}, { 3, 3}, // in-range
         { 4, 3}, { 5, 2}, { 6, 1}, { 7, 0}  // out-of-range
    };
    for (auto& d : data) {
        REQUIRE(mirror(d.idx, sz) == d.exp);
    }
}

TEST_CASE("MirrorBoundaryPolicy with size=5") {
    MirrorBoundaryPolicy mirror;
    const int sz = 5;
    REQUIRE(mirror(-1, sz) == 0);
    REQUIRE(mirror(-2, sz) == 1);
    REQUIRE(mirror( 4, sz) == 4);
    REQUIRE(mirror( 5, sz) == 4);
    REQUIRE(mirror( 6, sz) == 3);
    REQUIRE(mirror( 7, sz) == 2);
}

// --------------------------------------------------------------------------------------
// cubicInterpolate
// --------------------------------------------------------------------------------------

TEST_CASE("cubicInterpolate handles multi-dimensional input", "[cubicInterpolate]") {
    Eigen::Array4f P0(0.f, 0.f, 0.f, 0.f);
    Eigen::Array4f P1(1.f, 1.f, 1.f, 1.f);
    Eigen::Array4f P2(2.f, 2.f, 2.f, 2.f);
    Eigen::Array4f P3(3.f, 3.f, 3.f, 3.f);

    float t = 0.5f;
    auto result = cubicInterpolate(P0, P1, P2, P3, t);

    // Ensure all dimensions interpolate correctly
    for (int i = 0; i < result.size(); ++i) {
        REQUIRE_THAT(result(i), Catch::Matchers::WithinAbs(1.5f, 1e-5f));
    }
}


TEST_CASE("cubicInterpolate produces correct interpolations", "[cubicInterpolate]") {
    // Control points as vectors
    Eigen::Array3f P0(0.0f, 1.0f, 2.0f);
    Eigen::Array3f P1(1.0f, 2.0f, 3.0f);
    Eigen::Array3f P2(4.0f, 5.0f, 6.0f);
    Eigen::Array3f P3(9.0f, 10.0f, 11.0f);

    SECTION("t = 0.5") {
        float t = 0.5f;
        Eigen::Array3f expected(2.25f, 3.25f, 4.25f); // Precomputed expected values
        Eigen::Array3f result = cubicInterpolate(P0, P1, P2, P3, t);

        for (int i = 0; i < result.size(); ++i) {
            REQUIRE_THAT(result(i), Catch::Matchers::WithinAbs(expected(i), 1e-5f));
        }
    }

    SECTION("t = 0.75") {
        float t = 0.75f;
        Eigen::Array3f expected(3.0625f, 4.0625f, 5.0625f); // Precomputed expected values
        Eigen::Array3f result = cubicInterpolate(P0, P1, P2, P3, t);

        for (int i = 0; i < result.size(); ++i) {
            REQUIRE_THAT(result(i), Catch::Matchers::WithinAbs(expected(i), 1e-5f));
        }
    }
}

// --------------------------------------------------------------------------------------
// applyVerticalFilter
// --------------------------------------------------------------------------------------
TEST_CASE("applyVerticalFilter uses a unit filter", "[applyVerticalFilter]")
{
    Eigen::Array<float, 3, 3, Eigen::RowMajor> src{
            {1, 2, 3},
            {4, 5, 6},
            {7, 8, 9}
    };

    Eigen::Array<float, 3, 1> unitFilter{0.f, 1.f, 0.f};

    ClampBoundaryPolicy clampPolicy;
    auto dst = applyVerticalFilter(src, unitFilter, clampPolicy);

    REQUIRE(dst.isApprox(src));
}

TEST_CASE("applyVerticalFilter handles negative filter coefficients", "[applyVerticalFilter]")
{
    Eigen::Array<float, 3, 3, Eigen::RowMajor> src{
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9}
    };

    Eigen::Array<float, 3, 1> negFilter;
    negFilter << -1.f, 2.f, -1.f;

    MirrorBoundaryPolicy mirrorPolicy;
    auto dst = applyVerticalFilter(src, negFilter, mirrorPolicy);

    Eigen::Array<float, 3, 3, Eigen::RowMajor> expected{
            {-3, -3, -3},
            {0, 0, 0},
            {3, 3, 3}
    };

    REQUIRE(dst.isApprox(expected));
}

TEST_CASE("applyVerticalFilter with different boundary policy (Mirror)", "[applyVerticalFilter]")
{
    Eigen::Array<float, 4, 2, Eigen::RowMajor> src{
        {1, 2},
        {3, 4},
        {5, 6},
        {7, 8}
    };

    Eigen::Array<float, 3, 1> filter;
    filter << 0.25f, 0.5f, 0.25f;

    MirrorBoundaryPolicy mirrorPolicy{};
    auto dst = applyVerticalFilter(src, filter, mirrorPolicy);

    Eigen::Array<float, 4, 2, Eigen::RowMajor> expected{
            {1.5, 2.5},
            {3, 4},
            {5, 6},
            {6.5, 7.5}
    };

    REQUIRE(dst.isApprox(expected));
}

// --------------------------------------------------------------------------------------
// createVerticalGaussianPyramid
// --------------------------------------------------------------------------------------

TEST_CASE("createVerticalGaussianPyramid with single substage", "[applyVerticalFilter]")
{
    Eigen::Array<float, 5, 3, Eigen::RowMajor> src{
            {1, 2, 3},
            {4, 5, 6},
            {7, 8, 9},
            {10, 11, 12},
            {13, 14, 15}
    };

    int numSubLevels{1};
    const auto& [scales, pyramid] = createVerticalGaussianPyramid(src, numSubLevels);

    REQUIRE(pyramid.size() == scales.size());
    REQUIRE(pyramid.size() == 2);
    REQUIRE(scales[0] == 1);
    REQUIRE(scales[1] == static_cast<float>(src.rows() / 2) / static_cast<float>(src.rows()));

    // Pyramid stage 0 is copy of the src image in float
    REQUIRE(pyramid[0].rows() == src.rows());
    REQUIRE(pyramid[0].cols() == src.cols());

    // Pyramid stage 1 is 0.5 row-downscale of the src image in float
    REQUIRE(pyramid[1].rows() == 2);
    REQUIRE(pyramid[0].cols() == src.cols());
}