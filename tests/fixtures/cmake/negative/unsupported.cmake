cmake_minimum_required(VERSION 3.16)
project(unsupported_case)

find_package(Qt6 COMPONENTS Core REQUIRED)
add_executable(unsupported_case main.cpp)
add_custom_command(OUTPUT generated.cpp COMMAND generator)
target_compile_options(unsupported_case PRIVATE "$<$<CONFIG:Debug>:-DDEBUG_ONLY>")
set_target_properties(unsupported_case PROPERTIES AUTOMOC ON)

