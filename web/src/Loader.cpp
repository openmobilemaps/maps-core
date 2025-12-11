#include "Loader.h"
#include "DataLoaderResult.h"
#include "TextureLoaderResult.h"
#include "TextureHolderInterface.h"
#include "LoadRequests.h"

#include "NativeLoaderInterface.h"

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/fetch.h>


#include <string>

Loader::Loader(const std::shared_ptr<GlobalLoaderInterface> &globalLoader) 
    : globalLoader(globalLoader) {
}

Loader::~Loader() {}

TextureLoaderResult Loader::loadTexture(const std::string &url, const std::optional<std::string> &etag) {
    // TODO assert(not main thread);
    return loadTextureAsync(url, etag).get();
}

DataLoaderResult Loader::loadData(const std::string &url, const std::optional<std::string> &etag) {
    // TODO assert(not main thread);
    return loadDataAsync(url, etag).get();
}

::djinni::Future<DataLoaderResult> Loader::loadDataAsync(const std::string &url, const std::optional<std::string> &etag) {
    return globalLoader->enqueueDataLoadRequest(url, etag);
}

::djinni::Future<TextureLoaderResult> Loader::loadTextureAsync(const std::string &url, const std::optional<std::string> &etag) {
    return globalLoader->enqueueTextureLoadRequest(url, etag);
}

// Cancel an ongoing fetch request by URL
void Loader::cancel(const std::string &url) { globalLoader->cancel(url); }