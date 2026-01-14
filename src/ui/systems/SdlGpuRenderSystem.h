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

  以后用来替换RenderSystem，支持SDL GPU渲染,为后续移除imGui SDL Renderer做准备

  支持的图形后端列表
    - Vulkan 优先
    - DX12

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

class SdlGpuRenderSystem : public ui::interface::EnableRegister<SdlGpuRenderSystem>
{
public:
};