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
#include <SDL3_ttf/SDL_ttf.h>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include <utils.h>
#include "src/ui/common/Components.h"
#include "src/ui/common/Events.h"
#include "src/ui/common/Policies.h"
#include "src/ui/common/Tags.h"
#include "src/ui/core/GraphicsContext.h"
#include "src/ui/interface/Isystem.h"

namespace ui::systems
{

class SdlGpuRenderSystem final : public ui::interface::EnableRegister<SdlGpuRenderSystem>
{
public:
    SdlGpuRenderSystem() = default;
    SdlGpuRenderSystem(const SdlGpuRenderSystem&) = delete;
    SdlGpuRenderSystem& operator=(const SdlGpuRenderSystem&) = delete;
    SdlGpuRenderSystem(SdlGpuRenderSystem&&) = default;
    SdlGpuRenderSystem& operator=(SdlGpuRenderSystem&&) = default;
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
        cleanup();
    }

    void update() noexcept
    {
        if (m_renderer == nullptr) return;

        auto& registry = utils::Registry::getInstance();
        int width = 0;
        int height = 0;

        if (m_graphicsContext != nullptr && m_graphicsContext->getWindow() != nullptr)
        {
            SDL_GetWindowSizeInPixels(m_graphicsContext->getWindow(), &width, &height);
        }

        const auto fwid = static_cast<float>(width);
        const auto fhei = static_cast<float>(height);
        auto canvasView = registry.view<components::MainWidgetTag, components::Size>();
        for (auto e : canvasView)
        {
            auto& size = canvasView.get<components::Size>(e);
            size.autoSize = false;
            if (size.size.x() != fwid || size.size.y() != fhei)
            {
                size.size = {fwid, fhei};
                registry.emplace_or_replace<components::LayoutDirtyTag>(e);
            }
        }

        SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
        SDL_RenderClear(m_renderer);

        auto view = registry.view<const components::Position,
                                  const components::Size,
                                  const components::VisibleTag,
                                  const components::Hierarchy>();

        for (auto entity : view)
        {
            const auto& hierarchy = registry.get<const components::Hierarchy>(entity);

            if (hierarchy.parent == entt::null)
            {
                renderEntityRecursive(registry, entity, Vec2{0.0F, 0.0F}, 1.0f);
            }
        }

        SDL_RenderPresent(m_renderer);

        auto dirtyView = registry.view<components::RenderDirtyTag>();
        for (auto entity : dirtyView)
        {
            registry.remove<components::RenderDirtyTag>(entity);
        }
    }

    /**
     * @brief 检查 GPU 是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    /**
     * @brief 获取 GPU 设备
     */
    [[nodiscard]] SDL_GPUDevice* getGpuDevice() const { return m_gpuDevice; }

private:
    /**
     * @brief 递归渲染单个实体及其子实体。
     */
    void renderEntityRecursive(entt::registry& registry,
                               entt::entity entity,
                               const Vec2& parentAbsolutePos,
                               float parentGlobalAlpha)
    {
        if (!registry.any_of<components::VisibleTag>(entity)) return;
        if (registry.any_of<components::SpacerTag>(entity)) return;

        const auto& pos = registry.get<const components::Position>(entity);
        const auto& size = registry.get<const components::Size>(entity);
        const auto* alphaComp = registry.try_get<components::Alpha>(entity);

        const float globalAlpha = parentGlobalAlpha * (alphaComp ? alphaComp->value : 1.0f);

        Vec2 absolutePos{parentAbsolutePos.x() + pos.value.x(), parentAbsolutePos.y() + pos.value.y()};
        Vec2 absoluteEndPos{absolutePos.x() + size.size.x(), absolutePos.y() + size.size.y()};

        if (registry.any_of<components::WindowTag>(entity) || registry.any_of<components::DialogTag>(entity))
        {
            renderWindow(entity, absolutePos, globalAlpha);
            return;
        }

        renderBackground(entity, absolutePos, absoluteEndPos, globalAlpha);

        renderSpecificComponent(registry, entity, absolutePos, {size.size.x(), size.size.y()}, globalAlpha);

        const auto* hierarchy = registry.try_get<components::Hierarchy>(entity);
        if (hierarchy && !hierarchy->children.empty())
        {
            for (entt::entity child : hierarchy->children)
            {
                renderEntityRecursive(registry, child, absolutePos, globalAlpha);
            }
        }
    }

    /**
     * @brief 渲染 ECS 窗口/对话框实体。
     */
    void renderWindow(entt::entity entity, const Vec2& absolutePos, float globalAlpha)
    {
        auto& registry = utils::Registry::getInstance();
        const auto& size = registry.get<const components::Size>(entity);
        const auto* hierarchy = registry.try_get<components::Hierarchy>(entity);

        std::string title;
        bool hasTitleBar = true;
        bool noResize = false;
        bool noMove = false;
        bool noCollapse = true;

        if (const auto* windowComp = registry.try_get<components::Window>(entity))
        {
            title = windowComp->title;
            hasTitleBar = windowComp->hasTitleBar;
            noResize = windowComp->noResize;
            noMove = windowComp->noMove;
            noCollapse = windowComp->noCollapse;
        }
        else if (const auto* dialogComp = registry.try_get<components::Dialog>(entity))
        {
            title = dialogComp->title;
            hasTitleBar = dialogComp->hasTitleBar;
            noResize = dialogComp->noResize;
            noMove = dialogComp->noMove;
            noCollapse = true;
        }

        Vec2 absEndPos{absolutePos.x() + size.size.x(), absolutePos.y() + size.size.y()};
        renderBackground(entity, absolutePos, absEndPos, globalAlpha);

        if (hierarchy && !hierarchy->children.empty())
        {
            for (entt::entity child : hierarchy->children)
            {
                renderEntityRecursive(registry, child, absolutePos, globalAlpha);
            }
        }
    }

    void renderBackground(entt::entity entity, const Vec2& startPos, const Vec2& endPos, float globalAlpha)
    {
        auto& registry = utils::Registry::getInstance();

        const auto* bg = registry.try_get<components::Background>(entity);
        const auto* shadow = registry.try_get<components::Shadow>(entity);
        if (shadow && shadow->enabled)
        {
            const Color& shadowColor = shadow->color;
            Color finalShadow{shadowColor.red, shadowColor.green, shadowColor.blue, shadowColor.alpha * globalAlpha};
            Vec2 shadowStart{startPos.x() + shadow->offset.x(), startPos.y() + shadow->offset.y()};
            Vec2 shadowEnd{endPos.x() + shadow->offset.x(), endPos.y() + shadow->offset.y()};
            drawRect(shadowStart, shadowEnd, finalShadow);
        }

        if (bg && bg->enabled)
        {
            Color finalColor{bg->color.red, bg->color.green, bg->color.blue, bg->color.alpha * globalAlpha};
            drawRect(startPos, endPos, finalColor);
        }

        const auto* border = registry.try_get<components::Border>(entity);
        if (border && border->enabled && border->thickness > 0.0f)
        {
            Color finalColor{
                border->color.red, border->color.green, border->color.blue, border->color.alpha * globalAlpha};
            drawBorder(startPos, endPos, border->thickness, finalColor);
        }
    }

    void renderSpecificComponent(
        entt::registry& registry, entt::entity entity, const Vec2& pos, const Vec2& size, float globalAlpha)
    {
        if (registry.any_of<components::TextTag>(entity) || registry.any_of<components::ButtonTag>(entity) ||
            registry.any_of<components::LabelTag>(entity))
        {
            const auto* textComp = registry.try_get<components::Text>(entity);
            if (textComp && !textComp->content.empty())
            {
                Color finalColor{textComp->color.red,
                                 textComp->color.green,
                                 textComp->color.blue,
                                 textComp->color.alpha * globalAlpha};

                Vec2 textPos = pos;
                const auto textSize = getTextSize(textComp->content, textComp->fontSize);
                const uint8_t align = static_cast<uint8_t>(textComp->alignment);
                if (align & static_cast<uint8_t>(policies::Alignment::HCENTER))
                {
                    textPos = {pos.x() + (size.x() - textSize.x) * 0.5f, textPos.y()};
                }
                else if (align & static_cast<uint8_t>(policies::Alignment::RIGHT))
                {
                    textPos = {pos.x() + (size.x() - textSize.x), textPos.y()};
                }

                if (align & static_cast<uint8_t>(policies::Alignment::VCENTER))
                {
                    textPos = {textPos.x(), pos.y() + (size.y() - textSize.y) * 0.5f};
                }
                else if (align & static_cast<uint8_t>(policies::Alignment::BOTTOM))
                {
                    textPos = {textPos.x(), pos.y() + (size.y() - textSize.y)};
                }

                drawText(entity, textComp->content, textPos, textComp->fontSize, finalColor);
            }
        }

        if (registry.any_of<components::ImageTag>(entity))
        {
            const auto& imageComp = registry.get<const components::Image>(entity);
            if (imageComp.textureId)
            {
                SDL_Texture* texture = static_cast<SDL_Texture*>(imageComp.textureId);
                if (texture != nullptr)
                {
                    SDL_FRect dst{pos.x(), pos.y(), size.x(), size.y()};
                    SDL_FRect src{};
                    int tw = 0;
                    int th = 0;
                    if (SDL_QueryTexture(texture, nullptr, nullptr, &tw, &th) == 0)
                    {
                        src.x = imageComp.uvMin.x() * static_cast<float>(tw);
                        src.y = imageComp.uvMin.y() * static_cast<float>(th);
                        src.w = (imageComp.uvMax.x() - imageComp.uvMin.x()) * static_cast<float>(tw);
                        src.h = (imageComp.uvMax.y() - imageComp.uvMin.y()) * static_cast<float>(th);
                    }
                    SDL_SetTextureColorMod(texture,
                                           static_cast<Uint8>(imageComp.tintColor.red * 255.0f),
                                           static_cast<Uint8>(imageComp.tintColor.green * 255.0f),
                                           static_cast<Uint8>(imageComp.tintColor.blue * 255.0f));
                    SDL_SetTextureAlphaMod(texture,
                                           static_cast<Uint8>(imageComp.tintColor.alpha * globalAlpha * 255.0f));
                    SDL_RenderTexture(m_renderer, texture, &src, &dst);
                }
            }
        }

        if (registry.any_of<components::TextEditTag>(entity))
        {
            const auto& textEditComp = registry.get<const components::TextEdit>(entity);
            Vec2 endPos{pos.x() + size.x(), pos.y() + size.y()};

            drawRect(pos, endPos, Color{0.1f, 0.1f, 0.1f, 0.8f * globalAlpha});
            drawBorder(pos, endPos, 1.0f, Color{0.0f, 0.0f, 0.0f, globalAlpha});

            const std::string& content = textEditComp.buffer.empty() ? textEditComp.placeholder : textEditComp.buffer;
            Color finalColor{textEditComp.textColor.red,
                             textEditComp.textColor.green,
                             textEditComp.textColor.blue,
                             textEditComp.textColor.alpha * globalAlpha};
            Vec2 textPos{pos.x() + 5.0f, pos.y() + 5.0f};
            drawText(entity, content, textPos, 0.0f, finalColor);
        }

        if (registry.any_of<components::ArrowTag>(entity))
        {
            const auto& arrowComp = registry.get<const components::Arrow>(entity);
            Color finalColor = arrowComp.color;
            finalColor.blue *= globalAlpha;
            SDL_SetRenderDrawColor(m_renderer,
                                   static_cast<Uint8>(finalColor.red * 255.0f),
                                   static_cast<Uint8>(finalColor.green * 255.0f),
                                   static_cast<Uint8>(finalColor.blue * 255.0f),
                                   static_cast<Uint8>(finalColor.alpha * 255.0f));
            SDL_RenderLine(m_renderer,
                           arrowComp.startPoint.x(),
                           arrowComp.startPoint.y(),
                           arrowComp.endPoint.x(),
                           arrowComp.endPoint.y());
        }
    }

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

        m_renderer = SDL_CreateGPURenderer(m_gpuDevice, m_window);
        if (m_renderer == nullptr)
        {
            LOG_ERROR("[SdlGpuRenderSystem] Failed to create GPU renderer: {}", SDL_GetError());
            SDL_ReleaseWindowFromGPUDevice(m_gpuDevice, m_window);
            SDL_DestroyGPUDevice(m_gpuDevice);
            m_gpuDevice = nullptr;
            return false;
        }

        if (!initTtf())
        {
            LOG_WARN("[SdlGpuRenderSystem] SDL_ttf init failed: {}", TTF_GetError());
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
        clearTextCache();

        if (m_defaultFont != nullptr)
        {
            TTF_CloseFont(m_defaultFont);
            m_defaultFont = nullptr;
        }
        if (m_ttfInitialized)
        {
            TTF_Quit();
            m_ttfInitialized = false;
        }

        if (m_renderer != nullptr)
        {
            SDL_DestroyRenderer(m_renderer);
            m_renderer = nullptr;
        }
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

    void drawRect(const Vec2& startPos, const Vec2& endPos, const Color& color)
    {
        SDL_FRect rect{startPos.x(), startPos.y(), endPos.x() - startPos.x(), endPos.y() - startPos.y()};
        SDL_SetRenderDrawColor(m_renderer,
                               static_cast<Uint8>(color.red * 255.0f),
                               static_cast<Uint8>(color.green * 255.0f),
                               static_cast<Uint8>(color.blue * 255.0f),
                               static_cast<Uint8>(color.alpha * 255.0f));
        SDL_RenderFillRect(m_renderer, &rect);
    }

    void drawBorder(const Vec2& startPos, const Vec2& endPos, float thickness, const Color& color)
    {
        if (thickness <= 0.0f) return;
        SDL_SetRenderDrawColor(m_renderer,
                               static_cast<Uint8>(color.red * 255.0f),
                               static_cast<Uint8>(color.green * 255.0f),
                               static_cast<Uint8>(color.blue * 255.0f),
                               static_cast<Uint8>(color.alpha * 255.0f));

        SDL_FRect top{startPos.x(), startPos.y(), endPos.x() - startPos.x(), thickness};
        SDL_FRect bottom{startPos.x(), endPos.y() - thickness, endPos.x() - startPos.x(), thickness};
        SDL_FRect left{startPos.x(), startPos.y(), thickness, endPos.y() - startPos.y()};
        SDL_FRect right{endPos.x() - thickness, startPos.y(), thickness, endPos.y() - startPos.y()};

        SDL_RenderFillRect(m_renderer, &top);
        SDL_RenderFillRect(m_renderer, &bottom);
        SDL_RenderFillRect(m_renderer, &left);
        SDL_RenderFillRect(m_renderer, &right);
    }

    bool initTtf()
    {
        if (m_ttfInitialized) return true;
        if (TTF_Init() != 0)
        {
            return false;
        }
        m_ttfInitialized = true;

        const std::vector<std::string> fontPaths = {"assets/fonts/NotoSansSC-VariableFont_wght.ttf",
                                                    "../assets/fonts/NotoSansSC-VariableFont_wght.ttf",
                                                    "C:/Windows/Fonts/msyh.ttc",
                                                    "C:/Windows/Fonts/simhei.ttf",
                                                    "C:/Windows/Fonts/simsun.ttc"};

        for (const auto& path : fontPaths)
        {
            if (std::filesystem::exists(path))
            {
                m_defaultFont = TTF_OpenFont(path.c_str(), 18);
                if (m_defaultFont != nullptr)
                {
                    LOG_INFO("[SdlGpuRenderSystem] Loaded font: {}", path);
                    break;
                }
            }
        }

        return m_defaultFont != nullptr;
    }

    Vec2 getTextSize(const std::string& text, float fontSize)
    {
        if (!m_ttfInitialized || m_defaultFont == nullptr || text.empty()) return {0.0F, 0.0F};
        int w = 0;
        int h = 0;
        TTF_Font* font = getFontForSize(fontSize);
        if (font == nullptr) return {0.0F, 0.0F};
        if (TTF_SizeUTF8(font, text.c_str(), &w, &h) == 0)
        {
            return {static_cast<float>(w), static_cast<float>(h)};
        }
        return {0.0F, 0.0F};
    }

    void drawText(entt::entity entity, const std::string& text, const Vec2& pos, float fontSize, const Color& color)
    {
        if (!m_ttfInitialized || m_defaultFont == nullptr || text.empty()) return;

        TextCacheEntry* cache = nullptr;
        auto it = m_textCache.find(entity);
        if (it != m_textCache.end())
        {
            cache = &it->second;
            if (cache->content != text || cache->fontSize != fontSize || cache->color != color)
            {
                destroyTextCacheEntry(*cache);
                cache = nullptr;
            }
        }

        if (cache == nullptr)
        {
            TextCacheEntry entry{};
            entry.content = text;
            entry.fontSize = fontSize;
            entry.color = color;

            TTF_Font* font = getFontForSize(fontSize);
            if (font == nullptr) return;

            SDL_Color sdlColor{static_cast<Uint8>(color.red * 255.0f),
                               static_cast<Uint8>(color.green * 255.0f),
                               static_cast<Uint8>(color.blue * 255.0f),
                               static_cast<Uint8>(color.alpha * 255.0f)};
            SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), sdlColor);
            if (surface == nullptr) return;

            SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, surface);
            entry.w = surface->w;
            entry.h = surface->h;
            SDL_DestroySurface(surface);

            if (texture == nullptr) return;
            entry.texture = texture;
            m_textCache.emplace(entity, entry);
            cache = &m_textCache[entity];
        }

        if (cache->texture != nullptr)
        {
            SDL_FRect dst{pos.x(), pos.y(), static_cast<float>(cache->w), static_cast<float>(cache->h)};
            SDL_RenderTexture(m_renderer, cache->texture, nullptr, &dst);
        }
    }

    TTF_Font* getFontForSize(float fontSize)
    {
        int size = fontSize > 0.0f ? static_cast<int>(fontSize) : 18;
        auto it = m_fontCache.find(size);
        if (it != m_fontCache.end()) return it->second;
        if (m_defaultFont == nullptr) return nullptr;

        const char* fontPath = TTF_GetFontFaceFamilyName(m_defaultFont);
        (void)fontPath;
        // 复用默认字体文件路径不可用，直接打开系统字体路径
        const std::vector<std::string> fontPaths = {"assets/fonts/NotoSansSC-VariableFont_wght.ttf",
                                                    "../assets/fonts/NotoSansSC-VariableFont_wght.ttf",
                                                    "C:/Windows/Fonts/msyh.ttc",
                                                    "C:/Windows/Fonts/simhei.ttf",
                                                    "C:/Windows/Fonts/simsun.ttc"};
        for (const auto& path : fontPaths)
        {
            if (std::filesystem::exists(path))
            {
                TTF_Font* font = TTF_OpenFont(path.c_str(), size);
                if (font != nullptr)
                {
                    m_fontCache.emplace(size, font);
                    return font;
                }
            }
        }
        return nullptr;
    }

    void destroyTextCacheEntry(TextCacheEntry& entry)
    {
        if (entry.texture != nullptr)
        {
            SDL_DestroyTexture(entry.texture);
            entry.texture = nullptr;
        }
        entry.w = 0;
        entry.h = 0;
        entry.content.clear();
    }

    void clearTextCache()
    {
        for (auto& [entity, entry] : m_textCache)
        {
            destroyTextCacheEntry(entry);
        }
        m_textCache.clear();
        for (auto& [size, font] : m_fontCache)
        {
            if (font != nullptr)
            {
                TTF_CloseFont(font);
            }
        }
        m_fontCache.clear();
    }

    struct TextCacheEntry
    {
        SDL_Texture* texture = nullptr;
        int w = 0;
        int h = 0;
        std::string content;
        float fontSize = 0.0f;
        Color color{1.0F, 1.0F, 1.0F, 1.0F};
    };

    GraphicsContext* m_graphicsContext = nullptr;
    SDL_GPUDevice* m_gpuDevice = nullptr;
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    bool m_initialized = false;
    bool m_ttfInitialized = false;
    TTF_Font* m_defaultFont = nullptr;
    std::unordered_map<entt::entity, TextCacheEntry> m_textCache;
    std::unordered_map<int, TTF_Font*> m_fontCache;
};

} // namespace ui::systems