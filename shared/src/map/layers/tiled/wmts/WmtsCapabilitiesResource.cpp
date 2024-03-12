/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "WmtsCapabilitiesResource.h"
#include "CoordinateConversionHelper.h"
#include "CoordinateSystemIdentifiers.h"
#include "Tiled2dMapRasterLayerInterface.h"
#include "WmtsLayerDescription.h"
#include "WmtsLayerDimension.h"
#include "WmtsTileMatrixSet.h"
#include "WmtsTiled2dMapLayerConfigFactory.h"
#include "pugixml.h"

class WmtsCapabilitiesResourceImpl : public WmtsCapabilitiesResource {

  public:
    WmtsCapabilitiesResourceImpl(const std::string &xml) {
        doc.load_string(xml.c_str());
        parseDoc();
    };

    std::shared_ptr<::Tiled2dMapRasterLayerInterface> createLayer(const std::string &identifier,
                                                                  const std::vector<std::shared_ptr<::LoaderInterface>> & tileLoaders) override {
        return createLayerTimed(identifier, tileLoaders, 1);
    };

    std::shared_ptr<::Tiled2dMapRasterLayerInterface> createLayerTimed(const std::string & identifier, 
                                                                       const std::vector<std::shared_ptr<::LoaderInterface>> & tileLoaders, 
                                                                      int32_t numZ) override {
        auto layerConfig = createLayerConfigTimed(identifier, numZ);
        if (!layerConfig)
            return nullptr;
        return Tiled2dMapRasterLayerInterface::create(layerConfig, tileLoaders);
    }


    std::shared_ptr<::Tiled2dMapRasterLayerInterface> createLayerWithZoomInfoTimed(const std::string & identifier, 
const std::vector<std::shared_ptr<::LoaderInterface>> & tileLoaders, 
const ::Tiled2dMapZoomInfo & zoomInfo, 
int32_t numZ) override {
        auto layerConfig = createLayerConfigWithZoomInfoTimed(identifier, zoomInfo, numZ);
        if (!layerConfig)
            return nullptr;
        return Tiled2dMapRasterLayerInterface::create(layerConfig, tileLoaders);
    }


    std::shared_ptr<::Tiled2dMapLayerConfig> createLayerConfigTimed(const std::string & identifier, int32_t numZ) override {
        return createLayerConfigWithZoomInfoTimed(identifier, Tiled2dMapZoomInfo(2.25, 2, false, true, true, true), numZ);
    }


    std::shared_ptr<::Tiled2dMapLayerConfig> createLayerConfigWithZoomInfoTimed(const std::string & identifier, const ::Tiled2dMapZoomInfo & zoomInfo, int32_t numZ) override {
        if (!layers.count(identifier)) {
            return nullptr;
        }

        auto description = layers.at(identifier);

        auto matrixSet = matrixSets.at(description.tileMatrixSetLink);

        std::unordered_map<std::string, std::string> dimensions;
        for (auto &dimension : description.dimensions) {
            dimensions.insert(std::make_pair(dimension.identifier, dimension.defaultValue));
        }
        dimensions.insert(std::make_pair("TileMatrixSet", description.tileMatrixSetLink));

        std::vector<::Tiled2dMapZoomLevelInfo> zoomLevels;

        int32_t coordinateSystem = matrixSet.coordinateSystemIdentifier;

        auto bounds = RectCoord(Coord(0, 0, 0, 0), Coord(0, 0, 0, 0));

        for (auto &matrix : matrixSet.matrices) {
            int32_t zoomLevelIdentifier = stoi(matrix.identifier);
            auto topLeft = Coord(coordinateSystem, matrix.topLeftCornerX, matrix.topLeftCornerY, 0);

            double magicNumber = 0.00028; // Each pixel is assumed to be 0.28mm – https://gis.stackexchange.com/a/315989
            double right =
            topLeft.x + matrix.scaleDenominator * (double)matrix.tileWidth * (double)matrix.matrixWidth * magicNumber;
            double bottom =
            topLeft.y - matrix.scaleDenominator * (double)matrix.tileHeight * (double)matrix.tileWidth * magicNumber;
            Coord bottomRight = Coord(topLeft.systemIdentifier, right, bottom, 0);
            RectCoord layerBounds = RectCoord(topLeft, bottomRight);

            double scaledTileWidth = matrix.scaleDenominator * (double)matrix.tileWidth * magicNumber;
            auto levelInfo = Tiled2dMapZoomLevelInfo(matrix.scaleDenominator, scaledTileWidth, matrix.matrixWidth,
                                                     matrix.matrixHeight, numZ, zoomLevelIdentifier, layerBounds);
            zoomLevels.push_back(levelInfo);

            bounds = layerBounds;
        }

        return WmtsTiled2dMapLayerConfigFactory::create(description, zoomLevels, zoomInfo, matrixSet.coordinateSystemIdentifier,
                                                        matrixSet.identifier);

    }



    std::shared_ptr<::Tiled2dMapRasterLayerInterface> createLayerWithZoomInfo(const std::string &identifier,
                                                                              const std::vector<std::shared_ptr<::LoaderInterface>> & tileLoaders,
                                                                              const ::Tiled2dMapZoomInfo &zoomInfo) override {
        return createLayerWithZoomInfoTimed(identifier, tileLoaders, zoomInfo, 1);
    };

    std::shared_ptr<::Tiled2dMapLayerConfig> createLayerConfig(const std::string &identifier) override {
        return createLayerConfigWithZoomInfo(identifier, Tiled2dMapZoomInfo(2.25, 2, false, true, true, true));
    };

    std::shared_ptr<::Tiled2dMapLayerConfig> createLayerConfigWithZoomInfo(const std::string &identifier,
                                                                           const ::Tiled2dMapZoomInfo &zoomInfo) override {
        return createLayerConfigWithZoomInfoTimed(identifier, zoomInfo, 1);
    };

    std::vector<WmtsLayerDescription> getAllLayers() override {
        std::vector<WmtsLayerDescription> layerVector;
        for (auto const &[identifier, layer] : layers) {
            layerVector.push_back(layer);
        }
        return layerVector;
    }

  private:
    pugi::xml_document doc;

    std::unordered_map<std::string, WmtsLayerDescription> layers;
    std::unordered_map<std::string, WmtsTileMatrixSet> matrixSets;

    void parseLayer(pugi::xml_node &layer) {
        std::string identifier = layer.child("ows:Identifier").child_value();
        std::optional<std::string> title = layer.child_value("ows:Title");
        std::optional<std::string> abstractText = layer.child_value("ows:Abstract");
        std::vector<WmtsLayerDimension> dimensions;

        for (auto dimension = layer.child("Dimension"); dimension; dimension = dimension.next_sibling("Dimension")) {
            std::string dimensionIdentifier = dimension.child_value("ows:Identifier");
            std::string defaultValue = dimension.child_value("Default");
            std::vector<std::string> dimensionValues;
            for (auto value = dimension.child("Value"); value; value = value.next_sibling("Value")) {
                dimensionValues.push_back(value.child_value());
            }
            auto dim = WmtsLayerDimension(dimensionIdentifier, defaultValue, dimensionValues);
            dimensions.push_back(dim);
        }

        std::vector<std::string> styleDimensionValues;
        std::string styleDefaultValue = "";
        for (auto style = layer.child("Style"); style; style = style.next_sibling("Style")) {
            std::string styleIdentifier = style.child_value("ows:Identifier");
            styleDimensionValues.push_back(styleIdentifier);
            std::string isDefault = style.attribute("isDefault").as_string();
            if (isDefault == "true") {
                styleDefaultValue = styleIdentifier;
            }
        }
        auto styleDim = WmtsLayerDimension("Style", styleDefaultValue, styleDimensionValues);
        dimensions.push_back(styleDim);

        auto boundingBox = layer.child("ows:WGS84BoundingBox");
        double c1 = 0;
        double c2 = 0;
        std::string lowerCorner = boundingBox.child_value("ows:LowerCorner");
        if (lowerCorner.size() > 0) {
            auto delimiterIndex = lowerCorner.find(" ");
            if (delimiterIndex != std::string::npos) {
                c1 = std::stod(lowerCorner.substr(0, delimiterIndex));
                c2 = std::stod(lowerCorner.substr(delimiterIndex + 1));
            }
        }
        double c3 = 0;
        double c4 = 0;
        std::string upperCorner = boundingBox.child_value("ows:UpperCorner");
        if (upperCorner.size() > 0) {
            auto delimiterIndex = upperCorner.find(" ");
            if (delimiterIndex != std::string::npos) {
                c3 = std::stod(upperCorner.substr(0, delimiterIndex));
                c4 = std::stod(upperCorner.substr(delimiterIndex + 1));
            }
        }

        std::optional<RectCoord> bounds;
        if (c1 != 0 && c2 != 0 && c3 != 0 && c4 != 0) {
            bounds = RectCoord(Coord(CoordinateSystemIdentifiers::EPSG4326(), c1, c4, 0),
                                 Coord(CoordinateSystemIdentifiers::EPSG4326(), c3, c2, 0));
        }
        std::string tileMatrixSetLink = layer.child("TileMatrixSetLink").child_value("TileMatrixSet");
        std::string resourceTemplate = layer.child("ResourceURL").attribute("template").as_string();
        std::string resourceFormat = layer.child("ResourceURL").attribute("format").as_string();

        auto wmtsLayer = WmtsLayerDescription(identifier, title, abstractText, dimensions, bounds, tileMatrixSetLink,
                                              resourceTemplate, resourceFormat);
        layers.insert({identifier, wmtsLayer});
    }

    void parseMatrixSet(pugi::xml_node &tileMatrixSet) {
        std::string identifier = tileMatrixSet.child_value("ows:Identifier");

        std::string supportedCRS = tileMatrixSet.child_value("ows:SupportedCRS");
        auto coordinateSystem = CoordinateSystemIdentifiers::fromCrsIdentifier(supportedCRS);
        if (coordinateSystem == -1) {
            printf("unknown coordinate system %s\n", supportedCRS.c_str());
        }
        std::vector<WmtsTileMatrix> matrices;

        for (auto tileMatrix = tileMatrixSet.child("TileMatrix"); tileMatrix; tileMatrix = tileMatrix.next_sibling("TileMatrix")) {
            std::string matrixIdentifier = tileMatrix.child_value("ows:Identifier");
            double scaleDenominator = std::stod(tileMatrix.child_value("ScaleDenominator"));
            std::string topLeftCorner = tileMatrix.child_value("TopLeftCorner");
            double c1 = std::stod(topLeftCorner.substr(0, topLeftCorner.find(" ")));
            double c2 = std::stod(topLeftCorner.substr(topLeftCorner.find(" ")));
            int32_t tileWidth = std::stoi(tileMatrix.child_value("TileWidth"));
            int32_t tileHeight = std::stoi(tileMatrix.child_value("TileHeight"));
            int32_t matrixWidth = std::stoi(tileMatrix.child_value("MatrixWidth"));
            int32_t matrixHeight = std::stoi(tileMatrix.child_value("MatrixHeight"));

            auto matrix =
                WmtsTileMatrix(matrixIdentifier, scaleDenominator, c1, c2, tileWidth, tileHeight, matrixWidth, matrixHeight);
            matrices.push_back(matrix);
        }

        auto matrixSet = WmtsTileMatrixSet(identifier, coordinateSystem, matrices);
        matrixSets.insert(std::make_pair(identifier, matrixSet));
    }

    void parseDoc() {
        auto root = doc.child("Capabilities");
        auto contents = root.child("Contents");

        for (auto layer = contents.child("Layer"); layer; layer = layer.next_sibling("Layer")) {
            parseLayer(layer);
        }

        for (auto tileMatrixSet = contents.child("TileMatrixSet"); tileMatrixSet;
             tileMatrixSet = tileMatrixSet.next_sibling("TileMatrixSet")) {
            parseMatrixSet(tileMatrixSet);
        }
    }
};

std::shared_ptr<WmtsCapabilitiesResource> WmtsCapabilitiesResource::create(const std::string &xml) {
    return std::make_shared<WmtsCapabilitiesResourceImpl>(xml);
}
