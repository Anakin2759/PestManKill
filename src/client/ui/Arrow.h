/**
 * ************************************************************************
 *
 * @file Arrow.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-02
 * @version 0.1
 * @brief 箭头特效组件定义
    从一个角色的image组件指向另一个角色的image组件 起点终点取二者最小距离线段
    可能实现是一个背景透明的image组件
    通过计算起点终点位置和角度 实现箭头的旋转
    起点位置是image顶部中心点 终点位置是image底部中心点

    - 提供设置箭头Image的方法 setArrowImage
    - 提供设置起点和终点的方法 setStartEnd
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "Widget.h"
#include "src/client/ui/Image.h"
