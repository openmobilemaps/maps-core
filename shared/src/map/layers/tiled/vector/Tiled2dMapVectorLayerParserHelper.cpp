/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "Tiled2dMapVectorLayerParserHelper.h"

#include "Logger.h"
#include "json.h"
#include "VectorMapSourceDescription.h"
#include "DataLoaderResult.h"
#include "Tiled2dMapVectorLayer.h"
#include "LineVectorLayerDescription.h"
#include "PolygonVectorLayerDescription.h"
#include "SymbolVectorLayerDescription.h"
#include "RasterVectorLayerDescription.h"
#include "BackgroundVectorLayerDescription.h"
#include "ColorUtil.h"
#include "Tiled2dMapVectorStyleParser.h"
#include "Tiled2dMapVectorLayerParserResult.h"
#include "LoaderHelper.h"
#include "GeoJsonParser.h"
#include "GeoJsonVTFactory.h"


Tiled2dMapVectorLayerParserResult Tiled2dMapVectorLayerParserHelper::parseStyleJsonFromUrl(const std::string &layerName,
                                                        const std::string &styleJsonUrl,
                                                        const std::shared_ptr<Tiled2dMapVectorLayerLocalDataProviderInterface> &localDataProvider,
                                                        const std::vector<std::shared_ptr<::LoaderInterface>> &loaders, const std::unordered_map<std::string, std::string> & sourceUrlParams) {
    DataLoaderResult result = LoaderHelper::loadData(styleJsonUrl, std::nullopt, loaders);
    if (result.status != LoaderStatus::OK) {
        LogError <<= "Unable to Load style.json from " + styleJsonUrl + " errorCode: " + (result.errorCode ? *result.errorCode : "");
        return Tiled2dMapVectorLayerParserResult(nullptr, result.status, result.errorCode, std::nullopt);
    }
    auto string = std::string((char*)result.data->buf(), result.data->len());

    return parseStyleJsonFromString(layerName, string, localDataProvider, loaders, sourceUrlParams);
};

std::string Tiled2dMapVectorLayerParserHelper::replaceUrlParams(const std::string & url, const std::unordered_map<std::string, std::string> & sourceUrlParams) {
    std::string replaced = url;
    for (const auto & param : sourceUrlParams) {
        size_t xIndex = replaced.find("{" + param.first + "}", 0);
        if (xIndex == std::string::npos) continue;;
        replaced = replaced.replace(xIndex, param.first.size() + 2, param.second);
    }
    return replaced;
}

Tiled2dMapVectorLayerParserResult Tiled2dMapVectorLayerParserHelper::parseStyleJsonFromString(const std::string &layerName,
                                                        const std::string &styleJsonString,
                                                        const std::shared_ptr<Tiled2dMapVectorLayerLocalDataProviderInterface> &localDataProvider,
                                                        const std::vector<std::shared_ptr<::LoaderInterface>> &loaders, const std::unordered_map<std::string, std::string> & sourceUrlParams) {

    nlohmann::json json;

    try {
        json = nlohmann::json::parse(styleJsonString);
    }
    catch (nlohmann::json::parse_error &ex) {
        return Tiled2dMapVectorLayerParserResult(nullptr, LoaderStatus::ERROR_OTHER, "", std::nullopt);
    }

    std::vector<std::shared_ptr<VectorLayerDescription>> layers;

    std::map<std::string, std::shared_ptr<RasterVectorLayerDescription>> rasterLayerMap;

    std::map<std::string, std::shared_ptr<GeoJSONVTInterface>> geojsonSources;

    std::map<std::string, nlohmann::json> tileJsons;
    for (auto&[key, val]: json["sources"].items()) {
        if (!val["type"].is_string()) {
            assert(false);
            continue;
        }
        const auto type = val["type"].get<std::string>();
        if (type == "raster") {
            std::string url;

            bool adaptScaleToScreen = true;
            int32_t numDrawPreviousLayers = 0;
            bool maskTiles = true;
            double zoomLevelScaleFactor = 1.0;

            bool overzoom = true;
            bool underzoom = false;

            int minZoom = val.value("minzoom", 0);
            int maxZoom = val.value("maxzoom", 22);
            std::optional<::RectCoord> bounds;
            std::optional<std::string> coordinateReferenceSystem;

            if (val["tiles"].is_array()) {
                auto str = val.dump();
                url = val["tiles"].begin()->get<std::string>();
                adaptScaleToScreen = val.value("adaptScaleToScreen", adaptScaleToScreen);
                numDrawPreviousLayers = val.value("numDrawPreviousLayers", numDrawPreviousLayers);
                maskTiles = val.value("maskTiles", maskTiles);
                zoomLevelScaleFactor = val.value("zoomLevelScaleFactor", zoomLevelScaleFactor);

                if (val.contains("bounds")) {
                    auto tmpBounds = std::vector<double>();
                    for (auto &el: val["bounds"].items()) {
                        tmpBounds.push_back(el.value().get<double>());
                    }
                    if (tmpBounds.size() == 4) {
                        const auto topLeft = Coord(CoordinateSystemIdentifiers::EPSG4326(), tmpBounds[0], tmpBounds[1], 0);
                        const auto bottomRight = Coord(CoordinateSystemIdentifiers::EPSG4326(), tmpBounds[2], tmpBounds[3], 0);
                        bounds = RectCoord(topLeft, bottomRight);
                    }
                }

                if(val["metadata"].is_object() && val["metadata"].contains("crs") && val["metadata"]["crs"].is_string()) {
                    coordinateReferenceSystem = val["metadata"]["crs"].get<std::string>();
                }

            } else if (val["url"].is_string()) {
                auto result = LoaderHelper::loadData(replaceUrlParams(val["url"].get<std::string>(), sourceUrlParams), std::nullopt, loaders);
                if (result.status != LoaderStatus::OK) {
                    return Tiled2dMapVectorLayerParserResult(nullptr, result.status, result.errorCode, std::nullopt);
                }
                auto string = std::string((char *) result.data->buf(), result.data->len());
                nlohmann::json json;
                try {
                    json = nlohmann::json::parse(string);
                }
                catch (nlohmann::json::parse_error &ex) {
                    return Tiled2dMapVectorLayerParserResult(nullptr, LoaderStatus::ERROR_OTHER, "", std::nullopt);
                }
                url = json["tiles"].begin()->get<std::string>();

                if (json.contains("bounds")) {
                    auto tmpBounds = std::vector<double>();
                    for (auto &el: json["bounds"].items()) {
                        tmpBounds.push_back(el.value().get<double>());
                    }
                    if (tmpBounds.size() == 4) {
                        const auto topLeft = Coord(CoordinateSystemIdentifiers::EPSG4326(), tmpBounds[0], tmpBounds[1], 0);
                        const auto bottomRight = Coord(CoordinateSystemIdentifiers::EPSG4326(), tmpBounds[2], tmpBounds[3], 0);
                        bounds = RectCoord(topLeft, bottomRight);
                    }
                }

                if(json["metadata"].is_object() && json["metadata"].contains("crs") && json["metadata"]["crs"].is_string()) {
                    coordinateReferenceSystem = json["metadata"]["crs"].get<std::string>();
                }

                minZoom = json.value("minzoom", 0);
                maxZoom = json.value("maxzoom", 22);
            }


            RasterVectorStyle style = RasterVectorStyle(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
            rasterLayerMap[key] = std::make_shared<RasterVectorLayerDescription>(layerName,
                                                                                 key,
                                                                                 minZoom,
                                                                                 maxZoom,
                                                                                 url,
                                                                                 nullptr,
                                                                                 style,
                                                                                 adaptScaleToScreen,
                                                                                 numDrawPreviousLayers,
                                                                                 maskTiles,
                                                                                 zoomLevelScaleFactor,
                                                                                 std::nullopt,
                                                                                 nullptr,
                                                                                 underzoom,
                                                                                 overzoom,
                                                                                 bounds,
                                                                                 coordinateReferenceSystem);

        } else if (type == "vector" && val["url"].is_string()) {
            auto result = LoaderHelper::loadData(replaceUrlParams(val["url"].get<std::string>(), sourceUrlParams), std::nullopt, loaders);
            if (result.status != LoaderStatus::OK) {
                return Tiled2dMapVectorLayerParserResult(nullptr, result.status, result.errorCode, std::nullopt);
            }
            auto string = std::string((char*)result.data->buf(), result.data->len());
            try {
                tileJsons[key] = nlohmann::json::parse(string);
            }
            catch (nlohmann::json::parse_error &ex) {
                return Tiled2dMapVectorLayerParserResult(nullptr, LoaderStatus::ERROR_OTHER, "", std::nullopt);
            }

        } else if (type == "vector" && val["tiles"].is_array()) {
            tileJsons[key] = val;
        } else if (type == "geojson") {
            nlohmann::json geojson;
            Options options;

            if (val["minzoom"].is_number_integer()) {
                options.minZoom = val["minzoom"].get<uint8_t>();
            }
            if (val["maxzoom"].is_number_integer()) {
                options.maxZoom = val["maxzoom"].get<uint8_t>();
            }
            if (val["data"].is_string()) {
                geojsonSources[key] = GeoJsonVTFactory::getGeoJsonVt(key, replaceUrlParams(val["data"].get<std::string>(), sourceUrlParams), loaders, localDataProvider, options);
            } else {
                assert(val["data"].is_object());
                // XXX: segfault on invalid data
                // XXX: fails on "type": "Feature" instead of "FeatureCollection"
                // XXX: fails if there is no "properties"
                geojsonSources[key] = GeoJsonVTFactory::getGeoJsonVt(GeoJsonParser::getGeoJson(val["data"]), options);
            }
        }
    }

    Tiled2dMapVectorStyleParser parser;

    std::optional<std::string> metadata;
    std::shared_ptr<Value> globalIsInteractable;
    bool persistingSymbolPlacement = false;

    if(json["metadata"].is_object()) {
        metadata = json["metadata"].dump();
        globalIsInteractable = parser.parseValue(json["metadata"]["interactable"]);
        persistingSymbolPlacement = json["metadata"].value("persistingSymbolPlacement", false);
    }

    int64_t globalTransitionDuration = 300;
    int64_t globalTransitionDelay = 0;
    if(json["transition"].is_object()) {
        globalTransitionDuration = json["transition"].value("duration", globalTransitionDuration);
        globalTransitionDelay = json["transition"].value("delay", globalTransitionDelay);
    }

    for (auto&[key, val]: json["layers"].items()) {
        if (val["layout"].is_object() && val["layout"]["visibility"] == "none") {
            continue;
        }

        std::optional<int32_t> renderPassIndex;
        std::shared_ptr<Value> interactable = globalIsInteractable;
        bool layerMultiselect = false;
        bool layerSelfMasked = false;

        std::shared_ptr<Value> blendMode;
        if (val["metadata"].is_object()) {
            if (val["metadata"]["render-pass-index"].is_number()) {
                renderPassIndex = val["metadata"].value("render-pass-index", 0);
            }
            if (!val["metadata"]["interactable"].is_null()) {
                interactable = parser.parseValue(val["metadata"]["interactable"]);
            }
            if (!val["metadata"]["multiselect"].is_null()) {
                layerMultiselect = val["metadata"].value("multiselect", false);
            }
            if (!val["metadata"]["selfMasked"].is_null()) {
                layerSelfMasked = val["metadata"].value("selfMasked", false);
            }
            if (!val["metadata"]["blend-mode"].is_null()) {
                blendMode = parser.parseValue(val["metadata"]["blend-mode"]);
            }
        }

        int64_t transitionDuration = globalTransitionDuration;
        int64_t transitionDelay = globalTransitionDelay;
        if(val["transition"].is_object()) {
            transitionDuration = val["transition"].value("duration", transitionDuration);
            transitionDelay = val["transition"].value("delay", transitionDelay);
        }

        if (val["type"] == "background") {
            auto layerDesc = std::make_shared<BackgroundVectorLayerDescription>(val["id"],
                                                                                BackgroundVectorStyle(parser.parseValue(val["paint"]["background-color"]),
                                                                                                      parser.parseValue(val["paint"]["background-pattern"]),
                                                                                                      blendMode),
                                                                                renderPassIndex,
                                                                                interactable);
            layers.push_back(layerDesc);

        } else if (val["type"] == "raster" && rasterLayerMap.count(val["source"]) != 0) {
            auto layer = rasterLayerMap[val["source"]];
            RasterVectorStyle style = RasterVectorStyle(parser.parseValue(val["paint"]["raster-opacity"]),
                                                        parser.parseValue(val["paint"]["raster-brightness-min"]),
                                                        parser.parseValue(val["paint"]["raster-brightness-max"]),
                                                        parser.parseValue(val["paint"]["raster-contrast"]),
                                                        parser.parseValue(val["paint"]["raster-saturation"]),
                                                        parser.parseValue(val["metadata"]["raster-gamma"]),
                                                        parser.parseValue(val["metadata"]["raster-brightness-shift"]),
                                                        blendMode);
            std::shared_ptr<Value> filter = parser.parseValue(val["filter"]);
            
            auto newLayer = std::make_shared<RasterVectorLayerDescription>(val["id"],
                                                                           val["source"],
                                                                           val.value("minzoom", layer->minZoom),
                                                                           val.value("maxzoom", layer->maxZoom),
                                                                           layer->url,
                                                                           filter,
                                                                           style,
                                                                           layer->adaptScaleToScreen,
                                                                           layer->numDrawPreviousLayers,
                                                                           layer->maskTiles,
                                                                           layer->zoomLevelScaleFactor,
                                                                           layer->renderPassIndex,
                                                                           interactable,
                                                                           layer->underzoom,
                                                                           layer->overzoom,
                                                                           layer->bounds,
                                                                           layer->coordinateReferenceSystem);
            layers.push_back(newLayer);
        } else if (val["type"] == "line") {

            std::shared_ptr<Value> filter = parser.parseValue(val["filter"]);
            LineVectorStyle style = LineVectorStyle(
                    parser.parseValue(val["paint"]["line-color"]),
                    parser.parseValue(val["paint"]["line-opacity"]),
                    parser.parseValue(val["paint"]["line-width"]),
                    parser.parseValue(val["paint"]["line-dasharray"]),
                    parser.parseValue(val["paint"]["line-blur"]),
                    parser.parseValue(val["layout"]["line-cap"]),
                    parser.parseValue(val["paint"]["line-offset"]),
                    blendMode,
                    parser.parseValue(val["paint"]["line-dotted"]),
                    parser.parseValue(val["paint"]["line-dotted-skew"])
            );
            auto layerDesc = std::make_shared<LineVectorLayerDescription>(
                    val["id"],
                    val["source"],
                    val.value("source-layer", ""),
                    val.value("minzoom", 0),
                    val.value("maxzoom", 24),
                    filter,
                    style,
                    renderPassIndex,
                    interactable,
                    layerMultiselect,
                    layerSelfMasked);

            layers.push_back(layerDesc);

        } else if (val["type"] == "symbol") {

            SymbolVectorStyle style(parser.parseValue(val["layout"]["text-size"]),
                                    parser.parseValue(val["paint"]["text-color"]),
                                    parser.parseValue(val["paint"]["text-halo-color"]),
                                    parser.parseValue(val["paint"]["text-halo-width"]),
                                    parser.parseValue(val["paint"]["text-halo-blur"]),
                                    parser.parseValue(val["paint"]["text-opacity"]),
                                    parser.parseValue(val["layout"]["text-font"]),
                                    parser.parseValue(val["layout"]["text-field"]),
                                    parser.parseValue(val["layout"]["text-transform"]),
                                    parser.parseValue(val["layout"]["text-offset"]),
                                    parser.parseValue(val["layout"]["text-radial-offset"]),
                                    parser.parseValue(val["layout"]["text-padding"]),
                                    parser.parseValue(val["layout"]["text-anchor"]),
                                    parser.parseValue(val["layout"]["text-justify"]),
                                    parser.parseValue(val["layout"]["text-variable-anchor"]),
                                    parser.parseValue(val["layout"]["text-rotate"]),
                                    parser.parseValue(val["layout"]["text-allow-overlap"]),
                                    parser.parseValue(val["layout"]["text-rotation-alignment"]),
                                    parser.parseValue(val["layout"]["text-optional"]),
                                    parser.parseValue(val["layout"]["symbol-sort-key"]),
                                    parser.parseValue(val["layout"]["symbol-spacing"]),
                                    parser.parseValue(val["layout"]["symbol-placement"]),
                                    parser.parseValue(val["layout"]["icon-rotation-alignment"]),
                                    parser.parseValue(val["layout"]["icon-image"]),
                                    parser.parseValue(val["layout"]["icon-image-custom-provider"]),
                                    parser.parseValue(val["layout"]["icon-anchor"]),
                                    parser.parseValue(val["layout"]["icon-offset"]),
                                    parser.parseValue(val["layout"]["icon-size"]),
                                    parser.parseValue(val["layout"]["icon-allow-overlap"]),
                                    parser.parseValue(val["layout"]["icon-padding"]),
                                    parser.parseValue(val["layout"]["icon-text-fit"]),
                                    parser.parseValue(val["layout"]["icon-text-fit-padding"]),
                                    parser.parseValue(val["paint"]["icon-opacity"]),
                                    parser.parseValue(val["layout"]["icon-rotate"]),
                                    parser.parseValue(val["layout"]["icon-optional"]),
                                    parser.parseValue(val["layout"]["text-line-height"]),
                                    parser.parseValue(val["layout"]["text-letter-spacing"]),
                                    parser.parseValue(val["layout"]["text-max-width"]),
                                    parser.parseValue(val["layout"]["text-max-angle"]),
                                    parser.parseValue(val["layout"]["symbol-z-order"]),
                                    blendMode,
                                    transitionDuration,
                                    transitionDelay);

            std::shared_ptr<Value> filter = parser.parseValue(val["filter"]);

            auto layerDesc = std::make_shared<SymbolVectorLayerDescription>(val["id"],
                                                                            val["source"],
                                                                            val.value("source-layer", ""),
                                                                            val.value("minzoom", 0),
                                                                            val.value("maxzoom", 24),
                                                                            filter,
                                                                            style,
                                                                            renderPassIndex,
                                                                            interactable,
                                                                            layerSelfMasked); // in-layer layerMultiselect not yet supported
            layers.push_back(layerDesc);
        } else if (val["type"] == "fill") {

            std::shared_ptr<Value> filter = parser.parseValue(val["filter"]);

            PolygonVectorStyle style(parser.parseValue(val["paint"]["fill-color"]),
                                     parser.parseValue(val["paint"]["fill-opacity"]),
                                     parser.parseValue(val["paint"]["fill-pattern"]),
                                     blendMode,
                                     (!val["layout"]["fadeInPattern"].is_null()) && val["layout"].value("fadeInPattern", false),
                                     parser.parseValue(val["paint"]["stripe-width"]));

            auto layerDesc = std::make_shared<PolygonVectorLayerDescription>(val["id"],
                                                                             val["source"],
                                                                             val.value("source-layer", ""),
                                                                             val.value("minzoom", 0),
                                                                             val.value("maxzoom", 24),
                                                                             filter,
                                                                             style,
                                                                             renderPassIndex,
                                                                             interactable,
                                                                             layerMultiselect,
                                                                             layerSelfMasked);

            layers.push_back(layerDesc);
        }
    }

    std::vector<std::shared_ptr<VectorMapSourceDescription>> sourceDescriptions;
    for (auto const &[identifier, tileJson]: tileJsons) {
        std::optional<::RectCoord> bounds;
        if (tileJson.contains("bounds")) {
            auto tmpBounds = std::vector<double>();
            for (auto &el: tileJson["bounds"].items()) {
                double d = el.value().get<double>();
                tmpBounds.push_back(d);
            }
            if (tmpBounds.size() == 4) {
                const auto topLeft = Coord(CoordinateSystemIdentifiers::EPSG4326(), tmpBounds[0], tmpBounds[1], 0);
                const auto bottomRight = Coord(CoordinateSystemIdentifiers::EPSG4326(), tmpBounds[2], tmpBounds[3], 0);
                bounds = RectCoord(topLeft, bottomRight);
            }
        }
        auto zoomLevelScaleFactor = tileJson.contains("zoomLevelScaleFactor") ? std::optional<float>(tileJson["zoomLevelScaleFactor"].get<float>()) : std::nullopt;
        auto adaptScaleToScreen = tileJson.contains("adaptScaleToScreen") ? std::optional<bool>(tileJson["adaptScaleToScreen"].get<bool>()) : std::nullopt;
        auto numDrawPreviousLayers = tileJson.contains("numDrawPreviousLayers") ? std::optional<int>(tileJson["numDrawPreviousLayers"].get<int>()) : std::nullopt;
        auto underzoom = tileJson.contains("underzoom") ? std::optional<bool>(tileJson["underzoom"].get<bool>()) : std::nullopt;
        auto overzoom = tileJson.contains("overzoom") ? std::optional<bool>(tileJson["overzoom"].get<bool>()) : std::nullopt;
        sourceDescriptions.push_back(
                std::make_shared<VectorMapSourceDescription>(identifier,
                                                             tileJson["tiles"].begin()->get<std::string>(),
                                                             tileJson["minzoom"].get<int>(),
                                                             tileJson["maxzoom"].get<int>(),
                                                             bounds,
                                                             zoomLevelScaleFactor,
                                                             adaptScaleToScreen,
                                                             numDrawPreviousLayers,
                                                             underzoom,
                                                             overzoom
                ));
    }


    std::optional<std::string> sprite;
    if (json["sprite"].is_string()) {
        sprite = json["sprite"].get<std::string>();
    }

    auto mapDesc = std::make_shared<VectorMapDescription>(layerName,
                                                          sourceDescriptions,
                                                          layers,
                                                          sprite,
                                                          geojsonSources,
                                                          persistingSymbolPlacement);
    return Tiled2dMapVectorLayerParserResult(mapDesc, LoaderStatus::OK, "", metadata);
};
