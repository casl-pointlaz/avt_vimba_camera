#-------------------------------------------------------------------------------
# Ensure Dependencies
#-------------------------------------------------------------------------------
if (NOT TARGET Eigen3::Eigen)
    message( FATAL_ERROR "Eigen3 could not be found")
endif()

#-------------------------------------------------------------------------------
# CMAKE CONFIGURATIONS
#-------------------------------------------------------------------------------
# No config

#-------------------------------------------------------------------------------
# Add test executable
#-------------------------------------------------------------------------------
#file(GLOB_RECURSE tests_utils_src src/*.cpp )
add_library(tests_utils INTERFACE)
target_link_libraries(tests_utils INTERFACE Eigen3::Eigen)
target_include_directories(tests_utils INTERFACE include/)
add_library(railcam::testsutils ALIAS tests_utils)