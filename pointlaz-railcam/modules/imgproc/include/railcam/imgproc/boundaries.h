#ifndef RAILCAM_RAILCAM_BOUNDARIES_H
#define RAILCAM_RAILCAM_BOUNDARIES_H
#include <concepts>

namespace railcam::imgproc
{
    /**
     * Concept for boundary handling policies.
     * A valid BoundaryPolicy must define a callable operator that adjusts out-of-bounds indices
     * to valid indices within the range of a given size.
     *
     * @tparam Policy The policy type.
     * @param policy The policy instance.
     * @param index The index to be adjusted.
     * @param size The total size of the array.
     * @return A valid index within the range [0, size - 1].
     *
     * Example:
     * ```
     * struct ClampBoundaryPolicy {
     *     int operator()(int index, int size) const {
     *         return std::clamp(index, 0, size - 1);
     *     }
     * };
     * ```
     */
    template <typename Policy>
    concept BoundaryPolicy = requires(Policy policy, int index, int size) {
        { policy(index, size) } -> std::convertible_to<int>;
    };


    /**
    * Boundary policy that mirrors indices outside the valid range.
    * Ensures smooth handling of out-of-bounds indices by reflecting them back into the valid range.
    *
    * @param index The out-of-bounds index.
    * @param size The total size of the array.
    * @return A valid mirrored index within the range [0, size - 1].
    *
    * Example:
    * ```
    * MirrorBoundaryPolicy mirrorPolicy;
    * int validIndex = mirrorPolicy(-2, 5);  // Returns 1 (mirrored index)
    * validIndex = mirrorPolicy(6, 5);      // Returns 3 (mirrored index)
    * ```
    */
    struct MirrorBoundaryPolicy {
        int operator()(int index, int size) const {
            if (index < 0) {
                return -index - 1;  // or something similar that returns 0 for index=-1
            } else if (index >= size) {
                return 2 * size - index - 1;
            }
            return index;
        }
    };
} //namespace railcam::railcam
#endif //RAILCAM_RAILCAM_BOUNDARIES_H
