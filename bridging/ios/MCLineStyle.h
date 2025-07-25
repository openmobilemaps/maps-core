// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from line.djinni

#import "MCColorStateList.h"
#import "MCLineCapType.h"
#import "MCLineJoinType.h"
#import "MCSizeType.h"
#import <Foundation/Foundation.h>

NS_SWIFT_SENDABLE
@interface MCLineStyle : NSObject
- (nonnull instancetype)init NS_UNAVAILABLE;
+ (nonnull instancetype)new NS_UNAVAILABLE;
- (nonnull instancetype)initWithColor:(nonnull MCColorStateList *)color
                             gapColor:(nonnull MCColorStateList *)gapColor
                              opacity:(float)opacity
                                 blur:(float)blur
                            widthType:(MCSizeType)widthType
                                width:(float)width
                            dashArray:(nonnull NSArray<NSNumber *> *)dashArray
                             dashFade:(float)dashFade
                   dashAnimationSpeed:(float)dashAnimationSpeed
                              lineCap:(MCLineCapType)lineCap
                             lineJoin:(MCLineJoinType)lineJoin
                               offset:(float)offset
                               dotted:(BOOL)dotted
                           dottedSkew:(float)dottedSkew NS_DESIGNATED_INITIALIZER;
+ (nonnull instancetype)lineStyleWithColor:(nonnull MCColorStateList *)color
                                  gapColor:(nonnull MCColorStateList *)gapColor
                                   opacity:(float)opacity
                                      blur:(float)blur
                                 widthType:(MCSizeType)widthType
                                     width:(float)width
                                 dashArray:(nonnull NSArray<NSNumber *> *)dashArray
                                  dashFade:(float)dashFade
                        dashAnimationSpeed:(float)dashAnimationSpeed
                                   lineCap:(MCLineCapType)lineCap
                                  lineJoin:(MCLineJoinType)lineJoin
                                    offset:(float)offset
                                    dotted:(BOOL)dotted
                                dottedSkew:(float)dottedSkew;

@property (nonatomic, readonly, nonnull) MCColorStateList * color;

@property (nonatomic, readonly, nonnull) MCColorStateList * gapColor;

@property (nonatomic, readonly) float opacity;

@property (nonatomic, readonly) float blur;

@property (nonatomic, readonly) MCSizeType widthType;

@property (nonatomic, readonly) float width;

@property (nonatomic, readonly, nonnull) NSArray<NSNumber *> * dashArray;

@property (nonatomic, readonly) float dashFade;

@property (nonatomic, readonly) float dashAnimationSpeed;

@property (nonatomic, readonly) MCLineCapType lineCap;

@property (nonatomic, readonly) MCLineJoinType lineJoin;

@property (nonatomic, readonly) float offset;

@property (nonatomic, readonly) BOOL dotted;

@property (nonatomic, readonly) float dottedSkew;

@end
