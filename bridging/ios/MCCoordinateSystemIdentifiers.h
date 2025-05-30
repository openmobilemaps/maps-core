// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from coordinate_system.djinni

#import <Foundation/Foundation.h>


@interface MCCoordinateSystemIdentifiers : NSObject

+ (int32_t)RENDERSYSTEM;

/**
 * WGS 84 / Pseudo-Mercator
 * https://epsg.io/3857
 */
+ (int32_t)EPSG3857;

/**
 * WGS 84
 * https://epsg.io/4326
 */
+ (int32_t)EPSG4326;

/**
 * LV03+
 * https://epsg.io/2056
 */
+ (int32_t)EPSG2056;

/**
 * CH1903 / LV03
 * https://epsg.io/21781
 */
+ (int32_t)EPSG21781;

/**
 * Unit Sphere Polar
 * phi, theta, radius with reference to earth as unit sphere
 */
+ (int32_t)UnitSphere;

/** e.g. urn:ogc:def:crs:EPSG:21781 */
+ (int32_t)fromCrsIdentifier:(nonnull NSString *)identifier;

/** Use supported coordinate system identifiers defined in this class */
+ (double)unitToMeterFactor:(int32_t)coordinateSystemIdentifier;

@end
