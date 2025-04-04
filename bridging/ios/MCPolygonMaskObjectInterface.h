// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from polygon.djinni

#import "MCCoordinateConversionHelperInterface.h"
#import "MCGraphicsObjectFactoryInterface.h"
#import "MCPolygon2dInterface.h"
#import "MCPolygonCoord.h"
#import "MCVec3D.h"
#import <Foundation/Foundation.h>
@class MCPolygonMaskObjectInterface;


@interface MCPolygonMaskObjectInterface : NSObject

+ (nullable MCPolygonMaskObjectInterface *)create:(nullable id<MCGraphicsObjectFactoryInterface>)graphicsObjectFactory
                                 conversionHelper:(nullable MCCoordinateConversionHelperInterface *)conversionHelper
                                             is3d:(BOOL)is3d;

- (void)setPolygons:(nonnull NSArray<MCPolygonCoord *> *)polygons
             origin:(nonnull MCVec3D *)origin;

- (void)setPolygon:(nonnull MCPolygonCoord *)polygon
            origin:(nonnull MCVec3D *)origin;

- (nullable id<MCPolygon2dInterface>)getPolygonObject;

@end
