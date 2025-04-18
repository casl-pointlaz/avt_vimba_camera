#-------------------------------------------------------------------------------
# Ensure Dependencies
#-------------------------------------------------------------------------------
if (NOT TARGET railcam::imgproc)
    message( FATAL_ERROR "railcam::imgproc could not be found")
endif()
if (NOT TARGET railcam::testrunner)
    message( FATAL_ERROR "railcam::testrunner could not be found")
endif()
if (NOT TARGET railcam::testsassets)
    message( FATAL_ERROR "railcam::testsassets could not be found")
endif()
if (NOT TARGET railcam::testsutils)
    message( FATAL_ERROR "railcam::testsutils could not be found")
endif()


#-------------------------------------------------------------------------------
# Add test executable
#-------------------------------------------------------------------------------
file(GLOB_RECURSE tests_src src/*.test.cpp)
add_executable(tests_imgproc ${tests_src})
target_link_libraries(tests_imgproc PRIVATE railcam::testrunner railcam::testsassets railcam::testsutils railcam::imgproc)
target_include_directories(tests_imgproc PRIVATE include/)
catch_discover_tests(tests_imgproc)


