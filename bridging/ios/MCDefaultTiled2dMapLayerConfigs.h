// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from tiled_layer.djinni

#import "MCTiled2dMapZoomInfo.h"
#import <Foundation/Foundation.h>
@protocol MCTiled2dMapLayerConfig;


@interface MCDefaultTiled2dMapLayerConfigs : NSObject

+ (nullable id<MCTiled2dMapLayerConfig>)webMercator:(nonnull NSString *)layerName
                                          urlFormat:(nonnull NSString *)urlFormat;

+ (nullable id<MCTiled2dMapLayerConfig>)webMercatorCustom:(nonnull NSString *)layerName
                                                urlFormat:(nonnull NSString *)urlFormat
                                                 zoomInfo:(nonnull MCTiled2dMapZoomInfo *)zoomInfo
                                             minZoomLevel:(int32_t)minZoomLevel
                                             maxZoomLevel:(int32_t)maxZoomLevel;

+ (nullable id<MCTiled2dMapLayerConfig>)epsg4326:(nonnull NSString *)layerName
                                       urlFormat:(nonnull NSString *)urlFormat;

+ (nullable id<MCTiled2dMapLayerConfig>)epsg4326Custom:(nonnull NSString *)layerName
                                             urlFormat:(nonnull NSString *)urlFormat
                                              zoomInfo:(nonnull MCTiled2dMapZoomInfo *)zoomInfo
                                          minZoomLevel:(int32_t)minZoomLevel
                                          maxZoomLevel:(int32_t)maxZoomLevel;

@end