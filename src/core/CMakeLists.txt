add_library(lrep_core STATIC
    PluginManager.cpp
)

target_include_directories(lrep_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(lrep_core PUBLIC Qt6::Core)
set_project_warnings(lrep_core)
