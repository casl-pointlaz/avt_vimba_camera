/**
 * @file dpcore.h
 *  @author Christoph Clausen (christoph.clausen@dotphoton.com)
 * @brief Dotphoton Core API for performing preparing images that can be
 *      losslessly compressed at a later stage using Dotphoton Jetraw.
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

#ifdef _WIN32
#define CHARTYPE wchar_t
#else
#define CHARTYPE char
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Try to load noise parameter files from default locations.
 *
 * Default locations are the current working directory, as well as an
 * OS-dependent user data directory:
 * - Windows: `%%APPDATA%\dpcore`
 * - macOS: `$HOME/Library/Application Support/dpcore`
 *
 * Parameter files are expected to have the extension `.dat`
 *
 * @return int The number of parameter files loaded.
 */
int dpcore_init();

/**
 * @brief Set logging verbosity.
 * By default, log output is sent to stdout. It can be redirected to a file
 * by calling dpcore_set_logfile.
 *
 * @param level 0=Off, 1=Informational, 2=Debugging
 */
void dpcore_set_loglevel(int level);

/**
 * @brief Write log output to file instead of to stdout.
 *
 * The log output will be written to the end of the file, i.e. the file
 * is not overwritten.
 *
 * @param file_path
 * @retval dp_success if all goes well
 * @retval dp_file_write_error on failure.
 */
dp_status dpcore_set_logfile(const CHARTYPE* file_path);

/**
 * @brief Manually load noise parameters from a file.
 *
 * @param file_path
 * @retval dp_success if parameters were successfully retrieved.
 * @retval dp_file_error if the file could not be opened.
 * @retval dp_file_corrupt if the file could not be parsed.
 */
dp_status dpcore_load_parameters(const CHARTYPE* file_path);

/**
 * @brief Prepare an image for efficient lossless compression.
 *
 * Note: An image that already was prepared will not be modified a
 * a second time. Still, the function will return dp_success.
 *
 * @param imgbuf Contains the pixel values that will be overwritten by the
 * perparation procedure.
 * @param imgsize Number of pixels in the image.
 * @param identifier Used to look up preparation parameters.
 * @param error_bound Maximum pixel modification in units of standard
deviations.
 * @retval dp_success if the image data was prepared now or previously.
 * @retval dp_unknown_identifier If no parameters could be found for the given
identifier.
 * @retval dp_memory_error If not enough memory could be allocated for the
preparation.
 * @retval dp_image_too_small if there are not enough pixels for reliable
preparation.
 */
dp_status dpcore_prepare_image(uint16_t* imgbuf, int32_t imgsize,
                               const char* identifier, float error_bound = 1);

/**
 * @brief Prepare an image for efficient lossless compression, but instead of
 * performing the full noise replacement, only the meta data is embedded. This
 * can be useful in situations where the image data is going to be compressed
 * immediately and avoids redundant quantization/de-quantization.
 *
 * Note: An image that already was prepared will not be modified a
 * a second time. Still, the function will return dp_success.
 *
 * @param imgbuf Contains the pixel values that will be overwritten by the
 * perparation procedure.
 * @param imgsize Number of pixels in the image.
 * @param identifier Used to look up preparation parameters.
 * @param error_bound Maximum pixel modification in units of standard
deviations.
 * @retval dp_success if the image data was prepared now or previously.
 * @retval dp_unknown_identifier If no parameters could be found for the given
identifier.
 * @retval dp_memory_error If not enough memory could be allocated for the
preparation.
 * @retval dp_image_too_small if there are not enough pixels for reliable
preparation.
 */
dp_status dpcore_embed_meta(uint16_t* imgbuf, int32_t imgsize,
                              const char* identifier, float error_bound = 1);

/**
 * Return the number of registered camera identifers.
 */
int dpcore_identifier_count();

/**
 * Write list of camera identifers to preallocated buffer.
 * @param buf Output buffer. Cannot be null. When the functions succeeds, the
 * buffer contains a sequantial list of null-terminated identifiers.
 * @param bufsize Pointer to the size of buf (in bytes). When the function
 * returns, error or not, the pointee will contain the total number of bytes
 * needed for all identifiers, including null-terminators. Hence, calling the
 * function with *bufsize = 0 allows to determine the required size of buf.
 * @retval dp_success if all went well.
 * @retval dp_parameter_error if bufsize is null.
 * @retval dp_memory_error if buffer is too small or buf is null.
 */
dp_status dpcore_get_identifiers(char* buf, int* bufsize);

#ifdef __cplusplus
}
#endif /* extern C */
