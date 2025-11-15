#include "sonnet/value.hpp"

#include <stdexcept>


namespace Sonnet {

    value::value(std::pmr::memory_resource* res) noexcept
        : m_MemRes{ res }, m_Storage{ std::monostate{} } {}

    value::value(std::nullptr_t, std::pmr::memory_resource* res) noexcept
        : m_MemRes{ res }, m_Storage{ std::monostate{} } {}


    value::value(bool b, std::pmr::memory_resource* res) noexcept
        : m_MemRes{ res }, m_Storage{ b } {}

    value::value(double d, std::pmr::memory_resource* res) noexcept
        : m_MemRes{ res }, m_Storage{ d } {}

    value::value(const char* s, std::pmr::memory_resource* res)
        : m_MemRes{ res }, m_Storage{ string{ s, res } } {}

    value::value(std::string_view sv, std::pmr::memory_resource* res)
        : m_MemRes{ res }, m_Storage{ string{ sv.begin(), sv.end(), res } } {}

    value::value(string s, std::pmr::memory_resource* res)
        : m_MemRes{ res }, m_Storage{ std::move(s) } {
        // Optionally: normalize allocator to m_MemRes if they differ
    }

    value::value(array a, std::pmr::memory_resource* res)
        : m_MemRes{ res }, m_Storage{ std::move(a) } {}

    value::value(object o, std::pmr::memory_resource* res)
        : m_MemRes{res}, m_Storage{ std::move(o) } {}

    value::value(const value& other)
        : m_MemRes{ other.m_MemRes }, m_Storage{ clone_storage(other.m_Storage, other.m_MemRes) } {}

    value::value(value&& other) noexcept
        : m_MemRes{ other.m_MemRes }, m_Storage{ std::move(other.m_Storage) } {}

    value& value::operator=(const value& other) {
        if (this == &other) return *this;
        m_MemRes = other.m_MemRes;
        m_Storage = clone_storage(other.m_Storage, other.m_MemRes);
        return *this;
    }

    value& value::operator=(value&& other) noexcept {
        if (this == &other) return *this;
        m_MemRes = other.m_MemRes;
        m_Storage = std::move(other.m_Storage);
        return *this;
    }

    kind value::type() const noexcept {
        switch(m_Storage.index()) {
        case 0: return kind::null;
        case 1: return kind::boolean;
        case 2: return kind::number;
        case 3: return kind::string;
        case 4: return kind::array;
        case 5: return kind::object;
        }
        return kind::null;
    }

    bool& value::as_bool() { return std::get<bool>(m_Storage); }
    const bool& value::as_bool() const { return std::get<bool>(m_Storage); }
    double& value::as_number() { return std::get<double>(m_Storage); }
    const double& value::as_number() const { return std::get<double>(m_Storage); }
    string& value::as_string() { return std::get<string>(m_Storage); }
    const string& value::as_string() const { return std::get<string>(m_Storage); }
    array& value::as_array() { if (!is_array()) m_Storage = array{ allocator_type(m_MemRes) }; return std::get<array>(m_Storage); }
    const array& value::as_array() const { return std::get<array>(m_Storage); }
    object& value::as_object() { if (!is_object()) m_Storage = object{ allocator_type(m_MemRes) }; return std::get<object>(m_Storage); }
    const object& value::as_object() const { return std::get<object>(m_Storage); }

    size_t value::size() const noexcept {
        if (is_array()) return as_array().size();
        if (is_object()) return as_object().size();
        return 0;
    }

    value& value::operator[](std::size_t idx) {
        auto& arr = as_array();
        if (idx >= arr.size()) {
            arr.resize(idx + 1, value{ m_MemRes });
        }
        return arr[idx];
    }

    const value& value::operator[](std::size_t idx) const {
        static const value null_sentinel{};
        if (!is_array()) return null_sentinel;
        const auto& arr = as_array();
        if (idx >= arr.size()) return null_sentinel;
        return arr[idx];
    }

    value& value::operator[](std::string_view key) {
        auto& obj = as_object();
        string k{ key.begin(), key.end(), m_MemRes };
        auto [it, inserted] = obj.emplace(std::move(k), value{ m_MemRes });
        return it->second;
    }

    const value* value::find(std::string_view key) const {
        if (!is_object()) return nullptr;
        const auto& obj = as_object();
        string k{ key.begin(), key.end(), m_MemRes };
        auto it = obj.find(k);
        if (it == obj.end()) return nullptr;
        return std::addressof(it->second);
    }

    const value& value::at(std::string_view key) const {
        if (auto* v = find(key)) return *v;
        throw std::out_of_range{ "Sonnet::value::at: key not found "};
    }

    storage_t value::clone_storage(const storage_t& s, std::pmr::memory_resource* res) {
        switch (s.index()) {
        case 0: return std::monostate{};
        case 1: return std::get<bool>(s);
        case 2: return std::get<double>(s);
        case 3: {
            const auto& str = std::get<string>(s);
            return string{ str, res };
        }
        case 4: {
            const auto& arr = std::get<array>(s);
            array copy(allocator_type{ res });
            copy.reserve(arr.size());
            for (const auto& v : arr) copy.emplace_back(v);
            return copy;
        }
        case 5: {
            const auto& obj = std::get<object>(s);
            object copy{ std::less<>{}, res };
            for (const auto& [k, v] : obj) copy.emplace(string{ k, res }, value{ v });
            return copy;
        }
        }
        return std::monostate{};
    }
    

} // namespace Sonnet