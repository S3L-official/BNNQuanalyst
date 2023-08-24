aux_source_directory(${PROJECT_SOURCE_DIR}/BNNQuanalystSRC/. SRC_LIST)

add_library(BNNQuanalyst_src STATIC ${SRC_LIST})

target_link_libraries(BNNQuanalyst_src cudd cudd_ddd cudd_cpp cudd_mtr cudd_st cudd_util cudd_epd sylvan)

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/libs/libsBNNQuanalyst)

