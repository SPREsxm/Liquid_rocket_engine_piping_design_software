add_library(lrep_utils STATIC
    MathStubs.h
    MathStubs.cpp
    FluidDynamics.h
    FluidDynamics.cpp
    IdGenerator.h
    NetworkValidator.h
    NetworkValidator.cpp
    NetworkSolver.h
    NetworkSolver.cpp
    ResultExporter.h
    ResultExporter.cpp
    TransientSolver.h
    TransientSolver.cpp
    ResistanceCoefficients.h
    NumericalMethods.h
    Benchmark.h
    PropellantProperties.h
    MethaneEOS.h
    HeatTransfer.h
    FluidStructureInteraction.h
    SSTTurbulence.h
    ThrustAnalysis.h
    GridRefinement.h
    ExpressionEngine.h
    PipeScheduleDatabase.h
    PipeScheduleDatabase.cpp
    ReportGenerator.h
    ReportGenerator.cpp
    DesignRules.h
    DesignRules.cpp
    BomGenerator.h
    BomGenerator.cpp
)

target_include_directories(lrep_utils PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(lrep_utils PUBLIC Qt6::Core Qt6::Widgets)

if(USE_EIGEN)
    target_link_libraries(lrep_utils PUBLIC Eigen3::Eigen)
    target_compile_definitions(lrep_utils PUBLIC USE_EIGEN)
endif()

if(USE_EXPRTK)
    target_link_libraries(lrep_utils PUBLIC exprtk)
    target_compile_definitions(lrep_utils PUBLIC USE_EXPRTK)
endif()

set_project_warnings(lrep_utils)
