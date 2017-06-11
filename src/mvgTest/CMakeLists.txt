cmake_minimum_required (VERSION 2.6)
project (mvgTest)

set(OpenMVG_DIR "/Users/espennordahl/openMVG/install/share/openMVG/cmake") 
set(CMAKE_BUILD_TYPE "Debug") 
ADD_DEFINITIONS(
    -std=c++11
)


find_package(OpenMVG REQUIRED)
include_directories(${OPENMVG_INCLUDE_DIRS})

add_executable(mvgTest mvgTest.cpp)

MESSAGE(STATUS "OPENMVG LIBRARIES: " ${OPENMVG_LIBRARIES})

target_link_libraries(mvgTest ${OPENMVG_LIBRARIES})

