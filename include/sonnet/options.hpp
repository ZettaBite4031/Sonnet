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
          (`/* ...* /`) comments in addition to standard JSON whitespace
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


namespace Sonnet {

    struct ParseOptions {
        bool allow_comments = false;
        bool allow_trailing_commas = true;
        // size_t max_depth = 0;
    };

    struct WriteOptions {
        bool pretty = false;
        size_t indent = 2;
        bool sort_keys = false;
    };

} // namespace Sonnet