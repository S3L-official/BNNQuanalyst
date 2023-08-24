aux_source_directory(${PROJECT_SOURCE_DIR}/cudd-3.0.0/util/. SRC_LIST_UTIL)
aux_source_directory(${PROJECT_SOURCE_DIR}/cudd-3.0.0/st/. SRC_LIST_ST)
aux_source_directory(${PROJECT_SOURCE_DIR}/cudd-3.0.0/mtr/. SRC_LIST_MTR)
aux_source_directory(${PROJECT_SOURCE_DIR}/cudd-3.0.0/epd/. SRC_LIST_EPD)
aux_source_directory(${PROJECT_SOURCE_DIR}/cudd-3.0.0/cudd/. SRC_LIST_CUDD)
aux_source_directory(${PROJECT_SOURCE_DIR}/cudd-3.0.0/cplusplus/. SRC_LIST_CUDDCPP)
aux_source_directory(${PROJECT_SOURCE_DIR}/cudd-3.0.0/dddmp/. SRC_LIST_DDDMP)

add_library(cudd_util STATIC ${SRC_LIST_UTIL})
add_library(cudd_st STATIC ${SRC_LIST_ST})
add_library(cudd_mtr STATIC ${SRC_LIST_MTR})
add_library(cudd_epd STATIC ${SRC_LIST_EPD})
add_library(cudd_ddd STATIC ${SRC_LIST_DDDMP})
add_library(cudd STATIC ${SRC_LIST_CUDD})
add_library(cudd_cpp STATIC ${SRC_LIST_CUDDCPP})

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/libs/libsCUDD)

