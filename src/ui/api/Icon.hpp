#pragma once
#include <entt/entt.hpp>
#include <string>
#include "../common/Policies.hpp"
#include "../common/Components.hpp"

namespace ui::icon
{
/**
 * @brief 设置图标组件
 * @param entity  实体
 * @param textureId 纹理ID
 * @param position 图标位置
 * @param iconSize 图标大小
 * @param spacing 图标与文本间距
 */
void SetIcon(entt::entity entity,
             const std::string& textureId,
             policies::IconFlag iconflag = policies::IconFlag::Default,
             float iconSize = 16.0F,
             float spacing = 4.0F);
/**
 * @brief 设置字体图标组件
 * @param entity 实体
 * @param fontName 字体库名称
 * @param codepoint Unicode 码点
 * @param position 图标位置
 * @param iconSize 图标大小
 * @param spacing 图标与文本间距
 */
void SetIcon(entt::entity entity,
             const std::string& fontName,
             uint32_t codepoint,
             policies::IconFlag iconflag = policies::IconFlag::Default,
             float iconSize = 16.0F,
             float spacing = 4.0F);

void RemoveIcon(entt::entity entity);
} // namespace ui::icon