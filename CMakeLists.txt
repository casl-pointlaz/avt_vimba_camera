cmake_minimum_required(VERSION 2.8.3)
project(avt_vimba_camera)

find_package(catkin REQUIRED COMPONENTS
  camera_info_manager
  diagnostic_updater
  dynamic_reconfigure
  cv_bridge
  image_transport
  message_filters
  roscpp
  sensor_msgs
  std_msgs
  nodelet
)

set(CMAKE_PREFIX_PATH "/opt/libjpeg-turbo/lib64/cmake/libjpeg-turbo/")
find_package(libjpeg-turbo REQUIRED)


set(THREADPOOL_INCLUDE_DIR " ${CMAKE_CURRENT_SOURCE_DIR}/include/thread_pool/include")

set(JETRAW_INCLUDE_DIR " ${CMAKE_CURRENT_SOURCE_DIR}/JetrawWithDPCore-22.02.16.1/include")

set (JETRAW_LIBS
        ${CMAKE_CURRENT_SOURCE_DIR}/JetrawWithDPCore-22.02.16.1/lib/libcrypto.so.1.1
        ${CMAKE_CURRENT_SOURCE_DIR}/JetrawWithDPCore-22.02.16.1/lib/libdpcore.so
        ${CMAKE_CURRENT_SOURCE_DIR}/JetrawWithDPCore-22.02.16.1/lib/libjetraw.so
        ${CMAKE_CURRENT_SOURCE_DIR}/JetrawWithDPCore-22.02.16.1/lib/libjetraw_tiff.so
        ${CMAKE_CURRENT_SOURCE_DIR}/JetrawWithDPCore-22.02.16.1/lib/libssl.so.1.1)

#Get architecture
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(TargetArchitecture)
target_architecture(ARCH)

#add dynamic reconfigure api
generate_dynamic_reconfigure_options(
        cfg/AvtVimbaCamera.cfg
)

catkin_package(
  INCLUDE_DIRS include
  CATKIN_DEPENDS camera_info_manager diagnostic_updater dynamic_reconfigure image_transport roscpp sensor_msgs std_msgs
)

###########
## Build ##
###########
message(STATUS "=====================${libjpeg-turbo}")

        include_directories(
  ${THREADPOOL_INCLUDE_DIR}
  ${catkin_INCLUDE_DIRS}
  ${JETRAW_INCLUDE_DIR}
  include
)

# C++11 support
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
elseif(COMPILER_SUPPORTS_CXX0X)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
  message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++17 support. Please use a different C++ compiler.")
endif()

function(add_dependencies_and_linkings arg)
  add_dependencies(${arg}
    ${PROJECT_NAME}_gencfg
  )

  if("${ARCH}" STREQUAL x86_64)
    target_link_libraries(${arg}
      ${catkin_LIBRARIES}
      ${JETRAW_LIBS}
      libjpeg-turbo::turbojpeg
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/64bit/libVimbaC.so
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/64bit/libVimbaCPP.so
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/64bit/libVimbaImageTransform.so
    )
  elseif("${ARCH}" STREQUAL armv7)
    target_link_libraries(${arg}
      ${catkin_LIBRARIES}
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/arm_32bit/libVimbaC.so
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/arm_32bit/libVimbaCPP.so
    )
  elseif("${ARCH}" STREQUAL armv8)
    target_link_libraries(${arg}
      ${catkin_LIBRARIES}
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/arm_64bit/libVimbaC.so
      ${CMAKE_CURRENT_SOURCE_DIR}/lib/arm_64bit/libVimbaCPP.so
    )
  else()
    message(FATAL_ERROR "[libvimba]: Architecture ${ARCH} not suported. Exiting...")
  endif()
endfunction(add_dependencies_and_linkings)

# Nodelets
add_library(avt_camera_nodelets
        src/avt_vimba_camera.cpp
        src/frame_observer.cpp
)
add_dependencies_and_linkings(avt_camera_nodelets)

add_executable(trigger_node
  src/nodes/trigger_node.cpp
  src/trigger.cpp
)
add_dependencies_and_linkings(trigger_node)

# Node added by pointlaz
add_executable(multi_camera_node
        src/nodes/multi_camera_node.cpp
        src/multi_camera.cpp
        src/avt_vimba_camera.cpp
        src/frame_observer.cpp
        src/jetraw.cpp
        src/jpegTurbo.cpp
        )
add_dependencies_and_linkings(multi_camera_node)

# Node added by pointlaz
add_executable(publish_jpg_to_images_topics
        src/nodes/publish_jpg_to_images_topics.cpp
)
add_dependencies_and_linkings(publish_jpg_to_images_topics)



#############
## Install ##
#############

## Mark executables and/or libraries for installation
install(TARGETS
  trigger_node
  avt_camera_nodelets
  multi_camera_node     # Added by pointlaz
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

## Mark cpp header files for installation
install(DIRECTORY include
  DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
  FILES_MATCHING PATTERN "*.h"
)

## Mark other files for installation (e.g. launch and bag files, etc.)
install(FILES
  README.md
  plugins.xml
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
)

install(DIRECTORY
  launch
  calibrations
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
)

if("${ARCH}" STREQUAL x86_64)
  install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/64bit/libVimbaC.so
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/64bit/libVimbaCPP.so
    DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  )
elseif("${ARCH}" STREQUAL armv7)
  install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/arm_32bit/libVimbaC.so
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/arm_32bit/libVimbaCPP.so
    DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  )
elseif("${ARCH}" STREQUAL armv8)
  install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/arm_64bit/libVimbaC.so
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/arm_64bit/libVimbaCPP.so
    DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  )
endif()