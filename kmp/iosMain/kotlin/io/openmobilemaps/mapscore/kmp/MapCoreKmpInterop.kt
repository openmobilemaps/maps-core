package io.openmobilemaps.mapscore.kmp

import platform.Foundation.NSData
import swiftPMImport.io.openmobilemaps.mapscore.kmp.MCTextureHolderInterfaceProtocol
import swiftPMImport.io.openmobilemaps.mapscore.kmp.TextureHolder

public fun createTextureHolder(data: KMDataRef): KMTextureHolderInterface? =
    (TextureHolder(data = data.asPlatform()) as? MCTextureHolderInterfaceProtocol)?.asKmp()

public fun createTextureHolder(bitmapData: NSData, width: Long, height: Long): KMTextureHolderInterface? =
    (TextureHolder(bitmapData = bitmapData, width = width, height = height) as? MCTextureHolderInterfaceProtocol)?.asKmp()
