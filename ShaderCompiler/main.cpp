#include <cstring>
#include <sstream>
#include <stdio.h>
#include <string>
#include <filesystem>
#include <fstream>
#include <vector>

#include "../src/constants.hpp"

#if defined(WIN32) || defined(_WIN32)
    #include <Windows.h>
    #define PLATFORM_WINDOWS
#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #define PLATFORM_POSIX
#else
    #error "UNKNOWN PLATFORM"
#endif

namespace fs = std::filesystem;

bool CompileVulkanShader(const std::string& executable, const std::string& stage, const char* shaderSource, size_t shaderSourceSize, const std::string& outputPath);

struct ShaderDef {
    std::string name;
    std::string value;
    
    ShaderDef(std::string name, std::string value) :
        name(std::move(name)),
        value(std::move(value)) {}
};

int main(int argc, const char** argv) {
    if (argc == 0) {
        printf("No input arguments are given.\n");
        return 0;
    }

    if (argc < 4) {
        printf("Usage: %s <SOURCE_DIRECTORY> <BUILD_DIRECTORY> <file>\n", argv[0]);
        printf("Not enough arguments.\n");
        return 0;
    }

    const fs::path source_dir = argv[1];
    const fs::path build_dir = argv[2];
    const fs::path file = argv[3];

    const std::vector<ShaderDef> shader_defs = {
        ShaderDef("DEF_SUBDIVISION", std::to_string(Constants::SUBDIVISION)),
        ShaderDef("DEF_SOLID_DECAY", std::to_string(Constants::LightDecay(true))),
        ShaderDef("DEF_AIR_DECAY", std::to_string(Constants::LightDecay(false))),
        ShaderDef("CHUNK_WIDTH", std::to_string(Constants::RENDER_CHUNK_SIZE_U)),
        ShaderDef("CHUNK_HEIGHT", std::to_string(Constants::RENDER_CHUNK_SIZE_U)),

        ShaderDef("TILE_SIZE", std::to_string(Constants::TILE_SIZE)),
        ShaderDef("WALL_SIZE", std::to_string(Constants::WALL_SIZE)),
    };

#ifdef PLATFORM_WINDOWS
    const fs::path executable = "glslang.exe";
#else
    const fs::path executable = "glslang";
#endif

    const fs::path dir = source_dir / "assets" / "shaders" / "vulkan";
    if (!fs::exists(dir)) {
        printf("ERROR: %s doesn't exist.\n", dir.string().c_str());
        return 0;
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
    if (!CompileVulkanShader(executable.string(), stage, shader_source.c_str(), shader_source.length(), output_path.string())) return -1;

    return 0;
}

#if defined(PLATFORM_WINDOWS)

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

bool CompileVulkanShader(const std::string& executable, const std::string& stage, const char* shaderSource, size_t shaderSourceSize, const std::string& outputPath)
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

    const std::string args = executable + " --quiet -V --enhanced-msgs --stdin -S " + stage + " -o " + outputPath;

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

#elif defined(PLATFORM_POSIX)

#define PIPE_READ 0
#define PIPE_WRITE 1

void read_pipe(int* pipe) {
    ssize_t nbytes = 0;
    char buffer[1024];
    while ((nbytes=read(pipe[PIPE_READ], buffer, sizeof(buffer))) > 0) {
        write(STDOUT_FILENO, buffer, nbytes);
    }
}

bool CompileVulkanShader(const std::string& executable, const std::string& stage, const char* shaderSource, size_t shaderSourceSize, const std::string& outputPath) {
    pid_t pid;
    int stdoutPipe[2];
    int stdinPipe[2];
    int status = 0;

    if (pipe(stdoutPipe) < 0) {
        printf("Couldn't create a stdout pipe.\n");
        return false;
    }

    if (pipe(stdinPipe) < 0) {
        close(stdoutPipe[PIPE_READ]);
        close(stdoutPipe[PIPE_WRITE]);
        printf("Couldn't create a stdin pipe.\n");
        return false;
    }

    if ((pid = fork()) < 0) {
        close(stdoutPipe[PIPE_READ]);
        close(stdoutPipe[PIPE_WRITE]);
        close(stdinPipe[PIPE_READ]);
        close(stdinPipe[PIPE_WRITE]);
        printf("Couldn't create a child process.\n");
        return false;
    }

    if (pid == 0) {
        close(stdinPipe[PIPE_WRITE]);
        close(stdoutPipe[PIPE_READ]);

        if (dup2(stdinPipe[PIPE_READ], STDIN_FILENO) == -1) {
            printf("Couldn't redirect stdin pipe.\n");
            return false;
        }

        if (dup2(stdoutPipe[PIPE_WRITE], STDOUT_FILENO) == -1) {
            printf("Couldn't redirect stdout pipe.\n");
            return false;
        }

        if (dup2(stdoutPipe[PIPE_WRITE], STDERR_FILENO) == -1) {
            printf("Couldn't redirect stderr pipe.\n");
            return false;
        }

        int ret = execlp(executable.c_str(),
            executable.c_str(),
            "--quiet",
            "-V",
            "--enhanced-msgs",
            "--stdin",
            "-S", stage.c_str(),
            "-o", outputPath.c_str(),
            NULL
        );

        if (ret < 0) {
            printf("An error occured: %s\n", strerror(errno));
            return false;
        }

        close(stdoutPipe[PIPE_WRITE]);
    } else {
        write(stdinPipe[PIPE_WRITE], shaderSource, shaderSourceSize);
        close(stdinPipe[PIPE_WRITE]);

        close(stdoutPipe[PIPE_WRITE]);

        read_pipe(stdoutPipe);

        for (;;) {
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                return true;
            }
        }
    }

    return true;    
}

#endif