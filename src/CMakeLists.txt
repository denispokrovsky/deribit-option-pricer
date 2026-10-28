add_library(dp_core
    black76.cpp
    implied_vol.cpp
    instrument.cpp
    quote_csv.cpp
    quote_pricer.cpp)
add_library(dp::core ALIAS dp_core)

target_include_directories(dp_core PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_compile_features(dp_core PUBLIC cxx_std_20)
dp_set_warnings(dp_core)
