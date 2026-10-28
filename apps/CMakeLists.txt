add_executable(dp_cli dp_cli.cpp)
target_link_libraries(dp_cli PRIVATE dp::core)
dp_set_warnings(dp_cli)

install(TARGETS dp_cli RUNTIME DESTINATION bin)
