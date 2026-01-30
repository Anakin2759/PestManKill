/**
 * ************************************************************************
 *
 * @file RenderSystem.cpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-30
 * @version 0.2
 * @brief SDL GPU 渲染系统实现 - 重构版
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#include "RenderSystem.hpp"
#include "../renderers/ShapeRenderer.hpp"
#include "../renderers/TextRenderer.hpp"
#include "../renderers/IconRenderer.hpp"
#include "../renderers/ScrollBarRenderer.hpp"
#include "../managers/IconManager.hpp"

namespace ui::systems
{

RenderSystem::RenderSystem()
    : m_deviceManager(std::make_unique<managers::DeviceManager>()),
      m_fontManager(std::make_unique<managers::FontManager>()), m_pipelineCache(nullptr), m_textTextureCache(nullptr),
      m_batchManager(std::make_unique<managers::BatchManager>()), m_commandBuffer(nullptr)
{
    m_stats.frameCount = 0;
    m_stats.batchCount = 0;
    m_stats.vertexCount = 0;
}

RenderSystem::~RenderSystem()
{
    Logger::info("[RenderSystem] 析构开始");
    cleanup();
    Logger::info("[RenderSystem] 析构完成");
}

RenderSystem::RenderSystem(RenderSystem&& other) noexcept
    : m_deviceManager(std::move(other.m_deviceManager)), m_fontManager(std::move(other.m_fontManager)),
      m_pipelineCache(std::move(other.m_pipelineCache)), m_textTextureCache(std::move(other.m_textTextureCache)),
      m_batchManager(std::move(other.m_batchManager)), m_commandBuffer(std::move(other.m_commandBuffer)),
      m_renderers(std::move(other.m_renderers)), m_stats(other.m_stats), m_whiteTexture(other.m_whiteTexture),
      m_screenWidth(other.m_screenWidth), m_screenHeight(other.m_screenHeight)
{
    Logger::info("[RenderSystem] 移动构造完成");
    other.m_whiteTexture = nullptr;
}

RenderSystem& RenderSystem::operator=(RenderSystem&& other) noexcept
{
    if (this != &other)
    {
        Logger::info("[RenderSystem] 移动赋值开始");
        cleanup();

        m_deviceManager = std::move(other.m_deviceManager);
        m_fontManager = std::move(other.m_fontManager);
        m_pipelineCache = std::move(other.m_pipelineCache);
        m_textTextureCache = std::move(other.m_textTextureCache);
        m_batchManager = std::move(other.m_batchManager);
        m_commandBuffer = std::move(other.m_commandBuffer);
        m_renderers = std::move(other.m_renderers);
        m_stats = other.m_stats;
        m_whiteTexture = other.m_whiteTexture;
        m_screenWidth = other.m_screenWidth;
        m_screenHeight = other.m_screenHeight;

        other.m_whiteTexture = nullptr;
        Logger::info("[RenderSystem] 移动赋值完成");
    }
    return *this;
}

void RenderSystem::onWindowsGraphicsContextSet(const events::WindowGraphicsContextSetEvent& event)
{
    Logger::info("[RenderSystem] 收到窗口图形上下文设置事件，实体ID: {}", static_cast<uint32_t>(event.entity));
    ensureInitialized();
    uint32_t windowID = Registry::Get<components::Window>(event.entity).windowID;
    SDL_Window* sdlWindow = SDL_GetWindowFromID(windowID);
    if (sdlWindow == nullptr)
    {
        Logger::warn("[RenderSystem] 无法获取 SDL_Window (ID: {})", windowID);
        return;
    }

    if (!m_deviceManager->claimWindow(sdlWindow))
    {
        Logger::error("[RenderSystem] 无法声明窗口 (ID: {})", windowID);
        return;
    }

    m_pipelineCache->createPipeline(sdlWindow);
    Logger::info("[RenderSystem] 窗口图形上下文设置完成 (Entity: {})", static_cast<uint32_t>(event.entity));
}

void RenderSystem::onWindowsGraphicsContextUnset(const events::WindowGraphicsContextUnsetEvent& event)
{
    if (auto* windowComp = Registry::TryGet<components::Window>(event.entity))
    {
        SDL_Window* sdlWindow = SDL_GetWindowFromID(windowComp->windowID);
        if (sdlWindow != nullptr)
        {
            m_deviceManager->unclaimWindow(sdlWindow);
            Logger::info("已从 GPU 设备释放窗口 (ID: {})", windowComp->windowID);
        }
    }
}

void RenderSystem::cleanup()
{
    Logger::info("[RenderSystem] cleanup() 开始");

    if (!m_deviceManager)
    {
        Logger::info("[RenderSystem] m_deviceManager 为空，跳过 cleanup");
        return;
    }

    SDL_GPUDevice* device = m_deviceManager->getDevice();
    if (device == nullptr)
    {
        Logger::info("[RenderSystem] GPU 设备为空，跳过 cleanup");
        return;
    }

    Logger::info("[RenderSystem] 等待 GPU 空闲...");
    SDL_WaitForGPUIdle(device);

    if (m_textTextureCache)
    {
        Logger::info("[RenderSystem] 清理文本纹理缓存");
        m_textTextureCache->clear();
    }

    if (m_whiteTexture != nullptr)
    {
        Logger::info("[RenderSystem] 释放白色纹理");
        SDL_ReleaseGPUTexture(device, m_whiteTexture);
        m_whiteTexture = nullptr;
    }

    Logger::info("[RenderSystem] 清理渲染器");
    m_renderers.clear();
    m_commandBuffer.reset();
    m_batchManager.reset();
    m_pipelineCache.reset();
    m_textTextureCache.reset();
    m_fontManager.reset();

    Logger::info("[RenderSystem] 清理设备管理器");
    m_deviceManager->cleanup();
    Logger::info("[RenderSystem] cleanup() 完成");
}

void RenderSystem::createWhiteTexture()
{
    SDL_GPUDevice* device = m_deviceManager->getDevice();
    if (device == nullptr) return;

    SDL_GPUTextureCreateInfo texInfo = {};
    texInfo.type = SDL_GPU_TEXTURETYPE_2D;
    texInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texInfo.width = 1;
    texInfo.height = 1;
    texInfo.layer_count_or_depth = 1;
    texInfo.num_levels = 1;
    texInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

    m_whiteTexture = SDL_CreateGPUTexture(device, &texInfo);
    if (m_whiteTexture == nullptr) return;

    uint32_t whitePixel = 0xFFFFFFFF;
    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = sizeof(whitePixel);

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &transferInfo);

    void* data = SDL_MapGPUTransferBuffer(device, transfer, false);
    SDL_memcpy(data, &whitePixel, sizeof(whitePixel));
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
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
    SDL_ReleaseGPUTransferBuffer(device, transfer);
}

void RenderSystem::update() noexcept
{
    static bool firstUpdate = true;
    auto windowView = Registry::View<components::Window, components::RenderDirtyTag>();

    if (windowView.begin() == windowView.end())
    {
        return;
    }

    if (firstUpdate)
    {
        Logger::info("[RenderSystem] update first call");
        firstUpdate = false;
    }

    ensureInitialized();

    SDL_GPUDevice* device = m_deviceManager->getDevice();
    if (device == nullptr)
    {
        Logger::warn("GPU device not ready");
        return;
    }

    if (m_pipelineCache == nullptr || m_pipelineCache->getPipeline() == nullptr)
    {
        Logger::warn("Pipeline not ready");
        return;
    }

    if (m_whiteTexture == nullptr)
    {
        createWhiteTexture();
    }

    m_stats.frameCount++;
    m_stats.batchCount = 0;
    m_stats.vertexCount = 0;

    for (auto windowEntity : windowView)
    {
        auto& windowComp = windowView.get<components::Window>(windowEntity);
        SDL_Window* sdlWindow = SDL_GetWindowFromID(windowComp.windowID);
        if (sdlWindow == nullptr)
        {
            Logger::warn("窗口实体的 sdlWindow 为空");
            continue;
        }

        int width = 0;
        int height = 0;
        SDL_GetWindowSizeInPixels(sdlWindow, &width, &height);
        if (width <= 0 || height <= 0) continue;

        m_screenWidth = static_cast<float>(width);
        m_screenHeight = static_cast<float>(height);

        m_batchManager->clear();

        if (Registry::AnyOf<components::VisibleTag>(windowEntity))
        {
            core::RenderContext rootContext;
            rootContext.screenWidth = m_screenWidth;
            rootContext.screenHeight = m_screenHeight;
            rootContext.deviceManager = m_deviceManager.get();
            rootContext.fontManager = m_fontManager.get();
            rootContext.textTextureCache = m_textTextureCache.get();
            rootContext.batchManager = m_batchManager.get();
            rootContext.sdlWindow = sdlWindow;
            rootContext.whiteTexture = m_whiteTexture;

            Eigen::Vector2f rootOffset = Eigen::Vector2f(0, 0);
            if (const auto* pos = Registry::TryGet<components::Position>(windowEntity))
            {
                rootOffset = -pos->value;
            }

            rootContext.position = rootOffset;
            rootContext.alpha = 1.0F;

            collectRenderData(windowEntity, rootContext);
        }

        m_batchManager->optimize();

        const auto& batches = m_batchManager->getBatches();
        if (!batches.empty())
        {
            m_commandBuffer->execute(sdlWindow, width, height, batches);
            m_stats.batchCount += static_cast<uint32_t>(batches.size());
            m_stats.vertexCount += static_cast<uint32_t>(m_batchManager->getTotalVertexCount());
        }
    }

    auto dirtyView = Registry::View<components::RenderDirtyTag>();
    for (auto entity : dirtyView)
    {
        Registry::Remove<components::RenderDirtyTag>(entity);
    }
}

void RenderSystem::ensureInitialized()
{
    Logger::info("[RenderSystem] ensureInitialized called");
    if ((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) == 0)
    {
        Logger::warn("[RenderSystem] SDL_INIT_VIDEO not initialized");
        return;
    }

    Logger::info("[RenderSystem] Initiating DeviceManager...");
    if (!m_deviceManager->initialize())
    {
        Logger::error("Failed to initialize RenderSystem: GPU device initialization failed");
        return;
    }
    Logger::info("[RenderSystem] DeviceManager initialized");

    if (m_pipelineCache == nullptr)
    {
        m_pipelineCache = std::make_unique<managers::PipelineCache>(*m_deviceManager);
        m_pipelineCache->loadShaders();
    }

    if (!m_fontManager->isLoaded())
    {
        auto filesystem = cmrc::ui_fonts::get_filesystem();
        const char* fontPath = "assets/fonts/NotoSansSC-VariableFont_wght.ttf";
        if (filesystem.exists(fontPath))
        {
            auto fontFile = filesystem.open(fontPath);
            m_fontManager->loadFromMemory(
                reinterpret_cast<const uint8_t*>(fontFile.begin()), static_cast<size_t>(fontFile.size()), 24.0F, 2.0F);
        }
    }

    if (m_textTextureCache == nullptr)
    {
        m_textTextureCache = std::make_unique<managers::TextTextureCache>(*m_deviceManager, *m_fontManager);
    }

    if (m_commandBuffer == nullptr)
    {
        m_commandBuffer = std::make_unique<managers::CommandBuffer>(*m_deviceManager, *m_pipelineCache);
    }

    if (m_renderers.empty())
    {
        initializeRenderers();
    }
}

void RenderSystem::initializeRenderers()
{
    // 按优先级顺序添加渲染器
    // 优先级小的先渲染（背景 -> 文本 -> 图标 -> 滚动条）

    m_renderers.push_back(std::make_unique<renderers::ShapeRenderer>());
    m_renderers.push_back(std::make_unique<renderers::TextRenderer>());

    // TODO: IconManager 需要初始化，暂时注释掉
    // static managers::IconManager iconManager;
    // m_renderers.push_back(std::make_unique<renderers::IconRenderer>(iconManager));

    m_renderers.push_back(std::make_unique<renderers::ScrollBarRenderer>());

    // 按优先级排序
    std::sort(m_renderers.begin(),
              m_renderers.end(),
              [](const std::unique_ptr<core::IRenderer>& a, const std::unique_ptr<core::IRenderer>& b)
              { return a->getPriority() < b->getPriority(); });

    Logger::info("[RenderSystem] 初始化了 {} 个渲染器", m_renderers.size());
}

void RenderSystem::collectRenderData(entt::entity entity, core::RenderContext& context)
{
    if (!Registry::AnyOf<components::VisibleTag>(entity)) return;
    if (Registry::AnyOf<components::SpacerTag>(entity)) return;

    const auto& pos = Registry::Get<components::Position>(entity);
    const auto& size = Registry::Get<components::Size>(entity);
    const auto* alphaComp = Registry::TryGet<components::Alpha>(entity);

    float globalAlpha = context.alpha * (alphaComp ? alphaComp->value : 1.0F);
    Eigen::Vector2f absolutePos = context.position + pos.value;
    Eigen::Vector2f contentOffset(0.0F, 0.0F);

    // 更新上下文
    core::RenderContext entityContext = context;
    entityContext.position = absolutePos;
    entityContext.size = size.size;
    entityContext.alpha = globalAlpha;

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

        entityContext.pushScissor(currentScissor);
        pushScissor = true;
        contentOffset = -scrollArea->scrollOffset;
    }

    // 使用渲染器收集数据
    for (auto& renderer : m_renderers)
    {
        if (renderer->canHandle(entity))
        {
            renderer->collect(entity, entityContext);
        }
    }

    // 递归处理子元素
    const auto* hierarchy = Registry::TryGet<components::Hierarchy>(entity);
    if (hierarchy && !hierarchy->children.empty())
    {
        for (entt::entity child : hierarchy->children)
        {
            core::RenderContext childContext = entityContext;
            childContext.position = absolutePos + contentOffset;
            collectRenderData(child, childContext);
        }
    }

    if (pushScissor)
    {
        entityContext.popScissor();
    }
}

} // namespace ui::systems
