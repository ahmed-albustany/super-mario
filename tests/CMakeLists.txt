# =============================================================================
# Unit Tests
# =============================================================================
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
    GIT_SHALLOW    TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

function(mario_add_test TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} PRIVATE
        MarioGameLib
        GTest::gtest_main
    )
    target_compile_options(${TEST_NAME} PRIVATE ${MARIO_WARNING_FLAGS})
    gtest_discover_tests(${TEST_NAME})
endfunction()

mario_add_test(test_physics       test_physics.cpp)
mario_add_test(test_collision     test_collision.cpp)
mario_add_test(test_statemachine  test_statemachine.cpp)
mario_add_test(test_levelloader   test_levelloader.cpp)
