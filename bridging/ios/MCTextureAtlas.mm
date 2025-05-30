// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from packer.djinni

#import "MCTextureAtlas.h"


@implementation MCTextureAtlas

- (nonnull instancetype)initWithUvMap:(nonnull NSDictionary<NSString *, MCRectI *> *)uvMap
                              texture:(nullable id<MCTextureHolderInterface>)texture
{
    if (self = [super init]) {
        _uvMap = [uvMap copy];
        _texture = texture;
    }
    return self;
}

+ (nonnull instancetype)textureAtlasWithUvMap:(nonnull NSDictionary<NSString *, MCRectI *> *)uvMap
                                      texture:(nullable id<MCTextureHolderInterface>)texture
{
    return [[self alloc] initWithUvMap:uvMap
                               texture:texture];
}

#ifndef DJINNI_DISABLE_DESCRIPTION_METHODS
- (NSString *)description
{
    return [NSString stringWithFormat:@"<%@ %p uvMap:%@ texture:%@>", self.class, (void *)self, self.uvMap, self.texture];
}

#endif
@end
