package io.openmobilemaps.mapscore.map.loader.networking

import android.content.Context
import android.content.pm.PackageManager
import android.os.Build


object RequestUtils {

    fun getDefaultUserAgent(context: Context): String {
        val packageInfo = try {
            context.packageManager.getPackageInfo(context.packageName, 0)
        } catch (e: PackageManager.NameNotFoundException) {
            e.printStackTrace()
            null
        }
        return context.packageName + ";" + (packageInfo?.versionName ?: "-") + ";Android;" + Build.VERSION.SDK_INT
    }
}