#pragma once

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
