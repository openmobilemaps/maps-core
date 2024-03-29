// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from common.djinni

#import "MCSharedBytes.h"


@implementation MCSharedBytes

- (nonnull instancetype)initWithAddress:(int64_t)address
                           elementCount:(int32_t)elementCount
                        bytesPerElement:(int32_t)bytesPerElement
{
    if (self = [super init]) {
        _address = address;
        _elementCount = elementCount;
        _bytesPerElement = bytesPerElement;
    }
    return self;
}

+ (nonnull instancetype)sharedBytesWithAddress:(int64_t)address
                                  elementCount:(int32_t)elementCount
                               bytesPerElement:(int32_t)bytesPerElement
{
    return [[self alloc] initWithAddress:address
                            elementCount:elementCount
                         bytesPerElement:bytesPerElement];
}

#ifndef DJINNI_DISABLE_DESCRIPTION_METHODS
- (NSString *)description
{
    return [NSString stringWithFormat:@"<%@ %p address:%@ elementCount:%@ bytesPerElement:%@>", self.class, (void *)self, @(self.address), @(self.elementCount), @(self.bytesPerElement)];
}

#endif
@end
