
#include "StringInterner.h"

#include <stdexcept>

StringInterner::StringInterner()
    : strings()
    , set(0, StringRefHash{*this}, StringRefEqual{*this}) {}

StringInterner::StringInterner(const StringInterner &o)
    : strings(o.strings)
    , set(o.set.begin(), o.set.end(), o.set.bucket_count(), StringRefHash{*this}, StringRefEqual{*this}) {}

StringInterner::StringInterner(StringInterner &&o)
    : strings(std::move(o.strings))
    // XXX: must copy set here because of the odd self-reference in StringRefHash/Equal (see below). Rehashes everything :(
    , set(o.set.begin(), o.set.end(), o.set.bucket_count(), StringRefHash{*this}, StringRefEqual{*this}) {}

InternedString StringInterner::add(const std::string &s) {
    std::lock_guard lock(mutex);

    size_t nextId = strings.size();
    if (nextId >= std::numeric_limits<InternedString::StringId>::max()) {
        throw new std::runtime_error("string table full");
    }

    insertionCandidate = &s;
    TableRef candidateRef{.idx = (InternedString::StringId)nextId};
    auto [it, isNew] = set.insert(candidateRef);
    insertionCandidate = nullptr;
    if (isNew) {
        strings.push_back(s);
        return InternedString{(InternedString::StringId)nextId};
    } else {
        return InternedString{it->idx};
    }
}
