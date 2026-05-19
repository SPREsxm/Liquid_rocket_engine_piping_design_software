add_executable(LiquidRocketPiping)

target_include_directories(LiquidRocketPiping PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(LiquidRocketPiping PRIVATE lrep_ui lrep_components lrep_utils lrep_core Qt6::PrintSupport)
target_sources(LiquidRocketPiping PRIVATE "${CMAKE_SOURCE_DIR}/src/main.cpp")

set_project_warnings(LiquidRocketPiping)

# Set output directory to build/bin
set_target_properties(LiquidRocketPiping PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Copy Qt6PrintSupport DLL to output directory (not auto-deployed by vcpkg)
if(Qt6_FOUND AND Qt6_DIR)
    get_filename_component(_qt6_cmake_dir "${Qt6_DIR}" DIRECTORY)
    get_filename_component(_qt6_lib_dir   "${_qt6_cmake_dir}" DIRECTORY)
    get_filename_component(_qt6_bin_dir   "${_qt6_lib_dir}" DIRECTORY)
    set(_qt6_bin_dir "${_qt6_bin_dir}/bin")
    if(EXISTS "${_qt6_bin_dir}")
        add_custom_command(TARGET LiquidRocketPiping POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${_qt6_bin_dir}/Qt6PrintSupport$<$<CONFIG:Debug>:d>.dll"
                "$<TARGET_FILE_DIR:LiquidRocketPiping>"
            COMMENT "Copying Qt6PrintSupport DLL to output..."
        )
    endif()
endif()

# Copy translation .qm files to output directory
add_custom_command(TARGET LiquidRocketPiping POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "$<TARGET_FILE_DIR:LiquidRocketPiping>/translations"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_BINARY_DIR}/translations/lrep_zh_CN.qm"
        "$<TARGET_FILE_DIR:LiquidRocketPiping>/translations/"
    COMMENT "Copying translation files to output..."
)
