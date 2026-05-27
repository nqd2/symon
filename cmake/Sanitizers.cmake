option(SYMON_ENABLE_ASAN "Enable AddressSanitizer instrumentation" OFF)
option(SYMON_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer instrumentation" OFF)

function(symon_enable_sanitizers target)
    if(NOT SYMON_ENABLE_ASAN AND NOT SYMON_ENABLE_UBSAN)
        return()
    endif()

    if(MSVC)
        message(FATAL_ERROR "SyMon sanitizer options require GCC or Clang")
    endif()

    set(sanitizers)
    if(SYMON_ENABLE_ASAN)
        list(APPEND sanitizers address)
    endif()
    if(SYMON_ENABLE_UBSAN)
        list(APPEND sanitizers undefined)
    endif()

    list(JOIN sanitizers "," sanitizer_flags)
    target_compile_options("${target}" PRIVATE "-fsanitize=${sanitizer_flags}" -fno-omit-frame-pointer)
    target_link_options("${target}" PRIVATE "-fsanitize=${sanitizer_flags}")
endfunction()
