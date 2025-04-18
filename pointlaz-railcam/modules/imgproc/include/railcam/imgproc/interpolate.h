#ifndef RAILCAM_IMGPROC_INTERPOLATE_H
#define RAILCAM_IMGPROC_INTERPOLATE_H
#include <Eigen/Dense>
#include "railcam/imgproc/boundaries.h"

namespace railcam::imgproc
{
    /**
     * Perform cubic interpolation for a given t in [0, 1].
     * Equation:
     * P(t) = P1 + 0.5 * t * (P2 - P0 + t * (2P0 - 5P1 + 4P2 - P3 + t * (3(P1 - P2) + P3 - P0)))
     *
     * @param P0 The first control point (previous pixel value).
     * @param P1 The second control point (current pixel value).
     * @param P2 The third control point (next pixel value).
     * @param P3 The fourth control point (next-next pixel value).
     * @param t  The interpolation factor in the range [0, 1].
     * @return The interpolated value.
     */
    template<typename Derived>
    auto cubicInterpolate(const Eigen::ArrayBase<Derived>& P0,
                          const Eigen::ArrayBase<Derived>& P1,
                          const Eigen::ArrayBase<Derived>& P2,
                          const Eigen::ArrayBase<Derived>& P3,
                          typename Derived::Scalar t) {
        auto a = -0.5f * P0 + 1.5f * P1 - 1.5f * P2 + 0.5f * P3;
        auto b = P0 - 2.5f * P1 + 2.0f * P2 - 0.5f * P3;
        auto c = -0.5f * P0 + 0.5f * P2;
        return (((a * t + b) * t + c) * t + P1).eval();
    }

    /**
     * Resize an image in the row direction using centered cubic interpolation.
     * Supports upscaling/downscaling with customizable boundary handling.
     *
     * @tparam Policy Boundary handling policy satisfying the BoundaryPolicy concept.
     * @param src Source image as an Eigen array.
     * @param newHeight Desired height of the resized image.
     * @param boundaryPolicy Callable for handling out-of-bounds indices.
     * @return Resized image as an Eigen array.
     *
     * @note Clips values to [0, 255] and assumes floating-point pixel values.
     */
    template <typename Derived, BoundaryPolicy Policy=MirrorBoundaryPolicy>
    requires ((Derived::IsRowMajor == true) || ((Derived::IsRowMajor == false) && Derived::ColsAtCompileTime == 1)) &&
    (std::is_floating_point_v<typename Derived::Scalar>)
    inline auto resizeRowsCubic(const Eigen::ArrayBase<Derived>& _src,
        int newHeight, Policy boundaryPolicy=MirrorBoundaryPolicy{}) {
        assert(newHeight > 0);
        const auto& src = _src.eval();

        // Define the storage order as a constexpr. Prefer RowMajor for performance (benefit L1)
        constexpr Eigen::StorageOptions StorageOrder =
                (Derived::ColsAtCompileTime == 1) ? Eigen::ColMajor : Eigen::RowMajor;

        // Define the output array with the determined storage order
        using Scalar = typename Derived::Scalar;
        Eigen::Array<Scalar, -1, Derived::ColsAtCompileTime, StorageOrder> dst(newHeight, src.cols());

        float scale = static_cast<float>(src.rows()) / static_cast<float>(newHeight);

        for (int y{0}; y < newHeight; ++y) {
            // Centered mapping formula
            float srcY = (static_cast<float>(y) + 0.5f) * scale - 0.5f;
            int y1 = static_cast<int>(std::floor(srcY));
            auto t = srcY - static_cast<typename Derived::Scalar>(y1);

            // Get four neighboring rows using boundary policy
            int y0 = boundaryPolicy(y1 - 1, src.rows());
            y1 = boundaryPolicy(y1, src.rows());
            int y2 = boundaryPolicy(y1 + 1, src.rows());
            int y3 = boundaryPolicy(y1 + 2, src.rows());

            // Perform cubic interpolation
            auto P0 = src.row(y0);
            auto P1 = src.row(y1);
            auto P2 = src.row(y2);
            auto P3 = src.row(y3);

            dst.row(y) = cubicInterpolate(P0, P1, P2, P3, t);
        }

        return dst;
    }

    /**
     * Resize an image in the row direction using linear interpolation.
     * Supports upscaling/downscaling with customizable boundary handling.
     *
     * @tparam Policy Boundary handling policy satisfying the BoundaryPolicy concept.
     * @param src Source image as an Eigen array.
     * @param newHeight Desired height of the resized image.
     * @param boundaryPolicy Callable for handling out-of-bounds indices.
     * @return Resized image as an Eigen array.
     *
     * @note Clips values to [0, 255] and assumes floating-point pixel values.
     */
    template <typename Derived, typename Policy = MirrorBoundaryPolicy>
    requires ((Derived::IsRowMajor == true) || ((Derived::IsRowMajor == false) && Derived::ColsAtCompileTime == 1))
    inline auto resizeRowsLinear(const Eigen::ArrayBase<Derived>& _src,
                                 int newHeight, Policy boundaryPolicy = MirrorBoundaryPolicy{}) {
        assert(newHeight > 0);
        const auto& src = _src.eval();

        // Define the storage order for the output array
        constexpr Eigen::StorageOptions StorageOrder =
                (Derived::ColsAtCompileTime == 1) ? Eigen::ColMajor : Eigen::RowMajor;

        // Define the output array
        using Scalar = typename Derived::Scalar;
        Eigen::Array<Scalar, -1, Derived::ColsAtCompileTime, StorageOrder> dst(newHeight, src.cols());

        // Calculate the scaling factor
        float scale = static_cast<float>(src.rows()) / static_cast<float>(newHeight);

        for (int y = 0; y < newHeight; ++y) {
            // Map the output row to the source row
            float srcY = (static_cast<float>(y) + 0.5f) * scale - 0.5f;
            int y0 = static_cast<int>(std::floor(srcY)); // The first row index
            int y1 = y0 + 1;                            // The second row index
            auto t = srcY - static_cast<float>(y0);     // Linear interpolation factor

            // Apply boundary policy to handle out-of-bounds indices
            y0 = boundaryPolicy(y0, src.rows());
            y1 = boundaryPolicy(y1, src.rows());

            // Perform linear interpolation
            auto row0 = src.row(y0).template cast<float>();
            auto row1 = src.row(y1).template cast<float>();
            dst.row(y) = ((1.0f - t) * row0 + t * row1).template cast<typename Derived::Scalar>();
        }

        return dst;
    }
} //namespace railcam::imgproc
#endif //RAILCAM_IMGPROC_INTERPOLATE_H
