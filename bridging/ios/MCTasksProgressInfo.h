// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from network_activity_manager.djinni

#import <Foundation/Foundation.h>

@interface MCTasksProgressInfo : NSObject
- (nonnull instancetype)initWithLayerName:(nonnull NSString *)layerName
                                 maxCount:(int32_t)maxCount
                           remainingCount:(int32_t)remainingCount
                                 progress:(float)progress;
+ (nonnull instancetype)tasksProgressInfoWithLayerName:(nonnull NSString *)layerName
                                              maxCount:(int32_t)maxCount
                                        remainingCount:(int32_t)remainingCount
                                              progress:(float)progress;

@property (nonatomic, readonly, nonnull) NSString * layerName;

@property (nonatomic, readonly) int32_t maxCount;

@property (nonatomic, readonly) int32_t remainingCount;

@property (nonatomic, readonly) float progress;

@end