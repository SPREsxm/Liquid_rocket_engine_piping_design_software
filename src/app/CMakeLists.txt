add_executable(LiquidRocketPiping
    Application.h
    Application.cpp
)

target_include_directories(LiquidRocketPiping PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(LiquidRocketPiping PRIVATE lrep_ui lrep_components lrep_utils lrep_core)
target_sources(LiquidRocketPiping PRIVATE "${CMAKE_SOURCE_DIR}/src/main.cpp")

set_project_warnings(LiquidRocketPiping)

# Set output directory to build/bin
set_target_properties(LiquidRocketPiping PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
