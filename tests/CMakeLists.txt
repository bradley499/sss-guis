cmake_minimum_required(VERSION 3.14)

find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(unit_tests
    test_dependencies.cpp
    test_structure.cpp
    test_generation.cpp
    test_guis.cpp
)

target_link_libraries(unit_tests PRIVATE
    ${LIBRARY_NAME}
    GTest::gtest
    GTest::gtest_main
    yaml-cpp::yaml-cpp
    nlohmann_json::nlohmann_json
    Threads::Threads
)

target_compile_features(unit_tests PRIVATE cxx_std_17)
target_compile_options(unit_tests PRIVATE -Wall -Wextra -Wpedantic)

gtest_discover_tests(unit_tests)
