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
    /**
     * @brief 加载 IconFont 字体和 codepoints 文件
     * @param name 字体库名称（用于后续引用）
     * @param fontPath TTF 字体文件路径
     * @param codepointsPath codepoints 文件路径（支持 .txt 或 .json）
     * @param fontSize 字体大小
     * @return 是否加载成功
     */
    static bool LoadIconFont(const std::string& name,
                             const std::string& fontPath,
                             const std::string& codepointsPath,
                             int fontSize = 16)
    {
        // Read file
        std::ifstream file(fontPath, std::ios::binary | std::ios::ate);
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
        if (!stbtt_InitFont(&info, buffer.data(), stbtt_GetFontOffsetForIndex(buffer.data(), 0)))
        {
            Logger::error("Failed to init font: {}", fontPath);
            return false;
        }

        // 解析 codepoints 文件
        auto codepoints = ParseCodepoints(codepointsPath);
        if (codepoints.empty())
        {
            Logger::warn("No codepoints loaded from: {}", codepointsPath);
        }

        // 存储字体和映射
        s_fonts[name] = FontData{std::move(buffer), info, fontSize};
        s_codepoints[name] = std::move(codepoints);

        Logger::info("IconFont '{}' loaded: {} icons", name, s_codepoints[name].size());
        return true;
    }

    /**
     * @brief 通过图标名称获取 Unicode 码点
     * @param fontName 字体库名称
     * @param iconName 图标名称（如 "home", "user"）
     * @return Unicode 码点，未找到返回 0
     */
    static uint32_t GetCodepoint(const std::string& fontName, const std::string& iconName)
    {
        auto fontIt = s_codepoints.find(fontName);
        if (fontIt == s_codepoints.end())
        {
            Logger::warn("IconFont '{}' not found", fontName);
            return 0;
        }

        auto iconIt = fontIt->second.find(iconName);
        if (iconIt == fontIt->second.end())
        {
            Logger::warn("Icon '{}' not found in font '{}'", iconName, fontName);
            return 0;
        }

        return iconIt->second;
    }

    /**
     * @brief 获取字体信息
     * @param fontName 字体库名称
     * @return stbtt_fontinfo 指针，未找到返回 nullptr
     */
    static stbtt_fontinfo* GetFont(const std::string& fontName)
    {
        auto it = s_fonts.find(fontName);
        if (it == s_fonts.end())
        {
            Logger::warn("Font '{}' not found", fontName);
            return nullptr;
        }
        return &it->second.info;
    }

    /**
     * @brief Check if icon exists
     */
    static bool HasIcon(const std::string& fontName, const std::string& iconName)
    {
        auto fontIt = s_codepoints.find(fontName);
        if (fontIt == s_codepoints.end()) return false;
        return fontIt->second.find(iconName) != fontIt->second.end();
    }

    /**
     * @brief Get all icon names in a font
     */
    static std::vector<std::string> GetIconNames(const std::string& fontName)
    {
        std::vector<std::string> names;
        auto it = s_codepoints.find(fontName);
        if (it != s_codepoints.end())
        {
            names.reserve(it->second.size());
            for (const auto& [name, codepoint] : it->second)
            {
                names.push_back(name);
            }
        }
        return names;
    }

    /**
     * @brief Unload a specific font
     */
    static void UnloadIconFont(const std::string& fontName)
    {
        s_fonts.erase(fontName);
        s_codepoints.erase(fontName);
        Logger::info("IconFont '{}' unloaded", fontName);
    }

    /**
     * @brief Clean up all resources
     */
    static void Shutdown()
    {
        s_fonts.clear();
        s_codepoints.clear();
        Logger::info("IconManager shutdown");
    }

    // Instance method required by IconRenderer
    const TextureInfo* getTextureInfo(const std::string& id) const
    {
        return nullptr; // Not implemented yet
    }

private:
    /**
     * @brief 解析 codepoints 文件
     *
     * 支持格式：
     * 1. TXT 格式：
     *    home e001
     *    user e002
     *
     * 2. JSON 格式：
     *    {"home": "e001", "user": "e002"}
     */
    static std::unordered_map<std::string, uint32_t> ParseCodepoints(const std::string& filePath)
    {
        std::unordered_map<std::string, uint32_t> result;
        std::ifstream file(filePath);

        if (!file.is_open())
        {
            Logger::error("Failed to open codepoints file: {}", filePath);
            return result;
        }

        // 判断文件格式
        if (filePath.ends_with(".json"))
        {
            result = ParseCodepointsJSON(file);
        }
        else
        {
            result = ParseCodepointsTXT(file);
        }

        file.close();
        return result;
    }

    /**
     * @brief 解析 TXT 格式的 codepoints 文件
     * 格式：iconName hexCodepoint
     */
    static std::unordered_map<std::string, uint32_t> ParseCodepointsTXT(std::ifstream& file)
    {
        std::unordered_map<std::string, uint32_t> result;
        std::string line;

        while (std::getline(file, line))
        {
            if (line.empty() || line[0] == '#') continue; // 跳过空行和注释

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

    /**
     * @brief 解析 JSON 格式的 codepoints 文件
     * 简化版 JSON 解析（仅支持 {"name": "code"} 格式）
     */
    static std::unordered_map<std::string, uint32_t> ParseCodepointsJSON(std::ifstream& file)
    {
        std::unordered_map<std::string, uint32_t> result;
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        // 简化的 JSON 解析
        size_t pos = 0;
        while (true)
        {
            // 查找键
            size_t keyStart = content.find('"', pos);
            if (keyStart == std::string::npos) break;

            size_t keyEnd = content.find('"', keyStart + 1);
            if (keyEnd == std::string::npos) break;

            std::string key = content.substr(keyStart + 1, keyEnd - keyStart - 1);

            // 查找值
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

    static std::unordered_map<std::string, FontData> s_fonts;
    static std::unordered_map<std::string, std::unordered_map<std::string, uint32_t>> s_codepoints;
};

// 静态成员初始化
inline std::unordered_map<std::string, FontData> IconManager::s_fonts;
inline std::unordered_map<std::string, std::unordered_map<std::string, uint32_t>> IconManager::s_codepoints;

} // namespace ui::managers