#pragma once

#include <stdint.h>  //NOLINT(modernize-deprecated-headers)

#include "dp_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 *@brief Compresses the passed image.
 *
 *@param pImgBuffer Points to the pixel data of the image that should be
 *compressed. Pointer must not be null.
 *@param imgWidth Width of the image in pixels.
 *@param imgHeight Height of the image in pixels.
 *@param pDstBuffer Pointer to output buffer. Must not be null.
 *@param pDstLen Points to maximum number of bytes that can be written to
 *pDstBuffer. Rule of thumb: at least half the size of the uncompressed data. If
 *the compression succeeds, the number of actual bytes is written here. Must not
 *be null.
 *@retval dp_success if successful.
 *@retval dp_not_initialized if the image data is not suitable for jetraw
    compression.
 *@retval dp_parameter_error if one of the pointers is null..
 *@retval dp_license_error if no valid license was found.
 *@retval dp_unknown_identifier if a bad combination of flags was provided.
 *@retval dp_memory_error if memory allocation failed..
 */
dp_status jetraw_encode(const uint16_t* pImgBuffer, uint32_t imgWidth,
                        uint32_t imgHeight, char* pDstBuffer, int32_t* pDstLen);

/**
 *@brief This function decompresses the passed image.
 *
 *@param pSrcBuffer Points to the bytes of the compressed image data. Must not
 *be null.
 *@param srcLen Size of compressed data in bytes.
 *@param pImgBuffer Points to the output buffer. Must not be null..
 *@param imgPixels the maximum number of pixels that can be written to
 *pImgBuffer.
 *@retval dp_success if successful.
 *@retval dp_parameter_error if one of the pointers is null.
 *@retval dp_image_too_small if imgPixels is smaller than the
 * pixels of the compressed image.
 *@retval dp_memory_error if memory allocation failed.
 */
dp_status jetraw_decode(const char* pSrcBuffer, int32_t srcLen,
                        uint16_t* pImgBuffer, int32_t imgPixels);

/**
 *@brief This function returns the current jetraw version.
 *
 */
const char* jetraw_version();

#ifdef __cplusplus
}  // extern "C"
#endif