cmake_minimum_required(VERSION 3.16)
project(STB C)

add_library(${PROJECT_NAME} STATIC)
target_sources(${PROJECT_NAME} PRIVATE "src/stb_image.c")
target_include_directories(${PROJECT_NAME} PUBLIC include)
