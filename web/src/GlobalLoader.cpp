#include "DataLoaderResult.h"
#include "FontLoader.h"
#include "TextureLoaderResult.h"
#include "TextureHolderInterface.h"
#include "GlobalLoader.h"

#include "NativeLoaderInterface.h"
#include "ImageValTextureHolder.h"

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/fetch.h>

#include <memory>
#include <mutex>

std::shared_ptr<GlobalLoaderInterface> GlobalLoaderInterface::create() {
    return std::make_shared<GlobalLoader>();
}

djinni::Future<DataLoaderResult> GlobalLoader::enqueueDataLoadRequest(const std::string &url,
                                                                      const std::optional<std::string> &etag) {
    std::lock_guard<std::mutex> lock(mutex);
    auto &req = dataLoadQueue.emplace_front(url, etag);
    return req.promise.getFuture();
}

djinni::Future<TextureLoaderResult> GlobalLoader::enqueueTextureLoadRequest(const std::string &url,
                                                                            const std::optional<std::string> &etag) {
    std::lock_guard<std::mutex> lock(mutex);
    auto &req = textureLoadQueue.emplace_front(url, etag);
    return req.promise.getFuture();
}

/**
 * Explicitly keep alive em::val image values on garbagePile, to ensure
 * destruction on the main thread that created the values.
 */
static std::list<std::shared_ptr<ImageValTextureHolder>> garbagePile;

void global_loader_load_image_callback_onsuccess(em::val img, uintptr_t userData) {
    auto loadRequest = reinterpret_cast<TextureLoadRequest *>(userData);
    auto textureHolder = std::make_shared<ImageValTextureHolder>(img);
    garbagePile.push_back(textureHolder);
    loadRequest->promise.setValue(TextureLoaderResult(textureHolder, "", LoaderStatus::OK, ""));
    delete loadRequest;
}

void global_loader_load_image_callback_onerror(uintptr_t userData) {
    auto loadRequest = reinterpret_cast<TextureLoadRequest *>(userData);
    printf("load error %s\n", loadRequest->url.c_str());
    loadRequest->promise.setValue(TextureLoaderResult(nullptr, "", LoaderStatus::ERROR_OTHER, "Failed"));
    delete loadRequest;
}

EM_JS(void, load_image_async, (const char *url, uintptr_t userData), {
    var urlS = UTF8ToString(url);
    var img = new Image();
    img.src = urlS;
    img.crossOrigin="anonymous";
    img
      .decode()
      .then(() => {
          Module.global_loader_load_image_callback_onsuccess(img, userData);
        })
        .catch((error) => {
        console.log(error);
        Module.global_loader_load_image_callback_onerror(userData);
    });
});

void GlobalLoader::processTextureLoadRequest(TextureLoadRequest &&textureLoadRequest) {
    // Place this directly on the heap for now...
    auto loadRequest = new TextureLoadRequest{std::move(textureLoadRequest)};
    static_assert(sizeof(loadRequest) == sizeof(void *));

    load_image_async(loadRequest->url.c_str(), reinterpret_cast<uintptr_t>(loadRequest));
}

void GlobalLoader::processDataLoadRequest(DataLoadRequest &&dataLoadRequest) {
    // Place this directly on the heap for now...
    auto loadRequest = new DataLoadRequest{std::move(dataLoadRequest)};
    static_assert(sizeof(loadRequest) == sizeof(void *));

    // Fetch attributes setup
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.userData = reinterpret_cast<void *>(loadRequest);
    attr.onsuccess = [](emscripten_fetch_t *fetch) {
        auto loadRequest = reinterpret_cast<DataLoadRequest *>(fetch->userData);
        if (fetch->status != 200) {
            loadRequest->promise.setValue(
                DataLoaderResult({}, "", LoaderStatus::ERROR_OTHER, "Failed with status: " + std::to_string(fetch->status)));
        } else {
            loadRequest->promise.setValue(
                DataLoaderResult(::djinni::DataRef(fetch->data, fetch->numBytes), "", LoaderStatus::OK, ""));
        }
        emscripten_fetch_close(fetch);
        delete loadRequest;
    };
    attr.onerror = [](emscripten_fetch_t *fetch) {
        auto loadRequest = reinterpret_cast<DataLoadRequest *>(fetch->userData);
        loadRequest->promise.setValue(
            DataLoaderResult({}, "", LoaderStatus::ERROR_OTHER, "Failed with status: " + std::to_string(fetch->status)));
        emscripten_fetch_close(fetch);
        delete loadRequest;
    };

    emscripten_fetch(&attr, loadRequest->url.c_str());
}

// // Entry point for main thread.
// void processLoadRequests() {

//     globalLoader.processLoadRequests();

//     for (auto it = garbagePile.begin(); it != garbagePile.end();) {
//         std::weak_ptr<ImageValTextureHolder> weak = *it;
//         *it = nullptr;
//         *it = weak.lock();
//         if (*it == nullptr) {
//             it = garbagePile.erase(it);
//         } else {
//             ++it;
//         }
//     }
// }

std::shared_ptr<LoaderInterface> GlobalLoader::createLoader() {
    return std::static_pointer_cast<LoaderInterface>(std::make_shared<Loader>(shared_from_this()));
}

std::shared_ptr<FontLoaderInterface> GlobalLoader::createFontLoader(const std::shared_ptr<LoaderInterface> &loader, const std::string &baseUrl) {
    return std::static_pointer_cast<FontLoaderInterface>(std::make_shared<FontLoader>(loader, baseUrl));
}

void GlobalLoader::processLoadRequests() {
    std::lock_guard<std::mutex> lock(mutex);
    while (!dataLoadQueue.empty()) {
        processDataLoadRequest(std::move(dataLoadQueue.front()));
        dataLoadQueue.pop_front();
    }
    while (!textureLoadQueue.empty()) {
        processTextureLoadRequest(std::move(textureLoadQueue.front()));
        textureLoadQueue.pop_front();
    }

    for (auto it = garbagePile.begin(); it != garbagePile.end();) {
        std::weak_ptr<ImageValTextureHolder> weak = *it;
        *it = nullptr;
        *it = weak.lock();
        if (*it == nullptr) {
            it = garbagePile.erase(it);
        } else {
            ++it;
        }
    }  
}

void GlobalLoader::cancel(const std::string &url) {
    // noop for now
}

EMSCRIPTEN_BINDINGS(_web_extras_loader) {
    emscripten::function("global_loader_load_image_callback_onsuccess", &global_loader_load_image_callback_onsuccess);
    emscripten::function("global_loader_load_image_callback_onerror", &global_loader_load_image_callback_onerror);
}
