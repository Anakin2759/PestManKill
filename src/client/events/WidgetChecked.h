#pragma once

#include <cstdint>
enum class WidgetType : uint8_t
{
    CHRACTER_CARD,
    HAND_CARD,
    EQUIP_CARD,
};

struct WidgetChecked
{
    WidgetType type;
    bool longPressed;
};