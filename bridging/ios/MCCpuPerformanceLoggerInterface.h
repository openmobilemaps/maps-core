// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from map_helpers.djinni

#import <Foundation/Foundation.h>
@class MCCpuPerformanceLoggerInterface;
@protocol MCPerformanceLoggerInterface;


@interface MCCpuPerformanceLoggerInterface : NSObject

+ (nullable MCCpuPerformanceLoggerInterface *)create;

+ (nullable MCCpuPerformanceLoggerInterface *)createSpecifically:(int32_t)numBuckets
                                                    bucketSizeMs:(int64_t)bucketSizeMs;

- (nullable id<MCPerformanceLoggerInterface>)asPerformanceLoggerInterface;

@end
