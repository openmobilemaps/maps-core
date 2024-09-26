// swift-tools-version:5.5

import PackageDescription

let package = Package(
    name: "MapCore",
    platforms: [
        .iOS(.v14)
    ],
    products: [
        .library(
            name: "MapCore",
            targets: ["MapCore"]
        ),
        .library(
            name: "MapCoreSharedModule",
            targets: ["MapCoreSharedModule"]
        ),
        .library(
            name: "MapCoreSharedModuleCpp",
            targets: ["MapCoreSharedModuleCpp"]
        ),
    ],
    dependencies: [
        .package(url: "https://github.com/UbiqueInnovation/djinni.git", .upToNextMajor(from: "1.0.7")),
    ],
    targets: [
        .target(
            name: "earcut",
            path: "external/earcut/",
            exclude: [
                "earcut/glfw",
                "earcut/test",
                "earcut/appveyor.yml",
                "earcut/CHANGELOG.md",
                "earcut/CMakeLists.txt",
                "earcut/LICENSE",
                "earcut/README.md",
            ],
            sources: [
                "",
                "earcut/include/",
                "earcut/include/mapbox/",
            ],
            publicHeadersPath: "earcut/include/mapbox/",
            cxxSettings: [
                .headerSearchPath("earcut/include/"),
                .headerSearchPath("earcut/include/mapbox/"),
            ]
        ),
        .target(
            name: "protozero",
            path: "external/protozero/",
            exclude: [
                "protozero/bench",
                "protozero/cmake",
                "protozero/test",
                "protozero/doc",
                "protozero/test",
                "protozero/tools",
                "protozero/appveyor.yml",
                "protozero/CHANGELOG.md",
                "protozero/CMakeLists.txt",
                "protozero/CONTRIBUTING.md",
                "protozero/FUZZING.md",
                "protozero/LICENSE.from_folly",
                "protozero/LICENSE.md",
                "protozero/README.md",
                "protozero/UPGRADING.md",
            ],
            sources: [
                "",
                "protozero/include/",
            ],
            publicHeadersPath: "protozero/include/",
            cxxSettings: [
                .headerSearchPath("protozero/include/"),
            ]
        ),
        .target(
            name: "vtzero",
            dependencies: ["protozero"],
            path: "external/vtzero/",
            exclude: [
                "vtzero/cmake",
                "vtzero/test",
                "vtzero/doc",
                "vtzero/test",
                "vtzero/examples",
                "vtzero/include-external",
                "vtzero/appveyor.yml",
                "vtzero/build-appveyor.bat",
                "vtzero/build-msys2.bat",
                "vtzero/CHANGELOG.md",
                "vtzero/CMakeLists.txt",
                "vtzero/codecov.yml",
                "vtzero/CONTRIBUTING.md",
                "vtzero/EXTERNAL_LICENSES.txt",
                "vtzero/LICENSE",
                "vtzero/README.md",
            ],
            sources: [
                "",
                "vtzero/include/",
            ],
            publicHeadersPath: "vtzero/include/",
            cxxSettings: [
                .headerSearchPath("vtzero/include/"),
            ]
        ),
        .target(
            name: "MapCore",
            dependencies: ["MapCoreSharedModule"],
            path: "ios",
            exclude: ["readme.md"],
            resources: [
                .process("graphics/Shader/Metal/"),
            ]
        ),
        .target(
            name: "MapCoreSharedModule",
            dependencies: [
                "MapCoreSharedModuleCpp",
            ],
            path: "bridging/ios",
            publicHeadersPath: ""
        ),
        .target(
            name: "MapCoreSharedModuleCpp",
            dependencies: [
                "vtzero",
                "earcut",
                .product(name: "DjinniSupport", package: "djinni"),
            ],
            path: "shared",
            sources: ["src", "public"],
            publicHeadersPath: "public",
            cxxSettings: [
                .headerSearchPath("public"),
                .headerSearchPath("src"),
                .headerSearchPath("src/external/pugixml"),
                .headerSearchPath("src/external/gpc"),
                .headerSearchPath("src/logger"),
                .headerSearchPath("src/graphics"),
                .headerSearchPath("src/graphics/helpers"),
                .headerSearchPath("src/map"),
                .headerSearchPath("src/map/camera"),
                .headerSearchPath("src/map/controls"),
                .headerSearchPath("src/map/coordinates"),
                .headerSearchPath("src/map/layers"),
                .headerSearchPath("src/map/layers/objects"),
                .headerSearchPath("src/map/layers/tiled"),
                .headerSearchPath("src/map/layers/tiled/raster"),
                .headerSearchPath("src/map/layers/tiled/wmts"),
                .headerSearchPath("src/map/layers/tiled/vector"),
                .headerSearchPath("src/map/layers/tiled/vector/geojson"),
                .headerSearchPath("src/map/layers/tiled/vector/geojson/geojsonvt"),
                .headerSearchPath("src/map/layers/tiled/vector/tiles"),
                .headerSearchPath("src/map/layers/tiled/vector/tiles/raster"),
                .headerSearchPath("src/map/layers/tiled/vector/tiles/polygon"),
                .headerSearchPath("src/map/layers/tiled/vector/tiles/line"),
                .headerSearchPath("src/map/layers/tiled/vector/sourcemanagers"),
                .headerSearchPath("src/map/layers/tiled/vector/sublayers"),
                .headerSearchPath("src/map/layers/tiled/vector/sublayers/raster"),
                .headerSearchPath("src/map/layers/tiled/vector/sublayers/line"),
                .headerSearchPath("src/map/layers/tiled/vector/sublayers/polygon"),
                .headerSearchPath("src/map/layers/tiled/vector/sublayers/symbol"),
                .headerSearchPath("src/map/layers/tiled/vector/sublayers/background"),
                .headerSearchPath("src/map/layers/tiled/vector/symbol"),
                .headerSearchPath("src/map/layers/tiled/vector/description"),
                .headerSearchPath("src/map/layers/tiled/vector/parsing"),
                .headerSearchPath("src/map/layers/polygon"),
                .headerSearchPath("src/map/layers/icon"),
                .headerSearchPath("src/map/layers/line"),
                .headerSearchPath("src/map/layers/text"),
                .headerSearchPath("src/map/scheduling"),
                .headerSearchPath("src/utils"),
                .define("DEBUG", to: "1", .when(configuration: .debug)),
                .define("NDEBUG", to: "1", .when(configuration: .release)),
            ]
        ),
    ],
    cxxLanguageStandard: .cxx17
)
