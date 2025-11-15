#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

#include "sonnet/sonnet.hpp"

#include <random>
#include <limits>
#include <print>

using namespace Catch;

namespace {
  
    struct rng {
        std::mt19937_64 eng;
        
        rng() : eng(std::random_device{}()) {}
        
        size_t uniform_size(size_t min, size_t max) {
            std::uniform_int_distribution<size_t> dist(min, max);
            return dist(eng);
        }
        
        bool coin(double p = 0.5) {
            std::bernoulli_distribution dist(p);
            return dist(eng);
        }
        
        double uniform_double() {
            std::uniform_real_distribution dist(-1e6, 1e6);
            return dist(eng);
        }
        
        char ascii_char() {
            std::uniform_int_distribution<int> dist(32, 126);
            return static_cast<char>(dist(eng));
        }
        
        std::string random_string(size_t max_len = 16) {
            size_t len = uniform_size(0, max_len);
            std::string s;
            s.reserve(len);
            for (size_t i = 0; i < len; i++) 
            s.push_back(ascii_char());
            return s;
        }
    };
    
    Sonnet::value random_json_value(rng& r, int depth = 0, int max_depth = 4);
    
    Sonnet::value random_primitive(rng& r) {
        switch (r.uniform_size(0, 4)) {
            case 0: return Sonnet::value{ nullptr };
            case 1: return Sonnet::value{ r.coin() };
            case 2: return Sonnet::value{ r.uniform_double() };
            case 3: {
                auto s = r.random_string();
                return Sonnet::value{ s.c_str() };
            }
        }
        return Sonnet::value{ nullptr };
    }
    
    Sonnet::value random_array(rng& r, int depth, int max_depth) {
        auto res = Sonnet::value{};
        auto& arr = res.as_array();
        size_t n = r.uniform_size(0, 8);
        for (size_t i = 0; i < n; i++) 
        arr.emplace_back(random_json_value(r, depth + 1, max_depth));
        return res;
    }
    
    Sonnet::value random_object(rng& r, int depth, int max_depth) {
        auto res = Sonnet::value{};
        auto& obj = res.as_object();
        size_t n = r.uniform_size(0, 8);
        for (size_t i = 0; i < n; i++) {
            auto key = r.random_string();
            auto value = random_json_value(r, depth + 1, max_depth);
            obj.emplace(Sonnet::string{ key.c_str(), res.resource() }, std::move(value));
        }
        return res;
    }
    
    Sonnet::value random_json_value(rng& r, int depth, int max_depth) {
        if (depth >= max_depth) return random_primitive(r);
        
        size_t choice = r.uniform_size(0, 5);
        switch (choice) {
            case 0:
            case 1:
            return random_primitive(r);
            case 2:
            case 3:
            return random_array(r, depth, max_depth);
            case 4:
            case 5:
            return random_object(r, depth, max_depth);
        }
        return random_primitive(r);
    }

    static Sonnet::ParseResult parse_str(std::string_view s, const Sonnet::ParseOptions& opts = {}) {
        return Sonnet::parse(s, opts);
    }

    static void expect_ok(std::string_view s, const Sonnet::ParseOptions& opts = {}) {
        auto r = parse_str(s, opts);
        REQUIRE(r);
    }

    static void expect_fail(std::string_view s, Sonnet::ParseError::code code, const Sonnet::ParseOptions& opts = {}) {
        auto r = parse_str(s, opts);
        REQUIRE_FALSE(r);
        REQUIRE(r.error().errc == code);
    }
}


TEST_CASE("DOM Dump/parse Round-Trip") {
    rng r;
    
    for (int i = 0; i < 100; i++) {
        Sonnet::value original = random_json_value(r);
        
        Sonnet::WriteOptions opts;
        opts.pretty = (i % 2 == 0);
        
        std::string s = Sonnet::dump(original, opts);
        
        auto parsed = Sonnet::parse(s);
        REQUIRE(parsed.has_value());

        const Sonnet::value& reparsed = parsed.value();

        if (reparsed != original) {
            std::println("Reparsed and Original do not match!");
            std::println("Reparsed: {}", Sonnet::dump(reparsed, {}));
            std::println("Original: {}", Sonnet::dump(original, {}));
        }
        
        REQUIRE(reparsed == original);
    }
}

TEST_CASE("Parse/Dump/Parse Property on Random Text") {
    rng r;

    for (int i = 0; i < 100; i++) {
        std::string input = r.random_string(64);

        Sonnet::ParseOptions opts;
        opts.allow_comments = true;
        opts.allow_trailing_commas = true;

        auto res = Sonnet::parse(input, opts);
        if (!res.has_value()) continue; 

        Sonnet::value& v = res.value();

        std::string dumped = Sonnet::dump(v, {.pretty = (i % 2 == 0)});
        auto res2 = Sonnet::parse(dumped, opts);
        REQUIRE(res2.has_value());
        REQUIRE(res2.value() == v);
    }
}

TEST_CASE("Parse Primitives") {
    using Sonnet::parse;

    auto n = parse("null");
    REQUIRE(n);
    REQUIRE(n->is_null());

    auto t = parse("true");
    REQUIRE(t);
    REQUIRE(t->is_bool());
    REQUIRE(t->as_bool() == true);

    auto num = parse("123.5e-1");
    REQUIRE(num);
    REQUIRE(num->as_number() == Approx(12.35));
}

TEST_CASE("Parse String Escapes") {
    using Sonnet::parse;

    auto r = parse(R"("line\nbreak")");
    REQUIRE(r);
    REQUIRE(r->as_string() == "line\nbreak");

    auto unicode = parse(R"("\u20AC")");
    REQUIRE(unicode);
    REQUIRE(unicode->as_string().size() > 0);
}

TEST_CASE("Reject Leading Zeros") {
    using Sonnet::parse;
    
    auto r = parse("01");
    REQUIRE_FALSE(r.has_value());
    REQUIRE(r.error().errc == Sonnet::ParseError::code::invalid_number);
}

TEST_CASE("Reject Trailing Characters") {
    using Sonnet::parse;

    auto r = parse("1 2");
    REQUIRE_FALSE(r.has_value());
    REQUIRE(r.error().errc == Sonnet::ParseError::code::trailing_characters);
}

TEST_CASE("Empty Array and Object Round-Trip") {
    Sonnet::value arr;
    auto _ = arr.as_array();

    Sonnet::value obj;
    auto _ = obj.as_object();

    auto r1 = Sonnet::parse(Sonnet::dump(arr));
    auto r2 = Sonnet::parse(Sonnet::dump(obj));

    REQUIRE(r1);
    REQUIRE(r1->is_array());
    REQUIRE(r1->as_array().empty());

    REQUIRE(r2);
    REQUIRE(r2->is_object());
    REQUIRE(r2->as_object().empty());
}

TEST_CASE("Object Operator[] Inserts Keys") {
    Sonnet::value v;
    v["x"] = 1.0;

    REQUIRE(v.is_object());
    REQUIRE(v["x"].as_number() == Approx(1.0));
}

TEST_CASE("Array Operator[] Grows and Fills with Null") {
    Sonnet::value v;
    v["a"][3] = 42.0;

    auto& arr = v["a"].as_array();
    REQUIRE(arr.size() == 4);
    REQUIRE(arr[0].is_null());
    REQUIRE(arr[3].as_number() == Approx(42.0));
}

TEST_CASE("Line and Block Comments Are Accepted When Allowed") {
    std::string s = R"(
        // comment
        {
            "x": 1, /* comment */ "y": 2
        }
    )";

    Sonnet::ParseOptions opts;
    opts.allow_comments = true;

    auto r = Sonnet::parse(s, opts);
    REQUIRE(r);
    REQUIRE(r->as_object().size() == 2);
}

TEST_CASE("Comments Rejected When Not Allowed") {
    std::string s = "{ // comment\n \"x\": 1 }";

    Sonnet::ParseOptions opts;
    opts.allow_comments = false;

    auto r = Sonnet::parse(s, opts);
    REQUIRE_FALSE(r.has_value());
}

TEST_CASE("Trailing Comments Controlled by Option") {
    std::string s = "{ \"a\": 1, }";

    Sonnet::ParseOptions strict;
    strict.allow_trailing_commas = false;

    auto strict_r = Sonnet::parse(s, strict);
    REQUIRE_FALSE(strict_r);

    Sonnet::ParseOptions relaxed;
    relaxed.allow_trailing_commas = true;

    auto relaxed_r = Sonnet::parse(s, relaxed);
    REQUIRE(relaxed_r);
}

TEST_CASE("Valid Surrogate Pair Parses") {
    auto r = Sonnet::parse(R"("\uD83D\uDE00")");
    REQUIRE(r);
    auto& s = r->as_string();
    REQUIRE(!s.empty());
}

TEST_CASE("Unpaired Surrogate Rejected") {
    auto r = Sonnet::parse(R"("\uD83D")");
    REQUIRE_FALSE(r);
    REQUIRE(r.error().errc == Sonnet::ParseError::code::invalid_unicode_escape);
}

TEST_CASE("Large Exponent Parses") {
    auto r = Sonnet::parse("1e308");
    REQUIRE(r);
    REQUIRE(std::isfinite(r->as_number()));
}

TEST_CASE("NaN and Inf Serialize as Null") {
    Sonnet::value v_nan{ std::numeric_limits<double>::quiet_NaN() };
    Sonnet::value v_inf{ std::numeric_limits<double>::infinity() };

    REQUIRE(Sonnet::dump(v_nan) == "null");
    REQUIRE(Sonnet::dump(v_inf) == "null");
}

TEST_CASE("Error Position in Range") {
    std::string s = "{\n  \"x\": 1,\n  oops\n}";
    auto r = Sonnet::parse(s);
    REQUIRE_FALSE(r);

    const auto& e = r.error();
    REQUIRE(e.offset <= s.size());
    REQUIRE(e.line >= 1);
    REQUIRE(e.column >= 1);
    REQUIRE_FALSE(e.msg.empty());
}

TEST_CASE("Value Equality is Structural") {
    Sonnet::value a;
    a["x"] = 1.0;
    a["y"].as_array().emplace_back(true);
    
    Sonnet::value b;
    b["x"] = 1.0;
    b["y"].as_array().emplace_back(true);
    
    REQUIRE(a == b);
}

TEST_CASE("Regression: Empty Array Round-Trip") {
    std::string s = "[]";
    auto r1 = Sonnet::parse(s);
    REQUIRE(r1);
    auto s2 = Sonnet::dump(r1.value());
    auto r2 = Sonnet::parse(s2);
    REQUIRE(r2);
    REQUIRE(r1.value() == r2.value());
}

struct CountingResource : std::pmr::memory_resource {
    size_t allocs = 0;
    size_t deallocs = 0;

    void* do_allocate(size_t bytes, size_t alignment) override {
        allocs++;
        return std::pmr::get_default_resource()->allocate(bytes, alignment);
    }

    void do_deallocate(void* p, size_t bytes, size_t alignment) override {
        deallocs++;
        return std::pmr::get_default_resource()->deallocate(p, bytes, alignment);
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

TEST_CASE("Value Uses Provided memory_resource") {
    CountingResource res;
    Sonnet::value v{ &res };

    v["key"] = "value";
    v["arr"].as_array().emplace_back(123.0);

    REQUIRE(res.allocs > 0);
}

TEST_CASE("as_array Converts Null to Empty Array") {
    Sonnet::value v;
    REQUIRE(v.is_null());
    auto& arr = v.as_array();
    REQUIRE(v.is_array());
    REQUIRE(arr.empty());    
}

TEST_CASE("operator[] String Inserts Null When Missing") {
    Sonnet::value v;
    v["foo"];
    REQUIRE(v.is_object());
    REQUIRE(v["foo"].is_null());
}

TEST_CASE("operator[] Index Grows and Fills With Null") {
    Sonnet::value v;
    v[3] = 42.0;
    auto& arr = v.as_array();
    REQUIRE(arr.size() == 4);
    REQUIRE(arr[0].is_null());
    REQUIRE(arr[3].as_number() == Approx(42.0));
}

TEST_CASE("RFC8259 - Top-Level Single Value with Whitespace") {
    expect_ok(" 42 ");
    expect_ok("\n\n {\"a\":1}  \t");
    expect_ok("[1, 2, 3]");
    expect_ok("null");
    expect_ok("\"string\"");
}

TEST_CASE("RFC8259 - Trailing Characters are Rejected") {
    expect_fail("null true", Sonnet::ParseError::code::trailing_characters);
    expect_fail("{\"a\":1} 0", Sonnet::ParseError::code::trailing_characters);
    expect_fail("[] [ ]", Sonnet::ParseError::code::trailing_characters);
}

TEST_CASE("RFC8259 - Allowed Whitespace is Accepted") {
    expect_ok(" \t\r\n null \t\n ");
    expect_ok("\r\n  [ \n 1 , 2 \t ] \r");
}

TEST_CASE("RFC8259 - non-JSON Whitespace is Rejected") {
    std::string s = "\xC2\xA0\x01";
    auto r = parse_str(s);
    REQUIRE_FALSE(r);
}

TEST_CASE("RFC8259 - Valid Numbers") {
    for (auto s : {
        "0",
        "123",
        "-0",
        "-123",
        "0.0",
        "-0.1",
        "10.5",
        "1e10",
        "1E10",
        "1e+10",
        "1e-10",
        "-1E-10"
    }) {
        INFO("parsing: " << s);
        auto r = parse_str(s);
        REQUIRE(r);
        REQUIRE(r->is_number());
    }
}

TEST_CASE("RFC8259 - Invalid Numbers are Rejected") {
    // Leading zeros
    expect_fail("01",     Sonnet::ParseError::code::invalid_number);
    expect_fail("-01",    Sonnet::ParseError::code::invalid_number);

    // Trailing decimal point / no digits
    expect_fail("1.",     Sonnet::ParseError::code::invalid_number);
    expect_fail("1.e10",  Sonnet::ParseError::code::invalid_number);
    expect_fail(".5",     Sonnet::ParseError::code::invalid_number);

    // Malformed exponent
    expect_fail("1e",     Sonnet::ParseError::code::invalid_number);
    expect_fail("1e+",    Sonnet::ParseError::code::invalid_number);
    expect_fail("1e-",    Sonnet::ParseError::code::invalid_number);
    expect_fail("1e1.2",  Sonnet::ParseError::code::invalid_number);

    // Leading plus not allowed
    expect_fail("+1",     Sonnet::ParseError::code::unexpected_character);
}

TEST_CASE("RFC8259 - Valid String Escapes") {
    expect_ok("\"simple\"");
    expect_ok("\"quote: \\\"\"");
    expect_ok("\"backslash: \\\\\"");
    expect_ok("\"controls: \\b\\f\\n\\r\\t\"");
    expect_ok("\"solidus: \\/\"");
}

TEST_CASE("RFC8259 - Control Characters Must be Escaped") {
    std::string s = "\"Hello\nWorld\""; // raw LF inside
    expect_fail(s, Sonnet::ParseError::code::invalid_string);

    std::string s2 = "\"\x01\""; // raw control char
    expect_fail(s2, Sonnet::ParseError::code::invalid_string);
}

TEST_CASE("RFC8259 - Valid Unicode Escapes") {
    // Basic Latin
    expect_ok("\"hello \\u0041\""); // A

    // Surrogate pair (😀 = U+1F600 = \uD83D\uDE00)
    expect_ok("\"emoji: \\uD83D\\uDE00\"");
}

TEST_CASE("RFC8259 - Invalid Unicode Escapes") {
    // Not enough hex digits
    expect_fail("\"\\u12\"", Sonnet::ParseError::code::invalid_unicode_escape);

    // Non-hex chars
    expect_fail("\"\\uZZZZ\"", Sonnet::ParseError::code::invalid_unicode_escape);

    // Lone high surrogate
    expect_fail("\"\\uD800\"", Sonnet::ParseError::code::invalid_unicode_escape);

    // High surrogate not followed by low surrogate
    expect_fail("\"\\uD800abc\"", Sonnet::ParseError::code::invalid_unicode_escape);
}

TEST_CASE("RFC8259 - Basic Arrays") {
    expect_ok("[]");
    expect_ok("[1, 2, 3]");
    expect_ok("[true, false, null, \"x\", {\"a\":1}, [2]]");
}

TEST_CASE("RFC8259 - Invalid Arrays") {
    expect_fail("[", Sonnet::ParseError::code::unexpected_end_of_input);
    expect_fail("[1", Sonnet::ParseError::code::unexpected_end_of_input);
    expect_fail("[1,", Sonnet::ParseError::code::unexpected_end_of_input);
    expect_fail("[1 2]", Sonnet::ParseError::code::unexpected_character);
    expect_fail("[,1]", Sonnet::ParseError::code::unexpected_character);
}

TEST_CASE("RFC8259 - Trailing Comma Behavior") {
    Sonnet::ParseOptions strict{};
    strict.allow_trailing_commas = false;
    (void)strict; // default behavior

    Sonnet::ParseOptions relaxed{};
    relaxed.allow_trailing_commas = true;

    // Disallowed in strict
    expect_fail("[1,]", Sonnet::ParseError::code::trailing_characters /* or trailing_comma_not_allowed, depending on your design */);

    // Allowed in relaxed
    auto r = Sonnet::parse("[1,]", relaxed);
    REQUIRE(r);
    REQUIRE(r->is_array());
    REQUIRE(r->as_array().size() == 1);
}

TEST_CASE("RFC8259 - Basic Objects") {
    expect_ok("{}");
    expect_ok("{\"a\":1}");
    expect_ok("{\"a\":1,\"b\":2}");
    expect_ok("{\"nested\":{\"x\":true},\"arr\":[1,2]}");
}

TEST_CASE("RFC8259 - Invalid Objects") {
    expect_fail("{", Sonnet::ParseError::code::unexpected_end_of_input);
    expect_fail("{\"a\":1", Sonnet::ParseError::code::unexpected_end_of_input);
    expect_fail("{\"a\":1,", Sonnet::ParseError::code::unexpected_end_of_input);
    expect_fail("{a:1}", Sonnet::ParseError::code::unexpected_character); // key must be string
    expect_fail("{\"a\" 1}", Sonnet::ParseError::code::unexpected_character); // missing colon
    expect_fail("{,\"a\":1}", Sonnet::ParseError::code::unexpected_character);
}

TEST_CASE("RFC8259 - Object Duplicate Names Last-Wins Semantics") {
    auto r = parse_str("{\"a\":1,\"a\":2}");
    REQUIRE(r);
    auto& obj = r->as_object();
    REQUIRE(obj.size() == 1);
    REQUIRE(obj.at("a").as_number() == Approx(2.0));
}

TEST_CASE("RFC8259 - Valid UTF-8 in Strings") {
    // Some multi-byte characters
    expect_ok("\"caf\xC3\xA9\"");           // café
    expect_ok("\"snowman: \xE2\x98\x83\""); // ☃

}

TEST_CASE("RFC8259 - Invalid UTF-8 is Rejected") {
    // Overlong encoding of '/'
    std::string s = "\"\xC0\xAF\"";
    expect_fail(s, Sonnet::ParseError::code::invalid_string);
    // errc might be invalid_string or invalid_unicode_escape or unexpected_character,
    // depending on how you classify UTF-8 errors.
}

TEST_CASE("RFC8259 - NaN and Infinity tokens are rejected") {
    expect_fail("NaN",      Sonnet::ParseError::code::unexpected_character);
    expect_fail("Infinity", Sonnet::ParseError::code::unexpected_character);
    expect_fail("-Infinity", Sonnet::ParseError::code::unexpected_character);
}

TEST_CASE("Empty Input is Rejected") {
    expect_fail("", Sonnet::ParseError::code::unexpected_end_of_input);
}

TEST_CASE("Whitespace-only Input is Rejected") {
    expect_fail("   \n\t  ", Sonnet::ParseError::code::unexpected_end_of_input);
}

TEST_CASE("Minus Zero (-0) Parses as Number") {
    auto r = Sonnet::parse("-0");
    REQUIRE(r);
    REQUIRE(r->is_number());
    REQUIRE(r->as_number() == Approx(0.0));
}

TEST_CASE("Leading Plus Sign is Rejected") {
    expect_fail("+1", Sonnet::ParseError::code::unexpected_character);
}

TEST_CASE("Empty String Parses") {
    auto r = Sonnet::parse("\"\"");
    REQUIRE(r);
    REQUIRE(r->is_string());
    REQUIRE(r->as_string().empty());
}

TEST_CASE("String With Multiple Escape Kinds Parses") {
    auto r = Sonnet::parse("\"line1\\nline2\\t\\\"q\\\"\\\\\"");
    REQUIRE(r);
    REQUIRE(r->is_string());
}

TEST_CASE("Max depth is enforced") {
    Sonnet::ParseOptions opts{};
    opts.max_depth = 3;

    expect_ok("[[[]]]", opts);
    expect_fail("[[[[]]]]", Sonnet::ParseError::code::depth_limit_exceeded, opts);
    expect_ok("{ \"1\": { \"2\": {}}}", opts);
    expect_fail("{ \"1\": { \"2\": { \"3\": {}}}}", Sonnet::ParseError::code::depth_limit_exceeded, opts);
}