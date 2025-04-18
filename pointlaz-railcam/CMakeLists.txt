cmake_minimum_required(VERSION 3.16)

#-------------------------------------------------------------------------------
# Project Definitions
#-------------------------------------------------------------------------------
project(RAILCAM
        DESCRIPTION "The PointLaz RailCam Camera Software is designed to perform 3D measurements of elevator rails as part of the Lazaruss scanner."
        LANGUAGES CXX C)

#-------------------------------------------------------------------------------
# VERSIONING
#-------------------------------------------------------------------------------
add_subdirectory(cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(PYPROJECT_PATH "${CMAKE_CURRENT_BINARY_DIR}/pyproject.toml")
configure_file("${PROJECT_SOURCE_DIR}/pyproject.toml" ${PYPROJECT_PATH})
ReadVersion(${PYPROJECT_PATH})

#-------------------------------------------------------------------------------
# COMPILATION
#-------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Compiler-specific flags
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}  -O3 -fno-math-errno")
    set(CMAKE_CUDA_FLAGS_RELEASE "${CMAKE_CUDA_FLAGS_RELEASE} -O3 --use_fast_math")
    set(CMAKE_CXX_FLAGS_DEBUG  "-O0 -g")
    set(CMAKE_CUDA_FLAGS_DEBUG "${CMAKE_CUDA_FLAGS_DEBUG} -O0 -g")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /EHsc")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi")
endif()

# Do not allow to build in main repo
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
    message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file).
    Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

# Set the default build type
set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
            STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
            "Debug" "Release")
endif()

#-------------------------------------------------------------------------------
# CMAKE OPTIONS
#-------------------------------------------------------------------------------
option(RAILCAM_BUILD_TESTS "Enable the compilation of the test files." OFF)
option(RAILCAM_BUILD_DOCS "Enable the build of the documentation and website." OFF)

if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DDEBUG)
endif()

#-------------------------------------------------------------------------------
# CONFIGURATIONS
#-------------------------------------------------------------------------------
# No config yet

#-------------------------------------------------------------------------------
# Add components
#-------------------------------------------------------------------------------
set(RAILCAM_MODULES core imgproc)
foreach(module ${RAILCAM_MODULES})
    add_subdirectory(modules/${module})
endforeach()

#-------------------------------------------------------------------------------
# Apps
#-------------------------------------------------------------------------------
# No Apps

#-------------------------------------------------------------------------------
# Tests
#-------------------------------------------------------------------------------
if(RAILCAM_BUILD_TESTS)
    add_subdirectory(tests)
endif()

#-------------------------------------------------------------------------------
# Docs
#-------------------------------------------------------------------------------
if(RAILCAM_BUILD_DOCS)
    add_subdirectory(docs)
endif()

