// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from network_activity_manager.djinni

#import "MCTasksProgressInfo.h"


@implementation MCTasksProgressInfo

- (nonnull instancetype)initWithLayerName:(nonnull NSString *)layerName
                                 maxCount:(int32_t)maxCount
                           remainingCount:(int32_t)remainingCount
                                 progress:(float)progress
{
    if (self = [super init]) {
        _layerName = [layerName copy];
        _maxCount = maxCount;
        _remainingCount = remainingCount;
        _progress = progress;
    }
    return self;
}

+ (nonnull instancetype)tasksProgressInfoWithLayerName:(nonnull NSString *)layerName
                                              maxCount:(int32_t)maxCount
                                        remainingCount:(int32_t)remainingCount
                                              progress:(float)progress
{
    return [(MCTasksProgressInfo*)[self alloc] initWithLayerName:layerName
                                                        maxCount:maxCount
                                                  remainingCount:remainingCount
                                                        progress:progress];
}

- (NSString *)description
{
    return [NSString stringWithFormat:@"<%@ %p layerName:%@ maxCount:%@ remainingCount:%@ progress:%@>", self.class, (void *)self, self.layerName, @(self.maxCount), @(self.remainingCount), @(self.progress)];
}

@end