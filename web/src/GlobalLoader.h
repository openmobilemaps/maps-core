#pragma once

#include "Loader.h"
#include "DataLoaderResult.h"
#include "FontLoader.h"
#include "TextureLoaderResult.h"
#include "TextureHolderInterface.h"
#include "GlobalLoaderInterface.h"
#include "LoadRequests.h"

#include <memory>
#include <forward_list>
#include <list>
#include <mutex>
#include <string>

class GlobalLoader : public GlobalLoaderInterface, public std::enable_shared_from_this<GlobalLoader> {
public:
    // GlobalLoader(){};

    virtual void processLoadRequests() override;
    virtual std::shared_ptr<LoaderInterface> createLoader() override;
    virtual std::shared_ptr<FontLoaderInterface> createFontLoader(const std::shared_ptr<LoaderInterface> &loader, const std::string &baseUrl) override;

    virtual djinni::Future<DataLoaderResult> enqueueDataLoadRequest(const std::string &url, const std::optional<std::string> &etag) override;
    virtual djinni::Future<TextureLoaderResult> enqueueTextureLoadRequest(const std::string &url, const std::optional<std::string> &etag) override;

    virtual void cancel(const std::string &url) override;

  private:
    std::mutex mutex;
    std::forward_list<DataLoadRequest> dataLoadQueue;
    std::forward_list<TextureLoadRequest> textureLoadQueue;

    std::list<TextureLoadRequest> textureLoads;

    void processDataLoadRequest(DataLoadRequest &&dataLoadRequest);
    void processTextureLoadRequest(TextureLoadRequest &&textureLoadRequest);
};
