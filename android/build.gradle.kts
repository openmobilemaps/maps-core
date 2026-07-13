import com.vanniktech.maven.publish.AndroidSingleVariantLibrary
import org.gradle.internal.extensions.core.extra
import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import org.jetbrains.kotlin.gradle.tasks.KotlinCompile
import java.util.Properties

plugins {
	alias(libs.plugins.android.library)
	alias(libs.plugins.kotlin.parcelize)
	alias(libs.plugins.compose.compiler)
	alias(libs.plugins.ksp)
	alias(libs.plugins.vanniktech.publish)
}

fun moduleProperty(name: String): Provider<String> =
	providers.gradleProperty(name).orElse(
		providers.provider {
			val file = layout.projectDirectory.file("gradle.properties").asFile
			if (!file.isFile) null
			else Properties().run {
				file.inputStream().use(::load)
				getProperty(name)
			}
		}
	)

android {
	namespace = "io.openmobilemaps.mapscore"

	compileSdk = libs.versions.android.compileSdk.get().toInt()

	defaultConfig {
		minSdk = libs.versions.android.minSdk.get().toInt()

		testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
		consumerProguardFiles("consumer-rules.pro")
		externalNativeBuild {
			cmake {
				arguments += listOf("-DANDROID_STL=c++_shared", "-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON")
				cppFlags += "-frtti -fexceptions -O2"
			}
		}
	}

	externalNativeBuild {
		cmake {
			path = file("CMakeLists.txt")
		}
	}

	sourceSets {
		named("main") {
			kotlin.directories += "../bridging/android/java"
		}
	}

	compileOptions {
		sourceCompatibility = JavaVersion.VERSION_21
		targetCompatibility = JavaVersion.VERSION_21
	}

	buildFeatures {
		prefabPublishing = true
		buildConfig = true
	}

	prefab {
		create("mapscore") {
			headers = ".cpp_includes"
		}
	}

	buildTypes {
		getByName("debug") {
			externalNativeBuild {
				cmake {
					cppFlags += "-DDEBUG"
				}
			}
		}
		getByName("release") {
			externalNativeBuild {
				cmake {
					cppFlags += "-DNDEBUG"
				}
			}
			ndk {
				debugSymbolLevel = "SYMBOL_TABLE"
			}
		}
	}

	// Latest LTS version (as of April 2026)
	ndkVersion = "27.3.13750724"
}

tasks.withType<KotlinCompile>().configureEach {
	compilerOptions.jvmTarget.set(JvmTarget.JVM_21)
}

// Collect public C++ headers into .cpp_includes; prefab points consumers'
// INTERFACE_INCLUDE_DIRECTORIES at this dir. Modelled as a Sync task so it has
// declared inputs/outputs (proper up-to-date checks) and does its work at
// execution time — the old configuration-time copy left no outputs, so a clean
// or fresh checkout wiped the dir and CMake configure failed before it ran.
val copyHeaders by tasks.registering(Sync::class) {
	val headerSources = project.files(
		"src/main/cpp",
		"../bridging/android/jni",
		"../shared/public",
		"../shared/src"
	)
	duplicatesStrategy = DuplicatesStrategy.FAIL
	from(headerSources.asFileTree.files)
	include("**/*.h")
	into(project.file(".cpp_includes"))
}

tasks.named("preBuild") {
	dependsOn(copyHeaders)
}

// Consumers depend on :mapscore:prefab<Variant>Package to obtain the prefab
// cmake config, whose INTERFACE_INCLUDE_DIRECTORIES points at .cpp_includes.
// That dir is produced by copyHeaders, so the prefab tasks must run it first —
// otherwise a clean build configures CMake against a non-existent path.
tasks.matching { it.name.startsWith("prefab") }.configureEach {
	dependsOn(copyHeaders)
}


afterEvaluate {
	tasks.named("bundleReleaseLocalLintAar") {
		dependsOn("prefabReleaseConfigurePackage")
	}
}

publishing {
	repositories {
		maven {
			val ubiqueMavenUrl: String by extra
			val ubiqueMavenUser: String by extra
			val ubiqueMavenPass: String by extra

			name = "UbiqueMaven"
			url = uri(ubiqueMavenUrl)
			credentials {
				username = ubiqueMavenUser
				password = ubiqueMavenPass
			}
			authentication {
				create<BasicAuthentication>("basic")
				create<DigestAuthentication>("digest")
			}
		}
	}
}

mavenPublishing {
	configure(AndroidSingleVariantLibrary(variant = "release"))

	publishToMavenCentral(true)
	signAllPublications()
}

dependencies {
	implementation(fileTree("libs") { include("*.jar") })
	implementation(libs.androidx.annotation)
	implementation(libs.androidx.lifecycle.runtime)
	implementation(libs.androidx.lifecycle.runtime.compose)
	implementation(libs.kotlinx.coroutines.android)

	implementation(libs.moshi)
	ksp(libs.moshi.kotlin.codegen)
	implementation(libs.okhttp)

	api(libs.ubique.djinni)

	// Compose
	implementation(platform(libs.compose.bom))
	implementation(libs.compose.runtime)
	implementation(libs.compose.ui)
	implementation(libs.compose.foundation)

	testImplementation(libs.junit)
	androidTestImplementation(libs.androidx.junit)
	androidTestImplementation(libs.androidx.espresso.core)
}

tasks.named("clean") {
	doLast {
		project.delete("${projectDir}/build")
		project.delete("${projectDir}/.cxx")
		project.delete("${projectDir}/.gradle")
	}
}
