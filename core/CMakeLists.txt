cmake_minimum_required(VERSION 3.29)
project(henry_reinhardt)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)

set(CPM_DOWNLOAD_VERSION 0.40.2)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake")
    file(DOWNLOAD https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif()

include(${CPM_DOWNLOAD_LOCATION})
CPMAddPackage(
        NAME cppoptlib
        GITHUB_REPOSITORY PatWie/CppNumericalSolvers
        GIT_TAG "1.0.0"
)
CPMAddPackage(
        GITLAB_REPOSITORY libeigen/eigen
        GIT_TAG 3.4.0
        OPTIONS "EIGEN_BUILD_DOC OFF"
        "EIGEN_BUILD_PKGCONFIG OF"
        "EIGEN_BUILD_DOC OFF"
        "EIGEN_BUILD_TESTING OFF"
)
CPMAddPackage("gh:fmtlib/fmt#11.0.2")



if (BUILD_TESTS)
    message(STATUS "BUILD_TESTS=${BUILD_TESTS}")
    add_subdirectory(tests)
endif ()


if (BUILD_MAIN)
    add_executable(henry_reinhardt_main main.cpp
            include/spline.h
            include/helpers.h
            include/pchip.h
            include/hr.h
            include/optimize.h)
    target_link_libraries(henry_reinhardt_main fmt::fmt Eigen3::Eigen)
endif ()

message(STATUS "CMAKE_C_COMPILER=${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include" "${cppoptlib_SOURCE_DIR}/include")
if (BUILD_WASM)
    add_executable(core
            include/spline.h
            include/helpers.h
            include/pchip.h
            include/hr.h
            include/optimize.h
            source/core.cpp)

    target_link_libraries(core PUBLIC fmt::fmt Eigen3::Eigen embind)
    set_target_properties(core PROPERTIES LINK_FLAGS "-O2 -s EXPORT_NAME='core' -s MODULARIZE=1 -s EXPORT_ES6=1 -s ENVIRONMENT='web' --emit-tsd core.d.ts")
endif ()

