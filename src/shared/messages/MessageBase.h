/**
 * ************************************************************************
 *
 * @file MessageBase.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.1
 * @brief 消息基类定义，提供序列化/反序列化接口
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <cstdint>
#include <span>
#include <vector>
#include <expected>
#include <nlohmann/json.hpp>

// 消息序列化错误类型
enum class MessageError
{
    SerializeFailed,   // 序列化失败
    DeserializeFailed, // 反序列化失败
    InvalidFormat,     // 无效格式
    BufferTooSmall     // 缓冲区过小
};

/**
 * @brief 消息接口基类（CRTP 模式）
 * @tparam Derived 派生类类型
 *
 * 使用示例：
 * struct MyMessage : public MessageBase<MyMessage> {
 *     uint32_t data;
 *
 *     std::expected<std::span<uint8_t>, MessageError>
 *     serializeImpl(std::span<uint8_t> buffer) const;
 *
 *     static std::expected<MyMessage, MessageError>
 *     deserializeImpl(std::span<const uint8_t> data);
 * };
 */
template <typename Derived>
class MessageBase
{
public:
    /**
     * @brief 序列化消息到二进制缓冲区
     * @param buffer 输出缓冲区
     * @return 成功返回实际使用的缓冲区切片，失败返回错误码
     */
    std::expected<std::span<uint8_t>, MessageError> serialize(std::span<uint8_t> buffer) const
    {
        return static_cast<const Derived*>(this)->serializeImpl(buffer);
    }

    /**
     * @brief 从二进制数据反序列化消息
     * @param data 输入数据
     * @return 成功返回消息对象，失败返回错误码
     */
    static std::expected<Derived, MessageError> deserialize(std::span<const uint8_t> data)
    {
        return Derived::deserializeImpl(data);
    }

    /**
     * @brief 序列化到 JSON
     * @return JSON 对象
     */
    [[nodiscard]] nlohmann::json toJson() const { return static_cast<const Derived*>(this)->toJsonImpl(); }

    /**
     * @brief 从 JSON 反序列化
     * @param json JSON 对象
     * @return 成功返回消息对象，失败返回错误码
     */
    static std::expected<Derived, MessageError> fromJson(const nlohmann::json& json)
    {
        return Derived::fromJsonImpl(json);
    }

protected:
    // 防止直接实例化基类
    MessageBase() = default;
    ~MessageBase() = default;
};

/**
 * @brief JSON 序列化辅助宏（简化实现）
 *
 * 使用示例：
 * struct MyMessage {
 *     uint32_t id;
 *     std::string name;
 *
 *     DEFINE_JSON_SERIALIZATION(MyMessage, id, name)
 * };
 */
#define DEFINE_JSON_SERIALIZATION(Type, ...)                                                                           \
    [[nodiscard]] nlohmann::json toJsonImpl() const                                                                    \
    {                                                                                                                  \
        nlohmann::json j;                                                                                              \
        return nlohmann::json{__VA_ARGS__};                                                                            \
    }                                                                                                                  \
    static std::expected<Type, MessageError> fromJsonImpl(const nlohmann::json& j)                                     \
    {                                                                                                                  \
        try                                                                                                            \
        {                                                                                                              \
            return Type{__VA_ARGS__};                                                                                  \
        }                                                                                                              \
        catch (...)                                                                                                    \
        {                                                                                                              \
            return std::unexpected(MessageError::DeserializeFailed);                                                   \
        }                                                                                                              \
    }
