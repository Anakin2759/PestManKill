/**
 * ************************************************************************
 *
 * @file SdlGpuRenderSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-13
 * @version 0.1
 * @brief SDL GPU 渲染系统

  - 负责使用 SDL3 GPU 模块进行 UI 渲染
  - 响应 UI 渲染事件，执行实际渲染操作

  加载assets/sharder/spir-v文件
  以后用来替换RenderSystem，使用SDL3 GPU进行渲染,为后续移除imGui SDL Renderer做准备

  支持的图形后端列表
    - Vulkan
    - DX12

    使用SDL ttf渲染字体到GPU纹理上
    默认字体用 assets/fonts/NotoSansSC-VariableFont_wght.ttf
    静态资源都用cmrc打包

 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <entt/entt.hpp>
#include <Eigen/Dense>
#include <cmrc/cmrc.hpp>
#include <utils.h>
#include "Logger.h"
#include "common/Components.h"
#include "common/Tags.h"
#include "common/Events.h"
#include "interface/Isystem.h"
#include "core/GraphicsContext.h"
#include "core/Helper.h"

CMRC_DECLARE(ui_fonts);

namespace ui
{
class GraphicsContext;
}

namespace ui::systems
{
class GPUBackendHandler
{
protected:
    std::shared_ptr<GPUBackendHandler> nextHandler;

public:
    GPUBackendHandler() = default;
    GPUBackendHandler(const GPUBackendHandler&) = delete;
    GPUBackendHandler& operator=(const GPUBackendHandler&) = delete;
    GPUBackendHandler(GPUBackendHandler&&) = delete;
    GPUBackendHandler& operator=(GPUBackendHandler&&) = delete;
    virtual ~GPUBackendHandler() = default;

    // 设置链中的下一个处理器
    void setNext(std::shared_ptr<GPUBackendHandler> next) { nextHandler = next; }

    // 处理请求的核心逻辑
    virtual SDL_GPUDevice* handle(SDL_Window* window, std::string& outDriverName)
    {
        if (nextHandler)
        {
            return nextHandler->handle(window, outDriverName);
        }
        return nullptr;
    }
};
class D3D12Handler final : public GPUBackendHandler
{
public:
    SDL_GPUDevice* handle(SDL_Window* window, std::string& outDriverName) override
    {
        LOG_INFO("责任链：尝试初始化 D3D12...");

        SDL_PropertiesID props = SDL_CreateProperties();
        SDL_SetStringProperty(props, SDL_PROP_GPU_DEVICE_CREATE_NAME_STRING, "direct3d12");
        SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, true);
        SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_DXIL_BOOLEAN, true);

        SDL_GPUDevice* device = SDL_CreateGPUDeviceWithProperties(props);
        SDL_DestroyProperties(props);

        if (device != nullptr)
        {
            outDriverName = "direct3d12";
            return device;
        }

        LOG_WARN("D3D12 不可用，传递给链中下一个处理器。原因: {}", SDL_GetError());
        return GPUBackendHandler::handle(window, outDriverName);
    }
};
class VulkanHandler final : public GPUBackendHandler
{
public:
    SDL_GPUDevice* handle(SDL_Window* window, std::string& outDriverName) override
    {
        LOG_INFO("责任链：尝试初始化 Vulkan...");

        SDL_PropertiesID props = SDL_CreateProperties();
        SDL_SetStringProperty(props, SDL_PROP_GPU_DEVICE_CREATE_NAME_STRING, "vulkan");
        SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, false);
        SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);

        SDL_GPUDevice* device = SDL_CreateGPUDeviceWithProperties(props);
        SDL_DestroyProperties(props);

        if (device)
        {
            outDriverName = "vulkan";
            return device;
        }

        LOG_WARN("Vulkan 不可用。原因: {}", SDL_GetError());
        return GPUBackendHandler::handle(window, outDriverName);
    }
};

/**
 * @brief Push Constants 结构体，与着色器 common.hlsl 中的定义对应
 * 必须保证 16 字节对齐，字段顺序必须与 HLSL PushConstants 完全一致
 */
struct alignas(16) UiPushConstants
{
    float screen_size[2];  // 屏幕尺寸 (float2)
    float rect_size[2];    // 矩形尺寸 (float2)
    float radius[4];       // 四角圆角 (float4: 左上, 右上, 右下, 左下)
    float shadow_soft;     // 阴影柔和度
    float shadow_offset_x; // 阴影 X 偏移
    float shadow_offset_y; // 阴影 Y 偏移
    float opacity;         // 整体透明度
    float _padding;        // 填充到 16 字节倍数
};
/**
 * @brief SDL GPU 渲染系统
 *
 * 使用 SDL3 GPU API 实现高性能 UI 渲染，替代 ImGui DrawList
 */
class SdlGpuRenderSystem final : public interface::EnableRegister<SdlGpuRenderSystem>
{
private:
    GraphicsContext* m_graphicsContext = nullptr;
    SDL_GPUDevice* m_gpuDevice = nullptr;
    SDL_GPUGraphicsPipeline* m_pipeline = nullptr;
    SDL_GPUShader* m_vertexShader = nullptr;
    SDL_GPUShader* m_fragmentShader = nullptr;
    SDL_GPUSampler* m_sampler = nullptr;
    TTF_Font* m_defaultFont = nullptr;
    std::string m_gpuDriver; // GPU 驱动名称（vulkan 或 d3d12）

    // 渲染缓冲区 - 顶点结构与着色器 VSInput 对应
    struct Vertex
    {
        float position[2]; // POSITION
        float texCoord[2]; // TEXCOORD0
        float color[4];    // COLOR0
    };

    // 渲染批次，每个批次对应一个 Draw Call 和独立的 Push Constants
    struct RenderBatch
    {
        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;
        UiPushConstants pushConstants;
        SDL_GPUTexture* texture = nullptr;
    };

    std::vector<RenderBatch> m_batches;
    SDL_GPUBuffer* m_vertexBuffer = nullptr;
    SDL_GPUBuffer* m_indexBuffer = nullptr;
    SDL_GPUTexture* m_whiteTexture = nullptr; // 默认白色纹理用于纯色渲染

    // 纹理缓存（用于文本）
    struct CachedTexture
    {
        SDL_GPUTexture* texture = nullptr;
        uint32_t width = 0;
        uint32_t height = 0;
    };
    std::unordered_map<std::string, CachedTexture> m_textureCache;

    // 当前屏幕尺寸
    float m_screenWidth = 0.0F;
    float m_screenHeight = 0.0F;

public:
    SdlGpuRenderSystem() = default;

    ~SdlGpuRenderSystem() { cleanup(); }

    /**
     * @brief 注册事件处理器
     */
    void registerHandlersImpl()
    {
        auto& dispatcher = utils::Dispatcher::getInstance();
        dispatcher.sink<events::GraphicsContextSetEvent>().connect<&SdlGpuRenderSystem::onGraphicsContextSet>(*this);
        dispatcher.sink<ui::events::UpdateRendering>().connect<&SdlGpuRenderSystem::update>(*this);
    }

    /**
     * @brief 注销事件处理器
     */
    void unregisterHandlersImpl()
    {
        auto& dispatcher = utils::Dispatcher::getInstance();
        dispatcher.sink<events::GraphicsContextSetEvent>().disconnect<&SdlGpuRenderSystem::onGraphicsContextSet>(*this);
        dispatcher.sink<ui::events::UpdateRendering>().disconnect<&SdlGpuRenderSystem::update>(*this);
    }

private:
    /**
     * @brief 处理图形上下文设置事件
     */
    void onGraphicsContextSet(const events::GraphicsContextSetEvent& event)
    {
        m_graphicsContext = static_cast<GraphicsContext*>(event.graphicsContext);

        if (m_graphicsContext != nullptr)
        {
            initializeGPU();
            loadShaders();
            createPipeline();
            loadDefaultFont();
        }
    }

    /**
     * @brief 初始化 SDL GPU 设备
     */
    void initializeGPU()
    {
        if (m_graphicsContext == nullptr || m_graphicsContext->getWindow() == nullptr) return;

        // 1. 组装责任链
        auto d3d12 = std::make_shared<D3D12Handler>();
        auto vulkan = std::make_shared<VulkanHandler>();

        // 设置顺序：D3D12 -> Vulkan
        d3d12->setNext(vulkan);

        // 2. 启动链式处理
        std::string finalDriverName;
        m_gpuDevice = d3d12->handle(m_graphicsContext->getWindow(), finalDriverName);

        // 3. 结果检查
        if (m_gpuDevice == nullptr)
        {
            LOG_ERROR("所有 GPU 后端均初始化失败！");
            return;
        }

        m_gpuDriver = finalDriverName;

        // 4. 声明窗口
        if (!SDL_ClaimWindowForGPUDevice(m_gpuDevice, m_graphicsContext->getWindow()))
        {
            LOG_ERROR("窗口声明失败: {}", SDL_GetError());
            SDL_DestroyGPUDevice(m_gpuDevice);
            m_gpuDevice = nullptr;
            return;
        }

        LOG_INFO("GPU 初始化成功，最终使用后端: {}", m_gpuDriver);
    }

    /**
     * @brief 加载 SPIR-V 着色器（从 cmrc 嵌入资源）
     */
    void loadShaders()
    {
        if (m_gpuDevice == nullptr) return;

        // 根据驱动类型选择着色器格式（使用已在 initializeGPU 中设置的 m_gpuDriver）
        bool isVulkan = (m_gpuDriver == "vulkan");

        if (isVulkan)
        {
            // 加载 SPIR-V 着色器（Vulkan）
            m_vertexShader = loadShaderFromResource(
                "assets/shader/vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, SDL_GPU_SHADERFORMAT_SPIRV);
            m_fragmentShader = loadShaderFromResource(
                "assets/shader/frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, SDL_GPU_SHADERFORMAT_SPIRV);
        }
        else
        {
            // 加载 DXIL 着色器（DX12）
            m_vertexShader = loadShaderFromResource(
                "assets/shader/vert.dxil", SDL_GPU_SHADERSTAGE_VERTEX, SDL_GPU_SHADERFORMAT_DXIL);
            m_fragmentShader = loadShaderFromResource(
                "assets/shader/frag.dxil", SDL_GPU_SHADERSTAGE_FRAGMENT, SDL_GPU_SHADERFORMAT_DXIL);
        }

        if (m_vertexShader == nullptr || m_fragmentShader == nullptr)
        {
            LOG_ERROR("着色器加载失败 (驱动: {}, 顶点着色器: {}, 片段着色器: {})",
                      m_gpuDriver,
                      (m_vertexShader != nullptr ? "成功" : "失败"),
                      (m_fragmentShader != nullptr ? "成功" : "失败"));
        }
        else
        {
            LOG_INFO("着色器加载成功 (驱动: {})", m_gpuDriver);
        }
    }

    /**
     * @brief 从 cmrc 嵌入资源加载着色器
     */
    SDL_GPUShader*
        loadShaderFromResource(const char* resourcePath, SDL_GPUShaderStage stage, SDL_GPUShaderFormat format)
    {
        auto fs = cmrc::ui_fonts::get_filesystem();

        if (!fs.exists(resourcePath))
        {
            LOG_ERROR("着色器资源未找到: {}", resourcePath);
            return nullptr;
        }

        auto file = fs.open(resourcePath);

        SDL_GPUShaderCreateInfo shaderInfo = {};
        shaderInfo.code = reinterpret_cast<const uint8_t*>(file.begin());
        shaderInfo.code_size = static_cast<size_t>(file.size());
        // 着色器入口点：顶点着色器是 main_vs，片段着色器是 main_ps
        shaderInfo.entrypoint = (stage == SDL_GPU_SHADERSTAGE_VERTEX) ? "main_vs" : "main_ps";
        shaderInfo.format = format;
        shaderInfo.stage = stage;
        // 片段着色器使用 1 个采样器（用于纹理采样）
        shaderInfo.num_samplers = (stage == SDL_GPU_SHADERSTAGE_FRAGMENT) ? 1u : 0u;
        // 不使用 storage textures/buffers
        shaderInfo.num_storage_textures = 0u;
        shaderInfo.num_storage_buffers = 0u;
        // 使用 uniform buffers 传递 Push Constants 数据
        shaderInfo.num_uniform_buffers = 1u;

        SDL_GPUShader* shader = SDL_CreateGPUShader(m_gpuDevice, &shaderInfo);
        if (shader == nullptr)
        {
            LOG_ERROR("着色器创建失败: {} (错误: {})", resourcePath, SDL_GetError());
        }
        else
        {
            LOG_INFO("着色器创建成功: {} (大小: {} 字节)", resourcePath, file.size());
        }
        return shader;
    }

    /**
     * @brief 创建渲染管线
     */
    void createPipeline()
    {
        if (m_gpuDevice == nullptr || m_vertexShader == nullptr || m_fragmentShader == nullptr)
        {
            return;
        }

        // 顶点属性描述
        SDL_GPUVertexAttribute vertexAttributes[3] = {};

        // Position (vec2)
        vertexAttributes[0].location = 0;
        vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        vertexAttributes[0].buffer_slot = 0;
        vertexAttributes[0].offset = static_cast<uint32_t>(offsetof(Vertex, position));

        // TexCoord (vec2)
        vertexAttributes[1].location = 1;
        vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        vertexAttributes[1].buffer_slot = 0;
        vertexAttributes[1].offset = static_cast<uint32_t>(offsetof(Vertex, texCoord));

        // Color (vec4)
        vertexAttributes[2].location = 2;
        vertexAttributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
        vertexAttributes[2].buffer_slot = 0;
        vertexAttributes[2].offset = static_cast<uint32_t>(offsetof(Vertex, color));

        SDL_GPUVertexBufferDescription vertexBufferDesc = {};
        vertexBufferDesc.slot = 0;
        vertexBufferDesc.pitch = sizeof(Vertex);
        vertexBufferDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
        vertexBufferDesc.instance_step_rate = 0;

        SDL_GPUVertexInputState vertexInputState = {};
        vertexInputState.vertex_buffer_descriptions = &vertexBufferDesc;
        vertexInputState.num_vertex_buffers = 1;
        vertexInputState.vertex_attributes = vertexAttributes;
        vertexInputState.num_vertex_attributes = 3;

        // 颜色附件描述 - 使用预乘 Alpha 混合模式（与着色器输出匹配）
        SDL_GPUColorTargetBlendState blendState = {};
        blendState.enable_blend = true;
        // 预乘 Alpha: srcColor * 1 + dstColor * (1 - srcAlpha)
        blendState.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        blendState.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        blendState.color_blend_op = SDL_GPU_BLENDOP_ADD;
        blendState.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        blendState.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        blendState.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
        blendState.color_write_mask =
            SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A;
        blendState.enable_color_write_mask = true;

        SDL_GPUColorTargetDescription colorTargetDesc = {}; // 颜色目标描述
        colorTargetDesc.format =
            SDL_GetGPUSwapchainTextureFormat(m_gpuDevice, m_graphicsContext->getWindow()); // 颜色格式
        colorTargetDesc.blend_state = blendState;

        // 光栅化状态
        SDL_GPURasterizerState rasterizerState = {};
        rasterizerState.fill_mode = SDL_GPU_FILLMODE_FILL;
        rasterizerState.cull_mode = SDL_GPU_CULLMODE_NONE;
        rasterizerState.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
        rasterizerState.enable_depth_clip = true;

        // 多重采样状态（默认不开启）
        SDL_GPUMultisampleState multisampleState = {};
        multisampleState.sample_count = SDL_GPU_SAMPLECOUNT_1;
        multisampleState.sample_mask = 0;
        multisampleState.enable_mask = false;
        multisampleState.enable_alpha_to_coverage = false;

        // 深度/模板状态（不使用深度/模板）
        SDL_GPUStencilOpState stencilState = {};
        stencilState.fail_op = SDL_GPU_STENCILOP_KEEP;
        stencilState.pass_op = SDL_GPU_STENCILOP_KEEP;
        stencilState.depth_fail_op = SDL_GPU_STENCILOP_KEEP;
        stencilState.compare_op = SDL_GPU_COMPAREOP_ALWAYS;

        SDL_GPUDepthStencilState depthStencilState = {};
        depthStencilState.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
        depthStencilState.back_stencil_state = stencilState;
        depthStencilState.front_stencil_state = stencilState;
        depthStencilState.compare_mask = 0xFF;
        depthStencilState.write_mask = 0xFF;
        depthStencilState.enable_depth_test = false;
        depthStencilState.enable_depth_write = false;
        depthStencilState.enable_stencil_test = false;

        // 创建图形管线
        SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.vertex_shader = m_vertexShader;                                   // 顶点着色器
        pipelineInfo.fragment_shader = m_fragmentShader;                               // 片段着色器
        pipelineInfo.vertex_input_state = vertexInputState;                            // 顶点输入状态
        pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;              // 图元类型
        pipelineInfo.rasterizer_state = rasterizerState;                               // 光栅化状态
        pipelineInfo.multisample_state = multisampleState;                             // 多重采样状态
        pipelineInfo.depth_stencil_state = depthStencilState;                          // 深度/模板状态
        pipelineInfo.target_info.num_color_targets = 1;                                // 设置为1个颜色目标
        pipelineInfo.target_info.color_target_descriptions = &colorTargetDesc;         // 颜色目标描述
        pipelineInfo.target_info.has_depth_stencil_target = false;                     // 不使用深度模板缓冲
        pipelineInfo.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_INVALID; // 无深度模板格式

        m_pipeline = SDL_CreateGPUGraphicsPipeline(m_gpuDevice, &pipelineInfo);

        if (m_pipeline == nullptr)
        {
            LOG_ERROR("图形管线创建失败: {}", SDL_GetError());
        }
        else
        {
            LOG_INFO("图形管线创建成功");
        }

        // 创建采样器
        SDL_GPUSamplerCreateInfo samplerInfo = {};
        samplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
        samplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
        samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
        samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

        m_sampler = SDL_CreateGPUSampler(m_gpuDevice, &samplerInfo);
    }

    /**
     * @brief 加载默认字体（从 cmrc 嵌入资源）
     */
    void loadDefaultFont()
    {
        if (!TTF_Init())
        {
            LOG_ERROR("Failed to initialize SDL_ttf: {}", SDL_GetError());
            return;
        }

        // 从 cmrc 嵌入资源加载字体
        auto fs = cmrc::ui_fonts::get_filesystem();
        const char* fontPath = "assets/fonts/NotoSansSC-VariableFont_wght.ttf";

        if (!fs.exists(fontPath))
        {
            LOG_ERROR("Font resource not found: {}", fontPath);
            return;
        }

        auto fontFile = fs.open(fontPath);

        // 使用 SDL_IOFromConstMem 从内存加载字体
        SDL_IOStream* fontIO = SDL_IOFromConstMem(fontFile.begin(), static_cast<size_t>(fontFile.size()));
        if (fontIO == nullptr)
        {
            LOG_INFO("Failed to create IOStream for font: %s", SDL_GetError());
            return;
        }

        m_defaultFont = TTF_OpenFontIO(fontIO, true, 16.0F);
        if (m_defaultFont == nullptr)
        {
            LOG_INFO("Failed to load default font: %s", SDL_GetError());
        }
    }

    /**
     * @brief 清理资源
     */
    void cleanup()
    {
        if (m_gpuDevice == nullptr) return;

        // 等待 GPU 空闲
        SDL_WaitForGPUIdle(m_gpuDevice);

        // 清理纹理缓存
        for (auto& [key, entry] : m_textureCache)
        {
            if (entry.texture != nullptr)
            {
                SDL_ReleaseGPUTexture(m_gpuDevice, entry.texture);
            }
        }
        m_textureCache.clear();

        // 清理白色纹理
        if (m_whiteTexture != nullptr)
        {
            SDL_ReleaseGPUTexture(m_gpuDevice, m_whiteTexture);
            m_whiteTexture = nullptr;
        }

        // 清理 GPU 资源
        if (m_vertexBuffer != nullptr)
        {
            SDL_ReleaseGPUBuffer(m_gpuDevice, m_vertexBuffer);
            m_vertexBuffer = nullptr;
        }

        if (m_indexBuffer != nullptr)
        {
            SDL_ReleaseGPUBuffer(m_gpuDevice, m_indexBuffer);
            m_indexBuffer = nullptr;
        }

        if (m_sampler != nullptr)
        {
            SDL_ReleaseGPUSampler(m_gpuDevice, m_sampler);
            m_sampler = nullptr;
        }

        if (m_pipeline != nullptr)
        {
            SDL_ReleaseGPUGraphicsPipeline(m_gpuDevice, m_pipeline);
            m_pipeline = nullptr;
        }

        if (m_vertexShader != nullptr)
        {
            SDL_ReleaseGPUShader(m_gpuDevice, m_vertexShader);
            m_vertexShader = nullptr;
        }

        if (m_fragmentShader != nullptr)
        {
            SDL_ReleaseGPUShader(m_gpuDevice, m_fragmentShader);
            m_fragmentShader = nullptr;
        }

        if (m_defaultFont != nullptr)
        {
            TTF_CloseFont(m_defaultFont);
            m_defaultFont = nullptr;
        }

        // 释放窗口声明
        if (m_graphicsContext != nullptr && m_graphicsContext->getWindow() != nullptr)
        {
            SDL_ReleaseWindowFromGPUDevice(m_gpuDevice, m_graphicsContext->getWindow());
        }

        if (m_gpuDevice != nullptr)
        {
            SDL_DestroyGPUDevice(m_gpuDevice);
            m_gpuDevice = nullptr;
        }

        TTF_Quit();
    }

    /**
     * @brief 创建默认白色纹理（用于纯色渲染）
     */
    void createWhiteTexture()
    {
        if (m_gpuDevice == nullptr) return;

        SDL_GPUTextureCreateInfo texInfo = {};
        texInfo.type = SDL_GPU_TEXTURETYPE_2D;
        texInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        texInfo.width = 1;
        texInfo.height = 1;
        texInfo.layer_count_or_depth = 1;
        texInfo.num_levels = 1;
        texInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

        m_whiteTexture = SDL_CreateGPUTexture(m_gpuDevice, &texInfo);
        if (m_whiteTexture == nullptr) return;

        // 上传 1x1 白色像素
        uint32_t whitePixel = 0xFFFFFFFF;
        SDL_GPUTransferBufferCreateInfo transferInfo = {};
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferInfo.size = sizeof(whitePixel);

        SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(m_gpuDevice, &transferInfo);

        void* data = SDL_MapGPUTransferBuffer(m_gpuDevice, transfer, false);
        SDL_memcpy(data, &whitePixel, sizeof(whitePixel));
        SDL_UnmapGPUTransferBuffer(m_gpuDevice, transfer);

        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(m_gpuDevice);
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);

        SDL_GPUTextureTransferInfo srcInfo = {};
        srcInfo.transfer_buffer = transfer;
        srcInfo.pixels_per_row = 1;
        srcInfo.rows_per_layer = 1;

        SDL_GPUTextureRegion dstRegion = {};
        dstRegion.texture = m_whiteTexture;
        dstRegion.w = 1;
        dstRegion.h = 1;
        dstRegion.d = 1;

        SDL_UploadToGPUTexture(copyPass, &srcInfo, &dstRegion, false);
        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(cmd);
        SDL_ReleaseGPUTransferBuffer(m_gpuDevice, transfer);
    }

public:
    /**
     * @brief 主渲染更新函数
     */
    void update() noexcept
    {
        if (m_gpuDevice == nullptr || m_pipeline == nullptr || m_graphicsContext == nullptr)
        {
            return;
        }

        // 延迟创建白色纹理
        if (m_whiteTexture == nullptr)
        {
            createWhiteTexture();
        }

        auto& registry = utils::Registry::getInstance();

        // 更新主窗口大小
        int width = 0;
        int height = 0;
        SDL_GetWindowSizeInPixels(m_graphicsContext->getWindow(), &width, &height);

        m_screenWidth = static_cast<float>(width);
        m_screenHeight = static_cast<float>(height);

        // 更新主 Widget 大小
        auto canvasView = registry.view<components::MainWidgetTag, components::Size>();
        for (auto entity : canvasView)
        {
            auto& size = canvasView.get<components::Size>(entity);
            size.autoSize = false;
            if (size.size.x() != m_screenWidth || size.size.y() != m_screenHeight)
            {
                size.size = {m_screenWidth, m_screenHeight};
                registry.emplace_or_replace<components::LayoutDirtyTag>(entity);
            }
        }

        // 清空渲染批次
        m_batches.clear();

        // 遍历顶层实体并收集渲染数据
        auto view = registry.view<const components::Position,
                                  const components::Size,
                                  const components::VisibleTag,
                                  const components::Hierarchy>();

        for (auto entity : view)
        {
            const auto& hierarchy = registry.get<const components::Hierarchy>(entity);
            if (hierarchy.parent == entt::null)
            {
                collectRenderData(registry, entity, Eigen::Vector2f(0, 0), 1.0F);
            }
        }

        // 执行 GPU 渲染
        if (!m_batches.empty())
        {
            renderToGPU(width, height);
        }

        // 清除脏标记
        auto dirtyView = registry.view<components::RenderDirtyTag>();
        for (auto entity : dirtyView)
        {
            registry.remove<components::RenderDirtyTag>(entity);
        }
    }

private:
    /**
     * @brief 递归收集实体渲染数据
     */
    void collectRenderData(entt::registry& registry,
                           entt::entity entity,
                           const Eigen::Vector2f& parentPos,
                           float parentAlpha)
    {
        if (!registry.any_of<components::VisibleTag>(entity)) return;
        if (registry.any_of<components::SpacerTag>(entity)) return;

        const auto& pos = registry.get<const components::Position>(entity);
        const auto& size = registry.get<const components::Size>(entity);
        const auto* alphaComp = registry.try_get<components::Alpha>(entity);

        float globalAlpha = parentAlpha * (alphaComp ? alphaComp->value : 1.0F);
        Eigen::Vector2f absolutePos = parentPos + pos.value;

        // Window/Dialog 目前按普通容器渲染（背景 + 子元素），后续再做特殊处理

        // 收集背景渲染数据
        collectBackgroundData(registry, entity, absolutePos, size.size, globalAlpha);

        // 收集特定组件渲染数据
        collectComponentData(registry, entity, absolutePos, size.size, globalAlpha);

        // 递归处理子元素
        const auto* hierarchy = registry.try_get<components::Hierarchy>(entity);
        if (hierarchy && !hierarchy->children.empty())
        {
            for (entt::entity child : hierarchy->children)
            {
                collectRenderData(registry, child, absolutePos, globalAlpha);
            }
        }
    }

    /**
     * @brief 收集背景渲染数据
     */
    void collectBackgroundData(entt::registry& registry,
                               entt::entity entity,
                               const Eigen::Vector2f& pos,
                               const Eigen::Vector2f& size,
                               float alpha)
    {
        // 获取阴影信息
        const auto* shadow = registry.try_get<components::Shadow>(entity);
        float shadowSoft = 0.0F;
        float shadowOffsetX = 0.0F;
        float shadowOffsetY = 0.0F;
        if (shadow && shadow->enabled)
        {
            shadowSoft = shadow->softness;
            shadowOffsetX = shadow->offset.x();
            shadowOffsetY = shadow->offset.y();
        }

        const auto* bg = registry.try_get<components::Background>(entity);
        if (bg && bg->enabled)
        {
            Eigen::Vector4f color(bg->color.red, bg->color.green, bg->color.blue, bg->color.alpha);
            Eigen::Vector4f radius(
                bg->borderRadius.x(), bg->borderRadius.y(), bg->borderRadius.z(), bg->borderRadius.w());
            addRectFilledWithRounding(pos, size, color, radius, alpha, shadowSoft, shadowOffsetX, shadowOffsetY);
        }

        const auto* border = registry.try_get<components::Border>(entity);
        if (border && border->thickness > 0.0F)
        {
            Eigen::Vector4f color(border->color.red, border->color.green, border->color.blue, border->color.alpha);
            Eigen::Vector4f radius(
                border->borderRadius.x(), border->borderRadius.y(), border->borderRadius.z(), border->borderRadius.w());
            // TODO: 边框需要单独的着色器处理，暂时用线段绘制
            addRect(pos, pos + size, color, border->thickness, alpha);
        }
    }

    /**
     * @brief 收集组件特定渲染数据
     */
    void collectComponentData(entt::registry& registry,
                              entt::entity entity,
                              const Eigen::Vector2f& pos,
                              const Eigen::Vector2f& size,
                              float alpha)
    {
        // 文本渲染
        if (registry.any_of<components::TextTag, components::ButtonTag, components::LabelTag>(entity))
        {
            const auto* textComp = registry.try_get<components::Text>(entity);
            if (textComp && !textComp->content.empty())
            {
                Eigen::Vector4f color(
                    textComp->color.red, textComp->color.green, textComp->color.blue, textComp->color.alpha);
                addText(textComp->content, pos, size, color, textComp->alignment, alpha);
            }
        }

        // 图像渲染
        if (registry.any_of<components::ImageTag>(entity))
        {
            const auto& imageComp = registry.get<const components::Image>(entity);
            if (imageComp.textureId)
            {
                Eigen::Vector4f tint(imageComp.tintColor.red,
                                     imageComp.tintColor.green,
                                     imageComp.tintColor.blue,
                                     imageComp.tintColor.alpha);
                addImageBatch(static_cast<SDL_GPUTexture*>(imageComp.textureId),
                              pos,
                              size,
                              imageComp.uvMin,
                              imageComp.uvMax,
                              tint,
                              alpha);
            }
        }
    }

    /**
     * @brief 添加带圆角的填充矩形批次
     */
    void addRectFilledWithRounding(const Eigen::Vector2f& pos,
                                   const Eigen::Vector2f& size,
                                   const Eigen::Vector4f& color,
                                   const Eigen::Vector4f& radius,
                                   float opacity,
                                   float shadowSoft = 0.0F,
                                   float shadowOffsetX = 0.0F,
                                   float shadowOffsetY = 0.0F)
    {
        RenderBatch batch;
        batch.texture = m_whiteTexture;

        // 设置 Push Constants（与着色器 PushConstants 结构体字段顺序一致）
        batch.pushConstants.screen_size[0] = m_screenWidth;
        batch.pushConstants.screen_size[1] = m_screenHeight;
        batch.pushConstants.rect_size[0] = size.x();
        batch.pushConstants.rect_size[1] = size.y();
        batch.pushConstants.radius[0] = radius.x(); // 左上
        batch.pushConstants.radius[1] = radius.y(); // 右上
        batch.pushConstants.radius[2] = radius.z(); // 右下
        batch.pushConstants.radius[3] = radius.w(); // 左下
        batch.pushConstants.shadow_soft = shadowSoft;
        batch.pushConstants.shadow_offset_x = shadowOffsetX;
        batch.pushConstants.shadow_offset_y = shadowOffsetY;
        batch.pushConstants.opacity = opacity;
        batch.pushConstants._padding = 0.0F;

        // 创建矩形顶点（四个角）
        Eigen::Vector2f max = pos + size;

        batch.vertices.push_back({{pos.x(), pos.y()}, {0.0F, 0.0F}, {color.x(), color.y(), color.z(), color.w()}});
        batch.vertices.push_back({{max.x(), pos.y()}, {1.0F, 0.0F}, {color.x(), color.y(), color.z(), color.w()}});
        batch.vertices.push_back({{max.x(), max.y()}, {1.0F, 1.0F}, {color.x(), color.y(), color.z(), color.w()}});
        batch.vertices.push_back({{pos.x(), max.y()}, {0.0F, 1.0F}, {color.x(), color.y(), color.z(), color.w()}});

        batch.indices = {0, 1, 2, 0, 2, 3};

        m_batches.push_back(std::move(batch));
    }

    /**
     * @brief 添加矩形边框（简化版，用线段绘制）
     */
    void addRect(const Eigen::Vector2f& min,
                 const Eigen::Vector2f& max,
                 const Eigen::Vector4f& color,
                 float thickness,
                 float opacity)
    {
        // 简化实现：用4个细矩形绘制边框
        addRectFilledWithRounding(min, {max.x() - min.x(), thickness}, color, {0, 0, 0, 0}, opacity);
        addRectFilledWithRounding(
            {min.x(), max.y() - thickness}, {max.x() - min.x(), thickness}, color, {0, 0, 0, 0}, opacity);
        addRectFilledWithRounding(min, {thickness, max.y() - min.y()}, color, {0, 0, 0, 0}, opacity);
        addRectFilledWithRounding(
            {max.x() - thickness, min.y()}, {thickness, max.y() - min.y()}, color, {0, 0, 0, 0}, opacity);
    }

    /**
     * @brief 添加文本渲染批次
     */
    void addText(const std::string& text,
                 const Eigen::Vector2f& pos,
                 const Eigen::Vector2f& size,
                 const Eigen::Vector4f& color,
                 policies::Alignment alignment,
                 float opacity)
    {
        if (m_defaultFont == nullptr || text.empty()) return;

        // 使用 SDL_ttf 渲染文本到纹理
        uint32_t textWidth = 0;
        uint32_t textHeight = 0;
        SDL_GPUTexture* textTexture = renderTextToTexture(text, color, textWidth, textHeight);
        if (textTexture == nullptr) return;

        Eigen::Vector2f textSize(static_cast<float>(textWidth), static_cast<float>(textHeight));

        float drawX = pos.x();
        float drawY = pos.y();

        // 水平对齐
        if (ui::helper::HasAlignment(alignment, policies::Alignment::HCENTER))
        {
            drawX += (size.x() - textSize.x()) * 0.5F;
        }
        else if (ui::helper::HasAlignment(alignment, policies::Alignment::RIGHT))
        {
            drawX += size.x() - textSize.x();
        }

        // 垂直对齐
        if (ui::helper::HasAlignment(alignment, policies::Alignment::VCENTER))
        {
            drawY += (size.y() - textSize.y()) * 0.5F;
        }
        else if (ui::helper::HasAlignment(alignment, policies::Alignment::BOTTOM))
        {
            drawY += size.y() - textSize.y();
        }

        addImageBatch(textTexture, {drawX, drawY}, textSize, {0, 0}, {1, 1}, {1, 1, 1, 1}, opacity);
    }

    /**
     * @brief 添加图像渲染批次
     */
    void addImageBatch(SDL_GPUTexture* texture,
                       const Eigen::Vector2f& pos,
                       const Eigen::Vector2f& size,
                       const Eigen::Vector2f& uvMin,
                       const Eigen::Vector2f& uvMax,
                       const Eigen::Vector4f& tint,
                       float opacity)
    {
        RenderBatch batch;
        batch.texture = texture;

        batch.pushConstants.screen_size[0] = m_screenWidth;
        batch.pushConstants.screen_size[1] = m_screenHeight;
        batch.pushConstants.rect_size[0] = size.x();
        batch.pushConstants.rect_size[1] = size.y();
        batch.pushConstants.radius[0] = 0.0F; // 左上
        batch.pushConstants.radius[1] = 0.0F; // 右上
        batch.pushConstants.radius[2] = 0.0F; // 右下
        batch.pushConstants.radius[3] = 0.0F; // 左下
        batch.pushConstants.shadow_soft = 0.0F;
        batch.pushConstants.shadow_offset_x = 0.0F;
        batch.pushConstants.shadow_offset_y = 0.0F;
        batch.pushConstants.opacity = opacity;
        batch.pushConstants._padding = 0.0F;

        Eigen::Vector2f max = pos + size;

        batch.vertices.push_back(
            {{pos.x(), pos.y()}, {uvMin.x(), uvMin.y()}, {tint.x(), tint.y(), tint.z(), tint.w()}});
        batch.vertices.push_back(
            {{max.x(), pos.y()}, {uvMax.x(), uvMin.y()}, {tint.x(), tint.y(), tint.z(), tint.w()}});
        batch.vertices.push_back(
            {{max.x(), max.y()}, {uvMax.x(), uvMax.y()}, {tint.x(), tint.y(), tint.z(), tint.w()}});
        batch.vertices.push_back(
            {{pos.x(), max.y()}, {uvMin.x(), uvMax.y()}, {tint.x(), tint.y(), tint.z(), tint.w()}});

        batch.indices = {0, 1, 2, 0, 2, 3};

        m_batches.push_back(std::move(batch));
    }

    /**
     * @brief 使用 SDL_ttf 渲染文本到 GPU 纹理
     */
    SDL_GPUTexture* renderTextToTexture(const std::string& text,
                                        const Eigen::Vector4f& color,
                                        uint32_t& outWidth,
                                        uint32_t& outHeight)
    {
        if (m_defaultFont == nullptr) return nullptr;

        // 检查缓存
        std::string cacheKey = text + "_" + std::to_string(color.x()) + "_" + std::to_string(color.y()) + "_" +
                               std::to_string(color.z()) + "_" + std::to_string(color.w());

        auto it = m_textureCache.find(cacheKey);
        if (it != m_textureCache.end())
        {
            outWidth = it->second.width;
            outHeight = it->second.height;
            return it->second.texture;
        }

        // 渲染文本到 Surface
        SDL_Color sdlColor = {static_cast<uint8_t>(color.x() * 255),
                              static_cast<uint8_t>(color.y() * 255),
                              static_cast<uint8_t>(color.z() * 255),
                              static_cast<uint8_t>(color.w() * 255)};

        SDL_Surface* rawSurface = TTF_RenderText_Blended(m_defaultFont, text.c_str(), text.size(), sdlColor);
        if (rawSurface == nullptr)
        {
            LOG_INFO("Failed to render text: %s", SDL_GetError());
            return nullptr;
        }

        // 转换为 RGBA32，确保格式与 GPU 纹理一致
        SDL_Surface* textSurface = SDL_ConvertSurface(rawSurface, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(rawSurface);
        if (textSurface == nullptr)
        {
            LOG_INFO("Failed to convert text surface: %s", SDL_GetError());
            return nullptr;
        }

        // 创建 GPU 纹理
        SDL_GPUTextureCreateInfo textureInfo = {};
        textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
        textureInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        textureInfo.width = static_cast<uint32_t>(textSurface->w);
        textureInfo.height = static_cast<uint32_t>(textSurface->h);
        textureInfo.layer_count_or_depth = 1;
        textureInfo.num_levels = 1;
        textureInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

        SDL_GPUTexture* texture = SDL_CreateGPUTexture(m_gpuDevice, &textureInfo);
        if (texture == nullptr)
        {
            SDL_DestroySurface(textSurface);
            return nullptr;
        }

        // 上传纹理数据
        SDL_GPUTransferBufferCreateInfo transferInfo = {};
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferInfo.size = static_cast<uint32_t>(textSurface->pitch * textSurface->h);

        SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(m_gpuDevice, &transferInfo);

        if (transferBuffer != nullptr)
        {
            void* data = SDL_MapGPUTransferBuffer(m_gpuDevice, transferBuffer, false);
            if (data != nullptr)
            {
                SDL_memcpy(data, textSurface->pixels, textSurface->pitch * textSurface->h);
                SDL_UnmapGPUTransferBuffer(m_gpuDevice, transferBuffer);

                SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(m_gpuDevice);
                SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

                SDL_GPUTextureTransferInfo transferInfo = {};
                transferInfo.transfer_buffer = transferBuffer;
                transferInfo.offset = 0;
                const uint32_t bytesPerPixel = 4;
                transferInfo.pixels_per_row = static_cast<uint32_t>(textSurface->pitch / bytesPerPixel);
                transferInfo.rows_per_layer = static_cast<uint32_t>(textSurface->h);

                SDL_GPUTextureRegion textureRegion = {};
                textureRegion.texture = texture;
                textureRegion.w = static_cast<uint32_t>(textSurface->w);
                textureRegion.h = static_cast<uint32_t>(textSurface->h);
                textureRegion.d = 1;

                SDL_UploadToGPUTexture(copyPass, &transferInfo, &textureRegion, false);
                SDL_EndGPUCopyPass(copyPass);
                SDL_SubmitGPUCommandBuffer(uploadCmdBuf);

                SDL_ReleaseGPUTransferBuffer(m_gpuDevice, transferBuffer);
            }
        }

        SDL_DestroySurface(textSurface);

        // 缓存纹理
        m_textureCache[cacheKey] = {texture, textureInfo.width, textureInfo.height};

        outWidth = textureInfo.width;
        outHeight = textureInfo.height;

        return texture;
    }

    /**
     * @brief 执行 GPU 渲染（批次渲染）
     */
    void renderToGPU(int width, int height)
    {
        // 获取命令缓冲区
        SDL_GPUCommandBuffer* cmdBuf = SDL_AcquireGPUCommandBuffer(m_gpuDevice);
        if (cmdBuf == nullptr) return;

        // 获取交换链纹理
        SDL_GPUTexture* swapchainTexture = nullptr;
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(
                cmdBuf, m_graphicsContext->getWindow(), &swapchainTexture, nullptr, nullptr))
        {
            SDL_CancelGPUCommandBuffer(cmdBuf);
            return;
        }

        if (swapchainTexture == nullptr)
        {
            SDL_SubmitGPUCommandBuffer(cmdBuf);
            return;
        }

        // 开始渲染通道
        SDL_GPUColorTargetInfo colorTarget = {};
        colorTarget.texture = swapchainTexture;
        colorTarget.clear_color = {0.0F, 0.0F, 0.0F, 1.0F};
        colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTarget.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmdBuf, &colorTarget, 1, nullptr);

        // 绑定管线
        SDL_BindGPUGraphicsPipeline(renderPass, m_pipeline);

        // 设置视口
        SDL_GPUViewport viewport = {};
        viewport.x = 0;
        viewport.y = 0;
        viewport.w = static_cast<float>(width);
        viewport.h = static_cast<float>(height);
        viewport.min_depth = 0.0F;
        viewport.max_depth = 1.0F;
        SDL_SetGPUViewport(renderPass, &viewport);

        // 逐批次渲染
        for (const auto& batch : m_batches)
        {
            if (batch.vertices.empty() || batch.indices.empty()) continue;

            // 上传顶点和索引数据
            SDL_GPUBuffer* vertexBuffer = uploadBatchVertices(batch.vertices);
            SDL_GPUBuffer* indexBuffer = uploadBatchIndices(batch.indices);

            if (vertexBuffer == nullptr || indexBuffer == nullptr)
            {
                if (vertexBuffer) SDL_ReleaseGPUBuffer(m_gpuDevice, vertexBuffer);
                if (indexBuffer) SDL_ReleaseGPUBuffer(m_gpuDevice, indexBuffer);
                continue;
            }

            // 绑定顶点缓冲区
            SDL_GPUBufferBinding vertexBinding = {};
            vertexBinding.buffer = vertexBuffer;
            vertexBinding.offset = 0;
            SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);

            // 绑定索引缓冲区
            SDL_GPUBufferBinding indexBinding = {};
            indexBinding.buffer = indexBuffer;
            indexBinding.offset = 0;
            SDL_BindGPUIndexBuffer(renderPass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

            // 绑定纹理和采样器（着色器使用 register(s1, t1)，但 SDL GPU 绑定从 0 开始）
            if (batch.texture != nullptr && m_sampler != nullptr)
            {
                SDL_GPUTextureSamplerBinding texSamplerBinding = {};
                texSamplerBinding.texture = batch.texture;
                texSamplerBinding.sampler = m_sampler;
                SDL_BindGPUFragmentSamplers(renderPass, 0, &texSamplerBinding, 1);
            }

            // 推送 Push Constants
            SDL_PushGPUVertexUniformData(cmdBuf, 0, &batch.pushConstants, sizeof(UiPushConstants));
            SDL_PushGPUFragmentUniformData(cmdBuf, 0, &batch.pushConstants, sizeof(UiPushConstants));

            // 绘制
            SDL_DrawGPUIndexedPrimitives(renderPass, static_cast<uint32_t>(batch.indices.size()), 1, 0, 0, 0);

            // 释放临时缓冲区（实际应该池化复用）
            SDL_ReleaseGPUBuffer(m_gpuDevice, vertexBuffer);
            SDL_ReleaseGPUBuffer(m_gpuDevice, indexBuffer);
        }

        // 结束渲染通道
        SDL_EndGPURenderPass(renderPass);

        // 提交命令缓冲区
        SDL_SubmitGPUCommandBuffer(cmdBuf);
    }

    /**
     * @brief 上传批次顶点数据到 GPU 缓冲区
     */
    SDL_GPUBuffer* uploadBatchVertices(const std::vector<Vertex>& vertices)
    {
        const uint32_t bufferSize = static_cast<uint32_t>(vertices.size() * sizeof(Vertex));

        SDL_GPUBufferCreateInfo bufferInfo = {};
        bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        bufferInfo.size = bufferSize;
        SDL_GPUBuffer* buffer = SDL_CreateGPUBuffer(m_gpuDevice, &bufferInfo);

        if (buffer == nullptr) return nullptr;

        // 上传数据
        SDL_GPUTransferBufferCreateInfo transferInfo = {};
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferInfo.size = bufferSize;

        SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(m_gpuDevice, &transferInfo);

        if (transferBuffer == nullptr)
        {
            SDL_ReleaseGPUBuffer(m_gpuDevice, buffer);
            return nullptr;
        }

        void* data = SDL_MapGPUTransferBuffer(m_gpuDevice, transferBuffer, false);
        SDL_memcpy(data, vertices.data(), bufferSize);
        SDL_UnmapGPUTransferBuffer(m_gpuDevice, transferBuffer);

        SDL_GPUCommandBuffer* uploadCmd = SDL_AcquireGPUCommandBuffer(m_gpuDevice);
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmd);

        SDL_GPUTransferBufferLocation srcLocation = {};
        srcLocation.transfer_buffer = transferBuffer;
        srcLocation.offset = 0;

        SDL_GPUBufferRegion dstRegion = {};
        dstRegion.buffer = buffer;
        dstRegion.offset = 0;
        dstRegion.size = bufferSize;

        SDL_UploadToGPUBuffer(copyPass, &srcLocation, &dstRegion, false);
        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(uploadCmd);

        SDL_ReleaseGPUTransferBuffer(m_gpuDevice, transferBuffer);

        return buffer;
    }

    /**
     * @brief 上传批次索引数据到 GPU 缓冲区
     */
    SDL_GPUBuffer* uploadBatchIndices(const std::vector<uint16_t>& indices)
    {
        const uint32_t bufferSize = static_cast<uint32_t>(indices.size() * sizeof(uint16_t));

        SDL_GPUBufferCreateInfo bufferInfo = {};
        bufferInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        bufferInfo.size = bufferSize;
        SDL_GPUBuffer* buffer = SDL_CreateGPUBuffer(m_gpuDevice, &bufferInfo);

        if (buffer == nullptr) return nullptr;

        SDL_GPUTransferBufferCreateInfo transferInfo = {};
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferInfo.size = bufferSize;

        SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(m_gpuDevice, &transferInfo);

        if (transferBuffer == nullptr)
        {
            SDL_ReleaseGPUBuffer(m_gpuDevice, buffer);
            return nullptr;
        }

        void* data = SDL_MapGPUTransferBuffer(m_gpuDevice, transferBuffer, false);
        SDL_memcpy(data, indices.data(), bufferSize);
        SDL_UnmapGPUTransferBuffer(m_gpuDevice, transferBuffer);

        SDL_GPUCommandBuffer* uploadCmd = SDL_AcquireGPUCommandBuffer(m_gpuDevice);
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmd);

        SDL_GPUTransferBufferLocation srcLocation = {};
        srcLocation.transfer_buffer = transferBuffer;
        srcLocation.offset = 0;

        SDL_GPUBufferRegion dstRegion = {};
        dstRegion.buffer = buffer;
        dstRegion.offset = 0;
        dstRegion.size = bufferSize;

        SDL_UploadToGPUBuffer(copyPass, &srcLocation, &dstRegion, false);
        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(uploadCmd);

        SDL_ReleaseGPUTransferBuffer(m_gpuDevice, transferBuffer);

        return buffer;
    }
};

} // namespace ui::systems
