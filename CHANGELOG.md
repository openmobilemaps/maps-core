# Changelog for Open Mobile Maps

## Version 2.6.0
- Increases Deployment target to iOS 14
- fixes iOS builds with Xcode 16
- Allow reuse of Android OffscreenMapRenderer
- Fix halo blur artifacts for certain combinations

## Version 2.5.1
- Fix of partially transparent line gap color on OpenGl
- Fix a bug when using a huge number of text styles on OpenGl

## Version 2.5.0
- Initial version of the integration of compute shaders into the render pipeline
- Fix of an issue for OpenGl that could lead to unnecessary buffer creations
- Fix logging of OpenGl Shader compilation/linking errors
- Fix of an invalid usage of clamp in the MapCamera2D
- Add support for a "raster-brightness-shift" attribute in the metadata clause of raster layers in vector style.jsons
- Add support for "text-halo-blur" in vector layers
- Fix of a superfluous alpha multiplication in the Metal RasterShader

## Version 2.4.0
- Add pass masking for single polygons
- Fixing pass masking in non-tile-masked contexts for OpenGl
- Improving pause/resume handling in ThreadPoolScheduler
- Update AGP and Kotlin versions for Android
- Improving gesture handling for iOS mac apps
- Other fixes and stability imrpovements

## Version 2.3.0
- Adds numDrawPreviousOrLaterTLayers field to Tiled2dMapZoomInfo to control the t dimension loading

## Version 2.2.0
- Adds support for `{bbox-epsg-3857}` placeholder in URL template
- Improves the handling of non-interactive polygon layers
- Improves performance logger
- Improves loading behavior of paused vector layer
- Pauses iOS MapView when in background
- Improves the stability of geojson parsing
- Fixes the interpolation for non-numeric values in vector layer styles
- Fixes a bug caused by an ill-timed vector layer reload
- Fixes issues with the zoom during moveToCenter animations

## Version 2.1.0 (08.04.2024)
- Adds dotted line style (line-dotted boolean and an optional skewFactor, defaulting to 1.0 and denoting the stretch factor)
- Fixes style for short dashes
- Fixes line touch area
- Fix map description issues in concurrent settings
- Added a Helper to create geojson strings from GeoJsonPoints

## Version 2.0.8 (04.06.2024)
- Add edge fill options for OpenGL textures
- iOS instanced rendering alpha multiplication fix

## Version 2.0.7 (29.04.2024)
- Adds performance logger (disabled by default)
- Fixes a bug in the android striped pattern shader
- Fixes a bug in geojson hole simplification

## Version 2.0.6 (10.04.2024)
- Log exception when a font can not be loaded

## Version 2.0.5 (08.04.2024)
- Expose thread_pool_callbacks in scheduler interface
- Call onRemoved on layerInterface before the mapScene is destroyed

## Version 2.0.4 (22.03.2024)
- Changes in symbol vector map collision detection: Run collision detection when zoom changes. This change ensures that icons never collide but also carries a performance burden.

## Version 2.0.3 (19.03.2024)
- Fixes thread safety in iOS shader
- Fixes coordinate conversion from EPSG:4326 to EPSG:3857

## Version 2.0.2 (12.03.2024)
- Fixes a bug in iOS Polygon Pattern Shader
- Fixes a bug in camera restricted bounds calculations
- iOS Quad2dInstanced setup bugfix
- Adds density enforcement for android map view

## Version 2.0.1 (07.03.2024)
- Fixed WGS84 bounding box parsing for WMTS layers
- Fix deadlock in renderToImage on iOS
- Fix crash for geojsons without type
- Fix renderToImage for vector layers
- Reference djinni by version on iOS
- Fix crash on certain polygon patters on iOS
- Fix constant loading for some geojsons
- Fix crash of symbol objects
- Fix crash from race condition in geojson reload
- Fix deallocation on OffscreenMapRenderer on Android

## Version 2.0.0 (16.02.2024)
- Major improvements in vector map performance, stability, and rendering, alongside expanded options for style specifications and GeoJSON support.
- Enhanced stability and performance for raster, icon, polygon, and line layers.
- Many additional improvements and bug fixes.

## Version 1.5.3 (16.05.2023)
- Improved MapInterface teardown

## Version 1.5.2 (02.05.2023)
- Fix for Android context memory leak

## Version 1.5.1 (14.04.2023)
- Fix for XCode 14.3.0
- Convenience function for RectCoord to PolygonCoord conversion

## Version 1.5.0 (23.03.2023)
- Implements vector icon anchor
- Evaluate color from feature context
- Expose bounding box in djinni
- Fixes touch propagation in VectorLayer
- iOS: fixes graphics object race conditions
- Fixes vector tile origin
- Adds option to enable/disable underzoom and overzoom
- Adds exception logger
- Adds network activity manager
- Adds two finger zoom out gesture
- Adds option to specify multiple loaders
- Many improvements and bugfixes

## Version 1.4.1 (16.08.2022)
- iOS: updates device to ppi mapping
- iOS: fixes masked line groups
- iOS: fixes bug that caused the map to be cropped if display zoom is enabled
- iOS: fixes metal crash when attempting to load empty texture
- Fixes a error in MapCamera2d::getPaddingCorrectedBounds

## Version 1.4.0 (09.06.2022)
- includes earcut.hpp dependency for polygon triangulation
- Adds polygon and line group rendering objects which can be used to efficiently render many objects with a limited number of styles
- Expands line style options
- Adds masking methods to the layerInterface
- Adds scissoring methods to the layerInterface
- Adds option to display tiled raster layer density dependent
- Adds off screen rendering helpers
- Many improvements and bugfixes

## Version 1.3.3 (16.08.2021)
- Memory leak fix for Android
- Fix async graphics objects setup in lines, polygons and raster tiles

## Version 1.3.2 (13.08.2021)
- Tiled2dMapRasterLayer crash fix for Android

## Version 1.3.1 (11.08.2021)
- Several bugfixes
- Add line layer implementation

## Version 1.3.0 (19.04.2021)
- Fixes iOS retain cycle
- Native library and relevant header files are now properly included in the published dependency
- Added a Circle2dLayerObject
- Bugfixes & improvements

## Version 1.2.0 (03.03.2021)

- Add WMTS capability parsing capabilities
- Add icon layer implementation
- Implemented camera movement inertia
- Improves animation handling
- Bugfixes and improvements

## Version 1.1.0 (01.03.2021)
- Various improvements & bugfixes

## Version 1.0.0 (19.02.2021)
- Initial version of open mobile maps
