cmake_minimum_required(VERSION 3.16)

project(ShaderCompiler CXX)

find_package(Slang REQUIRED)

add_subdirectory(./thirdparty/SPIRV-Cross)

add_executable(${PROJECT_NAME} main.cpp)
if(RELEASE_BUILD)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG=0)
    if (MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE "/O3" "/Ob3")
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE "-O3" "-g0")
    endif()
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG=1)
    if (MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE "/O0")
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE "-O0" "-ggdb")
    endif()
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE slang::slang spirv-cross-glsl)