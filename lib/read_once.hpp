#pragma once

#include <optional>

template<typename T>
struct ReadOnce
{
    static_assert(std::is_trivially_copyable_v<T>);

    // Initialize based on a given source pointer.
    ReadOnce(T* address) : m_address{address} {}

    // Read the source.
    // Only returns the object once and std::nullopt afterwards.
    std::optional<T> read()
    {
        if (m_address == nullptr)
        {
            return std::nullopt;
        }
        T value   = *m_address;
        m_address = nullptr;
        return value;
    }

    // Delete copy operations as they could lead to reading the source twice.
    ReadOnce(const ReadOnce&)            = delete;
    ReadOnce& operator=(const ReadOnce&) = delete;

    // For move operations, explicitely mark the other object as read to avoid
    // accidental reading after move.
    ReadOnce(ReadOnce&& other)
    {
        m_address       = other.m_address;
        other.m_address = nullptr;
    }
    ReadOnce& operator=(ReadOnce&& other)
    {
        if (this != &other)
        {
            m_address       = other.m_address;
            other.m_address = nullptr;
        }
    }

private:
    T* m_address;
};
