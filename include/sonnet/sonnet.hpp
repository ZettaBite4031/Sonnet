#pragma once


/*
    ----------------------------------------------------------------
    Sonnet - Modern C++ Json library (DOM + parsing + serialization)
    ----------------------------------------------------------------

    This is the main public header for Sonnet

    It brings together:
        - The dynamic JSON DOM type:    `Sonnet::value`
        - Error reporting types:        `Sonnet::ParseError`
        - Parsing functions:            `Sonnet::parse(...)`
        - Serialization functions:      `Sonnet::dump(...)`
        - Configuration options:        `Sonnet::ParseOptions`,
                                        `Sonnet::WriteOptions`
        - Conversion utilities:         `to_json` / `from_json` support

    -------------------
    High-Level Overview
    -------------------
    - DOM: 
        * `Sonnet::value` represents any JSON value and uses `std::pmr`
          allocators for efficient memory management
        * It supports structural equality, object/array manipulation
    - Parsing:
        * `std::expected<value, ParseError> parse(std::string_view, const ParseOptions& = {})`
        * `std::expected<value, ParseError> parse(std::istream&, const ParseOptions& = {})`
        * `std::expected<value, ParseError> parse(const std::filesystem::path&, const ParseOptions& = {})`
        * Parsing is configurable via `ParseOptions` (comments, trailing commas, depth limits, etc.)
    - Serialization: 
        * `std::string dump(const value&, const WriteOptions& = {})`
        * `void dump(const value&, std::ostream&, const WriteOptions& = {})`
        * Pretty-printing and compact output are controlled via `WriteOptions`
    - Conversion:
        - User-defined types can be converted to/from `Sonnet::value` via
          `to_json` and `from_json` customization points defined in
          `convert.hpp`
    
    ------------
    Design Goals
    ------------
    - Modern C++:
        * Uses C++20/23 features (e.g. `std::expected` where available)
          and `std::pmr` for allocator-aware containers
    - Correctness:
        * Aims for RFC 8259-compliant parsing by default, with opt-in
          extensions (comments, trailing commas)
    - Performance: 
        * Designed for efficient DOM manipulation; future versions may
          add streaming/SAX-style APIs and additional optimizations
    - Composability:
        * Avoids global state; user controls allocator and options
        * Plays well with user-defined types via non-intrusive
          conversion hooks

    -----
    Usage
    -----
        #include <sonnet/sonnet.hpp>

        int main() {
            auto result = Sonnet::parse(R"({"hello":"world","x":42})");
            if (!result) {
                std::println("Parse Error: {}", result.error().msg.c_str());
                return 1;
            }

            Sonnet::value& root = result.value();
            root["y"] = true

            std::println("{}", Sonnet::dump(root, {.pretty = true}));
        }

    Include this header if you want the full Sonnet API. For finer-grained
    control or faster build times, you can include individual headers such as 
    `value.hpp`, `error.hpp`, `options.hpp`, and `convert.hpp` directly
*/

/// @defgroup Sonnet Sonnet JSON Library
/// @brief Core types and functions for Sonnet

/// @defgroup SonnetAPI Top-level Parsing and Serialization API
/// @ingroup Sonnet
/// @brief Convenient free functions for parsing and writing JSON

#include <expected>
#include <string>
#include <string_view>
#include <iosfwd>

#include "sonnet/value.hpp"
#include "sonnet/error.hpp"
#include "sonnet/options.hpp"

namespace Sonnet {

    /// @ingroup SonnetAPI
    /// @brief Alias for the result type returned by JSON parsing function
    ///
    /// @details
    /// `ParseResult` is a convenience alias for:
    ///         std::expected<value, ParseError>
    ///
    /// Successful parsing yeilds a fully constructed `Sonnet::value` DOM tree.
    /// Failures are reported through a `ParseError` containing:
    ///  - error code
    ///  - line/column information
    ///  - byte offset
    ///  - human-readable message
    ///
    /// This type is used by all `parse(...)` overloads.
    using ParseResult = std::expected<value, ParseError>;

    /// @ingroup SonnetAPI
    /// @brief Parses a JSON document from a string view
    ///
    /// @details
    /// Attempts to parse the UTF-8 JSON text provided in @p input according
    /// to the rules defined by RFC 8259 and modified by the supplied
    /// `ParseOptions`. On success, a fully constructed `Sonnet::value` DOM tree
    /// is returned inside a `ParseResult`. On failure, a `ParseError` describing
    /// the error location and reason is returned instead.
    /// 
    /// Example:
    /// @code
    /// auto res = Sonnet::parse(R"({"x":42})");
    /// if (!res) {
    ///     std::cerr << res.error().msg << '\n';
    /// } else {
    ///     std::cout << Sonnet::dump(res.value(), {.pretty = true });
    /// }
    /// @endcode
    ///
    /// @param input UTF-8 encoded JSON text to parse
    /// @param opts Parsing configuration options (comments, trailing commas, etc.)
    /// @return A `ParseResult` containing either a DOM tree or a parse error
    [[nodiscard]] ParseResult parse(std::string_view input, const ParseOptions& opts = {});

    /// @ingroup SonnetAPI
    /// @brief Parses a JSON document from an input stream
    ///
    /// @details
    /// Reads the entire contents of @p is and attempts to parse it as JSON using
    /// the provided `ParseOptions`. This overload is useful for parsing files,
    /// network streams, stringstreams, or any other `std::istream`.
    /// 
    /// Example:
    /// @code
    /// std::ifstream ifs("data.json");
    /// auto res = Sonnet::parse(ifs);
    /// @endcode
    ///
    /// On success, a DOM tree is returned. On failure, a `ParseError` containing
    /// precise error location metadata is returned
    ///
    /// @param is Input stream containing UTF-8 JSON text
    /// @param opts Parsing configuration options (comments, trailing commas, etc.)
    /// @return A `ParseResult` containing either a DOM tree or a parse error
    [[nodiscard]] ParseResult parse(std::istream& is, const ParseOptions& opts = {});

    /// @ingroup SonnetAPI
    /// @brief Serializes a JSON DOM value to a string
    ///
    /// @details
    /// Produces a UTF-8 JSON representation of the given `value` using the
    /// formatting rules specified in @p opts. Supports:
    ///  - compact serialization (default)
    ///  - pretty printing (indentation)
    ///  - optional key sourting (if enabled)
    ///
    /// The returned string owns its memory and does not use polymorphic allocators.
    ///
    /// Example:
    /// @code
    /// std::string json = Sonnet::dump(v, {.pretty = true});
    /// std::cout << json;
    /// @endcode
    ///
    /// @param v The DOM value to serialize
    /// @param opts Formatting options
    /// @return A UTF-8 JSON string representation of @p `v`.
    [[nodiscard]] std::string dump(const value& v, const WriteOptions& opts = {});

    /// @ingroup SonnetAPI
    /// @brief Serializes a JSON DOM value and writes it to an output stream
    ///
    /// @details 
    /// Writes a JSON representation of @p v to the output stream @p os using the
    /// formatting rules defined by @p opts. This overload avoids allocating a 
    /// temporary string, making it suitable for large outputs or streaming use.
    ///
    /// Example:
    /// @code
    /// Sonnet::dump(v, std::cout, {.pretty = true});
    /// @endcode
    ///
    /// @param v The DOM value to serialize 
    /// @param os Output stream to receive JSON text
    /// @param opts Formatting options 
    void dump(const value& v, std::ostream& os, const WriteOptions& opts = {});

} // namespace Sonnet
