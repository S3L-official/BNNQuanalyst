cmake_minimum_required(VERSION 3.1)

project(sylvan VERSION 1.6.0)

set(CMAKE_INSTALL_PREFIX  ${PROJECT_SOURCE_DIR})
set(PROJECT_DESCRIPTION "Sylvan, a parallel decision diagram library")
set(PROJECT_URL "https://github.com/trolando/sylvan")

message(STATUS "CMake build configuration for Sylvan ${PROJECT_VERSION}")
enable_language(C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

# Some info
message(STATUS "OS: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/")

set(CMAKE_C_FLAGS "-O2 -Wextra -Wall -Werror -fno-strict-aliasing ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-O2 -Wextra -Wall -Werror -fno-strict-aliasing -Wno-deprecated ${CMAKE_CXX_FLAGS}")

option(WITH_COVERAGE "Add generation of test coverage" OFF)
if(WITH_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g -coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -coverage")

    find_program(GCOV_PATH gcov)
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)

    add_custom_target(coverage
        # Cleanup lcov
        ${LCOV_PATH} --directory . --zerocounters
        # Run tests
        COMMAND make test
        # Capture counters
        COMMAND ${LCOV_PATH} --gcov-tool ${GCOV_PATH} --directory . --capture --output-file coverage.info
        COMMAND ${LCOV_PATH} --remove coverage.info 'test/*' '/usr/*' 'examples/*' 'src/lace*' 'src/sha2.c' --output-file coverage.info.cleaned
        COMMAND ${GENHTML_PATH} -o coverage coverage.info.cleaned
        COMMAND ${CMAKE_COMMAND} -E remove coverage.info coverage.info.cleaned
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include(GNUInstallDirs)
include(CTest)

add_subdirectory(src)
#add_subdirectory(test)

option(SYLVAN_BUILD_EXAMPLES "Build example tools" OFF)
if(SYLVAN_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

option(SYLVAN_BUILD_DOCS "Build documentation" OFF)
if(SYLVAN_BUILD_DOCS)
    configure_file("docs/conf.py.in" "docs/conf.py" @ONLY)
    find_package(Sphinx REQUIRED)
    Sphinx_add_targets(sylvan ${CMAKE_CURRENT_BINARY_DIR}/docs ${CMAKE_CURRENT_SOURCE_DIR}/docs ${CMAKE_CURRENT_BINARY_DIR})
    add_custom_target(update_gh_pages
        COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_MODULE_PATH}/UpdateGHPages.cmake")
    add_dependencies(update_gh_pages sylvan_html)
endif()

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Multi-core implementation of decision diagrams")
set(CPACK_PACKAGE_VENDOR "Tom van Dijk")
set(CPACK_PACKAGE_CONTACT "Tom van Dijk <tom@tvandijk.nl>")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_MAJOR_VERSION})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_MINOR_VERSION})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_PATCH_VERSION})
include(CPack)

# pkg-config
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/sylvan.pc.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/sylvan.pc" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/sylvan.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
