include(FetchContent)

FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
    URL_HASH SHA256=7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    SYSTEM)  # their headers must not trigger our warnings-as-errors
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(dp_tests
    black76_test.cpp
    implied_vol_test.cpp
    deribit_test.cpp)
target_link_libraries(dp_tests PRIVATE dp::core GTest::gtest_main)
dp_set_warnings(dp_tests)

include(GoogleTest)
gtest_discover_tests(dp_tests)

# The CLI is tested end-to-end on the committed Deribit snapshot.
add_test(NAME cli_prices_deribit_sample
    COMMAND ${CMAKE_COMMAND}
        -DCLI=$<TARGET_FILE:dp_cli>
        -DINPUT=${PROJECT_SOURCE_DIR}/data/btc_options_sample.csv
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_cli.cmake)
add_test(NAME cli_rejects_unknown_solver COMMAND dp_cli --solver secant)
set_tests_properties(cli_rejects_unknown_solver PROPERTIES WILL_FAIL ON)
