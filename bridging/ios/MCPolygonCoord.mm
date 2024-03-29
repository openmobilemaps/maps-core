// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from coordinate_system.djinni

#import "MCPolygonCoord.h"


@implementation MCPolygonCoord

- (nonnull instancetype)initWithPositions:(nonnull NSArray<MCCoord *> *)positions
                                    holes:(nonnull NSArray<NSArray<MCCoord *> *> *)holes
{
    if (self = [super init]) {
        _positions = [positions copy];
        _holes = [holes copy];
    }
    return self;
}

+ (nonnull instancetype)polygonCoordWithPositions:(nonnull NSArray<MCCoord *> *)positions
                                            holes:(nonnull NSArray<NSArray<MCCoord *> *> *)holes
{
    return [[self alloc] initWithPositions:positions
                                     holes:holes];
}

#ifndef DJINNI_DISABLE_DESCRIPTION_METHODS
- (NSString *)description
{
    return [NSString stringWithFormat:@"<%@ %p positions:%@ holes:%@>", self.class, (void *)self, self.positions, self.holes];
}

#endif
@end
