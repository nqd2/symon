function(symon_enable_project_warnings target)
    target_compile_options(
        "${target}"
        PRIVATE
            "$<$<COMPILE_LANG_AND_ID:C,GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wformat=2;-Wshadow>"
    )
endfunction()
