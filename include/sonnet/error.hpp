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

#include "sonnet/config.hpp"


/// @defgroup SonnetError Parsing Errors
/// @ingroup Sonnet
/// @brief Error codes and structures produced by the JSON parser
namespace Sonnet {
    
    /// @ingroup SonnetError
    /// @brief Structured error information produced during JSON parsing.
    ///
    /// @details
    /// A `ParseError` is returned whenever `Sonnet::parse(...)` fails to
    /// interpret the input as valid JSON. Each error contains:
    ///
    /// - **errc** — a classification of the error (syntax/format issue)
    /// - **offset** — byte offset from the start of input where the error occurred
    /// - **line** — 1-based line number of the error position
    /// - **column** — 1-based column number (UTF-8 byte offset within the line)
    /// - **msg** — human-readable explanation of the error
    ///
    /// The combination of structured fields and a descriptive message allows
    /// callers to produce helpful diagnostics, highlight error locations in
    /// editors, or convert errors to other formats.
    ///
    /// `ParseError` objects are typically returned in a `std::expected<value, ParseError>`
    /// through the `Sonnet::parse(...)` family of functions.
    struct ParseError {
        /// @ingroup SonnetError
        /// @brief Enumeration of possible error categories detected by the parser.
        ///
        /// @details
        /// Each enumerate describes a distinct syntactic or semantic violation of
        /// the JSON standard (RFC 8259). Additional error codes may be introduced
        /// in future versions as Sonnet gains more parsing options.
        ///
        /// Members:
        /// - `unexpected_character`  
        ///     Encountered a character that is not valid in the current parsing state.
        ///     Example: using `@` where a value or delimiter is expected.
        ///
        /// - `invalid_number`  
        ///     Number format does not match JSON grammar. Examples: leading zeros
        ///     (`012`), malformed exponents (`1e+`), missing digits after decimal.
        ///
        /// - `invalid_string`  
        ///     String literal violated JSON constraints (e.g. unescaped control
        ///     characters, missing terminating quote).
        ///
        /// - `invalid_escape`  
        ///     Invalid escape sequence inside a string (e.g. `\k`, `\xFF`).
        ///
        /// - `invalid_unicode_escape`  
        ///     Invalid `\uXXXX` sequence, malformed hex digits, or unpaired surrogate.
        ///
        /// - `unexpected_end_of_input`  
        ///     Input ended before a complete JSON value could be parsed.
        ///
        /// - `trailing_characters`  
        ///     Successfully parsed a complete JSON value, but non-whitespace
        ///     characters remain afterward.
        /// - `depth_limit_exceeded`
        ///     Successfully parse a complete JSON value, but maximum nesting depth
        ///     was reached. Off by default.
        enum class code : uint8_t {
            unexpected_character,   ///< Invalid or unexpected character.
            invalid_number,         ///< Malformed numeric literal.
            invalid_string,         ///< Malformed string literal.
            invalid_escape,         ///< Invalid escape sequence.
            invalid_unicode_escape, ///< Invalid or malformed Unicode escape.
            unexpected_end_of_input,///< Input ended prematurely.
            trailing_characters,    ///< Extra characters after valid JSON.
            depth_limit_exceeded,   ///< Maximum depth limit exceeded.
        };

        code errc{};       ///< The classification of the parsing error.
        std::size_t offset{}; ///< Byte offset from the beginning of the input.
        std::size_t line{};   ///< Line number where the error occurred (1-based).
        std::size_t column{}; ///< Column number where the error occurred (1-based).
        std::string msg{};    ///< Human-readable diagnostic message.

        /// @ingroup SonnetError
        /// @brief Constructs a fully-populated `ParseError` instance.
        ///
        /// @details
        /// This static helper is used internally by the parser to generate a
        /// structured error object with consistent formatting. All fields must
        /// be supplied, making the function suitable for both string-based and
        /// stream-based parsing.
        ///
        /// Example internal use:
        /// @code
        /// return ParseError::make(
        ///     code::invalid_number, cursor.offset(), cursor.line(),
        ///     cursor.column(), "Malformed exponent"
        /// );
        /// @endcode
        ///
        /// @param c    The error code describing the category of failure.
        /// @param o    Byte offset from the start of the input.
        /// @param l    Line number (1-based).
        /// @param col  Column number (1-based).
        /// @param m    Human-readable error message.
        /// @return A fully constructed `ParseError`.
        SONNET_API static ParseError make(code c, size_t o, size_t l, size_t col, std::string_view m);
    };

} // namespace Sonnet