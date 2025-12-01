/**
 * ************************************************************************
 *
 * @file Dialog.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-01
 * @version 0.1
 * @brief  对话框组件定义
  模拟 Qt 的对话框组件 QDialog
  支持模态和非模态对话框
  支持设置标题和内容
  支持显示和隐藏对话框
  基于ImGui实现对话框渲染
  支持自定义按钮和回调函数
  支持设置对话框大小和位置
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include "Widget.h"
#include <string>
#include <functional>
#include <imgui.h>