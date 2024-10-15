cmake_minimum_required(VERSION 3.29)
project(henry_reinhardt)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)



FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt
        GIT_TAG        e69e5f977d458f2650bb346dadf2ad30c5320281) # 10.2.1
FetchContent_MakeAvailable(fmt)

cmake_policy(SET CMP0024 OLD)
find_package (Eigen3 3.4 CONFIG QUIET)
if (NOT Eigen3_FOUND)
    FetchContent_Declare(
            Eigen3
            GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
            GIT_TAG 3.4
            GIT_SHALLOW TRUE
            GIT_PROGRESS TRUE
            OVERRIDE_FIND_PACKAGE
    )
    set(BUILD_TESTING OFF)
    set(EIGEN_BUILD_TESTING OFF)
    set(EIGEN_MPL2_ONLY ON)
    set(EIGEN_BUILD_DOC OFF)
    FetchContent_MakeAvailable(Eigen3)
endif ()


FetchContent_Declare(
        LBFGSpp
        GIT_REPOSITORY https://github.com/yixuan/LBFGSpp.git
        GIT_TAG v0.3.0
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(LBFGSpp)

if (BUILD_TESTS)
    message(STATUS "BUILD_TESTS=${BUILD_TESTS}")
    add_subdirectory(tests)
endif ()


include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
add_executable(henry_reinhardt_main main.cpp
        include/spline.h
        include/helpers.h
        include/pchip.h
        include/hr.h
        include/optimize.h)
target_link_libraries(henry_reinhardt_main fmt::fmt Eigen3::Eigen lbfgspp)