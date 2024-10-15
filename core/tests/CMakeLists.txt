include(FetchContent)


# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

find_package(GTest CONFIG QUIET)
if (NOT GTest_FOUND)
    FetchContent_Declare(
            GTest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.15.2
            OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(GTest)
endif ()

message(STATUS "${GTest_FOUND}")


enable_testing()

include_directories(${PROJECT_SOURCE_DIR}/include)
add_executable(test_pchip
        main.cpp
        test_pchip.cpp
)
target_link_libraries(test_pchip gtest gtest_main Eigen3::Eigen fmt::fmt-header-only)
add_test(NAME test_pchip COMMAND test_pchip)


