#ifndef DATA_STRUCTURE_LRU_CACHE_HPP
#define DATA_STRUCTURE_LRU_CACHE_HPP

#include <unordered_map>
#include <utility>
#include <list>

// A cache which evicts the least recently used item when it is full
template<class Key, class Value>
class LRUCache
{
public:
    using key_type = Key;
    using value_type = Value;
    using list_type = std::list<key_type>;
    using map_type = std::unordered_map<key_type, std::pair<value_type, typename list_type::iterator>>;

    LRUCache(size_t capacity)
        : m_capacity(capacity) {}

    ~LRUCache() = default;

    [[nodiscard]]
    size_t size() const noexcept {
        return m_map.size();
    }

    [[nodiscard]]
    size_t capacity() const noexcept {
        return m_capacity;
    }

    [[nodiscard]]
    bool empty() const noexcept {
        return m_map.empty();
    }

    [[nodiscard]]
    bool contains(const key_type& key) const noexcept {
        return m_map.find(key) != m_map.end();
    }

    void insert(const key_type& key, value_type value) {
        typename map_type::iterator i = m_map.find(key);
        if (i == m_map.end()) {
            // insert item into the cache, but first check if it is full
            if (size() >= m_capacity) {
                // cache is full, evict the least recently used item
                evict();
            }

            // insert the new item
            m_list.push_front(key);
            m_map.try_emplace(key, std::move(value), m_list.begin());
            // m_map[key] = std::make_pair(std::move(value), m_list.begin());
        }
    }

    const value_type* get(const key_type& key) const {
        // lookup value in the cache
        typename map_type::iterator i = m_map.find(key);
        if (i == m_map.end()) {
            // value not in cache
            return nullptr;
        }

        // return the value, but first update its place in the most
        // recently used list
        typename list_type::iterator j = i->second.second;
        if (j != m_list.begin()) {
            // move item to the front of the most recently used list
            m_list.erase(j);
            m_list.push_front(key);

            // update iterator in map
            j = m_list.begin();
            i->second.second = j;

            // return the value
            return &i->second.first;
        } else {
            // the item is already at the front of the most recently
            // used list so just return it
            return &i->second.first;
        }
    }

    value_type* get_unchecked(const key_type& key) const {
        typename map_type::iterator i = m_map.find(key);
        if (i == m_map.end()) {
            // value not in cache
            return nullptr;
        }

        return &i->second.first;
    }

    typename map_type::iterator begin() noexcept {
        return m_map.begin();
    }

    typename map_type::iterator end() noexcept {
        return m_map.end();
    }

    void set_capacity(size_t capacity) noexcept {
        m_capacity = capacity;
    }

    void clear() {
        m_map.clear();
        m_list.clear();
    }

private:
    void evict() {
        // evict item from the end of most recently used list
        typename list_type::iterator i = --m_list.end();
        m_map.erase(*i);
        m_list.erase(i);
    }

private:
    mutable map_type m_map;
    mutable list_type m_list;
    size_t m_capacity;
};

#endif