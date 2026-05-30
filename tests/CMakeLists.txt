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
    if(WIN32)
        add_custom_command(TARGET ${TEST_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${TEST_NAME}>
                $<TARGET_FILE_DIR:${TEST_NAME}>
            COMMAND_EXPAND_LISTS
        )
        gtest_discover_tests(${TEST_NAME}
            DISCOVERY_TIMEOUT 30
            DISCOVERY_MODE PRE_TEST
        )
    else()
        gtest_discover_tests(${TEST_NAME})
    endif()
endfunction()

mario_add_test(test_physics       test_physics.cpp)
mario_add_test(test_collision     test_collision.cpp)
mario_add_test(test_statemachine  test_statemachine.cpp)
mario_add_test(test_levelloader   test_levelloader.cpp)
