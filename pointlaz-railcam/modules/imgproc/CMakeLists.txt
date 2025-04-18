message(STATUS "Adding railcam::imgproc module")

#-------------------------------------------------------------------------------
# CMAKE OPTIONS
#-------------------------------------------------------------------------------
# No options yet

#-------------------------------------------------------------------------------
# CMAKE VARIABLES
#-------------------------------------------------------------------------------
# No variable yet

#-------------------------------------------------------------------------------
# CMAKE CONFIGURATIONS
#-------------------------------------------------------------------------------
# No configuration

#-------------------------------------------------------------------------------
# Add external components
#-------------------------------------------------------------------------------
# Read Versions
ReadDependencyVersion(eigen3 ${PYPROJECT_PATH})

# Include extern
add_subdirectory(extern/eigen3)

#-------------------------------------------------------------------------------
# Ensure requirements
#-------------------------------------------------------------------------------
set(APP_LIBRARIES railcam::core Eigen3::Eigen)

foreach(lib IN LISTS APP_LIBRARIES)
    if(NOT TARGET ${lib})
        message(FATAL_ERROR "Target '${lib}' could not be found")
    endif()
endforeach()

#-------------------------------------------------------------------------------
# Build target
#-------------------------------------------------------------------------------
#file(GLOB_RECURSE src_files src/*.cpp)
add_library(railcam_imgproc INTERFACE)
target_link_libraries(railcam_imgproc INTERFACE ${APP_LIBRARIES})
target_include_directories(railcam_imgproc INTERFACE include/)
add_library(railcam::imgproc ALIAS railcam_imgproc)

