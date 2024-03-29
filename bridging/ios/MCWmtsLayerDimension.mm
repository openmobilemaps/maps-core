// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from wmts_capabilities.djinni

#import "MCWmtsLayerDimension.h"


@implementation MCWmtsLayerDimension

- (nonnull instancetype)initWithIdentifier:(nonnull NSString *)identifier
                              defaultValue:(nonnull NSString *)defaultValue
                                    values:(nonnull NSArray<NSString *> *)values
{
    if (self = [super init]) {
        _identifier = [identifier copy];
        _defaultValue = [defaultValue copy];
        _values = [values copy];
    }
    return self;
}

+ (nonnull instancetype)wmtsLayerDimensionWithIdentifier:(nonnull NSString *)identifier
                                            defaultValue:(nonnull NSString *)defaultValue
                                                  values:(nonnull NSArray<NSString *> *)values
{
    return [[self alloc] initWithIdentifier:identifier
                               defaultValue:defaultValue
                                     values:values];
}

#ifndef DJINNI_DISABLE_DESCRIPTION_METHODS
- (NSString *)description
{
    return [NSString stringWithFormat:@"<%@ %p identifier:%@ defaultValue:%@ values:%@>", self.class, (void *)self, self.identifier, self.defaultValue, self.values];
}

#endif
@end
