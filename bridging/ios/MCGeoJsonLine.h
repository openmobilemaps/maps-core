// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from geo_json_parser.djinni

#import "MCCoord.h"
#import "MCVectorLayerFeatureInfo.h"
#import <Foundation/Foundation.h>

NS_SWIFT_SENDABLE
@interface MCGeoJsonLine : NSObject
- (nonnull instancetype)init NS_UNAVAILABLE;
+ (nonnull instancetype)new NS_UNAVAILABLE;
- (nonnull instancetype)initWithPoints:(nonnull NSArray<MCCoord *> *)points
                           featureInfo:(nonnull MCVectorLayerFeatureInfo *)featureInfo NS_DESIGNATED_INITIALIZER;
+ (nonnull instancetype)geoJsonLineWithPoints:(nonnull NSArray<MCCoord *> *)points
                                  featureInfo:(nonnull MCVectorLayerFeatureInfo *)featureInfo;

@property (nonatomic, readonly, nonnull) NSArray<MCCoord *> * points;

@property (nonatomic, readonly, nonnull) MCVectorLayerFeatureInfo * featureInfo;

@end