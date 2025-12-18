cmake_minimum_required(VERSION 3.29)
project(henry_reinhardt)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)

set(EIGEN_BUILD_DOC OFF CACHE BOOL "Disable Eigen docs" FORCE)
set(EIGEN_BUILD_PKGCONFIG OFF CACHE BOOL "Disable Eigen pkg-config" FORCE)
set(EIGEN_BUILD_TESTING OFF CACHE BOOL "Disable Eigen tests" FORCE)

FetchContent_Declare(
        eigen
        GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
        GIT_TAG 3.4.0
)

FetchContent_MakeAvailable(eigen)

FetchContent_Declare(
        cppoptlib
        GIT_REPOSITORY https://github.com/PatWie/CppNumericalSolvers.git
        GIT_TAG 1.0.0
)

FetchContent_MakeAvailable(cppoptlib)


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

    target_link_libraries(core PRIVATE Eigen3::Eigen embind)
    target_compile_options(core PRIVATE
            -Os
    )
    target_compile_definitions(core PRIVATE
            EIGEN_NO_DEBUG
            EIGEN_MPL2_ONLY
    )
    target_link_options(core PRIVATE
            -Os
            -flto
            -sWASM=1
            -sALLOW_MEMORY_GROWTH=0
            -sASSERTIONS=0
            -sSAFE_HEAP=0
            -sSTACK_OVERFLOW_CHECK=0
            -sDISABLE_EXCEPTION_CATCHING=1
            -sNO_FILESYSTEM=1
            -sFILESYSTEM=0
            -sMODULARIZE=1
            -sEXPORT_ES6=1
            -sEXPORT_NAME=core
            -sMALLOC=emmalloc
            -sABORTING_MALLOC=1
            -sELIMINATE_DUPLICATE_FUNCTIONS=1
            --emit-tsd=core.d.ts
    )
endif ()

