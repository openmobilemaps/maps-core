// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from loader.djinni

#import "DJFuture.h"
#import "MCDataLoaderResult.h"
#import "MCTextureLoaderResult.h"
#import <Foundation/Foundation.h>


@protocol MCLoaderInterface

- (nonnull MCTextureLoaderResult *)loadTexture:(nonnull NSString *)url
                                          etag:(nullable NSString *)etag;

- (nonnull MCDataLoaderResult *)loadData:(nonnull NSString *)url
                                    etag:(nullable NSString *)etag;

- (nonnull DJFuture<MCTextureLoaderResult *> *)loadTextureAsync:(nonnull NSString *)url
                                                           etag:(nullable NSString *)etag;

- (nonnull DJFuture<MCDataLoaderResult *> *)loadDataAsync:(nonnull NSString *)url
                                                     etag:(nullable NSString *)etag;

- (void)cancel:(nonnull NSString *)url;

@end
