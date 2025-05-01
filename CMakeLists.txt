cmake_minimum_required (VERSION 3.25)

project(SmolChess DESCRIPTION "Smol Chess Engine")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(/W3)
add_compile_definitions(BUILD_ROOT=\"${CMAKE_SOURCE_DIR}\")

# Chess
file(GLOB_RECURSE source_list "src/chess/*.c" "src/chess/*.cpp" )
file(GLOB_RECURSE headers_list "src/chess/*.h" "src/chess/*.hpp" )
add_library( SmolChess STATIC ${source_list} )
target_include_directories( SmolChess PUBLIC "ext/pcg" )
target_include_directories( SmolChess INTERFACE "src/chess" )

# Tests
add_subdirectory("ext/catch2")
file(GLOB_RECURSE test_list "src/tests/*.c" "src/tests/*.cpp")
add_executable( SmolChessTests ${test_list} )

target_link_libraries( SmolChessTests PRIVATE Catch2::Catch2WithMain )
target_link_libraries( SmolChessTests PRIVATE SmolChess )
target_include_directories( SmolChessTests PRIVATE "src" )

add_executable( SmolChessProgram "src/Main.cpp" )
target_link_libraries( SmolChessProgram SmolChess )

if (CMAKE_VERSION VERSION_GREATER 3.12)
    set_property(TARGET SmolChess PROPERTY CXX_STANDARD 23)
    set_property(TARGET SmolChessTests PROPERTY CXX_STANDARD 23)
    set_property(TARGET SmolChessProgram PROPERTY CXX_STANDARD 23)
endif()