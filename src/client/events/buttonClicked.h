
#pragma once
#include <cstdint>
enum class ButtonType : uint8_t
{
    END_TURN,
    USE_CARD,
    DRAW_CARD,
    PLAY_SKILL
};
struct ButtonClicked
{
    ButtonType type;
};