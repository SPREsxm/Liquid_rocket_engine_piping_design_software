add_library(lrep_components STATIC
    PortDescriptor.h
    ComponentDescriptor.h
    ComponentDescriptor.cpp
    ComponentFactory.h
    ComponentFactory.cpp
    ComponentInstance.h
)

target_include_directories(lrep_components PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(lrep_components PUBLIC lrep_core)
set_project_warnings(lrep_components)
