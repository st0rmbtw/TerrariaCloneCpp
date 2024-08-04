cmake_minimum_required(VERSION 3.16)

project(TerrariaClone CXX)

set(CMAKE_CXX_STANDARD 20)

if(APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(RELEASE_BUILD OFF)
else()
    set(RELEASE_BUILD ON)
endif()

if (RELEASE_BUILD)
    message("-- Build Type: Release")
    if(MSVC)
        add_compile_options("/O2" "/Ob3" "/Qfast_transcendentals")
        STRING (REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        STRING (REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
        STRING (REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
        STRING (REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    else()
        add_compile_options("-g0" "-O3" "-fomit-frame-pointer" "-ffast-math" "${BITCODE}")
    endif()
    set(DISABLE_FORCE_DEBUG_POSTFIX ON)
    # set(LLGL_BUILD_STATIC_LIB ON CACHE BOOL "")
else()
    message("-- Build Type: Debug")
    if(MSVC)
        add_compile_options("/ggdb" "/O0")
    else()
        add_compile_options("-ggdb" "-O0")
    endif()
    set(DISABLE_FORCE_DEBUG_POSTFIX OFF)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
# if(CMAKE_GENERATOR STREQUAL "Xcode")
# 	if(CMAKE_EXPORT_COMPILE_COMMANDS)
# 		set(CMAKE_XCODE_ATTRIBUTE_OTHER_CFLAGS  "$(inherited) -gen-cdb-fragment-path ${CMAKE_SOURCE_DIR}/CompilationDatabase")
# 	endif()
# endif()

if (WIN32)
    add_compile_definitions(NOMINMAX)
endif()

add_compile_options("-Wno-ignored-attributes")

set(LLGL_BUILD_RENDERER_NULL OFF CACHE BOOL "" FORCE)

if (APPLE)
    set(LLGL_BUILD_RENDERER_METAL ON CACHE BOOL "" FORCE)
else()
    set(LLGL_BUILD_RENDERER_METAL OFF CACHE BOOL "" FORCE)
endif()

if (WIN32)
    set(LLGL_BUILD_RENDERER_DIRECT3D12 OFF CACHE BOOL "" FORCE)
    set(LLGL_BUILD_RENDERER_DIRECT3D11 ON CACHE BOOL "" FORCE)
else()
    set(LLGL_BUILD_RENDERER_DIRECT3D12 OFF CACHE BOOL "" FORCE)
    set(LLGL_BUILD_RENDERER_DIRECT3D11 OFF CACHE BOOL "" FORCE)
endif()

set(LLGL_BUILD_RENDERER_VULKAN ON CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)

add_subdirectory(thirdparty/LLGL)
add_subdirectory(thirdparty/glm)
add_subdirectory(thirdparty/glfw)
add_subdirectory(thirdparty/STB)
add_subdirectory(thirdparty/FastNoiseLite)
add_subdirectory(shader_compiler)
# add_subdirectory(thirdparty/freetype-2.13.2)

unset(EXECUTABLE_OUTPUT_PATH CACHE)
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} CACHE STRING "" FORCE)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.mm")

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE ${SRC})

if(RELEASE_BUILD)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG=0)
    target_compile_options(${PROJECT_NAME} PUBLIC "-O3" "-g0")
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG=1)
    target_compile_options(${PROJECT_NAME} PUBLIC "-O0" "-ggdb")
endif()

target_link_libraries(${PROJECT_NAME} glm STB glfw LLGL LLGL_Direct3D11 LLGL_OpenGL FastNoiseLite)

file(GLOB_RECURSE ASSETS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/**/*.*)

set(ASSETS_OUTPUT)
foreach(asset ${ASSETS})
    file(RELATIVE_PATH out_file ${CMAKE_CURRENT_SOURCE_DIR}/assets/ ${asset})
    get_filename_component(out_dir ${out_file} DIRECTORY)
    get_filename_component(out_name ${out_file} NAME)

    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/assets/${out_dir})

    if (NOT ${out_dir} STREQUAL "shaders/vulkan")
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/assets/${out_dir}/${out_name}
            COMMAND ${CMAKE_COMMAND} -E copy ${asset} ${CMAKE_CURRENT_BINARY_DIR}/assets/${out_dir}
            DEPENDS ${asset}
        )

        list(APPEND ASSETS_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/assets/${out_dir}/${out_name})
    endif()
endforeach()

add_custom_target(COPY_ASSETS DEPENDS ${ASSETS_OUTPUT})
add_dependencies(${PROJECT_NAME} COPY_ASSETS)

# -----------------------------------------------------------------------

file(GLOB_RECURSE VULKAN_SHADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/vulkan/*.*)

set(VULKAN_SHADERS_OUTPUT)

foreach(shader ${VULKAN_SHADERS})
    get_filename_component(shader_name ${shader} NAME)
    add_custom_command(
        OUTPUT assets/shaders/vulkan/${shader_name}.spv
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/shader_compiler/ShaderCompiler.exe ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${shader}
        DEPENDS ${shader}
    )
    list(APPEND VULKAN_SHADERS_OUTPUT assets/shaders/vulkan/${shader_name}.spv)
endforeach()

add_custom_target(COMPILE_VULKAN_SHADERS DEPENDS ${VULKAN_SHADERS_OUTPUT})

add_dependencies(${PROJECT_NAME} COMPILE_VULKAN_SHADERS)