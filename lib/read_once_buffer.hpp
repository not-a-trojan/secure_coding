#pragma once

#include <cstring>
#include <optional>
#include <algorithm>

template<typename T>
struct ReadOnceBuffer
{
    static_assert(std::is_trivially_copyable_v<T>);

    // Initialize based on a given source pointer and length.
    ReadOnceBuffer(T* address, size_t length) : m_address{address}, m_remaining{length} {}

    // Read the source.
    // Returns the number of read objects, 0 after the available objects are exhausted.
    size_t read(T* destination, size_t requested_length)
    {
        size_t n = std::min(requested_length, m_remaining);
        if (n > 0)
        {
            std::memcpy(destination, m_address, n * sizeof(T));
            m_address += n;
            m_remaining -= n;
        }
        return n;
    }

    // Delete copy operations as they could lead to reading the source twice.
    ReadOnceBuffer(const ReadOnceBuffer&)            = delete;
    ReadOnceBuffer& operator=(const ReadOnceBuffer&) = delete;

    // For move operations, explicitely mark the other object as read to avoid
    // accidental reading after move.
    ReadOnceBuffer(ReadOnceBuffer&& other)
    {
        m_address         = other.m_address;
        m_remaining       = other.m_remaining;
        other.m_remaining = 0;
    }
    ReadOnceBuffer& operator=(ReadOnceBuffer&& other)
    {
        if (this != &other)
        {
            m_address         = other.m_address;
            m_remaining       = other.m_remaining;
            other.m_remaining = 0;
        }
    }

private:
    T* m_address;
    size_t m_remaining;
};
