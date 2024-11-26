#ifndef TYPES_SHADER_PATH
#define TYPES_SHADER_PATH

#include "shader_type.hpp"

struct ShaderPath {
    ShaderPath(ShaderType shader_type, std::string name, std::string func_name = {}) :
        shader_type(shader_type),
        name(std::move(name)),
        func_name(std::move(func_name)) {}

    ShaderType shader_type;
    std::string name;
    std::string func_name;
};

#endif