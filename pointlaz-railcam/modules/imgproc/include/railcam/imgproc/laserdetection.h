#ifndef RAILCAM_IMGPROC_LASERDETECTION_H
#define RAILCAM_IMGPROC_LASERDETECTION_H
#include <ranges>
#include <set>
#include <unordered_set>
#include <algorithm>
#include "railcam/imgproc/pyramids.h"
#include "railcam/imgproc/blob.h"

namespace railcam::imgproc
{
    struct LaserDetectionOptions
    {
        int numLines;
        int8_t pyramidsSublevels;
        float loGThreshold, upScaleForSubPixelDetection;
    };

    /**
    * @brief Generates the second derivative of a Gaussian kernel (Laplacian of Gaussian).
    *
    * This function computes the second derivative of the Gaussian function (also known as the Laplacian of Gaussian, LoG)
    * for a given standard deviation (sigma) and a kernel size. The result is a 1D kernel represented by an Eigen vector,
    * which highlights the center of edges or lines in an image. The kernel is computed using the formula:
    *
    * g''(x) = -(x² / σ⁴ - 1 / σ²) * exp(-x² / (2 * σ²))
    *
    * The second derivative of the Gaussian (LoG) is often used in edge detection algorithms to find zero-crossings,
    * which correspond to edges or transitions in an image.
    *
    * @tparam SIZE The size of the kernel (must be an odd number to center it around zero).
    * @param sigma The standard deviation of the Gaussian function.
    * @return Eigen::VectorXf The kernel as a 1D Eigen vector.
    */
    template<int SIZE>
    Eigen::Array<float, SIZE, 1> makeLoGKernel(float sigma) {
        Eigen::Array<float, SIZE, 1> kernel{};
        int half_size = SIZE / 2;

        // Calculate kernel values for the first half
        for (int i = 0; i <= half_size; ++i) {
            auto x = static_cast<float>(i - half_size);
            kernel(i) = -(std::pow(x, 2) / std::pow(sigma, 4) - 1 / std::pow(sigma, 2))
                        * std::exp(-std::pow(x, 2) / (2 * std::pow(sigma, 2)));
        }

        // Make the kernel symmetric
        for (int i = 0; i < half_size; ++i) {
            kernel(SIZE - i - 1) = kernel(i); // Symmetrize the kernel
        }

        // Normalize the kernel to ensure that its sum is 0
        kernel.array() -= kernel.sum();
        return kernel;
    }


    /**
     * Performs per-column laser line detection with sub-pixel accuracy using a Gaussian pyramid approach.
     *
     * This function detects laser lines in a grayscale image, leveraging a multi-resolution pyramid
     * to enhance detection performance. The function scans each image column, detecting laser lines
     * at various resolutions and pinpointing the corresponding blob locations in the highest resolution.
     * Sub-pixel accuracy is achieved by refining the detected positions using the Gaussian pyramid data.
     *
     * @param buffer The input grayscale image represented as an Eigen::Array<uint8_t, -1, -1, Eigen::RowMajor>.
     *               Each column of the buffer corresponds to an image column that will be processed.
     * @param options Configuration options for laser line detection, including parameters such as the number of
     *                expected laser lines, Gaussian pyramid levels, and filtering thresholds.
     * @return A NxM col-major Eigen::Array<float, -1, -1>, where N is the number of laser lines and M is the
     *         number of image columns. Each element represents the sub-pixel row coordinate of the detected
     *         laser line in the corresponding column. If no detection is made for a line in a column, the value
     *         is set to NaN.
     */
    template<typename Derived> requires (Derived::IsRowMajor == true)
    auto computeLaserDetection(const Eigen::ArrayBase<Derived>& buffer, const LaserDetectionOptions& options)
    {
        // Create a pyramid of Gaussian images where the scale is applied row-wise only.
        // Hence, each level have the same number of columns.
        auto [scales, colMajorPyramid] = createVerticalGaussianPyramid(buffer, static_cast<int>(options.pyramidsSublevels));

        const auto laplacianKernel = makeLoGKernel<9>(1.0);

        // Detect the blobs per columns starting from lower resolution to higher
        // For each column, identify the lowest resolution that matches numLines
        std::vector<std::vector<Blob>> blobsPerCol(buffer.cols());
        std::vector<float> detectedColScales(buffer.cols());
        std::unordered_set<uint16_t> undetectedCols{}; undetectedCols.reserve(buffer.cols());
        for (uint16_t i = 0; i < static_cast<uint16_t>(buffer.cols()); ++i) {undetectedCols.insert(i);}
        std::set<uint16_t> detectedCols{};

        for (int levelId(colMajorPyramid.size()-1); levelId >= 0; --levelId){
            if(undetectedCols.empty()){
                break;
            }

            // At this point we use ColMajor convention to benefit from L1 cache
            const auto& level = colMajorPyramid[levelId];
            for (auto it = std::begin(undetectedCols); it != std::end(undetectedCols);) {
                uint16_t colId = *it;

                auto const hatDetection = applyVerticalFilter(level.col(colId), laplacianKernel);
                auto const& thresholdLevel = (hatDetection >= options.loGThreshold).template cast<bool>().eval();
                auto blobs = detectBlobs(thresholdLevel);

                if (blobs.size() == options.numLines) {
                    blobsPerCol[colId] = std::move(blobs);
                    detectedColScales[colId] = scales.at(levelId);

                    // Safely erase and update iterator
                    it = undetectedCols.erase(it);
                    detectedCols.insert(colId);
                } else {
                    ++it; // Only increment if not erased
                }
            }
        }

        // Pinpoint the corresponding blob in the highest resolution for each col that have found the required numLines
        auto lineScanData = Eigen::Array<float, -1, -1>::Constant(options.numLines, buffer.cols(), NAN).eval();
        Eigen::Index maxIndex;
        for(const auto colId : detectedCols){
            const auto& blobsForThisCol = blobsPerCol.at(colId);
            const auto detectedScale = detectedColScales.at(colId);

            const auto blobScaleInOriginal = 1.f / detectedScale;

            for(Eigen::Index laserRowId{0}; laserRowId < options.numLines; ++laserRowId){
                const auto detectedBlob = blobsForThisCol.at(laserRowId);
                auto rowStartInOriginal = static_cast<Eigen::Index>(blobScaleInOriginal*detectedBlob.row_start);
                auto rowEndInOriginal = static_cast<Eigen::Index>(std::ceil(blobScaleInOriginal*detectedBlob.row_end));
                auto lenInOriginal = std::max<Eigen::Index>(rowEndInOriginal - rowStartInOriginal, 4);

                rowStartInOriginal = std::max<Eigen::Index>(0ul, rowStartInOriginal- lenInOriginal);
                rowEndInOriginal = std::min<Eigen::Index>(buffer.rows(), rowEndInOriginal + lenInOriginal);
                lenInOriginal = rowEndInOriginal - rowStartInOriginal;

                const Eigen::Array<float, -1, 1, Eigen::ColMajor> blockInOriginal = colMajorPyramid[0].template block<-1,1>(rowStartInOriginal, colId, lenInOriginal, 1).eval();

                const auto& upScaledHeight = static_cast<int>(static_cast<float>(blockInOriginal.rows())*options.upScaleForSubPixelDetection);
                const auto& upScaledBlock = resizeRowsCubic(blockInOriginal, upScaledHeight);
                const auto& upScaledScale = static_cast<float>(upScaledHeight) / static_cast<float>(blockInOriginal.rows());

                upScaledBlock.maxCoeff(&maxIndex);
                auto indxInOriginal = static_cast<float>(maxIndex) / upScaledScale + static_cast<float>(rowStartInOriginal);
                lineScanData(laserRowId, colId) = indxInOriginal;
            }
        }
        return lineScanData;
    }

} //namespace railcam::imgproc
#endif //RAILCAM_IMGPROC_LASERDETECTION_H
