#pragma once
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

struct CreateRoomRequest
{
    std::string roomName;
    uint32_t maxPlayers;

    [[nodiscard]] nlohmann::json toJson() const
    {
        return {{"roomName", roomName}, {"maxPlayers", maxPlayers}};
    }

    static CreateRoomRequest fromJson(const nlohmann::json& json)
    {
        return {.roomName = json.at("roomName").get<std::string>(),
                .maxPlayers = json.at("maxPlayers").get<uint32_t>()};
    }
};