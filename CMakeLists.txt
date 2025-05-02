cmake_minimum_required (VERSION 3.25)

project(DynTrt DESCRIPTION "DynTrt")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(/W3)
add_compile_definitions(BUILD_ROOT=\"${CMAKE_SOURCE_DIR}\")

# Chess
file(GLOB_RECURSE source_list "src/*.c" "src/*.cpp" )
file(GLOB_RECURSE headers_list "src/*.h" "src/*.hpp" )
add_library( DynTrt STATIC ${source_list} )
target_include_directories( DynTrt INTERFACE "src/include" )


# Tests
add_subdirectory("ext/catch2")
file(GLOB_RECURSE test_list "tests/*.c" "tests/*.cpp")
add_executable( DynTrtTests ${test_list} )

target_link_libraries( DynTrtTests PRIVATE DynTrt Catch2::Catch2WithMain )

if (CMAKE_VERSION VERSION_GREATER 3.12)
    set_property(TARGET DynTrtTests PROPERTY CXX_STANDARD 23)
    #set_property(TARGET DynTrt PROPERTY CXX_STANDARD 23)
endif()