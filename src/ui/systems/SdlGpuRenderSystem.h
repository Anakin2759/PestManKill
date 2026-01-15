/**
 * ************************************************************************
 *
 * @file SdlGpuRenderSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-13
 * @version 0.1
 * @brief SDL GPU 渲染系统

  - 负责使用 SDL GPU 进行 UI 渲染
  - 集成 ImGui 渲染到 SDL GPU 上下文
  - 响应 UI 渲染事件，执行实际渲染操作

  加载assets/sharder/spir-v文件
  以后用来替换RenderSystem，支持SDL GPU渲染,为后续移除imGui SDL Renderer做准备

  支持的图形后端列表
    - Vulkan 优先
    - DX12 暂时不支持

    使用SDL ttf渲染字体到GPU纹理上
    默认字体用 assets/fonts/NotoSansSC-VariableFont_wght.ttf

 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

#include <utils.h>
#include "src/ui/interface/Isystem.h"
#include "src/ui/common/Events.h"
#include "src/ui/core/GraphicsContext.h"

namespace ui::systems
{

class SdlGpuRenderSystem : public ui::interface::EnableRegister<SdlGpuRenderSystem>
{
private:
    GraphicsContext* m_graphicsContext = nullptr;
    SDL_GPUDevice* m_gpuDevice = nullptr;
    SDL_Window* m_window = nullptr;
    bool m_initialized = false;

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
        // 暂时不接管渲染，仅初始化 GPU 设备
        // dispatcher.sink<ui::events::UpdateRendering>().connect<&SdlGpuRenderSystem::render>(*this);
    }

    /**
     * @brief 注销事件处理器
     */
    void unregisterHandlersImpl()
    {
        auto& dispatcher = utils::Dispatcher::getInstance();
        dispatcher.sink<events::GraphicsContextSetEvent>().disconnect<&SdlGpuRenderSystem::onGraphicsContextSet>(*this);
        // dispatcher.sink<ui::events::UpdateRendering>().disconnect<&SdlGpuRenderSystem::render>(*this);
        cleanup();
    }

private:
    /**
     * @brief 处理图形上下文设置事件，初始化 GPU 设备
     */
    void onGraphicsContextSet(const events::GraphicsContextSetEvent& event)
    {
        m_graphicsContext = static_cast<GraphicsContext*>(event.graphicsContext);
        if (m_graphicsContext == nullptr) return;

        m_window = m_graphicsContext->getWindow();
        if (m_window == nullptr) return;

        initGpuDevice();
    }

    /**
     * @brief 初始化 SDL GPU 设备
     */
    bool initGpuDevice()
    {
        if (m_initialized) return true;

        // 优先使用 Vulkan 后端
        m_gpuDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr);

        if (m_gpuDevice == nullptr)
        {
            LOG_ERROR("[SdlGpuRenderSystem] Failed to create GPU device: {}", SDL_GetError());
            return false;
        }

        // 将窗口与 GPU 设备关联
        if (!SDL_ClaimWindowForGPUDevice(m_gpuDevice, m_window))
        {
            LOG_ERROR("[SdlGpuRenderSystem] Failed to claim window for GPU: {}", SDL_GetError());
            SDL_DestroyGPUDevice(m_gpuDevice);
            m_gpuDevice = nullptr;
            return false;
        }

        m_initialized = true;
        LOG_INFO("[SdlGpuRenderSystem] GPU device initialized successfully");
        LOG_INFO("[SdlGpuRenderSystem] GPU Driver: {}", SDL_GetGPUDeviceDriver(m_gpuDevice));

        return true;
    }

    /**
     * @brief 清理 GPU 资源
     */
    void cleanup()
    {
        if (m_gpuDevice != nullptr)
        {
            if (m_window != nullptr)
            {
                SDL_ReleaseWindowFromGPUDevice(m_gpuDevice, m_window);
            }
            SDL_DestroyGPUDevice(m_gpuDevice);
            m_gpuDevice = nullptr;
        }
        m_initialized = false;
    }

public:
    /**
     * @brief 渲染一帧（极简实现：仅清屏）
     */
    void render()
    {
        if (!m_initialized || m_gpuDevice == nullptr || m_window == nullptr) return;

        // 获取命令缓冲区
        SDL_GPUCommandBuffer* cmdBuffer = SDL_AcquireGPUCommandBuffer(m_gpuDevice);
        if (cmdBuffer == nullptr)
        {
            LOG_ERROR("[SdlGpuRenderSystem] Failed to acquire command buffer");
            return;
        }

        // 获取交换链纹理
        SDL_GPUTexture* swapchainTexture = nullptr;
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdBuffer, m_window, &swapchainTexture, nullptr, nullptr))
        {
            LOG_ERROR("[SdlGpuRenderSystem] Failed to acquire swapchain texture");
            SDL_CancelGPUCommandBuffer(cmdBuffer);
            return;
        }

        if (swapchainTexture != nullptr)
        {
            // 设置渲染目标并清屏（深蓝色背景）
            SDL_GPUColorTargetInfo colorTargetInfo = {};
            colorTargetInfo.texture = swapchainTexture;
            colorTargetInfo.clear_color = {.r = 0.1F, .g = 0.1F, .b = 0.2F, .a = 1.0F};
            colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
            colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

            SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmdBuffer, &colorTargetInfo, 1, nullptr);
            if (renderPass != nullptr)
            {
                // 这里可以添加绑定管线、绘制等操作
                SDL_EndGPURenderPass(renderPass);
            }
        }

        // 提交命令缓冲区
        SDL_SubmitGPUCommandBuffer(cmdBuffer);
    }

    /**
     * @brief 检查 GPU 是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    /**
     * @brief 获取 GPU 设备
     */
    [[nodiscard]] SDL_GPUDevice* getGpuDevice() const { return m_gpuDevice; }
};

} // namespace ui::systems