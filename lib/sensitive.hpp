#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

template<typename T>
struct Sensitive
{
    static_assert(std::is_trivial_v<T>);

    template<typename... Args>
    Sensitive(Args&&... args) : m_content{std::forward<Args>(args)...}
    {
    }

    using Func = void (*)(T&);
    void with_sensitive_content(Func&& func) { func(m_content); }

    void secure_erase()
    {
        volatile std::byte* erase_ptr = reinterpret_cast<std::byte*>(this);
        size_t num_bytes              = sizeof(*this);
        while (num_bytes--)
        {
            *erase_ptr++ = std::byte{0};
        }
    }

    ~Sensitive() { secure_erase(); }

    Sensitive(const Sensitive&)            = delete;
    Sensitive& operator=(const Sensitive&) = delete;

    Sensitive(Sensitive&& other)
    {
        m_content = other.m_content;
        other.secure_erase();
    }

    Sensitive& operator=(Sensitive&& other)
    {
        if (this != &other)
        {
            m_content = other.m_content;
            other.secure_erase();
        }
    }

private:
    T m_content;
};
