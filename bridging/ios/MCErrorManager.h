// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from map_helpers.djinni

#import "MCTiledLayerError.h"
#import <Foundation/Foundation.h>
@class MCErrorManager;
@protocol MCErrorManagerListener;


@interface MCErrorManager : NSObject

+ (nullable MCErrorManager *)create;

- (void)addTiledLayerError:(nonnull MCTiledLayerError *)error;

- (void)removeError:(nonnull NSString *)url;

- (void)removeAllErrorsForLayer:(nonnull NSString *)layerName;

- (void)clearAllErrors;

- (void)addErrorListener:(nullable id<MCErrorManagerListener>)listener;

- (void)removeErrorListener:(nullable id<MCErrorManagerListener>)listener;

@end
