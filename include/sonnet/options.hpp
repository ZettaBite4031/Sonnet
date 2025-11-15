#pragma once


/*
    ----------------------------------
    Sonnet parsing and writing options
    ----------------------------------
    This header defines configuration structures that control the behavior
    of parsing (JSON -> DOM) and writing (DOM -> JSON) operations

    --------------------------------------
    Parsing Options - Sonnet::ParseOptions
    --------------------------------------
    `ParseOptions` tunes how `Sonnet::parse(...)` behaves:

    - `bool allow_comments`:
        * When true, the parser accepts line (`// ...`) and block
          (`/ * ... * /`) comments in addition to standard JSON whitespace
        * When false (defaults for strict JSON), encountering a comment
          results in a `comment_not_allowed` parse error
    - `bool allow_trailing_commas`:
        * When true, the parser accepts trailing commas in arrays and objects
          e.g. `[1,2,]` or `{"a": 1,}`
        * When false (strict JSON), trailing commas produce a
          `trailing_commas_not_allowed` parse error
    - `size_t max_depth`:
        * Optional limit on nesting depth of arrays/objects
        * If exceeded, the parser fails with `depth_limit_exceeded`
        * A value of 0 is treated as no explicit limit
    
    - Additional fields may be added in the future to control
      performance and validation behavior (e.g. max str len, max arr size)
    
    --------------------------------------
    Writing Options - Sonnet::WriteOptions
    --------------------------------------
    `WriteOptions` tunes how `Sonnet::dump(...)` and related functions
    serialize DOM values back to JSON text:

    - `bool pretty`:
        * When false (default), produces compact JSON without extra whitespace
        * When true, outputs indented, human-readable JSON with newlines and spaces
    - `size_t indent`:
        * Number of spaces to indent per nesting level in pretty mode
        * Ignored if `pretty == false`
    - `bool sort_keys`:
        * When true, object keys are written in sorted order when the
          underlying container type does not already guarantee ordering
        * For `std::pmr::map`-based objects (already ordered), this flag
          has no effect but is provided for future expansions

    -----
    Usage
    -----
    - Parsing:
        * `auto result = Sonnet::parse(text, ParseOptions{ ... })`
    - Writing:
        * `std::string json = Sonnet::dump(value, WriteOptions{ ... })`

    These option structures provide a stable configuration interface for
    parsing and writing JSON. They are plain aggregates suitable for
    brace-initialization
*/


#include <cstddef>

/// @defgroup SonnetOptions Parsing and Writing Options
/// @ingroup Sonnet
/// @brief Configuration objects controlling parsing and serialization

namespace Sonnet {

    /// @ingroup SonnetOptions
    /// @brief Configuration controlling JSON parsing behavior
    ///
    /// @details
    /// `ParseOptions` allows adjusting how strictly Sonnet parses JSON input.
    /// By default, the parser is strict according to RFC 8259 except for
    /// allowing trailing commas, which is often desirable in configuration files.
    ///
    /// Fields: 
    /// `allow_comments`
    ///   - When `true`, the parser accepts both line comments (`// ...`) and
    ///     block comments (`/* ... */`)
    ///   - When `false` (default), encountering a comment produces a 
    ///     `ParseError` with code `comment_not_allowed`.
    /// `allow_trailing_commas`
    ///   - When `true` the parser accepts trailing commas in arrays and objects,
    ///     such as `[1,2,]` or `{"a":1,}`.
    ///   - When `false` (default), trailing commas result in a `ParseError` with code
    ///     `trailing_comma_not_allowed`.
    /// `max_depth`
    ///   - Maximum allowed nesting depth of arrays/objects.
    ///   - A value of `0` means "no explicit depth limit"
    ///   - If the nesting depth exceeds this limit during parsing, a
    ///     `ParseError` with code `depth_limit_exceeded` is returned.
    ///
    /// Example:
    /// @code
    /// ParseOptions opts;
    /// opts.allow_comments = true;
    /// opts.max_depth = 32;
    /// auto result = Sonnet::parse(text, opts);
    /// @endcode
    struct ParseOptions {
        bool allow_comments = false; ///< Accept `//` and `/* */` comments if true
        bool allow_trailing_commas = false; ///< Permit trailing commas in arrays/objects if true
        size_t max_depth = 0; ///< Maximum allowed nesting depth (0 = unlimited)
    };

    /// @ingroup SonnetOptions
    /// @brief Configuration options controlling JSON serialization (dumping).
    ///
    /// @details
    /// `WriteOptions` allows customizing how a `Sonnet::value` is formatted
    /// when serialized to JSON text.
    ///
    /// Fields:
    /// `pretty`: 
    ///   - When `true`, enables human-readable formatting with newlines and
    ///     indentation.  
    ///   - When `false` (default), output is compact with minimal whitespace.
    ///
    /// `indent`:
    ///   - Number of spaces to indent for each nesting level when `pretty`
    ///     formatting is enabled.  
    ///   - Ignored if `pretty == false`.
    ///
    /// `sort_keys`:
    ///   - When `true`, object keys are written in sorted (lexicographic)
    ///     order.  
    ///   - This ensures deterministic output when object key order should not
    ///     depend on insertion order.  
    ///   - For containers that already maintain sorted keys (e.g. `pmr::map`),
    ///     this may have no observable effect.
    ///
    /// Example:
    /// @code
    /// WriteOptions wo;
    /// wo.pretty = true;
    /// wo.indent = 4;
    /// std::string json = Sonnet::dump(v, wo);
    /// @endcode
    struct WriteOptions {
        bool pretty = false;        ///< Enable pretty-printing (formatted output).
        std::size_t indent = 2;     ///< Number of spaces per indentation level.
        bool sort_keys = false;     ///< Sort object keys before writing if true.
    };


} // namespace Sonnet