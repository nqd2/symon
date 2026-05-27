cmake_minimum_required(VERSION 3.27)

project(
    SyMon
    VERSION 0.1.0
    DESCRIPTION "Lightweight System Monitor"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CTest)
include(GNUInstallDirs)
include(CompilerOptions)
include(Sanitizers)
include(StaticAnalysis)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)
find_program(GLIB_COMPILE_RESOURCES_EXECUTABLE glib-compile-resources REQUIRED)

set(SYMON_RESOURCE_XML "${CMAKE_CURRENT_SOURCE_DIR}/data/io.github.symon.SyMon.gresource.xml")
set(SYMON_UI_FILE "${CMAKE_CURRENT_SOURCE_DIR}/data/ui/main-window.ui")
set(SYMON_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(SYMON_RESOURCE_SOURCE "${SYMON_GENERATED_DIR}/symon-resources.c")

file(MAKE_DIRECTORY "${SYMON_GENERATED_DIR}")

add_custom_command(
    OUTPUT "${SYMON_RESOURCE_SOURCE}"
    COMMAND
        "${GLIB_COMPILE_RESOURCES_EXECUTABLE}"
        --generate-source
        --target "${SYMON_RESOURCE_SOURCE}"
        --sourcedir "${CMAKE_CURRENT_SOURCE_DIR}/data"
        "${SYMON_RESOURCE_XML}"
    DEPENDS "${SYMON_RESOURCE_XML}" "${SYMON_UI_FILE}"
    VERBATIM
)

add_executable(
    symon
    src/app/main.c
    src/app/symon_app.c
    "${SYMON_RESOURCE_SOURCE}"
)

set_source_files_properties(
    "${SYMON_RESOURCE_SOURCE}"
    PROPERTIES GENERATED TRUE COMPILE_OPTIONS "-w" SKIP_LINTING TRUE
)

target_include_directories(symon PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/app")
target_link_libraries(symon PRIVATE PkgConfig::GTK4)

symon_enable_project_warnings(symon)
symon_enable_sanitizers(symon)
symon_enable_static_analysis(symon)

install(TARGETS symon RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(
    FILES data/io.github.symon.SyMon.desktop
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications"
)
install(
    FILES data/io.github.symon.SyMon.metainfo.xml
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/metainfo"
)

if(BUILD_TESTING)
    add_executable(symon_app_id_test tests/unit/test_app_id.c)
    target_include_directories(symon_app_id_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/app")
    symon_enable_project_warnings(symon_app_id_test)
    symon_enable_sanitizers(symon_app_id_test)
    symon_enable_static_analysis(symon_app_id_test)

    add_test(NAME symon.app_id COMMAND symon_app_id_test)
endif()
