#pragma once

#include <cstdint>
#include <functional>

// Reference/handle for a string interned by StringInterner.
// StringInterner returns InternedString references for identical strings.
class InternedString {
  public:
    using StringId = uint32_t;

  public:
    InternedString(StringId id)
        : _id(id) {}

    bool operator==(const InternedString &o) const { return _id == o._id; }

    bool operator!=(const InternedString &o) const { return _id != o._id; }

    StringId id() const { return _id; }

  private:
    StringId _id;
};

namespace std {
template <> struct hash<InternedString> {
    size_t operator()(const InternedString &c) const { return c.id(); }
};
} // namespace std
