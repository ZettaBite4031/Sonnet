#pragma once

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