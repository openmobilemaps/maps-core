// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from packer.djinni

#import "MCRectI.h"
#import <Foundation/Foundation.h>

NS_SWIFT_SENDABLE
@interface MCRectanglePackerPage : NSObject
- (nonnull instancetype)init NS_UNAVAILABLE;
+ (nonnull instancetype)new NS_UNAVAILABLE;
- (nonnull instancetype)initWithUvs:(nonnull NSDictionary<NSString *, MCRectI *> *)uvs NS_DESIGNATED_INITIALIZER;
+ (nonnull instancetype)rectanglePackerPageWithUvs:(nonnull NSDictionary<NSString *, MCRectI *> *)uvs;

@property (nonatomic, readonly, nonnull) NSDictionary<NSString *, MCRectI *> * uvs;

@end
