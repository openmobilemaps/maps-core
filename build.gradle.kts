@file:Suppress("DEPRECATION")
@file:OptIn(ExperimentalSpmForKmpFeature::class)

import io.github.frankois944.spmForKmp.swiftPackageConfig
import io.github.frankois944.spmForKmp.utils.ExperimentalSpmForKmpFeature
import org.gradle.api.DefaultTask
import org.gradle.api.file.ConfigurableFileCollection
import org.gradle.api.file.DirectoryProperty
import org.gradle.api.file.RegularFileProperty
import org.gradle.api.provider.Property
import org.gradle.api.tasks.Input
import org.gradle.api.tasks.InputDirectory
import org.gradle.api.tasks.InputFile
import org.gradle.api.tasks.InputFiles
import org.gradle.api.tasks.Internal
import org.gradle.api.tasks.Optional
import org.gradle.api.tasks.OutputDirectory
import org.gradle.api.tasks.OutputFile
import org.gradle.api.tasks.CacheableTask
import org.gradle.api.tasks.PathSensitive
import org.gradle.api.tasks.PathSensitivity
import org.gradle.api.tasks.TaskAction
import org.gradle.process.ExecOperations
import org.jetbrains.kotlin.gradle.ExperimentalKotlinGradlePluginApi
import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import org.jetbrains.kotlin.gradle.plugin.mpp.KotlinAndroidTarget
import org.jetbrains.kotlin.gradle.tasks.CInteropProcess
import java.io.ByteArrayOutputStream
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
val mapCoreSpmBuildType = (project.findProperty("KOTLIN_FRAMEWORK_BUILD_TYPE") as? String)
    ?.lowercase()
    ?.takeIf { it == "debug" || it == "release" }
    ?: "release"
val mapCoreSpmScratchDir = project.layout.buildDirectory.dir("spmKmpPlugin/MapCoreKmp/scratch")
val isIosOnlyInvocationFromRoot = System.getProperty("b2g.iosOnlyInvocation")?.toBoolean() == true
val requestedTasksLower = gradle.startParameter.taskNames.map { it.lowercase() }
val isIosOnlyInvocation =
    isIosOnlyInvocationFromRoot ||
        (
            requestedTasksLower.isNotEmpty() &&
                requestedTasksLower.all { task ->
                    val looksLikeIosTask =
                        task.contains("ios") ||
                            task.contains("swiftpackage") ||
                            task.contains("xcode") ||
                            task.contains("cinterop")
                    val looksLikeAndroidTask =
                        task.contains("android") ||
                            task.contains("assemble") ||
                            task.contains("bundle") ||
                            task.contains("lint") ||
                            task.contains("install") ||
                            task.contains("connected")
                    looksLikeIosTask && !looksLikeAndroidTask
                }
            )
val isAndroidOrIdeInvocation =
    requestedTasksLower.any { task ->
        task.contains("android") ||
            task.contains("assemble") ||
            task.contains("bundle") ||
            task.contains("lint") ||
            task.contains("install") ||
            task.contains("connected") ||
            task.contains("preparekotlinbuildscriptmodel") ||
            task.contains("kotlinbuildscriptmodel")
    }
val skipIosTasksForCurrentInvocation = !isIosOnlyInvocation && isAndroidOrIdeInvocation

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
        iosTarget.binaries.framework {
            isStatic = true
        }
        iosTarget.compilations {
            val main by getting {
                cinterops.create(mapCoreCinteropName)
            }
        }
        iosTarget.swiftPackageConfig(cinteropName = mapCoreCinteropName) {
            customPackageSourcePath = "$mapCoreCheckoutPath/kmp"
            minIos = "16.4"
            debug = mapCoreSpmBuildType == "debug"
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
                    add("MapCore")
                    add("MapCoreSharedModule", exportToKotlin = true)
                }
            }
        }
    }

    sourceSets {
        val commonMain by getting {
            kotlin.srcDir("bridging/kmp/commonMain/kotlin")
            kotlin.srcDir("kmp/commonMain/kotlin")
            dependencies {
                api("org.jetbrains.kotlinx:kotlinx-coroutines-core:1.10.2")
            }
        }
        val androidMain by getting {
            kotlin.srcDir("bridging/kmp/androidMain/kotlin")
            kotlin.srcDir("kmp/androidMain/kotlin")
            dependencies {
                api("io.openmobilemaps:mapscore:3.6.0")
                implementation("androidx.lifecycle:lifecycle-runtime-ktx:2.9.3")
                implementation("androidx.lifecycle:lifecycle-viewmodel-ktx:2.9.3")
                implementation("ch.ubique.android:djinni-support-lib:1.1.1")
            }
        }
        val iosMain by getting {
            kotlin.srcDir("bridging/kmp/iosMain/kotlin")
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

abstract class CheckMapCoreGitTagTask : DefaultTask() {
    @get:Internal
    abstract val repoDir: DirectoryProperty

    @get:Inject
    abstract val execOperations: ExecOperations

    @TaskAction
    fun run() {
        val stderr = ByteArrayOutputStream()
        val result =
            execOperations.exec {
                workingDir(repoDir.get().asFile)
                commandLine("git", "describe", "--tags", "--exact-match", "HEAD")
                isIgnoreExitValue = true
                errorOutput = stderr
            }
        if (result.exitValue == 0) return
        val errorText = stderr.toString().trim()
        val detail = if (errorText.isNotBlank()) ": $errorText" else ""
        val warningMessage =
            "MapCore KMP build: current commit is not tagged. " +
                "Please use a release-tagged version of maps-core. " +
                "(git describe --tags --exact-match HEAD failed$detail)"
        val buildFilePath = project.layout.projectDirectory.file("build.gradle.kts").asFile.absolutePath
        println("$buildFilePath:1: warning: $warningMessage")
    }
}

abstract class CopyFileIfExistsTask : DefaultTask() {
    @get:InputFile
    @get:Optional
    abstract val sourceFile: RegularFileProperty

    @get:OutputFile
    abstract val targetFile: RegularFileProperty

    @TaskAction
    fun run() {
        val source = sourceFile.orNull?.asFile ?: return
        if (!source.exists()) return
        val target = targetFile.get().asFile
        target.parentFile.mkdirs()
        source.copyTo(target, overwrite = true)
    }
}

abstract class RewriteMapCorePackageManifestTask : DefaultTask() {
    @get:InputFile
    @get:Optional
    abstract val packageFile: RegularFileProperty

    @TaskAction
    fun run() {
        val file = packageFile.orNull?.asFile ?: return
        if (!file.exists()) return
        val original = file.readText()
        var updated = original.replace("type: .dynamic", "type: .static")
        updated =
            updated.replace(
                ".product(name: \"MapCore\", package: \"maps-core\"),.product(name: \"MapCoreSharedModule\", package: \"maps-core\")",
                ".product(name: \"MapCore\", package: \"maps-core\")",
            )
        updated =
            updated.replace(
                ".product(name: \"MapCore\", package: \"maps-core\"), .product(name: \"MapCoreSharedModule\", package: \"maps-core\")",
                ".product(name: \"MapCore\", package: \"maps-core\")",
            )
        if (!updated.contains("linkerSettings:")) {
            updated =
                updated.replace(
                    ",cSettings: [.headerSearchPath(\"Sources/djinni-objc\")]",
                    ",cSettings: [.headerSearchPath(\"Sources/djinni-objc\")],linkerSettings: [.linkedLibrary(\"objc\"),.linkedFramework(\"Foundation\")]",
                )
        }
        if (updated != original) {
            file.writeText(updated)
        }
    }
}

abstract class PatchMapCoreCInteropDefsTask : DefaultTask() {
    @get:Internal
    abstract val buildDirRoot: DirectoryProperty

    @get:Input
    abstract val targetDirName: Property<String>

    @get:Input
    abstract val platformDirName: Property<String>

    @get:Input
    abstract val buildType: Property<String>

    @TaskAction
    fun run() {
        val buildRoot = buildDirRoot.get().asFile
        val targetDir = targetDirName.get()
        val platformDir = platformDirName.get()
        val type = buildType.get()
        val libDir = File(buildRoot, "spmKmpPlugin/MapCoreKmp/scratch/$platformDir/$type")
        val defRoot = File(buildRoot, "spmKmpPlugin/MapCoreKmp/defFiles/$targetDir")
        val defNames = listOf("MapCoreSharedModule", "MapCoreKmp_bridge", "MapCoreKmp")

        defNames.forEach { defName ->
            val defFile = File(defRoot, "$defName.def")
            if (!defFile.exists()) return@forEach
            val extraOpts =
                when (defName) {
                    "MapCoreKmp_bridge" -> listOf("-L\"${libDir.absolutePath}\"", "-lMapCoreKmp")
                    else -> emptyList()
                }
            if (extraOpts.isNotEmpty()) {
                ensureLinkerOpts(defFile, extraOpts)
            }
            stripLinkerOpts(defFile, setOf("-lMapCoreSharedModule"))
        }
    }

    private fun ensureLinkerOpts(defFile: File, opts: List<String>) {
        val lines = defFile.readLines().toMutableList()
        val idx = lines.indexOfFirst { it.trimStart().startsWith("linkerOpts") }
        val optsText = opts.joinToString(" ")
        if (idx >= 0) {
            val current = lines[idx]
            val missing = opts.filterNot { current.contains(it) }
            if (missing.isNotEmpty()) {
                lines[idx] = current + " " + missing.joinToString(" ")
            }
        } else {
            lines.add("linkerOpts = $optsText")
        }
        defFile.writeText(lines.joinToString("\n"))
    }

    private fun stripLinkerOpts(defFile: File, optsToRemove: Set<String>) {
        val lines = defFile.readLines().toMutableList()
        val idx = lines.indexOfFirst { it.trimStart().startsWith("linkerOpts") }
        if (idx < 0) return
        val line = lines[idx]
        val prefix = line.substringBefore("=").trimEnd() + " ="
        val tokens = line.substringAfter("=").trim().split(Regex("\\s+"))
        val filtered = tokens.filterNot { it in optsToRemove }
        if (filtered.isEmpty()) {
            lines.removeAt(idx)
        } else {
            lines[idx] = "$prefix ${filtered.joinToString(" ")}"
        }
        defFile.writeText(lines.joinToString("\n"))
    }
}

abstract class CopyDirectoryContentsIfExistsTask : DefaultTask() {
    @get:InputDirectory
    @get:Optional
    @get:PathSensitive(PathSensitivity.RELATIVE)
    abstract val sourceDir: DirectoryProperty

    @get:OutputDirectory
    abstract val targetDir: DirectoryProperty

    @TaskAction
    fun run() {
        val sourceRoot = sourceDir.orNull?.asFile ?: return
        if (!sourceRoot.exists()) return
        val targetRoot = targetDir.get().asFile
        targetRoot.mkdirs()

        sourceRoot.walkTopDown().forEach { source ->
            val relativePath = source.relativeTo(sourceRoot).path
            if (relativePath.isEmpty()) return@forEach
            val target = File(targetRoot, relativePath)
            if (source.isDirectory) {
                target.mkdirs()
            } else {
                target.parentFile.mkdirs()
                source.copyTo(target, overwrite = true)
            }
        }
    }
}

val checkMapCoreGitTag = tasks.register<CheckMapCoreGitTagTask>("checkMapCoreGitTag") {
    group = "verification"
    description = "Warn if the current maps-core commit does not have a tag."
    repoDir.set(project.layout.projectDirectory)
}

tasks.matching { it.name.startsWith("compileKotlin") }
    .configureEach { dependsOn(checkMapCoreGitTag) }
tasks
    .matching { it.name.startsWith("SwiftPackageConfigAppleMapCoreKmpCompileSwiftPackage") }
    .configureEach { dependsOn(checkMapCoreGitTag) }
tasks.matching { it.name == "build" || it.name == "assemble" }
    .configureEach { dependsOn(checkMapCoreGitTag) }
tasks.matching { it.name == "kmpPartiallyResolvedDependenciesChecker" }
    .configureEach {
        enabled = !isIosOnlyInvocation
    }

// spmForKmp tasks still use non-cache-safe execution APIs. Keep them functional while
// allowing configuration cache reuse for compatible task graphs.
tasks
    .matching { it.name.startsWith("SwiftPackageConfigAppleMapCoreKmp") }
    .configureEach {
        notCompatibleWithConfigurationCache(
            "spmForKmp tasks access task/project state during execution",
        )
    }
if (skipIosTasksForCurrentInvocation) {
    tasks.configureEach {
        val taskName = name.lowercase()
        val isIosSpecificTask =
            taskName.contains("ios") ||
                taskName.startsWith("swiftpackageconfigapplemapcorekmp") ||
                taskName.contains("mapcoremetallib") ||
                taskName.startsWith("syncmapcorepackageresolvedios")
        if (isIosSpecificTask) {
            enabled = false
        }
    }
}

// Avoid overlapping Package.resolved outputs between per-target SwiftPM compile tasks.
val syncPackageResolvedIosArm64 = tasks.register<CopyFileIfExistsTask>("syncMapCorePackageResolvedIosArm64") {
    sourceFile.set(project.layout.buildDirectory.file("spmKmpPlugin/MapCoreKmp/Package.resolved"))
    targetFile.set(
        project.layout.buildDirectory.file(
            "spmKmpPlugin/MapCoreKmp/package-resolved/SwiftPackageConfigAppleMapCoreKmpCompileSwiftPackageIosArm64/Package.resolved",
        ),
    )
}
val syncPackageResolvedIosSimulatorArm64 =
    tasks.register<CopyFileIfExistsTask>("syncMapCorePackageResolvedIosSimulatorArm64") {
        sourceFile.set(project.layout.buildDirectory.file("spmKmpPlugin/MapCoreKmp/Package.resolved"))
        targetFile.set(
            project.layout.buildDirectory.file(
                "spmKmpPlugin/MapCoreKmp/package-resolved/SwiftPackageConfigAppleMapCoreKmpCompileSwiftPackageIosSimulatorArm64/Package.resolved",
            ),
        )
    }
listOf(syncPackageResolvedIosArm64, syncPackageResolvedIosSimulatorArm64).forEach { syncTask ->
    syncTask.configure {
        mustRunAfter("SwiftPackageConfigAppleMapCoreKmpCompileSwiftPackageIosArm64")
        mustRunAfter("SwiftPackageConfigAppleMapCoreKmpCompileSwiftPackageIosSimulatorArm64")
    }
}

tasks.matching { it.name == "SwiftPackageConfigAppleMapCoreKmpCompileSwiftPackageIosArm64" }
    .configureEach {
        val packageResolveFileGetter = javaClass.methods.firstOrNull { it.name == "getPackageResolveFile" }
        val packageResolveProperty =
            packageResolveFileGetter
                ?.invoke(this) as? RegularFileProperty
        packageResolveProperty?.set(syncPackageResolvedIosArm64.flatMap { it.targetFile })
        finalizedBy(syncPackageResolvedIosArm64)
    }
tasks.matching { it.name == "SwiftPackageConfigAppleMapCoreKmpCompileSwiftPackageIosSimulatorArm64" }
    .configureEach {
        val packageResolveFileGetter = javaClass.methods.firstOrNull { it.name == "getPackageResolveFile" }
        val packageResolveProperty =
            packageResolveFileGetter
                ?.invoke(this) as? RegularFileProperty
        packageResolveProperty?.set(syncPackageResolvedIosSimulatorArm64.flatMap { it.targetFile })
        finalizedBy(syncPackageResolvedIosSimulatorArm64)
    }

// Ensure MapCoreKmp is built as a static SwiftPM library; MapCore symbols should be provided by the host app.
val rewriteMapCorePackageManifest = tasks.register<RewriteMapCorePackageManifestTask>("rewriteMapCorePackageManifest") {
    packageFile.set(project.layout.buildDirectory.file("spmKmpPlugin/MapCoreKmp/Package.swift"))
}
tasks.matching { it.name.startsWith("SwiftPackageConfigAppleMapCoreKmpGenerateSwiftPackage") }
    .configureEach { finalizedBy(rewriteMapCorePackageManifest) }

// SwiftPM emits libMapCoreKmp.a for static builds (debug + release).
val patchMapCoreCInteropDefsIosArm64 = tasks.register<PatchMapCoreCInteropDefsTask>("patchMapCoreCInteropDefsIosArm64") {
    buildDirRoot.set(project.layout.buildDirectory)
    targetDirName.set("iosArm64")
    platformDirName.set("arm64-apple-ios")
    buildType.set(mapCoreSpmBuildType)
}
val patchMapCoreCInteropDefsIosSimulatorArm64 =
    tasks.register<PatchMapCoreCInteropDefsTask>("patchMapCoreCInteropDefsIosSimulatorArm64") {
        buildDirRoot.set(project.layout.buildDirectory)
        targetDirName.set("iosSimulatorArm64")
        platformDirName.set("arm64-apple-ios-simulator")
        buildType.set(mapCoreSpmBuildType)
    }

tasks.matching { it.name == "SwiftPackageConfigAppleMapCoreKmpGenerateCInteropDefinitionIosArm64" }
    .configureEach {
        val compiledBinaryProp =
            javaClass.getMethod("getCompiledBinary").invoke(this) as RegularFileProperty
        compiledBinaryProp.set(
            project.layout.buildDirectory.file(
                "spmKmpPlugin/MapCoreKmp/scratch/arm64-apple-ios/$mapCoreSpmBuildType/libMapCoreKmp.a",
            ),
        )
        finalizedBy(patchMapCoreCInteropDefsIosArm64)
    }
tasks.matching { it.name == "SwiftPackageConfigAppleMapCoreKmpGenerateCInteropDefinitionIosSimulatorArm64" }
    .configureEach {
        val compiledBinaryProp =
            javaClass.getMethod("getCompiledBinary").invoke(this) as RegularFileProperty
        compiledBinaryProp.set(
            project.layout.buildDirectory.file(
                "spmKmpPlugin/MapCoreKmp/scratch/arm64-apple-ios-simulator/$mapCoreSpmBuildType/libMapCoreKmp.a",
            ),
        )
        finalizedBy(patchMapCoreCInteropDefsIosSimulatorArm64)
    }

tasks.withType<CInteropProcess>().configureEach {
    if (name.contains("MapCore")) {
        settings.compilerOpts("-I$mapCoreCheckoutPath/external/djinni/support-lib/objc")
        settings.compilerOpts("-I$mapCoreCheckoutPath/bridging/ios")
    }
}

val mapCoreSpmBuiltDir =
    project.layout.buildDirectory.dir("spmKmpPlugin/MapCoreKmp/scratch/arm64 x86_64-apple-ios-simulator/$mapCoreSpmBuildType").get().asFile
mapCoreSpmBuiltDir.mkdirs()

val mapCoreSpmDeviceDir =
    project.layout.buildDirectory.dir("spmKmpPlugin/MapCoreKmp/scratch/arm64-apple-ios/$mapCoreSpmBuildType")
val mapCoreSpmSimulatorDir =
    project.layout.buildDirectory.dir("spmKmpPlugin/MapCoreKmp/scratch/arm64-apple-ios-simulator/$mapCoreSpmBuildType")

afterEvaluate {
    val deviceTaskName = "SwiftPackageConfigAppleMapCoreKmpCompileSwiftPackageIosArm64"
    val simulatorTaskName = "SwiftPackageConfigAppleMapCoreKmpCompileSwiftPackageIosSimulatorArm64"
    if (tasks.findByName(deviceTaskName) != null) return@afterEvaluate
    runCatching {
        tasks.register<CopyDirectoryContentsIfExistsTask>(deviceTaskName) {
            group = "io.github.frankois944.spmForKmp.tasks"
            description = "Fallback: copy simulator SwiftPM output for iOS device metal compilation"
            dependsOn(simulatorTaskName)
            sourceDir.set(mapCoreSpmSimulatorDir)
            targetDir.set(mapCoreSpmDeviceDir)
        }
    }.onFailure { error ->
        val message = error.message.orEmpty()
        if (!message.contains("already exists")) {
            throw error
        }
    }
}

@CacheableTask
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
    @get:Optional
    @get:PathSensitive(PathSensitivity.RELATIVE)
    abstract val bundleDir: DirectoryProperty

    @get:InputFiles
    @get:PathSensitive(PathSensitivity.RELATIVE)
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
            .dir("spmKmpPlugin/MapCoreKmp/scratch/arm64-apple-ios-simulator/$mapCoreSpmBuildType/MapCore_MapCore.bundle"),
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
            .dir("spmKmpPlugin/MapCoreKmp/scratch/arm64-apple-ios/$mapCoreSpmBuildType/MapCore_MapCore.bundle"),
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
