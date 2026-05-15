# Common compiler warning flags for MSVC, GCC, and Clang

function(set_project_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /utf-8
            /permissive-
            /Zc:__cplusplus
        )
        target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wconversion
            -Wsign-conversion
            -Wnull-dereference
            -Wformat=2
        )
    endif()
endfunction()
