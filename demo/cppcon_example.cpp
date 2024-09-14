#include "sensitive.hpp"
#include "untrusted.hpp"
#include "write_only.hpp"

#include <algorithm>
#include <fmt/core.h>
#include <fmt/format.h>

// ##################################################
// ##################################################
// ##################################################
// The API structs in their two different versions

namespace api
{
    enum class Status
    {
        waiting,
        accepted,
        rejected
    };

    namespace v1
    {
        struct ShowMessageCommand
        {
            std::string_view message;    // [input]  text to display
            Status* status;              // [output] reports status to user
        };
    }    // namespace v1

    namespace v2
    {
        // api.h
        struct ShowMessageCommand
        {
            const char* message;    // [input]  text to display
            size_t length;          // [input]  length of text to display
            Status* status;         // [output] reports status to user

            ShowMessageCommand(const char* message, Status* status)
            {
                this->message = message;
                this->length  = strlen(message);
                this->status  = status;
            }
        };

        // internal/api.h
        namespace untrusted
        {
            struct ShowMessageCommand
            {
                Untrusted<const char*> message;
                Untrusted<size_t> length;
                Untrusted<Status*> status;
            };
        }    // namespace untrusted
    }    // namespace v2
}    // namespace api

// Helper to pretty-print the status enum
namespace fmt
{
    template<>
    struct formatter<api::Status> : formatter<string_view>
    {
        template<typename FormatContext>
        auto format(api::Status status, FormatContext& ctx)
        {
            string_view name = "unknown";
            switch (status)
            {
                case api::Status::waiting:
                    name = "waiting";
                    break;
                case api::Status::accepted:
                    name = "accepted";
                    break;
                case api::Status::rejected:
                    name = "rejected";
                    break;
            }
            return formatter<string_view>::format(name, ctx);
        }
    };
}    // namespace fmt

#define RED "\33[0;31m"
#define GREEN "\33[0;32m"
#define COLOR_RESET "\33[0m"

namespace shared_memory
{
    namespace
    {
        std::byte* m_ptr;
    }

    template<typename T>
    T deserialize()
    {
        std::byte buffer[sizeof(T)];
        std::memcpy(buffer, m_ptr, sizeof(T));
        return std::bit_cast<T>(buffer);
    }

    void send(void* ptr) { m_ptr = reinterpret_cast<std::byte*>(ptr); }
}    // namespace shared_memory

// ##################################################
// ##################################################
// ##################################################
// The following helper functions emulate the internals of the Friendly Billboard firmware

bool is_friendly(std::string_view x) { return x.find("sucks") == std::string_view::npos; }

size_t truncate_to_max_text_length(size_t x) { return std::min(x, static_cast<size_t>(50)); };

static const char* wifi_password = RED "nobody_will_ever_guess_this_pw" COLOR_RESET;
bool is_in_shared_memory(const void* ptr) { return ptr != wifi_password; }

static std::function<void(void)> timed_exploit;
void update_statistics(api::Status status)
{
    fmt::print("  Decision: {}\n", status);
    // this is the point in time where the attacker would strike
    if (timed_exploit)
    {
        timed_exploit();
    }
}
template<typename T>
void show_on_billboard(T& s)
{
    fmt::print("  Billboard: \"{}\"\n", s);
}

// ##################################################
// ##################################################
// ##################################################

using namespace api;

template<typename Cmd>
void try_to_exploit(auto& receiver)
{
#define GOOD_MESSAGE (GREEN "C++ rocks!" COLOR_RESET)
#define BAD_MESSAGE (RED "C++ sucks!" COLOR_RESET)
    timed_exploit = {};
    fmt::print("Sending good message:\n");
    {
        Status status = Status::waiting;
        Cmd command{GOOD_MESSAGE, &status};
        shared_memory::send(&command);
        receiver();
    }
    fmt::print("\n");
    fmt::print("Sending unfriendly message:\n");
    {
        Status status = Status::waiting;
        Cmd command{BAD_MESSAGE, &status};
        shared_memory::send(&command);
        receiver();
    }
    fmt::print("\n");
    fmt::print("Exploit: String view with manipulated pointer\n");
    {
        Status status = Status::waiting;
        Cmd command{"looooooooooong messageeeeeeeee", &status};
        command.message = wifi_password;
        shared_memory::send(&command);
        receiver();
    }
    fmt::print("\n");

    // When running this exploit we may try to access a std::nullopt
    // This crashes in a debug build, but (on my machine) works fine in release mode.
#ifndef DEBUG
    fmt::print("Exploit: Status override\n");
    {
        Status status = Status::waiting;
        Cmd command{BAD_MESSAGE, &status};
        timed_exploit = [&]() {
            status = Status::accepted;
            fmt::print("  Overwriting status -> {}\n", status);
        };
        shared_memory::send(&command);
        receiver();
    }
    fmt::print("\n");
#endif
    fmt::print("Exploit: Message TOCTOU\n");
    {
        char message[] = GOOD_MESSAGE;
        Status status  = Status::waiting;
        Cmd command{message, &status};
        timed_exploit = [&]() { std::memcpy((void*)message, BAD_MESSAGE, sizeof(BAD_MESSAGE)); };
        shared_memory::send(&command);
        receiver();
    }
    fmt::print("\n");
#undef GOOD_MESSAGE
#undef BAD_MESSAGE
}

void friendly_billboard_attempt_1()
{
    fmt::print("######################\n{}\n\n", __func__);
    using namespace api::v1;

    // Friendly Billboard Firmware
    auto process_message_command = [](std::string_view message, Status& status) {
        status = is_friendly(message) ? Status::accepted : Status::rejected;
        update_statistics(status);
        if (status == Status::accepted)
        {
            show_on_billboard(message);
        }
    };
    auto receiver = [&process_message_command]() {
        auto received = shared_memory::deserialize<ShowMessageCommand>();
        process_message_command(received.message, *received.status);
    };

    try_to_exploit<ShowMessageCommand>(receiver);
}

void friendly_billboard_attempt_2()
{
    fmt::print("######################\n{}\n\n", __func__);
    using namespace api::v1;

    // Friendly Billboard Firmware
    auto process_message_command = [](Untrusted<std::string_view> message, Status& status) {
        auto verified = message.verify(is_friendly);
        status        = verified.has_value() ? Status::accepted : Status::rejected;
        update_statistics(status);
        if (status == Status::accepted)
        {
            show_on_billboard(*verified);
        }
    };
    auto receiver = [&process_message_command]() {
        auto received = shared_memory::deserialize<ShowMessageCommand>();
        process_message_command(received.message, *received.status);
    };

    try_to_exploit<ShowMessageCommand>(receiver);
}

void friendly_billboard_attempt_3()
{
    fmt::print("######################\n{}\n\n", __func__);
    using namespace api::v2;

    // Friendly Billboard Firmware
    auto process_message_command = [](Untrusted<std::string_view> message, Status& status) {
        auto verified = message.verify(is_friendly);
        status        = verified.has_value() ? Status::accepted : Status::rejected;
        update_statistics(status);
        if (status == Status::accepted)
        {
            show_on_billboard(*verified);
        }
    };
    auto receiver = [&process_message_command]() {
        auto received = shared_memory::deserialize<untrusted::ShowMessageCommand>();

        auto message = received.message.verify(is_in_shared_memory);
        auto length  = received.length.sanitize(truncate_to_max_text_length);
        auto status  = received.status.verify(is_in_shared_memory);

        if (message.has_value() && status.has_value())
        {
            Untrusted<std::string_view> partially_valid(message.value(), length);
            process_message_command(partially_valid, *status.value());
        }
        else if (status.has_value())
        {
            *status.value() = Status::rejected;
        }
    };

    try_to_exploit<ShowMessageCommand>(receiver);
}

void friendly_billboard_attempt_4()
{
    fmt::print("######################\n{}\n\n", __func__);
    using namespace api::v2;

    // Friendly Billboard Firmware
    auto process_message_command = [](Untrusted<std::string_view> message, WriteOnly<Status> status) {
        auto verified     = message.verify(is_friendly);
        auto local_status = verified.has_value() ? Status::accepted : Status::rejected;
        status            = local_status;
        update_statistics(local_status);
        if (local_status == Status::accepted)
        {
            show_on_billboard(*verified);
        }
    };
    auto receiver = [&process_message_command]() {
        auto received = shared_memory::deserialize<untrusted::ShowMessageCommand>();

        auto message = received.message.verify(is_in_shared_memory);
        auto length  = received.length.sanitize(truncate_to_max_text_length);
        auto status  = received.status.verify(is_in_shared_memory);

        if (message.has_value() && status.has_value())
        {
            Untrusted<std::string_view> partially_valid(message.value(), length);
            WriteOnly wrapped_status{status.value()};
            process_message_command(partially_valid, wrapped_status);
        }
        else if (status.has_value())
        {
            *status.value() = Status::rejected;
        }
    };

    try_to_exploit<ShowMessageCommand>(receiver);
}

void friendly_billboard_attempt_5()
{
    fmt::print("######################\n{}\n\n", __func__);
    using namespace api::v2;

    // Friendly Billboard Firmware
    auto process_message_command = [](Untrusted<std::string>& message, WriteOnly<Status> status) {
        auto verified     = message.verify(is_friendly);
        auto local_status = verified.has_value() ? Status::accepted : Status::rejected;
        status            = local_status;
        update_statistics(local_status);
        if (local_status == Status::accepted)
        {
            show_on_billboard(*verified);
        }
    };
    auto receiver = [&process_message_command]() {
        auto received = shared_memory::deserialize<untrusted::ShowMessageCommand>();

        auto message = received.message.verify(is_in_shared_memory);
        auto length  = received.length.sanitize(truncate_to_max_text_length);
        auto status  = received.status.verify(is_in_shared_memory);

        if (message.has_value() && status.has_value())
        {
            Untrusted<std::string> partially_valid(message.value(), length);
            WriteOnly wrapped_status{status.value()};
            process_message_command(partially_valid, wrapped_status);
        }
        else if (status.has_value())
        {
            *status.value() = Status::rejected;
        }
    };

    try_to_exploit<ShowMessageCommand>(receiver);
}

void cppcon_example()
{
    friendly_billboard_attempt_1();
    friendly_billboard_attempt_2();
    friendly_billboard_attempt_3();
    friendly_billboard_attempt_4();
    friendly_billboard_attempt_5();
}
