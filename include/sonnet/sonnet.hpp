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


#include <expected>
#include <string>
#include <string_view>
#include <iosfwd>

#include "sonnet/value.hpp"
#include "sonnet/error.hpp"
#include "sonnet/options.hpp"


namespace Sonnet {

    using ParseResult = std::expected<value, ParseError>;

    [[nodiscard]] ParseResult parse(std::string_view input, const ParseOptions& opts = {});
    [[nodiscard]] ParseResult parse(std::istream& is, const ParseOptions& opts = {});
    [[nodiscard]] std::string dump(const value& v, const WriteOptions& opts = {});

    void dump(const value& v, std::ostream& os, const WriteOptions& opts = {});

} // namespace Sonnet
