#import "MCMapCoreObjCFactory.h"

@import MapCore;
@import MapCoreSharedModule;

@implementation MCMapCoreObjCFactory

+ (id<MCLoaderInterface>)createTextureLoader {
    return [[MCTextureLoader alloc] initWithUrlSession:nil];
}

+ (id<MCFontLoaderInterface>)createFontLoaderWithBundle:(NSBundle *)bundle {
    return [[MCFontLoader alloc] initWithBundle:bundle preload:@[]];
}

+ (id<MCTextureHolderInterface>)createTextureHolderWithData:(NSData *)data {
    return [[TextureHolder alloc] initWithData:data];
}

@end
