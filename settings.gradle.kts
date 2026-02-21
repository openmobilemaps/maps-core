rootProject.name = "maps-core"

val requestedTasksLower = gradle.startParameter.taskNames.map { it.lowercase() }
val isIosOnlyInvocationFromProperty = System.getProperty("b2g.iosOnlyInvocation")?.toBoolean() == true
val isIosOnlyInvocationFromTasks =
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
val isIosOnlyInvocation = isIosOnlyInvocationFromProperty || isIosOnlyInvocationFromTasks

pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}

dependencyResolutionManagement {
    repositories {
        google()
        mavenCentral()
    }
}

if (!isIosOnlyInvocation) {
    includeBuild("android") {
        dependencySubstitution {
            substitute(module("io.openmobilemaps:mapscore")).using(project(":"))
        }
    }
}
