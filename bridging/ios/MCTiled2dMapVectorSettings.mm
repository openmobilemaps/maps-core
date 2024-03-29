// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from tiled_layer.djinni

#import "MCTiled2dMapVectorSettings.h"


@implementation MCTiled2dMapVectorSettings

- (nonnull instancetype)initWithTileOrigin:(MCTiled2dMapVectorTileOrigin)tileOrigin
{
    if (self = [super init]) {
        _tileOrigin = tileOrigin;
    }
    return self;
}

+ (nonnull instancetype)tiled2dMapVectorSettingsWithTileOrigin:(MCTiled2dMapVectorTileOrigin)tileOrigin
{
    return [[self alloc] initWithTileOrigin:tileOrigin];
}

#ifndef DJINNI_DISABLE_DESCRIPTION_METHODS
- (NSString *)description
{
    return [NSString stringWithFormat:@"<%@ %p tileOrigin:%@>", self.class, (void *)self, @(self.tileOrigin)];
}

#endif
@end
