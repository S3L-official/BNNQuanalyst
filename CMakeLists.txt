cmake_minimum_required(VERSION 3.0)
project(BNNQuanalyst LANGUAGES "CXX" "C")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS -pthread)

include_directories(${PROJECT_SOURCE_DIR}/cudd-3.0.0)
include_directories(${PROJECT_SOURCE_DIR}/cudd-3.0.0/cudd)
include_directories(${PROJECT_SOURCE_DIR}/cudd-3.0.0/st)
include_directories(${PROJECT_SOURCE_DIR}/cudd-3.0.0/mtr)
include_directories(${PROJECT_SOURCE_DIR}/cudd-3.0.0/util)
include_directories(${PROJECT_SOURCE_DIR}/cudd-3.0.0/epd)
include_directories(${PROJECT_SOURCE_DIR}/cudd-3.0.0/cplusplus)
include_directories(${PROJECT_SOURCE_DIR}/cudd-3.0.0/dddmp)
include_directories(${PROJECT_SOURCE_DIR}/sylvan-master/src)
include_directories(include)

add_subdirectory(libs/libsCUDD)
add_subdirectory(sylvan-master)
add_subdirectory(libs/libsBNNQuanalyst)

#set(CMAKE_BUILD_RPATH /build)
set(CMAKE_INSTALL_RPATH /build)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

add_executable(BNNQuanalyst main.cpp)

target_link_libraries(BNNQuanalyst cudd_ddd cudd_cpp cudd cudd_mtr cudd_st cudd_util cudd_epd sylvan BNNQuanalyst_src)