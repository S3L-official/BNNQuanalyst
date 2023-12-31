set(SOURCES
    lace.c
    sha2.c
    sylvan_bdd.c
    sylvan_cache.c
    sylvan_common.c
    sylvan_gmp.c
    sylvan_hash.c
    sylvan_ldd.c
    sylvan_mt.c
    sylvan_mtbdd.c
    sylvan_obj.cpp
    sylvan_refs.c
    sylvan_sl.c
    sylvan_stats.c
    sylvan_table.c
)

set(HEADERS
    lace.h
    sylvan.h
    sylvan_bdd.h
    sylvan_cache.h
    sylvan_config.h
    sylvan_common.h
    sylvan_gmp.h
    sylvan_hash.h
    sylvan_int.h
    sylvan_ldd.h
    sylvan_ldd_int.h
    sylvan_mt.h
    sylvan_mtbdd.h
    sylvan_mtbdd_int.h
    sylvan_obj.hpp
    sylvan_stats.h
    sylvan_table.h
    sylvan_tls.h
)

option(BUILD_SHARED_LIBS "Enable/disable creation of shared libraries" ON)
option(BUILD_STATIC_LIBS "Enable/disable creation of static libraries" ON)

add_library(sylvan ${SOURCES})

find_package(GMP REQUIRED)

target_include_directories(sylvan PRIVATE ${GMP_INCLUDE_DIR})

target_link_libraries(sylvan m pthread ${GMP_LIBRARIES})

if(UNIX AND NOT APPLE)
    target_link_libraries(sylvan rt)
endif()

option(SYLVAN_STATS "Collect statistics" OFF)
if(SYLVAN_STATS)
    set_target_properties(sylvan PROPERTIES COMPILE_DEFINITIONS "SYLVAN_STATS")
endif()


set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../libs/libsSYLVAN)
#install(TARGETS sylvannew DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(FILES ${HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}") # No use on CLion
#install(TARGETS sylvannew DESTINATION ${PROJECT_SOURCE_DIR}/lib)