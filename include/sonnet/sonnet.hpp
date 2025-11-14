#pragma once

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