#include "sonnet/sonnet.hpp"

#include <sstream>
#include <charconv>
#include <cctype>
#include <limits>
#include <cmath>


namespace Sonnet {

    namespace detail {
        ParseResult parse_impl(std::string_view text, const ParseOptions& opts);
        void dump_impl(const value& v, std::ostream& os, const WriteOptions& opts, size_t depth);
    } // namespace detail

    ParseResult parse(std::string_view input, const ParseOptions& opts) {
        return detail::parse_impl(input, opts);
    }

    ParseResult parse(std::istream& is, const ParseOptions& opts) {
        std::ostringstream oss;
        oss << is.rdbuf();
        return detail::parse_impl(oss.str(), opts);
    }

    std::string dump(const value& v, const WriteOptions& opts) {
        std::ostringstream oss;
        detail::dump_impl(v, oss, opts, 0);
        return oss.str();
    }

    
#pragma region Parser
    // ================================
    // Internal parser implementation
    // ================================

    namespace detail {
        using expected_void = std::expected<void, ParseError>;
        template<typename T>
        using expected_t = std::expected<T, ParseError>;

        struct Scanner {
            std::string_view text;
            const ParseOptions& opts;
            size_t idx = 0;
            size_t line = 1;
            size_t column = 1;
            size_t depth = 0;
            size_t max_depth = 0;
            std::pmr::memory_resource* mem_res;

            Scanner(std::string_view t, const ParseOptions& o, std::pmr::memory_resource* r)
                : text{ t }, opts{ o }, max_depth{ o.max_depth }, mem_res{ r } {}

            [[nodiscard]] bool eof() const noexcept { return idx >= text.size(); }
            [[nodiscard]] char peek() const noexcept { return eof() ? '\0' : text[idx]; }
            [[nodiscard]] char peek_next() const noexcept { return (idx + 1 < text.size()) ? text[idx + 1] : '\0'; }

            char get() {
                if (eof()) return '\0';
                char c = text[idx++];
                if (c == '\n') { 
                    line++;
                    column = 1;
                } else column++;
                return c;
            }

            bool consume(char c) {
                if (peek() == c) {
                    get();
                    return true;
                }
                return false;
            }

            ParseError make_error(ParseError::code code, std::string_view msg) const {
                return ParseError::make(code, idx, line, column, msg);
            }
        };

        struct DepthGuard {
            Scanner& s;
            bool active = false;

            DepthGuard(Scanner& sc) : s(sc) {
                if (s.max_depth != 0) {
                    if (s.depth + 1 > s.max_depth) active = false;
                    else {
                        s.depth++;
                        active = true;
                    }
                } else {
                    s.depth++;
                    active = true;
                }
            }

            ~DepthGuard() {
                if (active) s.depth--;
            }

            bool ok() const {
                return active;
            }
        };

        expected_t<value> parse_value(Scanner& s);
        expected_t<value> parse_object(Scanner& s);
        expected_t<value> parse_array(Scanner& s);
        expected_t<double> parse_number(Scanner& s);
        expected_t<string> parse_string(Scanner& s);
        expected_void parse_literal(Scanner& s, std::string_view literal, ParseError::code code, std::string_view fail_msg);
        expected_void skip_ws_and_comments(Scanner& s);
        
        inline bool is_valid_utf8(std::string_view s, size_t& error_idx) {
            const unsigned char* data = reinterpret_cast<const unsigned char*>(s.data());
            size_t i = 0; 
            size_t n = s.size();

            auto fail = [&](size_t idx) { error_idx = idx; return false; };

            while (i < n) {
                unsigned char c = data[i];

                if (c <= 0x7F) {
                    i++;
                    continue;
                }

                if (c >= 0xC2 && c <= 0xDF) {
                    if (i + 1 >= n) return fail(i);
                    unsigned char c1 = data[i + 1];
                    if ((c1 & 0xC0) != 0x80) return fail(i);
                    i += 2;
                    continue;
                }

                if (c >= 0xE1 && c <= 0xEC) {
                    if (i + 2 >= n) return fail(i);
                    unsigned char c1 = data[i + 1];
                    unsigned char c2 = data[i + 2];
                    if ((c1 & 0xC0) != 0x80) return fail(i);
                    if ((c2 & 0xC0) != 0x80) return fail(i);
                    i += 3;
                    continue;
                }

                if (c == 0xE0) {
                    if (i + 2 >= n) return fail(i);
                    unsigned char c1 = data[i + 1];
                    unsigned char c2 = data[i + 2];
                    if (c1 < 0xA0 || c1 > 0xBF) return fail(i);
                    if ((c2 & 0xC0) != 0x80) return fail(i);
                    i += 3;
                    continue;
                }

                if (c == 0xED) {
                    if (i + 2 >= n) return fail(i);
                    unsigned char c1 = data[i + 1];
                    unsigned char c2 = data[i + 2];
                    if (c1 < 0x80 || c1 > 0x9F) return fail(i);
                    if ((c2 & 0xC0) != 0x80) return fail(i);
                    i += 3;
                    continue;
                }

                if (c >= 0xEE && c <= 0xEF) {
                    if (i + 2 >= n) return fail(i);
                    unsigned char c1 = data[i + 1];
                    unsigned char c2 = data[i + 2];
                    if ((c1 & 0xC0) != 0x80) return fail(i);
                    if ((c2 & 0xC0) != 0x80) return fail(i);
                    i += 3;
                    continue;
                }

                if (c == 0xF0) {
                    if (i + 3 >= n) return fail(i);
                    unsigned char c1 = data[i + 1];
                    unsigned char c2 = data[i + 2];
                    unsigned char c3 = data[i + 3];
                    if (c1 < 0x90 || c1 > 0xBF) return fail(i);
                    if ((c2 & 0xC0) != 0x80) return fail(i);
                    if ((c3 & 0xC0) != 0x80) return fail(i);
                    i += 4;
                    continue;
                }

                if (c >= 0xF1 && c <= 0xF3) {
                    if (i + 3 >= n) return fail(i);
                    unsigned char c1 = data[i + 1];
                    unsigned char c2 = data[i + 2];
                    unsigned char c3 = data[i + 3];
                    if ((c1 & 0xC0) != 0x80) return fail(i);
                    if ((c2 & 0xC0) != 0x80) return fail(i);
                    if ((c3 & 0xC0) != 0x80) return fail(i);
                    i += 4;
                    continue;
                }

                if (c == 0xF4) {
                    if (i + 3 >= n) return fail(i);
                    unsigned char c1 = data[i + 1];
                    unsigned char c2 = data[i + 2];
                    unsigned char c3 = data[i + 3];
                    if (c1 < 0x80 || c1 > 0x8F) return fail(i);
                    if ((c2 & 0xC0) != 0x80) return fail(i);
                    if ((c3 & 0xC0) != 0x80) return fail(i);
                    i += 4;
                    continue;
                }

                return fail(i);
            }
            return true;
        }

        void append_utf8(uint32_t cp, string& out) {
            if (cp <= 0x7F) {
                out.push_back(static_cast<char>(cp));
            } else if (cp <= 0x7FF) {
                out.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            } else if (cp <= 0xFFFF) {
                out.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            } else if (cp <= 0x10FFFF) {
                out.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            } else {
                // Invalid codepoint; replace with replacement character
                append_utf8(0xFFFDu, out);
            }
        }

        expected_void skip_ws_and_comments(Scanner& s) {
            while (!s.eof()) {
                char c = s.peek();

                if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                    s.get();
                    continue;
                }

                if (!s.opts.allow_comments || c != '/') break;

                char next = s.peek_next();
                if (next == '/') {
                    // Line comment
                    s.get();
                    s.get();
                    while (!s.eof() && s.peek() != '\n') {
                        s.get();
                    }
                    continue;
                } else if (next == '*') {
                    s.get();
                    s.get();
                    bool closed = false;
                    while (!s.eof()) {
                        char ch = s.get();
                        if (ch == '*' && s.peek() == '/') {
                            s.get();
                            closed = true;
                            break;
                        }
                    }
                    if (!closed) return std::unexpected(s.make_error(ParseError::code::unexpected_end_of_input, "Nonterminated block comment"));
                    continue;
                } else {
                    break;
                }
            }
            return {};
        }

        expected_void parse_literal(Scanner& s, std::string_view literal, ParseError::code code, std::string_view fail_msg) {
            for (char expected : literal) {
                if (s.eof()) return std::unexpected(s.make_error(ParseError::code::unexpected_end_of_input, fail_msg));
                char c = s.get();
                if (c != expected) return std::unexpected(s.make_error(code, fail_msg));
            }
            return {};
        }

        expected_t<string> parse_string(Scanner& s) {
            if (!s.consume('"')) return std::unexpected(s.make_error(ParseError::code::invalid_string, "Expected '\"' to start a string"));
            
            string out{ Sonnet::allocator_type(s.mem_res) };

            while (!s.eof()) {
                char c = s.get();
                if (c == '"') { 
                    size_t bad_idx = 0;
                    if (!detail::is_valid_utf8(std::string_view(out.data(), out.size()), bad_idx)) 
                        return std::unexpected(s.make_error(ParseError::code::invalid_string, "Invalid UTF-8 sequence in string")); 
                    return out; 
                }
                if (static_cast<unsigned char>(c) < 0x20) return std::unexpected(s.make_error(ParseError::code::invalid_string, "Control character in string"));
                if (c == '\\') {
                    if (s.eof()) return std::unexpected(s.make_error(ParseError::code::invalid_escape, "Unfinished escape sequence"));
                    char esc = s.get();
                    switch (esc) {
                        case '"': out.push_back('"'); break;
                        case '\\': out.push_back('\\'); break;
                        case '/': out.push_back('/'); break;
                        case 'b': out.push_back('\b'); break;
                        case 'f': out.push_back('\f'); break;
                        case 'n': out.push_back('\n'); break;
                        case 'r': out.push_back('\r'); break;
                        case 't': out.push_back('\t'); break;
                        case 'u': {
                            auto parse_hex4 = [&](uint16_t& out_code) -> expected_void {
                                uint16_t val = 0;
                                for (int i = 0; i < 4; i++) {
                                    if (s.eof()) return std::unexpected(s.make_error(ParseError::code::invalid_unicode_escape, "Unexpected end in unicode escape"));
                                    char h = s.get();
                                    unsigned digit = 0;
                                    if (h >= '0' && h <= '9') digit = h - '0';
                                    else if (h >= 'A' && h <= 'F') digit = 10 + (h - 'A');
                                    else if (h >= 'a' && h <= 'f') digit = 10 + (h - 'a');
                                    else return std::unexpected(s.make_error(ParseError::code::invalid_unicode_escape, "Invalid hex digit in unicode escape"));
                                    val = static_cast<uint16_t>((val << 4) | digit);
                                }
                                out_code = val;
                                return {};
                            };

                            uint16_t first = 0;
                            if (auto r = parse_hex4(first); !r) return std::unexpected(r.error());

                            uint32_t codepoint = 0;

                            if (first >= 0xD800 && first <= 0xDBFF) {
                                if (!(s.consume('\\') && s.consume('u'))) return std::unexpected(s.make_error(ParseError::code::invalid_unicode_escape, "Expected low surrogate after high surrogate"));
                                uint16_t second = 0;
                                if (auto r = parse_hex4(second); !r) return std::unexpected(r.error());
                                if (!(second >= 0xDC00 && second <= 0xDFFF)) return std::unexpected(s.make_error(ParseError::code::invalid_unicode_escape, "Invalid low surrogate"));
                                codepoint = 0x10000u + ((static_cast<uint32_t>(first - 0xD800) << 10) | (static_cast<uint32_t>(second - 0xDC00)));
                            } else if (first >= 0xDC00 && first <= 0xDFFF) return std::unexpected(s.make_error(ParseError::code::invalid_unicode_escape, "Unpaired low surrogate"));
                            else codepoint = first;

                            append_utf8(codepoint, out);
                            break;
                        }
                    default: return std::unexpected(s.make_error(ParseError::code::invalid_escape, "Invalid escape sequence"));
                    }
                } else {
                    out.push_back(c);
                }
            }

            return std::unexpected(s.make_error(ParseError::code::unexpected_end_of_input, "Nonterminated string"));
        }

        expected_t<double> parse_number(Scanner& s) {
            size_t start = s.idx;

            char c = s.peek();
            if (c == '-') {
                s.get();
                if (!std::isdigit(static_cast<unsigned char>(s.peek()))) return std::unexpected(s.make_error(ParseError::code::unexpected_character, "Expected digit after '-'"));
            }

            char first_digit = s.get();
            if (!std::isdigit(static_cast<unsigned char>(first_digit))) return std::unexpected(s.make_error(ParseError::code::invalid_number, "Expected digit"));
            if (first_digit == '0' && std::isdigit(static_cast<unsigned char>(s.peek()))) return std::unexpected(s.make_error(ParseError::code::invalid_number, "Leading zeros disallowed"));
            while (std::isdigit(static_cast<unsigned char>(s.peek()))) s.get();
            
            if (s.peek() == '.') {
                s.get();
                if (!std::isdigit(static_cast<unsigned char>(s.peek()))) return std::unexpected(s.make_error(ParseError::code::invalid_number, "Expected digit after '.'"));
                while (std::isdigit(static_cast<unsigned char>(s.peek()))) s.get();
            }

            char p = s.peek();
            if (p == 'e' || p == 'E') {
                s.get();
                char sign = s.peek();
                if (sign == '+' || sign =='-') { 
                    s.get();
                    c = s.peek();
                }
                if (!std::isdigit(static_cast<unsigned char>(s.peek()))) return std::unexpected(s.make_error(ParseError::code::invalid_number, "Expected digit in exponent"));
                do {
                    s.get();
                    c = s.peek();
                } while (std::isdigit(static_cast<unsigned char>(c)));

                auto is_ws = [](char ch) { return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r'; };
                if (!(c == '\0' || c == ',' || c == ']' || c == '}' || is_ws(c))) return std::unexpected(s.make_error(ParseError::code::invalid_number, "Invalid character in exponent"));
            }

            size_t end = s.idx;
            auto num_sv = s.text.substr(start, end - start);
            double res = 0.0;
            auto fc_res = std::from_chars(num_sv.data(), num_sv.data() + num_sv.size(), res);
            if (fc_res.ec != std::errc{}) return std::unexpected(s.make_error(ParseError::code::invalid_number, "Failed to parse number"));
            return res;
        }

        expected_t<value> parse_array(Scanner& s) {
            DepthGuard guard{ s };
            if (s.max_depth != 0 && !guard.ok()) return std::unexpected(s.make_error(ParseError::code::depth_limit_exceeded, "Maximum nesting depth exceeded"));
            if (!s.consume('[')) return std::unexpected(s.make_error(ParseError::code::unexpected_character, "Expected '[' to start array"));

            array arr{ Sonnet::allocator_type(s.mem_res) };

            if (auto ws = skip_ws_and_comments(s); !ws) return std::unexpected(ws.error());
            if (s.consume(']')) return value{ std::move(arr), s.mem_res };

            while (true) {
                auto elem = parse_value(s);
                if (!elem) return std::unexpected(std::move(elem.error()));
                arr.emplace_back(std::move(*elem));

                if (auto ws = skip_ws_and_comments(s); !ws) return std::unexpected(ws.error());

                char c = s.peek();
                if (c == ',') {
                    s.get();
                    if (auto ws = skip_ws_and_comments(s); !ws) return std::unexpected(ws.error());
                    
                    char next = s.peek();
                    if (next == ']') {
                        if (s.opts.allow_trailing_commas) {
                            s.get();
                            break;
                        } else return std::unexpected(s.make_error(ParseError::code::trailing_characters, "Trailing commas not allowed"));
                    }
                    continue;
                }
                if (c == ']') { s.get(); break; }
                if (c == '\0') return std::unexpected(s.make_error(ParseError::code::unexpected_end_of_input, "Unterminated array, expected ',' or ']'"));
                return std::unexpected(s.make_error(ParseError::code::unexpected_character, "Expected ',' or ']' in array"));
            }
            return value{ std::move(arr), s.mem_res };
        }

        expected_t<value> parse_object(Scanner& s) {
            DepthGuard guard{ s };
            if (s.max_depth != 0 && !guard.ok()) return std::unexpected(s.make_error(ParseError::code::depth_limit_exceeded, "Maximum nesting depth exceeded"));
            if (!s.consume('{')) return std::unexpected(s.make_error(ParseError::code::unexpected_character, "Expected '{' to start object"));
            object obj{ std::less<>{}, Sonnet::allocator_type(s.mem_res) };
            if (auto ws = skip_ws_and_comments(s); !ws) return std::unexpected(ws.error());
            if (s.consume('}')) return value{ std::move(obj), s.mem_res };
            while (true) {
                char c = s.peek();
                if (c == '\0') return std::unexpected(s.make_error(ParseError::code::unexpected_end_of_input, "Unterminted object, expected '}' or string key"));
                if (c != '"') return std::unexpected(s.make_error(ParseError::code::unexpected_character, "Expected \" to start object key"));
                auto key_val = parse_string(s);
                if (!key_val) return std::unexpected(key_val.error());
                string key = std::move(*key_val);
                if (auto ws = skip_ws_and_comments(s); !ws) return std::unexpected(ws.error());
                c = s.peek();
                if (c == '\0') return std::unexpected(s.make_error(ParseError::code::unexpected_end_of_input, "Unterminated object, expected ':' after key"));
                if (c != ':') return std::unexpected(s.make_error(ParseError::code::unexpected_character, "Expected ':' after object key"));
                s.get();
                if (auto ws = skip_ws_and_comments(s); !ws) return std::unexpected(ws.error());
                auto val = parse_value(s);
                if (!val) return std::unexpected(val.error());
                obj[std::move(key)] = std::move(*val); // Enforce last-wins here
                if (auto ws = skip_ws_and_comments(s); !ws) return std::unexpected(ws.error());
                c = s.peek();
                if (c == ',') {
                    s.get();
                    if (auto ws = skip_ws_and_comments(s); !ws) return std::unexpected(ws.error());
                    if (s.opts.allow_trailing_commas && s.peek() == '}') { s.get(); break; }
                    continue;
                }
                if (c == '}') { s.get(); break; }
                if (c == '\0') return std::unexpected(s.make_error(ParseError::code::unexpected_end_of_input, "Unterminated object, expected ',' or '}'"));
                return std::unexpected(s.make_error(ParseError::code::unexpected_character, "Expected ',' or '}' in object"));
            }
            return value{ std::move(obj), s.mem_res };
        }

        expected_t<value> parse_value(Scanner& s) {
            if (auto ws = skip_ws_and_comments(s); !ws) return std::unexpected(ws.error());
            if (s.eof()) return std::unexpected(s.make_error(ParseError::code::unexpected_end_of_input, "Expected JSON value"));
            char c = s.peek();
            switch (c) {
            case 'n': {
                if (auto r = parse_literal(s, "null", ParseError::code::unexpected_character, "Invalid 'null' literal"); !r) return std::unexpected(r.error());
                return value{ nullptr, s.mem_res };
            } 
            case 't': {
                if (auto r = parse_literal(s, "true", ParseError::code::unexpected_character, "Invalid 'true' literal"); !r) return std::unexpected(r.error());
                return value{ true, s.mem_res };
            }
            case 'f': {
                if (auto r = parse_literal(s, "false", ParseError::code::unexpected_character, "Invalid 'false' literal"); !r) return std::unexpected(r.error());
                return value{ false, s.mem_res };
            }
            case '"': {
                auto str = parse_string(s);
                if (!str) return std::unexpected(str.error());
                return value{ std::move(*str), s.mem_res };
            }
            case '[': return parse_array(s);
            case '{': return parse_object(s);
            default:
                if (c == '-' || (c >= '0' && c <= '9')) {
                    auto num = parse_number(s);
                    if (!num) return std::unexpected(num.error());
                    return value{ *num, s.mem_res };
                }
                else if (c == '.') return std::unexpected(s.make_error(ParseError::code::invalid_number, "Fractional values must start with a 0"));
                return std::unexpected(s.make_error(ParseError::code::unexpected_character, "Unexpected character while parsing value"));
            }
        }

        ParseResult parse_impl(std::string_view text, const ParseOptions& opts) {
            std::pmr::memory_resource* res = std::pmr::get_default_resource();
            Scanner s{ text, opts, res };

            auto v = parse_value(s);
            if (!v) return std::unexpected(v.error());
            if (auto ws = skip_ws_and_comments(s); !ws) return std::unexpected(ws.error());
            if (!s.eof()) return std::unexpected(s.make_error(ParseError::code::trailing_characters, "Trailing characters after top-level JSON value"));
            return *std::move(v);
        }
#pragma endregion
#pragma region Serializer

        // ================================
        // Internal serializer implementation
        // ================================

        void dump_string(const string& s, std::ostream& os) {
            os.put('"');
            for (unsigned char c : s) {
                switch (c) {
                case '"': os << "\\\""; break;
                case '\\': os << "\\\\"; break;
                case '\b': os << "\\b"; break;
                case '\f': os << "\\f"; break;
                case '\n': os << "\\n"; break;
                case '\r': os << "\\r"; break;
                case '\t': os << "\\t"; break;
                default:
                    if (c < 0x20) {
                        // control characters -> \u00XX
                        static constexpr char hex[] = "0123456789ABCDEF";
                        os << "\\u00" << hex[(c >> 4) & 0xF] << hex[c & 0xF];
                    } else {
                        os.put(static_cast<char>(c));
                    }
                    break;
                }
            }
            os.put('"');
        }

        void dump_indent(std::ostream& os, size_t depth, const WriteOptions& opts) {
            if (!opts.pretty || opts.indent == 0) return;
            size_t spaces = depth * opts.indent;
            for (size_t i = 0; i < spaces; i++) os.put(' ');
        }

        void dump_impl(const value& v, std::ostream& os, const WriteOptions& opts, size_t depth) {
            switch (v.type()) {
            case kind::null: os << "null"; return;
            case kind::boolean: os << (v.as_bool() ? "true" : "false"); return;
            case kind::number: {
                double d = v.as_number();
                if (!std::isfinite(d)) {
                    os << "null"; 
                    return;
                }

                char buf[64];
                auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), d, std::chars_format::general);
                if (ec != std::errc{}) os << "0"; // in case something goes wrong
                else os.write(buf, ptr - buf);
                return;
            }
            case kind::string: {
                dump_string(v.as_string(), os);
                return;
            }
            case kind::array: {
                const auto& arr = v.as_array();
                size_t n = arr.size();

                os.put('[');
                if (n == 0) {
                    os.put(']');
                    return;
                }

                if (opts.pretty) os.put('\n');
                for (size_t i = 0; i < n; i++) {
                    if (opts.pretty) dump_indent(os, depth + 1, opts);
                    dump_impl(arr[i], os, opts, depth + 1);
                    if (i + 1 < n) os.put(',');
                    if (opts.pretty) os.put('\n');
                }
                if (opts.pretty) dump_indent(os, depth, opts);
                os.put(']');
                return;
            }
            case kind::object: {
                const auto& obj = v.as_object();
                size_t n = obj.size();

                os.put('{');
                if (n == 0) {
                    os.put('}');
                    return;
                }

                // Note: object is a std::pmr::map so keys are already sorted by
                // lexicographical order; write_options::sort_keys currently
                // doesn't change behavior, but it's there for future unordered_map.
                if (opts.pretty) os.put('\n');

                size_t i = 0;
                for (const auto& [k, val] : obj) {
                    if (opts.pretty) dump_indent(os, depth + 1, opts);
                    dump_string(k, os);
                    os << (opts.pretty ? ": " : ":");
                    dump_impl(val, os, opts, depth + 1);
                    if (i + 1 < n) os.put(',');
                    if (opts.pretty) os.put('\n');
                    i++;
                }
                if (opts.pretty) dump_indent(os, depth, opts);
                os.put('}');
                return;
            }
            }
            os << "null";
        }

#pragma endregion

    } // namespace detail

} // namespace Sonnet