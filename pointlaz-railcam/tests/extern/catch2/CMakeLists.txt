Include(FetchContent)
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        ${catch2_VERSION}
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
set(CATCH_INSTALL_DOCS OFF CACHE BOOL "Disable building Catch2 docs")
FetchContent_MakeAvailable(Catch2)