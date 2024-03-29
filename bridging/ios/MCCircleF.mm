// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from common.djinni

#import "MCCircleF.h"


@implementation MCCircleF

- (nonnull instancetype)initWithX:(float)x
                                y:(float)y
                           radius:(float)radius
{
    if (self = [super init]) {
        _x = x;
        _y = y;
        _radius = radius;
    }
    return self;
}

+ (nonnull instancetype)circleFWithX:(float)x
                                   y:(float)y
                              radius:(float)radius
{
    return [[self alloc] initWithX:x
                                 y:y
                            radius:radius];
}

#ifndef DJINNI_DISABLE_DESCRIPTION_METHODS
- (NSString *)description
{
    return [NSString stringWithFormat:@"<%@ %p x:%@ y:%@ radius:%@>", self.class, (void *)self, @(self.x), @(self.y), @(self.radius)];
}

#endif
@end
