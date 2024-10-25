
CPMAddPackage(
        NAME googletest
        GITHUB_REPOSITORY google/googletest
        GIT_TAG v1.15.2
        OPTIONS "INSTALL_GTEST OFF" "gtest_force_shared_crt"
)

enable_testing()

include_directories(${PROJECT_SOURCE_DIR}/include)
add_executable(test_pchip
        main.cpp
        test_pchip.cpp
)
target_link_libraries(test_pchip gtest gtest_main Eigen3::Eigen fmt::fmt-header-only)
add_test(NAME test_pchip COMMAND test_pchip)


