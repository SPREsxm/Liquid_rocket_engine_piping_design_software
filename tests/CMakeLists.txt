add_executable(lrep_tests
    test_main.cpp
    test_ComponentDescriptor.cpp
    test_ComponentFactory.cpp
    test_FluidDynamics.cpp
    test_MathStubs.cpp
    test_BlockScene.cpp
    test_UndoCommands.cpp
    test_Serialization.cpp
    test_HeatTransfer.cpp
    test_ResistanceCoefficients.cpp
    test_PropellantProperties.cpp
    test_GasDynamics.cpp
    test_NumericalMethods.cpp
    test_FluidStructureInteraction.cpp
    test_ThrustAnalysis.cpp
    test_SSTTurbulence.cpp
    test_MixedPrecision.cpp
    test_Benchmark.cpp
    test_GridRefinement.cpp
    test_MethaneEOS.cpp
    test_NetworkSolver.cpp
    test_TransientSolver.cpp
    test_ExpressionEngine.cpp
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

# ExprTk is a massive header-only library — needs /bigobj on MSVC
if(MSVC)
    target_compile_options(lrep_tests PRIVATE /bigobj)
endif()

include(CTest)
add_test(NAME lrep_tests COMMAND lrep_tests)
