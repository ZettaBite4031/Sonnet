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
#include "sonnet/config.hpp"

/// @defgroup SonnetConvert Type Conversion
/// @ingroup Sonnet
/// @brief Converting between user-defined C++ types and JSON values

namespace Sonnet {

    /// @ingroup SonnetConvert
    /// @brief Concept representing types that can be serialized to a JSON value.
    ///
    /// @details
    /// A type `T` satisfies `JsonSerializable` if an overload of:
    ///
    ///     void to_json(Sonnet::value& out, const T& src);
    ///
    /// is available (via ADL or in the `Sonnet` namespace) and is callable
    /// with the appropriate argument types.
    ///
    /// This function is responsible for writing the JSON representation
    /// of `src` into `out`. The function must not assume that `out` has
    /// any specific initial kind; it may overwrite `out` freely.
    ///
    /// Example:
    /// @code
    /// struct Point { int x, y; };
    ///
    /// void to_json(Sonnet::value& v, const Point& p) {
    ///     auto& obj = v.as_object();
    ///     obj["x"] = p.x;
    ///     obj["y"] = p.y;
    /// }
    /// @endcode
    ///
    /// @tparam T Type to check.
    template<typename T>
    concept JsonSerializable = requires(const T& t, value& v) { { to_json(v, t) } -> std::same_as<void>; };

    /// @ingroup SonnetConvert
    /// @brief Concept representing types that can be deserialized from a JSON value.
    ///
    /// @details
    /// A type `T` satisfies `JsonDeserializable` if an overload of:
    ///
    ///     void from_json(const Sonnet::value& src, T& out);
    ///
    /// is available (via ADL or in the `Sonnet` namespace) and is callable.
    ///
    /// This function is responsible for reading the JSON representation from
    /// `src` and assigning the parsed result into `out`. Implementations may
    /// throw exceptions or perform validation as appropriate.
    ///
    /// Example:
    /// @code
    /// struct Point { int x, y; };
    ///
    /// void from_json(const Sonnet::value& v, Point& p) {
    ///     const auto& obj = v.as_object();
    ///     p.x = static_cast<int>(obj.at("x").as_number());
    ///     p.y = static_cast<int>(obj.at("y").as_number());
    /// }
    /// @endcode
    ///
    /// @tparam T Type to check.
    template<typename T>
    concept JsonDeserializable = requires(const value& v, T& t) { { from_json(v, t) } -> std::same_as<void>; };
    
    /// @ingroup SonnetConvert
    /// @brief Serializes a user-defined type into a JSON value.
    ///
    /// @details
    /// This helper invokes the user-provided `to_json` function to produce the
    /// JSON representation of an object of type `T`. The resulting `value` is
    /// constructed using the supplied polymorphic memory resource.
    ///
    /// Example:
    /// @code
    /// User u = {"Alice", 30};
    /// auto json = Sonnet::serialize(u);
    /// std::cout << Sonnet::dump(json, {.pretty = true});
    /// @endcode
    ///
    /// @tparam T A type that satisfies `JsonSerializable`.
    /// @param t   The object to serialize.
    /// @param res Memory resource used for the resulting JSON DOM tree.
    /// @return A fully constructed JSON `value` representing @p t.
    template<JsonSerializable T>
    [[nodiscard]] inline value serialize(const T& t, std::pmr::memory_resource* res = std::pmr::get_default_resource()) {
        value v{ res };
        to_json(v, t);
        return v;
    }

    /// @ingroup SonnetConvert
    /// @brief Deserializes a user-defined type from a JSON value.
    ///
    /// @details
    /// This helper invokes the user-provided `from_json` function to construct
    /// a value of type `T` from a JSON DOM node. The destination object is
    /// value-initialized and populated by the deserialization routine.
    ///
    /// Example:
    /// @code
    /// auto parsed = Sonnet::parse(R"({"x":1,"y":2})");
    /// Point p = Sonnet::deserialize<Point>(*parsed);
    /// // p.x == 1, p.y == 2
    /// @endcode
    ///
    /// @note
    /// Implementations of `from_json` may perform validation, throw exceptions,
    /// or assume well-formed input, depending on application needs.
    ///
    /// @tparam T A type that satisfies `JsonDeserializable`.
    /// @param v  The JSON value to deserialize from.
    /// @return A new instance of `T` populated from the JSON data.
    template<JsonDeserializable T>
    [[nodiscard]] inline T deserialize(const value& v) {
        T t{};
        from_json(v, t);
        return t;
    }

} // namespace Sonnet