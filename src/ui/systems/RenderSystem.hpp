/**
 * ************************************************************************
 *
 * @file RenderSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-13
 * @version 0.1
 * @brief SDL GPU 渲染系统

  - 负责使用 SDL3 GPU 模块进行 UI 渲染
  - 响应 UI 渲染事件，执行实际渲染操作

  加载assets/sharder/spir-v文件

  支持的图形后端列表

    - Vulkan 默认
    - DX12

    使用SDL ttf渲染字体到GPU纹理上
    默认字体用 assets/fonts/NotoSansSC-VariableFont_wght.ttf
    静态资源都用cmrc打包
    驱动和窗口是1对多关系
    所有窗口共用同一个GPU设备
    窗口数量可能变化
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <cstdlib>
#include <optional>
#include <stack>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <entt/entt.hpp>
#include <Eigen/Dense>
#include <cmrc/cmrc.hpp>
#include <entt/entt.hpp>
#include "../singleton/Logger.hpp"
#include "../singleton/Registry.hpp"
#include "../singleton/Dispatcher.hpp"
#include "SDL3/SDL_video.h"
#include "../common/Components.hpp"
#include "../common/Tags.hpp"
#include "../common/Events.hpp"
#include "../interface/Isystem.hpp"
#include "../api/Utils.hpp"
#include "../api/Layout.hpp"

CMRC_DECLARE(ui_fonts); // NOLINT

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
    virtual SDL_GPUDevice* handle(std::string& outDriverName)
    {
        if (nextHandler)
        {
            return nextHandler->handle(outDriverName);
        }
        return nullptr;
    }
};
class D3D12Handler final : public GPUBackendHandler
{
public:
    SDL_GPUDevice* handle(std::string& outDriverName) override
    {
        Logger::info("责任链：尝试初始化 D3D12...");

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

        Logger::warn("D3D12 不可用，传递给链中下一个处理器。原因: {}", SDL_GetError());
        return GPUBackendHandler::handle(outDriverName);
    }
};
class VulkanHandler final : public GPUBackendHandler
{
public:
    SDL_GPUDevice* handle(std::string& outDriverName) override
    {
        Logger::info("责任链：尝试初始化 Vulkan...");

        SDL_PropertiesID props = SDL_CreateProperties();
        SDL_SetStringProperty(props, SDL_PROP_GPU_DEVICE_CREATE_NAME_STRING, "vulkan");
        SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, false);
        SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);

        SDL_GPUDevice* device = SDL_CreateGPUDeviceWithProperties(props);
        SDL_DestroyProperties(props);

        if (device != nullptr)
        {
            outDriverName = "vulkan";
            return device;
        }

        Logger::warn("Vulkan 不可用。原因: {}", SDL_GetError());
        return GPUBackendHandler::handle(outDriverName);
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
    float padding;         // 填充到 16 字节倍数
};
/**
 * @brief SDL GPU 渲染系统
 *
 * 使用 SDL3 GPU API 实现高性能 UI 渲染，替代 ImGui DrawList
 */
class RenderSystem final : public interface::EnableRegister<RenderSystem>
{
    // 渲染缓冲区 - 顶点结构与着色器 VSInput 对应
    struct Vertex
    {
        float position[2]; // POSITION
        float texCoord[2]; // TEXCOORD0
        float color[4];    // COLOR0
    };

    /**
     * @brief 渲染批次结构
     */
    struct RenderBatch
    {
        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;
        UiPushConstants pushConstants{};
        SDL_GPUTexture* texture = nullptr;
        std::optional<SDL_Rect> scissorRect;
    };
    // 纹理缓存（用于文本）
    struct CachedTexture
    {
        SDL_GPUTexture* texture = nullptr;
        uint32_t width = 0;
        uint32_t height = 0;
    };

public:
    RenderSystem()
    {
        // 延迟初始化：等待 SDL Video 子系统就绪
        Logger::info("RenderSystem 构造完成，等待初始化...");
    };
    RenderSystem(const RenderSystem&) = delete;
    RenderSystem& operator=(const RenderSystem&) = delete;
    RenderSystem(RenderSystem&&) noexcept = default;
    RenderSystem& operator=(RenderSystem&&) noexcept = default;
    ~RenderSystem() { cleanup(); }

    /**
     * @brief 注册事件处理器
     */
    void registerHandlersImpl()
    {
        Dispatcher::Sink<events::WindowGraphicsContextSetEvent>().connect<&RenderSystem::onWindowsGraphicsContextSet>(
            *this);
        Dispatcher::Sink<events::WindowGraphicsContextUnsetEvent>()
            .connect<&RenderSystem::onWindowsGraphicsContextUnset>(*this);
        Dispatcher::Sink<events::UpdateRendering>().connect<&RenderSystem::update>(*this);
    }

    /**
     * @brief 注销事件处理器
     */
    void unregisterHandlersImpl()
    {
        Dispatcher::Sink<events::WindowGraphicsContextSetEvent>()
            .disconnect<&RenderSystem::onWindowsGraphicsContextSet>(*this);
        Dispatcher::Sink<events::WindowGraphicsContextUnsetEvent>()
            .disconnect<&RenderSystem::onWindowsGraphicsContextUnset>(*this);
        Dispatcher::Sink<events::UpdateRendering>().disconnect<&RenderSystem::update>(*this);
    }

private:
    /**
     * @brief 处理图形上下文设置事件
     */
    void onWindowsGraphicsContextSet(const events::WindowGraphicsContextSetEvent& event)
    {
        ensureInitialized();
        uint32_t windowID = Registry::Get<components::Window>(event.entity).windowID;
        SDL_Window* sdlWindow = SDL_GetWindowFromID(windowID);
        if (sdlWindow == nullptr)
        {
            return;
        }
        // 声明窗口给 GPU 设备
        claimWindowForGPUDevice(sdlWindow);
        // 为窗口创建渲染管线
        createPipeline(sdlWindow);

        Logger::info("RenderSystem: Window graphics context setup completed for entity {}",
                     static_cast<uint32_t>(event.entity));
    }

    void onWindowsGraphicsContextUnset(const events::WindowGraphicsContextUnsetEvent& event)
    {
        // 尝试获取窗口组件以查找 SDL_Window
        if (auto* windowComp = Registry::TryGet<components::Window>(event.entity))
        {
            SDL_Window* sdlWindow = SDL_GetWindowFromID(windowComp->windowID);
            if (sdlWindow != nullptr)
            {
                // 检查是否在已声明窗口列表中
                auto it = m_claimedWindows.find(sdlWindow);
                if (it != m_claimedWindows.end())
                {
                    SDL_ReleaseWindowFromGPUDevice(m_gpuDevice, sdlWindow);
                    m_claimedWindows.erase(it);
                    Logger::info("已从 GPU 设备释放窗口 (ID: {})", windowComp->windowID);
                }
            }
        }
    }

    /**
     * @brief 初始化 SDL GPU 设备
     */
    void initializeGPU()
    {
        // 1. 组装责任链
        auto d3d12 = std::make_shared<D3D12Handler>();
        auto vulkan = std::make_shared<VulkanHandler>();

        vulkan->setNext(d3d12);
        d3d12->setNext(nullptr);
        // 2. 启动链式处理
        std::string finalDriverName;
        m_gpuDevice = vulkan->handle(finalDriverName);

        // 3. 结果检查
        if (m_gpuDevice == nullptr)
        {
            Logger::error("所有 GPU 后端均初始化失败！");
            return;
        }

        m_gpuDriver = finalDriverName;
    }

    bool claimWindowForGPUDevice(SDL_Window* sdlWindow)
    {
        if (m_gpuDevice == nullptr || sdlWindow == nullptr)
        {
            return false;
        }

        if (m_claimedWindows.find(sdlWindow) != m_claimedWindows.end())
        {
            return true;
        }

        // 4. 声明窗口
        if (!SDL_ClaimWindowForGPUDevice(m_gpuDevice, sdlWindow))
        {
            Logger::error("窗口声明失败: {}", SDL_GetError());
            return false;
        }

        m_claimedWindows.insert(sdlWindow);
        Logger::info("GPU 初始化成功，最终使用后端: {}", m_gpuDriver);
        return true;
    }

    /**
     * @brief 加载  着色器（从 cmrc 嵌入资源）
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
            Logger::error("着色器加载失败 (驱动: {}, 顶点着色器: {}, 片段着色器: {})",
                          m_gpuDriver,
                          (m_vertexShader != nullptr ? "成功" : "失败"),
                          (m_fragmentShader != nullptr ? "成功" : "失败"));
        }
        else
        {
            Logger::info("着色器加载成功 (驱动: {})", m_gpuDriver);
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
            Logger::error("着色器资源未找到: {}", resourcePath);
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
            Logger::error("着色器创建失败: {} (错误: {})", resourcePath, SDL_GetError());
        }
        else
        {
            Logger::info("着色器创建成功: {} (大小: {} 字节)", resourcePath, file.size());
        }
        return shader;
    }

    /**
     * @brief 创建渲染管线（只创建一次，多窗口共用）
     */
    void createPipeline(SDL_Window* sdlWindow)
    {
        if (m_gpuDevice == nullptr || m_vertexShader == nullptr || m_fragmentShader == nullptr)
        {
            return;
        }

        // 管线只需创建一次，多窗口共用（假设所有窗口使用相同的交换链格式）
        if (m_pipeline != nullptr)
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

        SDL_GPUColorTargetDescription colorTargetDesc = {};                                // 颜色目标描述
        colorTargetDesc.format = SDL_GetGPUSwapchainTextureFormat(m_gpuDevice, sdlWindow); // 颜色格式
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
            Logger::error("图形管线创建失败: {}", SDL_GetError());
        }
        else
        {
            Logger::info("图形管线创建成功");
        }

        // 创建采样器
        SDL_GPUSamplerCreateInfo samplerInfo = {};
        samplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
        samplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
        samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
        samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

        m_sampler = SDL_CreateGPUSampler(m_gpuDevice, &samplerInfo);

        if (m_sampler == nullptr)
        {
            Logger::error("采样器创建失败: {}", SDL_GetError());
        }
        else
        {
            Logger::info("采样器创建成功");
        }
    }

    /**
     * @brief 加载默认字体（从 cmrc 嵌入资源）
     */
    void loadDefaultFont()
    {
        // SDL3_ttf: TTF_Init() 返回 bool，true 表示成功，false 表示失败
        if (TTF_WasInit() == 0 && !TTF_Init())
        {
            Logger::error("Failed to initialize SDL_ttf: {}", SDL_GetError());
            return;
        }

        // 从 cmrc 嵌入资源加载字体
        auto filesystem = cmrc::ui_fonts::get_filesystem();
        const char* fontPath = "assets/fonts/NotoSansSC-VariableFont_wght.ttf";

        if (!filesystem.exists(fontPath))
        {
            Logger::error("Font resource not found: {}", fontPath);
            return;
        }

        auto fontFile = filesystem.open(fontPath);

        // 使用 SDL_IOFromConstMem 从内存加载字体
        SDL_IOStream* fontIO = SDL_IOFromConstMem(fontFile.begin(), static_cast<size_t>(fontFile.size()));
        if (fontIO == nullptr)
        {
            Logger::info("Failed to create IOStream for font: {}", SDL_GetError());
            return;
        }

        m_defaultFont = TTF_OpenFontIO(fontIO, true, 16.0F);
        if (m_defaultFont == nullptr)
        {
            Logger::info("Failed to load default font: {}", SDL_GetError());
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
        for (auto* window : m_claimedWindows)
        {
            if (window != nullptr)
            {
                SDL_ReleaseWindowFromGPUDevice(m_gpuDevice, window);
            }
        }
        m_claimedWindows.clear();

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
        static bool firstUpdate = true;
        auto windowView = Registry::View<components::Window, components::RenderDirtyTag>();
        // 检查是否有窗口
        if (windowView.begin() == windowView.end())
        {
            return;
        }
        if (firstUpdate)
        {
            Logger::info("RenderSystem::update first call");
            firstUpdate = false;
        }
        ensureInitialized();
        if (m_gpuDevice == nullptr || m_pipeline == nullptr)
        {
            Logger::warn("GPU 设备或管线未初始化");
            return;
        }

        // 延迟创建白色纹理
        if (m_whiteTexture == nullptr)
        {
            createWhiteTexture();
        }

        // 遍历每个窗口进行渲染
        for (auto windowEntity : windowView)
        {
            auto& windowComp = windowView.get<components::Window>(windowEntity);
            SDL_Window* sdlWindow = SDL_GetWindowFromID(windowComp.windowID);
            if (sdlWindow == nullptr)
            {
                Logger::warn("窗口实体的 sdlWindow 为空");
                continue;
            }

            // 获取当前窗口大小
            int width = 0;
            int height = 0;
            SDL_GetWindowSizeInPixels(sdlWindow, &width, &height);
            if (width <= 0 || height <= 0) continue;

            m_screenWidth = static_cast<float>(width);
            m_screenHeight = static_cast<float>(height);

            // 清空渲染批次
            m_batches.clear();

            // 收集当前窗口及其子元素的渲染数据
            if (Registry::AnyOf<components::VisibleTag>(windowEntity))
            {
                // 窗口实体的 Position 是屏幕坐标，而渲染是在窗口坐标系中进行的 (0,0 为窗口左上角)
                // 因此需要抵消窗口自身的位置偏移，确保窗口内容从 (0,0) 开始渲染
                Eigen::Vector2f rootOffset = Eigen::Vector2f(0, 0);
                if (const auto* pos = Registry::TryGet<components::Position>(windowEntity))
                {
                    rootOffset = -pos->value;
                }
                collectRenderData(windowEntity, rootOffset, 1.0F, sdlWindow);
            }

            // 执行 GPU 渲染
            if (!m_batches.empty())
            {
                renderToGPU(sdlWindow, width, height);
            }
        }

        // 清除脏标记
        auto dirtyView = Registry::View<components::RenderDirtyTag>();
        for (auto entity : dirtyView)
        {
            Registry::Remove<components::RenderDirtyTag>(entity);
        }
    }

    // 窗口属性同步函数已迁移至 StateSystem

private:
    void ensureInitialized()
    {
        if ((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) == 0)
        {
            return;
        }

        if (m_gpuDevice == nullptr)
        {
            initializeGPU();
            loadShaders();
        }

        if (m_defaultFont == nullptr)
        {
            loadDefaultFont();
        }
    }

    /**
     * @brief 递归收集渲染数据
     * @param registry  注册表
     * @param entity  当前实体
     * @param parentPos  父实体位置
     * @param parentAlpha  父实体透明度
     * @param sdlWindow  SDL 窗口指针
     */
    void collectRenderData(entt::entity entity,
                           const Eigen::Vector2f& parentPos,
                           float parentAlpha,
                           SDL_Window* sdlWindow = nullptr)
    {
        if (!Registry::AnyOf<components::VisibleTag>(entity)) return;
        if (Registry::AnyOf<components::SpacerTag>(entity)) return;

        const auto& pos = Registry::Get<components::Position>(entity);
        const auto& size = Registry::Get<components::Size>(entity);
        const auto* alphaComp = Registry::TryGet<components::Alpha>(entity);

        float globalAlpha = parentAlpha * (alphaComp ? alphaComp->value : 1.0F);
        Eigen::Vector2f absolutePos = parentPos + pos.value;
        Eigen::Vector2f contentOffset(0.0F, 0.0F);

        // Window/Dialog 目前按普通容器渲染（背景 + 子元素），后续再做特殊处理

        // 处理 ScrollArea
        const auto* scrollArea = Registry::TryGet<components::ScrollArea>(entity);
        bool pushScissor = false;
        if (scrollArea != nullptr)
        {
            SDL_Rect currentScissor;
            currentScissor.x = static_cast<int>(absolutePos.x());
            currentScissor.y = static_cast<int>(absolutePos.y());
            currentScissor.w = static_cast<int>(size.size.x());
            currentScissor.h = static_cast<int>(size.size.y());

            // 保持与父级裁剪区域的交集
            if (!m_scissorStack.empty())
            {
                const SDL_Rect& parentRect = m_scissorStack.back();
                if (!SDL_GetRectIntersection(&currentScissor, &parentRect, &currentScissor))
                {
                    // 不可见时设置为空
                    currentScissor.w = 0;
                    currentScissor.h = 0;
                }
            }

            m_scissorStack.push_back(currentScissor);
            pushScissor = true;
            contentOffset = -scrollArea->scrollOffset;
        }

        // 收集背景渲染数据
        collectBackgroundData(entity, absolutePos, size.size, globalAlpha);

        // 收集特定组件渲染数据
        collectComponentData(entity, absolutePos, size.size, globalAlpha, sdlWindow);

        // 递归处理子元素
        const auto* hierarchy = Registry::TryGet<components::Hierarchy>(entity);
        if (hierarchy && !hierarchy->children.empty())
        {
            for (entt::entity child : hierarchy->children)
            {
                // 子元素位置受滚动偏移影响
                collectRenderData(child, absolutePos + contentOffset, globalAlpha, sdlWindow);
            }
        }

        if (pushScissor)
        {
            // 绘制滚动条 (在裁剪之前)
            drawScrollBars(entity, absolutePos, size.size, *scrollArea, globalAlpha);

            m_scissorStack.pop_back();
        }
    }

    void drawScrollBars(entt::entity entity,
                        const Eigen::Vector2f& pos,
                        const Eigen::Vector2f& size,
                        const components::ScrollArea& scrollArea,
                        float alpha)
    {
        if (scrollArea.showScrollbars == policies::ScrollBarVisibility::AlwaysOff) return;

        // 计算可视区域高度（减去垂直内边距）
        float viewportHeight = size.y();
        if (const auto* padding = Registry::TryGet<components::Padding>(entity))
        {
            viewportHeight = std::max(0.0F, size.y() - padding->values.x() - padding->values.z());
        }

        // 简单绘制一个垂直滚动条
        bool hasVerticalScroll =
            (scrollArea.scroll == policies::Scroll::Vertical || scrollArea.scroll == policies::Scroll::Both);
        if (hasVerticalScroll && scrollArea.contentSize.y() > viewportHeight)
        {
            float trackSize = size.y();
            float visibleRatio = viewportHeight / scrollArea.contentSize.y();
            float thumbSize = std::max(20.0f, trackSize * visibleRatio);
            float maxScroll = std::max(0.0f, scrollArea.contentSize.y() - viewportHeight);
            float scrollRatio =
                maxScroll > 0.0f ? std::clamp(scrollArea.scrollOffset.y() / maxScroll, 0.0f, 1.0f) : 0.0f;
            float thumbPos = (trackSize - thumbSize) * scrollRatio;

            // 确保滑块位置不超出轨道
            thumbPos = std::clamp(thumbPos, 0.0f, trackSize - thumbSize);

            // 绘制滑块
            float barWidth = 10.0f;
            Eigen::Vector2f barPos(pos.x() + size.x() - barWidth - 2.0f, pos.y() + thumbPos);
            Eigen::Vector2f barSize(barWidth, thumbSize);

            addRectFilledWithRounding(barPos, barSize, {0.6f, 0.6f, 0.6f, 0.8f}, {5.0f, 5.0f, 5.0f, 5.0f}, alpha);
        }
    }

    /** */
    void
        collectBackgroundData(entt::entity entity, const Eigen::Vector2f& pos, const Eigen::Vector2f& size, float alpha)
    {
        // 获取阴影信息
        const auto* shadow = Registry::TryGet<components::Shadow>(entity);
        float shadowSoft = 0.0F;
        float shadowOffsetX = 0.0F;
        float shadowOffsetY = 0.0F;
        if (shadow && shadow->enabled == policies::Feature::Enabled)
        {
            shadowSoft = shadow->softness;
            shadowOffsetX = shadow->offset.x();
            shadowOffsetY = shadow->offset.y();
        }

        const auto* bg = Registry::TryGet<components::Background>(entity);
        if (bg && bg->enabled == policies::Feature::Enabled)
        {
            Eigen::Vector4f color(bg->color.red, bg->color.green, bg->color.blue, bg->color.alpha);
            Eigen::Vector4f radius(
                bg->borderRadius.x(), bg->borderRadius.y(), bg->borderRadius.z(), bg->borderRadius.w());
            addRectFilledWithRounding(pos, size, color, radius, alpha, shadowSoft, shadowOffsetX, shadowOffsetY);
        }

        const auto* border = Registry::TryGet<components::Border>(entity);
        bool focused = Registry::AnyOf<components::FocusedTag>(entity);

        if (focused || (border && border->thickness > 0.0F))
        {
            Eigen::Vector4f color(0.0f, 0.0f, 0.0f, 1.0f);
            Eigen::Vector4f radius(0.0f, 0.0f, 0.0f, 0.0f);
            float thickness = 0.0f;

            if (border)
            {
                color =
                    Eigen::Vector4f(border->color.red, border->color.green, border->color.blue, border->color.alpha);
                radius = Eigen::Vector4f(border->borderRadius.x(),
                                         border->borderRadius.y(),
                                         border->borderRadius.z(),
                                         border->borderRadius.w());
                thickness = border->thickness;
            }

            if (focused)
            {
                color = Eigen::Vector4f(0.2f, 0.6f, 1.0f, 1.0f); // Blue focus ring
                if (thickness < 2.0f) thickness = 2.0f;
            }

            if (thickness > 0.0f)
            {
                // TODO: 边框需要单独的着色器处理，暂时用线段绘制
                addRect(pos, pos + size, color, thickness, alpha);
            }
        }
    }

    /**
     * @brief 收集组件特定渲染数据
     */
    void collectComponentData(entt::entity entity,
                              const Eigen::Vector2f& pos,
                              const Eigen::Vector2f& size,
                              float alpha,
                              SDL_Window* sdlWindow = nullptr)
    {
        // 文本渲染（普通文本、按钮、标签）
        if (Registry::AnyOf<components::TextTag, components::ButtonTag, components::LabelTag>(entity))
        {
            const auto* textComp = Registry::TryGet<components::Text>(entity);
            if (textComp && !textComp->content.empty())
            {
                Eigen::Vector4f color(
                    textComp->color.red, textComp->color.green, textComp->color.blue, textComp->color.alpha);

                policies::TextWrap wrapMode = textComp->wordWrap;
                float wrapWidth = textComp->wrapWidth;

                if (wrapMode == policies::TextWrap::None)
                {
                    // 如果在 ScrollArea 内，默认启用换行
                    float inferredWidth = getAncestorScrollAreaTextWidth(entity);
                    if (inferredWidth > 0.0F)
                    {
                        wrapMode = policies::TextWrap::Word;
                        wrapWidth = inferredWidth;
                    }
                }

                if (wrapMode != policies::TextWrap::None && wrapWidth <= 0.0F)
                {
                    wrapWidth = size.x();
                }

                if (wrapMode != policies::TextWrap::None && wrapWidth > 0.0F)
                {
                    // 根据换行结果动态修正自动高度，避免滚动区内容高度不匹配
                    if (auto* sizeComp = Registry::TryGet<components::Size>(entity))
                    {
                        if (policies::HasFlag(sizeComp->sizePolicy, policies::Size::VAuto))
                        {
                            const float lineHeight = static_cast<float>(TTF_GetFontHeight(m_defaultFont));
                            if (lineHeight > 0.0F)
                            {
                                const auto lines =
                                    wrapTextLines(textComp->content, static_cast<int>(wrapWidth), wrapMode);
                                const float desiredHeight = static_cast<float>(lines.size()) * lineHeight;
                                if (std::abs(sizeComp->size.y() - desiredHeight) > 0.5F)
                                {
                                    sizeComp->size.y() = desiredHeight;
                                    Registry::EmplaceOrReplace<components::LayoutDirtyTag>(entity);
                                }
                            }
                        }
                    }

                    addWrappedText(
                        textComp->content, pos, size, color, textComp->alignment, wrapMode, wrapWidth, alpha);
                }
                else
                {
                    addText(textComp->content, pos, size, color, textComp->alignment, alpha);
                }
            }

            // Icon 渲染逻辑 - 如果控件有 Icon 组件，则渲染图标
            if (const auto* iconComp = Registry::TryGet<components::Icon>(entity))
            {
                if (iconComp->textureId)
                {
                    renderIcon(entity, iconComp, pos, size, alpha);
                }
            }
        }

        // 文本输入框渲染
        if (Registry::AnyOf<components::TextEditTag>(entity))
        {
            const auto* textComp = Registry::TryGet<components::Text>(entity);
            const auto* textEdit = Registry::TryGet<components::TextEdit>(entity);
            if (textComp != nullptr && textEdit != nullptr)
            {
                // 计算文本区域（考虑内边距）
                Eigen::Vector2f textPos = pos;
                Eigen::Vector2f textSize = size;
                if (const auto* padding = Registry::TryGet<components::Padding>(entity))
                {
                    textPos.x() += padding->values.w();
                    textPos.y() += padding->values.x();
                    textSize.x() = std::max(0.0F, textSize.x() - padding->values.y() - padding->values.w());
                    textSize.y() = std::max(0.0F, textSize.y() - padding->values.x() - padding->values.z());
                }

                // 在输入框内部裁剪，避免文本溢出
                bool pushScissor = false;
                SDL_Rect currentScissor;
                currentScissor.x = static_cast<int>(textPos.x());
                currentScissor.y = static_cast<int>(textPos.y());
                currentScissor.w = static_cast<int>(textSize.x());
                currentScissor.h = static_cast<int>(textSize.y());

                if (!m_scissorStack.empty())
                {
                    const SDL_Rect& parentRect = m_scissorStack.back();
                    if (!SDL_GetRectIntersection(&currentScissor, &parentRect, &currentScissor))
                    {
                        currentScissor.w = 0;
                        currentScissor.h = 0;
                    }
                }
                m_scissorStack.push_back(currentScissor);
                pushScissor = true;

                std::string displayText = textEdit->buffer;
                Eigen::Vector4f color(
                    textComp->color.red, textComp->color.green, textComp->color.blue, textComp->color.alpha);

                // 如果没有内容且有 placeholder，显示 placeholder（灰色）
                if (displayText.empty() && !textEdit->placeholder.empty())
                {
                    displayText = textEdit->placeholder;
                    color = Eigen::Vector4f(0.5F, 0.5F, 0.5F, alpha);
                }

                const float lineHeight = static_cast<float>(TTF_GetFontHeight(m_defaultFont));

                const auto modeVal = static_cast<uint8_t>(textEdit->inputMode);
                const auto multiFlag = static_cast<uint8_t>(policies::TextFlag::Multiline);

                if ((modeVal & multiFlag) == 0)
                {
                    // 单行：水平滚动显示尾部
                    float visibleWidth = 0.0F;
                    std::string visibleText =
                        getTailThatFits(displayText, static_cast<int>(textSize.x()), visibleWidth);

                    const policies::Alignment align = policies::Alignment::LEFT | policies::Alignment::VCENTER;
                    if (!visibleText.empty())
                    {
                        addText(visibleText, textPos, textSize, color, align, alpha);
                    }

                    // 绘制光标 (仅当获焦时)
                    if (Registry::AnyOf<components::FocusedTag>(entity))
                    {
                        float cursorX = textPos.x() + visibleWidth;
                        float cursorY = textPos.y() + (textSize.y() - lineHeight) * 0.5F;

                        if ((SDL_GetTicks() / 500) % 2 == 0)
                        {
                            addRectFilledWithRounding(
                                {cursorX, cursorY}, {2.0F, lineHeight}, {1.0F, 1.0F, 1.0F, 1.0F}, {0, 0, 0, 0}, alpha);
                        }

                        if (sdlWindow)
                        {
                            SDL_Rect rect;
                            rect.x = static_cast<int>(cursorX);
                            rect.y = static_cast<int>(cursorY);
                            rect.w = 2;
                            rect.h = static_cast<int>(lineHeight);
                            SDL_SetTextInputArea(sdlWindow, &rect, 0);
                        }
                    }
                }
                else
                {
                    // 多行：自动换行 + 支持滚动
                    policies::TextWrap wrapMode =
                        textComp->wordWrap != policies::TextWrap::None ? textComp->wordWrap : policies::TextWrap::Word;
                    std::vector<std::string> lines =
                        wrapTextLines(displayText, static_cast<int>(textSize.x()), wrapMode);

                    // 计算文本总高度并更新 ScrollArea contentSize
                    float totalTextHeight = lines.size() * lineHeight;
                    if (auto* scrollArea = Registry::TryGet<components::ScrollArea>(entity))
                    {
                        // 视口高度即为当前的 textSize.y() (已去除 Padding)
                        float viewportHeight = textSize.y();

                        if (scrollArea->contentSize.y() != totalTextHeight)
                        {
                            float oldHeight = scrollArea->contentSize.y();
                            float newHeight = totalTextHeight;

                            scrollArea->contentSize.x() = textSize.x();
                            scrollArea->contentSize.y() = totalTextHeight;

                            // 应用锚定策略
                            if (scrollArea->anchor == policies::ScrollAnchor::Bottom)
                            {
                                // 锚定底部：Offset 随高度差增加，保持距离底部不变
                                scrollArea->scrollOffset.y() += (newHeight - oldHeight);
                            }
                            else if (scrollArea->anchor == policies::ScrollAnchor::Smart)
                            {
                                // 智能模式：如果之前在底部，则保持在底部
                                float oldMaxScroll = std::max(0.0F, oldHeight - viewportHeight);
                                // 给予 2.0 像素的容差
                                bool wasAtBottom = (scrollArea->scrollOffset.y() >= oldMaxScroll - 2.0F);

                                if (wasAtBottom)
                                {
                                    float newMaxScroll = std::max(0.0F, newHeight - viewportHeight);
                                    scrollArea->scrollOffset.y() = newMaxScroll;
                                }
                            }
                        }

                        // 始终更新宽度记录
                        scrollArea->contentSize.x() = textSize.x();

                        // 始终执行 Clamping，防止视口变化(Resize)导致的越界
                        // 注意：这里使用当前的 totalTextHeight 和 viewportHeight 进行计算
                        float maxScroll = std::max(0.0F, totalTextHeight - viewportHeight);
                        scrollArea->scrollOffset.y() = std::clamp(scrollArea->scrollOffset.y(), 0.0F, maxScroll);

                        // 使用滚动偏移来确定起始行
                        const int maxVisibleLines = lineHeight > 0.0F ? static_cast<int>(textSize.y() / lineHeight) : 0;
                        const int scrollOffsetLines =
                            lineHeight > 0.0F ? static_cast<int>(scrollArea->scrollOffset.y() / lineHeight) : 0;
                        const size_t startIndex =
                            std::min(static_cast<size_t>(scrollOffsetLines), lines.size() > 0 ? lines.size() - 1 : 0);
                        const size_t endIndex =
                            std::min(startIndex + static_cast<size_t>(maxVisibleLines) + 1, lines.size());

                        float y = textPos.y() - (scrollArea->scrollOffset.y() - scrollOffsetLines * lineHeight);
                        for (size_t i = startIndex; i < endIndex; ++i)
                        {
                            const std::string& line = lines[i];
                            if (!line.empty())
                            {
                                addText(line,
                                        {textPos.x(), y},
                                        {textSize.x(), lineHeight},
                                        color,
                                        policies::Alignment::LEFT,
                                        alpha);
                            }
                            y += lineHeight;
                        }

                        // 绘制光标 (仅当获焦时) - ScrollArea 分支
                        if (Registry::AnyOf<components::FocusedTag>(entity))
                        {
                            float cursorX = textPos.x();
                            float cursorY = textPos.y();

                            if (!lines.empty())
                            {
                                // 光标在最后一行末尾
                                const std::string& lastLine = lines.back();
                                float lastWidth = measureTextWidth(lastLine);

                                // 计算光标在可见区域中的位置
                                const int lastLineIndex = static_cast<int>(lines.size()) - 1;

                                // 如果最后一行在可见区域内
                                if (lastLineIndex >= scrollOffsetLines &&
                                    lastLineIndex < scrollOffsetLines + maxVisibleLines)
                                {
                                    const int visibleLineIndex = lastLineIndex - scrollOffsetLines;
                                    cursorY = textPos.y() + visibleLineIndex * lineHeight -
                                              (scrollArea->scrollOffset.y() - scrollOffsetLines * lineHeight);
                                    cursorX = textPos.x() + lastWidth;
                                }
                                else
                                {
                                    // 光标不在可见区域，不显示
                                    cursorX = -1000.0F;
                                    cursorY = -1000.0F;
                                }
                            }

                            if (cursorX >= 0.0F && cursorY >= 0.0F && (SDL_GetTicks() / 500) % 2 == 0)
                            {
                                addRectFilledWithRounding({cursorX, cursorY},
                                                          {2.0F, lineHeight},
                                                          {1.0F, 1.0F, 1.0F, 1.0F},
                                                          {0, 0, 0, 0},
                                                          alpha);
                            }

                            if (sdlWindow && cursorX >= 0.0F && cursorY >= 0.0F)
                            {
                                SDL_Rect rect;
                                rect.x = static_cast<int>(cursorX);
                                rect.y = static_cast<int>(cursorY);
                                rect.w = 2;
                                rect.h = static_cast<int>(lineHeight);
                                SDL_SetTextInputArea(sdlWindow, &rect, 0);
                            }
                        }
                    }
                    else
                    {
                        // 没有 ScrollArea 的情况：显示末尾内容（保持原有行为）
                        const int maxLines = lineHeight > 0.0F ? static_cast<int>(textSize.y() / lineHeight) : 0;
                        const size_t startIndex = (maxLines > 0 && lines.size() > static_cast<size_t>(maxLines))
                                                      ? (lines.size() - static_cast<size_t>(maxLines))
                                                      : 0;

                        float y = textPos.y();
                        for (size_t i = startIndex; i < lines.size(); ++i)
                        {
                            const std::string& line = lines[i];
                            if (!line.empty())
                            {
                                addText(line,
                                        {textPos.x(), y},
                                        {textSize.x(), lineHeight},
                                        color,
                                        policies::Alignment::LEFT,
                                        alpha);
                            }
                            y += lineHeight;
                        }

                        // 绘制光标 (仅当获焦时) - 无 ScrollArea 分支
                        if (Registry::AnyOf<components::FocusedTag>(entity))
                        {
                            float cursorX = textPos.x();
                            float cursorY = textPos.y();
                            if (!lines.empty())
                            {
                                const std::string& lastLine = lines.back();
                                float lastWidth = measureTextWidth(lastLine);
                                const int visibleLineCount =
                                    maxLines > 0 ? std::min(maxLines, static_cast<int>(lines.size())) : 1;
                                cursorY = textPos.y() + (visibleLineCount - 1) * lineHeight;
                                cursorX = textPos.x() + lastWidth;
                            }

                            if ((SDL_GetTicks() / 500) % 2 == 0)
                            {
                                addRectFilledWithRounding({cursorX, cursorY},
                                                          {2.0F, lineHeight},
                                                          {1.0F, 1.0F, 1.0F, 1.0F},
                                                          {0, 0, 0, 0},
                                                          alpha);
                            }

                            if (sdlWindow)
                            {
                                SDL_Rect rect;
                                rect.x = static_cast<int>(cursorX);
                                rect.y = static_cast<int>(cursorY);
                                rect.w = 2;
                                rect.h = static_cast<int>(lineHeight);
                                SDL_SetTextInputArea(sdlWindow, &rect, 0);
                            }
                        }
                    }
                }

                if (pushScissor)
                {
                    m_scissorStack.pop_back();
                }
            }
        }

        // 图像渲染
        if (Registry::AnyOf<components::ImageTag>(entity))
        {
            const auto& imageComp = Registry::Get<components::Image>(entity);
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
        if (!m_scissorStack.empty())
        {
            batch.scissorRect = m_scissorStack.back();
        }
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
        batch.pushConstants.padding = 0.0F;

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
        if (utils::HasAlignment(alignment, policies::Alignment::HCENTER))
        {
            drawX += (size.x() - textSize.x()) * 0.5F;
        }
        else if (utils::HasAlignment(alignment, policies::Alignment::RIGHT))
        {
            drawX += size.x() - textSize.x();
        }

        // 垂直对齐
        if (utils::HasAlignment(alignment, policies::Alignment::VCENTER))
        {
            drawY += (size.y() - textSize.y()) * 0.5F;
        }
        else if (utils::HasAlignment(alignment, policies::Alignment::BOTTOM))
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
        if (!m_scissorStack.empty())
        {
            batch.scissorRect = m_scissorStack.back();
        }
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
        batch.pushConstants.padding = 0.0F;

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
     * @brief 渲染图标
     * @param entity 实体
     * @param iconComp Icon组件指针
     * @param widgetPos 控件位置
     * @param widgetSize 控件尺寸
     * @param alpha 透明度
     */
    void renderIcon(entt::entity entity,
                    const components::Icon* iconComp,
                    const Eigen::Vector2f& widgetPos,
                    const Eigen::Vector2f& widgetSize,
                    float alpha)
    {
        if (!iconComp) return;

        // 根据类型判断是否有有效数据
        if (iconComp->type == policies::IconType::Texture && !iconComp->textureId) return;
        if (iconComp->type == policies::IconType::Font && (!iconComp->fontHandle || iconComp->codepoint == 0)) return;

        // 获取文本组件以计算文本尺寸
        const auto* textComp = Registry::TryGet<components::Text>(entity);

        // 计算文本实际尺寸
        Eigen::Vector2f textSize{0.0F, 0.0F};
        if (textComp && !textComp->content.empty())
        {
            float textWidth = measureTextWidth(textComp->content);
            float textHeight = m_defaultFont ? static_cast<float>(TTF_GetFontHeight(m_defaultFont)) : 16.0F;
            textSize = {textWidth, textHeight};
        }

        // 图标位置计算
        Eigen::Vector2f iconPos;
        Eigen::Vector2f iconSize{iconComp->size.x(), iconComp->size.y()};

        switch (iconComp->position)
        {
            case policies::IconPosition::Left:
                // 图标在文本左侧
                iconPos.x() = widgetPos.x() + (widgetSize.x() - textSize.x() - iconSize.x() - iconComp->spacing) * 0.5F;
                iconPos.y() = widgetPos.y() + (widgetSize.y() - iconSize.y()) * 0.5F;
                break;

            case policies::IconPosition::Right:
                // 图标在文本右侧
                iconPos.x() = widgetPos.x() + (widgetSize.x() + textSize.x() + iconComp->spacing - iconSize.x()) * 0.5F;
                iconPos.y() = widgetPos.y() + (widgetSize.y() - iconSize.y()) * 0.5F;
                break;

            case policies::IconPosition::Top:
                // 图标在文本上方
                iconPos.x() = widgetPos.x() + (widgetSize.x() - iconSize.x()) * 0.5F;
                iconPos.y() = widgetPos.y() + (widgetSize.y() - textSize.y() - iconSize.y() - iconComp->spacing) * 0.5F;
                break;

            case policies::IconPosition::Bottom:
                // 图标在文本下方
                iconPos.x() = widgetPos.x() + (widgetSize.x() - iconSize.x()) * 0.5F;
                iconPos.y() = widgetPos.y() + (widgetSize.y() + textSize.y() + iconComp->spacing - iconSize.y()) * 0.5F;
                break;
        }

        // 根据类型渲染图标
        if (iconComp->type == policies::IconType::Texture)
        {
            // 渲染纹理图标
            Eigen::Vector4f tint(iconComp->tintColor.red,
                                 iconComp->tintColor.green,
                                 iconComp->tintColor.blue,
                                 iconComp->tintColor.alpha);

            addImageBatch(static_cast<SDL_GPUTexture*>(iconComp->textureId),
                          iconPos,
                          iconSize,
                          {iconComp->uvMin.x(), iconComp->uvMin.y()},
                          {iconComp->uvMax.x(), iconComp->uvMax.y()},
                          tint,
                          alpha);
        }
        else if (iconComp->type == policies::IconType::Font)
        {
            // 渲染字体图标
            renderFontIcon(iconComp, iconPos, iconSize, alpha);
        }
    }

    /**
     * @brief 渲染字体图标（IconFont）
     */
    void renderFontIcon(const components::Icon* iconComp,
                        const Eigen::Vector2f& pos,
                        const Eigen::Vector2f& size,
                        float alpha)
    {
        if (!iconComp || !iconComp->fontHandle || iconComp->codepoint == 0) return;

        // 将 codepoint 转换为 UTF-8 字符串
        std::string iconChar;
        uint32_t cp = iconComp->codepoint;
        if (cp < 0x80)
        {
            iconChar.push_back(static_cast<char>(cp));
        }
        else if (cp < 0x800)
        {
            iconChar.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            iconChar.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
        else if (cp < 0x10000)
        {
            iconChar.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            iconChar.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            iconChar.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
        else
        {
            iconChar.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            iconChar.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            iconChar.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            iconChar.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }

        // 使用 IconFont 渲染文本
        TTF_Font* iconFont = static_cast<TTF_Font*>(iconComp->fontHandle);

        // 临时保存当前字体
        TTF_Font* prevFont = m_defaultFont;
        m_defaultFont = iconFont;

        // 渲染图标字符
        Eigen::Vector4f color(
            iconComp->tintColor.red, iconComp->tintColor.green, iconComp->tintColor.blue, iconComp->tintColor.alpha);

        addText(iconChar, pos, size, color, policies::Alignment::CENTER, alpha);

        // 恢复原字体
        m_defaultFont = prevFont;
    }

    static size_t nextUtf8CharLen(unsigned char c)
    {
        if (c < 0x80) return 1;
        if ((c & 0xE0) == 0xC0) return 2;
        if ((c & 0xF0) == 0xE0) return 3;
        if ((c & 0xF8) == 0xF0) return 4;
        return 1;
    }

    float measureTextWidth(const std::string& text) const
    {
        if (m_defaultFont == nullptr || text.empty()) return 0.0F;
        int measuredWidth = 0;
        size_t measuredLength = 0;
        TTF_MeasureString(m_defaultFont, text.c_str(), text.size(), 100000, &measuredWidth, &measuredLength);
        return static_cast<float>(measuredWidth);
    }

    std::string ltrimSpaces(std::string text) const
    {
        size_t i = 0;
        while (i < text.size() && (text[i] == ' ' || text[i] == '\t'))
        {
            ++i;
        }
        if (i > 0) text.erase(0, i);
        return text;
    }

    std::vector<std::string> wrapTextLines(const std::string& text, int maxWidth, policies::TextWrap wrapMode) const
    {
        std::vector<std::string> lines;
        if (m_defaultFont == nullptr)
        {
            lines.push_back(text);
            return lines;
        }

        if (wrapMode == policies::TextWrap::None || maxWidth <= 0)
        {
            lines.push_back(text);
            return lines;
        }

        size_t start = 0;
        while (start <= text.size())
        {
            size_t end = text.find('\n', start);
            if (end == std::string::npos) end = text.size();
            std::string segment = text.substr(start, end - start);

            if (segment.empty())
            {
                lines.emplace_back();
            }
            else
            {
                while (!segment.empty())
                {
                    int measuredWidth = 0;
                    size_t measuredLength = 0;
                    bool ok = TTF_MeasureString(
                        m_defaultFont, segment.c_str(), segment.size(), maxWidth, &measuredWidth, &measuredLength);

                    if (!ok || measuredLength == 0)
                    {
                        size_t len = nextUtf8CharLen(static_cast<unsigned char>(segment[0]));
                        lines.push_back(segment.substr(0, len));
                        segment.erase(0, len);
                        segment = ltrimSpaces(segment);
                        continue;
                    }

                    if (measuredLength >= segment.size())
                    {
                        lines.push_back(segment);
                        segment.clear();
                        break;
                    }

                    size_t breakLen = measuredLength;
                    if (wrapMode == policies::TextWrap::Word)
                    {
                        size_t spacePos = segment.find_last_of(' ', measuredLength - 1);
                        if (spacePos != std::string::npos && spacePos > 0)
                        {
                            breakLen = spacePos;
                        }
                    }

                    lines.push_back(segment.substr(0, breakLen));
                    segment.erase(0, breakLen);
                    segment = ltrimSpaces(segment);
                }
            }

            if (end == text.size()) break;
            start = end + 1;
        }

        return lines;
    }

    std::string getTailThatFits(const std::string& text, int maxWidth, float& outWidth) const
    {
        outWidth = 0.0F;
        if (m_defaultFont == nullptr || text.empty() || maxWidth <= 0) return std::string();

        size_t start = 0;
        while (start < text.size())
        {
            int measuredWidth = 0;
            size_t measuredLength = 0;
            bool ok = TTF_MeasureString(
                m_defaultFont, text.c_str() + start, text.size() - start, maxWidth, &measuredWidth, &measuredLength);

            if (ok && measuredLength == text.size() - start)
            {
                outWidth = static_cast<float>(measuredWidth);
                return text.substr(start);
            }

            start += nextUtf8CharLen(static_cast<unsigned char>(text[start]));
        }

        return std::string();
    }

    float getAncestorScrollAreaTextWidth(entt::entity entity) const
    {
        entt::entity current = entity;
        while (current != entt::null && Registry::Valid(current))
        {
            const auto* hierarchy = Registry::TryGet<components::Hierarchy>(current);
            if (hierarchy == nullptr) break;
            current = hierarchy->parent;
            if (current == entt::null) break;

            if (Registry::AnyOf<components::ScrollArea>(current))
            {
                const auto* size = Registry::TryGet<components::Size>(current);
                if (size == nullptr) return 0.0F;

                float width = size->size.x();
                if (const auto* padding = Registry::TryGet<components::Padding>(current))
                {
                    width -= (padding->values.y() + padding->values.z());
                }
                return std::max(0.0F, width);
            }
        }
        return 0.0F;
    }

    void addWrappedText(const std::string& text,
                        const Eigen::Vector2f& pos,
                        const Eigen::Vector2f& size,
                        const Eigen::Vector4f& color,
                        policies::Alignment alignment,
                        policies::TextWrap wrapMode,
                        float wrapWidth,
                        float opacity)
    {
        if (m_defaultFont == nullptr || text.empty() || wrapWidth <= 0.0F) return;

        const float lineHeight = static_cast<float>(TTF_GetFontHeight(m_defaultFont));
        if (lineHeight <= 0.0F) return;

        std::vector<std::string> lines = wrapTextLines(text, static_cast<int>(wrapWidth), wrapMode);
        const float totalHeight = static_cast<float>(lines.size()) * lineHeight;

        float startY = pos.y();
        if (ui::utils::HasAlignment(alignment, policies::Alignment::VCENTER))
        {
            startY += (size.y() - totalHeight) * 0.5F;
        }
        else if (ui::utils::HasAlignment(alignment, policies::Alignment::BOTTOM))
        {
            startY += size.y() - totalHeight;
        }

        const uint8_t horizontalMask = static_cast<uint8_t>(policies::Alignment::LEFT) |
                                       static_cast<uint8_t>(policies::Alignment::HCENTER) |
                                       static_cast<uint8_t>(policies::Alignment::RIGHT);
        const uint8_t alignValue = static_cast<uint8_t>(alignment);
        policies::Alignment horizontalAlign = static_cast<policies::Alignment>(alignValue & horizontalMask);
        if (horizontalAlign == policies::Alignment::NONE)
        {
            horizontalAlign = policies::Alignment::LEFT;
        }

        float y = startY;
        for (const auto& line : lines)
        {
            if (!line.empty())
            {
                addText(line, {pos.x(), y}, {wrapWidth, lineHeight}, color, horizontalAlign, opacity);
            }
            y += lineHeight;
        }
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
            Logger::info("Failed to render text: %s", SDL_GetError());
            return nullptr;
        }

        // 转换为 RGBA32，确保格式与 GPU 纹理一致
        SDL_Surface* textSurface = SDL_ConvertSurface(rawSurface, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(rawSurface);
        if (textSurface == nullptr)
        {
            Logger::info("Failed to convert text surface: %s", SDL_GetError());
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
    void renderToGPU(SDL_Window* window, int width, int height)
    {
        static bool firstRender = true;
        if (firstRender)
        {
            Logger::info("RenderSystem::renderToGPU first call - Attempting to acquire command buffer");
        }

        // 获取命令缓冲区
        SDL_GPUCommandBuffer* cmdBuf = SDL_AcquireGPUCommandBuffer(m_gpuDevice);
        if (cmdBuf == nullptr) return;

        if (firstRender)
        {
            Logger::info(
                "RenderSystem::renderToGPU - Command buffer acquired. Attempting to acquire swapchain texture");
        }

        // 获取交换链纹理
        SDL_GPUTexture* swapchainTexture = nullptr;
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdBuf, window, &swapchainTexture, nullptr, nullptr))
        {
            if (firstRender)
                Logger::error("RenderSystem::renderToGPU - Failed to acquire swapchain texture: {}", SDL_GetError());
            SDL_CancelGPUCommandBuffer(cmdBuf);
            return;
        }

        if (firstRender)
        {
            firstRender = false;
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

            // 设置裁剪
            if (batch.scissorRect.has_value())
            {
                SDL_SetGPUScissor(renderPass, &batch.scissorRect.value());
            }
            else
            {
                SDL_Rect fullRect = {0, 0, width, height};
                SDL_SetGPUScissor(renderPass, &fullRect);
            }

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

private:
    SDL_GPUDevice* m_gpuDevice = nullptr;
    SDL_GPUGraphicsPipeline* m_pipeline = nullptr;
    SDL_GPUShader* m_vertexShader = nullptr;
    SDL_GPUShader* m_fragmentShader = nullptr;
    SDL_GPUSampler* m_sampler = nullptr;
    TTF_Font* m_defaultFont = nullptr;
    std::string m_gpuDriver; // GPU 驱动名称（vulkan 或 d3d12）

    std::vector<RenderBatch> m_batches;
    SDL_GPUBuffer* m_vertexBuffer = nullptr;
    SDL_GPUBuffer* m_indexBuffer = nullptr;
    SDL_GPUTexture* m_whiteTexture = nullptr; // 默认白色纹理用于纯色渲染

    std::unordered_map<std::string, CachedTexture> m_textureCache;
    std::unordered_set<SDL_Window*> m_claimedWindows;
    std::vector<SDL_Rect> m_scissorStack;

    // 当前屏幕尺寸
    float m_screenWidth = 0.0F;
    float m_screenHeight = 0.0F;
};

} // namespace ui::systems
