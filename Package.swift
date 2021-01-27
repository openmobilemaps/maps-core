// swift-tools-version:5.2

import PackageDescription

let package = Package(
    name: "MapCore",
    products: [
        .library(
            name: "MapCore",
            targets: ["MapCore"]),
        .library(
            name: "MapCoreSharedModule",
            targets: ["MapCoreSharedModule"])
    ],
    dependencies: [
        .package(name: "DjinniSupport", url: "https://bitbucket.org/ubique-innovation/djinni", .branch("master"))
    ],
    targets: [
        .target(
            name: "MapCore",
            dependencies: ["MapCoreSharedModule"],
            path: "ios/src"),
        .target(
            name: "MapCoreSharedModule",
            dependencies: ["DjinniSupport"],
            path: "shared",
            exclude: ["generated/jni",
                      "generated/java",
                      "generated/objc/objcpp"],
            publicHeadersPath: "generated/objc/objc",
            cxxSettings: [
                .headerSearchPath("generated/objc/objc/"),
            .headerSearchPath("src/grpahics/common"),
            .headerSearchPath("src/grpahics/object"),
            .headerSearchPath("src/grpahics/shader"),
            .headerSearchPath("src/grpahics"),
            .headerSearchPath("**")
            ]),
        .testTarget(
            name: "Tests",
            dependencies: ["MapCore"],
            path: "ios/Tests/Tests"),
    ],
    cxxLanguageStandard: .cxx1z
)
