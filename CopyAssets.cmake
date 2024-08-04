if (NOT EXISTS ${BUILD_DIR}/assets)
    file(MAKE_DIRECTORY ${BUILD_DIR}/assets)

    message("Copying assets...")

    file(GLOB_RECURSE ASSETS ${CMAKE_CURRENT_SOURCE_DIR}/assets/**/*.*)

    foreach(f IN LISTS ASSETS) 
        file(RELATIVE_PATH out_file ${CMAKE_CURRENT_SOURCE_DIR}/assets/ ${f})
        get_filename_component(out_dir ${out_file} DIRECTORY)
        get_filename_component(out_name ${out_file} NAME)

        file(MAKE_DIRECTORY ${BUILD_DIR}/assets/${out_dir})

        if (NOT ${out_dir} STREQUAL "shaders/vulkan")
            file(COPY ${f} DESTINATION ${BUILD_DIR}/assets/${out_dir})
        endif()
    endforeach()
endif()