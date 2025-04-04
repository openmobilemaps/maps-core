#pragma once
#include "TestData.h"
#include "Tiled2dMapVectorLayerLocalDataProviderInterface.h"

#include <string>
#include <utility>

class TestLocalDataProvider: public Tiled2dMapVectorLayerLocalDataProviderInterface {
public:
    explicit TestLocalDataProvider(const std::unordered_map<std::string, std::string> &&geojsonMapping)
        : geojsonMapping(geojsonMapping) {}

    djinni::Future<DataLoaderResult> loadGeojson(const std::string &sourceName, const std::string &url) override {
        auto promise = ::djinni::Promise<DataLoaderResult>();
        auto it = geojsonMapping.find(sourceName);
        if (it != geojsonMapping.end()) {
            auto data = TestData::readFileToBuffer(("style/" + it->second).c_str());
            std::vector<uint8_t> uint8Vec(data.begin(), data.end());
            promise.setValue(DataLoaderResult(::djinni::DataRef(uint8Vec), std::nullopt, LoaderStatus::OK, std::nullopt));
        } else {
            throw std::runtime_error("Failed to load geojson from file");
        }
        return promise.getFuture();
    }
    std::optional<std::string> getStyleJson() override {
        return std::nullopt;
    }
    djinni::Future<TextureLoaderResult> loadSpriteAsync(int32_t scale) override {
        auto promise = ::djinni::Promise<TextureLoaderResult>();
        promise.setValue(TextureLoaderResult(nullptr, std::nullopt, LoaderStatus::ERROR_404, std::nullopt));
        return promise.getFuture();
    }
    djinni::Future<DataLoaderResult> loadSpriteJsonAsync(int32_t scale) override {
        auto promise = ::djinni::Promise<DataLoaderResult>();
        promise.setValue(DataLoaderResult(std::nullopt, std::nullopt, LoaderStatus::ERROR_404, std::nullopt));
        return promise.getFuture();
    }

private:
    std::unordered_map<std::string, std::string> geojsonMapping;
};