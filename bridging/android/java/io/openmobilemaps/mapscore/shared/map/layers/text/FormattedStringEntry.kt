// AUTOGENERATED FILE - DO NOT MODIFY!
// This file was generated by Djinni from text.djinni

package io.openmobilemaps.mapscore.shared.map.layers.text

data class FormattedStringEntry(
    val text: String,
    val scale: Float,
) : Comparable<FormattedStringEntry> {

    override fun compareTo(other: FormattedStringEntry): Int {
        var tempResult : Int
        tempResult = this.text.compareTo(other.text)
        if (tempResult != 0) {
            return tempResult
        }
        if (this.scale < other.scale) {
            tempResult = -1;
        } else if (this.scale > other.scale) {
            tempResult = 1;
        } else {
            tempResult = 0;
        }
        if (tempResult != 0) {
            return tempResult
        }
        return 0
    }
}
