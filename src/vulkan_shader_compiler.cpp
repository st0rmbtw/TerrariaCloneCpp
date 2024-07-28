#include "vulkan_shader_compiler.hpp"

#ifdef _WIN32

#include <Windows.h>
#include <strsafe.h>
#include "common.h"
#include "log.hpp"

#if DEBUG
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
#endif

bool CompileVulkanShader(LLGL::ShaderType shaderType, const char* shaderSource, size_t shaderSourceSize, const char* outputPath)
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
        LOG_ERROR("Couldn't create a pipe");
        return false;
    }

    success = SetHandleInformation(hStdInWr, HANDLE_FLAG_INHERIT, 0);
    if (!success) {
        LOG_ERROR("Couldn't set handle information");
        return false;
    }

    success = CreatePipe(&hStdOutRd, &hStdOutWr, &saAttr, 0);
    if (!success) {
        LOG_ERROR("Couldn't create a pipe");
        return false;
    }

    // success = SetHandleInformation(hStdOutRd, HANDLE_FLAG_INHERIT, 0);
    // ASSERT(success, "ASDASDSd")

    success = WriteFile(hStdInWr, shaderSource, shaderSourceSize, nullptr, NULL);
    if (!success) {
        LOG_ERROR("Couldn't write to the pipe");
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

    std::string stage;
    switch (shaderType) {
        case LLGL::ShaderType::Vertex: stage = "vert";
        break;
        case LLGL::ShaderType::Fragment: stage = "frag";
        break;
        case LLGL::ShaderType::Geometry: stage = "geom";
        break;
        default: UNREACHABLE()
    }

    const std::string args = "glslangValidator.exe -V --stdin -S " + stage + " -o " + outputPath;

    // start the program up
    success = CreateProcessA(
        "./assets/bin/glslangValidator.exe", // the path
        (char*) args.c_str(),                // Command line
        NULL,                   // Process handle not inheritable
        NULL,                   // Thread handle not inheritable
        TRUE,                  // Set handle inheritance to FALSE
        CREATE_NO_WINDOW,     // Opens file in a separate console
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi           // Pointer to PROCESS_INFORMATION structure
    );
    if (!success) {
        LOG_ERROR("Couldn't create a process");
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(hStdOutWr);

#if DEBUG
    ReadFromPipe(hStdOutRd);
#endif

    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hStdInRd);

    CloseHandle(hStdOutRd);

    return true;
}

#endif