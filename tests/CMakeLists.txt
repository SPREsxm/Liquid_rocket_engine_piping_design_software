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
    test_NetworkValidator.cpp
    test_IdGenerator.cpp
    test_Constants.cpp
    test_ResultExporter.cpp
    test_PaintChart.cpp
    test_PipeScheduleDatabase.cpp
    test_PluginManager.cpp
    test_LibraryTreeModel.cpp
    test_ReportGenerator.cpp
    test_DesignRules.cpp
    test_BomGenerator.cpp
    test_SensitivitySolver.cpp
    test_PathProfile.cpp
    test_PipeOptimizer.cpp
    test_ThermalSolver.cpp
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

# Ensure Qt bin directory is on PATH for runtime DLL resolution
if(Qt6_FOUND AND Qt6_DIR)
    # Qt6_DIR = <prefix>/lib/cmake/Qt6, so go up 3 levels to get <prefix>
    get_filename_component(_qt6_cmake_dir "${Qt6_DIR}" DIRECTORY)
    get_filename_component(_qt6_lib_dir   "${_qt6_cmake_dir}" DIRECTORY)
    get_filename_component(_qt6_prefix    "${_qt6_lib_dir}" DIRECTORY)
    if(EXISTS "${_qt6_prefix}/bin")
        set_tests_properties(lrep_tests PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${_qt6_prefix}/bin")
    endif()
endif()
