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

/// @defgroup Sonnet Sonnet JSON Library
/// @brief Core types and functions for Sonnet

/// @defgroup SonnetValue DOM Value
/// @ingroup Sonnet

#include <variant>
#include <string>
#include <vector>
#include <map>
#include <memory_resource>
#include <compare>
#include <cstddef>
#include <concepts>
#include <utility>
#include "sonnet/config.hpp"

namespace Sonnet {
    /// @brief Enumerates the possible JSON value kinds held by Sonnet::value
    enum class kind : uint8_t {
        null, ///< JSON null value     
        boolean, ///< JSON boolean value (`true` or `false`)
        number, ///< JSON number value (stored as `double`)
        string, ///< JSON string value
        array, ///< JSON array value
        object, ///< JSON object value
    };
    
    
    template<class T>
    using pmr_vector = std::pmr::vector<T>;
    
    template<class Key, class T, class Compare = std::less<>> 
    using pmr_map = std::pmr::map<Key, T, Compare>;
    
    /// @ingroup SonnetValue
    /// @brief String type used by Sonnet::value (allocator-aware)
    using string = std::pmr::string;
    
    struct value;
    using allocator_type = std::pmr::polymorphic_allocator<value>;

    /// @ingroup SonnetValue
    /// @brief Array type used by Sonnet::value (JSON arrays)
    using array = pmr_vector<value>;

    /// @ingroup SonnetValue
    /// @brief Object type used by Sonnet::value (JSON objects)
    using object = pmr_map<string, value>;

    /// @ingroup SonnetValue
    /// @brief Variant storage used internally by Sonnet::value
    /// @details Exposed only for completness; most users interact via
    ///          Sonnet::value member functions instead of using this alias
    using storage_t = std::variant<
        std::monostate,
        bool,
        double,
        string,
        array,
        object
    >;

    
    /// @ingroup SonnetValue
    /// @brief Dynamic JSON DOM types.
    ///
    /// @details
    /// `Sonnet::value` can hold any JSON value:
    /// - null
    /// - boolean
    /// - number 
    /// - string
    /// - array
    /// - object
    ///
    /// All nested allocations (string, arrays, objects) are performed using
    /// a `std::pmr::memory_resource` associated with each `value` instance.
    /// Container-like operations (e.g. `as_array`, `as_object`, `operator[]`)
    /// use this allocator
    struct value {
        // ------------------------------------------------------------
        // Constructors / assignment / destructor
        // ------------------------------------------------------------

        /// @ingroup SonnetValue
        /// @brief Constructs a null JSON value using the given memory resource
        /// @param res Pointer to the memory resource used for all internal 
        ///            allocations in this value, If omitted, the global 
        ///            default resource is used
        SONNET_API explicit value(std::pmr::memory_resource* res = std::pmr::get_default_resource()) noexcept;

        /// @ingroup SonnetValue
        /// @brief Constructs a null JSON value using the given memory resource
        ///
        /// @param nullptr_t Unused; this overload exists to disambiguate 
        ///        explicit creation of a null value 
        /// @param res Memory resource used for nested allocations
        SONNET_API value(std::nullptr_t, std::pmr::memory_resource* res = std::pmr::get_default_resource()) noexcept;

        /// @ingroup SonnetValue
        /// @brief Constructs a boolean JSON value
        ///
        /// @param b Boolean value to store 
        /// @param res Memory resource used for nested allocations (if any)
        SONNET_API value(bool b, std::pmr::memory_resource* res = std::pmr::get_default_resource()) noexcept;

        /// @ingroup SonnetValue
        /// @brief Constructs a numeric JSON value from a double
        ///
        /// @param d Numeric value to store
        /// @param res Memory resource used for nested allocations (if any)
        SONNET_API value(double d, std::pmr::memory_resource* res = std::pmr::get_default_resource()) noexcept;

        /// @ingroup SonnetValue
        /// @brief Constructs a numeric JSON value from an integral type
        ///
        /// @tparam I Integral type (e.g. int, long, int64_t)
        /// @param i Integer value to convert and store as a double
        /// @param res Memory resource used for nested allocations (if any)
        template<std::integral I>
        explicit value(I i, std::pmr::memory_resource* res = std::pmr::get_default_resource()) noexcept
            : m_MemRes{ res }, m_Storage{ static_cast<double>(i) } {}
        
        /// @ingroup SonnetValue
        /// @brief Constructs a string JSON value from a C string
        ///
        /// @param s Null-terminated UTF-8 string
        /// @param res Memory resource used for string storage
        SONNET_API value(const char* s, std::pmr::memory_resource* res = std::pmr::get_default_resource());

        /// @ingroup SonnetValue
        /// @brief Constructs a string JSON value from a string_view
        ///
        /// @param sv UTF-8 string view; characters are copied into an
        ///           allocator-backed `Sonnet::string`
        /// @param res Memory resource used for string storage
        SONNET_API value(std::string_view sv, std::pmr::memory_resource* res = std::pmr::get_default_resource());

        /// @ingroup SonnetValue
        /// @brief Constructs a string JSON from existing Sonnet::string 
        ///
        /// @param s String to store. May be moved from
        /// @param res Memory resource used for string storage. If `res`
        ///            differs from `s`'s allocator, the string will be
        ///            copied into a new string using `res`
        SONNET_API value(string s, std::pmr::memory_resource* res = std::pmr::get_default_resource());

        /// @ingroup SonnetValue
        /// @brief Constructs an object JSON value from an existing array
        ///
        /// @param a Array to store. May be moved from
        /// @param res Memory resource used for nested allocations in the
        ///            array and its elements. If `res` differs from `o`'s 
        ///            allocator, contents are cloned into a new array
        SONNET_API value(array a, std::pmr::memory_resource* res = std::pmr::get_default_resource());

        /// @ingroup SonnetValue
        /// @brief Constructs an object JSON value from an existing object
        ///
        /// @param o Object to store. May be moved from
        /// @param res Memory resource used for nested allocations in the
        ///            object and its children. If `res` differs from `o`'s 
        ///            allocator, contents are cloned into a new object
        SONNET_API value(object o, std::pmr::memory_resource* res = std::pmr::get_default_resource());

        /// @ingroup SonnetValue
        /// @brief Copy-constructs a JSON value
        ///
        /// @details 
        /// The new value adopts the alloctor of @p other. The entire JSON
        /// tree rooted at @p other is deeply copied into the new value using
        /// that allocator
        ///
        /// @param other Value to copy 
        SONNET_API value(const value& other);

        /// @ingroup SonnetValue
        /// @brief Move-constructs a JSON value
        ///
        /// @details 
        /// The new value steals the allocator and storage from @p other.
        /// After the move, @p other is left in a valid but unspecified state
        /// (typically null with the same allocator)
        ///
        /// @param other Value to move from
        SONNET_API value(value&& other) noexcept;

        /// @ingroup SonnetValue
        /// @brief Copy-assigns a JSON value
        ///
        /// @details 
        /// The left-hand side value adopts the allocator of @p other and its
        /// contents are replaced with a deep copy of @p other
        ///
        /// @param other Value to copy from
        /// @return Reference to this value
        SONNET_API value& operator=(const value& other);

        /// @ingroup SonnetValue
        /// @brief Move-assigns a JSON value
        /// 
        /// @details
        /// The left-hand side value's previous contents are destroyed, and
        /// it takes ownership of the allocator and storage of @p other
        ///
        /// @param other Value to move from
        /// @return Reference to this value
        SONNET_API value& operator=(value&& other) noexcept;

        // ------------------------------------------------------------
        // Introspection
        // ------------------------------------------------------------

        /// @ingroup SonnetValue
        /// @brief Returns the kind of JSON value currently stored. 
        /// 
        /// @return The `Sonnet::kind` enum representing the active type
        SONNET_API [[nodiscard]] kind type() const noexcept;

        /// @ingroup SonnetValue
        /// @brief Checks whether the value holds JSON null
        SONNET_API [[nodiscard]] bool is_null()   const noexcept { return type() == kind::null;    }

        /// @ingroup SonnetValue
        /// @brief Checks whether the value holds a boolean
        SONNET_API [[nodiscard]] bool is_bool()   const noexcept { return type() == kind::boolean; }

        /// @ingroup SonnetValue
        /// @brief Checks whether the value holds a number
        SONNET_API [[nodiscard]] bool is_number() const noexcept { return type() == kind::number;  }

        /// @ingroup SonnetValue
        /// @brief Checks whether the value holds a string
        SONNET_API [[nodiscard]] bool is_string() const noexcept { return type() == kind::string;  }

        /// @ingroup SonnetValue
        /// @brief Checks whether the value holds an array
        SONNET_API [[nodiscard]] bool is_array()  const noexcept { return type() == kind::array;   }

        /// @ingroup SonnetValue
        /// @brief Checks whether the value holds an object
        SONNET_API [[nodiscard]] bool is_object() const noexcept { return type() == kind::object;  }
        
        // ------------------------------------------------------------
        // Scalar accessors
        // ------------------------------------------------------------

        /// @ingroup SonnetValue
        /// @brief Returns a reference to the stored boolean value 
        /// @pre `is_bool()` must be true. Calling this when the active kind
        ///      is not `kind::boolean` is undefined behavior
        SONNET_API [[nodiscard]] bool&       as_bool();

        /// @ingroup SonnetValue
        /// @brief Returns a const reference to the stored boolean value 
        /// @pre `is_bool()` must be true.
        SONNET_API [[nodiscard]] const bool& as_bool() const;

        /// @ingroup SonnetValue
        /// @brief Returns a reference to the stored number value 
        /// @pre `is_number()` must be true. Calling this when the active kind
        ///      is not `kind::number` is undefined behavior
        SONNET_API [[nodiscard]] double&       as_number();
        
        /// @ingroup SonnetValue
        /// @brief Returns a const reference to the stored number value 
        /// @pre `is_number()` must be true.
        SONNET_API [[nodiscard]] const double& as_number() const;

        /// @ingroup SonnetValue
        /// @brief Returns a reference to the stored string value 
        /// @pre `is_string()` must be true. Calling this when the active kind
        ///      is not `kind::string` is undefined behavior
        SONNET_API [[nodiscard]] string&       as_string();

        /// @ingroup SonnetValue
        /// @brief Returns a const reference to the stored string value
        /// @pre `is_string()` must be true.
        SONNET_API [[nodiscard]] const string& as_string() const;
    
        // ------------------------------------------------------------
        // Container accessors
        // ------------------------------------------------------------    

        /// @ingroup SonnetValue
        /// @brief Returns a reference to the stored array value 
        /// @details
        /// If `is_array()` is true, returns the existing array.
        /// Otherwise, the current contents are discarded and replaced with
        /// an empty array allocated from `resource()`, and that array is returned
        /// @pre `is_array()` must be true. Calling this when the active kind
        ///      is not `kind::array` is undefined behavior
        SONNET_API [[nodiscard]] array&       as_array();

        /// @ingroup SonnetValue
        /// @brief Returns a const reference to the stored array value 
        /// @pre `is_array()` must be true.
        SONNET_API [[nodiscard]] const array& as_array() const;

        /// @ingroup SonnetValue
        /// @brief Returns a reference to the stored object value
        /// @details
        /// If `is_object()` is true, returns the existing object.
        /// Otherwise, the current contents are discarded and replaced with
        /// an empty object allocated from `resource()`, and that object is returned
        /// @pre `is_object()` must be true. Calling this when the active kind
        ///      is not `kind::object` is undefined behavior
        SONNET_API [[nodiscard]] object&       as_object();

        /// @ingroup SonnetValue
        /// @brief Returns a const reference to the stored object value
        /// @pre `is_object()` must be true.
        SONNET_API [[nodiscard]] const object& as_object() const;

        /// @ingroup SonnetValue
        /// @brief Returns the size of the array or object
        /// @details
        /// For arrays, this is the number of elements
        /// For objects, this is the number of key/value pairs
        /// For non-container types, returns 0
        SONNET_API [[nodiscard]] size_t size() const noexcept;

        // ------------------------------------------------------------
        // Object indexing
        // ------------------------------------------------------------

        /// @ingroup SonnetValue
        /// @brief Accesses or creates an array element by index, growing array as needed
        /// @details
        /// If the current value is **not** an array, it is implicitly converted
        /// into an empty array (`[]`) before access.
        /// If @p idx is greater than or equal to the current array size,
        /// the array is resized to `idx + 1`. All newly created elements are
        /// default-constructed JSON values (`null`).
        /// Returns a reference to the element at @p idx
        ///
        /// @param idx Zero based index into the array
        /// @return  Reference to the value at index @p idx
        SONNET_API value& operator[](size_t idx);

        /// @ingroup SonnetValue
        /// @brief Accesses an array element by index (const overload)
        ///
        /// @details
        /// Unlike the non-const overload, this function does **not**
        /// perform type converion or resizing. It assumes that:
        ///  - The current value is already an array
        ///  - The index @p idx is within array bounds
        /// If either condition is violated, resulting behavior is undefined
        /// @param idx Zero based into the array
        /// @return Const reference to the value at index @p idx
        SONNET_API const value& operator[](size_t idx) const;

        /// @ingroup SonnetValue
        /// @brief Accesses or creates an object member by key
        ///
        /// @details
        /// If the value is not an object, it is converted to an empty object
        /// If @p key does not exist, a new entry is inserted with a `null` value
        /// Returns a reference to the value associated with @p key
        ///
        /// @param key Object key to access
        /// @return Reference to the value mapped to @p key
        SONNET_API value& operator[](std::string_view key);

        /// @ingroup SonnetValue
        /// @brief Finds a member with the given key in the object
        ///
        /// @details
        /// If the value is not an object, returns nullptr
        /// If the key exists, return a pointer to the corresponding value
        /// Otherwise, returns nullptr
        ///
        /// @param key Object key to look up 
        /// @return Pointer to the value mapped to @p key or nullptr
        SONNET_API const value* find(std::string_view key) const;

        /// @ingroup SonnetValue
        /// @brief Returns a const reference to the value associated with @p key
        ///
        /// @details
        /// If the value is not an object or @p key does not exist
        /// this function throws `std::out_of_range` 
        ///
        /// @param key Object key to access
        /// @return Const reference to the value mapped by @p
        /// @throws std::out_of_range If the key does not exist or the value is not an object
        SONNET_API const value& at(std::string_view key) const;

        /// @ingroup SonnetValue
        /// @brief Defaulted three-way comparison for structural ordering.
        /// 
        /// @details 
        /// Values are compared first by kind, then by their stored contents.
        /// For arrays and objects, comparison is structural (lexicographical for arrays, key/value-wise for objects)
        SONNET_API friend auto operator<=>(const value& lhs, const value& rhs) = default;
        
        /// @ingroup SonnetValue
        /// @brief Returns the memory resource associated with this value 
        ///
        /// @details 
        /// All nested allocations (strings, arrays, objects) within this
        /// value and its descendents use this resource
        ///         
        /// @return Pointer to the memory resource 
        [[nodiscard]] SONNET_API std::pmr::memory_resource* resource() const noexcept { return m_MemRes; }

        /// @ingroup SonnetValue
        /// @brief Returns a const reference to the underlying variant storage
        ///
        /// @details
        /// This function provides direct access to the internal `storage_t` 
        /// variant that backs this `value`. The returned reference exposes the
        /// raw representation:
        ///         std::variant<std::monostate, bool, double, string, array, object>
        /// Typical users should prefer higher-level accessors such as the `as_*()` functions, 
        /// which provide safer, JSON-semantic behavior.
        /// @return Const reference to the internal storage variant
        [[nodiscard]] SONNET_API const storage_t& storage() const noexcept { return m_Storage; }
        
        /// @ingroup SonnetValue
        /// @brief Returns a mutable reference to the underlying variant storage
        ///
        /// @details
        /// This function exposes the raw `storage_t` variant that holds the
        /// current JSON value in its unwrapped, low-level form.
        ///
        /// Because this provides unrestricted mutable access, it bypasses all
        /// type-safety guarantees and invariants normally enforced by the
        /// `value` API. Modifying the variant directly can invalidate
        /// assumptions made by high-level functions such as `as_array()`,
        /// `operator[]`, or type predicates (`is_array()`, `is_object()`, etc.).
        ///
        /// **Use with extreme caution.**
        /// @return Mutable reference to the internal storage variant
        [[nodiscard]] SONNET_API storage_t& storage() noexcept { return m_Storage; }

        
    private:
        std::pmr::memory_resource* m_MemRes{};
        storage_t m_Storage{};

        static storage_t clone_storage(const storage_t& s, std::pmr::memory_resource* res);
    };

} // namespace Sonnet
