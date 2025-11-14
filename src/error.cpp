#include "sonnet/error.hpp"

namespace Sonnet {

    ParseError ParseError::make(code c, size_t o, size_t l, size_t col, std::string_view m) {
        ParseError e;
        e.errc = c;
        e.offset = o;
        e.line = l;
        e.column = col;
        e.msg.assign(m.begin(), m.end());
        return e;
    }

} // namespace Sonnet 