#pragma once
#include <array>
#include <atomic>
#include <optional>
#include <cassert>

// 高效单线程循环队列
template <typename T, size_t Capacity>
class CircularQueue
{
    static_assert(Capacity > 0, "Capacity must be greater than 0");

public:
    CircularQueue() = default;

    bool push(const T& value)
    {
        if (m_full) return false; // 队列已满
        m_buffer[m_tail] = value;
        m_tail = (m_tail + 1) & mask();
        m_full = (m_tail == m_head);
        return true;
    }

    bool push(T&& value)
    {
        if (m_full) return false;
        m_buffer[m_tail] = std::move(value);
        m_tail = (m_tail + 1) & mask();
        m_full = (m_tail == m_head);
        return true;
    }

    std::optional<T> pop()
    {
        if (empty()) return std::nullopt;
        T value = std::move(m_buffer[m_head]);
        m_head = (m_head + 1) & mask();
        m_full = false;
        return value;
    }

    [[nodiscard]] bool empty() const { return !m_full && (m_head == m_tail); }
    [[nodiscard]] bool full() const { return m_full; }

    [[nodiscard]] size_t size() const
    {
        if (m_full) return Capacity;
        return (m_tail + Capacity - m_head) & mask();
    }

    /**
     * @brief 获取队首元素（不移除）
     * @return 队首元素的引用
     */
    const T& front() const
    {
        assert(!empty() && "Cannot get front of empty queue");
        return m_buffer[m_head];
    }

    /**
     * @brief 获取队首元素（不移除）
     * @return 队首元素的引用
     */
    T& front()
    {
        assert(!empty() && "Cannot get front of empty queue");
        return m_buffer[m_head];
    }

    /**
     * @brief 旋转队列（队首移到队尾）
     */
    void rotate()
    {
        if (empty()) return;
        auto value = pop();
        if (value.has_value())
        {
            push(std::move(value.value()));
        }
    }

    void clear()
    {
        m_head = m_tail = 0;
        m_full = false;
    }

private:
    static constexpr size_t mask() { return Capacity - 1; }
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2 for bitmask optimization");

    std::array<T, Capacity> m_buffer;
    size_t m_head = 0;
    size_t m_tail = 0;
    bool m_full = false;
};

// ========================================================
// 可选：单生产者/单消费者多线程安全版本
template <typename T, size_t Capacity>
class SPSCQueue
{
    static_assert(Capacity > 0 && (Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");

public:
    SPSCQueue() = default;

    bool push(const T& value)
    {
        size_t next = (m_tail + 1) & mask();
        if (next == m_head.load(std::memory_order_acquire)) return false; // 队满
        m_buffer[m_tail] = value;
        m_tail = next;
        return true;
    }

    bool push(T&& value)
    {
        size_t next = (m_tail + 1) & mask();
        if (next == m_head.load(std::memory_order_acquire)) return false;
        m_buffer[m_tail] = std::move(value);
        m_tail = next;
        return true;
    }

    std::optional<T> pop()
    {
        size_t head = m_head.load(std::memory_order_relaxed);
        if (head == m_tail) return std::nullopt; // 空队列
        T value = std::move(m_buffer[head]);
        m_head.store((head + 1) & mask(), std::memory_order_release);
        return value;
    }

    [[nodiscard]] bool empty() const { return m_head.load() == m_tail; }

private:
    static constexpr size_t mask() { return Capacity - 1; }
    std::array<T, Capacity> m_buffer;
    std::atomic<size_t> m_head{0};
    size_t m_tail = 0;
};
