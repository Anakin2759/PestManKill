#pragma once
#include <vector>
#include <cstddef>
#include <stdexcept>
// NOLINTBEGIN
namespace utils
{
template <typename T>
class RoundRobin
{
public:
    using container_type = std::vector<T>;
    using iterator = container_type::iterator;
    using const_iterator = container_type::const_iterator;

    RoundRobin() = default;

    explicit RoundRobin(std::vector<T> data) : data_(std::move(data)) {}

    // ---- ranges 支持 ----
    iterator begin() noexcept { return data_.begin(); }
    iterator end() noexcept { return data_.end(); }
    const_iterator begin() const noexcept { return data_.begin(); }
    const_iterator end() const noexcept { return data_.end(); }

    // ---- 基本容器操作 ----
    void push_back(const T& v) { data_.push_back(v); }
    void push_back(T&& v) { data_.push_back(std::move(v)); }

    void erase(size_t index)
    {
        if (index >= data_.size()) return;
        data_.erase(data_.begin() + index);
        if (current_ >= data_.size()) current_ = 0; // 收缩后调整
    }

    bool empty() const noexcept { return data_.empty(); }
    size_t size() const noexcept { return data_.size(); }

    // ---- 核心：循环 next() ----
    T& next()
    {
        if (data_.empty()) throw std::runtime_error("RoundRobin is empty");

        auto& ref = data_[current_];
        current_ = (current_ + 1) % data_.size();
        return ref;
    }

    // 返回当前元素但不推进
    T& current()
    {
        if (data_.empty()) throw std::runtime_error("RoundRobin is empty");
        return data_[current_];
    }

private:
    std::vector<T> data_;
    size_t current_ = 0;
};
} // namespace utils
// NOLINTEND