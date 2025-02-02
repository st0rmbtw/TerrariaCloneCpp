#ifndef _ENGINE_RENDERER_HPP_
#define _ENGINE_RENDERER_HPP_

#include <GLFW/glfw3.h>

#include <LLGL/Shader.h>
#include <LLGL/RenderSystem.h>
#include <LLGL/Utils/Utility.h>

#include "../types/backend.hpp"
#include "../types/texture.hpp"
#include "../types/shader_path.hpp"

#include "custom_surface.hpp"
#include "batch.hpp"
#include "macros.hpp"
#include "camera.hpp"

struct ALIGN(16) ProjectionsUniform {
    glm::mat4 screen_projection_matrix;
    glm::mat4 view_projection_matrix;
    glm::mat4 nonscale_view_projection_matrix;
    glm::mat4 nonscale_projection_matrix;
    glm::mat4 transform_matrix;
    glm::mat4 inv_view_proj_matrix;
    glm::vec2 camera_position;
    glm::vec2 window_size;
};

template <typename Container>
static inline std::size_t GetArraySize(const Container& container)
{
    return (container.size() * sizeof(typename Container::value_type));
}

template <typename T, std::size_t N>
static inline std::size_t GetArraySize(const T (&)[N])
{
    return (N * sizeof(T));
}

class Renderer {
public:
    bool InitEngine(RenderBackend backend);
    bool Init(GLFWwindow* window, const LLGL::Extent2D& resolution, bool vsync, bool fullscreen);

    void Begin(const Camera& camera);
    void End();

    void RenderBatch(Batch& batch);

    Texture CreateTexture(LLGL::TextureType type, LLGL::ImageFormat image_format, LLGL::DataType data_type, uint32_t width, uint32_t height, uint32_t layers, int sampler, const void* data, bool generate_mip_maps = false);

    Texture CreateTexture(LLGL::TextureType type, LLGL::ImageFormat image_format, uint32_t width, uint32_t height, uint32_t layers, int sampler, const uint8_t* data, bool generate_mip_maps = false) {
        return CreateTexture(type, image_format, LLGL::DataType::UInt8, width, height, layers, sampler, data, generate_mip_maps);
    }
    Texture CreateTexture(LLGL::TextureType type, LLGL::ImageFormat image_format, uint32_t width, uint32_t height, uint32_t layers, int sampler, const int8_t* data, bool generate_mip_maps = false) {
        return CreateTexture(type, image_format, LLGL::DataType::Int8, width, height, layers, sampler, data, generate_mip_maps);
    }

    LLGL::Shader* LoadShader(const ShaderPath& shader_path, const std::vector<ShaderDef>& shader_defs = {}, const std::vector<LLGL::VertexAttribute>& vertex_attributes = {});

    void Terminate();

    #if DEBUG
        void PrintDebugInfo();
    #endif

    template <typename Container>
    inline LLGL::Buffer* CreateVertexBuffer(const Container& vertices, const LLGL::VertexFormat& vertexFormat, const char* debug_name = nullptr)
    {
        LLGL::BufferDescriptor bufferDesc = LLGL::VertexBufferDesc(GetArraySize(vertices), vertexFormat);
        bufferDesc.debugName = debug_name;
        return m_context->CreateBuffer(bufferDesc, &vertices[0]);
    }

    inline LLGL::Buffer* CreateVertexBuffer(size_t size, const LLGL::VertexFormat& vertexFormat, const char* debug_name = nullptr)
    {
        LLGL::BufferDescriptor bufferDesc = LLGL::VertexBufferDesc(size, vertexFormat);
        bufferDesc.debugName = debug_name;
        return m_context->CreateBuffer(bufferDesc, nullptr);
    }

    inline LLGL::Buffer* CreateVertexBufferInit(size_t size, const void* data, const LLGL::VertexFormat& vertexFormat, const char* debug_name = nullptr)
    {
        LLGL::BufferDescriptor bufferDesc = LLGL::VertexBufferDesc(size, vertexFormat);
        bufferDesc.debugName = debug_name;
        return m_context->CreateBuffer(bufferDesc, data);
    }

    template <typename Container>
    inline LLGL::Buffer* CreateIndexBuffer(const Container& indices, const LLGL::Format format, const char* debug_name = nullptr)
    {
        LLGL::BufferDescriptor bufferDesc = LLGL::IndexBufferDesc(GetArraySize(indices), format);
        bufferDesc.debugName = debug_name;
        return m_context->CreateBuffer(bufferDesc, &indices[0]);
    }

    inline LLGL::Buffer* CreateConstantBuffer(const size_t size, const char* debug_name = nullptr)
    {
        LLGL::BufferDescriptor bufferDesc = LLGL::ConstantBufferDesc(size);
        bufferDesc.debugName = debug_name;
        return m_context->CreateBuffer(bufferDesc);
    }

    inline void UpdateBuffer(LLGL::Buffer* buffer, void* data, size_t length) {
        if (length < (1 << 16)) {
            m_command_buffer->UpdateBuffer(*buffer, 0, data, length);
        } else {
            m_context->WriteBuffer(*buffer, 0, data, length);
        }
    }

    [[nodiscard]] inline const LLGL::RenderSystemPtr& Context() const { return m_context; }
    [[nodiscard]] inline LLGL::SwapChain* SwapChain() const { return m_swap_chain; };
    [[nodiscard]] inline LLGL::CommandBuffer* CommandBuffer() const { return m_command_buffer; };
    [[nodiscard]] inline LLGL::CommandQueue* CommandQueue() const { return m_command_queue; };
    [[nodiscard]] inline const std::shared_ptr<CustomSurface>& Surface() const { return m_surface; };
    [[nodiscard]] inline LLGL::Buffer* GlobalUniformBuffer() const { return m_constant_buffer; };
    [[nodiscard]] inline RenderBackend Backend() const { return m_backend; };

#if DEBUG
    [[nodiscard]] inline LLGL::RenderingDebugger* Debugger() const { return m_debugger; }
#endif

private:
    void InitSpriteBatchPipeline();
    void InitGlyphBatchPipeline();
    void InitNinepatchBatchPipeline();

    void ApplyBatchDrawCommands(Batch& batch);

private:
    struct {
        LLGL::PipelineState* pipeline = nullptr;
        LLGL::PipelineState* pipeline_depth = nullptr;

        LLGL::Buffer* vertex_buffer = nullptr;
        LLGL::Buffer* instance_buffer = nullptr;
        LLGL::BufferArray* buffer_array = nullptr;
    } m_sprite_batch_data;

    struct {
        LLGL::PipelineState* pipeline = nullptr;

        LLGL::Buffer* vertex_buffer = nullptr;
        LLGL::Buffer* instance_buffer = nullptr;
        LLGL::BufferArray* buffer_array = nullptr;
    } m_glyph_batch_data;

    struct NinePatchBatchData {
        LLGL::PipelineState* pipeline = nullptr;

        LLGL::Buffer* vertex_buffer = nullptr;
        LLGL::Buffer* instance_buffer = nullptr;
        LLGL::BufferArray* buffer_array = nullptr;

    } m_ninepatch_batch_data;

    LLGL::RenderSystemPtr m_context = nullptr;
    std::shared_ptr<CustomSurface> m_surface = nullptr;

    LLGL::SwapChain* m_swap_chain = nullptr;
    LLGL::CommandBuffer* m_command_buffer = nullptr;
    LLGL::CommandQueue* m_command_queue = nullptr;
    LLGL::Buffer* m_constant_buffer = nullptr;

#if DEBUG
    LLGL::RenderingDebugger* m_debugger = nullptr;
#endif

    uint32_t m_texture_index;

    RenderBackend m_backend;
};

#endif