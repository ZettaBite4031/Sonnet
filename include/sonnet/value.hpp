#pragma once


/*
    -------------------------------------
    Sonnet::value - Dynamic JSON DOM node
    -------------------------------------
    The `Sonnet::value` type represents any JSON value:
        - null
        - boolean
        - number (as double)
        - string
        - array
        - object
    It is the core DOM building block of Sonnet
    
    -----------------
    Memory Management
    -----------------
    - `value` is allocator-aware and uses `std::pmr::memory_resource` for all
      internal allocations (strings, array, objects)
    - Each `value` instance stores a pointer to its `memory_resource`:
        * All nested containers and strings owned by that `value` are allocated
          from this resource
    - Copy construction/assignment:
        * The destination `value` adopts the allocator of the source and performs
          a deep copy of the underlying JSON tree into that allocator
    - Move construction/assignment:
        * The destination `value` steals the allocator and storage of the source
        
    -----------------
    Kinds and Queries
    -----------------
    - `kind type() const` returns the current kind: null, boolean, number
      string, array, object
    - Convenience predicates:
        * `is_null()`, `is_bool()`, `is_number()`, `is_string()`,
          `is_array()`, `is_object()`

    -----------------------------
    Accessors and Auto-Conversion
    -----------------------------
    - Scalar accessors:
        * `as_bool()`, `as_number()`, `as_string()`
        * These assume the current type matches; calling them on the wrong kind
          is undefined behavior (or may crash/asset/abort in later builds)
    - Container accessors:
        * `array& as_array()`
        * `object& as_object()`
        * Non-const versions will **convert** the value in-place if neccessary:
            - If the current kind is not an array, `as_array()` replaces the
              stored value with an empty `array` using the current allocator
            - Similarly for `as_object()`
        * Const versions (`const array&`, `const object&`) assumes the type
          is already correct and do not perform conversion
          
    -------------------
    Indexing Operations
    -------------------
    - `value& operator[](std::string_view)`
        * If the value is not an object, it is converted to an empty object
        * If provided key does not exist, a new entry is inserted with a `null` value
        * Returns a reference to the associated `value`
    - `value& operator[](size_t)`
        * If the value is not an array, it is converted to an empty array
        * if `index >= size()`, the array is resized to `index + 1` and all
          new elements are default-constructed (i.e. `null` values)
        * Returns a reference to the element at `index`

    ---------------------
    Equality and Ordering
    ---------------------
    - `value` supports structural equality and ordering via defaulted 
      three-way comparison:
        * Two values compare equal if they have the same kind and equal
          underlying contents (for arrays/objects, structural equality)
        * Comparison across different kinds are well-defined but primarily
          useful for mapping/ordering, not for semantic ranking
    
    -------------
    Thread-Safety
    -------------
    - `value` is not inherently thread-safe
    - It is safe to use separate `value` instances from multiple threads
    - Concurrent access to the same `value` instance must be externally synchronized

    This header defines only the DOM node type
    Parsing, serialization, and conversion utilities are provided elsewhere within Sonnet
*/


#include <variant>
#include <string>
#include <vector>
#include <map>
#include <memory_resource>
#include <compare>
#include <cstddef>
#include <concepts>
#include <utility>


namespace Sonnet {
    enum class kind : uint8_t {
        null,
        boolean,
        number,
        string,
        array,
        object,
    };
    
    
    template<class T>
    using pmr_vector = std::pmr::vector<T>;
    
    template<class Key, class T, class Compare = std::less<>> 
    using pmr_map = std::pmr::map<Key, T, Compare>;
    
    using string = std::pmr::string;
    
    struct value;
    using allocator_type = std::pmr::polymorphic_allocator<value>;

    using array = pmr_vector<value>;
    using object = pmr_map<string, value>;

    using storage_t = std::variant<
        std::monostate,
        bool,
        double,
        string,
        array,
        object
    >;

    struct value {
        explicit value(std::pmr::memory_resource* res = std::pmr::get_default_resource()) noexcept;
        value(std::nullptr_t, std::pmr::memory_resource* res = std::pmr::get_default_resource()) noexcept;
        value(bool b, std::pmr::memory_resource* res = std::pmr::get_default_resource()) noexcept;
        value(double d, std::pmr::memory_resource* res = std::pmr::get_default_resource()) noexcept;

        template<std::integral I>
        explicit value(I i, std::pmr::memory_resource* res = std::pmr::get_default_resource()) noexcept
            : m_MemRes{ res }, m_Storage{ static_cast<double>(i) } {}
        
        value(const char* s, std::pmr::memory_resource* res = std::pmr::get_default_resource());
        value(std::string_view sv, std::pmr::memory_resource* res = std::pmr::get_default_resource());
        value(string s, std::pmr::memory_resource* res = std::pmr::get_default_resource());
        value(array a, std::pmr::memory_resource* res = std::pmr::get_default_resource());
        value(object o, std::pmr::memory_resource* res = std::pmr::get_default_resource());

        value(const value& other);
        value(value&& other) noexcept;
        value& operator=(const value& other);
        value& operator=(value&& other) noexcept;

        [[nodiscard]] kind type() const noexcept;

        [[nodiscard]] bool is_null()   const noexcept { return type() == kind::null;    }
        [[nodiscard]] bool is_bool()   const noexcept { return type() == kind::boolean; }
        [[nodiscard]] bool is_number() const noexcept { return type() == kind::number;  }
        [[nodiscard]] bool is_string() const noexcept { return type() == kind::string;  }
        [[nodiscard]] bool is_array()  const noexcept { return type() == kind::array;   }
        [[nodiscard]] bool is_object() const noexcept { return type() == kind::object;  }
        
        [[nodiscard]] bool&       as_bool();
        [[nodiscard]] const bool& as_bool() const;

        [[nodiscard]] double&       as_number();
        [[nodiscard]] const double& as_number() const;

        [[nodiscard]] string&       as_string();
        [[nodiscard]] const string& as_string() const;

        [[nodiscard]] array&       as_array();
        [[nodiscard]] const array& as_array() const;

        [[nodiscard]] object&       as_object();
        [[nodiscard]] const object& as_object() const;

        [[nodiscard]] size_t size() const noexcept;

        value& operator[](size_t idx);
        const value& operator[](size_t idx) const;

        value& operator[](std::string_view key);

        const value* find(std::string_view key) const;
        const value& at(std::string_view key) const;

        friend auto operator<=>(const value& lhs, const value& rhs) = default;

        [[nodiscard]] std::pmr::memory_resource* resource() const noexcept { return m_MemRes; }

        [[nodiscard]] const storage_t& storage() const noexcept { return m_Storage; }
        [[nodiscard]] storage_t& storage() noexcept { return m_Storage; }

        
    private:
        std::pmr::memory_resource* m_MemRes{};
        storage_t m_Storage{};

        static storage_t clone_storage(const storage_t& s, std::pmr::memory_resource* res);
    };

} // namespace Sonnet
