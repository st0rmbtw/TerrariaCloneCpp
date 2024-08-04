cmake_minimum_required(VERSION 3.16)

project(ShaderCompiler CXX)

add_executable(${PROJECT_NAME} main.cpp)
if(RELEASE_BUILD)
    add_compile_definitions(${PROJECT_NAME} DEBUG=0)
else()
    add_compile_definitions(${PROJECT_NAME} DEBUG=1)
endif()

if (MSVC)
    add_compile_options("/O3" "/Ob3")
else()
    add_compile_options("-O3" "-g0")
endif()