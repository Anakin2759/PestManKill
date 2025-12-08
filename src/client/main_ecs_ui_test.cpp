/**
 * ************************************************************************
 *
 * @file main_ecs_ui_test.cpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief ECS UI系统测试程序
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#include "src/client/view/MainWindowECS.h"
#include "src/client/utils/Logger.h"
#include <exception>
#define STB_IMAGE_IMPLEMENTATION
int main(int /* argc */, char* /* argv */[])
{
    try
    {
        LOG_INFO("=== Starting ECS UI Test Application ===");

        ui::MainWindowECS app;

        LOG_INFO("Application initialized, entering main loop...");
        app.run();

        LOG_INFO("=== Application terminated successfully ===");
        return 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Fatal error: {}", e.what());
        return 1;
    }
    catch (...)
    {
        LOG_ERROR("Unknown fatal error occurred");
        return 1;
    }
}
