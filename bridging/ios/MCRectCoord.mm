// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from coordinate_system.djinni

#import "MCRectCoord.h"


@implementation MCRectCoord

- (nonnull instancetype)initWithTopLeft:(nonnull MCCoord *)topLeft
                            bottomRight:(nonnull MCCoord *)bottomRight
{
    if (self = [super init]) {
        _topLeft = topLeft;
        _bottomRight = bottomRight;
    }
    return self;
}

+ (nonnull instancetype)rectCoordWithTopLeft:(nonnull MCCoord *)topLeft
                                 bottomRight:(nonnull MCCoord *)bottomRight
{
    return [[self alloc] initWithTopLeft:topLeft
                             bottomRight:bottomRight];
}

- (BOOL)isEqual:(id)other
{
    if (![other isKindOfClass:[MCRectCoord class]]) {
        return NO;
    }
    MCRectCoord *typedOther = (MCRectCoord *)other;
    return [self.topLeft isEqual:typedOther.topLeft] &&
            [self.bottomRight isEqual:typedOther.bottomRight];
}

- (NSUInteger)hash
{
    return NSStringFromClass([self class]).hash ^
            self.topLeft.hash ^
            self.bottomRight.hash;
}

- (NSComparisonResult)compare:(MCRectCoord *)other
{
    NSComparisonResult tempResult;
    tempResult = [self.topLeft compare:other.topLeft];
    if (tempResult != NSOrderedSame) {
        return tempResult;
    }
    tempResult = [self.bottomRight compare:other.bottomRight];
    if (tempResult != NSOrderedSame) {
        return tempResult;
    }
    return NSOrderedSame;
}

#ifndef DJINNI_DISABLE_DESCRIPTION_METHODS
- (NSString *)description
{
    return [NSString stringWithFormat:@"<%@ %p topLeft:%@ bottomRight:%@>", self.class, (void *)self, self.topLeft, self.bottomRight];
}

#endif
@end
