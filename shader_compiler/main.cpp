#include <sstream>
#include <stdio.h>
#include <string>
#include <filesystem>
#include <fstream>

#include "../src/constants.hpp"

#if defined(WIN32) || defined(_WIN32)
#include <Windows.h>
#else
// TODO
#endif

namespace fs = std::filesystem;

bool CompileVulkanShader(const std::string& executable, const std::string& stage, const char* shaderSource, size_t shaderSourceSize, const char* outputPath);

struct ShaderDef {
    std::string name;
    std::string value;
    
    ShaderDef(std::string name, std::string value) :
        name(std::move(name)),
        value(std::move(value)) {}
};

int main(int argc, const char** argv) {
    if (argc == 0) {
        printf("No input arguments given.\n");
        return -1;
    }

    if (argc < 4) {
        printf("Usage: %s <SOURCE_DIRECTORY> <BUILD_DIRECTORY> <file>\n", argv[0]);
        return -1;
    }

    const fs::path source_dir = argv[1];
    const fs::path build_dir = argv[2];
    const fs::path file = argv[3];

    const std::vector<ShaderDef> shader_defs = {
        // ShaderDef("DEF_SUBDIVISION", std::to_string(config::SUBDIVISION)),
        ShaderDef("CHUNK_WIDTH", std::to_string(Constants::RENDER_CHUNK_SIZE_U)),
        ShaderDef("CHUNK_HEIGHT", std::to_string(Constants::RENDER_CHUNK_SIZE_U)),

        ShaderDef("TILE_SIZE", std::to_string(Constants::TILE_SIZE)),
        ShaderDef("WALL_SIZE", std::to_string(Constants::WALL_SIZE)),

        ShaderDef("TILE_TEXTURE_WIDTH", std::to_string(Constants::MAX_TILE_TEXTURE_WIDTH)),
        ShaderDef("TILE_TEXTURE_HEIGHT", std::to_string(Constants::MAX_TILE_TEXTURE_HEIGHT)),
        ShaderDef("TILE_TEXTURE_PADDING", std::to_string(Constants::TILE_TEXTURE_PADDING)),

        ShaderDef("WALL_TEXTURE_WIDTH", std::to_string(Constants::MAX_WALL_TEXTURE_WIDTH)),
        ShaderDef("WALL_TEXTURE_HEIGHT", std::to_string(Constants::MAX_WALL_TEXTURE_HEIGHT)),
        ShaderDef("WALL_TEXTURE_PADDING", std::to_string(Constants::WALL_TEXTURE_PADDING)),

    };

#ifdef _WIN32
    const fs::path executable = source_dir / "glslang.exe";
#else
    const fs::path executable = source_dir / "glslang";
#endif

    if (!fs::exists(executable)) {
        printf("%s doesn't exist.\n", source_dir.string().c_str());
        return -1;
    }

    const fs::path dir = source_dir / "assets" / "shaders" / "vulkan";
    if (!fs::exists(dir)) {
        printf("%s doesn't exist.\n", dir.string().c_str());
        return -1;
    }

    std::ifstream shader(file);
    std::stringstream content;
    content << shader.rdbuf();
    shader.close();

    std::string shader_source = content.str();

    for (const ShaderDef& shader_def : shader_defs) {
        size_t pos;
        while ((pos = shader_source.find(shader_def.name)) != std::string::npos) {
            shader_source.replace(pos, shader_def.name.length(), shader_def.value);
        }
    }

    const fs::path output_path = build_dir / "assets" / "shaders" / "vulkan/" / (file.filename().string() + ".spv");
    const std::string stage = file.extension().string().substr(1);
    
    printf("Compiling %s\n", file.string().c_str());
    if (!CompileVulkanShader(executable.string(), stage, shader_source.c_str(), shader_source.length(), output_path.string().c_str())) return -1;

    return 0;
}

#if defined(WIN32) || defined(_WIN32)

void ReadFromPipe(HANDLE pipe) { 
   DWORD dwRead, dwWritten; 
   CHAR chBuf[1024]; 
   BOOL bSuccess = FALSE;
   HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

   for (;;) 
   { 
      bSuccess = ReadFile(pipe, chBuf, 1024, &dwRead, NULL);
      if (!bSuccess || dwRead == 0) break;

      bSuccess = WriteFile(hParentStdOut, chBuf, dwRead, &dwWritten, NULL);
      if (!bSuccess) break; 
   } 
}

bool CompileVulkanShader(const std::string& executable, const std::string& stage, const char* shaderSource, size_t shaderSourceSize, const char* outputPath)
{
    SECURITY_ATTRIBUTES saAttr; 
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL; 

    HANDLE hStdInWr;
    HANDLE hStdInRd;

    HANDLE hStdOutWr;
    HANDLE hStdOutRd;

    bool success = CreatePipe(&hStdInRd, &hStdInWr, &saAttr, 0);
    if (!success) {
        printf("Couldn't create a pipe.\n");
        return false;
    }

    success = SetHandleInformation(hStdInWr, HANDLE_FLAG_INHERIT, 0);
    if (!success) {
        printf("Couldn't set handle information.\n");
        return false;
    }

    success = CreatePipe(&hStdOutRd, &hStdOutWr, &saAttr, 0);
    if (!success) {
        printf("Couldn't create a pipe.\n");
        return false;
    }

    // success = SetHandleInformation(hStdOutRd, HANDLE_FLAG_INHERIT, 0);
    // ASSERT(success, "ASDASDSd")

    success = WriteFile(hStdInWr, shaderSource, shaderSourceSize, nullptr, NULL);
    if (!success) {
        printf("Couldn't write to the pipe.\n");
        return false;
    }

    CloseHandle(hStdInWr);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&pi, sizeof(pi));

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput = hStdInRd;
    si.hStdOutput = hStdOutWr;
    si.hStdError = hStdOutWr;

#if DEBUG
    const std::string strip_debug_info = "";
#else
    const std::string strip_debug_info = " -g0";
#endif

    const std::string args = executable + strip_debug_info + " --quiet -V --enhanced-msgs --stdin -S " + stage + " -o " + outputPath;

    // start the program up
    success = CreateProcessA(executable.c_str(), const_cast<char*>(args.c_str()), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    if (!success) {
        printf("Couldn't create a process.\n");
        return false;
    }

    CloseHandle(hStdOutWr);

    WaitForSingleObject(pi.hProcess, INFINITE);

    ReadFromPipe(hStdOutRd);

    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hStdInRd);
    CloseHandle(hStdOutWr);

    return true;
}

#else
// TODO
#endif