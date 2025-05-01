cmake_minimum_required (VERSION 3.25)

project(SmolChess DESCRIPTION "AnyTraits")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(/W3)
add_compile_definitions(BUILD_ROOT=\"${CMAKE_SOURCE_DIR}\")

# Chess
file(GLOB_RECURSE source_list "src/*.c" "src/*.cpp" )
file(GLOB_RECURSE headers_list "src/*.h" "src/*.hpp" )
#add_library( AnyTraits STATIC ${source_list} )
#target_include_directories( AnyTraits INTERFACE "src/" )

# Tests
add_subdirectory("ext/catch2")
file(GLOB_RECURSE test_list "tests/*.c" "tests/*.cpp")
add_executable( AnyTraitsTests ${test_list} )

target_link_libraries( AnyTraitsTests PRIVATE Catch2::Catch2WithMain )
target_include_directories( AnyTraitsTests PRIVATE "src" )

if (CMAKE_VERSION VERSION_GREATER 3.12)
    set_property(TARGET AnyTraitsTests PROPERTY CXX_STANDARD 23)
    #set_property(TARGET AnyTraits PROPERTY CXX_STANDARD 23)
endif()