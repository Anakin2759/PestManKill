/**
 * ************************************************************************
 *
 * @file test_MainWindow.cpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-23 (Updated)
 * @version 0.2
 * @brief UI 模块单元与集成测试（更新以适配新的 Application 接口）
 *
 * 说明：Application 当前已精简，不再提供可覆盖的 setup 接口。
 * 因此测试直接使用 `utils::Registry`、`ui::factory` 与 `ui::helper` 来构建和验证 UI 实体。
 *
 * ************************************************************************
 */
#include <entt/entt.hpp>
#include <gtest/gtest.h>

#include "src/utils/utils.h"
#include "src/ui/core/Helper.h"
#include "src/ui/core/Factory.h"
#include "src/ui/common/Components.h"
#include "src/ui/common/Tags.h"

namespace ui::tests
{

} // namespace ui::tests
