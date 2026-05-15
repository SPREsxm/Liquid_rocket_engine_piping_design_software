add_library(lrep_ui STATIC
    MainWindow.h
    MainWindow.cpp
    PreferencesDialog.h
    PreferencesDialog.cpp
    SolverResultsPanel.h
    SolverResultsPanel.cpp
)

target_include_directories(lrep_ui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(lrep_ui PUBLIC Qt6::Widgets Qt6::Svg lrep_core lrep_components)
set_project_warnings(lrep_ui)

add_subdirectory(actions)
add_subdirectory(graphics)
add_subdirectory(library)
add_subdirectory(properties)
