/**
 * ************************************************************************
 *
 * @file TextTextureCache.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-30
 * @version 0.1
 * @brief 字体文本纹理缓存管理器
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <unordered_map>
#include <string>
#include <SDL3/SDL_gpu.h>
#include "DeviceManager.hpp"
#include "FontManager.hpp"
#include "../common/RenderTypes.hpp"

namespace ui::managers
{

class TextTextureCache
{
public:
    TextTextureCache(ui::managers::DeviceManager& deviceManager, ui::managers::FontManager& fontManager)
        : m_deviceManager(deviceManager), m_fontManager(fontManager)
    {
    }

    ~TextTextureCache() { clear(); }

    void clear()
    {
        SDL_GPUDevice* device = m_deviceManager.getDevice();
        if (device == nullptr) return;

        for (auto& [key, entry] : m_cache)
        {
            if (entry.texture != nullptr)
            {
                SDL_ReleaseGPUTexture(device, entry.texture);
            }
        }
        m_cache.clear();
    }

    SDL_GPUTexture*
        getOrUpload(const std::string& text, const Eigen::Vector4f& color, uint32_t& outWidth, uint32_t& outHeight)
    {
        SDL_GPUDevice* device = m_deviceManager.getDevice();
        if (device == nullptr || !m_fontManager.isLoaded()) return nullptr;

        std::string cacheKey = text + "_" + std::to_string(color.x()) + "_" + std::to_string(color.y()) + "_" +
                               std::to_string(color.z()) + "_" + std::to_string(color.w());

        auto it = m_cache.find(cacheKey);
        if (it != m_cache.end())
        {
            outWidth = it->second.width;
            outHeight = it->second.height;
            return it->second.texture;
        }

        int bitmapWidth = 0;
        int bitmapHeight = 0;

        std::vector<uint8_t> bitmap = m_fontManager.renderTextBitmap(text,
                                                                     static_cast<uint8_t>(color.x() * 255),
                                                                     static_cast<uint8_t>(color.y() * 255),
                                                                     static_cast<uint8_t>(color.z() * 255),
                                                                     static_cast<uint8_t>(color.w() * 255),
                                                                     bitmapWidth,
                                                                     bitmapHeight);

        if (bitmap.empty() || bitmapWidth == 0 || bitmapHeight == 0)
        {
            return nullptr;
        }

        SDL_GPUTextureCreateInfo textureInfo = {};
        textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
        textureInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        textureInfo.width = static_cast<uint32_t>(bitmapWidth);
        textureInfo.height = static_cast<uint32_t>(bitmapHeight);
        textureInfo.layer_count_or_depth = 1;
        textureInfo.num_levels = 1;
        textureInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

        SDL_GPUTexture* texture = SDL_CreateGPUTexture(device, &textureInfo);
        if (texture == nullptr) return nullptr;

        SDL_GPUTransferBufferCreateInfo transferInfo = {};
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferInfo.size = static_cast<uint32_t>(bitmap.size());

        SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);

        if (transferBuffer != nullptr)
        {
            void* data = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
            SDL_memcpy(data, bitmap.data(), bitmap.size());
            SDL_UnmapGPUTransferBuffer(device, transferBuffer);

            SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
            SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);

            SDL_GPUTextureTransferInfo srcInfo = {};
            srcInfo.transfer_buffer = transferBuffer;
            srcInfo.pixels_per_row = textureInfo.width;
            srcInfo.rows_per_layer = textureInfo.height;

            SDL_GPUTextureRegion dstRegion = {};
            dstRegion.texture = texture;
            dstRegion.w = textureInfo.width;
            dstRegion.h = textureInfo.height;
            dstRegion.d = 1;

            SDL_UploadToGPUTexture(copyPass, &srcInfo, &dstRegion, false);
            SDL_EndGPUCopyPass(copyPass);
            SDL_SubmitGPUCommandBuffer(cmd);
            SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
        }

        m_cache[cacheKey] = {texture, textureInfo.width, textureInfo.height};

        outWidth = textureInfo.width;
        outHeight = textureInfo.height;

        return texture;
    }

private:
    DeviceManager& m_deviceManager;
    FontManager& m_fontManager;
    std::unordered_map<std::string, ui::render::CachedTexture> m_cache;
};

} // namespace ui::managers
