/**
 * ************************************************************************
 *
 * @file CommandBuffer.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-30
 * @version 0.1
 * @brief 命令缓冲区包装器 - 封装SDL GPU命令和资源管理
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <vector>
#include <SDL3/SDL_gpu.h>
#include "../managers/DeviceManager.hpp"
#include "../managers/PipelineCache.hpp"
#include "../common/RenderTypes.hpp"
#include "../singleton/Logger.hpp"

namespace ui::managers
{

/**
 * @brief 命令缓冲区包装器
 *
 * 负责：
 * 1. 封装SDL GPU命令的提交、渲染通道等操作
 * 2. 管理顶点/索引缓冲区的生命周期和池化
 */
class CommandBuffer
{
public:
    CommandBuffer(DeviceManager& deviceManager, PipelineCache& pipelineCache)
        : m_deviceManager(deviceManager), m_pipelineCache(pipelineCache)
    {
    }

    ~CommandBuffer() { cleanup(); }

    /**
     * @brief 执行渲染批次
     * @param batches 渲染批次列表
     */
    void execute(SDL_Window* window, int width, int height, const std::vector<render::RenderBatch>& batches)
    {
        SDL_GPUDevice* device = m_deviceManager.getDevice();
        if (device == nullptr) return;

        SDL_GPUCommandBuffer* cmdBuf = SDL_AcquireGPUCommandBuffer(device);
        if (cmdBuf == nullptr) return;

        SDL_GPUTexture* swapchainTexture = nullptr;
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdBuf, window, &swapchainTexture, nullptr, nullptr))
        {
            Logger::warn("Swapchain texture not ready yet.");
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
        colorTarget.clear_color = {0.0F, 0.0F, 0.0F, 1.0F}; // 清除为黑色
        colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTarget.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmdBuf, &colorTarget, 1, nullptr);

        // 绑定管线
        SDL_BindGPUGraphicsPipeline(renderPass, m_pipelineCache.getPipeline());

        // 设置视口
        SDL_GPUViewport viewport = {};
        viewport.x = 0;
        viewport.y = 0;
        viewport.w = static_cast<float>(width);
        viewport.h = static_cast<float>(height);
        viewport.min_depth = 0.0F;
        viewport.max_depth = 1.0F;
        SDL_SetGPUViewport(renderPass, &viewport);

        for (const auto& batch : batches)
        {
            if (batch.vertices.empty() || batch.indices.empty()) continue;

            // 设置裁剪
            if (batch.scissorRect.has_value())
            {
                SDL_Rect r = batch.scissorRect.value();
                SDL_SetGPUScissor(renderPass, &r);
            }
            else
            {
                // 没有裁剪区域时，设置为整个视口
                SDL_Rect fullViewport = {0, 0, width, height};
                SDL_SetGPUScissor(renderPass, &fullViewport);
            }

            // 上传顶点和索引数据
            SDL_GPUBuffer* vertexBuffer = uploadBuffer(
                SDL_GPU_BUFFERUSAGE_VERTEX, batch.vertices.size() * sizeof(render::Vertex), batch.vertices.data());
            SDL_GPUBuffer* indexBuffer =
                uploadBuffer(SDL_GPU_BUFFERUSAGE_INDEX, batch.indices.size() * sizeof(uint16_t), batch.indices.data());

            if (vertexBuffer == nullptr || indexBuffer == nullptr)
            {
                Logger::error("Failed to upload vertex or index buffer.");
                // Release any buffers that might have been created before continuing to the next batch.
                if (vertexBuffer)
                {
                    SDL_ReleaseGPUBuffer(device, vertexBuffer);
                }
                if (indexBuffer)
                {
                    SDL_ReleaseGPUBuffer(device, indexBuffer);
                }
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

            // 绑定纹理和采样器
            if (batch.texture != nullptr)
            {
                SDL_GPUTextureSamplerBinding texSamplerBinding = {};
                texSamplerBinding.texture = batch.texture;
                texSamplerBinding.sampler = m_pipelineCache.getSampler();
                SDL_BindGPUFragmentSamplers(renderPass, 0, &texSamplerBinding, 1);
            }

            // 推送 Push Constants
            SDL_PushGPUVertexUniformData(cmdBuf, 0, &batch.pushConstants, sizeof(render::UiPushConstants));
            SDL_PushGPUFragmentUniformData(cmdBuf, 0, &batch.pushConstants, sizeof(render::UiPushConstants));

            // 绘制
            SDL_DrawGPUIndexedPrimitives(renderPass, static_cast<uint32_t>(batch.indices.size()), 1, 0, 0, 0);

            // 释放临时缓冲区（未来考虑池化）
            SDL_ReleaseGPUBuffer(device, vertexBuffer);
            SDL_ReleaseGPUBuffer(device, indexBuffer);
        }

        // 结束渲染通道
        SDL_EndGPURenderPass(renderPass);

        // 提交命令缓冲区
        SDL_SubmitGPUCommandBuffer(cmdBuf);
    }

    /**
     * @brief 清理资源
     */
    void cleanup()
    {
        // 目前是每次都释放，未来可以实现缓冲区池化
    }

private:
    SDL_GPUBuffer* uploadBuffer(SDL_GPUBufferUsageFlags usage, uint32_t size, const void* data)
    {
        SDL_GPUDevice* device = m_deviceManager.getDevice();
        if (device == nullptr) return nullptr;

        SDL_GPUBufferCreateInfo bufferInfo = {};
        bufferInfo.usage = usage;
        bufferInfo.size = size;
        SDL_GPUBuffer* buffer = SDL_CreateGPUBuffer(device, &bufferInfo);

        if (buffer == nullptr) return nullptr;

        SDL_GPUTransferBufferCreateInfo transferInfo = {};
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferInfo.size = size;

        SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);

        if (transferBuffer == nullptr)
        {
            SDL_ReleaseGPUBuffer(device, buffer);
            return nullptr;
        }

        void* mapData = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
        SDL_memcpy(mapData, data, size);
        SDL_UnmapGPUTransferBuffer(device, transferBuffer);

        SDL_GPUCommandBuffer* uploadCmd = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmd);

        SDL_GPUTransferBufferLocation srcLocation = {};
        srcLocation.transfer_buffer = transferBuffer;
        srcLocation.offset = 0;

        SDL_GPUBufferRegion dstRegion = {};
        dstRegion.buffer = buffer;
        dstRegion.offset = 0;
        dstRegion.size = size;

        SDL_UploadToGPUBuffer(copyPass, &srcLocation, &dstRegion, false);
        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(uploadCmd);

        SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

        return buffer;
    }

private:
    DeviceManager& m_deviceManager;
    PipelineCache& m_pipelineCache;
};

} // namespace ui::managers
