#include "WebLoader.h"
#include "DataLoaderResult.h"
#include "FontLoader.h"
#include "ImageValTextureHolder.h"
#include "TextureLoaderResult.h"

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/fetch.h>

#include <memory>
#include <mutex>

std::shared_ptr<WebLoaderInterface> WebLoaderInterface::create() {
    return std::make_shared<WebLoader>();
}

djinni::Future<DataLoaderResult> WebLoader::enqueueDataLoadRequest(const std::string &url,
                                                                   const std::optional<std::string> &etag) {
    std::lock_guard<std::mutex> lock(mutex);
    auto &req = dataLoadQueue.emplace_back(std::make_unique<DataLoadRequest>(url, etag));
    req->loader = shared_from_this();
    return req->promise.getFuture();
}

djinni::Future<TextureLoaderResult> WebLoader::enqueueTextureLoadRequest(const std::string &url,
                                                                         const std::optional<std::string> &etag) {
    std::lock_guard<std::mutex> lock(mutex);
    auto &req = textureLoadQueue.emplace_back(std::make_unique<TextureLoadRequest>(url, etag));
    return req->promise.getFuture();
}

void web_loader_load_image_callback_onsuccess(emscripten::val img, uintptr_t loadRequestPtr) {
    auto loadRequest = reinterpret_cast<TextureLoadRequest *>(loadRequestPtr);
    auto textureHolder = ImageValTextureHolder::create(img);
    // Note: etag not readily accessible here, likely it will not be very useful anyway.
    loadRequest->promise.setValue(TextureLoaderResult(textureHolder, "", LoaderStatus::OK, ""));
    delete loadRequest;
}

void web_loader_load_image_callback_onerror(uintptr_t loadRequestPtr) {
    auto loadRequest = reinterpret_cast<TextureLoadRequest *>(loadRequestPtr);
    loadRequest->promise.setValue(TextureLoaderResult(nullptr, "", LoaderStatus::ERROR_OTHER, "Failed"));
    delete loadRequest;
}

EM_JS(void, load_image_async, (uintptr_t loaderPtr, const char *url, uintptr_t loadRequestPtr), {
    var urlS = UTF8ToString(url);

    var img = new Image();
    img.src = urlS;
    img.crossOrigin="anonymous";

    // Register image to allow cancellation.
    if(!Module.web_loader_image_loads) {
        Module.web_loader_image_loads = {};
    }
    if(!Module.web_loader_image_loads[loaderPtr]) {
        Module.web_loader_image_loads[loaderPtr] = {};
    }
    Module.web_loader_image_loads[loaderPtr][urlS] = img;

    img
      .decode()
      .then(() => {
        Module.web_loader_load_image_callback_onsuccess(img, loadRequestPtr);
        delete Module.web_loader_image_loads[loaderPtr][urlS];
      })
      .catch((error) => {
        Module.web_loader_load_image_callback_onerror(loadRequestPtr);
        delete Module.web_loader_image_loads[loaderPtr][urlS];
      });
});

EM_JS(void, cancel_load_image, (uintptr_t loaderPtr, const char *url), {
    var urlS = UTF8ToString(url);

    // Cancel loading image.
    var img = Module.web_loader_image_loads?.[loaderPtr]?.[urlS];
    if(img) {
        img.src = "";
    }
    delete Module.web_loader_image_loads?.[loaderPtr]?.[urlS];
});

void WebLoader::processTextureLoadRequest(std::unique_ptr<TextureLoadRequest> textureLoadRequest) {
    // release unique pointer. will be freed in loader
    TextureLoadRequest* loadRequest = textureLoadRequest.release();
    load_image_async(
            reinterpret_cast<uintptr_t>(this),
            loadRequest->url.c_str(),
            reinterpret_cast<uintptr_t>(loadRequest));
}

void WebLoader::processDataLoadRequest(std::unique_ptr<DataLoadRequest> dataLoadRequest) {
    // release unique pointer, will be freed in onsuccess/onerror.
    DataLoadRequest* loadRequest = dataLoadRequest.release();

    // Fetch attributes setup
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.userData = reinterpret_cast<void *>(loadRequest);
    attr.onsuccess = [](emscripten_fetch_t *fetch) {
        auto loadRequest = reinterpret_cast<DataLoadRequest *>(fetch->userData);
        auto loader = loadRequest->loader.lock();
        if(loader) {
            loader->ongoingDataLoads.erase(loadRequest->url);
        }
        if (fetch->status != 200) {
            loadRequest->promise.setValue(
                DataLoaderResult({}, "", LoaderStatus::ERROR_OTHER, "Failed with status: " + std::to_string(fetch->status)));
        } else {
            loadRequest->promise.setValue(
                DataLoaderResult(djinni::DataRef(fetch->data, fetch->numBytes), "", LoaderStatus::OK, ""));
        }
        emscripten_fetch_close(fetch);
        delete loadRequest;
    };
    attr.onerror = [](emscripten_fetch_t *fetch) {
        const bool cancelled = (fetch->status == (unsigned short)-1); // undocumented, see emscripten_fetch.c emscripten_fetch_close

        auto loadRequest = reinterpret_cast<DataLoadRequest *>(fetch->userData);

        if(!cancelled) {
            auto loader = loadRequest->loader.lock();
            if(loader) {
                loader->ongoingDataLoads.erase(loadRequest->url);
            }
        }
        loadRequest->promise.setValue(
            DataLoaderResult({}, "", LoaderStatus::ERROR_OTHER, "Failed with status: " + std::to_string(fetch->status)));
        if(!cancelled) { // important! otherwise inifinite recursion
            emscripten_fetch_close(fetch);
        }
        delete loadRequest;
    };

    emscripten_fetch_t *fetch = emscripten_fetch(&attr, loadRequest->url.c_str());
    ongoingDataLoads[loadRequest->url] = fetch;
}

std::shared_ptr<LoaderInterface> WebLoader::asLoaderInterface() {
    return std::static_pointer_cast<LoaderInterface>(shared_from_this());
}

std::shared_ptr<FontLoaderInterface> WebLoader::createFontLoader(const std::string &baseUrl) {
    return std::static_pointer_cast<FontLoaderInterface>(std::make_shared<FontLoader>(shared_from_this(), baseUrl));
}

void WebLoader::processLoadRequests() {
    // Somewhat unrelated, but a good place to call this.
    ImageValTextureHolder::collectGarbage();

    std::lock_guard<std::mutex> lock(mutex);
    for(const std::string &url : cancellationQueue) {
        // cancel data load
        auto it = ongoingDataLoads.find(url);
        if(it != ongoingDataLoads.end()) {
            // calls onerror unless already done.
            emscripten_fetch_close(it->second);
            ongoingDataLoads.erase(it);
        }
        // cancel texture load
        cancel_load_image(reinterpret_cast<uintptr_t>(this), url.c_str());
    }
    cancellationQueue.clear();
    for(auto &&request : dataLoadQueue) {
        processDataLoadRequest(std::move(request));
    }
    dataLoadQueue.clear();
    for(auto &&request : textureLoadQueue) {
        processTextureLoadRequest(std::move(request));
    }
    textureLoadQueue.clear();
}

TextureLoaderResult WebLoader::loadTexture(const std::string &url, const std::optional<std::string> &etag) {
    // Must not be called from main browser thread
    assert(pthread_self() != emscripten_main_runtime_thread_id());
    return loadTextureAsync(url, etag).get();
}

DataLoaderResult WebLoader::loadData(const std::string &url, const std::optional<std::string> &etag) {
    // Must not be called from main browser thread
    assert(pthread_self() != emscripten_main_runtime_thread_id());
    return loadDataAsync(url, etag).get();
}

djinni::Future<DataLoaderResult> WebLoader::loadDataAsync(const std::string &url, const std::optional<std::string> &etag) {
    return enqueueDataLoadRequest(url, etag);
}

djinni::Future<TextureLoaderResult> WebLoader::loadTextureAsync(const std::string &url, const std::optional<std::string> &etag) {
    return enqueueTextureLoadRequest(url, etag);
}

// Cancel an ongoing fetch request by URL
void WebLoader::cancel(const std::string &url) {
    std::lock_guard<std::mutex> lock(mutex);
    // remove it if still queued
    dataLoadQueue.erase(std::remove_if(
            dataLoadQueue.begin(),
            dataLoadQueue.end(),
            [&url](const std::unique_ptr<DataLoadRequest> &r) { return r->url == url; }),
        dataLoadQueue.end());
    textureLoadQueue.erase(std::remove_if(
            textureLoadQueue.begin(),
            textureLoadQueue.end(),
            [&url](const std::unique_ptr<TextureLoadRequest> &r) { return r->url == url; }),
        textureLoadQueue.end());

    // queue for cancellation on next process call on main thread.
    cancellationQueue.push_back(url);
}

EMSCRIPTEN_BINDINGS(_web_extras_loader) {
    emscripten::function("web_loader_load_image_callback_onsuccess", &web_loader_load_image_callback_onsuccess);
    emscripten::function("web_loader_load_image_callback_onerror", &web_loader_load_image_callback_onerror);
}
