/**
 * @file dpcore_pco.h
 * @brief Extended DPCore API for integration with PCO.SDK.
 *
 * Establishes an association between the camera handles that are used
 * extensively in PCO.SDK, and camera identifiers used by DPCore and Jetraw.
 *
 * @author Christoph Clausen (christoph.clausen@dotphoton.com)
 * @version 1.0
 * @date 2020-06-10
 *
 * @copyright Copyright (c) Dotphoton AG 2020
 *
 * Christoph Clausen
 */
#pragma once
#include <jetraw/dp_status.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief "Shelve" BCD-coded timestamps in the first 14px of the image buffer
 * by shifting values to the left by 1 bit.
 *
 * pco.sdk provides the function PCO_SetTimestampMode, which allows to embed a
 * timestamp in the image data in binary form in the first 14 pixels, as
 * readible ASCII text, or both. dpcore modifies the least significant bits
 * of these pixels, which corrupts the timestamp.
 *
 * Calling this function _before_ dpcore_prepare_image or dpcore_embed_meta
 * allows to retrieve the timestamp information later.
 *
 * @param imgbuf Image buffer with the pixel values. Must not be null. Timestamp
 *  is expected to be BCD-coded into the first 14 pixels. The actual presence of
 *  the timestamp is not checked.
 * @return dp_status
 * @retval dp_success if all goes well.
 * @retval dp_memory_error if imgbuf is a nullptr.
 */
dp_status dpcore_pco_timestamp_shelve(uint16_t* imgbuf);

typedef struct {
    int counter;
    int year;
    int month;
    int day;
    int hour;
    int minutes;
    int seconds;
    int microseconds;
} DPCore_PCOTimestamp;

/**
 * @brief Read binary-coded timestamp from image buffer. Automatically detects
 * whether dpcore was applied to buffer and acts correspondingly.
 *
 * @param imgbuf Image buffer with pixel values. Must not be null.
 * @param imgsize Number of pixels in imgbuf. For automatic detection of dpcore,
 *  please give the entire buffer and not just 14 pixels.
 * @param[out] timestamp must not be null and will be populated with decoded
 *  timestamp data on success.
 * @return dp_status
 * @retval dp_success if valid timestamp could be extracted.
 * @retval dp_parameter_error if imgsize < 14.
 * @retval dp_memory_error if imgbuf or timestamp is null.
 * @retval dp_file_corrupt if no valid timestamp was found.
 */
dp_status dpcore_pco_timestamp_read(const uint16_t* imgbuf, int32_t imgsize,
                                    DPCore_PCOTimestamp* timestamp);

#ifdef WIN32 /* For PCO SDK, supported on Windows only */

/**
 * @brief Initializes the PCO part of dpcore.
 *
 * More specifically, checks of PCO's "SC2_Cam.dll" is loaded and retrieves
 * pointers to the functions that are used in dpcore_register_camhandle.
 *
 * @return dp_status
 * @retval dp_success if all goes well
 * @retval dp_not_initialized if SC2_Cam.dll is not in memory.
 * @retval dp_unknown_error if there is a SC2_Cam.dll in memory, but it does
 *      not expose the expected functions.
 */
dp_status dpcore_pco_init();

/**
 * @brief Register camera handle for later noise replacement.
 *
 * Retrieves camera serial number, shutter mode, pixel rate and noise filter
 * setting using calls to PCO.SDK. A camera identifier is constructed from the
 * return values and looked up in the parameter registry, which contains all
 * parameter sets that have been loaded using dpcore_init() or
 * dpcore_load_parameters().
 *
 * @todo This has so far only been tested with a pco.edge 5.5 camera.
 *
 * @param ph Handle of the camera.
 * @return dp_status
 * @retval dp_success if the constructed identifier matches an identifier in
 *    the registry.
 * @retval dp_unknown_identifier if the identifier does not have a matching
 *    set of parameters in the registry.
 * @retval dp_unknown_error if the calls to PCO.SDK failed.
 */
dp_status dpcore_register_camhandle(void* ph);

/**
 * @brief Perform image preparation based on parameters associated to the given
 * camera handle.
 *
 * @param imgbuf Contains the pixel values that will be overwritten by the
 * perparation procedure.
 * @param imgsize Number of pixels in the image.
 * @param ph Handle of the camera that produced the image data.
 * @param error_bound Maximum pixel modification in units of standard
 * deviations.
 * @retval dp_success if the image data was prepared now or previously.
 * @retval dp_unknown_identifier If no parameters could be found for the given
 * identifier.
 * @retval dp_memory_error If not enough memory could be allocated for the
 * preparation.
 * @retval dp_image_too_small if there are not enough pixels for reliable
 * preparation.
 */
dp_status dpcore_prepare_image_with_camhandle(uint16_t* imgbuf, int32_t imgsize,
                                              void* ph, float error_bound = 1);

#endif /* WIN32 */

#ifdef __cplusplus
} /* extern C */
#endif

