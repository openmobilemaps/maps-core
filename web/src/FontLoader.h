#pragma once

#include "Font.h"
#include "FontLoaderInterface.h"
#include "FontLoaderResult.h"
#include "LoaderInterface.h"

#include <memory>
#include <string>

class FontLoader : public FontLoaderInterface {
  public:
    FontLoader(std::shared_ptr<LoaderInterface> dataLoader, std::string baseUrl);

    virtual FontLoaderResult loadFont(const Font &font);

  private:
    std::shared_ptr<LoaderInterface> dataLoader;
    std::string baseUrl;
};
