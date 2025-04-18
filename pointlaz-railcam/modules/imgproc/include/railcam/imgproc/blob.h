#ifndef RAILCAM_IMGPROC_DISJOINTSET_H
#define RAILCAM_IMGPROC_DISJOINTSET_H
#include <Eigen/Dense>
#include <vector>

namespace railcam::imgproc
{

    struct Blob {
        float row_start; // The start row of the blob
        float row_end;   // The end row of the blob
    };

    /**
     * @brief Detects blobs in a boolean Eigen::Vector.
     * A blob is defined as a contiguous region of ones separated by zeros.
     *
     * @param arr The input Eigen::Vector<bool> where true represents the blob.
     * @return A vector of blobs, each containing the blob region range.
     */
    template <typename Derived>
    requires (Derived::IsVectorAtCompileTime == true) && (std::is_same_v<typename Derived::Scalar, bool>)
    inline std::vector<Blob> detectBlobs(const Eigen::DenseBase<Derived>& arr) {

        std::vector<Blob> blobs{}; blobs.reserve(arr.size()/2);
        for (int i = 0; i < arr.size(); ++i) {
            if (arr(i)) { 
                int row_start{i};
                while((i < arr.size()) && arr(i) ) { ++i;}
                blobs.emplace_back(row_start, i);
            }
        }
        return blobs;
    }
} //namespace railcam::imgproc
#endif //RAILCAM_IMGPROC_DISJOINTSET_H
