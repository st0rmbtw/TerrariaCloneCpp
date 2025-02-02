#ifndef _ENGINE_TYPES_SHADER_PATH_HPP_
#define _ENGINE_TYPES_SHADER_PATH_HPP_

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