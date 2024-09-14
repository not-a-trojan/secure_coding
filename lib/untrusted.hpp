#pragma once

#include <optional>

template<typename T>
struct Untrusted
{
    template<typename... Args>
    Untrusted(Args&&... args) : m_value{std::forward<Args>(args)...}
    {
    }

    template<typename Sanitizer>
    auto sanitize(Sanitizer&& sanitizer)
    {
        return sanitizer(std::move(m_value));
    }

    template<typename Predicate>
    std::optional<T> verify(Predicate&& verifier)
    {
        return verifier(m_value) ? std::optional{std::move(m_value)} : std::nullopt;
    }

    Untrusted(const Untrusted&) = default;
    Untrusted(Untrusted& other) : Untrusted{const_cast<const Untrusted&>(other)} {}
    Untrusted& operator=(const Untrusted&) = default;

    Untrusted(Untrusted&& other)      = default;
    Untrusted& operator=(Untrusted&&) = default;

private:
    T m_value;
};
