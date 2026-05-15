add_executable(lrep_tests
    test_main.cpp
    test_ComponentDescriptor.cpp
    test_ComponentFactory.cpp
    test_FluidDynamics.cpp
    test_MathStubs.cpp
    test_BlockScene.cpp
    test_UndoCommands.cpp
    test_Serialization.cpp
)

target_include_directories(lrep_tests PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(lrep_tests PRIVATE
    Catch2::Catch2
    lrep_core
    lrep_components
    lrep_utils
    lrep_ui
    Qt6::Widgets
)

set_project_warnings(lrep_tests)

include(CTest)
add_test(NAME lrep_tests COMMAND lrep_tests)
