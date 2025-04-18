#ifndef RAILCAM_RAILCAM_PYRAMIDS_H
#define RAILCAM_RAILCAM_PYRAMIDS_H
#include <tuple>
#include "railcam/imgproc/interpolate.h"

namespace railcam::imgproc
{
    /**
     * Apply a vertical filter to an image with customizable boundary handling.
     * Optimized by iterating over rows for better vectorization.
     *
     * @tparam Derived The Eigen type of the filter.
     * @tparam Policy The boundary handling policy, satisfying the BoundaryPolicy concept.
     * @param src The source image as an Eigen array.
     * @param filter The 1D filter to apply vertically.
     * @param boundaryPolicy Callable object to handle out-of-bounds indices.
     * @return The filtered image as an Eigen array.
     */
    template <typename DerivedA, typename DerivedB, BoundaryPolicy Policy=MirrorBoundaryPolicy>
    requires ((DerivedA::IsRowMajor == true) || ((DerivedA::IsRowMajor == false) && DerivedA::ColsAtCompileTime == 1))
    && (DerivedB::SizeAtCompileTime != Eigen::Dynamic) &&
        (DerivedB::IsVectorAtCompileTime == true)
    inline auto applyVerticalFilter(const Eigen::ArrayBase<DerivedA>& src, const Eigen::ArrayBase<DerivedB>& filter,
        Policy boundaryPolicy=MirrorBoundaryPolicy{}) {
        static constexpr int filterSize = DerivedB::SizeAtCompileTime;
        static constexpr int filterRadius = filterSize / 2;
        static_assert(filterSize % 2 == 1 && "Filter size must be odd for symmetric application.");
        assert(filterRadius < src.rows());

        // Define the storage order as a constexpr. Prefer RowMajor for performance (benefit L1)
        constexpr Eigen::StorageOptions StorageOrder =
                (DerivedA::ColsAtCompileTime == 1) ? Eigen::ColMajor : Eigen::RowMajor;

        // Define the output array with the determined storage order
        using Scalar = typename DerivedA::Scalar;
        auto dst = Eigen::Array<Scalar, DerivedA::RowsAtCompileTime, DerivedA::ColsAtCompileTime, StorageOrder>::Zero(
            src.rows(), src.cols()).eval();

        for (int row{filterRadius}; row < (src.rows() - filterRadius); ++row) {
            dst.row(row) = (filter.matrix().transpose() * src.template block<filterSize, -1>(row - filterRadius, 0, filterSize, src.cols()).matrix()).array();
        }

        // Boundaries
        for (int row{0}; row < filterRadius; ++row) {
            for (int k = 0; k < filterSize; ++k) {
                int srcIndex = boundaryPolicy(row + k - filterRadius, src.rows());
                dst.row(row) += filter(k) * src.row(srcIndex);
            }
        }
        for (int row = (src.rows() - filterRadius); row < src.rows(); ++row) {
            for (int k = 0; k < filterSize; ++k) {
                int srcIndex = boundaryPolicy(row + k - filterRadius, src.rows());
                dst.row(row) += filter(k) * src.row(srcIndex);
            }
        }
        return dst;
    }

    /**
     * Generate a vertically downsampled Gaussian pyramid of an image.
     *
     * This function constructs a Gaussian pyramid by applying a vertical Gaussian filter
     * to the input image at each level and downsampling the result. Each level is stored
     * in ColMajor format, and the corresponding scale factor is tracked.
     *
     * @tparam Derived Eigen type of the input image (must be RowMajor).
     * @param src The input image as an Eigen array (RowMajor).
     * @param sublevels The number of levels to generate in the pyramid.
     * @return A tuple containing:
     *         1. A vector of scale factors (float) for each pyramid level.
     *         2. A vector of Eigen arrays (ColMajor) representing the pyramid levels.
     *
     * @note The input image is cast to `float` for processing.
     *       The function stops if the image height becomes less than 2 pixels.
     */
    template <typename Derived>
    requires (Derived::IsRowMajor == true)
    inline auto createVerticalGaussianPyramid(const Eigen::ArrayBase<Derived>& src, uint8_t sublevels) {
        // Gaussian filter for vertical filtering
        const Eigen::Array<float, 5, 1> gaussianFilter{1 / 16.f, 4 / 16.f, 6 / 16.f, 4 / 16.f, 1 / 16.f};

        // Preallocate the pyramid and scales
        std::vector<Eigen::Array<float, -1, -1, Eigen::ColMajor>> pyramid;
        pyramid.reserve(sublevels + 1);

        std::vector<float> scales;
        scales.reserve(sublevels + 1);

        // Initialize the pyramid with the input image cast to float
        Eigen::Array<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> current = src.template cast<float>().eval();
        pyramid.emplace_back(current); // Level 0 (original image)
        scales.emplace_back(1.0f);     // Original scale factor is 1.0

        for (int i = 0; i < sublevels; ++i) {
            // Apply vertical Gaussian filter
            current = applyVerticalFilter(current, gaussianFilter);

            // Compute the new height for downsampling
            int newHeight = current.rows() / 2;
            if (newHeight < 2) break; // Stop if the image becomes too small

            // Compute scale and add it to the scales vector
            scales.emplace_back(static_cast<float>(newHeight) / static_cast<float>(current.rows()));

            // Downsample the image vertically
            current = resizeRowsLinear(current, newHeight);

            // Store the downsampled image in the pyramid
            pyramid.emplace_back(std::move(current));
        }
        return std::make_tuple(scales, pyramid);
    }

} //namespace railcam::railcam
#endif //RAILCAM_RAILCAM_PYRAMIDS_H
