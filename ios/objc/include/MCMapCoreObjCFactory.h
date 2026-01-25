#import <Foundation/Foundation.h>
@import MapCoreSharedModule;

NS_ASSUME_NONNULL_BEGIN

@interface MCMapCoreObjCFactory : NSObject

+ (id<MCLoaderInterface>)createTextureLoader;
+ (id<MCFontLoaderInterface>)createFontLoaderWithBundle:(NSBundle *)bundle;
+ (id<MCTextureHolderInterface>)createTextureHolderWithData:(NSData *)data;

@end

NS_ASSUME_NONNULL_END
