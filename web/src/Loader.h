#pragma once

#include <optional>
#include <string>
#include <memory>
#include "LoaderInterface.h"
#include "DataLoaderResult.h"
#include "TextureLoaderResult.h"
#include "GlobalLoaderInterface.h"

class Loader final : public LoaderInterface 
{
public:
    explicit Loader(const std::shared_ptr<GlobalLoaderInterface> &globalLoader);
    ~Loader() override;

    virtual TextureLoaderResult loadTexture(const std::string &url, const std::optional<std::string> &etag) override;

    virtual DataLoaderResult loadData(const std::string &url, const std::optional<std::string> &etag) override;

    virtual ::djinni::Future<TextureLoaderResult> loadTextureAsync(const std::string &url, const std::optional<std::string> &etag) override;

    virtual ::djinni::Future<DataLoaderResult> loadDataAsync(const std::string &url, const std::optional<std::string> &etag) override;

    virtual void cancel(const std::string &url) override;

private:
    const std::shared_ptr<GlobalLoaderInterface> globalLoader;
};
