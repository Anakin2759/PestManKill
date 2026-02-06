/**
 * ************************************************************************
 *
 * @file CommandBuffer.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-30
 * @version 0.1
 * @brief 命令缓冲区包装器 - 封装SDL GPU命令和资源管理
    池化
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
    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;
    CommandBuffer(CommandBuffer&&) = delete;
    CommandBuffer& operator=(CommandBuffer&&) = delete;

    /**
     * @brief 执行渲染批次
     * @param batches 渲染批次列表
     */
    void execute(SDL_Window* window, int width, int height, const std::vector<render::RenderBatch>& batches)
    {
        SDL_GPUDevice* device = m_deviceManager.getDevice();
        if (device == nullptr) return;

        // 计算所需的总缓冲区大小
        uint32_t totalVertexCount = 0;
        uint32_t totalIndexCount = 0;
        for (const auto& batch : batches)
        {
            totalVertexCount += static_cast<uint32_t>(batch.vertices.size());
            totalIndexCount += static_cast<uint32_t>(batch.indices.size());
        }

        if (totalVertexCount == 0 || totalIndexCount == 0) return;

        uint32_t totalVertexSize = totalVertexCount * sizeof(render::Vertex);
        uint32_t totalIndexSize = totalIndexCount * sizeof(uint16_t);

        // 获取当前帧资源
        FrameResource& currentFrame = m_frameResources[m_frameIndex % MAX_FRAMES_IN_FLIGHT];

        // 确保缓冲区足够大
        if (!resizeBuffers(device, currentFrame, totalVertexSize, totalIndexSize))
        {
            Logger::error("Failed to resize buffers.");
            return;
        }

        // 上传所有数据到传输缓冲区
        // 使用 cycle=true 让 SDL 自动管理传输缓冲区的轮替，避免 CPU 等待
        void* mapData = SDL_MapGPUTransferBuffer(device, m_transferBuffer, true);
        if (mapData != nullptr)
        {
            auto* ptr = static_cast<uint8_t*>(mapData);
            uint32_t vOffset = 0;
            uint32_t iOffset = totalVertexSize; // 索引数据紧跟在顶点数据之后

            for (const auto& batch : batches)
            {
                if (batch.vertices.empty()) continue;
                uint32_t vSize = static_cast<uint32_t>(batch.vertices.size() * sizeof(render::Vertex));
                SDL_memcpy(ptr + vOffset, batch.vertices.data(), vSize);
                vOffset += vSize;
            }

            for (const auto& batch : batches)
            {
                if (batch.indices.empty()) continue;
                uint32_t iSize = static_cast<uint32_t>(batch.indices.size() * sizeof(uint16_t));
                SDL_memcpy(ptr + iOffset, batch.indices.data(), iSize);
                iOffset += iSize; // Not strictly needed but good for logic
            }

            SDL_UnmapGPUTransferBuffer(device, m_transferBuffer);
        }
        else
        {
            Logger::error("Failed to map transfer buffer.");
            return;
        }

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

        // 1. 执行复制过程 (Host -> Device)
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmdBuf);

        SDL_GPUTransferBufferLocation srcLoc = {};
        srcLoc.transfer_buffer = m_transferBuffer;
        srcLoc.offset = 0;

        SDL_GPUBufferRegion dstReg = {};
        dstReg.buffer = currentFrame.vertexBuffer;
        dstReg.offset = 0;
        dstReg.size = totalVertexSize;

        // 复制顶点数据
        SDL_UploadToGPUBuffer(copyPass, &srcLoc, &dstReg, false);

        // 复制索引数据
        srcLoc.offset = totalVertexSize;
        dstReg.buffer = currentFrame.indexBuffer;
        dstReg.size = totalIndexSize;
        SDL_UploadToGPUBuffer(copyPass, &srcLoc, &dstReg, false);

        SDL_EndGPUCopyPass(copyPass);

        // 2. 开始渲染通道
        SDL_GPUColorTargetInfo colorTarget = {};
        colorTarget.texture = swapchainTexture;
        colorTarget.clear_color = {.r = 0.0F, .g = 0.0F, .b = 0.0F, .a = 1.0F}; // 清除为黑色
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

        // 绑定顶点和索引缓冲区 (使用当前帧的缓冲区)
        SDL_GPUBufferBinding vertexBinding = {};
        vertexBinding.buffer = currentFrame.vertexBuffer;
        vertexBinding.offset = 0;
        SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);

        SDL_GPUBufferBinding indexBinding = {};
        indexBinding.buffer = currentFrame.indexBuffer;
        indexBinding.offset = 0;
        SDL_BindGPUIndexBuffer(renderPass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

        // 绘制批次
        uint32_t currentVertexOffset = 0; // 顶点偏移 (Added to index value)
        uint32_t currentIndexOffset = 0;  // 索引缓冲区偏移 (In elements)

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
            SDL_DrawGPUIndexedPrimitives(renderPass,
                                         static_cast<uint32_t>(batch.indices.size()),
                                         1,
                                         currentIndexOffset,
                                         static_cast<int32_t>(currentVertexOffset),
                                         0);

            // 更新偏移
            currentVertexOffset += static_cast<uint32_t>(batch.vertices.size());
            currentIndexOffset += static_cast<uint32_t>(batch.indices.size());
        }

        // 结束渲染通道
        SDL_EndGPURenderPass(renderPass);

        // 提交命令缓冲区
        SDL_SubmitGPUCommandBuffer(cmdBuf);

        // 切换到下一帧
        m_frameIndex++;
    }

    /**
     * @brief 清理资源
     */
    void cleanup()
    {
        SDL_GPUDevice* device = m_deviceManager.getDevice();
        if (device)
        {
            for (auto& frame : m_frameResources)
            {
                if (frame.vertexBuffer)
                {
                    SDL_ReleaseGPUBuffer(device, frame.vertexBuffer);
                    frame.vertexBuffer = nullptr;
                }
                if (frame.indexBuffer)
                {
                    SDL_ReleaseGPUBuffer(device, frame.indexBuffer);
                    frame.indexBuffer = nullptr;
                }
                frame.vertexBufferSize = 0;
                frame.indexBufferSize = 0;
            }

            if (m_transferBuffer)
            {
                SDL_ReleaseGPUTransferBuffer(device, m_transferBuffer);
                m_transferBuffer = nullptr;
            }
        }
        m_transferBufferSize = 0;
    }

private:
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    struct FrameResource
    {
        SDL_GPUBuffer* vertexBuffer = nullptr;
        SDL_GPUBuffer* indexBuffer = nullptr;
        uint32_t vertexBufferSize = 0;
        uint32_t indexBufferSize = 0;
    };

    bool resizeBuffers(SDL_GPUDevice* device, FrameResource& frame, uint32_t vSize, uint32_t iSize)
    {
        // 传输缓冲区 (Shared across frames, handled by cycle=true)
        uint32_t neededTransfer = vSize + iSize;

        if (m_transferBufferSize < neededTransfer)
        {
            if (m_transferBuffer) SDL_ReleaseGPUTransferBuffer(device, m_transferBuffer);

            m_transferBufferSize =
                neededTransfer > m_transferBufferSize * 2 ? neededTransfer : m_transferBufferSize * 2;
            if (m_transferBufferSize < neededTransfer) m_transferBufferSize = neededTransfer;

            SDL_GPUTransferBufferCreateInfo tInfo = {};
            tInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            tInfo.size = m_transferBufferSize;
            m_transferBuffer = SDL_CreateGPUTransferBuffer(device, &tInfo);
            if (!m_transferBuffer) return false;
        }

        // 顶点缓冲区 (Per Frame)
        if (frame.vertexBufferSize < vSize)
        {
            if (frame.vertexBuffer) SDL_ReleaseGPUBuffer(device, frame.vertexBuffer);
            frame.vertexBufferSize = vSize > frame.vertexBufferSize * 2 ? vSize : frame.vertexBufferSize * 2;

            SDL_GPUBufferCreateInfo bInfo = {};
            bInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
            bInfo.size = frame.vertexBufferSize;
            frame.vertexBuffer = SDL_CreateGPUBuffer(device, &bInfo);
            if (!frame.vertexBuffer) return false;
        }

        // 索引缓冲区 (Per Frame)
        if (frame.indexBufferSize < iSize)
        {
            if (frame.indexBuffer) SDL_ReleaseGPUBuffer(device, frame.indexBuffer);
            frame.indexBufferSize = iSize > frame.indexBufferSize * 2 ? iSize : frame.indexBufferSize * 2;

            SDL_GPUBufferCreateInfo bInfo = {};
            bInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
            bInfo.size = frame.indexBufferSize;
            frame.indexBuffer = SDL_CreateGPUBuffer(device, &bInfo);
            if (!frame.indexBuffer) return false;
        }
        return true;
    }

    DeviceManager& m_deviceManager;
    PipelineCache& m_pipelineCache;

    // 帧资源池
    FrameResource m_frameResources[MAX_FRAMES_IN_FLIGHT];
    uint32_t m_frameIndex = 0;

    SDL_GPUTransferBuffer* m_transferBuffer = nullptr;
    uint32_t m_transferBufferSize = 0;
};

} // namespace ui::managers
