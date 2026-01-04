/**
 * ************************************************************************
 *
 * @file main.cpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.2
 * @brief 客户端主程序（集成 UI 和 Net 模块）
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#include <iostream>
#include <memory>
#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

// 工具
#include <utils.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "src/utils/Functions.h"

#include "src/ui/core/Application.h"

int main(int argc, char* argv[])
{
    utils::functions::setConsoleToUTF8();
    try
    {
        // 创建并运行 UI 应用程序
        ui::Application app("PestManKill Client", 1024, 768);
        app.exec();
    }
    catch (const std::exception& e)
    {
        std::cerr << "应用程序异常终止: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}