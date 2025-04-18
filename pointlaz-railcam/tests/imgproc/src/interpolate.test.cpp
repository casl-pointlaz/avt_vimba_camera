#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "railcam/imgproc/interpolate.h"

using namespace railcam::imgproc;


TEST_CASE("resizeColumnsCubic simple downscaling", "[resizeColumnsCubic][downscaling]") {
    Eigen::Array<float, 4, -1, Eigen::RowMajor> src(4,1);
    src << 10.f, 20.f, 30.f, 40.f;

    int newHeight = 2;  // Downscale to 2 rows
    MirrorBoundaryPolicy clampPolicy;
    auto resized = resizeRowsCubic(src, newHeight, clampPolicy);

    REQUIRE(resized.rows() == newHeight);
    REQUIRE(resized.cols() == 1);

    REQUIRE_THAT(resized(0), Catch::Matchers::WithinAbs(14.375, 1e-5f));
    REQUIRE_THAT(resized(1), Catch::Matchers::WithinAbs(35.625, 1e-5f));
}

TEST_CASE("resizeColumnsCubic simple upscaling", "[resizeColumnsCubic][upscaling]") {
    Eigen::Array<float, 4, -1, Eigen::RowMajor> src(4,1);
    src << 10.f, 20.f, 30.f, 40.f;

    int newHeight = 5;  // Upscale to 5 rows
    MirrorBoundaryPolicy clampPolicy;
    auto resized = resizeRowsCubic(src, newHeight, clampPolicy);

    REQUIRE(resized.rows() == newHeight);
    REQUIRE(resized.cols() == 1);

    REQUIRE_THAT(resized(0), Catch::Matchers::WithinAbs(18.91f, 1e-5f));    // Influenced by mirror
    REQUIRE_THAT(resized(1), Catch::Matchers::WithinAbs(16.685f, 1e-5f));   // Influenced by mirror
    REQUIRE_THAT(resized(2), Catch::Matchers::WithinAbs(25.0f, 1e-5f));
    REQUIRE_THAT(resized(3), Catch::Matchers::WithinAbs(33.315f, 1e-5f));   // Influenced by mirror
    REQUIRE_THAT(resized(4), Catch::Matchers::WithinAbs(40.45f, 1e-5f));    // Influenced by mirror
}

TEST_CASE("resizeColumnsCubic handles boundary conditions", "[resizeColumnsCubic]") {
    Eigen::Array<float, 3, 5, Eigen::RowMajor> src{
            {1, 2, 3, 4, 5},
            {6, 7, 8, 9, 10},
            {11, 12, 13, 14, 15}
    };

    // Resize to fewer rows (newHeight=2)
    int newHeight = 2;
    MirrorBoundaryPolicy clampPolicy; // Ensures boundary-safe behavior
    auto resized = resizeRowsCubic(src, newHeight, clampPolicy);

    REQUIRE(resized.rows() == newHeight);
    REQUIRE(resized.cols() == 5);

    // Validate no out-of-bounds access occurred
    for (int r = 0; r < resized.rows(); ++r) {
        for (int c = 0; c < resized.cols(); ++c) {
            REQUIRE(std::isfinite(resized(r, c)));
        }
    }
}