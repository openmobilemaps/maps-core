// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from wmts_capabilities.djinni

#import <Foundation/Foundation.h>

NS_SWIFT_SENDABLE
@interface MCWmtsTileMatrix : NSObject
- (nonnull instancetype)init NS_UNAVAILABLE;
+ (nonnull instancetype)new NS_UNAVAILABLE;
- (nonnull instancetype)initWithIdentifier:(nonnull NSString *)identifier
                          scaleDenominator:(double)scaleDenominator
                            topLeftCornerX:(double)topLeftCornerX
                            topLeftCornerY:(double)topLeftCornerY
                                 tileWidth:(int32_t)tileWidth
                                tileHeight:(int32_t)tileHeight
                               matrixWidth:(int32_t)matrixWidth
                              matrixHeight:(int32_t)matrixHeight NS_DESIGNATED_INITIALIZER;
+ (nonnull instancetype)wmtsTileMatrixWithIdentifier:(nonnull NSString *)identifier
                                    scaleDenominator:(double)scaleDenominator
                                      topLeftCornerX:(double)topLeftCornerX
                                      topLeftCornerY:(double)topLeftCornerY
                                           tileWidth:(int32_t)tileWidth
                                          tileHeight:(int32_t)tileHeight
                                         matrixWidth:(int32_t)matrixWidth
                                        matrixHeight:(int32_t)matrixHeight;

@property (nonatomic, readonly, nonnull) NSString * identifier;

@property (nonatomic, readonly) double scaleDenominator;

@property (nonatomic, readonly) double topLeftCornerX;

@property (nonatomic, readonly) double topLeftCornerY;

@property (nonatomic, readonly) int32_t tileWidth;

@property (nonatomic, readonly) int32_t tileHeight;

@property (nonatomic, readonly) int32_t matrixWidth;

@property (nonatomic, readonly) int32_t matrixHeight;

@end
