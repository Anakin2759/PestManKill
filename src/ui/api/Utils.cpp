#include "Utils.hpp"
#include <cstdint>

namespace ui::utils
{
bool HasAlignment(policies::Alignment value, policies::Alignment flag)
{
    return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
}

void SetWindowFlag(::entt::entity entity, policies::WindowFlag flag)
{
    if (!Registry::Valid(entity)) return;
    auto& windowComp = Registry::GetOrEmplace<components::Window>(entity);

    windowComp.flags |= flag;
}

} // namespace ui::utils
