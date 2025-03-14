// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from shader.djinni

#import <Foundation/Foundation.h>
@protocol MCAlphaInstancedShaderInterface;
@protocol MCAlphaShaderInterface;
@protocol MCColorCircleShaderInterface;
@protocol MCColorShaderInterface;
@protocol MCLineGroupShaderInterface;
@protocol MCPolygonGroupShaderInterface;
@protocol MCPolygonPatternGroupShaderInterface;
@protocol MCRasterShaderInterface;
@protocol MCSkySphereShaderInterface;
@protocol MCSphereEffectShaderInterface;
@protocol MCStretchInstancedShaderInterface;
@protocol MCStretchShaderInterface;
@protocol MCTextInstancedShaderInterface;
@protocol MCTextShaderInterface;


@protocol MCShaderFactoryInterface

- (nullable id<MCAlphaShaderInterface>)createAlphaShader;

- (nullable id<MCAlphaShaderInterface>)createUnitSphereAlphaShader;

- (nullable id<MCAlphaInstancedShaderInterface>)createAlphaInstancedShader;

- (nullable id<MCAlphaInstancedShaderInterface>)createUnitSphereAlphaInstancedShader;

- (nullable id<MCLineGroupShaderInterface>)createLineGroupShader;

- (nullable id<MCLineGroupShaderInterface>)createUnitSphereLineGroupShader;

- (nullable id<MCLineGroupShaderInterface>)createSimpleLineGroupShader;

- (nullable id<MCLineGroupShaderInterface>)createUnitSphereSimpleLineGroupShader;

- (nullable id<MCColorShaderInterface>)createUnitSphereColorShader;

- (nullable id<MCColorShaderInterface>)createColorShader;

- (nullable id<MCColorCircleShaderInterface>)createColorCircleShader;

- (nullable id<MCColorCircleShaderInterface>)createUnitSphereColorCircleShader;

- (nullable id<MCPolygonGroupShaderInterface>)createPolygonGroupShader:(BOOL)isStriped
                                                            unitSphere:(BOOL)unitSphere;

- (nullable id<MCPolygonPatternGroupShaderInterface>)createPolygonPatternGroupShader:(BOOL)fadeInPattern
                                                                          unitSphere:(BOOL)unitSphere;

- (nullable id<MCTextShaderInterface>)createTextShader;

- (nullable id<MCTextInstancedShaderInterface>)createTextInstancedShader;

- (nullable id<MCTextInstancedShaderInterface>)createUnitSphereTextInstancedShader;

- (nullable id<MCRasterShaderInterface>)createRasterShader;

- (nullable id<MCRasterShaderInterface>)createUnitSphereRasterShader;

- (nullable id<MCStretchShaderInterface>)createStretchShader;

- (nullable id<MCStretchInstancedShaderInterface>)createStretchInstancedShader:(BOOL)unitSphere;

- (nullable id<MCColorShaderInterface>)createIcosahedronColorShader;

- (nullable id<MCSphereEffectShaderInterface>)createSphereEffectShader;

- (nullable id<MCSkySphereShaderInterface>)createSkySphereShader;

@end
