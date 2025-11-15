cmake_minimum_required(VERSION 3.20)

include(FetchContent) 

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.11.0
)

FetchContent_MakeAvailable(Catch2)

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
