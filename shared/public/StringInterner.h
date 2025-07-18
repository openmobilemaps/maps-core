#pragma once

#include "InternedString.h"

#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

// StringInterner manages a table of unique strings.
// Returns only references / handles for the strings contained in the table.
// These references can be compared and hashed cheaply.
class StringInterner {
  public:
    StringInterner();

    // Copy; all StringRefs for o remain valid.
    StringInterner(const StringInterner &o);

    // Move; all StringRefs for o remain valid
    StringInterner(StringInterner &&o);

    // Get an interned string reference for s, insert into table if not already existing.
    // Thread-safe.
    InternedString add(const std::string &s);

    // Batch add(), keeping the lock.
    template <typename InputIterator, typename OutputIterator>
    void add(InputIterator begin, InputIterator end, OutputIterator out) {
        std::lock_guard lock(mutex);
        for (auto it = begin; it != end; it++) {
            *out++ = addLocked(*it);
        }
    }

    // Return string for interned string reference s.
    // Thread-safe.
    const std::string &get(InternedString s) const {
        std::lock_guard lock(mutex);
        return strings[s.id()];
    }

  private:
    InternedString addLocked(const std::string &s);

  private:
    struct TableRef {
        InternedString::StringId idx;
    };

    const std::string &resolve(const TableRef &r) const {
        if (r.idx < strings.size()) {
            return strings[r.idx];
        } else {
            return *insertionCandidate;
        }
    }

    struct StringRefEqual {
        StringInterner &interner;

        bool operator()(const TableRef &lhs, const TableRef &rhs) const { return interner.resolve(lhs) == interner.resolve(rhs); }
    };

    struct StringRefHash {
        StringInterner &interner;
        const std::hash<std::string> stringHash{};

        std::size_t operator()(const TableRef &s) const { return stringHash(interner.resolve(s)); }
    };

  private:
    mutable std::mutex mutex;
    std::vector<std::string> strings;
    // Hash set of the strings contained in `strings`. This does not store
    // additional string data; the strings are defined by references into `strings`.
    std::unordered_set<TableRef, StringRefHash, StringRefEqual> set;

    // temporary value; this is the new string to be compared against, used in the StringRefCompare/Hash.
    // XXX Could be avoided with "heterogenous" lookup / insertion for unordered containers added only in c++23.
    const std::string *insertionCandidate = nullptr;
};
