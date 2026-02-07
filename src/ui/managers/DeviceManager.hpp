/**
 * ************************************************************************
 *
 * @file DeviceManager.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-30
 * @version 0.1
 * @brief 管理 GPU 设备和窗口声明

 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once
#include <memory>
#include <string>
#include <unordered_set>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include "../singleton/Logger.hpp"
#include "../common/GPUWrappers.hpp"

namespace ui::managers
{

class GPUBackendHandler
{
public:
    std::shared_ptr<GPUBackendHandler> nextHandler;
    GPUBackendHandler() = default;
    GPUBackendHandler(const GPUBackendHandler&) = delete;
    GPUBackendHandler& operator=(const GPUBackendHandler&) = delete;
    virtual ~GPUBackendHandler() = default;
    GPUBackendHandler(GPUBackendHandler&&) noexcept = default;
    GPUBackendHandler& operator=(GPUBackendHandler&&) noexcept = default;

    // 设置链中的下一个处理器
    void setNext(std::shared_ptr<GPUBackendHandler> next) { nextHandler = next; }

    // 处理请求的核心逻辑
    virtual wrappers::UniqueGPUDevice handle(std::string& outDriverName)
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
    wrappers::UniqueGPUDevice handle(std::string& outDriverName) override
    {
        Logger::info("责任链：尝试初始化 D3D12...");

        wrappers::UniquePropertiesID props(SDL_CreateProperties());
        SDL_SetStringProperty(props, SDL_PROP_GPU_DEVICE_CREATE_NAME_STRING, "direct3d12");
        SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, true);
        SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_DXIL_BOOLEAN, true);

        SDL_GPUDevice* device = SDL_CreateGPUDeviceWithProperties(props);

        if (device != nullptr)
        {
            outDriverName = "direct3d12";
            return wrappers::UniqueGPUDevice(device);
        }

        Logger::warn("D3D12 不可用，传递给链中下一个处理器。原因: {}", SDL_GetError());
        return GPUBackendHandler::handle(outDriverName);
    }
};

class VulkanHandler final : public GPUBackendHandler
{
public:
    wrappers::UniqueGPUDevice handle(std::string& outDriverName) override
    {
        Logger::info("责任链：尝试初始化 Vulkan...");

        wrappers::UniquePropertiesID props(SDL_CreateProperties());
        SDL_SetStringProperty(props, SDL_PROP_GPU_DEVICE_CREATE_NAME_STRING, "vulkan");
        SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, false);
        SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);

        SDL_GPUDevice* device = SDL_CreateGPUDeviceWithProperties(props);

        if (device != nullptr)
        {
            outDriverName = "vulkan";
            return wrappers::UniqueGPUDevice(device);
        }

        Logger::warn("Vulkan 不可用。原因: {}", SDL_GetError());
        return GPUBackendHandler::handle(outDriverName);
    }
};
/**
 * @brief 管理 GPU 设备和窗口声明
 */
class DeviceManager
{
public:
    DeviceManager() = default;
    ~DeviceManager() { cleanup(); }
    DeviceManager(const DeviceManager&) = delete;
    DeviceManager& operator=(const DeviceManager&) = delete;
    DeviceManager(DeviceManager&&) noexcept = default;
    DeviceManager& operator=(DeviceManager&&) noexcept = default;

    bool initialize()
    {
        if (m_gpuDevice != nullptr) return true;

        // 1. 组装责任链
        Logger::info("DeviceManager: Building handler chain");
        auto d3d12 = std::make_shared<D3D12Handler>();
        auto vulkan = std::make_shared<VulkanHandler>();

        d3d12->setNext(vulkan);
        vulkan->setNext(nullptr);
        // 2. 启动链式处理
        m_gpuDevice = d3d12->handle(m_gpuDriver);

        // 3. 结果检查
        if (m_gpuDevice == nullptr)
        {
            Logger::error("所有 GPU 后端均初始化失败！");
            return false;
        }

        Logger::info("GPU 初始化成功，使用后端: {}", m_gpuDriver);
        return true;
    }

    bool claimWindow(SDL_Window* sdlWindow)
    {
        if (m_gpuDevice == nullptr || sdlWindow == nullptr)
        {
            return false;
        }

        SDL_WindowID windowID = SDL_GetWindowID(sdlWindow);
        if (m_claimedWindows.contains(windowID))
        {
            return true;
        }

        // 4. 声明窗口
        if (!SDL_ClaimWindowForGPUDevice(m_gpuDevice.get(), sdlWindow))
        {
            Logger::error("窗口声明失败: {}", SDL_GetError());
            return false;
        }

        m_claimedWindows.insert(windowID);
        return true;
    }

    void unclaimWindow(SDL_Window* sdlWindow)
    {
        if (m_gpuDevice == nullptr || sdlWindow == nullptr) return;

        SDL_WindowID windowID = SDL_GetWindowID(sdlWindow);
        if (m_claimedWindows.contains(windowID))
        {
            SDL_ReleaseWindowFromGPUDevice(m_gpuDevice.get(), sdlWindow);
            m_claimedWindows.erase(windowID);
        }
    }

    void cleanup()
    {
        if (m_gpuDevice == nullptr) return;

        SDL_WaitForGPUIdle(m_gpuDevice.get());

        for (auto windowID : m_claimedWindows)
        {
            SDL_Window* window = SDL_GetWindowFromID(windowID);
            if (window != nullptr)
            {
                SDL_ReleaseWindowFromGPUDevice(m_gpuDevice.get(), window);
            }
        }
        m_claimedWindows.clear();

        m_gpuDevice.reset();
    }

    SDL_GPUDevice* getDevice() const { return m_gpuDevice.get(); }
    const std::string& getDriverName() const { return m_gpuDriver; }

    SDL_GPUTexture* getWhiteTexture() const { return nullptr; } // TODO: Implement white texture creation

private:
    wrappers::UniqueGPUDevice m_gpuDevice;
    std::string m_gpuDriver;
    std::unordered_set<SDL_WindowID> m_claimedWindows;
};

} // namespace ui::managers
