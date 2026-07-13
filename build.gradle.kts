import org.gradle.internal.extensions.core.extra
import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import java.util.Properties

plugins {
	alias(libs.plugins.kotlin.multiplatform)
	alias(libs.plugins.android.multiplatform.library)
	alias(libs.plugins.kotlin.parcelize)
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

group = moduleProperty("GROUP").get()
version = moduleProperty("VERSION_NAME").get()

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
	publishToMavenCentral(true)
	signAllPublications()
}

kotlin {
	compilerOptions {
		freeCompilerArgs.add("-Xexpect-actual-classes")
		optIn.add("kotlin.experimental.ExperimentalObjCName")
		optIn.add("kotlinx.cinterop.ExperimentalForeignApi")
	}

	android {
		namespace = "io.openmobilemaps.mapscore.kmp"
		compileSdk = libs.versions.android.compileSdk.get().toInt()
		minSdk = libs.versions.android.minSdk.get().toInt()

		withJava()

		compilerOptions {
			jvmTarget.set(JvmTarget.JVM_21)
		}
	}

	iosArm64()
	iosSimulatorArm64()

	sourceSets {
		all {
			languageSettings.optIn("kotlin.experimental.ExperimentalObjCName")
			languageSettings.optIn("kotlinx.cinterop.ExperimentalForeignApi")
		}

		commonMain {
			kotlin.srcDir("bridging/kmp/commonMain/kotlin")
			kotlin.srcDir("kmp/commonMain/kotlin")

			dependencies {
				implementation(libs.kotlinx.coroutines)
				implementation(libs.kotlinx.serialization)
			}
		}

		androidMain {
			kotlin.srcDir("bridging/kmp/androidMain/kotlin")
			kotlin.srcDir("kmp/androidMain/kotlin")

			dependencies {
				api(projects.mapscore)
			}
		}

		iosMain {
			kotlin.srcDir("bridging/kmp/iosMain/kotlin")
			kotlin.srcDir("kmp/iosMain/kotlin")
		}
	}

	swiftPMDependencies {
		discoverClangModulesImplicitly.set(false)

		localSwiftPackage(
			directory = project.layout.projectDirectory,
			products = listOf("MapCore", "MapCoreSharedModule"),
			importedClangModules = listOf("MapCore", "MapCoreSharedModule"),
		)
		localSwiftPackage(
			directory = project.layout.projectDirectory.dir("../external/djinni"),
			products = listOf("DjinniSupport"),
			importedClangModules = listOf("DjinniSupport"),
		)
	}
}
