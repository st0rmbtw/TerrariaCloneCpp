#ifndef TERRARIA_VULKAN_SHADER_COMPILER
#define TERRARIA_VULKAN_SHADER_COMPILER

#pragma once

#include "types/shader_type.hpp"

bool CompileVulkanShader(ShaderType shaderType, const char* shaderSource, size_t shaderSourceSize, const char* outputPath);

#endif