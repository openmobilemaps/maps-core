// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from wmts_capabilities.djinni

#import "MCWmtsTileMatrixSet.h"


@implementation MCWmtsTileMatrixSet

- (nonnull instancetype)initWithIdentifier:(nonnull NSString *)identifier
                coordinateSystemIdentifier:(int32_t)coordinateSystemIdentifier
                                  matrices:(nonnull NSArray<MCWmtsTileMatrix *> *)matrices
{
    if (self = [super init]) {
        _identifier = [identifier copy];
        _coordinateSystemIdentifier = coordinateSystemIdentifier;
        _matrices = [matrices copy];
    }
    return self;
}

+ (nonnull instancetype)wmtsTileMatrixSetWithIdentifier:(nonnull NSString *)identifier
                             coordinateSystemIdentifier:(int32_t)coordinateSystemIdentifier
                                               matrices:(nonnull NSArray<MCWmtsTileMatrix *> *)matrices
{
    return [[self alloc] initWithIdentifier:identifier
                 coordinateSystemIdentifier:coordinateSystemIdentifier
                                   matrices:matrices];
}

#ifndef DJINNI_DISABLE_DESCRIPTION_METHODS
- (NSString *)description
{
    return [NSString stringWithFormat:@"<%@ %p identifier:%@ coordinateSystemIdentifier:%@ matrices:%@>", self.class, (void *)self, self.identifier, @(self.coordinateSystemIdentifier), self.matrices];
}

#endif
@end
