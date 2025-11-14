#pragma once


/* 
    -----------------------------------------------------
    Sonnet::ParseError - Structured parse error reporting
    -----------------------------------------------------
    `Sonnet::ParseError` describes a failure that occured while parsing JSON.

    ------
    Fields
    ------
    - `code errc`:
        * Enumerated error code describing failure category:
            - `unexpected_character`
            - `invalid_number`
            - `invalid_string`
            - `invalid_unicode_escape`
            - `unexpected_end_of_input`
            - `trailing_characters`
            - `comment_not_allowed` (all lower added soon)
            - `trailing_comma_not_allowed`
            - `io_error`
            - `depth_limit_exceeded`
        * The exact set of codes is documented alongside enum definition
    - `size_t offset`:
        * Byte offset from the start of the input where the error was detected
        * Guaranteed to be in the range `[0, input.size()]` for string-based
          parsing
    - `size_t line`:
        * 1-based line number at which the error occurred
        * Maintained by the parser as it consumes characters
    - `size_t column`:
        * 1-based column number within the current line where the error
          was detected
    - `std::string msg`:
        * Human-readable description of the error
        * Intended for debugging and logging; not stable for programmatic use
    
    -----
    Usage
    -----
    - Parsing functions such as `Sonnet::parse(...)` return
      `std::expected<value, ParseError>` (or compatible expected type)
    - On failure, `ParseError` gives enough information to:
        * Display a helpful error message
        * Highlight the position of the error in the source
        * Decide how to recover or report the error
    
    ------------
    Construction
    ------------
    - `ParseError` is usually created via helper functions in the parse
      implementation (e.g. `ParseError::make(...)`), which compute the
      offset, line, column, and msg

    This header defines the error reporting structure and its error code
    enum; it does not contain parsing logic
*/

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