// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from tiled_vector_layer.djinni

#import "MCVectorLayerFeatureInfoValue.h"
#import <Foundation/Foundation.h>

@interface MCVectorLayerFeatureInfo : NSObject
- (nonnull instancetype)init NS_UNAVAILABLE;
+ (nonnull instancetype)new NS_UNAVAILABLE;
- (nonnull instancetype)initWithIdentifier:(nonnull NSString *)identifier
                                properties:(nonnull NSDictionary<NSString *, MCVectorLayerFeatureInfoValue *> *)properties NS_DESIGNATED_INITIALIZER;
+ (nonnull instancetype)vectorLayerFeatureInfoWithIdentifier:(nonnull NSString *)identifier
                                                  properties:(nonnull NSDictionary<NSString *, MCVectorLayerFeatureInfoValue *> *)properties;

@property (nonatomic, readonly, nonnull) NSString * identifier;

@property (nonatomic, readonly, nonnull) NSDictionary<NSString *, MCVectorLayerFeatureInfoValue *> * properties;

@end