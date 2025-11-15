cmake_minimum_required(VERSION 3.20)

# Assuming Catch2 is vendored or fetched some other way.
# For example, using FetchContent, or add_subdirectory(external/catch2).

add_executable(sonnet_tests
    tests.cpp
    # ... all your test files
)

target_link_libraries(sonnet_tests
    PRIVATE
        Sonnet::sonnet
        Catch2::Catch2WithMain
)

add_test(NAME sonnet_tests COMMAND sonnet_tests)
