// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from error_manager.djinni

#import "MCLoaderStatus.h"
#import "MCRectCoord.h"
#import <Foundation/Foundation.h>

@interface MCTiledLayerError : NSObject
- (nonnull instancetype)initWithStatus:(MCLoaderStatus)status
                             errorCode:(nullable NSString *)errorCode
                             layerName:(nonnull NSString *)layerName
                                   url:(nonnull NSString *)url
                         isRecoverable:(BOOL)isRecoverable
                                bounds:(nullable MCRectCoord *)bounds;
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