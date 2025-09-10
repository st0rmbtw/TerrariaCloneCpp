#include <cstring>
#include <format>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include <optional>

#include <slang.h>
#include <slang-com-ptr.h>
#include <slang-com-helper.h>

#define SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS

#include <spirv.hpp>
#include <spirv_cross.hpp>
#include <spirv_glsl.hpp>

#include "../src/constants.hpp"

namespace fs = std::filesystem;

static bool CompileSlangShaders(const fs::path& shaders_dir, const fs::path& output_dir);
static bool CompileOpenGLShaders(const fs::path& output_dir);

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
        return 1;
    }

    if (argc < 3) {
        printf("Usage: %s <SOURCE_DIRECTORY> <BUILD_DIRECTORY>\n", argv[0]);
        printf("Not enough arguments.\n");
        return 1;
    }

    const fs::path source_dir = argv[1];
    const fs::path build_dir = argv[2];

    const fs::path dir = source_dir / "assets" / "shaders";
    if (!fs::exists(dir)) {
        printf("ERROR: %s doesn't exist.\n", dir.string().c_str());
        return 1;
    }

    const fs::path output_dir = build_dir / "assets" / "shaders";
    
    if (!CompileSlangShaders(dir, output_dir)) return 1;
    if (!CompileOpenGLShaders(output_dir)) return 1;

    return 0;
}

namespace TargetType {
    enum : uint8_t {
        SPIRV = 0,
        HLSL,
        METAL,
        Count
    };
}

static const std::array SHADER_DEFS = std::to_array<ShaderDef>({
    { "SLANG_COMPILING", "1" },
    { "DEF_SUBDIVISION", std::to_string(Constants::SUBDIVISION) },
    { "DEF_SOLID_DECAY", std::to_string(Constants::LightDecay(true)) },
    { "DEF_AIR_DECAY", std::to_string(Constants::LightDecay(false)) },
    { "CHUNK_WIDTH", std::to_string(Constants::RENDER_CHUNK_SIZE_U) },
    { "CHUNK_HEIGHT", std::to_string(Constants::RENDER_CHUNK_SIZE_U) },
    { "TILE_SIZE", std::to_string(Constants::TILE_SIZE) },
    { "WALL_SIZE", std::to_string(Constants::WALL_SIZE) },
});

struct ResultFile {
    std::string name;
    Slang::ComPtr<slang::IBlob> blob;
};

static void DiagnoseIfNeeded(slang::IBlob* diagnosticsBlob) {
    if (diagnosticsBlob != nullptr) {
        printf("%s\n", (const char*)diagnosticsBlob->getBufferPointer());
    }
}

static std::optional<std::vector<ResultFile>> GenerateSpirvTarget(const Slang::ComPtr<slang::ISession>& session, const Slang::ComPtr<slang::IModule>& slangModule) {
    std::vector<ResultFile> resultFiles;

    std::vector<slang::IComponentType*> componentTypes;
    componentTypes.reserve(slangModule->getDefinedEntryPointCount() + 1);
    componentTypes.push_back(slangModule);

    std::vector<slang::FunctionReflection*> entryPointReflections;
    componentTypes.reserve(slangModule->getDefinedEntryPointCount());

    for (SlangInt32 i = 0; i < slangModule->getDefinedEntryPointCount(); ++i) {
        Slang::ComPtr<slang::IEntryPoint> entryPoint;
        {
            Slang::ComPtr<slang::IBlob> diagnosticsBlob;
            slangModule->getDefinedEntryPoint(i, entryPoint.writeRef());
        }

        componentTypes.push_back(entryPoint);
        entryPointReflections.push_back(entryPoint->getFunctionReflection());
    }

    Slang::ComPtr<slang::IComponentType> composedProgram;
    {
        Slang::ComPtr<slang::IBlob> diagnosticsBlob;
        SlangResult result = session->createCompositeComponentType(
            componentTypes.data(),
            componentTypes.size(),
            composedProgram.writeRef(),
            diagnosticsBlob.writeRef());
        DiagnoseIfNeeded(diagnosticsBlob);
    }
    
    Slang::ComPtr<slang::IComponentType> linkedProgram;
    {
        Slang::ComPtr<slang::IBlob> diagnosticsBlob;
        SlangResult result = composedProgram->link(
            linkedProgram.writeRef(),
            diagnosticsBlob.writeRef());
        DiagnoseIfNeeded(diagnosticsBlob);
        
        if (SLANG_FAILED(result)) {
            return std::nullopt;
        }
    }

    for (int i = 0; i < slangModule->getDefinedEntryPointCount(); ++i) {
        slang::FunctionReflection* reflection = entryPointReflections[i];
        slang::Attribute* shaderAttribute = reflection->findAttributeByName(session->getGlobalSession(), "shader");
        const char* entryPointType = shaderAttribute->getArgumentValueString(0, nullptr);
        const char* functionName = reflection->getName();

        if (entryPointType == nullptr) {
            printf("ERROR: Each entry point must have the `shader` attibute. Error caused by '%s'.\n", functionName);
            return std::nullopt;
        }

        if (strcmp(entryPointType, "vertex") == 0) {
            Slang::ComPtr<slang::IBlob> outputBlob;
            {
                Slang::ComPtr<slang::IBlob> diagnosticsBlob;
                SlangResult result = linkedProgram->getEntryPointCode(
                    i, // entryPointIndex
                    TargetType::SPIRV, // targetIndex
                    outputBlob.writeRef(),
                    diagnosticsBlob.writeRef());
                DiagnoseIfNeeded(diagnosticsBlob);
                
                if (SLANG_SUCCEEDED(result)) {
                    std::string name = std::format("{}.vert.spv", slangModule->getName());
                    resultFiles.push_back(ResultFile{ std::move(name), outputBlob });
                }
            }
        } else if (strcmp(entryPointType, "fragment") == 0) {
            Slang::ComPtr<slang::IBlob> outputBlob;
            {
                Slang::ComPtr<slang::IBlob> diagnosticsBlob;
                SlangResult result = linkedProgram->getEntryPointCode(
                    i, // entryPointIndex
                    TargetType::SPIRV, // targetIndex
                    outputBlob.writeRef(),
                    diagnosticsBlob.writeRef());
                DiagnoseIfNeeded(diagnosticsBlob);
                
                if (SLANG_SUCCEEDED(result)) {
                    std::string name = std::format("{}.frag.spv", slangModule->getName());
                    resultFiles.push_back(ResultFile{ std::move(name), outputBlob });
                }
            }
        } else if (strcmp(entryPointType, "compute") == 0) {
            Slang::ComPtr<slang::IBlob> outputBlob;
            {
                Slang::ComPtr<slang::IBlob> diagnosticsBlob;
                SlangResult result = linkedProgram->getEntryPointCode(
                    i, // entryPointIndex
                    TargetType::SPIRV, // targetIndex
                    outputBlob.writeRef(),
                    diagnosticsBlob.writeRef());
                DiagnoseIfNeeded(diagnosticsBlob);
                
                if (SLANG_SUCCEEDED(result)) {
                    std::string name = std::format("{}.{}.comp.spv", functionName, slangModule->getName());
                    resultFiles.push_back(ResultFile{ std::move(name), outputBlob });
                }
            }
        }
    }

    return resultFiles;
}

static std::optional<std::vector<ResultFile>> GenerateHLSLTarget(const Slang::ComPtr<slang::ISession>& session, const Slang::ComPtr<slang::IModule>& slangModule) {
    std::vector<ResultFile> resultFiles;

    std::vector<slang::IComponentType*> componentTypes;
    componentTypes.reserve(slangModule->getDefinedEntryPointCount() + 1);
    componentTypes.push_back(slangModule);

    std::vector<slang::FunctionReflection*> entryPointReflections;
    componentTypes.reserve(slangModule->getDefinedEntryPointCount());

    for (SlangInt32 i = 0; i < slangModule->getDefinedEntryPointCount(); ++i) {
        Slang::ComPtr<slang::IEntryPoint> entryPoint;
        {
            Slang::ComPtr<slang::IBlob> diagnosticsBlob;
            slangModule->getDefinedEntryPoint(i, entryPoint.writeRef());
        }

        componentTypes.push_back(entryPoint);
        entryPointReflections.push_back(entryPoint->getFunctionReflection());
    }

    Slang::ComPtr<slang::IComponentType> composedProgram;
    {
        Slang::ComPtr<slang::IBlob> diagnosticsBlob;
        SlangResult result = session->createCompositeComponentType(
            componentTypes.data(),
            componentTypes.size(),
            composedProgram.writeRef(),
            diagnosticsBlob.writeRef());
        DiagnoseIfNeeded(diagnosticsBlob);
    }
    
    Slang::ComPtr<slang::IComponentType> linkedProgram;
    {
        Slang::ComPtr<slang::IBlob> diagnosticsBlob;
        SlangResult result = composedProgram->link(
            linkedProgram.writeRef(),
            diagnosticsBlob.writeRef());
        DiagnoseIfNeeded(diagnosticsBlob);
        
        if (SLANG_FAILED(result)) {
            return std::nullopt;
        }
    }

    for (int i = 0; i < slangModule->getDefinedEntryPointCount(); ++i) {
        slang::FunctionReflection* reflection = entryPointReflections[i];
        slang::Attribute* shaderAttribute = reflection->findAttributeByName(session->getGlobalSession(), "shader");
        const char* entryPointType = shaderAttribute->getArgumentValueString(0, nullptr);
        const char* functionName = reflection->getName();

        if (entryPointType == nullptr) {
            printf("ERROR: Each entry point must have the `shader` attibute. Error caused by '%s'.\n", functionName);
            return std::nullopt;
        }

        if (strcmp(entryPointType, "vertex") == 0) {
            Slang::ComPtr<slang::IBlob> outputBlob;
            {
                Slang::ComPtr<slang::IBlob> diagnosticsBlob;
                SlangResult result = linkedProgram->getEntryPointCode(
                    i, // entryPointIndex
                    TargetType::HLSL, // targetIndex
                    outputBlob.writeRef(),
                    diagnosticsBlob.writeRef());
                DiagnoseIfNeeded(diagnosticsBlob);
                
                if (SLANG_SUCCEEDED(result)) {
                    std::string name = std::format("{}.vert.hlsl", slangModule->getName());
                    resultFiles.push_back(ResultFile{ std::move(name), outputBlob });
                }
            }
        } else if (strcmp(entryPointType, "fragment") == 0) {
            Slang::ComPtr<slang::IBlob> outputBlob;
            {
                Slang::ComPtr<slang::IBlob> diagnosticsBlob;
                SlangResult result = linkedProgram->getEntryPointCode(
                    i, // entryPointIndex
                    TargetType::HLSL, // targetIndex
                    outputBlob.writeRef(),
                    diagnosticsBlob.writeRef());
                DiagnoseIfNeeded(diagnosticsBlob);
                
                if (SLANG_SUCCEEDED(result)) {
                    std::string name = std::format("{}.frag.hlsl", slangModule->getName());
                    resultFiles.push_back(ResultFile{ std::move(name), outputBlob });
                }
            }
        } else if (strcmp(entryPointType, "compute") == 0) {
            Slang::ComPtr<slang::IBlob> outputBlob;
            {
                Slang::ComPtr<slang::IBlob> diagnosticsBlob;
                SlangResult result = linkedProgram->getEntryPointCode(
                    i, // entryPointIndex
                    TargetType::HLSL, // targetIndex
                    outputBlob.writeRef(),
                    diagnosticsBlob.writeRef());
                DiagnoseIfNeeded(diagnosticsBlob);
                
                if (SLANG_SUCCEEDED(result)) {
                    std::string name = std::format("{}.{}.comp.hlsl", functionName, slangModule->getName());
                    resultFiles.push_back(ResultFile{ std::move(name), outputBlob });
                }
            }
        }
    }

    return resultFiles;
}

static std::optional<std::vector<ResultFile>> GenerateMetalTarget(const Slang::ComPtr<slang::ISession>& session, const Slang::ComPtr<slang::IModule>& slangModule) {
    std::vector<ResultFile> resultFiles;
    
    Slang::ComPtr<slang::IComponentType> linkedProgram;
    {
        Slang::ComPtr<slang::IBlob> diagnosticsBlob;
        SlangResult result = slangModule->link(
            linkedProgram.writeRef(),
            diagnosticsBlob.writeRef());
        DiagnoseIfNeeded(diagnosticsBlob);
        
        if (SLANG_FAILED(result)) {
            return std::nullopt;
        }
    }

    Slang::ComPtr<slang::IBlob> outputBlob;
    {
        Slang::ComPtr<slang::IBlob> diagnosticsBlob;
        SlangResult result = linkedProgram->getTargetCode(
            TargetType::METAL, // targetIndex
            outputBlob.writeRef(),
            diagnosticsBlob.writeRef());
        DiagnoseIfNeeded(diagnosticsBlob);
        
        if (SLANG_SUCCEEDED(result)) {
            std::string name = std::format("{}.metal", slangModule->getName());
            resultFiles.push_back(ResultFile{ std::move(name), outputBlob });
        }
    }

    return resultFiles;
}

static bool CompileSlangShaders(const fs::path& shaders_dir, const fs::path& outputShadersDir) {
    Slang::ComPtr<slang::IGlobalSession> globalSession;
    createGlobalSession(globalSession.writeRef());

    std::vector<slang::PreprocessorMacroDesc> preprocessorMacroDesc;
    preprocessorMacroDesc.reserve(SHADER_DEFS.size());
    for (const ShaderDef& shader_def : SHADER_DEFS) {
        preprocessorMacroDesc.push_back({ shader_def.name.c_str(), shader_def.value.c_str() });
    }

    slang::SessionDesc sessionDesc = {};
    sessionDesc.preprocessorMacros = preprocessorMacroDesc.data();
    sessionDesc.preprocessorMacroCount = preprocessorMacroDesc.size();

#if DEBUG
    static constexpr int DEBUG_INFO = SLANG_DEBUG_INFO_LEVEL_MAXIMAL;
    static constexpr int OPTIMIZATION = SLANG_OPTIMIZATION_LEVEL_NONE;
#else
    static constexpr int DEBUG_INFO = SLANG_DEBUG_INFO_LEVEL_NONE;
    static constexpr int OPTIMIZATION = SLANG_OPTIMIZATION_LEVEL_HIGH;
#endif

    std::array options = std::to_array<slang::CompilerOptionEntry>({
        {
            slang::CompilerOptionName::DebugInformation,
            {slang::CompilerOptionValueKind::Int, DEBUG_INFO, 0, nullptr, nullptr}
        },
        {
            slang::CompilerOptionName::Optimization,
            {slang::CompilerOptionValueKind::Int, OPTIMIZATION, 0, nullptr, nullptr}
        },
        {
            slang::CompilerOptionName::NoMangle,
            {slang::CompilerOptionValueKind::Int, 1, 1, nullptr, nullptr}
        },
    });

    const std::string shaders_dir_str = shaders_dir.string();

    std::array searchPaths = std::to_array({
        shaders_dir_str.c_str()
    });
    
    sessionDesc.compilerOptionEntries = options.data();
    sessionDesc.compilerOptionEntryCount = options.size();
    sessionDesc.searchPaths = searchPaths.data();
    sessionDesc.searchPathCount = searchPaths.size();
    sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;

    std::array<slang::TargetDesc, TargetType::Count> targets;
    targets[TargetType::SPIRV] = {
        .format = SLANG_SPIRV,
        .lineDirectiveMode = SLANG_LINE_DIRECTIVE_MODE_NONE,
        .compilerOptionEntries = options.data(),
        .compilerOptionEntryCount = options.size()
    };
    targets[TargetType::HLSL] = {
        .format = SLANG_HLSL,
        .lineDirectiveMode = SLANG_LINE_DIRECTIVE_MODE_NONE,
        .compilerOptionEntries = options.data(),
        .compilerOptionEntryCount = options.size()
    };
    targets[TargetType::METAL] = {
        .format = SLANG_METAL,
        .lineDirectiveMode = SLANG_LINE_DIRECTIVE_MODE_NONE,
        .compilerOptionEntries = options.data(),
        .compilerOptionEntryCount = options.size()
    };

    sessionDesc.targets = targets.data();
    sessionDesc.targetCount = targets.size();

    Slang::ComPtr<slang::ISession> session;
    globalSession->createSession(sessionDesc, session.writeRef());

    for (const auto& entry : fs::directory_iterator(shaders_dir)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".slang") continue;

        printf("Compiling %s ...\n", entry.path().string().c_str());

        std::ifstream shader(entry.path());
        std::stringstream content;
        content << shader.rdbuf();
        shader.close();

        std::string source = content.str();

        Slang::ComPtr<slang::IModule> slangModule;
        {
            Slang::ComPtr<slang::IBlob> diagnosticsBlob;
            const std::string moduleName = entry.path().stem().string();
            const std::string modulePath = entry.path().filename().string();
            slangModule = session->loadModuleFromSourceString(moduleName.c_str(), modulePath.c_str(), source.c_str(), diagnosticsBlob.writeRef());
            DiagnoseIfNeeded(diagnosticsBlob);
            if (!slangModule) {
                return false;
            }
        }

        static constexpr std::array TARGETS = {
            TargetType::SPIRV,
            TargetType::HLSL,
            TargetType::METAL,
        };

        for (uint8_t targetType : TARGETS) {
            switch (targetType) {
            case TargetType::SPIRV: {
                const fs::path outputDir = outputShadersDir / "vulkan";
                if (!fs::exists(outputDir))
                    fs::create_directories(outputDir);

                auto resultFiles = GenerateSpirvTarget(session, slangModule);
                if (!resultFiles) return false;

                for (const ResultFile& resultFile : resultFiles.value()) {
                    std::ofstream outputFile(outputDir / resultFile.name, std::ios::out | std::ios::binary);
                    outputFile.write(static_cast<const char*>(resultFile.blob->getBufferPointer()), resultFile.blob->getBufferSize());
                    outputFile.close();
                }
            }
            break;
            
            case TargetType::HLSL: {
                const fs::path outputDir = outputShadersDir / "d3d11";
                if (!fs::exists(outputDir))
                    fs::create_directories(outputDir);

                auto resultFiles = GenerateHLSLTarget(session, slangModule);
                if (!resultFiles) return false;

                for (const ResultFile& resultFile : resultFiles.value()) {
                    std::ofstream outputFile(outputDir / resultFile.name, std::ios::out | std::ios::binary);
                    outputFile.write(static_cast<const char*>(resultFile.blob->getBufferPointer()), resultFile.blob->getBufferSize());
                    outputFile.close();
                }
            }
            break;

            case TargetType::METAL: {
                const fs::path outputDir = outputShadersDir / "metal";
                if (!fs::exists(outputDir))
                    fs::create_directories(outputDir);

                auto resultFiles = GenerateMetalTarget(session, slangModule);
                if (!resultFiles) return false;

                for (const ResultFile& resultFile : resultFiles.value()) {
                    std::ofstream outputFile(outputDir / resultFile.name, std::ios::out | std::ios::binary);
                    outputFile.write(static_cast<const char*>(resultFile.blob->getBufferPointer()), resultFile.blob->getBufferSize());
                    outputFile.close();
                }
            }
            break;
            }
        }        
    }

    return true;
}

static std::optional<std::string> TranslateSpirvToGlsl(const fs::path& file_path) {
    std::fstream file(file_path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        printf("Couldn't open file '%s'\n", file_path.string().c_str());
        return std::nullopt;
    }

    const size_t size = fs::file_size(file_path);

    if (size % 4 != 0) {
        printf("The size of the file is not a multiple of 4!\n");
        return std::nullopt;
    }

    std::vector<uint32_t> spirv;
    spirv.reserve(size / 4);

    uint32_t words[32];
    do {
        file.read(reinterpret_cast<char*>(words), sizeof(words));
        spirv.insert(spirv.end(), words, words + file.gcount() / sizeof(uint32_t));
    } while (file.gcount() > 0);

    spirv_cross::CompilerGLSL compiler(spirv);
    spirv_cross::CompilerGLSL::Options options;
    {
        options.version = 430;
        options.es = false;
        options.enable_420pack_extension = false;
        options.emit_line_directives = false;
        options.enable_row_major_load_workaround = false;
    }
    compiler.set_common_options(options);

    compiler.build_dummy_sampler_for_combined_images();
    compiler.build_combined_image_samplers();

    for (auto& sampler : compiler.get_combined_image_samplers()) {
        const std::string& name = compiler.get_name(sampler.image_id);
        compiler.set_name(sampler.combined_id, name);
    }

    const spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    for (auto& resource : resources.uniform_buffers) {
        const std::string& name = compiler.get_name(resource.id);
        compiler.set_name(resource.base_type_id, name);
    }

    for (auto& resource : resources.storage_buffers) {
        const std::string& name = compiler.get_name(resource.id);
        compiler.set_name(resource.base_type_id, name);
    }

    return compiler.compile();
}

static bool CompileOpenGLShaders(const fs::path& build_dir) {
    const fs::path shaders_dir = build_dir / "vulkan";
    const fs::path output_dir = build_dir / "opengl";

    fs::create_directories(output_dir);

    for (const auto& entry : fs::directory_iterator(shaders_dir)) {
        if (!entry.is_regular_file())
            continue;

        const fs::path& path = entry.path();

        const auto result = TranslateSpirvToGlsl(path);
        if (!result)
            return false;

        const fs::path filename = path.filename().replace_extension(".glsl");

        std::ofstream outputFile(output_dir / filename, std::ios::out | std::ios::binary);
        outputFile.write(result->data(), result->size());
        outputFile.close();
    }

    return true;
}