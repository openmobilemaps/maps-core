package io.openmobilemaps.mapscore.kmp

import MapCoreKmp.MapCoreKmpBridge
import MapCoreSharedModule.MCFontLoaderInterfaceProtocol
import MapCoreSharedModule.MCLoaderInterfaceProtocol
import MapCoreSharedModule.MCTextureHolderInterfaceProtocol
import platform.Foundation.NSBundle
import platform.Foundation.NSData

fun createTextureLoader(): KMLoaderInterface =
    (MapCoreKmpBridge.createTextureLoader() as MCLoaderInterfaceProtocol).asKmp()

fun createFontLoader(bundle: NSBundle, resourcePath: String? = null): KMFontLoaderInterface =
    (MapCoreKmpBridge.createFontLoaderWithBundle(bundle, resourcePath) as MCFontLoaderInterfaceProtocol).asKmp()

fun createTextureHolder(data: NSData): KMTextureHolderInterface? =
    (MapCoreKmpBridge.createTextureHolderWithData(data) as? MCTextureHolderInterfaceProtocol)?.asKmp()
