message(STATUS "Adding railcam::core module")

#-------------------------------------------------------------------------------
# CMAKE OPTIONS
#-------------------------------------------------------------------------------
# No options yet

#-------------------------------------------------------------------------------
# CMAKE VARIABLES
#-------------------------------------------------------------------------------
# No variable yet

#-------------------------------------------------------------------------------
# Add external components
#-------------------------------------------------------------------------------
# No external component

#-------------------------------------------------------------------------------
# Ensure requirements
#-------------------------------------------------------------------------------
set(APP_LIBRARIES )

foreach(lib IN LISTS APP_LIBRARIES)
    if(NOT TARGET ${lib})
        message(FATAL_ERROR "Target '${lib}' could not be found")
    endif()
endforeach()

#-------------------------------------------------------------------------------
# CMAKE CONFIGURATIONS
#-------------------------------------------------------------------------------
configure_file(include/railcam/core/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/generated/railcam/core/version.h)

#-------------------------------------------------------------------------------
# Build core
#-------------------------------------------------------------------------------
#file(GLOB_RECURSE src_files src/*.cpp)
add_library(railcam_core INTERFACE)
target_link_libraries(railcam_core INTERFACE ${APP_LIBRARIES})
target_include_directories(railcam_core INTERFACE include/ ${CMAKE_CURRENT_BINARY_DIR}/generated/)
add_library(railcam::core ALIAS railcam_core)

