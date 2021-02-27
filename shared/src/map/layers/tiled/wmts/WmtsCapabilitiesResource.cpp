//
//  File.cpp
//  
//
//  Created by Nicolas Märki on 26.02.21.
//

#include "WmtsCapabilitiesResource.h"
#include "Tiled2dMapRasterLayerInterface.h"
#include "WmtsTiled2dMapLayerConfigFactory.h"
#include "WmtsLayerDescription.h"
#include "pugixml.hpp"
#include "WmtsLayerDimension.h"
#include "CoordinateSystemIdentifiers.h"
#include "WmtsLayerConfiguration.h"
#include "WmtsTileMatrixSet.h"

class WmtsCapabilitiesResourceImpl: public WmtsCapabilitiesResource {

public:

    WmtsCapabilitiesResourceImpl(const std::string & xml) {
        doc.load_string(xml.c_str());
        parseDoc();
    };

    std::shared_ptr<::Tiled2dMapRasterLayerInterface> createLayer(const std::string & identifier, const std::shared_ptr<::TextureLoaderInterface> & textureLoader, const ::MapCoordinateSystem & mapCoordinateSystem) {

        for (auto &description : layers) {

            if (description.identifier != identifier) {
                continue; // TODO: Codestyle - Map or find instead of loop
            }

            auto bounds = mapCoordinateSystem.bounds;

            auto matrixSet = matrixSets.at(description.tileMatrixSetLink);

            std::unordered_map<std::string, std::string> dimensions;
            for (auto &dimension : description.dimensions) {
                dimensions.insert(std::make_pair(dimension.identifier, dimension.defaultValue));
            }
            dimensions.insert(std::make_pair("TileMatrixSet", description.tileMatrixSetLink));

            WmtsLayerConfiguration configuration = WmtsLayerConfiguration(identifier,
                                                                          description.resourceTemplate,
                                                                          bounds, dimensions);
            Tiled2dMapZoomInfo zoomInfo = Tiled2dMapZoomInfo(1.0, 0);

            std::vector<::Tiled2dMapZoomLevelInfo>  zoomLevels;

            for (auto & matrix : matrixSet.matrices) {
                double widthLayerSystemUnits = (bounds.bottomRight.x - bounds.topLeft.x) / matrix.matrixWidth;
                int32_t zoomLevelIdentifier = stoi(matrix.identifier);
                auto levelInfo = Tiled2dMapZoomLevelInfo(matrix.scaleDenominator, widthLayerSystemUnits, matrix.matrixWidth, matrix.matrixHeight, zoomLevelIdentifier);
                zoomLevels.push_back(levelInfo);
            }


            auto layerConfig = WmtsTiled2dMapLayerConfigFactory::create(configuration, zoomLevels, zoomInfo);
            //
            return Tiled2dMapRasterLayerInterface::create(layerConfig, textureLoader);

        }

        return nullptr;



    };

    std::vector<WmtsLayerDescription> getAllLayers() {
        return layers;
    }

private:

    pugi::xml_document doc;

    std::vector<WmtsLayerDescription> layers;
    std::unordered_map<std::string, WmtsTileMatrixSet> matrixSets;

    void parseDoc() {
        auto root = doc.child("Capabilities");
        auto contents = root.child("Contents");

        for (auto & layer : contents.children("Layer")) {
            std::string identifier = layer.child("ows:Identifier").child_value();
            std::optional<std::string> title = layer.child_value("ows:Title");
            std::optional<std::string> abstractText = layer.child_value("ows:Abstract");
            std::vector<WmtsLayerDimension> dimensions;

            for(auto & dimension : layer.children("Dimension")) {
                std::string dimensionIdentifier = dimension.child_value("ows:Identifier");
                std::string defaultValue = dimension.child_value("Default");
                std::vector<std::string> dimensionValues;
                for (auto & value : dimension.children("Value")) {
                    dimensionValues.push_back(value.child_value());
                }
                auto dim = WmtsLayerDimension(dimensionIdentifier, defaultValue, dimensionValues);
                dimensions.push_back(dim);
            }

            auto boundingBox = layer.child("ows:WGS84BoundingBox");
            double c1 = 0;
            double c2 = 0;
            std::string lowerCorner = boundingBox.child_value("ows:LowerCorner");
            if (lowerCorner.size() > 0) {
                c1 = std::stod(lowerCorner.substr(0, lowerCorner.find(" ")));
                c2 = std::stod(lowerCorner.substr(lowerCorner.find(" ")));
            }
            double c3 = 0;
            double c4 = 0;
            std::string upperCorner = boundingBox.child_value("ows:UpperCorner");
            if (upperCorner.size() > 0) {
                c3 = std::stod(upperCorner.substr(0, lowerCorner.find(" ")));
                c4 = std::stod(upperCorner.substr(lowerCorner.find(" ")));
            }


            auto bounds = RectCoord(Coord(CoordinateSystemIdentifiers::EPSG3857(), c1, c2, 0),
                                    Coord(CoordinateSystemIdentifiers::EPSG3857(), c3, c4, 0));
            std::string tileMatrixSetLink = layer.child("TileMatrixSetLink").child_value("TileMatrixSet");
            std::string resourceTemplate = layer.child("ResourceURL").attribute("template").as_string();
            std::string resourceFormat = layer.child("ResourceURL").attribute("format").as_string();

            auto wmtsLayer = WmtsLayerDescription(identifier, title, abstractText, dimensions, bounds, tileMatrixSetLink, resourceTemplate, resourceFormat);
            layers.push_back(wmtsLayer);
        }

        for (auto & tileMatrixSet : contents.children("TileMatrixSet")) {
            std::string identifier = tileMatrixSet.child_value("ows:Identifier");

            std::vector<WmtsTileMatrix> matrices;

            for (auto & tileMatrix : tileMatrixSet.children("TileMatrix")) {
                std::string matrixIdentifier = tileMatrix.child_value("ows:Identifier");
                double scaleDenominator = std::stod(tileMatrix.child_value("ScaleDenominator"));
                std::string topLeftCorner = tileMatrix.child_value("TopLeftCorner");
                double c1 = std::stod(topLeftCorner.substr(0, topLeftCorner.find(" ")));
                double c2 = std::stod(topLeftCorner.substr(topLeftCorner.find(" ")));
                int32_t tileWidth = std::stoi(tileMatrix.child_value("TileWidth"));
                int32_t tileHeight = std::stoi(tileMatrix.child_value("TileHeight"));
                int32_t matrixWidth = std::stoi(tileMatrix.child_value("MatrixWidth"));
                int32_t matrixHeight = std::stoi(tileMatrix.child_value("MatrixHeight"));

                auto matrix = WmtsTileMatrix(matrixIdentifier, scaleDenominator, c1, c2, tileWidth, tileHeight, matrixWidth, matrixHeight);
                matrices.push_back(matrix);
            }

            auto matrixSet = WmtsTileMatrixSet(identifier, matrices);
            matrixSets.insert(std::make_pair(identifier, matrixSet));
        }
    }

};


std::shared_ptr<WmtsCapabilitiesResource> WmtsCapabilitiesResource::create(const std::string & xml) {
    return std::make_shared<WmtsCapabilitiesResourceImpl>(xml);
}

