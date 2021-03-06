// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from icon.djinni

#import "MCCoord.h"
#import "MCIconType.h"
#import "MCTextureHolderInterface.h"
#import "MCVec2F.h"
#import <Foundation/Foundation.h>


@interface MCIconInfoInterface : NSObject

- (nonnull NSString *)getIdentifier;

- (nullable id<MCTextureHolderInterface>)getTexture;

- (void)setCoordinate:(nonnull MCCoord *)coord;

- (nonnull MCCoord *)getCoordinate;

- (void)setIconSize:(nonnull MCVec2F *)size;

- (nonnull MCVec2F *)getIconSize;

- (void)setType:(MCIconType)scaleType;

- (MCIconType)getType;

@end
