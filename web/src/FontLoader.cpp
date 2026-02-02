#include "FontLoader.h"
#include "DataLoaderResult.h"
#include "TextureLoaderResult.h"
#include "Font.h"
#include "FontLoaderResult.h"
#include "FontManifestParser.h"
#include "Logger.h"

static FontLoaderResult loadFontFromUrls(LoaderInterface &loader, const std::string &fontManifestUrl, const std::string &fontTextureUrl);

FontLoader::FontLoader(std::shared_ptr<LoaderInterface> dataLoader, std::string baseUrl)
    : dataLoader(dataLoader)
    , baseUrl(baseUrl) {}

FontLoaderResult FontLoader::loadFont(const Font &font) {
    const std::string manifestUrl = baseUrl + font.name + ".json";
    const std::string textureUrl = baseUrl + font.name + ".png";
    return loadFontFromUrls(*dataLoader, manifestUrl, textureUrl);
}

static FontLoaderResult loadFontFromUrls(LoaderInterface &loader, const std::string &fontManifestUrl, const std::string &fontTextureUrl) {
  //return FontLoaderResult(nullptr, std::nullopt, LoaderStatus::ERROR_OTHER);
    LogInfo << "Loading font data from " << fontManifestUrl << " / " <<= fontTextureUrl;
    auto futManifest = loader.loadDataAsync(fontManifestUrl, std::nullopt).then([&](auto future) -> FontLoaderResult {
        auto loaderResult = future.get();
        if(loaderResult.status != LoaderStatus::OK) {
            LogError << "Error loading font manifest from " << fontManifestUrl << ": " <<= toString(loaderResult.status);
            return FontLoaderResult{nullptr, std::nullopt, loaderResult.status};
        }
        if(!loaderResult.data) {
            LogError << "No font manifest data from " <<= fontManifestUrl;
            return FontLoaderResult{nullptr, std::nullopt, LoaderStatus::ERROR_OTHER};
        }
        try {
            auto manifest = FontManifestParser::parse(reinterpret_cast<const char*>(loaderResult.data->buf()), loaderResult.data->len());
            return FontLoaderResult{nullptr, manifest, LoaderStatus::OK};
        } catch (std::exception e) {
            LogError << "Error parsing font manifest from " << fontManifestUrl << ": " <<= e.what();
            return FontLoaderResult{nullptr, std::nullopt, LoaderStatus::ERROR_OTHER};
        }
    });
    auto futTexture = loader.loadTextureAsync(fontTextureUrl, std::nullopt);

    auto manifestResult = futManifest.get();
    if(manifestResult.status != LoaderStatus::OK) {
        loader.cancel(fontTextureUrl);
        return manifestResult;
    }

    auto textureResult= futTexture.get();
    return FontLoaderResult{textureResult.data, manifestResult.fontData, textureResult.status};
}
