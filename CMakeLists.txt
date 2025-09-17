cmake_minimum_required(VERSION 3.24)

project(TerrariaClone LANGUAGES CXX)

cmake_policy(SET CMP0069 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)

find_package(OpenMP)

set(CMAKE_CXX_STANDARD 20)

if(APPLE)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 20)
endif()

set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
set(CMAKE_COLOR_DIAGNOSTICS ON)

option(ENABLE_DEBUG_TOOLS "Enable Debug Tools" OFF)

set(DEBUG_INFO OFF)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(RELEASE_BUILD OFF)
    set(DEBUG_INFO ON)
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(RELEASE_BUILD ON)
    set(DEBUG_INFO ON)
else()
    set(RELEASE_BUILD ON)
    set(DEBUG_INFO OFF)
endif()

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

if (RELEASE_BUILD)
    message("-- Build Type: Release")
    if(MSVC)
        add_compile_options("/O2" "/Ob3" "/Qfast_transcendentals" "${BITCODE}")
        STRING(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        STRING(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
        STRING(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
        STRING(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    else()
        add_compile_options("-O3")
        add_compile_options("-fomit-frame-pointer")
        add_compile_options("-fno-math-errno" "-fno-signed-zeros" "-fno-trapping-math" "-fassociative-math")
        add_compile_options("-fno-exceptions")
    endif()
    set(DISABLE_FORCE_DEBUG_POSTFIX ON)
    # set(LLGL_BUILD_STATIC_LIB ON CACHE BOOL "")
    set(LLGL_ENABLE_DEBUG_LAYER OFF CACHE BOOL "" FORCE)
    set(LLGL_ENABLE_CHECKED_CAST OFF CACHE BOOL "" FORCE)
    set(LLGL_ENABLE_EXCEPTIONS OFF CACHE BOOL "" FORCE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message("-- Build Type: Debug")
    if(MSVC)
        add_compile_options("/Od")
    else()
        add_compile_options("-O0")
    endif()
    set(DISABLE_FORCE_DEBUG_POSTFIX OFF)
    set(LLGL_ENABLE_EXCEPTIONS ON CACHE BOOL "" FORCE)
    set(ENABLE_DEBUG_TOOLS ON)
endif()

if(DEBUG_INFO)
    message("-- Debug Info: Enabled")
    if(MSVC)
        add_compile_options("/DEBUG")
    else()
        add_compile_options("-ggdb3")
    endif()
else()
    message("-- Debug Info: Disabled")
    if(NOT MSVC)
        add_compile_options("-g0")
    endif()
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

if (WIN32)
    add_compile_definitions(NOMINMAX)
endif()

if (UNIX AND NOT APPLE)
   set(LLGL_LINUX_ENABLE_WAYLAND ON)
endif()

if (MSVC)
    add_compile_options(/wd5240)
else()
    add_compile_options(-Wno-ignored-attributes -march=native)
endif()

add_compile_definitions(GLM_FORCE_INTRINSICS)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)

unset(EXECUTABLE_OUTPUT_PATH CACHE)
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} CACHE STRING "" FORCE)
set(LLGL_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE STRING "" FORCE)

add_subdirectory(thirdparty/SGE)
add_subdirectory(thirdparty/FastNoiseLite)
add_subdirectory(ShaderCompiler)
add_subdirectory(FontAssetGenerator)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.mm")
if (APPLE)
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/game.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++")
else()
    set_source_files_properties(${SRC} PROPERTIES COMPILE_FLAGS "-x c++")
endif()

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${SRC})

if(NOT MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE "-Wall" "-Wextra" "-Wno-gnu-zero-variadic-macro-arguments")
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE "/Zc:preprocessor")
endif()

if(RELEASE_BUILD)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG=0)
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG=1)
endif()

if(ENABLE_DEBUG_TOOLS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG_TOOLS=1)
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG_TOOLS=0)
endif()

if(OpenMP_CXX_FOUND)
    target_link_libraries(${PROJECT_NAME} PUBLIC OpenMP::OpenMP_CXX)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE SGE FastNoiseLite)

if (LINUX)
    target_link_libraries(${PROJECT_NAME} PRIVATE SGE atomic)
endif()

# ========================= Copy Assets =========================

file(GLOB_RECURSE ASSETS ${CMAKE_CURRENT_SOURCE_DIR}/assets/**/*.*)
file(GLOB_RECURSE SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/*.*)

list(REMOVE_ITEM ASSETS ${SHADERS})

list(FILTER ASSETS EXCLUDE REGEX ".*\.(otf|ttf)")

set(ASSETS_OUTPUT)
foreach(asset ${ASSETS})
    file(RELATIVE_PATH out_file ${CMAKE_CURRENT_SOURCE_DIR}/assets/ ${asset})
    get_filename_component(out_dir ${out_file} DIRECTORY)
    get_filename_component(out_name ${out_file} NAME)

    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/assets/${out_dir})

    add_custom_command(
        OUTPUT
            ${CMAKE_CURRENT_BINARY_DIR}/assets/${out_dir}/${out_name}
        COMMAND
            ${CMAKE_COMMAND} -E copy ${asset} ${CMAKE_CURRENT_BINARY_DIR}/assets/${out_dir}
        DEPENDS
            ${asset}
    )

    list(APPEND ASSETS_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/assets/${out_dir}/${out_name})
endforeach()

add_custom_target(COPY_ASSETS ALL DEPENDS ${ASSETS_OUTPUT})

# ===================== Compile Shaders =====================

add_custom_target(
    CompileShadersTarget
    COMMAND $<TARGET_FILE:ShaderCompiler> ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ShaderCompiler
)

add_dependencies(${PROJECT_NAME} CompileShadersTarget)

# ===================== Generate Font Assets =====================

file(GLOB_RECURSE FONT_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/*.*)

set(FONT_FILES_OUTPUT)

foreach(font ${FONT_FILES})
    get_filename_component(font_name ${font} NAME_WLE)
    set(font_output_path assets/fonts/${font_name}.png)
    set(font_meta_output_path assets/fonts/${font_name}.meta)
    add_custom_command(
        OUTPUT
            ${font_output_path} ${font_meta_output_path}
        COMMAND
            $<TARGET_FILE:FontAssetGenerator> ${font} ${CMAKE_CURRENT_BINARY_DIR}/${font_output_path}
        DEPENDS
            ${font} FontAssetGenerator
    )
    list(APPEND FONT_FILES_OUTPUT ${font_output_path})
endforeach()

add_custom_target(GENERATE_FONT_ATLASES ALL DEPENDS ${FONT_FILES_OUTPUT})
add_dependencies(GENERATE_FONT_ATLASES FontAssetGenerator)
