//
//  LRUCache.h
//  
//
//  Created by Stefan Mitterrutzner on 31.05.23.
//

#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <unordered_map>

template <typename Key, typename Value>
class LRUCache {
public:
    using iterator = typename std::unordered_map<Key, Value>::iterator;
    using const_iterator = typename std::unordered_map<Key, Value>::const_iterator;
    using on_erase_cb = typename std::function<void(const Key &key, const Value &value)>;

    using lfu_iterator = typename std::multimap<std::size_t, Key>::iterator;

    explicit LRUCache(size_t max_size) : max_cache_size(max_size) {
        if (max_cache_size == 0)
        {
            throw std::invalid_argument{"Size of the cache should be non-zero"};
        }
    }

    ~LRUCache() noexcept {
        clear();
    }

    void store(const Key &key, const Value &value) noexcept {
        std::scoped_lock<std::recursive_mutex> lock(mutex);
        auto elem_it = findElem(key);

        if (elem_it == cache_items_map.end())
        {
            // add new element to the cache
            if (cache_items_map.size() + 1 > max_cache_size)
            {
                auto disp_candidate_key = freqReplCandidate();

                erase(disp_candidate_key);
            }

            insert(key, value);
        }
        else
        {
            // update previous value
            update(key, value);
        }
    }


    const std::optional<Value> get(const Key &key) {
        std::scoped_lock<std::recursive_mutex> lock(mutex);
        auto elem = getInternal(key);

        if (elem.second)
        {
            return elem.first->second;
        }
        else
        {
            return std::nullopt;
        }
    }

    bool contains(const Key &key) noexcept {
        std::scoped_lock<std::recursive_mutex> lock(mutex);
        return findElem(key) != cache_items_map.cend();
    }

    std::size_t size() {
        std::scoped_lock<std::recursive_mutex> lock(mutex);
        return cache_items_map.size();
    }

    bool erase(const Key &key) {
        std::scoped_lock<std::recursive_mutex> lock(mutex);

        auto elem = findElem(key);

        if (elem == cache_items_map.end())
        {
            return false;
        }

        eraseIt(elem);

        return true;
    }

protected:
    void clear() {
        std::scoped_lock<std::recursive_mutex> lock(mutex);

        std::for_each(begin(), end(),
                      [&](const std::pair<const Key, Value> &el) { freqErase(el.first); });
        cache_items_map.clear();
    }

    const_iterator begin() const noexcept {
        return cache_items_map.cbegin();
    }

    const_iterator end() const noexcept {
        return cache_items_map.cend();
    }

protected:
    void insert(const Key &key, const Value &value) {
        freqInsert(key);
        cache_items_map.emplace(std::make_pair(key, value));
    }

    void eraseIt(const_iterator elem) {
        freqErase(elem->first);
        cache_items_map.erase(elem);
    }

    void eraseKey(const Key &key) {
        auto elem_it = findElem(key);

        eraseIt(elem_it);
    }

    void update(const Key &key, const Value &value) {
        freqTouch(key);
        cache_items_map[key] = value;
    }

    const_iterator findElem(const Key &key) const {
        return cache_items_map.find(key);
    }

    std::pair<const_iterator, bool> getInternal(const Key &key) noexcept {
        auto elem_it = findElem(key);

        if (elem_it != end())
        {
            freqTouch(key);
            return {elem_it, true};
        }

        return {elem_it, false};
    }

    void freqInsert(const Key &key) {
        constexpr std::size_t INIT_VAL = 1;
        lfu_storage[key] =
        frequency_storage.emplace_hint(frequency_storage.cbegin(), INIT_VAL, key);
    }

    void freqTouch(const Key &key) {
        // get the previous frequency value of a key
        auto elem_for_update = lfu_storage[key];
        auto updated_elem = std::make_pair(elem_for_update->first + 1, elem_for_update->second);
        // update the previous value
        frequency_storage.erase(elem_for_update);
        lfu_storage[key] =
        frequency_storage.emplace_hint(frequency_storage.cend(), std::move(updated_elem));
    }

    void freqErase(const Key &key) noexcept {
        frequency_storage.erase(lfu_storage[key]);
        lfu_storage.erase(key);
    }

    const Key &freqReplCandidate() const noexcept {
        // at the beginning of the frequency_storage we have the
        // least frequency used value
        return frequency_storage.cbegin()->second;
    }

private:
    std::unordered_map<Key, Value> cache_items_map;
    std::recursive_mutex mutex;
    std::size_t max_cache_size;

    std::multimap<std::size_t, Key> frequency_storage;
    std::unordered_map<Key, lfu_iterator> lfu_storage;
};
