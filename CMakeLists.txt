cmake_minimum_required( VERSION 3.12 ) # CXX_STANDARD 20

project(
    UnTup
    VERSION 0.0.1
    DESCRIPTION "Convert nested tuple-likes into a single tuple
                 or a tuple of references"
    LANGUAGES CXX
)

if( CMAKE_CXX_STANDARD LESS 20 )
    message(
        FATAL_ERROR
        "At least C++20 is required to use ${PROJECT_NAME}!"
    )
endif()

if( UT_HEADERONLY )
    add_library( ${PROJECT_NAME} INTERFACE )
else()
    add_library( ${PROJECT_NAME} STATIC )
    target_compile_features( ${PROJECT_NAME} PUBLIC cxx_std_20 )
    set_source_files_properties(
        untup.xx PROPERTIES
        LANGUAGE CXX
    )
    target_sources(
        ${PROJECT_NAME} PUBLIC
        FILE_SET untup_module
        TYPE CXX_MODULES
        FILES untup.xx
    )
endif()
add_library(
    ${PROJECT_NAME}::${PROJECT_NAME}
    ALIAS ${PROJECT_NAME}
)
target_include_directories( ${PROJECT_NAME} INTERFACE . )
