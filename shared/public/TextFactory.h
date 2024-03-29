// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from text.djinni

#pragma once

#include "Coord.h"
#include "Font.h"
#include <memory>
#include <vector>

class TextInfoInterface;
enum class Anchor;
enum class TextJustify;
struct FormattedStringEntry;

class TextFactory {
public:
    virtual ~TextFactory() = default;

    static /*not-null*/ std::shared_ptr<TextInfoInterface> createText(const std::vector<FormattedStringEntry> & text, const ::Coord & coordinate, const ::Font & font, Anchor textAnchor, TextJustify textJustify);
};
