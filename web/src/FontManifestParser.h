#pragma once

#include "FontData.h"

class FontManifestParser {
  public:
    static FontData parse(const char *buf, size_t bufSize);
};
