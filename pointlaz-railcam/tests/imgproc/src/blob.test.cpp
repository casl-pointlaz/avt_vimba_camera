#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "railcam/imgproc/blob.h"

using namespace railcam::imgproc;


TEST_CASE("Blob detection - Simple Case") {
    Eigen::Array<bool, 5, 5, Eigen::ColMajor> arr;
    arr << 0, 0, 1, 1, 0,
           0, 1, 1, 0, 0,
           1, 1, 0, 0, 0,
           0, 0, 0, 1, 1,
           0, 1, 1, 0, 0;

    SECTION("Check blob locations") {
        // Check for the first column (index 0)
        auto blobs0 = detectBlobs(arr.col(0));
        REQUIRE(blobs0.size() == 1);
        REQUIRE(blobs0[0].row_start == 2);
        REQUIRE(blobs0[0].row_end == 3);

        // Check for the second column (index 1)
        auto blobs1 = detectBlobs(arr.col(1));
        REQUIRE(blobs1.size() == 2);
        REQUIRE(blobs1[0].row_start == 1);
        REQUIRE(blobs1[0].row_end == 3);
        REQUIRE(blobs1[1].row_start == 4);
        REQUIRE(blobs1[1].row_end == 5);

        // Check for the third column (index 2)
        auto blobs2 = detectBlobs(arr.col(2));
        REQUIRE(blobs2.size() == 2);
        REQUIRE(blobs2[0].row_start == 0);
        REQUIRE(blobs2[0].row_end == 2);
        REQUIRE(blobs2[1].row_start == 4);
        REQUIRE(blobs2[1].row_end == 5);

        // Check for the fourth column (index 3)
        auto blobs3 = detectBlobs(arr.col(3));
        REQUIRE(blobs3.size() == 2);
        REQUIRE(blobs3[0].row_start == 0);
        REQUIRE(blobs3[0].row_end == 1);
        REQUIRE(blobs3[1].row_start == 3);
        REQUIRE(blobs3[1].row_end == 4);

        // Check for the fifth column (index 4)
        auto blobs4 = detectBlobs(arr.col(4));
        REQUIRE(blobs4.size() == 1);
        REQUIRE(blobs4[0].row_start == 3);
        REQUIRE(blobs4[0].row_end == 4);
    }
}

TEST_CASE("Blob detection - No Blobs") {
    Eigen::Array<bool, 5, 5, Eigen::ColMajor> arr;
    arr << 0, 0, 0, 0, 0,
           0, 0, 0, 0, 0,
           0, 0, 0, 0, 0,
           0, 0, 0, 0, 0,
           0, 0, 0, 0, 0;

    SECTION("Check no blobs detected") {
        for (const auto& col : arr.colwise()) {
            REQUIRE(detectBlobs(col).empty());  // No blobs in any column
        }
    }
}

TEST_CASE("Blob detection - Edge Case with Single Cell") {
    Eigen::Array<bool, 1, 1, Eigen::ColMajor> arr{1};

    auto blobs = detectBlobs(arr);

    SECTION("Check blob location") {
        REQUIRE(blobs.size() == 1);
        REQUIRE(blobs[0].row_start == 0);
        REQUIRE(blobs[0].row_end == 1);
    }
}