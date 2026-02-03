#include "IconManager.hpp"
#include "DeviceManager.hpp"
#include <SDL3/SDL_gpu.h>
#include <stb_truetype.h>
#include <cstring>
#include <fstream> // 提供 basic_ifstream 的完整定义

namespace ui::managers
{

bool IconManager::loadIconFont(const std::string& name,
                               const std::string& fontPath,
                               const std::string& codepointsPath,
                               int fontSize)
{
    Logger::info("Loading IconFont '{}' from '{}'", name, fontPath);
    // Read file
    std::ifstream file(fontPath, std::ios::binary | std::ios::ate); // NOLINT
    if (!file.is_open())
    {
        Logger::error("Failed to open font file: {}", fontPath);
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> buffer(size);
    if (!file.read((char*)buffer.data(), size))
    {
        Logger::error("Failed to read font file: {}", fontPath);
        return false;
    }

    stbtt_fontinfo info;
    if (stbtt_InitFont(&info, buffer.data(), stbtt_GetFontOffsetForIndex(buffer.data(), 0)) == 0)
    {
        Logger::error("Failed to init font: {}", fontPath);
        return false;
    }

    // 解析 codepoints 文件
    auto codepoints = parseCodepoints(codepointsPath);
    if (codepoints.empty())
    {
        Logger::warn("No codepoints loaded from: {}", codepointsPath);
    }

    // 存储字体和映射
    m_fonts[name] = FontData{.buffer = std::move(buffer), .info = info, .fontSize = fontSize};
    m_codepoints[name] = std::move(codepoints);

    Logger::info("IconFont '{}' loaded: {} icons", name, m_codepoints[name].size());
    return true;
}

bool IconManager::loadIconFontFromMemory(const std::string& name,
                                         const void* fontData,
                                         size_t fontLength,
                                         const void* codepointsData,
                                         size_t codepointsLength,
                                         int fontSize)
{
    if (fontData == nullptr || fontLength == 0) return false;

    // Copy font data because we store it
    std::vector<unsigned char> buffer(fontLength);
    std::memcpy(buffer.data(), fontData, fontLength);

    stbtt_fontinfo info;
    if (stbtt_InitFont(&info, buffer.data(), stbtt_GetFontOffsetForIndex(buffer.data(), 0)) == 0)
    {
        Logger::error("Failed to init font from memory: {}", name);
        return false;
    }

    // Parse codepoints from memory
    std::string codepointsStr(static_cast<const char*>(codepointsData), codepointsLength);
    std::istringstream stream(codepointsStr);

    CodepointMap codepoints;

    // Heuristic to detect JSON vs TXT: check for '{'
    char firstChar = 0;
    while (stream >> std::ws && stream.peek() != EOF)
    {
        firstChar = static_cast<char>(stream.peek());
        break; // Only peek first non-whitespace char
    }
    stream.seekg(0); // reset stream position

    if (firstChar == '{')
    {
        codepoints = parseCodepointsJSON(stream);
    }
    else
    {
        codepoints = parseCodepointsTXT(stream);
    }

    if (codepoints.empty())
    {
        Logger::warn("No codepoints loaded from memory for: {}", name);
    }

    m_fonts[name] = FontData{.buffer = std::move(buffer), .info = info, .fontSize = fontSize};
    m_codepoints[name] = std::move(codepoints);

    Logger::info("IconFont '{}' loaded from memory: {} icons", name, m_codepoints[name].size());
    return true;
}

uint32_t IconManager::getCodepoint(std::string_view fontName, std::string_view iconName) const
{
    if (auto fontIt = m_codepoints.find(fontName); fontIt != m_codepoints.end())
    {
        if (auto iconIt = fontIt->second.find(iconName); iconIt != fontIt->second.end())
        {
            return iconIt->second;
        }
        Logger::warn("Icon '{}' not found in font '{}'", iconName, fontName);
        return 0;
    }

    Logger::warn("IconFont '{}' not found", fontName);
    return 0;
}

stbtt_fontinfo* IconManager::getFont(std::string_view fontName)
{
    if (auto iterator = m_fonts.find(fontName); iterator != m_fonts.end())
    {
        return &iterator->second.info;
    }
    return nullptr;
}

bool IconManager::hasIcon(std::string_view fontName, std::string_view iconName) const
{
    // C++20/23 标准优化：利用透明哈希进行 contains 检查，避免 string 分配
    if (auto it = m_codepoints.find(fontName); it != m_codepoints.end())
    {
        return it->second.contains(iconName);
    }
    return false;
}

std::vector<std::string> IconManager::getIconNames(std::string_view fontName) const
{
    std::vector<std::string> names;
    if (auto it = m_codepoints.find(fontName); it != m_codepoints.end())
    {
        names.reserve(it->second.size());
        for (const auto& [name, codepoint] : it->second)
        {
            names.push_back(name);
        }
    }
    return names;
}

void IconManager::unloadIconFont(std::string_view fontName)
{
    m_fonts.erase(fontName);
    m_codepoints.erase(fontName);
    Logger::info("IconFont '{}' unloaded", fontName);
}

void IconManager::shutdown()
{
    SDL_GPUDevice* device = m_deviceManager->getDevice();
    if (device != nullptr)
    {
        for (auto& [key, info] : m_fontTextureCache)
        {
            if (info.texture != nullptr) SDL_ReleaseGPUTexture(device, info.texture);
        }
        for (auto& [key, info] : m_imageTextureCache)
        {
            if (info.texture != nullptr) SDL_ReleaseGPUTexture(device, info.texture);
        }
    }
    m_fontTextureCache.clear();
    m_imageTextureCache.clear();
    m_fonts.clear();
    m_codepoints.clear();
    Logger::info("IconManager shutdown");
}

const TextureInfo* IconManager::getTextureInfo(std::string_view fontName, uint32_t codepoint, float size)
{
    // 构造 cache key：在频繁调用中仍可能分配内存，但内部 Map 查找已透明化
    std::string cacheKey =
        std::string(fontName) + "_" + std::to_string(codepoint) + "_" + std::to_string(static_cast<int>(size));
    if (auto it = m_fontTextureCache.find(cacheKey); it != m_fontTextureCache.end())
    {
        return &it->second;
    }

    auto fontDataIt = m_fonts.find(fontName);
    if (fontDataIt == m_fonts.end())
    {
        return nullptr;
    }

    stbtt_fontinfo* info = &fontDataIt->second.info;
    float scale = stbtt_ScaleForPixelHeight(info, size);

    int width{};
    int height{};
    int xoff{};
    int yoff{};
    unsigned char* bitmap =
        stbtt_GetCodepointBitmap(info, 0, scale, static_cast<int>(codepoint), &width, &height, &xoff, &yoff);

    if (bitmap == nullptr)
    {
        return nullptr;
    }

    SDL_GPUDevice* device = m_deviceManager->getDevice();
    if (device == nullptr)
    {
        stbtt_FreeBitmap(bitmap, nullptr);
        return nullptr;
    }

    // Convert alpha bitmap to RGBA
    std::vector<uint32_t> rgbaPixels(static_cast<size_t>(width) * static_cast<size_t>(height));
    for (int i = 0; i < width * height; ++i)
    {
        uint8_t alpha = bitmap[i];
        rgbaPixels[i] = (alpha << 24) | 0x00FFFFFF; // White with alpha
    }

    stbtt_FreeBitmap(bitmap, nullptr);

    SDL_GPUTextureCreateInfo texInfo = {};
    texInfo.type = SDL_GPU_TEXTURETYPE_2D;
    texInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texInfo.width = static_cast<uint32_t>(width);
    texInfo.height = static_cast<uint32_t>(height);
    texInfo.layer_count_or_depth = 1;
    texInfo.num_levels = 1;
    texInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

    SDL_GPUTexture* texture = SDL_CreateGPUTexture(device, &texInfo);
    if (texture == nullptr)
    {
        return nullptr;
    }

    // Upload to GPU
    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = static_cast<uint32_t>(rgbaPixels.size() * sizeof(uint32_t));

    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    void* mappedData = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
    SDL_memcpy(mappedData, rgbaPixels.data(), transferInfo.size);
    SDL_UnmapGPUTransferBuffer(device, transferBuffer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTextureTransferInfo srcInfo = {};
    srcInfo.transfer_buffer = transferBuffer;
    srcInfo.pixels_per_row = static_cast<uint32_t>(width);
    srcInfo.rows_per_layer = static_cast<uint32_t>(height);

    SDL_GPUTextureRegion dstRegion = {};
    dstRegion.texture = texture;
    dstRegion.w = static_cast<uint32_t>(width);
    dstRegion.h = static_cast<uint32_t>(height);
    dstRegion.d = 1;

    SDL_UploadToGPUTexture(copyPass, &srcInfo, &dstRegion, false);
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

    TextureInfo texInfoResult{};
    texInfoResult.texture = texture;
    texInfoResult.uvMin = {0.0F, 0.0F};
    texInfoResult.uvMax = {1.0F, 1.0F};
    texInfoResult.width = static_cast<float>(width);
    texInfoResult.height = static_cast<float>(height);

    m_fontTextureCache[cacheKey] = texInfoResult;
    return &m_fontTextureCache[cacheKey];
}

IconManager::CodepointMap IconManager::parseCodepoints(const std::string& filePath)
{
    CodepointMap result;
    std::ifstream file(filePath);

    if (!file.is_open())
    {
        Logger::error("Failed to open codepoints file: {}", filePath);
        return result;
    }

    if (filePath.find(".json") != std::string::npos)
    {
        result = parseCodepointsJSON(file);
    }
    else
    {
        result = parseCodepointsTXT(file);
    }

    file.close();
    return result;
}

IconManager::CodepointMap IconManager::parseCodepointsTXT(std::istream& file)
{
    CodepointMap result;
    std::string line;

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string iconName;
        std::string hexCode;

        if (iss >> iconName >> hexCode)
        {
            try
            {
                uint32_t codepoint = std::stoul(hexCode, nullptr, 16);
                result[iconName] = codepoint;
            }
            catch (...)
            {
                Logger::warn("Invalid codepoint format: {} - {}", iconName, hexCode);
            }
        }
    }

    return result;
}

IconManager::CodepointMap IconManager::parseCodepointsJSON(std::istream& file)
{
    CodepointMap result;
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    size_t pos = 0;
    while (true)
    {
        size_t keyStart = content.find('"', pos);
        if (keyStart == std::string::npos) break;

        size_t keyEnd = content.find('"', keyStart + 1);
        if (keyEnd == std::string::npos) break;

        std::string key = content.substr(keyStart + 1, keyEnd - keyStart - 1);

        size_t valueStart = content.find('"', keyEnd + 1);
        if (valueStart == std::string::npos) break;

        size_t valueEnd = content.find('"', valueStart + 1);
        if (valueEnd == std::string::npos) break;

        std::string value = content.substr(valueStart + 1, valueEnd - valueStart - 1);

        try
        {
            uint32_t codepoint = std::stoul(value, nullptr, 16);
            result[key] = codepoint;
        }
        catch (...)
        {
            Logger::warn("Invalid codepoint in JSON: {} - {}", key, value);
        }

        pos = valueEnd + 1;
    }

    return result;
}

} // namespace ui::managers
