# Changelog for Open Mobile Maps

## Version 1.5.0 (23.03.2022)
- implements vector icon anchor
- evaluate color from feature context 
- expose bounding box in djinni
- fixes touch propagation in VectorLayer
- iOS: fixes graphics object race conditions  
- fixes vector tile origin 
- adds option to enable/disable underzoom and overzoom
- adds exception logger 
- adds network activity manager
- adds two finger zoom out gesture 
- adds option to specify multiple loaders
- many improvements and bugfixes

## Version 1.4.1 (16.08.2022)
- iOS: updates device to ppi mapping
- iOS: fixes masked line groups
- iOS: fixes bug that caused the map to be cropped if display zoom is enabled
- iOS: fixes metal crash when attempting to load empty texture
- fixes a error in MapCamera2d::getPaddingCorrectedBounds

## Version 1.4.0 (09.06.2022)
- includes earcut.hpp dependency for polygon triangulation
- adds polygon and line group rendering objects which can be used to efficiently render many objects with a limited number of styles
- expands line style options
- adds masking methods to the layerInterface
- adds scissoring methods to the layerInterface
- adds option to display tiled raster layer density dependent
- adds off screen rendering helpers
- many improvements and bugfixes

## Version 1.3.3 (16.08.2021)
- Memory leak fix for Android
- Fix async graphics objects setup in lines, polygons and raster tiles

## Version 1.3.2 (13.08.2021)
- Tiled2dMapRasterLayer crash fix for Android

## Version 1.3.1 (11.08.2021)
- Several bugfixes
- Add line layer implementation

## Version 1.3.0 (19.04.2021)
- fixes iOS retain cycle
- Native library and relevant header files are now properly included in the published dependency
- Added a Circle2dLayerObject
- bugfixes & improvements

## Version 1.2.0 (03.03.2021)

- Add WMTS capability parsing capabilities
- Add icon layer implementation
- implemented camera movement inertia
- improves animation handling
- bugfixes and improvements

## Version 1.1.0 (01.03.2021)
- various improvements & bugfixes

## Version 1.0.0 (19.02.2021)
- initial version of open mobile maps
