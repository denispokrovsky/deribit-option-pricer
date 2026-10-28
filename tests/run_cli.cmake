# Runs dp_cli with INPUT on stdin and checks that most quotes were priced.
# Usage: cmake -DCLI=<path> -DINPUT=<csv> -P run_cli.cmake

execute_process(
    COMMAND ${CLI}
    INPUT_FILE ${INPUT}
    OUTPUT_VARIABLE output
    ERROR_VARIABLE diagnostics
    RESULT_VARIABLE exit_code)

if(NOT exit_code EQUAL 0)
    message(FATAL_ERROR "dp_cli exited with ${exit_code}:\n${diagnostics}")
endif()

string(REGEX MATCH "priced ([0-9]+), skipped ([0-9]+)" summary "${diagnostics}")
set(priced ${CMAKE_MATCH_1})
set(skipped ${CMAKE_MATCH_2})
message(STATUS "dp_cli: priced ${priced}, skipped ${skipped}")

math(EXPR total "${priced} + ${skipped}")
math(EXPR priced_percent "100 * ${priced} / ${total}")
if(priced_percent LESS 90)
    message(FATAL_ERROR "only ${priced_percent}% of quotes were priced")
endif()
