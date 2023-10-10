package io.openmobilemaps.mapscore.extensions

import io.openmobilemaps.mapscore.shared.graphics.common.Color
import java.lang.IllegalArgumentException

/**
 * Parse the receiving #RRGGBB or #AARRGGBB color string to a mapscore color.
 * @return The mapscore color matching the provided color string or null, if the input argument could not be parsed.
 */
fun String.toColor() : Color? {
    val color = try {
        android.graphics.Color.parseColor(this)
    } catch (e: IllegalArgumentException) {
        null
    }
    return color?.toColor()
}

/**
 * Interprets the receiving integer as ARGB color int and converts it to a mapscore color.
 * @return The converted mapscore color specified by the receiving integer.
 */
fun Int.toColor() : Color = Color(
    android.graphics.Color.red(this) / 255f,
    android.graphics.Color.green(this) / 255f,
    android.graphics.Color.blue(this) / 255f,
    android.graphics.Color.alpha(this) / 255f
)

/**
 * Converts the receiving mapscore color to a ARGB color integer
 * @return The ARGB color integer as specified by the receiving mapscore color
 */
fun Color.toInt() : Int = android.graphics.Color.argb(this.a, this.r, this.g, this.b)