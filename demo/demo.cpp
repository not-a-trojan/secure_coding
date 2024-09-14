#include "read_once.hpp"
#include "read_once_buffer.hpp"
#include "sensitive.hpp"
#include "untrusted.hpp"
#include "write_only.hpp"

#include "cppcon_example.hpp"

#include <cmath>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

namespace fmt
{
    template<typename T>
    struct formatter<std::optional<T>> : fmt::formatter<T>
    {
        template<typename FormatContext>
        auto format(const std::optional<T>& opt, FormatContext& ctx)
        {
            if (opt)
            {
                return fmt::format_to(ctx.out(), "Some({})", *opt);
            }
            return fmt::format_to(ctx.out(), "None");
        }
    };
}    // namespace fmt

void demo_untrusted()
{
    fmt::print("### {} ###\n", __func__);
    for (auto input : {5, 42})
    {
        Untrusted<int> untrusted{input};
        auto sanitized = untrusted.sanitize([](auto x) { return std::min(x, 40); });
        auto verified  = untrusted.verify([](auto x) { return x > 40; });

        fmt::print("raw:       {}\n", input);
        fmt::print("sanitized: {}\n", sanitized);
        fmt::print("verified:  {}\n\n", verified);
    }
}

__attribute__((no_sanitize("address")))    // we need to disable address sanitizer in order to directly read memory
void demo_sensitive()
{
    fmt::print("### {} ###\n", __func__);

    using Key = std::array<std::byte, 16>;

    auto print_raw_memory = [&](auto addr, auto title) {
        fmt::print("{:<29} ", title);
        for (std::byte* p = (std::byte*)addr; p < (((std::byte*)addr) + sizeof(Sensitive<Key>)); ++p)
        {
            fmt::print("{:02x} ", *p);
        }
        fmt::print("\n");
    };

    void* raw_address = 0;
    {
        Sensitive<Key> a;
        a.with_sensitive_content([](auto key) {
            for (size_t i = 0; i < key.size(); ++i)
            {
                key[i] = static_cast<std::byte>(i);
            }
        });
        print_raw_memory(&a, "Initial memory:");

        Sensitive<Key> b = std::move(a);
        print_raw_memory(&a, "Initial memory after move:");
        print_raw_memory(&b, "New memory after move:");

        raw_address = &b;
    }
    print_raw_memory(raw_address, "New memory after destruction:");
}

void demo_read_once()
{
    fmt::print("### {} ###\n", __func__);
    {
        int x = 42;
        ReadOnce<int> once(&x);

        fmt::print("first read:  {}\n", once.read());
        fmt::print("second read: {}\n", once.read());
    }
    {
        std::array<float, 5> array{1, 2, 3.5, 4.2, 213123.23};
        ReadOnceBuffer<float> once(array.data(), array.size());
        std::array<float, 3> x;

        for (auto requested : {1, 3, 3})
        {
            x.fill(std::numeric_limits<float>::infinity());
            size_t n = once.read(x.data(), requested);
            fmt::print("requested {}, read {}: {}\n", requested, n, x);
        }
    }
}

void demo_write_only()
{
    fmt::print("### {} ###\n", __func__);

    int x = 16;
    WriteOnly<int> w(&x);
    fmt::print("x = {}\n", x);
    w = 18;
    fmt::print("x = {}\n", x);
}

int main()
{
    demo_untrusted();
    fmt::print("\n");
    demo_sensitive();
    fmt::print("\n");
    demo_read_once();
    fmt::print("\n");
    demo_write_only();
    fmt::print("\n");
    cppcon_example();
}
