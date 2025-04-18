#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "railcam/testsutils/image.h"
#include "railcam/testsassets/assets_path.h"
#include "railcam/imgproc/laserdetection.h"
#include <random>
#include <chrono>
#include <iostream>

using namespace railcam::imgproc;


// -----------------------------------------------------------------------------------------
// computeLaserDetection
// -----------------------------------------------------------------------------------------

/**
 * Creates a test array with randomly distributed blobs and applies Gaussian blur.
 *
 * @param rows Number of rows in the array.
 * @param cols Number of columns in the array.
 * @param numBlobsPerCol Number of blobs to generate per column.
 * @param blobIntensity Intensity value for the blobs after normalization.
 * @param blurIterations Number of Gaussian blur iterations to apply.
 * @return A tuple containing the resulting blobbed image (uint8_t) and the laserCoords (float).
 */
std::tuple<Eigen::Array<uint8_t, -1, -1, Eigen::RowMajor>, Eigen::Array<float, -1, -1>>
createFakeImageWithBlobs(int rows, int cols, int numBlobsPerCol, uint8_t blobIntensity, int blurIterations) {
    // Initialize the array with zeros
    Eigen::Array<float, -1, -1, Eigen::RowMajor> FakeImage =
            Eigen::Array<float, -1, -1, Eigen::RowMajor>::Zero(rows, cols);

    // Set up random number generation with a fixed seed for deterministic output
    std::mt19937 rng(42); // Fixed seed for deterministic output
    std::uniform_int_distribution<int> rowDist(50, rows - 50); // Avoid points on close boundaries

    Eigen::Array<float, -1, -1> laserCoords(numBlobsPerCol, cols);
    int neighborRadius = std::max(1, rows / (numBlobsPerCol * 2));

    for (int col = 0; col < cols; ++col) {
        std::vector<int> blobCoordinates; blobCoordinates.reserve(numBlobsPerCol);
        for (int blob = 0; blob < numBlobsPerCol; ++blob) {
            int blobRow;
            bool isTooClose;

            // Ensure blobs are not too close to each other
            do {
                blobRow = rowDist(rng);
                isTooClose = std::any_of(
                        std::begin(blobCoordinates), std::end(blobCoordinates),
                        [&](int existingRow) {
                            return std::abs(existingRow - blobRow) < neighborRadius;
                        });
            } while (isTooClose);

            FakeImage(blobRow, col) = 1.0f;
            blobCoordinates.emplace_back(blobRow);
        }
        std::ranges::sort(blobCoordinates);
        // For each row, insert the blob index in sorted manner
        for(int blobId{0}; blobId < blobCoordinates.size(); ++blobId){
            laserCoords(blobId, col) = static_cast<float>(blobCoordinates[blobId]) + 0.5f;  // + 0.5 because center of pixel
        }
    }

    Eigen::Array<float, 5, 1> gaussianFilter{1.0f / 16, 4.0f / 16, 6.0f / 16, 4.0f / 16, 1.0f / 16};
    for (int i = 0; i < blurIterations; ++i) {
        FakeImage = applyVerticalFilter(FakeImage, gaussianFilter);
    }

    // Normalize and scale to blob intensity
    FakeImage /= FakeImage.maxCoeff();
    FakeImage *= blobIntensity;

    return {FakeImage.cast<uint8_t>().eval(), laserCoords};
}

inline int countNaNColumns(const Eigen::Array<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>& array) {
    // Check which elements are NaN
    Eigen::Array<bool, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> nanMask = array.isNaN();

    // Sum along rows to get NaN counts per column
    Eigen::Array<bool, 1, Eigen::Dynamic, Eigen::RowMajor> nanColumns = (nanMask.colwise().all());

    // Count NaN-only columns
    return nanColumns.count();
}

TEST_CASE("computeLaserDetection basic test with gradient image", "[LaserDetection]") {
    // Define the LaserDetectionOptions
    LaserDetectionOptions options{
            .numLines = 7,
            .pyramidsSublevels = 1,
            .loGThreshold = 1.0f,
            .upScaleForSubPixelDetection = 5.0f
    };

    // Create the gradient buffer (image)
    // 2064, 2464
    int genScaling{10};
    auto [buffer, laserCoords] = createFakeImageWithBlobs(2064*genScaling, 2464, options.numLines, 255, 5*genScaling);
    auto downScaledRows = static_cast<int>(static_cast<float>(buffer.rows())/static_cast<float>(genScaling));
    auto downScaledRatio = static_cast<float>(buffer.rows()) / static_cast<float>(downScaledRows);
    auto resized = resizeRowsCubic(buffer.cast<float>().eval(), downScaledRows);
    buffer = resized.cast<uint8_t>();
    railcam::testsutils::writePGM("laserLines_fake.pgm", buffer);
    laserCoords /= downScaledRatio;


    auto [scales, pyramid] = createVerticalGaussianPyramid(buffer, static_cast<int>(options.pyramidsSublevels));
    for(int i{0};i<pyramid.size();++i){
        auto const img = (pyramid[i] / pyramid[i].maxCoeff() * 255).cast<uint8_t>().eval();
        railcam::testsutils::writePGM("pyramid_" + std::to_string(scales[i]) + ".pgm", img);
    }

    // Call the function
    auto start = std::chrono::high_resolution_clock::now();
    auto resultLaserCoords = computeLaserDetection(buffer, options);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "Elapsed time for computeLaserDetection (total): " << elapsed.count() << " ms\n";

    std::cout << "laserCoords (Ground Truth):\n" << laserCoords << "\n";
    std::cout << "resultLaserCoords (Computed):\n" << resultLaserCoords << "\n";

    auto diff = (resultLaserCoords - laserCoords).abs().eval();
    auto mean = diff.mean();
    double variance = (diff - mean).square().mean();

    std::cout << "image size :\n" << buffer.rows() << " rows X " << buffer.cols() << " cols" << "\n";
    std::cout << "mean Error :\n" << mean << "\n";
    std::cout << "std Error :\n" << std::sqrt(variance) << "\n";
    std::cout << "max Error :\n" << diff.maxCoeff() << "\n";
    REQUIRE(mean < 0.2);
    REQUIRE(std::sqrt(variance) < 0.1);
    REQUIRE(diff.maxCoeff() < 0.5);

    // Verify the output dimensions
    REQUIRE(resultLaserCoords.rows() == options.numLines);   // Should match the expected number of lines
    REQUIRE(resultLaserCoords.cols() == buffer.cols());      // Should match the number of columns in the buffer

    // Verify specific values in the result (example expected values based on function's logic)
    REQUIRE_FALSE(resultLaserCoords.isNaN().any());  // No undetected column
    // REQUIRE(laserCoords.isApprox(resultLaserCoords));
}

TEST_CASE("computeLaserDetection edge cases", "[LaserDetection]") {
    // Define LaserDetectionOptions with stricter thresholds
    LaserDetectionOptions options{
            .numLines = 1,
            .pyramidsSublevels = 1,
            .loGThreshold = 5000.0f, // Very high threshold, no detections expected
            .upScaleForSubPixelDetection = 2.0f
    };

    // Create an empty buffer
    auto buffer = Eigen::Array<uint8_t, 10, 10, Eigen::RowMajor>::Zero().eval();

    // Call the function
    auto result = computeLaserDetection(buffer, options);

    // Verify the result is filled with NaN (no detections)
    for (Eigen::Index i = 0; i < result.rows(); ++i) {
        for (Eigen::Index j = 0; j < result.cols(); ++j) {
            REQUIRE(std::isnan(result(i, j)));
        }
    }
}

TEST_CASE("computeLaserDetection on real image A", "[LaserDetection]") {
    LaserDetectionOptions options{
            .numLines = 9,
            .pyramidsSublevels = 1,
            .loGThreshold = 15.0f,
            .upScaleForSubPixelDetection = 5.0f
    };

    const std::string sceneA_path = RAILCAM_TEST_ASSETSDIR + std::string{"/250us_20db.pgm"};
    const auto& sceneA = railcam::testsutils::readPGM(sceneA_path);


    auto const& lineScanData = computeLaserDetection(sceneA, options);

    auto rgbScene = railcam::testsutils::grayscaleToRGB(sceneA);
    railcam::testsutils::drawRedPixels(rgbScene, lineScanData);
    railcam::testsutils::writePPM("detection_250us_20db.ppm", rgbScene);

    REQUIRE(countNaNColumns(lineScanData) == 1783);
}

TEST_CASE("computeLaserDetection on real image B", "[LaserDetection]") {
    LaserDetectionOptions options{
            .numLines = 9,
            .pyramidsSublevels = 1,
            .loGThreshold = 60.0f,
            .upScaleForSubPixelDetection = 5.0f
    };

    const std::string sceneB_path = RAILCAM_TEST_ASSETSDIR + std::string{"/2000us_20db.pgm"};
    const auto& sceneB = railcam::testsutils::readPGM(sceneB_path);

    auto const& lineScanData = computeLaserDetection(sceneB, options);
    
    auto rgbScene = railcam::testsutils::grayscaleToRGB(sceneB);
    railcam::testsutils::drawRedPixels(rgbScene, lineScanData);
    railcam::testsutils::writePPM("detection_2000us_20db.ppm", rgbScene);

    REQUIRE(countNaNColumns(lineScanData) == 1784);
}