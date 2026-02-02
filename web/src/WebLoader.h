#pragma once

#include "DataLoaderResult.h"
#include "LoadRequests.h"
#include "LoaderInterface.h"
#include "TextureLoaderResult.h"
#include "WebLoaderInterface.h"

#include <memory>
#include <vector>
#include <mutex>
#include <string>

struct emscripten_fetch_t;

// WebLoader is an implementation of the LoaderInterface for the web / emscripten,
// extended with static functions for creation and invoke load request
// processing on the main browser thread.
class WebLoader : public WebLoaderInterface, public LoaderInterface, public std::enable_shared_from_this<WebLoader> {
public:
    virtual ~WebLoader() = default;

    // WebLoaderInterface
    virtual std::shared_ptr<LoaderInterface> asLoaderInterface() override;
    virtual std::shared_ptr<FontLoaderInterface> createFontLoader(const std::string &baseUrl) override;
    virtual void processLoadRequests() override;

    // LoaderInterface
    virtual TextureLoaderResult loadTexture(const std::string & url, const std::optional<std::string> & etag) override;
    virtual DataLoaderResult loadData(const std::string & url, const std::optional<std::string> & etag) override;
    virtual djinni::Future<TextureLoaderResult> loadTextureAsync(const std::string & url, const std::optional<std::string> & etag) override;
    virtual djinni::Future<DataLoaderResult> loadDataAsync(const std::string & url, const std::optional<std::string> & etag) override;
    virtual void cancel(const std::string &url) override;

private:
    djinni::Future<DataLoaderResult> enqueueDataLoadRequest(const std::string &url, const std::optional<std::string> &etag);
    djinni::Future<TextureLoaderResult> enqueueTextureLoadRequest(const std::string &url, const std::optional<std::string> &etag);

    void processDataLoadRequest(std::unique_ptr<DataLoadRequest> dataLoadRequest);
    void processTextureLoadRequest(std::unique_ptr<TextureLoadRequest> textureLoadRequest);
    void collectGarbageTextureHolders();

private:
    std::mutex mutex;
    std::vector<std::unique_ptr<DataLoadRequest>> dataLoadQueue;
    std::vector<std::unique_ptr<TextureLoadRequest>> textureLoadQueue;
    std::vector<std::string> cancellationQueue;

    std::unordered_map<std::string, emscripten_fetch_t*> ongoingDataLoads;
};
