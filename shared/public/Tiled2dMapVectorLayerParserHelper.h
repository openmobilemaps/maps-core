/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#pragma once

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
#include <string>

class Tiled2dMapVectorLayerParserHelper {
public:
    static Tiled2dMapVectorLayerParserResult parseStyleJsonFromUrl(const std::string &layerName,
                                                            const std::string &styleJsonUrl,
                                                            const double &dpFactor,
                                                            const std::vector<std::shared_ptr<::LoaderInterface>> &loaders) {
        DataLoaderResult result = LoaderHelper::loadData(styleJsonUrl, std::nullopt, loaders);
        if (result.status != LoaderStatus::OK) {
            LogError <<= "Unable to Load style.json from " + styleJsonUrl + " errorCode: " + (result.errorCode ? *result.errorCode : "");
            return Tiled2dMapVectorLayerParserResult(nullptr, result.status, result.errorCode, std::nullopt);
        }
        auto string = std::string((char*)result.data->buf(), result.data->len());

        return parseStyleJsonFromString(layerName, string, dpFactor, loaders);
    }

    static Tiled2dMapVectorLayerParserResult parseStyleJsonFromString(const std::string &layerName,
                                                            const std::string &styleJsonString,
                                                            const double &dpFactor,
                                                            const std::vector<std::shared_ptr<::LoaderInterface>> &loaders) {

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
                double zoomLevelScaleFactor = 0.65;
                
                bool overzoom = true;
                bool underzoom = false;

                int minZoom = val.value("minzoom", 0);
                int maxZoom = val.value("maxzoom", 22);
                
                if (val["tiles"].is_array()) {
                    auto str = val.dump();
                    url = val["tiles"].begin()->get<std::string>();
                    adaptScaleToScreen = val.value("adaptScaleToScreen", adaptScaleToScreen);
                    numDrawPreviousLayers = val.value("numDrawPreviousLayers", numDrawPreviousLayers);
                    maskTiles = val.value("maskTiles", maskTiles);
                    zoomLevelScaleFactor = val.value("zoomLevelScaleFactor", zoomLevelScaleFactor);
                } else if (val["url"].is_string()) {
                    auto result = LoaderHelper::loadData(val["url"].get<std::string>(), std::nullopt, loaders);
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
                    
                    minZoom = json.value("minzoom", 0);
                    maxZoom = json.value("maxzoom", 22);
                }
                
                rasterLayerMap[key] = std::make_shared<RasterVectorLayerDescription>(layerName,
                                                                                     key,
                                                                                     minZoom,
                                                                                     maxZoom,
                                                                                     url,
                                                                                     RasterVectorStyle(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr),
                                                                                     adaptScaleToScreen,
                                                                                     numDrawPreviousLayers,
                                                                                     maskTiles,
                                                                                     zoomLevelScaleFactor,
                                                                                     std::nullopt,
                                                                                     nullptr,
                                                                                     underzoom,
                                                                                     overzoom);
                
            } else if (type == "vector" && val["url"].is_string()) {
                auto result = LoaderHelper::loadData(val["url"].get<std::string>(), std::nullopt, loaders);
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
                if (val["data"].is_string()) {
                    // load geojson
                    auto result = LoaderHelper::loadData(val["data"].get<std::string>(), std::nullopt, loaders);
                    if (result.status != LoaderStatus::OK) {
                        return Tiled2dMapVectorLayerParserResult(nullptr, result.status, result.errorCode, std::nullopt);
                    }
                    auto string = std::string((char*)result.data->buf(), result.data->len());
                    nlohmann::json json;
                    try {
                        geojson = nlohmann::json::parse(string);
                    }
                    catch (nlohmann::json::parse_error &ex) {
                        return Tiled2dMapVectorLayerParserResult(nullptr, LoaderStatus::ERROR_OTHER, "", std::nullopt);
                    }
                } else {
                    assert(val["data"].is_object());
                    geojson = val["data"];
                }

                geojsonSources[key] = GeoJsonParser::getGeoJsonVt(geojson);
            }
        }

        Tiled2dMapVectorStyleParser parser;

        std::optional<std::string> metadata;
        std::shared_ptr<Value> globalIsInteractable;

        if(json["metadata"].is_object()) {
            metadata = json["metadata"].dump();
            globalIsInteractable = parser.parseValue(json["metadata"]["interactable"]);
        }

        for (auto&[key, val]: json["layers"].items()) {
			if (val["layout"].is_object() && val["layout"]["visibility"] == "none") {
                continue;
            }

            std::optional<int32_t> renderPassIndex;
            std::shared_ptr<Value> interactable = globalIsInteractable;

            std::shared_ptr<Value> blendMode;
            if (val["metadata"].is_object()) {
                if (val["metadata"]["render-pass-index"].is_number()) {
                    renderPassIndex = val["metadata"].value("render-pass-index", 0);
                }
                if (!val["metadata"]["interactable"].is_null()) {
                    interactable = parser.parseValue(val["metadata"]["interactable"]);
                }
                if (!val["metadata"]["blend-mode"].is_null()) {
                    blendMode = parser.parseValue(val["metadata"]["blend-mode"]);
                }
            }

            if (val["type"] == "background" && !val["paint"]["background-color"].is_null()) {
                auto layerDesc = std::make_shared<BackgroundVectorLayerDescription>(val["id"],
                                                                                    BackgroundVectorStyle(parser.parseValue(
                                                                                            val["paint"]["background-color"]),
                                                                                                          blendMode),
                                                                                    renderPassIndex,
                                                                                    interactable);
                layers.push_back(layerDesc);
                
            } else if (val["type"] == "raster" && rasterLayerMap.count(val["source"]) != 0) {
                auto layer = rasterLayerMap.at(val["source"]);

                auto newLayer = std::make_shared<RasterVectorLayerDescription>(val["id"],
                                             val["source"],
                                             val.value("minzoom", layer->minZoom),
                                             val.value("maxzoom", layer->maxZoom),
                                             layer->url,
                                             RasterVectorStyle(parser.parseValue(val["paint"]["raster-opacity"]),
                                                                parser.parseValue(val["paint"]["raster-brightness-min"]),
                                                                parser.parseValue(val["paint"]["raster-brightness-max"]),
                                                                parser.parseValue(val["paint"]["raster-contrast"]),
                                                                parser.parseValue(val["paint"]["raster-saturation"]),
                                                                parser.parseValue(val["metadata"]["raster-gamma"]),
                                                                blendMode),
                                             layer->adaptScaleToScreen,
                                             layer->numDrawPreviousLayers,
                                             layer->maskTiles,
                                             layer->zoomLevelScaleFactor,
                                             layer->renderPassIndex,
                                             interactable,
                                             layer->underzoom,
                                             layer->overzoom);
                layers.push_back(newLayer);
            } else if (val["type"] == "line") {

                    std::shared_ptr<Value> filter = parser.parseValue(val["filter"]);

                    auto layerDesc = std::make_shared<LineVectorLayerDescription>(
                        val["id"],
                        val["source"],
                        val.value("source-layer", ""),
                        val.value("minzoom", 0),
                        val.value("maxzoom", 24),
                        filter,
                        LineVectorStyle(
                            parser.parseValue(val["paint"]["line-color"]),
                            parser.parseValue(val["paint"]["line-opacity"]),
                            parser.parseValue(val["paint"]["line-width"]),
                            parser.parseValue(val["paint"]["line-dasharray"]),
                            parser.parseValue(val["paint"]["line-blur"]),
                            parser.parseValue(val["layout"]["line-cap"]),
                            parser.parseValue(val["paint"]["line-offset"]),
                            blendMode,
                            dpFactor
                        ),
                        renderPassIndex,
                        interactable);

                    layers.push_back(layerDesc);

            } else if (val["type"] == "symbol") {

                    SymbolVectorStyle style(parser.parseValue(val["layout"]["text-size"]),
                                            parser.parseValue(val["paint"]["text-color"]),
                                            parser.parseValue(val["paint"]["text-halo-color"]),
                                            parser.parseValue(val["paint"]["text-halo-width"]),
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
                                            dpFactor);

                    std::shared_ptr<Value> filter = parser.parseValue(val["filter"]);

                    auto layerDesc = std::make_shared<SymbolVectorLayerDescription>(val["id"],
                                                                                    val["source"],
                                                                                    val.value("source-layer", ""),
                                                                                    val.value("minzoom", 0),
                                                                                    val.value("maxzoom", 24),
                                                                                    filter,
                                                                                    style,
                                                                                    renderPassIndex,
                                                                                    interactable);
                    layers.push_back(layerDesc);
                } else if (val["type"] == "fill") {

                    std::shared_ptr<Value> filter = parser.parseValue(val["filter"]);

                    PolygonVectorStyle style(parser.parseValue(val["paint"]["fill-color"]),
                                             parser.parseValue(val["paint"]["fill-opacity"]),
                                             parser.parseValue(val["paint"]["fill-pattern"]),
                                             blendMode);

                    auto layerDesc = std::make_shared<PolygonVectorLayerDescription>(val["id"],
                                                                                     val["source"],
                                                                                     val.value("source-layer", ""),
                                                                                     val.value("minzoom", 0),
                                                                                     val.value("maxzoom", 24),
                                                                                     filter,
                                                                                     style,
                                                                                     renderPassIndex,
                                                                                     interactable);

                    layers.push_back(layerDesc);
                }
        }

        std::vector<std::shared_ptr<VectorMapSourceDescription>> sourceDescriptions;
        for (auto const &[identifier, tileJson]: tileJsons) {
            sourceDescriptions.push_back(std::make_shared<VectorMapSourceDescription>(identifier,
                                                                   tileJson["tiles"].begin()->get<std::string>(),
                                                                   tileJson["minzoom"].get<int>(),
                                                                                     tileJson["maxzoom"].get<int>()));
        }


        std::optional<std::string> sprite;
        if (json["sprite"].is_string()) {
            sprite = json["sprite"].get<std::string>();
        }

        auto mapDesc = std::make_shared<VectorMapDescription>(layerName,
                                                              sourceDescriptions,
                                                              layers,
                                                              sprite,
                                                              geojsonSources);
        return Tiled2dMapVectorLayerParserResult(mapDesc, LoaderStatus::OK, "", metadata);
    }
};
