// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from network_activity_manager.djinni

#import "MCTiledLayerError.h"
#import <Foundation/Foundation.h>
@class MCNetworkActivityManagerInterface;
@protocol MCNetworkActivityListenerInterface;


@interface MCNetworkActivityManagerInterface : NSObject

+ (nullable MCNetworkActivityManagerInterface *)create;

- (void)addTiledLayerError:(nonnull MCTiledLayerError *)error;

- (void)removeError:(nonnull NSString *)url;

- (void)removeAllErrorsForLayer:(nonnull NSString *)layerName;

- (void)clearAllErrors;

- (void)addNetworkActivityListener:(nullable id<MCNetworkActivityListenerInterface>)listener;

- (void)removeNetworkActivityListener:(nullable id<MCNetworkActivityListenerInterface>)listener;

- (void)updateRemainingTasks:(nonnull NSString *)layerName
                   taskCount:(int32_t)taskCount;

@end