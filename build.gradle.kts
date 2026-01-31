@file:Suppress("DEPRECATION")

import org.gradle.api.DefaultTask
import org.gradle.api.file.ConfigurableFileCollection
import org.gradle.api.file.DirectoryProperty
import org.gradle.api.file.RegularFileProperty
import org.gradle.api.provider.Property
import org.gradle.api.tasks.Input
import org.gradle.api.tasks.InputDirectory
import org.gradle.api.tasks.InputFiles
import org.gradle.api.tasks.OutputDirectory
import org.gradle.api.tasks.OutputFile
import org.gradle.api.tasks.TaskAction
import org.gradle.process.ExecOperations
import org.jetbrains.kotlin.gradle.ExperimentalKotlinGradlePluginApi
import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import org.jetbrains.kotlin.gradle.plugin.mpp.KotlinAndroidTarget
import org.jetbrains.kotlin.gradle.tasks.CInteropProcess
import java.net.URI
import javax.inject.Inject

group = "io.openmobilemaps.mapscore"

val mapCoreCheckoutPath = project.layout.projectDirectory.asFile.absolutePath
val mapCoreMetalToolchain = providers.environmentVariable("MAPCORE_METAL_TOOLCHAIN")
    .orElse(providers.gradleProperty("mapCoreMetalToolchain"))
    .orElse("")
val mapCoreMetallibToolchain = providers.environmentVariable("MAPCORE_METALLIB_TOOLCHAIN")
    .orElse(providers.gradleProperty("mapCoreMetallibToolchain"))
    .orElse("com.apple.dt.toolchain.XcodeDefault")
val mapCoreMetalTargetSimulator = providers.environmentVariable("MAPCORE_METAL_TARGET_SIMULATOR")
    .orElse(providers.gradleProperty("mapCoreMetalTargetSimulator"))
    .orElse("air64-apple-ios26.0-simulator")
val mapCoreMetalTargetDevice = providers.environmentVariable("MAPCORE_METAL_TARGET_DEVICE")
    .orElse(providers.gradleProperty("mapCoreMetalTargetDevice"))
    .orElse("air64-apple-ios26.0")

plugins {
    id("org.jetbrains.kotlin.multiplatform") version "2.3.0"
    id("com.android.library") version "8.13.2"
    id("maven-publish")
    id("io.github.frankois944.spmForKmp") version "1.4.6"
}

@OptIn(ExperimentalKotlinGradlePluginApi::class)
kotlin {
    compilerOptions {
        freeCompilerArgs.add("-Xexpect-actual-classes") // Opt-in for expect/actual classes
    }

    applyDefaultHierarchyTemplate()

    androidTarget {
        compilerOptions {
            jvmTarget.set(JvmTarget.JVM_17)
        }
        publishLibraryVariants("debug", "release")
    }

    val mapCoreCinteropName = "MapCoreKmp"
    val iosTargets = listOf(
        iosArm64(),
        iosSimulatorArm64()
    )

    iosTargets.forEach { iosTarget ->
        iosTarget.compilations {
            val main by getting {
                cinterops.create(mapCoreCinteropName)
            }
        }
    }

    sourceSets {
        val commonMain by getting {
            kotlin.srcDir("kmp/commonMain/kotlin")
            dependencies {
                api("org.jetbrains.kotlinx:kotlinx-coroutines-core:1.10.2")
            }
        }
        val androidMain by getting {
            kotlin.srcDir("kmp/androidMain/kotlin")
            dependencies {
                api("io.openmobilemaps:mapscore:3.6.0")
                api("io.openmobilemaps:layer-gps:3.6.0")
                implementation("androidx.lifecycle:lifecycle-runtime-ktx:2.9.3")
                implementation("androidx.lifecycle:lifecycle-viewmodel-ktx:2.9.3")
                implementation("ch.ubique.android:djinni-support-lib:1.1.1")
            }
        }
        val iosMain by getting {
            kotlin.srcDir("kmp/iosMain/kotlin")
            languageSettings.optIn("kotlinx.cinterop.ExperimentalForeignApi")
        }
        val iosArm64Main by getting {
            languageSettings.optIn("kotlinx.cinterop.ExperimentalForeignApi")
        }
        val iosSimulatorArm64Main by getting {
            languageSettings.optIn("kotlinx.cinterop.ExperimentalForeignApi")
        }
    }
}

android {
    namespace = "io.openmobilemaps.mapscore.kmp"
    compileSdk = 36
    defaultConfig {
        minSdk = 28
    }
}

swiftPackageConfig {
    create("MapCoreKmp") {
        minIos = "14.0"
        bridgeSettings {
            cSetting {
                headerSearchPath = listOf("Sources/djinni-objc")
            }
        }
        dependency {
            localPackage(
                mapCoreCheckoutPath,
                "maps-core"
            ) {
                add("MapCoreObjC", exportToKotlin = true)
                add("MapCoreSharedModule", exportToKotlin = true)
            }
            remotePackageVersion(
                url = URI("https://github.com/openmobilemaps/layer-gps"),
                packageName = "layer-gps",
                version = "3.6.0",
            ) {
                add("LayerGpsSharedModule", exportToKotlin = true)
            }
        }
    }
}

// Avoid overlapping Package.resolved outputs between per-target SwiftPM compile tasks.
tasks
    .matching { it.name.startsWith("SwiftPackageConfigAppleMapCoreKmpCompileSwiftPackage") }
    .configureEach {
        val packageResolveFileGetter = javaClass.methods.firstOrNull { it.name == "getPackageResolveFile" }
        val packageResolveFile =
            project.layout.buildDirectory.file(
                "spmKmpPlugin/MapCoreKmp/package-resolved/${name}/Package.resolved",
            )
        val packageResolveProperty =
            packageResolveFileGetter
                ?.invoke(this) as? RegularFileProperty
        packageResolveProperty?.set(packageResolveFile)

        doLast {
            val sourceFile =
                project.layout.buildDirectory
                    .file("spmKmpPlugin/MapCoreKmp/Package.resolved")
                    .get()
                    .asFile
            if (!sourceFile.exists()) return@doLast
            val targetFile =
                project.layout.buildDirectory
                    .file("spmKmpPlugin/MapCoreKmp/package-resolved/${name}/Package.resolved")
                    .get()
                    .asFile
            targetFile.parentFile.mkdirs()
            sourceFile.copyTo(targetFile, overwrite = true)
        }
    }

tasks.withType<CInteropProcess>().configureEach {
    if (name.contains("MapCoreKmp")) {
        settings.compilerOpts("-I$mapCoreCheckoutPath/external/djinni/support-lib/objc")
        settings.compilerOpts("-I$mapCoreCheckoutPath/bridging/ios")
    }
}

val mapCoreSpmBuiltDir =
    project.layout.buildDirectory.dir("spmKmpPlugin/MapCoreKmp/scratch/arm64 x86_64-apple-ios-simulator/release").get().asFile
mapCoreSpmBuiltDir.mkdirs()

val mapCoreSpmDeviceDir =
    project.layout.buildDirectory.dir("spmKmpPlugin/MapCoreKmp/scratch/arm64-apple-ios/release")
val mapCoreSpmSimulatorDir =
    project.layout.buildDirectory.dir("spmKmpPlugin/MapCoreKmp/scratch/arm64-apple-ios-simulator/release")

afterEvaluate {
    val deviceTaskName = "SwiftPackageConfigAppleMapCoreKmpCompileSwiftPackageIosArm64"
    val simulatorTaskName = "SwiftPackageConfigAppleMapCoreKmpCompileSwiftPackageIosSimulatorArm64"
    if (tasks.findByName(deviceTaskName) != null) return@afterEvaluate
    runCatching {
        tasks.register(deviceTaskName) {
            group = "io.github.frankois944.spmForKmp.tasks"
            description = "Fallback: copy simulator SwiftPM output for iOS device metal compilation"
            dependsOn(simulatorTaskName)
            doLast {
                val sourceDir = mapCoreSpmSimulatorDir.get().asFile
                if (!sourceDir.exists()) return@doLast
                val targetDir = mapCoreSpmDeviceDir.get().asFile
                targetDir.mkdirs()
                copy {
                    from(sourceDir)
                    into(targetDir)
                }
            }
        }
    }.onFailure { error ->
        val message = error.message.orEmpty()
        if (!message.contains("already exists")) {
            throw error
        }
    }
}

abstract class CompileMapCoreMetallibTask : DefaultTask() {
    @get:Input
    abstract val sdk: Property<String>

    @get:Input
    abstract val toolchainId: Property<String>

    @get:Input
    abstract val metallibToolchainId: Property<String>

    @get:Input
    abstract val targetTriple: Property<String>

    @get:InputDirectory
    abstract val bundleDir: DirectoryProperty

    @get:InputFiles
    abstract val metalSources: ConfigurableFileCollection

    @get:OutputDirectory
    abstract val outputDir: DirectoryProperty

    @get:OutputFile
    abstract val metallibFile: RegularFileProperty

    @get:Inject
    abstract val execOperations: ExecOperations

    @TaskAction
    fun run() {
        val bundleRoot = bundleDir.get().asFile
        if (!bundleRoot.exists()) return
        val metalFiles = metalSources.files.sortedBy { it.name }
        if (metalFiles.isEmpty()) return
        val toolchain = toolchainId.orNull?.takeIf { it.isNotBlank() }
        val metallibToolchain = metallibToolchainId.orNull?.takeIf { it.isNotBlank() }

        val outputRoot = outputDir.get().asFile
        outputRoot.mkdirs()

        val airFiles = metalFiles.map { file ->
            File(outputRoot, "${file.nameWithoutExtension}.air")
        }

        metalFiles.zip(airFiles).forEach { (metalFile, airFile) ->
            execOperations.exec {
                val args = buildList {
                    add("xcrun")
                    if (toolchain != null) {
                        add("--toolchain")
                        add(toolchain)
                    }
                    addAll(
                        listOf(
                            "-sdk",
                            sdk.get(),
                            "metal",
                            "-target",
                            targetTriple.get(),
                            "-c",
                            metalFile.absolutePath,
                            "-o",
                            airFile.absolutePath,
                        ),
                    )
                }
                commandLine(args)
            }
        }

        val metallibArgs = buildList {
            add("xcrun")
            if (metallibToolchain != null) {
                add("--toolchain")
                add(metallibToolchain)
            }
            addAll(
                listOf(
                    "-sdk",
                    sdk.get(),
                    "metallib",
                ),
            )
            airFiles.forEach { add(it.absolutePath) }
            add("-o")
            add(metallibFile.get().asFile.absolutePath)
        }

        execOperations.exec {
            commandLine(metallibArgs)
        }
    }
}

val compileMapCoreMetallibIosSimulator = tasks.register<CompileMapCoreMetallibTask>("compileMapCoreMetallibIosSimulator") {
    dependsOn("SwiftPackageConfigAppleMapCoreKmpCompileSwiftPackageIosSimulatorArm64")
    sdk.set("iphonesimulator")
    toolchainId.set(mapCoreMetalToolchain)
    metallibToolchainId.set(mapCoreMetallibToolchain)
    targetTriple.set(mapCoreMetalTargetSimulator)
    bundleDir.set(
        project.layout.buildDirectory
            .dir("spmKmpPlugin/MapCoreKmp/scratch/arm64-apple-ios-simulator/release/MapCore_MapCore.bundle"),
    )
    outputDir.set(project.layout.buildDirectory.dir("spmKmpPlugin/MapCoreKmp/metal/iphonesimulator"))
    metalSources.from(bundleDir.map { it.asFileTree.matching { include("**/*.metal") } })
    metallibFile.set(bundleDir.map { it.file("default.metallib") })
}

val compileMapCoreMetallibIosArm64 = tasks.register<CompileMapCoreMetallibTask>("compileMapCoreMetallibIosArm64") {
    dependsOn("SwiftPackageConfigAppleMapCoreKmpCompileSwiftPackageIosArm64")
    sdk.set("iphoneos")
    toolchainId.set(mapCoreMetalToolchain)
    metallibToolchainId.set(mapCoreMetallibToolchain)
    targetTriple.set(mapCoreMetalTargetDevice)
    bundleDir.set(
        project.layout.buildDirectory
            .dir("spmKmpPlugin/MapCoreKmp/scratch/arm64-apple-ios/release/MapCore_MapCore.bundle"),
    )
    outputDir.set(project.layout.buildDirectory.dir("spmKmpPlugin/MapCoreKmp/metal/iphoneos"))
    metalSources.from(bundleDir.map { it.asFileTree.matching { include("**/*.metal") } })
    metallibFile.set(bundleDir.map { it.file("default.metallib") })
}

tasks.matching { it.name == "SwiftPackageConfigAppleMapCoreKmpCopyPackageResourcesIosSimulatorArm64" }
    .configureEach { dependsOn(compileMapCoreMetallibIosSimulator) }
tasks.matching { it.name == "SwiftPackageConfigAppleMapCoreKmpCopyPackageResourcesIosArm64" }
    .configureEach { dependsOn(compileMapCoreMetallibIosArm64) }

tasks.matching { it.name == "compileKotlinIosSimulatorArm64" }
    .configureEach { dependsOn(compileMapCoreMetallibIosSimulator) }
tasks.matching { it.name == "compileKotlinIosArm64" }
    .configureEach { dependsOn(compileMapCoreMetallibIosArm64) }
