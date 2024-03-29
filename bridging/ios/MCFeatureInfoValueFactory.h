// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from tiled_vector_layer.djinni

#import "MCColor.h"
#import "MCVectorLayerFeatureInfoValue.h"
#import <Foundation/Foundation.h>


@interface MCFeatureInfoValueFactory : NSObject

+ (nonnull MCVectorLayerFeatureInfoValue *)createString:(nonnull NSString *)value;

+ (nonnull MCVectorLayerFeatureInfoValue *)createDouble:(double)value;

+ (nonnull MCVectorLayerFeatureInfoValue *)createInt:(int64_t)value;

+ (nonnull MCVectorLayerFeatureInfoValue *)createBool:(BOOL)value;

+ (nonnull MCVectorLayerFeatureInfoValue *)createColor:(nonnull MCColor *)value;

+ (nonnull MCVectorLayerFeatureInfoValue *)createListFloat:(nonnull NSArray<NSNumber *> *)value;

+ (nonnull MCVectorLayerFeatureInfoValue *)createListString:(nonnull NSArray<NSString *> *)value;

@end
