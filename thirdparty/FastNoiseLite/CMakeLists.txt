cmake_minimum_required(VERSION 3.16)
project(FastNoiseLite CXX)

add_library(${PROJECT_NAME} STATIC)
target_sources(${PROJECT_NAME} PRIVATE "./src/FastNoiseLite.cpp")
target_include_directories(${PROJECT_NAME} PUBLIC ./include/)
