// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from packer.djinni

#import "MCRectI.h"
#import "MCTextureHolderInterface.h"
#import <Foundation/Foundation.h>

NS_SWIFT_SENDABLE
@interface MCTextureAtlas : NSObject
- (nonnull instancetype)init NS_UNAVAILABLE;
+ (nonnull instancetype)new NS_UNAVAILABLE;
- (nonnull instancetype)initWithUvMap:(nonnull NSDictionary<NSString *, MCRectI *> *)uvMap
                              texture:(nullable id<MCTextureHolderInterface>)texture NS_DESIGNATED_INITIALIZER;
+ (nonnull instancetype)textureAtlasWithUvMap:(nonnull NSDictionary<NSString *, MCRectI *> *)uvMap
                                      texture:(nullable id<MCTextureHolderInterface>)texture;

@property (nonatomic, readonly, nonnull) NSDictionary<NSString *, MCRectI *> * uvMap;

@property (nonatomic, readonly, nullable) id<MCTextureHolderInterface> texture;

@end
