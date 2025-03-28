// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from tiled_vector_layer.djinni

#import "MCCoord.h"
#import "MCVectorLayerFeatureInfo.h"
#import <Foundation/Foundation.h>

NS_SWIFT_SENDABLE
@interface MCVectorLayerFeatureCoordInfo : NSObject
- (nonnull instancetype)init NS_UNAVAILABLE;
+ (nonnull instancetype)new NS_UNAVAILABLE;
- (nonnull instancetype)initWithFeatureInfo:(nonnull MCVectorLayerFeatureInfo *)featureInfo
                                coordinates:(nonnull MCCoord *)coordinates NS_DESIGNATED_INITIALIZER;
+ (nonnull instancetype)vectorLayerFeatureCoordInfoWithFeatureInfo:(nonnull MCVectorLayerFeatureInfo *)featureInfo
                                                       coordinates:(nonnull MCCoord *)coordinates;

@property (nonatomic, readonly, nonnull) MCVectorLayerFeatureInfo * featureInfo;

@property (nonatomic, readonly, nonnull) MCCoord * coordinates;

@end
