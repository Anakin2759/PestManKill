/**
 * @file PacketCreatorExample.cpp
 * @brief PacketCreator使用示例
 */

#include "PacketCreator.h"
#include "src/shared/messages/UseCardMasage.h"
#include <iostream>

void exampleUsage()
{
    // ========== 方式1: 分别创建Header和Payload(旧方式,仍然支持) ==========
    {
        // 只创建payload
        auto payload = PacketCreator::createLoginRequest("user123", "pass456");
        
        // NetworkManager会自动添加header
        // netManager.sendReliablePacket(PacketCreator::toUint16(MessageType::LOGIN), payload);
    }

    // ========== 方式2: 手动创建完整数据包(Header + Payload) ==========
    {
        // 创建header
        PacketHeader header{1, 0, PacketCreator::toUint16(MessageType::USE_CARD), 0};
        
        // 创建payload
        nlohmann::json cardData = {{"player", 1}, {"card", 42}, {"targets", std::vector<uint32_t>{2, 3}}};
        std::string jsonStr = cardData.dump();
        std::vector<uint8_t> payload(jsonStr.begin(), jsonStr.end());
        
        // 更新header的size字段
        header.size = static_cast<uint16_t>(payload.size());
        
        // 组合成完整数据包
        auto fullPacket = PacketCreator::createFullPacket(header, payload);
        
        // 直接发送完整数据包
        // socket.send(fullPacket);
    }

    // ========== 方式3: 使用便捷方法创建完整数据包(推荐!) ==========
    {
        // 创建完整心跳包
        auto heartbeatPacket = PacketCreator::createFullHeartbeat(123);
        
        // 创建完整登录包
        auto loginPacket = PacketCreator::createFullLoginRequest(456, "alice", "secret");
        
        // 创建完整使用卡牌包
        auto useCardPacket = PacketCreator::createFullUseCardMessage(789, 1, 42, {2, 3, 4});
        
        // 直接发送
        // socket.send(heartbeatPacket);
        // socket.send(loginPacket);
        // socket.send(useCardPacket);
    }

    // ========== 方式4: 使用POD结构体创建完整数据包 ==========
    {
        // 定义POD结构体
        struct PlayerPosition
        {
            float x;
            float y;
            float z;
        };
        
        PlayerPosition pos{10.5f, 20.3f, 5.0f};
        
        // 创建完整数据包
        auto posPacket = PacketCreator::createFullStructPacket(101, 0x0400, pos);
        
        // 发送
        // socket.send(posPacket);
    }

    // ========== 解析完整数据包 ==========
    {
        // 假设收到一个完整数据包
        auto packet = PacketCreator::createFullLoginRequest(999, "bob", "password");
        
        // 解析header和payload
        auto [header, payload] = PacketCreator::parseFullPacket(packet);
        
        std::cout << "Seq: " << header.seq << std::endl;
        std::cout << "Type: " << header.type << std::endl;
        std::cout << "Size: " << header.size << std::endl;
        
        // 进一步解析JSON
        if (header.type == PacketCreator::toUint16(MessageType::LOGIN))
        {
            auto jsonData = PacketCreator::parseJsonPacket(payload);
            std::string username = jsonData["username"];
            std::cout << "Username: " << username << std::endl;
        }
    }

    // ========== 一步解析JSON数据包 ==========
    {
        auto packet = PacketCreator::createFullUseCardMessage(555, 1, 99, {10, 20});
        
        // 一次性解析header和JSON
        auto [header, jsonData] = PacketCreator::parseFullJsonPacket(packet);
        
        std::cout << "Player: " << jsonData["player"] << std::endl;
        std::cout << "Card: " << jsonData["card"] << std::endl;
        
        // 转换为UseCardMessage结构体
        UseCardMessage msg = UseCardMessage::fromJson(jsonData);
        std::cout << "Targets count: " << msg.targets.size() << std::endl;
    }

    // ========== 一步解析POD结构体数据包 ==========
    {
        struct PlayerPosition
        {
            float x;
            float y;
            float z;
        };
        
        PlayerPosition originalPos{1.0f, 2.0f, 3.0f};
        auto packet = PacketCreator::createFullStructPacket(777, 0x0400, originalPos);
        
        // 一次性解析header和结构体
        auto [header, pos] = PacketCreator::parseFullStructPacket<PlayerPosition>(packet);
        
        std::cout << "Position: (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
    }
}

int main()
{
    exampleUsage();
    return 0;
}
