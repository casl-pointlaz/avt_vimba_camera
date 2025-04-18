message(STATUS "Adding testing module" )

#-------------------------------------------------------------------------------
# Add external components
#-------------------------------------------------------------------------------
# Read Versions
ReadDependencyVersion(catch2 ${PYPROJECT_PATH})

# Include extern
add_subdirectory(extern/catch2)

#-------------------------------------------------------------------------------
# Ensure requirements
#-------------------------------------------------------------------------------
set(APP_LIBRARIES Catch2::Catch2)

foreach(lib IN LISTS APP_LIBRARIES)
    if(NOT TARGET ${lib})
        message(FATAL_ERROR "Target '${lib}' could not be found")
    endif()
endforeach()

#-------------------------------------------------------------------------------
# CMAKE CONFIGURATIONS
#-------------------------------------------------------------------------------
# Configure automatic test registration
list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
enable_testing()
include(CTest)
include(Catch)

#-------------------------------------------------------------------------------
# Add tests
#-------------------------------------------------------------------------------
add_library(test_runner main.cpp)
target_link_libraries(test_runner PUBLIC Catch2::Catch2)
add_library(railcam::testrunner ALIAS test_runner)

add_subdirectory(testsassets)
add_subdirectory(testsutils)
#add_subdirectory(core)
add_subdirectory(imgproc)
