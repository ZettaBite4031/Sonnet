#pragma once


/*
    ----------------------------------------------------
    Sonnet type conversion utilities = to_json/from_json
    ----------------------------------------------------
    This header defines the customization points and helpers used to
    convert between `Sonnet::value` and user-defined C++ types

    -----
    Goals
    -----
    - Provide a simple way to serialize/deserialize custom types without
      tying Sonnet to any particular reflection or metaobject system.
    - Mirror the familiar `to_json`/`from_json` pattern used in other
      JSON libraries, while remaining allocator- and error-aware

    ----------
    Core Ideas
    ----------
    - For a user-defined type `T`, you can define:

        // Serialize T into Sonnet::value
        void to_json(Sonnet::value& out, const T& src);

        // Deserialize T from a Sonnet::value
        void from_json(Sonnet::value& src, T& out);
    
      in either the same namespace as `T` or in the `Sonnet` namespace,
      subject to argument-dependent lookup (ADL)
    - Sonnet provides generic helpers that use these functions:

        template<typename T>
        Sonnet::value to_json_value(const T& obj);

        template<typename T>
        std::expected<T, Sonnet::ConvertError> from_json_value(const Sonnet::value& v);

      (Exact names and error types may vary depending on the library's design.)

    -------------------
    Builtin Conversions
    -------------------
    - Sonnet may provide builtin `to_json`/`from_json` support for:
        * `bool`, `double`, `string`, `string_view`, etc.
        * STL containers like `vector<T>`, where `T` is itself
          convertible via `to_json`/`from_json`
        * Associative containers mapping string-like keys to values

    --------------
    Error Handling
    --------------
    - Deserialization (`from_json`) will be able to report failures,
      e.g. via:
        * `std::expected<T, ConvertError>`
        * throwing exceptions (if enabled)
      depending on library configuration
    - Typical errors might include:
        * Type mismatch (e.g. expected object, got array)
        * Missing require fields
        * Invalid or out-of-range values
    
    ----------------
    Conceptual Usage
    ----------------
        struct user {
            std::string name;
            int age;
        };

        void to_json(Sonnet::value& v, const user& u) {
            auto& obj = v.as_object();
            obj["name"] = u.name;
            obj["age"] = u.age;
        }

        void from_json(const Sonnet::value& v, user& u) {
            const auto& obj = v.as_object();
            u.name = obj.at("name").as_string();
            u.age = static_cast<int>(obj.at("age").as_number());
        }

    This header focuses on the conversion interface and helper templates.
    The DOM type itself is defined in `value.hpp` and parsing/writing
    functions are provided in `sonnet.hpp`
*/


#include <type_traits>
#include <concepts>

#include "sonnet/value.hpp"


namespace Sonnet {

    template<typename T>
    concept JsonSerializable = requires(const T& t, value& v) { { to_json(v, t) } -> std::same_as<void>; };

    template<typename T>
    concept JsonDeserializable = requires(const value& v, T& t) { { from_json(v, t) } -> std::same_as<void>; };
    
    template<JsonSerializable T>
    [[nodiscard]] inline value serialize(const T& t, std::pmr::memory_resource* res = std::pmr::get_default_resource()) {
        value v{ res };
        to_json(v, t);
        return v;
    }

    template<JsonDeserializable T>
    [[nodiscard]] inline T deserialize(const value& v) {
        T t{};
        from_json(v, t);
        return t;
    }

} // namespace Sonnet