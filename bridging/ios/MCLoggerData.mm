// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from map_helpers.djinni

#import "MCLoggerData.h"


@implementation MCLoggerData

- (nonnull instancetype)initWithId:(nonnull NSString *)id
                           buckets:(nonnull NSArray<NSNumber *> *)buckets
                      bucketSizeMs:(int32_t)bucketSizeMs
                             start:(int64_t)start
                               end:(int64_t)end
                        numSamples:(int64_t)numSamples
                           average:(double)average
                          variance:(double)variance
                      stdDeviation:(double)stdDeviation
{
    if (self = [super init]) {
        _id = [id copy];
        _buckets = [buckets copy];
        _bucketSizeMs = bucketSizeMs;
        _start = start;
        _end = end;
        _numSamples = numSamples;
        _average = average;
        _variance = variance;
        _stdDeviation = stdDeviation;
    }
    return self;
}

+ (nonnull instancetype)loggerDataWithId:(nonnull NSString *)id
                                 buckets:(nonnull NSArray<NSNumber *> *)buckets
                            bucketSizeMs:(int32_t)bucketSizeMs
                                   start:(int64_t)start
                                     end:(int64_t)end
                              numSamples:(int64_t)numSamples
                                 average:(double)average
                                variance:(double)variance
                            stdDeviation:(double)stdDeviation
{
    return [[self alloc] initWithId:id
                            buckets:buckets
                       bucketSizeMs:bucketSizeMs
                              start:start
                                end:end
                         numSamples:numSamples
                            average:average
                           variance:variance
                       stdDeviation:stdDeviation];
}

#ifndef DJINNI_DISABLE_DESCRIPTION_METHODS
- (NSString *)description
{
    return [NSString stringWithFormat:@"<%@ %p id:%@ buckets:%@ bucketSizeMs:%@ start:%@ end:%@ numSamples:%@ average:%@ variance:%@ stdDeviation:%@>", self.class, (void *)self, self.id, self.buckets, @(self.bucketSizeMs), @(self.start), @(self.end), @(self.numSamples), @(self.average), @(self.variance), @(self.stdDeviation)];
}

#endif
@end
