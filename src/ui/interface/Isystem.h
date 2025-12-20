/**
 * ************************************************************************
 *
 * @file Isystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-20
 * @version 0.1
 * @brief UI系统接口
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

namespace ui::interface
{
struct ISystem : entt::type_list<>
{
    template <typename Base>
    struct type : Base
    {
        void registerHandlers() { entt::poly_call<0>(*this); }
        void unregisterHandlers() { entt::poly_call<1>(*this); }
        void update() { entt::poly_call<2>(*this); }
    };

    template <typename T>
    using impl = entt::value_list<&T::registerHandlers, &T::unregisterHandlers, &T::update>;
};

template <typename Derived>
struct EnableRegister
{
    /**
     * @brief 注册事件处理器
     */
    void registerEvents() { static_cast<Derived*>(this)->registerEventsImpl(); }
    /**
     * @brief 注销事件处理器
     */
    void unregisterEvents() { static_cast<Derived*>(this)->unregisterEventsImpl(); }
    /**
     * @brief 更新系统状态
     */
    void update() { static_cast<Derived*>(this)->updateImpl(); }
};
} // namespace ui::interface