#pragma once

#include <cstddef>


namespace Sonnet {

    struct ParseOptions {
        bool allow_comments = true;
        bool allow_trailing_commas = true;
    };

    struct WriteOptions {
        bool pretty = true;
        size_t indent = 2;
        bool sort_keys = false;
    };

} // namespace Sonnet