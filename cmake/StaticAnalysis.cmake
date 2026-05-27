option(SYMON_ENABLE_CLANG_TIDY "Run clang-tidy as part of compilation" OFF)
option(SYMON_ENABLE_CPPCHECK "Run cppcheck as part of compilation" OFF)

function(symon_enable_static_analysis target)
    if(SYMON_ENABLE_CLANG_TIDY)
        find_program(SYMON_CLANG_TIDY_EXECUTABLE clang-tidy REQUIRED)
        set_property(TARGET "${target}" PROPERTY C_CLANG_TIDY "${SYMON_CLANG_TIDY_EXECUTABLE}")
    endif()

    if(SYMON_ENABLE_CPPCHECK)
        find_program(SYMON_CPPCHECK_EXECUTABLE cppcheck REQUIRED)
        set_property(
            TARGET "${target}"
            PROPERTY C_CPPCHECK
                     "${SYMON_CPPCHECK_EXECUTABLE};--enable=warning,style,performance,portability;--error-exitcode=1;--suppress=missingIncludeSystem"
        )
    endif()
endfunction()
