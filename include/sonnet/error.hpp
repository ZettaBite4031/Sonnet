#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>


namespace Sonnet {

    struct ParseError {
        enum class code : uint8_t {
            unexpected_character,
            invalid_number,
            invalid_string,
            invalid_escape,
            invalid_unicode_escape,
            unexpected_end_of_input,
            trailing_characters,
        };

        code errc{};
        size_t offset{};
        size_t line{};
        size_t column{};
        std::string msg{};

        static ParseError make(code c, size_t o, size_t l, size_t col, std::string_view m);
    };

} // namespace Sonnet