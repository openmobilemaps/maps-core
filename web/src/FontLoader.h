#pragma once

#include "FontLoaderInterface.h"
#include "LoaderInterface.h"

#include <mutex>
#include <string>
#include <unordered_map>

class FontLoader : public FontLoaderInterface {
  public:
    FontLoader(std::shared_ptr<LoaderInterface> dataLoader, std::string baseUrl);

    virtual FontLoaderResult loadFont(const Font &font);

  private:
    std::shared_ptr<LoaderInterface> dataLoader;
    std::string baseUrl;

    std::mutex cacheMutex;
    std::unordered_map<std::string, FontLoaderResult> cache;
};
