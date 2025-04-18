#-------------------------------------------------------------------------------
# Ensure Dependencies
#-------------------------------------------------------------------------------
# No dependency

#-------------------------------------------------------------------------------
# CMAKE CONFIGURATIONS
#-------------------------------------------------------------------------------
set(SOURCE_ASSETS_DIR "./")
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(RAILCAM_TEST_ASSETSDIR "${GENERATED_DIR}/railcam/testsassets")
file(GLOB_RECURSE ASSET_FILES "${SOURCE_ASSETS_DIR}/*")
list(FILTER ASSET_FILES EXCLUDE REGEX "\\.in$")
file(COPY ${ASSET_FILES} DESTINATION ${RAILCAM_TEST_ASSETSDIR})
configure_file(./assets_path.h.in ${RAILCAM_TEST_ASSETSDIR}/assets_path.h)
# assets_path.h will be accessible via include "railcam/testsassets/assets_path.h"

#-------------------------------------------------------------------------------
# Add test executable
#-------------------------------------------------------------------------------
add_library(tests_assets INTERFACE)
target_include_directories(tests_assets INTERFACE include/ ${GENERATED_DIR}/)
add_library(railcam::testsassets ALIAS tests_assets)