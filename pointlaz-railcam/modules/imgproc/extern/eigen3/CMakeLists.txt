include(FetchContent)
FetchContent_Declare(
        Eigen3
        GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
        GIT_TAG ${eigen3_VERSION}
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE)
set(EIGEN_BUILD_DOC OFF CACHE BOOL "Disable building Eigen documentation")
set(BUILD_TESTING OFF CACHE BOOL "Disable building Eigen tests")
FetchContent_MakeAvailable(Eigen3)
