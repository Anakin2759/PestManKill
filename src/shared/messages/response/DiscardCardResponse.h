#pragma once
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>
struct DiscardCardRequest
{
    uint32_t player;
    std::vector<uint32_t> cardIndexs;

    [[nodiscard]] nlohmann::json toJson() const { return {{"player", player}, {"cardIndexs", cardIndexs}}; }

    static DiscardCardMessage fromJson(const nlohmann::json& json)
    {
        return {.player = json.at("player").get<uint32_t>(),
                .cardIndexs = json.at("cardIndexs").get<std::vector<uint32_t>>()};
    }
};