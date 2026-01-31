/**
 * ************************************************************************
 *
 * @file IconManager.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-29
 * @version 0.1
 * @brief 加载 ttf格式图标文件和管理图标资源
 *
 * 支持：
 * - 加载 TTF格式 图标文件 作为默认图标
 * - 解析 codepoints 映射文件（JSON 或 TXT 格式）
 * - 通过图标名称获取 Unicode 码点
 * - 管理多个 IconFont 图标库
    - 默认加载ui/assets/icons/*.ttf和codepoints文件
    使用stb_truetype进行字体渲染不再引入stbttf库
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include <unordered_map>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <stb_truetype.h>
#include <SDL3/SDL.h>
#include <Eigen/Core>
#include "../singleton/Logger.hpp"

namespace ui::managers
{

class DeviceManager;

struct FontData
{
    std::vector<unsigned char> buffer;
    stbtt_fontinfo info;
    int fontSize;
};

struct TextureInfo
{
    SDL_GPUTexture* texture;
    Eigen::Vector2f uvMin;
    Eigen::Vector2f uvMax;
    float width;
    float height;
};

/**
 * @brief IconFont 管理器
 *
 * 负责加载和管理 IconFont 字体及其 codepoints 映射
 *
 * 使用示例：
 * @code
 * IconManager::LoadIconFont("default", "assets/fonts/iconfont.ttf", "assets/fonts/codepoints.txt", 16);
 * uint32_t homeIcon = IconManager::GetCodepoint("default", "home");
 * auto* font = IconManager::GetFont("default");
 * @endcode
 */
class IconManager
{
public:
    IconManager(DeviceManager& deviceManager) : m_deviceManager(deviceManager) {}
    ~IconManager() { shutdown(); }

    /**
     * @brief 加载 IconFont 字体和 codepoints 文件
     * @param name 字体库名称（用于后续引用）
     * @param fontPath TTF 字体文件路径
     * @param codepointsPath codepoints 文件路径（支持 .txt 或 .json）
     * @param fontSize 字体大小
     * @return 是否加载成功
     */
    bool loadIconFont(const std::string& name,
                      const std::string& fontPath,
                      const std::string& codepointsPath,
                      int fontSize = 16);

    /**
     * @brief 通过图标名称获取 Unicode 码点
     * @param fontName 字体库名称
     * @param iconName 图标名称（如 "home", "user"）
     * @return Unicode 码点，未找到返回 0
     */
    uint32_t getCodepoint(const std::string& fontName, const std::string& iconName) const;

    /**
     * @brief 获取字体信息
     * @param fontName 字体库名称
     * @return stbtt_fontinfo 指针，未找到返回 nullptr
     */
    stbtt_fontinfo* getFont(const std::string& fontName);

    /**
     * @brief Check if icon exists
     */
    bool hasIcon(const std::string& fontName, const std::string& iconName) const;

    /**
     * @brief Get all icon names in a font
     */
    std::vector<std::string> getIconNames(const std::string& fontName) const;

    /**
     * @brief Unload a specific font
     */
    void unloadIconFont(const std::string& fontName);

    /**
     * @brief Clean up all resources
     */
    void shutdown();

    /**
     * @brief 获取图标纹理信息（字体图标）
     */
    const TextureInfo* getTextureInfo(const std::string& fontName, uint32_t codepoint, float size);

    /**
     * @brief 获取图标纹理信息（普通纹理图标 - 暂未实现完整逻辑，目前仅作为接口）
     */
    const TextureInfo* getTextureInfo(const std::string& textureId) const
    {
        auto it = m_imageTextureCache.find(textureId);
        if (it != m_imageTextureCache.end())
        {
            return &it->second;
        }
        return nullptr;
    }

private:
    /**
     * @brief 解析 codepoints 文件
     */
    std::unordered_map<std::string, uint32_t> parseCodepoints(const std::string& filePath);

    std::unordered_map<std::string, uint32_t> parseCodepointsTXT(std::ifstream& file);

    std::unordered_map<std::string, uint32_t> parseCodepointsJSON(std::ifstream& file);

    DeviceManager& m_deviceManager;
    std::unordered_map<std::string, FontData> m_fonts;
    std::unordered_map<std::string, std::unordered_map<std::string, uint32_t>> m_codepoints;

    // 缓存：键为 "fontName_codepoint_size"
    std::unordered_map<std::string, TextureInfo> m_fontTextureCache;
    // 缓存：键为 textureId
    std::unordered_map<std::string, TextureInfo> m_imageTextureCache;
};

} // namespace ui::managers