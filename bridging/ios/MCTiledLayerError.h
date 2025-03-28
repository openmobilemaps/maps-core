// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from map_helpers.djinni

#import "MCLoaderStatus.h"
#import "MCRectCoord.h"
#import <Foundation/Foundation.h>

NS_SWIFT_SENDABLE
@interface MCTiledLayerError : NSObject
- (nonnull instancetype)init NS_UNAVAILABLE;
+ (nonnull instancetype)new NS_UNAVAILABLE;
- (nonnull instancetype)initWithStatus:(MCLoaderStatus)status
                             errorCode:(nullable NSString *)errorCode
                             layerName:(nonnull NSString *)layerName
                                   url:(nonnull NSString *)url
                         isRecoverable:(BOOL)isRecoverable
                                bounds:(nullable MCRectCoord *)bounds NS_DESIGNATED_INITIALIZER;
+ (nonnull instancetype)tiledLayerErrorWithStatus:(MCLoaderStatus)status
                                        errorCode:(nullable NSString *)errorCode
                                        layerName:(nonnull NSString *)layerName
                                              url:(nonnull NSString *)url
                                    isRecoverable:(BOOL)isRecoverable
                                           bounds:(nullable MCRectCoord *)bounds;

@property (nonatomic, readonly) MCLoaderStatus status;

@property (nonatomic, readonly, nullable) NSString * errorCode;

@property (nonatomic, readonly, nonnull) NSString * layerName;

@property (nonatomic, readonly, nonnull) NSString * url;

@property (nonatomic, readonly) BOOL isRecoverable;

@property (nonatomic, readonly, nullable) MCRectCoord * bounds;

@end
